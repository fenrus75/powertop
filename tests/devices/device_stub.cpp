/*
 * Minimal device base-class stub for unit tests.
 *
 * Provides the implementations of device methods that derived classes
 * (e.g. alsa) need, without pulling in the full device.cpp dependency
 * chain (report/, display/, all device subtypes, etc.).
 */
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <vector>

#include "devices/device.h"

device::device()
{
	cached_valid = false;
	hide = false;
}

void device::register_sysfs_path(const std::string &path)
{
	char resolved[PATH_MAX + 1];
	if (realpath(path.c_str(), resolved))
		real_path = resolved;
	else
		real_path.clear();
}

void device::start_measurement() { hide = false; }
void device::end_measurement() {}
double device::utilization() { return 0.0; }

void device::collect_json_fields(std::string &_js)
{
	JSON_KV("class", class_name());
	JSON_KV("name", device_name());
	JSON_FIELD(hide);
	JSON_FIELD(guilty);
	JSON_FIELD(real_path);
}

std::vector<class device *> all_devices;

/* Stubs for symbols declared in device.h that are not needed by tests */
void devices_start_measurement() {}
void devices_end_measurement() {}
void show_report_devices() {}
void report_devices() {}
void create_all_devices() {}
void clear_all_devices() {}

/* Stubs for measurement/devlist symbols pulled in transitively */
double global_power() { return 0.0; }
void save_all_results([[maybe_unused]] const std::string &filename) {}
void register_devpower([[maybe_unused]] const std::string &devstring,
                       [[maybe_unused]] double power,
                       [[maybe_unused]] class device *dev) {}
