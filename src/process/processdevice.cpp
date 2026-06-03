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
#include <cstdio>
#include <memory>

std::vector<std::unique_ptr<class device_consumer>> all_proc_devices;


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

double device_consumer::Witts(void) const
{
	return power;
}

static void add_device(class device *dev)
{
	/* first check if we want to be shown at all */

	if (dev->show_in_list() == 0)
		return;

	/* then check if a device with the same underlying object is already registered */
	for (auto &cdev_ptr : all_proc_devices) {
		class device_consumer *cdev = cdev_ptr.get();
		if (!dev->real_path.empty() && cdev->device->real_path == dev->real_path) {
			/* we have a device with the same underlying object */

			/* aggregate the power */
			cdev->power += dev->power_usage(&all_results, &all_parameters);

			if (cdev->prio < dev->grouping_prio()) {
				cdev->device = dev;
				cdev->prio = dev->grouping_prio();
			}

			return;
		}
	}

	auto consumer = std::make_unique<class device_consumer>(dev);
	all_power.push_back(consumer.get());
	all_proc_devices.push_back(std::move(consumer));
}

void all_devices_to_all_power(void)
{
	for (auto *dev : all_devices)
		add_device(dev);
}

void clear_proc_devices(void)
{
	all_proc_devices.clear();
}

void device_consumer::collect_json_fields(std::string &_js) const
{
    power_consumer::collect_json_fields(_js);
    JSON_FIELD(prio);
    JSON_FIELD(power);
    JSON_KV("device_name", device ? device->device_name() : std::string(""));
}