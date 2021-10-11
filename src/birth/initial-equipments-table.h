#pragma once

#include "system/angband.h"
#include <tuple>
#include <vector>

enum class ItemPrimaryType : short;
extern std::vector<std::vector<std::tuple<ItemPrimaryType, byte>>> player_init;
