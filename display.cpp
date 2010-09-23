#include "display.h"

#include <ncurses.h>


#include <vector>
#include <string>

using namespace std;

vector<string> tab_names;



void init_display(void)
{
	initscr();
	start_color();

	cbreak(); /* character at a time input */
	noecho(); /* don't show the user input */
	keypad(stdscr, TRUE); /* enable cursor/etc keys */

	use_default_colors();

	tab_names.push_back("Overview");
	tab_names.push_back("Idle stats");
	tab_names.push_back("Frequency stats");
	tab_names.push_back("Device stats");
	tab_names.push_back("Checklist");
	tab_names.push_back("Actions");

        init_pair(PT_COLOR_HEADER_BAR, COLOR_WHITE, COLOR_BLACK);
        init_pair(PT_COLOR_HEADER_LIGHT, COLOR_BLACK, COLOR_WHITE);

}

WINDOW *tab_bar = NULL;

void show_tab(int tab)
{
	unsigned int i;
	if (tab_bar) {
		delwin(tab_bar);
		tab_bar = NULL;
	}	

	tab_bar = newwin(1, 0, 0, 0);


        wbkgd(tab_bar, COLOR_PAIR(PT_COLOR_HEADER_BAR));
        werase(tab_bar);
	wattrset(tab_bar, A_NORMAL);
	mvwprintw(tab_bar, 0,0, "PowerTOP 1.99");



	for (i = 0; i < tab_names.size() - 1; i++) {
			if (i == tab)
				wattrset(tab_bar, COLOR_PAIR(PT_COLOR_HEADER_LIGHT));
			else
				wattrset(tab_bar, COLOR_PAIR(PT_COLOR_HEADER_BAR));
			mvwprintw(tab_bar, 0, (i + 1) * 18, " %s ", tab_names[i].c_str());
	}
	

	wrefresh(tab_bar);
}