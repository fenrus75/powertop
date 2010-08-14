


#include "process.h"

void process::account_disk_dirty(void)
{
	disk_hits++;
}

void process::schedule_thread(uint64_t time, int thread_id, int from_idle)
{

	if (from_idle)
		wake_ups++;
}


void process::deschedule_thread(uint64_t time, int thread_id)
{
}
