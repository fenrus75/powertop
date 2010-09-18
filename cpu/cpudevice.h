#ifndef _INCLUDE_GUARD_CPUDEVICE_H
#define _INCLUDE_GUARD_CPUDEVICE_H

#include <vector>
#include <string>

using namespace std;

#include "../devices/device.h"
#include "cpu.h"

class cpudevice: public device {
	char _class[128];
	char _cpuname[128];

	vector<string> params;
	class abstract_cpu *cpu;
	int wake_index;
	int consumption_index;
	int r_wake_index;
	int r_consumption_index;

public:
	cpudevice(const char *classname = "cpu", const char *device_name = "cpu0", class abstract_cpu *_cpu = NULL);
	virtual const char * class_name(void) { return _class;};

	virtual const char * device_name(void) {return _cpuname;};

	virtual double power_usage(struct result_bundle *result, struct parameter_bundle *bundle);
	virtual bool show_in_list(void) {return false;};
};


#endif