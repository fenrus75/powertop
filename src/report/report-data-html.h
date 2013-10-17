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
};


struct table_size {
	int rows;
	int cols;
};

