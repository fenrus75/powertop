/*
 * Snapshot tests for abstract_cpu base class serialize()
 *
 * abstract_cpu has no pure virtual methods so it can be instantiated
 * directly. collect_json_fields emits: type (from get_type()), number,
 * first_cpu, idle, has_intel_MSR, current_frequency, effective_frequency,
 * cstates[], pstates[], children[].
 *
 * With no cstates/pstates/children added the arrays are empty.
 */
#include <string>
#include "cpu/cpu.h"
#include "../test_helper.h"

static void test_abstract_cpu_defaults()
{
	abstract_cpu cpu;
	cpu.set_type("GenericCPU");
	cpu.number = 2;

	std::string got = cpu.serialize();

	PT_ASSERT_TRUE(got.find("\"type\":\"GenericCPU\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"number\":2") != std::string::npos);
	/* vectors empty → empty JSON arrays */
	PT_ASSERT_TRUE(got.find("\"cstates\":[]") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"pstates\":[]") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"children\":[]") != std::string::npos);
}

static void test_abstract_cpu_idle_flag()
{
	abstract_cpu cpu;
	cpu.set_type("IdleCPU");
	cpu.go_idle(0);

	std::string got = cpu.serialize();

	PT_ASSERT_TRUE(got.find("\"idle\":true") != std::string::npos);
}

int main()
{
	std::cout << "=== abstract_cpu base class tests ===\n";
	PT_RUN_TEST(test_abstract_cpu_defaults);
	PT_RUN_TEST(test_abstract_cpu_idle_flag);
	return pt_test_summary();
}
