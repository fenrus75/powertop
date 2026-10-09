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

#include <vector>
#include <cstdint>

#include "tunable.h"

class ethernet_tunable : public tunable {
public:
	std::string interf;
	explicit ethernet_tunable(const std::string &iface);

	int good_bad(void) override;

	void toggle(void) override;

	void collect_json_fields(std::string &_js) override;

	/* Virtual hardware helpers — override in tests to inject fake data */
	virtual int  get_wol(uint32_t &wolopts) const;
	virtual void set_wol(uint32_t wolopts);
};

extern void add_ethernet_tunable(void);

