#include <iostream>
#include <fstream>

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>


using namespace std;

#include "device.h"
#include "alsa.h"
#include "../parameters/parameters.h"

#include <string.h>


alsa::alsa(char *_name, char *path)
{
	char devname[128];
	end_active = 0;
	start_active = 0;
	end_inactive = 0;
	start_inactive = 0;
	strncpy(sysfs_path, path, sizeof(sysfs_path));
	sprintf(devname, "alsa:%s", _name);
	strncpy(name, devname, sizeof(name));
	rindex = get_result_index(name);
}

void alsa::start_measurement(void)
{
	char filename[4096];
	ifstream file;

	sprintf(filename, "%s/power_off_acct", sysfs_path);
	try {
		file.open(filename, ios::in);
		if (file) {
			file >> start_inactive;
		}
		file.close();
		sprintf(filename, "%s/power_on_acct", sysfs_path);
		file.open(filename, ios::in);
		
		if (file) {
			file >> start_active;
		}
		file.close();
	} catch (std::ios_base::failure c) {
		cout << "Fail\n";
	}

}

void alsa::end_measurement(void)
{
	char filename[4096];
	ifstream file;
	double p;

	sprintf(filename, "%s/power_off_acct", sysfs_path);
	try {
		file.open(filename, ios::in);
		if (file) {
			file >> end_inactive;
		}
		file.close();
		sprintf(filename, "%s/power_on_acct", sysfs_path);
		file.open(filename, ios::in);
		
		if (file) {
			file >> end_active;
		}
		file.close();
	} catch (std::ios_base::failure c) {
	}


	p = (end_active - start_active) / (0.001 + end_active + end_inactive - start_active - start_inactive) * 100.0;
	report_utilization(name, p);
}


double alsa::utilization(void)
{
	double p;

	p = (end_active - start_active) / (0.001 + end_active - start_active + end_inactive - start_inactive) * 100.0;

	return p;
}

const char * alsa::device_name(void)
{
	return name;
}

void create_all_alsa(void)
{
	struct dirent *entry;
	DIR *dir;
	char filename[4096];
	
	dir = opendir("/sys/class/sound/card0/");
	if (!dir)
		return;
	while (1) {
		class alsa *bl;
		ofstream file;
		entry = readdir(dir);
		if (!entry)
			break;
		if (entry->d_name[0] == '.')
			continue;
		sprintf(filename, "/sys/class/sound/card0/%s/power_on_acct", entry->d_name);

		if (access(filename, R_OK) != 0)
			continue;

		sprintf(filename, "/sys/class/sound/card0/%s", entry->d_name);

		bl = new class alsa(entry->d_name, filename);
		all_devices.push_back(bl);
		register_parameter("alsa-codec-power");
	}
	closedir(dir);

}



double alsa::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	double power;
	double factor;
	double utilization;
	static int index = 0;

	power = 0;
	if (!index)
		index = get_param_index("alsa-codec-power");
	factor = get_parameter_value(index, bundle);

	utilization = get_result_value(rindex, result);

	power += utilization * factor / 100.0;

	return power;
}