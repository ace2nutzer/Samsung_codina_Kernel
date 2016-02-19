#
# linux/arch/arm/vfp/Makefile
#
# Copyright (C) 2001 ARM Limited
#

# ccflags-y := -DNDEBUG
# asflags-y := -DNDEBUG

KBUILD_AFLAGS	:= $(KBUILD_AFLAGS: -D_NDK_MATH_NO_SOFTFP=1 -mfpu=neon -mfloat-abi=hard)
LDFLAGS		+=

obj-y			+= vfp.o

vfp-$(CONFIG_VFP)	+= vfpmodule.o entry.o vfphw.o vfpsingle.o vfpdouble.o
