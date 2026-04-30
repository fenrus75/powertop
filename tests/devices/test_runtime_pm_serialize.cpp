/*
 * Snapshot tests for runtime_pmdevice::serialize()
 *
 * Constructor is pure (no sysfs reads).
 * start_measurement() reads: runtime_suspended_time, runtime_active_time  (2 reads)
 * end_measurement()   reads: runtime_suspended_time, runtime_active_time  (2 reads)
 * Total: 4 fixture records per scenario, in that order.
 *
 * utilization() = 100 * (after_active - before_active)
 *                      / (0.0001 + after_active - before_active
 *                               + after_suspended - before_suspended)
 *
 * Three scenarios:
 *   idle   — suspended the whole interval: susp 100→200, active 0→0  → util=0%
 *   active — active   the whole interval: susp 0→0,   active 100→200 → util≈100%
 *   mixed  — 50/50 split: susp 0→100, active 100→200                 → util≈50%
 */
#include <string>
#include "devices/runtime_pm.h"
#include "test_framework.h"
#include "../test_helper.h"

static const std::string DATA_DIR = TEST_DATA_DIR;
static const std::string SYSFS = "/sys/bus/pci/devices/0000:00:1f.0";

static void test_idle()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/runtime_pm_idle.ptrecord");

	runtime_pmdevice d("test-dev", SYSFS);
	d.start_measurement();
	d.end_measurement();
	std::string got = d.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("\"name\":\"test-dev\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"humanname\":\"runtime-test-dev\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"sysfs_path\":\"" + SYSFS + "\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"before_suspended_time\":100") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"before_active_time\":0") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"after_suspended_time\":200") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"after_active_time\":0") != std::string::npos);
}

static void test_active()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/runtime_pm_active.ptrecord");

	runtime_pmdevice d("test-dev", SYSFS);
	d.start_measurement();
	d.end_measurement();
	std::string got = d.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("\"before_active_time\":100") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"after_active_time\":200") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"before_suspended_time\":0") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"after_suspended_time\":0") != std::string::npos);
}

static void test_mixed()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/runtime_pm_mixed.ptrecord");

	runtime_pmdevice d("test-dev", SYSFS);
	d.start_measurement();
	d.end_measurement();
	std::string got = d.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("\"before_active_time\":100") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"after_active_time\":200") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"before_suspended_time\":0") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"after_suspended_time\":100") != std::string::npos);
}

int main()
{
	std::cout << "=== runtime_pmdevice serialize tests ===\n";
	PT_RUN_TEST(test_idle);
	PT_RUN_TEST(test_active);
	PT_RUN_TEST(test_mixed);
	return pt_test_summary();
}
