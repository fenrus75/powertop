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
#ifndef _INCLUDE_GUARD_PERF_BUNDLE_H_
#define _INCLUDE_GUARD_PERF_BUNDLE_H_

#include <iostream>
#include <vector>
#include <map>

using namespace std;

#include "perf.h"
class perf_event;


class  perf_bundle {
protected:
	vector<class perf_event *> events;
	std::map<int, char*> event_names;
public:
	vector<void *> records;
	virtual ~perf_bundle() {};

	virtual void release(void);
	bool add_event(const char *event_name);

	void start(void);
	void stop(void);
	void clear(void);

	void process(void);

	virtual void handle_trace_point(void *trace, int cpu = 0, uint64_t time = 0);
};


#endif
