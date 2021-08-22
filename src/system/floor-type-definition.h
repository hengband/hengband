﻿#pragma once

#include "floor/floor-base-definitions.h"
#include "floor/sight-definitions.h"
#include "monster/monster-timed-effect-types.h"
#include "system/angband.h"

typedef struct grid_type grid_type;
typedef struct object_type object_type;
typedef struct monster_type monster_type;
enum class DUNGEON_IDX : int;

typedef struct floor_type {
    DUNGEON_IDX dungeon_idx;
    grid_type *grid_array[MAX_HGT];
    DEPTH dun_level; /*!< 現在の実ダンジョン階層 base_level の参照元となる / Current dungeon level */
    DEPTH base_level; /*!< 基本生成レベル、後述のobject_level, monster_levelの参照元となる / Base dungeon level */
    DEPTH object_level; /*!< アイテムの生成レベル、 base_level を起点に一時変更する時に参照 / Current object creation level */
    DEPTH monster_level; /*!< モンスターの生成レベル、 base_level を起点に一時変更する時に参照 / Current monster creation level */
    POSITION width; /*!< Current dungeon width */
    POSITION height; /*!< Current dungeon height */
    MONSTER_NUMBER num_repro; /*!< Current reproducer count */

    GAME_TURN generated_turn; /* Turn when level began */

    object_type *o_list; /*!< The array of dungeon items [max_o_idx] */
    OBJECT_IDX o_max; /* Number of allocated objects */
    OBJECT_IDX o_cnt; /* Number of live objects */

    monster_type *m_list; /*!< The array of dungeon monsters [max_m_idx] */
    MONSTER_IDX m_max; /* Number of allocated monsters */
    MONSTER_IDX m_cnt; /* Number of live monsters */

    int16_t *mproc_list[MAX_MTIMED]; /*!< The array to process dungeon monsters[max_m_idx] */
    int16_t mproc_max[MAX_MTIMED]; /*!< Number of monsters to be processed */

    POSITION_IDX lite_n; //!< Array of grids lit by player lite
    POSITION lite_y[LITE_MAX];
    POSITION lite_x[LITE_MAX];

    POSITION_IDX mon_lite_n; //!< Array of grids lit by player lite
    POSITION mon_lite_y[MON_LITE_MAX];
    POSITION mon_lite_x[MON_LITE_MAX];

    POSITION_IDX view_n; //!< Array of grids viewable to the player
    POSITION view_y[VIEW_MAX];
    POSITION view_x[VIEW_MAX];

    POSITION_IDX redraw_n; //!< Array of grids for delayed visual updating
    POSITION redraw_y[REDRAW_MAX];
    POSITION redraw_x[REDRAW_MAX];

    bool monster_noise;
    QUEST_IDX inside_quest; /* Inside quest level */
    bool inside_arena; /* Is character inside on_defeat_arena_monster? */

} floor_type;
