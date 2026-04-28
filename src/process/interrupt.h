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

class interrupt : public power_consumer {
	uint64_t	running_since = 0;
	std::string	desc;
public:
	std::string	handler;
	int		number = 0;

	int		raw_count = 0;

	interrupt(const std::string &_handler, int _number);

	virtual void start_interrupt(uint64_t time);
	virtual uint64_t end_interrupt(uint64_t time);

	virtual std::string description(void) override;

	virtual std::string name(void) override { return "interrupt"; };
	virtual std::string type(void) override { return "Interrupt"; };
	virtual double usage_summary(void) override;
	virtual std::string usage_units_summary(void) override;
};

extern std::vector <class interrupt *> all_interrupts;
extern const std::vector<std::string> softirqs;


extern class interrupt * find_create_interrupt(const std::string &_handler, int nr, int cpu);
extern void all_interrupts_to_all_power(void);
extern void clear_interrupts(void);

