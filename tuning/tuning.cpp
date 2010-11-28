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

class tuning_window: public tab_window {
public:
	virtual void repaint(void);
};

void initialize_tuning(void)
{
	class tuning_window *w;

	w = new tuning_window();
	create_tab("Tunables", w);
	add_sysfs_tunable("Enable Audio codec power management", "/sys/module/snd_hda_intel/parameters/power_save", "1");
	add_sysfs_tunable("Enable SATA link power management for /dev/sda", "/sys/class/scsi_host/host0/link_power_management_policy", "min_performance");


	w->cursor_max = all_tunables.size() - 1;
}



static void __tuning_update_display(int cursor_pos)
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
		char desc[4096];
		strcpy(res, all_tunables[i]->result_string());
		strcpy(desc, all_tunables[i]->description());
		while (strlen(res) < 12)
			strcat(res, " ");

		while (strlen(desc) < 103)
			strcat(desc, " ");
		if ((int)i != cursor_pos) {
			wattrset(win, A_NORMAL);
			wprintw(win, "   ");
		} else {
			wattrset(win, A_REVERSE);
			wprintw(win, ">> ");
		}
		wprintw(win, "%s  %s\n", res, desc);
	}
}

void tuning_update_display(void)
{
	class tab_window *w;

	w = tab_windows["Tunables"];
	if (!w)
		return;
	w->repaint();
}

void tuning_window::repaint(void)
{
	__tuning_update_display(cursor_pos);
}