#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/ktime.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/ctype.h>
#include <linux/version.h>

// --- Headers e Guards ---
#ifndef _PERFMGR_V6_H_
#define _PERFMGR_V6_H_

/*
 * Nomes dos arquivos /proc criados pelo módulo v6.
 * ATENÇÃO: Estes nomes podem conflitar com módulos legados.
 * O código legado (ex: drivers/misc/mediatek/perfmgr/...) pode criar
 * /proc/perfmgr_status, /proc/perfmgr_debug, /proc/perfmgr_hint.
 * Este módulo (v6) também utiliza esses nomes para manter a UX.
 * Para evitar colisão com módulos legados carregados, os símbolos internos
 * do kernel (como a função init/exit e a estrutura global) são prefixados com 'v6'.
 * Se conflitos persistentes ocorrerem, considere alterar os nomes de /proc aqui também.
 */
#define PERFMGR_STATUS_PATH "perfmgr_status" // Mantido para UX, veja aviso acima
#define PERFMGR_DEBUG_PATH "perfmgr_debug"   // Mantido para UX, veja aviso acima
#define PERFMGR_HINT_PATH "perfmgr_hint"     // Mantido para UX, veja aviso acima

// --- Definições de Política ---
enum perfmgr_policy_state {
    POLICY_ECO = 0,
    POLICY_BALANCED,
    POLICY_OVERDRIVE,
    POLICY_THERMAL_LIMIT,
    POLICY_MAX
};

// --- Estruturas de Dados ---
struct policy_config {
    unsigned int gpu_min_freq;
    unsigned int gpu_max_freq;
    unsigned int vcore_opp;
    unsigned int bandwidth;
    unsigned int ppm_sysboost_level;
    const char *ppm_config;
};

struct perfmgr_global_data {
    struct mutex lock;
    bool enabled;
    bool debug_enabled;
    bool manual_override;
    enum perfmgr_policy_state current_policy;
    enum perfmgr_policy_state prev_policy;
    enum perfmgr_policy_state manual_policy;
    unsigned int cpu_load;
    unsigned int prev_cpu_load;
    unsigned int gpu_busy;
    unsigned int prev_gpu_busy;
    unsigned int gpu_freq; // kHz
    unsigned int temp_cpu; // m°C
    unsigned int temp_gpu; // m°C
    unsigned int psi_cpu_avg10; // 100 * valor real (0.19 -> 19)
    unsigned int psi_cpu_avg60;
    unsigned int psi_cpu_avg300;
    unsigned int psi_io_avg10;
    unsigned int psi_io_avg60;
    unsigned int psi_io_avg300;
    unsigned int psi_memory_avg10;
    unsigned int psi_memory_avg60;
    unsigned int psi_memory_avg300;
    unsigned long long last_cpu_total;
    unsigned long long last_cpu_idle;
    unsigned long last_transition_jiffies;
    unsigned long last_policy_check_jiffies;
    unsigned long boost_hold_until_jiffies;
    unsigned int fallback_count;
    unsigned int retry_count;
    bool fpsgo_boost_ta_active;
    bool pnpmgr_boost_active;
    bool eara_fake_throttle;
    bool resym_ready;
    struct task_struct *monitor_thread;
    struct {
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
    } subsys;
};

// --- Protótipos de Funções (Removidas as funções internas 'static') ---
bool perfmgr_check_file_exists(const char *path);
int perfmgr_write_sysfs(const char *path, const char *value);
int perfmgr_read_sysfs(const char *path, char *buf, size_t size);
int perfmgr_parse_psi_avg10(const char *line, unsigned int *avg10);
int perfmgr_parse_psi_avg60(const char *line, unsigned int *avg60);
int perfmgr_parse_psi_avg300(const char *line, unsigned int *avg300);
void perfmgr_update_psi_data(void);
void perfmgr_update_cpu_load(void);
void perfmgr_update_gpu_data(void);
void perfmgr_update_thermal_data(void);
void perfmgr_update_subsystem_states(void);
bool perfmgr_is_gpu_freq_in_range(unsigned int freq, unsigned int min_freq, unsigned int max_freq);
int perfmgr_apply_policy(enum perfmgr_policy_state policy);
int perfmgr_monitor_update(void);
bool perfmgr_should_change_policy(void);
enum perfmgr_policy_state perfmgr_decide_policy(void);
const char* perfmgr_policy_name(enum perfmgr_policy_state policy);
// int perfmgr_monitor_thread(void *data); // [FIX] Removido protótipo 'extern'
// int perfmgr_status_show(struct seq_file *m, void *v); // [FIX] Removido protótipo 'extern'
// int perfmgr_debug_show(struct seq_file *m, void *v); // [FIX] Removido protótipo 'extern'
// int perfmgr_status_open(struct inode *inode, struct file *file); // [FIX] Removido protótipo 'extern'
// int perfmgr_debug_open(struct inode *inode, struct file *file); // [FIX] Removido protótipo 'extern'
// static const struct file_operations perfmgr_status_fops; // [FIX] Removido protótipo 'extern'
// static const struct file_operations perfmgr_debug_fops; // [FIX] Removido protótipo 'extern'
// static const struct file_operations perfmgr_hint_fops; // [FIX] Removido protótipo 'extern'
int perfmgr_poll_resym_ready(void);
static int __init perfmgr_v6_init(void);
static void __exit perfmgr_v6_exit(void);

#endif // _PERFMGR_V6_H_

// --- Variáveis Globais ---
static struct platform_device *perfmgr_device; /* Local copy for v6 */
struct perfmgr_global_data perfmgr_global;

/* Policy configurations for MT6768 */
static const struct policy_config policy_configs[POLICY_MAX] = {
    [POLICY_ECO] = {
        .gpu_min_freq = 400000,
        .gpu_max_freq = 600000,
        .vcore_opp = 3,
        .bandwidth = 128,
        .ppm_sysboost_level = 0,
        .ppm_config = "1 1",
    },
    [POLICY_BALANCED] = {
        .gpu_min_freq = 700000,
        .gpu_max_freq = 875000,
        .vcore_opp = 2,
        .bandwidth = 2000,
        .ppm_sysboost_level = 1,
        .ppm_config = "0 0",
    },
    [POLICY_OVERDRIVE] = {
        .gpu_min_freq = 875000,
        .gpu_max_freq = 1100000,
        .vcore_opp = 0,
        .bandwidth = 4000,
        .ppm_sysboost_level = 2,
        .ppm_config = "0 0",
    },
    [POLICY_THERMAL_LIMIT] = {
        .gpu_min_freq = 400000,
        .gpu_max_freq = 550000,
        .vcore_opp = 4,
        .bandwidth = 256,
        .ppm_sysboost_level = 0,
        .ppm_config = "1 1",
    }
};

/**
 * @brief Verifica se um arquivo existe e é legível.
 * @param path Caminho do arquivo.
 * @return true se o arquivo existe e é legível, false caso contrário.
 */
bool perfmgr_check_file_exists(const char *path)
{
    struct file *filp;

    filp = filp_open(path, O_RDONLY, 0);
    if (IS_ERR(filp))
        return false;

    filp_close(filp, NULL);
    return true;
}

/**
 * @brief Escreve dados em um arquivo sysfs/proc com lógica de retry.
 * @param path Caminho do arquivo.
 * @param value Valor a ser escrito.
 * @return 0 em sucesso, código de erro negativo em falha.
 */
int perfmgr_write_sysfs(const char *path, const char *value)
{
    struct file *filp;
    loff_t pos = 0;
    int ret;
    int retries = 3;

    while (retries > 0) {
        filp = filp_open(path, O_WRONLY, 0);
        if (IS_ERR(filp)) {
            ret = PTR_ERR(filp);
            if (perfmgr_global.debug_enabled)
                pr_warn("perfmgr: failed to open %s for writing: %d\n", path, ret);
            retries--;
            if (retries > 0)
                msleep(10);
            continue;
        }

        ret = kernel_write(filp, value, strlen(value), &pos);
        filp_close(filp, NULL);

        // Verifica se a escrita foi completa
        if (ret == strlen(value)) {
            perfmgr_global.retry_count++;
            return 0;
        }

        if (perfmgr_global.debug_enabled)
            pr_warn("perfmgr: failed to write to %s: %d, retries left: %d\n", path, ret, retries - 1);

        retries--;
        if (retries > 0)
            msleep(10);
    }

    perfmgr_global.fallback_count++;
    return ret;
}

/**
 * @brief Lê dados de um arquivo sysfs/proc com tratamento de erros.
 * @param path Caminho do arquivo.
 * @param buf Buffer para armazenar os dados lidos.
 * @param size Tamanho do buffer.
 * @return 0 em sucesso, código de erro negativo em falha.
 */
int perfmgr_read_sysfs(const char *path, char *buf, size_t size)
{
    struct file *filp;
    loff_t pos = 0;
    int ret;
    int retries = 3;

    while (retries > 0) {
        filp = filp_open(path, O_RDONLY, 0);
        if (IS_ERR(filp)) {
            ret = PTR_ERR(filp);
            if (perfmgr_global.debug_enabled)
                pr_warn("perfmgr: failed to open %s for reading: %d\n", path, ret);
            retries--;
            if (retries > 0)
                msleep(10);
            continue;
        }

        ret = kernel_read(filp, buf, size - 1, &pos);
        filp_close(filp, NULL);

        if (ret > 0) {
            buf[ret] = '\0';
            /* Remove trailing newline if present */
            if (buf[ret - 1] == '\n')
                buf[ret - 1] = '\0';
            return 0;
        }

        if (perfmgr_global.debug_enabled)
            pr_warn("perfmgr: failed to read from %s: %d, retries left: %d\n", path, ret, retries - 1);

        retries--;
        if (retries > 0)
            msleep(10);
    }

    perfmgr_global.fallback_count++;
    return ret;
}

/**
 * @brief Parse PSI avg10 como porcentagem inteira (0.19 -> 19) usando apenas matemática inteira.
 * @param line Linha contendo o valor PSI.
 * @param avg10 Ponteiro para armazenar o valor parseado.
 * @return 0 em sucesso, -1 em erro.
 */
int perfmgr_parse_psi_avg10(const char *line, unsigned int *avg10)
{
    const char *avg10_str = strstr(line, "avg10=");
    if (!avg10_str)
        return -1;

    avg10_str += 6; /* Skip "avg10=" */

    /* Skip leading whitespace */
    while (*avg10_str == ' ' || *avg10_str == '\t')
        avg10_str++;

    if (!*avg10_str)
        return -1;

    /* Parse integer part */
    unsigned long int_part = 0;
    const char *ptr = avg10_str;

    /* Parse integer part before decimal point */
    while (*ptr >= '0' && *ptr <= '9') {
        int_part = int_part * 10 + (*ptr - '0');
        ptr++;
    }

    /* Check for decimal point */
    if (*ptr == '.') {
        ptr++;
        unsigned long decimal_part = 0;
        unsigned long decimal_places = 0;

        /* Parse up to 3 decimal digits */
        while (*ptr >= '0' && *ptr <= '9' && decimal_places < 3) {
            decimal_part = decimal_part * 10 + (*ptr - '0');
            decimal_places++;
            ptr++;
        }

        /* Convert to percentage: multiply by 100 */
        unsigned long multiplier = 1;
        unsigned int i; /* kernel-safe loop variable */
        for (i = 0; i < decimal_places; i++)
            multiplier *= 10;

        *avg10 = (unsigned int)(int_part * 100 + (decimal_part * 100) / multiplier);
    } else {
        /* No decimal point - treat as integer percentage */
        *avg10 = (unsigned int)(int_part * 100);
    }

    return 0;
}

/**
 * @brief Parse PSI avg60 como porcentagem inteira.
 * @param line Linha contendo o valor PSI.
 * @param avg60 Ponteiro para armazenar o valor parseado.
 * @return 0 em sucesso, -1 em erro.
 */
int perfmgr_parse_psi_avg60(const char *line, unsigned int *avg60)
{
    const char *avg60_str = strstr(line, "avg60=");
    if (!avg60_str)
        return -1;

    avg60_str += 6; /* Skip "avg60=" */

    /* Skip leading whitespace */
    while (*avg60_str == ' ' || *avg60_str == '\t')
        avg60_str++;

    if (!*avg60_str)
        return -1;

    /* Parse integer part */
    unsigned long int_part = 0;
    const char *ptr = avg60_str;

    /* Parse integer part before decimal point */
    while (*ptr >= '0' && *ptr <= '9') {
        int_part = int_part * 10 + (*ptr - '0');
        ptr++;
    }

    /* Check for decimal point */
    if (*ptr == '.') {
        ptr++;
        unsigned long decimal_part = 0;
        unsigned long decimal_places = 0;

        /* Parse up to 3 decimal digits */
        unsigned long i; /* kernel-safe loop variable */
        while (*ptr >= '0' && *ptr <= '9' && decimal_places < 3) {
            decimal_part = decimal_part * 10 + (*ptr - '0');
            decimal_places++;
            ptr++;
        }

        /* Convert to percentage: multiply by 100 */
        unsigned long multiplier = 1;
        for (i = 0; i < decimal_places; i++)
            multiplier *= 10;

        *avg60 = (unsigned int)(int_part * 100 + (decimal_part * 100) / multiplier);
    } else {
        /* No decimal point - treat as integer percentage */
        *avg60 = (unsigned int)(int_part * 100);
    }

    return 0;
}

/**
 * @brief Parse PSI avg300 como porcentagem inteira.
 * @param line Linha contendo o valor PSI.
 * @param avg300 Ponteiro para armazenar o valor parseado.
 * @return 0 em sucesso, -1 em erro.
 */
int perfmgr_parse_psi_avg300(const char *line, unsigned int *avg300)
{
    const char *avg300_str = strstr(line, "avg300=");
    if (!avg300_str)
        return -1;

    avg300_str += 7; /* Skip "avg300=" */

    /* Skip leading whitespace */
    while (*avg300_str == ' ' || *avg300_str == '\t')
        avg300_str++;

    if (!*avg300_str)
        return -1;

    /* Parse integer part */
    unsigned long int_part = 0;
    const char *ptr = avg300_str;

    /* Parse integer part before decimal point */
    unsigned long i; /* kernel-safe loop variable */
    while (*ptr >= '0' && *ptr <= '9') {
        int_part = int_part * 10 + (*ptr - '0');
        ptr++;
    }

    /* Check for decimal point */
    if (*ptr == '.') {
        ptr++;
        unsigned long decimal_part = 0;
        unsigned long decimal_places = 0;

        /* Parse up to 3 decimal digits */
        while (*ptr >= '0' && *ptr <= '9' && decimal_places < 3) {
            decimal_part = decimal_part * 10 + (*ptr - '0');
            decimal_places++;
            ptr++;
        }

        /* Convert to percentage: multiply by 100 */
        unsigned long multiplier = 1;
        for (i = 0; i < decimal_places; i++)
            multiplier *= 10;

        *avg300 = (unsigned int)(int_part * 100 + (decimal_part * 100) / multiplier);
    } else {
        /* No decimal point - treat as integer percentage */
        *avg300 = (unsigned int)(int_part * 100);
    }

    return 0;
}

/**
 * @brief Atualiza os dados de pressão PSI com parsing inteiro.
 */
void perfmgr_update_psi_data(void)
{
    char buf[512];
    char *line;
    char *saveptr;

    /* Read PSI CPU pressure */
    if (perfmgr_read_sysfs("/proc/pressure/cpu", buf, sizeof(buf)) == 0) {
        line = buf;
        while ((saveptr = strchr(line, '\n')) != NULL) {
            *saveptr = '\0';
            if (strstr(line, "some")) {
                perfmgr_parse_psi_avg10(line, &perfmgr_global.psi_cpu_avg10);
                perfmgr_parse_psi_avg60(line, &perfmgr_global.psi_cpu_avg60);
                perfmgr_parse_psi_avg300(line, &perfmgr_global.psi_cpu_avg300);
                break;
            }
            line = saveptr + 1;
        }
    }

    /* Read PSI IO pressure */
    if (perfmgr_read_sysfs("/proc/pressure/io", buf, sizeof(buf)) == 0) {
        line = buf;
        while ((saveptr = strchr(line, '\n')) != NULL) {
            *saveptr = '\0';
            if (strstr(line, "some")) {
                perfmgr_parse_psi_avg10(line, &perfmgr_global.psi_io_avg10);
                perfmgr_parse_psi_avg60(line, &perfmgr_global.psi_io_avg60);
                perfmgr_parse_psi_avg300(line, &perfmgr_global.psi_io_avg300);
                break;
            }
            line = saveptr + 1;
        }
    }

    /* Read PSI Memory pressure */
    if (perfmgr_read_sysfs("/proc/pressure/memory", buf, sizeof(buf)) == 0) {
        line = buf;
        while ((saveptr = strchr(line, '\n')) != NULL) {
            *saveptr = '\0';
            if (strstr(line, "some")) {
                perfmgr_parse_psi_avg10(line, &perfmgr_global.psi_memory_avg10);
                perfmgr_parse_psi_avg60(line, &perfmgr_global.psi_memory_avg60);
                perfmgr_parse_psi_avg300(line, &perfmgr_global.psi_memory_avg300);
                break;
            }
            line = saveptr + 1;
        }
    }

    /* Log parsed values for verification if debug is enabled */
    if (perfmgr_global.debug_enabled) {
        pr_info("[perfmgr_v6] PSI CPU: %u.%u%%/%u.%u%%/%u.%u%% | IO: %u.%u%%/%u.%u%%/%u.%u%% | Memory: %u.%u%%/%u.%u%%/%u.%u%%\n",
                perfmgr_global.psi_cpu_avg10 / 100, perfmgr_global.psi_cpu_avg10 % 100,
                perfmgr_global.psi_cpu_avg60 / 100, perfmgr_global.psi_cpu_avg60 % 100,
                perfmgr_global.psi_cpu_avg300 / 100, perfmgr_global.psi_cpu_avg300 % 100,
                perfmgr_global.psi_io_avg10 / 100, perfmgr_global.psi_io_avg10 % 100,
                perfmgr_global.psi_io_avg60 / 100, perfmgr_global.psi_io_avg60 % 100,
                perfmgr_global.psi_io_avg300 / 100, perfmgr_global.psi_io_avg300 % 100,
                perfmgr_global.psi_memory_avg10 / 100, perfmgr_global.psi_memory_avg10 % 100,
                perfmgr_global.psi_memory_avg60 / 100, perfmgr_global.psi_memory_avg60 % 100,
                perfmgr_global.psi_memory_avg300 / 100, perfmgr_global.psi_memory_avg300 % 100);
    }
}

/**
 * @brief Atualiza o cálculo de carga da CPU com delta.
 */
void perfmgr_update_cpu_load(void)
{
    char buf[256];
    unsigned long long user, nice, system, idle, iowait, irq, softirq;
    unsigned long long curr_idle, curr_total;
    unsigned long long total_diff, idle_diff;

    if (perfmgr_read_sysfs("/proc/stat", buf, sizeof(buf)) < 0)
        return;

    if (sscanf(buf, "cpu %llu %llu %llu %llu %llu %llu %llu",
               &user, &nice, &system, &idle, &iowait, &irq, &softirq) < 4)
        return;

    curr_idle = idle + iowait;
    curr_total = user + nice + system + idle + iowait + irq + softirq;

    if (perfmgr_global.last_cpu_total != 0) {
        total_diff = curr_total - perfmgr_global.last_cpu_total;
        idle_diff = curr_idle - perfmgr_global.last_cpu_idle;

        if (total_diff > 0) {
            perfmgr_global.prev_cpu_load = perfmgr_global.cpu_load; // Atualiza delta
            perfmgr_global.cpu_load = (unsigned int)
                ((total_diff - idle_diff) * 100 / total_diff);
        }
    }

    perfmgr_global.last_cpu_idle = curr_idle;
    perfmgr_global.last_cpu_total = curr_total;
}

/**
 * @brief Atualiza dados da GPU com fallback.
 */
void perfmgr_update_gpu_data(void)
{
    char buf[64];
    int value;

    /* Try primary GED path for GPU utilization */
    if (perfmgr_global.subsys.ged_available &&
        perfmgr_read_sysfs("/proc/ged/hal/current_utilization", buf, sizeof(buf)) == 0) {
        // Exemplo de saída de GED: "0, 60, 0, 0, 0, 0" -> pegamos o segundo valor (GPU Busy)
        char *ptr = buf;
        char *token = strsep(&ptr, ","); // kernel-safe parse
        token = strsep(&ptr, ","); // Pula o primeiro, pega o segundo (GPU Busy)
        if (token && kstrtoint(token, 10, &value) == 0) {
            perfmgr_global.prev_gpu_busy = perfmgr_global.gpu_busy; // Atualiza delta
            perfmgr_global.gpu_busy = (unsigned int)value;
        }
    } else if (perfmgr_global.subsys.gpu_available &&
               perfmgr_read_sysfs("/proc/gpufreq/gpufreq_opp_freq", buf, sizeof(buf)) == 0) {
        // Exemplo de saída de gpufreq: "Current OPP: 0, Freq: 875000 kHz"
        char *freq_str = strstr(buf, "Freq:");
        if (freq_str) {
            freq_str += 5; // Pula "Freq:"
            while (*freq_str == ' ')
                freq_str++;
            if (kstrtoint(freq_str, 10, &value) == 0) {
                perfmgr_global.gpu_freq = (unsigned int)value; // kHz
            }
        }
    } else {
        /* Fallback to constant 60% if all paths fail */
        perfmgr_global.prev_gpu_busy = perfmgr_global.gpu_busy; // Atualiza delta
        perfmgr_global.gpu_busy = 60;
        perfmgr_global.fallback_count++;
    }
}

/**
 * @brief Atualiza dados térmicos.
 */
void perfmgr_update_thermal_data(void)
{
    char buf[64];
    int value;

    /* Read CPU temperature */
    if (perfmgr_global.subsys.thermal_available &&
        perfmgr_read_sysfs("/sys/class/thermal/thermal_zone0/temp", buf, sizeof(buf)) == 0) {
        if (kstrtoint(buf, 10, &value) == 0) {
            // Verifica se o valor já está em milicelsius
            if (value < 1000)
                value *= 1000; // Converte para m°C se necessário
            perfmgr_global.temp_cpu = (unsigned int)value;
        }
    }

    /* Read GPU temperature */
    if (perfmgr_global.subsys.thermal_available &&
        perfmgr_read_sysfs("/sys/class/thermal/thermal_zone1/temp", buf, sizeof(buf)) == 0) {
        if (kstrtoint(buf, 10, &value) == 0) {
            // Verifica se o valor já está em milicelsius
            if (value < 1000)
                value *= 1000; // Converte para m°C se necessário
            perfmgr_global.temp_gpu = (unsigned int)value;
        }
    }
}

/**
 * @brief Atualiza estados de subsistemas com detecção aprimorada de boost.
 */
void perfmgr_update_subsystem_states(void)
{
    char buf[64];
    int value;

    /* Check FPSGO boost state */
    if (perfmgr_global.subsys.fpsgo_available) {
        /* Check boost_ta flag */
        if (perfmgr_read_sysfs("/proc/fpsgo/boost_ta", buf, sizeof(buf)) == 0) {
            if (kstrtoint(buf, 10, &value) == 0) {
                bool new_ta_state = (value > 0);
                if (new_ta_state && !perfmgr_global.fpsgo_boost_ta_active) {
                    perfmgr_global.last_policy_check_jiffies = jiffies;
                }
                perfmgr_global.fpsgo_boost_ta_active = new_ta_state;
            }
        }
    }

    /* Check PNPMGR boost states */
    bool new_pnpmgr_boost = false;
    if (perfmgr_global.subsys.pnpmgr_available) {
        /* Check boost_enable */
        if (perfmgr_read_sysfs("/proc/pnpmgr/boost_enable", buf, sizeof(buf)) == 0) {
            if (kstrtoint(buf, 10, &value) == 0) {
                new_pnpmgr_boost = (value > 0);
            }
        }

        /* Also check boost_mode */
        if (perfmgr_read_sysfs("/proc/pnpmgr/boost_mode", buf, sizeof(buf)) == 0) {
            if (kstrtoint(buf, 10, &value) == 0) {
                new_pnpmgr_boost = new_pnpmgr_boost || (value > 0);
            }
        }
    }

    /* Update boost active state */
    if (new_pnpmgr_boost && !perfmgr_global.pnpmgr_boost_active) {
        perfmgr_global.boost_hold_until_jiffies = jiffies + 3 * HZ; /* Hold for 3 seconds */
    }
    perfmgr_global.pnpmgr_boost_active = new_pnpmgr_boost;

    /* Check EARA thermal fake throttle */
    if (perfmgr_global.subsys.eara_thermal_available &&
        perfmgr_read_sysfs("/proc/eara_thermal/fake_throttle", buf, sizeof(buf)) == 0) {
        if (kstrtoint(buf, 10, &value) == 0) {
            perfmgr_global.eara_fake_throttle = (value > 0);
        }
    }

    /* Check RESYM state */
    if (perfmgr_global.subsys.resym_available &&
        perfmgr_read_sysfs("/proc/resym/state", buf, sizeof(buf)) == 0) {
        if (kstrtoint(buf, 10, &value) == 0) {
            perfmgr_global.resym_ready = (value == 1);
        }
    }
}

/**
 * @brief Verifica se a frequência da GPU está dentro do intervalo alvo.
 * @param freq Frequência atual.
 * @param min_freq Frequência mínima.
 * @param max_freq Frequência máxima.
 * @return true se estiver no intervalo, false caso contrário.
 */
bool perfmgr_is_gpu_freq_in_range(unsigned int freq, unsigned int min_freq, unsigned int max_freq)
{
    return (freq >= min_freq && freq <= max_freq);
}

/**
 * @brief Aplica uma política ao sistema com verificação de erros e fallback.
 * @param policy Política a ser aplicada.
 * @return 0 em sucesso, código de erro negativo em falha.
 */
int perfmgr_apply_policy(enum perfmgr_policy_state policy)
{
    char buf[64];
    const struct policy_config *config = &policy_configs[policy];
    int ret = 0;

    /* Apply GPU min clock (only if not already in range) */
    if (perfmgr_global.subsys.gpu_available &&
        !perfmgr_is_gpu_freq_in_range(perfmgr_global.gpu_freq, config->gpu_min_freq, config->gpu_max_freq)) {
        snprintf(buf, sizeof(buf), "%u", config->gpu_min_freq);
        if (perfmgr_write_sysfs("/proc/gpufreq/gpufreq_opp_freq", buf) < 0)
            ret = -EIO;
    }

    /* Apply GPU max clock (only if not already in range) */
    if (perfmgr_global.subsys.gpu_available &&
        !perfmgr_is_gpu_freq_in_range(perfmgr_global.gpu_freq, config->gpu_min_freq, config->gpu_max_freq)) {
        snprintf(buf, sizeof(buf), "%u", config->gpu_max_freq);
        if (perfmgr_write_sysfs("/proc/gpufreq/gpufreq_opp_freq", buf) < 0)
            ret = -EIO;
    }

    /* Apply vcore_opp (if QoS available) */
    if (perfmgr_global.subsys.qos_available) {
        snprintf(buf, sizeof(buf), "%u", config->vcore_opp);
        if (perfmgr_write_sysfs("/proc/mtk_pm_qos/vcore_opp", buf) < 0)
            ret = -EIO;
    }

    /* Apply memory bandwidth */
    if (perfmgr_global.subsys.qos_available) {
        snprintf(buf, sizeof(buf), "%u", config->bandwidth);
        if (perfmgr_write_sysfs("/proc/mtk_qos/bw_hrt", buf) < 0)
            ret = -EIO;
    }

    /* Apply PPM CPU frequency limit */
    if (perfmgr_global.subsys.ppm_available) {
        if (perfmgr_write_sysfs("/proc/ppm/policy_userlimit_freq", config->ppm_config) < 0)
            ret = -EIO;
    }

    /* Apply PPM sysboost cluster freq limit */
    if (perfmgr_global.subsys.ppm_available) {
        snprintf(buf, sizeof(buf), "%u %u", config->ppm_sysboost_level, config->ppm_sysboost_level);
        if (perfmgr_write_sysfs("/proc/ppm/policy_sysboost_freq", buf) < 0)
            ret = -EIO;
    }

    return ret;
}

/**
 * @brief Função de atualização do monitor - lê todos os sensores e atualiza dados.
 * @return 0 em sucesso.
 */
int perfmgr_monitor_update(void)
{
    perfmgr_update_cpu_load();
    perfmgr_update_gpu_data();
    perfmgr_update_thermal_data();
    perfmgr_update_psi_data();
    perfmgr_update_subsystem_states();

    return 0;
}

/**
 * @brief Determina se a política deve mudar (anti-flicker).
 * @return true se a política deve mudar, false caso contrário.
 */
bool perfmgr_should_change_policy(void)
{
    unsigned long time_since_last = jiffies - perfmgr_global.last_transition_jiffies;
    unsigned int cpu_delta = abs((int)perfmgr_global.cpu_load - (int)perfmgr_global.prev_cpu_load);
    unsigned int gpu_delta = abs((int)perfmgr_global.gpu_busy - (int)perfmgr_global.prev_gpu_busy);

    /* Force change if thermal limit state changed or EARA throttle is active */
    if (perfmgr_global.eara_fake_throttle ||
        perfmgr_global.temp_cpu > 95000 ||
        perfmgr_global.temp_gpu > 95000)
        return true;

    /* Check if boost hold is active */
    if (time_before(jiffies, perfmgr_global.boost_hold_until_jiffies))
        return false; /* Don't change policy while boost is being held */

    /* Allow change if enough time has passed or significant load change */
    if (time_since_last > 3 * HZ / 10) /* 300ms minimum cooldown */
        return true;

    if (cpu_delta > 10 || gpu_delta > 10)
        return true;

    return false;
}

/**
 * @brief Determina a próxima política com base nas métricas atuais.
 * @return A política a ser aplicada.
 */
enum perfmgr_policy_state perfmgr_decide_policy(void)
{
    /* Emergency thermal limit if temps exceed 95°C */
    if (perfmgr_global.temp_cpu > 95000 || perfmgr_global.temp_gpu > 95000)
        return POLICY_THERMAL_LIMIT;

    /* Check EARA thermal fake throttle */
    if (perfmgr_global.eara_fake_throttle)
        return POLICY_THERMAL_LIMIT;

    /* Check PSI pressure (system under heavy pressure) */
    if (perfmgr_global.psi_cpu_avg10 > 50 || perfmgr_global.psi_memory_avg10 > 50)
        return POLICY_OVERDRIVE; // PSI alto -> OVERDRIVE

    /* Check for FPSGO/PNPMGR boost requests */
    if (perfmgr_global.fpsgo_boost_ta_active || perfmgr_global.pnpmgr_boost_active)
        return POLICY_OVERDRIVE;

    /* OVERDRIVE policy check: GPU busy > 80% ou PSI alto */
    if (perfmgr_global.gpu_busy > 80 || perfmgr_global.psi_io_avg10 > 50)
        return POLICY_OVERDRIVE;

    /* Balanced policy check */
    if (perfmgr_global.gpu_busy > 40 || perfmgr_global.cpu_load > 35 || perfmgr_global.psi_io_avg10 > 20)
        return POLICY_BALANCED;

    /* If temp > 85°C, don't allow OVERDRIVE mode */
    if (perfmgr_global.temp_cpu > 85000 || perfmgr_global.temp_gpu > 85000) {
        if (perfmgr_global.current_policy == POLICY_OVERDRIVE)
            return POLICY_BALANCED; /* Smooth down from OVERDRIVE */
    }

    /* Default to ECO if CPU load < 25% and PSI baixo */
    if (perfmgr_global.cpu_load < 25 && perfmgr_global.psi_cpu_avg10 < 10)
        return POLICY_ECO;

    /* Fallback to BALANCED */
    return POLICY_BALANCED;
}

/**
 * @brief Obtém o nome da política como string.
 * @param policy Política.
 * @return Nome da política.
 */
const char* perfmgr_policy_name(enum perfmgr_policy_state policy)
{
    switch (policy) {
    case POLICY_ECO:
        return "ECO";
    case POLICY_BALANCED:
        return "BALANCED";
    case POLICY_OVERDRIVE:
        return "OVERDRIVE";
    case POLICY_THERMAL_LIMIT:
        return "THERMAL_LIMIT";
    default:
        return "UNKNOWN";
    }
}

/**
 * @brief Função principal da thread do monitor.
 * @param data Ponteiro de dados (não utilizado).
 * @return Código de saída da thread.
 */
static int perfmgr_monitor_thread(void *data) // [FIX] Removido protótipo conflitante, definido como static
{
    enum perfmgr_policy_state new_policy;

    while (!kthread_should_stop() && perfmgr_global.enabled) {
        mutex_lock(&perfmgr_global.lock);

        /* Update monitoring data */
        perfmgr_monitor_update();

        /* Use manual override if set */
        if (perfmgr_global.manual_override) {
            new_policy = perfmgr_global.manual_policy;
        } else {
            /* Decide new policy */
            new_policy = perfmgr_decide_policy();
        }

        /* Apply new policy if different from current and conditions allow */
        if (new_policy != perfmgr_global.current_policy && perfmgr_should_change_policy()) {
            if (perfmgr_apply_policy(new_policy) == 0) {
                perfmgr_global.prev_policy = perfmgr_global.current_policy;
                perfmgr_global.current_policy = new_policy;
                perfmgr_global.last_transition_jiffies = jiffies;

                if (perfmgr_global.debug_enabled) {
                    pr_info("[perfmgr_v6] policy %s → %s | CPU %u%% | GPU %u%% | PSI %u/%u/%u | TEMP %u°C/%u°C | FPSGO %d | PNPMGR %d\n",
                            perfmgr_policy_name(perfmgr_global.prev_policy),
                            perfmgr_policy_name(perfmgr_global.current_policy),
                            perfmgr_global.cpu_load,
                            perfmgr_global.gpu_busy,
                            perfmgr_global.psi_cpu_avg10,
                            perfmgr_global.psi_io_avg10,
                            perfmgr_global.psi_memory_avg10,
                            perfmgr_global.temp_cpu / 1000,
                            perfmgr_global.temp_gpu / 1000,
                            perfmgr_global.fpsgo_boost_ta_active,
                            perfmgr_global.pnpmgr_boost_active);
                }
            }
        }

        mutex_unlock(&perfmgr_global.lock);

        /* Sleep for 200-300ms before next update */
        usleep_range(200000, 300000);
    }

    return 0;
}

/**
 * @brief Função show para o arquivo de status do proc.
 * @param m Arquivo de sequência.
 * @param v Dados de entrada (não utilizado).
 * @return 0 em sucesso.
 */
static int perfmgr_status_show(struct seq_file *m, void *v) // [FIX] Removido protótipo conflitante, definido como static
{
    mutex_lock(&perfmgr_global.lock);

    seq_printf(m, "perfmgr_v6_intparse_safe status:\n");
    seq_printf(m, "Enabled: %s\n", perfmgr_global.enabled ? "yes" : "no");
    seq_printf(m, "Debug: %s\n", perfmgr_global.debug_enabled ? "yes" : "no");
    seq_printf(m, "Current Policy: %s (%d)\n",
               perfmgr_policy_name(perfmgr_global.current_policy),
               perfmgr_global.current_policy);
    seq_printf(m, "Previous Policy: %s (%d)\n",
               perfmgr_policy_name(perfmgr_global.prev_policy),
               perfmgr_global.prev_policy);
    seq_printf(m, "Manual Override: %s\n", perfmgr_global.manual_override ? "yes" : "no");
    seq_printf(m, "Manual Policy: %s\n",
               perfmgr_policy_name(perfmgr_global.manual_policy));
    seq_printf(m, "CPU Load: %u%% (prev: %u%%)\n",
               perfmgr_global.cpu_load, perfmgr_global.prev_cpu_load);
    seq_printf(m, "GPU Busy: %u%% (prev: %u%%)\n",
               perfmgr_global.gpu_busy, perfmgr_global.prev_gpu_busy);
    seq_printf(m, "GPU Freq: %u kHz\n", perfmgr_global.gpu_freq);
    seq_printf(m, "CPU Temp: %u.%u°C\n",
               perfmgr_global.temp_cpu / 1000, (perfmgr_global.temp_cpu % 1000) / 10);
    seq_printf(m, "GPU Temp: %u.%u°C\n",
               perfmgr_global.temp_gpu / 1000, (perfmgr_global.temp_gpu % 1000) / 10);
    seq_printf(m, "PSI CPU: avg10=%u.%u%%, avg60=%u.%u%%, avg300=%u.%u%%\n",
               perfmgr_global.psi_cpu_avg10 / 100, perfmgr_global.psi_cpu_avg10 % 100,
               perfmgr_global.psi_cpu_avg60 / 100, perfmgr_global.psi_cpu_avg60 % 100,
               perfmgr_global.psi_cpu_avg300 / 100, perfmgr_global.psi_cpu_avg300 % 100);
    seq_printf(m, "PSI IO: avg10=%u.%u%%, avg60=%u.%u%%, avg300=%u.%u%%\n",
               perfmgr_global.psi_io_avg10 / 100, perfmgr_global.psi_io_avg10 % 100,
               perfmgr_global.psi_io_avg60 / 100, perfmgr_global.psi_io_avg60 % 100,
               perfmgr_global.psi_io_avg300 / 100, perfmgr_global.psi_io_avg300 % 100);
    seq_printf(m, "PSI Memory: avg10=%u.%u%%, avg60=%u.%u%%, avg300=%u.%u%%\n",
               perfmgr_global.psi_memory_avg10 / 100, perfmgr_global.psi_memory_avg10 % 100,
               perfmgr_global.psi_memory_avg60 / 100, perfmgr_global.psi_memory_avg60 % 100,
               perfmgr_global.psi_memory_avg300 / 100, perfmgr_global.psi_memory_avg300 % 100);
    seq_printf(m, "FPSGO Boost TA: %s\n", perfmgr_global.fpsgo_boost_ta_active ? "active" : "inactive");
    seq_printf(m, "PNPMGR Boost: %s\n", perfmgr_global.pnpmgr_boost_active ? "active" : "inactive");
    seq_printf(m, "EARA Fake Throttle: %s\n", perfmgr_global.eara_fake_throttle ? "active" : "inactive");
    seq_printf(m, "RESYM Ready: %s\n", perfmgr_global.resym_ready ? "yes" : "no");
    seq_printf(m, "Fallback Count: %u\n", perfmgr_global.fallback_count);
    seq_printf(m, "Retry Count: %u\n", perfmgr_global.retry_count);
    seq_printf(m, "Time since last transition: %lu ms\n",
               jiffies_to_msecs(jiffies - perfmgr_global.last_transition_jiffies));
    seq_printf(m, "Boost hold until: %lu ms\n",
               jiffies_to_msecs(perfmgr_global.boost_hold_until_jiffies));

    /* Show subsystem availability */
    seq_printf(m, "\nSubsystem Availability:\n");
    seq_printf(m, "PPM: %s, QoS: %s, FPSGO: %s, EARA: %s, PNPMGR: %s\n",
               perfmgr_global.subsys.ppm_available ? "yes" : "no",
               perfmgr_global.subsys.qos_available ? "yes" : "no",
               perfmgr_global.subsys.fpsgo_available ? "yes" : "no",
               perfmgr_global.subsys.eara_thermal_available ? "yes" : "no",
               perfmgr_global.subsys.pnpmgr_available ? "yes" : "no");
    seq_printf(m, "GED: %s, GPU: %s, PSI: %s, RESYM: %s, Thermal: %s\n",
               perfmgr_global.subsys.ged_available ? "yes" : "no",
               perfmgr_global.subsys.gpu_available ? "yes" : "no",
               perfmgr_global.subsys.psi_available ? "yes" : "no",
               perfmgr_global.subsys.resym_available ? "yes" : "no",
               perfmgr_global.subsys.thermal_available ? "yes" : "no");

    mutex_unlock(&perfmgr_global.lock);
    return 0;
}

static int perfmgr_status_open(struct inode *inode, struct file *file) // [FIX] Removido protótipo conflitante, definido como static
{
    return single_open(file, perfmgr_status_show, NULL);
}

static const struct file_operations perfmgr_status_fops = {
    .open = perfmgr_status_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

// [FIX] Função show específica para o arquivo de debug
static int perfmgr_debug_show(struct seq_file *m, void *v) // [FIX] Removido protótipo conflitante, definido como static
{
    mutex_lock(&perfmgr_global.lock);
    seq_printf(m, "%d\n", perfmgr_global.debug_enabled ? 1 : 0);
    mutex_unlock(&perfmgr_global.lock);
    return 0;
}

static int perfmgr_debug_open(struct inode *inode, struct file *file) // [FIX] Removido protótipo conflitante, definido como static
{
    return single_open(file, perfmgr_debug_show, NULL); // [FIX] Chama a função show correta
}

static ssize_t perfmgr_debug_write(struct file *file, const char __user *buffer,
                                   size_t count, loff_t *pos)
{
    char buf[10];
    int value;

    if (count >= sizeof(buf))
        return -EINVAL;

    if (copy_from_user(buf, buffer, count))
        return -EFAULT;

    buf[count] = '\0';
    if (kstrtoint(buf, 10, &value) < 0)
        return -EINVAL;

    mutex_lock(&perfmgr_global.lock);
    perfmgr_global.debug_enabled = (value != 0);
    mutex_unlock(&perfmgr_global.lock);

    return count;
}

// [FIX] Definição correta da estrutura file_operations para debug
static const struct file_operations perfmgr_debug_fops = {
    .open = perfmgr_debug_open,
    .read = seq_read, // Usa seq_read porque single_open associa a função show
    .write = perfmgr_debug_write,
    .llseek = seq_lseek,
    .release = single_release,
};

static ssize_t perfmgr_hint_write(struct file *file, const char __user *buffer,
                                  size_t count, loff_t *pos)
{
    char buf[32];
    enum perfmgr_policy_state policy = POLICY_MAX;

    if (count >= sizeof(buf))
        return -EINVAL;

    if (copy_from_user(buf, buffer, count))
        return -EFAULT;

    buf[count] = '\0';

    /* Remove trailing newline */
    if (buf[count - 1] == '\n')
        buf[count - 1] = '\0';

    /* Parse policy hint */
    if (strncmp(buf, "eco", 3) == 0)
        policy = POLICY_ECO;
    else if (strncmp(buf, "balanced", 8) == 0)
        policy = POLICY_BALANCED;
    else if (strncmp(buf, "overdrive", 9) == 0)
        policy = POLICY_OVERDRIVE;
    else if (strncmp(buf, "thermal_limit", 13) == 0)
        policy = POLICY_THERMAL_LIMIT;
    else if (strncmp(buf, "auto", 4) == 0)
        policy = POLICY_MAX; /* Special case to disable manual override */

    if (policy != POLICY_MAX) {
        mutex_lock(&perfmgr_global.lock);
        if (policy == POLICY_MAX) {
            /* Disable manual override */
            perfmgr_global.manual_override = false;
        } else {
            perfmgr_global.manual_policy = policy;
            perfmgr_global.manual_override = true;
        }
        mutex_unlock(&perfmgr_global.lock);
    }

    return count;
}

static const struct file_operations perfmgr_hint_fops = {
    .write = perfmgr_hint_write,
};

int perfmgr_poll_resym_ready(void)
{
    char buf[16];
    int value;
    int attempts = 10; /* 5 seconds with 500ms intervals */

    while (attempts > 0) {
        if (perfmgr_read_sysfs("/proc/resym/state", buf, sizeof(buf)) == 0) {
            if (kstrtoint(buf, 10, &value) == 0 && value == 1) {
                pr_info("perfmgr: RESYM is ready\n");
                return 0; /* Ready */
            }
        }
        attempts--;
        msleep(500);
    }

    return -ETIMEDOUT;
}

static int __init perfmgr_v6_init(void)
{
    int ret;

    /* Initialize global data */
    memset(&perfmgr_global, 0, sizeof(perfmgr_global));
    mutex_init(&perfmgr_global.lock);
    perfmgr_global.enabled = true;
    perfmgr_global.current_policy = POLICY_ECO;
    perfmgr_global.prev_policy = POLICY_ECO;
    perfmgr_global.last_transition_jiffies = jiffies;
    perfmgr_global.eara_fake_throttle = false;
    perfmgr_global.resym_ready = true; /* Assume ready unless RESYM is available */

    /* Check subsystem availability */
    perfmgr_global.subsys.ppm_available = perfmgr_check_file_exists("/proc/ppm/policy_userlimit_freq");
    perfmgr_global.subsys.qos_available = perfmgr_check_file_exists("/proc/mtk_qos/bw_hrt");
    perfmgr_global.subsys.fpsgo_available = perfmgr_check_file_exists("/proc/fpsgo/boost_ta");
    perfmgr_global.subsys.eara_thermal_available = perfmgr_check_file_exists("/proc/eara_thermal/fake_throttle");
    perfmgr_global.subsys.pnpmgr_available = perfmgr_check_file_exists("/proc/pnpmgr/boost_enable");
    perfmgr_global.subsys.ged_available = perfmgr_check_file_exists("/proc/ged/hal/current_utilization");
    perfmgr_global.subsys.gpu_available = perfmgr_check_file_exists("/proc/gpufreq/gpufreq_opp_freq");
    perfmgr_global.subsys.psi_available = perfmgr_check_file_exists("/proc/pressure/cpu");
    perfmgr_global.subsys.resym_available = perfmgr_check_file_exists("/proc/resym/state");
    perfmgr_global.subsys.thermal_available = perfmgr_check_file_exists("/sys/class/thermal/thermal_zone0/temp");

    /* Poll RESYM if available */
    if (perfmgr_global.subsys.resym_available) {
        ret = perfmgr_poll_resym_ready();
        if (ret == -ETIMEDOUT) {
            pr_warn("perfmgr: RESYM not ready after 5 seconds, continuing anyway\n");
            perfmgr_global.resym_ready = false;
        } else {
            perfmgr_global.resym_ready = true;
        }
    }

    /* Create proc entries */
    if (!proc_create(PERFMGR_STATUS_PATH, 0444, NULL, &perfmgr_status_fops)) {
        pr_err("perfmgr: failed to create status proc entry\n");
        return -ENOMEM;
    }

    if (!proc_create(PERFMGR_DEBUG_PATH, 0666, NULL, &perfmgr_debug_fops)) { // [FIX] Usa a estrutura correta
        pr_err("perfmgr: failed to create debug proc entry\n");
        remove_proc_entry(PERFMGR_STATUS_PATH, NULL);
        return -ENOMEM;
    }

    if (!proc_create(PERFMGR_HINT_PATH, 0222, NULL, &perfmgr_hint_fops)) {
        pr_err("perfmgr: failed to create hint proc entry\n");
        remove_proc_entry(PERFMGR_DEBUG_PATH, NULL);
        remove_proc_entry(PERFMGR_STATUS_PATH, NULL);
        return -ENOMEM;
    }

    /* Create and start monitor thread */
    perfmgr_global.monitor_thread = kthread_run(perfmgr_monitor_thread, NULL, "perfmgr_v6");
    if (IS_ERR(perfmgr_global.monitor_thread)) {
        pr_err("perfmgr: failed to create monitor thread\n");
        remove_proc_entry(PERFMGR_HINT_PATH, NULL);
        remove_proc_entry(PERFMGR_DEBUG_PATH, NULL);
        remove_proc_entry(PERFMGR_STATUS_PATH, NULL);
        return PTR_ERR(perfmgr_global.monitor_thread);
    }

    /* Register platform device */
    perfmgr_device = platform_device_register_simple("perfmgr_v6_intparse", -1, NULL, 0);
    if (IS_ERR(perfmgr_device)) {
        pr_err("perfmgr: failed to register platform device\n");
        kthread_stop(perfmgr_global.monitor_thread);
        remove_proc_entry(PERFMGR_HINT_PATH, NULL);
        remove_proc_entry(PERFMGR_DEBUG_PATH, NULL);
        remove_proc_entry(PERFMGR_STATUS_PATH, NULL);
        return PTR_ERR(perfmgr_device);
    }

    pr_info("perfmgr_v6_intparse_safe: initialized successfully\n");
    pr_info("perfmgr: subsystems available - PPM: %d, QoS: %d, FPSGO: %d, EARA: %d, PNPMGR: %d\n",
            perfmgr_global.subsys.ppm_available,
            perfmgr_global.subsys.qos_available,
            perfmgr_global.subsys.fpsgo_available,
            perfmgr_global.subsys.eara_thermal_available,
            perfmgr_global.subsys.pnpmgr_available);

    return 0;
}

static void __exit perfmgr_v6_exit(void)
{
    /* Stop the monitor thread */
    if (perfmgr_global.monitor_thread) {
        kthread_stop(perfmgr_global.monitor_thread);
        perfmgr_global.monitor_thread = NULL;
    }

    /* Unregister platform device */
    if (perfmgr_device) {
        platform_device_unregister(perfmgr_device);
        perfmgr_device = NULL;
    }

    /* Remove proc entries */
    remove_proc_entry(PERFMGR_HINT_PATH, NULL);
    remove_proc_entry(PERFMGR_DEBUG_PATH, NULL);
    remove_proc_entry(PERFMGR_STATUS_PATH, NULL);

    pr_info("perfmgr_v6_intparse_safe: unloaded\n");
}

module_init(perfmgr_v6_init);
module_exit(perfmgr_v6_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Safe Integer-Only MediaTek kernel performance manager v6");
MODULE_AUTHOR("OrkGabb");
MODULE_VERSION("6.0.0");
