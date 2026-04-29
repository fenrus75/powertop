/*
 * Snapshot test for alsa::serialize()
 *
 * Feeds fixed sysfs data via the replay framework, drives a full
 * start/end measurement cycle, then asserts that serialize() produces
 * a JSON object containing all expected field values.
 *
 * Fixture: data/alsa_hwC0D0.ptrecord
 *   - modelname / vendor_name: not-found (humanname stays as "alsa:hwC0D0")
 *   - start: power_off_acct=1000000  power_on_acct=500000
 *   - end:   power_off_acct=1500000  power_on_acct=600000
 */
#include <string>

#include "devices/alsa.h"
#include "test_framework.h"
#include "../test_helper.h"

/* Stub: defined in main.cpp, not linked here */
void (*ui_notify_user)(const std::string &) = nullptr;

static const std::string DATA_DIR = TEST_DATA_DIR;

static void test_alsa_serialize()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/alsa_hwC0D0.ptrecord");

	alsa dev("hwC0D0", "/sys/class/sound/hwC0D0");
	dev.start_measurement();
	dev.end_measurement();

	test_framework_manager::get().reset();

	std::string got = dev.serialize();

	/* Base class fields */
	PT_ASSERT_TRUE(got.find("\"class\":\"alsa\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"name\":\"alsa:hwC0D0\"") != std::string::npos);

	/* alsa-specific fields */
	PT_ASSERT_TRUE(got.find("\"sysfs_path\":\"/sys/class/sound/hwC0D0\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"humanname\":\"alsa:hwC0D0\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"start_inactive\":1000000") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"start_active\":500000") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"end_inactive\":1500000") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"end_active\":600000") != std::string::npos);
}

int main()
{
	PT_RUN_TEST(test_alsa_serialize);
	return pt_test_summary();
}
