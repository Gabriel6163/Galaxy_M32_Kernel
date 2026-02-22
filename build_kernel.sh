#!/bin/bash
set -e

# ==========================================================
# TOOLCHAIN ABSOLUTE RESOLUTION
# ==========================================================
CLANG_BIN="$(pwd)/clang/bin"

if [ ! -f "$CLANG_BIN/ld.lld" ]; then
    echo "[FAIL] Linker not found at: $CLANG_BIN/ld.lld"
    exit 1
fi

export PATH="$CLANG_BIN:$PATH"

# ==========================================================
# MAKE OVERRIDE FLAGS (VDSO KILLER)
# ==========================================================
# A chave aqui é injetar o caminho ABSOLUTO para CC e LD diretamente
# nas variáveis do Make, impedindo que o sub-make do VDSO faça hardcode.

MAKE_FLAGS=(
    "O=$(pwd)/out"
    "ARCH=arm64"
    "SUBARCH=arm64"
    "CROSS_COMPILE=aarch64-linux-gnu-"
    "CLANG_TRIPLE=aarch64-linux-gnu-"
    "CC=$CLANG_BIN/clang"
    "LD=$CLANG_BIN/ld.lld"
    "AR=$CLANG_BIN/llvm-ar"
    "NM=$CLANG_BIN/llvm-nm"
    "OBJCOPY=$CLANG_BIN/llvm-objcopy"
    "OBJDUMP=$CLANG_BIN/llvm-objdump"
    "STRIP=$CLANG_BIN/llvm-strip"
    "LLVM=1"
    "LLVM_IAS=1"
    "LTO=thin"
    "KCFLAGS=-w"
    "CONFIG_SECTION_MISMATCH_WARN_ONLY=y"
)

echo "[i] Linker forced to absolute path: $CLANG_BIN/ld.lld"

# ==========================================================
# KBUILD EXECUTION
# ==========================================================
make -C "$(pwd)" "${MAKE_FLAGS[@]}" rsuntk_defconfig
make -C "$(pwd)" "${MAKE_FLAGS[@]}" -j"$(nproc)"

# ==========================================================
# PAYLOAD EXTRACTION & ANYKERNEL3 PACKAGING (PURE IMAGE)
# ==========================================================
IMAGE_SRC="$(pwd)/out/arch/arm64/boot/Image"
AK3_DIR="$(pwd)/AnyKernel3"

if [ -f "$IMAGE_SRC" ]; then
    echo "[OK] Image successfully compiled."
    
    [ ! -d "$AK3_DIR" ] && git clone https://github.com/rsuntk/AnyKernel3.git --depth=1
    
    # 1. Limpeza Atômica: Remove kernels antigos da pasta AnyKernel3
    rm -rf "$AK3_DIR/.git"
    rm -f "$AK3_DIR/Image" "$AK3_DIR/Image.gz" "$AK3_DIR/Image.lz4"
    
    # 2. Injeção Pura
    cp -f "$IMAGE_SRC" "$AK3_DIR/"
    
    # 3. Formatação
    GITSHA=$(git rev-parse --short HEAD 2>/dev/null || echo "local")
    DATE=$(date +'%Y%m%d%H%M%S')
    ZIP_NAME="AnyKernel3-$(make kernelversion)-M325FV_${GITSHA}-${DATE}.zip"
    
    # 4. Empacotamento Extremo (Deflate 9)
    echo "[i] Packing $ZIP_NAME..."
    cd "$AK3_DIR"
    
    sed -i "s/BLOCK=.*/BLOCK=\/dev\/block\/platform\/bootdevice\/by-name\/boot;/" anykernel.sh
    zip -r9 "../$ZIP_NAME" * -x "README.md" "LICENSE"
    cd ..
    
    echo "[OK] Kernel Zip is ready at: $(pwd)/$ZIP_NAME"
else
    echo "[FAIL] Kernel Image was not generated."
    exit 1
fi
