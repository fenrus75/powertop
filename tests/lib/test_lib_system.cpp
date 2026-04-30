/*
 * Tests for filesystem-touching functions in src/lib.cpp:
 *   get_max_cpu / set_max_cpu, process_directory, process_glob
 *
 * These tests create real temporary files/directories and clean them up
 * afterwards.  No test-framework fixtures are needed.
 */
#include <algorithm>
#include <vector>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "lib.h"
#include "test_framework.h"
#include "../test_helper.h"

/* Stub: defined in main.cpp, not linked here */
void (*ui_notify_user)(const std::string &) = nullptr;

/* ── get_max_cpu / set_max_cpu ──────────────────────────────────────────── */

static void test_set_get_max_cpu()
{
	int base = get_max_cpu();

	/* Larger value is accepted */
	set_max_cpu(base + 100);
	PT_ASSERT_EQ(get_max_cpu(), base + 100);

	/* Smaller value is ignored — _max_cpu only ever grows */
	set_max_cpu(base + 50);
	PT_ASSERT_EQ(get_max_cpu(), base + 100);

	/* Even larger value updates again */
	set_max_cpu(base + 200);
	PT_ASSERT_EQ(get_max_cpu(), base + 200);
}

/* ── process_directory ──────────────────────────────────────────────────── */

/* callback cannot capture — use a file-scope collector */
static std::vector<std::string> dir_entries;
static void collect_dir_entry(const std::string& name)
{
	dir_entries.push_back(name);
}

static void test_process_directory()
{
	char tmpdir[] = "/tmp/powertop_dir_XXXXXX";
	mkdtemp(tmpdir);
	std::string d(tmpdir);

	for (const char *name : {"alpha", "beta", "gamma"}) {
		int fd = open((d + "/" + name).c_str(), O_CREAT | O_WRONLY, 0644);
		close(fd);
	}

	dir_entries.clear();
	process_directory(d, collect_dir_entry);

	std::sort(dir_entries.begin(), dir_entries.end());
	PT_ASSERT_EQ(dir_entries.size(), (size_t)3);
	PT_ASSERT_EQ(dir_entries[0], "alpha");
	PT_ASSERT_EQ(dir_entries[1], "beta");
	PT_ASSERT_EQ(dir_entries[2], "gamma");

	for (const char *name : {"alpha", "beta", "gamma"})
		unlink((d + "/" + name).c_str());
	rmdir(tmpdir);
}

static void test_process_directory_nonexistent_is_silent()
{
	/* process_directory on a missing dir should simply do nothing */
	dir_entries.clear();
	process_directory("/tmp/powertop_no_such_dir_xyz", collect_dir_entry);
	PT_ASSERT_EQ(dir_entries.size(), (size_t)0);
}

/* ── process_glob ───────────────────────────────────────────────────────── */

static std::vector<std::string> glob_entries;
static void collect_glob_entry(const std::string& path)
{
	glob_entries.push_back(path);
}

static void test_process_glob_matches_pattern()
{
	char tmpdir[] = "/tmp/powertop_glob_XXXXXX";
	mkdtemp(tmpdir);
	std::string d(tmpdir);

	/* Create two .txt files and one .cpp — only .txt should be matched */
	for (const char *name : {"foo.txt", "bar.txt", "baz.cpp"}) {
		int fd = open((d + "/" + name).c_str(), O_CREAT | O_WRONLY, 0644);
		close(fd);
	}

	glob_entries.clear();
	process_glob(d + "/*.txt", collect_glob_entry);

	std::sort(glob_entries.begin(), glob_entries.end());
	PT_ASSERT_EQ(glob_entries.size(), (size_t)2);
	PT_ASSERT_EQ(glob_entries[0], d + "/bar.txt");
	PT_ASSERT_EQ(glob_entries[1], d + "/foo.txt");

	for (const char *name : {"foo.txt", "bar.txt", "baz.cpp"})
		unlink((d + "/" + name).c_str());
	rmdir(tmpdir);
}

static void test_process_glob_no_match()
{
	/*
	 * Use an existing directory with a pattern that matches nothing, so
	 * glob() returns GLOB_NOMATCH (not GLOB_ABORTED which fires when the
	 * directory itself doesn't exist under GLOB_ERR).
	 */
	char tmpdir[] = "/tmp/pt_glob_nomatch_XXXXXX";
	mkdtemp(tmpdir);
	glob_entries.clear();
	process_glob(std::string(tmpdir) + "/*.nothing_xyz_abc", collect_glob_entry);
	PT_ASSERT_EQ(glob_entries.size(), (size_t)0);
	rmdir(tmpdir);
}

/* ── main ───────────────────────────────────────────────────────────────── */

int main()
{
	std::cout << "=== get_max_cpu / set_max_cpu tests ===\n";
	PT_RUN_TEST(test_set_get_max_cpu);

	std::cout << "\n=== process_directory tests ===\n";
	PT_RUN_TEST(test_process_directory);
	PT_RUN_TEST(test_process_directory_nonexistent_is_silent);

	std::cout << "\n=== process_glob tests ===\n";
	PT_RUN_TEST(test_process_glob_matches_pattern);
	PT_RUN_TEST(test_process_glob_no_match);

	return pt_test_summary();
}
