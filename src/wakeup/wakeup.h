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

#include<vector>
#include <limits.h>
#include <string>
#include "../lib.h"

#define WAKEUP_ENABLE 1
#define WAKEUP_DISABLE 0

class wakeup {
	std::string wakeup_enable;
	std::string wakeup_disable;
	std::string wakeup_idle;
protected:
	std::string toggle_enable;
	std::string toggle_disable;
public:
	std::string desc;
	double score = 0.0;

	wakeup(const std::string &str, double _score, const std::string &enable = "", const std::string &disable = "");
	wakeup(void);

	virtual ~wakeup () {};

	virtual int wakeup_value(void) { return WAKEUP_DISABLE; }

	virtual const std::string& wakeup_string(void)
	{
		switch (wakeup_value()) {
		case WAKEUP_ENABLE:
			return wakeup_enable;
		case WAKEUP_DISABLE:
			return wakeup_disable;
		}
		return wakeup_idle;
	}


	virtual std::string description(void) { return desc; };

	virtual void wakeup_toggle(void) { };

	virtual std::string wakeup_toggle_script(void) { return toggle_enable; }

	virtual void collect_json_fields(std::string &_js);
	std::string serialize() { JSON_START(); collect_json_fields(_js); JSON_END(); }

};

extern std::vector<class wakeup *> wakeup_all;

extern void initialize_wakeup(void);
extern void wakeup_update_display(void);
extern void report_show_wakeup(void);
extern void clear_wakeup(void);

