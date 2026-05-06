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
#include "usb.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <limits.h>
#include <format>

#include "../lib.h"
#include "../devlist.h"
#include "../parameters/parameters.h"

#include <iostream>
#include <fstream>

usbdevice::usbdevice(const std::string &_name, const std::string &path, const std::string &devid): device()
{
	std::string vendor;
	std::string product;

	sysfs_path = path;
	register_sysfs_path(sysfs_path);
	name = _name;
	devname = devid;
	humanname = pt_format(_("USB device: {}"), pretty_print(devid));
	active_before = 0;
	active_after = 0;
	connected_before = 0;
	connected_after = 0;
	busnum = 0;
	devnum = 0;

	index = get_param_index(devname);
	r_index = get_result_index(name);
	rootport = 0;
	cached_valid = 0;


	/* root ports and hubs should count as 0 power ... their activity is derived */
	if (read_sysfs(std::format("{}/bDeviceClass", path)) == 9)
		rootport = 1;

	vendor = read_sysfs_string(std::format("{}/manufacturer", path));
	if (vendor.find("Linux ") != std::string::npos)
		vendor = "";

	product = read_sysfs_string(std::format("{}/product", path));

	if (!vendor.empty() && !product.empty())
		humanname = pt_format(_("USB device: {} ({})"), product, vendor);
	else if (!product.empty())
		humanname = pt_format(_("USB device: {}"), product);
	else if (!vendor.empty())
		humanname = pt_format(_("USB device: {}"), vendor);

	/* For usbdevfs we need bus number and device number */
	busnum = read_sysfs(std::format("{}/busnum", path));
	devnum = read_sysfs(std::format("{}/devnum", path));
}



void usbdevice::start_measurement(void)
{
	active_before = read_sysfs(std::format("{}/power/active_duration", sysfs_path));
	connected_before = read_sysfs(std::format("{}/power/connected_duration", sysfs_path));
}

void usbdevice::end_measurement(void)
{
	active_after = read_sysfs(std::format("{}/power/active_duration", sysfs_path));
	connected_after = read_sysfs(std::format("{}/power/connected_duration", sysfs_path));

	report_utilization(name, utilization());
}

double usbdevice::utilization(void) /* percentage */
{
	double d;
	double total_connected = (double)connected_after - connected_before;

	if (total_connected < 0.01)
		return 0.0;

	d = 100.0 * (active_after - active_before) / total_connected;
	if (d < 0.0)
		d = 0.0;
	if (d > 99.8)
		d = 100.0;
	return d;
}

void usbdevice::register_power_with_devlist(struct result_bundle *results, struct parameter_bundle *bundle)
{
	std::string devfs_name = std::format("usb/{:03}/{:03}", busnum, devnum);
	register_devpower(devfs_name.c_str(), power_usage(results, bundle), this);
}

double usbdevice::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	double power;
	double factor;
	double util;

	if (rootport || !cached_valid)
		return 0.0;


	power = 0;
	factor = get_parameter_value(index, bundle);
	util = get_result_value(r_index, result);

	power += util * factor / 100.0;

	return power;
}

static void create_all_usb_devices_callback(const std::string &d_name)
{
	class usbdevice *usb;
	std::string vendorid, devid;

	if (access(std::format("/sys/bus/usb/devices/{}/power/active_duration", d_name).c_str(), R_OK) != 0)
		return;

	vendorid = read_sysfs_string(std::format("/sys/bus/usb/devices/{}/idVendor", d_name));
	devid = read_sysfs_string(std::format("/sys/bus/usb/devices/{}/idProduct", d_name));

	std::string devid_name = std::format("usb-device-{}-{}", vendorid, devid);
	std::string device_name = std::format("usb-device-{}-{}-{}", d_name, vendorid, devid);
	if (result_device_exists(device_name))
		return;

	usb = new usbdevice(device_name, std::format("/sys/bus/usb/devices/{}", d_name), devid_name);
	all_devices.push_back(usb);
	register_parameter(devid_name, 0.1);
}

void create_all_usb_devices(void)
{
	process_directory("/sys/bus/usb/devices/", create_all_usb_devices_callback);
}

void usbdevice::collect_json_fields(std::string &_js)
{
    device::collect_json_fields(_js);
    JSON_FIELD(active_before);
    JSON_FIELD(active_after);
    JSON_FIELD(connected_before);
    JSON_FIELD(connected_after);
    JSON_FIELD(sysfs_path);
    JSON_FIELD(name);
    JSON_FIELD(devname);
    JSON_FIELD(humanname);
    JSON_FIELD(index);
    JSON_FIELD(r_index);
    JSON_FIELD(rootport);
    JSON_FIELD(busnum);
    JSON_FIELD(devnum);
}
