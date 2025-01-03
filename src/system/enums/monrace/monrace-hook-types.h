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
    GRASS,
    WOOD,
    VOLCANO,
    MOUNTAIN,
};

enum class MonraceHookTerrain {
    NONE,
    FLOOR,
    SHALLOW_WATER,
    DEEP_WATER,
    TRAPPED_PIT,
    LAVA,
};
