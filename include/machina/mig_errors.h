/*
  MACHINA: a NUX-based Mach clone.
  Copyright (C) 2024 Gianluca Guida, glguida@tlbflush.org
  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef _MACHINA_MIG_ERRORS_H_
#define _MACHINA_MIG_ERRORS_H_

#include <machina/types.h>
#include <machina/message.h>

#define MIG_TYPE_ERROR -300
#define MIG_REPLY_MISMATCH -301
#define MIG_REMOTE_ERROR -302
#define MIG_BAD_ID -303
#define MIG_BAD_ARGUMENTS -304
#define MIG_NO_REPLY -305
#define MIG_EXCEPTION -306
#define MIG_ARRAY_TOO_LARGE -307
#define MIG_SERVER_DIED -308
#define MIG_DESTROY_REQUEST -309

typedef struct {
  mcn_msgheader_t Head;
  mcn_msgtype_t RetCodeType;
  mcn_return_t RetCode;
} mig_reply_header_t;

#endif
