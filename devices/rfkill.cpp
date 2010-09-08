#include <iostream>
#include <fstream>

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>


using namespace std;

#include "device.h"
#include "rfkill.h"
#include "../parameters/parameters.h"

#include <string.h>


rfkill::rfkill(char *_name, char *path)
{
	char devname[128];
	start_soft = 0;
	start_hard = 0;
	end_soft = 0;
	end_hard = 0;
	strncpy(sysfs_path, path, sizeof(sysfs_path));
	sprintf(devname, "radio:%s", _name);
	strncpy(name, devname, sizeof(name));
	register_parameter(devname);
}

void rfkill::start_measurement(void)
{
	char filename[4096];
	ifstream file;

	start_hard = 1;
	start_soft = 1;
	end_hard = 1;
	end_soft = 1;

	sprintf(filename, "%s/hard", sysfs_path);
	file.open(filename, ios::in);
	if (file) {
		file >> start_hard;
	}
	file.close();

	sprintf(filename, "%s/soft", sysfs_path);
	file.open(filename, ios::in);
	if (file) {
		file >> start_soft;
	}
	file.close();
}

void rfkill::end_measurement(void)
{
	char filename[4096];
	ifstream file;

	sprintf(filename, "%s/hard", sysfs_path);
	file.open(filename, ios::in);
	if (file) {
		file >> end_hard;
	}
	file.close();
	sprintf(filename, "%s/soft", sysfs_path);
	file.open(filename, ios::in);
	if (file) {
		file >> end_soft;
	}
	file.close();

	report_utilization(name, utilization());
}


double rfkill::utilization(void)
{
	double p;
	int rfk;

	rfk = start_soft+end_soft;
	if (rfk <  start_hard+end_hard)
		rfk = start_hard+end_hard;

	p = 100 - 50.0 * rfk;

	return p;
}

const char * rfkill::device_name(void)
{
	return name;
}

void create_all_rfkills(void)
{
	struct dirent *entry;
	DIR *dir;
	char filename[4096];
	char name[4096];
	
	dir = opendir("/sys/class/rfkill/");
	if (!dir)
		return;
	while (1) {
		class rfkill *bl;
		ifstream file;
		entry = readdir(dir);
		if (!entry)
			break;
		if (entry->d_name[0] == '.')
			continue;
		sprintf(filename, "/sys/class/rfkill/%s/name", entry->d_name);
		strcpy(name, entry->d_name);
		file.open(filename, ios::in);
		if (file) {
			file.getline(name, 100);		
			file.close();
		}

		sprintf(filename, "/sys/class/rfkill/%s", entry->d_name);
		bl = new class rfkill(name, filename);
		all_devices.push_back(bl);
	}
	closedir(dir);

}



double rfkill::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
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