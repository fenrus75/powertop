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

#include <ncurses.h>


#include <vector>
#include <map>
#include <string>

using namespace std;

static int display = 0;

vector<string> tab_names;
map<string, class tab_window *> tab_windows;

void create_tab(string name, class tab_window *w)
{
	if (!w)
		w = new(class tab_window);

	w->win = newpad(1000,1000);
	tab_names.push_back(name);
	tab_windows[name] = w;
}


void init_display(void)
{
	initscr();
	start_color();

	cbreak(); /* character at a time input */
	noecho(); /* don't show the user input */
	keypad(stdscr, TRUE); /* enable cursor/etc keys */

	use_default_colors();

	create_tab("Overview");
	create_tab("Idle stats");
	create_tab("Frequency stats");
	create_tab("Device stats");
//	create_tab("Tunables");
//	create_tab("Checklist");
//	create_tab("Actions");

	display = 1;
}

WINDOW *tab_bar = NULL;
WINDOW *bottom_line = NULL;

static int current_tab;

void show_tab(unsigned int tab)
{
	WINDOW *win;
	unsigned int i;
	int tab_pos = 17;

	if (!display)
		return;

	if (tab_bar) {
		delwin(tab_bar);
		tab_bar = NULL;
	}	

	if (bottom_line) {
		delwin(bottom_line);
		bottom_line = NULL;
	}	

	tab_bar = newwin(1, 0, 0, 0);

	wattrset(tab_bar, A_REVERSE);
	mvwprintw(tab_bar, 0,0, "%120s", "");
	mvwprintw(tab_bar, 0,0, "PowerTOP 1.99");

	bottom_line = newwin(1, 0, LINES-1, 0);
	wattrset(bottom_line, A_REVERSE);
	mvwprintw(bottom_line, 0,0, "%120s", "");
	mvwprintw(bottom_line, 0,0, " <ESC> Exit | ");


	current_tab = tab;

	for (i = 0; i < tab_names.size(); i++) {
			if (i == tab)
				wattrset(tab_bar, A_NORMAL);
			else
				wattrset(tab_bar, A_REVERSE);
			mvwprintw(tab_bar, 0, tab_pos, " %s ", tab_names[i].c_str());

			tab_pos += 3 + tab_names[i].length();
	}
	
	wrefresh(tab_bar);
	wrefresh(bottom_line);

	win = get_ncurses_win(tab_names[tab]);
	if (!win)
		return;

	prefresh(win, 0, 0, 1, 0, LINES - 3, COLS - 1);
}

WINDOW *get_ncurses_win(const char *name)
{
	class tab_window *w;
	WINDOW *win;

	w= tab_windows[name];
	if (!w)
		return NULL;

	win = w->win;

	return win;
}

WINDOW *get_ncurses_win(int nr)
{
	class tab_window *w;
	WINDOW *win;

	w= tab_windows[tab_names[nr]];
	if (!w)
		return NULL;

	win = w->win;

	return win;
}

WINDOW *get_ncurses_win(string name)
{
	return get_ncurses_win(name.c_str());
}


void show_next_tab(void)
{
	if (!display)
		return;
	current_tab ++;
	if (current_tab >= (int)tab_names.size())
		current_tab = 0;
	show_tab(current_tab);
}

void show_prev_tab(void)
{
	if (!display)
		return;
	current_tab --;
	if (current_tab < 0)
		current_tab = tab_names.size() - 1;
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
	if (w)
		w->cursor_down();

	show_cur_tab();
}

void cursor_up(void)
{
	class tab_window *w;

	w = tab_windows[tab_names[current_tab]];

	if (w)
		w->cursor_up();

	show_cur_tab();
}
void cursor_enter(void)
{
	class tab_window *w;

	w = tab_windows[tab_names[current_tab]];

	if (w)
		w->cursor_enter();
	show_cur_tab();
}
