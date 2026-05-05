/*
 * Copyright 2010, Intel Corporation
 *
 * This file is part of PowerTOP
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 * or just google for it.
 *
 * Authors:
 *	Arjan van de Ven <arjan@linux.intel.com>
 */

/*
 * Regression test: learn_parameters() must not produce a division-by-zero
 * when a parameter's weight causes (1 + delta * weight) == 0.
 *
 * In learn.cpp line 210:
 *   value = orgvalue * 1 / (1 + weight);
 * where weight = delta * best_so_far->weights[i].
 *
 * When weights[i] == -20.0 and delta == 0.05 (the value delta is clamped to
 * when best_score / N < 4, i.e. when there are no real devices), we get
 *   weight = -1.0  →  denominator = 0  →  FE_DIVBYZERO is raised.
 *
 * This test asserts FE_DIVBYZERO is NOT raised.  It FAILS while the bug
 * exists and passes after the fix is applied.
 */

#include <cfenv>
#include <iostream>
#include <string>
#include <vector>

#include "parameters/parameters.h"
#include "test_framework.h"
#include "../test_helper.h"

/* ── Stubs required by parameters.cpp and learn.cpp ────────────────────── */

/* all_devices is used in compute_bundle(); keep it empty so the bundle score
 * stays at zero (which causes delta to be clamped to 0.05 — the value that
 * triggers the bug). */
class device;
std::vector<class device *> all_devices;

/* min_power is used in learn_parameters() for the base-power parameter cap. */
double min_power = 50000.0;

/* debug_learning controls verbose printf output; keep it off. */
int debug_learning = 0;

/* global_power() is called in store_results(); return 0 (no devices). */
double global_power(void) { return 0.0; }

/* save_all_results() is called in store_results(); no-op for tests. */
void save_all_results(const std::string &) {}

/* ── Test ───────────────────────────────────────────────────────────────── */

static void test_learn_parameters_no_divzero_with_negative_weight()
{
	/*
	 * Register "base power" first so learn_parameters() finds a valid bpi
	 * slot (it calls get_param_index("base power") internally).
	 */
	register_parameter("base power", 50.0, 1.0);

	/*
	 * Register a parameter whose weight makes (1 + delta * weight) == 0
	 * when delta == 0.05:  0.05 * (-20.0) = -1.0  →  1 + (-1.0) = 0.0.
	 *
	 * delta is clamped to 0.05 by learn_parameters() whenever
	 *   best_score / past_results.size() < 4
	 * which holds here because all_devices is empty, so every compute_bundle
	 * call returns score == 0.
	 */
	register_parameter("test-neg-weight", 1.0, -20.0);

	/*
	 * Populate past_results.  learn_parameters() returns early unless
	 *   past_results.size() > all_parameters.parameters.size()
	 * After two register_parameter calls, parameters.size() == 3
	 * (slot 0 is unused, slots 1 & 2 are our two params), so we need
	 * at least 4 entries; use 10 for safety.
	 */
	for (int i = 0; i < 10; i++) {
		auto *r = new result_bundle();
		r->power = 0.0;
		past_results.push_back(r);
	}

	/* Clear all FP exception flags before the call under test. */
	std::feclearexcept(FE_ALL_EXCEPT);

	/*
	 * iterations=16 forces delta to 0.2 (via the formula), which is then
	 * clamped to 0.05 because best_score/N < 4 with no devices.
	 * At delta=0.05: weight = 0.05 * (-20.0) = -1.0  → 1+weight = 0.0.
	 */
	learn_parameters(16, 0);

	int fp_flags    = std::fetestexcept(FE_ALL_EXCEPT);
	int divbyzero   = std::fetestexcept(FE_DIVBYZERO);
	int overflow    = std::fetestexcept(FE_OVERFLOW);
	int invalid     = std::fetestexcept(FE_INVALID);
	int inexact     = std::fetestexcept(FE_INEXACT);

	std::cout << "  FP exceptions after learn_parameters(16, 0):\n";
	std::cout << "    FE_DIVBYZERO : " << (divbyzero ? "SET" : "clear") << "\n";
	std::cout << "    FE_OVERFLOW  : " << (overflow  ? "SET" : "clear") << "\n";
	std::cout << "    FE_INVALID   : " << (invalid   ? "SET" : "clear") << "\n";
	std::cout << "    FE_INEXACT   : " << (inexact   ? "SET" : "clear") << "\n";
	std::cout << "    raw flags    : 0x" << std::hex << fp_flags << std::dec << "\n";

	/* Show the numerical result of the exact division that should be zero. */
	{
		double delta    = 0.05;
		double weight   = delta * (-20.0);        /* -1.0 */
		double orgvalue = 1.0;
		double result   = orgvalue * 1.0 / (1.0 + weight);
		std::cout << "  Division result (orgvalue / (1 + weight)):\n";
		std::cout << "    delta=" << delta
		          << "  weights[i]=-20.0  weight=" << weight
		          << "  (1+weight)=" << (1.0 + weight)
		          << "  result=" << result << "\n";
	}

	/*
	 * FE_DIVBYZERO is set by the x86 FPU when a non-zero value is divided
	 * by zero.  A correct implementation must not raise it.
	 */
	PT_ASSERT_EQ(divbyzero, 0);

	/* Cleanup */
	for (auto *r : past_results)
		delete r;
	past_results.clear();
}

int main()
{
	std::cout << "=== learn_parameters: division-by-zero safety ===\n";
	PT_RUN_TEST(test_learn_parameters_no_divzero_with_negative_weight);
	return pt_test_summary();
}
