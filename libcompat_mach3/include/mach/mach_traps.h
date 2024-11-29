#ifndef MACH_TRAPS_H
#define MACH_TRAPS_H

#include <mach/port.h>

mach_port_t mach_reply_port(void);
mach_port_t mach_thread_self(void);
mach_port_t mach_task_self(void);
mach_port_t mach_host_self(void);

#endif
