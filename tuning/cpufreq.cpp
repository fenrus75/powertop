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

#include "tuning.h"
#include "tunable.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility>
#include <iostream>
#include <fstream>
#include <unistd.h> 
#include <dirent.h>
#include <errno.h>

#include "../lib.h"
#include "cpufreq.h"

cpufreq_tunable::cpufreq_tunable(void) : tunable("", 0.3, _("Good"), _("Bad"), _("Unknown"))
{
	string str;
	sprintf(desc, _("Using 'ondemand' cpufreq governor"));

	str = read_sysfs_string("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
	strcpy(original, str.c_str());
	if (strlen(original) < 1)
		strcpy(original, "ondemand");
}


int cpufreq_tunable::good_bad(void)
{
	DIR *dir;
	struct dirent *dirent;
	FILE *file;
	char filename[PATH_MAX];
	char line[1024];

	char gov[1024];
	int ret = TUNE_GOOD;


	gov[0] = 0;


	dir = opendir("/sys/devices/system/cpu");
	if (!dir)
		return ret;

	while ((dirent = readdir(dir))) {
		if (dirent->d_name[0]=='.')
			continue;
		sprintf(filename, "/sys/devices/system/cpu/%s/cpufreq/scaling_governor", dirent->d_name);
		file = fopen(filename, "r");
		if (!file)
			continue;
		memset(line, 0, 1024);
		if (fgets(line, 1023,file)==NULL) {
			fclose(file);
			continue;
		}
		if (strlen(gov)==0)
			strcpy(gov, line);
		else
			/* if the governors are inconsistent, warn */
			if (strcmp(gov, line))
				ret = TUNE_BAD;
		fclose(file);
	}

	closedir(dir);

	/* if the governor is set to userspace, also warn */
	if (strstr(gov, "userspace"))
		ret = TUNE_BAD;

	/* if the governor is set to performance, also warn */
	/* FIXME: check if this is fair on all cpus */
	if (strstr(gov, "performance"))
		ret = TUNE_BAD;

	return ret;
}

void cpufreq_tunable::toggle(void)
{
	DIR *dir;
	struct dirent *dirent;
	FILE *file;
	char filename[PATH_MAX];
	int good;
	good = good_bad();

	system("/sbin/modprobe cpufreq_ondemand > /dev/null 2>&1");

	if (good == TUNE_GOOD) {
		dir = opendir("/sys/devices/system/cpu");
		if (!dir)
			return;

		while ((dirent = readdir(dir))) {
			if (dirent->d_name[0]=='.')
				continue;
			sprintf(filename, "/sys/devices/system/cpu/%s/cpufreq/scaling_governor", dirent->d_name);
			file = fopen(filename, "w");
			if (!file)
				continue;
			fprintf(file, "%s\n", original);
			fclose(file);
		}

		closedir(dir);
		return;
	}
	dir = opendir("/sys/devices/system/cpu");
	if (!dir)
		return;

	while ((dirent = readdir(dir))) {
		if (dirent->d_name[0]=='.')
			continue;
		sprintf(filename, "/sys/devices/system/cpu/%s/cpufreq/scaling_governor", dirent->d_name);
		file = fopen(filename, "w");
		if (!file)
			continue;
		fprintf(file, "ondemand\n");
		fclose(file);
	}

	closedir(dir);
}



void add_cpufreq_tunable(void)
{
	class cpufreq_tunable *cf;

	cf = new class cpufreq_tunable();
	all_tunables.push_back(cf);
}

