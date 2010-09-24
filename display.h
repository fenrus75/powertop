#ifndef __INCLUDE_GUARD_DISPLAY_H_
#define __INCLUDE_GUARD_DISPLAY_H_


#include <map>
#include <string>
#include <ncurses.h>

using namespace std;

extern void init_display(void);

extern void show_tab(unsigned int tab);
extern void show_next_tab(void);

extern map<string, WINDOW *> tab_windows;

#endif