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


#include <vector>
#include <map>
#include <string>
#include <string.h>

using namespace std;

static int display = 0;

vector<string> tab_names;
map<string, class tab_window *> tab_windows;
map<string, string> tab_translations;

map<string, string> bottom_lines;

#ifndef DISABLE_NCURSES
void create_tab(const string &name, const string &translation, class tab_window *w, string bottom_line)
{
	if (!w)
		w = new(class tab_window);

	w->win = newpad(1000,1000);
	tab_names.push_back(name);
	tab_windows[name] = w;
	tab_translations[name] = translation;
	bottom_lines[name] = bottom_line;
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
	create_tab("Device stats", _("Device stats"));

	display = 1;
}

void reset_display(void)
{
	if (!display)
		return;

	keypad(stdscr, FALSE);
	echo();
	nocbreak();

	resetterm();
}


WINDOW *tab_bar = NULL;
WINDOW *bottom_line = NULL;

static int current_tab;

void show_tab(unsigned int tab)
{
	WINDOW *win;
	unsigned int i;
	int tab_pos = 17;
	const char *c;

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
	mvwprintw(tab_bar, 0,0, "PowerTOP %s", POWERTOP_SHORT_VERSION);

	bottom_line = newwin(1, 0, LINES-1, 0);
	wattrset(bottom_line, A_REVERSE);
	mvwprintw(bottom_line, 0,0, "%120s", "");

	c = bottom_lines[tab_names[tab]].c_str();
	if (c && strlen(c) > 0)
		mvwprintw(bottom_line, 0,0, c);
	else
		mvwprintw(bottom_line, 0,0, _(" <ESC> Exit | "));


	current_tab = tab;

	for (i = 0; i < tab_names.size(); i++) {
			if (i == tab)
				wattrset(tab_bar, A_NORMAL);
			else
				wattrset(tab_bar, A_REVERSE);
			mvwprintw(tab_bar, 0, tab_pos, " %s ", tab_translations[tab_names[i]].c_str());

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

WINDOW *get_ncurses_win(const string &name)
{
	return get_ncurses_win(name.c_str());
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

	show_tab(current_tab);
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
		w->window_refresh();
		w->repaint();
	}

	show_cur_tab();
}

int ncurses_initialized(void)
{
	if (display)
		return 1;
	return 0;
}

#else /* DISABLE_NCURSES - stub implementations*/

void create_tab(const string &name, const string &translation, class tab_window *w, string bottom_line)
{
}


void init_display(void)
{
}

void reset_display(void)
{
}

void show_tab(unsigned int tab)
{
}

void show_next_tab(void)
{
}

void show_prev_tab(void)
{
}

void show_cur_tab(void)
{
}

void cursor_down(void)
{
}

void cursor_up(void)
{
}
void cursor_enter(void)
{
}

int ncurses_initialized(void)
{
	return 0;
}

#endif