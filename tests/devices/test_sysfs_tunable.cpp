/*
 * Snapshot tests for sysfs_tunable::serialize()
 *
 * sysfs_tunable has two distinct states worth testing:
 *
 *  Good state: the sysfs file already holds the target value.
 *              bad_value and toggle_bad are empty; result is "Good".
 *
 *  Bad  state: the sysfs file holds a different value.
 *              After good_bad() is called, bad_value and toggle_bad
 *              are populated from the actual file content.
 *
 * Note: serialize() calls result_string() which calls good_bad() which
 * reads the sysfs path — so each serialize() call consumes one replay
 * entry.  The "bad" fixture provides two entries: one for the explicit
 * good_bad() call that populates bad_value, and one for the serialize()
 * call that follows.
 */
#include <string>

#include "tuning/tuningsysfs.h"
#include "test_framework.h"
#include "../test_helper.h"

/* Stub: defined in main.cpp, not linked here */
void (*ui_notify_user)(const std::string &) = nullptr;

static const std::string DATA_DIR = TEST_DATA_DIR;

static const std::string PATH =
	"/sys/class/scsi_host/host0/link_power_management_policy";
static const std::string TARGET = "med_power_with_dipm";
static const std::string DESC   = "Enable SATA link power management for host0";

/* ── good state ────────────────────────────────────────────────────────── */

static void test_good_state()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/sysfs_tunable_good.ptrecord");

	sysfs_tunable t(DESC, PATH, TARGET);

	std::string got = t.serialize();
	test_framework_manager::get().reset();

	/* base tunable fields */
	PT_ASSERT_TRUE(got.find("\"desc\":\"" + DESC + "\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"score\":1") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"result\":\"Good\"") != std::string::npos);

	/* toggle_good set at construction; toggle_bad empty */
	PT_ASSERT_TRUE(got.find("\"toggle_good\":\"echo 'med_power_with_dipm'") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"toggle_bad\":\"\"") != std::string::npos);

	/* sysfs_tunable-specific fields */
	PT_ASSERT_TRUE(got.find("\"sysfs_path\":\"" + PATH + "\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"target_value\":\"" + TARGET + "\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"bad_value\":\"\"") != std::string::npos);
}

/* ── bad state ─────────────────────────────────────────────────────────── */

static void test_bad_state()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/sysfs_tunable_bad.ptrecord");

	sysfs_tunable t(DESC, PATH, TARGET);

	/* first read: populates bad_value and toggle_bad */
	int state = t.good_bad();
	PT_ASSERT_EQ(state, TUNE_BAD);

	/* second read: consumed by serialize() → result_string() → good_bad() */
	std::string got = t.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("\"result\":\"Bad\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"bad_value\":\"max_performance\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"toggle_bad\":\"echo 'max_performance'") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"target_value\":\"" + TARGET + "\"") != std::string::npos);
}

int main()
{
	PT_RUN_TEST(test_good_state);
	PT_RUN_TEST(test_bad_state);
	return pt_test_summary();
}
