/*
 * Snapshot tests for power_consumer base class serialize()
 *
 * Instantiate power_consumer directly and verify that collect_json_fields
 * emits all the base-class member fields.
 */
#include <string>
#include "process/powerconsumer.h"
#include "../test_helper.h"

double measurement_time = 1.0;

static void test_power_consumer_defaults()
{
	power_consumer pc;

	std::string got = pc.serialize();

	/* base class name()/type() return translated "abstract" */
	PT_ASSERT_TRUE(got.find("\"accumulated_runtime\":0") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"child_runtime\":0") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"wake_ups\":0") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"gpu_ops\":0") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"disk_hits\":0") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"hard_disk_hits\":0") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"xwakes\":0") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"power_charge\":0") != std::string::npos);
}

static void test_power_consumer_fields()
{
	power_consumer pc;
	pc.accumulated_runtime = 500000;
	pc.child_runtime       = 100000;
	pc.wake_ups            = 42;
	pc.gpu_ops             = 7;
	pc.disk_hits           = 3;
	pc.hard_disk_hits      = 1;
	pc.xwakes              = 2;
	pc.power_charge        = 1.5;

	std::string got = pc.serialize();

	PT_ASSERT_TRUE(got.find("\"accumulated_runtime\":500000") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"child_runtime\":100000") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"wake_ups\":42") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"gpu_ops\":7") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"disk_hits\":3") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"hard_disk_hits\":1") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"xwakes\":2") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"power_charge\":1.5") != std::string::npos);
}

int main()
{
	std::cout << "=== power_consumer base class tests ===\n";
	PT_RUN_TEST(test_power_consumer_defaults);
	PT_RUN_TEST(test_power_consumer_fields);
	return pt_test_summary();
}
