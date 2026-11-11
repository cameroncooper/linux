/* SPDX-License-Identifier: GPL-2.0-only */
/* Written before the driver: source-integrated register failure fixture. */
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include "../../../drivers/phy/apple/j716c-usb2-sequence.h"

struct fake {
	unsigned int r[3][4096];
	unsigned int writes;
	int fault;
};
static unsigned int rd(void *ctx, unsigned int bank, unsigned int off)
{
	struct fake *f = ctx;
	if (f->fault == 1 && bank == 2 && off == 0x1c)
		return 0x11;
	if (f->fault == 2 && bank == 2 && off == 0x0c)
		return 0;
	if (f->fault == 3 && bank == 0 && off == 0)
		return 4;
	return f->r[bank][off / 4];
}
static void wr(void *ctx, unsigned int bank, unsigned int off, unsigned int val)
{
	struct fake *f = ctx;
	f->writes++;
	f->r[bank][off / 4] = val;
}
static void delay(void *ctx, unsigned int us)
{
	(void)ctx;
	(void)us;
}
static const struct j716c_usb2_io io = { .read = rd,
					 .write = wr,
					 .delay = delay };
static struct fake stopped(void)
{
	struct fake f = { 0 };
	f.r[2][0xc / 4] = 0x42;
	f.r[2][0x1c / 4] = 0x10;
	return f;
}
int main(void)
{
	struct fake f = stopped();
	assert(!j716c_usb2_hold_reset(&io, &f));
	assert(!j716c_usb2_start(&io, &f));
	assert((f.r[2][0x1c / 4] & 0x11) == 1);
	assert((f.r[0][4 / 4] & 0xf) == 4);
	assert((f.r[0][0] & 7) == 2);
	for (unsigned int mux = 0; mux < 0x80; mux++) {
		if ((mux & 0x73) == 0x42)
			continue;
		f = stopped();
		f.r[2][0xc / 4] = mux;
		assert(j716c_usb2_hold_reset(&io, &f) == -EBUSY);
		assert(!f.writes);
	}
	f = stopped();
	f.r[2][0x1c / 4] = 0x11;
	assert(j716c_usb2_hold_reset(&io, &f) == -EBUSY);
	assert(!f.writes);
	for (int fault = 1; fault <= 3; fault++) {
		f = stopped();
		f.fault = fault;
		assert(j716c_usb2_start(&io, &f) < 0);
	}
	puts("PASS: exact retired state, all other mux refusal, live reset refusal, successful public USB2 sequence, reset/mux/PHY readback failures");
}
