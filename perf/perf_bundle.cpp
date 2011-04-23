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
#include <malloc.h>
#include <algorithm>
#include <string.h>
#include <stdint.h>

#include "perf_bundle.h"
#include "perf_event.h"
#include "perf.h"

#include "../cpu/cpu.h"

class perf_bundle_event: public perf_event 
{
public:
	perf_bundle_event(void);
	virtual void handle_event(struct perf_event_header *header, void *cookie);
};

perf_bundle_event::perf_bundle_event(void) : perf_event()
{
}


void perf_bundle_event::handle_event(struct perf_event_header *header, void *cookie)
{
	unsigned char *buffer;
	vector<void *> *vector;

	buffer = (unsigned char *)malloc(header->size);
	memcpy(buffer, header, header->size);	

	vector = (typeof(vector))cookie;
	vector->push_back(buffer);
}


void perf_bundle::release(void) 
{
	class perf_event *ev;
	unsigned int i = 0;

	for (i = 0; i < events.size(); i++) {
		ev = events[i];
		if (!ev)
			continue;
		ev->clear();
		delete ev;
	}
	events.clear();

	for (i = 0; i < event_names.size(); i++) {
		free((void*)event_names[i]);
	}
	event_names.clear();

	for(i = 0; i < records.size(); i++) {
		free(records[i]);
	}
	records.clear();
}


void perf_bundle::add_event(const char *event_name)
{
	unsigned int i;
	class perf_event *ev;

	for (i = 0; i < all_cpus.size(); i++) {

		if (!all_cpus[i])
			continue;

		ev = new class perf_bundle_event();

		ev->set_event_name(event_name);
		ev->set_cpu(i);

		if ((int)ev->trace_type >= 0) {
			if (event_names.find(ev->trace_type) == event_names.end())
				event_names[ev->trace_type] = strdup(event_name);

			events.push_back(ev);
		} else {
			delete ev;
		}
	}
}

void perf_bundle::start(void)
{
	unsigned int i;
	class perf_event *ev;

	for (i = 0; i < events.size(); i++) {
		ev = events[i];
		if (!ev)
			continue;
		ev->start();
	}		
}
void perf_bundle::stop(void)
{
	unsigned int i;
	class perf_event *ev;

	for (i = 0; i < events.size(); i++) {
		ev = events[i];
		if (!ev)
			continue;
		ev->stop();
	}		
}
void perf_bundle::clear(void)
{
	unsigned int i;

	class perf_event *ev;

	for (i = 0; i < events.size(); i++) {
		ev = events[i];
		if (!ev)
			continue;
		ev->clear();
	}		

	for (i = 0; i < records.size(); i++) {
		free(records[i]);
	}
	records.resize(0);
}


struct trace_entry {
	uint64_t		time;
	uint32_t		cpu;
	uint32_t		res;
	__u32			size;
	unsigned short		type;
	unsigned char		flags;
	unsigned char		preempt_count;
	int			pid;
	int			lock_depth;
} __attribute__((packed));;


struct perf_sample {
	struct perf_event_header        header;
	struct trace_entry		trace;
	unsigned char			data[0];
} __attribute__((packed));

static uint64_t timestamp(perf_event_header *event)
{
	struct perf_sample *sample;

	if (event->type != PERF_RECORD_SAMPLE)
		return 0;

	sample = (struct perf_sample *)event;

#if 0
	int i;
	unsigned char *x;

	printf("header:\n");
	printf("	type  is %x \n", sample->header.type);
	printf("	misc  is %x \n", sample->header.misc);
	printf("	size  is %i \n", sample->header.size);
	printf("sample:\n");
	printf("	time  is %llx \n", sample->trace.time);
	printf("	cpu   is %i / %x \n", sample->trace.cpu, sample->trace.cpu);
	printf("	res   is %i / %x \n", sample->trace.res, sample->trace.res);
	printf("	size  is %i / %x \n", sample->trace.size, sample->trace.size);
	printf("	type  is %i / %x \n", sample->trace.type, sample->trace.type);
	printf("	flags is %i / %x \n", sample->trace.flags, sample->trace.flags);
	printf("	p/c   is %i / %x \n", sample->trace.preempt_count, sample->trace.preempt_count);
	printf("	pid   is %i / %x \n", sample->trace.pid, sample->trace.pid);
	printf("	lock dept  is %i / %x \n", sample->trace.lock_depth, sample->trace.lock_depth);

	x = (unsigned char *)sample;
	for (i = 0; i < sample->header.size; i++)
		printf("%02x ", *(x+i));
	printf("\n");
#endif
	return sample->trace.time;
	
}

static bool event_sort_function (void *i, void *j) 
{ 
	struct perf_event_header *I, *J;

	I = (struct perf_event_header *) i;
	J = (struct perf_event_header *) j;
	return (timestamp(I)<timestamp(J)); 
}

void perf_bundle::process(void)
{
	unsigned int i;
	class perf_event *ev;

	/* fixme: reserve enough space in the array in one go */
	for (i = 0; i < events.size(); i++) {
		ev = events[i];
		if (!ev)
			continue;
		ev->process(&records);
	}		
	sort(records.begin(), records.end(), event_sort_function);

	for (i = 0; i < records.size(); i++) {
		struct perf_sample *sample;

		sample = (struct perf_sample *)records[i];
		if (!sample)
			continue;

		if (sample->header.type != PERF_RECORD_SAMPLE)
			continue;

		handle_trace_point(sample->trace.type, &sample->data, sample->trace.cpu, sample->trace.time, sample->trace.flags);
		
	}
}

void perf_bundle::handle_trace_point(int type, void *trace, int cpu, uint64_t time, unsigned char flags)
{
	printf("UH OH... abstract handle_trace_point called\n");
}

