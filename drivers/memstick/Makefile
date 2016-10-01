#
# Makefile for the kernel MemoryStick device drivers.
#

ifeq ($(CONFIG_THUMB2),y)
AFLAGS_AUTOIT	:=$(call as-option,-Wa$(comma)-mimplicit-it=always,-Wa$(comma)-mauto-it)
AFLAGS_NOWARN	:=$(call as-option,-Wa$(comma)-mno-warn-deprecated,-Wa$(comma)-W)
CFLAGS_THUMB2	:=-mthumb $(AFLAGS_AUTOIT) $(AFLAGS_NOWARN)
AFLAGS_THUMB2	:=$(CFLAGS_THUMB2) -Wa$(comma)-mthumb
KBUILD_CFLAGS	+= $(CFLAGS_THUMB2)
KBUILD_AFLAGS	+= $(AFLAGS_THUMB2)
endif

subdir-ccflags-$(CONFIG_MEMSTICK_DEBUG) := -DDEBUG

obj-$(CONFIG_MEMSTICK)		+= core/
obj-$(CONFIG_MEMSTICK)		+= host/

