/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _J716C_USB2_SEQUENCE_H
#define _J716C_USB2_SEQUENCE_H

/* Public Aura usb_t6050_init USB2 subset. Bank 0 PHY, 1 digital, 2 PIPE.
 * Kept independent of Linux so the real register sequence runs in the fixture.
 */
struct j716c_usb2_io {
	unsigned int (*read)(void *ctx, unsigned int bank, unsigned int offset);
	void (*write)(void *ctx, unsigned int bank, unsigned int offset,
		      unsigned int value);
	void (*delay)(void *ctx, unsigned int us);
	int (*tune)(void *ctx);
};
static inline void j716c_usb2_mask(const struct j716c_usb2_io *io, void *ctx,
				   unsigned int bank, unsigned int off,
				   unsigned int mask, unsigned int value)
{
	io->write(ctx, bank, off, (io->read(ctx, bank, off) & ~mask) | value);
}
static inline int j716c_usb2_poll(const struct j716c_usb2_io *io, void *ctx,
				  unsigned int off, unsigned int mask,
				  unsigned int expected)
{
	for (unsigned int i = 0; i < 1000; i++) {
		if ((io->read(ctx, 2, off) & mask) == expected)
			return 0;
		io->delay(ctx, 1);
	}
	return -ETIMEDOUT;
}
/* First assertion may only adopt the exact state left by checked retirement. */
static inline int j716c_usb2_hold_reset(const struct j716c_usb2_io *io,
					void *ctx)
{
	if ((io->read(ctx, 2, 0x0c) & 0x73) != 0x42 ||
	    (io->read(ctx, 2, 0x1c) & 0x11) != 0x10)
		return -EBUSY;
	return 0;
}
static inline int j716c_usb2_stop(const struct j716c_usb2_io *io, void *ctx)
{
	if ((io->read(ctx, 2, 0x0c) & 0x73) != 0x42)
		return -EBUSY;
	j716c_usb2_mask(io, ctx, 2, 0x1c, 0, 0x10);
	j716c_usb2_mask(io, ctx, 2, 0x1c, 1, 0);
	return j716c_usb2_poll(io, ctx, 0x1c, 0x11, 0x10);
}
/* Tunables must have been validated/applied while the consumer is held reset. */
static inline int j716c_usb2_start(const struct j716c_usb2_io *io, void *ctx)
{
	int ret = j716c_usb2_hold_reset(io, ctx);
	if (ret)
		return ret;
	j716c_usb2_mask(io, ctx, 2, 0x0c, 0x70, 0);
	io->delay(ctx, 10);
	j716c_usb2_mask(io, ctx, 2, 0x0c, 3, 2);
	io->delay(ctx, 10);
	j716c_usb2_mask(io, ctx, 2, 0x0c, 0x70, 0x40);
	io->delay(ctx, 10);
	if ((io->read(ctx, 2, 0x0c) & 0x73) != 0x42)
		return -EIO;
	if (io->tune) {
		ret = io->tune(ctx);
		if (ret)
			return ret;
	}
	j716c_usb2_mask(io, ctx, 0, 0x08, 0, 0xf);
	io->delay(ctx, 10000);
	j716c_usb2_mask(io, ctx, 0, 0x04, 8, 0);
	io->delay(ctx, 10);
	j716c_usb2_mask(io, ctx, 0, 0x04, 3, 0);
	j716c_usb2_mask(io, ctx, 1, 0, 0, 9);
	j716c_usb2_mask(io, ctx, 0, 0x04, 0, 4);
	j716c_usb2_mask(io, ctx, 0, 0x1c, 0x60000000, 0);
	io->delay(ctx, 30);
	io->delay(ctx, 5000);
	j716c_usb2_mask(io, ctx, 0, 0, 7, 2);
	j716c_usb2_mask(io, ctx, 2, 0x20, 0, 0x8000);
	j716c_usb2_mask(io, ctx, 2, 0x1c, 0x10, 0);
	j716c_usb2_mask(io, ctx, 2, 0x1c, 0, 1);
	ret = j716c_usb2_poll(io, ctx, 0x1c, 0x11, 1);
	if (ret)
		return ret;
	if ((io->read(ctx, 0, 0) & 7) != 2 || (io->read(ctx, 0, 4) & 0xf) != 4)
		return -EIO;
	return 0;
}
#endif
