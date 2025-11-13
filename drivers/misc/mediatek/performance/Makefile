# Cabeçalhos comuns
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/performance/perf_ioctl/ \
                    -I$(srctree)/drivers/misc/mediatek/performance/include/ \
                    -I$(srctree)/drivers/misc/mediatek/base/power/include/

# Perfmgr legado (mantido para compatibilidade, mas pode ser desativado)
obj-$(CONFIG_MTK_PERFMGR)     += perfmgr_main.o
obj-$(CONFIG_MTK_PERFMGR)     += base/
obj-$(CONFIG_MTK_PERFMGR)     += boost_ctrl/
obj-$(CONFIG_MTK_PERFMGR)     += syslimiter/
obj-$(CONFIG_MTK_PERFMGR)     += tchbst/

# Novos módulos auxiliares e frameworks opcionais
obj-$(CONFIG_MTK_PERF_OBSERVER) += observer/
obj-$(CONFIG_MTK_RESYM)         += resym/
obj-$(CONFIG_MTK_FPSGO_V3)      += fpsgo_v3/
obj-$(CONFIG_MTK_GBE)           += gbe/
obj-$(CONFIG_MTK_LOAD_TRACKER)  += load_track/
obj-$(CONFIG_MTK_LOAD_TRACKER)  += uload_ind/
obj-$(CONFIG_MTK_EARA_THERMAL)  += eara_thermal/
obj-y += perf_ioctl/

# Novo Perfmgr v6 (arquivo único, self-contained)
obj-$(CONFIG_MTK_PERFMGR_V6)   += perfmgr_v6.o

