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
#ifndef __INCLUDE_GUARD_DISPLAY_H_
#define __INCLUDE_GUARD_DISPLAY_H_


#include <map>
#include <string>
#include <ncurses.h>

using namespace std;

extern void init_display(void);
extern void reset_display(void);
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

class tab_window {
public:
	int cursor_pos;
	int cursor_max;
	short int xpad_pos, ypad_pos; 
	WINDOW *win;

	tab_window() {
		cursor_pos = 0;
		cursor_max = 0;
		xpad_pos =0;
		ypad_pos = 0;
		win = NULL;
	}

	virtual void cursor_down(void) { 
		if (cursor_pos < cursor_max ) cursor_pos++; repaint(); 
	} ;
	virtual void cursor_up(void) { 
		if (cursor_pos > 0) cursor_pos--; repaint(); 
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
		win = NULL;
	}
};

extern map<string, class tab_window *> tab_windows;

WINDOW *get_ncurses_win(const char *name);
WINDOW *get_ncurses_win(const string &name);
WINDOW *get_ncurses_win(int nr);

void create_tab(const string &name, const string &translation, class tab_window *w = NULL, string bottom_line = "");

#endif
