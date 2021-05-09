#include "info-reader/dungeon-info-tokens-table.h"
#include "dungeon/dungeon-flag-types.h"

/*!
 * ダンジョン特性トークンの定義 /
 * Dungeon flags
 */
const std::unordered_map<std::string_view, DF> d_info_flags = {
    { "WINNER", DF::WINNER },
    { "MAZE", DF::MAZE },
    { "SMALLEST", DF::SMALLEST },
    { "BEGINNER", DF::BEGINNER },
    { "BIG", DF::BIG },
    { "NO_DOORS", DF::NO_DOORS },
    { "WATER_RIVER", DF::WATER_RIVER },
    { "LAVA_RIVER", DF::LAVA_RIVER },
    { "CURTAIN", DF::CURTAIN },
    { "GLASS_DOOR", DF::GLASS_DOOR },
    { "CAVE", DF::CAVE },
    { "CAVERN", DF::CAVERN },
    { "ARCADE", DF::ARCADE },
    { "LAKE_ACID", DF::LAKE_ACID },
    { "LAKE_POISONOUS", DF::LAKE_POISONOUS },
    { "NO_ROOM", DF::NO_ROOM },
    { "FORGET", DF::FORGET },
    { "LAKE_WATER", DF::LAKE_WATER },
    { "LAKE_LAVA", DF::LAKE_LAVA },
    { "LAKE_RUBBLE", DF::LAKE_RUBBLE },
    { "LAKE_TREE", DF::LAKE_TREE },
    { "NO_VAULT", DF::NO_VAULT },
    { "ARENA", DF::ARENA },
    { "DESTROY", DF::DESTROY },
    { "GLASS_ROOM", DF::GLASS_ROOM },
    { "NO_CAVE", DF::NO_CAVE },
    { "NO_MAGIC", DF::NO_MAGIC },
    { "NO_MELEE", DF::NO_MELEE },
    { "CHAMELEON", DF::CHAMELEON },
    { "DARKNESS", DF::DARKNESS },
    { "ACID_RIVER", DF::ACID_RIVER },
    { "POISONOUS_RIVER", DF::POISONOUS_RIVER },
};
