#!/usr/bin/env python3
import os
import sys

def main():
    print(">>> INICIANDO INJEÇÃO: CPU DVFS BOOST (SCHEDUTIL) <<<")
    path = "kernel/sched/cpufreq_schedutil.c"
    
    if not os.path.exists(path):
        print(f"[!] FATAL: {path} não encontrado.")
        sys.exit(1)

    with open(path, "r") as f:
        lines = f.readlines()

    if any("SYSARCH: CPU DVFS Boost" in l for l in lines):
        print("[*] Schedutil já está com o DVFS Boost ativo.")
        return

    # Procura as linhas 825/826 pelo padrão molecular
    up_idx = -1
    down_idx = -1
    
    for i, l in enumerate(lines):
        if "tunables->up_rate_limit_us = cpufreq_policy_transition_delay_us(policy);" in l:
            up_idx = i
        if "tunables->down_rate_limit_us = cpufreq_policy_transition_delay_us(policy);" in l:
            down_idx = i

    if up_idx == -1 or down_idx == -1:
        print("[!] FATAL: Baseline da MediaTek não encontrada no Schedutil.")
        sys.exit(1)

    # Injeta a agressividade ignorando a DTB da Samsung
    lines[up_idx] = "\ttunables->up_rate_limit_us = 1000; /* SYSARCH: CPU DVFS Boost (1ms) */\n"
    lines[down_idx] = "\ttunables->down_rate_limit_us = 20000; /* SYSARCH: Anti-Hysteresis (20ms) */\n"

    with open(path, "w") as f:
        f.writelines(lines)
        
    print("[*] Schedutil reprogramado. Up: 1000us | Down: 20000us.")
    print("    -> Sinergia total com BORE + Mali 8-Step Boost.")

if __name__ == "__main__":
    main()
