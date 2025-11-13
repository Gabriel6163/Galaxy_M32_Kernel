#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include "mtk_perfmgr_internal.h"
#include <linux/cpumask.h>

/* Global variable for number of CPU clusters */
int clstr_num;
EXPORT_SYMBOL(clstr_num);

/* Extern Declarations for Sub-module Init Functions */
extern int init_tchbst(struct proc_dir_entry *perfmgr_root);
extern int init_boostctrl(struct proc_dir_entry *perfmgr_root);
extern int syslimiter_init(struct proc_dir_entry *perfmgr_root);
extern int init_perfctl(struct proc_dir_entry *perfmgr_root);

/* Explicitly declare init functions for FPSGO and EARA */
#ifdef CONFIG_MTK_FPSGO_V3
extern int init_fpsgo_v3(struct proc_dir_entry *perfmgr_root);
#endif
#ifdef CONFIG_MTK_EARA_THERMAL
extern int init_eara_thermal(struct proc_dir_entry *perfmgr_root);
#endif

static int perfmgr_probe(struct platform_device *dev)
{
	/* Get cluster number from topology */
	clstr_num = arch_get_nr_clusters();
	return 0;
}

struct platform_device perfmgr_device = {
	.name   = "perfmgr",
	.id     = -1,
};

static int perfmgr_suspend(struct device *dev)
{
	return 0;
}

static int perfmgr_resume(struct device *dev)
{
	return 0;
}
static int perfmgr_remove(struct platform_device *dev)
{
	return 0;
}

static const struct dev_pm_ops perfmgr_pm_ops = {
	.suspend = perfmgr_suspend,
	.resume = perfmgr_resume,
};

static struct platform_driver perfmgr_driver = {
	.probe   = perfmgr_probe,
	.remove  = perfmgr_remove,
	.driver  = {
		.name = "perfmgr",
		.pm = &perfmgr_pm_ops,
	},
};

static int perfmgr_main_data_init(void)
{
	return 0;
}

static int __init init_perfmgr(void)
{
	struct proc_dir_entry *perfmgr_root = NULL;
	int ret = 0;

	ret = platform_device_register(&perfmgr_device);
	if (ret)
		return ret;

	ret = platform_driver_register(&perfmgr_driver);
	if (ret)
		return ret;

	perfmgr_main_data_init();

	perfmgr_root = proc_mkdir("perfmgr", NULL);
	if (perfmgr_root) {
		init_tchbst(perfmgr_root);
		init_boostctrl(perfmgr_root);
		syslimiter_init(perfmgr_root);
		init_perfctl(perfmgr_root);

#ifdef CONFIG_MTK_FPSGO_V3
		init_fpsgo_v3(perfmgr_root);
#endif
#ifdef CONFIG_MTK_EARA_THERMAL
		init_eara_thermal(perfmgr_root);
#endif
	}

	return 0;
}
device_initcall(init_perfmgr);
