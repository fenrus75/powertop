/*
 * Method tests for sysfs_tunable: good_bad() and toggle()
 *
 * good_bad() reads the sysfs path and returns TUNE_GOOD/TUNE_BAD:
 *   - match target_value → TUNE_GOOD
 *   - mismatch → TUNE_BAD, sets bad_value and toggle_bad script
 *
 * toggle() calls good_bad() internally then writes via write_sysfs():
 *   - currently bad → writes target_value
 *   - currently good with prior bad_value set → writes bad_value back
 *   - currently good with no prior bad_value → no write
 *
 * write_sysfs() is mockable; writes are captured in the framework's
 * write_log without needing W records in the fixture.
 *
 * Tests:
 *  good_bad_currently_bad       — TUNE_BAD, toggle_bad script set
 *  good_bad_currently_good      — TUNE_GOOD
 *  toggle_from_bad              — writes target "deep" to sysfs
 *  toggle_good_with_prior_bad   — good_bad sets bad_value, then toggle writes it back
 */
#include <string>
#include "tuning/tuningsysfs.h"
#include "test_framework.h"
#include "../test_helper.h"

static const std::string DATA_DIR = TEST_DATA_DIR;
static const std::string PATH = "/sys/power/mem_sleep";
static const std::string TARGET = "deep";

/* Expose toggle_bad (protected in tunable base) for assertions */
class test_sysfs_tunable : public sysfs_tunable {
public:
	test_sysfs_tunable(const std::string &desc, const std::string &path,
	                   const std::string &target)
	    : sysfs_tunable(desc, path, target) {}

	const std::string& get_toggle_bad() const { return toggle_bad; }
};

static void test_good_bad_currently_bad()
{
	test_framework_manager::get().reset();
	/* Two R records: one for good_bad() call, one for result_string() inside serialize() */
	test_framework_manager::get().set_replay(DATA_DIR + "/tunable_currently_bad.ptrecord");

	test_sysfs_tunable t("Sleep mode", PATH, TARGET);
	int result = t.good_bad();

	PT_ASSERT_EQ(result, TUNE_BAD);
	PT_ASSERT_TRUE(t.get_toggle_bad().find("s2idle") != std::string::npos);
	PT_ASSERT_TRUE(t.get_toggle_bad().find(PATH) != std::string::npos);
	test_framework_manager::get().reset();
}

static void test_good_bad_currently_good()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/tunable_currently_good.ptrecord");

	test_sysfs_tunable t("Sleep mode", PATH, TARGET);
	int result = t.good_bad();
	test_framework_manager::get().reset();

	PT_ASSERT_EQ(result, TUNE_GOOD);
}

static void test_toggle_from_bad()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/tunable_currently_bad.ptrecord");

	test_sysfs_tunable t("Sleep mode", PATH, TARGET);
	t.toggle();  /* reads s2idle (bad) → writes TARGET "deep" */

	auto& log = test_framework_manager::get().get_write_log();
	PT_ASSERT_EQ((int)log.size(), 1);
	PT_ASSERT_EQ(log[0].first,  PATH);
	PT_ASSERT_EQ(log[0].second, TARGET);
	test_framework_manager::get().reset();
}

static void test_toggle_good_with_prior_bad()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/tunable_toggle_good_prior_bad.ptrecord");

	test_sysfs_tunable t("Sleep mode", PATH, TARGET);
	/* First call sets bad_value = "s2idle" */
	t.good_bad();
	/* Second call (inside toggle) reads "deep" → TUNE_GOOD → writes bad_value */
	t.toggle();

	auto& log = test_framework_manager::get().get_write_log();
	PT_ASSERT_EQ((int)log.size(), 1);
	PT_ASSERT_EQ(log[0].first,  PATH);
	PT_ASSERT_EQ(log[0].second, std::string("s2idle"));
	test_framework_manager::get().reset();
}

int main()
{
	std::cout << "=== sysfs_tunable method tests ===\n";
	PT_RUN_TEST(test_good_bad_currently_bad);
	PT_RUN_TEST(test_good_bad_currently_good);
	PT_RUN_TEST(test_toggle_from_bad);
	PT_RUN_TEST(test_toggle_good_with_prior_bad);
	return pt_test_summary();
}
