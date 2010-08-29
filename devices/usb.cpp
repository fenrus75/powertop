#include "usb.h"

#include <string.h>

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

#include "../parameters/parameters.h"

#include <iostream>
#include <fstream>

usbdevice::usbdevice(char *_name, char *path)
{
	strcpy(sysfs_path, path);
	strcpy(name, _name);
	active_before = 0;
	active_after = 0;
	connected_before = 0;
	connected_after = 0;
}



void usbdevice::start_measurement(void)
{
	ifstream file;
	char fullpath[4096];
	
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
	return 100.0 * (active_after - active_before) / (0.01 + connected_after - connected_before);
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

	power = 0;
	factor = get_parameter_value(name, bundle);
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
		class usbdevice *usb;
		char device_name[4096];
		entry = readdir(dir);
		if (!entry)
			break;
		if (entry->d_name[0] == '.')
			continue;

		if (entry->d_name[0] == 'u')
			continue;

		sprintf(filename, "/sys/bus/usb/devices/%s", entry->d_name);
		sprintf(device_name, "usb-device-%s", entry->d_name);
		usb = new class usbdevice(device_name, filename);
		all_devices.push_back(usb);
		register_parameter(device_name, 0.0);
	}
	closedir(dir);
}

