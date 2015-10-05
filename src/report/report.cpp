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
#include <string.h>
#include <utility>
#include <iostream>
#include <fstream>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <limits.h>
#include "report-data-html.h"

using namespace std;

struct reportstream reportout;
report_type reporttype = REPORT_OFF;
report_maker report(REPORT_OFF);

string cpu_model(void)
{
	ifstream file;

	file.open("/proc/cpuinfo", ios::in);

	if (!file)
		return "";

	while (file) {
		char line[4096];
		file.getline(line, 4096);
		if (strstr(line, "model name")) {
			char *c;
			c = strchr(line, ':');
			if (c) {
				file.close();
				c++;
				return c;
			}
		}
	}
	file.close();
	return "";
}

static string read_os_release(const string &filename)
{
	ifstream file;
	char content[4096];
	char *c;
	const char *pname = "PRETTY_NAME=";
	string os("");

	file.open(filename.c_str(), ios::in);
	if (!file)
		return "";
	while (file.getline(content, 4096)) {
		if (strncasecmp(pname, content, strlen(pname)) == 0) {
			c = strchr(content, '=');
			if (!c)
				break;
			c += 1;
			if (*c == '"' || *c == '\'')
				c += 1;
			*strchrnul(c, '"') = 0;
			*strchrnul(c, '\'') = 0;
			os = c;
			break;
		}
	}
	file.close();
	return os;
}

static void system_info(void)
{
	string str;
	char version_date[64];
	time_t now = time(NULL);

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
	string *system_data = new string[sys_table.rows * sys_table.cols];
	system_data[0]=__("PowerTOP Version");
	snprintf(version_date, sizeof(version_date), "%s ran at %s", POWERTOP_VERSION, ctime(&now));
	system_data[1]=version_date;

	str = read_sysfs_string("/proc/version");
	size_t  found = str.find(" ");
	found = str.find(" ", found+1);
	found = str.find(" ", found+1);
	str = str.substr(0,found);
	system_data[2]=__("Kernel Version");
	system_data[3]=str.c_str();

	str  = read_sysfs_string("/sys/devices/virtual/dmi/id/board_vendor");
	system_data[4]=__("System Name");
	system_data[5]= str.c_str();
	str = read_sysfs_string("/sys/devices/virtual/dmi/id/board_name");
	system_data[5].append(str.c_str());
	str = read_sysfs_string("/sys/devices/virtual/dmi/id/product_version");
	system_data[5].append(str.c_str());
	str = cpu_model();
	system_data[6]=__("CPU Information");
	stringstream n_proc;
	n_proc << sysconf(_SC_NPROCESSORS_ONLN);
	system_data[7]= n_proc.str();
	system_data[7].append(str.c_str());

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
	delete [] system_data;
}

void init_report_output(char *filename_str, int iterations)
{
	size_t period;
	string filename;
	time_t stamp;
	char datestr[200];

	if (iterations == 1)
		snprintf(reportout.filename, PATH_MAX, "%s", filename_str);
	else
	{
		filename = string(filename_str);
		period = filename.find_last_of(".");
		if (period > filename.length())
			period = filename.length();
		memset(&datestr, 0, 200);
		memset(&stamp, 0, sizeof(time_t));
		stamp = time(NULL);
		strftime(datestr, sizeof(datestr), "%Y%m%d-%H%M%S", localtime(&stamp));
		snprintf(reportout.filename, PATH_MAX, "%s-%s%s",
			filename.substr(0, period).c_str(), datestr,
			filename.substr(period).c_str());
	}
	
	reportout.report_file = fopen(reportout.filename, "wm");
	if (!reportout.report_file) {
		fprintf(stderr, _("Cannot open output file %s (%s)\n"),
			reportout.filename, strerror(errno));
	}

	report.set_type(reporttype);
	system_info();
}

void finish_report_output(void)
{
	if (reporttype == REPORT_OFF)
		return;

	report.finish_report();
	if (reportout.report_file)
	{
		fprintf(stderr, _("PowerTOP outputing using base filename %s\n"), reportout.filename);
		fputs(report.get_result(), reportout.report_file);
		fdatasync(fileno(reportout.report_file));
		fclose(reportout.report_file);
	}
	report.clear_result();
}
