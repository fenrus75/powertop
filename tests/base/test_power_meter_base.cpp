/*
 * Snapshot tests for power_meter base class serialize()
 *
 * The base class can be instantiated directly. collect_json_fields emits:
 *   name, discharging (from is_discharging()), power (from power()),
 *   capacity (from dev_capacity()).
 * All virtual methods return zero/false in the base class.
 */
#include <string>
#include "measurement/measurement.h"
#include "../test_helper.h"

static void test_power_meter_defaults()
{
	power_meter m("BAT0");

	std::string got = m.serialize();

	PT_ASSERT_TRUE(got.find("\"name\":\"BAT0\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"discharging\":false") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"power\":0") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"capacity\":0") != std::string::npos);
}

static void test_power_meter_discharging()
{
	power_meter m("BAT1");
	m.set_discharging(true);

	std::string got = m.serialize();

	PT_ASSERT_TRUE(got.find("\"name\":\"BAT1\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"discharging\":true") != std::string::npos);
}

static void test_power_meter_start_end_noop()
{
	/* start/end measurement are no-ops in the base class */
	power_meter m("AC");
	m.start_measurement();
	m.end_measurement();

	std::string got = m.serialize();

	PT_ASSERT_TRUE(got.find("\"name\":\"AC\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"power\":0") != std::string::npos);
}

int main()
{
	std::cout << "=== power_meter base class tests ===\n";
	PT_RUN_TEST(test_power_meter_defaults);
	PT_RUN_TEST(test_power_meter_discharging);
	PT_RUN_TEST(test_power_meter_start_end_noop);
	return pt_test_summary();
}
