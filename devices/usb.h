#ifndef _INCLUDE_GUARD_USB_H
#define _INCLUDE_GUARD_USB_H


#include "device.h"

class usbdevice: public device {
	int active_before, active_after;
	int connected_before, connected_after;
	char sysfs_path[4096];
	char name[4096];
	char devname[4096];
public:

	usbdevice(const char *_name, const char *path, const char *devid);

	virtual void start_measurement(void);
	virtual void end_measurement(void);

	virtual double	utilization(void); /* percentage */

	virtual const char * class_name(void) { return "usb";};

	virtual const char * device_name(void);
	virtual double power_usage(struct result_bundle *result, struct parameter_bundle *bundle);
};

extern void create_all_usb_devices(void);


#endif