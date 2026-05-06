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
 * Unit tests for parameters.cpp data-manipulation functions.
 *
 * param_index and result_index are static globals that persist across
 * test functions, so these tests use unique names.
 */
#include <iostream>
#include <string>
#include <vector>

#include "parameters/parameters.h"
#include "devices/device.h"
#include "../test_helper.h"

extern double get_parameter_weight(int index, struct parameter_bundle *the_bundle);

std::vector<class device *> all_devices;

double global_power(void)
{
	return 0.0;
}

void save_all_results([[maybe_unused]] const std::string &fn)
{
}

static void test_register_and_get_parameter()
{
	register_parameter("test-reg-param-1", 42.0, 1.0);

	struct parameter_bundle b;
	set_parameter_value("test-reg-param-1", 7.5, &b);
	PT_ASSERT_TRUE(get_parameter_value("test-reg-param-1", &b) > 7.49);
	PT_ASSERT_TRUE(get_parameter_value("test-reg-param-1", &b) < 7.51);
}

static void test_get_parameter_oob()
{
	struct parameter_bundle b;
	PT_ASSERT_TRUE(get_parameter_value(999u, &b) == 0.0);
}

static void test_get_weight_oob()
{
	struct parameter_bundle b;
	PT_ASSERT_TRUE(get_parameter_weight(-1, &b) == 1.0);
	PT_ASSERT_TRUE(get_parameter_weight(9999, &b) == 1.0);
}

static void test_set_and_get_result_value()
{
	struct result_bundle b;
	set_result_value("test-result-1", 3.14, &b);
	PT_ASSERT_TRUE(get_result_value("test-result-1", &b) > 3.13);
	PT_ASSERT_TRUE(get_result_value("test-result-1", &b) < 3.15);
}

static void test_get_result_value_null_bundle()
{
	PT_ASSERT_TRUE(get_result_value(0, nullptr) == 0.0);
}

static void test_get_result_value_oob()
{
	struct result_bundle b;
	PT_ASSERT_TRUE(get_result_value(999, &b) == 0.0);
}

static void test_clone_results()
{
	struct result_bundle src;
	set_result_value("test-clone-result", 9.9, &src);
	src.power = 5.5;

	struct result_bundle *dst = clone_results(&src);
	PT_ASSERT_TRUE(dst != nullptr);
	PT_ASSERT_TRUE(dst->power > 5.49);
	PT_ASSERT_TRUE(dst->power < 5.51);

	src.power = 0.0;
	PT_ASSERT_TRUE(dst->power > 5.49);
	PT_ASSERT_TRUE(dst->power < 5.51);

	delete dst;
}

static void test_clone_parameters()
{
	struct parameter_bundle src;
	set_parameter_value("test-clone-param", 2.0, &src);
	src.actual_power = 11.0;

	struct parameter_bundle *dst = clone_parameters(&src);
	PT_ASSERT_TRUE(dst != nullptr);
	PT_ASSERT_TRUE(dst->actual_power > 10.99);
	PT_ASSERT_TRUE(dst->actual_power < 11.01);
	PT_ASSERT_TRUE(dst->score == 0.0);
	PT_ASSERT_TRUE(dst->guessed_power == 0.0);

	delete dst;
}

static void test_average_power_empty()
{
	past_results.clear();
	PT_ASSERT_TRUE(average_power() > 0.00009);
	PT_ASSERT_TRUE(average_power() < 0.00011);
}

static void test_average_power_with_values()
{
	past_results.clear();

	auto *r1 = new result_bundle();
	r1->power = 10.0;
	auto *r2 = new result_bundle();
	r2->power = 20.0;
	past_results.push_back(r1);
	past_results.push_back(r2);

	PT_ASSERT_TRUE(average_power() > 15.0);
	PT_ASSERT_TRUE(average_power() < 15.01);

	for (auto *r : past_results)
		delete r;
	past_results.clear();
}

static void test_utilization_power_valid_no_past()
{
	past_results.clear();
	PT_ASSERT_TRUE(utilization_power_valid("test-util-valid-name") == 0);
}

static void test_utilization_power_valid_varying()
{
	past_results.clear();
	set_result_value("test-uvv", 0.0, &all_results);

	auto *r1 = new result_bundle();
	set_result_value("test-uvv", 10.0, r1);
	auto *r2 = new result_bundle();
	set_result_value("test-uvv", 50.0, r2);
	past_results.push_back(r1);
	past_results.push_back(r2);

	PT_ASSERT_TRUE(utilization_power_valid("test-uvv") == 1);

	for (auto *r : past_results)
		delete r;
	past_results.clear();
}

static void test_global_power_valid_insufficient()
{
	past_results.clear();
	global_power_override = 0;
	PT_ASSERT_TRUE(global_power_valid() == 0);
}

static void test_get_param_directory_no_crash()
{
	std::string dir = get_param_directory("test.dat");
	PT_ASSERT_TRUE(dir.empty() || !dir.empty());
}

static void test_result_device_exists_empty()
{
	all_devices.clear();
	PT_ASSERT_TRUE(result_device_exists("nonexistent-device") == 0);
}

int main()
{
	std::cout << "=== parameters data-manipulation tests ===\n";
	PT_RUN_TEST(test_register_and_get_parameter);
	PT_RUN_TEST(test_get_parameter_oob);
	PT_RUN_TEST(test_get_weight_oob);
	PT_RUN_TEST(test_set_and_get_result_value);
	PT_RUN_TEST(test_get_result_value_null_bundle);
	PT_RUN_TEST(test_get_result_value_oob);
	PT_RUN_TEST(test_clone_results);
	PT_RUN_TEST(test_clone_parameters);
	PT_RUN_TEST(test_average_power_empty);
	PT_RUN_TEST(test_average_power_with_values);
	PT_RUN_TEST(test_utilization_power_valid_no_past);
	PT_RUN_TEST(test_utilization_power_valid_varying);
	PT_RUN_TEST(test_global_power_valid_insufficient);
	PT_RUN_TEST(test_get_param_directory_no_crash);
	PT_RUN_TEST(test_result_device_exists_empty);
	return pt_test_summary();
}
