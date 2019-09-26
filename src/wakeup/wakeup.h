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
#ifndef _INCLUDE_GUARD_WAKEUP_H
#define _INCLUDE_GUARD_WAKEUP_H

#include<vector>
#include <limits.h>

using namespace std;

#define WAKEUP_ENABLE 1
#define WAKEUP_DISABLE 0

class wakeup {
	char wakeup_enable[128];
	char wakeup_disable[128];
	char wakeup_idle[128];
protected:
	char toggle_enable[4096];
	char toggle_disable[4096];
public:
	char desc[4096];
	double score;

	wakeup(const char *str, double _score, const char *enable = "", const char *disable = "");
	wakeup(void);

	virtual ~wakeup () {};

	virtual int wakeup_value(void) { return WAKEUP_DISABLE; }

	virtual char *wakeup_string(void)
	{
		switch (wakeup_value()) {
		case WAKEUP_ENABLE:
			return wakeup_enable;
		case WAKEUP_DISABLE:
			return wakeup_disable;
		}
		return wakeup_idle;
	}


	virtual const char *description(void) { return desc; };

	virtual void wakeup_toggle(void) { };

	virtual const char *wakeup_toggle_script(void) { return toggle_enable; }

};

extern vector<class wakeup *> wakeup_all;

extern void initialize_wakeup(void);
extern void wakeup_update_display(void);
extern void report_show_wakeup(void);
extern void clear_wakeup(void);

#endif
