#!/usr/bin/env python3
import os

def main():
    print(">>> AUDITORIA DE CONSUMO: M32 HYBRID ENGINE <<<")
    path = "drivers/misc/mediatek/base/power/cpufreq_v1/src/mtk_cpufreq_interface.c"
    
    with open(path, "r") as f:
        src = f.read()

    # Verificamos se existe o comando de 'DVFS_LOW_POWER'
    if "DVFS_LOW_POWER" in src:
        print("[+] Sucesso: O driver possui caminhos de economia de energia.")
    else:
        print("[!] Aviso: O driver parece estar operando em modo de alta performance fixo.")

if __name__ == "__main__":
    main()
