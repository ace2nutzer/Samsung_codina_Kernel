ifeq ($(CONFIG_THUMB2),y)
AFLAGS_AUTOIT	:=$(call as-option,-Wa$(comma)-mimplicit-it=always,-Wa$(comma)-mauto-it)
AFLAGS_NOWARN	:=$(call as-option,-Wa$(comma)-mno-warn-deprecated,-Wa$(comma)-W)
CFLAGS_THUMB2	:=-mthumb $(AFLAGS_AUTOIT) $(AFLAGS_NOWARN)
AFLAGS_THUMB2	:=$(CFLAGS_THUMB2) -Wa$(comma)-mthumb
KBUILD_CFLAGS	+= $(CFLAGS_THUMB2)
KBUILD_AFLAGS	+= $(AFLAGS_THUMB2)
endif

prefix = /usr
nosy-dump-version = 0.4

CC = gcc

all : nosy-dump

nosy-dump : CFLAGS = -Wall
nosy-dump : CPPFLAGS = -DVERSION=\"$(nosy-dump-version)\" -I../../drivers/firewire
nosy-dump : LDFLAGS = -g
nosy-dump : LDLIBS = -lpopt

nosy-dump : nosy-dump.o decode-fcp.o

clean :
	rm -rf *.o nosy-dump

install :
	install nosy-dump $(prefix)/bin/nosy-dump
