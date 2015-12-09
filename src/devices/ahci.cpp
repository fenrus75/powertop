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
#include <limits.h>


using namespace std;

#include "device.h"
#include "report/report.h"
#include "report/report-maker.h"
#include "ahci.h"
#include "../parameters/parameters.h"
#include "report/report-data-html.h"
#include <string.h>

vector <class ahci *> links;

static string disk_name(char *path, char *target, char *shortname)
{

	DIR *dir;
	struct dirent *dirent;
	char pathname[PATH_MAX];
	string diskname = "";

	snprintf(pathname, PATH_MAX, "%s/%s", path, target);
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

		snprintf(line, PATH_MAX, "%s/%s/model", pathname, dirent->d_name);
		file = fopen(line, "r");
		if (file) {
			if (fgets(line, 4096, file) == NULL) {
				fclose(file);
				break;
			}
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

	snprintf(pathname, PATH_MAX, "%s/device", path);

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
	end_devslp = 0;
	end_partial = 0;
	start_active = 0;
	start_slumber = 0;
	start_devslp = 0;
	start_partial = 0;
	strncpy(sysfs_path, path, sizeof(sysfs_path));

	register_sysfs_path(sysfs_path);

	snprintf(devname, 128, "ahci:%s", _name);
	strncpy(name, devname, sizeof(name));
	active_index = get_param_index("ahci-link-power-active");
	partial_index = get_param_index("ahci-link-power-partial");

	snprintf(buffer, 4096, "%s-active", name);
	active_rindex = get_result_index(buffer);

	snprintf(buffer, 4096, "%s-partial", name);
	partial_rindex = get_result_index(buffer);

	snprintf(buffer, 4096, "%s-slumber", name);
	slumber_rindex = get_result_index(buffer);

	snprintf(buffer, 4096, "%s-devslp", name);
	devslp_rindex = get_result_index(buffer);

	diskname = model_name(path, _name);

	if (strlen(diskname.c_str()) == 0)
		snprintf(humanname, 4096, _("SATA link: %s"), _name);
	else
		snprintf(humanname, 4096, _("SATA disk: %s"), diskname.c_str());
}

void ahci::start_measurement(void)
{
	char filename[PATH_MAX];
	ifstream file;

	snprintf(filename, PATH_MAX, "%s/ahci_alpm_active", sysfs_path);
	try {
		file.open(filename, ios::in);
		if (file) {
			file >> start_active;
		}
		file.close();
		snprintf(filename, PATH_MAX, "%s/ahci_alpm_partial", sysfs_path);
		file.open(filename, ios::in);

		if (file) {
			file >> start_partial;
		}
		file.close();
		snprintf(filename, PATH_MAX, "%s/ahci_alpm_slumber", sysfs_path);
		file.open(filename, ios::in);
		if (file) {
			file >> start_slumber;
		}
		file.close();
		snprintf(filename, PATH_MAX, "%s/ahci_alpm_devslp", sysfs_path);
		file.open(filename, ios::in);
		if (file) {
			file >> start_devslp;
		}
		file.close();
	}
	catch (std::ios_base::failure &c) {
		fprintf(stderr, "%s\n", c.what());
	}

}

void ahci::end_measurement(void)
{
	char filename[PATH_MAX];
	char powername[4096];
	ifstream file;
	double p;
	double total;

	try {
		snprintf(filename, 4096, "%s/ahci_alpm_active", sysfs_path);
		file.open(filename, ios::in);
		if (file) {
			file >> end_active;
		}
		file.close();
		snprintf(filename, 4096, "%s/ahci_alpm_partial", sysfs_path);
		file.open(filename, ios::in);
		if (file) {
			file >> end_partial;
		}
		file.close();
		snprintf(filename, 4096, "%s/ahci_alpm_slumber", sysfs_path);
		file.open(filename, ios::in);
		if (file) {
			file >> end_slumber;
		}
		file.close();
		snprintf(filename, 4096, "%s/ahci_alpm_devslp", sysfs_path);
		file.open(filename, ios::in);
		if (file) {
			file >> end_devslp;
		}
		file.close();
	}
	catch (std::ios_base::failure &c) {
		fprintf(stderr, "%s\n", c.what());
	}
	if (end_active < start_active)
		end_active = start_active;
	if (end_partial < start_partial)
		end_partial = start_partial;
	if (end_slumber < start_slumber)
		end_slumber = start_slumber;

	total = 0.001 + end_active + end_partial + end_slumber + end_devslp -
		start_active - start_partial - start_slumber - start_devslp;

	/* percent in active */
	p = (end_active - start_active) / total * 100.0;
	if (p < 0)
		 p = 0;
	snprintf(powername, 4096, "%s-active", name);
	report_utilization(powername, p);

	/* percent in partial */
	p = (end_partial - start_partial) / total * 100.0;
	if (p < 0)
		 p = 0;
	snprintf(powername, 4096, "%s-partial", name);
	report_utilization(powername, p);

	/* percent in slumber */
	p = (end_slumber - start_slumber) / total * 100.0;
	if (p < 0)
		 p = 0;
	snprintf(powername, 4096, "%s-slumber", name);
	report_utilization(powername, p);

	/* percent in devslp */
	p = (end_devslp - start_devslp) / total * 100.0;
	if (p < 0)
		 p = 0;
	snprintf(powername, 4096, "%s-devslp", name);
	report_utilization(powername, p);
}


double ahci::utilization(void)
{
	double p;

	p = (end_partial - start_partial + end_active - start_active) / (0.001 + end_active + end_partial + end_slumber + end_devslp - start_active - start_partial - start_slumber - start_devslp) * 100.0;

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
	char filename[PATH_MAX];

	dir = opendir("/sys/class/scsi_host/");
	if (!dir)
		return;
	while (1) {
		class ahci *bl;
		ofstream file;
		ifstream check_file;
		entry = readdir(dir);
		if (!entry)
			break;
		if (entry->d_name[0] == '.')
			continue;
		snprintf(filename, PATH_MAX, "/sys/class/scsi_host/%s/ahci_alpm_accounting", entry->d_name);

		check_file.open(filename, ios::in);
		check_file.get();
		check_file.close();
		if (check_file.bad())
			continue;

		file.open(filename, ios::in);
		if (!file)
			continue;
		file << 1 ;
		file.close();
		snprintf(filename, PATH_MAX, "/sys/class/scsi_host/%s", entry->d_name);

		bl = new class ahci(entry->d_name, filename);
		all_devices.push_back(bl);
		register_parameter("ahci-link-power-active", 0.6);  /* active sata link takes about 0.6 W */
		register_parameter("ahci-link-power-partial");
		links.push_back(bl);
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

void ahci_create_device_stats_table(void)
{
	unsigned int i;
	int cols=0;
	int rows=0;

	/* div attr css_class and css_id */
	tag_attr div_attr;
	init_div(&div_attr, "clear_block", "ahci");

	/* Set Title attributes */
	tag_attr title_attr;
	init_title_attr(&title_attr);

	/* Add section */
	report.add_div(&div_attr);

	if (links.size() == 0) {
		report.add_title(&title_attr, __("AHCI ALPM Residency Statistics - Not supported on this macine"));
		report.end_div();
		return;
	}

	/* Set Table attributes, rows, and cols */
	table_attributes std_table_css;
	cols=5;
	rows=links.size()+1;
	init_std_side_table_attr(&std_table_css, rows, cols);



	/* Set array of data in row Major order */
	string *ahci_data = new string[cols * rows];
	ahci_data[0]=__("Link");
	ahci_data[1]=__("Active");
	ahci_data[2]=__("Partial");
	ahci_data[3]=__("Slumber");
	ahci_data[4]=__("Devslp");

	/* traverse list of all devices and put their residency in the table */
	for (i = 0; i < links.size(); i++){
		links[i]->report_device_stats(ahci_data, i);
	}
	report.add_title(&title_attr, __("AHCI ALPM Residency Statistics"));
	report.add_table(ahci_data, &std_table_css);
	report.end_div();
	delete [] ahci_data;
}

void ahci::report_device_stats(string *ahci_data, int idx)
{
	int offset=(idx*5+5);
	char util[128];
	double active_util = get_result_value(active_rindex, &all_results);
	double partial_util = get_result_value(partial_rindex, &all_results);
	double slumber_util = get_result_value(slumber_rindex, &all_results);
	double devslp_util = get_result_value(devslp_rindex, &all_results);

	ahci_data[offset]=humanname;
	printf("\nData from ahci %s\n",ahci_data[offset].c_str());
	offset +=1;

	sprintf(util, "%5.1f",  active_util);
	ahci_data[offset]= util;
	offset +=1;

	sprintf(util, "%5.1f",  partial_util);
	ahci_data[offset]= util;
	offset +=1;

	sprintf(util, "%5.1f",  slumber_util);
	ahci_data[offset]= util;
	offset +=1;

	sprintf(util, "%5.1f",  devslp_util);
	ahci_data[offset]= util;
}
