#!/usr/bin/env python3
import os

def apply_strict_patch():
    print(">>> APLICANDO PATCH 4.14 (STRICT RSUNTK) <<<")
    
    # Exemplo: fs/read_write.c
    path = "fs/read_write.c"
    if os.path.exists(path):
        with open(path, "r") as f:
            src = f.read()
        
        # Injeta o extern antes da função vfs_read
        if "ksu_handle_vfs_read" not in src:
            hook_def = """#ifdef CONFIG_KSU
extern bool ksu_vfs_read_hook __read_mostly;
extern int ksu_handle_vfs_read(struct file **file_ptr, char __user **buf_ptr,
            size_t *count_ptr, loff_t **pos);
#endif
"""
            src = src.replace("ssize_t vfs_read(", hook_def + "ssize_t vfs_read(")
            
            # Injeta a chamada com ponteiros duplos
            call_code = """{
    ssize_t ret;

#ifdef CONFIG_KSU 
    if (unlikely(ksu_vfs_read_hook))
        ksu_handle_vfs_read(&file, &buf, &count, &pos);
#endif"""
            src = src.replace("{", call_code, 1) # Só no primeiro vfs_read
            
        with open(path, "w") as f:
            f.write(src)
        print(f"[+] {path} sincronizado com patch oficial.")

if __name__ == "__main__":
    apply_strict_patch()
