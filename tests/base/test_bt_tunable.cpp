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
 * Unit tests for bt_tunable::good_bad() and bt_tunable::toggle().
 *
 * A fake_bt subclass overrides all virtual hardware helpers so tests run
 * without real Bluetooth hardware.  The snap_bytes[]/snap_time[] snapshot
 * state is now per-instance (protected class members), so each test that
 * creates a fresh fake_bt starts with a clean slate.
 */

#include <iostream>
#include <string>
#include <vector>
#include <ctime>

#include "tuning/tunable.h"
#include "tuning/bluetooth.h"
#include "test_framework.h"
#include "../test_helper.h"

/* tunable.cpp (linked into this test) defines all_tunables */

/* ------------------------------------------------------------------ */
/* fake_bt: controllable stand-in for the real bt_tunable              */
/* ------------------------------------------------------------------ */

class fake_bt : public bt_tunable {
public:
	/* Controls returned by hci_get_dev_info */
	int          info_ret   = 0;      /* return value; -1 = failure    */
	unsigned int fake_flags = 1;      /* bit 0 set = interface UP      */
	unsigned int fake_rx    = 0;
	unsigned int fake_tx    = 0;

	/* Observations from hci_set_power */
	int  set_power_calls = 0;
	bool last_up         = false;

	/* Injected clock; starts at an arbitrary epoch value */
	time_t fake_time = 10000;

	fake_bt() : bt_tunable(0, "hci0") {}

	int hci_get_dev_info(unsigned int &flags,
	                     unsigned int &byte_rx,
	                     unsigned int &byte_tx) override
	{
		if (info_ret < 0)
			return -1;
		flags   = fake_flags;
		byte_rx = fake_rx;
		byte_tx = fake_tx;
		return 0;
	}

	void hci_set_power(bool up) override
	{
		last_up = up;
		set_power_calls++;
	}

	time_t current_time() override { return fake_time; }

	/* Test-only accessor to verify snapshot state */
	int get_snap_bytes(int i) { return snap_bytes[i]; }
};

/* ------------------------------------------------------------------ */
/* good_bad() tests                                                    */
/* ------------------------------------------------------------------ */

static void test_good_bad_no_hardware()
{
	fake_bt bt;
	bt.info_ret = -1;

	PT_ASSERT_EQ(bt.good_bad(), TUNE_GOOD);
}

static void test_good_bad_interface_down()
{
	fake_bt bt;
	bt.fake_flags = 0; /* HCI_UP bit clear */

	PT_ASSERT_EQ(bt.good_bad(), TUNE_GOOD);
}

static void test_good_bad_first_call_initialises_snapshot()
{
	fake_bt bt;
	bt.fake_rx = 100;
	bt.fake_tx = 50;

	/* First call — slot 0 is uninitialised, must return TUNE_GOOD */
	PT_ASSERT_EQ(bt.good_bad(), TUNE_GOOD);

	/* Verify slot 0 was populated (snap_bytes[0] == 150) */
	PT_ASSERT_EQ(bt.get_snap_bytes(0), 150);
}

static void test_good_bad_no_window_yet()
{
	fake_bt bt;
	bt.fake_rx = 100;
	bt.fake_tx = 50;

	bt.good_bad(); /* initialise slot 0 */

	/* Call again immediately (same timestamp) — slot 1 still uninitialised */
	PT_ASSERT_EQ(bt.good_bad(), TUNE_GOOD);
	PT_ASSERT_EQ(bt.get_snap_bytes(1), -1);
}

static void test_good_bad_active_after_window()
{
	fake_bt bt;
	bt.fake_rx = 100;
	bt.fake_tx = 50;

	bt.good_bad(); /* initialise slot 0 at t=10000, bytes=150 */

	/* 65 seconds later, bytes have increased — BT is active */
	bt.fake_time = 10065;
	bt.fake_rx   = 200;
	PT_ASSERT_EQ(bt.good_bad(), TUNE_GOOD);
}

static void test_good_bad_idle_after_window()
{
	fake_bt bt;
	bt.fake_rx = 100;
	bt.fake_tx = 50;

	bt.good_bad(); /* initialise slot 0 at t=10000, bytes=150 */

	/* 65 seconds later, NO byte change — slot 1 gets populated with old value
	 * and current bytes == slot-1 bytes → TUNE_BAD */
	bt.fake_time = 10065;
	PT_ASSERT_EQ(bt.good_bad(), TUNE_BAD);
}

/* ------------------------------------------------------------------ */
/* toggle() tests                                                      */
/* ------------------------------------------------------------------ */

static void test_toggle_turns_off_idle_bt()
{
	fake_bt bt;
	bt.fake_rx = 100;
	bt.fake_tx = 50;

	bt.good_bad();        /* init slot 0 */
	bt.fake_time = 10065; /* advance clock so good_bad() → TUNE_BAD */

	bt.toggle();
	PT_ASSERT_EQ(bt.set_power_calls, 1);
	PT_ASSERT_EQ(bt.last_up, false); /* idle → power down */
}

static void test_toggle_keeps_active_bt_on()
{
	fake_bt bt;
	bt.fake_rx = 100;
	bt.fake_tx = 50;

	bt.good_bad();        /* init slot 0 */
	bt.fake_time = 10065;
	bt.fake_rx   = 200;   /* bytes changed → TUNE_GOOD */

	bt.toggle();
	PT_ASSERT_EQ(bt.set_power_calls, 1);
	PT_ASSERT_EQ(bt.last_up, true); /* active → power stays up */
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */

int main()
{
	std::cout << "bt_tunable tests:\n";

	PT_RUN_TEST(test_good_bad_no_hardware);
	PT_RUN_TEST(test_good_bad_interface_down);
	PT_RUN_TEST(test_good_bad_first_call_initialises_snapshot);
	PT_RUN_TEST(test_good_bad_no_window_yet);
	PT_RUN_TEST(test_good_bad_active_after_window);
	PT_RUN_TEST(test_good_bad_idle_after_window);
	PT_RUN_TEST(test_toggle_turns_off_idle_bt);
	PT_RUN_TEST(test_toggle_keeps_active_bt_on);

	return pt_test_summary();
}
