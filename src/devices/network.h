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

#include <sys/time.h>
#include <limits.h>
#include <string>

#include "device.h"
#include "../parameters/parameters.h"

class network: public device {
	int start_up = 0, end_up = 0;
	uint64_t start_pkts = 0, end_pkts = 0;
	struct timeval before, after;

	int start_speed = 0; /* 0 is "no link" */
	int end_speed = 0; /* 0 is "no link" */

	std::string sysfs_path;
	std::string name;
	std::string humanname;
	int index_up = 0;
	int rindex_up = 0;
	int index_link_100 = 0;
	int rindex_link_100 = 0;
	int index_link_1000 = 0;
	int rindex_link_1000 = 0;
	int index_link_high = 0;
	int rindex_link_high = 0;
	int index_pkts = 0;
	int rindex_pkts = 0;
	int index_powerunsave = 0;
	int rindex_powerunsave = 0;

	int valid_100 = 0;
	int valid_1000 = 0;
	int valid_high = 0;
	int valid_powerunsave = 0;
public:
	uint64_t pkts = 0;
	double duration = 0.0;

	network(const std::string &_name, const std::string &path);

	virtual void start_measurement(void) override;
	virtual void end_measurement(void) override;

	virtual double	utilization(void) override;
	virtual std::string util_units(void) override { return " pkts/s"; };

	virtual std::string class_name(void) override { return "ethernet";};

	virtual std::string device_name(void) override { return name; };
	virtual std::string human_name(void) override { return humanname; };
	virtual double power_usage(struct result_bundle *result, struct parameter_bundle *bundle) override;
	virtual bool power_valid(void) override { return utilization_power_valid(rindex_up) || utilization_power_valid(rindex_link_100) || utilization_power_valid(rindex_link_1000) || utilization_power_valid(rindex_link_high);};
	virtual int grouping_prio(void) override { return 10; };
};

extern void create_all_nics(callback fn = nullptr);

