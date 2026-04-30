/*
 * Snapshot tests for thinkpad_fan and thinkpad_light.
 *
 * Both classes follow the same pattern:
 *   - Constructor: no sysfs I/O (just parameter/result index registration)
 *   - start_measurement(): 1 read_sysfs
 *   - end_measurement():   1 read_sysfs + report_utilization() side-effect
 *   - utilization():       pure arithmetic on start_rate / end_rate
 *
 * Fixtures built with trace_tool.py add:
 *   thinkpad_fan_cycle.ptrecord   — fan1_input: 1800, 2400 rpm
 *   thinkpad_light_cycle.ptrecord — brightness: 100, 155
 *
 * Utilization formulas:
 *   fan:   (start + end) / 2            → (1800 + 2400) / 2 = 2100.0
 *   light: (start + end) / 2.55 / 2.0  → (100 + 155) / 2.55 / 2.0 ≈ 50.0
 */
#include <string>
#include "devices/thinkpad-fan.h"
#include "devices/thinkpad-light.h"
#include "test_framework.h"
#include "../test_helper.h"

static const std::string DATA_DIR = TEST_DATA_DIR;

/* ── thinkpad_fan ──────────────────────────────────────────────── */

static void test_fan_constructor()
{
	test_framework_manager::get().reset();
	thinkpad_fan f;
	std::string got = f.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("\"start_rate\":0") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"end_rate\":0") != std::string::npos);
	/* class_name and human_name are virtual; verify device_name indirectly via class_name */
	PT_ASSERT_TRUE(f.class_name() == "fan");
}

static void test_fan_measurement_cycle()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/thinkpad_fan_cycle.ptrecord");

	thinkpad_fan f;
	f.start_measurement();   /* reads fan1_input = 1800 */
	f.end_measurement();     /* reads fan1_input = 2400 */
	std::string got = f.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("\"start_rate\":1800") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"end_rate\":2400") != std::string::npos);

	/* utilization = (1800 + 2400) / 2 = 2100.0 */
	double util = f.utilization();
	PT_ASSERT_TRUE(util > 2099.9 && util < 2100.1);
}

/* ── thinkpad_light ────────────────────────────────────────────── */

static void test_light_constructor()
{
	test_framework_manager::get().reset();
	thinkpad_light l;
	std::string got = l.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("\"start_rate\":0") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"end_rate\":0") != std::string::npos);
	PT_ASSERT_TRUE(l.class_name() == "light");
}

static void test_light_measurement_cycle()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/thinkpad_light_cycle.ptrecord");

	thinkpad_light l;
	l.start_measurement();   /* reads brightness = 100 */
	l.end_measurement();     /* reads brightness = 155 */
	std::string got = l.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("\"start_rate\":100") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"end_rate\":155") != std::string::npos);

	/* utilization = (100 + 155) / 2.55 / 2.0 = 255 / 5.1 = 50.0 */
	double util = l.utilization();
	PT_ASSERT_TRUE(util > 49.9 && util < 50.1);
}

int main()
{
	std::cout << "=== thinkpad fan/light serialize tests ===\n";
	PT_RUN_TEST(test_fan_constructor);
	PT_RUN_TEST(test_fan_measurement_cycle);
	PT_RUN_TEST(test_light_constructor);
	PT_RUN_TEST(test_light_measurement_cycle);
	return pt_test_summary();
}
