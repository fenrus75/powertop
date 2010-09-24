#include "display.h"

#include <ncurses.h>


#include <vector>
#include <map>
#include <string>

using namespace std;

vector<string> tab_names;
map<string, WINDOW *> tab_windows;

static void create_tab(string name)
{
	tab_names.push_back(name);
	tab_windows[name] = newpad(1000,1000);
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
//	create_tab("Checklist");
//	create_tab("Actions");

}

WINDOW *tab_bar = NULL;

static unsigned int current_tab;

void show_tab(unsigned int tab)
{
	WINDOW *win;
	unsigned int i;
	if (tab_bar) {
		delwin(tab_bar);
		tab_bar = NULL;
	}	

	tab_bar = newwin(1, 0, 0, 0);


	wattrset(tab_bar, A_REVERSE);
	mvwprintw(tab_bar, 0,0, "%120s", "");
	mvwprintw(tab_bar, 0,0, "PowerTOP 1.99");


	current_tab = tab;

	for (i = 0; i < tab_names.size(); i++) {
			if (i == tab)
				wattrset(tab_bar, A_NORMAL);
			else
				wattrset(tab_bar, A_REVERSE);
			mvwprintw(tab_bar, 0, (i + 1) * 18, " %s ", tab_names[i].c_str());
	}
	
	wrefresh(tab_bar);

	win = tab_windows[tab_names[tab]];
	if (!win)
		return;

	prefresh(win, 0, 0, 1, 0, LINES - 3, COLS - 1);
}

void show_next_tab(void)
{
	current_tab ++;
	if (current_tab >= tab_names.size())
		current_tab = 0;
	show_tab(current_tab);
}