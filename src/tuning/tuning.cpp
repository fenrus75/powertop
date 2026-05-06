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
#include <ncurses.h>


#include "tuning.h"
#include "tuningi2c.h"
#include "tuningsysfs.h"
#include "tuningusb.h"
#include "runtime.h"
#include "bluetooth.h"
#include "ethernet.h"
#include "wifi.h"
#include "../display.h"
#include "../report/report.h"
#include "../report/report-maker.h"
#include "../report/report-data-html.h"
#include "../lib.h"

static void sort_tunables(void);
static bool should_clear = false;

class tuning_window *tune_window;

class tuning_window: public tab_window {
public:
	virtual void repaint(void) override;
	virtual void cursor_enter(void) override;
	virtual void expose(void) override;
	virtual void window_refresh(void) override;
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
	add_bt_tunable();
	add_wifi_tunables();
	add_i2c_tunables();

	add_sysfs_tunable(_("VM dirty ratio"), "/proc/sys/vm/dirty_ratio", "50");
	add_sysfs_tunable(_("Tuning for scan sleep millisecs"), "/sys/kernel/mm/transparent_hugepage/khugepaged/scan_sleep_millisecs", "30000");
	add_sysfs_tunable(_("Tuning for cursor blink"), "/sys/devices/virtual/graphics/fbcon/cursor_blink", "0");
	add_sysfs_tunable(_("Tuning for sleep millisecs"), "/sys/kernel/mm/ksm/sleep_millisecs", "4000");
	add_sysfs_tunable(_("Batch size for KSM"), "/sys/kernel/mm/ksm/pages_to_scan", "1000");
	add_sysfs_tunable(_("Autogroup scheduling"), "/proc/sys/kernel/sched_autogroup_enabled", "0");
	add_sysfs_tunable(_("Intel P-state minimum performance"), "/sys/devices/system/cpu/intel_pstate/min_perf_pct", "50");
	add_sysfs_tunable(_("Tuning for default qdisc"), "/proc/sys/net/core/default_qdisc", "fq");
	add_sysfs_tunable(_("Intel ITMT (Turbo Boost Max 3.0)"), "/proc/sys/kernel/sched_itmt_enabled", "1");
	add_sysfs_tunable(_("Intel energy performance bias"), "/sys/devices/system/cpu/cpu*/power/energy_perf_bias", "balance-performance");
	add_sysfs_tunable(_("Intel energy performance preference"), "/sys/devices/system/cpu/cpufreq/policy*/energy_performance_preference", "balance_performance");
	add_sysfs_tunable(_("Intel energy performance preference"), "/sys/devices/system/cpu/cpu*/cpufreq/energy_performance_preference", "balance_performance");
	add_sysfs_tunable(_("CPU C1 demotion"), "/sys/devices/system/cpu/cpuidle/c1_demotion", "1");

	sort_tunables();
}

void initialize_tuning(void)
{
	class tuning_window *w;

	w = new tuning_window();
	create_tab("Tunables", _("Tunables"), w, _(" <ESC> Exit | <Enter> Toggle tunable | <r> Window refresh"));

	init_tuning();

	w->cursor_max = all_tunables.empty() ? 0 : static_cast<int>(all_tunables.size()) - 1;

	if (tune_window)
		delete tune_window;

	tune_window = w;
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
		std::string res = all_tunables[i]->result_string();
		std::string desc = all_tunables[i]->description();

		align_string(res, 12, 12);
		align_string(desc, 103, 103);

		if ((int)i != cursor_pos) {
			wattroff(win, A_REVERSE);
			wprintw(win, "   ");
		} else {
			wattron(win, A_REVERSE);
			wprintw(win, ">> ");
		}
		wprintw(win, "%s  %s\n", res.c_str(), desc.c_str());
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
	std::string toggle_script;
	tun = all_tunables[cursor_pos];
	if (!tun)
		return;
	/** device will change its state so need to store toggle script before
	 * we toggle()*/
	toggle_script = tun->toggle_script();
	tun->toggle();
	if (!toggle_script.empty())
		ui_notify_user(std::format(">> {}\n", toggle_script));
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

	return strcasecmp(i->description().c_str(), j->description().c_str()) < 0;
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
	/* three tables; bad, unfixable, good */
	sort_tunables();
	int idx, rows = 0, cols;

	/* First Table */

	 /* div attr css_class and css_id */
        tag_attr div_attr;
        init_div(&div_attr, "clear_block", "tuning");

	/* Set Title attributes */
       	tag_attr title_attr;
        init_title_attr(&title_attr);

	/* Set Table attributes, rows, and cols */
	table_attributes tune_table_css;
	cols=2;
	idx = cols;

	for (i = 0; i < all_tunables.size(); i++) {
		int tgb;
		tgb = all_tunables[i]->good_bad();
		if (tgb == TUNE_BAD)
			rows+=1;
	}
	/* add section */
	report.add_div(&div_attr);

	if (rows > 0){
		rows= rows + 1;
		init_tune_table_attr(&tune_table_css, rows, cols);

		/* Set array of data in row Major order */
		std::vector<std::string> tunable_data(cols * rows);

		tunable_data[0]=__("Description");
		tunable_data[1]=__("Script");

		for (i = 0; i < all_tunables.size(); i++) {
			int gb;
			gb = all_tunables[i]->good_bad();
			if (gb != TUNE_BAD)
				continue;
			tunable_data[idx]=all_tunables[i]->description();
			idx+=1;
			tunable_data[idx]=std::string(all_tunables[i]->toggle_script());
			idx+=1;
		}

		/* Report Output */
		report.add_title(&title_attr,__("Software Settings in Need of Tuning"));
		report.add_table(tunable_data, &tune_table_css);
	}

	/* Second Table */
	/* Set Table attributes, rows, and cols */
	cols=1;
	rows= all_untunables.size() + 1;
	init_tune_table_attr(&tune_table_css, rows, cols);

	/* Set array of data in row Major order */
	std::vector<std::string> untunable_data(rows);
	untunable_data[0]=__("Description");

	for (i = 0; i < all_untunables.size(); i++) {
		untunable_data[i+1]= all_untunables[i]->description();
	}
	/* Report Output */
	report.add_title(&title_attr,__("Untunable Software Issues"));
	report.add_table(untunable_data, &tune_table_css);

	/* Third Table */
	/* Set Table attributes, rows, and cols */
	rows = 1;
	for (i = 0; i < all_tunables.size(); i++) {
                int gb;
                gb = all_tunables[i]->good_bad();
                if (gb != TUNE_GOOD)
                        continue;
		rows+=1;
	}
	cols = 1;
	init_tune_table_attr(&tune_table_css, rows, cols);

	/* Set array of data in row Major order */
	std::vector<std::string> tuned_data(rows);
	tuned_data[0]=__("Description");
	idx=cols;
	for (i = 0; i < all_tunables.size(); i++) {
		int gb;
		gb = all_tunables[i]->good_bad();
		if (gb != TUNE_GOOD)
			continue;

		tuned_data[idx]=all_tunables[i]->description();
		idx+=1;
        }
	/* Report Output */
	report.add_title(&title_attr,__("Optimal Tuned Software Settings"));
        report.add_table(tuned_data, &tune_table_css);
        report.end_div();
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

void auto_toggle_tuning(bool dump_only)
{
	if (dump_only) fprintf(stdout, "### auto-tune-dump commands BEGIN\n\n");
	for (unsigned int i = 0; i < all_tunables.size(); i++) {
		if (all_tunables[i]->good_bad() == TUNE_BAD) {
			if (dump_only) {
				all_tunables[i]->dump_cmd_good(stdout);
			} else {
				all_tunables[i]->toggle();
			}
		}
	}
	if (dump_only) fprintf(stdout, "\n### auto-tune-dump commands END\n\n");
}
