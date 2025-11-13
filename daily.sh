#!/usr/bin/env bash
set -euo pipefail

KERNEL_DIR="$(pwd)"
OUT_DIR="$KERNEL_DIR/out"
DEFCONFIG="rsuntk_defconfig"
DEFCONFIG_PATH="$KERNEL_DIR/arch/arm64/configs/$DEFCONFIG"
SCRIPTS_CONFIG="$KERNEL_DIR/scripts/config"

echo "[*] DAILY.SH: Preparing build config"

# 1. Limpa e prepara a configuração base
rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"
cp "$DEFCONFIG_PATH" "$OUT_DIR/.config"
make O="$OUT_DIR" olddefconfig

# --- Helpers ---
set_opt() { "$SCRIPTS_CONFIG" --file "$OUT_DIR/.config" --enable "$1" || true; }
disable_opt() { "$SCRIPTS_CONFIG" --file "$OUT_DIR/.config" --disable "$1" || true; }
set_str() { "$SCRIPTS_CONFIG" --file "$OUT_DIR/.config" --set-str "$1" "$2" || true; }
set_val() { "$SCRIPTS_CONFIG" --file "$OUT_DIR/.config" --set-val "$1" "$2" || true; }
set_mod() { "$SCRIPTS_CONFIG" --file "$OUT_DIR/.config" --module "$1" || true; }

# --------------------------------------------------------------------
# 2. APLICA NOSSAS OTIMIZAÇÕES DESEJADAS
# --------------------------------------------------------------------

# QoS / MTK / EAS / Performance
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
set_opt CONFIG_MTK_PERFMGR_V6

# TCP default:
set_str CONFIG_DEFAULT_TCP_CONG "bbr"

#Try MGLRU:

disable_opt LRU_GEN


# Tweaks/Misc

set_opt CONFIG_HZ_300
disable_opt CONFIG_MTK_SCHED_HMP
set_opt CONFIG_MTK_SCHED_EAS

# KernelSU
set_opt CONFIG_KSU_TRACEPOINT_HOOK
disable_opt CONFIG_KSU_MANUAL_HOOK
set_mod CONFIG_KSU

# Depuração mínima
# disable_opt CONFIG_DYNAMIC_DEBUG
# disable_opt CONFIG_FUNCTION_TRACER
# disable_opt CONFIG_PRINTK_TIME
# disable_opt CONFIG_SCHEDSTATS
# disable_opt CONFIG_DEBUG_SECTION_MISMATCH
# set_val CONFIG_RCU_CPU_STALL_TIMEOUT 120

# LTO e PSI
set_opt CONFIG_LTO_CLANG
set_opt CONFIG_PSI
set_opt CONFIG_USERFAULTFD

# --------------------------------------------------------------------
# 3. CGROUP / BPF – mínima necessária para One UI 8
# --------------------------------------------------------------------

# Cgroup base
set_opt CONFIG_CGROUPS
set_opt CONFIG_MEMCG
set_opt CONFIG_MEMCG_SWAP
set_opt CONFIG_PERF_EVENTS

# Núcleo cgroup e controladores úteis
set_opt CONFIG_CGROUP_SCHED
set_opt CONFIG_CGROUP_CPUACCT
set_opt CONFIG_CGROUP_BPF
set_opt CONFIG_CGROUP_FREEZER
set_opt CONFIG_CGROUP_DEVICE
set_opt CONFIG_CGROUP_PERF
set_opt CONFIG_CGROUP_DEBUG
set_opt CONFIG_CGROUP_NET_CLASSID
set_opt CONFIG_CGROUP_NET_PRIO

# BPF (já existe, mantido apenas o essencial)
set_opt CONFIG_BPF
set_opt CONFIG_BPF_SYSCALL
set_opt CONFIG_BPF_JIT
set_opt CONFIG_BPF_JIT_ALWAYS_ON
set_opt CONFIG_BPF_EVENTS
set_opt CONFIG_NET_CLS_BPF
set_opt CONFIG_NET_ACT_BPF

# Netfilter match opcional
set_opt CONFIG_NETFILTER_XT_MATCH_BPF

# (NÃO adicionamos BPF_LSM nem STREAM_PARSER/VERDICT: não suportado no 4.14 e desnecessário)

# --------------------------------------------------------------------
# 4. Salva defconfig
# --------------------------------------------------------------------
make O="$OUT_DIR" savedefconfig
cp -v "$OUT_DIR/defconfig" "$DEFCONFIG_PATH"

echo "[*] DAILY.SH: Configuração final aplicada e pronta para compilar."
echo ">>> Run: bash build.sh kernel --jobs $(nproc) rsuntk_defconfig"
