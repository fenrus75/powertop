/*
 * Tests for usb_tunable serialize, good_bad(), and toggle().
 *
 * Constructor reads 4 sysfs strings: idVendor, idProduct, manufacturer, product.
 * serialize() → collect_json_fields() → result_string() → good_bad() → 1 more
 * read of power/control.  So construct + serialize = 5 records total.
 *
 * For toggle tests: construct with 4-record fixture (no serialize), then
 * swap fixture to the 1-record toggle fixture before calling toggle().
 *
 * Fixtures:
 *   usbtunable_named_auto.ptrecord — 4 ctor + 1 good_bad: named device, auto (good)
 *   usbtunable_anon_on.ptrecord    — 4 ctor + 1 good_bad: anon device, on (bad)
 *   usbtunable_toggle_from_good.ptrecord — 1 R: power/control = "auto"
 *   usbtunable_toggle_from_bad.ptrecord  — 1 R: power/control = "on"
 */
#include <string>
#include "tuning/tuningusb.h"
#include "test_framework.h"
#include "../test_helper.h"

static const std::string DATA_DIR = TEST_DATA_DIR;
static const std::string USB_BASE = "/sys/bus/usb/devices/1-2";
static const std::string USB_CTL  = USB_BASE + "/power/control";

static void test_named_device_auto()
{
test_framework_manager::get().reset();
test_framework_manager::get().set_replay(DATA_DIR + "/usbtunable_named_auto.ptrecord");

usb_tunable t(USB_BASE, "1-2");
std::string got = t.serialize();   /* calls good_bad() once → consumes record 5 */
test_framework_manager::get().reset();

/* desc uses "product [vendor]" form */
PT_ASSERT_TRUE(got.find("USB Hub") != std::string::npos);
PT_ASSERT_TRUE(got.find("Intel Corp.") != std::string::npos);
PT_ASSERT_TRUE(got.find("\"usb_path\":\"" + USB_CTL + "\"") != std::string::npos);
/* power/control == "auto" → result = "Good" */
PT_ASSERT_TRUE(got.find("\"result\":\"Good\"") != std::string::npos);
}

static void test_anon_device_on()
{
test_framework_manager::get().reset();
test_framework_manager::get().set_replay(DATA_DIR + "/usbtunable_anon_on.ptrecord");

usb_tunable t(USB_BASE, "1-2");
std::string got = t.serialize();   /* calls good_bad() once → consumes record 5 */
test_framework_manager::get().reset();

/* no manufacturer/product → desc uses "unknown USB device 1-2 (1234:abcd)" */
PT_ASSERT_TRUE(got.find("1234") != std::string::npos);
PT_ASSERT_TRUE(got.find("abcd") != std::string::npos);
/* power/control == "on" → result = "Bad" */
PT_ASSERT_TRUE(got.find("\"result\":\"Bad\"") != std::string::npos);
}

static void test_toggle_from_good()
{
/* Construct with named_auto (4 constructor reads only, 5th record not needed) */
test_framework_manager::get().reset();
test_framework_manager::get().set_replay(DATA_DIR + "/usbtunable_named_auto.ptrecord");
usb_tunable t(USB_BASE, "1-2");

/* Swap to toggle fixture: 1 R = "auto" → toggle() writes "on" */
test_framework_manager::get().reset();
test_framework_manager::get().set_replay(DATA_DIR + "/usbtunable_toggle_from_good.ptrecord");
t.toggle();

auto log = test_framework_manager::get().get_write_log();
test_framework_manager::get().reset();

PT_ASSERT_TRUE(!log.empty());
PT_ASSERT_TRUE(log[0].first == USB_CTL);
PT_ASSERT_TRUE(log[0].second == "on");
}

static void test_toggle_from_bad()
{
/* Construct with anon_on (4 constructor reads), then toggle reads "on" → writes "auto" */
test_framework_manager::get().reset();
test_framework_manager::get().set_replay(DATA_DIR + "/usbtunable_anon_on.ptrecord");
usb_tunable t(USB_BASE, "1-2");

test_framework_manager::get().reset();
test_framework_manager::get().set_replay(DATA_DIR + "/usbtunable_toggle_from_bad.ptrecord");
t.toggle();

auto log = test_framework_manager::get().get_write_log();
test_framework_manager::get().reset();

PT_ASSERT_TRUE(!log.empty());
PT_ASSERT_TRUE(log[0].first == USB_CTL);
PT_ASSERT_TRUE(log[0].second == "auto");
}

int main()
{
std::cout << "=== usb_tunable tests ===\n";
PT_RUN_TEST(test_named_device_auto);
PT_RUN_TEST(test_anon_device_on);
PT_RUN_TEST(test_toggle_from_good);
PT_RUN_TEST(test_toggle_from_bad);
return pt_test_summary();
}
