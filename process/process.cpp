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



#include "process.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <iostream>
#include <fstream>


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
		printf("%llu time    %llu since \n", (unsigned long long)time, 
						     (unsigned long long)running_since);

	if (thread_id == 0) /* idle thread */
		delta = 0;
	accumulated_runtime += delta;
	running = 0;

	return delta;
}

static void cmdline_to_string(char *str)
{
	char *c = str;
	char prev = 0;

	while (prev != 0 || *c  != 0) {
		prev = *c;
		if (*c == 0)
			*c = ' ';
		c++;
	}
}


process::process(const char *_comm, int _pid, int _tid) : power_consumer()
{
	char line[4096];
	ifstream file;

	strcpy(comm, _comm);
	pid = _pid;
	is_idle = 0;
	running = 0;
	last_waker = NULL;
	waker = NULL;
	is_kernel = 0;
	tgid = _tid;

	if (_tid == 0) {
		sprintf(line, "/proc/%i/status", _pid);
		file.open(line);
		while (file) {
			file.getline(line, 4096);
			if (strstr(line, "Tgid")) {
				char *c;
				c = strchr(line, ':');
				if (!c)
					continue;
				c++;
				tgid = strtoull(c, NULL, 10);
				break;
			}
		}
		file.close();
	}

	if (strncmp(_comm, "kondemand/", 10) == 0)
		is_idle = 1;

	sprintf(desc, "%s", comm);


	sprintf(line, "/proc/%i/cmdline", _pid);
	file.open(line, ios::binary);
	if (file) {
		memset(line, 0, sizeof(line));
		file.read(line, 4096);
		file.close();
		if (strlen(line) < 1) {
			is_kernel = 1;
			sprintf(desc, "[%s]", comm);
		} else {
			cmdline_to_string(line);
			strncpy(desc, line, sizeof(desc) - 1);
		}
	}
}

const char * process::description(void)
{

	if (child_runtime > accumulated_runtime)
		child_runtime = 0;

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
	one->hard_disk_hits += two->hard_disk_hits;
	one->gpu_ops += two->gpu_ops;

	two->accumulated_runtime = 0;
	two->child_runtime = 0;
	two->wake_ups = 0;
	two->disk_hits = 0;
	two->hard_disk_hits = 0;
	two->gpu_ops = 0;
}


void merge_processes(void)
{
	unsigned int i,j;
	class process *one, *two;

	/* fold threads */
	for (i = 0; i < all_processes.size() ; i++) {
		one = all_processes[i];
		for (j = i + 1; j < all_processes.size(); j++) {
			two = all_processes[j];
			if (one->pid == two->tgid && two->tgid != 0)
				merge_process(one, two);
		}
	}

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
