/*!
 * @brief モンスター生成条件関数の種別
 * @author Hourier
 * @date 2024/12/28
 */

#pragma once

enum class MonraceHook {
    NONE,
    DUNGEON,
    TOWN,
    OCEAN,
    SHORE,
    WASTE,
    GRASS, // 草原 (取り違え注意)
    WOOD,
    VOLCANO,
    MOUNTAIN,
    FIGURINE,
    ARENA,
    NIGHTMARE,
    HUMAN,
    GLASS, // ガラス (取り違え注意)
    SHARDS,
    TANUKI,
    FISHING,
    QUEST,
    VAULT,
    CLONE,
    JELLY,
    GOOD,
    EVIL,
    MIMIC,
    HORROR,
    KENNEL,
    ANIMAL,
    CHAPEL,
    UNDEAD,
    ORC,
    TROLL,
    GIANT,
    DRAGON,
    DEMON,
    DARK_ELF,
};

enum class MonraceHookTerrain {
    NONE,
    FLOOR,
    SHALLOW_WATER,
    DEEP_WATER,
    TRAPPED_PIT,
    LAVA,
};
