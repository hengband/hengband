#pragma once

#include "util/point-2d.h"

class FloorType;
bool los(const FloorType &floor, const Pos2D &pos_from, const Pos2D &pos_to);
