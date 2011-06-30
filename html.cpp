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

#include "html.h"
#include <errno.h>
#include <string.h>
#include <utility>
#include <iostream>
#include <fstream>

#include "css.h"
#include "lib.h"
#include <unistd.h>

using namespace std;


FILE *htmlout;

static void css_header(void)
{
	if (!htmlout)
		return;

#ifdef EXTERNAL_CSS_FILE
	fprintf(htmlout, "<link rel=\"stylesheet\" href=\"powertop.css\">\n");
#else
	fprintf(htmlout, "<style type=\"text/css\">\n");
	fprintf(htmlout, "%s\n", css);
	fprintf(htmlout, "</style>\n");
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

static void system_info(void)
{
	string str, str2, str3;
	if (!htmlout)
		return;

	fprintf(htmlout, "<h1>PowerTOP report</h1>\n");

	fprintf(htmlout, "<h2>System information</h2>\n");

	fprintf(htmlout, "<table width=100%%>\n");

	fprintf(htmlout, "<tr class=\"system_even\"><td width=20%%>PowerTOP version</td><td>%s</td></tr>\n", POWERTOP_VERSION);

	str = read_sysfs_string("/proc/version");
	fprintf(htmlout, "<tr class=\"system_odd\"><td>Kernel version</td><td>%s</td></tr>\n", str.c_str());

	str = read_sysfs_string("/sys/devices/virtual/dmi/id/board_vendor");
	str2 = read_sysfs_string("/sys/devices/virtual/dmi/id/board_name");
	str3 = read_sysfs_string("/sys/devices/virtual/dmi/id/product_version");
	fprintf(htmlout, "<tr class=\"system_even\"><td>System name</td><td>%s %s %s</td></tr>\n", str.c_str(), str2.c_str(), str3.c_str());

	str = cpu_model();

	fprintf(htmlout, "<tr class=\"system_odd\"><td>CPU information</td><td>%lix %s</td></tr>\n", sysconf(_SC_NPROCESSORS_ONLN), str.c_str());
	str = read_sysfs_string("/etc/system-release");
	if (str.length() < 1)
		str = read_sysfs_string("/etc/redhat-release");
	fprintf(htmlout, "<tr class=\"system_even\"><td>OS information</td><td>%s</td></tr>\n", str.c_str());

	fprintf(htmlout, "</table>\n");

}



void init_html_output(const char *filename)
{
	htmlout = fopen(filename, "wm");

	if (!htmlout) {
		fprintf(stderr, "Cannot open output file %s (%s)\n", filename, strerror(errno));
	}

	printf("Writing PowerTOP output to file %s\n", filename);
	fprintf(htmlout, "<!DOCTYPE html PUBLIC \"-//W3C/DTD HTML 4.01//EN\">\n");
	fprintf(htmlout, "<html>\n\n");
	fprintf(htmlout, "<head>\n");
	
	css_header();

	fprintf(htmlout, "</head>\n\n");
	fprintf(htmlout, "<body>\n");

	system_info();
}

void finish_html_output(void)
{
	if (!htmlout)
		return;

	fprintf(htmlout, "</body>\n\n");
	fprintf(htmlout, "</html>\n");
	fflush(htmlout);
	fclose(htmlout);
}
