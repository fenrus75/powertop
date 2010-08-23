
#include "device.h"
#include <vector>
using namespace std;


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
