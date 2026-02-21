#!/usr/bin/env python3
import os
import re
import sys

def main():
    print(">>> INICIANDO BLOQUEIO DE USER-SPACE (I/O & TCP) <<<")

    blk_file = "./block/blk-sysfs.c"
    tcp_file = "./net/ipv4/tcp_cong.c"

    # ==========================================
    # VETOR 1: FORÇAR READ-AHEAD 512KB (BLOCK LAYER)
    # ==========================================
    if os.path.exists(blk_file):
        with open(blk_file, 'r') as f:
            blk_src = f.read()
            
        if "SYSARCH: Hardcode 512KB min" not in blk_src:
            print("[*] Patcheando block/blk-sysfs.c (Read-Ahead Lock)...")
            # Intercepta a escrita de RA antes de gravar na Backing Dev Info
            blk_src = re.sub(
                r'(q->backing_dev_info->ra_pages\s*=\s*ra_kb\s*>>\s*\(PAGE_SHIFT\s*-\s*10\);)',
                r'if (ra_kb < 512) ra_kb = 512; /* SYSARCH: Hardcode 512KB min (Anti-Samsung) */\n\t\1',
                blk_src
            )
            with open(blk_file, 'w') as f:
                f.write(blk_src)
            print("    -> Read-Ahead cravado em 512KB. O Kernel ignorará valores menores.")
        else:
            print("[*] Read-Ahead já está blindado.")
    else:
        print(f"[!] Erro: {blk_file} não encontrado.")

    # ==========================================
    # VETOR 2: FORÇAR BBR E MATAR CUBIC (TCP LAYER)
    # ==========================================
    if os.path.exists(tcp_file):
        with open(tcp_file, 'r') as f:
            tcp_src = f.read()

        if "SYSARCH: Anti-Samsung Init BBR Lock" not in tcp_src:
            print("[*] Patcheando net/ipv4/tcp_cong.c (BBR Lock)...")
            
            # Intercepta a chamada global (echo cubic > /proc/sys/net/ipv4/tcp_congestion_control)
            tcp_src = re.sub(
                r'(int\s+tcp_set_default_congestion_control\(const\s+char\s*\*name\)\n\s*\{)',
                r'\1\n\t/* SYSARCH: Anti-Samsung Init BBR Lock */\n\tif (name && strncmp(name, "cubic", 5) == 0) name = "bbr";\n',
                tcp_src
            )
            
            # Intercepta chamadas de Socket direto da stack de rede do Android (setsockopt)
            tcp_src = re.sub(
                r'(int\s+tcp_set_congestion_control\(struct\s+sock\s*\*sk,\s*const\s+char\s*\*name,\s*bool\s+load,\s*bool\s+cap_net_admin\)\n\s*\{)',
                r'\1\n\t/* SYSARCH: Socket BBR Force-Override */\n\tif (name && strncmp(name, "cubic", 5) == 0) name = "bbr";\n',
                tcp_src
            )

            with open(tcp_file, 'w') as f:
                f.write(tcp_src)
            print("    -> BBR Locked. Se o Android pedir Cubic, o Kernel entregará BBR.")
        else:
            print("[*] TCP Congestion já está blindado.")
    else:
        print(f"[!] Erro: {tcp_file} não encontrado.")

    print("\n>>> BLINDAGEM DE USER-SPACE FINALIZADA <<<")

if __name__ == "__main__":
    main()
