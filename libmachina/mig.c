#include <machina/mig.h>
#include <machina/machina.h>

static mcn_portid_t __mig_reply_port = MCN_PORTID_NULL; /* XXX: MAKE __THREAD __thread PER THREAD GIANLUCA */

mcn_portid_t
mig_get_reply_port(void)
{
  //  if (__mig_reply_port == MCN_PORTID_NULL)
    __mig_reply_port = mcn_reply_port();

  return __mig_reply_port;
}

void
mig_dealloc_reply_port(void)
{
  /* XXX: IMPLEMENT ME */
  __mig_reply_port = MCN_PORTID_NULL;
}
