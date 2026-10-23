/*
 * Tested source file: src/lib.cpp
 *
 * Tests for the built-in pci.ids parser: pci_ids_parse(), pci_id_to_name(),
 * end_pci_access(). Built with -DHAVE_NO_PCI so the host's pci.ids is never
 * read; all data comes from the fixtures below.
 */
#include <cstdint>
#include <iostream>
#include <sstream>

#include "lib.h"
#include "test_framework.h"
#include "../test_helper.h"

void (*ui_notify_user)(const std::string &) = nullptr;

static const char *fixture =
	"# comment line\n"
	"\n"
	"8086  Intel Corporation\n"
	"\t# comment inside vendor\n"
	"\t1237  440FX - 82441FX PMC [Natoma]\n"
	"\t\t8086 1237  subsystem line must be ignored\n"
	"\t29c0  82G33/G31/P35/P33 Express DRAM Controller\n"
	"10de  NVIDIA Corporation\r\n"
	"\t1c82  GP107 [GeForce GTX 1050 Ti]\r\n"
	"bad!  not a vendor line\n"
	"\t0001  device of invalid vendor, must be ignored\n"
	"C 00  Unclassified device\n"
	"\t00  Non-VGA unclassified device\n"
	"ffff  vendor after class section, must be ignored\n";

static void load()
{
	std::istringstream in(fixture);
	pci_ids_parse(in);
}

static void test_vendor_and_device()
{
	load();
	PT_ASSERT_EQ(pci_id_to_name(0x8086, 0x1237),
		     std::string("Intel Corporation 440FX - 82441FX PMC [Natoma]"));
	PT_ASSERT_EQ(pci_id_to_name(0x8086, 0x29c0),
		     std::string("Intel Corporation 82G33/G31/P35/P33 Express DRAM Controller"));
}

static void test_crlf_line_endings()
{
	load();
	PT_ASSERT_EQ(pci_id_to_name(0x10de, 0x1c82),
		     std::string("NVIDIA Corporation GP107 [GeForce GTX 1050 Ti]"));
}

static void test_unknown_device_returns_vendor()
{
	load();
	PT_ASSERT_EQ(pci_id_to_name(0x8086, 0xdead), std::string("Intel Corporation"));
}

static void test_unknown_vendor_returns_empty()
{
	load();
	PT_ASSERT_EQ(pci_id_to_name(0x1234, 0x0001), std::string(""));
	PT_ASSERT_EQ(pci_id_to_name(0xffff, 0x0001), std::string(""));
}

static void test_subsystem_and_invalid_lines_ignored()
{
	load();
	PT_ASSERT_EQ(pci_id_to_name(0x8086, 0x8086), std::string("Intel Corporation"));
	PT_ASSERT_EQ(pci_id_to_name(0x10de, 0x0001), std::string("NVIDIA Corporation"));
}

static void test_empty_input_and_end_access()
{
	std::istringstream in("");
	pci_ids_parse(in);
	PT_ASSERT_EQ(pci_id_to_name(0x8086, 0x1237), std::string(""));

	load();
	end_pci_access();
	/* HAVE_NO_PCI: tables are not reloaded from the host after cleanup */
	PT_ASSERT_EQ(pci_id_to_name(0x8086, 0x1237), std::string(""));
}

int main()
{
	std::cout << "=== pci.ids parser tests ===\n";
	PT_RUN_TEST(test_vendor_and_device);
	PT_RUN_TEST(test_crlf_line_endings);
	PT_RUN_TEST(test_unknown_device_returns_vendor);
	PT_RUN_TEST(test_unknown_vendor_returns_empty);
	PT_RUN_TEST(test_subsystem_and_invalid_lines_ignored);
	PT_RUN_TEST(test_empty_input_and_end_access);

	return pt_test_summary();
}
