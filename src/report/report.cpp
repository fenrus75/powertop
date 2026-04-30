/*
 * Copyright 2011, Intel Corporation
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
 *  Chris Ferron <chris.e.ferron@linux.intel.com>
 */

#include "lib.h"
#include "report.h"
#include "report-maker.h"
#include <errno.h>
#include <cstring>
#include <utility>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <malloc.h>
#include <unistd.h>
#include <limits.h>
#include <format>
#include "report-data-html.h"

struct reportstream reportout;
report_type reporttype = REPORT_OFF;
report_maker report(REPORT_OFF);

std::string cpu_model(void)
{
	std::string content = read_file_content("/proc/cpuinfo");
	if (content.empty())
		return "";

	std::istringstream stream(content);
	std::string line;

	while (std::getline(stream, line)) {
		if (line.find("model name") != std::string::npos) {
			size_t pos = line.find(':');
			if (pos != std::string::npos) {
				std::string model = line.substr(pos + 1);
				/* Trim leading space if any */
				if (!model.empty() && model[0] == ' ')
					model.erase(0, 1);
				return model;
			}
		}
	}
	return "";
}

static std::string read_os_release(const std::string &filename)
{
	const std::string pname = "PRETTY_NAME=";
	std::string content = read_file_content(filename);
	if (content.empty())
		return "";

	std::istringstream stream(content);
	std::string line;
	while (std::getline(stream, line)) {
		if (line.starts_with(pname)) {
			size_t pos = line.find('=');
			if (pos == std::string::npos)
				break;
			std::string os = line.substr(pos + 1);
			if (os.length() >= 2 && (os.front() == '"' || os.front() == '\'')) {
				os.erase(0, 1);
				size_t end_pos = os.find_first_of("\"\'");
				if (end_pos != std::string::npos)
					os.erase(end_pos);
			}
			return os;
		}
	}
	return "";
}

static std::string get_time_string(const std::string &fmt, time_t t)
{
	char buf[128];
	struct tm *tm_info = localtime(&t);
	if (!tm_info)
		return "";
	if (strftime(buf, sizeof(buf), fmt.c_str(), tm_info))
		return buf;
	return "";
}

static void system_info(void)
{
	std::string str;
	time_t now = time(nullptr);

	/* div attr css_class and css_id */
	tag_attr div_attr;
	init_div(&div_attr, "sys_info", "");

	/* Set Table attributes, rows, and cols */
	table_attributes sys_table;
	init_top_table_attr(&sys_table, 5, 2);

	/* Set Title attributes */
	tag_attr title_attr;
        init_title_attr(&title_attr);

	/* Set array of data in row Major order */
	std::vector<std::string> system_data(sys_table.rows * sys_table.cols);
	system_data[0]=__("PowerTOP Version");
	system_data[1] = pt_format(_("{} ran at {}"), PACKAGE_VERSION, get_time_string("%c", now));

	str = read_sysfs_string("/proc/version");
	size_t  found = str.find(" ");
	found = str.find(" ", found+1);
	found = str.find(" ", found+1);
	str = str.substr(0,found);
	system_data[2]=__("Kernel Version");
	system_data[3]=str;

	str  = read_sysfs_string("/sys/devices/virtual/dmi/id/board_vendor");
	system_data[4]=__("System Name");
	system_data[5]= str;
	system_data[5] += read_sysfs_string("/sys/devices/virtual/dmi/id/board_name");
	system_data[5] += read_sysfs_string("/sys/devices/virtual/dmi/id/product_version");

	system_data[6]=__("CPU Information");
	system_data[7] = std::to_string(sysconf(_SC_NPROCESSORS_ONLN));
	system_data[7] += cpu_model();

	str = read_sysfs_string("/etc/system-release");
	if (str.length() < 1)
	str = read_sysfs_string("/etc/redhat-release");
	if (str.length() < 1)
	str = read_os_release("/etc/os-release");

	system_data[8]=__("OS Information");
	system_data[9]=str;

	/* Report Output */
	report.add_header();
	report.add_logo();
	report.add_div(&div_attr);
	report.add_title(&title_attr, __("System Information"));
	report.add_table(system_data, &sys_table);
	report.end_header();
	report.end_div();
	report.add_navigation();
}

void init_report_output(const std::string &filename_str, int iterations)
{
	size_t period;
	time_t stamp;

	if (iterations == 1)
		reportout.filename = filename_str;
	else
	{
		period = filename_str.find_last_of(".");
		if (period == std::string::npos)
			period = filename_str.length();

		stamp = time(nullptr);
		reportout.filename = std::format("{}-{}{}",
			filename_str.substr(0, period),
			get_time_string("%Y%m%d-%H%M%S", stamp),
			filename_str.substr(period));
	}

	reportout.report_file.open(reportout.filename, std::ios::out);
	if (!reportout.report_file) {
		fprintf(stderr, _("Cannot open output file %s (%s)\n"),
			reportout.filename.c_str(), strerror(errno));
	}

	report.set_type(reporttype);
	system_info();
}

void finish_report_output(void)
{
	if (reporttype == REPORT_OFF)
		return;

	report.finish_report();
	if (reportout.report_file.is_open())
	{
		fprintf(stderr, _("PowerTOP outputting using base filename %s\n"), reportout.filename.c_str());
		reportout.report_file << report.get_result();
		reportout.report_file.flush();
		reportout.report_file.close();
	}
	report.clear_result();
}
