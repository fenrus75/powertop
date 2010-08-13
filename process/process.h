#ifndef _INCLUDE_GUARD_PROCESS_H
#define _INCLUDE_GUARD_PROCESS_H


/*
Need to collect
 * CPU time consumed by each application
 * Number of wakeups out of idle -- received and wakeups sent
 * Number of disk dirties (inode for now)
 */
class process {
	char comm[16];

	uint64_t accumulated_runtime;

	public:

	virtual void schedule_thread(uint64_t time, int thread_id, int from_idle = 0);
	virtual void deschedule_thread(uint64_t time, int thread_id);

	virtual void account_disk_dirty(void)
	
	
};

#endif