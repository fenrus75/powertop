/*
 * Copyright 2010, Intel Corporation
 *
 * This file is part of PowerTOP
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 * or just google for it.
 *
 * Authors:
 *	Arjan van de Ven <arjan@linux.intel.com>
 */
#ifndef _INCLUDE_GUARD_PROCESS_H
#define _INCLUDE_GUARD_PROCESS_H

#include <stdint.h>

#include "powerconsumer.h"

#ifdef __x86_64__
#define BIT64 1 
#endif

/*
Need to collect
 * CPU time consumed by each application
 * Number of wakeups out of idle -- received and wakeups sent
 * Number of disk dirties (inode for now)
 */
class process : public power_consumer {
	uint64_t	running_since;
public:
	char		desc[256];
	int		tgid;
	char 		comm[16];
	int 		pid;


	int		is_idle;   /* count this as if the cpu was idle */
	int 		running;
	int		is_kernel; /* kernel thread */

	process(const char *_comm, int _pid, int _tid = 0);

	virtual void schedule_thread(uint64_t time, int thread_id);
	virtual uint64_t deschedule_thread(uint64_t time, int thread_id = 0);

	virtual void account_disk_dirty(void);

	virtual const char * description(void);
	virtual const char * name(void) { return "process"; };
	virtual const char * type(void) { return "Process"; };

	virtual double usage_summary(void);
	virtual const char * usage_units_summary(void);

};

extern vector <class process *> all_processes;

extern double measurement_time;


extern void start_process_measurement(void);
extern void end_process_measurement(void);
extern void process_process_data(void);
extern void end_process_data(void);
extern void clear_process_data(void);
extern void merge_processes(void);

extern class process * find_create_process(char *comm, int pid);
extern void all_processes_to_all_power(void);

extern void clear_processes(void);
extern void process_update_display(void);
extern void html_process_update_display(void);
extern void html_summary(void);



extern void clear_timers(void);



#define TASK_COMM_LEN 16
struct sched_switch {
	char prev_comm[TASK_COMM_LEN];
	int  prev_pid;
	int  prev_prio;
	long prev_state; /* Arjan weeps. */
#ifdef BIT64
	int dummy;
#endif
	char next_comm[TASK_COMM_LEN];
	int  next_pid;
	int  next_prio;
} __attribute__((packed));

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
#ifdef BIT64
	int padding;
#endif
	void		*timer;
	void		*function;
} __attribute__((packed));;

struct timer_cancel {
#ifdef BIT64
	int padding;
#endif
	void		*timer;
} __attribute__((packed));;

struct timer_expire {
#ifdef BIT64
	int padding;
#endif
	void		*timer;
	unsigned long	now;
	void		*function;
} __attribute__((packed));;
struct hrtimer_expire {
#ifdef BIT64
	int padding;
#endif
	void		*timer;
	int64_t		now;
	void		*function;
} __attribute__((packed));;
struct workqueue_start {
#ifdef BIT64
	int padding;
#endif
	void		*work;
	void		*function;
} __attribute__((packed));;
struct workqueue_end {
#ifdef BIT64
	int padding;
#endif
	void		*work;
} __attribute__((packed));

struct  dirty_inode {
	uint32_t dev;
	uint32_t inode;
	uint32_t flags;
};


#endif
