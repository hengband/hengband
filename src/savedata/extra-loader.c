/*!
 * todo 「その他」が雑多すぎて肥大化している。今後の課題として分割を検討する
 * @brief その他の情報を読み込む処理
 * @date 2020/07/05
 * @author Hourier
 */

#include "savedata/extra-loader.h"
#include "cmd-building/cmd-building.h"
#include "dungeon/dungeon.h"
#include "floor/floor.h"
#include "game-option/birth-options.h"
#include "market/arena.h"
#include "market/bounty.h"
#include "monster-race/monster-race.h"
#include "mutation/mutation.h"
#include "object-enchant/tr-types.h"
#include "player/attack-defense-types.h"
#include "player/avatar.h"
#include "player/patron.h"
#include "player/player-skill.h"
#include "player/special-defense-types.h"
#include "realm/realm-types.h"
#include "savedata/angband-version-comparer.h"
#include "savedata/birth-loader.h"
#include "savedata/load-util.h"
#include "savedata/load-v1-3-0.h"
#include "savedata/load-v1-7-0.h"
#include "savedata/dummy-loader.h"
#include "savedata/load-zangband.h"
#include "savedata/monster-loader.h"
#include "savedata/player-info-loader.h"
#include "world/world.h"

static void set_undead_turn_limit(player_type *creature_ptr)
{
    switch (creature_ptr->start_race) {
    case RACE_VAMPIRE:
    case RACE_SKELETON:
    case RACE_ZOMBIE:
    case RACE_SPECTRE:
        current_world_ptr->game_turn_limit = TURNS_PER_TICK * TOWN_DAWN * MAX_DAYS + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
        break;
    default:
        current_world_ptr->game_turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
        break;
    }
}

static void rd_world_info(player_type *creature_ptr)
{
    set_undead_turn_limit(creature_ptr);
    current_world_ptr->dungeon_turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
    rd_s32b(&creature_ptr->current_floor_ptr->generated_turn);
    if (h_older_than(1, 7, 0, 4))
        creature_ptr->feeling_turn = creature_ptr->current_floor_ptr->generated_turn;
    else
        rd_s32b(&creature_ptr->feeling_turn);

    rd_s32b(&current_world_ptr->game_turn);
    if (z_older_than(10, 3, 12))
        current_world_ptr->dungeon_turn = current_world_ptr->game_turn;
    else
        rd_s32b(&current_world_ptr->dungeon_turn);

    if (z_older_than(11, 0, 13))
        set_zangband_game_turns(creature_ptr);

    if (z_older_than(10, 3, 13))
        current_world_ptr->arena_start_turn = current_world_ptr->game_turn;
    else
        rd_s32b(&current_world_ptr->arena_start_turn);

    if (z_older_than(10, 0, 3))
        determine_daily_bounty(creature_ptr, TRUE);
    else {
        rd_s16b(&today_mon);
        rd_s16b(&creature_ptr->today_mon);
    }
}

static void rd_visited_towns(player_type *creature_ptr)
{
    if (z_older_than(10, 3, 9)) {
        creature_ptr->visit = 1L;
        return;
    }

    if (z_older_than(10, 3, 10)) {
        set_zangband_visited_towns(creature_ptr);
        return;
    }

    s32b tmp32s;
    rd_s32b(&tmp32s);
    creature_ptr->visit = (BIT_FLAGS)tmp32s;
}

static void rd_global_configurations(player_type *creature_ptr)
{
    rd_u32b(&current_world_ptr->seed_flavor);
    rd_u32b(&current_world_ptr->seed_town);

    rd_u16b(&creature_ptr->panic_save);
    rd_u16b(&current_world_ptr->total_winner);
    rd_u16b(&current_world_ptr->noscore);

    byte tmp8u;
    rd_byte(&tmp8u);
    creature_ptr->is_dead = tmp8u;

    rd_byte(&creature_ptr->feeling);
    rd_world_info(creature_ptr);
}

/*!
 * @brief その他の情報を読み込む / Read the "extra" information
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void rd_extra(player_type *creature_ptr)
{
    rd_byte((byte *)&preserve_mode);
    rd_byte((byte *)&creature_ptr->wait_report_score);
    rd_dummy2();
    rd_global_configurations(creature_ptr);
    if (z_older_than(10, 0, 7))
        creature_ptr->riding = 0;
    else
        rd_s16b(&creature_ptr->riding);

    if (h_older_than(1, 5, 0, 0))
        creature_ptr->floor_id = 0;
    else
        rd_s16b(&creature_ptr->floor_id);

    rd_dummy_monsters(creature_ptr);
    if (z_older_than(10, 1, 2))
        current_world_ptr->play_time = 0;
    else
        rd_u32b(&current_world_ptr->play_time);

    rd_visited_towns(creature_ptr);
    if (!z_older_than(11, 0, 5))
        rd_u32b(&creature_ptr->count);
}
