/*
 * Tests for nhm_cpu formatting methods (src/cpu/intel_cpus.cpp).
 */
#include <iostream>
#include <string>
#include <stdint.h>

#include "cpu/cpu.h"
#include "cpu/intel_cpus.h"
#include "test_framework.h"
#include "../test_helper.h"

void (*ui_notify_user)(const std::string &) = nullptr;
void reset_display(void) {}

class test_nhm_cpu : public nhm_cpu {
public:
	void set_time_factor(double f) { time_factor = f; }
	void set_msr_values(uint64_t tsc_b, uint64_t tsc_a, uint64_t aperf_b, uint64_t aperf_a, uint64_t mperf_b, uint64_t mperf_a) {
		tsc_before = tsc_b;
		tsc_after = tsc_a;
		aperf_before = aperf_b;
		aperf_after = aperf_a;
		mperf_before = mperf_b;
		mperf_after = mperf_a;
	}
};

static void test_nhm_cpu_pstate_line_c0_normal()
{
	test_nhm_cpu cpu;
	cpu.set_time_factor(1000.0);
	/* F = 1.0 * (tsc_delta) * (aperf_delta) / (mperf_delta) / time_factor * 1000 */
	/* We want F = 2,000,000,000 Hz (2.0 GHz) */
	/* Let tsc_delta = 2,000,000, aperf_delta = 1000, mperf_delta = 1000, time_factor = 1000 */
	/* F = 1.0 * 2,000,000 * 1000 / 1000 / 1000 * 1000 = 2,000,000,000 Hz */
	cpu.set_msr_values(0, 2000000, 0, 1000, 0, 1000);

	std::string s = cpu.fill_pstate_line(LEVEL_C0);
	/* 2,000,000,000 Hz = 2.0 GHz (with digits=1) */
	PT_ASSERT_TRUE(s.find("2.0") != std::string::npos);
	PT_ASSERT_TRUE(s.find("GHz") != std::string::npos);
}

static void test_nhm_cpu_pstate_line_c0_zero_time_factor()
{
	test_nhm_cpu cpu;
	cpu.set_time_factor(0.0);
	cpu.set_msr_values(0, 1000, 0, 1000, 0, 1000);

	std::string s = cpu.fill_pstate_line(LEVEL_C0);
	PT_ASSERT_EQ(s, std::string(""));
}

static void test_nhm_cpu_pstate_line_c0_zero_mperf_delta()
{
	test_nhm_cpu cpu;
	cpu.set_time_factor(1000.0);
	/* MPERF delta = 0 */
	cpu.set_msr_values(0, 1000, 0, 1000, 1000, 1000);

	std::string s = cpu.fill_pstate_line(LEVEL_C0);
	PT_ASSERT_EQ(s, std::string(""));
}

static void test_nhm_cpu_serialize()
{
	test_nhm_cpu cpu;
	cpu.set_msr_values(1, 2, 3, 4, 5, 6);
	std::string got = cpu.serialize();

	/* Verify some expected fields are present in JSON */
	PT_ASSERT_TRUE(got.find("\"aperf_before\":3") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"aperf_after\":4") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"mperf_before\":5") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"mperf_after\":6") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"tsc_before\":1") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"tsc_after\":2") != std::string::npos);
}

int main()
{
	std::cout << "=== nhm_cpu: pstate formatting and serialize tests ===\n";
	PT_RUN_TEST(test_nhm_cpu_pstate_line_c0_normal);
	PT_RUN_TEST(test_nhm_cpu_pstate_line_c0_zero_time_factor);
	PT_RUN_TEST(test_nhm_cpu_pstate_line_c0_zero_mperf_delta);
	PT_RUN_TEST(test_nhm_cpu_serialize);

	return pt_test_summary();
}
