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
#include "../parameters/parameters.h"

#include <cstdint>
#include <climits>
#include <string>

class alsa: public device {
	uint64_t start_active = 0, end_active = 0;
	uint64_t start_inactive = 0, end_inactive = 0;
	std::string sysfs_path;
	std::string name;
	std::string humanname;
	int rindex = 0;
public:

	alsa(const std::string &_name, const std::string &path);

	void start_measurement(void) override;
	void end_measurement(void) override;

	double	utilization(void) const override; /* percentage */

	std::string class_name(void) const override { return "alsa";};

	std::string device_name(void) const override { return name; };
	std::string human_name(void) override;
	double power_usage(struct result_bundle *result, struct parameter_bundle *bundle) override;
	bool power_valid(void) const override { return utilization_power_valid(rindex);};

	void register_power_with_devlist(struct result_bundle *results, struct parameter_bundle *bundle) override;
	int grouping_prio(void) const override { return 0; };
	void collect_json_fields(std::string &_js) override;

};

extern void create_all_alsa(void);

