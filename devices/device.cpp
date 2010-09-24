
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

#include "../parameters/parameters.h"
#include "../display.h"

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
        return (i->power_usage(&all_results, &all_parameters)) > j->power_usage(&all_results, &all_parameters);
}


void report_devices(void)
{
	WINDOW *win;
	unsigned int i;

	win = tab_windows["Device stats"];
        if (!win)
                return;

        wclear(win);
        wmove(win, 2,0);

	sort(all_devices.begin(), all_devices.end(), power_device_sort);

	wprintw(win, "Device statistics\n");
	for (i = 0; i < all_devices.size(); i++) {
		wprintw(win, "%5.1f%% %6.2fW  %s (%s) \n", 
			all_devices[i]->utilization(),
			all_devices[i]->power_usage(&all_results, &all_parameters),
			all_devices[i]->class_name(),
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
}
