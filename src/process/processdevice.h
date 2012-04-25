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
#ifndef _INCLUDE_GUARD_DEVICE2_H
#define _INCLUDE_GUARD_DEVICE2_H

#include <stdint.h>

#include "powerconsumer.h"
#include "../devices/device.h"

class device_consumer : public power_consumer {
	char str[4096];
public:
	int prio;
	double power;
	class device *device;
	device_consumer(class device *dev);

	virtual const char * description(void);
	virtual const char * name(void) { return "device"; };
	virtual const char * type(void) { return "Device"; };
	virtual double Witts(void);
	virtual double usage(void) { return device->utilization();};
	virtual const char * usage_units(void) {return device->util_units();};
	virtual int show_events(void) { return 0; };
};

extern void all_devices_to_all_power(void);
extern vector<class device_consumer *> all_proc_devices;

extern void clear_proc_devices(void);

#endif
