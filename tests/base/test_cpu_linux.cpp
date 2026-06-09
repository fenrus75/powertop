/*
 * Tests for cpu_linux cstate formatting methods (src/cpu/cpu_linux.cpp).
 *
 * Covers the time_factor zero-guard added to fill_cstate_line() and
 * fill_cstate_percentage(): when time_factor is 0.0 (default before the
 * first measurement_end()) both methods must return "" rather than
 * producing NaN or Inf via 0.0/0.0 or x/0.0.
 *
 * Also covers the measurement_start/end cycle via fixture replay to
 * exercise parse_cstates_start/end (list_directory paths).
 *
 * time_factor is protected in abstract_cpu, so it is exposed via a thin
 * test subclass.
 */
#include <iostream>
#include <string>

#include "cpu/cpu.h"
#include "cpu/frequency.h"
#include "test_framework.h"
#include "../test_helper.h"

static const std::string DATA_DIR = TEST_DATA_DIR;

class test_cpu_linux : public cpu_linux {
public:
	explicit test_cpu_linux(int cpu_nr = 0) { number = cpu_nr; }
	void set_time_factor(double f) { time_factor = f; }
};

/* ── fill_cstate_line ─────────────────────────────────────────────────────── */

static void test_cpu_linux_cstate_header()
{
	test_cpu_linux cpu(2);
	std::string s = cpu.fill_cstate_line(LEVEL_HEADER);
	/* Header should mention the CPU number */
	PT_ASSERT_TRUE(s.find("2") != std::string::npos);
}

static void test_cpu_linux_cstate_line_c0_normal()
{
	test_cpu_linux cpu;
	/* linux_name "active" triggers line_level = LEVEL_C0 in insert_cstate */
	cpu.insert_cstate("active", "active", 0, 0, 0);
	cpu.cstates[0]->duration_delta = 750;
	cpu.set_time_factor(1000.0);   /* 75% */

	std::string s = cpu.fill_cstate_line(LEVEL_C0);
	/* C0 has no residency time; padded to match non-C0 column width (16 chars) */
	PT_ASSERT_EQ(s, std::string(" 75.0%          "));
}

static void test_cpu_linux_cstate_line_c0_zero_time_factor()
{
	test_cpu_linux cpu;
	cpu.insert_cstate("active", "active", 0, 0, 0);
	cpu.cstates[0]->duration_delta = 0;
	cpu.set_time_factor(0.0);

	std::string s = cpu.fill_cstate_line(LEVEL_C0);
	PT_ASSERT_EQ(s, std::string(""));
	PT_ASSERT_TRUE(s.find("nan") == std::string::npos);
	PT_ASSERT_TRUE(s.find("inf") == std::string::npos);
}

static void test_cpu_linux_cstate_line_cx_zero_time_factor()
{
	test_cpu_linux cpu;
	cpu.insert_cstate("C3", "C3", 0, 0, 1, 1);
	cpu.cstates[0]->duration_delta = 0;
	cpu.set_time_factor(0.0);

	std::string s = cpu.fill_cstate_line(1);
	PT_ASSERT_EQ(s, std::string(""));
	PT_ASSERT_TRUE(s.find("nan") == std::string::npos);
	PT_ASSERT_TRUE(s.find("inf") == std::string::npos);
}

/* ── fill_cstate_percentage ───────────────────────────────────────────────── */

static void test_cpu_linux_cstate_percentage_normal()
{
	test_cpu_linux cpu;
	cpu.insert_cstate("C6", "C6", 0, 0, 1, 2);
	cpu.cstates[0]->duration_delta = 300;
	cpu.set_time_factor(1000.0);   /* 30% */

	std::string s = cpu.fill_cstate_percentage(2);
	PT_ASSERT_EQ(s, std::string(" 30.0%"));
}

static void test_cpu_linux_cstate_percentage_zero_time_factor()
{
	test_cpu_linux cpu;
	cpu.insert_cstate("C6", "C6", 0, 0, 1, 2);
	cpu.cstates[0]->duration_delta = 0;
	cpu.set_time_factor(0.0);

	std::string s = cpu.fill_cstate_percentage(2);
	PT_ASSERT_EQ(s, std::string(""));
	PT_ASSERT_TRUE(s.find("nan") == std::string::npos);
	PT_ASSERT_TRUE(s.find("inf") == std::string::npos);
}

/* ── fill_cstate_time ─────────────────────────────────────────────────────── */

static void test_fill_cstate_time_c0_always_empty()
{
	test_cpu_linux cpu;
	/* fill_cstate_time always returns "" for LEVEL_C0 */
	PT_ASSERT_EQ(cpu.fill_cstate_time(LEVEL_C0), std::string(""));
}

static void test_fill_cstate_time_no_match()
{
	test_cpu_linux cpu;
	PT_ASSERT_EQ(cpu.fill_cstate_time(5), std::string(""));
}

static void test_fill_cstate_time_normal()
{
	test_cpu_linux cpu;
	cpu.insert_cstate("C3", "C3", 0, 0, 1, 3);
	/* duration_delta / (1 + usage_delta) / 1000 ms
	 * = 2000 / (1+1) / 1000 = 1.0 ms */
	cpu.cstates[0]->duration_delta = 2000;
	cpu.cstates[0]->usage_delta    = 1;
	std::string s = cpu.fill_cstate_time(3);
	PT_ASSERT_TRUE(s.find("1.0") != std::string::npos);
	PT_ASSERT_TRUE(s.find("ms") != std::string::npos);
}

/* ── fill_cstate_name ─────────────────────────────────────────────────────── */

static void test_fill_cstate_name_hit()
{
	test_cpu_linux cpu;
	cpu.insert_cstate("C6", "Deep Sleep C6", 0, 0, 1, 6);
	PT_ASSERT_EQ(cpu.fill_cstate_name(6), std::string("Deep Sleep C6"));
}

static void test_fill_cstate_name_miss()
{
	test_cpu_linux cpu;
	PT_ASSERT_EQ(cpu.fill_cstate_name(99), std::string(""));
}

/* ── fill_pstate_name / fill_pstate_line ─────────────────────────────────── */

static void test_fill_pstate_name_normal()
{
	test_cpu_linux cpu;
	cpu.insert_pstate(1200000, "1.2 GHz", 0, 0);
	PT_ASSERT_EQ(cpu.fill_pstate_name(0), std::string("1.2 GHz"));
}

static void test_fill_pstate_name_out_of_range()
{
	test_cpu_linux cpu;
	PT_ASSERT_EQ(cpu.fill_pstate_name(0), std::string(""));
	PT_ASSERT_EQ(cpu.fill_pstate_name(-1), std::string(""));
}

static void test_fill_pstate_line_header()
{
	test_cpu_linux cpu(4);
	std::string s = cpu.fill_pstate_line(LEVEL_HEADER);
	PT_ASSERT_TRUE(s.find("4") != std::string::npos);
}

static void test_fill_pstate_line_percent()
{
	test_cpu_linux cpu;
	cpu.insert_pstate(1000000, "1.0 GHz", 0, 0);
	cpu.pstates[0]->time_after = 1000;   /* 100% of total */
	std::string s = cpu.fill_pstate_line(0);
	PT_ASSERT_TRUE(s.find("100") != std::string::npos);
}

static void test_fill_pstate_line_out_of_range()
{
	test_cpu_linux cpu;
	PT_ASSERT_EQ(cpu.fill_pstate_line(5), std::string(""));
}

/* ── measurement_start / measurement_end cycle ──────────────────────────── */

/*
 * Exercises parse_cstates_start/end (list_directory) via fixture replay.
 * Fixture: cpu_linux_cycle.ptrecord
 *   stamp_before=1000s, stamp_after=1001s → time_factor = 1 000 000 µs
 *   cpuidle/state0 (name=C1): usage before=100, after=200 → delta=100
 *                             time  before=500000, after=1000000 → delta=500000
 * Expected:
 *   fill_cstate_name(1)       → "C1"
 *   fill_cstate_time(1)       → "   5.0 ms"  (500000/101/1000 ≈ 4.95 rounds up)
 *   fill_cstate_percentage(1) → " 50.0%"     (500000/1000000 * 100)
 */
static void test_cpu_linux_measurement_cycle()
{
	auto &tf = test_framework_manager::get();
	tf.reset();
	tf.set_replay(DATA_DIR + "/cpu_linux_cycle.ptrecord");

	test_cpu_linux cpu;
	cpu.measurement_start();
	cpu.measurement_end();

	tf.reset();

	PT_ASSERT_EQ(cpu.fill_cstate_name(1), std::string("C1"));
	PT_ASSERT_TRUE(cpu.fill_cstate_time(1).find("ms") != std::string::npos);
	PT_ASSERT_TRUE(cpu.fill_cstate_percentage(1).find("%") != std::string::npos);
	/* No NaN or Inf in any output */
	PT_ASSERT_TRUE(cpu.fill_cstate_time(1).find("nan") == std::string::npos);
	PT_ASSERT_TRUE(cpu.fill_cstate_percentage(1).find("nan") == std::string::npos);
}

/* ── main ───────────────────────────────────────────────────────────────── */

int main()
{
	std::cout << "=== cpu_linux: cstate formatting tests ===\n";
	PT_RUN_TEST(test_cpu_linux_cstate_header);
	PT_RUN_TEST(test_cpu_linux_cstate_line_c0_normal);
	PT_RUN_TEST(test_cpu_linux_cstate_line_c0_zero_time_factor);
	PT_RUN_TEST(test_cpu_linux_cstate_line_cx_zero_time_factor);
	PT_RUN_TEST(test_cpu_linux_cstate_percentage_normal);
	PT_RUN_TEST(test_cpu_linux_cstate_percentage_zero_time_factor);
	PT_RUN_TEST(test_fill_cstate_time_c0_always_empty);
	PT_RUN_TEST(test_fill_cstate_time_no_match);
	PT_RUN_TEST(test_fill_cstate_time_normal);
	PT_RUN_TEST(test_fill_cstate_name_hit);
	PT_RUN_TEST(test_fill_cstate_name_miss);
	PT_RUN_TEST(test_fill_pstate_name_normal);
	PT_RUN_TEST(test_fill_pstate_name_out_of_range);
	PT_RUN_TEST(test_fill_pstate_line_header);
	PT_RUN_TEST(test_fill_pstate_line_percent);
	PT_RUN_TEST(test_fill_pstate_line_out_of_range);
	PT_RUN_TEST(test_cpu_linux_measurement_cycle);
	return pt_test_summary();
}
