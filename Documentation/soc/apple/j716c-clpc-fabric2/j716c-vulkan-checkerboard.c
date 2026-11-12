/* Native Apple Vulkan render/readback probe. No software render fallback. */
#include <vulkan/vulkan_core.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "checkerboard-shaders.h"

enum { WIDTH = 1009, HEIGHT = 769, FRAMES = 32, MAX_QUEUE_FAMILIES = 64 };
#define IMAGE_BYTES ((VkDeviceSize)WIDTH * HEIGHT * 4)
#define MEMORY_BUDGET ((VkDeviceSize)64 * 1024 * 1024)
#define CHECK(call) do { if ((call) != VK_SUCCESS) fail(#call); } while (0)
static uint32_t checked_frames;
static uint64_t checked_pixels, mismatched_pixels;

static __attribute__((noreturn)) void fail(const char *step)
{
    fprintf(stderr, "FAIL stage=%s frames=%u pixels=%" PRIu64 " mismatches=%" PRIu64 "\n",
            step, checked_frames, checked_pixels, mismatched_pixels);
    /* A failed/timeout submission must not enter an unbounded idle wait. */
    exit(EXIT_FAILURE);
}

static uint32_t memory_type(const VkPhysicalDeviceMemoryProperties *properties,
                            uint32_t compatible, VkMemoryPropertyFlags required)
{
    for (uint32_t n = 0; n < properties->memoryTypeCount; n++)
        if ((compatible & (1u << n)) &&
            (properties->memoryTypes[n].propertyFlags & required) == required)
            return n;
    fail("compatible-memory-type");
}

static void verify_pixels(const volatile unsigned char *pixels, uint32_t frame)
{
    /* Independent oracle: row y, column x maps to byte4*(1009*y+x).
     * Addition/modulo expresses parity independently of the shader XOR. */
    for (uint32_t y = 0; y < HEIGHT; y++) {
        for (uint32_t x = 0; x < WIDTH; x++) {
            size_t offset = 4 * ((size_t)y * WIDTH + x);
            unsigned char r = pixels[offset], g = pixels[offset + 1];
            unsigned char b = pixels[offset + 2], a = pixels[offset + 3];
            unsigned char expected_r = 255 * ((x + y) % 2);
            unsigned char expected_g = 255 * (((x / 17) + (y / 31)) % 2);
            unsigned char expected_b = 255 * (frame % 2);
            mismatched_pixels += r != expected_r || g != expected_g ||
                                 b != expected_b || a != 255;
            checked_pixels++;
        }
    }
    if (mismatched_pixels)
        fail("exact-RGBA-readback");
    checked_frames++;
}

int main(void)
{
    const char *const overrides[] = {
        "LD_LIBRARY_PATH", "LD_PRELOAD", "LD_AUDIT", "VK_DRIVER_FILES",
        "VK_ICD_FILENAMES", "VK_ADD_DRIVER_FILES", "VK_LOADER_DRIVERS_SELECT",
        "VK_LOADER_DRIVERS_DISABLE", "VK_LAYER_PATH", "VK_ADD_LAYER_PATH",
        "VK_INSTANCE_LAYERS", "VK_LOADER_LAYERS_ENABLE", "VK_LOADER_LAYERS_ALLOW",
        "VK_LOADER_LAYERS_DISABLE", "MESA_VK_DEVICE_SELECT",
        "MESA_VK_DEVICE_SELECT_FORCE_DEFAULT_DEVICE", "DRI_PRIME",
        "MESA_LOADER_DRIVER_OVERRIDE", "GALLIUM_DRIVER",
    };
    for (size_t n = 0; n < sizeof(overrides) / sizeof(overrides[0]); n++) {
        const char *value = getenv(overrides[n]);
        if (value && *value)
            fail("native-loader-environment");
    }
    const VkInstanceCreateInfo instance_info = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    VkInstance instance;
    CHECK(vkCreateInstance(&instance_info, NULL, &instance));
    uint32_t count = 0;
    CHECK(vkEnumeratePhysicalDevices(instance, &count, NULL));
    if (count != 1)
        fail("exactly-one-physical-device");
    VkPhysicalDevice physical;
    CHECK(vkEnumeratePhysicalDevices(instance, &count, &physical));
    if (count != 1)
        fail("stable-physical-device-count");
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physical, &properties);
    if (properties.vendorID != VK_VENDOR_ID_MESA ||
        properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ||
        strncmp(properties.deviceName, "Apple M5 Max", 12) != 0)
        fail("real-Apple-M5-Max-GPU");
    if (properties.limits.maxImageDimension2D < WIDTH ||
        properties.limits.maxImageDimension2D < HEIGHT ||
        properties.limits.maxFramebufferWidth < WIDTH ||
        properties.limits.maxFramebufferHeight < HEIGHT ||
        properties.limits.maxPushConstantsSize < sizeof(uint32_t))
        fail("render-limits");
    VkImageFormatProperties format_properties;
    CHECK(vkGetPhysicalDeviceImageFormatProperties(physical, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        0, &format_properties));
    if (format_properties.maxExtent.width < WIDTH ||
        format_properties.maxExtent.height < HEIGHT ||
        !(format_properties.sampleCounts & VK_SAMPLE_COUNT_1_BIT) ||
        format_properties.maxResourceSize < IMAGE_BYTES)
        fail("RGBA8-image-format-limits");
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, NULL);
    if (!count || count > MAX_QUEUE_FAMILIES)
        fail("bounded-queue-family-count");
    VkQueueFamilyProperties families[MAX_QUEUE_FAMILIES];
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, families);
    if (!count || count > MAX_QUEUE_FAMILIES)
        fail("stable-queue-family-count");
    uint32_t family = 0;
    while (family < count && (!(families[family].queueFlags & VK_QUEUE_GRAPHICS_BIT) ||
                             !families[family].queueCount))
        family++;
    if (family == count)
        fail("graphics-queue");
    const float priority = 1.0f;
    const VkDeviceQueueCreateInfo queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = family, .queueCount = 1, .pQueuePriorities = &priority,
    };
    const VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1, .pQueueCreateInfos = &queue_info,
    };
    VkDevice device;
    CHECK(vkCreateDevice(physical, &device_info, NULL, &device));
    VkQueue queue;
    vkGetDeviceQueue(device, family, 0, &queue);
    const VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = IMAGE_BYTES,
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT, .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VkBuffer buffer;
    CHECK(vkCreateBuffer(device, &buffer_info, NULL, &buffer));
    const VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM, .extent = {WIDTH, HEIGHT, 1},
        .mipLevels = 1, .arrayLayers = 1, .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE, .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VkImage image;
    CHECK(vkCreateImage(device, &image_info, NULL, &image));
    VkMemoryRequirements buffer_requirements, image_requirements;
    vkGetBufferMemoryRequirements(device, buffer, &buffer_requirements);
    vkGetImageMemoryRequirements(device, image, &image_requirements);
    if (buffer_requirements.size < IMAGE_BYTES || image_requirements.size == 0 ||
        buffer_requirements.size >= MEMORY_BUDGET || image_requirements.size >= MEMORY_BUDGET ||
        buffer_requirements.size >= MEMORY_BUDGET - image_requirements.size)
        fail("padded-allocation-budget");
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical, &memory_properties);
    if (!memory_properties.memoryTypeCount || memory_properties.memoryTypeCount > VK_MAX_MEMORY_TYPES)
        fail("memory-type-count");
    const VkMemoryAllocateInfo buffer_allocation = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .allocationSize = buffer_requirements.size,
        .memoryTypeIndex = memory_type(&memory_properties, buffer_requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    };
    const VkMemoryAllocateInfo image_allocation = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .allocationSize = image_requirements.size,
        .memoryTypeIndex = memory_type(&memory_properties, image_requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };
    VkDeviceMemory buffer_memory, image_memory;
    CHECK(vkAllocateMemory(device, &buffer_allocation, NULL, &buffer_memory));
    CHECK(vkAllocateMemory(device, &image_allocation, NULL, &image_memory));
    CHECK(vkBindBufferMemory(device, buffer, buffer_memory, 0));
    CHECK(vkBindImageMemory(device, image, image_memory, 0));
    void *mapped;
    CHECK(vkMapMemory(device, buffer_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
    const VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D, .format = image_info.format,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
    };
    VkImageView view;
    CHECK(vkCreateImageView(device, &view_info, NULL, &view));
    const VkAttachmentDescription attachment = {
        .format = image_info.format, .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const VkAttachmentReference color = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    const VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1, .pColorAttachments = &color,
    };
    const VkRenderPassCreateInfo pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1, .pAttachments = &attachment,
        .subpassCount = 1, .pSubpasses = &subpass,
    };
    VkRenderPass render_pass;
    CHECK(vkCreateRenderPass(device, &pass_info, NULL, &render_pass));
    const VkFramebufferCreateInfo framebuffer_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, .renderPass = render_pass,
        .attachmentCount = 1, .pAttachments = &view, .width = WIDTH, .height = HEIGHT, .layers = 1,
    };
    VkFramebuffer framebuffer;
    CHECK(vkCreateFramebuffer(device, &framebuffer_info, NULL, &framebuffer));
    const VkShaderModuleCreateInfo vertex_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = sizeof(vertex_spirv), .pCode = vertex_spirv,
    };
    const VkShaderModuleCreateInfo fragment_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = sizeof(fragment_spirv), .pCode = fragment_spirv,
    };
    VkShaderModule vertex, fragment;
    CHECK(vkCreateShaderModule(device, &vertex_info, NULL, &vertex));
    CHECK(vkCreateShaderModule(device, &fragment_info, NULL, &fragment));
    const VkPipelineShaderStageCreateInfo shaders[] = {
        { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = vertex, .pName = "main" },
        { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = fragment, .pName = "main" },
    };
    const VkPipelineVertexInputStateCreateInfo vertex_input = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };
    const VkPipelineInputAssemblyStateCreateInfo assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };
    const VkViewport viewport = {0, 0, WIDTH, HEIGHT, 0, 1};
    const VkRect2D area = { {0, 0}, {WIDTH, HEIGHT} };
    const VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1, .pViewports = &viewport, .scissorCount = 1, .pScissors = &area,
    };
    const VkPipelineRasterizationStateCreateInfo raster = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL, .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE, .lineWidth = 1,
    };
    const VkPipelineMultisampleStateCreateInfo multisample = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };
    const VkPipelineColorBlendAttachmentState blend_attachment = { .colorWriteMask = 0xf };
    const VkPipelineColorBlendStateCreateInfo blend = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1, .pAttachments = &blend_attachment,
    };
    const VkPushConstantRange push_range = {
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT, .offset = 0, .size = sizeof(uint32_t),
    };
    const VkPipelineLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pushConstantRangeCount = 1, .pPushConstantRanges = &push_range,
    };
    VkPipelineLayout layout;
    CHECK(vkCreatePipelineLayout(device, &layout_info, NULL, &layout));
    const VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2, .pStages = shaders, .pVertexInputState = &vertex_input,
        .pInputAssemblyState = &assembly, .pViewportState = &viewport_state,
        .pRasterizationState = &raster, .pMultisampleState = &multisample,
        .pColorBlendState = &blend, .layout = layout, .renderPass = render_pass,
    };
    VkPipeline pipeline;
    CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &pipeline));
    const VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .queueFamilyIndex = family,
    };
    VkCommandPool pool;
    CHECK(vkCreateCommandPool(device, &pool_info, NULL, &pool));
    const VkCommandBufferAllocateInfo command_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1,
    };
    VkCommandBuffer command;
    CHECK(vkAllocateCommandBuffers(device, &command_info, &command));
    const VkCommandBufferBeginInfo begin = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    const VkFenceCreateInfo fence_info = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    VkFence fence;
    CHECK(vkCreateFence(device, &fence_info, NULL, &fence));
    const VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &command,
    };
    /* Clear is deliberately outside valid output (alpha0, non-extreme RGB). */
    const VkClearValue clear = { .color = { .float32 = {0.25f, 0.5f, 0.75f, 0} } };
    const VkRenderPassBeginInfo pass_begin = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = render_pass, .framebuffer = framebuffer, .renderArea = area,
        .clearValueCount = 1, .pClearValues = &clear,
    };
    const VkBufferImageCopy region = {
        .bufferOffset = 0, .bufferRowLength = 0, .bufferImageHeight = 0,
        .imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        .imageOffset = {0, 0, 0}, .imageExtent = {WIDTH, HEIGHT, 1},
    };
    const VkBufferMemoryBarrier before_copy = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT, .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = buffer, .offset = 0, .size = VK_WHOLE_SIZE,
    };
    const VkBufferMemoryBarrier after_copy = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT, .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = buffer, .offset = 0, .size = VK_WHOLE_SIZE,
    };
    const VkImageMemoryBarrier after_render = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image, .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };
    for (uint32_t frame = 0; frame < FRAMES; frame++) {
        /* Prior submission has completed before touching/reusing host memory. */
        memset(mapped, 0xa5, (size_t)buffer_requirements.size);
        CHECK(vkResetCommandPool(device, pool, 0));
        CHECK(vkBeginCommandBuffer(command, &begin));
        vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, NULL, 1, &before_copy, 0, NULL);
        const VkImageMemoryBarrier before_render = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = frame ? VK_ACCESS_TRANSFER_READ_BIT : 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = frame ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image, .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
        };
        vkCmdPipelineBarrier(command, frame ? VK_PIPELINE_STAGE_TRANSFER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &before_render);
        vkCmdBeginRenderPass(command, &pass_begin, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdPushConstants(command, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(frame), &frame);
        vkCmdDraw(command, 3, 1, 0, 0);
        vkCmdEndRenderPass(command);
        vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, NULL, 0, NULL, 1, &after_render);
        vkCmdCopyImageToBuffer(command, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);
        vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                             0, 0, NULL, 1, &after_copy, 0, NULL);
        CHECK(vkEndCommandBuffer(command));
        CHECK(vkResetFences(device, 1, &fence));
        CHECK(vkQueueSubmit(queue, 1, &submit, fence));
        CHECK(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_C(3000000000)));
        verify_pixels(mapped, frame);
    }
    vkUnmapMemory(device, buffer_memory);
    vkDestroyFence(device, fence, NULL);
    vkDestroyCommandPool(device, pool, NULL);
    vkDestroyPipeline(device, pipeline, NULL);
    vkDestroyPipelineLayout(device, layout, NULL);
    vkDestroyShaderModule(device, fragment, NULL);
    vkDestroyShaderModule(device, vertex, NULL);
    vkDestroyFramebuffer(device, framebuffer, NULL);
    vkDestroyRenderPass(device, render_pass, NULL);
    vkDestroyImageView(device, view, NULL);
    vkDestroyImage(device, image, NULL);
    vkDestroyBuffer(device, buffer, NULL);
    vkFreeMemory(device, image_memory, NULL);
    vkFreeMemory(device, buffer_memory, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroyInstance(instance, NULL);
    printf("PASS frames=%u pixels=%" PRIu64 " mismatches=%" PRIu64 "\n",
           checked_frames, checked_pixels, mismatched_pixels);
    return EXIT_SUCCESS;
}
