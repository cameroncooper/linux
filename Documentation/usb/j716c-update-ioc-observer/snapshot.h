/* SPDX-License-Identifier: GPL-2.0-only */
/* One passive sample per module; no exit/unload or live sysfs MMIO callback. */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/io.h>
#include <linux/cpu.h>
#include <linux/ktime.h>
#include <linux/soc/apple/j716c-usb2.h>

#define PAIR_ARRAY(name, n) \
 static unsigned int name[n]; \
 static unsigned int name##_count = n; \
 module_param_array(name, uint, &name##_count, 0444)

PAIR_ARRAY(params3, 2);
PAIR_ARRAY(params4, 2);
PAIR_ARRAY(fault_valid, 2);
PAIR_ARRAY(error_legacy, 2);
PAIR_ARRAY(error_streams_legacy, 16);
PAIR_ARRAY(error_mask_legacy, 2);
PAIR_ARRAY(exceptions, 16);
PAIR_ARRAY(error_mask_v2, 2);
PAIR_ARRAY(irq_status, 2);
PAIR_ARRAY(irq_enable, 2);
PAIR_ARRAY(sid_enable, 16);
static unsigned long long begin_ns, end_ns;
module_param(begin_ns, ullong, 0444);
module_param(end_ns, ullong, 0444);
static bool active_before, active_after;
module_param(active_before, bool, 0444);
module_param(active_after, bool, 0444);

static bool same_irq(struct device_node *a, struct device_node *b)
{
 struct of_phandle_args x, y;
 bool equal;
 if (of_irq_parse_one(a, 0, &x))
  return false;
 if (of_irq_parse_one(b, 0, &y)) {
  of_node_put(x.np);
  return false;
 }
 equal = x.np == y.np && x.args_count == 4 && y.args_count == 4 &&
  x.args[0] == 0 && x.args[1] == 0 && x.args[2] == 1423 &&
  x.args[3] == IRQ_TYPE_LEVEL_HIGH &&
  !memcmp(x.args, y.args, sizeof(u32) * 4);
 of_node_put(x.np);
 of_node_put(y.np);
 return equal;
}

static void sample(void __iomem *base, unsigned int bank)
{
 unsigned int version, streams, words, i, status, enable, sid;
 params3[bank] = readl(base + 0x08);
 params4[bank] = readl(base + 0x0c);
 version = params3[bank] & 0xffff;
 streams = params4[bank] & 0x1ff;
 if (version < 0x0200 || version > 0x0204 ||
     !streams || streams > 256 || (bank == 1 && streams != 16))
  return;
 words = DIV_ROUND_UP(streams, 32);
 /* Legacy diagnostics remain passive W1C registers, also on v2 hardware. */
 error_legacy[bank] = readl(base + 0x100);
 error_mask_legacy[bank] = readl(base + 0x104);
 for (i = 0; i < words; i++)
  error_streams_legacy[8 * bank + i] = readl(base + 0x1c0 + 4 * i);
 if (version >= 0x0202) {
  error_mask_v2[bank] = readl(base + 0x4060);
  for (i = 0; i < words; i++)
   exceptions[8 * bank + i] = readl(base + 0x4000 + 4 * i);
  status = version == 0x0204 ? 0x4204 : 0x704;
  enable = version == 0x0204 ? 0x4200 : 0x700;
  sid = version == 0x0204 ? 0x4220 : 0x720;
  if (version >= 0x0203)
   irq_status[bank] = readl(base + status);
  irq_enable[bank] = readl(base + enable);
  for (i = 0; i < words; i++)
   sid_enable[8 * bank + i] = readl(base + sid + 4 * i);
 }
 fault_valid[bank] = 1;
}

static int __init observer_init(void)
{
 struct device_node *chosen __free(device_node) = of_find_node_by_path("/chosen");
 struct device_node *peer __free(device_node) =
  of_find_node_by_path(J716C_USB0_SOC "iommu@2a2f00000");
 struct device_node *owned __free(device_node) = of_find_node_by_path(J716C_USB0_DART);
 struct platform_device *pdev;
 const char *marker, *observe;
 void __iomem *base[2] = { NULL, NULL };
 int ret;
 if (!apple_j716c_usb2_ready() || num_online_cpus() != 18 || !chosen ||
     of_property_count_strings(chosen, "j716c,ram-test") != 1 ||
     of_property_read_string(chosen, "j716c,ram-test", &marker) ||
     (strcmp(marker, "usb2-bulk-control") && strcmp(marker, "usb2-bulk-events") &&
      strcmp(marker, "usb2-bulk-update-ioc")) ||
     of_property_count_strings(chosen, "j716c,dart-fault-observe") != 1 ||
     of_property_read_string(chosen, "j716c,dart-fault-observe", &observe) ||
     strcmp(observe, "read-only") || !peer || !owned ||
     !apple_j716c_usb2_resource(peer, 0, 0x2a2f00000ULL, 0xc000) ||
     !same_irq(peer, owned) ||
     !apple_j716c_usb2_power(peer, J716C_USB0_SOC
       "power-management@288300000/power-controller@150", NULL) ||
     of_property_present(peer, "clocks") || of_property_present(owned, "clocks"))
  return -EPERM;
 pdev = of_find_device_by_node(owned);
 if (!pdev)
  return -ENODEV;
 if (!device_is_bound(&pdev->dev) || !pdev->dev.driver ||
     strcmp(pdev->dev.driver->name, "apple-dart")) {
  ret = -ENODEV;
  goto put_device;
 }
 /* This call never resumes a suspended bank or its suppliers. */
 ret = pm_runtime_get_if_active(&pdev->dev);
 if (ret <= 0) {
  ret = -EAGAIN;
  goto put_device;
 }
 active_before = pm_runtime_active(&pdev->dev);
 if (!active_before) {
  ret = -EAGAIN;
  goto put_pm;
 }
 base[0] = ioremap(0x2a2f00000ULL, 0xc000);
 base[1] = ioremap(0x2a2f80000ULL, 0xc000);
 if (!base[0] || !base[1]) {
  ret = -ENOMEM;
  goto unmap;
 }
 begin_ns = ktime_get_boottime_ns();
 sample(base[0], 0);
 sample(base[1], 1);
 end_ns = ktime_get_boottime_ns();
 active_after = pm_runtime_active(&pdev->dev);
 ret = active_after ? 0 : -EAGAIN;
unmap:
 if (base[1])
  iounmap(base[1]);
 if (base[0])
  iounmap(base[0]);
put_pm:
 pm_runtime_put_noidle(&pdev->dev);
put_device:
 put_device(&pdev->dev);
 return ret;
}
module_init(observer_init);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Exact J716c RAM-only cached USB0 shared DART fault observer");
