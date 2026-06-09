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
#include <sstream>
#include <inttypes.h>
#include <cstdio>
#include <cstdlib>
#include <climits>
#include "cpu.h"
#include "../lib.h"

abstract_cpu::~abstract_cpu() = default;

void abstract_cpu::account_freq(uint64_t freq, uint64_t duration)
{
	class frequency *state = nullptr;
	unsigned int i;

	for (i = 0; i < pstates.size(); i++) {
		if (freq == pstates[i]->freq) {
			state = pstates[i].get();
			break;
		}
	}


	if (!state) {
		auto &uptr = pstates.emplace_back(std::make_unique<frequency>());
		state = uptr.get();

		state->freq = freq;
		state->human_name = hz_to_human(freq);
		if (freq == 0)
			state->human_name = _("Idle");
		if (is_turbo(freq, max_frequency, max_minus_one_frequency))
			state->human_name = _("Turbo Mode");

		state->after_count = 1;
	}


	state->time_after += duration;


}

void abstract_cpu::freq_updated(uint64_t time)
{
	if (parent)
		parent->calculate_freq(time);
	old_idle = idle;
}

void abstract_cpu::measurement_start(void)
{
	std::string filename;

	last_stamp = 0;

	cstates.clear();

	pstates.clear();

	current_frequency = 0;
	idle = false;
	old_idle = true;


	filename = std::format("/sys/devices/system/cpu/cpu{}/cpufreq/scaling_available_frequencies", number);
	const std::string content = read_file_content(filename);
	if (!content.empty()) {
		std::istringstream stream(content);
		stream >> max_frequency;
		stream >> max_minus_one_frequency;
	}

	for (auto &child : children)
		if (child)
			child->measurement_start();

	stamp_before = pt_gettime();

	last_stamp = 0;

	for (auto &child : children)
		if (child)
			child->wiggle();

}

void abstract_cpu::measurement_end(void)
{
	total_stamp = 0;
	stamp_after = pt_gettime();
	for (auto &child : children)
		if (child)
			child->wiggle();

	time_factor = 1000000.0 * (stamp_after.tv_sec - stamp_before.tv_sec) + stamp_after.tv_usec - stamp_before.tv_usec;


	for (auto &child : children)
		if (child)
			child->measurement_end();

	for (auto &child : children)
		if (child) {
			for (const auto &state : child->cstates) {

				update_cstate( state->linux_name, state->human_name, state->usage_before, state->duration_before, state->before_count);
				finalize_cstate(state->linux_name,                   state->usage_after,  state->duration_after,  state->after_count);
			}
			for (const auto &state : child->pstates) {

				update_pstate(  state->freq, state->human_name, state->time_before, state->before_count);
				finalize_pstate(state->freq,                    state->time_after,  state->after_count);
			}
		}

	for (const auto &state : cstates) {
		if (state->after_count == 0)
			continue;

		if (state->after_count != state->before_count)
			continue;

		state->usage_delta =    (state->usage_after    - state->usage_before)    / state->after_count;
		state->duration_delta = (state->duration_after - state->duration_before) / state->after_count;
	}
}

void abstract_cpu::insert_cstate(const std::string &linux_name, const std::string &human_name, uint64_t usage, uint64_t duration, int count, int level)
{
	cstates.push_back(std::make_unique<idle_state>());
	struct idle_state *state = cstates.back().get();

	state->usage_before = 0;
	state->usage_after = 0;
	state->usage_delta = 0;
	state->duration_before = 0;
	state->duration_after = 0;
	state->duration_delta = 0;
	state->before_count = 0;
	state->after_count = 0;

	state->linux_name = linux_name;
	state->human_name = human_name;

	state->line_level = -1;

	if (state->linux_name == "active") {
		state->line_level = LEVEL_C0;
	} else {
		for (size_t i = 0; i < human_name.length(); i++) {
			char c = human_name[i];
			if (c >= '0' && c <= '9') {
				try {
					state->line_level = std::stoi(human_name.substr(i));
				} catch (...) {}

				if (i + 1 < human_name.length() && human_name[i+1] != '-') {
					int greater_line_level = state->line_level;
					for (unsigned int pos = 0; pos < cstates.size(); pos++){
						if (cstates[pos]->human_name.length() > 2) {
							if (c == cstates[pos]->human_name[1]){
								if (i + 1 < human_name.length() && human_name[i+1] != cstates[pos]->human_name[2]){
									greater_line_level = std::max(greater_line_level, cstates[pos]->line_level);
									state->line_level = greater_line_level + 1;
								}
							}
						}
					}
				}
				break;
			}
		}
	}

	/* some architectures (ARM) don't have good numbers in their human name.. fall back to the linux name for those */
	if (state->line_level < 0) {
		for (size_t i = 0; i < linux_name.length(); i++) {
			char c = linux_name[i];
			if (c >= '0' && c <= '9') {
				try {
					state->line_level = std::stoi(linux_name.substr(i));
				} catch (...) {}
				break;
			}
		}
	}

	if (level >= 0)
		state->line_level = level;

	state->usage_before = usage;
	state->duration_before = duration;
	state->before_count = count;
}

void abstract_cpu::finalize_cstate(const std::string &linux_name, uint64_t usage, uint64_t duration, int count)
{
	unsigned int i;
	struct idle_state *state = nullptr;

	for (i = 0; i < cstates.size(); i++) {
		if (cstates[i]->linux_name == linux_name) {
			state = cstates[i].get();
			break;
		}
	}

	if (!state) {
		fprintf(stderr, _("Invalid C state finalize %s\n"), linux_name.c_str());
		return;
	}

	state->usage_after += usage;
	state->duration_after += duration;
	state->after_count += count;
}

void abstract_cpu::update_cstate(const std::string &linux_name, const std::string &human_name, uint64_t usage, uint64_t duration, int count, int level)
{
	unsigned int i;
	struct idle_state *state = nullptr;

	for (i = 0; i < cstates.size(); i++) {
		if (cstates[i]->linux_name == linux_name) {
			state = cstates[i].get();
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

int abstract_cpu::has_cstate_level(int level) const
{
	if (level == LEVEL_HEADER)
		return 1;

	for (const auto &s : cstates)
		if (s->line_level == level)
			return 1;

	for (auto &child : children)
		if (child)
			if (child->has_cstate_level(level))
				return 1;
	return  0;
}

int abstract_cpu::has_pstate_level(int level) const
{
	if (level == LEVEL_HEADER)
		return 1;

	if (level >= 0 && level < (int)pstates.size())
		return 1;

	for (auto &child : children)
		if (child)
			if (child->has_pstate_level(level))
				return 1;
	return  0;
}



void abstract_cpu::insert_pstate(uint64_t freq, const std::string &human_name, uint64_t duration, int count)
{
	pstates.push_back(std::make_unique<frequency>());
	class frequency *state = pstates.back().get();

	state->freq = freq;
	state->human_name = human_name;


	state->time_before = duration;
	state->before_count = count;
}

void abstract_cpu::finalize_pstate(uint64_t freq, uint64_t duration, int count)
{
	unsigned int i;
	class frequency *state = nullptr;

	for (i = 0; i < pstates.size(); i++) {
		if (freq == pstates[i]->freq) {
			state = pstates[i].get();
			break;
		}
	}

	if (!state) {
		fprintf(stderr, _("Invalid P state finalize %" PRIu64 "\n"), freq);
		return;
	}
	state->time_after += duration;
	state->after_count += count;

}

void abstract_cpu::update_pstate(uint64_t freq, const std::string &human_name, uint64_t duration, int count)
{
	unsigned int i;
	class frequency *state = nullptr;

	for (i = 0; i < pstates.size(); i++) {
		if (freq == pstates[i]->freq) {
			state = pstates[i].get();
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

	/* calculate the maximum frequency of all children */
	for (auto &child : children)
		if (child && child->has_pstates()) {
			uint64_t f = 0;
			if (!child->idle) {
				f = child->current_frequency;
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
	for (auto &child : children)
		if (child)
			child->change_effective_frequency(time, frequency);
}


void abstract_cpu::wiggle(void)
{
	uint64_t minf, maxf;
	uint64_t setspeed = 0;

	/* wiggle a CPU so that we have a record of it at the start and end of the perf trace */

	maxf = read_sysfs(std::format("/sys/devices/system/cpu/cpu{}/cpufreq/scaling_max_freq", first_cpu));
	minf = read_sysfs(std::format("/sys/devices/system/cpu/cpu{}/cpufreq/scaling_min_freq", first_cpu));

	/* In case of the userspace governor, remember the old setspeed setting, it will be affected by wiggle */
	/* Note that non-userspace governors report "<unsupported>". In that case read_sysfs will return 0 */
	setspeed = read_sysfs(std::format("/sys/devices/system/cpu/cpu{}/cpufreq/scaling_setspeed", first_cpu));

	write_sysfs(std::format("/sys/devices/system/cpu/cpu{}/cpufreq/scaling_setspeed", first_cpu), std::to_string(maxf));
	write_sysfs(std::format("/sys/devices/system/cpu/cpu{}/cpufreq/scaling_min_freq", first_cpu), std::to_string(minf));
	write_sysfs(std::format("/sys/devices/system/cpu/cpu{}/cpufreq/scaling_max_freq", first_cpu), std::to_string(minf));
	write_sysfs(std::format("/sys/devices/system/cpu/cpu{}/cpufreq/scaling_max_freq", first_cpu), std::to_string(maxf));

	if (setspeed != 0) {
		write_sysfs(std::format("/sys/devices/system/cpu/cpu{}/cpufreq/scaling_setspeed", first_cpu), std::to_string(setspeed));
	}
}
uint64_t abstract_cpu::total_pstate_time(void) const
{
	uint64_t stamp = 0;

	for (const auto &s : pstates)
		stamp += s->time_after;

	return stamp;
}


void abstract_cpu::validate(void)
{
	for (auto &child : children)
		if (child)
			child->validate();
}

void abstract_cpu::reset_pstate_data(void)
{
	for (auto &s : pstates) {
		s->time_before = 0;
		s->time_after = 0;
	}
	for (auto &s : cstates) {
		s->duration_before = 0;
		s->duration_after = 0;
		s->before_count = 0;
		s->after_count = 0;
	}

	for (auto &child : children)
		if (child)
			child->reset_pstate_data();
}

void abstract_cpu::collect_json_fields(std::string &_js)
{
	JSON_KV("type", get_type());
	JSON_FIELD(number);
	JSON_FIELD(first_cpu);
	JSON_FIELD(idle);
	JSON_FIELD(has_intel_MSR);
	JSON_FIELD(current_frequency);
	JSON_FIELD(effective_frequency);
	JSON_ARRAY("cstates", cstates);
	JSON_ARRAY("pstates", pstates);
	JSON_ARRAY("children", children);
}
