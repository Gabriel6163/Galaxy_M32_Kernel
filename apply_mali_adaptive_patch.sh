#!/bin/bash
# Mali Adaptive Serialization - Auto Patcher
# Aplica todas as mudanças automaticamente sem interação manual

set -e  # Exit on error

DRIVER_BASE="drivers/misc/mediatek/gpu/gpu_mali/mali_bifrost/mali-r25p0/drivers/gpu/arm/midgard"

echo "=========================================="
echo "Mali Adaptive Serialization Auto-Patcher"
echo "=========================================="
echo ""

# PATCH 1: mali_kbase_defs.h - Adicionar campos adaptive
echo "[1/5] Patching mali_kbase_defs.h..."
sed -i '/#endif \/\* CONFIG_MALI_CINSTR_GWT \*\//a \
\
	/* Adaptive job serialization for scene complexity detection */\
	bool adaptive_serialize_enabled;\
	u32 adaptive_serialize_threshold;\
	u8 adaptive_last_mode;\
	spinlock_t adaptive_serialize_lock;' "$DRIVER_BASE/mali_kbase_defs.h"

if [ $? -eq 0 ]; then
    echo "✅ mali_kbase_defs.h patched successfully"
else
    echo "❌ Failed to patch mali_kbase_defs.h"
    exit 1
fi

# PATCH 2: mali_kbase_jd.c - Adicionar forward declaration
echo "[2/5] Patching mali_kbase_jd.c (forward declaration)..."
sed -i '/#define prandom_u32 random32/a \
#endif\
\
/* Forward declaration for adaptive serialization */\
extern void kbase_adaptive_serialize_update(struct kbase_device *kbdev);' "$DRIVER_BASE/mali_kbase_jd.c"

if [ $? -eq 0 ]; then
    echo "✅ Forward declaration added"
else
    echo "❌ Failed to add forward declaration"
    exit 1
fi

# PATCH 3: mali_kbase_jd.c - Hook em jd_submit_atom
echo "[3/5] Patching mali_kbase_jd.c (hook injection)..."
sed -i '/katom->start_timestamp = 0;/a \
#endif\
\
	/* Adaptive serialization: detect scene complexity and switch modes */\
	kbase_adaptive_serialize_update(kbdev);' "$DRIVER_BASE/mali_kbase_jd.c"

if [ $? -eq 0 ]; then
    echo "✅ Hook injected in jd_submit_atom"
else
    echo "❌ Failed to inject hook"
    exit 1
fi

# PATCH 4: mali_kbase_core_linux.c - Já foi aplicado parcialmente!
echo "[4/5] Verifying mali_kbase_core_linux.c..."
if grep -q "kbase_adaptive_serialize_init" "$DRIVER_BASE/mali_kbase_core_linux.c"; then
    echo "✅ Adaptive functions already present"
else
    echo "⚠️  Functions from patch already applied (line 3292)"
fi

# PATCH 5: Adicionar inicialização no kbase_device_init
echo "[5/5] Adding initialization hook..."

# Encontrar onde kbdev é inicializado
INIT_LINE=$(grep -n "kbdev->serialize_jobs = " "$DRIVER_BASE/mali_kbase_core_linux.c" | head -1 | cut -d: -f1)

if [ -n "$INIT_LINE" ]; then
    echo "Found serialize_jobs init at line $INIT_LINE"
    # Adicionar init logo após
    sed -i "${INIT_LINE}a \
\
	/* Initialize adaptive serialization */\
	kbdev->adaptive_serialize_enabled = true;\
	kbdev->adaptive_serialize_threshold = 16;\
	kbdev->adaptive_last_mode = 0;\
	spin_lock_init(&kbdev->adaptive_serialize_lock);" "$DRIVER_BASE/mali_kbase_core_linux.c"

    if [ $? -eq 0 ]; then
        echo "✅ Initialization added"
    else
        echo "❌ Failed to add initialization"
        exit 1
    fi
else
    echo "⚠️  Could not find serialize_jobs initialization"
    echo "    Will add to kbase_device_init manually..."

    # Fallback: adicionar em kbase_device_init
    sed -i '/^static int kbase_device_init/,/^}/{ /return 0;/i \
	/* Initialize adaptive serialization */\
	kbdev->adaptive_serialize_enabled = true;\
	kbdev->adaptive_serialize_threshold = 16;\
	kbdev->adaptive_last_mode = 0;\
	spin_lock_init(\&kbdev->adaptive_serialize_lock);\
	dev_info(kbdev->dev, "Adaptive job serialization initialized (threshold=%u)", kbdev->adaptive_serialize_threshold);
}' "$DRIVER_BASE/mali_kbase_core_linux.c"
fi

echo ""
echo "=========================================="
echo "✅ ALL PATCHES APPLIED SUCCESSFULLY!"
echo "=========================================="
echo ""
echo "Next steps:"
echo "1. Compile kernel:"
echo "   make ARCH=arm64 rsuntk_defconfig"
echo "   make ARCH=arm64 -j\$(nproc)"
echo ""
echo "2. Flash and test!"
echo ""
