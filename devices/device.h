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
#ifndef _INCLUDE_GUARD_DEVICE_H
#define _INCLUDE_GUARD_DEVICE_H


#include <vector>
#include <limits.h>

struct parameter_bundle;
struct result_bundle;

#ifdef DISABLE_TRYCATCH
#define try		if(1)
#endif

class device {
public:
	int cached_valid;
	bool hide;

	char guilty[4096];
	char real_path[PATH_MAX+1];

	virtual void start_measurement(void);
	virtual void end_measurement(void);

	device(void);

	virtual ~device() {};

	void register_sysfs_path(const char *path);

	virtual double	utilization(void); /* percentage */

	virtual const char * util_units(void) { return "%"; };

	virtual const char * class_name(void) { return "abstract device";};
	virtual const char * device_name(void) { return "abstract device";};

	virtual const char * human_name(void) { return device_name(); };

	virtual double power_usage(struct result_bundle *results, struct parameter_bundle *bundle) { return 0.0; };

	virtual bool show_in_list(void) {return !hide;};

	virtual int power_valid(void) { return 1;};

	virtual void register_power_with_devlist(struct result_bundle *results, struct parameter_bundle *bundle) { ; };

	virtual int grouping_prio(void) { return 0; }; /* priority of this device class if multiple classes match to the same underlying device. 0 is lowest */
};

using namespace std;

extern vector<class device *> all_devices;

extern void devices_start_measurement(void);
extern void devices_end_measurement(void);
extern void show_report_devices(void);
extern void report_devices(void);


extern void create_all_devices(void);
extern void clear_all_devices(void);

#endif
