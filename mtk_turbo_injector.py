#!/usr/bin/env python3
import os
import sys

def main():
    print(">>> INICIANDO INTERCEPTAÇÃO DVFS MEDIATEK (ROTA B) <<<")
    
    target_file = "./drivers/misc/mediatek/base/power/mt6768/mtk_gpufreq_core.c"
    
    if not os.path.exists(target_file):
        print(f"[!] FATAL: {target_file} não encontrado.")
        sys.exit(1)

    with open(target_file, 'r') as f:
        lines = f.readlines()

    patched = any("SYSARCH: 32-OPP Turbo Boost" in line for line in lines)
    
    if not patched:
        anchor_idx = -1
        # Buscando restritamente a PRIMEIRA ocorrência (Linha 312 no dump pré-thermal)
        for i, line in enumerate(lines):
            if "target_freq = g_opp_table[target_idx].gpufreq_khz;" in line:
                anchor_idx = i
                break
                
        if anchor_idx == -1:
            print("[!] FATAL: Âncora de frequência não encontrada.")
            sys.exit(1)

        # Injeção Molecular com identação cravada para o Kernel Padrão
        injection = [
            "\t/* --- SYSARCH: 32-OPP Turbo Boost (Mali G52 OC) --- */\n",
            "\tif (target_idx < g_cur_opp_idx) {\n",
            "\t\tif (g_cur_opp_idx > 8) {\n",
            "\t\t\ttarget_idx = g_cur_opp_idx - 8;\n",
            "\t\t} else {\n",
            "\t\t\ttarget_idx = 0;\n",
            "\t\t}\n",
            "\t}\n",
            "\t/* ------------------------------------------------- */\n"
        ]
        
        lines = lines[:anchor_idx] + injection + lines[anchor_idx:]
        
        with open(target_file, 'w') as f:
            f.writelines(lines)
        print("[*] Injeção aplicada na âncora primária (pré-thermal checks).")
    else:
        print("[*] MTK Governor já possui a injeção Turbo.")

    print("\n>>> POST-APPLICATION CHECK (LEITURA DIRETA DO DISCO) <<<")
    with open(target_file, 'r') as f:
        check_lines = f.readlines()
        
    check_idx = -1
    for i, line in enumerate(check_lines):
        if "SYSARCH: 32-OPP Turbo Boost" in line:
            check_idx = i
            break
            
    if check_idx != -1:
        start = max(0, check_idx - 2)
        end = min(len(check_lines), check_idx + 13)
        for i in range(start, end):
            # Adiciona o ponteiro visual apenas nas linhas injetadas
            prefix = "->" if i >= check_idx and i <= check_idx + 8 else "  "
            print(f"{prefix} [{i+1:04d}] {check_lines[i].rstrip()}")
    else:
        print("[!] FATAL: Falha na verificação pós-aplicação.")
        sys.exit(1)

if __name__ == "__main__":
    main()
