#include <iostream>

#include "perf_bundle.h"
#include "perf.h"

void perf_bundle::add_event(const char *event_name)
{
	class perf_event *ev;

	ev = new class perf_event(event_name);

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
void perf_bundle::process(void)
{
}

