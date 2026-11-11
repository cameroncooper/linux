// SPDX-License-Identifier: GPL-2.0-only
/* Experimental exact-unit USB0 USB2 device provider; public Aura USB2 sequence. */
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>
#include <linux/reset-controller.h>
#include <linux/soc/apple/j716c-usb2.h>
#include <linux/soc/apple/j716c-usb2-snapshot.h>
#include <linux/soc/apple/tunable.h>
#include "j716c-usb2-sequence.h"

struct j716c_usb2 {
	struct device *dev;
	struct dev_pm_domain_list *domains;
	void __iomem *regs[3];
	struct apple_tunable *defaults, *device;
	struct reset_controller_dev rcdev;
	struct mutex lock;
	bool adopted, running, device_mode;
};
static unsigned int j716c_usb2_read(void *ctx, unsigned int bank,
				    unsigned int off)
{
	struct j716c_usb2 *p = ctx;
	return readl(p->regs[bank] + off);
}
static void j716c_usb2_write(void *ctx, unsigned int bank, unsigned int off,
			     unsigned int value)
{
	struct j716c_usb2 *p = ctx;
	writel(value, p->regs[bank] + off);
}
static void j716c_usb2_delay(void *ctx, unsigned int us)
{
	if (us < 1000)
		udelay(us);
	else
		usleep_range(us, us + 100);
}
static int j716c_usb2_tune(void *ctx)
{
	struct j716c_usb2 *p = ctx;
	apple_tunable_apply(p->regs[0], p->defaults);
	apple_tunable_apply(p->regs[0], p->device);
	return 0;
}
static const struct j716c_usb2_io j716c_usb2_mmio = {
	.read = j716c_usb2_read,
	.write = j716c_usb2_write,
	.delay = j716c_usb2_delay,
	.tune = j716c_usb2_tune,
};
static int j716c_usb2_reset_assert(struct reset_controller_dev *rcdev,
				   unsigned long id)
{
	struct j716c_usb2 *p = container_of(rcdev, struct j716c_usb2, rcdev);
	int ret;
	guard(mutex)
		(&p->lock);
	if (id || !apple_j716c_usb2_ready())
		return -EPERM;
	ret = p->adopted ? j716c_usb2_stop(&j716c_usb2_mmio, p) :
			   j716c_usb2_hold_reset(&j716c_usb2_mmio, p);
	if (!ret) {
		p->adopted = true;
		p->running = false;
	}
	return ret;
}
static int j716c_usb2_reset_deassert(struct reset_controller_dev *rcdev,
				     unsigned long id)
{
	struct j716c_usb2 *p = container_of(rcdev, struct j716c_usb2, rcdev);
	int ret;
	guard(mutex)
		(&p->lock);
	if (id || !apple_j716c_usb2_ready() || !p->adopted || !p->device_mode)
		return -EPERM;
	if (p->running)
		return 0;
	ret = j716c_usb2_start(&j716c_usb2_mmio, p);
	if (ret) {
		int stopped = j716c_usb2_stop(&j716c_usb2_mmio, p);
		if (stopped)
			dev_err(p->dev,
				"failed startup also failed reset: %d\n",
				stopped);
		return ret;
	}
	p->running = true;
	return 0;
}
static int j716c_usb2_reset_status(struct reset_controller_dev *rcdev,
				   unsigned long id)
{
	struct j716c_usb2 *p = container_of(rcdev, struct j716c_usb2, rcdev);
	if (id)
		return -EINVAL;
	return !(readl(p->regs[2] + 0x1c) & 1);
}
static int j716c_usb2_reset_xlate(struct reset_controller_dev *rcdev,
				  const struct of_phandle_args *args)
{
	(void)rcdev;
	return args->args_count ? -EINVAL : 0;
}
static const struct reset_control_ops j716c_usb2_reset_ops = {
	.assert = j716c_usb2_reset_assert,
	.deassert = j716c_usb2_reset_deassert,
	.status = j716c_usb2_reset_status,
};
static int j716c_usb2_set_mode(struct phy *phy, enum phy_mode mode, int submode)
{
	struct j716c_usb2 *p = phy_get_drvdata(phy);
	guard(mutex)
		(&p->lock);
	if (mode != PHY_MODE_USB_DEVICE || submode || !apple_j716c_usb2_ready())
		return -EOPNOTSUPP;
	p->device_mode = true;
	return 0;
}
static int j716c_usb2_phy_init(struct phy *phy)
{
	struct j716c_usb2 *p = phy_get_drvdata(phy);
	guard(mutex)
		(&p->lock);
	return p->running && p->device_mode ? 0 : -EPERM;
}
/* The DWC3 glue owns reset. Power-off must not reset a running consumer. */
static const struct phy_ops j716c_usb2_phy_ops = {
	.init = j716c_usb2_phy_init,
	.set_mode = j716c_usb2_set_mode,
	.owner = THIS_MODULE,
};
static int j716c_usb2_probe(struct platform_device *pdev)
{
	static const u32 expected_default[] = {
		8, 0x7003, 3, 0x18, 0x703, 0x103
	};
	static const u32 expected_device[] = { 8, 0x7003, 3 };
	struct device *dev = &pdev->dev;
	struct j716c_usb2 *p;
	struct phy_provider *provider;
	struct phy *phy;
	struct resource *res;
	u32 defaults[6], device[3];
	int ret;

	if (!apple_j716c_usb2_node_is(dev->of_node, J716C_USB0_PHY) ||
	    !apple_j716c_usb2_ready()) {
		const char *reason = !apple_j716c_usb2_node_is(dev->of_node, J716C_USB0_PHY) ?
			"phy-node-path" : apple_j716c_usb2_refusal();

		return dev_err_probe(dev, -EPERM, "USB0 admission: %s (node %pOF)\n",
				     reason ? reason : "metadata", dev->of_node);
	}
	if (of_property_count_u32_elems(dev->of_node,
					"apple,tunable-usb2-default") != 6 ||
	    of_property_count_u32_elems(dev->of_node,
					"apple,tunable-usb2-device") != 3 ||
	    of_property_read_u32_array(
		    dev->of_node, "apple,tunable-usb2-default", defaults, 6) ||
	    of_property_read_u32_array(
		    dev->of_node, "apple,tunable-usb2-device", device, 3) ||
	    memcmp(defaults, expected_default, sizeof(defaults)) ||
	    memcmp(device, expected_device, sizeof(device)))
		return dev_err_probe(dev, -EINVAL,
				     "changed USB2 default/device tunables\n");
	p = devm_kzalloc(dev, sizeof(*p), GFP_KERNEL);
	if (!p)
		return -ENOMEM;
	p->dev = dev;
	mutex_init(&p->lock);
	for (unsigned int i = 0; i < 3; i++) {
		p->regs[i] = devm_platform_ioremap_resource(pdev, i);
		if (IS_ERR(p->regs[i]))
			return PTR_ERR(p->regs[i]);
	}
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	p->defaults = devm_apple_tunable_parse(
		dev, dev->of_node, "apple,tunable-usb2-default", res);
	if (IS_ERR(p->defaults))
		return PTR_ERR(p->defaults);
	p->device = devm_apple_tunable_parse(dev, dev->of_node,
					     "apple,tunable-usb2-device", res);
	if (IS_ERR(p->device))
		return PTR_ERR(p->device);
	/* PIPE needs both already-validated WRAP and USB resources. Establish
	 * both suppliers before its first read; do not rely on probe ordering. */
	const struct dev_pm_domain_attach_data attach = {
		.pd_flags = PD_FLAG_DEV_LINK_ON,
	};

	ret = devm_pm_domain_attach_list(dev, &attach, &p->domains);
	if (ret < 0)
		return dev_err_probe(dev, ret, "USB0 power suppliers\n");
	if (ret != 2)
		return -ENODEV;
	/* The explicit links keep both suppliers on for this no-suspend trial. */
	/* No MMIO write at probe; only adopt the stopped, clamped dummy backend. */
	ret = j716c_usb2_hold_reset(&j716c_usb2_mmio, p);
	if (ret)
		return dev_err_probe(dev, ret, "USB0 PIPE is not retired\n");
	phy = devm_phy_create(dev, NULL, &j716c_usb2_phy_ops);
	if (IS_ERR(phy))
		return PTR_ERR(phy);
	phy_set_drvdata(phy, p);
	p->rcdev = (struct reset_controller_dev){
		.owner = THIS_MODULE,
		.ops = &j716c_usb2_reset_ops,
		.nr_resets = 1,
		.of_node = dev->of_node,
		.of_reset_n_cells = 0,
		.of_xlate = j716c_usb2_reset_xlate,
		.dev = dev,
	};
	ret = devm_reset_controller_register(dev, &p->rcdev);
	if (ret)
		return ret;
	provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);
	if (IS_ERR(provider))
		return PTR_ERR(provider);
	/* No independently tested suspend/reconnect path in this experiment. */
	dev_pm_syscore_device(dev, true);
	dev_info(
		dev,
		"retired USB0 USB2 device profile; fixed cable, experimental\n");
	return 0;
}
static const struct of_device_id j716c_usb2_of_match[] = {
	{ .compatible = "apple,j716c-usb2-device-phy" },
	{}
};
MODULE_DEVICE_TABLE(of, j716c_usb2_of_match);
static struct platform_driver j716c_usb2_driver = {
	.probe = j716c_usb2_probe,
	.driver = { .name = "j716c-usb2-device-phy",
		    .of_match_table = j716c_usb2_of_match,
		    .suppress_bind_attrs = true },
};

#if APPLE_J716C_USB2_SNAPSHOT_ENABLED
/* No PM operation: inspect the references already owned by this profile. */
static int j716c_snapshot_pm(struct device *dev, bool disabled,
			    const char **refusal)
{
	unsigned long flags;
	bool valid;

	if (!spin_trylock_irqsave(&dev->power.lock, flags)) {
		*refusal = "provider-power-lock";
		return -EAGAIN;
	}
	valid = !dev->power.runtime_error;
	if (disabled)
		valid &= dev->power.disable_depth > 0 && dev->power.syscore;
	else
		valid &= !dev->power.disable_depth &&
			 dev->power.runtime_status == RPM_ACTIVE &&
			 !dev->power.request_pending &&
			 atomic_read(&dev->power.usage_count) > 0;
	spin_unlock_irqrestore(&dev->power.lock, flags);
	if (!valid) {
		*refusal = disabled ? "provider-runtime-state" :
				     "supplier-runtime-state";
		return -EIO;
	}
	return 0;
}

void apple_j716c_usb2_snapshot_end(
	struct apple_j716c_usb2_snapshot_guard *guard)
{
	int i;

	if (guard->owner) {
		mutex_unlock(&guard->owner->lock);
		guard->owner = NULL;
	}
	for (i = 3; i >= 0; i--) {
		if (!guard->pmgr_owner[i])
			continue;
		device_unlock(guard->pmgr_owner[i]);
		put_device(guard->pmgr_owner[i]);
		guard->pmgr_owner[i] = NULL;
	}
	if (guard->provider) {
		device_unlock(guard->provider);
		put_device(guard->provider);
		guard->provider = NULL;
	}
}

/* Fresh boot only: no prior PMGR owner unbind/rebind in this diagnostic. */
int apple_j716c_usb2_snapshot_try_begin(
	struct phy *phy, struct device *consumer,
	struct apple_j716c_usb2_snapshot_guard *guard, const char **refusal)
{
	static const char *const paths[] = {
		J716C_USB0_SOC "power-management@280600000/power-controller@208",
		J716C_USB0_SOC "power-management@288300000/power-controller@148",
		J716C_USB0_SOC "power-management@288300000/power-controller@150",
		J716C_USB0_SOC "power-management@288300000/power-controller@158",
	};
	static const int supplier_owner[] = { 3, 2 }; /* WRAP, USB */
	struct device *provider;
	struct j716c_usb2 *p;
	int ret = -EPERM, i;

	if (!refusal)
		return -EINVAL;
	*refusal = "provider-object";
	if (!guard || guard->owner || guard->provider ||
	    guard->pmgr_owner[0] || guard->pmgr_owner[1] ||
	    guard->pmgr_owner[2] || guard->pmgr_owner[3] ||
	    IS_ERR_OR_NULL(phy) || !consumer ||
	    phy->ops != &j716c_usb2_phy_ops)
		return -EINVAL;
	if (!apple_j716c_usb2_node_is(consumer->of_node, J716C_USB0_DWC) ||
	    !apple_j716c_usb2_node_is(phy->dev.of_node, J716C_USB0_PHY) ||
	    !apple_j716c_usb2_ready())
		return -EPERM;
	provider = get_device(phy->dev.parent);
	if (!provider)
		return -ENODEV;
	if (!device_trylock(provider)) {
		put_device(provider);
		*refusal = "provider-lock";
		return -EAGAIN;
	}
	guard->provider = provider;
	*refusal = "provider-not-bound";
	if (!device_is_registered(provider) ||
	    provider->driver != &j716c_usb2_driver.driver ||
	    !apple_j716c_usb2_node_is(provider->of_node, J716C_USB0_PHY)) {
		ret = -ENODEV;
		goto out;
	}
	/* Exact owner locks precede any genpd pointer dereference. */
	for (i = 0; i < ARRAY_SIZE(paths); i++) {
		struct device_node *np __free(device_node) =
			of_find_node_by_path(paths[i]);
		struct platform_device *pdev;
		struct device *owner;

		*refusal = "pmgr-owner-object";
		if (!np || of_property_present(np, "apple,firmware-owned") ||
		    of_property_present(np, "apple,consumer-switched"))
			goto out;
		pdev = of_find_device_by_node(np);
		if (!pdev) {
			ret = -ENODEV;
			goto out;
		}
		owner = &pdev->dev; /* lookup reference, not a PM reference */
		if (!device_trylock(owner)) {
			put_device(owner);
			*refusal = "pmgr-owner-lock";
			ret = -EAGAIN;
			goto out;
		}
		guard->pmgr_owner[i] = owner;
		*refusal = "pmgr-owner-not-bound";
		if (!device_is_registered(owner) || owner->of_node != np ||
		    !owner->driver ||
		    strcmp(owner->driver->name, "apple-pmgr-pwrstate")) {
			ret = -ENODEV;
			goto out;
		}
	}
	p = phy_get_drvdata(phy);
	*refusal = "provider-private-object";
	if (!p || p->dev != provider)
		goto out;
	if (!mutex_trylock(&p->lock)) {
		*refusal = "provider-private-lock";
		ret = -EAGAIN;
		goto out;
	}
	guard->owner = p;
	*refusal = "provider-not-running";
	if (!p->adopted || !p->running || !p->device_mode)
		goto out;
	ret = j716c_snapshot_pm(provider, true, refusal);
	if (ret)
		goto out;
	*refusal = "supplier-list";
	ret = -EPERM;
	if (!p->domains || p->domains->num_pds != 2 ||
	    !p->domains->pd_devs || !p->domains->pd_links ||
	    p->domains->pd_devs[0] == p->domains->pd_devs[1] ||
	    p->domains->pd_links[0] == p->domains->pd_links[1])
		goto out;
	for (i = 0; i < 2; i++) {
		struct device *supplier = p->domains->pd_devs[i];
		struct device_link *link = p->domains->pd_links[i];
		struct device *gpd_dev;
		struct generic_pm_domain *gpd;
		struct device_node *expected =
			guard->pmgr_owner[supplier_owner[i]]->of_node;
		u32 required = DL_FLAG_STATELESS | DL_FLAG_PM_RUNTIME;

		*refusal = "supplier-rpm-reference";
		ret = -EIO;
		if (!supplier || !link || link->consumer != provider ||
		    link->supplier != supplier ||
		    (READ_ONCE(link->flags) & required) != required ||
		    refcount_read(&link->rpm_active) <= 1 ||
		    !device_is_registered(supplier))
			goto out;
		ret = j716c_snapshot_pm(supplier, false, refusal);
		if (ret)
			goto out;
		*refusal = "supplier-domain-identity";
		ret = -EIO;
		gpd_dev = dev_to_genpd_dev(supplier);
		if (IS_ERR_OR_NULL(gpd_dev) || gpd_dev->of_node != expected ||
		    !device_is_registered(gpd_dev))
			goto out;
		gpd = container_of(gpd_dev, struct generic_pm_domain, dev);
		if (!READ_ONCE(gpd->has_provider) ||
		    READ_ONCE(gpd->provider) != of_fwnode_handle(expected) ||
		    READ_ONCE(gpd->status) != GENPD_STATE_ON)
			goto out;
	}
	*refusal = NULL;
	return 0;
out:
	apple_j716c_usb2_snapshot_end(guard);
	return ret;
}
#endif
module_platform_driver(j716c_usb2_driver);
MODULE_DESCRIPTION("Experimental J716c USB0 USB2-only device PHY and reset");
MODULE_LICENSE("GPL");
