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

#include <stdint.h>

#include "powerconsumer.h"

#ifdef __x86_64__
#define BIT64 1
#endif

/*
Need to collect
 * CPU time consumed by each application
 * Number of wakeups out of idle -- received and wakeups sent
 * Number of disk dirties (inode for now)
 */
class process : public power_consumer {
	uint64_t	running_since = 0;
public:
	std::string	desc;
	int		tgid = 0;
	std::string	comm;
	int		pid = 0;


	int		is_idle = 0;   /* count this as if the cpu was idle */
	int		running = 0;
	int		is_kernel = 0; /* kernel thread */

	process(const std::string &_comm, int _pid, int _tid = 0);

	virtual void schedule_thread(uint64_t time, int thread_id);
	virtual uint64_t deschedule_thread(uint64_t time, int thread_id = 0);

	virtual void account_disk_dirty(void);

	virtual std::string description(void) override;
	virtual std::string name(void) override { return "process"; };
	virtual std::string type(void) override { return "Process"; };

	virtual double usage_summary(void) override;
	virtual std::string usage_units_summary(void) override;
	void collect_json_fields(std::string &_js) override;

};

extern std::vector <class process *> all_processes;

extern double measurement_time;


extern void start_process_measurement(void);
extern void end_process_measurement(void);
extern void process_process_data(void);
extern void end_process_data(void);
extern void clear_process_data(void);
extern void merge_processes(void);

extern class process * find_create_process(const std::string &comm, int pid);
extern void all_processes_to_all_power(void);

extern void clear_processes(void);
extern void process_update_display(void);
extern void report_process_update_display(void);
extern void report_summary(void);



extern void clear_timers(void);

