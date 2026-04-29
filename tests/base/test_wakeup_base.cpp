/*
 * Snapshot tests for wakeup base class serialize()
 *
 * Constructs wakeup directly and verifies that collect_json_fields
 * emits the correct JSON for the base-class fields:
 *   desc, score, enabled (from wakeup_value()), toggle_enable, toggle_disable
 *
 * Note: toggle_enable/toggle_disable are protected members intended for
 * derived classes to set; in the base class they default to "".
 * The ctor enable/disable params go to the display strings (wakeup_string()),
 * not to the toggle script fields.
 */
#include <string>
#include "wakeup/wakeup.h"
#include "../test_helper.h"

/* Minimal concrete subclass that exposes toggle fields for testing */
class test_wakeup : public wakeup {
public:
	test_wakeup(const std::string &d, double s,
		    const std::string &ton, const std::string &toff)
		: wakeup(d, s)
	{
		toggle_enable  = ton;
		toggle_disable = toff;
	}
};

static void test_wakeup_fields()
{
	test_wakeup w("USB mouse", 2.5, "enable_cmd", "disable_cmd");

	std::string got = w.serialize();

	PT_ASSERT_TRUE(got.find("\"desc\":\"USB mouse\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"score\":2.5") != std::string::npos);
	/* base class wakeup_value() always returns WAKEUP_DISABLE (0) */
	PT_ASSERT_TRUE(got.find("\"enabled\":false") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"toggle_enable\":\"enable_cmd\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"toggle_disable\":\"disable_cmd\"") != std::string::npos);
}

static void test_wakeup_base_toggle_empty()
{
	/* Base class leaves toggle scripts empty — derived classes fill them in */
	wakeup w("eth0", 1.0);

	std::string got = w.serialize();

	PT_ASSERT_TRUE(got.find("\"desc\":\"eth0\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"enabled\":false") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"toggle_enable\":\"\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"toggle_disable\":\"\"") != std::string::npos);
}

int main()
{
	std::cout << "=== wakeup base class tests ===\n";
	PT_RUN_TEST(test_wakeup_fields);
	PT_RUN_TEST(test_wakeup_base_toggle_empty);
	return pt_test_summary();
}
