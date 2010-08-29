#include "usb.h"

#include <string.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#include "../parameters/parameters.h"

#include <iostream>
#include <fstream>

usbdevice::usbdevice(const char *_name, const char *path, const char *devid)
{
	strcpy(sysfs_path, path);
	strcpy(name, _name);
	strcpy(devname, devid);
	active_before = 0;
	active_after = 0;
	connected_before = 0;
	connected_after = 0;
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
	return d;
}

const char * usbdevice::device_name(void)
{
	return name;
}


double usbdevice::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	double power;
	double factor;
	double utilization;

	/* root ports should count as 0 .. their activity is derived */
	if (strcmp(devname, "usb-device-1d6b-0001") == 0)
		return 0.0;
	if (strcmp(devname, "usb-device-1d6b-0002") == 0)
		return 0.0;

	power = 0;
	factor = get_parameter_value(devname, bundle);
	utilization = get_result_value(name, result);

	power += utilization * factor / 100.0;

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

		sprintf(device_name, "usb-device-%s", entry->d_name);
		usb = new class usbdevice(device_name, filename, devid_name);
		all_devices.push_back(usb);
		register_parameter(devid_name, 1.0);
	}
	closedir(dir);
}

