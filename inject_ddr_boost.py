#!/usr/bin/env python3
import os

def main():
    print(">>> INICIANDO BOOST DE LARGURA DE BANDA (DDR_OPP_0) <<<")
    path = "drivers/devfreq/helio-dvfsrc-v2/helio-dvfsrc-opp-mt6768.c"
    
    if not os.path.exists(path):
        print(f"[!] FATAL: {path} não encontrado.")
        return

    with open(path, "r") as f:
        src = f.read()

    if "SYSARCH: DDR_OPP_0 Force" in src:
        print("[*] Interconnect já está com o DDR Boost ativo.")
        return

    # Substitui o OPP_1 (frequência média) pelo OPP_0 (frequência máxima)
    # Adicionamos um comentário para auditoria posterior
    new_src = src.replace("DDR_OPP_1", "DDR_OPP_0 /* SYSARCH: DDR_OPP_0 Force */")
    
    # Se houver menções a OPP_2 (frequência baixa), também subimos para o topo
    # opcional, mas recomendado para o seu perfil HyperEngine
    new_src = new_src.replace("DDR_OPP_2", "DDR_OPP_0 /* SYSARCH: DDR_OPP_0 Force */")

    with open(path, "w") as f:
        f.write(new_src)
        
    print("[*] Mapeamento DVFSRC alterado: Todos os estados de VCORE agora usam DDR_OPP_0.")
    print("    -> Latência de memória minimizada.")

if __name__ == "__main__":
    main()
