/*!
 * @brief 広域マップ地形ID
 * @author Hourier
 * @date 2025/01/31
 */

#pragma once

enum wt_type : int {
    TERRAIN_EDGE = 0, /* Edge of the World */
    TERRAIN_TOWN = 1, /* Town */
    TERRAIN_DEEP_WATER = 2, /* Deep water */
    TERRAIN_SHALLOW_WATER = 3, /* Shallow water */
    TERRAIN_SWAMP = 4, /* Swamp */
    TERRAIN_DIRT = 5, /* Dirt */
    TERRAIN_GRASS = 6, /* Grass */
    TERRAIN_TREES = 7, /* Trees */
    TERRAIN_DESERT = 8, /* Desert */
    TERRAIN_SHALLOW_LAVA = 9, /* Shallow lava */
    TERRAIN_DEEP_LAVA = 10, /* Deep lava */
    TERRAIN_MOUNTAIN = 11, /* Mountain */
    MAX_WILDERNESS = 12, /* Maximum wilderness index */
};
