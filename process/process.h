#ifndef _INCLUDE_GUARD_PROCESS_H
#define _INCLUDE_GUARD_PROCESS_H

#include <stdint.h>

#include "powerconsumer.h"


/*
Need to collect
 * CPU time consumed by each application
 * Number of wakeups out of idle -- received and wakeups sent
 * Number of disk dirties (inode for now)
 */
class process : public power_consumer {
	uint64_t	running_since;
	char		desc[256];
public:
	char 		comm[16];
	int 		pid;


	int		is_idle;   /* count this as if the cpu was idle */
	int 		running;

	process(const char *_comm, int _pid);

	virtual void schedule_thread(uint64_t time, int thread_id);
	virtual void deschedule_thread(uint64_t time, int thread_id = 0);

	virtual void account_disk_dirty(void);

	virtual double Witts(void);
	virtual const char * description(void);

};

extern vector <class process *> all_processes;



extern void start_process_measurement(void);
extern void end_process_measurement(void);
extern void process_process_data(void);
extern void end_process_data(void);
extern void merge_processes(void);

extern class process * find_create_process(char *comm, int pid);
extern void all_processes_to_all_power(void);





#define TASK_COMM_LEN 16
struct sched_switch {
	char prev_comm[TASK_COMM_LEN];
	int  prev_pid;
	int  prev_prio;
	long prev_state; /* Arjan weeps. */
	char next_comm[TASK_COMM_LEN];
	int  next_pid;
	int  next_prio;
};

struct irq_entry {
	int irq;
	int len;
	char handler[16];
};



struct wakeup_entry {
	char comm[TASK_COMM_LEN];
	int   pid;
	int   prio;
	int   success;
};


struct irq_exit {
	int irq;
	int ret;
};

struct  softirq_entry {
	uint32_t vec;
};

struct timer_start {
	void		*timer;
	void		*function;
};

struct timer_cancel {
	void		*timer;
};

struct timer_expire {
	void		*timer;
	unsigned long	now;
	void		*function;
};


#endif