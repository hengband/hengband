﻿#include "dungeon/quest-monster-placer.h"
#include "dungeon/quest.h"
#include "floor/floor-generator-util.h"
#include "floor/geometry.h"
#include "grid/feature.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief クエストに関わるモンスターの配置を行う / Place quest monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 成功したならばTRUEを返す
 */
bool place_quest_monsters(PlayerType *player_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    for (int i = 0; i < max_q_idx; i++) {
        monster_race *r_ptr;
        BIT_FLAGS mode;
        if (quest[i].status != QuestStatusType::TAKEN || (quest[i].type != QuestKindType::KILL_LEVEL && quest[i].type != QuestKindType::RANDOM)
            || quest[i].level != floor_ptr->dun_level || player_ptr->dungeon_idx != quest[i].dungeon || (quest[i].flags & QUEST_FLAG_PRESET)) {
            continue;
        }

        r_ptr = &r_info[quest[i].r_idx];
        if ((r_ptr->flags1 & RF1_UNIQUE) && (r_ptr->cur_num >= r_ptr->max_num))
            continue;

        mode = PM_NO_KAGE | PM_NO_PET;
        if (!(r_ptr->flags1 & RF1_FRIENDS))
            mode |= PM_ALLOW_GROUP;

        for (int j = 0; j < (quest[i].max_num - quest[i].cur_num); j++) {
            int k;
            for (k = 0; k < SAFE_MAX_ATTEMPTS; k++) {
                POSITION x = 0;
                POSITION y = 0;
                int l;
                for (l = SAFE_MAX_ATTEMPTS; l > 0; l--) {
                    grid_type *g_ptr;
                    feature_type *f_ptr;
                    y = randint0(floor_ptr->height);
                    x = randint0(floor_ptr->width);
                    g_ptr = &floor_ptr->grid_array[y][x];
                    f_ptr = &f_info[g_ptr->feat];
                    if (f_ptr->flags.has_none_of({FF::MOVE, FF::CAN_FLY}))
                        continue;

                    if (!monster_can_enter(player_ptr, y, x, r_ptr, 0))
                        continue;

                    if (distance(y, x, player_ptr->y, player_ptr->x) < 10)
                        continue;

                    if (g_ptr->is_icky())
                        continue;
                    else
                        break;
                }

                if (l == 0)
                    return false;

                if (place_monster_aux(player_ptr, 0, y, x, quest[i].r_idx, mode))
                    break;
                else
                    continue;
            }

            if (k == SAFE_MAX_ATTEMPTS)
                return false;
        }
    }

    return true;
}
