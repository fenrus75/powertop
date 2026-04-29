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
	int min_level = 0, max_level = 0;
	int start_level = 0, end_level = 0;
	std::string sysfs_path;
	std::string name;
	int r_index = 0;
	int r_index_power = 0;
public:

	backlight(const std::string &_name, const std::string &path);

	virtual void start_measurement(void) override;
	virtual void end_measurement(void) override;

	virtual double	utilization(void) override; /* percentage */

	virtual std::string class_name(void) override { return "backlight";};

	virtual std::string device_name(void) override { return name; };
	virtual std::string human_name(void) override { return _("Display backlight");};
	virtual double power_usage(struct result_bundle *result, struct parameter_bundle *bundle) override;
	virtual int grouping_prio(void) override { return 10; };
	void collect_json_fields(std::string &_js) override;
};

extern void create_all_backlights(void);

