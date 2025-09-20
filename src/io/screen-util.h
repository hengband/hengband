#pragma once

#include "util/point-2d.h"
#include <utility>

std::pair<int, int> get_screen_size();
void resize_map();
bool panel_contains(const Pos2D &pos);
