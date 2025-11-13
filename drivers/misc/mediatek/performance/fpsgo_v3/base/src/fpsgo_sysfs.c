// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc.
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kobject.h>
#include <linux/proc_fs.h>

#include "fpsgo_sysfs.h"
#include "fpsgo_base.h"

#define FPSGO_SYSFS_DIR_NAME "fpsgo"

static struct kobject *fpsgo_kobj;

/*
 * ===========================================================================
 * A "MÁGICA" DO GED: Parâmetros de Módulo para Compatibilidade
 * ===========================================================================
 * Estas linhas criam os arquivos em /sys/module/mtk_fpsgo/parameters/
 * que os scripts da comunidade procuram, mesmo com o driver sendo built-in.
 */
static int adjust_loading = 1;
static int boost_affinity = 1;
static int boost_LR = 1;
static int gcc_hwui_hint = 1;
static int uboost_enhance_f = 100;
static int isolation_limit_cap;

module_param_named(adjust_loading, adjust_loading, int, 0644);
module_param_named(boost_affinity, boost_affinity, int, 0644);
module_param_named(boost_LR, boost_LR, int, 0644);
module_param_named(gcc_hwui_hint, gcc_hwui_hint, int, 0644);
module_param_named(uboost_enhance_f, uboost_enhance_f, int, 0644);
module_param_named(isolation_limit_cap, isolation_limit_cap, int, 0644);
/* Fim da seção de compatibilidade */


struct proc_dir_entry *fpsgo_debugfs_dir;
EXPORT_SYMBOL(fpsgo_debugfs_dir);

int fpsgo_sysfs_create_dir(struct kobject *parent,
                const char *name, struct kobject **ppsKobj)
{
        struct kobject *psKobj = NULL;

        if (name == NULL || ppsKobj == NULL) {
                FPSGO_LOGE("Failed to create sysfs directory %p %p\n",
                                name, ppsKobj);
                return -1;
        }

        parent = (parent != NULL) ? parent : fpsgo_kobj;
        psKobj = kobject_create_and_add(name, parent);
        if (!psKobj) {
                FPSGO_LOGE("Failed to create '%s' sysfs directory\n",
                                name);
                return -1;
        }
        *ppsKobj = psKobj;

        return 0;
}
// --------------------------------------------------
void fpsgo_sysfs_remove_dir(struct kobject **ppsKobj)
{
        if (ppsKobj == NULL)
                return;
        kobject_put(*ppsKobj);
        *ppsKobj = NULL;
}
// --------------------------------------------------
void fpsgo_sysfs_create_file(struct kobject *parent,
                struct kobj_attribute *kobj_attr)
{
        if (kobj_attr == NULL) {
                FPSGO_LOGE("Failed to create sysfs file kobj_attr=NULL\n");
                return;
        }

        parent = (parent != NULL) ? parent : fpsgo_kobj;
        if (sysfs_create_file(parent, &(kobj_attr->attr))) {
                FPSGO_LOGE("Failed to create sysfs file\n");
                return;
        }

}
// --------------------------------------------------
void fpsgo_sysfs_remove_file(struct kobject *parent,
                struct kobj_attribute *kobj_attr)
{
        if (kobj_attr == NULL)
                return;
        parent = (parent != NULL) ? parent : fpsgo_kobj;
        sysfs_remove_file(parent, &(kobj_attr->attr));
}
// --------------------------------------------------
void fpsgo_sysfs_init(void)
{
        fpsgo_kobj = kobject_create_and_add(FPSGO_SYSFS_DIR_NAME,
                        kernel_kobj);
        if (!fpsgo_kobj) {
                FPSGO_LOGE("Failed to create '%s' sysfs root directory\n",
                                FPSGO_SYSFS_DIR_NAME);
        }
}
// --------------------------------------------------
void fpsgo_sysfs_exit(void)
{
        kobject_put(fpsgo_kobj);
        fpsgo_kobj = NULL;
}
