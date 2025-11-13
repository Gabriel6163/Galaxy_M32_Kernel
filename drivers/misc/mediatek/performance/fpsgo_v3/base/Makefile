#
# Copyright (C) 2017 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#

MTK_TOP = $(srctree)/drivers/misc/mediatek/
FPSGO_TOP = $(MTK_TOP)/performance/fpsgo_v3/
BASE_TOP = $(FPSGO_TOP)/base/
COMP_TOP = $(FPSGO_TOP)/composer/
FBT_TOP = $(FPSGO_TOP)/fbt/

# Compila o código base normal do FPSGO
obj-y += \
	src/fpsgo_base.o \

# ===========================================================================
# CORREÇÃO DEFINITIVA: Compila o fpsgo_sysfs.c em um objeto chamado
# mtk_fpsgo.o. Isso força a criação da pasta /sys/module/mtk_fpsgo/
# para compatibilidade com os scripts da comunidade.
# ===========================================================================
obj-y += mtk_fpsgo.o
mtk_fpsgo-objs := src/fpsgo_sysfs.o

ccflags-y += \
	-I$(srctree)/include/ \
	-I$(MTK_TOP)/include/ \
	-I$(BASE_TOP)/include/ \
	-I$(COMP_TOP)/include/ \
	-I$(FBT_TOP)/include/ \
