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
