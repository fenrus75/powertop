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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "perf_bundle.h"
#include "perf_event.h"
#include "perf.h"

#include "../cpu/cpu.h"

#if defined(__GXX_EXPERIMENTAL_CXX0X__) || (__cplusplus >= 201103L)
# define USE_DECLTYPE
#endif

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

#ifdef USE_DECLTYPE
	vector = (decltype(vector))cookie;
#else
	vector = (typeof(vector))cookie;
#endif
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

static char * read_file(const char *file)
{
	char *buffer = NULL; /* quient gcc */
	char buf[4096];
	int len = 0;
	int fd;
	int r;

	fd = open(file, O_RDONLY);
	if (fd < 0)
		exit(-1);

	while((r = read(fd, buf, 4096)) > 0) {
		if (len) {
			char *tmp = (char *)realloc(buffer, len + r + 1);
			if (!tmp)
				free(buffer);
			buffer = tmp;
		} else
			buffer = (char *)malloc(r + 1);
		if (!buffer)
			goto out;
		memcpy(buffer + len, buf, r);
		len += r;
		buffer[len] = '\0';
	}
out:
	close(fd);
	return buffer;
}

static void parse_event_format(const char *event_name)
{
	char *tptr;
	char *name = strdup(event_name);
	char *sys = strtok_r(name, ":", &tptr);
	char *event = strtok_r(NULL, ":", &tptr);
	char *file;
	char *buf;

	file = (char *)malloc(strlen(sys) + strlen(event) +
		      strlen("/sys/kernel/debug/tracing/events////format") + 2);
	sprintf(file, "/sys/kernel/debug/tracing/events/%s/%s/format", sys, event);

	buf = read_file(file);
	free(file);
	if (!buf) {
		free(name);
		return;
	}

	pevent_parse_event(perf_event::pevent, buf, strlen(buf), sys);
	free(name);
	free(buf);
}

bool perf_bundle::add_event(const char *event_name)
{
	unsigned int i;
	int event_added = false;
	class perf_event *ev;


	for (i = 0; i < all_cpus.size(); i++) {

		if (!all_cpus[i])
			continue;

		ev = new class perf_bundle_event();

		ev->set_event_name(event_name);
		ev->set_cpu(i);

		if ((int)ev->trace_type >= 0) {
			if (event_names.find(ev->trace_type) == event_names.end()) {
				event_names[ev->trace_type] = strdup(event_name);
				parse_event_format(event_name);
			}
			events.push_back(ev);
			event_added = true;
		} else {
			delete ev;
		}
	}
	return event_added;
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

/*
 * sample's PERF_SAMPLE_CPU cpu nr is a raw_smp_processor_id() by the
 * time of perf_event_output(), which may differ from struct perf_event
 * cpu, thus we need to fix sample->trace.cpu.
 */
static void fixup_sample_trace_cpu(struct perf_sample *sample)
{
	struct event_format *event;
	struct pevent_record rec;
	unsigned long long cpu_nr;
	int type;
	int ret;

	rec.data = &sample->data;
	type = pevent_data_type(perf_event::pevent, &rec);
	event = pevent_find_event(perf_event::pevent, type);
	if (!event)
		return;
	/** don't touch trace if event does not contain cpu_id field*/
	ret = pevent_get_field_val(NULL, event, "cpu_id", &rec, &cpu_nr, 0);
	if (ret < 0)
		return;
	sample->trace.cpu = cpu_nr;
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

		fixup_sample_trace_cpu(sample);
		handle_trace_point(&sample->data, sample->trace.cpu, sample->trace.time);
	}
}

void perf_bundle::handle_trace_point(void *trace, int cpu, uint64_t time)
{
	printf("UH OH... abstract handle_trace_point called\n");
}
