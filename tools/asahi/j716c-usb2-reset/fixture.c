/* SPDX-License-Identifier: GPL-2.0-only */
/* Written before the fix; OF/list/mutex operations are modeled boundaries. */
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
struct device_node {
	int present;
};
struct fwnode_handle {
	int present;
};
struct device {
	struct device_node *of_node;
};
struct fwnode_reference_args {
	unsigned int args[1];
};
struct of_phandle_args {
	unsigned int args_count, args[1];
};
struct reset_control_ops {
	int unused;
};
struct reset_controller_dev {
	struct device_node *of_node;
	struct fwnode_handle *fwnode;
	struct device *dev;
	void *owner;
	const struct reset_control_ops *ops;
	unsigned int nr_resets, of_reset_n_cells, fwnode_reset_n_cells;
	int lock, reset_control_head, list;
	int (*of_xlate)(struct reset_controller_dev *,
			const struct of_phandle_args *);
	int (*fwnode_xlate)(struct reset_controller_dev *,
			    const struct fwnode_reference_args *);
};
#define THIS_MODULE NULL
#define guard(x) (void)
#define INIT_LIST_HEAD(x) ((void)(x))
static int reset_list_mutex, reset_controller_list;
static void mutex_init(int *m)
{
	(void)m;
}
static void list_add(int *a, int *b)
{
	(void)a;
	(void)b;
}
static struct fwnode_handle *of_fwnode_handle(struct device_node *np)
{
	return (struct fwnode_handle *)np;
}
static struct fwnode_handle *dev_fwnode(struct device *dev)
{
	return of_fwnode_handle(dev->of_node);
}
static const struct reset_control_ops j716c_usb2_reset_ops = { 0 };
struct provider {
	struct reset_controller_dev rcdev;
};
/* ACTUAL_FUNCTIONS */
static void initialize(struct provider *p, struct device *dev)
{
	/* ACTUAL_INITIALIZER */
}
int main(void)
{
	struct device_node np = { 1 };
	struct device dev = { .of_node = &np };
	struct provider p = { 0 };
	initialize(&p, &dev);
	assert(!reset_controller_register(&p.rcdev));
	/* This is the exact nargs equality checked by __fwnode_reset_control_get. */
	assert(p.rcdev.fwnode_reset_n_cells == 0);
	assert(p.rcdev.of_xlate && !p.rcdev.fwnode_xlate);
	struct of_phandle_args spec = { .args_count = 0 };
	assert(p.rcdev.of_xlate(&p.rcdev, &spec) == 0);
	spec.args_count = 1;
	assert(p.rcdev.of_xlate(&p.rcdev, &spec) == -EINVAL);
	spec.args_count = 2;
	assert(p.rcdev.of_xlate(&p.rcdev, &spec) == -EINVAL);
	puts("PASS: actual reset registration preserves zero cells; actual translator returns ID0 and rejects malformed counts");
}
