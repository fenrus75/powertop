#include <iostream>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include <fstream>

#include "report/report-maker.h"
#include "report/report-formatter-md.h"
#include "report/report-data-html.h"
#include "test_framework.h"
#include "../test_helper.h"

void (*ui_notify_user)(const std::string &) = nullptr;

/* only used by the markdown linter checks, which depend on HAVE_MDL/HAVE_MDFORMAT */
[[maybe_unused]] static std::string make_md_tmpfile()
{
	char tmp[] = "/tmp/pt_md_XXXXXX";
	int fd = mkstemp(tmp);
	close(fd);
	std::string md = std::string(tmp) + ".md";
	rename(tmp, md.c_str());
	return md;
}

#ifdef HAVE_MDL
static bool mdl_ok(const std::string &path)
{
	std::string cmd = std::string(MDL_BIN) + " " + path + " > /dev/null 2>&1";
	int status = system(cmd.c_str());
	return WEXITSTATUS(status) == 0;
}
#endif

#ifdef HAVE_MDFORMAT
static bool mdformat_ok(const std::string &path)
{
	/* mdformat --check returns 0 if formatted, non-zero otherwise */
	std::string cmd = std::string(MDFORMAT_BIN) + " --check " + path + " > /dev/null 2>&1";
	int status = system(cmd.c_str());
	return WEXITSTATUS(status) == 0;
}
#endif

static void test_md_basic()
{
	report_maker report(REPORT_MD);
	
	report.add_logo();
	report.add_header();
	
	struct tag_attr title_attr;
	report.add_title(&title_attr, "Summary");
	
	std::vector<std::string> summary = {"Field1", "Value1", "Field2", "Value2"};
	report.add_summary_list(summary);
	
	struct table_attributes tb_attr;
	tb_attr.rows = 2;
	tb_attr.cols = 2;
	/* default is pos_table_title = T */
	std::vector<std::string> table_data = {"Header1", "Header2", "Row1Col1", "Row1Col2"};
	report.add_table(table_data, &tb_attr);
	
	report.finish_report();
	std::string res = report.get_result();
	
	PT_ASSERT_TRUE(res.find("# PowerTOP Report") != std::string::npos);
	PT_ASSERT_TRUE(res.find("______________________________________________________________________") != std::string::npos);
	PT_ASSERT_TRUE(res.find("## Summary") != std::string::npos);
	PT_ASSERT_TRUE(res.find("- **Field1**: Value1") != std::string::npos);
	PT_ASSERT_TRUE(res.find("| Header1 | Header2 |") != std::string::npos);
	PT_ASSERT_TRUE(res.find("|---|---|") != std::string::npos);
	PT_ASSERT_TRUE(res.find("| Row1Col1 | Row1Col2 |") != std::string::npos);
	/* no NaN or Inf values must appear in table output */
	PT_ASSERT_TRUE(res.find("nan") == std::string::npos);
	PT_ASSERT_TRUE(res.find("inf") == std::string::npos);
}

static void test_md_l_table()
{
	report_maker report(REPORT_MD);
	
	struct table_attributes tb_attr;
	tb_attr.rows = 2;
	tb_attr.cols = 2;
	tb_attr.pos_table_title = L;
	std::vector<std::string> table_data = {"Prop1", "Val1", "Prop2", "Val2"};
	report.add_table(table_data, &tb_attr);
	
	report.finish_report();
	std::string res = report.get_result();
	
	PT_ASSERT_TRUE(res.find("| Property | Value |") != std::string::npos);
	PT_ASSERT_TRUE(res.find("|---|---|") != std::string::npos);
	PT_ASSERT_TRUE(res.find("| Prop1 | Val1 |") != std::string::npos);
	PT_ASSERT_TRUE(res.find("| Prop2 | Val2 |") != std::string::npos);
}

static void test_md_mdl()
{
#ifdef HAVE_MDL
	report_maker report(REPORT_MD);
	
	report.add_logo();
	report.add_header();
	
	struct tag_attr title_attr;
	report.add_title(&title_attr, "Summary");
	
	std::vector<std::string> summary = {"Field1", "Value1", "Field2", "Value2"};
	report.add_summary_list(summary);
	
	struct table_attributes tb_attr;
	tb_attr.rows = 2;
	tb_attr.cols = 2;
	tb_attr.pos_table_title = L;
	std::vector<std::string> table_data = {"Prop1", "Val1", "Prop2", "Val2"};
	report.add_table(table_data, &tb_attr);
	
	report.finish_report();
	std::string res = report.get_result();

	std::string md_path = make_md_tmpfile();
	std::ofstream f(md_path);
	f << res;
	f.close();

	PT_ASSERT_TRUE(mdl_ok(md_path));
	unlink(md_path.c_str());
#endif
}

static void test_md_mdformat()
{
#ifdef HAVE_MDFORMAT
	report_maker report(REPORT_MD);
	
	report.add_logo();
	report.add_header();
	
	struct tag_attr title_attr;
	report.add_title(&title_attr, "Summary");
	
	std::vector<std::string> summary = {"Field1", "Value1", "Field2", "Value2"};
	report.add_summary_list(summary);
	
	struct table_attributes tb_attr;
	tb_attr.rows = 2;
	tb_attr.cols = 2;
	tb_attr.pos_table_title = L;
	std::vector<std::string> table_data = {"Prop1", "Val1", "Prop2", "Val2"};
	report.add_table(table_data, &tb_attr);
	
	report.finish_report();
	std::string res = report.get_result();

	std::string md_path = make_md_tmpfile();
	std::ofstream f(md_path);
	f << res;
	f.close();

	PT_ASSERT_TRUE(mdformat_ok(md_path));
	unlink(md_path.c_str());
#endif
}

int main()
{
	PT_RUN_TEST(test_md_basic);
	PT_RUN_TEST(test_md_l_table);
	PT_RUN_TEST(test_md_mdl);
	PT_RUN_TEST(test_md_mdformat);
	return pt_test_summary();
}
