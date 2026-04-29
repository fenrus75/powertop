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
#pragma once

#include <vector>
#include <string>

#include <sys/time.h>
#include "cpudevice.h"
#include "rapl/rapl_interface.h"

class dram_rapl_device: public cpudevice {

	c_rapl_interface *rapl = nullptr;
	time_t		last_time = 0;
	double		last_energy = 0.0;
	double 		consumed_power = 0.0;
	bool		device_valid = false;

public:
	dram_rapl_device(cpudevice *parent, const std::string &classname = "dram_core", const std::string &device_name = "dram_core", class abstract_cpu *_cpu = nullptr);
	~dram_rapl_device() { delete rapl; }
	virtual std::string device_name(void) override {return "DRAM";};
	virtual std::string human_name(void) override {return "DRAM";};
	bool device_present() { return device_valid;}
	virtual double power_usage(struct result_bundle *result, struct parameter_bundle *bundle) override;
	void start_measurement(void) override;
	void end_measurement(void) override;
	void collect_json_fields(std::string &_js) override;

};

