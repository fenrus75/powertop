/*
 * Tested source file: src/lib.cpp
 *
 * Tests for the usb.ids lookup: usb_ids_parse() and usb_id_to_names().
 * Built with -DHAVE_NO_HOST_IDS so the host's usb.ids is never read; all
 * data comes from the fixture below.
 */
#include <iostream>
#include <sstream>

#include "lib.h"
#include "test_framework.h"
#include "../test_helper.h"

void (*ui_notify_user)(const std::string &) = nullptr;

static const char *fixture =
	"# List of USB ID's\n"
	"\n"
	"0cf3  Qualcomm Atheros Communications\n"
	"\t0001  AR5523\n"
	"\te300  QCA61x4 Bluetooth 4.0\n"
	"1d6b  Linux Foundation\n"
	"\t0003  3.0 root hub\n"
	"\t\t01  interface line must be ignored\n"
	"C 00  (Defined at Interface level)\n"
	"\t01  Audio\n"
	"AT 0409  English (US)\n"
	"HID 22  Report\n";

static void load()
{
	std::istringstream in(fixture);
	usb_ids_parse(in);
}

static void test_vendor_and_product()
{
	load();
	usb_id_names n = usb_id_to_names("0cf3", "e300");
	PT_ASSERT_EQ(n.vendor, std::string("Qualcomm Atheros Communications"));
	PT_ASSERT_EQ(n.product, std::string("QCA61x4 Bluetooth 4.0"));
}

static void test_uppercase_hex()
{
	load();
	usb_id_names n = usb_id_to_names("0CF3", "E300");
	PT_ASSERT_EQ(n.product, std::string("QCA61x4 Bluetooth 4.0"));
}

static void test_unknown_product_keeps_vendor()
{
	load();
	usb_id_names n = usb_id_to_names("1d6b", "0002");
	PT_ASSERT_EQ(n.vendor, std::string("Linux Foundation"));
	PT_ASSERT_EQ(n.product, std::string(""));
}

static void test_unknown_vendor_is_empty()
{
	load();
	usb_id_names n = usb_id_to_names("1234", "abcd");
	PT_ASSERT_EQ(n.vendor, std::string(""));
	PT_ASSERT_EQ(n.product, std::string(""));
}

static void test_malformed_ids_are_empty()
{
	load();
	PT_ASSERT_EQ(usb_id_to_names("", "").vendor, std::string(""));
	PT_ASSERT_EQ(usb_id_to_names("0cf", "e300").vendor, std::string(""));
	PT_ASSERT_EQ(usb_id_to_names("0cf3x", "e300").vendor, std::string(""));
	PT_ASSERT_EQ(usb_id_to_names("0cf3", "e30g").vendor, std::string(""));
}

static void test_tables_independent_of_pci()
{
	load();
	std::istringstream empty("");
	pci_ids_parse(empty);
	PT_ASSERT_EQ(usb_id_to_names("0cf3", "e300").vendor,
		     std::string("Qualcomm Atheros Communications"));
	PT_ASSERT_EQ(pci_id_to_name(0x0cf3, 0xe300), std::string(""));
}

int main()
{
	std::cout << "=== usb.ids lookup tests ===\n";
	PT_RUN_TEST(test_vendor_and_product);
	PT_RUN_TEST(test_uppercase_hex);
	PT_RUN_TEST(test_unknown_product_keeps_vendor);
	PT_RUN_TEST(test_unknown_vendor_is_empty);
	PT_RUN_TEST(test_malformed_ids_are_empty);
	PT_RUN_TEST(test_tables_independent_of_pci);

	return pt_test_summary();
}
