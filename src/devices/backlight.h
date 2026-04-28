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

class backlight: public device {
	int min_level, max_level;
	int start_level, end_level;
	std::string sysfs_path;
	std::string name;
	int r_index;
	int r_index_power;
public:

	backlight(const std::string &_name, const std::string &path);

	virtual void start_measurement(void);
	virtual void end_measurement(void);

	virtual double	utilization(void); /* percentage */

	virtual std::string class_name(void) { return "backlight";};

	virtual std::string device_name(void) { return name; };
	virtual std::string human_name(void) { return "Display backlight";};
	virtual double power_usage(struct result_bundle *result, struct parameter_bundle *bundle);
	virtual int grouping_prio(void) { return 10; };
};

extern void create_all_backlights(void);

