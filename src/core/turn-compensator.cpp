﻿#include "core/turn-compensator.h"
#include "floor/floor-town.h"
#include "player/player-race-types.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/floor-type-definition.h"
#include "world/world.h"

/*!
 * @brief ゲームターンからの実時間換算を行うための補正をかける
 * @param hoge ゲームターン
 * @details アンデッド種族は18:00からゲームを開始するので、この修正を予め行う。
 * @return 修正をかけた後のゲームターン
 */
s32b turn_real(player_type *player_ptr, s32b hoge)
{
    switch (player_ptr->start_race) {
    case RACE_VAMPIRE:
    case RACE_SKELETON:
    case RACE_ZOMBIE:
    case RACE_SPECTRE:
        return hoge - (TURNS_PER_TICK * TOWN_DAWN * 3 / 4);
    default:
        return hoge;
    }
}

/*!
 * @brief ターンのオーバーフローに対する対処
 * @param player_ptr プレーヤーへの参照ポインタ
 * @details ターン及びターンを記録する変数をターンの限界の1日前まで巻き戻す.
 * @return 修正をかけた後のゲームターン
 */
void prevent_turn_overflow(player_type *player_ptr)
{
    if (current_world_ptr->game_turn < current_world_ptr->game_turn_limit)
        return;

    int rollback_days = 1 + (current_world_ptr->game_turn - current_world_ptr->game_turn_limit) / (TURNS_PER_TICK * TOWN_DAWN);
    s32b rollback_turns = TURNS_PER_TICK * TOWN_DAWN * rollback_days;

    if (current_world_ptr->game_turn > rollback_turns)
        current_world_ptr->game_turn -= rollback_turns;
    else
        current_world_ptr->game_turn = 1;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (floor_ptr->generated_turn > rollback_turns)
        floor_ptr->generated_turn -= rollback_turns;
    else
        floor_ptr->generated_turn = 1;
    if (current_world_ptr->arena_start_turn > rollback_turns)
        current_world_ptr->arena_start_turn -= rollback_turns;
    else
        current_world_ptr->arena_start_turn = 1;
    if (player_ptr->feeling_turn > rollback_turns)
        player_ptr->feeling_turn -= rollback_turns;
    else
        player_ptr->feeling_turn = 1;

    for (int i = 1; i < max_towns; i++) {
        for (int j = 0; j < MAX_STORES; j++) {
            store_type *store_ptr = &town_info[i].store[j];

            if (store_ptr->last_visit > -10L * TURNS_PER_TICK * STORE_TICKS) {
                store_ptr->last_visit -= rollback_turns;
                if (store_ptr->last_visit < -10L * TURNS_PER_TICK * STORE_TICKS)
                    store_ptr->last_visit = -10L * TURNS_PER_TICK * STORE_TICKS;
            }

            if (store_ptr->store_open) {
                store_ptr->store_open -= rollback_turns;
                if (store_ptr->store_open < 1)
                    store_ptr->store_open = 1;
            }
        }
    }
}
