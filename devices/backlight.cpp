#include <iostream>
#include <fstream>

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>


using namespace std;

#include "device.h"
#include "backlight.h"
#include "../parameters/parameters.h"

#include <string.h>


backlight::backlight(char *_name, char *path)
{
	char devname[128];
	min_level = 0;
	max_level = 0;
	start_level = 0;
	strncpy(sysfs_path, path, sizeof(sysfs_path));
	sprintf(devname, "backlight:%s", _name);
	strncpy(name, devname, sizeof(name));
}

void backlight::start_measurement(void)
{
	char filename[4096];
	ifstream file;

	sprintf(filename, "%s/max_brightness", sysfs_path);
	file.open(filename, ios::in);
	if (file) {
		file >> max_level;
	}
	file.close();

	sprintf(filename, "%s/actual_brightness", sysfs_path);
	file.open(filename, ios::in);
	if (file) {
		file >> start_level;		
		file.close();
	}
}

static int dpms_screen_on(void)
{
	DIR *dir;
	struct dirent *entry;
	char filename[4096];
	char line[4096];
	ifstream file;

	dir = opendir("/sys/class/drm/card0");
	if (!dir)
		return 1;
	while (1) {
		entry = readdir(dir);
		if (!entry)
			break;

		sprintf(filename, "/sys/class/drm/card0/%s/enabled", entry->d_name);
		file.open(filename, ios::in);
		if (!file)
			continue;
		file.getline(line, 4096);
		file.close();
		if (strcmp(line, "enabled") != 0)
			continue;
		sprintf(filename, "/sys/class/drm/card0/%s/dpms", entry->d_name);
		file.open(filename, ios::in);
		if (!file)
			continue;
		file.getline(line, 4096);
		file.close();
		if (strcmp(line, "On") == 0) {
			closedir(dir);
			return 1;
		}
	}
	closedir(dir);
	return 0;
}

void backlight::end_measurement(void)
{
	char filename[4096];
	char powername[4096];
	ifstream file;
	double p;
	int backlight = 0;

	sprintf(filename, "%s/actual_brightness", sysfs_path);
	file.open(filename, ios::in);
	if (file) {
		file >> end_level;
	}
	file.close();

	if (dpms_screen_on()) {
		p = 100.0 * (end_level + start_level) / 2 / max_level;
		backlight = 100;
	} else {
		p = 0;
	}

	report_utilization(name, p);
	sprintf(powername, "%s-power", name);
	report_utilization(powername, backlight);
}


double backlight::utilization(void)
{
	double p;

	p = 100.0 * (end_level + start_level) / 2 / max_level;
	return p;
}

const char * backlight::device_name(void)
{
	return name;
}

void create_all_backlights(void)
{
	struct dirent *entry;
	DIR *dir;
	char filename[4096];
	
	dir = opendir("/sys/class/backlight/");
	if (!dir)
		return;
	while (1) {
		class backlight *bl;
		entry = readdir(dir);
		if (!entry)
			break;
		if (entry->d_name[0] == '.')
			continue;
		sprintf(filename, "/sys/class/backlight/%s", entry->d_name);
		bl = new class backlight(entry->d_name, filename);
		all_devices.push_back(bl);
		register_parameter("backlight", 2.8);
		register_parameter("backlight-power", 2.8);
	}
	closedir(dir);

}



double backlight::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	double power;
	double factor;
	double utilization;

	power = 0;
	factor = get_parameter_value("backlight", bundle);
	utilization = get_result_value(name, result);

	power += utilization * factor / 100.0;

	factor = get_parameter_value("backlight-power", bundle);
	utilization = get_result_value(name, result);

	power += utilization * factor / 100.0;

	return power;
}