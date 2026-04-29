/*
 * Snapshot tests for device base class serialize()
 *
 * The base class can be instantiated directly — all virtuals have defaults.
 * collect_json_fields emits: class (from class_name()), name (from
 * device_name()), hide, guilty, real_path.
 */
#include <string>
#include "devices/device.h"
#include "../test_helper.h"

/* Stub required by device.cpp */
double global_power() { return 0.0; }

static void test_device_base_defaults()
{
	device d;

	std::string got = d.serialize();

	/* base class virtuals return "abstract device" */
	PT_ASSERT_TRUE(got.find("\"class\":\"abstract device\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"name\":\"abstract device\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"hide\":false") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"guilty\":\"\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"real_path\":\"\"") != std::string::npos);
}

static void test_device_base_fields()
{
	device d;
	d.hide    = true;
	d.guilty  = "some_process";
	d.real_path = "/sys/devices/pci0000:00/foo";

	std::string got = d.serialize();

	PT_ASSERT_TRUE(got.find("\"hide\":true") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"guilty\":\"some_process\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"real_path\":\"/sys/devices/pci0000:00/foo\"") != std::string::npos);
}

int main()
{
	std::cout << "=== device base class tests ===\n";
	PT_RUN_TEST(test_device_base_defaults);
	PT_RUN_TEST(test_device_base_fields);
	return pt_test_summary();
}
