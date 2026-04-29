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
#pragma once

#include <string>
#include <stdint.h>
#include "../lib.h"

#define LEVEL_C0 -1
#define LEVEL_HEADER -2

#define PSTATE 1
#define CSTATE 2

struct idle_state {
	std::string linux_name; /* state0 etc.. cpuidle name */
	std::string human_name;

	uint64_t usage_before = 0;
	uint64_t usage_after = 0;
	uint64_t usage_delta = 0;

	uint64_t duration_before = 0;
	uint64_t duration_after = 0;
	uint64_t duration_delta = 0;

	int before_count = 0;
	int after_count = 0;

	int line_level = 0;

	std::string serialize() {
		JSON_START();
		JSON_FIELD(linux_name); JSON_FIELD(human_name);
		JSON_FIELD(usage_before); JSON_FIELD(usage_after); JSON_FIELD(usage_delta);
		JSON_FIELD(duration_before); JSON_FIELD(duration_after); JSON_FIELD(duration_delta);
		JSON_FIELD(before_count); JSON_FIELD(after_count);
		JSON_FIELD(line_level);
		JSON_END();
	}
};

class frequency {
public:
	frequency(void);
	std::string human_name = "";
	int line_level = 0;

	uint64_t freq = 0;

	uint64_t time_after = 0;
	uint64_t time_before = 0;

	int before_count = 0;
	int after_count = 0;

	double   display_value = 0.0;

	std::string serialize() {
		JSON_START();
		JSON_FIELD(human_name); JSON_FIELD(line_level);
		JSON_FIELD(freq);
		JSON_FIELD(time_after); JSON_FIELD(time_before);
		JSON_FIELD(before_count); JSON_FIELD(after_count);
		JSON_FIELD(display_value);
		JSON_END();
	}
};
