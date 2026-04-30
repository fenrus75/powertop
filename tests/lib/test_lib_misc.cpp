/*
 * Tests for pt_gettime() and align_string() in src/lib.cpp
 *
 * Also covers: ui_notify_user_console
 */
#include <cstdio>
#include <unistd.h>
#include <locale.h>

#include "lib.h"
#include "test_framework.h"
#include "../test_helper.h"

/* Stub required because lib.cpp declares it extern (defined in main.cpp) */
void (*ui_notify_user)(const std::string &) = nullptr;

static const std::string DATA_DIR = TEST_DATA_DIR;

/* ── pt_gettime: normal mode ─────────────────────────────────────────────── */

static void test_pt_gettime_normal_returns_real_time()
{
	test_framework_manager::get().reset();
	struct timeval tv = pt_gettime();
	/* tv_sec should be a plausible Unix timestamp (post-2001) */
	PT_ASSERT_TRUE(tv.tv_sec > 1000000000);
	PT_ASSERT_TRUE(tv.tv_usec >= 0 && tv.tv_usec < 1000000);
}

/* ── pt_gettime: recording mode ──────────────────────────────────────────── */

static void test_pt_gettime_recording_round_trip()
{
	test_framework_manager::get().reset();

	char tmpfile[] = "/tmp/pt_test_time_XXXXXX";
	int fd = mkstemp(tmpfile);
	close(fd);

	test_framework_manager::get().set_record(tmpfile);
	struct timeval tv_recorded = pt_gettime();

	test_framework_manager::get().save();
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(tmpfile);

	struct timeval tv_replayed = pt_gettime();
	PT_ASSERT_EQ(tv_replayed.tv_sec,  tv_recorded.tv_sec);
	PT_ASSERT_EQ(tv_replayed.tv_usec, tv_recorded.tv_usec);

	test_framework_manager::get().reset();
	unlink(tmpfile);
}

/* ── pt_gettime: replay mode ─────────────────────────────────────────────── */

static void test_pt_gettime_replay_returns_fixture_time()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(DATA_DIR + "/replay_time.ptrecord");

	struct timeval tv = pt_gettime();
	PT_ASSERT_EQ(tv.tv_sec,  (time_t)1000000000);
	PT_ASSERT_EQ(tv.tv_usec, (suseconds_t)123456);

	test_framework_manager::get().reset();
}

/* ── align_string ────────────────────────────────────────────────────────── */

static void test_align_string_pads_short_string()
{
	std::string s = "abc";
	align_string(s, 8, 100);
	PT_ASSERT_EQ(s, "abc     ");   /* padded to 8 chars */
}

static void test_align_string_already_long_enough()
{
	std::string s = "abcdefghij";   /* 10 chars */
	align_string(s, 5, 100);
	PT_ASSERT_EQ(s, "abcdefghij");  /* unchanged */
}

static void test_align_string_exact_min_sz()
{
	std::string s = "hello";
	align_string(s, 5, 100);
	PT_ASSERT_EQ(s, "hello");       /* exactly at min_sz, unchanged */
}

static void test_align_string_empty_string()
{
	std::string s = "";
	align_string(s, 4, 100);
	PT_ASSERT_EQ(s, "    ");        /* four spaces */
}

static void test_align_string_max_sz_has_no_effect()
{
	/*
	 * POSIX specifies that mbsrtowcs() ignores nwc when dst is nullptr,
	 * so max_sz does not limit the counted width. The full string is
	 * always counted, so a string longer than min_sz is never padded
	 * regardless of max_sz.
	 */
	std::string s = "abcdefghij";   /* 10 chars, sz=10 */
	align_string(s, 7, 3);          /* max_sz=3, but ignored */
	PT_ASSERT_EQ(s, "abcdefghij");  /* 10 >= 7, no padding added */
}

/* ── ui_notify_user_console ───────────────────────────────────────────────── */

static void test_ui_notify_user_console()
{
	/* Just calls printf — verify it doesn't crash */
	ui_notify_user_console("powertop test message");
}

/* ── main ────────────────────────────────────────────────────────────────── */

int main()
{
	setlocale(LC_ALL, "");

	std::cout << "=== pt_gettime tests ===\n";
	PT_RUN_TEST(test_pt_gettime_normal_returns_real_time);
	PT_RUN_TEST(test_pt_gettime_recording_round_trip);
	PT_RUN_TEST(test_pt_gettime_replay_returns_fixture_time);

	std::cout << "\n=== align_string tests ===\n";
	PT_RUN_TEST(test_align_string_pads_short_string);
	PT_RUN_TEST(test_align_string_already_long_enough);
	PT_RUN_TEST(test_align_string_exact_min_sz);
	PT_RUN_TEST(test_align_string_empty_string);
	PT_RUN_TEST(test_align_string_max_sz_has_no_effect);

	std::cout << "\n=== ui_notify_user_console tests ===\n";
	PT_RUN_TEST(test_ui_notify_user_console);

	return pt_test_summary();
}
