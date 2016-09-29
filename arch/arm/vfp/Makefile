#
# linux/arch/arm/vfp/Makefile
#
# Copyright (C) 2001 ARM Limited
#

KBUILD_CFLAGS	+= -mfpu=neon -ftree-vectorize -ftree-loop-vectorize -ftree-slp-vectorize -fvect-cost-model
KBUILD_AFLAGS	+= -mfpu=neon -ftree-vectorize -ftree-loop-vectorize -ftree-slp-vectorize -fvect-cost-model

obj-y			+= vfp.o

vfp-$(CONFIG_VFP)	+= vfpmodule.o entry.o vfphw.o vfpsingle.o vfpdouble.o
