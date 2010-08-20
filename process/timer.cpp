#include <map>
#include <utility>

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "timer.h"
#include "../lib.h"

using namespace std;


timer::timer(unsigned long address)
{
	running_since = 0;
	strncpy(handler, kernel_function(address), 31);
	wake_ups = 0;
	disk_hits = 0;
	accumulated_runtime = 0;
	child_runtime = 0;
	waker = NULL;
	raw_count = 0;
}


static map<unsigned long, class timer *> all_timers;

void timer::fire(uint64_t time)
{
	running_since = time;
}

uint64_t timer::done(uint64_t time)
{
	uint64_t delta;

	delta = time - running_since;
	accumulated_runtime += delta;
	raw_count++;

	return delta;
}



static void add_timer(const pair<unsigned long, class timer*>& elem)
{
	all_power.push_back(elem.second);
}

void all_timers_to_all_power(void)
{
	for_each(all_timers.begin(), all_timers.end(), add_timer);

}


double timer::Witts(void)
{
	double cost;

	cost = 0.1 * wake_ups + ( (accumulated_runtime - child_runtime) / 1000000.0);

	return cost;
}

const char * timer::description(void)
{
	sprintf(desc, "Timer  %23s      time  %5.1fms    wakeups %3i  (total: %i)",
			handler,  (accumulated_runtime - child_runtime) / 1000000.0, wake_ups, raw_count);
	return desc;
}


class timer * find_create_timer(uint64_t func)
{
	/* FIXME: need to do start/stop on a per cpu basis! */
	
}