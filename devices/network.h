#ifndef _INCLUDE_GUARD_NETWORK_H
#define _INCLUDE_GUARD_NETWORK_H


#include "device.h"
#include "../parameters/parameters.h"

class network: public device {
	int start_link, end_link;
	int start_up, end_up;
	uint64_t start_pkts, end_pkts;
	
	char sysfs_path[4096];
	char name[4096];
	char humanname[4096];
	int index_up;
	int rindex_up;
	int index_link;
	int rindex_link;
	int index_pkts;
	int rindex_pkts;
public:
	uint64_t pkts;

	network(char *_name, char *path);

	virtual void start_measurement(void);
	virtual void end_measurement(void);

	virtual double	utilization(void); 
	virtual const char * util_units(void) { return " pkts/s"; };

	virtual const char * class_name(void) { return "ethernet";};

	virtual const char * device_name(void);
	virtual const char * human_name(void) { return humanname; };
	virtual double power_usage(struct result_bundle *result, struct parameter_bundle *bundle);
	virtual int power_valid(void) { return utilization_power_valid(rindex_up) + utilization_power_valid(rindex_link);};
};

extern void create_all_nics(void);


#endif