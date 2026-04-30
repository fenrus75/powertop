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
#include <sys/stat.h>

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

/* ── read_sysfs: non-integer content (catch block, lines 210-214) ────────── */

static void test_read_sysfs_non_integer_content()
{
	auto& tf = test_framework_manager::get();
	tf.reset();
	tf.set_replay(DATA_DIR + "/replay_sysfs_reads.ptrecord");

	/* Consume the first three standard entries first */
	bool ok = true;
	read_sysfs("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", &ok);
	ok = true;
	read_sysfs("/sys/nonexistent/file", &ok);
	read_sysfs_string("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");

	/* Fourth fixture entry: "not_a_number\n" — stoi throws → ok=false */
	ok = true;
	int val = read_sysfs("/sys/devices/system/cpu/cpu0/cpufreq/not_an_int", &ok);
	PT_ASSERT_EQ(ok, false);
	PT_ASSERT_EQ(val, 0);

	tf.reset();
}

/* ── read_sysfs_string: content without trailing newline ─────────────────── */

static void test_read_sysfs_string_no_newline()
{
	auto& tf = test_framework_manager::get();
	tf.reset();
	tf.set_replay(DATA_DIR + "/replay_sysfs_reads.ptrecord");

	/* Consume first four entries, then get the no-newline one */
	bool ok;
	read_sysfs("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", &ok);
	read_sysfs("/sys/nonexistent/file", nullptr);
	read_sysfs_string("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
	read_sysfs("/sys/devices/system/cpu/cpu0/cpufreq/not_an_int", nullptr);

	/* Fifth fixture entry: "value_without_newline" — no '\n', no erase */
	std::string s = read_sysfs_string(
	    "/sys/devices/system/cpu/cpu0/cpufreq/no_newline");
	PT_ASSERT_EQ(s, "value_without_newline");

	tf.reset();
}

/* ── pt_readlink: non-replay paths (lines 263-276) ──────────────────────── */

static void test_pt_readlink_real_symlink()
{
	auto& tf = test_framework_manager::get();
	tf.reset(); /* not in replay mode */

	char tmpdir[] = "/tmp/pt_link_test_XXXXXX";
	mkdtemp(tmpdir);
	std::string target = std::string(tmpdir) + "/target";
	std::string link   = std::string(tmpdir) + "/mylink";

	/* Create a real file and symlink to it */
	{ std::ofstream f(target); f << "x"; }
	symlink(target.c_str(), link.c_str());

	std::string result = pt_readlink(link);
	PT_ASSERT_EQ(result, target);

	unlink(link.c_str());
	unlink(target.c_str());
	rmdir(tmpdir);
}

static void test_pt_readlink_nonexistent()
{
	auto& tf = test_framework_manager::get();
	tf.reset(); /* not in replay mode */

	/* Non-existent path → filesystem_error caught → returns "" */
	std::string result = pt_readlink("/tmp/pt_no_such_symlink_xyz");
	PT_ASSERT_EQ(result, std::string(""));
}

static void test_pt_readlink_recording()
{
	auto& tf = test_framework_manager::get();

	char tmpdir[] = "/tmp/pt_link_rec_XXXXXX";
	mkdtemp(tmpdir);
	std::string target = std::string(tmpdir) + "/target";
	std::string link   = std::string(tmpdir) + "/mylink";
	char rec_tmp[]     = "/tmp/pt_link_record_XXXXXX";
	int fd = mkstemp(rec_tmp); close(fd);

	{ std::ofstream f(target); f << "x"; }
	symlink(target.c_str(), link.c_str());

	tf.reset();
	tf.set_record(rec_tmp);
	std::string recorded = pt_readlink(link);
	tf.save();
	tf.reset();

	PT_ASSERT_EQ(recorded, target);

	/* Replay should return the same value */
	tf.set_replay(rec_tmp);
	std::string replayed = pt_readlink(link);
	tf.reset();
	PT_ASSERT_EQ(replayed, target);

	unlink(link.c_str());
	unlink(target.c_str());
	rmdir(tmpdir);
	unlink(rec_tmp);
}

static void test_pt_readlink_recording_fail()
{
	auto& tf = test_framework_manager::get();
	char rec_tmp[] = "/tmp/pt_link_recfail_XXXXXX";
	int fd = mkstemp(rec_tmp); close(fd);

	/* Record a failed readlink (non-existent path) → covers line 268 */
	tf.reset();
	tf.set_record(rec_tmp);
	std::string result = pt_readlink("/tmp/pt_absolutely_no_such_path_xyz");
	tf.save();
	tf.reset();

	PT_ASSERT_EQ(result, std::string(""));

	/* Replay should also return "" */
	tf.set_replay(rec_tmp);
	std::string replayed = pt_readlink("/tmp/pt_absolutely_no_such_path_xyz");
	tf.reset();
	PT_ASSERT_EQ(replayed, std::string(""));

	unlink(rec_tmp);
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
	PT_RUN_TEST(test_read_sysfs_non_integer_content);
	PT_RUN_TEST(test_read_sysfs_string_no_newline);

	std::cout << "\n=== write_sysfs tests ===\n";
	PT_RUN_TEST(test_write_sysfs_recording_and_replay);

	std::cout << "\n=== read_msr tests ===\n";
	PT_RUN_TEST(test_read_msr_replay);

	std::cout << "\n=== pt_readlink tests ===\n";
	PT_RUN_TEST(test_pt_readlink_real_symlink);
	PT_RUN_TEST(test_pt_readlink_nonexistent);
	PT_RUN_TEST(test_pt_readlink_recording);
	PT_RUN_TEST(test_pt_readlink_recording_fail);

	return pt_test_summary();
}
