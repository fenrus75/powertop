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


#include "device.h"

class i915gpu: public device {
	int index = 0;
	int rindex = 0;
	std::vector<device *>child_devices;

public:

	i915gpu();

	virtual void start_measurement(void) override;
	virtual void end_measurement(void) override;

	virtual double	utilization(void) override; /* percentage */

	virtual std::string class_name(void) override { return "GPU";};

	virtual std::string device_name(void) override {
		if (child_devices.size())
			return "GPU misc";
		return "GPU";
	};
	virtual std::string human_name(void) override { return "Intel GPU"; };
	virtual double power_usage(struct result_bundle *result, struct parameter_bundle *bundle) override;
	virtual bool show_in_list(void) override {return false;};
	virtual std::string util_units(void) override { return " ops/s"; };

	virtual void add_child(device *dev_ptr) { child_devices.push_back(dev_ptr);}
};

extern void create_i915_gpu(void);

