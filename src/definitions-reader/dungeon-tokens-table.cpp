#include "definitions-reader/dungeon-tokens-table.h"
#include "dungeon/dungeon-flag-types.h"
#include "room/pit-nest-util.h"
#include "system/dungeon/dungeon-definition.h"

/*!
 * ダンジョン特性トークンの定義 /
 * Dungeon flags
 */
const std::unordered_map<std::string_view, DungeonFeatureType> dungeon_flags = {
    { "WINNER", DungeonFeatureType::WINNER },
    { "MAZE", DungeonFeatureType::MAZE },
    { "BEGINNER", DungeonFeatureType::BEGINNER },
    { "SMALLEST", DungeonFeatureType::SMALLEST },
    { "SMALL", DungeonFeatureType::SMALL },
    { "LARGE", DungeonFeatureType::LARGE },
    { "LARGEST", DungeonFeatureType::LARGEST },
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
/*!
 * モンスター生成条件フラグの結合モードトークンの定義 /
 * Dungeon monster flag matching modes
 */
const std::unordered_map<std::string_view, DungeonMode> dungeon_modes = {
    { "NONE", DungeonMode::NONE },
    { "AND", DungeonMode::AND },
    { "NAND", DungeonMode::NAND },
    { "OR", DungeonMode::OR },
    { "NOR", DungeonMode::NOR },
};

/*!
 * pit種別トークンの定義 /
 * Dungeon pit kinds
 */
const std::unordered_map<std::string_view, PitKind> dungeon_pit_kinds = {
    { "ORC", PitKind::ORC },
    { "TROLL", PitKind::TROLL },
    { "GIANT", PitKind::GIANT },
    { "HORROR", PitKind::HORROR },
    { "SYMBOL_GOOD", PitKind::SYMBOL_GOOD },
    { "SYMBOL_EVIL", PitKind::SYMBOL_EVIL },
    { "CHAPEL", PitKind::CHAPEL },
    { "DRAGON", PitKind::DRAGON },
    { "DEMON", PitKind::DEMON },
    { "DARK_ELF", PitKind::DARK_ELF },
};

/*!
 * nest種別トークンの定義 /
 * Dungeon nest kinds
 */
const std::unordered_map<std::string_view, NestKind> dungeon_nest_kinds = {
    { "CLONE", NestKind::CLONE },
    { "JELLY", NestKind::JELLY },
    { "SYMBOL_GOOD", NestKind::SYMBOL_GOOD },
    { "SYMBOL_EVIL", NestKind::SYMBOL_EVIL },
    { "MIMIC", NestKind::MIMIC },
    { "HORROR", NestKind::HORROR },
    { "KENNEL", NestKind::KENNEL },
    { "ANIMAL", NestKind::ANIMAL },
    { "CHAPEL", NestKind::CHAPEL },
    { "UNDEAD", NestKind::UNDEAD },
};
