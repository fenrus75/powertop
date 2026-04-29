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

#include <iostream>
#include <vector>
#include <string>
#include <stdint.h>
#include <sys/time.h>
#include "../lib.h"
#include "frequency.h"

class abstract_cpu
{
protected:
	int	first_cpu = 0;
	struct timeval	stamp_before, stamp_after;
	double  time_factor = 0.0;
	uint64_t max_frequency = 0;
	uint64_t max_minus_one_frequency = 0;

	virtual void	account_freq(uint64_t frequency, uint64_t duration);
	virtual void	freq_updated(uint64_t time);

public:
	uint64_t	last_stamp = 0;
	uint64_t	total_stamp = 0;
	int	number = 0;
	int	childcount = 0;
	std::string    name;
	bool	idle = false, old_idle = false, has_intel_MSR = false;
	uint64_t	current_frequency = 0;
	uint64_t	effective_frequency = 0;

	std::vector<class abstract_cpu *> children;
	std::vector<struct idle_state *> cstates;
	std::vector<class frequency *> pstates;

	virtual ~abstract_cpu();

	class abstract_cpu *parent = nullptr;


	int	get_first_cpu() { return first_cpu; }
	void	set_number(int _number, int cpu) {this->number = _number; this->first_cpu = cpu;};
	void	set_intel_MSR(bool _bool_value) {this->has_intel_MSR =  _bool_value;};
	void	set_type(const std::string& _name) {this->name = _name;};
	int	get_number(void) { return number; };
	std::string get_type(void) { return name; };

	virtual void	measurement_start(void);
	virtual void	measurement_end(void);

	virtual int     can_collapse(void) { return 0;};


	/* C state related methods */

	void		insert_cstate(const std::string &linux_name, const std::string &human_name, uint64_t usage, uint64_t duration, int count, int level = -1);
	void		update_cstate(const std::string &linux_name, const std::string &human_name, uint64_t usage, uint64_t duration, int count, int level = -1);
	void		finalize_cstate(const std::string &linux_name, uint64_t usage, uint64_t duration, int count);

	virtual int	has_cstate_level(int level);

	virtual std::string  fill_cstate_line([[maybe_unused]] int line_nr, [[maybe_unused]] const std::string &separator ="") { return "";};
	virtual std::string  fill_cstate_percentage([[maybe_unused]] int line_nr) { return ""; };
	virtual std::string  fill_cstate_time([[maybe_unused]] int line_nr) { return ""; };
	virtual std::string  fill_cstate_name([[maybe_unused]] int line_nr) { return "";};


	/* P state related methods */
	void		insert_pstate(uint64_t freq, const std::string &human_name, uint64_t duration, int count);
	void		update_pstate(uint64_t freq, const std::string &human_name, uint64_t duration, int count);
	void		finalize_pstate(uint64_t freq, uint64_t duration, int count);


	virtual std::string  fill_pstate_line([[maybe_unused]] int line_nr) { return "";};
	virtual std::string  fill_pstate_name([[maybe_unused]] int line_nr) { return "";};
	virtual int	has_pstate_level(int level);
	virtual int	has_pstates(void) { return 1; };

	/* Frequency micro accounting methods */
	virtual void    calculate_freq(uint64_t time);
	virtual void    go_idle(uint64_t time) { idle = true; freq_updated(time); }
	virtual void    go_unidle(uint64_t time) { idle = false; freq_updated(time); }
	virtual void    change_freq(uint64_t time, int freq) { current_frequency = freq; freq_updated(time); }

	virtual void	change_effective_frequency(uint64_t time, uint64_t freq);

	virtual void    wiggle(void);

	virtual uint64_t total_pstate_time(void);

	virtual void validate(void);
	virtual void reset_pstate_data(void);

	virtual void collect_json_fields(std::string &_js);
	std::string serialize() { JSON_START(); collect_json_fields(_js); JSON_END(); }
};

extern std::vector<class abstract_cpu *> all_cpus;

class cpu_linux: public abstract_cpu
{
	void 	parse_pstates_start(void);
	void 	parse_cstates_start(void);
	void 	parse_pstates_end(void);
	void 	parse_cstates_end(void);

public:
	virtual void	measurement_start(void) override;
	virtual void	measurement_end(void) override;

	virtual std::string  fill_cstate_line(int line_nr, const std::string &separator="") override;
	virtual std::string  fill_cstate_name(int line_nr) override;
	virtual std::string  fill_cstate_percentage(int line_nr) override;
	virtual std::string  fill_cstate_time(int line_nr) override;

	virtual std::string  fill_pstate_line(int line_nr) override;
	virtual std::string  fill_pstate_name(int line_nr) override;
};

class cpu_core: public abstract_cpu
{
public:
	virtual std::string  fill_cstate_line(int line_nr, const std::string &separator="") override;
	virtual std::string  fill_cstate_name(int line_nr) override;

	virtual std::string  fill_pstate_line(int line_nr) override;
	virtual std::string  fill_pstate_name(int line_nr) override;

	virtual int     can_collapse(void) override { return childcount == 1;};
};

class cpu_package: public abstract_cpu
{
protected:
	virtual void	freq_updated(uint64_t time) override;
public:
	virtual std::string  fill_cstate_line(int line_nr, const std::string &separator="") override;
	virtual std::string  fill_cstate_name(int line_nr) override;

	virtual std::string  fill_pstate_line(int line_nr) override;
	virtual std::string  fill_pstate_name(int line_nr) override;
	virtual int     can_collapse(void) override { return childcount == 1;};
};

extern void enumerate_cpus(void);

extern void report_display_cpu_pstates(void);
extern void report_display_cpu_cstates(void);


extern void w_display_cpu_cstates(void);
extern void w_display_cpu_pstates(void);


extern void start_cpu_measurement(void);
extern void end_cpu_measurement(void);
extern void process_cpu_data(void);
extern void end_cpu_data(void);
extern void clear_cpu_data(void);
extern void clear_all_cpus(void);

