/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _LINUX_SOC_APPLE_J716C_USB2_SNAPSHOT_H
#define _LINUX_SOC_APPLE_J716C_USB2_SNAPSHOT_H
#include <linux/kconfig.h>

/* system_transition_mutex is not exported: never enable this in modules. */
#if IS_BUILTIN(CONFIG_USB_DWC3) && IS_BUILTIN(CONFIG_USB_DWC3_APPLE) && \
	IS_BUILTIN(CONFIG_PHY_APPLE_J716C_USB2_DEVICE) && IS_ENABLED(CONFIG_DEBUG_FS)
#define APPLE_J716C_USB2_SNAPSHOT_ENABLED 1
#else
#define APPLE_J716C_USB2_SNAPSHOT_ENABLED 0
#endif

#if APPLE_J716C_USB2_SNAPSHOT_ENABLED
struct device;
struct phy;
struct j716c_usb2;

#define J716C_USB2_SNAPSHOT_MARKER "usb2-live-state"

/* Internal lock/ref ledger, never part of the pointer-free output record. */
struct apple_j716c_usb2_snapshot_guard {
	struct j716c_usb2 *owner;
	struct device *provider;
	struct device *pmgr_owner[4];
};

int apple_j716c_usb2_snapshot_try_begin(
	struct phy *phy, struct device *consumer,
	struct apple_j716c_usb2_snapshot_guard *guard, const char **refusal);
void apple_j716c_usb2_snapshot_end(
	struct apple_j716c_usb2_snapshot_guard *guard);
#endif
#endif
