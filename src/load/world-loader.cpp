#include "load/world-loader.h"
#include "floor/wild.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "load/load-zangband.h"
#include "market/bounty.h"
#include "system/angband-system.h"
#include "system/building-type-definition.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-record.h"
#include "system/floor-type-definition.h"
#include "system/inner-game-data.h"
#include "system/player-type-definition.h"
#include "world/world.h"

static void rd_hengband_dungeons(void)
{
    auto max = rd_byte();
    for (auto i = 0U; i < max; i++) {
        auto tmp16s = rd_s16b();
        if (i >= dungeons_info.size()) {
            continue;
        }

        max_dlv[i] = tmp16s;
        if (max_dlv[i] > dungeons_info[i].maxdepth) {
            max_dlv[i] = dungeons_info[i].maxdepth;
        }
    }
}

void rd_dungeons(PlayerType *player_ptr)
{
    if (h_older_than(0, 3, 8)) {
        rd_zangband_dungeon();
    } else {
        rd_hengband_dungeons();
    }

    if (player_ptr->max_plv < player_ptr->lev) {
        player_ptr->max_plv = player_ptr->lev;
    }
}

/*!
 * @brief 現実変容処理の有無及びその残りターン数を読み込む
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void rd_alter_reality(PlayerType *player_ptr)
{
    if (h_older_than(0, 3, 8)) {
        player_ptr->recall_dungeon = DUNGEON_ANGBAND;
    } else {
        player_ptr->recall_dungeon = rd_s16b();
    }

    if (h_older_than(1, 5, 0, 0)) {
        player_ptr->alter_reality = 0;
    } else {
        player_ptr->alter_reality = rd_s16b();
    }
}

void set_gambling_monsters()
{
    auto &melee_arena = MeleeArena::get_instance();
    for (auto i = 0; i < NUM_GLADIATORS; i++) {
        const auto monrace_id = i2enum<MonraceId>(rd_s16b());
        uint32_t odds;
        if (h_older_than(0, 3, 4)) {
            odds = rd_s16b();
        } else {
            odds = rd_u32b();
        }

        melee_arena.set_gladiator(i, { monrace_id, odds });
    }
}

/*!
 * @details 自動拾い関係はこれしかないのでworldに突っ込むことにする。必要があれば再分割する
 */
void rd_autopick(PlayerType *player_ptr)
{
    player_ptr->autopick_autoregister = rd_bool();
}

static void rd_world_info(PlayerType *player_ptr)
{
    auto &igd = InnerGameData::get_instance();
    igd.init_turn_limit();
    auto &world = AngbandWorld::get_instance();
    world.dungeon_turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
    player_ptr->current_floor_ptr->generated_turn = rd_s32b();
    if (h_older_than(1, 7, 0, 4)) {
        player_ptr->feeling_turn = player_ptr->current_floor_ptr->generated_turn;
    } else {
        player_ptr->feeling_turn = rd_s32b();
    }

    world.game_turn = rd_s32b();
    if (h_older_than(0, 3, 12)) {
        world.dungeon_turn = world.game_turn;
    } else {
        world.dungeon_turn = rd_s32b();
    }

    if (h_older_than(1, 0, 13)) {
        set_zangband_game_turns(player_ptr);
    }

    if (h_older_than(0, 3, 13)) {
        world.arena_start_turn = world.game_turn;
    } else {
        world.arena_start_turn = rd_s32b();
    }

    if (h_older_than(0, 0, 3)) {
        determine_daily_bounty(player_ptr, true);
    } else {
        world.today_mon = i2enum<MonraceId>(rd_s16b());
        world.knows_daily_bounty = rd_s16b() != 0; // 現在bool型だが、かつてモンスター種族IDを保存していた仕様に合わせる
    }
}

void rd_visited_towns(PlayerType *player_ptr)
{
    if (h_older_than(0, 3, 9)) {
        player_ptr->visit = 1L;
        return;
    }

    if (h_older_than(0, 3, 10)) {
        set_zangband_visited_towns(player_ptr);
        return;
    }

    player_ptr->visit = rd_u32b();
}

void rd_global_configurations(PlayerType *player_ptr)
{
    auto &system = AngbandSystem::get_instance();
    system.set_seed_flavor(rd_u32b());
    system.set_seed_town(rd_u32b());

    player_ptr->panic_save = rd_u16b();
    auto &world = AngbandWorld::get_instance();
    world.total_winner = rd_u16b();
    world.noscore = rd_u16b();

    player_ptr->is_dead = rd_bool();

    player_ptr->feeling = rd_byte();
    rd_world_info(player_ptr);
}

void load_wilderness_info(PlayerType *player_ptr)
{
    player_ptr->wilderness_x = rd_s32b();
    player_ptr->wilderness_y = rd_s32b();
    if (h_older_than(0, 3, 13)) {
        player_ptr->wilderness_x = 5;
        player_ptr->wilderness_y = 48;
    }

    auto &world = AngbandWorld::get_instance();
    if (h_older_than(0, 3, 7)) {
        world.set_wild_mode(false);
    } else {
        world.set_wild_mode(rd_bool());
    }

    if (h_older_than(0, 3, 7)) {
        player_ptr->ambush_flag = false;
    } else {
        player_ptr->ambush_flag = rd_bool();
    }
}

errr analyze_wilderness(void)
{
    const auto wild_x_size = rd_s32b();
    const auto wild_y_size = rd_s32b();
    auto &world = AngbandWorld::get_instance();
    if ((wild_x_size > world.max_wild_x) || (wild_y_size > world.max_wild_y)) {
        load_note(format(_("荒野が大きすぎる(%u/%u)！", "Wilderness is too big (%u/%u)!"), wild_x_size, wild_y_size));
        return 23;
    }

    for (int i = 0; i < wild_x_size; i++) {
        for (int j = 0; j < wild_y_size; j++) {
            wilderness[j][i].seed = rd_u32b();
        }
    }

    return 0;
}
