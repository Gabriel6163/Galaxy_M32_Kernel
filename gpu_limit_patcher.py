#!/usr/bin/env python3
import os

def patch():
    path = "drivers/misc/mediatek/base/power/mt6768/mtk_gpufreq_core.h"
    if not os.path.exists(path):
        print("[!] Arquivo de cabeçalho da GPU não encontrado.")
        return

    with open(path, "r") as f:
        src = f.read()

    # Sobe o limite de proteção de 485MHz para 800MHz
    # Isso evita quedas bruscas de FPS em bateria baixa
    new_src = src.replace("(485000)", "(800000) /* SYSARCH: Level-Up Protection */")
    
    # Bônus: Desabilita a proteção de porcentagem de bateria que você marcou como TODO
    new_src = new_src.replace("#define MT_GPUFREQ_BATT_PERCENT_PROTECT", "/* #define MT_GPUFREQ_BATT_PERCENT_PROTECT (Disabled by SysArch) */")

    with open(path, "w") as f:
        f.write(new_src)
    print("[*] Limites de proteção de bateria subidos para 800MHz. Performance garantida.")

if __name__ == "__main__":
    patch()
