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
#ifndef _INCLUDE_GUARD_INTERRUPT_H
#define _INCLUDE_GUARD_INTERRUPT_H

#include <stdint.h>

#include "powerconsumer.h"

class interrupt : public power_consumer {
	uint64_t	running_since;
	char		desc[256];
public:
	char		handler[32];
	int		number;

	int		raw_count;

	interrupt(const char *_handler, int _number);

	virtual void start_interrupt(uint64_t time);
	virtual uint64_t end_interrupt(uint64_t time);

	virtual const char * description(void);

	virtual const char * name(void) { return "interrupt"; };
	virtual const char * type(void) { return "Interrupt"; };
	virtual double usage_summary(void);
	virtual const char * usage_units_summary(void);
};

extern vector <class interrupt *> all_interrupts;
extern const char* softirqs[];


extern class interrupt * find_create_interrupt(const char *_handler, int nr, int cpu);
extern void all_interrupts_to_all_power(void);
extern void clear_interrupts(void);

#endif
