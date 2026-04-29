/*
 * Snapshot tests for acpi_power_meter::serialize()
 *
 * Tests the power_meter / measurement hierarchy.
 * measure() has three distinct code paths, each tested separately:
 *
 *  Charging:        status != "Discharging" — measure() returns early,
 *                   rate/capacity/voltage all stay 0.
 *
 *  power_now path:  status == "Discharging", power_now present (µW) —
 *                   rate = power_now / 1e6.
 *                   15000000 µW = 15.0 W.
 *
 *  Fallback path:   status == "Discharging", power_now absent (N record),
 *                   current_now × voltage_now (µA × µV / 1e12) used.
 *                   2000000 µA × 12000000 µV / 1e12 = 24.0 W.
 */
#include <string>
#include <cmath>

#include "measurement/acpi.h"
#include "test_framework.h"
#include "../test_helper.h"

/* Stub: defined in main.cpp, not linked here */
void (*ui_notify_user)(const std::string &) = nullptr;

static const std::string DATA_DIR = TEST_DATA_DIR;

/* ── charging: all fields zero ─────────────────────────────────────────── */

static void test_charging()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/acpi_charging.ptrecord");

	acpi_power_meter m("BAT0");
	m.start_measurement();
	m.end_measurement();

	std::string got = m.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("\"name\":\"BAT0\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"discharging\":false") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"rate\":0") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"capacity\":0") != std::string::npos);
}

/* ── discharging via power_now ─────────────────────────────────────────── */

static void test_power_now()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/acpi_discharging_power_now.ptrecord");

	acpi_power_meter m("BAT0");
	m.start_measurement();
	m.end_measurement();

	std::string got = m.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("\"name\":\"BAT0\"") != std::string::npos);
	/* rate = 15000000 µW / 1e6 = 15.0 W */
	PT_ASSERT_TRUE(got.find("\"rate\":15") != std::string::npos);
}

/* ── discharging via current_now × voltage_now fallback ────────────────── */

static void test_fallback()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/acpi_discharging_fallback.ptrecord");

	acpi_power_meter m("BAT0");
	m.start_measurement();
	m.end_measurement();

	std::string got = m.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("\"name\":\"BAT0\"") != std::string::npos);
	/* rate = (2000000 µA × 12000000 µV) / 1e12 = 24.0 W */
	PT_ASSERT_TRUE(got.find("\"rate\":24") != std::string::npos);
}

int main()
{
	PT_RUN_TEST(test_charging);
	PT_RUN_TEST(test_power_now);
	PT_RUN_TEST(test_fallback);
	return pt_test_summary();
}
