#ifndef MACH_ERROR_H
#define MACH_ERROR_H

#include <stdio.h>
#include <mach/error.h>

static inline char *
mach_error_string (mach_error_t error_value)
{
  /*
    XXX: Implement me.
  */
  return "Generic Error (Unimplemented)";
}

static inline void
mach_error(char *str, mach_error_t error_value)
{
  /*
    XXX: Implement me.
  */
  printf("ERROR: %s (%x)\n", str, error_value);
}

static inline char *
mach_error_type (mach_error_t error_value)
{
  /*
    XXX: Implement me.
  */
  return "Generic Error (Unimplemented)";
}

#endif
