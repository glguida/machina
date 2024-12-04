/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef _MACHINA_MIG_H_
#define _MACHINA_MIG_H_

#include <machina/types.h>
#include <machina/mig_errors.h>

void mig_strncpy(char *dst, char *src, int len);
mcn_portid_t mig_get_reply_port (void);
void mig_dealloc_reply_port (void);

#endif
