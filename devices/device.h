#ifndef _INCLUDE_GUARD_DEVICE_H
#define _INCLUDE_GUARD_DEVICE_H


#include <vector>


struct parameter_bundle;
struct result_bundle;

class device {
public:

	virtual void start_measurement(void);
	virtual void end_measurement(void);

	virtual double	utilization(void); /* percentage */

	virtual const char * util_units(void) { return "%"; };

	virtual const char * class_name(void) { return "abstract device";};
	virtual const char * device_name(void) { return "abstract device";};

	virtual const char * human_name(void) { return device_name(); };

	virtual double power_usage(struct result_bundle *results, struct parameter_bundle *bundle) { return 0.0; };

	virtual bool show_in_list(void) {return true;};

	virtual int power_valid(void) { return 1;};
};

using namespace std;

extern vector<class device *> all_devices;

extern void devices_start_measurement(void);
extern void devices_end_measurement(void);
extern void report_devices(void);


extern void create_all_devices(void);

#endif