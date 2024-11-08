#include <stdio.h>
#include <nux/syscalls.h>
#include <machina/message.h>
#include <machina/syscalls.h>
#include <machina/machina.h>
#include <machina/mig.h>
#include <machina/error.h>
#include <string.h>

#include <ks.h>
#include <cs.h>
#include <cs_server.h>

void
putchar (int c)
{
  (void) syscall1 (4096, c);
}

void
exit (int status)
{
  syscall1 (4097, status);
}

void
test (void)
{
  syscall0 (0);
  syscall1 (1, 1);
  syscall2 (2, 1, 2);
  syscall3 (3, 1, 2, 3);
  syscall4 (4, 1, 2, 3, 4);
  syscall5 (5, 1, 2, 3, 4, 5);
  syscall6 (6, 1, 2, 3, 4, 5, 6);
}

int
puts (const char *s)
{
  char c;

  while ((c = *s++) != '\0')
    putchar (c);

  return 0;
}

int
th1(void)
{

  while(1)
    {
      syscall_msgrecv(3, MCN_MSGOPT_NONE, 0, MCN_PORTID_NULL);
      cstest_replies_t t;
      cstest_server((mcn_msgheader_t *)syscall_msgbuf(), (mcn_msgheader_t *)&t);
      memcpy(syscall_msgbuf(), (void *)&t, sizeof(t));
    }
  //  syscall_msgsend(MCN_MSGOPT_NONE, 0, MCN_PORTID_NULL);

}

char stack[64*1024];

int
main (void)
{
  puts ("Hello from userspace, NUX!\n");

  test();

  printf("msgbuf: %p", syscall_msgbuf());
  printf("ALlocated port %ld\n", mcn_reply_port());
  printf("ALlocated port %ld\n", mcn_reply_port());
  printf("ALlocated port %ld\n", mcn_reply_port());
  printf("ALlocated port %ld\n", mcn_reply_port());

  printf("Calling simple! %x\n", simple(1));

  long cnt;
  printf("Calling inc! %x\n", inc(1, &cnt));
  printf("cnt: %ld\n", cnt);

  printf("Calling inc! %x\n", inc(1, &cnt));
  printf("cnt: %ld\n", cnt);

  printf("Calling inc! %x\n", inc(1, &cnt));
  printf("cnt: %ld\n", cnt);

  printf("Calling add 3! %x\n", add(1, 3, &cnt));
  printf("cnt: %ld\n", cnt);

  printf("Calling mul 4! %x\n", mul(1, 4, &cnt));
  printf("cnt: %ld\n", cnt);
  
  {
    printf("************************* TEST START *****************************\n");
    mcn_portid_t tmp_port = mcn_reply_port();
    volatile struct mcn_msgheader *msgh = (struct mcn_msgheader *) syscall_msgbuf();
    msgh->msgh_bits = MCN_MSGBITS(MCN_MSGTYPE_MAKESEND, MCN_MSGTYPE_MAKESEND);
    msgh->msgh_size = 40;
    msgh->msgh_remote = 2;
    msgh->msgh_local = tmp_port;
    msgh->msgh_msgid = 2000;
    asm volatile ("" ::: "memory");
    printf("MSGIORET: %x", syscall_msgsend(MCN_MSGOPT_NONE, 0, MCN_PORTID_NULL));
    printf("MSGIORET: %x", syscall_msgrecv(2, MCN_MSGOPT_NONE, 0, MCN_PORTID_NULL));

    volatile struct mcn_msgheader *msgr = (struct mcn_msgheader *) syscall_msgbuf();
    printf("BITS: %lx\n", msgr->msgh_bits);
    printf("LOCAL: %lx\n", msgr->msgh_local);
    printf("REMOTE: %lx\n", msgr->msgh_remote);
    printf("SEQNO: %lx\n", msgr->msgh_seqno);
    printf("MSGID: %ld\n", msgr->msgh_msgid);

    printf("************************* TEST END *******************************\n");
  }

  volatile struct mcn_msgheader *msgh = (struct mcn_msgheader *) syscall_msgbuf();
  msgh->msgh_bits = MCN_MSGBITS(MCN_MSGTYPE_MAKESEND, MCN_MSGTYPE_MAKESEND);
  msgh->msgh_size = 40;
  msgh->msgh_remote = 3;
  msgh->msgh_local = 3;
  msgh->msgh_msgid = 505050;
  printf("MSGIORET: %x", syscall_msgsend(MCN_MSGOPT_NONE, 0, MCN_PORTID_NULL));
  printf("MSGIORET: %x", syscall_msgrecv(3, MCN_MSGOPT_NONE, 0, MCN_PORTID_NULL));

  for (int i = 0; i < 2*2 + 2; i++)
    printf("s: %d\n", simple(3));
  
  create_thread(1, (long)th1, (long)stack + 64 * 1024);

  //  while(1)
  //    printf("s: %d\n", simple(3));

  return 42;
}

static int c = 0;

mcn_return_t
__srv_user_simple(mcn_portid_t port)
{
  printf("\t\t\t\tSimple port %d!\n", port);
  return KERN_SUCCESS;
}

mcn_return_t
__srv_user_inc(mcn_portid_t port, long *new)
{
  printf("Inc!\n");
  c+=1;
  *new = c;
  return KERN_SUCCESS;
}

mcn_return_t
__srv_user_add(mcn_portid_t port, long b, long *new)
{
  printf("Inc!\n");
  c += b;
  *new = c;
  return KERN_SUCCESS;
}

mcn_return_t
__srv_user_mul(mcn_portid_t port, int b, long *new)
{
  printf("Inc!\n");
  c *= b;
  *new = c;
  return KERN_SUCCESS;
}
