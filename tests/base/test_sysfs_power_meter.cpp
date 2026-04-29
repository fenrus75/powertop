/*
 * Snapshot tests for sysfs_power_meter::serialize()
 *
 * sysfs_power_meter::measure() is called by end_measurement() and performs
 * the following sequential sysfs reads:
 *   present, status, power_now, energy_now
 *   (fallback: voltage_now, current_now, charge_now)
 *
 * Each call to end_measurement() consumes one set of fixture records.
 *
 * Three scenarios:
 *   charging         — status=Charging; rate=15W, capacity=129600J
 *   discharging_direct  — status=Discharging, power_now + energy_now
 *   discharging_fallback — status=Discharging, no power_now/energy_now;
 *                          uses current×voltage and charge×voltage
 *
 * Expected values:
 *   power_now=15000000µW  → rate=15.0W
 *   energy_now=36000000µWh → capacity=36*3600=129600J
 *   voltage_now=12000000µV → 12.0V
 *   current_now=2000000µA  → 2.0A → rate=24.0W
 *   charge_now=3000000µAh  → 3.0Ah → capacity=3*12*3600=129600J
 */
#include <string>
#include "measurement/sysfs.h"
#include "test_framework.h"
#include "../test_helper.h"
#include <cmath>

static const std::string DATA_DIR = TEST_DATA_DIR;

static void test_charging()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/sysfs_charging.ptrecord");

	sysfs_power_meter m("BAT0");
	m.end_measurement();

	std::string got = m.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("\"name\":\"BAT0\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"discharging\":false") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"rate\":15") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"capacity\":129600") != std::string::npos);
}

static void test_discharging_direct()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/sysfs_discharging_direct.ptrecord");

	sysfs_power_meter m("BAT0");
	m.end_measurement();

	std::string got = m.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("\"discharging\":true") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"rate\":15") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"capacity\":129600") != std::string::npos);
}

static void test_discharging_fallback()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/sysfs_discharging_fallback.ptrecord");

	sysfs_power_meter m("BAT0");
	m.end_measurement();

	std::string got = m.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("\"discharging\":true") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"rate\":24") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"capacity\":129600") != std::string::npos);
}

int main()
{
	std::cout << "=== sysfs_power_meter serialize tests ===\n";
	PT_RUN_TEST(test_charging);
	PT_RUN_TEST(test_discharging_direct);
	PT_RUN_TEST(test_discharging_fallback);
	return pt_test_summary();
}
