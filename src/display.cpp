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

static int display = 0;
static std::string current_notification;

std::vector<std::string> tab_names;
std::map<std::string, class tab_window *> tab_windows;
std::map<std::string, std::string> tab_translations;

std::map<std::string, std::string> bottom_lines;

void create_tab(const std::string &name, const std::string &translation, class tab_window *w, std::string bottom_line)
{
	if (!w)
		w = new tab_window;

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

	endwin();
	display = 0;
}


WINDOW *tab_bar = nullptr;
WINDOW *bottom_line = nullptr;

static int current_tab;

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
		tab_bar = nullptr;
	}

	if (bottom_line) {
		delwin(bottom_line);
		bottom_line = nullptr;
	}

	tab_bar = newwin(1, 0, 0, 0);

	wattrset(tab_bar, A_REVERSE);
	mvwprintw(tab_bar, 0, 0, "%-*s", COLS, "");
	mvwprintw(tab_bar, 0,0, "PowerTOP %s", PACKAGE_VERSION);

	bottom_line = newwin(1, 0, LINES-1, 0);
	wattrset(bottom_line, A_REVERSE);
	mvwprintw(bottom_line, 0, 0, "%-*s", COLS, "");

	std::string bottom = bottom_lines[tab_names[tab]];
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
				wattrset(tab_bar, A_NORMAL);
			else
				wattrset(tab_bar, A_REVERSE);
			mvwprintw(tab_bar, 0, tab_pos, " %s ", tab_translations[tab_names[i]].c_str());

			tab_pos += 3 + tab_names[i].length();
	}

	wrefresh(tab_bar);
	wrefresh(bottom_line);

	win = tab_windows[tab_names[tab]];
	if (!win)
		return;

	prefresh(win->win, win->ypad_pos, win->xpad_pos, 1, 0, LINES - 3, COLS - 1);
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
		if (w->ypad_pos < 1000) {
			if (tab_names[current_tab] == "Tunables" || tab_names[current_tab] == "WakeUp") {
		                if ((w->cursor_pos + 7) >= LINES) {
					prefresh(w->win, ++w->ypad_pos, w->xpad_pos,
						1, 0, LINES - 3, COLS - 1);
				}
					w->cursor_down();
			} else {
				prefresh(w->win, ++w->ypad_pos, w->xpad_pos,
					1, 0, LINES - 3, COLS - 1);
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
				 1, 0, LINES - 3, COLS - 1);
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
				1, 0, LINES - 3, COLS - 1);
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
				1, 0, LINES - 3, COLS - 1);
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
