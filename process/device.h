#ifndef _INCLUDE_GUARD_DEVICE2_H
#define _INCLUDE_GUARD_DEVICE2_H

#include <stdint.h>

#include "powerconsumer.h"
#include "../devices/device.h"

class device_consumer : public power_consumer {
	class device *device;
	double power;
	char str[4096];
public:
	device_consumer(class device *dev);

	virtual const char * description(void);
	virtual const char * name(void) { return "device"; };
	virtual const char * type(void) { return "Device"; };
	virtual double Witts(void);
	virtual double usage(void) { return device->utilization();};
	virtual const char * usage_units(void) {return device->util_units();};
	virtual int show_events(void) { return 0; };
};

extern void all_devices_to_all_power(void);


#endif