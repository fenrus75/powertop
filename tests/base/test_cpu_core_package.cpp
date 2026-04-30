/*
 * Tests for cpu_core and cpu_package formatting methods in src/cpu/
 *
 * Both classes are pure string formatters — they read fields already
 * populated in the abstract_cpu base (cstates, pstates, time_factor,
 * total_stamp, has_intel_MSR).  No I/O occurs; no fixtures needed.
 *
 * time_factor is protected, so we expose it via thin test subclasses.
 */
#include <iostream>
#include <string>
#include <cstdint>

#include "cpu/cpu.h"
#include "cpu/frequency.h"
#include "../test_helper.h"

/* ── thin subclasses to expose protected time_factor ─────────────────────── */

class test_cpu_core : public cpu_core {
public:
	void set_time_factor(double f) { time_factor = f; }
};

class test_cpu_package : public cpu_package {
public:
	void set_time_factor(double f) { time_factor = f; }
	void call_freq_updated(uint64_t t) { freq_updated(t); }
};

/* ═══════════════════════════════════════════════════════════════════════════
 * cpu_core tests
 * ═════════════════════════════════════════════════════════════════════════*/

static void test_cpu_core_cstate_header_no_msr()
{
	test_cpu_core cpu;
	cpu.has_intel_MSR = false;
	PT_ASSERT_EQ(cpu.fill_cstate_line(LEVEL_HEADER), std::string(" Core(OS)"));
}

static void test_cpu_core_cstate_header_with_msr()
{
	test_cpu_core cpu;
	cpu.has_intel_MSR = true;
	PT_ASSERT_EQ(cpu.fill_cstate_line(LEVEL_HEADER), std::string(" Core(HW)"));
}

static void test_cpu_core_cstate_line_percentage()
{
	test_cpu_core cpu;
	/* level=1 explicitly to get predictable line_level */
	cpu.insert_cstate("C3", "C3", 0, 0, 1, 1);
	cpu.cstates[0]->duration_delta = 800;
	cpu.set_time_factor(1000.0);   /* 800/1000 = 80% */

	std::string s = cpu.fill_cstate_line(1);
	PT_ASSERT_EQ(s, std::string(" 80.0%"));
}

static void test_cpu_core_cstate_line_no_match()
{
	test_cpu_core cpu;
	PT_ASSERT_EQ(cpu.fill_cstate_line(99), std::string(""));
}

static void test_cpu_core_cstate_name()
{
	test_cpu_core cpu;
	cpu.insert_cstate("C6", "C6-HSW", 0, 0, 1, 2);
	PT_ASSERT_EQ(cpu.fill_cstate_name(2), std::string("C6-HSW"));
}

static void test_cpu_core_cstate_name_no_match()
{
	test_cpu_core cpu;
	PT_ASSERT_EQ(cpu.fill_cstate_name(99), std::string(""));
}

static void test_cpu_core_pstate_header()
{
	test_cpu_core cpu;
	PT_ASSERT_EQ(cpu.fill_pstate_line(LEVEL_HEADER), std::string("  Core"));
}

static void test_cpu_core_pstate_line_single()
{
	test_cpu_core cpu;
	cpu.insert_pstate(2400000, "2400 MHz", 0, 1);
	cpu.finalize_pstate(2400000, 1000, 1);  /* time_after = 1000 */

	/* Only pstate → 100% of total_stamp */
	std::string s = cpu.fill_pstate_line(0);
	PT_ASSERT_EQ(s, std::string(" 100.0% "));
}

static void test_cpu_core_pstate_line_split()
{
	test_cpu_core cpu;
	cpu.insert_pstate(3600000, "3600 MHz", 0, 1);
	cpu.finalize_pstate(3600000, 600, 1);   /* time_after = 600 */
	cpu.insert_pstate(1200000, "1200 MHz", 0, 1);
	cpu.finalize_pstate(1200000, 400, 1);   /* time_after = 400, total = 1000 */

	PT_ASSERT_EQ(cpu.fill_pstate_line(0), std::string("  60.0% "));
	PT_ASSERT_EQ(cpu.fill_pstate_line(1), std::string("  40.0% "));
}

static void test_cpu_core_pstate_name()
{
	test_cpu_core cpu;
	cpu.insert_pstate(2400000, "2400 MHz", 0, 1);
	PT_ASSERT_EQ(cpu.fill_pstate_name(0), std::string("2400 MHz"));
}

static void test_cpu_core_pstate_out_of_range()
{
	test_cpu_core cpu;
	cpu.insert_pstate(2400000, "2400 MHz", 0, 1);

	PT_ASSERT_EQ(cpu.fill_pstate_line(-1), std::string(""));
	PT_ASSERT_EQ(cpu.fill_pstate_line(99), std::string(""));
	PT_ASSERT_EQ(cpu.fill_pstate_name(-1), std::string(""));
	PT_ASSERT_EQ(cpu.fill_pstate_name(99), std::string(""));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * cpu_package tests
 * ═════════════════════════════════════════════════════════════════════════*/

static void test_cpu_package_cstate_header_no_msr()
{
	test_cpu_package cpu;
	cpu.has_intel_MSR = false;
	PT_ASSERT_EQ(cpu.fill_cstate_line(LEVEL_HEADER), std::string(" Package(OS)"));
}

static void test_cpu_package_cstate_header_with_msr()
{
	test_cpu_package cpu;
	cpu.has_intel_MSR = true;
	PT_ASSERT_EQ(cpu.fill_cstate_line(LEVEL_HEADER), std::string(" Package(HW)"));
}

static void test_cpu_package_cstate_line_percentage()
{
	test_cpu_package cpu;
	cpu.insert_cstate("PC8", "PC8", 0, 0, 1, 1);
	cpu.cstates[0]->duration_delta = 500;
	cpu.set_time_factor(1000.0);   /* 50% */

	PT_ASSERT_EQ(cpu.fill_cstate_line(1), std::string(" 50.0%"));
}

static void test_cpu_package_cstate_name()
{
	test_cpu_package cpu;
	cpu.insert_cstate("PC8", "PC8-deep", 0, 0, 1, 3);
	PT_ASSERT_EQ(cpu.fill_cstate_name(3), std::string("PC8-deep"));
}

static void test_cpu_package_pstate_header()
{
	test_cpu_package cpu;
	PT_ASSERT_EQ(cpu.fill_pstate_line(LEVEL_HEADER), std::string("Package"));
}

static void test_cpu_package_pstate_line()
{
	test_cpu_package cpu;
	cpu.insert_pstate(3600000, "3600 MHz", 0, 1);
	cpu.finalize_pstate(3600000, 1000, 1);

	PT_ASSERT_EQ(cpu.fill_pstate_line(0), std::string(" 100.0% "));
	PT_ASSERT_EQ(cpu.fill_pstate_name(0), std::string("3600 MHz"));
}

static void test_cpu_package_freq_updated_no_parent()
{
	test_cpu_package cpu;
	cpu.parent = nullptr;
	/* With null parent, freq_updated() skips calculate_freq() — no crash */
	cpu.call_freq_updated(1000000);
}

/* ── main ───────────────────────────────────────────────────────────────── */

int main()
{
	std::cout << "=== cpu_core: cstate tests ===\n";
	PT_RUN_TEST(test_cpu_core_cstate_header_no_msr);
	PT_RUN_TEST(test_cpu_core_cstate_header_with_msr);
	PT_RUN_TEST(test_cpu_core_cstate_line_percentage);
	PT_RUN_TEST(test_cpu_core_cstate_line_no_match);
	PT_RUN_TEST(test_cpu_core_cstate_name);
	PT_RUN_TEST(test_cpu_core_cstate_name_no_match);

	std::cout << "\n=== cpu_core: pstate tests ===\n";
	PT_RUN_TEST(test_cpu_core_pstate_header);
	PT_RUN_TEST(test_cpu_core_pstate_line_single);
	PT_RUN_TEST(test_cpu_core_pstate_line_split);
	PT_RUN_TEST(test_cpu_core_pstate_name);
	PT_RUN_TEST(test_cpu_core_pstate_out_of_range);

	std::cout << "\n=== cpu_package: cstate tests ===\n";
	PT_RUN_TEST(test_cpu_package_cstate_header_no_msr);
	PT_RUN_TEST(test_cpu_package_cstate_header_with_msr);
	PT_RUN_TEST(test_cpu_package_cstate_line_percentage);
	PT_RUN_TEST(test_cpu_package_cstate_name);

	std::cout << "\n=== cpu_package: pstate tests ===\n";
	PT_RUN_TEST(test_cpu_package_pstate_header);
	PT_RUN_TEST(test_cpu_package_pstate_line);
	PT_RUN_TEST(test_cpu_package_freq_updated_no_parent);

	return pt_test_summary();
}
