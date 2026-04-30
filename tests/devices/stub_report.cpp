/*
 * Minimal report stubs for device unit tests that link ahci.cpp.
 * ahci_create_device_stats_table() and report_device_stats() reference
 * the report object but are never called by the unit tests, so we only
 * need the symbols to link — no actual HTML generation needed.
 */

#include "report/report.h"
#include "report/report-maker.h"
#include "report/report-data-html.h"

/* ── Global report objects ───────────────────────────────────────── */
report_type reporttype = REPORT_OFF;
struct reportstream reportout;

/* ── report_maker stubs ──────────────────────────────────────────── */
report_maker::report_maker(report_type t) : type(t), formatter(nullptr) {}
report_maker::~report_maker() { delete formatter; }
report_type report_maker::get_type()  { return type; }
void report_maker::set_type(report_type t) { type = t; }
void report_maker::finish_report()    {}
std::string report_maker::get_result() { return {}; }
void report_maker::clear_result()      {}
void report_maker::add(const std::string &) {}
void report_maker::add_header()        {}
void report_maker::end_header()        {}
void report_maker::add_logo()          {}
void report_maker::add_div(struct tag_attr *) {}
void report_maker::end_div()           {}
void report_maker::add_title(struct tag_attr *, const std::string &) {}
void report_maker::add_navigation()    {}
void report_maker::add_summary_list(const std::vector<std::string> &) {}
void report_maker::add_table(const std::vector<std::string> &, struct table_attributes *) {}
void report_maker::setup_report_formatter() {}

report_maker report(REPORT_OFF);

/* ── report-data-html stubs ──────────────────────────────────────── */
void init_div(struct tag_attr *a, const std::string &cls, const std::string &id)
{
	a->css_class = cls;
	a->css_id    = id;
}

void init_title_attr(struct tag_attr *a)
{
	a->css_class = "content_title";
	a->css_id    = "";
}

void init_std_side_table_attr(struct table_attributes *t, int rows, int cols)
{
	t->table_class  = "emphasis2 side_by_side_left";
	t->tr_class     = "emph1";
	t->th_class     = "emph_title";
	t->td_class     = "";
	t->pos_table_title = T;
	t->title_mod    = 0;
	t->rows         = rows;
	t->cols         = cols;
}

void init_report_output(const std::string &, int) {}
void finish_report_output(void) {}
