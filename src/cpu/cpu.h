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

#ifndef __INCLUDE_GUARD_CPUDEV_H
#define __INCLUDE_GUARD_CPUDEV_H

#include <iostream>
#include <vector>
#include <string>
#include <stdint.h>
#include <sys/time.h>

using namespace std;

class abstract_cpu;

#define LEVEL_C0 -1
#define LEVEL_HEADER -2

#define PSTATE 1
#define CSTATE 2

struct idle_state {
	char linux_name[16]; /* state0 etc.. cpuidle name */
	char human_name[32];

	uint64_t usage_before;
	uint64_t usage_after;
	uint64_t usage_delta;

	uint64_t duration_before;
	uint64_t duration_after;
	uint64_t duration_delta;

	int before_count;
	int after_count;

	int line_level;
};

struct frequency {
	char human_name[32];
	int line_level;

	uint64_t freq;

	uint64_t time_after;
	uint64_t time_before;

	int before_count;
	int after_count;

	double   display_value;
};

class abstract_cpu
{
protected:
	int	first_cpu;
	struct timeval	stamp_before, stamp_after;
	double  time_factor;
	uint64_t max_frequency;
	uint64_t max_minus_one_frequency;

	virtual void	account_freq(uint64_t frequency, uint64_t duration);
	virtual void	freq_updated(uint64_t time);
public:
	uint64_t	last_stamp;
	uint64_t	total_stamp;
	int	number;
	int	childcount;
	const char*    name;
	bool	idle, old_idle;
	uint64_t	current_frequency;
	uint64_t	effective_frequency;

	vector<class abstract_cpu *> children;
	vector<struct idle_state *> cstates;
	vector<struct frequency *> pstates;

	virtual ~abstract_cpu() {};

	class abstract_cpu *parent;


	int	get_first_cpu() { return first_cpu; }
	void	set_number(int _number, int cpu) {this->number = _number; this->first_cpu = cpu;};
	void	set_type(const char* _name) {this->name = _name;};
	int	get_number(void) { return number; };
	const char* get_type(void) { return name; };

	virtual void	measurement_start(void);
	virtual void	measurement_end(void);

	virtual int     can_collapse(void) { return 0;};


	/* C state related methods */

	void		insert_cstate(const char *linux_name, const char *human_name, uint64_t usage, uint64_t duration, int count, int level = -1);
	void		update_cstate(const char *linux_name, const char *human_name, uint64_t usage, uint64_t duration, int count, int level = -1);
	void		finalize_cstate(const char *linux_name, uint64_t usage, uint64_t duration, int count);

	virtual int	has_cstate_level(int level);

	virtual char *  fill_cstate_line(int line_nr, char *buffer, const char *separator="") { return buffer;};
	virtual char *  fill_cstate_percentage(int line_nr, char *buffer) { return buffer; };
	virtual char *  fill_cstate_time(int line_nr, char *buffer) { return buffer; };
	virtual char *  fill_cstate_name(int line_nr, char *buffer) { return buffer;};


	/* P state related methods */
	void		insert_pstate(uint64_t freq, const char *human_name, uint64_t duration, int count);
	void		update_pstate(uint64_t freq, const char *human_name, uint64_t duration, int count);
	void		finalize_pstate(uint64_t freq, uint64_t duration, int count);


	virtual char *  fill_pstate_line(int line_nr, char *buffer) { return buffer;};
	virtual char *  fill_pstate_name(int line_nr, char *buffer) { return buffer;};
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
};

extern vector<class abstract_cpu *> all_cpus;

class cpu_linux: public abstract_cpu
{
	void 	parse_pstates_start(void);
	void 	parse_cstates_start(void);
	void 	parse_pstates_end(void);
	void 	parse_cstates_end(void);

public:
	virtual void	measurement_start(void);
	virtual void	measurement_end(void);

	virtual char *  fill_cstate_line(int line_nr, char *buffer, const char *separator="");
	virtual char *  fill_cstate_name(int line_nr, char *buffer);
	virtual char *  fill_cstate_percentage(int line_nr, char *buffer);
	virtual char *  fill_cstate_time(int line_nr, char *buffer);

	virtual char *  fill_pstate_line(int line_nr, char *buffer);
	virtual char *  fill_pstate_name(int line_nr, char *buffer);
};

class cpu_core: public abstract_cpu
{
public:
	virtual char *  fill_cstate_line(int line_nr, char *buffer, const char *separator="");
	virtual char *  fill_cstate_name(int line_nr, char *buffer);

	virtual char *  fill_pstate_line(int line_nr, char *buffer);
	virtual char *  fill_pstate_name(int line_nr, char *buffer);

	virtual int     can_collapse(void) { return childcount == 1;};
};

class cpu_package: public abstract_cpu
{
protected:
	virtual void	freq_updated(uint64_t time);
public:
	virtual char *  fill_cstate_line(int line_nr, char *buffer, const char *separator="");
	virtual char *  fill_cstate_name(int line_nr, char *buffer);

	virtual char *  fill_pstate_line(int line_nr, char *buffer);
	virtual char *  fill_pstate_name(int line_nr, char *buffer);
	virtual int     can_collapse(void) { return childcount == 1;};
};

extern void enumerate_cpus(void);

extern void report_display_cpu_pstates(void);
extern void report_display_cpu_cstates(void);



extern void display_cpu_cstates(const char *start= "",
				const char *end = "",
				const char *linestart = "",
				const char *separator = "| ",
				const char *lineend = "\n");

extern void w_display_cpu_cstates(void);
extern void w_display_cpu_pstates(void);


extern void start_cpu_measurement(void);
extern void end_cpu_measurement(void);
extern void process_cpu_data(void);
extern void end_cpu_data(void);
extern void clear_cpu_data(void);
extern void clear_all_cpus(void);

#endif
