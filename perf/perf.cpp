/*
 * Copyright 2009, Intel Corporation
 *
 * This file is part of PowerTOP
 *
 * Portions borrowed from the "perf" tool (C) Ingo Molnar and others
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
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * Authors:
 *	Arjan van de Ven <arjan@linux.intel.com>
 */

#include <iostream>
#include <fstream>

#include <unistd.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <fcntl.h>

#include "perf_event.h"

#include "perf.h"


static int get_trace_type(const char *eventname)
{
	ifstream file;

	int this_trace;

	char filename[4096];
	sprintf(filename, "/sys/kernel/debug/tracing/events/%s/id", eventname);
	file.open("/sys/kernel/debug/tracing/events/%s/id", ios::in);
	if (!file)
		return 0;

	file >> this_trace;

	file.close();
	return this_trace;
}

static inline int sys_perf_event_open(struct perf_event_attr *attr,
                      pid_t pid, int cpu, int group_fd,
                      unsigned long flags)
{
	attr->size = sizeof(*attr);
	return syscall(__NR_perf_event_open, attr, pid, cpu,
			group_fd, flags);
}

void perf_event::create_perf_event(char *eventname)
{
	struct perf_event_attr attr;
	int ret;

	struct {
		__u64 count;
		__u64 time_enabled;
		__u64 time_running;
		__u64 id;
	} read_data;

	memset(&attr, 0, sizeof(attr));

	attr.read_format	= PERF_FORMAT_TOTAL_TIME_ENABLED |
				  PERF_FORMAT_TOTAL_TIME_RUNNING |
				  PERF_FORMAT_ID;

	attr.sample_freq	= 0;
	attr.sample_period	= 1;
	attr.sample_type	|= PERF_SAMPLE_RAW;

	attr.mmap		= 1;
	attr.comm		= 1;
	attr.inherit		= 0;
	attr.disabled		= 1;

	attr.type		= PERF_TYPE_TRACEPOINT;
	attr.config		= trace_type;

	if (attr.config <= 0)
		return;

	perf_fd = sys_perf_event_open(&attr, -1, 0, -1, 0);

	if (perf_fd < 0) {
		fprintf(stderr, "Perf syscall failed: %i / %i (%s)\n", perf_fd, errno, strerror(errno));
		return;
	}
	if (read(perf_fd, &read_data, sizeof(read_data)) == -1) {
		perror("Unable to read perf file descriptor\n");
		exit(-1);
	}

	fcntl(perf_fd, F_SETFL, O_NONBLOCK);

	perf_mmap = mmap(NULL, (bufsize+1)*getpagesize(),
				PROT_READ | PROT_WRITE, MAP_SHARED, perf_fd, 0);
	if (perf_mmap == MAP_FAILED) {
		fprintf(stderr, "failed to mmap with %d (%s)\n", errno, strerror(errno));
		return;
	}

	ret = ioctl(perf_fd, PERF_EVENT_IOC_ENABLE);

	if (ret < 0)
		fprintf(stderr, "failed to enable perf \n");

	pc = (perf_event_mmap_page *)perf_mmap;
	data_mmap = (unsigned char *)perf_mmap + getpagesize();


}

perf_event::perf_event(const char *event_name, int buffer_size)
{
	name = strdup(event_name);
	char *c;

	c = strchr(name, ':');
	if (c)
		*c = '/';

	trace_type = get_trace_type(name);
	bufsize = buffer_size;
}







