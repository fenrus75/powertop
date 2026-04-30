/*
 * Snapshot tests for rfkill::serialize()
 *
 * The rfkill constructor makes exactly two pt_readlink() calls:
 *   1. {path}/device/driver        — sets humanname if non-empty
 *   2. {path}/device/device/driver — overrides humanname if non-empty
 *
 * Three scenarios exercise the symlink logic:
 *   no_driver           — both return "": humanname stays empty
 *   device_driver       — first returns a path: humanname = "Radio device: ath9k_htc"
 *   device_device_driver — second returns a path (overrides): humanname = "Radio device: iwlwifi"
 *
 * After construction, serialize() is called with no further sysfs reads.
 */
#include <string>
#include "devices/rfkill.h"
#include "test_framework.h"
#include "../test_helper.h"

static const std::string DATA_DIR = TEST_DATA_DIR;

static void test_no_driver()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/rfkill_no_driver.ptrecord");

	rfkill r("rfkill0", "/sys/class/rfkill/rfkill0");
	std::string got = r.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("\"humanname\":\"\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"sysfs_path\":\"/sys/class/rfkill/rfkill0\"") != std::string::npos);
}

static void test_device_driver()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/rfkill_device_driver.ptrecord");

	rfkill r("rfkill0", "/sys/class/rfkill/rfkill0");
	std::string got = r.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("\"humanname\":\"Radio device: ath9k_htc\"") != std::string::npos);
}

static void test_device_device_driver()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/rfkill_device_device_driver.ptrecord");

	rfkill r("rfkill0", "/sys/class/rfkill/rfkill0");
	std::string got = r.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("\"humanname\":\"Radio device: iwlwifi\"") != std::string::npos);
}

static const std::string PATH = "/sys/class/rfkill/rfkill0";

static void test_measurement_not_blocked()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/rfkill_not_blocked.ptrecord");

	rfkill r("rfkill0", PATH);
	r.start_measurement();
	r.end_measurement();
	std::string got = r.serialize();
	test_framework_manager::get().reset();

	/* start and end both read hard=0, soft=0 */
	PT_ASSERT_TRUE(got.find("\"start_hard\":0") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"start_soft\":0") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"end_hard\":0") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"end_soft\":0") != std::string::npos);
	/* utilization: rfk=0, p = 100 - 50*0 = 100% */
	double util = r.utilization();
	PT_ASSERT_TRUE(util > 99.9 && util <= 100.0);
}

static void test_measurement_blocked()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/rfkill_blocked.ptrecord");

	rfkill r("rfkill0", PATH);
	r.start_measurement();
	r.end_measurement();
	std::string got = r.serialize();
	test_framework_manager::get().reset();

	/* start: hard=0, soft=0; end: hard=1, soft=1 */
	PT_ASSERT_TRUE(got.find("\"start_hard\":0") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"start_soft\":0") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"end_hard\":1") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"end_soft\":1") != std::string::npos);
	/* utilization: rfk = max(0+1, 0+1) = 1? no: rfk=start_soft+end_soft=1,
	   start_hard+end_hard=1, max(1,1)=1, p = 100 - 50*1 = 50 */
	double util = r.utilization();
	PT_ASSERT_TRUE(util > 49.9 && util < 50.1);
}

int main()
{
	std::cout << "=== rfkill serialize tests ===\n";
	PT_RUN_TEST(test_no_driver);
	PT_RUN_TEST(test_device_driver);
	PT_RUN_TEST(test_device_device_driver);
	PT_RUN_TEST(test_measurement_not_blocked);
	PT_RUN_TEST(test_measurement_blocked);
	return pt_test_summary();
}
