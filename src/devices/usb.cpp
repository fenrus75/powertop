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

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <limits.h>

#include "../lib.h"
#include "../devlist.h"
#include "../parameters/parameters.h"

#include <iostream>
#include <fstream>

usbdevice::usbdevice(const char *_name, const char *path, const char *devid): device()
{
	ifstream file;
	char filename[PATH_MAX];
	char vendor[4096];
	char product[4096];

	pt_strcpy(sysfs_path, path);
	register_sysfs_path(sysfs_path);
	pt_strcpy(name, _name);
	pt_strcpy(devname, devid);
	snprintf(humanname, sizeof(humanname), _("USB device: %s"), pretty_print(devid, vendor, 4096));
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
	snprintf(filename, sizeof(filename), "%s/bDeviceClass", path);
	file.open(filename, ios::in);
	if (file) {
		int dclass = 0;

		file >> dclass;
		file.close();
		if (dclass == 9)
			rootport = 1;
	};

	vendor[0] = 0;
	product[0] = 0;
	snprintf(filename, sizeof(filename), "%s/manufacturer", path);
	file.open(filename, ios::in);
	if (file) {
		file.getline(vendor, 2047);
		if (strstr(vendor, "Linux "))
			vendor[0] = 0;
		file.close();
	};
	snprintf(filename, sizeof(filename), "%s/product", path);
	file.open(filename, ios::in);
	if (file) {
		file.getline(product, 2040);
		file.close();
	};
	if (strlen(vendor) && strlen(product))
		snprintf(humanname, sizeof(humanname), _("USB device: %s (%s)"), product, vendor);
	else if (strlen(product))
		snprintf(humanname, sizeof(humanname), _("USB device: %s"), product);
	else if (strlen(vendor))
		snprintf(humanname, sizeof(humanname), _("USB device: %s"), vendor);

	/* For usbdevfs we need bus number and device number */
	snprintf(filename, sizeof(filename), "%s/busnum", path);
	file.open(filename, ios::in);
	if (file) {

		file >> busnum;
		file.close();
	};
	snprintf(filename, sizeof(filename), "%s/devnum", path);
	file.open(filename, ios::in);
	if (file) {

		file >> devnum;
		file.close();
	};
}



void usbdevice::start_measurement(void)
{
	ifstream file;
	char fullpath[PATH_MAX];

	active_before = 0;
	active_after = 0;
	connected_before = 0;
	connected_after = 0;

	snprintf(fullpath, sizeof(fullpath), "%s/power/active_duration", sysfs_path);
	file.open(fullpath, ios::in);
	if (file) {
		file >> active_before;
	}
	file.close();

	snprintf(fullpath, sizeof(fullpath), "%s/power/connected_duration", sysfs_path);
	file.open(fullpath, ios::in);
	if (file) {
		file >> connected_before;
	}
	file.close();
}

void usbdevice::end_measurement(void)
{
	ifstream file;
	char fullpath[PATH_MAX];

	snprintf(fullpath, sizeof(fullpath), "%s/power/active_duration", sysfs_path);
	file.open(fullpath, ios::in);
	if (file) {
		file >> active_after;
	}
	file.close();

	snprintf(fullpath, sizeof(fullpath), "%s/power/connected_duration", sysfs_path);
	file.open(fullpath, ios::in);
	if (file) {
		file >> connected_after;
	}
	file.close();
	report_utilization(name, utilization());

}

double usbdevice::utilization(void) /* percentage */
{
	double d;
	d = 100.0 * (active_after - active_before) / (0.01 + connected_after - connected_before);
	if (d < 0.0)
		d = 0.0;
	if (d > 99.8)
		d = 100.0;
	return d;
}

const char * usbdevice::device_name(void)
{
	return name;
}

const char * usbdevice::human_name(void)
{
	return humanname;
}

void usbdevice::register_power_with_devlist(struct result_bundle *results, struct parameter_bundle *bundle)
{
	char devfs_name[1024];

	snprintf(devfs_name, sizeof(devfs_name), "usb/%03d/%03d", busnum,
		 devnum);

        register_devpower(devfs_name, power_usage(results, bundle), this);
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

static void create_all_usb_devices_callback(const char *d_name)
{
	char filename[PATH_MAX];
	ifstream file;
	class usbdevice *usb;
	char device_name[PATH_MAX];
	char vendorid[64], devid[64];
	char devid_name[4096];

	snprintf(filename, sizeof(filename), "/sys/bus/usb/devices/%s", d_name);
	snprintf(device_name, sizeof(device_name), "%s/power/active_duration", filename);
	if (access(device_name, R_OK) != 0)
		return;

	snprintf(device_name, sizeof(device_name), "%s/idVendor", filename);
	file.open(device_name, ios::in);
	if (file)
		file.getline(vendorid, 64);
	file.close();
	snprintf(device_name, sizeof(device_name), "%s/idProduct", filename);
	file.open(device_name, ios::in);
	if (file)
		file.getline(devid, 64);
	file.close();

	snprintf(devid_name, sizeof(devid_name), "usb-device-%s-%s", vendorid, devid);
	snprintf(device_name, sizeof(device_name), "usb-device-%s-%s-%s", d_name, vendorid, devid);
	if (result_device_exists(device_name))
		return;

	usb = new class usbdevice(device_name, filename, devid_name);
	all_devices.push_back(usb);
	register_parameter(devid_name, 0.1);
}

void create_all_usb_devices(void)
{
	process_directory("/sys/bus/usb/devices/", create_all_usb_devices_callback);
}
