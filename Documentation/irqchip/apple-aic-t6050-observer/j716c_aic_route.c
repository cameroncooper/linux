// SPDX-License-Identifier: GPL-2.0-only
/* One guarded read of the known public M5 SIQ route field per online CPU. */
#include <linux/module.h>
#include <linux/of.h>
#include <linux/smp.h>
#include <linux/cpu.h>
#include <asm/sysreg.h>

static unsigned int routes[18];
static unsigned int route_count = ARRAY_SIZE(routes);
module_param_array(routes, uint, &route_count, 0444);
MODULE_PARM_DESC(routes, "Read-only SIQ route bits indexed by CPU; no IRQ steering");

static void capture(void *unused)
{
 unsigned int cpu = smp_processor_id();
 if (cpu >= ARRAY_SIZE(routes))
  return;
 if ((read_sysreg(CurrentEL) >> 2) != 2) {
  WRITE_ONCE(routes[cpu], 5);
  return;
 }
 WRITE_ONCE(routes[cpu], read_sysreg_s(sys_reg(3, 4, 15, 10, 4)) & 3);
}

static int __init j716c_aic_route_init(void)
{
 struct device_node *chosen;
 const char *marker, *observe;
 unsigned int cpu;
 bool allowed;

 if (!of_machine_is_compatible("apple,j716c") ||
     !of_machine_is_compatible("apple,t6050") || num_online_cpus() != 18)
  return -EPERM;
 chosen = of_find_node_by_path("/chosen");
 allowed = chosen &&
  of_property_count_strings(chosen, "j716c,ram-test") == 1 &&
  !of_property_read_string(chosen, "j716c,ram-test", &marker) &&
  !strcmp(marker, "usb2-acm-last") &&
  of_property_count_strings(chosen, "j716c,aic-route-observe") == 1 &&
  !of_property_read_string(chosen, "j716c,aic-route-observe", &observe) &&
  !strcmp(observe, "read-only");
 of_node_put(chosen);
 if (!allowed)
  return -EPERM;
 for_each_online_cpu(cpu)
  if (cpu >= ARRAY_SIZE(routes))
   return -EPERM;
 for (cpu = 0; cpu < ARRAY_SIZE(routes); cpu++)
  routes[cpu] = 4;
 on_each_cpu(capture, NULL, 1);
 for (cpu = 0; cpu < ARRAY_SIZE(routes); cpu++) {
  if (READ_ONCE(routes[cpu]) > 3)
   return -EPERM;
  pr_info("j716c-aic-route: CPU%u route=%u\n", cpu, routes[cpu]);
 }
 return 0;
}
module_init(j716c_aic_route_init);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Guarded read-only J716c per-CPU SIQ route observer");
