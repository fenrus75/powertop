/*
 * Tests for process/timer.cpp
 *
 * Fixtures:
 *   timer_test.ptrecord        — /proc/kallsyms (2 entries) + /proc/timer_stats
 *                                  aaaa000000001111 → "deferred_handler" (D, in stats)
 *                                  aaaa000000002222 → "normal_handler"   (no D, in stats)
 *   timer_empty_stats.ptrecord — N /proc/timer_stats (empty → deferred=false)
 *
 * IMPORTANT: kallsyms_read is a static in lib.cpp that persists across all tests
 * in this binary. After test_timer_deferred_true (first test), kallsyms is cached:
 *   DEFERRED_ADDR → "deferred_handler", NORMAL_ADDR → "normal_handler"
 * Subsequent tests do NOT consume the kallsyms record from the fixture.
 */

#include <string>
#include <cstdint>

#include "process/timer.h"
#include "process/powerconsumer.h"
#include "test_framework.h"
#include "../test_helper.h"

/* defined in do_process.cpp; provided here to avoid linking that file */
double measurement_time = 1.0;

static const std::string DATA_DIR = TEST_DATA_DIR;

/* Addresses defined in timer_test.ptrecord kallsyms */
static constexpr uint64_t DEFERRED_ADDR = 0xaaaa000000001111UL;
static constexpr uint64_t NORMAL_ADDR   = 0xaaaa000000002222UL;
static constexpr uint64_t UNKNOWN_ADDR  = 0x0UL;

/* ── deferred flag ─────────────────────────────────────────────────── */

static void test_timer_deferred_true()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/timer_test.ptrecord");
	clear_timers();

	timer t(DEFERRED_ADDR);
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(t.is_deferred());
	PT_ASSERT_TRUE(t.deferred == true);
	PT_ASSERT_TRUE(t.handler == "deferred_handler");
}

static void test_timer_deferred_false()
{
	/* kallsyms already cached from previous test; fixture's kallsyms record unused */
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/timer_test.ptrecord");
	clear_timers();

	timer t(NORMAL_ADDR);
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(!t.is_deferred());
	PT_ASSERT_TRUE(t.handler == "normal_handler");
}

/* ── fire / done ───────────────────────────────────────────────────── */

static void test_timer_fire_done_basic()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/timer_empty_stats.ptrecord");
	clear_timers();

	timer t(UNKNOWN_ADDR);
	test_framework_manager::get().reset();

	static constexpr uint64_t TS = 0xdead0001ULL;
	t.fire(1000, TS);
	uint64_t delta = t.done(6000, TS);

	PT_ASSERT_TRUE(delta == 5000ULL);
	PT_ASSERT_TRUE(t.raw_count == 1);
	PT_ASSERT_TRUE(t.accumulated_runtime == 5000ULL);
}

static void test_timer_done_unknown_struct()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/timer_empty_stats.ptrecord");
	clear_timers();

	timer t(UNKNOWN_ADDR);
	test_framework_manager::get().reset();

	static constexpr uint64_t TS = 0xbeef0001ULL;
	uint64_t delta = t.done(1000, TS);  /* no prior fire() */

	PT_ASSERT_TRUE(delta == ~0ULL);
	PT_ASSERT_TRUE(t.raw_count == 0);
	PT_ASSERT_TRUE(t.accumulated_runtime == 0ULL);
}

static void test_timer_done_running_since_future()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/timer_empty_stats.ptrecord");
	clear_timers();

	timer t(UNKNOWN_ADDR);
	test_framework_manager::get().reset();

	static constexpr uint64_t TS = 0xface0001ULL;
	t.fire(5000, TS);              /* recorded at t=5000 */
	uint64_t delta = t.done(3000, TS); /* done at t=3000 < fire time */

	PT_ASSERT_TRUE(delta == 0ULL);
	PT_ASSERT_TRUE(t.accumulated_runtime == 0ULL);
}

/* ── usage_summary ─────────────────────────────────────────────────── */

static void test_timer_usage_summary()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/timer_empty_stats.ptrecord");
	clear_timers();

	timer t(UNKNOWN_ADDR);
	test_framework_manager::get().reset();

	static constexpr uint64_t TS = 0xabc00001ULL;
	/* fire at 0, done at 10 000 000 us → runtime = 10 000 000
	 * usage_summary = (10000000 - 0) / 1000000.0 / 1.0 / 10 = 1.0 */
	t.fire(0, TS);
	t.done(10000000, TS);

	double s = t.usage_summary();
	PT_ASSERT_TRUE(s > 0.99 && s < 1.01);
	PT_ASSERT_TRUE(t.usage_units_summary() == "%");
}

/* ── description ───────────────────────────────────────────────────── */

static void test_timer_description()
{
	/* kallsyms cached; NORMAL_ADDR → "normal_handler" */
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/timer_test.ptrecord");
	clear_timers();

	timer t(NORMAL_ADDR);
	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(t.description() == "normal_handler");
}

/* ── collect_json_fields ───────────────────────────────────────────── */

static void test_timer_json_fields()
{
	/* Use DEFERRED_ADDR so deferred=true is exercised in JSON */
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/timer_test.ptrecord");
	clear_timers();

	timer t(DEFERRED_ADDR);
	test_framework_manager::get().reset();

	static constexpr uint64_t TS = 0xcafe0001ULL;
	t.fire(0, TS);
	t.done(500, TS);

	std::string js = t.serialize();
	PT_ASSERT_TRUE(js.find("\"handler\":\"deferred_handler\"") != std::string::npos);
	PT_ASSERT_TRUE(js.find("\"raw_count\":1") != std::string::npos);
	PT_ASSERT_TRUE(js.find("\"deferred\":true") != std::string::npos);
}

/* ── all_timers_to_all_power ───────────────────────────────────────── */

static void test_all_timers_to_all_power()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/timer_empty_stats.ptrecord");
	clear_timers();

	class timer *t = find_create_timer(UNKNOWN_ADDR);
	test_framework_manager::get().reset();

	static constexpr uint64_t TS = 0x11110001ULL;
	t->fire(0, TS);
	t->done(2000, TS);  /* accumulated_runtime = 2000 > 0 */

	all_power.clear();
	all_timers_to_all_power();

	PT_ASSERT_TRUE(all_power.size() == 1);
	PT_ASSERT_TRUE(all_power[0] == t);
}

/* ── find_create_timer deduplication ──────────────────────────────── */

static void test_find_create_timer_dedup()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/timer_empty_stats.ptrecord");
	clear_timers();

	class timer *t1 = find_create_timer(UNKNOWN_ADDR);
	test_framework_manager::get().reset();

	/* second call with same address — must NOT construct (no fixture read needed) */
	class timer *t2 = find_create_timer(UNKNOWN_ADDR);

	PT_ASSERT_TRUE(t1 == t2);
}

/* ── clear_timers / running_since reset ───────────────────────────── */

static void test_clear_timers_resets_running_since()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/timer_empty_stats.ptrecord");
	clear_timers();

	timer t(UNKNOWN_ADDR);
	test_framework_manager::get().reset();

	static constexpr uint64_t TS = 0x55550001ULL;
	t.fire(100, TS);           /* running_since[TS] = 100 */

	clear_timers();            /* also clears running_since */

	/* TS is no longer in running_since → done() returns ~0ULL */
	uint64_t delta = t.done(200, TS);
	PT_ASSERT_TRUE(delta == ~0ULL);
}

int main()
{
	std::cout << "=== timer tests ===\n";
	PT_RUN_TEST(test_timer_deferred_true);
	PT_RUN_TEST(test_timer_deferred_false);
	PT_RUN_TEST(test_timer_fire_done_basic);
	PT_RUN_TEST(test_timer_done_unknown_struct);
	PT_RUN_TEST(test_timer_done_running_since_future);
	PT_RUN_TEST(test_timer_usage_summary);
	PT_RUN_TEST(test_timer_description);
	PT_RUN_TEST(test_timer_json_fields);
	PT_RUN_TEST(test_all_timers_to_all_power);
	PT_RUN_TEST(test_find_create_timer_dedup);
	PT_RUN_TEST(test_clear_timers_resets_running_since);
	return pt_test_summary();
}
