#include <iostream>
#include <malloc.h>
#include <algorithm>
#include <string.h>
#include <stdint.h>

#include "perf_bundle.h"
#include "perf_event.h"
#include "perf.h"

class perf_bundle_event: public perf_event 
{
	virtual void handle_event(struct perf_event_header *header, void *cookie);
};


void perf_bundle_event::handle_event(struct perf_event_header *header, void *cookie)
{
	unsigned char *buffer;
	vector<void *> *vector;

	buffer = (unsigned char *)malloc(header->size);
	memcpy(buffer, header, header->size);	

	vector = (typeof(vector))cookie;
	vector->push_back(buffer);
}


void perf_bundle::add_event(const char *event_name)
{
	class perf_event *ev;

	ev = new class perf_bundle_event();

	ev->set_event_name(event_name);

	events.push_back(ev);
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
}

struct perf_sample {
	struct perf_event_header        header;
	uint64_t 			ip; 
	uint32_t 			pid, tid;
	uint64_t			time;
};

static uint64_t timestamp(perf_event_header *event)
{
	struct perf_sample *sample;
	if (event->type != PERF_RECORD_SAMPLE)
		return 0;

	sample = (struct perf_sample *)event;
	return sample->time;
	
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

	printf("We got %u records total \n", records.size());
}

