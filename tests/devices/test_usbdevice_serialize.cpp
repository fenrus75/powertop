/*
 * Snapshot tests for usbdevice::serialize()
 *
 * Constructor reads (in order):
 *   bDeviceClass, manufacturer, product, busnum, devnum
 *   (5 sysfs records per scenario; N records for missing files)
 *
 * humanname assembly logic:
 *   bDeviceClass==9                    → rootport=1, default humanname unchanged
 *   vendor+product                     → "USB device: {product} ({vendor})"
 *   product only (no vendor)           → "USB device: {product}"
 *
 * Three scenarios:
 *   hub          — class=9, no vendor/product: rootport=1
 *   named        — class=0, vendor="Intel Corp.", product="USB Keyboard"
 *   product_only — class=0, no vendor, product="USB Mouse"
 */
#include <cmath>
#include <string>
#include "devices/usb.h"
#include "test_framework.h"
#include "../test_helper.h"

static const std::string DATA_DIR = TEST_DATA_DIR;
static const std::string USB_PATH = "/sys/bus/usb/devices/1-1";

static void test_hub()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/usb_hub.ptrecord");

	usbdevice d("1-1", USB_PATH, "1-1");
	std::string got = d.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("\"rootport\":1") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"busnum\":1") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"devnum\":2") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"sysfs_path\":\"" + USB_PATH + "\"") != std::string::npos);
}

static void test_named()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/usb_named.ptrecord");

	usbdevice d("1-1", USB_PATH, "1-1");
	std::string got = d.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("\"rootport\":0") != std::string::npos);
	PT_ASSERT_TRUE(got.find("USB Keyboard") != std::string::npos);
	PT_ASSERT_TRUE(got.find("Intel Corp.") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"devnum\":3") != std::string::npos);
}

static void test_product_only()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/usb_product_only.ptrecord");

	usbdevice d("1-1", USB_PATH, "1-1");
	std::string got = d.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("USB Mouse") != std::string::npos);
	PT_ASSERT_TRUE(got.find("Intel Corp.") == std::string::npos);
	PT_ASSERT_TRUE(got.find("\"devnum\":4") != std::string::npos);
}

static void test_measurement()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/usb_measurement.ptrecord");

	usbdevice d("1-1", USB_PATH, "1-1");
	d.start_measurement();  /* active_before=100, connected_before=100 */
	d.end_measurement();    /* active_after=150, connected_after=200  */
	std::string got = d.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(got.find("\"active_before\":100") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"active_after\":150") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"connected_before\":100") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"connected_after\":200") != std::string::npos);

	/* utilization = 100 * (150-100) / (200-100) = 5000/100 = 50.0 */
	double util = d.utilization();
	PT_ASSERT_TRUE(fabs(util - 50.0) < 0.0001);
}

int main()
{
	std::cout << "=== usbdevice serialize tests ===\n";
	PT_RUN_TEST(test_hub);
	PT_RUN_TEST(test_named);
	PT_RUN_TEST(test_product_only);
	PT_RUN_TEST(test_measurement);
	return pt_test_summary();
}
