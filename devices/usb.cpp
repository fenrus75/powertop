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
#include <dirent.h>

#include "../lib.h"
#include "../parameters/parameters.h"

#include <iostream>
#include <fstream>

usbdevice::usbdevice(const char *_name, const char *path, const char *devid): device()
{
	ifstream file;
	char filename[4096];
	char vendor[4096];
	char product[4096];

	strcpy(sysfs_path, path);
	register_sysfs_path(sysfs_path);
	strcpy(name, _name);
	strcpy(devname, devid);
	sprintf(humanname, _("USB device: %s"), pretty_print(devid, vendor, 4096));
	active_before = 0;
	active_after = 0;
	connected_before = 0;
	connected_after = 0;

	index = get_param_index(devname);
	r_index = get_result_index(name);
	rootport = 0;
	cached_valid = 0;


	/* root ports and hubs should count as 0 power ... their activity is derived */
	sprintf(filename, "%s/bDeviceClass", path);
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
	sprintf(filename, "%s/manufacturer", path);
	file.open(filename, ios::in);
	if (file) {
		file.getline(vendor, 2047);
		if (strstr(vendor, "Linux "))
			vendor[0] = 0;
		file.close();
	};
	sprintf(filename, "%s/product", path);
	file.open(filename, ios::in);
	if (file) {
		file.getline(product, 2040);
		file.close();
	};
	if (strlen(vendor) && strlen(product))
		sprintf(humanname, _("USB device: %s (%s)"), product, vendor);
	else if (strlen(product))
		sprintf(humanname, _("USB device: %s"), product);
	else if (strlen(vendor))
		sprintf(humanname, _("USB device: %s"), vendor);
}



void usbdevice::start_measurement(void)
{
	ifstream file;
	char fullpath[4096];

	active_before = 0;
	active_after = 0;
	connected_before = 0;
	connected_after = 0;

	sprintf(fullpath, "%s/power/active_duration", sysfs_path);
	file.open(fullpath, ios::in);
	if (file) {
		file >> active_before;
	}
	file.close();

	sprintf(fullpath, "%s/power/connected_duration", sysfs_path);
	file.open(fullpath, ios::in);
	if (file) {
		file >> connected_before;
	}
	file.close();
}

void usbdevice::end_measurement(void)
{
	ifstream file;
	char fullpath[4096];

	sprintf(fullpath, "%s/power/active_duration", sysfs_path);
	file.open(fullpath, ios::in);
	if (file) {
		file >> active_after;
	}
	file.close();

	sprintf(fullpath, "%s/power/connected_duration", sysfs_path);
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


void create_all_usb_devices(void)
{
	struct dirent *entry;
	DIR *dir;
	char filename[4096];

	dir = opendir("/sys/bus/usb/devices/");
	if (!dir)
		return;
	while (1) {
		ifstream file;
		class usbdevice *usb;
		char device_name[4096];
		char vendorid[64], devid[64];
		char devid_name[4096];
		entry = readdir(dir);

		if (!entry)
			break;
		if (entry->d_name[0] == '.')
			continue;

		sprintf(filename, "/sys/bus/usb/devices/%s", entry->d_name);

		sprintf(device_name, "%s/power/active_duration", filename);
		if (access(device_name, R_OK)!=0)
			continue;

		sprintf(device_name, "%s/idVendor", filename);
		file.open(device_name, ios::in);
		if (file)
			file.getline(vendorid, 64);
		file.close();
		sprintf(device_name, "%s/idProduct", filename);
		file.open(device_name, ios::in);
		if (file)
			file.getline(devid, 64);
		file.close();

		sprintf(devid_name, "usb-device-%s-%s", vendorid, devid);

		sprintf(device_name, "usb-device-%s-%s-%s", entry->d_name, vendorid, devid);

		if (result_device_exists(device_name))
			continue;

		usb = new class usbdevice(device_name, filename, devid_name);
		all_devices.push_back(usb);

		register_parameter(devid_name, 0.1);
	}
	closedir(dir);
}
