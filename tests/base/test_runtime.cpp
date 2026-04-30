/*
 * Tests for tuning/runtime.cpp — runtime_tunable class methods.
 *
 * Fixtures all use non-existent sysfs paths (testdev-99, 0000:99:00.0)
 * to avoid real filesystem hits.
 *
 * device_has_runtime_pm reads:
 *   {path}/power/runtime_suspended_time  (if non-zero → has PM)
 *   {path}/power/runtime_active_time     (if first is 0)
 *
 * Good/bad/toggle reads:
 *   {path}/power/control  → "auto" (TUNE_GOOD) or "on" (TUNE_BAD)
 *
 * Writes are captured in write_log without needing W fixture records.
 */

#include <string>

#include "tuning/runtime.h"
#include "tuning/tunable.h"
#include "test_framework.h"
#include "../test_helper.h"

static const std::string DATA_DIR = TEST_DATA_DIR;

static const std::string USB_PATH = "/sys/bus/usb/devices/testdev-99";
static const std::string USB_CTL  = USB_PATH + "/power/control";
static const std::string PCI_PATH = "/sys/bus/pci/devices/0000:99:00.0";

/* ── constructor: USB, no runtime PM ──────────────────────────────── */

static void test_runtime_usb_no_pm_desc()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/runtime_usb_no_pm.ptrecord");

	runtime_tunable t(USB_PATH, "usb", "testdev-99", "");
	test_framework_manager::get().reset();

	/* device_has_runtime_pm → false → desc uses "has no runtime power management" */
	std::string got = t.serialize();
	PT_ASSERT_TRUE(got.find("no runtime power management") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"runtime_path\":\"" + USB_CTL + "\"") != std::string::npos);
}

/* ── constructor: USB, has runtime PM via suspended_time ──────────── */

static void test_runtime_usb_has_pm_desc()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/runtime_usb_has_pm.ptrecord");

	runtime_tunable t(USB_PATH, "usb", "testdev-99", "");
	test_framework_manager::get().reset();

	std::string got = t.serialize();
	PT_ASSERT_TRUE(got.find("Runtime PM") != std::string::npos);
}

/* ── constructor: PCI device with vendor/device IDs ──────────────── */

static void test_runtime_pci_with_vendor_device()
{
	/* HAVE_NO_PCI: pci_id_to_name returns ""; desc still references PCI */
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/runtime_pci_with_id.ptrecord");

	runtime_tunable t(PCI_PATH, "pci", "0000:99:00.0", "");
	test_framework_manager::get().reset();

	std::string got = t.serialize();
	/* no PM + vendor/device present → "PCI Device ... has no runtime power management" */
	PT_ASSERT_TRUE(got.find("PCI Device") != std::string::npos);
	PT_ASSERT_TRUE(got.find("no runtime power management") != std::string::npos);
}

/* ── good_bad: "auto" → TUNE_GOOD ────────────────────────────────── */

static void test_good_bad_returns_good()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/runtime_usb_no_pm.ptrecord");
	runtime_tunable t(USB_PATH, "usb", "testdev-99", "");

	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/runtime_control_auto.ptrecord");

	int result = t.good_bad();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(result == TUNE_GOOD);
}

/* ── good_bad: "on" → TUNE_BAD ───────────────────────────────────── */

static void test_good_bad_returns_bad()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/runtime_usb_no_pm.ptrecord");
	runtime_tunable t(USB_PATH, "usb", "testdev-99", "");

	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/runtime_control_on.ptrecord");

	int result = t.good_bad();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(result == TUNE_BAD);
}

/* ── toggle: good → writes "on" ──────────────────────────────────── */

static void test_toggle_from_good_writes_on()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/runtime_usb_no_pm.ptrecord");
	runtime_tunable t(USB_PATH, "usb", "testdev-99", "");

	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/runtime_control_auto.ptrecord");
	t.toggle();

	auto &log = test_framework_manager::get().get_write_log();
	PT_ASSERT_TRUE(!log.empty());
	PT_ASSERT_TRUE(log[0].first == USB_CTL);
	PT_ASSERT_TRUE(log[0].second == "on");
	test_framework_manager::get().reset();
}

/* ── toggle: bad → writes "auto" ─────────────────────────────────── */

static void test_toggle_from_bad_writes_auto()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/runtime_usb_no_pm.ptrecord");
	runtime_tunable t(USB_PATH, "usb", "testdev-99", "");

	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/runtime_control_on.ptrecord");
	t.toggle();

	auto &log = test_framework_manager::get().get_write_log();
	PT_ASSERT_TRUE(!log.empty());
	PT_ASSERT_TRUE(log[0].first == USB_CTL);
	PT_ASSERT_TRUE(log[0].second == "auto");
	test_framework_manager::get().reset();
}

/* ── serialize includes runtime_path and result ───────────────────── */

static void test_serialize_good_bad_result()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/runtime_usb_has_pm.ptrecord");
	runtime_tunable t(USB_PATH, "usb", "testdev-99", "");

	/* serialize() calls result_string() → good_bad() → reads control */
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/runtime_control_auto.ptrecord");
	std::string got = t.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("\"result\":\"Good\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"runtime_path\":\"" + USB_CTL + "\"") != std::string::npos);
}

int main()
{
	std::cout << "=== runtime_tunable tests ===\n";
	PT_RUN_TEST(test_runtime_usb_no_pm_desc);
	PT_RUN_TEST(test_runtime_usb_has_pm_desc);
	PT_RUN_TEST(test_runtime_pci_with_vendor_device);
	PT_RUN_TEST(test_good_bad_returns_good);
	PT_RUN_TEST(test_good_bad_returns_bad);
	PT_RUN_TEST(test_toggle_from_good_writes_on);
	PT_RUN_TEST(test_toggle_from_bad_writes_auto);
	PT_RUN_TEST(test_serialize_good_bad_result);
	return pt_test_summary();
}
