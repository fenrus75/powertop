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
#ifndef _INCLUDE_GUARD_CPUDEVICE_H
#define _INCLUDE_GUARD_CPUDEVICE_H

#include <vector>
#include <string>

using namespace std;

#include "../devices/device.h"
#include "cpu.h"

class cpudevice: public device {
	char _class[128];
	char _cpuname[128];

	vector<string> params;
	class abstract_cpu *cpu;
	int wake_index;
	int consumption_index;
	int r_wake_index;
	int r_consumption_index;

public:
	cpudevice(const char *classname = "cpu", const char *device_name = "cpu0", class abstract_cpu *_cpu = NULL);
	virtual const char * class_name(void) { return _class;};

	virtual const char * device_name(void) {return "CPU use";};

	virtual double power_usage(struct result_bundle *result, struct parameter_bundle *bundle);
	virtual bool show_in_list(void) {return false;};
	virtual double	utilization(void); /* percentage */
};


#endif