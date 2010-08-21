#include "measurement.h"
#include "acpi.h"


#include <sys/types.h>
#include <dirent.h>

void power_meter::start_measurement(void)
{
}


void power_meter::end_measurement(void)
{
}


double power_meter::joules_consumed(void)
{
	return 0.0;
}


vector<class power_meter *> power_meters;

void start_power_measurement(void)
{
	unsigned int i;
	for (i = 0; i < power_meters.size(); i++)
		power_meters[i]->start_measurement();
}
void end_power_measurement(void)
{
	unsigned int i;
	for (i = 0; i < power_meters.size(); i++)
		power_meters[i]->end_measurement();
}

double global_joules_consumed(void)
{
	double total = 0.0;
	unsigned int i;
	for (i = 0; i < power_meters.size(); i++)
		total += power_meters[i]->joules_consumed();
	return total;
}


void detect_power_meters(void)
{
	DIR *dir;
	struct dirent *entry;
	
	dir = opendir("/proc/acpi/battery");
	if (!dir)
		return;
	while (1) {
		class acpi_power_meter *meter;
		entry = readdir(dir);
		if (!entry)
			break;
		if (entry->d_name[0] == '.')
			continue;

		meter = new class acpi_power_meter(entry->d_name);

		power_meters.push_back(meter);
		
	}
	closedir(dir);
}
