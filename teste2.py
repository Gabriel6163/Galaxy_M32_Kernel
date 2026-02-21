#!/usr/bin/env python3
import os
import re

print(">>> INICIANDO NEUTRALIZAÇÃO REGEX (TCP KCONFIG) <<<")

kconfig = "net/ipv4/Kconfig"
if os.path.exists(kconfig):
    with open(kconfig, "r") as f:
        src = f.read()

    # 1. Neutraliza o default do CUBIC
    src, c_cubic = re.subn(r'(config TCP_CONG_CUBIC\s*\n(?:.*\n)*?\s*default\s+)y', r'\1n /* KILLED BY SYSARCH */', src)
    if c_cubic: print("[+] Kconfig: CUBIC rebaixado via Regex.")

    # 2. Neutraliza o default do BIC
    src, c_bic = re.subn(r'(config TCP_CONG_BIC\s*\n(?:.*\n)*?\s*default\s+)y', r'\1n /* KILLED BY SYSARCH */', src)
    if c_bic: print("[+] Kconfig: BIC rebaixado via Regex.")

    # 3. Força BBR como default global nativo na base do kernel
    src, c_def = re.subn(r'(config DEFAULT_TCP_CONG\s*\n(?:.*\n)*?\s*default\s+)"(?:cubic|bic)"', r'\1"bbr" /* TIER S+ NATIVE */', src)
    if c_def: print("[+] Kconfig: BBR injetado como string de fallback primária.")

    with open(kconfig, "w") as f:
        f.write(src)
else:
    print(f"[X] Arquivo {kconfig} não encontrado.")

print(">>> OPERAÇÃO CONCLUÍDA. DISPARE O KBUILD. <<<")
