#pragma once

#include "dungeon/dungeon-flag-types.h"
#include "util/flag-group.h"

inline const EnumClassFlagGroup<DungeonFeatureType> DF_LAKE_MASK({ DungeonFeatureType::LAKE_WATER, DungeonFeatureType::LAKE_LAVA, DungeonFeatureType::LAKE_RUBBLE, DungeonFeatureType::LAKE_TREE, DungeonFeatureType::LAKE_POISONOUS, DungeonFeatureType::LAKE_ACID });
inline const EnumClassFlagGroup<DungeonFeatureType> DF_RIVER_MASK({ DungeonFeatureType::WATER_RIVER, DungeonFeatureType::LAVA_RIVER, DungeonFeatureType::ACID_RIVER, DungeonFeatureType::POISONOUS_RIVER });
