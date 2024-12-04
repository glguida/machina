#
# top.mk
# Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>
#  SPDX-License-Identifier:	GPL2.0+
#

.PHONY: all install clean
CC=i686-unknown-elf-gcc
CCLD=i686-unknown-elf-gcc
LD=i686-unknown-elf-ld
AR=i686-unknown-elf-ar
OBJCOPY=i686-unknown-elf-objcopy
OBJDIR=./i386
SRCDIR=./
SRCROOT=../../../../
BUILDROOT=../../../../
MKDIR=$(SRCROOT)/nux/mk
INSTALLDIR=NONE
CFLAGS+=-O7 -g -Werror -I/nux/include -I/nux

all: do_all

INCDIR=include/mach/machine

INCS+= \
boolean.h \
vm_param.h \
vm_types.h \
kern_return.h

#
# bottom.mk
# Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>
#  SPDX-License-Identifier:	GPL2.0+
#

ifneq (z$(SUBDIRS),z)
include $(MKDIR)/subdir.mk
endif

ifneq (z$(SRCS),z)
include $(MKDIR)/obj.mk
else
ifneq (z$(CUSTOBJS),z)
include $(MKDIR)/obj.mk
endif
endif

ifneq (z$(LIBRARY),z)
include $(MKDIR)/lib.mk
endif

ifneq (z$(FATOBJ),z)
include $(MKDIR)/fatobj.mk
endif

ifneq (z$(PROGRAM),z)
include $(MKDIR)/exe.mk
endif

include $(MKDIR)/inc.mk

ifneq (z$(ALL_TARGET),z)
do_all: $(ALL_PREDEP)
	$(MAKE) $(ALL_TARGET)
else
do_all: $(ALL_PREDEP)
endif

install: $(INSTALL_TARGET)

clean: $(CLEAN_TARGET)
ifneq (z$(CLEAN_FILES),z)
	-rm $(CLEAN_FILES)
endif

