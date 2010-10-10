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
#include "device.h"
#include "../parameters/parameters.h"
#include <stdio.h>

device_consumer::device_consumer(class device *dev)
{
	device = dev;
	power = device->power_usage(&all_results, &all_parameters);
}


const char * device_consumer::description(void)
{
	sprintf(str, "%s", device->human_name());
	return str;
}

double device_consumer::Witts(void)
{
	return power;
}

static void add_device(class device *device)
{
	class device_consumer *dev;

	if (device->show_in_list() == 0)
		return;

	dev = new class device_consumer(device);
	all_power.push_back(dev);
}

void all_devices_to_all_power(void)
{
	unsigned int i;
	for (i = 0; i < all_devices.size(); i++)
		add_device(all_devices[i]);
}
