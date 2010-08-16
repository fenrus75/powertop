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

	virtual void schedule_thread(uint64_t time, int thread_id, int from_idle = 0);
	virtual void deschedule_thread(uint64_t time, int thread_id = 0);

	virtual void account_disk_dirty(void);

	virtual double Witts(void);
	virtual const char * description(void);

};



extern void start_process_measurement(void);
extern void end_process_measurement(void);
extern void process_process_data(void);



#endif