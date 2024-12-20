#ifdef NUXPERF_DECLARE
#define NUXPERF(_s) extern nuxperf_t __perf _s
#endif

#ifdef NUXPERF_DEFINE
#define NUXPERF(_s) nuxperf_t __perf _s = { .name = #_s , .val = 0 }
#endif

NUXPERF(pmachina_sysc_msgbuf);
NUXPERF(pmachina_sysc_msgrecv);
NUXPERF(pmachina_sysc_msgsend);
NUXPERF(pmachina_sysc_reply_port);
NUXPERF(pmachina_sysc_task_self);
NUXPERF(pmachina_sysc_vm_map);
NUXPERF(pmachina_sysc_vm_allocate);
NUXPERF(pmachina_sysc_vm_region);

NUXPERF(pmachina_cpu_kick);

NUXPERF(pmachina_ipc_send_invaliddata);
NUXPERF(pmachina_ipc_send_internfailed);
NUXPERF(pmachina_ipc_send_enqueuefailed);
NUXPERF(pmachina_ipc_send_success);

NUXPERF(pmachina_ipc_recv_invalidname);
NUXPERF(pmachina_ipc_recv_dequeuefailed);
NUXPERF(pmachina_ipc_recv_success);

NUXPERF(pmachina_cacheobj_addmapping);
NUXPERF(pmachina_cacheobj_updatemapping);
NUXPERF(pmachina_cacheobj_delmapping);
NUXPERF(pmachina_cacheobj_shadow);
NUXPERF(pmachina_cacheobj_map);
NUXPERF(pmachina_cacheobj_lookup);
