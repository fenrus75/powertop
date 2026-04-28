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

#include <format>

std::string cpu_core::fill_cstate_line(int line_nr, const std::string &[[maybe_unused]] separator)
{
	unsigned int i;

	if (line_nr == LEVEL_HEADER)
		return this->has_intel_MSR ? _(" Core(HW)"): _(" Core(OS)");

	for (i = 0; i < cstates.size(); i++) {
		if (cstates[i]->line_level != line_nr)
			continue;
		return std::format("{:5.1f}%", percentage(cstates[i]->duration_delta / time_factor));
	}

	return "";
}


std::string cpu_core::fill_cstate_name(int line_nr)
{
	unsigned int i;

	for (i = 0; i < cstates.size(); i++) {
		if (cstates[i]->line_level != line_nr)
			continue;

		return cstates[i]->human_name;
	}

	return "";
}



std::string cpu_core::fill_pstate_name(int line_nr)
{
	if (line_nr >= (int)pstates.size() || line_nr < 0)
		return "";

	return pstates[line_nr]->human_name;
}

std::string cpu_core::fill_pstate_line(int line_nr)
{
	unsigned int i;

	if (total_stamp ==0) {
		for (i = 0; i < pstates.size(); i++)
			total_stamp += pstates[i]->time_after;
		if (total_stamp == 0)
			total_stamp = 1;
	}

	if (line_nr == LEVEL_HEADER) {
		return _("  Core");
	}

	if (line_nr >= (int)pstates.size() || line_nr < 0)
		return "";

	return std::format(" {:5.1f}% ", percentage(1.0* (pstates[line_nr]->time_after) / total_stamp));
}
