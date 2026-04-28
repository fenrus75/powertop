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


#include "device.h"
#include "report/report.h"
#include "report/report-maker.h"
#include "ahci.h"
#include "../parameters/parameters.h"
#include "report/report-data-html.h"
#include <string.h>
#include <format>

std::vector <class ahci *> links;

static std::string disk_name(const std::string &path, const std::string &target, const std::string &shortname __unused)
{

	DIR *dir;
	struct dirent *dirent;
	std::string diskname = "";
	std::string pathname;

	pathname = std::format("{}/{}", path, target);
	dir = opendir(pathname.c_str());
	if (!dir)
		return diskname;

	while ((dirent = readdir(dir))) {
		if (dirent->d_name[0]=='.')
			continue;

		if (std::string(dirent->d_name).find(':') == std::string::npos)
			continue;

		diskname = read_sysfs_string(std::format("{}/{}/model", pathname, dirent->d_name));
		if (!diskname.empty())
			break;
	}
	closedir(dir);

	return diskname;
}

static std::string model_name(const std::string &path, const std::string &shortname)
{

	DIR *dir;
	struct dirent *dirent;
	std::string pathname;

	pathname = std::format("{}/device", path);

	dir = opendir(pathname.c_str());
	if (!dir)
		return shortname;

	while ((dirent = readdir(dir))) {
		std::string d_name(dirent->d_name);
		if (d_name[0]=='.')
			continue;

		if (d_name.find(':') == std::string::npos)
			continue;
		if (d_name.find("target") == std::string::npos)
			continue;
		std::string result = disk_name(pathname, d_name, shortname);
		closedir(dir);
		return result;
	}
	closedir(dir);

	return "";
}

ahci::ahci(const std::string &_name, const std::string &path): device()
{
	std::string diskname;

	end_active = 0;
	end_slumber = 0;
	end_devslp = 0;
	end_partial = 0;
	start_active = 0;
	start_slumber = 0;
	start_devslp = 0;
	start_partial = 0;
	sysfs_path = path;

	register_sysfs_path(sysfs_path);

	name = std::format("ahci:{}", _name);
	active_index = get_param_index("ahci-link-power-active");
	partial_index = get_param_index("ahci-link-power-partial");

	active_rindex = get_result_index(std::format("{}-active", name));
	partial_rindex = get_result_index(std::format("{}-partial", name));
	slumber_rindex = get_result_index(std::format("{}-slumber", name));
	devslp_rindex = get_result_index(std::format("{}-devslp", name));

	diskname = model_name(sysfs_path, _name);

	if (diskname.empty())
		humanname = pt_format(_("SATA link: {}"), _name);
	else
		humanname = pt_format(_("SATA disk: {}"), diskname);
}

void ahci::start_measurement(void)
{
	start_active = read_sysfs(std::format("{}/ahci_alpm_active", sysfs_path));
	start_partial = read_sysfs(std::format("{}/ahci_alpm_partial", sysfs_path));
	start_slumber = read_sysfs(std::format("{}/ahci_alpm_slumber", sysfs_path));
	start_devslp = read_sysfs(std::format("{}/ahci_alpm_devslp", sysfs_path));
}

void ahci::end_measurement(void)
{
	double p;
	double total;

	end_active = read_sysfs(std::format("{}/ahci_alpm_active", sysfs_path));
	end_partial = read_sysfs(std::format("{}/ahci_alpm_partial", sysfs_path));
	end_slumber = read_sysfs(std::format("{}/ahci_alpm_slumber", sysfs_path));
	end_devslp = read_sysfs(std::format("{}/ahci_alpm_devslp", sysfs_path));

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
	report_utilization(std::format("{}-active", name), p);

	/* percent in partial */
	p = (end_partial - start_partial) / total * 100.0;
	if (p < 0)
		 p = 0;
	report_utilization(std::format("{}-partial", name), p);

	/* percent in slumber */
	p = (end_slumber - start_slumber) / total * 100.0;
	if (p < 0)
		 p = 0;
	report_utilization(std::format("{}-slumber", name), p);

	/* percent in devslp */
	p = (end_devslp - start_devslp) / total * 100.0;
	if (p < 0)
		 p = 0;
	report_utilization(std::format("{}-devslp", name), p);
}


double ahci::utilization(void)
{
	double p;

	p = (end_partial - start_partial + end_active - start_active) / (0.001 + end_active + end_partial + end_slumber + end_devslp - start_active - start_partial - start_slumber - start_devslp) * 100.0;

	if (p < 0)
		p = 0;

	return p;
}

void create_all_ahcis(void)
{
	struct dirent *entry;
	DIR *dir;

	dir = opendir("/sys/class/scsi_host/");
	if (!dir)
		return;
	while (1) {
		class ahci *bl;
		entry = readdir(dir);
		if (!entry)
			break;
		if (entry->d_name[0] == '.')
			continue;

		bool ok = false;
		read_sysfs(std::format("/sys/class/scsi_host/{}/ahci_alpm_accounting", entry->d_name), &ok);
		if (!ok)
			continue;

		write_sysfs(std::format("/sys/class/scsi_host/{}/ahci_alpm_accounting", entry->d_name), "1");

		bl = new class ahci(entry->d_name, std::format("/sys/class/scsi_host/{}", entry->d_name));
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
	std::vector<std::string> ahci_data(cols * rows);
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
}

void ahci::report_device_stats(std::vector<std::string> &ahci_data, int idx)
{
	int offset=(idx*5+5);
	double active_util = get_result_value(active_rindex, &all_results);
	double partial_util = get_result_value(partial_rindex, &all_results);
	double slumber_util = get_result_value(slumber_rindex, &all_results);
	double devslp_util = get_result_value(devslp_rindex, &all_results);

	ahci_data[offset] = humanname;
	offset += 1;

	ahci_data[offset]= std::format("{:5.1f}",  active_util);
	offset +=1;

	ahci_data[offset]= std::format("{:5.1f}",  partial_util);
	offset +=1;

	ahci_data[offset]= std::format("{:5.1f}",  slumber_util);
	offset +=1;

	ahci_data[offset]= std::format("{:5.1f}",  devslp_util);
}
