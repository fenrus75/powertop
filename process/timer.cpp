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
	waker = NULL;
	raw_count = 0;
}


static map<unsigned long, class timer_list *> all_timer_list;

static map<unsigned long, class timer *> all_timers;



void timer_arm(uint64_t timer_address, uint64_t timer_func)
{
	class timer_list *timer;

	timer = all_timer_list[timer_address];
	if (timer) {
		timer->timer_func = timer_func;
		return;
	}

	timer = new timer_list();
	timer->timer_address = timer_address;
	timer->timer_func = timer_func;
	all_timer_list[timer_address] = timer;
}

class timer * timer_fire(uint64_t timer_address, uint64_t timer_func, uint64_t time)
{
	class timer_list *timer_list;
	class timer *timer;
	timer_list = all_timer_list[timer_address];

	if (!timer_list) {
		timer_list = new class timer_list();
		timer_list->timer_address = timer_address;
		timer_list->timer_func = timer_func;
		all_timer_list[timer_address] = timer_list;
	}

	timer = all_timers[timer_list->timer_func];
	if (!timer) {
		timer = new class timer(timer_list->timer_func);
		all_timers[timer_list->timer_func] = timer;
	}
	timer->fire(time);
	return timer;
}

void timer_done(uint64_t timer_address, uint64_t time)
{
	class timer_list *timer_list;
	class timer *timer;

	timer_list = all_timer_list[timer_address];
	if (!timer_list)
		return;
	timer = all_timers[timer_list->timer_func];
	if (!timer)
		return;
	timer->done(time);
}

void timer_cancel(uint64_t timer_address)
{
	class timer_list *timer;

	timer = all_timer_list[timer_address];

	all_timer_list[timer_address] = NULL;
	delete timer;	
}


void timer::fire(uint64_t time)
{
	running_since = time;
}

void timer::done(uint64_t time)
{
	uint64_t delta;

	delta = time - running_since;
	accumulated_runtime += delta;
	raw_count++;
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

	cost = 0.1 * wake_ups + (accumulated_runtime / 1000000.0);

	return cost;
}

const char * timer::description(void)
{
	sprintf(desc, "Timer  %23s      time  %5.1fms    wakeups %3i  (total: %i)",
			handler,  accumulated_runtime / 1000000.0, wake_ups, raw_count);
	return desc;
}
