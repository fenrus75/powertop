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
#include "processdevice.h"
#include "../parameters/parameters.h"
#include <stdio.h>

std::vector<class device_consumer *> all_proc_devices;


device_consumer::device_consumer(class device *dev) : power_consumer()
{
	device = dev;
	power = device->power_usage(&all_results, &all_parameters);
	prio = dev->grouping_prio();
}


std::string device_consumer::description(void)
{
	return device->human_name();
}

double device_consumer::Witts(void)
{
	return power;
}

static void add_device(class device *device)
{
	class device_consumer *dev;
	unsigned int i;

	/* first check if we want to be shown at all */

	if (device->show_in_list() == 0)
		return;

	/* then check if a device with the same underlying object is already registered */
	for (i = 0; i < all_proc_devices.size(); i++) {
		class device_consumer *cdev;
		cdev = all_proc_devices[i];
		if (!device->real_path.empty() && cdev->device->real_path == device->real_path) {
			/* we have a device with the same underlying object */

			/* aggregate the power */
			cdev->power += device->power_usage(&all_results, &all_parameters);

			if (cdev->prio < device->grouping_prio()) {
				cdev->device = device;
				cdev->prio = device->grouping_prio();
			}

			return;
		}
	}

	dev = new device_consumer(device);
	all_power.push_back(dev);
	all_proc_devices.push_back(dev);
}

void all_devices_to_all_power(void)
{
	unsigned int i;
	for (i = 0; i < all_devices.size(); i++)
		add_device(all_devices[i]);
}

void clear_proc_devices(void)
{
	std::vector<class device_consumer *>::iterator it = all_proc_devices.begin();
	while (it != all_proc_devices.end()) {
		delete *it;
		it = all_proc_devices.erase(it);
	}
}

void device_consumer::collect_json_fields(std::string &_js)
{
    power_consumer::collect_json_fields(_js);
    JSON_FIELD(prio);
    JSON_FIELD(power);
    JSON_KV("device_name", device ? device->device_name() : std::string(""));
}
