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

#define printf(...) printf("\t\tTEST: " __VA_ARGS__);

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
th1 (void)
{

  while (1)
    {
      if (syscall_msgrecv (3, MCN_MSGOPT_NONE, 0, MCN_PORTID_NULL))
	continue;
      cstest_server ((mcn_msgheader_t *) syscall_msgbuf (),
		     (mcn_msgheader_t *) syscall_msgbuf ());
      (void) syscall_msgsend (MCN_MSGOPT_NONE, 0, MCN_PORTID_NULL);
    }

}

char stack[64 * 1024];

int
main (void)
{
  puts ("Hello from userspace, NUX!\n");

  test ();

  printf ("msgbuf: %p", syscall_msgbuf ());
  printf ("ALlocated port %ld\n", mcn_reply_port ());
  printf ("ALlocated port %ld\n", mcn_reply_port ());
  printf ("ALlocated port %ld\n", mcn_reply_port ());
  printf ("ALlocated port %ld\n", mcn_reply_port ());

  printf ("Calling simple! %x\n", simple (1));

  long cnt;
  printf ("Calling inc! %x\n", inc (1, &cnt));
  printf ("cnt: %ld\n", cnt);

  printf ("Calling inc! %x\n", inc (1, &cnt));
  printf ("cnt: %ld\n", cnt);

  printf ("Calling inc! %x\n", inc (1, &cnt));
  printf ("cnt: %ld\n", cnt);

  printf ("Calling add 3! %x\n", add (1, 3, &cnt));
  printf ("cnt: %ld\n", cnt);

  printf ("Calling mul 4! %x\n", mul (1, 4, &cnt));
  printf ("cnt: %ld\n", cnt);

  {
    printf
      ("************************* TEST START *****************************\n");
    mcn_portid_t tmp_port = mcn_reply_port ();
    volatile struct mcn_msgheader *msgh =
      (struct mcn_msgheader *) syscall_msgbuf ();
    msgh->msgh_bits =
      MCN_MSGBITS (MCN_MSGTYPE_MAKESEND, MCN_MSGTYPE_MAKESEND);
    msgh->msgh_size = 40;
    msgh->msgh_remote = 2;
    msgh->msgh_local = tmp_port;
    msgh->msgh_msgid = 2000;
    asm volatile ("":::"memory");
    printf ("MSGIORET: %x",
	    syscall_msgsend (MCN_MSGOPT_NONE, 0, MCN_PORTID_NULL));
    printf ("MSGIORET: %x",
	    syscall_msgrecv (2, MCN_MSGOPT_NONE, 0, MCN_PORTID_NULL));

    volatile struct mcn_msgheader *msgr =
      (struct mcn_msgheader *) syscall_msgbuf ();
    printf ("BITS: %lx\n", msgr->msgh_bits);
    printf ("LOCAL: %lx\n", msgr->msgh_local);
    printf ("REMOTE: %lx\n", msgr->msgh_remote);
    printf ("SEQNO: %lx\n", msgr->msgh_seqno);
    printf ("MSGID: %ld\n", msgr->msgh_msgid);

    printf
      ("************************* TEST END *******************************\n");
  }

  volatile struct mcn_msgheader *msgh =
    (struct mcn_msgheader *) syscall_msgbuf ();
  msgh->msgh_bits = MCN_MSGBITS (MCN_MSGTYPE_MAKESEND, MCN_MSGTYPE_MAKESEND);
  msgh->msgh_size = 40;
  msgh->msgh_remote = 3;
  msgh->msgh_local = 3;
  msgh->msgh_msgid = 505050;
  printf ("MSGIORET: %x",
	  syscall_msgsend (MCN_MSGOPT_NONE, 0, MCN_PORTID_NULL));
  syscall_msgrecv (3, MCN_MSGOPT_NONE, 0, MCN_PORTID_NULL);

  //  printf("create thread: %d\n",
  //     create_thread (1, (long) th1, (long) stack + 64 * 1024));

  printf ("create thread2: %d\n",
	  create_thread2 (syscall_task_self (), (long) th1,
			  (long) stack + 64 * 1024));


  //  while(1)
  //    printf("s: %d\n", simple(3));

  long a = -1;

  printf ("=======================================\n");
  printf ("=======================================\n");
  printf ("=======================================\n");
  printf ("=======================================\n");
  printf ("=======================================\n");
  printf ("=======================================\n");
  printf ("=======================================\n");
  printf ("=======================================\n");
  printf ("=======================================\n");
  printf ("=======================================\n");

  printf ("INC: %d\n", user_inc (3, &a));
  printf ("A %ld\n", a);
  printf ("INC: %d\n", user_inc (3, &a));
  printf ("A %ld\n", a);
  printf ("INC: %d\n", user_inc (3, &a));
  printf ("A %ld\n", a);

  printf ("\n--\n\n");

  volatile int *ptr = (int *) 0x2000;
  //  printf ("ptr is %lx\n", *ptr);

  //  *ptr = 1;
  //  printf ("ptr is %lx\n", *ptr);

  printf ("task_self is %lx\n", syscall_task_self ());

  mcn_vmaddr_t addr;
  printf ("vm allocate %lx\n",
	  syscall_vm_allocate (syscall_task_self (), &addr, 3 * 4096, 1));
  printf ("Allocated address %lx\n", addr);
  *(volatile int *) addr = 5;
  printf ("[%lx] is %d\n", addr, *(volatile int *) addr);


  {
    mcn_vmaddr_t newaddr;
    unsigned long size;
    mcn_vmprot_t curprot;
    mcn_vmprot_t maxprot;
    mcn_vminherit_t inherit;
    unsigned shared;
    mcn_portid_t nameid;
    mcn_vmoff_t off;

    addr = addr + 0x800;
    printf ("vm_region %lx\n",
	    syscall_vm_region (syscall_task_self (), &addr, &size, &curprot,
			       &maxprot, &inherit, &shared, &nameid, &off));
    printf
      ("\t%Addr: %lx\n\tSize: %lx\n\tCurprot: %lx\n\tMaxprot: %lx\n\tnameid: %lx\n\tOff: %lx\n",
       addr, size, curprot, maxprot, nameid, off);

    printf ("vm map %lx\n",
	    syscall_vm_map (syscall_task_self (), &newaddr, size, 0, 1,
			    nameid, off, 1, curprot, maxprot, inherit));

    printf ("New Addr is %lx\n", newaddr);
    //    printf("[%lx] = %x\n",   newaddr, *(volatile int *) newaddr);
    printf ("Writing ff to new addr\n");
    *(volatile int *) newaddr = 0xff;
    printf ("Reading from new addr\n");
    printf ("[%lx] = %x\n", newaddr, *(volatile int *) newaddr);
    printf ("Reading from old addr\n");
    printf ("[%lx] is %d\n", addr, *(volatile int *) addr);

    addr += 4096;
    newaddr += 4096;
    *(volatile int *) addr = 0xff;
    printf ("[%lx] is %d\n", addr, *(volatile int *) addr);
    printf ("[%lx] is %d\n", newaddr, *(volatile int *) newaddr);


  }

  user_sendport (3, 8, 7);

  ptr = (int *) 0x3000;
  printf ("ptr is %lx\n", *ptr);


  while (1);
  return 42;
}

static int c = 0;

mcn_return_t
__srv_user_simple (mcn_portid_t port)
{
  printf ("\t\t\t\tSimple port %d!\n", port);
  return KERN_SUCCESS;
}

mcn_return_t
__srv_user_inc (mcn_portid_t port, long *new)
{
  printf ("Inc! %d\n", c);
  c += 1;
  *new = c;
  return KERN_SUCCESS;
}

mcn_return_t
__srv_user_add (mcn_portid_t port, long b, long *new)
{
  printf ("Inc!\n");
  c += b;
  *new = c;
  return KERN_SUCCESS;
}

mcn_return_t
__srv_user_mul (mcn_portid_t port, int b, long *new)
{
  printf ("Inc!\n");
  c *= b;
  *new = c;
  return KERN_SUCCESS;
}

mcn_return_t
__srv_user_sendport (mcn_portid_t resp, mcn_portid_t port, mcn_portid_t port2)
{
  printf ("Port is %ld %ld\n", port, port2);
  return KERN_SUCCESS;
}
