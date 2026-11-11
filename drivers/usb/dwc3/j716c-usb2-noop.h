/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __DWC3_J716C_USB2_NOOP_H
#define __DWC3_J716C_USB2_NOOP_H
#include <linux/kconfig.h>

struct dwc3;

enum dwc3_apple_j716c_noop_profile {
	DWC3_APPLE_J716C_NOOP_DEFAULT,
	DWC3_APPLE_J716C_NOOP_CONTROL,
	DWC3_APPLE_J716C_NOOP_SKIP,
};

#if IS_BUILTIN(CONFIG_USB_DWC3) && IS_BUILTIN(CONFIG_USB_DWC3_APPLE)
enum dwc3_apple_j716c_noop_profile
dwc3_apple_j716c_noop_profile(struct dwc3 *dwc);
#else
static inline enum dwc3_apple_j716c_noop_profile
dwc3_apple_j716c_noop_profile(struct dwc3 *dwc)
{
	return DWC3_APPLE_J716C_NOOP_DEFAULT;
}
#endif
#endif
