/*
 * Tests for work::fire()/done() measurement cycle and serialize().
 *
 * work(address) calls kernel_function(address) → reads /proc/kallsyms once
 * (static kallsyms_read flag prevents re-reading across the test binary).
 * Fixture kallsyms_one_entry.ptrecord maps 0xdeadbeef00000001 → "test_work_fn".
 *
 * IMPORTANT: kallsyms_read is a static bool in lib.cpp that persists across
 * tests within this binary. After the first work construction triggers the
 * kallsyms read, subsequent constructions reuse the already-loaded map.
 * Tests using address 0x0 (not in map) will get handler="".
 *
 * Methods exercised:
 *   work(address)        — constructor sets handler/desc via kernel_function()
 *   fire(time, wstruct)  — records start time for a work item
 *   done(time, wstruct)  — computes delta, accumulates runtime, increments raw_count
 *   done() missing start — returns ~0ULL, no side-effects on accumulated_runtime
 *   usage_summary()      — accumulated_runtime / 1000000.0 / measurement_time / 10
 *   description()        — returns handler name
 *
 * usage_summary() formula (same as process):
 *   measurement_time = 1.0 (defined below)
 *   fire at t=0, done at t=5000000 us → runtime=5000000
 *   summary = 5000000 / 1000000.0 / 1.0 / 10 = 0.5
 */
#include <string>
#include <cstdint>
#include <vector>
#include "process/work.h"
#include "test_framework.h"
#include "../test_helper.h"

extern std::vector<class power_consumer *> all_power;

double measurement_time = 1.0;

static const std::string DATA_DIR = TEST_DATA_DIR;

/* 0xdeadbeef00000001 mapped to "test_work_fn" in the kallsyms fixture */
static constexpr unsigned long KNOWN_ADDR = 0xdeadbeef00000001UL;
static constexpr unsigned long UNKNOWN_ADDR = 0x0UL;

static void test_constructor_known_address()
{
	test_framework_manager::get().reset();
	/* First work construction triggers kallsyms read from /proc/kallsyms */
	test_framework_manager::get().set_replay(DATA_DIR + "/kallsyms_one_entry.ptrecord");

	work w(KNOWN_ADDR);
	std::string got = w.serialize();
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(w.handler == "test_work_fn");
	PT_ASSERT_TRUE(w.description() == "test_work_fn");
	PT_ASSERT_TRUE(got.find("\"handler\":\"test_work_fn\"") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"raw_count\":0") != std::string::npos);
}

static void test_constructor_unknown_address()
{
	test_framework_manager::get().reset();
	/* kallsyms already loaded — no I/O needed; address 0 not in map → empty string */

	work w(UNKNOWN_ADDR);
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(w.handler == "");
	PT_ASSERT_TRUE(w.description() == "");
}

static void test_fire_done_cycle()
{
	test_framework_manager::get().reset();

	work w(UNKNOWN_ADDR);
	static constexpr uint64_t WSTRUCT = 0xabcd1234ULL;

	w.fire(0, WSTRUCT);                    /* start at t=0 */
	uint64_t delta = w.done(5000000, WSTRUCT); /* end at t=5000000 us */
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(delta == 5000000ULL);
	PT_ASSERT_TRUE(w.raw_count == 1);

	std::string got = w.serialize();
	PT_ASSERT_TRUE(got.find("\"accumulated_runtime\":5000000") != std::string::npos);
	PT_ASSERT_TRUE(got.find("\"raw_count\":1") != std::string::npos);

	/* usage_summary = 5000000 / 1000000.0 / 1.0 / 10 = 0.5 */
	double summary = w.usage_summary();
	PT_ASSERT_TRUE(summary > 0.49 && summary < 0.51);
	PT_ASSERT_TRUE(w.usage_units_summary() == "%");
}

static void test_done_missing_start()
{
	test_framework_manager::get().reset();

	work w(UNKNOWN_ADDR);
	static constexpr uint64_t WSTRUCT = 0xbeef5678ULL;

	/* done() without a prior fire() for this work_struct → returns ~0ULL */
	uint64_t delta = w.done(1000000, WSTRUCT);
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(delta == ~0ULL);
	PT_ASSERT_TRUE(w.raw_count == 0);
	PT_ASSERT_TRUE(w.accumulated_runtime == 0ULL);
}

static void test_multiple_work_items()
{
	test_framework_manager::get().reset();

	work w(UNKNOWN_ADDR);
	static constexpr uint64_t WS1 = 0x1111ULL;
	static constexpr uint64_t WS2 = 0x2222ULL;

	/* Fire and complete two overlapping work items */
	w.fire(0, WS1);
	w.fire(1000000, WS2);
	w.done(3000000, WS1);  /* WS1: 3000000 us */
	w.done(4000000, WS2);  /* WS2: 3000000 us */
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(w.raw_count == 2);
	/* total accumulated = 3000000 + 3000000 = 6000000 */
	std::string got = w.serialize();
	PT_ASSERT_TRUE(got.find("\"accumulated_runtime\":6000000") != std::string::npos);
}

static void test_find_create_work_idempotent()
{
	test_framework_manager::get().reset();

	static constexpr unsigned long ADDR = 0x1234abcd5678UL;

	class work *w1 = find_create_work(ADDR);
	class work *w2 = find_create_work(ADDR);

	PT_ASSERT_TRUE(w1 != nullptr);
	PT_ASSERT_TRUE(w1 == w2);

	clear_work();
}

static void test_all_work_to_all_power_and_clear()
{
	test_framework_manager::get().reset();
	all_power.clear();

	static constexpr unsigned long ADDR = 0xaaaa1111bbbbUL;

	class work *w = find_create_work(ADDR);
	w->accumulated_runtime = 1000000;

	all_work_to_all_power();
	PT_ASSERT_TRUE(!all_power.empty());

	clear_work();
	all_power.clear();
}

int main()
{
	std::cout << "=== work tests ===\n";
	PT_RUN_TEST(test_constructor_known_address);
	PT_RUN_TEST(test_constructor_unknown_address);
	PT_RUN_TEST(test_fire_done_cycle);
	PT_RUN_TEST(test_done_missing_start);
	PT_RUN_TEST(test_multiple_work_items);
	PT_RUN_TEST(test_find_create_work_idempotent);
	PT_RUN_TEST(test_all_work_to_all_power_and_clear);
	return pt_test_summary();
}
