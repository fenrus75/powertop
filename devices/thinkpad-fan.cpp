#include <iostream>
#include <fstream>

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <math.h>

#include "../lib.h"


#include "device.h"
#include "thinkpad-fan.h"
#include "../parameters/parameters.h"
#include "../process/powerconsumer.h"

#include <string.h>


thinkpad_fan::thinkpad_fan()
{
	start_rate = 0;
	end_rate = 0;
}

void thinkpad_fan::start_measurement(void)
{
	/* read the rpms of the fan */
	start_rate = read_sysfs("/sys/devices/platform/thinkpad_hwmon/fan1_input");
}

void thinkpad_fan::end_measurement(void)
{
	end_rate = read_sysfs("/sys/devices/platform/thinkpad_hwmon/fan1_input");

	report_utilization("thinkpad-fan", utilization());
}


double thinkpad_fan::utilization(void)
{
	return (start_rate+end_rate) / 100.0;
}

void create_thinkpad_fan(void)
{
	char filename[4096];
	class thinkpad_fan *fan;

	strcpy(filename, "/sys/devices/platform/thinkpad_hwmon/fan1_input");

	if (access(filename, R_OK) !=0)
		return;

	register_parameter("thinkpad-fan");

	fan = new class thinkpad_fan();
	all_devices.push_back(fan);
}



double thinkpad_fan::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	double power;
	double factor;
	double utilization;

	static int fan_index = 0, fansqr_index = 0;

	if (!fan_index)
		fan_index = get_param_index("thinkpad-fan");
	if (!fansqr_index)
		fansqr_index = get_param_index("thinkpad-fan-sqr");

	power = 0;
	utilization = get_result_value("thinkpad-fan", result);

	utilization = utilization - 50;
	if (utilization < 0)
		utilization = 0;


	factor = get_parameter_value(fansqr_index, bundle);
	power += factor * pow(utilization / 100.0, 2);

	factor = get_parameter_value(fan_index, bundle);
	power -= utilization * factor / 100.0;

	if (power <= 0.0)
		power = 0.0;

	return power;
}
