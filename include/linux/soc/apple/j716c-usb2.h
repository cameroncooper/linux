/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _LINUX_SOC_APPLE_J716C_USB2_H
#define _LINUX_SOC_APPLE_J716C_USB2_H
#include <linux/of.h>
#include <linux/of_address.h>

#define J716C_USB0_SOC "/soc@200000000/"
#define J716C_USB0_PHY J716C_USB0_SOC "phy@2a2a90000"
#define J716C_USB0_DWC J716C_USB0_SOC "usb@2a2280000"
#define J716C_USB0_DART J716C_USB0_SOC "iommu@2a2f80000"

static inline bool apple_j716c_usb2_resource(struct device_node *np, int index,
					     resource_size_t start,
					     resource_size_t size)
{
	struct resource r;
	return !of_address_to_resource(np, index, &r) && r.start == start &&
	       resource_size(&r) == size;
}

/* Every enabled member independently requires the late loader proof, the exact
 * group and the peripheral wiring. Validation-only request 2 never owns MMIO.
 */
static inline bool apple_j716c_usb2_tunables(struct device_node *np)
{
	static const u32 expected[] = { 8, 0x7003, 3, 0x18, 0x703, 0x103 };
	u32 defaults[6], device[3];
	return of_property_count_u32_elems(np, "apple,tunable-usb2-default") ==
		       6 &&
	       of_property_count_u32_elems(np, "apple,tunable-usb2-device") ==
		       3 &&
	       !of_property_read_u32_array(np, "apple,tunable-usb2-default",
					   defaults, 6) &&
	       !of_property_read_u32_array(np, "apple,tunable-usb2-device",
					   device, 3) &&
	       !memcmp(defaults, expected, sizeof(defaults)) &&
	       !memcmp(device, expected, sizeof(device));
}

static inline bool apple_j716c_usb2_power(struct device_node *np,
					  const char *parent0,
					  const char *parent1)
{
	const char *parents[] = { parent0, parent1 };
	int count = parent1 ? 2 : parent0 ? 1 : 0;
	if (!count)
		return !of_property_present(np, "power-domains");
	if (of_property_count_u32_elems(np, "power-domains") != count)
		return false;
	for (int i = 0; i < count; i++) {
		struct device_node *expected __free(device_node) =
			of_find_node_by_path(parents[i]);
		struct of_phandle_args args;
		bool match;
		if (!expected ||
		    of_parse_phandle_with_args(np, "power-domains",
					       "#power-domain-cells", i, &args))
			return false;
		match = args.np == expected && args.args_count == 0;
		of_node_put(args.np);
		if (!match)
			return false;
	}
	return true;
}

/* Modern unflattened OF full_name is a basename. Resolve the complete
 * expected path and compare node identity, with a balanced lookup reference. */
static inline bool apple_j716c_usb2_node_is(struct device_node *np, const char *path)
{
	struct device_node *expected = of_find_node_by_path(path);
	bool match = np && expected == np;

	of_node_put(expected);
	return match;
}

/* Fixed public field labels diagnose refusal without dumping per-unit data. */
static inline const char *apple_j716c_usb2_refusal(void)
{
	static const char *const domains[] = {
		J716C_USB0_SOC
		"power-management@280600000/power-controller@208",
		J716C_USB0_SOC
		"power-management@288300000/power-controller@148",
		J716C_USB0_SOC
		"power-management@288300000/power-controller@150",
		J716C_USB0_SOC
		"power-management@288300000/power-controller@158",
	};
	static const char *const virtuals[] = {
		J716C_USB0_SOC
		"power-management@280600000/power-controller@1e0",
		J716C_USB0_SOC
		"power-management@280600000/power-controller@1d8",
	};
	struct device_node *chosen __free(device_node) =
		of_find_node_by_path("/chosen");
	struct device_node *phy __free(device_node) =
		of_find_node_by_path(J716C_USB0_PHY);
	struct device_node *dwc __free(device_node) =
		of_find_node_by_path(J716C_USB0_DWC);
	struct device_node *dart __free(device_node) =
		of_find_node_by_path(J716C_USB0_DART);
	struct device_node *dart0 __free(device_node) =
		of_find_node_by_path(J716C_USB0_SOC "iommu@2a2f00000");
	struct of_phandle_args args;
	const char *status, *mode, *speed;
	u32 request, contract;
	bool match;

	if (!of_machine_is_compatible("apple,j716c") ||
	    !of_machine_is_compatible("apple,t6050"))
		return "board-compatible";
	if (!chosen)
		return "chosen-node";
	if (of_property_count_u32_elems(chosen, "j716c,usb0-handoff") != 1 ||
	    of_property_read_u32(chosen, "j716c,usb0-handoff", &request))
		return "request-encoding";
	if (request != 1)
		return "request-value";
	if (of_property_count_u32_elems(chosen, "j716c,usb0-retirement-contract") != 1 ||
	    of_property_read_u32(chosen, "j716c,usb0-retirement-contract", &contract))
		return "contract-encoding";
	if (contract != 2)
		return "contract-value";
	if (of_property_read_string(chosen, "j716c,usb0-handoff-status", &status) ||
	    strcmp(status, "retired"))
		return "retirement-status";
	if (!of_device_is_available(phy))
		return "phy-status";
	if (!of_device_is_available(dwc))
		return "dwc-status";
	if (!of_device_is_available(dart))
		return "dart-status";
	if (!dart0 || of_device_is_available(dart0))
		return "dart0-disabled";
	if (of_property_count_strings(phy, "compatible") != 1 ||
	    !of_device_is_compatible(phy, "apple,j716c-usb2-device-phy"))
		return "phy-compatible";
	if (of_property_count_strings(dwc, "compatible") != 1 ||
	    !of_device_is_compatible(dwc, "apple,j716c-usb2-device-dwc3"))
		return "dwc-compatible";
	if (of_property_count_strings(dart, "compatible") != 2 ||
	    !of_device_is_compatible(dart, "apple,j716c-usb2-device-dart") ||
	    !of_device_is_compatible(dart, "apple,t8110-dart"))
		return "dart-compatible";
	if (of_property_present(dart, "apple,tunable"))
		return "dart-tunable";
	if (of_property_present(dart, "apple,bypass-sids"))
		return "dart-bypass-sids";
	if (of_property_present(dart, "apple,remap-streams"))
		return "dart-remap-streams";
	if (of_property_present(dart, "apple,managed-sids"))
		return "dart-managed-sids";
	if (!apple_j716c_usb2_tunables(phy))
		return "phy-tunables";
	if (!apple_j716c_usb2_resource(phy, 0, 0x2a2a90000ULL, 0x4000))
		return "phy-usb2-resource";
	if (!apple_j716c_usb2_resource(phy, 1, 0x2a2800000ULL, 0x4000))
		return "phy-digital-resource";
	if (!apple_j716c_usb2_resource(phy, 2, 0x2a2a84000ULL, 0x4000))
		return "phy-pipe-resource";
	if (!apple_j716c_usb2_resource(dwc, 0, 0x2a2280000ULL, 0xcd00))
		return "dwc-resource";
	if (!apple_j716c_usb2_resource(dart, 0, 0x2a2f80000ULL, 0xc000))
		return "dart-resource";
	if (of_property_count_u32_elems(phy, "reg") != 12)
		return "phy-reg-encoding";
	if (of_property_count_u32_elems(dwc, "reg") != 4)
		return "dwc-reg-encoding";
	if (of_property_count_u32_elems(dart, "reg") != 4)
		return "dart-reg-encoding";
	if (of_property_read_string(dwc, "dr_mode", &mode) || strcmp(mode, "peripheral"))
		return "dwc-mode";
	if (of_property_read_string(dwc, "maximum-speed", &speed) || strcmp(speed, "high-speed"))
		return "dwc-speed";
	if (of_property_count_u32_elems(dwc, "iommus") != 2 ||
	    of_parse_phandle_with_args(dwc, "iommus", "#iommu-cells", 0, &args))
		return "dwc-iommus-encoding";
	match = args.np == dart && args.args_count == 1 && args.args[0] == 1;
	of_node_put(args.np);
	if (!match)
		return "dwc-iommu-reference";
	if (of_property_count_u32_elems(dwc, "phys") != 1 ||
	    of_parse_phandle_with_args(dwc, "phys", "#phy-cells", 0, &args))
		return "dwc-phys-encoding";
	match = args.np == phy && args.args_count == 0;
	of_node_put(args.np);
	if (!match)
		return "dwc-phy-reference";
	if (of_property_count_u32_elems(dwc, "resets") != 1 ||
	    of_parse_phandle_with_args(dwc, "resets", "#reset-cells", 0, &args))
		return "dwc-resets-encoding";
	match = args.np == phy && args.args_count == 0;
	of_node_put(args.np);
	if (!match)
		return "dwc-reset-reference";
	if (!apple_j716c_usb2_power(phy, domains[3], domains[2]))
		return "phy-power-domains";
	if (!apple_j716c_usb2_power(dwc, domains[2], NULL))
		return "dwc-power-domain";
	if (!apple_j716c_usb2_power(dart, domains[2], NULL))
		return "dart-power-domain";
	static const char *const domain_status[] = {
		"common-status", "aon-status", "usb-power-status", "wrap-status"
	};
	static const char *const domain_parent[] = {
		"common-power-parent", "aon-power-parent", "usb-power-parents", "wrap-power-parents"
	};
	for (unsigned int i = 0; i < ARRAY_SIZE(domains); i++) {
		struct device_node *pd __free(device_node) = of_find_node_by_path(domains[i]);

		if (!of_device_is_available(pd))
			return domain_status[i];
		if (!apple_j716c_usb2_power(pd, i >= 2 ? domains[0] : NULL,
					       i >= 2 ? domains[1] : NULL))
			return domain_parent[i];
	}
	for (unsigned int i = 0; i < ARRAY_SIZE(virtuals); i++) {
		struct device_node *np __free(device_node) = of_find_node_by_path(virtuals[i]);

		if (!np || of_device_is_available(np))
			return i ? "virtual-fab0-disabled" : "virtual-fab3-disabled";
	}
	return NULL;
}

static inline bool apple_j716c_usb2_ready(void)
{
	return !apple_j716c_usb2_refusal();
}
#endif
