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
#include <map>
#include <utility>

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "timer.h"
#include "../lib.h"
#include "process.h"

using namespace std;


timer::timer(unsigned long address) : power_consumer()
{
	strncpy(handler, kernel_function(address), 31);
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

	accumulated_runtime += delta;

	raw_count++;

	return delta;
}

double timer::usage_summary(void) 
{ 
	double t;
	t = (accumulated_runtime - child_runtime) / 1000000.0 / measurement_time / 10;
	return t; 
}

const char * timer::usage_units_summary(void)
{
	return "%";
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

	sprintf(desc, "%s", handler);
	return desc;
}


class timer * find_create_timer(uint64_t func)
{
	class timer * timer;
	if (all_timers.find(func) != all_timers.end())
		return all_timers[func];

	timer = new class timer(func);
	all_timers[func] = timer;
	return timer;
	
}

void clear_timers(void)
{
	std::map<unsigned long, class timer *>::iterator it = all_timers.begin();
	while (it != all_timers.end()) {
		delete it->second;
		all_timers.erase(it);
		it = all_timers.begin();
	}
}

bool get_timerstats(void)
{
	FILE *file;
	file = fopen("/proc/timer_stats", "w");
	if (!file) {
		return false;
	}
	fprintf(file, "1\n");
	fclose(file);
	return true;
}

bool timer::is_deferred(void)
{
	FILE *file;
	char line[4096];

	if (!get_timerstats()){
		return false;
	}
	file = fopen("/proc/timer_stats", "r");
	if (!file) {
		return false;
	}

	while (file && !feof(file)) {
		char *c;
		if (fgets(line, 4096,file)== NULL)
			break;
		if (strstr(line, "total events"))
			break;
		if (!handler)
			break;
		if (strstr(line, handler)){
			c = strchr(line, ',');
			if (!c)
				continue;
			c--;
			if (*c == 'D') {
				fclose(file);
				return true;
			}
		}
	}
	fclose(file);
	return false;
}