#include "info-reader/dungeon-info-tokens-table.h"
#include "dungeon/dungeon-flag-types.h"

/*!
 * ダンジョン特性トークンの定義 /
 * Dungeon flags
 */
const std::unordered_map<std::string_view, DungeonFeatureType> d_info_flags = {
    { "WINNER", DungeonFeatureType::WINNER },
    { "MAZE", DungeonFeatureType::MAZE },
    { "SMALLEST", DungeonFeatureType::SMALLEST },
    { "BEGINNER", DungeonFeatureType::BEGINNER },
    { "BIG", DungeonFeatureType::BIG },
    { "NO_DOORS", DungeonFeatureType::NO_DOORS },
    { "WATER_RIVER", DungeonFeatureType::WATER_RIVER },
    { "LAVA_RIVER", DungeonFeatureType::LAVA_RIVER },
    { "CURTAIN", DungeonFeatureType::CURTAIN },
    { "GLASS_DOOR", DungeonFeatureType::GLASS_DOOR },
    { "CAVE", DungeonFeatureType::CAVE },
    { "CAVERN", DungeonFeatureType::CAVERN },
    { "ARCADE", DungeonFeatureType::ARCADE },
    { "LAKE_ACID", DungeonFeatureType::LAKE_ACID },
    { "LAKE_POISONOUS", DungeonFeatureType::LAKE_POISONOUS },
    { "NO_ROOM", DungeonFeatureType::NO_ROOM },
    { "FORGET", DungeonFeatureType::FORGET },
    { "LAKE_WATER", DungeonFeatureType::LAKE_WATER },
    { "LAKE_LAVA", DungeonFeatureType::LAKE_LAVA },
    { "LAKE_RUBBLE", DungeonFeatureType::LAKE_RUBBLE },
    { "LAKE_TREE", DungeonFeatureType::LAKE_TREE },
    { "NO_VAULT", DungeonFeatureType::NO_VAULT },
    { "ARENA", DungeonFeatureType::ARENA },
    { "DESTROY", DungeonFeatureType::DESTROY },
    { "GLASS_ROOM", DungeonFeatureType::GLASS_ROOM },
    { "NO_CAVE", DungeonFeatureType::NO_CAVE },
    { "NO_MAGIC", DungeonFeatureType::NO_MAGIC },
    { "NO_MELEE", DungeonFeatureType::NO_MELEE },
    { "CHAMELEON", DungeonFeatureType::CHAMELEON },
    { "DARKNESS", DungeonFeatureType::DARKNESS },
    { "ACID_RIVER", DungeonFeatureType::ACID_RIVER },
    { "POISONOUS_RIVER", DungeonFeatureType::POISONOUS_RIVER },
};
