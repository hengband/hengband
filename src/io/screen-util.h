#pragma once

#include <utility>

std::pair<int, int> get_screen_size();
void resize_map();
bool panel_contains(int y, int x);
