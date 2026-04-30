/*
 * Tests for ethernet_wakeup serialize, wakeup_value, wakeup_toggle, and
 * wakeup_toggle_script.
 *
 * Constructor is pure I/O-free (string formatting only).
 * wakeup_value() reads eth_path via read_sysfs_string — 1 R record per call.
 * wakeup_toggle() calls wakeup_value() (1 R) then write_sysfs() (captured
 * in write_log, not sent to real sysfs).
 * wakeup_toggle_script() calls wakeup_value() (1 R) and returns a shell
 * script string.
 *
 * Fixtures (all 1 R record each):
 *   eth_wakeup_enabled.ptrecord        — wakeup path returns "enabled"
 *   eth_wakeup_disabled.ptrecord       — wakeup path returns "disabled"
 *   eth_toggle_from_enabled.ptrecord   — "enabled" before toggle
 *   eth_toggle_from_disabled.ptrecord  — "disabled" before toggle
 *   eth_toggle_script_enabled.ptrecord — "enabled" for script query
 *   eth_toggle_script_disabled.ptrecord— "disabled" for script query
 */
#include <string>
#include "wakeup/wakeup_ethernet.h"
#include "test_framework.h"
#include "../test_helper.h"

static const std::string DATA_DIR = TEST_DATA_DIR;
static const std::string IFACE    = "eth0";
static const std::string ETH_PATH = "/sys/class/net/eth0/device/power/wakeup";

static void test_constructor_fields()
{
	test_framework_manager::get().reset();
	ethernet_wakeup w("", IFACE);
	test_framework_manager::get().reset();

	std::string got = w.serialize();
	/* skip because wakeup::collect_json_fields calls wakeup_value() — not called here,
	   just check the pure fields via the object directly */
	PT_ASSERT_TRUE(w.interf == IFACE);
}

static void test_serialize_enabled()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/eth_wakeup_enabled.ptrecord");

	ethernet_wakeup w("", IFACE);
	std::string got = w.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("\"interf\":\"eth0\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"eth_path\":\"" + ETH_PATH + "\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"enabled\":true") != std::string::npos);
}

static void test_serialize_disabled()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/eth_wakeup_disabled.ptrecord");

	ethernet_wakeup w("", IFACE);
	std::string got = w.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("\"enabled\":false") != std::string::npos);
}

static void test_toggle_from_enabled()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/eth_toggle_from_enabled.ptrecord");

	ethernet_wakeup w("", IFACE);
	w.wakeup_toggle();   /* reads "enabled" → writes "disabled" to eth_path */

	/* save write_log before reset() clears it */
	auto log = test_framework_manager::get().get_write_log();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(!log.empty());
	PT_ASSERT_TRUE(log[0].first == ETH_PATH);
	PT_ASSERT_TRUE(log[0].second == "disabled");
}

static void test_toggle_from_disabled()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/eth_toggle_from_disabled.ptrecord");

	ethernet_wakeup w("", IFACE);
	w.wakeup_toggle();   /* reads "disabled" → writes "enabled" to eth_path */

	auto log = test_framework_manager::get().get_write_log();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(!log.empty());
	PT_ASSERT_TRUE(log[0].first == ETH_PATH);
	PT_ASSERT_TRUE(log[0].second == "enabled");
}

static void test_toggle_script_enabled()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/eth_toggle_script_enabled.ptrecord");

	ethernet_wakeup w("", IFACE);
	std::string script = w.wakeup_toggle_script();
	test_framework_manager::get().reset();

	/* currently enabled → script should disable it */
	PT_ASSERT_TRUE(script.find("disabled") != std::string::npos);
	PT_ASSERT_TRUE(script.find(ETH_PATH) != std::string::npos);
}

static void test_toggle_script_disabled()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/eth_toggle_script_disabled.ptrecord");

	ethernet_wakeup w("", IFACE);
	std::string script = w.wakeup_toggle_script();
	test_framework_manager::get().reset();

	/* currently disabled → script should enable it */
	PT_ASSERT_TRUE(script.find("enabled") != std::string::npos);
	PT_ASSERT_TRUE(script.find(ETH_PATH) != std::string::npos);
}

int main()
{
	std::cout << "=== ethernet_wakeup tests ===\n";
	PT_RUN_TEST(test_constructor_fields);
	PT_RUN_TEST(test_serialize_enabled);
	PT_RUN_TEST(test_serialize_disabled);
	PT_RUN_TEST(test_toggle_from_enabled);
	PT_RUN_TEST(test_toggle_from_disabled);
	PT_RUN_TEST(test_toggle_script_enabled);
	PT_RUN_TEST(test_toggle_script_disabled);
	return pt_test_summary();
}
