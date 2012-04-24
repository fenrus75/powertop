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
#ifndef _INCLUDE_GUARD_TIMER_H
#define _INCLUDE_GUARD_TIMER_H

#include <stdint.h>

#include "powerconsumer.h"

class timer : public power_consumer {
	char desc[256];
public:
	char		handler[32];
	int		raw_count;
	bool		deferred;

	timer(unsigned long timer_func);

	void fire(uint64_t time, uint64_t timer_struct);
	uint64_t done(uint64_t time, uint64_t timer_struct);
	bool is_deferred(void);

	virtual const char * description(void);
	virtual const char * name(void) { return "timer"; };
	virtual const char * type(void) { return "Timer"; };
	virtual double usage_summary(void);
	virtual const char * usage_units_summary(void);

};

class timer_list {
public:
	uint64_t	timer_address;
	uint64_t	timer_func;
};


extern void all_timers_to_all_power(void);
extern class timer * find_create_timer(uint64_t func);
extern void clear_timers(void);

#endif
