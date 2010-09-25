#ifndef _INCLUDE_GUARD_AHCI_H
#define _INCLUDE_GUARD_AHCI_H


#include <string>
#include "device.h"
#include "../parameters/parameters.h"
#include <stdint.h>

class ahci: public device {
	uint64_t start_active, end_active;
	uint64_t start_partial, end_partial;
	uint64_t start_slumber, end_slumber;
	char sysfs_path[4096];
	char name[4096];
	int partial_rindex;
	int active_rindex;
	int partial_index;
	int active_index;
	char humanname[4096];
public:

	ahci(char *_name, char *path);

	virtual void start_measurement(void);
	virtual void end_measurement(void);

	virtual double	utilization(void); /* percentage */

	virtual const char * class_name(void) { return "ahci";};

	virtual const char * device_name(void);
	virtual const char * human_name(void) { return humanname;};
	virtual double power_usage(struct result_bundle *result, struct parameter_bundle *bundle);
	virtual int power_valid(void) { return utilization_power_valid(partial_rindex) + utilization_power_valid(active_rindex);};
};

extern void create_all_ahcis(void);


#endif