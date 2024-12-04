#include "core/turn-compensator.h"
#include "floor/floor-town.h"
#include "player-info/race-types.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/floor/floor-info.h"
#include "system/inner-game-data.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "world/world.h"

/*!
 * @brief ターンのオーバーフローに対する対処
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details ターン及びターンを記録する変数をターンの限界の1日前まで巻き戻す.
 * @return 修正をかけた後のゲームターン
 */
void prevent_turn_overflow(PlayerType *player_ptr)
{
    const auto &igd = InnerGameData::get_instance();
    const auto game_turn_limit = igd.get_game_turn_limit();
    auto &world = AngbandWorld::get_instance();
    if (world.game_turn < game_turn_limit) {
        return;
    }

    int rollback_days = 1 + (world.game_turn - game_turn_limit) / (TURNS_PER_TICK * TOWN_DAWN);
    int32_t rollback_turns = TURNS_PER_TICK * TOWN_DAWN * rollback_days;

    if (world.game_turn > rollback_turns) {
        world.game_turn -= rollback_turns;
    } else {
        world.game_turn = 1;
    }
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (floor_ptr->generated_turn > rollback_turns) {
        floor_ptr->generated_turn -= rollback_turns;
    } else {
        floor_ptr->generated_turn = 1;
    }
    if (world.arena_start_turn > rollback_turns) {
        world.arena_start_turn -= rollback_turns;
    } else {
        world.arena_start_turn = 1;
    }
    if (player_ptr->feeling_turn > rollback_turns) {
        player_ptr->feeling_turn -= rollback_turns;
    } else {
        player_ptr->feeling_turn = 1;
    }

    for (size_t i = 1; i < towns_info.size(); i++) {
        for (auto sst : STORE_SALE_TYPE_LIST) {
            auto *store_ptr = &towns_info[i].stores[sst];
            if (store_ptr->last_visit > -10L * TURNS_PER_TICK * STORE_TICKS) {
                store_ptr->last_visit -= rollback_turns;
                if (store_ptr->last_visit < -10L * TURNS_PER_TICK * STORE_TICKS) {
                    store_ptr->last_visit = -10L * TURNS_PER_TICK * STORE_TICKS;
                }
            }

            if (store_ptr->store_open) {
                store_ptr->store_open -= rollback_turns;
                if (store_ptr->store_open < 1) {
                    store_ptr->store_open = 1;
                }
            }
        }
    }
}
