


#include "process.h"
#include <string.h>
#include <stdio.h>

void process::account_disk_dirty(void)
{
	disk_hits++;
}

void process::schedule_thread(uint64_t time, int thread_id, int from_idle)
{

	if (from_idle && !is_idle)
		wake_ups++;

	running_since = time;
}


void process::deschedule_thread(uint64_t time, int thread_id)
{
	uint64_t delta;

	delta = time - running_since;
	accumulated_runtime += delta;
}


process::process(const char *_comm, int _pid)
{
	strcpy(comm, _comm);
	pid = _pid;
	accumulated_runtime = 0;
	wake_ups = 0;
	disk_hits = 0;
	is_idle = 0;

	if (strncmp(_comm, "kondemand/", 10) == 0)
		is_idle = 1;
}