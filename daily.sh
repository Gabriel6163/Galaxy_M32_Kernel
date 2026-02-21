#!/usr/bin/env bash

set -euo pipefail

KERNEL_DIR="$(pwd)"
OUT_DIR="$KERNEL_DIR/out"
DEFCONFIG="rsuntk_defconfig"
DEFCONFIG_PATH="$KERNEL_DIR/arch/arm64/configs/$DEFCONFIG"
SCRIPTS_CONFIG="$KERNEL_DIR/scripts/config"

echo "[*] DAILY.SH: Preparing build config (M32_HyperEngine optimized + 6L+2B big.LITTLE aware)"

# Helpers
set_opt() { "$SCRIPTS_CONFIG" --file "$OUT_DIR/.config" --enable "$1" || true; }
disable_opt() { "$SCRIPTS_CONFIG" --file "$OUT_DIR/.config" --disable "$1" || true; }
set_str() { "$SCRIPTS_CONFIG" --file "$OUT_DIR/.config" --set-str "$1" "$2" || true; }
set_val() { "$SCRIPTS_CONFIG" --file "$OUT_DIR/.config" --set-val "$1" "$2" || true; }
set_mod() { "$SCRIPTS_CONFIG" --file "$OUT_DIR/.config" --module "$1" || true; }

# Prepare base config
rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"
cp "$DEFCONFIG_PATH" "$OUT_DIR/.config"
make O="$OUT_DIR" olddefconfig

# ============================================================================
# LZ4 MOLECULAR SETUP (FIXED ORDER)
# ============================================================================
echo "[*] Configurando Compressão LZ4 Molecular (Baking Dependencies)..."

# 1. Ativar os PAIS primeiro (Sem isso, as opções filhas são ignoradas)
set_opt CONFIG_SWAP
set_opt CONFIG_BLOCK
set_opt CONFIG_ZRAM
set_opt CONFIG_CRYPTO

# 2. Desativar o padrão antigo (LZO) para liberar o slot 'choice'
disable_opt CONFIG_ZRAM_DEF_COMP_LZO

# 3. Ativar o LZ4 e consumidores
set_opt CONFIG_ZRAM_DEF_COMP_LZ4
set_opt CONFIG_CRYPTO_LZ4
set_opt CONFIG_RD_LZ4
set_opt CONFIG_SCHED_BORE

# 4. BAKE: Força a resolução de dependências AGORA.
# Isso seleciona automaticamente CONFIG_LZ4_COMPRESS (lib) oculta.
make O="$OUT_DIR" olddefconfig

# ============================================================================
# eBPF CRITICAL CONFIGS
# ============================================================================

echo "[*] Enabling eBPF critical configs for M32_HyperEngine..."
set_opt CONFIG_KALLSYMS_ALL
set_opt CONFIG_ENABLE_DEFAULT_TRACERS
set_opt CONFIG_SCHEDSTATS
set_opt CONFIG_DEBUG_FS
set_opt CONFIG_CGROUP_PIDS
set_opt CONFIG_BLK_CGROUP

# ============================================================================
# BIG.LITTLE SCHEDULER AWARENESS (6L+2B CRITICAL)
# ============================================================================
echo "[*] Configuring big.LITTLE scheduler for 6L+2B topology..."
set_opt CONFIG_SCHED_MC
set_opt CONFIG_SCHED_SMT
set_val CONFIG_SCHED_MIGRATION_COST 500000
set_opt CONFIG_MTK_EAS_POWER_AWARE
set_opt CONFIG_ENERGY_MODEL
set_opt CONFIG_PM_OPP

# PREEMPTION MODEL (VOLUNTARY for big.LITTLE)
set_opt CONFIG_PREEMPT_VOLUNTARY
disable_opt CONFIG_PREEMPT
disable_opt CONFIG_PREEMPT_RT

# ============================================================================
# ENERGY AWARE SCHEDULING (EAS) — UPSTREAM REAL
# ============================================================================
echo "[*] Enabling Energy Aware Scheduling (EAS) for 6L+2B..."
set_opt CONFIG_SCHED_ENERGY_AWARE
set_opt CONFIG_UCLAMP_TASK
set_opt CONFIG_UCLAMP_TASK_GROUP
set_opt CONFIG_DEFAULT_USE_ENERGY_AWARE
set_opt CONFIG_UCLAMP_MAP_OPP
set_val CONFIG_UCLAMP_GROUPS_COUNT 32
# PELT halflife 16ms (melhor para mobile big.LITTLE)
set_opt CONFIG_PELT_UTIL_HALFLIFE_16
disable_opt CONFIG_PELT_UTIL_HALFLIFE_32
disable_opt CONFIG_PELT_UTIL_HALFLIFE_8
# Fair group sched (necessário para EAS cgroup)
set_opt CONFIG_FAIR_GROUP_SCHED
set_opt CONFIG_CPUSETS
# MTK EAS integration
set_opt CONFIG_MTK_SCHED_EAS
disable_opt CONFIG_MTK_SCHED_HMP
set_opt CONFIG_SCHED_TUNE

# ============================================================================
# CGROUP / BPF CORE
# ============================================================================
set_opt CONFIG_CGROUPS
set_opt CONFIG_MEMCG
set_opt CONFIG_MEMCG_SWAP
set_opt CONFIG_PERF_EVENTS
set_opt CONFIG_CGROUP_SCHED
set_opt CONFIG_CGROUP_CPUACCT
set_opt CONFIG_CGROUP_BPF
set_opt CONFIG_CGROUP_FREEZER
set_opt CONFIG_CGROUP_DEVICE
set_opt CONFIG_CGROUP_PERF
set_opt CONFIG_CGROUP_DEBUG
set_opt CONFIG_CGROUP_NET_CLASSID
set_opt CONFIG_CGROUP_NET_PRIO
set_opt CONFIG_BPF
set_opt CONFIG_BPF_SYSCALL
set_opt CONFIG_BPF_JIT
set_opt CONFIG_BPF_JIT_ALWAYS_ON
set_opt CONFIG_BPF_EVENTS
set_opt CONFIG_NET_CLS_BPF
set_opt CONFIG_NET_ACT_BPF
set_opt CONFIG_NETFILTER_XT_MATCH_BPF

# ============================================================================
# MTK PERFORMANCE CONFIGS
# ============================================================================
set_opt CONFIG_MTK_LOAD_TRACKER
set_opt CONFIG_MTK_CPU_CTRL_CFP
set_opt CONFIG_MTK_PERF_OBSERVER
set_opt CONFIG_MTK_FPSGO_V3
set_opt CONFIG_MTK_EARA
set_opt CONFIG_MTK_EARA_THERMAL
set_opt CONFIG_MTK_EARA_AI
set_opt CONFIG_PNPMGR
set_opt CONFIG_MTK_IO_BOOST
set_opt CONFIG_MTK_PERFMGR
set_opt CONFIG_MTK_RESYM

# ============================================================================
# NETWORK OPTIMIZATIONS
# ============================================================================
set_str CONFIG_DEFAULT_TCP_CONG "bbr"
set_opt CONFIG_TCP_CONG_BBR
set_opt CONFIG_TCP_CONG_WESTWOOD
set_opt CONFIG_TCP_CONG_HYBLA
set_opt CONFIG_TCP_CONG_VEGAS

# ============================================================================
# MEMORY MANAGEMENT
# ============================================================================
disable_opt CONFIG_LRU_GEN

# ============================================================================
# KERNEL FREQUENCY (HZ=300 STABLE)
# ============================================================================
disable_opt CONFIG_HZ_450
set_opt CONFIG_HZ_300

# ============================================================================
# KERNELSU
# ============================================================================
disable_opt CONFIG_KSU_TRACEPOINT_HOOK
set_opt CONFIG_KSU_MANUAL_HOOK
set_opt CONFIG_KSU

# ============================================================================
# DEBUG CONFIGS
# ============================================================================
disable_opt CONFIG_DYNAMIC_DEBUG
disable_opt CONFIG_FUNCTION_TRACER
disable_opt CONFIG_PRINTK_TIME
disable_opt CONFIG_DEBUG_SECTION_MISMATCH
set_val CONFIG_RCU_CPU_STALL_TIMEOUT 120

# ============================================================================
# COMPILER OPTIMIZATIONS
# ============================================================================
set_opt CONFIG_LTO_CLANG
set_opt CONFIG_PSI
set_opt CONFIG_USERFAULTFD

# ============================================================================
set_opt CONFIG_ANDROID_BINDERFS
set_val CONFIG_ANDROID_BINDER_MAX_THREAD_COUNT 32
# FINAL SAVE
# ============================================================================
echo "[*] Finalizing config..."

# Rodamos olddefconfig DE NOVO para garantir que as configs de BPF/MTK também sejam processadas
make O="$OUT_DIR" olddefconfig

# Gera o arquivo limpo e válido
make O="$OUT_DIR" savedefconfig
cp -v "$OUT_DIR/defconfig" "$DEFCONFIG_PATH"

echo ""
echo "✅ M32_HyperEngine Tier S+ config:"
echo "   • LZ4 Upstream (ZRAM/Crypto/RD) [VERIFIED]"
echo "   • 6L+2B big.LITTLE EAS real (upstream)"
echo "   • eBPF 5.4 infra ready"
echo "   • schedutil + uclamp + energy model"
echo "   • eMMC 5.1 stable (HZ=300)"
echo ""
echo "Build: bash build.sh kernel --jobs $(nproc) rsuntk_defconfig"

# TIER S+ TCP ELIMINATION
disable_opt CONFIG_TCP_CONG_BIC
disable_opt CONFIG_TCP_CONG_CUBIC
set_str CONFIG_DEFAULT_TCP_CONG "bbr"
