


#include "process.h"
#include <string.h>
#include <stdio.h>

vector <class process *> all_processes;

void process::account_disk_dirty(void)
{
	disk_hits++;
}

void process::schedule_thread(uint64_t time, int thread_id)
{
	running_since = time;
	running = 1;
}


uint64_t process::deschedule_thread(uint64_t time, int thread_id)
{
	uint64_t delta;

	if (!running_since)
		return 0;

	delta = time - running_since;

	if (time < running_since)
		printf("%llu time    %llu since \n", time, running_since);

	if (thread_id == 0) /* idle thread */
		delta = 0;
	accumulated_runtime += delta;
	running = 0;

	return delta;
}


process::process(const char *_comm, int _pid)
{
	strcpy(comm, _comm);
	pid = _pid;
	is_idle = 0;
	running = 0;
	last_waker = NULL;
	waker = NULL;

	if (strncmp(_comm, "kondemand/", 10) == 0)
		is_idle = 1;
}

const char * process::description(void)
{

	if (child_runtime > accumulated_runtime)
		child_runtime = 0;
	sprintf(desc, "%s", comm);
	return desc;
}


class process * find_create_process(char *comm, int pid)
{
	unsigned int i;
	class process *new_proc;

	for (i = 0; i < all_processes.size(); i++) {
		if (all_processes[i]->pid == pid && strcmp(comm, all_processes[i]->comm) == 0)
			return all_processes[i];
	}

	new_proc = new class process(comm, pid);
	all_processes.push_back(new_proc);
	return new_proc;
}


static void merge_process(class process *one, class process *two)
{
	one->accumulated_runtime += two->accumulated_runtime;
	one->child_runtime += two->child_runtime;
	one->wake_ups += two->wake_ups;
	one->disk_hits += two->disk_hits;

	two->accumulated_runtime = 0;
	two->child_runtime = 0;
	two->wake_ups = 0;
	two->disk_hits = 0;
}


void merge_processes(void)
{
	unsigned int i,j;
	class process *one, *two;
	/* find dupes and add up */
	for (i = 0; i < all_processes.size() ; i++) {
		one = all_processes[i];
		for (j = i + 1; j < all_processes.size(); j++) {
			two = all_processes[j];
			if (strcmp(one->comm, two->comm) == 0)
				merge_process(one, two);
		}
	}

}

void all_processes_to_all_power(void)
{
	unsigned int i;
	for (i = 0; i < all_processes.size() ; i++)
		if (all_processes[i]->accumulated_runtime)
			all_power.push_back(all_processes[i]);
}
