// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/atomic.h>
#include <linux/slab.h>
#include <linux/export.h>
#include <linux/ratelimit.h>

#define PNPMGR_NAME "pnpmgr"

/* Backport para printk_ratelimited no kernel 4.14 */
#ifndef pr_info_ratelimited
#define pr_info_ratelimited(fmt, ...)                                  \
do {                                                                   \
	static DEFINE_RATELIMIT_STATE(_rs, HZ, 10);                        \
	if (__ratelimit(&_rs))                                             \
		pr_info(fmt, ##__VA_ARGS__);                                   \
} while (0)
#endif

/* Kobjects para criar a estrutura de pastas */
static struct kobject *pnpmgr_kobj;
static struct kobject *fpsgo_boost_kobj;

/* Estado dos boosts */
static atomic_t pnpmgr_enabled = ATOMIC_INIT(1);
static atomic_t pnpmgr_installed = ATOMIC_INIT(0);
static atomic_t fpsgo_boost_enabled = ATOMIC_INIT(0);
static atomic_t fpsgo_boost_mode = ATOMIC_INIT(0);
static atomic_t io_boost_cnt = ATOMIC_INIT(0); // Contador para IO Boost


/* --- Funções da API exportadas para outros drivers --- */

void pnpmgr_set_boost(const char *who, int boost_type)
{
	if (!atomic_read(&pnpmgr_enabled))
		return;

	if (boost_type == 1) { /* 1 para fpsgo_boost */
		atomic_set(&fpsgo_boost_enabled, 1);
		pr_info_ratelimited("pnpmgr: %s requested fpsgo_boost\n", who ? who : "unknown");
	} else if (boost_type == 2) { /* 2 para io_boost */
		atomic_inc(&io_boost_cnt);
		pr_info_ratelimited("pnpmgr: %s requested io_boost -> cnt=%d\n",
			who ? who : "unknown", atomic_read(&io_boost_cnt));
	}
}
EXPORT_SYMBOL(pnpmgr_set_boost);

void pnpmgr_clear_boost(const char *who, int boost_type)
{
	if (boost_type == 1) {
		atomic_set(&fpsgo_boost_enabled, 0);
		pr_info_ratelimited("pnpmgr: %s cleared fpsgo_boost\n", who ? who : "unknown");
	} else if (boost_type == 2) {
		if (atomic_read(&io_boost_cnt) > 0)
			atomic_dec(&io_boost_cnt);
		pr_info_ratelimited("pnpmgr: %s cleared io_boost -> cnt=%d\n",
			who ? who : "unknown", atomic_read(&io_boost_cnt));
	}
}
EXPORT_SYMBOL(pnpmgr_clear_boost);

/* --- Implementação dos arquivos sysfs --- */

// Para /sys/pnpmgr/install
static ssize_t install_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", atomic_read(&pnpmgr_installed));
}
static ssize_t install_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int val;
	if (kstrtoint(buf, 10, &val) == 0)
		atomic_set(&pnpmgr_installed, val ? 1 : 0);
	return count;
}
static struct kobj_attribute install_attr = __ATTR(install, 0644, install_show, install_store);

// Para /sys/pnpmgr/fpsgo_boost/boost_enable
static ssize_t boost_enable_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", atomic_read(&fpsgo_boost_enabled));
}
static ssize_t boost_enable_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int val;
	if (kstrtoint(buf, 10, &val) == 0)
		atomic_set(&fpsgo_boost_enabled, val ? 1 : 0);
	return count;
}
static struct kobj_attribute boost_enable_attr = __ATTR(boost_enable, 0644, boost_enable_show, boost_enable_store);

// Para /sys/pnpmgr/fpsgo_boost/boost_mode
static ssize_t boost_mode_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", atomic_read(&fpsgo_boost_mode));
}
static ssize_t boost_mode_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int val;
	if (kstrtoint(buf, 10, &val) == 0)
		atomic_set(&fpsgo_boost_mode, val);
	return count;
}
static struct kobj_attribute boost_mode_attr = __ATTR(boost_mode, 0644, boost_mode_show, boost_mode_store);

// Para /sys/pnpmgr/io_boost
static ssize_t io_boost_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", atomic_read(&io_boost_cnt));
}
static ssize_t io_boost_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int val;
	if (kstrtoint(buf, 10, &val) == 0)
		atomic_set(&io_boost_cnt, val);
	return count;
}
static struct kobj_attribute io_boost_attr = __ATTR(io_boost, 0644, io_boost_show, io_boost_store);


/* --- Inicialização do Driver --- */
static int __init pnpmgr_init(void)
{
	int ret;

	pnpmgr_kobj = kobject_create_and_add(PNPMGR_NAME, NULL);
	if (!pnpmgr_kobj) {
		pr_err("pnpmgr: failed to create kobject\n");
		return -ENOMEM;
	}

	ret = sysfs_create_file(pnpmgr_kobj, &install_attr.attr);
	if (ret) goto err_kobj_put;

	ret = sysfs_create_file(pnpmgr_kobj, &io_boost_attr.attr);
	if (ret) goto err_remove_install;

	fpsgo_boost_kobj = kobject_create_and_add("fpsgo_boost", pnpmgr_kobj);
	if (!fpsgo_boost_kobj) {
		ret = -ENOMEM;
		goto err_remove_io;
	}

	ret = sysfs_create_file(fpsgo_boost_kobj, &boost_enable_attr.attr);
	if (ret) goto err_kobj_put_fpsgo;

	ret = sysfs_create_file(fpsgo_boost_kobj, &boost_mode_attr.attr);
	if (ret) {
		sysfs_remove_file(fpsgo_boost_kobj, &boost_enable_attr.attr);
		goto err_kobj_put_fpsgo;
	}

	pr_info("pnpmgr: initialized with full community-compatible sysfs\n");
	return 0;

err_kobj_put_fpsgo:
	kobject_put(fpsgo_boost_kobj);
err_remove_io:
	sysfs_remove_file(pnpmgr_kobj, &io_boost_attr.attr);
err_remove_install:
	sysfs_remove_file(pnpmgr_kobj, &install_attr.attr);
err_kobj_put:
	kobject_put(pnpmgr_kobj);
	pr_err("pnpmgr: failed to create sysfs entries\n");
	return ret;
}

module_init(pnpmgr_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek pnpmgr with community-compatible sysfs for FPSGO and IO boosts");

