/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef _MACHINA_MACHINA_H
#define _MACHINA_MACHINA_H

#include <machina/types.h>

mcn_msgioret_t mcn_msgio(mcn_msgopt_t option, mcn_portid_t recv, unsigned long timeout, mcn_portid_t notify);
mcn_portid_t mcn_reply_port(void);

#endif
