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
 *	Srinivas Pandruvada <Srinivas.Pandruvada@linux.intel.com>
 */
#ifndef _INCLUDE_GUARD_DRAM_RAPL_DEVICE_H
#define _INCLUDE_GUARD_DRAM_RAPL_DEVICE_H

#include <vector>
#include <string>

using namespace std;

#include <sys/time.h>
#include "cpudevice.h"
#include "rapl/rapl_interface.h"

class dram_rapl_device: public cpudevice {

	c_rapl_interface *rapl;
	time_t		last_time;
	double		last_energy;
	double 		consumed_power;
	bool		device_valid;

public:
	dram_rapl_device(cpudevice *parent, const char *classname = "dram_core", const char *device_name = "dram_core", class abstract_cpu *_cpu = NULL);
	~dram_rapl_device() { delete rapl; }
	virtual const char * device_name(void) {return "DRAM";};
	bool device_present() { return device_valid;}
	virtual double power_usage(struct result_bundle *result, struct parameter_bundle *bundle);
	void start_measurement(void);
	void end_measurement(void);

};


#endif
