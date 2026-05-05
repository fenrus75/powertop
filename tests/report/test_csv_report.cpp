/*
 * Tests for the CSV report generator (src/report/report-formatter-csv.cpp).
 *
 * Covers: all report_formatter_csv methods via report_maker(REPORT_CSV):
 *   - structural methods (add_header, add_logo, add_div, add_title, ...)
 *   - add_table: semicolon-delimited output, &nbsp; empty-row suppression
 *   - add_summary_list: even and odd-length lists (bug-fix: was UB on odd)
 *   - escape_string: double-quote doubling via add()
 *
 * No replay fixture needed — CSV formatter has no file I/O.
 */

#include <string>
#include <vector>

#include "test_helper.h"
#include "report/report-maker.h"
#include "report/report-data-html.h"

/* lib.cpp uses this as an extern function pointer defined in main.cpp */
void (*ui_notify_user)(const std::string &) = nullptr;

/* --------------------------------------------------------------------------
 * Test 1: structural / style methods
 * -------------------------------------------------------------------------- */

static void test_csv_formatter_methods()
{

	report_maker r(REPORT_CSV);

	r.add_header();
	r.add_logo();

	tag_attr d; d.css_class = ""; d.css_id = "";
	r.add_div(&d);

	tag_attr ta; init_title_attr(&ta);
	r.add_title(&ta, "Overview");

	r.add_navigation(); /* no-op for CSV */
	r.end_div();        /* no-op for CSV */
	r.end_header();     /* no-op for CSV */
	r.finish_report();  /* no-op for CSV */

	std::string result = r.get_result();
	PT_ASSERT_TRUE(result.find("____") != std::string::npos);
	PT_ASSERT_TRUE(result.find("P o w e r T O P") != std::string::npos);
	PT_ASSERT_TRUE(result.find("Overview") != std::string::npos);
	/* add_div emits a blank line */
	PT_ASSERT_TRUE(result.find('\n') != std::string::npos);
}

/* --------------------------------------------------------------------------
 * Test 2: add_table — semicolons between columns, newline after each row
 * -------------------------------------------------------------------------- */

static void test_csv_table()
{

	report_maker r(REPORT_CSV);

	std::vector<std::string> data = {
		"CPU",   "Freq",
		"Core0", "2.4GHz",
		"Core1", "2.8GHz",
	};
	table_attributes ta;
	init_std_table_attr(&ta, 3, 2);
	r.add_table(data, &ta);

	std::string result = r.get_result();
	PT_ASSERT_TRUE(result.find("CPU") != std::string::npos);
	PT_ASSERT_TRUE(result.find(";") != std::string::npos);
	PT_ASSERT_TRUE(result.find("Core0") != std::string::npos);
	PT_ASSERT_TRUE(result.find("2.8GHz") != std::string::npos);
	/* last column of each row should NOT have a trailing semicolon */
	PT_ASSERT_TRUE(result.find("Freq;") == std::string::npos);
	PT_ASSERT_TRUE(result.find("2.8GHz;") == std::string::npos);
	/* no NaN or Inf values must appear in table output */
	PT_ASSERT_TRUE(result.find("nan") == std::string::npos);
	PT_ASSERT_TRUE(result.find("inf") == std::string::npos);
}

/* --------------------------------------------------------------------------
 * Test 3: add_table — &nbsp; cells suppress that row's newline
 * -------------------------------------------------------------------------- */

static void test_csv_table_empty_rows()
{

	report_maker r(REPORT_CSV);

	/* Middle row is all &nbsp; — should be swallowed */
	std::vector<std::string> data = {
		"A",      "B",
		"&nbsp;", "&nbsp;",
		"C",      "D",
	};
	table_attributes ta;
	init_std_table_attr(&ta, 3, 2);
	r.add_table(data, &ta);

	std::string result = r.get_result();
	PT_ASSERT_TRUE(result.find("A") != std::string::npos);
	PT_ASSERT_TRUE(result.find("C") != std::string::npos);
	/* &nbsp; must not appear literally in the output */
	PT_ASSERT_TRUE(result.find("&nbsp;") == std::string::npos);
}

/* --------------------------------------------------------------------------
 * Test 3b: add_table — mixed &nbsp; cells MUST NOT shift columns
 * -------------------------------------------------------------------------- */

static void test_csv_table_mixed_empty()
{
	report_maker r(REPORT_CSV);

	/* Row with &nbsp; in middle: "A", "&nbsp;", "B"
	 * Should result in "A;;B" (two semicolons)
	 */
	std::vector<std::string> data = {
		"A", "&nbsp;", "B"
	};
	table_attributes ta;
	init_std_table_attr(&ta, 1, 3);
	r.add_table(data, &ta);

	std::string result = r.get_result();
	/* If columns shift, we get "A;B" instead of "A;;B" */
	PT_ASSERT_TRUE(result.find("A;;B") != std::string::npos);
}

/* --------------------------------------------------------------------------
 * Test 3c: add_table — entirely empty row must not corrupt next row
 *
 * Regression: the old fix emitted ";;" for the empty row but suppressed the
 * newline, causing the next row's data to be appended to the semicolons.
 * -------------------------------------------------------------------------- */

static void test_csv_table_empty_row_no_corruption()
{
	report_maker r(REPORT_CSV);

	/* Middle row is entirely &nbsp; */
	std::vector<std::string> data = {
		"A",      "B",
		"&nbsp;", "&nbsp;",
		"C",      "D",
	};
	table_attributes ta;
	init_std_table_attr(&ta, 3, 2);
	r.add_table(data, &ta);

	std::string result = r.get_result();
	/* C must be the start of its own field, not glued to a semicolon */
	PT_ASSERT_TRUE(result.find(";C") == std::string::npos);
	/* The two data rows must both be present */
	PT_ASSERT_TRUE(result.find("A;B") != std::string::npos);
	PT_ASSERT_TRUE(result.find("C;D") != std::string::npos);
}

/* --------------------------------------------------------------------------
 * Test 4: add_summary_list — even list and odd list (bug-fix verification)
 * -------------------------------------------------------------------------- */

static void test_csv_summary_list()
{

	report_maker r(REPORT_CSV);

	/* Even list: all pairs present */
	std::vector<std::string> even = {"Power", "5.2W", "CPU", "3.1W"};
	r.add_summary_list(even);

	std::string result = r.get_result();
	PT_ASSERT_TRUE(result.find("Power") != std::string::npos);
	PT_ASSERT_TRUE(result.find("5.2W") != std::string::npos);
	PT_ASSERT_TRUE(result.find("CPU 3.1W") != std::string::npos);

	/* Odd list: last element has no pair — must not crash (was UB before fix) */
	r.clear_result();
	std::vector<std::string> odd = {"Power", "5.2W", "Orphan"};
	r.add_summary_list(odd);
	result = r.get_result();
	PT_ASSERT_TRUE(result.find("Power 5.2W") != std::string::npos);
	/* "Orphan" at index 2 has no pair so must be silently skipped */
	PT_ASSERT_TRUE(result.find("Orphan") == std::string::npos);
}

/* --------------------------------------------------------------------------
 * Test 5: escape_string — double-quote is doubled (RFC 4180 style)
 * -------------------------------------------------------------------------- */

static void test_csv_escape()
{

	report_maker r(REPORT_CSV);

	/* add() routes through escape_string(); a '"' in input becomes '""' */
	r.add("say \"hello\"");
	std::string result = r.get_result();

	/* Input `"hello"` → escaped `""hello""` */
	PT_ASSERT_TRUE(result.find("\"\"hello\"\"") != std::string::npos);
	/* The comma delimiter triggers csv_need_quotes flag (exercised for coverage) */
	r.clear_result();
	r.add("a,b");
	result = r.get_result();
	PT_ASSERT_TRUE(result.find("a,b") != std::string::npos);
}

/* -------------------------------------------------------------------------- */

int main()
{
	PT_RUN_TEST(test_csv_formatter_methods);
	PT_RUN_TEST(test_csv_table);
	PT_RUN_TEST(test_csv_table_empty_rows);
	PT_RUN_TEST(test_csv_table_mixed_empty);
	PT_RUN_TEST(test_csv_table_empty_row_no_corruption);
	PT_RUN_TEST(test_csv_summary_list);
	PT_RUN_TEST(test_csv_escape);
	return pt_test_summary();
}
