#pragma once

#include "dungeon/dungeon-flag-types.h"
#include "util/flag-group.h"

inline const EnumClassFlagGroup<DF> DF_LAKE_MASK({ DF::LAKE_WATER, DF::LAKE_LAVA, DF::LAKE_RUBBLE, DF::LAKE_TREE, DF::LAKE_POISONOUS, DF::LAKE_ACID });
inline const EnumClassFlagGroup<DF> DF_RIVER_MASK({ DF::WATER_RIVER, DF::LAVA_RIVER, DF::ACID_RIVER, DF::POISONOUS_RIVER });
