#!/usr/bin/env python3
import os

def apply_fixes():
    print(">>> INICIANDO PATCHER MOLECULAR TIER S+ <<<")
    
    # 1. Backport da syscall close_range (Fix para o erro de Linker)
    fs_open = "fs/open.c"
    syscall_code = """
/* BACKPORT: sys_close_range (Kernel 5.9+) para estabilidade OneUI 8.5 */
SYSCALL_DEFINE3(close_range, unsigned int, fd, unsigned int, max_fd, unsigned int, flags)
{
    struct files_struct *files = current->files;
    struct fdtable *fdt;
    unsigned int i;
    if (flags & ~(0U)) return -EINVAL;
    if (fd > max_fd) return -EINVAL;
    spin_lock(&files->file_lock);
    fdt = files_fdtable(files);
    if (max_fd >= fdt->max_fds) max_fd = fdt->max_fds - 1;
    for (i = fd; i <= max_fd; i++) {
        struct file *file = fdt->fd[i];
        if (file) {
            __clear_bit(i, fdt->open_fds);
            __clear_bit(i, fdt->close_on_exec);
            spin_unlock(&files->file_lock);
            filp_close(file, files);
            spin_lock(&files->file_lock);
        }
    }
    spin_unlock(&files->file_lock);
    return 0;
}
"""
    if os.path.exists(fs_open):
        with open(fs_open, "r") as f:
            content = f.read()
        if "SYSCALL_DEFINE3(close_range" not in content:
            with open(fs_open, "a") as f:
                f.write(syscall_code)
            print("[+] fs/open.c: Backport de close_range aplicado.")

    # 2. Upgrade no daily.sh para OneUI 8.5 (BinderFS + Threads)
    daily_sh = "daily.sh"
    if os.path.exists(daily_sh):
        with open(daily_sh, "r") as f:
            lines = f.readlines()
        
        new_configs = [
            "set_opt CONFIG_ANDROID_BINDERFS\n",
            "set_val CONFIG_ANDROID_BINDER_MAX_THREAD_COUNT 32\n"
        ]
        
        # Evitar duplicatas e inserir antes do 'FINAL SAVE'
        final_lines = []
        for line in lines:
            if any(cfg.split()[1] in line for cfg in new_configs):
                continue
            if "# FINAL SAVE" in line:
                final_lines.extend(new_configs)
            final_lines.append(line)
            
        with open(daily_sh, "w") as f:
            f.writelines(final_lines)
        print("[+] daily.sh: BinderFS e 32 Threads configurados.")

if __name__ == "__main__":
    apply_fixes()
