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

#include "css.h"
#include "lib.h"
#include "report.h"
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
bool reporttype;

static void css_header(void)
{
	if (!reportout.http_report)
		return;


#ifdef EXTERNAL_CSS_FILE
	if (reporttype)
		fprintf(reportout.http_report, "<link rel=\"stylesheet\" href=\"powertop.css\">\n");
#else
	if (reporttype) {
		fprintf(reportout.http_report, "%s\n", css);
	}
#endif
}

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
	if ((!reportout.csv_report)&&(!reportout.http_report))
		return;

	if (reporttype) {
		fprintf(reportout.http_report, "<div id=\"top\">\n<h1><a href=\"#top\">&nbsp;</a></h1>\n</div>\n<div id=\"system\">\n<table>");
		fprintf(reportout.http_report, "<tr class=\"system_even\"><td width=20%%>PowerTOP Version</td><td>%s</td></tr>\n", POWERTOP_VERSION);
	} else {
		fprintf(reportout.csv_report, "***PowerTOP Report***, \n");
		fprintf(reportout.csv_report, "**System Information**, \n");
		fprintf(reportout.csv_report, "\n");
		fprintf(reportout.csv_report,  "PowerTOP Version:, \"%s\", \n", POWERTOP_VERSION);
	}
	str = read_sysfs_string("/proc/version");

	if (reporttype)
		fprintf(reportout.http_report,"<tr class=\"system_odd\"><td>Kernel Version</td><td>%s</td></tr>\n",
				str.c_str());
	else
		fprintf(reportout.csv_report, "Kernel Version:, \"%s\", \n",
				str.c_str());

	str = read_sysfs_string("/sys/devices/virtual/dmi/id/board_vendor");
	str2 = read_sysfs_string("/sys/devices/virtual/dmi/id/board_name");
	str3 = read_sysfs_string("/sys/devices/virtual/dmi/id/product_version");

	if (reporttype)
		fprintf(reportout.http_report, "<tr class=\"system_even\"><td>System Name</td><td>%s %s %s</td></tr>\n",
				str.c_str(), str2.c_str(), str3.c_str());
	else
		fprintf(reportout.csv_report,"System Name:,\"%s %s %s,\" \n",
				str.c_str(), str2.c_str(), str3.c_str());

	str = cpu_model();

	if (reporttype)
		fprintf(reportout.http_report, "<tr class=\"system_odd\"><td>CPU Information</td><td>%lix %s</td></tr>\n",
			 sysconf(_SC_NPROCESSORS_ONLN), str.c_str());
	else
		fprintf(reportout.csv_report,"CPU Information:, %lix \"%s\", \n",
			 sysconf(_SC_NPROCESSORS_ONLN), str.c_str());

	str = read_sysfs_string("/etc/system-release");
	if (str.length() < 1)
		str = read_sysfs_string("/etc/redhat-release");
	if (str.length() < 1)
		str = read_os_release("/etc/os-release");

	if (reporttype) {
		fprintf(reportout.http_report, "<tr class=\"system_even\"><td>OS Information</td><td>%s</td></tr>\n",
				 str.c_str());
		fprintf(reportout.http_report,"</table></div>\n");
	} else {
		fprintf(reportout.csv_report,"OS Information:,\"%s\", \n",
				 str.c_str());
		fprintf(reportout.csv_report,"\n");
	}
}


void init_report_output(char *filename_str)
{
	size_t period;
	char file_prefix[256];
	char file_postfix[8];
	time_t stamp;
	char datestr[200];

	string mystring = string(filename_str);
	sprintf(file_postfix, "%s", reporttype ? "html":"csv");
	period=mystring.find_last_of(".");
	sprintf(file_prefix, "%s",mystring.substr(0,period).c_str());
	if (reporttype) {
                memset(&datestr, 0, 200);
                memset(&stamp, 0, sizeof(time_t));
                stamp=time(NULL);
                strftime(datestr, sizeof(datestr), "%Y%m%d-%H%M%S", localtime(&stamp));
                sprintf(reportout.filename, "%s-%s.%s", file_prefix, datestr,file_postfix);
		reportout.http_report = fopen(reportout.filename, "wm");
		if (!reportout.http_report) {
			fprintf(stderr, "Cannot open output file %s (%s)\n", reportout.filename, strerror(errno));
		}
	}else {
		memset(&datestr, 0, 200);
		memset(&stamp, 0, sizeof(time_t));
		stamp=time(NULL);
		strftime(datestr, sizeof(datestr), "%Y%m%d-%H%M%S", localtime(&stamp));
		sprintf(reportout.filename, "%s-%s.%s", file_prefix, datestr,file_postfix);
		reportout.csv_report = fopen(reportout.filename, "wm");
		if (!reportout.csv_report) {
			fprintf(stderr, "Cannot open output file %s (%s)\n", reportout.filename, strerror(errno));
		}
	}

	http_header_output();
	system_info();
}

void http_header_output(void) {
	if (!reportout.http_report)
		return;

	css_header();


}

void finish_report_output(void)
{
	printf("PowerTOP outputing using base filename %s\n", reportout.filename);

	if (reportout.http_report){
		fprintf(reportout.http_report, "</body>\n\n </html>\n");
		fflush(reportout.http_report);
		fdatasync(fileno(reportout.http_report));
		fclose(reportout.http_report);
	}
	if (reportout.csv_report) {
		fflush(reportout.csv_report);
		fdatasync(fileno(reportout.csv_report));
		fclose(reportout.csv_report);
	}

}
