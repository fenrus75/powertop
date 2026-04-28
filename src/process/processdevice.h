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

#include <stdint.h>

#include "powerconsumer.h"
#include "../devices/device.h"

class device_consumer : public power_consumer {
public:
	int prio = 0;
	double power = 0.0;
	class device *device = nullptr;
	device_consumer(class device *dev);

	virtual std::string description(void) override;
	virtual std::string name(void) override { return "device"; };
	virtual std::string type(void) override { return "Device"; };
	virtual double Witts(void) override;
	virtual double usage(void) override { return device->utilization();};
	virtual std::string usage_units(void) override {return device->util_units();};
	virtual int show_events(void) override { return 0; };
};

extern void all_devices_to_all_power(void);
extern std::vector<class device_consumer *> all_proc_devices;

extern void clear_proc_devices(void);

