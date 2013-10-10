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

#include <algorithm>

#include <stdio.h>
#include <string.h>
#include <ncurses.h>


#include "tuning.h"
#include "tuningsysfs.h"
#include "tuningusb.h"
#include "runtime.h"
#include "bluetooth.h"
#include "cpufreq.h"
#include "ethernet.h"
#include "wifi.h"
#include "../display.h"
#include "../report/report.h"
#include "../report/report-maker.h"
#include "../lib.h"

static void sort_tunables(void);
static bool should_clear = false;

class tuning_window: public tab_window {
public:
	virtual void repaint(void);
	virtual void cursor_enter(void);
	virtual void expose(void);
	virtual void window_refresh(void);
};

static void init_tuning(void)
{
	add_sysfs_tunable(_("Enable Audio codec power management"), "/sys/module/snd_hda_intel/parameters/power_save", "1");
	add_sysfs_tunable(_("NMI watchdog should be turned off"), "/proc/sys/kernel/nmi_watchdog", "0");
	add_sysfs_tunable(_("Power Aware CPU scheduler"), "/sys/devices/system/cpu/sched_mc_power_savings", "1");
	add_sysfs_tunable(_("VM writeback timeout"), "/proc/sys/vm/dirty_writeback_centisecs", "1500");
	add_sata_tunables();
	add_usb_tunables();
	add_runtime_tunables("pci");
	add_ethernet_tunable();
	add_bt_tunable();
	add_wifi_tunables();
	add_cpufreq_tunable();

	sort_tunables();
}

void initialize_tuning(void)
{
	class tuning_window *w;

	w = new tuning_window();
	create_tab("Tunables", _("Tunables"), w, _(" <ESC> Exit | <Enter> Toggle tunable | <r> Window refresh"));

	init_tuning();

	w->cursor_max = all_tunables.size() - 1;
}



static void __tuning_update_display(int cursor_pos)
{
	WINDOW *win;
	unsigned int i;

	win = get_ncurses_win("Tunables");

	if (!win)
		return;

	if (should_clear) {
		should_clear = false;
		wclear(win);
	}

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
		wprintw(win, "%s  %s\n", _(res), _(desc));
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

void tuning_window::cursor_enter(void)
{
	class tunable *tun;
	const char *toggle_script;
	tun = all_tunables[cursor_pos];
	if (!tun)
		return;
	/** device will change its state so need to store toggle script before
	 * we toggle()*/
	toggle_script = tun->toggle_script();
	tun->toggle();
	ui_notify_user(">> %s\n", toggle_script);
}

static bool tunables_sort(class tunable * i, class tunable * j)
{
	int i_g, j_g;
	double d;

	i_g = i->good_bad();
	j_g = j->good_bad();

	if (!equals(i_g, j_g))
		return i_g < j_g;

	d = i->score - j->score;
	if (d < 0.0)
		d = -d;
	if (d > 0.0001)
		return i->score > j->score;

	if (strcasecmp(i->description(), j->description()) == -1)
		return true;

	return false;
}

void tuning_window::window_refresh()
{
	clear_tuning();
	should_clear = true;
	init_tuning();
}

static void sort_tunables(void)
{
	sort(all_tunables.begin(), all_tunables.end(), tunables_sort);
}

void tuning_window::expose(void)
{
	cursor_pos = 0;
	sort_tunables();
	repaint();
}

void report_show_tunables(void)
{
	unsigned int i;
	bool is_header;
	/* three tables; bad, unfixable, good */

	sort_tunables();
	report.begin_section(SECTION_TUNING);

	for (is_header = true, i = 0; i < all_tunables.size(); i++) {
		int gb;

		gb = all_tunables[i]->good_bad();
		if (gb != TUNE_BAD)
			continue;

		if (is_header) {
			report.add_header("Software Settings in need of Tuning");
			report.begin_table(TABLE_WIDE);
			report.begin_row();
			report.begin_cell(CELL_TUNABLE_HEADER);
			report.add("Description");
			report.begin_cell(CELL_TUNABLE_HEADER);
			report.add("Script");
			is_header = false;
		}

		report.begin_row(ROW_TUNABLE_BAD);
		report.begin_cell();
		report.add(all_tunables[i]->description());
		report.begin_cell();
		report.add(all_tunables[i]->toggle_script());
	}

	for (i = 0, is_header = true; i < all_untunables.size(); i++) {
		if (is_header) {
			report.add_header("Untunable Software Issues");
			report.begin_table(TABLE_WIDE);
			report.begin_row();
			report.begin_cell(CELL_TUNABLE_HEADER);
			report.add("Description");
			is_header = false;
		}

		report.begin_row(ROW_TUNABLE_BAD);
		report.begin_cell();
		report.add(all_untunables[i]->description());
	}

	for (i = 0, is_header = true; i < all_tunables.size(); i++) {
		int gb;

		gb = all_tunables[i]->good_bad();
		if (gb != TUNE_GOOD)
			continue;

		if (is_header) {
			report.add_header("Optimal Tuned Software Settings");
			report.begin_table(TABLE_WIDE);
			report.begin_row();
			report.begin_cell(CELL_TUNABLE_HEADER);
			report.add("Description");
			is_header = false;
		}

		report.begin_row(ROW_TUNABLE);
		report.begin_cell();
		report.add(all_tunables[i]->description());
	}
}

void clear_tuning()
{
	for (size_t i = 0; i < all_tunables.size(); i++) {
		delete all_tunables[i];
	}
	all_tunables.clear();

	for (size_t i = 0; i < all_untunables.size(); i++) {
		delete all_untunables[i];
	}
	all_untunables.clear();
}

void auto_toggle_tuning()
{
	for (unsigned int i = 0; i < all_tunables.size(); i++) {
		if (all_tunables[i]->good_bad() == TUNE_BAD)
			all_tunables[i]->toggle();
	}
}
