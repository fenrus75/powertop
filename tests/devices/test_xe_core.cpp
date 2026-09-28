/*
 * Tests for cpu/xe_gpu.cpp  (xe_core class)
 *
 * Exercises: constructor with no DRM cards, constructor with one GT,
 * start/end measurement (busy-% calculation), fill_cstate_line()
 * (LEVEL_HEADER, C0, C6, default, zero-delta guard),
 * fill_pstate_line/name(), and collect_json_fields().
 *
 * find_xe_card_path() uses a static cache and is exercised through
 * the xe_core constructor path — it must be the first call in the
 * process that touches /sys/class/drm, so each binary gets one shot.
 */

#include <string>
#include <cstdint>
#include <iostream>
#include <vector>
#include <memory>

#include "cpu/intel_cpus.h"
#include "cpu/frequency.h"
#include "devices/device.h"
#include "test_framework.h"
#include "../test_helper.h"

/* all_devices itself is defined in devices/device.cpp (linked below); this
 * file never populates it. */

/* ── no DRM: constructor builds an empty xe_core ────────────────── */

static void test_xe_core_no_drm()
{
	test_framework_manager::get().set_replay(
		TEST_DATA_DIR "/xe_core_no_drm.ptrecord");

	xe_core core;

	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(core.gt_labels.empty());
	PT_ASSERT_TRUE(core.per_gt_busy_pct.empty());
}

/* ── one GT: constructor finds the idle_residency_ms path ───────── */

static void test_xe_core_one_gt_constructor()
{
	test_framework_manager::get().set_replay(
		TEST_DATA_DIR "/xe_core_one_gt.ptrecord");

	xe_core core;

	/* Consume the start/end T+R records so reset() is clean */
	core.measurement_start();
	core.measurement_end();

	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(core.gt_labels.size() == 1);
	PT_ASSERT_TRUE(core.gt_labels[0] == "tile0 gt0");
	PT_ASSERT_TRUE(core.per_gt_busy_pct.size() == 1);
}

/* ── measurement: 2 s elapsed, 500 ms more idle → 75 % busy ─────── */

static void test_xe_core_measurement()
{
	test_framework_manager::get().set_replay(
		TEST_DATA_DIR "/xe_core_one_gt.ptrecord");

	xe_core core;
	core.measurement_start();
	core.measurement_end();

	test_framework_manager::get().reset();

	/* idle_start=500 ms, idle_end=1000 ms, delta_idle=500 ms
	 * time_delta=2 s=2 000 000 µs
	 * ratio = 100 000 / 2 000 000 = 0.05 %/ms
	 * busy  = 100 - 0.05*500 = 75 % */
	PT_ASSERT_TRUE(core.per_gt_busy_pct[0] > 74.9 &&
		       core.per_gt_busy_pct[0] < 75.1);
}

/* ── fill_cstate_line: LEVEL_HEADER returns "  GPU " ────────────── */

static void test_xe_core_fill_cstate_header()
{
	test_framework_manager::get().set_replay(
		TEST_DATA_DIR "/xe_core_no_drm.ptrecord");

	xe_core core;

	test_framework_manager::get().reset();

	std::string h = core.fill_cstate_line(LEVEL_HEADER, "");
	PT_ASSERT_TRUE(h.find("GPU") != std::string::npos);
}

/* ── fill_cstate_line: zero delta returns "" ────────────────────── */

static void test_xe_core_fill_cstate_zero_delta()
{
	test_framework_manager::get().set_replay(
		TEST_DATA_DIR "/xe_core_no_drm.ptrecord");

	xe_core core;

	test_framework_manager::get().reset();

	/* before==after=={0,0} so time_delta < 1.0 */
	PT_ASSERT_TRUE(core.fill_cstate_line(0, "") == "");
	PT_ASSERT_TRUE(core.fill_cstate_line(1, "") == "");
}

/* ── fill_cstate_line: line_nr>=2 always returns "" ─────────────── */

static void test_xe_core_fill_cstate_default()
{
	test_framework_manager::get().set_replay(
		TEST_DATA_DIR "/xe_core_no_drm.ptrecord");

	xe_core core;

	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(core.fill_cstate_line(2, "") == "");
	PT_ASSERT_TRUE(core.fill_cstate_line(99, "") == "");
}

/* ── fill_pstate_line / fill_pstate_name always return "" ───────── */

static void test_xe_core_fill_pstate()
{
	test_framework_manager::get().set_replay(
		TEST_DATA_DIR "/xe_core_no_drm.ptrecord");

	xe_core core;

	test_framework_manager::get().reset();

	PT_ASSERT_TRUE(core.fill_pstate_line(0) == "");
	PT_ASSERT_TRUE(core.fill_pstate_name(0) == "");
}

/* ── collect_json_fields ────────────────────────────────────────── */

static void test_xe_core_collect_json_fields()
{
	test_framework_manager::get().set_replay(
		TEST_DATA_DIR "/xe_core_one_gt.ptrecord");

	xe_core core;
	core.measurement_start();
	core.measurement_end();

	test_framework_manager::get().reset();

	std::string js = core.serialize();

	PT_ASSERT_TRUE(js.find("\"gt_idle_paths\":") != std::string::npos);
	PT_ASSERT_TRUE(js.find("\"idle_before\":") != std::string::npos);
	PT_ASSERT_TRUE(js.find("\"idle_after\":") != std::string::npos);
	PT_ASSERT_TRUE(js.find("\"before_sec\":") != std::string::npos);
	PT_ASSERT_TRUE(js.find("\"after_sec\":") != std::string::npos);
}

int main()
{
	std::cout << "=== xe_core cpu tests ===\n";
	PT_RUN_TEST(test_xe_core_no_drm);
	PT_RUN_TEST(test_xe_core_one_gt_constructor);
	PT_RUN_TEST(test_xe_core_measurement);
	PT_RUN_TEST(test_xe_core_fill_cstate_header);
	PT_RUN_TEST(test_xe_core_fill_cstate_zero_delta);
	PT_RUN_TEST(test_xe_core_fill_cstate_default);
	PT_RUN_TEST(test_xe_core_fill_pstate);
	PT_RUN_TEST(test_xe_core_collect_json_fields);
	return pt_test_summary();
}
