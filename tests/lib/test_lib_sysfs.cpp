/*
 * Tests for I/O functions in src/lib.cpp that go through the test framework:
 *   kernel_function, read_sysfs, read_sysfs_string,
 *   write_sysfs, read_msr
 *
 * Fixtures in data/ were seeded from live system values:
 *   cpuinfo_max_freq = 4500000, scaling_governor = "powersave"
 *   MSR 0x1a0 (IA32_MISC_ENABLE) replay value = 0x850089
 *
 * NOTE: kernel_function() uses a static kallsyms_read flag that is set on
 * the first call and never cleared.  The kernel_function test MUST run
 * first in this binary so it sees the replay fixture; subsequent calls in
 * the same process use the cached (empty) map and are unaffected.
 */
#include <fstream>
#include <cstdio>
#include <unistd.h>

#include "lib.h"
#include "test_framework.h"
#include "../test_helper.h"

/* Stub: defined in main.cpp, not linked here */
void (*ui_notify_user)(const std::string &) = nullptr;

static const std::string DATA_DIR = TEST_DATA_DIR;

/* ── kernel_function (MUST be first) ────────────────────────────────────── */

static void test_kernel_function_replay()
{
	auto& tf = test_framework_manager::get();
	tf.reset();
	tf.set_replay(DATA_DIR + "/replay_kallsyms.ptrecord");

	/* First call triggers read_kallsyms(), consuming the replay entry */
	PT_ASSERT_EQ(kernel_function(0xffffffff81001234UL), "startup_64");

	/* Subsequent calls use the cached map */
	PT_ASSERT_EQ(kernel_function(0xffffffff8200abcdUL), "do_page_fault");

	/* Address not in fixture returns empty string */
	PT_ASSERT_EQ(kernel_function(0x0000111111111111UL), "");

	tf.reset();
}

/* ── read_sysfs ─────────────────────────────────────────────────────────── */

static void test_read_sysfs_returns_integer()
{
	auto& tf = test_framework_manager::get();
	tf.reset();
	tf.set_replay(DATA_DIR + "/replay_sysfs_reads.ptrecord");

	bool ok = false;
	int val = read_sysfs(
	    "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", &ok);

	PT_ASSERT_EQ(ok, true);
	PT_ASSERT_EQ(val, 4500000);
	tf.reset();
}

static void test_read_sysfs_missing_file_returns_zero()
{
	auto& tf = test_framework_manager::get();
	tf.reset();
	tf.set_replay(DATA_DIR + "/replay_sysfs_reads.ptrecord");

	bool ok = true;
	int val = read_sysfs("/sys/nonexistent/file", &ok);

	PT_ASSERT_EQ(ok, false);
	PT_ASSERT_EQ(val, 0);
	tf.reset();
}

/* ── read_sysfs_string ──────────────────────────────────────────────────── */

static void test_read_sysfs_string_strips_newline()
{
	auto& tf = test_framework_manager::get();
	tf.reset();
	tf.set_replay(DATA_DIR + "/replay_sysfs_reads.ptrecord");

	std::string s = read_sysfs_string(
	    "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");

	/* Fixture content is "powersave\n"; the function strips the newline */
	PT_ASSERT_EQ(s, "powersave");
	tf.reset();
}

/* ── write_sysfs ────────────────────────────────────────────────────────── */

static void test_write_sysfs_recording_and_replay()
{
	auto& tf = test_framework_manager::get();

	/* Create temp files */
	char real_tmp[] = "/tmp/powertop_write_XXXXXX";
	int real_fd = mkstemp(real_tmp);
	close(real_fd);

	char rec_tmp[] = "/tmp/powertop_rec_XXXXXX";
	int rec_fd = mkstemp(rec_tmp);
	close(rec_fd);

	/* Record: write_sysfs writes to the real file AND records the op */
	tf.reset();
	tf.set_record(rec_tmp);
	write_sysfs(real_tmp, "hello");
	tf.save();
	tf.reset();

	/* Verify the actual file received the write */
	std::ifstream f(real_tmp);
	std::string content((std::istreambuf_iterator<char>(f)), {});
	PT_ASSERT_EQ(content, "hello");

	/* Replay: write_sysfs in replay mode validates the value matches */
	tf.set_replay(rec_tmp);
	write_sysfs(real_tmp, "hello");  /* must match what was recorded */
	tf.reset();

	unlink(real_tmp);
	unlink(rec_tmp);
}

/* ── read_msr ───────────────────────────────────────────────────────────── */

static void test_read_msr_replay()
{
	auto& tf = test_framework_manager::get();
	tf.reset();
	tf.set_replay(DATA_DIR + "/replay_msr_read.ptrecord");

	/*
	 * Fixture: M 0 1a0 850089
	 * cpu=0, offset=0x1a0 (IA32_MISC_ENABLE), value=0x850089
	 */
	uint64_t value = 0;
	int ret = read_msr(0, 0x1a0, &value);

	PT_ASSERT_EQ(ret, 0);
	PT_ASSERT_EQ(value, (uint64_t)0x850089);
	tf.reset();
}

/* ── main ───────────────────────────────────────────────────────────────── */

int main()
{
	/* kernel_function must run first — see file header comment */
	std::cout << "=== kernel_function tests ===\n";
	PT_RUN_TEST(test_kernel_function_replay);

	std::cout << "\n=== read_sysfs tests ===\n";
	PT_RUN_TEST(test_read_sysfs_returns_integer);
	PT_RUN_TEST(test_read_sysfs_missing_file_returns_zero);

	std::cout << "\n=== read_sysfs_string tests ===\n";
	PT_RUN_TEST(test_read_sysfs_string_strips_newline);

	std::cout << "\n=== write_sysfs tests ===\n";
	PT_RUN_TEST(test_write_sysfs_recording_and_replay);

	std::cout << "\n=== read_msr tests ===\n";
	PT_RUN_TEST(test_read_msr_replay);

	return pt_test_summary();
}
