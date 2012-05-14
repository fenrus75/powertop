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


static string disk_name(char *path, char *target, char *shortname)
{

	DIR *dir;
	struct dirent *dirent;
	char pathname[PATH_MAX];
	string diskname = "";

	sprintf(pathname, "%s/%s", path, target);
	dir = opendir(pathname);
	if (!dir)
		return diskname;

	while ((dirent = readdir(dir))) {
		char line[4096], *c;
		FILE *file;
		if (dirent->d_name[0]=='.')
			continue;

		if (!strchr(dirent->d_name, ':'))
			continue;

		sprintf(line, "%s/%s/model", pathname, dirent->d_name);
		file = fopen(line, "r");
		if (file) {
			if (fgets(line, 4096, file) == NULL)
				break;
			fclose(file);
			c = strchr(line, '\n');
			if (c)
				*c = 0;
			diskname = line;
			break;
		}
	}
	closedir(dir);

	return diskname;
}

static string model_name(char *path, char *shortname)
{

	DIR *dir;
	struct dirent *dirent;
	char pathname[PATH_MAX];

	sprintf(pathname, "%s/device", path);

	dir = opendir(pathname);
	if (!dir)
		return strdup(shortname);

	while ((dirent = readdir(dir))) {
		if (dirent->d_name[0]=='.')
			continue;

		if (!strchr(dirent->d_name, ':'))
			continue;
		if (!strstr(dirent->d_name, "target"))
			continue;
		return disk_name(pathname, dirent->d_name, shortname);
	}
	closedir(dir);

	return "";
}

ahci::ahci(char *_name, char *path): device()
{
	char buffer[4096];
	char devname[128];
	string diskname;

	end_active = 0;
	end_slumber = 0;
	end_partial = 0;
	start_active = 0;
	start_slumber = 0;
	start_partial = 0;
	strncpy(sysfs_path, path, sizeof(sysfs_path));

	register_sysfs_path(sysfs_path);

	sprintf(devname, "ahci:%s", _name);
	strncpy(name, devname, sizeof(name));
	active_index = get_param_index("ahci-link-power-active");
	partial_index = get_param_index("ahci-link-power-partial");

	sprintf(buffer, "%s-active", name);
	active_rindex = get_result_index(buffer);

	sprintf(buffer, "%s-partial", name);
	partial_rindex = get_result_index(buffer);

	diskname = model_name(path, _name);

	if (strlen(diskname.c_str()) == 0)
		sprintf(humanname, _("SATA link: %s"), _name);
	else
		sprintf(humanname, _("SATA disk: %s"), diskname.c_str());

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
	}
#ifndef DISABLE_TRYCATCH
	catch (std::ios_base::failure &c) {
		fprintf(stderr, "%s\n", c.what());
	}
#endif

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
	}
#ifndef DISABLE_TRYCATCH
	catch (std::ios_base::failure &c) {
		fprintf(stderr, "%s\n", c.what());
	}
#endif
	if (end_active < start_active)
		end_active = start_active;

	p = (end_active - start_active) / (0.001 + end_active + end_partial + end_slumber - start_active - start_partial - start_slumber) * 100.0;
	if (p < 0)
		 p = 0;
	sprintf(powername, "%s-active", name);
	report_utilization(powername, p);

	if (end_partial < start_partial)
		end_partial = start_partial;

	p = (end_partial - start_partial) / (0.001 + end_active + end_partial + end_slumber - start_active - start_partial - start_slumber) * 100.0;
	if (p < 0)
		 p = 0;
	sprintf(powername, "%s-partial", name);
	report_utilization(powername, p);
}


double ahci::utilization(void)
{
	double p;

	p = (end_partial - start_partial + end_active - start_active) / (0.001 + end_active + end_partial + end_slumber - start_active - start_partial - start_slumber) * 100.0;

	if (p < 0)
		p = 0;

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
		register_parameter("ahci-link-power-active", 0.6);  /* active sata link takes about 0.6 W */
		register_parameter("ahci-link-power-partial");
	}
	closedir(dir);

}



double ahci::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	double power;
	double factor;
	double util;

	power = 0;

	factor = get_parameter_value(active_index, bundle);
	util = get_result_value(active_rindex, result);
	power += util * factor / 100.0;


	factor = get_parameter_value(partial_index, bundle);
	util = get_result_value(partial_rindex, result);
	power += util * factor / 100.0;

	return power;
}
