#pragma once
#include <linux/device.h>
#include <linux/types.h>

#define ORK_VHEAP_CMA_ENABLED 1
#define ORK_VHEAP_MB          96   // você pode aumentar para 96/128 depois

int ork_vheap_init(struct device *dev);
void ork_vheap_term(struct device *dev);
void *ork_vheap_alloc(size_t nr_pages);
void ork_vheap_free(void *addr, size_t nr_pages);
