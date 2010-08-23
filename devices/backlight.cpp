#include "device.h"
#include "backlight.h"

#include <string.h>


backlight::backlight(char *path)
{
	min_level = 0;
	max_level = 0;
	start_level = 0;
	strncpy(sysfs_path, path, sizeof(sysfs_path));
}

void backlight::start_measurement(void)
{

}

void backlight::end_measurement(void)
{
}


double backlight::utilization(void)
{
	double p;

	p = 100.0 * (end_level + start_level) / 2 / max_level;
	return p;
}
