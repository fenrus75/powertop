/*
 * Tests for read_kallsyms() empty-content path in src/lib.cpp (line 118).
 *
 * read_kallsyms() is driven by a static flag (kallsyms_read) that is set on
 * the very first call and never cleared.  This binary must NOT share a
 * process with any other test that calls kernel_function() — it lives in its
 * own test executable precisely for that reason.
 *
 * Fixture: replay_kallsyms_empty.ptrecord supplies an N record for
 * /proc/kallsyms so that read_file_content() returns "" and the early-return
 * branch (line 118) is taken.
 */
#include <iostream>

#include "lib.h"
#include "test_framework.h"
#include "../test_helper.h"

void (*ui_notify_user)(const std::string &) = nullptr;

static const std::string DATA_DIR = TEST_DATA_DIR;

/* ── read_kallsyms empty content ─────────────────────────────────────────── */

static void test_kallsyms_empty_content()
{
	auto& tf = test_framework_manager::get();
	tf.reset();
	tf.set_replay(DATA_DIR + "/replay_kallsyms_empty.ptrecord");

	/*
	 * First (and only) call to kernel_function() in this binary.
	 * read_kallsyms() fires, read_file_content() returns "" (N record),
	 * and the early-return at line 118 is taken.
	 */
	std::string sym = kernel_function(0xdeadbeef);
	PT_ASSERT_EQ(sym, std::string(""));  /* no symbols loaded → "" */

	tf.reset();
}

/* ── main ───────────────────────────────────────────────────────────────── */

int main()
{
	std::cout << "=== read_kallsyms empty content ===\n";
	PT_RUN_TEST(test_kallsyms_empty_content);
	return pt_test_summary();
}
