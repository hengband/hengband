#pragma once

#include "system/angband.h"
#include <tuple>
#include <vector>

enum class ItemKindType : short;
extern std::vector<std::vector<std::tuple<ItemKindType, byte>>> player_init;
