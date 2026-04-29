/*
 * Snapshot tests for tunable base class serialize()
 *
 * The base class good_bad() returns TUNE_NEUTRAL so result_string()
 * returns the neutral display string. toggle_good/toggle_bad are
 * protected and intended for derived classes; we expose them via
 * a minimal test subclass.
 */
#include <string>
#include "tuning/tunable.h"
#include "../test_helper.h"

/* Minimal concrete subclass that exposes toggle fields for testing */
class test_tunable : public tunable {
public:
	test_tunable(const std::string &d, double s,
		     const std::string &tgood, const std::string &tbad)
		: tunable(d, s, "Good", "Bad", "Unknown")
	{
		toggle_good = tgood;
		toggle_bad  = tbad;
	}
};

static void test_tunable_fields()
{
	test_tunable t("vm.dirty_writeback_centisecs", 3.0,
		       "echo 1500 > /proc/sys/vm/dirty_writeback_centisecs",
		       "echo 500 > /proc/sys/vm/dirty_writeback_centisecs");

	std::string got = t.serialize();

	PT_ASSERT_TRUE(got.find("\"desc\":\"vm.dirty_writeback_centisecs\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"score\":3") != std::string::npos);
	/* base good_bad() == TUNE_NEUTRAL → neutral_string */
	PT_ASSERT_TRUE(got.find("\"result\":\"Unknown\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"toggle_good\":\"echo 1500") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"toggle_bad\":\"echo 500") != std::string::npos);
}

static void test_tunable_default_ctor()
{
	/* Default ctor sets good/bad/neutral display strings via i18n */
	tunable t;
	t.desc  = "test_setting";
	t.score = 1.5;

	std::string got = t.serialize();

	PT_ASSERT_TRUE(got.find("\"desc\":\"test_setting\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"score\":1.5") != std::string::npos);
	/* toggle_good/bad are empty strings in the base class */
	PT_ASSERT_TRUE(got.find("\"toggle_good\":\"\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"toggle_bad\":\"\"") != std::string::npos);
}

int main()
{
	std::cout << "=== tunable base class tests ===\n";
	PT_RUN_TEST(test_tunable_fields);
	PT_RUN_TEST(test_tunable_default_ctor);
	return pt_test_summary();
}
