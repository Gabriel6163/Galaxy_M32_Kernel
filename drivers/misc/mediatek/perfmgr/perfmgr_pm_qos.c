#include <linux/module.h>
#include <linux/pm_qos.h>
#include <linux/cpufreq.h>
#include <linux/sched.h>
#include "perfmgr_pm_qos.h"

static struct pm_qos_request qos_cpu_freq;

void perfmgr_pm_qos_init(void)
{
    pr_info("[PERFMGR] PM QoS init\n");
    pm_qos_add_request(&qos_cpu_freq, PM_QOS_CPU_FREQ_MIN, 0);
}

void perfmgr_pm_qos_update(int freq)
{
    pm_qos_update_request(&qos_cpu_freq, freq);
}

void perfmgr_pm_qos_remove(void)
{
    pm_qos_remove_request(&qos_cpu_freq);
}
