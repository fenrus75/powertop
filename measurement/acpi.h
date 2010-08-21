#ifndef __INCLUDE_GUARD_ACPI_H
#define __INCLUDE_GUARD_ACIP_

#include "measurement.h"

class acpi_power_meter: public power_meter {
	char battery_name[256];

	double capacity;
	double rate;
	void measure(void);
public:
	acpi_power_meter(const char *_battery_name);
	virtual void start_measurement(void);
	virtual void end_measurement(void);

	virtual double joules_consumed(void);
};

#endif