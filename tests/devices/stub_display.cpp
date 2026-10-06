/* Minimal stubs for display functions used by devfreq.cpp */
#include <memory>
#include <string>
#include "display.h"

void create_tab(const std::string &, const std::string &,
                std::unique_ptr<tab_window>, const std::string &) {}

WINDOW *get_ncurses_win(const std::string &) { return nullptr; }
