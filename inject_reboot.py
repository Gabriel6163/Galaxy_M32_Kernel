#!/usr/bin/env python3
import os

def main():
    print(">>> INICIANDO INJEÇÃO CIRÚRGICA: kernel/reboot.c <<<")
    path = "kernel/reboot.c"
    
    if not os.path.exists(path):
        print("[!] FATAL: kernel/reboot.c não encontrado.")
        return

    with open(path, "r") as f:
        lines = f.readlines()

    if any("ksu_handle_sys_reboot" in l for l in lines):
        print("[*] Hook de reboot já existe. Nenhuma ação necessária.")
        return

    anchor_idx = -1
    for i, l in enumerate(lines):
        if "SYSCALL_DEFINE4(reboot" in l:
            anchor_idx = i
            break

    if anchor_idx == -1:
        print("[!] FATAL: SYSCALL_DEFINE4(reboot não encontrado.")
        return

    # 1. Injeta a declaração do RKSU fora da função
    decl = "#ifdef CONFIG_KSU\nextern int ksu_handle_sys_reboot(int magic1, int magic2, unsigned int cmd, void __user **arg);\n#endif\n"
    lines.insert(anchor_idx, decl)
    anchor_idx += 1 

    # 2. Busca o início do corpo da função
    brace_idx = -1
    for i in range(anchor_idx, len(lines)):
        if "{" in lines[i]:
            brace_idx = i
            break

    # 3. Pula as declarações de variáveis (ISO C90 compliance) para evitar warnings do Clang
    insert_idx = brace_idx + 1
    for i in range(brace_idx, len(lines)):
        if "int ret = 0;" in lines[i]:
            insert_idx = i + 1
            break

    # 4. Injeta a execução do Hook
    hook = "\n#ifdef CONFIG_KSU\n\tksu_handle_sys_reboot(magic1, magic2, cmd, &arg);\n#endif\n"
    lines.insert(insert_idx, hook)

    with open(path, "w") as f:
        f.writelines(lines)
        
    print("[*] Hook ksu_handle_sys_reboot injetado com precisão molecular.")

if __name__ == "__main__":
    main()
