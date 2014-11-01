#ifndef PowerTop_REPORT_DATA_HTML_H_C58C116411234A34AC2EFB8D23A69713
#define PowerTop_REPORT_DATA_HTML_H_C58C116411234A34AC2EFB8D23A69713

#include <string>
#include <sstream>

using namespace std;

struct tag_attr {
	const char *css_class;
	const char *css_id;
};
/* T:Top, L:Left, TL:Top-Left, TLC: Top-Left-Center */
enum position { T, L, TL, TC, TLC };

struct table_attributes {
	const char *table_class;
	const char *td_class;
	const char *tr_class;
	const char *th_class;
	position pos_table_title;
	int title_mod;
	int rows;
	int cols;
};

struct table_size {
	int rows;
	int cols;
};


/* Definition of css attributes for the cases that apply to powertop
 * html report
 * */

void
init_div(struct tag_attr *div_attr, const char *css_class, const char *css_id);

void
init_top_table_attr(struct table_attributes *table_css, int rows, int cols);

void
init_title_attr(struct tag_attr *title_attr);

void
init_std_table_attr(struct table_attributes *table_css, int rows, int cols);

void
init_std_side_table_attr(struct table_attributes *table_css, int rows,
		int cols);

void
init_pkg_table_attr(struct table_attributes *table_css, int rows, int cols);

void
init_core_table_attr(struct table_attributes *table_css, int title_mod,
		int rows, int cols);

void
init_cpu_table_attr(struct table_attributes *table_css, int title_mod,
		int rows, int cols);
void
init_nowarp_table_attr(struct table_attributes *table_css, int rows, int cols);

void
init_tune_table_attr(struct table_attributes *table_css, int rows, int cols);

/* Other helper functions */
string
double_to_string(double dval);

#endif
