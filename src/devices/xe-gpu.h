/*
 * Copyright 2025, Intel Corporation
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

#include <string>
#include <sys/time.h>
#include <vector>
#include "device.h"

/*
 * One power domain exposed by the xe hwmon driver.
 * `current_watts` is -1.0 when no energy counter is available (TDP cap only).
 * `tdp_cap_watts` is -1.0 when no power*_crit file exists for this channel.
 */
struct xe_power_channel {
	std::string label;
	double      current_watts = -1.0;
	double      tdp_cap_watts = -1.0;
};

class xegpu: public device {
	int index = 0;
	int rindex = 0;

	/* Per-channel tracking state (parallel to power_channels). */
	struct channel_track {
		std::string    energy_path;
		double         last_energy = 0.0;
		struct timeval last_time   = {};
	};
	std::vector<channel_track> channel_tracks;

	double consumed_power = 0.0; /* package channel, for power_usage() */

public:
	std::vector<xe_power_channel> power_channels;

	xegpu();

	void start_measurement(void) override;
	void end_measurement(void) override;

	double utilization(void) const override;

	std::string class_name(void) const override { return "GPU"; }
	std::string device_name(void) const override { return "GPU"; }
	std::string human_name(void) override { return _("Intel Xe GPU"); }
	virtual double power_usage(struct result_bundle *result,
				   struct parameter_bundle *bundle) override;
	bool show_in_list(void) const override { return false; }
	std::string util_units(void) const override { return _(" ops/s"); }

	void collect_json_fields(std::string &_js) override;
};

extern void create_xe_gpu(void);

