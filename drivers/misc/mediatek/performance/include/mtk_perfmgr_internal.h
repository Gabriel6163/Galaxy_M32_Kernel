/*
 * Copyright (C) 2018 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _MTK_PERFMGR_INTERNAL_H
#define _MTK_PERFMGR_INTERNAL_H
#include "mtk_ppm_api.h"

/* PROCFS */
#define PROC_FOPS_RW(name) \
static int perfmgr_ ## name ## _proc_open(\
        struct inode *inode, struct file *file) \
{ \
        return single_open(file,\
         perfmgr_ ## name ## _proc_show, PDE_DATA(inode));\
} \
static const struct file_operations perfmgr_ ## name ## _proc_fops = { \
        .owner  = THIS_MODULE, \
        .open   = perfmgr_ ## name ## _proc_open, \
        .read   = seq_read, \
        .llseek = seq_lseek,\
        .release = single_release,\
        .write  = perfmgr_ ## name ## _proc_write,\
}

#define PROC_FOPS_RO(name) \
static int perfmgr_ ## name ## _proc_open(\
        struct inode *inode, struct file *file) \
{  \
        return single_open(file,\
         perfmgr_ ## name ## _proc_show, PDE_DATA(inode));\
}  \
static const struct file_operations perfmgr_ ## name ## _proc_fops = { \
        .owner  = THIS_MODULE, \
        .open   = perfmgr_ ## name ## _proc_open, \
        .read   = seq_read, \
        .llseek = seq_lseek,\
        .release = single_release, \
}

#define PROC_ENTRY(name) {__stringify(name), &perfmgr_ ## name ## _proc_fops}
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define for_each_perfmgr_clusters(i)    \
        for (i = 0; i < clstr_num; i++)

#define perfmgr_clusters clstr_num

#define LOG_BUF_SIZE (128)

extern int clstr_num;
extern int powerhal_tid;
extern char *perfmgr_copy_from_user_for_proc(const char __user *buffer,
                                        size_t count);

extern int check_proc_write(int *data, const char *ubuf, size_t cnt);

extern int check_group_proc_write(int *cgroup, int *data,
                                 const char *ubuf, size_t cnt);

extern void perfmgr_trace_count(int val, const char *fmt, ...);
extern void perfmgr_trace_end(void);
extern void perfmgr_trace_begin(char *name, int id, int a, int b);
extern void perfmgr_trace_printk(char *module, char *string);
extern void perfmgr_trace_log(char *module, const char *fmt, ...);
extern int perfmgr_common_userlimit_cpu_freq(unsigned int cluster_num,
                                        struct ppm_limit_data *final_freq);

/* V6 V6 V6 V6 V6 V6 V6 V6 V6 V6 V6 V6 V6 V6 V6 V6 V6 V6 V6 V6 */

/* ORK_VHEAP Light v2 - Safe Virtual Heap Accounting */
#define ORK_VHEAP_ENABLED 0 /* Set to 1 to enable */
#define ORK_VHEAP_MB_LIMIT 1536 /* Default 1.5GB limit */
#define ORK_VHEAP_VERBOSE 0 /* Set to 1 for verbose logging */
#define ORK_VHEAP_PAGES_LIMIT ((ORK_VHEAP_MB_LIMIT) * 1024 * 1024 / 4096)

/* Policy states */
enum perfmgr_policy_state {
    POLICY_ECO = 0,
    POLICY_BALANCED,
    POLICY_AGGRESSIVE,
    POLICY_THERMAL_LIMIT,
    POLICY_MAX
};

/* Subsystem availability flags */
struct perfmgr_subsystem_availability {
    bool ppm_available;
    bool qos_available;
    bool fpsgo_available;
    bool eara_thermal_available;
    bool pnpmgr_available;
    bool ged_available;
    bool gpu_available;
    bool psi_available;
    bool resym_available;
    bool thermal_available;
};

/* Perfmgr global structure */
struct perfmgr_global_data {
    struct task_struct *monitor_thread;
    struct mutex lock;
    bool enabled;
    bool debug_enabled;
    enum perfmgr_policy_state current_policy;
    enum perfmgr_policy_state prev_policy;
    
    /* Monitoring data */
    unsigned int cpu_load;
    unsigned int gpu_busy;
    unsigned int gpu_freq;
    unsigned int temp_cpu;
    unsigned int temp_gpu;
    unsigned long long last_cpu_idle;
    unsigned long long last_cpu_total;
    unsigned int psi_cpu_avg10;
    unsigned int psi_cpu_avg60;
    unsigned int psi_cpu_avg300;
    unsigned int psi_io_avg10;
    unsigned int psi_io_avg60;
    unsigned int psi_io_avg300;
    unsigned int psi_memory_avg10;
    unsigned int psi_memory_avg60;
    unsigned int psi_memory_avg300;
    
    /* State tracking */
    unsigned long last_transition_jiffies;
    unsigned long last_policy_check_jiffies;
    unsigned long boost_hold_until_jiffies;  /* Hold AGGRESSIVE for 3s */
    
    /* Subsystem states */
    bool fpsgo_boost_active;
    bool pnpmgr_boost_active;
    bool fpsgo_boost_ta_active;
    bool eara_fake_throttle;
    bool resym_ready;
    
    /* Subsystem availability */
    struct perfmgr_subsystem_availability subsys;
    
    /* Fallback tracking */
    unsigned int fallback_count;
    unsigned int retry_count;
    
    /* Manual override */
    bool manual_override;
    enum perfmgr_policy_state manual_policy;
    
    /* Hysteresis values */
    unsigned int prev_gpu_busy;
    unsigned int prev_cpu_load;
    
    /* Last warning timestamps to prevent floods */
    unsigned long last_warn_jiffies[10];  /* For different error types */
};

/* File paths for sysfs/proc access */
#define PPM_ENABLED_PATH "/proc/ppm/enabled"
#define PPM_USERLIMIT_CPU_FREQ "/proc/ppm/policy/userlimit_cpu_freq"
#define PPM_USERLIMIT_MAX_CPU_FREQ "/proc/ppm/policy/userlimit_max_cpu_freq"
#define PPM_USERLIMIT_MIN_CPU_FREQ "/proc/ppm/policy/userlimit_min_cpu_freq"
#define PPM_SYSBOOST_CLUSTER_FREQ_LIMIT "/proc/ppm/policy/sysboost_cluster_freq_limit"
#define MM_BANDWIDTH_PATH "/proc/mtk_pm_qos/mm_memory_bandwidth"
#define GPU_BUSY_PATH "/sys/kernel/gpu/gpu_busy"
#define GPU_CLOCK_PATH "/sys/kernel/gpu/gpu_clock"
#define GPU_MIN_CLOCK_PATH "/sys/kernel/gpu/gpu_min_clock"
#define GPU_MAX_CLOCK_PATH "/sys/kernel/gpu/gpu_max_clock"
#define GED_GPU_UTIL_PATH "/sys/kernel/ged/hal/gpu_utilization"
#define FPSGO_ENABLE_PATH "/sys/kernel/fpsgo/common/fpsgo_enable"
#define FPSGO_BOOST_TA_PATH "/sys/kernel/fpsgo/fbt/boost_ta"
#define PNPMGR_BOOST_ENABLE "/sys/pnpmgr/fpsgo_boost/boost_enable"
#define PNPMGR_BOOST_MODE "/sys/pnpmgr/fpsgo_boost/boost_mode"
#define EARA_FAKE_THROTTLE "/sys/kernel/eara_thermal/fake_throttle"
#define RESYM_STATE "/sys/kernel/resym/state/rs_state"
#define THERMAL_ZONE0_PATH "/sys/class/thermal/thermal_zone0/temp"
#define THERMAL_ZONE1_PATH "/sys/class/thermal/thermal_zone1/temp"
#define PSI_CPU_PATH "/proc/pressure/cpu"
#define PSI_IO_PATH "/proc/pressure/io"
#define PSI_MEMORY_PATH "/proc/pressure/memory"

/* Proc paths */
#define PERFMGR_STATUS_PATH "/proc/perfmgr_status"
#define PERFMGR_DEBUG_PATH "/proc/perfmgr_debug"
#define PERFMGR_HINT_PATH "/proc/perfmgr_hint"

/* Policy configurations */
struct policy_config {
    unsigned int gpu_min_freq;      /* kHz */
    unsigned int gpu_max_freq;      /* kHz */
    unsigned int vcore_opp;
    unsigned int bandwidth;
    unsigned int ppm_sysboost_level;  /* 0=off, 1=mid, 2=high */
    char ppm_config[32];            /* PPM policy string */
};

extern struct perfmgr_global_data perfmgr_global;

/* Function declarations */
int perfmgr_write_sysfs(const char *path, const char *value);
int perfmgr_read_sysfs(const char *path, char *buf, size_t size);
bool perfmgr_check_file_exists(const char *path);
int perfmgr_apply_policy(enum perfmgr_policy_state policy);
int perfmgr_monitor_update(void);
void perfmgr_update_cpu_load(void);
void perfmgr_update_gpu_data(void);
void perfmgr_update_thermal_data(void);
void perfmgr_update_psi_data(void);
void perfmgr_update_subsystem_states(void);
bool perfmgr_should_change_policy(void);
enum perfmgr_policy_state perfmgr_decide_policy(void);
const char* perfmgr_policy_name(enum perfmgr_policy_state policy);
int perfmgr_poll_resym_ready(void);
int perfmgr_parse_psi_avg10(const char *line, unsigned int *avg10);
int perfmgr_parse_psi_avg60(const char *line, unsigned int *avg60);
int perfmgr_parse_psi_avg300(const char *line, unsigned int *avg300);
bool perfmgr_is_gpu_freq_in_range(unsigned int freq, unsigned int min_freq, unsigned int max_freq);

/* V6 V6 V6 V6 V6 V6 V6 V6 V6 V6 V6 V6 V6 V6 V6 V6 V6 V6 V6 V6 */


/* === COMPAT LAYER (legacy API) === */
/* Preserve legacy symbols for existing modules */
// extern struct platform_device perfmgr_device; /* Se necessário, declare extern aqui também se o v6 o usar */

#endif /* _MTK_PERFMGR_INTERNAL_H */
