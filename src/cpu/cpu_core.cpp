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
#include <stdio.h>
#include "cpu.h"
#include "../lib.h"

#include "../parameters/parameters.h"

static int is_turbo(uint64_t freq, uint64_t max, uint64_t maxmo)
{
	if (freq != max)
		return 0;
	if (maxmo + 1000 != max)
		return 0;
	return 1;
}

char * cpu_core::fill_cstate_line(int line_nr, char *buffer, const char *separator)
{
	unsigned int i;
	buffer[0] = 0;

	if (line_nr == LEVEL_HEADER) {
		sprintf(buffer,_("  Core"));
		return buffer;
	}

	for (i = 0; i < cstates.size(); i++) {
		if (cstates[i]->line_level != line_nr)
			continue;
		sprintf(buffer,"%5.1f%%", percentage(cstates[i]->duration_delta / time_factor));
	}

	return buffer;
}


char * cpu_core::fill_cstate_name(int line_nr, char *buffer)
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



char * cpu_core::fill_pstate_name(int line_nr, char *buffer)
{
	buffer[0] = 0;

	if (line_nr >= (int)pstates.size() || line_nr < 0)
		return buffer;

	sprintf(buffer,"%s", pstates[line_nr]->human_name);

	return buffer;
}


void cpu_core::account_freq(uint64_t freq, uint64_t duration)
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


void cpu_core::calculate_freq(uint64_t time)
{
	uint64_t freq = 0;
	bool is_idle = true;
	unsigned int i;


	/* calculate the maximum frequency of all children */
	for (i = 0; i < children.size(); i++)
		if (children[i]) {
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
	if (parent)
		parent->calculate_freq(time);
	old_idle = idle;
}

void cpu_core::change_effective_frequency(uint64_t time, uint64_t frequency)
{
	uint64_t freq = 0;
	uint64_t time_delta, fr;


	if (last_stamp)
		time_delta = time - last_stamp;
	else
		time_delta = 1;

	fr = effective_frequency;

	if (old_idle)
		fr = 0;

	account_freq(fr, time_delta);

	effective_frequency = freq;
	last_stamp = time;
	abstract_cpu::change_effective_frequency(time, frequency);
}

char * cpu_core::fill_pstate_line(int line_nr, char *buffer)
{
	buffer[0] = 0;
	unsigned int i;

	if (total_stamp ==0) {
		for (i = 0; i < pstates.size(); i++)
			total_stamp += pstates[i]->time_after;
		if (total_stamp == 0)
			total_stamp = 1;
	}

	if (line_nr == LEVEL_HEADER) {
		sprintf(buffer,_("  Core"));
		return buffer;
	}

	if (line_nr >= (int)pstates.size() || line_nr < 0)
		return buffer;


	sprintf(buffer," %5.1f%% ", percentage(1.0* (pstates[line_nr]->time_after) / total_stamp));
	return buffer;
}
