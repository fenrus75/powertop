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
#include "../report.h"
#include "../lib.h"

static void sort_tunables(void);
static bool should_clear = FALSE;

#ifndef DISABLE_NCURSES
class tuning_window: public tab_window {
public:
	virtual void repaint(void);
	virtual void cursor_enter(void);
	virtual void expose(void);
	virtual void window_refresh(void);
};
#endif // DISABLE_NCURSES

static void init_tuning(void)
{
	add_sysfs_tunable(_("Enable Audio codec power management"), "/sys/module/snd_hda_intel/parameters/power_save", "1");
	add_sysfs_tunable(_("Enable SATA link power management for /dev/sda"), "/sys/class/scsi_host/host0/link_power_management_policy", "min_power");
	add_sysfs_tunable(_("NMI watchdog should be turned off"), "/proc/sys/kernel/nmi_watchdog", "0");
	add_sysfs_tunable(_("Power Aware CPU scheduler"), "/sys/devices/system/cpu/sched_mc_power_savings", "1");
	add_sysfs_tunable(_("VM writeback timeout"), "/proc/sys/vm/dirty_writeback_centisecs", "1500");

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
#ifndef DISABLE_NCURSES
	class tuning_window *w;

	w = new tuning_window();
	create_tab("Tunables", _("Tunables"), w, _(" <ESC> Exit | <Enter> Toggle tunable | <r> Window refresh"));
#endif // DISABLE_NCURSES

	init_tuning();

#ifndef DISABLE_NCURSES
	w->cursor_max = all_tunables.size() - 1;
#endif // DISABLE_NCURSES
}



static void __tuning_update_display(int cursor_pos)
{
#ifndef DISABLE_NCURSES
	WINDOW *win;
	unsigned int i;

	win = get_ncurses_win("Tunables");

	if (!win)
		return;

	if (should_clear) {
		should_clear = FALSE;
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
#endif
}

void tuning_update_display(void)
{
#ifndef DISABLE_NCURSES
	class tab_window *w;

	w = tab_windows["Tunables"];
	if (!w)
		return;
	w->repaint();
#endif
}
#ifndef DISABLE_NCURSES
void tuning_window::repaint(void)
{
	__tuning_update_display(cursor_pos);
}

void tuning_window::cursor_enter(void)
{
	class tunable *tun;

	tun = all_tunables[cursor_pos];
	if (!tun)
		return;
	tun->toggle();
}
#endif // DISABLE_NCURSES

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
	should_clear = TRUE;
	init_tuning();
}

static void sort_tunables(void)
{
	sort(all_tunables.begin(), all_tunables.end(), tunables_sort);
}
#ifndef DISABLE_NCURSES
void tuning_window::expose(void)
{
	cursor_pos = 0;
	sort_tunables();
	repaint();
}
#endif // DISABLE_NCURSES
static const char *tune_class(int line)
{
	if (line & 1) {
		return "tunable_odd";
	}
	return "tunable_even";
}

static const char *tune_class_bad(int line)
{
	if (line & 1) {
		return "tunable_odd_bad";
	}
	return "tunable_even_bad";
}


void report_show_tunables(void)
{
	unsigned int i, line;
	/* three sections; bad, unfixable, good */

	if ((!reportout.csv_report)&&(!reportout.http_report))
		return;

	sort_tunables();


	if (reporttype)
		fprintf(reportout.http_report, "<div id=\"tuning\">\n");

	line = 0;
	for (i = 0; i < all_tunables.size(); i++) {
		int gb;

		gb = all_tunables[i]->good_bad();

		if (gb != TUNE_BAD)
			continue;

		if (line == 0) {
			if(reporttype)
				fprintf(reportout.http_report,"<h2>Software Settings in need of Tuning</h2>\n <table width=\"100%%\">\n");
			else
				fprintf(reportout.csv_report,"**Software Settings in need of Tuning**, \n\n");

		}

		line++;
		if (i < 1 && !reporttype)
			fprintf(reportout.csv_report, "Description, \n");

		if (reporttype)
		        fprintf(reportout.http_report, "<tr class=\"%s\"><td>%s</td><td>%s</td></tr>\n", tune_class_bad(line), all_tunables[i]->description(), all_tunables[i]->toggle_script());
		else
			fprintf(reportout.csv_report, "\"%s\", \n", all_tunables[i]->description());
	}

	if (line > 0) {
		if(reporttype)
			fprintf(reportout.http_report,"</table>\n");
		else
			fprintf(reportout.csv_report, "\n");
	}


	line = 0;
	for (i = 0; i < all_untunables.size(); i++) {
		if (line == 0) {
			if(reporttype)
				fprintf(reportout.http_report,
					"<h2>Untunable Software Issues</h2>\n <table width=\"100%%\">\n");
			else
				fprintf(reportout.csv_report,
					"**Untunable Software Issues**,\n\n");
		}

		line++;
		if (i < 1 && !reporttype)
			fprintf(reportout.csv_report, "Description, \n");

		if (reporttype)
			fprintf(reportout.http_report,
					"<tr class=\"%s\"><td>%s</td></tr>\n",
					tune_class_bad(line), all_untunables[i]->description());
		else
			fprintf(reportout.csv_report,"\"%s\", \n", all_untunables[i]->description());
	}

	if (line > 0) {
		if(reporttype)
			fprintf(reportout.http_report,"</table>\n");
		else
			fprintf(reportout.csv_report,"\n");
	}

	line = 0;
	for (i = 0; i < all_tunables.size(); i++) {
		int gb;

		gb = all_tunables[i]->good_bad();

		if (gb != TUNE_GOOD)
			continue;

		if (line == 0) {
			if (reporttype)
				fprintf(reportout.http_report,
					"<h2>Optimal Tuned Software Settings</h2>\n <table width=\"100%%\">\n");
			else
				fprintf(reportout.csv_report,
					"**Optimal Tuned Software Settings**, \n\n");
		}

		line++;
		if (i < 1 && !reporttype)
			fprintf(reportout.csv_report, "Description, \n");

		if (reporttype)
			fprintf(reportout.http_report,"<tr class=\"%s\"><td>%s</td></tr>\n",
			tune_class(line), all_tunables[i]->description());
		else
			fprintf(reportout.csv_report,"\"%s\", \n", all_tunables[i]->description());
	}

	if (line > 0){
		if (reporttype)
			fprintf(reportout.http_report,"</table></div>\n");
		else
			fprintf(reportout.csv_report,"\n");
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
