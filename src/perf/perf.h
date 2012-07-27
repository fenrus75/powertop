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
#ifndef _INCLUDE_GUARD_PERF_H_
#define _INCLUDE_GUARD_PERF_H_

#include <iostream>


extern "C" {
	#include "../traceevent/event-parse.h"
}


using namespace std;

class  perf_event {
protected:
	int perf_fd;
	void * perf_mmap;
	void * data_mmap;
	struct perf_event_mmap_page *pc;



	int bufsize;
	char *name;
	int cpu;
	void create_perf_event(char *eventname, int cpu);

public:
	unsigned int trace_type;

	perf_event(void);
	perf_event(const char *event_name, int cpu = 0, int buffer_size = 128);

	virtual ~perf_event(void);


	void set_event_name(const char *event_name);
	void set_cpu(int cpu);

	void start(void);
	void stop(void);
	void clear(void);

	void process(void *cookie);

	virtual void handle_event(struct perf_event_header *header, void *cookie) { };

	static struct pevent *pevent;

};

#endif
