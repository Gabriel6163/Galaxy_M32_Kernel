#!/usr/bin/env python3
import os
import re
import sys

def main():
    print(">>> INICIANDO PROTOCOLO DE INJEÇÃO MOLECULAR (PYTHON) <<<")
    
    jm_file = None
    pm_file = None
    
    # 1. Localização Precisa (Bind estrito ao Bifrost r25p0)
    for root, dirs, files in os.walk('.'):
        if 'mali_bifrost' in root and 'mali-r25p0' in root:
            if 'mali_kbase_jm.c' in files:
                jm_file = os.path.join(root, 'mali_kbase_jm.c')
            if 'mali_kbase_pm_policy.c' in files:
                pm_file = os.path.join(root, 'mali_kbase_pm_policy.c')
            
    if not jm_file and not pm_file:
        print("[!] FATAL: Arquivos alvo do Bifrost r25p0 não encontrados na árvore.")
        sys.exit(1)
        
    print(f"[*] Alvos mapeados em: {os.path.dirname(jm_file)}")

    # ==========================================
    # VETOR 1: JOB MANAGER (SERIALIZAÇÃO & CACHE)
    # ==========================================
    if jm_file:
        with open(jm_file, 'r') as f:
            jm_src = f.read()
            
        if "SYSARCHITECT: R54P2" not in jm_src:
            print(f"[*] Patcheando: {os.path.basename(jm_file)}")
            
            helper_code = """/* --- SYSARCHITECT: R54P2 BACKPORT (ABI SAFE) --- */
static inline bool sysarch_inter_slot_throttle(struct kbase_device *kbdev, u32 js_mask)
{
    int i;
    for (i = 0; i < kbdev->gpu_props.num_job_slots; i++) {
        /* Se o slot vizinho tem jobs ativos e não é o alvo atual, estrangula */
        if ((js_mask & (1 << i)) == 0) {
            /* Bifrost HW Tracker Structure */
            if (kbdev->hwaccess.hw_tracker.slot_slots[i].submitted_jobs > 0)
                return true;
        }
    }
    return false;
}
/* ----------------------------------------------- */

u32 kbase_jm_kick("""
            jm_src = jm_src.replace("u32 kbase_jm_kick(", helper_code)
            
            kick_pattern = re.compile(r'(u32\s+kbase_jm_kick\s*\([^)]+\)\s*\{)')
            kick_injection = r"""\1
    /* --- SYSARCHITECT: R54P2 JOB SERIALIZATION --- */
    if (sysarch_inter_slot_throttle(kbdev, js_mask)) {
        return 0; /* Throttle: Retém a submissão para não engasgar a eMMC/RAM */
    }
    /* --------------------------------------------- */
"""
            jm_src = kick_pattern.sub(kick_injection, jm_src, count=1)
            
            # Anula o Flush Reduction para forçar o cache limpo nas texturas do Genshin
            jm_src = re.sub(r'([ \t]*)(cfg\s*\|\=\s*JS_CONFIG_ENABLE_FLUSH_REDUCTION;)', r'\1// \2 /* SYSARCH: Forced Flush */', jm_src)

            with open(jm_file, 'w') as f:
                f.write(jm_src)
            print("    -> Serialização Inter-Slot e Forced Flush Injetados.")
        else:
            print(f"[*] {os.path.basename(jm_file)} já possui a injeção.")

    # ==========================================
    # VETOR 2: POWER MANAGEMENT (COARSE DEMAND)
    # ==========================================
    if pm_file:
        with open(pm_file, 'r') as f:
            pm_src = f.read()
            
        if "SYSARCHITECT: PM TURBO" not in pm_src:
            print(f"[*] Patcheando: {os.path.basename(pm_file)}")
            
            # Janela de amostragem agressiva
            pm_src = re.sub(r'(#define\s+DEFAULT_PM_POLICY_WINDOW_SIZE\s+)\d+', r'\1 20 /* SYSARCHITECT: PM TURBO */', pm_src)
            
            # Thresholds rápidos e de sustentação
            pm_src = re.sub(r'(policy->up_threshold\s*=\s*)\d+;', r'\1 60; /* SYSARCH: Gatilho Rapido */', pm_src)
            pm_src = re.sub(r'(policy->down_threshold\s*=\s*)\d+;', r'\1 40; /* SYSARCH: Sustentacao */', pm_src)
            
            # Salto Quadruplo (Intercepta qualquer decremento unitário do index)
            pm_src = re.sub(r'(\w*freq_index\w*)\s*--\s*;', r'\1 -= 4; /* SYSARCH: Salto Quadruplo */\n\t\tif (\1 < 0) \1 = 0;', pm_src)
            pm_src = re.sub(r'(\w*freq_index\w*)\s*-=\s*1\s*;', r'\1 -= 4; /* SYSARCH: Salto Quadruplo */\n\t\tif (\1 < 0) \1 = 0;', pm_src)
            
            with open(pm_file, 'w') as f:
                f.write(pm_src)
            print("    -> Latência de 20ms e Salto Quádruplo Injetados.")
        else:
            print(f"[*] {os.path.basename(pm_file)} já possui a injeção.")

    print("\n>>> INJEÇÃO MOLECULAR FINALIZADA COM SUCESSO <<<")

if __name__ == "__main__":
    main()
