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
#ifndef _INCLUDE_GUARD_DEVFREQ_H
#define _INCLUDE_GUARD_DEVFREQ_H

#include "device.h"
#include "../parameters/parameters.h"

struct frequency;

class devfreq: public device {
	char dir_name[128];
	struct timeval  stamp_before, stamp_after;
	double sample_time;

	uint64_t parse_freq_time(char *ptr);
	void add_devfreq_freq_state(uint64_t freq, uint64_t time);
	void update_devfreq_freq_state(uint64_t freq, uint64_t time);
	void parse_devfreq_trans_stat(char *dname);
	void process_time_stamps();

public:

	vector<struct frequency *> dstates;

	devfreq(const char *c);
	void fill_freq_utilization(unsigned int idx, char *buf);
	void fill_freq_name(unsigned int idx, char *buf);

	virtual void start_measurement(void);
	virtual void end_measurement(void);

	virtual double	utilization(void); /* percentage */

	virtual const char * class_name(void) { return "devfreq";};

	virtual const char * device_name(void) { return dir_name;};
	virtual const char * human_name(void) { return "devfreq";};
	virtual double power_usage(struct result_bundle *result, struct parameter_bundle *bundle);
	virtual const char * util_units(void) { return " rpm"; };
	virtual int power_valid(void) { return 0; /*utilization_power_valid(r_index);*/};
	virtual int grouping_prio(void) { return 1; };
};

extern void create_all_devfreq_devices(void);
extern void clear_all_devfreq(void);
extern void display_devfreq_devices(void);
extern void report_devfreq_devices(void);
extern void initialize_devfreq(void);
extern void start_devfreq_measurement(void);
extern void end_devfreq_measurement(void);

#endif
