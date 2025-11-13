// SPDX-License-Identifier: GPL-2.0
#include <linux/dma-mapping.h>
#include <linux/genalloc.h>
#include <linux/cma.h>
#include <linux/mm.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include "ork_vheap_cma.h"

#define ORK_VHEAP_TAG "ORK_VHEAP"

static struct {
    struct gen_pool *pool;
    phys_addr_t paddr;
    void *vaddr;
    size_t size;
    bool ready;
    struct mutex lock;
} vheap;

int ork_vheap_init(struct device *dev)
{
    mutex_init(&vheap.lock);

    pr_info("%s: initializing CMA-backed virtual heap\n", ORK_VHEAP_TAG);

    vheap.size = ORK_VHEAP_MB * 1024 * 1024;

    vheap.vaddr = dma_alloc_coherent(dev, vheap.size, &vheap.paddr, GFP_KERNEL);
    if (!vheap.vaddr) {
        pr_err("%s: failed to allocate %zu bytes from CMA\n",
               ORK_VHEAP_TAG, vheap.size);
        return -ENOMEM;
    }

    vheap.pool = gen_pool_create(PAGE_SHIFT, -1);
    if (!vheap.pool) {
        pr_err("%s: failed to create gen_pool\n", ORK_VHEAP_TAG);
        dma_free_coherent(dev, vheap.size, vheap.vaddr, vheap.paddr);
        return -ENOMEM;
    }

    gen_pool_add(vheap.pool, (unsigned long)vheap.vaddr, vheap.size, -1);

    vheap.ready = true;

    pr_info("%s: CMA pool created: %zu MB at vaddr %p (phys %pa)\n",
            ORK_VHEAP_TAG, ORK_VHEAP_MB, vheap.vaddr, &vheap.paddr);

    return 0;
}

void ork_vheap_term(struct device *dev)
{
    if (!vheap.ready)
        return;

    gen_pool_destroy(vheap.pool);
    dma_free_coherent(dev, vheap.size, vheap.vaddr, vheap.paddr);

    vheap.pool = NULL;
    vheap.vaddr = NULL;
    vheap.ready = false;

    pr_info("%s: terminated\n", ORK_VHEAP_TAG);
}

void *ork_vheap_alloc(size_t nr_pages)
{
    if (!vheap.ready)
        return NULL;

    size_t size = nr_pages * PAGE_SIZE;

    mutex_lock(&vheap.lock);
    void *ptr = (void *)gen_pool_alloc(vheap.pool, size);
    mutex_unlock(&vheap.lock);

    return ptr;
}

void ork_vheap_free(void *addr, size_t nr_pages)
{
    if (!vheap.ready || !addr)
        return;

    size_t size = nr_pages * PAGE_SIZE;

    mutex_lock(&vheap.lock);
    gen_pool_free(vheap.pool, (unsigned long)addr, size);
    mutex_unlock(&vheap.lock);
}
