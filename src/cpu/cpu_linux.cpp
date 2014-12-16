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

#include "cpu.h"
#include "../lib.h"

#include <stdlib.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

void cpu_linux::parse_cstates_start(void)
{
	ifstream file;
	DIR *dir;
	struct dirent *entry;
	char filename[256];
	int len;

	len = snprintf(filename, 256, "/sys/devices/system/cpu/cpu%i/cpuidle", number);

	dir = opendir(filename);
	if (!dir)
		return;

	/* For each C-state, there is a stateX directory which
	 * contains a 'usage' and a 'time' (duration) file */
	while ((entry = readdir(dir))) {
		char linux_name[64];
		char human_name[64];
		uint64_t usage = 0;
		uint64_t duration = 0;


		if (strlen(entry->d_name) < 3)
			continue;

		strcpy(linux_name, entry->d_name);
		strcpy(human_name, linux_name);

		snprintf(filename + len, 256 - len, "/%s/name", entry->d_name);

		file.open(filename, ios::in);
		if (file) {
			file.getline(human_name, 64);
			file.close();
		}

		if (strcmp(human_name, "C0")==0)
			strcpy(human_name, _("C0 polling"));

		snprintf(filename + len, 256 - len, "/%s/usage", entry->d_name);
		file.open(filename, ios::in);
		if (file) {
			file >> usage;
			file.close();
		} else
			continue;

		snprintf(filename + len, 256 - len, "/%s/time", entry->d_name);

		file.open(filename, ios::in);
		if (file) {
			file >> duration;
			file.close();
		}


		update_cstate(linux_name, human_name, usage, duration, 1);

	}
	closedir(dir);
}


void cpu_linux::parse_pstates_start(void)
{
	ifstream file;
	char filename[256];
	unsigned int i;

	last_stamp = 0;
	for (i = 0; i < children.size(); i++)
		if (children[i])
			children[i]->wiggle();

	snprintf(filename, 256, "/sys/devices/system/cpu/cpu%i/cpufreq/stats/time_in_state", first_cpu);

	file.open(filename, ios::in);

	if (file) {
		char line[1024];

		while (file) {
			uint64_t f;
			file.getline(line, 1024);
			f = strtoull(line, NULL, 10);
			account_freq(f, 0);
		}
		file.close();
	}
	account_freq(0, 0);
}

void cpu_linux::measurement_start(void)
{
	abstract_cpu::measurement_start();
	parse_cstates_start();
	parse_pstates_start();
}

void cpu_linux::parse_cstates_end(void)
{
	DIR *dir;
	struct dirent *entry;
	char filename[256];
	ifstream file;
	int len;

	len = snprintf(filename, 256, "/sys/devices/system/cpu/cpu%i/cpuidle", number);

	dir = opendir(filename);
	if (!dir)
		return;

	/* For each C-state, there is a stateX directory which
	 * contains a 'usage' and a 'time' (duration) file */
	while ((entry = readdir(dir))) {
		char linux_name[64];
		char human_name[64];
		uint64_t usage = 0;
		uint64_t duration = 0;


		if (strlen(entry->d_name) < 3)
			continue;

		strcpy(linux_name, entry->d_name);
		strcpy(human_name, linux_name);


		snprintf(filename + len, 256 - len, "/%s/usage", entry->d_name);
		file.open(filename, ios::in);
		if (file) {
			file >> usage;
			file.close();
		} else
			continue;

		snprintf(filename + len, 256 - len, "/%s/time", entry->d_name);

		file.open(filename, ios::in);
		if (file) {
			file >> duration;
			file.close();
		}


		finalize_cstate(linux_name, usage, duration, 1);

	}
	closedir(dir);
}

void cpu_linux::parse_pstates_end(void)
{
	char filename[256];
	ifstream file;

	snprintf(filename, 256, "/sys/devices/system/cpu/cpu%i/cpufreq/stats/time_in_state", number);

	file.open(filename, ios::in);

	if (file) {
		char line[1024];

		while (file) {
			uint64_t f,count;
			char *c;

			memset(line, 0, 1024);

			file.getline(line, 1024);

			f = strtoull(line, &c, 10);
			if (!c)
				break;

			count = strtoull(c, NULL, 10);

			if (f > 0)
				finalize_pstate(f, count, 1);


		}
		file.close();
	}
}

void cpu_linux::measurement_end(void)
{
	parse_cstates_end();
	parse_pstates_end();
	abstract_cpu::measurement_end();
}

char * cpu_linux::fill_cstate_line(int line_nr, char *buffer, const char *separator)
{
	unsigned int i;
	buffer[0] = 0;

	if (line_nr == LEVEL_HEADER) {
		sprintf(buffer,_(" CPU %i"), number);
		return buffer;
	}

	for (i = 0; i < cstates.size(); i++) {
		if (cstates[i]->line_level != line_nr)
			continue;

		if (line_nr == LEVEL_C0)
			sprintf(buffer,"%5.1f%%", percentage(cstates[i]->duration_delta / time_factor));
		else
			sprintf(buffer,"%5.1f%%%s %6.1f ms",
				percentage(cstates[i]->duration_delta / time_factor),
				separator,
				1.0 * cstates[i]->duration_delta / (1 + cstates[i]->usage_delta) / 1000);
	}

	return buffer;
}

char * cpu_linux::fill_cstate_percentage(int line_nr, char *buffer)
{
	unsigned int i;
	buffer[0] = 0;

	for (i = 0; i < cstates.size(); i++) {
		if (cstates[i]->line_level != line_nr)
			continue;

		sprintf(buffer,"%5.1f%%",
			percentage(cstates[i]->duration_delta / time_factor));
		break;
	}

	return buffer;
}

char * cpu_linux::fill_cstate_time(int line_nr, char *buffer)
{
	unsigned int i;
	buffer[0] = 0;

	if (line_nr == LEVEL_C0)
		return buffer;

	for (i = 0; i < cstates.size(); i++) {
		if (cstates[i]->line_level != line_nr)
			continue;

		sprintf(buffer,"%6.1f ms",
			1.0 * cstates[i]->duration_delta /
			(1 + cstates[i]->usage_delta) / 1000);
		break;
	}

	return buffer;
}

char * cpu_linux::fill_cstate_name(int line_nr, char *buffer)
{
	unsigned int i;
	buffer[0] = 0;

	for (i = 0; i < cstates.size(); i++) {
		if (cstates[i]->line_level != line_nr)
			continue;

		sprintf(buffer,"%s", cstates[i]->human_name);
	}

	return buffer;
}


char * cpu_linux::fill_pstate_name(int line_nr, char *buffer)
{
	buffer[0] = 0;

	if (line_nr >= (int)pstates.size() || line_nr < 0)
		return buffer;

	sprintf(buffer,"%s", pstates[line_nr]->human_name);

	return buffer;
}

char * cpu_linux::fill_pstate_line(int line_nr, char *buffer)
{
	buffer[0] = 0;

	if (total_stamp ==0) {
		unsigned int i;
		for (i = 0; i < pstates.size(); i++)
			total_stamp += pstates[i]->time_after;
		if (total_stamp == 0)
			total_stamp = 1;
	}

	if (line_nr == LEVEL_HEADER) {
		sprintf(buffer,_(" CPU %i"), number);
		return buffer;
	}

	if (line_nr >= (int)pstates.size() || line_nr < 0)
		return buffer;

	sprintf(buffer," %5.1f%% ", percentage(1.0* (pstates[line_nr]->time_after) / total_stamp));
	return buffer;
}
