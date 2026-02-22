#!/usr/bin/env bash
set -euo pipefail

# ============================================================================
# Kernel Build Configuration Script
# Target: MT6769V (Samsung M32)
# Toolchain: Clang 21 + ThinLTO
# ============================================================================

export PATH="$(pwd)/clang/bin:$PATH"
export LD=ld.lld
export CC=clang
export CROSS_COMPILE=aarch64-linux-gnu-


KERNEL_DIR="$(pwd)"
OUT_DIR="$KERNEL_DIR/out"
DEFCONFIG="rsuntk_defconfig"
DEFCONFIG_PATH="$KERNEL_DIR/arch/arm64/configs/$DEFCONFIG"
SCRIPTS_CONFIG="$KERNEL_DIR/scripts/config"

echo "[i] daily.sh: Initializing build configuration..."

# Helper Functions (Config API)
set_opt() { "$SCRIPTS_CONFIG" --file "$OUT_DIR/.config" --enable "$1" || true; }
disable_opt() { "$SCRIPTS_CONFIG" --file "$OUT_DIR/.config" --disable "$1" || true; }
set_str() { "$SCRIPTS_CONFIG" --file "$OUT_DIR/.config" --set-str "$1" "$2" || true; }
set_val() { "$SCRIPTS_CONFIG" --file "$OUT_DIR/.config" --set-val "$1" "$2" || true; }
set_mod() { "$SCRIPTS_CONFIG" --file "$OUT_DIR/.config" --module "$1" || true; }

# Initialize Clean Config from Arch
rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"
cp "$DEFCONFIG_PATH" "$OUT_DIR/.config"
make O="$OUT_DIR" olddefconfig

# ----------------------------------------------------------------------------
# Block & Compression Subsystem (LZ4 Upstream Binding)
# ----------------------------------------------------------------------------
echo "[i] Configuring Block/ZRAM backend..."
set_opt CONFIG_SWAP
set_opt CONFIG_BLOCK
set_opt CONFIG_ZRAM
set_opt CONFIG_CRYPTO

# Switch ZRAM backend from LZO to LZ4
disable_opt CONFIG_ZRAM_DEF_COMP_LZO
set_opt CONFIG_ZRAM_DEF_COMP_LZ4
set_opt CONFIG_CRYPTO_LZ4
set_opt CONFIG_RD_LZ4

# Force dependency resolution (Bake LZ4 into kernel)
make O="$OUT_DIR" olddefconfig

# ----------------------------------------------------------------------------
# CPU Scheduler (EAS / BORE / Topology)
# ----------------------------------------------------------------------------
echo "[i] Configuring Scheduler (BORE + 6L/2B EAS)..."
set_opt CONFIG_SCHED_BORE

# Topology awareness
set_opt CONFIG_SCHED_MC
set_opt CONFIG_SCHED_SMT
set_val CONFIG_SCHED_MIGRATION_COST 500000

# EAS (Energy Aware Scheduling) - MTK Integration
set_opt CONFIG_MTK_EAS_POWER_AWARE
set_opt CONFIG_ENERGY_MODEL
set_opt CONFIG_PM_OPP
set_opt CONFIG_PREEMPT_VOLUNTARY
disable_opt CONFIG_PREEMPT
disable_opt CONFIG_PREEMPT_RT
set_opt CONFIG_SCHED_ENERGY_AWARE
set_opt CONFIG_UCLAMP_TASK
set_opt CONFIG_UCLAMP_TASK_GROUP
set_opt CONFIG_DEFAULT_USE_ENERGY_AWARE
set_opt CONFIG_UCLAMP_MAP_OPP
set_val CONFIG_UCLAMP_GROUPS_COUNT 32

# PELT Decay (16ms for mobile responsiveness)
set_opt CONFIG_PELT_UTIL_HALFLIFE_16
disable_opt CONFIG_PELT_UTIL_HALFLIFE_32
disable_opt CONFIG_PELT_UTIL_HALFLIFE_8

# Group Scheduling
set_opt CONFIG_FAIR_GROUP_SCHED
set_opt CONFIG_CPUSETS
set_opt CONFIG_MTK_SCHED_EAS
disable_opt CONFIG_MTK_SCHED_HMP
set_opt CONFIG_SCHED_TUNE

# ----------------------------------------------------------------------------
# eBPF / CGROUP Infrastructure
# ----------------------------------------------------------------------------
echo "[i] Enabling eBPF/CGROUP subsystem..."
set_opt CONFIG_KALLSYMS_ALL
set_opt CONFIG_ENABLE_DEFAULT_TRACERS
set_opt CONFIG_SCHEDSTATS
set_opt CONFIG_DEBUG_FS
set_opt CONFIG_CGROUP_PIDS
set_opt CONFIG_BLK_CGROUP
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

# ----------------------------------------------------------------------------
# MediaTek SoC Optimizations
# ----------------------------------------------------------------------------
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

# ----------------------------------------------------------------------------
# Network Subsystem (TCP Congestion Control)
# ----------------------------------------------------------------------------
echo "[i] Setting TCP Congestion Control to BBR..."
# Disable Legacy Algorithms (Fixed Order)
disable_opt CONFIG_TCP_CONG_BIC
disable_opt CONFIG_TCP_CONG_CUBIC

# Enable Modern Stack
set_str CONFIG_DEFAULT_TCP_CONG "bbr"
set_opt CONFIG_TCP_CONG_BBR
set_opt CONFIG_TCP_CONG_WESTWOOD
set_opt CONFIG_TCP_CONG_HYBLA
set_opt CONFIG_TCP_CONG_VEGAS

# ----------------------------------------------------------------------------
# Memory Management & Timing
# ----------------------------------------------------------------------------
disable_opt CONFIG_LRU_GEN
disable_opt CONFIG_HZ_450
set_opt CONFIG_HZ_300

# ----------------------------------------------------------------------------
# KernelSU & Misc Overrides
# ----------------------------------------------------------------------------
# KernelSU Manual Hooks (Safer on 4.14)
disable_opt CONFIG_KSU_TRACEPOINT_HOOK
set_opt CONFIG_KSU_MANUAL_HOOK
set_opt CONFIG_KSU

# Toolchain & Debugging
disable_opt CONFIG_DYNAMIC_DEBUG
disable_opt CONFIG_FUNCTION_TRACER
disable_opt CONFIG_PRINTK_TIME
disable_opt CONFIG_DEBUG_SECTION_MISMATCH
set_val CONFIG_RCU_CPU_STALL_TIMEOUT 120
set_opt CONFIG_LTO_CLANG
set_opt CONFIG_PSI
set_opt CONFIG_USERFAULTFD
set_opt CONFIG_ANDROID_BINDERFS
set_val CONFIG_ANDROID_BINDER_MAX_THREAD_COUNT 32

# ----------------------------------------------------------------------------
# Final Serialization
# ----------------------------------------------------------------------------
echo "[i] Serializing configuration back to arch/arm64..."

make O="$OUT_DIR" olddefconfig
make O="$OUT_DIR" savedefconfig
cp -v "$OUT_DIR/defconfig" "$DEFCONFIG_PATH"

# Dynamic thread detection for display
THREADS=$(nproc 2>/dev/null || echo 8)

echo ""
echo "✅ Configuration Sync Complete."
echo "   - TCP: BBR (Default)"
echo "   - SCHED: BORE + EAS (6L/2B)"
echo "   - HZ: 300"
echo "   - LTO: Enabled"
echo ""
echo "To build:"
echo "bash build_kernel.sh"
echo ""
