#include "device.h"
#include "../parameters/parameters.h"
#include <stdio.h>

device_consumer::device_consumer(class device *dev)
{
	device = dev;
	power = device->power_usage(&all_results, &all_parameters);
}


const char * device_consumer::description(void)
{
	sprintf(str, "%s", device->human_name());
	return str;
}

double device_consumer::Witts(void)
{
	return power;
}

static void add_device(class device *device)
{
	class device_consumer *dev;

	if (device->show_in_list() == 0)
		return;

	dev = new class device_consumer(device);
	all_power.push_back(dev);
}

void all_devices_to_all_power(void)
{
	unsigned int i;
	for (i = 0; i < all_devices.size(); i++)
		add_device(all_devices[i]);
}
