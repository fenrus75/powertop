/*
 * Copyright 2012, Linaro
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
 *	Rajagopal Venkat <rajagopal.venkat@linaro.org>
 */
#pragma once

#include <memory>
#include <string>
#include <sys/time.h>

#include "device.h"
#include "../parameters/parameters.h"

class frequency;

class devfreq: public device {
	std::string dir_name;
	struct timeval  stamp_before, stamp_after;
	double sample_time = 0.0;

	uint64_t parse_freq_time(const std::string &ptr) const;
	void add_devfreq_freq_state(uint64_t freq, uint64_t time);
	void update_devfreq_freq_state(uint64_t freq, uint64_t time);
	void parse_devfreq_trans_stat(const std::string &dname);
	void process_time_stamps();

public:

	std::vector<std::unique_ptr<frequency>> dstates;

	devfreq(const std::string &c);
	~devfreq() override;
	std::string fill_freq_utilization(unsigned int idx) const;
	std::string fill_freq_name(unsigned int idx) const;

	void start_measurement(void) override;
	void end_measurement(void) override;

	double	utilization(void) const override; /* percentage */

	std::string class_name(void) const override { return "devfreq";};

	std::string device_name(void) const override { return dir_name; };
	std::string human_name(void) override { return "devfreq";};
	double power_usage(struct result_bundle *result, struct parameter_bundle *bundle) override;
	std::string util_units(void) const override { return " rpm"; };
	bool power_valid(void) const override { return false; /*utilization_power_valid(r_index);*/};
	int grouping_prio(void) const override { return 1; };
	void collect_json_fields(std::string &_js) override;
};

extern void create_all_devfreq_devices(void);
extern void clear_all_devfreq(void);
extern void display_devfreq_devices(void);
extern void report_devfreq_devices(void);
extern void initialize_devfreq(void);
extern void start_devfreq_measurement(void);
extern void end_devfreq_measurement(void);

