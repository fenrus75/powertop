#ifndef __INCLUDE_GUARD_MEASUREMENT_H
#define __INCLUDE_GUARD_MEASUREMENT_H

class power_meter {
public:
	virtual void start_measurement(void);
	virtual void end_measurement(void);

	virtual double joules_consumed(void);
};

#endif