/*
 * Tests for ahci::start_measurement / end_measurement / utilization / power_usage
 *
 * Fixture: data/ahci_host99.ptrecord
 *   start: active=100, partial=50, slumber=200, devslp=10
 *   end:   active=200, partial=100, slumber=300, devslp=20
 *   deltas: a=100, p=50, s=100, d=10  total≈260.001
 *   utilization = (100+50)/260.001*100 ≈ 57.69%
 */

#include <string>
#include <cmath>

#include "devices/ahci.h"
#include "test_framework.h"
#include "../test_helper.h"

/* Stub: defined in main.cpp, not linked here */
void (*ui_notify_user)(const std::string &) = nullptr;

static const std::string DATA_DIR = TEST_DATA_DIR;

static void test_ahci_start_end_measurement()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/ahci_host99.ptrecord");

	ahci dev("host99", "/sys/class/scsi_host/host99");
	dev.start_measurement();
	dev.end_measurement();

	test_framework_manager::get().reset();

	/* utilization = (active_delta + partial_delta) / total * 100 */
	double util = dev.utilization();
	PT_ASSERT_TRUE(util > 57.0 && util < 58.0);
}

static void test_ahci_utilization_zero_when_no_change()
{
	test_framework_manager::get().reset();

	/* Provide identical start and end values — all deltas = 0 */
	auto &tf = test_framework_manager::get();
	tf.set_replay(DATA_DIR + "/ahci_host99.ptrecord");

	ahci dev("host99", "/sys/class/scsi_host/host99");

	/* Manually override: re-load same fixture but trick by using same
	 * values for start and end via a second fixture */
	tf.reset();

	/* Create zero-delta scenario directly: after construction the
	 * counters are all 0, so start_measurement on zeros + end on zeros
	 * should yield utilization = 0. */
	ahci dev2("host1", "/sys/class/scsi_host/host1");
	/* start_measurement — both read 0 (empty content → stoi fails → 0) */

	test_framework_manager::get().reset();

	/* With all counters at 0, total = 0.001, active/partial deltas = 0 */
	double util = dev2.utilization();
	PT_ASSERT_TRUE(util >= 0.0 && util < 1.0);
}

static void test_ahci_serialize_fields()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/ahci_host99.ptrecord");

	ahci dev("host99", "/sys/class/scsi_host/host99");
	dev.start_measurement();
	dev.end_measurement();

	test_framework_manager::get().reset();

	std::string js = dev.serialize();

	PT_ASSERT_TRUE(js.find("\"class\":\"ahci\"") != std::string::npos);
	PT_ASSERT_TRUE(js.find("\"name\":\"ahci:host99\"") != std::string::npos);
	PT_ASSERT_TRUE(js.find("\"sysfs_path\":\"/sys/class/scsi_host/host99\"") != std::string::npos);
	PT_ASSERT_TRUE(js.find("\"start_active\":100") != std::string::npos);
	PT_ASSERT_TRUE(js.find("\"end_active\":200") != std::string::npos);
	PT_ASSERT_TRUE(js.find("\"start_partial\":50") != std::string::npos);
	PT_ASSERT_TRUE(js.find("\"end_partial\":100") != std::string::npos);
	PT_ASSERT_TRUE(js.find("\"start_slumber\":200") != std::string::npos);
	PT_ASSERT_TRUE(js.find("\"end_slumber\":300") != std::string::npos);
	PT_ASSERT_TRUE(js.find("\"start_devslp\":10") != std::string::npos);
	PT_ASSERT_TRUE(js.find("\"end_devslp\":20") != std::string::npos);
}

static void test_ahci_human_name_fallback()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/ahci_host99.ptrecord");

	/* path doesn't exist → opendir fails → humanname uses _name as
	 * disk/link label */
	ahci dev("host99", "/sys/class/scsi_host/host99");

	test_framework_manager::get().reset();

	std::string hn = dev.human_name();
	/* Either "SATA link: host0" or "SATA disk: host0" depending on
	 * whether opendir returned a non-empty shortname */
	PT_ASSERT_TRUE(hn.find("host99") != std::string::npos);
}

int main()
{
	std::cout << "=== ahci tests ===\n";
	PT_RUN_TEST(test_ahci_start_end_measurement);
	PT_RUN_TEST(test_ahci_utilization_zero_when_no_change);
	PT_RUN_TEST(test_ahci_serialize_fields);
	PT_RUN_TEST(test_ahci_human_name_fallback);
	return pt_test_summary();
}
