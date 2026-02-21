#!/bin/bash
echo ">>> AUDITORIA MOLECULAR DO BINÁRIO <<<"

# 1. Verifica se o Schedutil 1ms persistiu
# Procuramos pela string de comentário que injetamos
strings out/vmlinux | grep "SYSARCH: CPU DVFS Boost"

# 2. Verifica se o Salto de 8-steps da GPU está lá
strings out/vmlinux | grep "SYSARCH: 32-OPP Turbo Boost"

# 3. Verifica se o CCI OC (FP 2, 1) entrou
# Como são valores hexadecimais, checamos se o símbolo está definido
nm out/vmlinux | grep "opp_tbl_method_CCI_G75"
