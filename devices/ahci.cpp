#include <iostream>
#include <fstream>

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>


using namespace std;

#include "device.h"
#include "ahci.h"
#include "../parameters/parameters.h"

#include <string.h>


ahci::ahci(char *_name, char *path)
{
	char devname[128];
	end_active = 0;
	end_slumber = 0;
	end_partial = 0;
	start_active = 0;
	start_slumber = 0;
	start_partial = 0;
	strncpy(sysfs_path, path, sizeof(sysfs_path));
	sprintf(devname, "ahci:%s", _name);
	strncpy(name, devname, sizeof(name));
}

void ahci::start_measurement(void)
{
	char filename[4096];
	ifstream file;

	sprintf(filename, "%s/ahci_alpm_active", sysfs_path);
	try {
		file.open(filename, ios::in);
		if (file) {
			file >> start_active;
		}
		file.close();
		sprintf(filename, "%s/ahci_alpm_partial", sysfs_path);
		file.open(filename, ios::in);
		
		if (file) {
			file >> start_partial;
		}
		file.close();
		sprintf(filename, "%s/ahci_alpm_slumber", sysfs_path);
		file.open(filename, ios::in);
		if (file) {
				file >> start_slumber;
		}	
		file.close();
	} catch (std::ios_base::failure c) {
	}

}

void ahci::end_measurement(void)
{
	char filename[4096];
	char powername[4096];
	ifstream file;
	double p;

	try {
		sprintf(filename, "%s/ahci_alpm_active", sysfs_path);
		file.open(filename, ios::in);
		if (file) {
			file >> end_active;
		}
		file.close();
		sprintf(filename, "%s/ahci_alpm_partial", sysfs_path);
		file.open(filename, ios::in);
		if (file) {
			file >> end_partial;
		}
		file.close();
		sprintf(filename, "%s/ahci_alpm_slumber", sysfs_path);
		file.open(filename, ios::in);
		if (file) {
			file >> end_slumber;
		}
		file.close();
	} catch (std::ios_base::failure c) {
	}

	p = (end_active - start_active) / (0.001 + end_active + end_partial + end_slumber - start_active - start_partial - start_slumber) * 100.0;
	sprintf(powername, "%s-active", name);
	report_utilization(powername, p);

	p = (end_partial - start_partial) / (0.001 + end_active + end_partial + end_slumber - start_active - start_partial - start_slumber) * 100.0;
	sprintf(powername, "%s-partial", name);
	report_utilization(powername, p);
}


double ahci::utilization(void)
{
	double p;

	p = (end_partial - start_partial + end_active - start_active) / (0.001 + end_active + end_partial + end_slumber - start_active - start_partial - start_slumber) * 100.0;

	return p;
}

const char * ahci::device_name(void)
{
	return name;
}

void create_all_ahcis(void)
{
	struct dirent *entry;
	DIR *dir;
	char filename[4096];
	
	dir = opendir("/sys/class/scsi_host/");
	if (!dir)
		return;
	while (1) {
		class ahci *bl;
		ofstream file;
		entry = readdir(dir);
		if (!entry)
			break;
		if (entry->d_name[0] == '.')
			continue;
		sprintf(filename, "/sys/class/scsi_host/%s/ahci_alpm_accounting", entry->d_name);
		file.open(filename, ios::in);
		if (!file)
			continue;
		file << 1 ;
		file.close();
		sprintf(filename, "/sys/class/scsi_host/%s", entry->d_name);

		bl = new class ahci(entry->d_name, filename);
		all_devices.push_back(bl);
		register_parameter("ahci-link-power-active");
		register_parameter("ahci-link-power-partial");
	}
	closedir(dir);

}



double ahci::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	double power;
	double factor;
	double utilization;
	char buffer[4096];

	power = 0;
	factor = get_parameter_value("ahci-link-power-active", bundle);

	sprintf(buffer, "%s-active", name);
	utilization = get_result_value(buffer, result);

	power += utilization * factor / 100.0;

	sprintf(buffer, "%s-partial", name);
	factor = get_parameter_value("ahci-link-power-partial", bundle);
	utilization = get_result_value(buffer, result);

	power += utilization * factor / 100.0;

	return power;
}