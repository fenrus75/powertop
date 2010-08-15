


#include "process.h"
#include <string.h>
#include <stdio.h>

void process::account_disk_dirty(void)
{
	disk_hits++;
}

void process::schedule_thread(uint64_t time, int thread_id, int from_idle)
{

	if (from_idle)
		wake_ups++;

	running_since = time;
}


void process::deschedule_thread(uint64_t time, int thread_id)
{
	uint64_t delta;

	delta = time - running_since;
	accumulated_runtime += delta;

	printf("I (%s) ran for %llu --> %llu\n", comm, delta, accumulated_runtime);
}


process::process(const char *_comm, int _pid)
{
	strcpy(comm, _comm);
	pid = _pid;
	accumulated_runtime = 0;
}