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
#ifndef __INCLUDE_GUARD_POWER_CONSUMER_
#define __INCLUDE_GUARD_POWER_CONSUMER_

#include <stdint.h>
#include <vector>
#include <algorithm>

using namespace std;

extern double measurement_time;

class power_consumer;

class power_consumer {

public:
	uint64_t	accumulated_runtime;
	uint64_t	child_runtime;
	int	 	disk_hits;
	int		wake_ups;
	int		gpu_ops;
	class power_consumer *waker;
	class power_consumer *last_waker;

	power_consumer(void);

	virtual double Witts(void);
	virtual const char * description(void) { return ""; };

	virtual const char * name(void) { return "abstract"; };
	virtual const char * type(void) { return "abstract"; };

	virtual double usage(void) { return (accumulated_runtime - child_runtime) / 1000000.0 / measurement_time;};
	virtual const char * usage_units(void) {return "msec";};
	virtual double events(void) { return  (wake_ups + gpu_ops) / measurement_time;};
	virtual int show_events(void) { return 1; };
};

extern vector <class power_consumer *> all_power;

extern double total_wakeups(void);
extern double total_cpu_time(void);
extern double total_gpu_ops(void);



#endif