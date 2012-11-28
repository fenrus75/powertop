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
	string str, str2, str3;

	report.begin_section(SECTION_SYSINFO);
	report.add_header("System Information");
	report.begin_table();
	report.begin_row(ROW_SYSINFO);
	report.begin_cell(CELL_SYSINFO);
	report.add("PowerTOP Version");
	report.begin_cell();
	report.add(POWERTOP_VERSION);

	str = read_sysfs_string("/proc/version");
	report.begin_row(ROW_SYSINFO);
	report.begin_cell();
	report.add("Kernel Version");
	report.begin_cell();
	report.add(str.c_str());

	str  = read_sysfs_string("/sys/devices/virtual/dmi/id/board_vendor");
	str2 = read_sysfs_string("/sys/devices/virtual/dmi/id/board_name");
	str3 = read_sysfs_string("/sys/devices/virtual/dmi/id/product_version");
	report.begin_row(ROW_SYSINFO);
	report.begin_cell();
	report.add("System Name");
	report.begin_cell();
	report.addf("%s %s %s", str.c_str(), str2.c_str(), str3.c_str());

	str = cpu_model();
	report.begin_row(ROW_SYSINFO);
	report.begin_cell();
	report.add("CPU Information");
	report.begin_cell();
	report.addf("%lix %s", sysconf(_SC_NPROCESSORS_ONLN), str.c_str());

	str = read_sysfs_string("/etc/system-release");
	if (str.length() < 1)
		str = read_sysfs_string("/etc/redhat-release");
	if (str.length() < 1)
		str = read_os_release("/etc/os-release");

	report.begin_row(ROW_SYSINFO);
	report.begin_cell();
	report.add("OS Information");
	report.begin_cell();
	report.add(str.c_str());
}

void init_report_output(char *filename_str, int iterations)
{
	size_t period;
	char file_prefix[4096];
	char file_postfix[8];
	time_t stamp;
	char datestr[200];

	string mystring = string(filename_str);
	sprintf(file_postfix, "%s",
		(reporttype == REPORT_HTML ? "html" : "csv"));
	period=mystring.find_last_of(".");
	sprintf(file_prefix, "%s",mystring.substr(0,period).c_str());
	memset(&datestr, 0, 200);
	memset(&stamp, 0, sizeof(time_t));
	stamp=time(NULL);
	strftime(datestr, sizeof(datestr), "%Y%m%d-%H%M%S", localtime(&stamp));

	if (iterations != 1)
		sprintf(reportout.filename, "%s-%s.%s",
			file_prefix, datestr,file_postfix);
	else
		sprintf(reportout.filename, "%s.%s",
			file_prefix, file_postfix);

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
