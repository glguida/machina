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
SRCROOT=../../../
BUILDROOT=../../../
MKDIR=$(SRCROOT)/nux/mk
INSTALLDIR=NONE
CFLAGS+=-O7 -g -Werror -I/nux/include -I/nux

all: do_all

INCDIR=include/mach
INCSUBDIRS=machine

INCS+= \
boolean.h \
boot_info.h \
error.h \
exception.h \
host_info.h \
kern_return.h \
mach_param.h \
mach_traps.h \
mach_types.h \
machine.h \
memory_object.h \
message.h \
mig_errors.h \
msg_type.h \
norma_special_ports.h \
notify.h \
policy.h \
port.h \
processor_info.h \
std_types.h \
syscall_sw.h \
task_info.h \
task_special_ports.h \
thread_info.h \
thread_special_ports.h \
thread_status.h \
thread_switch.h \
time_value.h \
vm_attributes.h \
vm_inherit.h \
vm_param.h \
vm_prot.h \
vm_statistics.h

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

