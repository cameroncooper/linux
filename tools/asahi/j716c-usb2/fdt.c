/* SPDX-License-Identifier: GPL-2.0-only */
/* Actual shared guard over a real flattened device tree, no MMIO. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libfdt.h>
#define __free(x)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
typedef uint32_t u32;
typedef uint64_t resource_size_t;
struct resource {
	resource_size_t start, size;
};
static resource_size_t resource_size(struct resource *r)
{
	return r->size;
}
struct device_node {
	int off;
};
struct of_phandle_args {
	struct device_node *np;
	int args_count;
	u32 args[8];
};
static void *dt;
static struct device_node nodes[256];
static int node_count;
static struct device_node *node(int off)
{
	if (off < 0)
		return NULL;
	for (int i = 0; i < node_count; i++)
		if (nodes[i].off == off)
			return &nodes[i];
	assert(node_count < 256);
	nodes[node_count].off = off;
	return &nodes[node_count++];
}
static bool of_machine_is_compatible(const char *s)
{
	return !fdt_node_check_compatible(dt, 0, s);
}
static struct device_node *of_find_node_by_path(const char *p)
{
	return node(fdt_path_offset(dt, p));
}
static void of_node_put(struct device_node *n)
{
	(void)n;
}
static bool of_device_is_available(struct device_node *n)
{
	if (!n)
		return false;
	const char *s = fdt_getprop(dt, n->off, "status", NULL);
	return !s || !strcmp(s, "okay") || !strcmp(s, "ok");
}
static bool of_device_is_compatible(struct device_node *n, const char *s)
{
	return n && !fdt_node_check_compatible(dt, n->off, s);
}
static int of_property_count_strings(struct device_node *n, const char *s)
{
	return n ? fdt_stringlist_count(dt, n->off, s) : -1;
}
static bool of_property_present(struct device_node *n, const char *s)
{
	return n && fdt_getprop(dt, n->off, s, NULL);
}
static int of_property_count_u32_elems(struct device_node *n, const char *s)
{
	int len;
	const void *p = n ? fdt_getprop(dt, n->off, s, &len) : NULL;
	return p && !(len % 4) ? len / 4 : -1;
}
static int of_property_read_u32_array(struct device_node *n, const char *s,
				      u32 *v, int count)
{
	int len;
	const fdt32_t *p = n ? fdt_getprop(dt, n->off, s, &len) : NULL;
	if (!p || len < count * 4)
		return -1;
	for (int i = 0; i < count; i++)
		v[i] = fdt32_to_cpu(p[i]);
	return 0;
}
static int of_property_read_u32(struct device_node *n, const char *s, u32 *v)
{
	return of_property_read_u32_array(n, s, v, 1);
}
static int of_property_read_string(struct device_node *n, const char *s,
				   const char **v)
{
	int len;
	*v = n ? fdt_getprop(dt, n->off, s, &len) : NULL;
	return *v && len > 0 && memchr(*v, 0, len) ? 0 : -1;
}
static int of_parse_phandle_with_args(struct device_node *n, const char *p,
				      const char *cells, int index,
				      struct of_phandle_args *args)
{
	int len, pos = 0;
	const fdt32_t *values = n ? fdt_getprop(dt, n->off, p, &len) : NULL;
	if (!values)
		return -1;
	for (int i = 0; i <= index; i++) {
		u32 count;
		if (pos * 4 >= len)
			return -1;
		args->np = node(fdt_node_offset_by_phandle(
			dt, fdt32_to_cpu(values[pos++])));
		if (of_property_read_u32(args->np, cells, &count) ||
		    count > 8 || (pos + count) * 4 > (unsigned int)len)
			return -1;
		args->args_count = count;
		for (u32 j = 0; j < count; j++)
			args->args[j] = fdt32_to_cpu(values[pos++]);
	}
	return 0;
}
static int of_address_to_resource(struct device_node *n, int index,
				  struct resource *r)
{
	int len;
	const fdt32_t *p = n ? fdt_getprop(dt, n->off, "reg", &len) : NULL;
	if (!p || len < (index + 1) * 16)
		return -1;
	p += index * 4;
	r->start = ((uint64_t)fdt32_to_cpu(p[0]) << 32) | fdt32_to_cpu(p[1]);
	r->size = ((uint64_t)fdt32_to_cpu(p[2]) << 32) | fdt32_to_cpu(p[3]);
	return 0;
}
/* ACTUAL_HEADER */
int main(int argc, char **argv)
{
	assert(argc == 3);
	FILE *f = fopen(argv[1], "rb");
	assert(f);
	assert(!fseek(f, 0, SEEK_END));
	long size = ftell(f);
	assert(size > 0);
	rewind(f);
	dt = malloc(size);
	assert(dt);
	assert(fread(dt, 1, size, f) == (size_t)size);
	fclose(f);
	assert(!fdt_check_header(dt));
	bool admitted = apple_j716c_usb2_ready();
	assert(admitted == !!atoi(argv[2]));
	printf("PASS: actual kernel profile guard %s real DT %s\n",
	       admitted ? "admits" : "refuses", argv[1]);
	free(dt);
}
