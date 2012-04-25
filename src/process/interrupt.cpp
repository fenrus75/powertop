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
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "process.h"
#include "interrupt.h"
#include "../lib.h"

const char* softirqs[] = {
	"HI_SOFTIRQ",
	"timer(softirq)",
	"net tx(softirq)",
	"net_rx(softirq)",
	"block(softirq)",
	"block_iopoll(softirq)",
	"tasklet(softirq)",
	"sched(softirq)",
	"hrtimer(softirq)",
	"RCU(softirq)",
	NULL
};


interrupt::interrupt(const char *_handler, int _number) : power_consumer()
{
	char buf[128];
	running_since = 0;
	number = _number;
	strncpy(handler, _handler, 31);
	raw_count = 0;
	sprintf(desc, "[%i] %s", number, pretty_print(handler, buf, 128));
}


vector <class interrupt *> all_interrupts;

void interrupt::start_interrupt(uint64_t time)
{
	running_since = time;
	raw_count ++;
}

uint64_t interrupt::end_interrupt(uint64_t time)
{
	uint64_t delta;

	delta = time - running_since;
	accumulated_runtime += delta;
	return delta;
}

const char * interrupt::description(void)
{
	if (child_runtime > accumulated_runtime)
		child_runtime = 0;
	return desc;
}

double interrupt::usage_summary(void)
{
	double t;
	t = (accumulated_runtime - child_runtime) / 1000000.0 / measurement_time / 10;
	return t;
}

const char * interrupt::usage_units_summary(void)
{
	return "%";
}


class interrupt * find_create_interrupt(const char *_handler, int nr, int cpu)
{
	char handler[64];
	unsigned int i;
	class interrupt *new_irq;

	strcpy(handler, _handler);
	if (strcmp(handler, "timer")==0)
		sprintf(handler, "timer/%i", cpu);


	for (i = 0; i < all_interrupts.size(); i++) {
		if (all_interrupts[i] && all_interrupts[i]->number == nr && strcmp(handler, all_interrupts[i]->handler) == 0)
			return all_interrupts[i];
	}

	new_irq = new class interrupt(handler, nr);
	all_interrupts.push_back(new_irq);
	return new_irq;
}

void all_interrupts_to_all_power(void)
{
	unsigned int i;
	for (i = 0; i < all_interrupts.size() ; i++)
		if (all_interrupts[i]->accumulated_runtime)
			all_power.push_back(all_interrupts[i]);
}

void clear_interrupts(void)
{
	std::vector<class interrupt *>::iterator it = all_interrupts.begin();
	while (it != all_interrupts.end()) {
		delete *it;
		it = all_interrupts.erase(it);
	}
}
