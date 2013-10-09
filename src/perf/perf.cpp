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

#include <iostream>
#include <fstream>

#include <errno.h>
#include <unistd.h>
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
#include "../lib.h"
#include "../display.h"

struct pevent *perf_event::pevent;

static int get_trace_type(const char *eventname)
{
	string str;
	int this_trace;

	str = read_sysfs_string("/sys/kernel/debug/tracing/events/%s/id",
					eventname);
	if (str.length() < 1)
		return -1;

	this_trace = strtoull(str.c_str(), NULL, 10);
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

void perf_event::create_perf_event(char *eventname, int _cpu)
{
	struct perf_event_attr attr;
	int ret;
	int err;

	struct {
		__u64 count;
		__u64 time_enabled;
		__u64 time_running;
		__u64 id;
	} read_data;

	if (perf_fd != -1)
		clear();

	memset(&attr, 0, sizeof(attr));

	attr.read_format	= PERF_FORMAT_TOTAL_TIME_ENABLED |
				  PERF_FORMAT_TOTAL_TIME_RUNNING |
				  PERF_FORMAT_ID;

	attr.sample_freq	= 0;
	attr.sample_period	= 1;
	attr.sample_type	|= PERF_SAMPLE_RAW | PERF_SAMPLE_CPU | PERF_SAMPLE_TIME;

	attr.mmap		= 1;
	attr.comm		= 1;
	attr.inherit		= 0;
	attr.disabled		= 1;

	attr.type		= PERF_TYPE_TRACEPOINT;
	attr.config		= trace_type;

	if (attr.config <= 0)
		return;

	perf_fd = sys_perf_event_open(&attr, -1, _cpu, -1, 0);

	if (perf_fd < 0) {
		err = errno;
		reset_display();
		if (err == EMFILE)
			fprintf(stderr, _("Too many open files, please increase the limit of open file descriptors.\n"));
		else {
			fprintf(stderr, _("PowerTOP %s needs the kernel to support the 'perf' subsystem\n"), POWERTOP_VERSION);
			fprintf(stderr, _("as well as support for trace points in the kernel:\n"));
			fprintf(stderr, "CONFIG_PERF_EVENTS=y\nCONFIG_PERF_COUNTERS=y\nCONFIG_TRACEPOINTS=y\nCONFIG_TRACING=y\n");
		}
		exit(EXIT_FAILURE);
	}
	if (read(perf_fd, &read_data, sizeof(read_data)) == -1) {
		reset_display();
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

	ret = ioctl(perf_fd, PERF_EVENT_IOC_ENABLE, 0);

	if (ret < 0) {
		fprintf(stderr, "failed to enable perf \n");
	}

	pc = (perf_event_mmap_page *)perf_mmap;
	data_mmap = (unsigned char *)perf_mmap + getpagesize();


}

void perf_event::set_event_name(const char *event_name)
{
	if (name)
		free(name);
	name = strdup(event_name);
	if (!name) {
		fprintf(stderr, "failed to allocate event name\n");
		return;
	}

	char *c;

	c = strchr(name, ':');
	if (c)
		*c = '/';

	trace_type = get_trace_type(name);
}

perf_event::~perf_event(void)
{
	if (name)
		free(name);

	if (perf_event::pevent->ref_count == 1) {
		pevent_free(perf_event::pevent);
		perf_event::pevent = NULL;
		clear();
	} else
		pevent_unref(perf_event::pevent);
}

void perf_event::set_cpu(int _cpu)
{
	cpu = _cpu;
}

static void allocate_pevent(void)
{
	if (!perf_event::pevent)
		perf_event::pevent = pevent_alloc();
	else
		pevent_ref(perf_event::pevent);
}

perf_event::perf_event(const char *event_name, int _cpu, int buffer_size)
{
	allocate_pevent();
	name = NULL;
	perf_fd = -1;
	bufsize = buffer_size;
	cpu = _cpu;
	perf_mmap = NULL;
	trace_type = 0;
	set_event_name(event_name);
}

perf_event::perf_event(void)
{
	allocate_pevent();
	name = NULL;
	perf_fd = -1;
	bufsize = 128;
	perf_mmap = NULL;
	cpu = 0;
	trace_type = 0;
}

void perf_event::start(void)
{
	create_perf_event(name, cpu);
}

void perf_event::stop(void)
{
	int ret;
	ret = ioctl(perf_fd, PERF_EVENT_IOC_DISABLE, 0);
	if (ret)
		cout << "stop failing\n";
}

void perf_event::process(void *cookie)
{
	struct perf_event_header *header;

	if (perf_fd < 0)
		return;

	while (pc->data_tail != pc->data_head ) {
		while (pc->data_tail >= (unsigned int)bufsize * getpagesize())
			pc->data_tail -= bufsize * getpagesize();

		header = (struct perf_event_header *)( (unsigned char *)data_mmap + pc->data_tail);

		if (header->size == 0)
			break;

		pc->data_tail += header->size;

		while (pc->data_tail >= (unsigned int)bufsize * getpagesize())
			pc->data_tail -= bufsize * getpagesize();

		if (header->type == PERF_RECORD_SAMPLE)
			handle_event(header, cookie);
	}
	pc->data_tail = pc->data_head;
}

void perf_event::clear(void)
{
	if (perf_mmap) {
//		memset(perf_mmap, 0, (bufsize)*getpagesize());
		munmap(perf_mmap, (bufsize+1)*getpagesize());
		perf_mmap = NULL;
	}
	if (perf_fd != -1)
		close(perf_fd);
	perf_fd = -1;
}
