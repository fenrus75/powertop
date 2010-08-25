#ifndef __INCLUDE_GUARD_MEASUREMENT_H
#define __INCLUDE_GUARD_MEASUREMENT_H

#include <vector>

using namespace std;

class power_meter {
public:
	virtual void start_measurement(void);
	virtual void end_measurement(void);

	virtual double joules_consumed(void);
};


extern vector<class power_meter *> power_meters;

extern void start_power_measurement(void);
extern void end_power_measurement(void);
extern double global_joules_consumed(void);
extern void detect_power_meters(void);

extern double min_power;

#endif