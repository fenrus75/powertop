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

#include <limits.h>
#include <string>

#include "device.h"
#include "../parameters/parameters.h"

class usbdevice: public device {
	int active_before = 0, active_after = 0;
	int connected_before = 0, connected_after = 0;
	std::string sysfs_path;
	std::string name;
	std::string devname;
	std::string humanname;
	int index = 0;
	int r_index = 0;
	int rootport = 0;
	int busnum = 0;
	int devnum = 0;
public:

	usbdevice(const std::string &_name, const std::string &path, const std::string &devid);

	virtual void start_measurement(void);
	virtual void end_measurement(void);

	virtual double	utilization(void); /* percentage */

	virtual std::string class_name(void) { return "usb";};

	virtual std::string device_name(void) { return devname; };
	virtual std::string human_name(void) { return humanname; };
	virtual void register_power_with_devlist(struct result_bundle *results, struct parameter_bundle *bundle);
	virtual double power_usage(struct result_bundle *result, struct parameter_bundle *bundle);
	virtual bool power_valid(void) { return utilization_power_valid(r_index);};
	virtual int grouping_prio(void) { return 4; };
};

extern void create_all_usb_devices(void);

