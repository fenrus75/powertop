/*
 * Copyright 2025, Intel Corporation
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
 */

/*
 * Unit tests for ethernet_tunable::good_bad() and ethernet_tunable::toggle().
 *
 * A fake_eth subclass overrides get_wol() and set_wol() so tests run without
 * real network hardware.
 */

#include <iostream>
#include <string>
#include <vector>
#include <cstdint>

#include "tuning/tunable.h"
#include "tuning/ethernet.h"
#include "test_framework.h"
#include "../test_helper.h"

/* tunable.cpp (linked) defines all_tunables */

/* Stub: add_ethernet_tunable() calls create_all_nics(); not used in tests */
void create_all_nics(void (*)(const std::string &)) {}

/* ethtunable_callback is not in the header — forward-declare to test it */
void ethtunable_callback(const std::string &d_name);

/* ------------------------------------------------------------------ */
/* fake_eth: controllable stand-in for the real ethernet_tunable       */
/* ------------------------------------------------------------------ */

class fake_eth : public ethernet_tunable {
public:
	int      get_wol_ret  = 0;    /* -1 = failure              */
	uint32_t fake_wolopts = 0;    /* value returned by get_wol */

	int  set_wol_calls = 0;
	uint32_t last_set_wolopts = 0xdeadbeef;

	explicit fake_eth() : ethernet_tunable("eth_test") {}

	int get_wol(uint32_t &wolopts) override
	{
		if (get_wol_ret < 0)
			return -1;
		wolopts = fake_wolopts;
		return 0;
	}

	void set_wol(uint32_t wolopts) override
	{
		last_set_wolopts = wolopts;
		set_wol_calls++;
	}
};

/* ------------------------------------------------------------------ */
/* good_bad() tests                                                    */
/* ------------------------------------------------------------------ */

static void test_good_bad_no_hardware()
{
	fake_eth eth;
	eth.get_wol_ret = -1;

	PT_ASSERT_EQ(eth.good_bad(), TUNE_GOOD);
}

static void test_good_bad_wol_disabled()
{
	fake_eth eth;
	eth.fake_wolopts = 0;

	PT_ASSERT_EQ(eth.good_bad(), TUNE_GOOD);
}

static void test_good_bad_wol_enabled()
{
	fake_eth eth;
	eth.fake_wolopts = 1; /* any non-zero wake option */

	PT_ASSERT_EQ(eth.good_bad(), TUNE_BAD);
}

/* ------------------------------------------------------------------ */
/* toggle() tests                                                      */
/* ------------------------------------------------------------------ */

static void test_toggle_no_hardware_does_nothing()
{
	fake_eth eth;
	eth.get_wol_ret = -1;

	eth.toggle();
	PT_ASSERT_EQ(eth.set_wol_calls, 0);
}

static void test_toggle_disables_wol()
{
	fake_eth eth;
	eth.fake_wolopts = 0xFF;

	eth.toggle();
	PT_ASSERT_EQ(eth.set_wol_calls, 1);
	PT_ASSERT_EQ(eth.last_set_wolopts, (uint32_t)0);
}

/* ------------------------------------------------------------------ */
/* collect_json_fields() / ethtunable_callback() tests                 */
/* ------------------------------------------------------------------ */

static void test_collect_json_fields_includes_iface()
{
	fake_eth eth;  /* constructed with interf = "eth_test" */

	std::string js = eth.serialize();
	PT_ASSERT_TRUE(js.find("\"interf\":\"eth_test\"") != std::string::npos);
}

static void test_callback_skips_lo()
{
	size_t before = all_tunables.size();
	ethtunable_callback("lo");
	PT_ASSERT_EQ(all_tunables.size(), before);
}

static void test_callback_creates_tunable()
{
	size_t before = all_tunables.size();
	ethtunable_callback("cbtest0");
	PT_ASSERT_EQ(all_tunables.size(), before + 1);

	/* Check the created tunable serialises the correct interface name */
	std::string js = all_tunables.back()->serialize();
	PT_ASSERT_TRUE(js.find("\"interf\":\"cbtest0\"") != std::string::npos);
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */

int main()
{
	std::cout << "ethernet_tunable tests:\n";

	PT_RUN_TEST(test_good_bad_no_hardware);
	PT_RUN_TEST(test_good_bad_wol_disabled);
	PT_RUN_TEST(test_good_bad_wol_enabled);
	PT_RUN_TEST(test_toggle_no_hardware_does_nothing);
	PT_RUN_TEST(test_toggle_disables_wol);
	PT_RUN_TEST(test_collect_json_fields_includes_iface);
	PT_RUN_TEST(test_callback_skips_lo);
	PT_RUN_TEST(test_callback_creates_tunable);

	int result = pt_test_summary();

	for (auto *t : all_tunables)
		delete t;
	all_tunables.clear();

	return result;
}
