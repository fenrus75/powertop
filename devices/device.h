#ifndef _INCLUDE_GUARD_DEVICE_H
#define _INCLUDE_GUARD_DEVICE_H


#include <vector>

class device {

public:

	virtual void start_measurement(void);
	virtual void end_measurement(void);

	virtual double	utilization(void); /* percentage */

	virtual const char * class_name(void) { return "abstract device";};

};

using namespace std;

extern vector<class device *> all_devices;
#endif