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
#pragma once


#include <algorithm>
#include <map>
#include <string>
#include <ncurses.h>

extern void init_display(void);
extern void reset_display(void);
extern void create_device_stats_tab(void);
extern int ncurses_initialized(void);
extern void show_tab(unsigned int tab);
extern void show_next_tab(void);
extern void show_prev_tab(void);
extern void show_cur_tab(void);
extern void cursor_up(void);
extern void cursor_down(void);
extern void cursor_right(void);
extern void cursor_left(void);
extern void cursor_enter(void);
extern void window_refresh(void);
extern void set_notification(const std::string &msg);
extern void clear_notification(void);

/*
 * # subclasses of tab_window
 *
 * | subclass        | filename                      |
 * | --------------- | ----------------------------- |
 * | gpu_tab_window  | src/gpu-tab.h                 |
 */
class tab_window {
public:
	int cursor_pos = 0;
	int cursor_max = 0;
	int content_max_y = 0;
	int content_max_x = 0;
	short int xpad_pos = 0, ypad_pos = 0;
	WINDOW *win = nullptr;

	tab_window() {
		cursor_pos = 0;
		cursor_max = 0;
		content_max_y = 0;
		content_max_x = 0;
		xpad_pos =0;
		ypad_pos = 0;
		win = nullptr;
	}

	void reset_content_size(void) {
		content_max_y = 0;
		content_max_x = 0;
	}

	void update_content_size(void) {
		if (!win)
			return;
		content_max_y = std::max(content_max_y, getcury(win));
		content_max_x = std::max(content_max_x, getcurx(win));
	}

	virtual void cursor_down(void) {
		if (cursor_pos < cursor_max)
			cursor_pos++;
		repaint();
	} ;
	virtual void cursor_up(void) {
		if (cursor_pos > 0)
			cursor_pos--;
		repaint();
	};
	virtual void cursor_left(void) { };
	virtual void cursor_right(void) { };

	virtual void cursor_enter(void) { };
	virtual void window_refresh() { };

	virtual void repaint(void) { };
	virtual void expose(void) { cursor_pos = 0; repaint();};
	virtual void hide(void) { };

	virtual ~tab_window()
	{
		delwin(win);
		win = nullptr;
	}
};

extern std::map<std::string, class tab_window *> tab_windows;

WINDOW *get_ncurses_win(const std::string &name);
WINDOW *get_ncurses_win(int nr);

void create_tab(const std::string &name, const std::string &translation, class tab_window *w = nullptr, const std::string &bottom_line = "");

/* Default width for draw_progress_bar */
static constexpr int BAR_WIDTH = 50;

/* Color-pair IDs for the four-segment progress bar.
 * Pairs are initialized by the GPU tab on first expose. */
static constexpr int BAR_COLOR_FLOOR    = 10;
static constexpr int BAR_COLOR_ACTIVE   = 11;
static constexpr int BAR_COLOR_HEADROOM = 12;
static constexpr int BAR_COLOR_BEYOND   = 13;
static constexpr int BAR_COLOR_BUSY     = 14;
static constexpr int BAR_COLOR_IDLE     = 15;

/*
 * Draw a horizontal progress bar with scale labels and optional policy markers.
 *
 * Outputs up to four lines into win:
 *   1.  "  <label>  <value_str>"
 *   2.  "  [bar]"
 *   3.  "  <scale labels>"
 *   4.  "  <marker caret '<' at marker_hi>"  (only when marker_lo is NAN)
 *
 * Pass NAN for marker_lo and marker_hi to use simple two-segment mode
 * (color_filled / color_empty).  Pass 0 for colors to use terminal default.
 */
void draw_progress_bar(WINDOW *win,
		       const std::string &label,
		       double value,
		       double scale_min, double scale_max,
		       double marker_lo, double marker_hi,
		       const std::string &value_str,
		       double label_interval,
		       int bar_width = BAR_WIDTH,
		       int color_filled = 0,
		       int color_empty = 0,
		       int attr_filled = 0,
		       int attr_empty = 0,
		       bool trailing_blank = true);

