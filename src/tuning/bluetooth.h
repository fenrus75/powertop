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
#include <ctime>

#include "tunable.h"

class bt_tunable : public tunable {
public:
	bt_tunable(int id, const std::string &name);

	virtual int good_bad(void) override;

	virtual void toggle(void) override;

	void collect_json_fields(std::string &_js) override;

	/* Virtual hardware helpers — override in tests to inject fake data */
	virtual int  hci_get_dev_info(unsigned int &flags,
	                              unsigned int &byte_rx,
	                              unsigned int &byte_tx);
	virtual void hci_set_power(bool up);
	virtual time_t current_time();

protected:
	int    snap_bytes[2];
	time_t snap_time[2];
	int    dev_id;
	std::string bt_name;
};

extern void add_bt_tunable(void);

