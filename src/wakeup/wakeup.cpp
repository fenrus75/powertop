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

#include <string.h>
#include <ncurses.h>
#include "wakeup.h"
#include <vector>
#include "../lib.h"

using namespace std;

vector<class wakeup *> wakeup_all;

wakeup::wakeup(const char *str, double _score, const char *enable, const char *disable)
{
	score = _score;
        pt_strcpy(desc, str);
        pt_strcpy(wakeup_enable, enable);
        pt_strcpy(wakeup_disable, disable);
}

wakeup::wakeup(void)
{
	score = 0;
        desc[0] = 0;
        pt_strcpy(wakeup_enable, _("Enabled"));
        pt_strcpy(wakeup_disable, _("Disabled"));
	pt_strcpy(wakeup_idle, _("Unknown"));
}

