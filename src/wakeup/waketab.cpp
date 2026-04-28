#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include "wakeup.h"
#include <vector>
#include "../lib.h"
#include "../measurement/sysfs.h"
#include "../display.h"
#include "../report/report.h"
#include "../report/report-maker.h"
#include "../report/report-data-html.h"
#include "wakeup_ethernet.h"
#include "wakeup_usb.h"

static bool should_clear = false;

class wakeup_window *newtab_window;

class wakeup_window: public tab_window {
public:
	virtual void repaint(void) override;
	virtual void cursor_enter(void) override;
	virtual void expose(void) override;
	virtual void window_refresh(void) override;
};

static void init_wakeup(void)
{
	add_ethernet_wakeup();
	add_usb_wakeup();
}

void initialize_wakeup(void)
{
	class wakeup_window *win;

	win = new wakeup_window();

	create_tab("WakeUp", _("WakeUp"), win, _(" <ESC> Exit | <Enter> Toggle wakeup | <r> Window refresh"));

	init_wakeup();

	if (wakeup_all.empty())
		win->cursor_max = 0;
	else
		win->cursor_max = (int)wakeup_all.size() - 1;

	if (newtab_window)
		delete newtab_window;

	newtab_window = win;
}

static void __wakeup_update_display(int cursor_pos)
{
	WINDOW *win;
	unsigned int i;

	win = get_ncurses_win("WakeUp");

	if (!win)
		return;

	if (should_clear) {
		should_clear = false;
		wclear(win);
	}

	wmove(win, 1,0);

	for (i = 0; i < wakeup_all.size(); i++) {
		std::string res = wakeup_all[i]->wakeup_string();
		std::string desc = wakeup_all[i]->description();

		if ((int)i != cursor_pos) {
			wattrset(win, A_NORMAL);
			wprintw(win, "   ");
		} else {
			wattrset(win, A_REVERSE);
			wprintw(win, ">> ");
		}
		wprintw(win, "%-12s  %-103s\n", res.c_str(), desc.c_str());
	}
}

void wakeup_update_display(void)
{
	class tab_window *wt;

	wt = tab_windows["WakeUp"];
	if (!wt)
		return;
	wt->repaint();
}

void wakeup_window::repaint(void)
{
	__wakeup_update_display(cursor_pos);
}

void wakeup_window::cursor_enter(void)
{
	if ((size_t)cursor_pos >= wakeup_all.size())
		return;
	class wakeup *wake;
	std::string wakeup_toggle_script;
	wake = wakeup_all[cursor_pos];
	if (!wake)
		return;
	wakeup_toggle_script = wake->wakeup_toggle_script();
	wake->wakeup_toggle();
	ui_notify_user(std::format(">> {}\n", wakeup_toggle_script));
}

void report_show_wakeup(void)
{
	unsigned int i;
	int idx, rows = 0, cols;

	/* div attr css_class and css_id */
	tag_attr div_attr;
	init_div(&div_attr, "clear_block", "wakeup");

	/* Set Title attributes */
	tag_attr title_attr;
	init_title_attr(&title_attr);

	/* Set Table attributes, rows, and cols */
	table_attributes wakeup_table_css;
	cols=2;
	idx = cols;

	for (i = 0; i < wakeup_all.size(); i++) {
		int tgb;
		tgb = wakeup_all[i]->wakeup_value();
		if (tgb == WAKEUP_DISABLE)
			rows+=1;
	}

	/* add section */

	report.add_div(&div_attr);
	if (rows > 0){
		rows= rows + 1;
		init_wakeup_table_attr(&wakeup_table_css, rows, cols);

		/* Set array of data in row Major order */
		std::vector<std::string> wakeup_data(cols * rows);

		wakeup_data[0]=__("Description");
		wakeup_data[1]=__("Script");

		for (i = 0; i < wakeup_all.size(); i++) {
			int gb;
			gb = wakeup_all[i]->wakeup_value();
			if (gb != WAKEUP_DISABLE)
				continue;
			wakeup_data[idx]=wakeup_all[i]->description();
			idx+=1;
			wakeup_data[idx]=std::string(wakeup_all[i]->wakeup_toggle_script());
			idx+=1;
		}

		/* Report Output */
		report.add_title(&title_attr,__("Wake status of the devices"));
		report.add_table(wakeup_data, &wakeup_table_css);
	}
}

void wakeup_window::expose(void)
{
	cursor_pos = 0;
	repaint();
}

void wakeup_window::window_refresh(void)
{
	clear_wakeup();
	should_clear = true;
	init_wakeup();
}

void clear_wakeup()
{
	for (size_t i = 0; i < wakeup_all.size(); i++) {
		delete wakeup_all[i];
	}
	wakeup_all.clear();
}
