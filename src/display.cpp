/*
 * Copyright 2010, Intel Corporation
 *
 * This file is part of PowerTOP
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 * or just google for it.
 *
 * Authors:
 *	Arjan van de Ven <arjan@linux.intel.com>
 */
#include "display.h"
#include "lib.h"

#include <ncurses.h>
#include <algorithm>
#include <cmath>
#include <format>
#include <string>
#include <vector>
#include <map>

static int display = 0;
static std::string current_notification;

/* Viewport rectangle for prefresh() — top row is 1 (below the tab bar),
 * bottom and right edges are dynamic and computed from LINES/COLS. */
static constexpr int PAD_TOP  = 1;
static constexpr int PAD_LEFT = 0;
#define PAD_BOTTOM (LINES - 3)
#define PAD_RIGHT  (COLS  - 2)

/* Start scrolling when the cursor is within this many lines of the
 * bottom of the visible area. */
static constexpr int SCROLL_MARGIN = 7;

std::vector<std::string> tab_names;
std::map<std::string, class tab_window *> tab_windows;
std::map<std::string, std::string> tab_translations;

std::map<std::string, std::string> bottom_lines;

WINDOW *tab_bar = nullptr;
WINDOW *bottom_line = nullptr;
static WINDOW *scrollbar_win = nullptr;

void create_tab(const std::string &name, const std::string &translation, class tab_window *w, const std::string &bottom_text)
{
	if (!w)
		w = new tab_window;

	w->win = newpad(1000,1000);
	tab_names.push_back(name);
	tab_windows[name] = w;
	tab_translations[name] = translation;
	bottom_lines[name] = bottom_text;
}


void init_display(void)
{
	initscr();
	start_color();

	cbreak(); /* character at a time input */
	noecho(); /* don't show the user input */
	keypad(stdscr, TRUE); /* enable cursor/etc keys */

	use_default_colors();

	create_tab("Overview", _("Overview"));
	create_tab("Idle stats", _("Idle stats"));
	create_tab("Frequency stats", _("Frequency stats"));

	display = 1;

	printw("%s", _("Preparing to take measurements\n"));
	refresh();
}

void create_device_stats_tab(void)
{
	create_tab("Device stats", _("Device stats"));
}

void reset_display(void)
{
	if (!display)
		return;

	for (auto const& [key, val] : tab_windows) {
		delete val;
	}
	tab_windows.clear();
	tab_names.clear();
	tab_translations.clear();
	bottom_lines.clear();

	keypad(stdscr, FALSE);
	echo();
	nocbreak();

	if (tab_bar) {
		delwin(tab_bar);
		tab_bar = nullptr;
	}
	if (bottom_line) {
		delwin(bottom_line);
		bottom_line = nullptr;
	}
	if (scrollbar_win) {
		delwin(scrollbar_win);
		scrollbar_win = nullptr;
	}

	endwin();
	display = 0;
}


static int current_tab;

static void draw_scrollbar(const class tab_window *w)
{
	const int viewport_height = PAD_BOTTOM - PAD_TOP + 1;
	const int content_height = w->content_max_y;

	if (scrollbar_win) {
		delwin(scrollbar_win);
		scrollbar_win = nullptr;
	}

	scrollbar_win = newwin(viewport_height, 1, PAD_TOP, COLS - 1);
	if (!scrollbar_win)
		return;

	if (content_height <= 0 || content_height <= viewport_height) {
		/* no scrollbar needed — clear the column */
		for (int row = 0; row < viewport_height; row++)
			mvwaddch(scrollbar_win, row, 0, ' ');
		wrefresh(scrollbar_win);
		return;
	}

	int thumb_top = (w->ypad_pos * viewport_height) / content_height;
	int thumb_end = ((w->ypad_pos + viewport_height) * viewport_height) / content_height;

	if (thumb_end == thumb_top)
		thumb_end = thumb_top + 1;
	if (thumb_end >= viewport_height)
		thumb_end = viewport_height - 1;

	for (int row = 0; row < viewport_height; row++) {
		chtype ch;
		if (row >= thumb_top && row <= thumb_end)
			ch = ACS_CKBOARD;
		else
			ch = ACS_VLINE;
		mvwaddch(scrollbar_win, row, 0, ch);
	}

	wrefresh(scrollbar_win);
}

void show_tab(unsigned int tab)
{
	class tab_window *win;
	unsigned int i;
	int tab_pos = 17;

	if (!display)
		return;

	if (tab >= tab_names.size())
		return;

	if (tab_bar) {
		delwin(tab_bar);
		tab_bar = nullptr;
	}

	if (bottom_line) {
		delwin(bottom_line);
		bottom_line = nullptr;
	}

	tab_bar = newwin(1, 0, 0, 0);

	wattron(tab_bar, A_REVERSE);
	mvwprintw(tab_bar, 0, 0, "%-*s", COLS, "");
	mvwprintw(tab_bar, 0,0, "PowerTOP %s", PACKAGE_VERSION);

	bottom_line = newwin(1, 0, LINES-1, 0);
	wattron(bottom_line, A_REVERSE);
	mvwprintw(bottom_line, 0, 0, "%-*s", COLS, "");

	const std::string bottom = bottom_lines[tab_names[tab]];
	if (!bottom.empty())
		mvwprintw(bottom_line, 0,0, "%s", bottom.c_str());
	else
		mvwprintw(bottom_line, 0, 0,
			"<ESC> %s | <TAB> / <Shift + TAB> %s | ", _("Exit"),
			_("Navigate"));

	if (!current_notification.empty()) {
		int len = (int)current_notification.length();
		if (len >= COLS)
			len = COLS - 1;
		mvwprintw(bottom_line, 0, COLS - len, "%.*s", len,
			  current_notification.c_str());
	}


	current_tab = tab;

	for (i = 0; i < tab_names.size(); i++) {
			if (i == tab)
				wattroff(tab_bar, A_REVERSE);
			else
				wattron(tab_bar, A_REVERSE);
			mvwprintw(tab_bar, 0, tab_pos, " %s ", tab_translations[tab_names[i]].c_str());

			tab_pos += 3 + tab_translations[tab_names[i]].length();
	}

	wrefresh(tab_bar);
	wrefresh(bottom_line);

	win = tab_windows[tab_names[tab]];
	if (!win)
		return;

	prefresh(win->win, win->ypad_pos, win->xpad_pos, PAD_TOP, PAD_LEFT, PAD_BOTTOM, PAD_RIGHT);
	draw_scrollbar(win);
}

WINDOW *get_ncurses_win(const std::string &name)
{
	class tab_window *w;
	WINDOW *win;

	if (tab_windows.count(name) == 0)
		return nullptr;

	w = tab_windows[name];
	if (!w)
		return nullptr;

	win = w->win;

	return win;
}

WINDOW *get_ncurses_win(int nr)
{
	class tab_window *w;
	WINDOW *win;

	if (nr < 0 || nr >= (int)tab_names.size())
		return nullptr;

	w = tab_windows[tab_names[nr]];
	if (!w)
		return nullptr;

	win = w->win;

	return win;
}

void show_prev_tab(void)
{
       class tab_window *w;

       if (!display)
               return;
       w = tab_windows[tab_names[current_tab]];
       if (w)
               w->hide();

       current_tab --;
       if (current_tab < 0)
               current_tab = tab_names.size() - 1;

       w = tab_windows[tab_names[current_tab]];
       if (w)
               w->expose();

       clear_notification();
       show_tab(current_tab);
}

void show_next_tab(void)
{
	class tab_window *w;

	if (!display)
		return;

	w = tab_windows[tab_names[current_tab]];
	if (w)
		w->hide();

	current_tab ++;
	if (current_tab >= (int)tab_names.size())
		current_tab = 0;

	w = tab_windows[tab_names[current_tab]];
	if (w)
		w->expose();

	clear_notification();
	show_tab(current_tab);
}

void show_cur_tab(void)
{
	if (!display)
		return;
	show_tab(current_tab);
}

void cursor_down(void)
{
	class tab_window *w;

	w = tab_windows[tab_names[current_tab]];
	if (w) {
		int viewport_height = PAD_BOTTOM - PAD_TOP + 1;
		int ypad_max;
		if (w->content_max_y > 0)
			ypad_max = w->content_max_y - viewport_height + 2;
		else
			ypad_max = 1000;
		if (ypad_max < 0)
			ypad_max = 0;

		if (w->ypad_pos < ypad_max) {
			if (tab_names[current_tab] == "Tunables" || tab_names[current_tab] == "WakeUp") {
		                if ((w->cursor_pos + SCROLL_MARGIN) >= LINES) {
					prefresh(w->win, ++w->ypad_pos, w->xpad_pos,
						PAD_TOP, PAD_LEFT, PAD_BOTTOM, PAD_RIGHT);
				}
					w->cursor_down();
			} else {
				prefresh(w->win, ++w->ypad_pos, w->xpad_pos,
					PAD_TOP, PAD_LEFT, PAD_BOTTOM, PAD_RIGHT);
			}
		}
	}

	show_cur_tab();
}

void cursor_up(void)
{
	class tab_window *w;

	w = tab_windows[tab_names[current_tab]];

	if (w) {
		w->cursor_up();
		if (w->ypad_pos > 0) {
			prefresh(w->win, --w->ypad_pos, w->xpad_pos,
				 PAD_TOP, PAD_LEFT, PAD_BOTTOM, PAD_RIGHT);
		}
	}

	show_cur_tab();
}

void cursor_left(void)
{
        class tab_window *w;

	w = tab_windows[tab_names[current_tab]];

	if (w) {
		if (w->xpad_pos > 0) {
			prefresh(w->win, w->ypad_pos,--w->xpad_pos,
				PAD_TOP, PAD_LEFT, PAD_BOTTOM, PAD_RIGHT);
		}
	}
}

void cursor_right(void)
{
        class tab_window *w;

	w = tab_windows[tab_names[current_tab]];

	if (w) {
		if (w->xpad_pos < 1000) {
			prefresh(w->win, w->ypad_pos, ++w->xpad_pos,
				PAD_TOP, PAD_LEFT, PAD_BOTTOM, PAD_RIGHT);
		}
	}
}

void cursor_enter(void)
{
	class tab_window *w;

	w = tab_windows[tab_names[current_tab]];

	if (w) {
		w->cursor_enter();
		w->repaint();
	}
	show_cur_tab();
}

void window_refresh()
{
	class tab_window *w;

	w = tab_windows[tab_names[current_tab]];

	if (w) {
		w->ypad_pos = 0;
		w->xpad_pos = 0;
		w->window_refresh();
		w->repaint();
	}

	clear_notification();
	show_cur_tab();
}

int ncurses_initialized(void)
{
	if (display)
		return 1;
	return 0;
}

void set_notification(const std::string &msg)
{
	current_notification = msg;
	while (!current_notification.empty() &&
	       (current_notification.back() == '\n' ||
	        current_notification.back() == '\r' ||
	        current_notification.back() == ' '))
		current_notification.pop_back();
	if (display)
		show_cur_tab();
}

void clear_notification(void)
{
	current_notification.clear();
}

void draw_progress_bar(WINDOW *win,
		       const std::string &label,
		       double value,
		       double scale_min, double scale_max,
		       double marker_lo, double marker_hi,
		       const std::string &value_str,
		       double label_interval,
		       int bar_width,
		       int color_filled,
		       int color_empty,
		       int attr_filled,
		       int attr_empty,
		       bool trailing_blank)
{
	const double range = scale_max - scale_min;
	if (range <= 0.0)
		return;

	/* Line 1: label + current value */
	wprintw(win, "  %s  %s\n", label.c_str(), value_str.c_str());

	/* Line 2: bar */
	{
		const double clamped = std::clamp(value, scale_min, scale_max);
		wprintw(win, "  ");

		if (!std::isnan(marker_lo)) {
			const int pos_min = std::clamp(
				static_cast<int>(std::round((marker_lo - scale_min) / range * bar_width)),
				0, bar_width);
			const int pos_cur = std::clamp(
				static_cast<int>(std::round((clamped - scale_min) / range * bar_width)),
				0, bar_width);
			const bool has_beyond = !std::isnan(marker_hi) &&
						std::abs(marker_hi - scale_max) > 0.5;
			int pos_max = bar_width;
			if (has_beyond)
				pos_max = std::clamp(
					static_cast<int>(std::round((marker_hi - scale_min) / range * bar_width)),
					0, bar_width);

			for (int i = 0; i < bar_width; i++) {
				if (i < pos_min) {
					wattron(win, COLOR_PAIR(BAR_COLOR_FLOOR) | A_BOLD);
					wprintw(win, "█");
					wattroff(win, COLOR_PAIR(BAR_COLOR_FLOOR) | A_BOLD);
				} else if (i < pos_cur) {
					wattron(win, COLOR_PAIR(BAR_COLOR_ACTIVE));
					wprintw(win, "█");
					wattroff(win, COLOR_PAIR(BAR_COLOR_ACTIVE));
				} else if (i < pos_max) {
					wattron(win, COLOR_PAIR(BAR_COLOR_HEADROOM));
					wprintw(win, "░");
					wattroff(win, COLOR_PAIR(BAR_COLOR_HEADROOM));
				} else {
					wattron(win, COLOR_PAIR(BAR_COLOR_BEYOND) | A_DIM);
					wprintw(win, "░");
					wattroff(win, COLOR_PAIR(BAR_COLOR_BEYOND) | A_DIM);
				}
			}
		} else {
			const int filled = std::clamp(
				static_cast<int>(std::round((clamped - scale_min) / range * bar_width)),
				0, bar_width);
			if (color_filled)
				wattron(win, COLOR_PAIR(color_filled) | attr_filled);
			for (int i = 0; i < filled; i++)
				wprintw(win, "█");
			if (color_filled)
				wattroff(win, COLOR_PAIR(color_filled) | attr_filled);
			if (color_empty)
				wattron(win, COLOR_PAIR(color_empty) | attr_empty);
			for (int i = filled; i < bar_width; i++)
				wprintw(win, "░");
			if (color_empty)
				wattroff(win, COLOR_PAIR(color_empty) | attr_empty);
		}
		wprintw(win, "\n");
	}

	/* Line 3: scale labels */
	{
		const double min_gap_chars = 6.0;
		while ((label_interval / range * bar_width) < min_gap_chars)
			label_interval *= 2.0;

		std::string scale_line(bar_width + 2, ' ');

		const std::string max_str =
			std::to_string(static_cast<int>(std::round(scale_max)));
		const int max_start =
			static_cast<int>(scale_line.size()) - static_cast<int>(max_str.size());

		const double first =
			std::ceil(scale_min / label_interval) * label_interval;
		for (double v = first;
		     v <= scale_max + label_interval * 0.01;
		     v += label_interval) {
			const int pos = std::clamp(
				static_cast<int>(std::round((v - scale_min) / range * bar_width)),
				0, bar_width - 1);
			const std::string num =
				std::to_string(static_cast<int>(std::round(v)));
			const int write_pos = pos + 2;
			if (write_pos + static_cast<int>(num.size()) > max_start)
				continue;
			if (write_pos + static_cast<int>(num.size()) <=
			    static_cast<int>(scale_line.size()))
				scale_line.replace(write_pos, num.size(), num);
		}

		scale_line.replace(max_start, max_str.size(), max_str);
		wprintw(win, "%s\n", scale_line.c_str());
	}

	/* Line 4: marker caret — only in simple mode when marker_hi is set */
	if (std::isnan(marker_lo) && !std::isnan(marker_hi)) {
		std::string marker_line(bar_width + 2, ' ');
		const int pos = std::clamp(
			static_cast<int>(std::round((marker_hi - scale_min) / range * bar_width)),
			0, bar_width - 1);
		marker_line[pos + 2] = '<';
		wprintw(win, "%s\n", marker_line.c_str());
	}

	if (trailing_blank)
		wprintw(win, "\n");
}
