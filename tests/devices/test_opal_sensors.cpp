/*
 * Tests for opal_sensors_power_meter power() and serialize().
 *
 * Constructor stores path as 'name', no I/O.
 * power() reads one sysfs integer (in µW) and divides by 1000000.
 * serialize() calls power() once via collect_json_fields().
 *
 * Fixtures:
 *   opal_power_42w.ptrecord — 1 R: "42000000" → 42.0 W
 *   opal_power_zero.ptrecord — 1 N: missing → ok=false → 0.0 W
 */
#include <string>
#include <cmath>
#include "measurement/opal-sensors.h"
#include "test_framework.h"
#include "../test_helper.h"

static const std::string DATA_DIR = TEST_DATA_DIR;
static const std::string SENSOR  = "/sys/firmware/opal/sensors/voltage_sensor/supply-0/in0_input";

static void test_power_good()
{
test_framework_manager::get().reset();
test_framework_manager::get().set_replay(DATA_DIR + "/opal_power_42w.ptrecord");

opal_sensors_power_meter m(SENSOR);
double p = m.power();
test_framework_manager::get().reset();

PT_ASSERT_TRUE(std::fabs(p - 42.0) < 0.001);
}

static void test_power_missing()
{
test_framework_manager::get().reset();
test_framework_manager::get().set_replay(DATA_DIR + "/opal_power_zero.ptrecord");

opal_sensors_power_meter m(SENSOR);
double p = m.power();
test_framework_manager::get().reset();

PT_ASSERT_TRUE(p == 0.0);
}

static void test_serialize()
{
test_framework_manager::get().reset();
test_framework_manager::get().set_replay(DATA_DIR + "/opal_power_42w.ptrecord");

opal_sensors_power_meter m(SENSOR);
std::string got = m.serialize();
test_framework_manager::get().reset();

/* name field should be the sensor path */
PT_ASSERT_TRUE(got.find("\"name\":\"" + SENSOR + "\"") != std::string::npos);
/* power field */
PT_ASSERT_TRUE(got.find("\"power\":42") != std::string::npos);
/* discharging defaults to false */
	PT_ASSERT_TRUE(got.find("\"discharging\":false") != std::string::npos);
}

int main()
{
std::cout << "=== opal_sensors_power_meter tests ===\n";
PT_RUN_TEST(test_power_good);
PT_RUN_TEST(test_power_missing);
PT_RUN_TEST(test_serialize);
return pt_test_summary();
}
