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

#include "device.h"
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <format>

#include "backlight.h"
#include "usb.h"
#include "ahci.h"
#include "alsa.h"
#include "rfkill.h"
#include "i915-gpu.h"
#include "thinkpad-fan.h"
#include "thinkpad-light.h"
#include "network.h"
#include "runtime_pm.h"

#include "../parameters/parameters.h"
#include "../display.h"
#include "../lib.h"
#include "../report/report.h"
#include "../report/report-maker.h"
#include "../report/report-data-html.h"
#include "../measurement/measurement.h"
#include "../devlist.h"
#include <unistd.h>

device::device(void)
{
	cached_valid = 0;
	hide = 0;
}


void device::register_sysfs_path(const std::string &path)
{
	std::string current_path = path;
	int iter = 0;
	char resolved_path[PATH_MAX + 1];

	while (iter++ < 10) {
		std::string test_path = current_path + "/device";
		if (access(test_path.c_str(), R_OK) == 0)
			current_path = test_path;
		else
			break;
	}

	if (realpath(current_path.c_str(), resolved_path))
		real_path = resolved_path;
	else
		real_path.clear();
}

void device::start_measurement(void)
{
	hide = false;
}

void device::end_measurement(void)
{
}

double	device::utilization(void)
{
	return 0.0;
}



std::vector<class device *> all_devices;


void devices_start_measurement(void)
{
	unsigned int i;
	for (i = 0; i < all_devices.size(); i++)
		all_devices[i]->start_measurement();
}

void devices_end_measurement(void)
{
	unsigned int i;
	for (i = 0; i < all_devices.size(); i++)
		all_devices[i]->end_measurement();

	clear_devpower();

	for (i = 0; i < all_devices.size(); i++) {
		all_devices[i]->hide = false;
		all_devices[i]->register_power_with_devlist(&all_results, &all_parameters);
	}
}

static bool power_device_sort(class device * i, class device * j)
{
	double pI, pJ;
	pI = i->power_usage(&all_results, &all_parameters);
	pJ = j->power_usage(&all_results, &all_parameters);

	if (equals(pI, pJ)) {
		int vI, vJ;
		vI = i->power_valid();
		vJ = j->power_valid();

		if (vI != vJ)
			return vI > vJ;

		return i->utilization() > j->utilization();
	}
	return pI > pJ;
}


void report_devices(void)
{
	WINDOW *win;
	unsigned int i;
	int show_power;
	double pw;

	std::string util;

	win = get_ncurses_win("Device stats");
        if (!win)
                return;

	show_power = global_power_valid();

        wclear(win);
        wmove(win, 2,0);

	sort(all_devices.begin(), all_devices.end(), power_device_sort);



	pw = global_power();
	if (pw > 0.0001) {
		wprintw(win, _("The battery reports a discharge rate of %sW\n"),
				fmt_prefix(pw).c_str());
		wprintw(win, _("The energy consumed was %sJ\n"),
				fmt_prefix(global_joules()).c_str());
	}

	if (show_power) {
		wprintw(win, _("System baseline power is estimated at %sW\n"),
				fmt_prefix(get_parameter_value("base power")).c_str());
	}

	if (pw > 0.0001 || show_power)
		wprintw(win, "\n");
	if (show_power)
		wprintw(win, _("Power est.    Usage     Device name\n"));
	else
		wprintw(win, _("              Usage     Device name\n"));

	for (i = 0; i < all_devices.size(); i++) {
		double P;
		std::string power;

		util = "";

		if (!all_devices[i]->util_units().empty()) {
			if (all_devices[i]->utilization() < 1000)
				util = std::format("{:5.1f}{}",  all_devices[i]->utilization(),  all_devices[i]->util_units());
			else
				util = std::format("{:5d}{}",  (int)all_devices[i]->utilization(),  all_devices[i]->util_units());
		}
		while (util.length() < 13) util.append(" ");

		P = all_devices[i]->power_usage(&all_results, &all_parameters);

		power = format_watts(P, 11);

		if (!show_power || !all_devices[i]->power_valid())
			power = "           ";


		wprintw(win, "%s %s %s\n",
			power.c_str(),
			util.c_str(),
			all_devices[i]->human_name().c_str()
			);
	}
}

void show_report_devices(void)
{
	unsigned int i;
	int show_power, cols, rows, idx;
	double pw;

	show_power = global_power_valid();
	sort(all_devices.begin(), all_devices.end(), power_device_sort);

	/* div attr css_class and css_id */
        tag_attr div_attr;
        init_div(&div_attr, "clear_block", "devinfo");

        /* Set Table attributes, rows, and cols */
        table_attributes std_table_css;
	cols=2;
        if (show_power)
                cols=3;

	idx = cols;
 	rows= all_devices.size() + 1;
        init_std_side_table_attr(&std_table_css, rows, cols);

        /* Set Title attributes */
        tag_attr title_attr;
        init_title_attr(&title_attr);

	/* Add section */
	report.add_div(&div_attr);

	/* Device Summary */
	int summary_size=2;
	std::vector<std::string> summary(summary_size);
	pw = global_power();
	if (pw > 0.0001) {
		summary[0]= __("The battery reports a discharge rate of: ");
		summary[1]=std::format("{} W", fmt_prefix(pw));
		report.add_summary_list(summary);

		summary[0]= __("The energy consumed was : ");
		summary[1]=std::format("{} J", fmt_prefix(global_joules()));
		report.add_summary_list(summary);
	}

	if (show_power) {
		summary[0]=__("The system baseline power is estimated at: ");
		summary[1]=std::format("{} W", fmt_prefix(get_parameter_value("base power")));
		report.add_summary_list(summary);
	}

        /* Set array of data in row Major order */
	std::vector<std::string> device_data(cols * rows);
	device_data[0]= __("Usage");
	device_data[1]= __("Device Name");
	if (show_power)
		device_data[2]= __("PW Estimate");

	for (i = 0; i < all_devices.size(); i++) {
		double P;
		std::string util;
		std::string power;

		if (!all_devices[i]->util_units().empty()) {
			if (all_devices[i]->utilization() < 1000)
				util = std::format("{:5.1f}{}",
					all_devices[i]->utilization(),
					all_devices[i]->util_units());
			else
				util = std::format("{:5d}{}",
					(int)all_devices[i]->utilization(),
					all_devices[i]->util_units());
		}

		P = all_devices[i]->power_usage(&all_results, &all_parameters);
		power = format_watts(P, 11);

		if (!show_power || !all_devices[i]->power_valid())
			power = "           ";

		device_data[idx]= util;
		idx+=1;

		device_data[idx]= all_devices[i]->human_name();
		idx+=1;

		if (show_power) {
			device_data[idx]= power;
			idx+=1;
		}
	}
	/* Report Output */
	report.add_title(&title_attr, __("Device Power Report"));
	report.add_table(device_data, &std_table_css);
}


void create_all_devices(void)
{
	create_all_backlights();
	create_all_usb_devices();
	create_all_ahcis();
	create_all_alsa();
	create_all_rfkills();
	create_i915_gpu();
	create_thinkpad_fan();
	create_thinkpad_light();
	create_all_nics();
	create_all_runtime_pm_devices();
}


void clear_all_devices(void)
{
	unsigned int i;
	for (i = 0; i < all_devices.size(); i++) {
		delete all_devices[i];
	}
	all_devices.clear();
}
