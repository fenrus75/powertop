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
#pragma once

#include <vector>
#include <string>

#include "../devices/device.h"
#include "cpu.h"

class cpudevice: public device {
protected:
	std::string _class;
	std::string _cpuname;

	std::vector<std::string> params;
	class abstract_cpu *cpu = nullptr;
	int wake_index = 0;
	int consumption_index = 0;
	int r_wake_index = 0;
	int r_consumption_index = 0;

	std::vector<device *>child_devices;

public:
	cpudevice(const std::string &classname = "cpu", const std::string &device_name = "cpu0", class abstract_cpu *_cpu = nullptr);
	virtual std::string class_name(void) override { return _class; };

	virtual std::string device_name(void) override;
	virtual std::string human_name(void) override;

	virtual double power_usage(struct result_bundle *result, struct parameter_bundle *bundle) override;
	virtual bool show_in_list(void) override {return false;};
	virtual double	utilization(void) override; /* percentage */
	void add_child(device *dev_ptr) { child_devices.push_back(dev_ptr);}
};

