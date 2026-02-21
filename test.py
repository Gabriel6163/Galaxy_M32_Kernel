#!/usr/bin/env python3
import os

def main():
    print(">>> OTIMIZANDO MÉTODOS DE DIVISÃO (CCI BOOST) <<<")
    path = "drivers/misc/mediatek/base/power/cpufreq_v1/src/mach/mt6768/mtk_cpufreq_opp_table.h"
    
    if not os.path.exists(path):
        print("[!] Arquivo não encontrado.")
        return

    with open(path, "r") as f:
        src = f.read()

    # Transforma os divisores lentos (4) em divisores rápidos (2) no CCI
    # Isso mantém o barramento mais alerta em frequências baixas
    if "opp_tbl_method_CCI_G75" in src:
        # Pega o bloco do CCI G75 e substitui FP(4, 1) por FP(2, 1)
        # Vamos usar um replace cirúrgico apenas nesse bloco
        start_marker = "static struct mt_cpu_freq_method opp_tbl_method_CCI_G75[] = {"
        end_marker = "};"
        
        start_idx = src.find(start_marker)
        end_idx = src.find(end_marker, start_idx)
        
        block = src[start_idx:end_idx]
        new_block = block.replace("FP(4,   1)", "FP(2,   1) /* SYSARCH: Faster Wakeup */")
        
        src = src[:start_idx] + new_block + src[end_idx:]
        
        with open(path, "w") as f:
            f.write(src)
        print("[*] CCI G75: Divisores de repouso otimizados para resposta rápida.")
    else:
        print("[!] Bloco CCI_G75 não mapeado.")

if __name__ == "__main__":
    main()
