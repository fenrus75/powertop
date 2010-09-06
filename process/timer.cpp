#include <map>
#include <utility>

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "timer.h"
#include "../lib.h"
#include "process.h"

using namespace std;


timer::timer(unsigned long address)
{
	strncpy(handler, kernel_function(address), 31);
	wake_ups = 0;
	disk_hits = 0;
	accumulated_runtime = 0;
	child_runtime = 0;
	waker = NULL;
	raw_count = 0;
}


static map<unsigned long, class timer *> all_timers;
static map<unsigned long, uint64_t> running_since;

void timer::fire(uint64_t time, uint64_t timer_struct)
{
	running_since[timer_struct] = time;
}

uint64_t timer::done(uint64_t time, uint64_t timer_struct)
{
	int64_t delta;

	if (running_since[timer_struct] > time)
		return 0;

	delta = time - running_since[timer_struct];
	if (delta < 0)
		printf("GOT HERE %llin", delta);
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


const char * timer::description(void)
{
	if (child_runtime > accumulated_runtime)
		child_runtime = 0;

	sprintf(desc, "Timer  %23s      time  %5.2fms    wakeups %4.1f  (total: %i)",
			handler,  (accumulated_runtime - child_runtime) / 1000000.0 / measurement_time, 
			wake_ups / measurement_time, raw_count);
	return desc;
}


class timer * find_create_timer(uint64_t func)
{
	class timer * timer;
	if (all_timers[func])
		return all_timers[func];

	timer = new class timer(func);
	all_timers[func] = timer;
	return timer;
	
}

void clear_timers(void)
{
	all_timers.erase(all_timers.begin(), all_timers.end());	
}