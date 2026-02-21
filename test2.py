#!/usr/bin/env python3
import os

def audit():
    path = "drivers/misc/mediatek/base/power/cpufreq_v1/src/mach/mt6768/mtk_cpufreq_opp_table.h"
    
    if not os.path.exists(path):
        print("[!] Arquivo não encontrado.")
        return

    with open(path, "r") as f:
        lines = f.readlines()

    print("\n>>> AUDITORIA DE SINTAXE: CCI_G75 METHODS <<<")
    
    start_line = -1
    end_line = -1
    
    for i, line in enumerate(lines):
        if "static struct mt_cpu_freq_method opp_tbl_method_CCI_G75[]" in line:
            start_line = i
        if start_line != -1 and "};" in line and i > start_line:
            end_line = i
            break

    if start_line != -1 and end_line != -1:
        # Imprime com margem de 2 linhas antes e depois
        for j in range(start_line - 2, end_line + 3):
            if j >= 0 and j < len(lines):
                pointer = "->" if "FP(" in lines[j] else "  "
                print(f"{pointer} [{j+1:04d}] {lines[j].rstrip()}")
    else:
        print("[!] Não foi possível localizar o bloco CCI_G75 para auditoria.")

if __name__ == "__main__":
    audit()
