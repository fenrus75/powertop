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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "cpu.h"
#include "../lib.h"

abstract_cpu::~abstract_cpu()
{
	unsigned int i=0;
	for (i=0; i < cstates.size(); i++){
		delete cstates[i];
	}
	cstates.clear();

	for (i=0; i < pstates.size(); i++){
		delete pstates[i];
	}
	pstates.clear();
}

void abstract_cpu::account_freq(uint64_t freq, uint64_t duration)
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

void abstract_cpu::freq_updated(uint64_t time)
{
	if(parent)
		parent->calculate_freq(time);
	old_idle = idle;
}

void abstract_cpu::measurement_start(void)
{
	unsigned int i;
	ifstream file;
	char filename[4096];

	last_stamp = 0;

	for (i = 0; i < cstates.size(); i++)
		delete cstates[i];
	cstates.resize(0);

	for (i = 0; i < pstates.size(); i++)
		delete pstates[i];
	pstates.resize(0);

	current_frequency = 0;
	idle = false;
	old_idle = true;


	snprintf(filename, PATH_MAX, "/sys/devices/system/cpu/cpu%i/cpufreq/scaling_available_frequencies", number);
	file.open(filename, ios::in);
	if (file) {
		file >> max_frequency;
		file >> max_minus_one_frequency;
		file.close();
	}

	for (i = 0; i < children.size(); i++)
		if (children[i])
			children[i]->measurement_start();

	gettimeofday(&stamp_before, NULL);

	last_stamp = 0;

	for (i = 0; i < children.size(); i++)
		if (children[i])
			children[i]->wiggle();

}

void abstract_cpu::measurement_end(void)
{
	unsigned int i, j;

	total_stamp = 0;
	gettimeofday(&stamp_after, NULL);
	for (i = 0; i < children.size(); i++)
		if (children[i])
			children[i]->wiggle();

	time_factor = 1000000.0 * (stamp_after.tv_sec - stamp_before.tv_sec) + stamp_after.tv_usec - stamp_before.tv_usec;


	for (i = 0; i < children.size(); i++)
		if (children[i])
			children[i]->measurement_end();

	for (i = 0; i < children.size(); i++)
		if (children[i]) {
			for (j = 0; j < children[i]->cstates.size(); j++) {
				struct idle_state *state;
				state = children[i]->cstates[j];
				if (!state)
					continue;

				update_cstate( state->linux_name, state->human_name, state->usage_before, state->duration_before, state->before_count);
				finalize_cstate(state->linux_name,                   state->usage_after,  state->duration_after,  state->after_count);
			}
			for (j = 0; j < children[i]->pstates.size(); j++) {
				struct frequency *state;
				state = children[i]->pstates[j];
				if (!state)
					continue;

				update_pstate(  state->freq, state->human_name, state->time_before, state->before_count);
				finalize_pstate(state->freq,                    state->time_after,  state->after_count);
			}
		}

	for (i = 0; i < cstates.size(); i++) {
		struct idle_state *state = cstates[i];

		if (state->after_count == 0) {
			cout << "after count is 0 " << state->linux_name << "\n";
			continue;
		}

		if (state->after_count != state->before_count) {
			cout << "count mismatch " << state->after_count << " " << state->before_count << " on cpu " << number << "\n";
			continue;
		}

		state->usage_delta =    (state->usage_after    - state->usage_before)    / state->after_count;
		state->duration_delta = (state->duration_after - state->duration_before) / state->after_count;
	}
}

void abstract_cpu::insert_cstate(const char *linux_name, const char *human_name, uint64_t usage, uint64_t duration, int count, int level)
{
	struct idle_state *state;
	const char *c;

	state = new(std::nothrow) struct idle_state;

	if (!state)
		return;

	memset(state, 0, sizeof(*state));

	cstates.push_back(state);

	strcpy(state->linux_name, linux_name);
	strcpy(state->human_name, human_name);

	state->line_level = -1;

	c = human_name;
	while (*c) {
		if (strcmp(linux_name, "active")==0) {
			state->line_level = LEVEL_C0;
			break;
		}
		if (*c >= '0' && *c <='9') {
			state->line_level = strtoull(c, NULL, 10);
			break;
		}
		c++;
	}

	/* some architectures (ARM) don't have good numbers in their human name.. fall back to the linux name for those */
	c = linux_name;
	while (*c && state->line_level < 0) {
		if (*c >= '0' && *c <='9') {
			state->line_level = strtoull(c, NULL, 10);
			break;
		}
		c++;
	}

	if (level >= 0)
		state->line_level = level;

	state->usage_before = usage;
	state->duration_before = duration;
	state->before_count = count;
}

void abstract_cpu::finalize_cstate(const char *linux_name, uint64_t usage, uint64_t duration, int count)
{
	unsigned int i;
	struct idle_state *state = NULL;

	for (i = 0; i < cstates.size(); i++) {
		if (strcmp(linux_name, cstates[i]->linux_name) == 0) {
			state = cstates[i];
			break;
		}
	}

	if (!state) {
		cout << "Invalid C state finalize " << linux_name << " \n";
		return;
	}

	state->usage_after += usage;
	state->duration_after += duration;
	state->after_count += count;
}

void abstract_cpu::update_cstate(const char *linux_name, const char *human_name, uint64_t usage, uint64_t duration, int count, int level)
{
	unsigned int i;
	struct idle_state *state = NULL;

	for (i = 0; i < cstates.size(); i++) {
		if (strcmp(linux_name, cstates[i]->linux_name) == 0) {
			state = cstates[i];
			break;
		}
	}

	if (!state) {
		insert_cstate(linux_name, human_name, usage, duration, count, level);
		return;
	}

	state->usage_before += usage;
	state->duration_before += duration;
	state->before_count += count;

}

int abstract_cpu::has_cstate_level(int level)
{
	unsigned int i;

	if (level == LEVEL_HEADER)
		return 1;

	for (i = 0; i < cstates.size(); i++)
		if (cstates[i]->line_level == level)
			return 1;

	for (i = 0; i < children.size(); i++)
		if (children[i])
			if (children[i]->has_cstate_level(level))
				return 1;
	return  0;
}

int abstract_cpu::has_pstate_level(int level)
{
	unsigned int i;

	if (level == LEVEL_HEADER)
		return 1;

	if (level >= 0 && level < (int)pstates.size())
		return 1;

	for (i = 0; i < children.size(); i++)
		if (children[i])
			if (children[i]->has_pstate_level(level))
				return 1;
	return  0;
}



void abstract_cpu::insert_pstate(uint64_t freq, const char *human_name, uint64_t duration, int count)
{
	struct frequency *state;

	state = new(std::nothrow) struct frequency;

	if (!state)
		return;

	memset(state, 0, sizeof(*state));

	pstates.push_back(state);

	state->freq = freq;
	strcpy(state->human_name, human_name);


	state->time_before = duration;
	state->before_count = count;
}

void abstract_cpu::finalize_pstate(uint64_t freq, uint64_t duration, int count)
{
	unsigned int i;
	struct frequency *state = NULL;

	for (i = 0; i < pstates.size(); i++) {
		if (freq == pstates[i]->freq) {
			state = pstates[i];
			break;
		}
	}

	if (!state) {
		cout << "Invalid P state finalize " << freq << " \n";
		return;
	}
	state->time_after += duration;
	state->after_count += count;

}

void abstract_cpu::update_pstate(uint64_t freq, const char *human_name, uint64_t duration, int count)
{
	unsigned int i;
	struct frequency *state = NULL;

	for (i = 0; i < pstates.size(); i++) {
		if (freq == pstates[i]->freq) {
			state = pstates[i];
			break;
		}
	}

	if (!state) {
		insert_pstate(freq, human_name, duration, count);
		return;
	}

	state->time_before += duration;
	state->before_count += count;
}


void abstract_cpu::calculate_freq(uint64_t time)
{
	uint64_t freq = 0;
	bool is_idle = true;
	unsigned int i;

	/* calculate the maximum frequency of all children */
	for (i = 0; i < children.size(); i++)
		if (children[i] && children[i]->has_pstates()) {
			uint64_t f = 0;
			if (!children[i]->idle) {
				f = children[i]->current_frequency;
				is_idle = false;
			}
			if (f > freq)
				freq = f;
		}

	current_frequency = freq;
	idle = is_idle;
	freq_updated(time);
}

void abstract_cpu::change_effective_frequency(uint64_t time, uint64_t frequency)
{
	unsigned int i;
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

	/* propagate to all children */
	for (i = 0; i < children.size(); i++)
		if (children[i]) {
			children[i]->change_effective_frequency(time, frequency);
		}
}


void abstract_cpu::wiggle(void)
{
	char filename[PATH_MAX];
	ifstream ifile;
	ofstream ofile;
	uint64_t minf,maxf;

	/* wiggle a CPU so that we have a record of it at the start and end of the perf trace */

	snprintf(filename, PATH_MAX, "/sys/devices/system/cpu/cpu%i/cpufreq/scaling_max_freq", first_cpu);
	ifile.open(filename, ios::in);
	ifile >> maxf;
	ifile.close();

	snprintf(filename, PATH_MAX, "/sys/devices/system/cpu/cpu%i/cpufreq/scaling_min_freq", first_cpu);
	ifile.open(filename, ios::in);
	ifile >> minf;
	ifile.close();

	ofile.open(filename, ios::out);
	ofile << maxf;
	ofile.close();
	ofile.open(filename, ios::out);
	ofile << minf;
	ofile.close();
	snprintf(filename, PATH_MAX, "/sys/devices/system/cpu/cpu%i/cpufreq/scaling_max_freq", first_cpu);
	ofile.open(filename, ios::out);
	ofile << minf;
	ofile.close();
	ofile.open(filename, ios::out);
	ofile << maxf;
	ofile.close();

}
uint64_t abstract_cpu::total_pstate_time(void)
{
	unsigned int i;
	uint64_t stamp = 0;

	for (i = 0; i < pstates.size(); i++)
		stamp += pstates[i]->time_after;

	return stamp;
}


void abstract_cpu::validate(void)
{
	unsigned int i;

	for (i = 0; i < children.size(); i++) {
		if (children[i])
			children[i]->validate();
	}
}

void abstract_cpu::reset_pstate_data(void)
{
	unsigned int i;

	for (i = 0; i < pstates.size(); i++) {
		pstates[i]->time_before = 0;
		pstates[i]->time_after = 0;
	}
	for (i = 0; i < cstates.size(); i++) {
		cstates[i]->duration_before = 0;
		cstates[i]->duration_after = 0;
		cstates[i]->before_count = 0;
		cstates[i]->after_count = 0;
	}

	for (i = 0; i < children.size(); i++)
		if (children[i])
			children[i]->reset_pstate_data();
}
