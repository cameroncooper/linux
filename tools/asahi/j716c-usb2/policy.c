/* SPDX-License-Identifier: GPL-2.0-only */
/* Actual admission header, with only OF/resource calls modeled. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
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
	int id;
};
struct of_phandle_args {
	struct device_node *np;
	int args_count;
	u32 args[1];
};
static struct device_node nodes[11] = { { 0 }, { 1 }, { 2 }, { 3 },
					{ 4 }, { 5 }, { 6 }, { 7 },
					{ 8 }, { 9 }, { 10 } };
static const char *paths[] = {
	"/chosen",
	"/soc@200000000/phy@2a2a90000",
	"/soc@200000000/usb@2a2280000",
	"/soc@200000000/iommu@2a2f80000",
	"/soc@200000000/iommu@2a2f00000",
	"/soc@200000000/power-management@280600000/power-controller@208",
	"/soc@200000000/power-management@288300000/power-controller@148",
	"/soc@200000000/power-management@288300000/power-controller@150",
	"/soc@200000000/power-management@288300000/power-controller@158",
	"/soc@200000000/power-management@280600000/power-controller@1e0",
	"/soc@200000000/power-management@280600000/power-controller@1d8",
};
static bool present[11], available[11], board, chip, changed_compatible,
	tunable, usb2_tunable, pd_fault, common_parent;
static int resource_fault, count_fault, sid, phandle_fault;
static u32 request, contract;
static bool contract_missing;
static const char *marker, *mode, *speed;
static void reset(void)
{
	for (int i = 0; i < 11; i++)
		present[i] = available[i] = true;
	available[4] = available[9] = available[10] = false;
	board = chip = true;
	changed_compatible = tunable = usb2_tunable = pd_fault = common_parent =
		false;
	resource_fault = count_fault = phandle_fault = 0;
	sid = 1;
	request = 1;
	contract = 2;
	contract_missing = false;
	marker = "retired";
	mode = "peripheral";
	speed = "high-speed";
}
static bool of_machine_is_compatible(const char *s)
{
	return !strcmp(s, "apple,j716c") ? board : chip;
}
static struct device_node *of_find_node_by_path(const char *p)
{
	for (int i = 0; i < 11; i++)
		if (!strcmp(paths[i], p))
			return present[i] ? &nodes[i] : NULL;
	return NULL;
}
static void of_node_put(struct device_node *n)
{
	(void)n;
}
static bool of_device_is_available(struct device_node *n)
{
	return n && available[n->id];
}
static bool of_device_is_compatible(struct device_node *n, const char *s)
{
	if (!n || changed_compatible)
		return false;
	if (n->id == 1)
		return !strcmp(s, "apple,j716c-usb2-device-phy");
	if (n->id == 2)
		return !strcmp(s, "apple,j716c-usb2-device-dwc3");
	return n->id == 3 && (!strcmp(s, "apple,j716c-usb2-device-dart") ||
			      !strcmp(s, "apple,t8110-dart"));
}
static int of_property_count_strings(struct device_node *n, const char *s)
{
	(void)s;
	return n->id == 3 ? 2 : 1;
}
static bool of_property_present(struct device_node *n, const char *s)
{
	if (!strcmp(s, "power-domains") && n->id == 5)
		return common_parent;
	return n->id == 6 || n->id == 10 ? false : tunable;
}
static int of_property_count_u32_elems(struct device_node *n, const char *s)
{
	if (!strcmp(s, "power-domains")) {
		if ((n->id == 5 && !common_parent) || n->id == 6 || n->id == 10)
			return -1;
		return n->id == 1 || n->id == 7 || n->id == 8 ? 2 : 1;
	}
	if (!strcmp(s, "apple,tunable-usb2-default"))
		return 6;
	if (!strcmp(s, "apple,tunable-usb2-device"))
		return 3;
	if (!strcmp(s, "j716c,usb0-retirement-contract"))
		return contract_missing ? -1 : count_fault == 5 ? 2 : 1;
	if (!strcmp(s, "j716c,usb0-handoff"))
		return count_fault == 1 ? 2 : 1;
	if (!strcmp(s, "reg"))
		return count_fault == 2 ? 3 : (n->id == 1 ? 12 : 4);
	if (!strcmp(s, "iommus"))
		return count_fault == 3 ? 4 : 2;
	return count_fault == 4 ? 2 : 1;
}
static int of_property_read_u32_array(struct device_node *n, const char *s,
				      u32 *v, int count)
{
	static const u32 expected[] = { 8, 0x7003, 3, 0x18, 0x703, 0x103 };
	(void)n;
	(void)s;
	memcpy(v, expected, count * sizeof(u32));
	if (usb2_tunable)
		v[0] += 4;
	return 0;
}
static int of_property_read_u32(struct device_node *n, const char *s, u32 *v)
{
	(void)n;
	*v = !strcmp(s, "j716c,usb0-retirement-contract") ? contract : request;
	return 0;
}
static int of_property_read_string(struct device_node *n, const char *s,
				   const char **v)
{
	(void)n;
	if (!strcmp(s, "j716c,usb0-handoff-status")) {
		*v = marker;
		return marker ? 0 : -1;
	}
	*v = !strcmp(s, "dr_mode") ? mode : speed;
	return 0;
}
static int of_parse_phandle_with_args(struct device_node *n, const char *p,
				      const char *cells, int index,
				      struct of_phandle_args *args)
{
	(void)cells;
	if (!strcmp(p, "power-domains")) {
		int id = n->id == 1		  ? (index == 0 ? 8 : 7) :
			 n->id == 2 || n->id == 3 ? 7 :
			 n->id == 5		  ? 9 :
			 n->id == 9		  ? 10 :
			 index == 0		  ? 5 :
						    6;
		args->np = &nodes[pd_fault ? 4 : id];
		args->args_count = 0;
		return 0;
	}
	args->np = &nodes[!strcmp(p, "iommus") ? 3 : 1];
	if (phandle_fault)
		args->np = &nodes[4];
	args->args_count = !strcmp(p, "iommus") ? 1 : 0;
	args->args[0] = sid;
	return 0;
}
static int of_address_to_resource(struct device_node *n, int index,
				  struct resource *r)
{
	static const uint64_t phy[] = { 0x2a2a90000ULL, 0x2a2800000ULL,
					0x2a2a84000ULL };
	if (!n)
		return -1;
	r->start = n->id == 1 ? phy[index] :
		   n->id == 2 ? 0x2a2280000ULL :
				0x2a2f80000ULL;
	r->size = n->id == 1 ? 0x4000 : n->id == 2 ? 0xcd00 : 0xc000;
	if (resource_fault == n->id * 4 + index)
		r->start += 4;
	return 0;
}
/* ACTUAL_HEADER */
int main(void)
{
	unsigned int cases = 0;
	reset(); request = 2;
	assert(!strcmp(apple_j716c_usb2_refusal(), "request-value"));
	reset(); contract = 1;
	assert(!strcmp(apple_j716c_usb2_refusal(), "contract-value"));
	reset(); board = false;
	assert(!strcmp(apple_j716c_usb2_refusal(), "board-compatible"));
	reset(); available[9] = true;
	assert(!strcmp(apple_j716c_usb2_refusal(), "virtual-fab3-disabled"));

	reset();
	assert(apple_j716c_usb2_ready());
	cases++;
	for (int i = 0; i < 11; i++) {
		reset();
		present[i] = false;
		assert(!apple_j716c_usb2_ready());
		cases++;
	}
	for (int i = 1; i < 11; i++) {
		reset();
		available[i] = !available[i];
		assert(!apple_j716c_usb2_ready());
		cases++;
	}
	reset();
	board = false;
	assert(!apple_j716c_usb2_ready());
	cases++;
	reset();
	chip = false;
	assert(!apple_j716c_usb2_ready());
	cases++;
	for (u32 r = 0; r < 5; r++) {
		reset();
		request = r;
		if (request == 1)
			continue;
		assert(!apple_j716c_usb2_ready());
		cases++;
	}
	reset(); contract_missing = true; assert(!apple_j716c_usb2_ready()); cases++;
	reset(); count_fault = 5; assert(!apple_j716c_usb2_ready()); cases++;
	reset(); contract = 1; assert(!apple_j716c_usb2_ready()); cases++;
	reset(); contract = 0; assert(!apple_j716c_usb2_ready()); cases++;
	reset();
	marker = NULL;
	assert(!apple_j716c_usb2_ready());
	cases++;
	reset();
	marker = "validated";
	assert(!apple_j716c_usb2_ready());
	cases++;
	reset();
	mode = "host";
	assert(!apple_j716c_usb2_ready());
	cases++;
	reset();
	speed = "super-speed";
	assert(!apple_j716c_usb2_ready());
	cases++;
	reset();
	changed_compatible = true;
	assert(!apple_j716c_usb2_ready());
	cases++;
	reset();
	tunable = true;
	assert(!apple_j716c_usb2_ready());
	cases++;
	for (int i = 1; i <= 3; i++)
		for (int j = 0; j < (i == 1 ? 3 : 1); j++) {
			reset();
			resource_fault = i * 4 + j;
			assert(!apple_j716c_usb2_ready());
			cases++;
		}
	for (int i = 1; i <= 4; i++) {
		reset();
		count_fault = i;
		assert(!apple_j716c_usb2_ready());
		cases++;
	}
	reset();
	common_parent = true;
	assert(!apple_j716c_usb2_ready());
	cases++;
	reset();
	pd_fault = true;
	assert(!apple_j716c_usb2_ready());
	cases++;
	reset();
	usb2_tunable = true;
	assert(!apple_j716c_usb2_ready());
	cases++;
	reset();
	sid = 0;
	assert(!apple_j716c_usb2_ready());
	cases++;
	reset();
	phandle_fault = 1;
	assert(!apple_j716c_usb2_ready());
	cases++;
	printf("PASS: %u actual profile admission cases, OF boundary modeled\n",
	       cases);
}
