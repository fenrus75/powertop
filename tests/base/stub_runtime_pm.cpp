/* Minimal stub for device_has_runtime_pm used in i2c_tunable tests. */
#include <string>
#include "../../src/lib.h"
#include "../../src/devices/runtime_pm.h"

bool device_has_runtime_pm(const std::string &sysfs_path)
{
/*
 * Check readability (via the `ok` out-param), not value truthiness: a
 * device that supports runtime PM but has never suspended/been active
 * yet legitimately reads 0, and must not be mistaken for "attribute
 * absent". Kept in sync with src/devices/runtime_pm.cpp's real
 * implementation.
 */
bool ok = false;

read_sysfs(std::format("{}/power/runtime_suspended_time", sysfs_path), &ok);
if (ok)
return true;

read_sysfs(std::format("{}/power/runtime_active_time", sysfs_path), &ok);
if (ok)
return true;

return false;
}
