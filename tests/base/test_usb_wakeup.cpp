/*
 * Snapshot tests for usb_wakeup::serialize()
 *
 * The constructor sets usb_path and toggle scripts with no sysfs reads.
 * wakeup_value() reads usb_path via read_sysfs_string — called once per
 * serialize() via wakeup::collect_json_fields.
 *
 * collect_json_fields emits (base + derived):
 *   desc, score, enabled (wakeup_value()==WAKEUP_ENABLE), toggle_enable,
 *   toggle_disable, usb_path, interf
 *
 * Two scenarios: enabled and disabled state.
 */
#include <string>
#include "wakeup/wakeup_usb.h"
#include "test_framework.h"
#include "../test_helper.h"

static const std::string DATA_DIR = TEST_DATA_DIR;

static void test_wakeup_enabled()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/usb_wakeup_enabled.ptrecord");

	usb_wakeup w("", "1-1");
	std::string got = w.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("\"interf\":\"1-1\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"usb_path\":\"/sys/bus/usb/devices/1-1/power/wakeup\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"enabled\":true") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"toggle_enable\":") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"toggle_disable\":") != std::string::npos);
}

static void test_wakeup_disabled()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/usb_wakeup_disabled.ptrecord");

	usb_wakeup w("", "1-1");
	std::string got = w.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("\"interf\":\"1-1\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"enabled\":false") != std::string::npos);
}

int main()
{
	std::cout << "=== usb_wakeup serialize tests ===\n";
	PT_RUN_TEST(test_wakeup_enabled);
	PT_RUN_TEST(test_wakeup_disabled);
	return pt_test_summary();
}
