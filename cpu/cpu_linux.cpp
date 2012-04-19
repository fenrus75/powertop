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

static int is_turbo(uint64_t freq, uint64_t max, uint64_t maxmo)
{
	if (freq != max)
		return 0;
	if (maxmo + 1000 != max)
		return 0;
	return 1;
}

void cpu_linux::measurement_start(void)
{
	ifstream file;

	DIR *dir;
	struct dirent *entry;
	char filename[256];
	int len;
	unsigned int i;

	abstract_cpu::measurement_start();

	len = sprintf(filename, "/sys/devices/system/cpu/cpu%i/cpuidle", number);

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

		sprintf(filename + len, "/%s/name", entry->d_name);

		file.open(filename, ios::in);
		if (file) {
			file.getline(human_name, 64);
			file.close();
		}

		if (strcmp(human_name, "C0")==0)
			strcpy(human_name, _("C0 polling"));

		sprintf(filename + len, "/%s/usage", entry->d_name);
		file.open(filename, ios::in);
		if (file) {
			file >> usage;
			file.close();
		}

		sprintf(filename + len, "/%s/time", entry->d_name);

		file.open(filename, ios::in);
		if (file) {
			file >> duration;
			file.close();
		}


		update_cstate(linux_name, human_name, usage, duration, 1);

	}
	closedir(dir);

	last_stamp = 0;

	for (i = 0; i < children.size(); i++)
		if (children[i])
			children[i]->wiggle();

	sprintf(filename, "/sys/devices/system/cpu/cpu%i/cpufreq/stats/time_in_state", first_cpu);

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


void cpu_linux::measurement_end(void)
{
	DIR *dir;
	struct dirent *entry;
	char filename[256];
	ifstream file;
	int len;

	len = sprintf(filename, "/sys/devices/system/cpu/cpu%i/cpuidle", number);

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


		sprintf(filename + len, "/%s/usage", entry->d_name);
		file.open(filename, ios::in);
		if (file) {
			file >> usage;
			file.close();
		}

		sprintf(filename + len, "/%s/time", entry->d_name);

		file.open(filename, ios::in);
		if (file) {
			file >> duration;
			file.close();
		}


		finalize_cstate(linux_name, usage, duration, 1);

	}
	closedir(dir);

	sprintf(filename, "/sys/devices/system/cpu/cpu%i/cpufreq/stats/time_in_state", number);

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
			sprintf(buffer,"%5.1f%%%s", percentage(cstates[i]->duration_delta / time_factor), separator);
		else
			sprintf(buffer,"%5.1f%%%s %6.1f ms", percentage(cstates[i]->duration_delta / time_factor), separator, 1.0 * cstates[i]->duration_delta / (1+cstates[i]->usage_delta) / 1000);
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




void cpu_linux::account_freq(uint64_t freq, uint64_t duration)
{
	struct frequency *state = NULL;
	unsigned int i;


	for (i = 0; i < pstates.size(); i++) {
		if (freq == pstates[i]->freq) {
			state = pstates[i];
			break;
		}
	}

	if (!state) {
		state = new(std::nothrow) struct frequency;

		if (!state)
			return;

		memset(state, 0, sizeof(*state));

		pstates.push_back(state);

		state->freq = freq;
		hz_to_human(freq, state->human_name);
		if (freq == 0)
			strcpy(state->human_name, _("Idle"));
		if (is_turbo(freq, max_frequency, max_minus_one_frequency))
			sprintf(state->human_name, _("Turbo Mode"));

		state->after_count = 1;
	}


	state->time_after += duration;


}

void cpu_linux::change_freq(uint64_t time, int frequency)
{
	current_frequency = frequency;

	if (parent)
		parent->calculate_freq(time);
	old_idle = idle;
}

void cpu_linux::change_effective_frequency(uint64_t time, uint64_t frequency)
{
	uint64_t time_delta, fr;

	if (last_stamp)
		time_delta = time - last_stamp;
	else
		time_delta = 1;

	fr = effective_frequency;
	if (old_idle)
		fr = 0;

	account_freq(fr, time_delta);

	effective_frequency = frequency;
	last_stamp = time;
}

void cpu_linux::go_idle(uint64_t time)
{

	idle = true;

	if (parent)
		parent->calculate_freq(time);
	old_idle = idle;
}


void cpu_linux::go_unidle(uint64_t time)
{
	idle = false;
	if (parent)
		parent->calculate_freq(time);
	old_idle = idle;
}
