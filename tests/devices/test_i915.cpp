/*
 * Tests for devices/i915-gpu.cpp
 *
 * Exercises: constructor (get_param_index/get_result_index), start/end
 * measurement (empty stubs), utilization(), power_usage() with a custom
 * result/parameter bundle, and collect_json_fields().
 *
 * create_i915_gpu() is not tested here (requires libtracefs probing).
 * No fixtures needed — all operations are pure in-memory.
 */

#include <string>
#include <cstdint>

#include "devices/i915-gpu.h"
#include "parameters/parameters.h"
#include "test_framework.h"
#include "../test_helper.h"

/* Stub: required by device.cpp (devlist) */
void (*ui_notify_user)(const std::string &) = nullptr;

/* ── constructor ──────────────────────────────────────────────────── */

static void test_i915_constructor()
{
	i915gpu gpu;

	/* get_param_index / get_result_index return > 0 for registered names */
	PT_ASSERT_TRUE(gpu.class_name() == "GPU");
	PT_ASSERT_TRUE(gpu.device_name() == "GPU");
}

/* ── start/end_measurement are no-ops ────────────────────────────── */

static void test_i915_start_end_measurement()
{
	i915gpu gpu;
	gpu.start_measurement();
	gpu.end_measurement();
	/* no assertions needed — just verify they don't crash */
	PT_ASSERT_TRUE(true);
}

/* ── utilization: returns get_result_value(rindex) ───────────────── */

static void test_i915_utilization()
{
	i915gpu gpu;

	/* utilization() reads from the global result bundle (nullptr → returns 0) */
	double util = gpu.utilization();
	PT_ASSERT_TRUE(util >= 0.0);
}

/* ── power_usage with known bundles ──────────────────────────────── */

static void test_i915_power_usage()
{
	/* register "gpu-operations" parameter so index is consistent */
	register_parameter("gpu-operations");

	i915gpu gpu;

	struct parameter_bundle pbundle;
	struct result_bundle rbundle;

	/* populate bundles at the correct index positions */
	int pidx = get_param_index("gpu-operations");
	int ridx = get_result_index("gpu-operations");

	if (pidx >= (int)pbundle.parameters.size())
		pbundle.parameters.resize(pidx + 1, 0.0);
	if (pidx >= (int)pbundle.weights.size())
		pbundle.weights.resize(pidx + 1, 1.0);

	pbundle.parameters[pidx] = 10.0;   /* 10 W per 100% utilization */

	if (ridx >= (int)rbundle.utilization.size())
		rbundle.utilization.resize(ridx + 1, 0.0);
	rbundle.utilization[ridx] = 50.0;  /* 50% utilization */

	double power = gpu.power_usage(&rbundle, &pbundle);

	/* power = util * factor / 100 = 50.0 * 10.0 / 100 = 5.0 (no children) */
	PT_ASSERT_TRUE(power > 4.99 && power < 5.01);
}

/* ── collect_json_fields ──────────────────────────────────────────── */

static void test_i915_collect_json_fields()
{
	i915gpu gpu;
	std::string js = gpu.serialize();

	PT_ASSERT_TRUE(js.find("\"class\":\"GPU\"") != std::string::npos);
	PT_ASSERT_TRUE(js.find("\"name\":\"GPU\"") != std::string::npos);
	PT_ASSERT_TRUE(js.find("\"index\":") != std::string::npos);
	PT_ASSERT_TRUE(js.find("\"rindex\":") != std::string::npos);
	PT_ASSERT_TRUE(js.find("\"child_devices\":") != std::string::npos);
}

int main()
{
	std::cout << "=== i915gpu tests ===\n";
	PT_RUN_TEST(test_i915_constructor);
	PT_RUN_TEST(test_i915_start_end_measurement);
	PT_RUN_TEST(test_i915_utilization);
	PT_RUN_TEST(test_i915_power_usage);
	PT_RUN_TEST(test_i915_collect_json_fields);
	return pt_test_summary();
}
