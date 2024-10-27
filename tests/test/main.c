#include <stdio.h>
#include <nux/syscalls.h>
#include <machina/message.h>
#include <machina/syscalls.h>
#include <machina/machina.h>
#include <machina/mig.h>

#include <ks.h>

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
main (void)
{
  puts ("Hello from userspace, NUX!\n");

  test();

  printf("msgbuf: %p", syscall_msgbuf());

  {
  volatile struct mcn_msgsend *msgh = (struct mcn_msgsend *) syscall_msgbuf();
  msgh->msgs_bits = MCN_MSGBITS(MCN_MSGTYPE_COPYSEND, 0);
  msgh->msgs_size = 0;
  msgh->msgs_remote = 1;
  msgh->msgs_local = MCN_PORTID_NULL;
  msgh->msgs_msgid = 101;
  asm volatile ("" ::: "memory");
  printf("MSGIORET: %d", syscall_msgio(MCN_MSGOPT_SEND, MCN_PORTID_NULL, 0, MCN_PORTID_NULL));
  }

    {
  volatile struct mcn_msgsend *msgh = (struct mcn_msgsend *) syscall_msgbuf();
  msgh->msgs_bits = MCN_MSGBITS(MCN_MSGTYPE_COPYSEND, 0);
  msgh->msgs_size = 32;
  msgh->msgs_remote = 1;
  msgh->msgs_local = MCN_PORTID_NULL;
  msgh->msgs_msgid = 101;
  asm volatile ("" ::: "memory");
  printf("MSGIORET: %d", syscall_msgio(MCN_MSGOPT_SEND, MCN_PORTID_NULL, 0, MCN_PORTID_NULL));
  }


    printf("ALlocated port %ld\n", mcn_reply_port());
    printf("ALlocated port %ld\n", mcn_reply_port());
    printf("ALlocated port %ld\n", mcn_reply_port());
    printf("ALlocated port %ld\n", mcn_reply_port());

  printf("Calling simple! %d\n", simple(1));

  long cnt;
  printf("Calling inc! %d\n", inc(1, &cnt));
  printf("cnt: %ld\n", cnt);

  printf("Calling inc! %d\n", inc(1, &cnt));
  printf("cnt: %ld\n", cnt);

  printf("Calling inc! %d\n", inc(1, &cnt));
  printf("cnt: %ld\n", cnt);

  printf("Calling add 3! %d\n", add(1, 3, &cnt));
  printf("cnt: %ld\n", cnt);

  printf("Calling mul 4! %d\n", mul(1, 4, &cnt));
  printf("cnt: %ld\n", cnt);


    {
        mcn_portid_t tmp_port = mcn_reply_port();
	volatile struct mcn_msgsend *msgh = (struct mcn_msgsend *) syscall_msgbuf();
	msgh->msgs_bits = MCN_MSGBITS(MCN_MSGTYPE_COPYSEND, MCN_MSGTYPE_MAKEONCE);
	msgh->msgs_size = 32;
	msgh->msgs_remote = 1;
	msgh->msgs_local = tmp_port;
	msgh->msgs_msgid = 2000;
	asm volatile ("" ::: "memory");
	printf("MSGIORET: %d", syscall_msgio(MCN_MSGOPT_SEND|MCN_MSGOPT_RECV, mig_get_reply_port(), 0, MCN_PORTID_NULL));
    }

  return 42;
}
