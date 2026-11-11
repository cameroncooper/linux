/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __DWC3_J716C_USB2_LIVE_STATE_H
#define __DWC3_J716C_USB2_LIVE_STATE_H
#include <linux/types.h>

#include <linux/soc/apple/j716c-usb2-snapshot.h>

#if APPLE_J716C_USB2_SNAPSHOT_ENABLED
struct dwc3;

struct j716c_usb2_trb_sample {
	u32 ctrl_before, size, ctrl_after;
	bool status_valid;
};

/* Scalars only. seq-file operations must never retain a driver pointer. */
struct j716c_usb2_live_sample {
	u64 monotonic_ns;
	u32 ordinal;
	u32 dctl, dsts, gevntcount, gevntsiz;
	u32 event_flags_before, event_count_before, event_lpos, event_length;
	u32 event_flags_after, event_count_after;
	u32 ep0state, ep0_next_event;
	struct j716c_usb2_trb_sample trb[2];
};

int dwc3_apple_j716c_live_snapshot(struct dwc3 *dwc,
	struct j716c_usb2_live_sample *sample, const char **refusal);
#endif
#endif
