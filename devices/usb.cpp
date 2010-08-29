#include "usb.h"

#include <string.h>

usbdevice::usbdevice(char *_name, char *path)
{
	strcpy(sysfs_path, path);
	strcpy(name, _name);
}



void usbdevice::start_measurement(void)
{
}

void usbdevice::end_measurement(void)
{
}

double usbdevice::utilization(void) /* percentage */
{
	return 0.0;
}

const char * usbdevice::device_name(void)
{
	return name;
}


double usbdevice::power_usage(struct result_bundle *result, struct parameter_bundle *bundle)
{
	return 0.0;
}


void create_all_usb_devices(void)
{
}

