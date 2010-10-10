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
using namespace std;

#include "backlight.h"
#include "usb.h"
#include "ahci.h"
#include "alsa.h"
#include "rfkill.h"
#include "i915-gpu.h"
#include "thinkpad-fan.h"
#include "network.h"

#include "../parameters/parameters.h"
#include "../display.h"
#include "../lib.h"

void device::start_measurement(void)
{
}

void device::end_measurement(void)
{
}

double	device::utilization(void)
{
	return 0.0;
}



vector<class device *> all_devices;


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
}

static bool power_device_sort(class device * i, class device * j)
{
	double pI, pJ;
	pI = i->power_usage(&all_results, &all_parameters);
	pJ = j->power_usage(&all_results, &all_parameters);

	if (pI == pJ) {
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

	char util[128];
	char power[128];

	win = tab_windows["Device stats"];
        if (!win)
                return;

        wclear(win);
        wmove(win, 2,0);

	sort(all_devices.begin(), all_devices.end(), power_device_sort);

	format_watts(get_parameter_value("base power"), power, 11);
	wprintw(win, "Platform base power is estimated at %s\n\n", power);

	wprintw(win, "Power est.    Usage     Device name\n");
	for (i = 0; i < all_devices.size(); i++) {
		double P;

		util[0] = 0;

		if (all_devices[i]->util_units()) {
			if (all_devices[i]->utilization() < 1000)
				sprintf(util, "%5.1f%s",  all_devices[i]->utilization(),  all_devices[i]->util_units());
			else
				sprintf(util, "%5i%s",  (int)all_devices[i]->utilization(),  all_devices[i]->util_units());
		}
		while (strlen(util) < 11) strcat(util, " ");

		P = all_devices[i]->power_usage(&all_results, &all_parameters);

		format_watts(P, power, 11);

		if (!all_devices[i]->power_valid())
			strcpy(power, "           ");


		wprintw(win, "%s %s %s\n", 
			power,
			util,
			all_devices[i]->human_name()
			);
	}
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
	create_all_nics();
}
