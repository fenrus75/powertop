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
#ifndef _INCLUDE_GUARD_USB_H
#define _INCLUDE_GUARD_USB_H

#include <limits.h>

#include "device.h"
#include "../parameters/parameters.h"

class usbdevice: public device {
	int active_before, active_after;
	int connected_before, connected_after;
	char sysfs_path[PATH_MAX];
	char name[4096];
	char devname[4096];
	char humanname[4096];
	int index;
	int r_index;
	int rootport;
	int busnum;
	int devnum;
public:

	usbdevice(const char *_name, const char *path, const char *devid);

	virtual void start_measurement(void);
	virtual void end_measurement(void);

	virtual double	utilization(void); /* percentage */

	virtual const char * class_name(void) { return "usb";};

	virtual const char * device_name(void);
	virtual const char * human_name(void);
	virtual void register_power_with_devlist(struct result_bundle *results, struct parameter_bundle *bundle);
	virtual double power_usage(struct result_bundle *result, struct parameter_bundle *bundle);
	virtual int power_valid(void) { return utilization_power_valid(r_index);};
	virtual int grouping_prio(void) { return 4; };
};

extern void create_all_usb_devices(void);


#endif