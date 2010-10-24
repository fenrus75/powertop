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

#include "work.h"
#include "../lib.h"
#include "process.h"

using namespace std;


work::work(unsigned long address)
{
	strncpy(handler, kernel_function(address), 31);
	raw_count = 0;
}


static map<unsigned long, class work *> all_work;
static map<unsigned long, uint64_t> running_since;

void work::fire(uint64_t time, uint64_t work_struct)
{
	running_since[work_struct] = time;
}

uint64_t work::done(uint64_t time, uint64_t work_struct)
{
	int64_t delta;

	if (running_since[work_struct] > time)
		return 0;

	delta = time - running_since[work_struct];
	if (delta < 0)
		printf("GOT HERE %llin", delta);
	accumulated_runtime += delta;

	raw_count++;

	return delta;
}



static void add_work(const pair<unsigned long, class work*>& elem)
{
	all_power.push_back(elem.second);
}

void all_work_to_all_power(void)
{
	for_each(all_work.begin(), all_work.end(), add_work);

}

void delete_all_work(void)
{
	all_work.erase(all_work.begin(), all_work.end());
}


const char * work::description(void)
{
	if (child_runtime > accumulated_runtime)
		child_runtime = 0;

	sprintf(desc, "%s", handler);
	return desc;
}


class work * find_create_work(uint64_t func)
{
	class work * work;
	if (all_work[func])
		return all_work[func];

	work = new class work(func);
	all_work[func] = work;
	return work;
	
}

