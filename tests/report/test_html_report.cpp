/*
 * Tests for the HTML report generator (src/report/).
 *
 * Covers: report.cpp (cpu_model, system_info, init/finish_report_output),
 *         report-maker.cpp, report-formatter-html.cpp,
 *         report-formatter-base.cpp, report-data-html.cpp.
 *
 * The full-pipeline test uses the replay framework so no real hardware is
 * needed.  The formatter tests drive report_maker directly with no I/O.
 *
 * If HAVE_TIDY is defined (set by meson when `tidy` is found), every test
 * that produces HTML also validates it with `tidy -e -q`.
 */

#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "test_helper.h"
#include "lib.h"
#include "test_framework.h"
#include "report/report.h"
#include "report/report-data-html.h"

/* lib.cpp uses this as an extern function pointer defined in main.cpp */
void (*ui_notify_user)(const std::string &) = nullptr;

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

static std::string make_html_tmpfile()
{
	char tmp[] = "/tmp/pt_html_XXXXXX";
	int fd = mkstemp(tmp);
	close(fd);
	std::string html = std::string(tmp) + ".html";
	rename(tmp, html.c_str());
	return html;
}

static std::string read_file(const std::string &path)
{
	std::ifstream f(path);
	std::ostringstream ss;
	ss << f.rdbuf();
	return ss.str();
}

#ifdef HAVE_TIDY
static bool tidy_ok(const std::string &path)
{
	std::string cmd = std::string(TIDY_BIN) + " -e -q " + path + " 2>/dev/null";
	int status = system(cmd.c_str());
	/* tidy exits 0=ok, 1=warnings, 2=errors; we accept warnings */
	return WEXITSTATUS(status) < 2;
}
#endif

/* --------------------------------------------------------------------------
 * Test 1: full pipeline via report.cpp globals + replay fixture
 * -------------------------------------------------------------------------- */

static void test_html_full_pipeline()
{
	test_framework_manager::get().reset();
	test_framework_manager::get().set_replay(
		std::string(TEST_DATA_DIR) + "/report_sysinfo.ptrecord");

	std::string html = make_html_tmpfile();

	reporttype = REPORT_HTML;
	init_report_output(html, 1);
	finish_report_output();
	reporttype = REPORT_OFF;

	test_framework_manager::get().reset();

	std::string content = read_file(html);
	PT_ASSERT_TRUE(content.size() > 1000);
	/* CSS header */
	PT_ASSERT_TRUE(content.find("<!DOCTYPE html>") != std::string::npos);
	/* System info table entries from fixture */
	PT_ASSERT_TRUE(content.find("Gigabyte") != std::string::npos);
	PT_ASSERT_TRUE(content.find("i9-7900X") != std::string::npos);
	PT_ASSERT_TRUE(content.find("Debian") != std::string::npos);
	PT_ASSERT_TRUE(content.find("6.19.11") != std::string::npos);

#ifdef HAVE_TIDY
	PT_ASSERT_TRUE(tidy_ok(html));
#endif
	unlink(html.c_str());
}

/* --------------------------------------------------------------------------
 * Test 2: exercise all report_formatter_html methods via a local report_maker
 * -------------------------------------------------------------------------- */

static void test_html_formatter_methods()
{
	test_framework_manager::get().reset();

	report_maker r(REPORT_HTML);

	r.add_header();
	r.add_logo();

	/* add_div: all four combinations of css_class / css_id */
	tag_attr d1; d1.css_class = "";        d1.css_id = "";
	r.add_div(&d1);  r.end_div();

	tag_attr d2; d2.css_class = "";        d2.css_id = "myid";
	r.add_div(&d2);  r.end_div();

	tag_attr d3; d3.css_class = "myclass"; d3.css_id = "";
	r.add_div(&d3);  r.end_div();

	tag_attr d4; d4.css_class = "myclass"; d4.css_id = "myid";
	r.add_div(&d4);
	tag_attr title_attr; init_title_attr(&title_attr);
	r.add_title(&title_attr, "Test Section");
	r.add_navigation();
	r.end_div();

	r.end_header();
	r.finish_report();

	std::string result = r.get_result();
	PT_ASSERT_TRUE(result.find("<div>") != std::string::npos);
	PT_ASSERT_TRUE(result.find("id=\"myid\"") != std::string::npos);
	PT_ASSERT_TRUE(result.find("class=\"myclass\"") != std::string::npos);
	PT_ASSERT_TRUE(result.find("Test Section") != std::string::npos);
	PT_ASSERT_TRUE(result.find("main_menu") != std::string::npos);

	std::string html = make_html_tmpfile();
	std::ofstream f(html);
	f << result;
	f.close();
#ifdef HAVE_TIDY
	PT_ASSERT_TRUE(tidy_ok(html));
#endif
	unlink(html.c_str());
}

/* --------------------------------------------------------------------------
 * Test 3: add_table — covers all `position` enum branches in add_table()
 * -------------------------------------------------------------------------- */

static void test_html_tables()
{
	test_framework_manager::get().reset();

	report_maker r(REPORT_HTML);
	r.add_header();

	std::vector<std::string> data = {"H1", "H2", "R1C1", "R1C2",
	                                 "R2C1", "R2C2", "R3C1", "R3C2"};

	/* T: top row is header row; non-header cells with empty td_class */
	{
		tag_attr d; d.css_class = "sec"; d.css_id = "t1"; r.add_div(&d);
		table_attributes ta; init_std_table_attr(&ta, 4, 2);
		r.add_table(data, &ta);
		r.end_div();
	}

	/* T: non-header cells with non-empty td_class (no_wrap variant) */
	{
		tag_attr d; d.css_class = "sec"; d.css_id = "t2"; r.add_div(&d);
		table_attributes ta; init_nowarp_table_attr(&ta, 4, 2);
		r.add_table(data, &ta);
		r.end_div();
	}

	/* L: left column is header column */
	{
		tag_attr d; d.css_class = "sec"; d.css_id = "t3"; r.add_div(&d);
		table_attributes ta; init_top_table_attr(&ta, 4, 2);
		r.add_table(data, &ta);
		r.end_div();
	}

	/* TL: first row AND first column are headers (manual setup) */
	{
		tag_attr d; d.css_class = "sec"; d.css_id = "t4"; r.add_div(&d);
		table_attributes ta;
		ta.table_class = "test_tl"; ta.th_class = "header";
		ta.td_class = ""; ta.tr_class = ""; ta.title_mod = 0;
		ta.pos_table_title = TL; ta.rows = 4; ta.cols = 2;
		r.add_table(data, &ta);
		r.end_div();
	}

	/* TC: header every title_mod rows */
	{
		tag_attr d; d.css_class = "sec"; d.css_id = "t5"; r.add_div(&d);
		table_attributes ta; init_core_table_attr(&ta, 2, 4, 2);
		r.add_table(data, &ta);
		r.end_div();
	}

	/* TLC: TC plus left column */
	{
		tag_attr d; d.css_class = "sec"; d.css_id = "t6"; r.add_div(&d);
		table_attributes ta; init_cpu_table_attr(&ta, 2, 4, 2);
		r.add_table(data, &ta);
		r.end_div();
	}

	r.end_header();
	r.finish_report();

	std::string result = r.get_result();
	PT_ASSERT_TRUE(result.find("<table") != std::string::npos);
	PT_ASSERT_TRUE(result.find("<th") != std::string::npos);
	PT_ASSERT_TRUE(result.find("<td") != std::string::npos);
	PT_ASSERT_TRUE(result.find("no_wrap") != std::string::npos);

	std::string html = make_html_tmpfile();
	std::ofstream f(html);
	f << result;
	f.close();
#ifdef HAVE_TIDY
	PT_ASSERT_TRUE(tidy_ok(html));
#endif
	unlink(html.c_str());
}

/* --------------------------------------------------------------------------
 * Test 4: add_summary_list — even and odd-length lists
 * -------------------------------------------------------------------------- */

static void test_html_summary_list()
{
	test_framework_manager::get().reset();

	report_maker r(REPORT_HTML);
	r.add_header();

	/* Even list: all items paired */
	std::vector<std::string> even = {"Label A", "Value A", "Label B", "Value B"};
	r.add_summary_list(even);

	/* Odd list: last item is unpaired — loop bound is i+1 < size, so
	 * "Orphan" at index 2 is skipped */
	std::vector<std::string> odd = {"Label X", "Value X", "Orphan"};
	r.add_summary_list(odd);

	r.end_header();
	r.finish_report();

	std::string result = r.get_result();
	PT_ASSERT_TRUE(result.find("Label A") != std::string::npos);
	PT_ASSERT_TRUE(result.find("Value B") != std::string::npos);
	PT_ASSERT_TRUE(result.find("Label X") != std::string::npos);
	PT_ASSERT_TRUE(result.find("Orphan") == std::string::npos);
}

/* --------------------------------------------------------------------------
 * Test 5: escape_string — special HTML characters via add()
 * -------------------------------------------------------------------------- */

static void test_html_escape()
{
	test_framework_manager::get().reset();

	report_maker r(REPORT_HTML);
	/* add() routes through escape_string(); add_exact() does not */
	r.add("<b>foo & bar</b>");
	std::string result = r.get_result();

	PT_ASSERT_TRUE(result.find("&lt;b&gt;") != std::string::npos);
	PT_ASSERT_TRUE(result.find("&amp;") != std::string::npos);
	/* Raw angle brackets must be gone */
	PT_ASSERT_TRUE(result.find("<b>") == std::string::npos);
}

/* -------------------------------------------------------------------------- */

int main()
{
	PT_RUN_TEST(test_html_full_pipeline);
	PT_RUN_TEST(test_html_formatter_methods);
	PT_RUN_TEST(test_html_tables);
	PT_RUN_TEST(test_html_summary_list);
	PT_RUN_TEST(test_html_escape);
	return pt_test_summary();
}
