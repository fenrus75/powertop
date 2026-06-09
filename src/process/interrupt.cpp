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
#include <cstdint>
#include <cstdio>
#include "process.h"
#include "interrupt.h"
#include "../lib.h"

constexpr std::array<std::string_view, 10> softirqs = {
	"HI_SOFTIRQ",
	"timer(softirq)",
	"net tx(softirq)",
	"net_rx(softirq)",
	"block(softirq)",
	"block_iopoll(softirq)",
	"tasklet(softirq)",
	"sched(softirq)",
	"hrtimer(softirq)",
	"RCU(softirq)"
};


interrupt::interrupt(const std::string &_handler, int _number) : power_consumer()
{
	running_since = 0;
	number = _number;
	handler = _handler;
	raw_count = 0;
	desc = std::format("[{}] {}", number, pretty_print(handler));
}


std::vector<std::unique_ptr<interrupt>> all_interrupts;

void interrupt::start_interrupt(uint64_t time)
{
	running_since = time;
	raw_count ++;
}

uint64_t interrupt::end_interrupt(uint64_t time)
{
	const uint64_t delta = time - running_since;

	accumulated_runtime += delta;
	return delta;
}

std::string interrupt::description(void)
{
	if (child_runtime > accumulated_runtime)
		child_runtime = 0;
	return desc;
}

double interrupt::usage_summary(void) const
{
	const double t = (accumulated_runtime - child_runtime) / 1000000.0 / measurement_time / 10;
	return t;
}

std::string interrupt::usage_units_summary(void) const
{
	return "%";
}


class interrupt * find_create_interrupt(const std::string &_handler, int nr, int cpu)
{
	std::string handler_s;

	handler_s = _handler;
	if (handler_s == "timer")
		handler_s = std::format("timer/{}", cpu);


	for (const auto &irq : all_interrupts) {
		if (irq->number == nr && irq->handler == handler_s)
			return irq.get();
	}

	all_interrupts.push_back(std::make_unique<interrupt>(handler_s, nr));
	return all_interrupts.back().get();
}

void all_interrupts_to_all_power(void)
{
	for (const auto &irq : all_interrupts)
		if (irq->accumulated_runtime)
			all_power.push_back(irq.get());
}

void clear_interrupts(void)
{
	all_interrupts.clear();
}

void interrupt::collect_json_fields(std::string &_js) const
{
    power_consumer::collect_json_fields(_js);
    JSON_FIELD(running_since);
    JSON_FIELD(desc);
    JSON_FIELD(handler);
    JSON_FIELD(number);
    JSON_FIELD(raw_count);
}
