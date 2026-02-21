#!/usr/bin/env python3
import os
import re
import sys

def main():
    print(">>> INICIANDO SINCRONIZAÇÃO DEVFREQ (PYTHON) <<<")
    
    devfreq_file = None
    
    for root, dirs, files in os.walk('.'):
        if 'mali_bifrost' in root and 'mali-r25p0' in root:
            if 'mali_kbase_devfreq.c' in files:
                devfreq_file = os.path.join(root, 'mali_kbase_devfreq.c')
                break
            
    if not devfreq_file:
        print("[!] FATAL: mali_kbase_devfreq.c não encontrado no Bifrost r25p0.")
        sys.exit(1)
        
    print(f"[*] Alvo mapeado: {os.path.basename(devfreq_file)}")

    with open(devfreq_file, 'r') as f:
        src = f.read()
        
    if "SYSARCH: Devfreq Sync" not in src:
        # Substitui a taxa de polling de 100ms (ou qualquer outro valor nativo) para 20ms
        src = re.sub(r'(dp->polling_ms\s*=\s*)\d+;', r'\1 20; /* SYSARCH: Devfreq Sync */', src)
        
        with open(devfreq_file, 'w') as f:
            f.write(src)
        print("    -> Polling do Devfreq sincronizado para 20ms.")
    else:
        print("    -> Devfreq já sincronizado.")

    print(">>> SINCRONIZAÇÃO FINALIZADA <<<")

if __name__ == "__main__":
    main()
