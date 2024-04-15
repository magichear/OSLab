#ifndef OSLAB2
#define OSLAB2

/*My syscalls define*/
struct my_processinfo {
	int pid;			      //line1608
	char comm[16];	                      // TASK_COMM_LEN
	long state;  			      //line1495 -1 unrunnable, 0 runnable, >0 stopped
					      // Here use 1 to match "isrunning"
	unsigned long long process_runtime;   //line1518 use sched_entity se->sum_exec_runtime
	unsigned long long process_startime;  //line1666
	int sorted;			      // Record if this process is sorted by CPU rate
// typedef unsigned long long u64   types.h        line11
// typedef int __kernel_pid_t       posix_types.h  line27
// typedef __kernel_pid_t pid_t     types.h        line21
};

#endif
