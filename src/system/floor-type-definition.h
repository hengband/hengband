#pragma once

#include "dungeon/quest.h"
#include "floor/floor-base-definitions.h"
#include "floor/sight-definitions.h"
#include "monster/monster-timed-effect-types.h"
#include "system/angband.h"

#include <vector>

struct grid_type;
;
class ObjectType;
struct monster_type;
struct floor_type {
    DUNGEON_IDX dungeon_idx;
    std::vector<std::vector<grid_type>> grid_array;
    DEPTH dun_level; /*!< 現在の実ダンジョン階層 base_level の参照元となる / Current dungeon level */
    DEPTH base_level; /*!< 基本生成レベル、後述のobject_level, monster_levelの参照元となる / Base dungeon level */
    DEPTH object_level; /*!< アイテムの生成レベル、 base_level を起点に一時変更する時に参照 / Current object creation level */
    DEPTH monster_level; /*!< モンスターの生成レベル、 base_level を起点に一時変更する時に参照 / Current monster creation level */
    POSITION width; /*!< Current dungeon width */
    POSITION height; /*!< Current dungeon height */
    MONSTER_NUMBER num_repro; /*!< Current reproducer count */

    GAME_TURN generated_turn; /* Turn when level began */

    std::vector<ObjectType> o_list; /*!< The array of dungeon items [max_o_idx] */
    OBJECT_IDX o_max; /* Number of allocated objects */
    OBJECT_IDX o_cnt; /* Number of live objects */

    std::vector<monster_type> m_list; /*!< The array of dungeon monsters [max_m_idx] */
    MONSTER_IDX m_max; /* Number of allocated monsters */
    MONSTER_IDX m_cnt; /* Number of live monsters */

    std::vector<int16_t> mproc_list[MAX_MTIMED]; /*!< The array to process dungeon monsters[max_m_idx] */
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
    QuestId quest_number; /* Inside quest level */
    bool inside_arena; /* Is character inside on_defeat_arena_monster? */

    bool is_in_dungeon() const;
};
