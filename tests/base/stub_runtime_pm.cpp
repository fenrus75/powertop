/* Minimal stub for device_has_runtime_pm used in i2c_tunable tests. */
#include <string>
#include "../../src/lib.h"
#include "../../src/devices/runtime_pm.h"

bool device_has_runtime_pm(const std::string &sysfs_path)
{
if (read_sysfs(std::format("{}/power/runtime_suspended_time", sysfs_path)))
return true;
if (read_sysfs(std::format("{}/power/runtime_active_time", sysfs_path)))
return true;
return false;
}
