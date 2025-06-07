/*!
 * @brief モンスター闘技場関連処理
 * @author Hourier
 * @date 2024/06/22
 */

#include "market/melee-arena.h"
#include "core/asking-player.h"
#include "floor/floor-mode-changer.h"
#include "io/input-key-acceptor.h"
#include "main/sound-of-music.h"
#include "market/building-util.h"
#include "status/buff-setter.h"
#include "system/angband-system.h"
#include "system/building-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <algorithm>
#include <sstream>
#include <string>

namespace {
void display_gladiators()
{
    prt(_("モンスター                                                     倍率", "Monsters                                                       Odds"), 4, 4);
    const auto &melee_arena = MeleeArena::get_instance();
    const auto names = melee_arena.build_gladiators_names();
    for (auto i = 0; i < NUM_GLADIATORS; i++) {
        constexpr auto fmt = _("%d) %-58s  %4d.%02d倍", "%d) %-58s  %4d.%02d");
        const auto &gladiator = melee_arena.get_gladiator(i);
        prt(format(fmt, i + 1, names[i].data(), gladiator.odds / 100, gladiator.odds % 100), 5 + i, 1);
    }

    prt(_("どれに賭けますか:", "Which monster: "), 0, 0);
}
}

/*!
 * @brief モンスター闘技場のメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 賭けを開始したか否か
 */
bool melee_arena_comm(PlayerType *player_ptr)
{
    auto &world = AngbandWorld::get_instance();
    if ((world.game_turn - world.arena_start_turn) > TURNS_PER_TICK * 250) {
        auto &melee_arena = MeleeArena::get_instance();
        melee_arena.update_gladiators(player_ptr);
        world.arena_start_turn = world.game_turn;
    }

    screen_save();

    /* No money */
    if (player_ptr->au <= 1) {
        msg_print(_("おい！おまえ一文なしじゃないか！こっから出ていけ！", "Hey! You don't have gold - get out of here!"));
        msg_erase();
        screen_load();
        return false;
    }

    clear_bldg(4, 10);
    display_gladiators();
    auto &melee_arena = MeleeArena::get_instance();
    while (true) {
        const auto i = inkey();
        if (i == ESCAPE) {
            screen_load();
            return false;
        }

        if ((i >= '1') && (i <= '4')) {
            melee_arena.set_bet_number(i - '1');
            break;
        }

        bell();
    }

    clear_bldg(4, 4);
    for (auto i = 0; i < NUM_GLADIATORS; i++) {
        if (!melee_arena.matches_bet_number(i)) {
            clear_bldg(i + 5, i + 5);
        }
    }

    auto maxbet = player_ptr->lev * 200;
    maxbet = std::min(maxbet, player_ptr->au);
    constexpr auto prompt = _("賭け金？", "Your wager? ");
    const auto wager = input_integer(prompt, 1, maxbet, 1);
    if (!wager) {
        screen_load();
        return false;
    }

    if (wager > player_ptr->au) {
        msg_print(_("おい！金が足りないじゃないか！出ていけ！", "Hey! You don't have the gold - get out of here!"));
        msg_erase();
        screen_load();
        return false;
    }

    msg_erase();
    melee_arena.set_wager(*wager);
    player_ptr->au -= *wager;
    reset_tim_flags(player_ptr);

    FloorChangeModesStore::get_instace()->set(FloorChangeMode::SAVE_FLOORS);
    AngbandSystem::get_instance().set_phase_out(true);
    player_ptr->leaving = true;
    screen_load();
    return true;
}
