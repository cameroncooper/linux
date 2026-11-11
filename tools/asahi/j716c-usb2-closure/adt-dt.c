/* SPDX-License-Identifier: GPL-2.0-only */
/* Actual ADT PMGR rows + public Aura struct/accessors + compiled kernel DT. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libfdt.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
#define PACKED __attribute__((packed))
#define PMGR_FLAG_VIRTUAL 0x10
static bool pmgr_u8id;
/* ACTUAL_PMGR */
static struct pmgr_device *devices;
static size_t count;
static struct pmgr_device *find(u16 id)
{
	for (size_t i = 0; i < count; i++)
		if (pmgr_adt_get_id(&devices[i]) == id)
			return &devices[i];
	assert(!"missing ADT PMGR device");
	return NULL;
}
static unsigned int flatten(u16 id, u16 *out, unsigned int n,
			    unsigned int depth)
{
	assert(depth < 32);
	if (!id)
		return n;
	struct pmgr_device *d = find(id);
	if (!(d->flags & PMGR_FLAG_VIRTUAL)) {
		assert(n < 2);
		out[n++] = id;
		return n;
	}
	u16 parents[2];
	pmgr_adt_get_parents(d, parents);
	n = flatten(parents[0], out, n, depth + 1);
	return flatten(parents[1], out, n, depth + 1);
}
static unsigned int realparents(u16 id, u16 *out)
{
	u16 parents[2];
	pmgr_adt_get_parents(find(id), parents);
	unsigned int n = flatten(parents[0], out, 0, 0);
	return flatten(parents[1], out, n, 0);
}
static void *readfile(const char *name, long *size)
{
	FILE *f = fopen(name, "rb");
	assert(f);
	assert(!fseek(f, 0, SEEK_END));
	*size = ftell(f);
	rewind(f);
	void *p = malloc(*size);
	assert(p);
	assert(fread(p, 1, *size, f) == (size_t)*size);
	fclose(f);
	return p;
}
int main(int argc, char **argv)
{
	assert(argc == 4);
	long size;
	devices = readfile(argv[1], &size);
	assert(sizeof(*devices) == 48 && !(size % 48));
	count = size / 48;
	pmgr_u8id = devices[0].id1 != devices[1].id1;
	assert(!pmgr_u8id);
	assert(!strcmp(find(50)->name, "FAB3_SOC") && (find(50)->flags & 0x10));
	assert(!strcmp(find(49)->name, "FAB0_PIOGW") &&
	       (find(49)->flags & 0x10));
	u16 p[2];
	pmgr_adt_get_parents(find(50), p);
	assert(p[0] == 49 && !p[1]);
	pmgr_adt_get_parents(find(49), p);
	assert(!p[0] && !p[1]);
	pmgr_adt_get_parents(find(55), p);
	assert(p[0] == 50 && !p[1]);
	unsigned int ids[] = { 55, 260, 261, 262 };
	const char *names[] = { "ATC0_COMMON", "ATC0_USB_AON", "ATC0_USB",
				"ATC0_PHYMXWRAP" };
	unsigned int offsets[] = { 0x208, 0x148, 0x150, 0x158 };
	unsigned int groups[] = { 0, 2, 2, 2 };
	for (unsigned int i = 0; i < 4; i++) {
		struct pmgr_device *d = find(ids[i]);
		assert(!(d->flags & 0x10) && !strcmp(d->name, names[i]));
		assert(d->group_and_offset.offset == offsets[i] &&
		       d->group_and_offset.group == groups[i]);
		unsigned int n = realparents(ids[i], p);
		assert(n == (i < 2 ? 0 : 2));
		if (n)
			assert(p[0] == 55 && p[1] == 260);
	}
	void *dt = readfile(argv[2], &size);
	assert(!fdt_check_header(dt));
	bool active = !!atoi(argv[3]);
	const char *paths[] = {
		"/soc@200000000/power-management@280600000/power-controller@208",
		"/soc@200000000/power-management@288300000/power-controller@148",
		"/soc@200000000/power-management@288300000/power-controller@150",
		"/soc@200000000/power-management@288300000/power-controller@158"
	};
	int nodes[4];
	for (unsigned int i = 0; i < 4; i++) {
		nodes[i] = fdt_path_offset(dt, paths[i]);
		assert(nodes[i] >= 0);
		int len;
		const fdt32_t *reg = fdt_getprop(dt, nodes[i], "reg", &len);
		assert(reg && len == 8 && fdt32_to_cpu(reg[0]) == offsets[i] &&
		       fdt32_to_cpu(reg[1]) == 4);
		const char *status = fdt_getprop(dt, nodes[i], "status", NULL);
		assert(status && !strcmp(status, active ? "okay" : "disabled"));
		const fdt32_t *pd =
			fdt_getprop(dt, nodes[i], "power-domains", &len);
		if (i < 2)
			assert(!pd);
		else
			assert(pd && len == 8 &&
			       fdt32_to_cpu(pd[0]) ==
				       fdt_get_phandle(dt, nodes[0]) &&
			       fdt32_to_cpu(pd[1]) ==
				       fdt_get_phandle(dt, nodes[1]));
	}
	const char *virtuals[] = {
		"/soc@200000000/power-management@280600000/power-controller@1e0",
		"/soc@200000000/power-management@280600000/power-controller@1d8"
	};
	for (unsigned int i = 0; i < 2; i++) {
		int off = fdt_path_offset(dt, virtuals[i]);
		assert(off >= 0);
		const char *s = fdt_getprop(dt, off, "status", NULL);
		assert(s && !strcmp(s, "disabled"));
	}
	puts("PASS: actual ADT no_ps FAB50->FAB49->none; real COMMON/AON no parent; USB/WRAP parents55+260; compiled DT matches and virtual inventory stays disabled");
	free(dt);
	free(devices);
}
