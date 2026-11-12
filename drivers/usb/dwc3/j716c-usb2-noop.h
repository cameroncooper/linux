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

/* Separate from the existing empty-START policy. */
enum dwc3_apple_j716c_fifo_profile {
	DWC3_APPLE_J716C_FIFO_DEFAULT = 0,
	DWC3_APPLE_J716C_FIFO_CONTROL = 2,
	DWC3_APPLE_J716C_FIFO_PHYSICAL = 3,
};

/* Experimental policies; cached after the same exact-board admission. */
enum dwc3_apple_j716c_bulk_policy {
	DWC3_APPLE_J716C_BULK_DEFAULT = 0,
	DWC3_APPLE_J716C_BULK_CONTROL = 2,
	DWC3_APPLE_J716C_BULK_ACK = 3,
	DWC3_APPLE_J716C_BULK_SINGLE = 4,
	DWC3_APPLE_J716C_BULK_PREMAP = 5,
	DWC3_APPLE_J716C_BULK_LAST = 6,
	DWC3_APPLE_J716C_ACM_CONTROL = 7,
	DWC3_APPLE_J716C_ACM_LAST = 8,
	DWC3_APPLE_J716C_ACM_INTERVAL_ZERO = 9,
	DWC3_APPLE_J716C_ACM_NOTIFY_FIFO = 10,
	DWC3_APPLE_J716C_ACM_MPS16 = 11,
};

#if IS_BUILTIN(CONFIG_USB_DWC3) && IS_BUILTIN(CONFIG_USB_DWC3_APPLE) && \
	IS_BUILTIN(CONFIG_PHY_APPLE_J716C_USB2_DEVICE)
enum dwc3_apple_j716c_bulk_policy
dwc3_apple_j716c_bulk_policy(struct dwc3 *dwc);
#else
static inline enum dwc3_apple_j716c_bulk_policy
dwc3_apple_j716c_bulk_policy(struct dwc3 *dwc)
{
	return DWC3_APPLE_J716C_BULK_DEFAULT;
}
#endif

#if IS_BUILTIN(CONFIG_USB_DWC3) && IS_BUILTIN(CONFIG_USB_DWC3_APPLE) && \
	IS_BUILTIN(CONFIG_PHY_APPLE_J716C_USB2_DEVICE)
enum dwc3_apple_j716c_fifo_profile
dwc3_apple_j716c_fifo_profile(struct dwc3 *dwc);
#else
static inline enum dwc3_apple_j716c_fifo_profile
dwc3_apple_j716c_fifo_profile(struct dwc3 *dwc)
{
	return DWC3_APPLE_J716C_FIFO_DEFAULT;
}
#endif

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
