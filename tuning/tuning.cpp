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


#include <stdio.h>
#include <string.h>
#include <ncurses.h>


#include "tuning.h"
#include "sysfs.h"
#include "../display.h"

void initialize_tuning(void)
{
	add_sysfs_tunable("Enable Audio codec power management", "/sys/module/snd_hda_intel/parameters/power_save", "1");
}



void tuning_update_display(void)
{
	WINDOW *win;
	unsigned int i;


	win = get_ncurses_win("Tunables");

	if (!win)
		return;

	wclear(win);

	wmove(win, 2,0);

	for (i = 0; i < all_tunables.size(); i++) {
		char res[128];
		strcpy(res, all_tunables[i]->result_string());
		while (strlen(res) < 12)
			strcat(res, " ");
		wprintw(win, "%s  %s\n", res, all_tunables[i]->description());
	}
}
