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
#include <memory>
#include <utility>

#include <cstdint>
#include <cstdio>

#include "work.h"
#include "../lib.h"
#include "process.h"


work::work(unsigned long address) : power_consumer()
{
	handler = kernel_function(address);
	raw_count = 0;
	desc = handler;
}


static std::map<unsigned long, std::unique_ptr<class work>> all_work;
static std::map<unsigned long, uint64_t> running_since;

void work::fire(uint64_t time, uint64_t work_struct)
{
	running_since[work_struct] = time;
}

uint64_t work::done(uint64_t time, uint64_t work_struct)
{
	int64_t delta;

	if (running_since.find(work_struct) == running_since.end())
		return ~0ULL;

	if (running_since[work_struct] > time)
		return 0;

	delta = time - running_since[work_struct];

	accumulated_runtime += delta;

	raw_count++;

	return delta;
}

double work::usage_summary(void) const
{
	const double t = (accumulated_runtime - child_runtime) / 1000000.0 / measurement_time / 10;
	return t;
}

std::string work::usage_units_summary(void) const
{
	return "%";
}




static void add_work(const std::pair<const unsigned long, std::unique_ptr<class work>>& elem)
{
	all_power.push_back(elem.second.get());
}

void all_work_to_all_power(void)
{
	std::for_each(all_work.begin(), all_work.end(), add_work);

}

void clear_work(void)
{
	all_work.clear();
	running_since.clear();
}


std::string work::description(void)
{
	if (child_runtime > accumulated_runtime)
		child_runtime = 0;

	return desc;
}


class work * find_create_work(uint64_t func)
{
	if (all_work.find(func) != all_work.end())
		return all_work[func].get();

	auto work_ptr = std::make_unique<class work>(func);
	class work *work = work_ptr.get();
	all_work[func] = std::move(work_ptr);
	return work;
}

void work::collect_json_fields(std::string &_js) const
{
    power_consumer::collect_json_fields(_js);
    JSON_FIELD(desc);
    JSON_FIELD(handler);
    JSON_FIELD(raw_count);
}
