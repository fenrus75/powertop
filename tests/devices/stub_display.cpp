/* Minimal stubs for display functions used by devfreq.cpp */
#include <string>
#include "display.h"

void create_tab(const std::string &, const std::string &,
                tab_window *, std::string) {}

WINDOW *get_ncurses_win(const std::string &) { return nullptr; }
