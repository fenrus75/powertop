
#include "powerconsumer.h"
#include "process.h"
#include "../parameters/parameters.h"

double power_consumer::Witts(void)
{
	double cost;
	double timecost;
	double wakeupcost;

	if (child_runtime > accumulated_runtime)
		child_runtime = 0;

	timecost = get_parameter_value("cpu-consumption");
	wakeupcost = get_parameter_value("cpu-wakeups");

	cost = wakeupcost * wake_ups / 10000.0;
	cost += ( (accumulated_runtime - child_runtime) / 1000000000.0 * timecost);

	cost = cost / measurement_time;

	return cost;
}

power_consumer::power_consumer(void)
{
	accumulated_runtime = 0;
	child_runtime = 0;
	disk_hits = 0;
	wake_ups = 0;
	gpu_ops = 0;
	waker = NULL;
}