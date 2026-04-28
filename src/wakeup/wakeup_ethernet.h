/*
 * Copyright 2018, Intel Corporation
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
 *      Gayatri Kammela <gayatri.kammela@intel.com>
 */
#pragma once

#include <vector>

#include "wakeup.h"

class ethernet_wakeup : public wakeup {
	std::string eth_path;
public:
	std::string interf;
	ethernet_wakeup(const std::string &eth_path, const std::string &iface);

	virtual int wakeup_value(void);

	virtual void wakeup_toggle(void);

	virtual std::string wakeup_toggle_script(void);

};

extern void add_ethernet_wakeup(void);

