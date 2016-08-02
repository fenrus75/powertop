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
#include "../lib.h"


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
	char line[4097];
	ifstream file;
	ssize_t pos;

	pt_strcpy(comm, _comm);
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
			line[4096] = '\0';
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

	pos = snprintf(desc, sizeof(desc), "[PID %d] ", pid);

	if (pos < 0)
		pos = 0;
	if ((size_t)pos > sizeof(desc))
		return;

	strncpy(desc + pos, comm, sizeof(desc) - pos - 1);
	desc[sizeof(desc) - 1] = '\0';

	sprintf(line, "/proc/%i/cmdline", _pid);
	file.open(line, ios::binary);
	if (file) {
		memset(line, 0, sizeof(line));
		file.read(line, 4096);
		file.close();
		if (strlen(line) < 1) {
			is_kernel = 1;
			snprintf(desc + pos, sizeof(desc) - pos, "[%s]", comm);
		} else {
			int sz = sizeof(desc) - pos - 1;
			cmdline_to_string(line);
			strncpy(desc + pos, line, sz);
			desc[sz] = 0x00;
		}
	}
}

const char * process::description(void)
{

	if (child_runtime > accumulated_runtime)
		child_runtime = 0;

	return desc;
}

double process::usage_summary(void)
{
	double t;
	t = (accumulated_runtime - child_runtime) / 1000000.0 / measurement_time / 10;
	return t;
}

const char * process::usage_units_summary(void)
{
	return "%";
}

class process * find_create_process(const char *comm, int pid)
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

class process * find_create_process(char *comm, int pid)
{
	return find_create_process((const char*)comm, pid);
}

static void merge_process(class process *one, class process *two)
{
	one->accumulated_runtime += two->accumulated_runtime;
	one->child_runtime += two->child_runtime;
	one->wake_ups += two->wake_ups;
	one->disk_hits += two->disk_hits;
	one->hard_disk_hits += two->hard_disk_hits;
	one->xwakes += two->xwakes;
	one->gpu_ops += two->gpu_ops;
	one->power_charge += two->power_charge;
}


void merge_processes(void)
{
	std::vector<class process*>::iterator it1, it2;
	class process *one, *two;

	it1 = all_processes.begin();
	while (it1 != all_processes.end()) {
		it2 = it1 + 1;
		one = *it1;
		while (it2 != all_processes.end()) {
			two = *it2;
			/* fold threads */
			if (one->pid == two->tgid && two->tgid != 0) {
				merge_process(one, two);
				delete *it2;
				it2 = all_processes.erase(it2);
				continue;
			}
			/* find dupes and add up */
			if (!strcmp(one->desc, two->desc)) {
				merge_process(one, two);
				delete *it2;
				it2 = all_processes.erase(it2);
				continue;
			}
			++it2;
		}
		++it1;
	}
}

void all_processes_to_all_power(void)
{
	unsigned int i;
	for (i = 0; i < all_processes.size() ; i++)
		if (all_processes[i]->accumulated_runtime ||
		    all_processes[i]->power_charge)
			all_power.push_back(all_processes[i]);
}

void clear_processes(void)
{
	std::vector <class process *>::iterator it = all_processes.begin();
	while (it != all_processes.end()) {
		delete *it;
		it = all_processes.erase(it);
	}
}
