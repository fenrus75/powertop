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
#include <sstream>

#include "cpu.h"
#include "../lib.h"

#include <cstdlib>

#include <unistd.h>
#include <cstdio>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <format>

void cpu_linux::parse_cstates_start(void)
{
	const std::string dir = std::format("/sys/devices/system/cpu/cpu{}/cpuidle", number);

	for (const auto &linux_name : list_directory(dir)) {
		std::string human_name = read_sysfs_string(std::format("{}/{}/name", dir, linux_name));
		if (human_name.empty())
			human_name = linux_name;

		if (human_name == "C0")
			human_name = _("C0 polling");

		bool ok = false;
		const uint64_t usage    = read_sysfs(std::format("{}/{}/usage", dir, linux_name), &ok);
		if (!ok)
			continue;
		const uint64_t duration = read_sysfs(std::format("{}/{}/time", dir, linux_name));

		update_cstate(linux_name, human_name, usage, duration, 1);
	}
}


void cpu_linux::parse_pstates_start(void)
{
	std::string filename;

	last_stamp = 0;
	for (auto &child : children)
		if (child)
			child->wiggle();

	filename = std::format("/sys/devices/system/cpu/cpu{}/cpufreq/stats/time_in_state", first_cpu);

	const std::string content = read_file_content(filename);
	if (!content.empty()) {
		std::istringstream stream(content);
		std::string line;
		while (std::getline(stream, line)) {
			uint64_t f;
			std::istringstream iss(line);
			if (iss >> f)
				account_freq(f, 0);
		}
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
	const std::string dir = std::format("/sys/devices/system/cpu/cpu{}/cpuidle", number);

	for (const auto &linux_name : list_directory(dir)) {
		bool ok = false;
		const uint64_t usage    = read_sysfs(std::format("{}/{}/usage", dir, linux_name), &ok);
		if (!ok)
			continue;
		const uint64_t duration = read_sysfs(std::format("{}/{}/time", dir, linux_name));

		finalize_cstate(linux_name, usage, duration, 1);
	}
}

void cpu_linux::parse_pstates_end(void)
{
	std::string filename;

	filename = std::format("/sys/devices/system/cpu/cpu{}/cpufreq/stats/time_in_state", number);

	const std::string content = read_file_content(filename);
	if (!content.empty()) {
		std::istringstream stream(content);
		std::string line;

		while (std::getline(stream, line)) {
			uint64_t f, count;
			std::istringstream iss(line);

			if (iss >> f >> count) {
				if (f > 0)
					finalize_pstate(f, count, 1);
			}
		}
	}
}

void cpu_linux::measurement_end(void)
{
	parse_cstates_end();
	parse_pstates_end();
	abstract_cpu::measurement_end();
}
#include <format>

std::string cpu_linux::fill_cstate_line(int line_nr, const std::string &separator)
{
	if (line_nr == LEVEL_HEADER) {
		return pt_format(_(" CPU(OS) {}"), number);
	}

	if (time_factor < 1.0)
		return "";

	for (const auto &s : cstates) {
		if (s->line_level != line_nr)
			continue;

		if (line_nr == LEVEL_C0)
			/* Pad to the same width as other cstate lines so TUI columns align */
			return std::format("{:5.1f}%          ", percentage(s->duration_delta / time_factor));
		else
			return std::format("{:5.1f}%{} {:6.1f} ms",
				percentage(s->duration_delta / time_factor),
				separator,
				1.0 * s->duration_delta / (1 + s->usage_delta) / 1000);
	}
	return "";
}

std::string cpu_linux::fill_cstate_percentage(int line_nr)
{
	if (time_factor < 1.0)
		return "";

	for (const auto &s : cstates) {
		if (s->line_level != line_nr)
			continue;

		return std::format("{:5.1f}%",
			percentage(s->duration_delta / time_factor));
	}

	return "";
}

std::string cpu_linux::fill_cstate_time(int line_nr)
{
	if (line_nr == LEVEL_C0)
		return "";

	for (const auto &s : cstates) {
		if (s->line_level != line_nr)
			continue;

		return std::format("{:6.1f} ms",
			1.0 * s->duration_delta /
			(1 + s->usage_delta) / 1000);
	}

	return "";
}

std::string cpu_linux::fill_cstate_name(int line_nr)
{
	for (const auto &s : cstates) {
		if (s->line_level != line_nr)
			continue;

		return std::format("{}", s->human_name);
	}

	return "";
}


std::string cpu_linux::fill_pstate_name(int line_nr)
{
	if (line_nr >= (int)pstates.size() || line_nr < 0)
		return "";

	return std::format("{}", pstates[line_nr]->human_name);
}

std::string cpu_linux::fill_pstate_line(int line_nr)
{
	if (total_stamp ==0) {
		for (const auto &s : pstates)
			total_stamp += s->time_after;
		if (total_stamp == 0)
			total_stamp = 1;
	}

	if (line_nr == LEVEL_HEADER) {
		return pt_format(_(" CPU {}"), number);
	}

	if (line_nr >= (int)pstates.size() || line_nr < 0)
		return "";

	return std::format(" {:5.1f}% ", percentage(1.0 * (pstates[line_nr]->time_after) / total_stamp));
}
