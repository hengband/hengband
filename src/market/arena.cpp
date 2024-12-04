/*!
 * @brief アリーナのUI処理
 * @author Hourier
 * @date 2024/06/22
 */

#include "market/arena.h"
#include "core/asking-player.h"
#include "core/show-file.h"
#include "core/stuff-handler.h"
#include "floor/floor-mode-changer.h"
#include "market/arena-entry.h"
#include "market/building-actions-table.h"
#include "market/building-util.h"
#include "player-base/player-class.h"
#include "status/buff-setter.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "term/screen-processor.h"
#include "tracking/lore-tracker.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <optional>

/*!
 * @brief 優勝時のメッセージを表示し、賞金を与える
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return まだ優勝していないか、挑戦者モンスターとの戦いではFALSE
 */
static std::optional<int> process_ostensible_arena_victory()
{
    auto &entries = ArenaEntryList::get_instance();
    if (!entries.is_player_victor()) {
        return std::nullopt;
    }

    clear_bldg(5, 19);
    prt(_("アリーナの優勝者！", "               Arena Victor!"), 5, 0);
    prt(_("おめでとう！あなたは全ての敵を倒しました。", "Congratulations!  You have defeated all before you."), 7, 0);
    prt(_("賞金として $1,000,000 が与えられます。", "For that, receive the prize: 1,000,000 gold pieces"), 8, 0);

    prt("", 10, 0);
    prt("", 11, 0);
    msg_print(_("スペースキーで続行", "Press the space bar to continue"));
    msg_print(nullptr);
    entries.increment_entry();
    return 1000000;
}

static bool check_battle_metal_babble(PlayerType *player_ptr)
{
    msg_print(_("最強の挑戦者が君に決闘を申し込んできた。", "The strongest challenger throws down the gauntlet to your feet."));
    msg_print(nullptr);
    if (!input_check(_("受けて立つかね？", "Do you take up the gauntlet? "))) {
        msg_print(_("失望したよ。", "We are disappointed."));
        return false;
    }

    msg_print(_("挑戦者「死ぬがよい。」", "The challenger says, 'Die, maggots.'"));
    msg_print(nullptr);

    AngbandWorld::get_instance().set_arena(false);
    reset_tim_flags(player_ptr);
    FloorChangeModesStore::get_instace()->set(FloorChangeMode::SAVE_FLOORS);
    player_ptr->current_floor_ptr->inside_arena = true;
    player_ptr->leaving = true;
    return true;
}

/*!
 * @brief アリーナへの入場処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return アリーナへ入場するか否か
 */
static bool go_to_arena(PlayerType *player_ptr)
{
    const auto prize_money = process_ostensible_arena_victory();
    if (prize_money) {
        player_ptr->au += *prize_money;
        return false;
    }

    const auto arena_record = ArenaEntryList::get_instance().check_arena_record();
    if (arena_record == ArenaRecord::METAL_BABBLE) {
        msg_print(_("あなたはアリーナに入り、しばらくの間栄光にひたった。", "You enter the arena briefly and bask in your glory."));
        msg_print(nullptr);
        return false;
    }

    if ((arena_record == ArenaRecord::POWER_WYRM) && !check_battle_metal_babble(player_ptr)) {
        return false;
    }

    if (player_ptr->riding && !PlayerClass(player_ptr).is_tamer()) {
        msg_print(_("ペットに乗ったままではアリーナへ入れさせてもらえなかった。", "You don't have permission to enter with pet."));
        msg_print(nullptr);
        return false;
    }

    AngbandWorld::get_instance().set_arena(false);
    reset_tim_flags(player_ptr);
    FloorChangeModesStore::get_instace()->set(FloorChangeMode::SAVE_FLOORS);
    player_ptr->current_floor_ptr->inside_arena = true;
    player_ptr->leaving = true;
    return true;
}

/*!
 * @brief アリーナ受付のコマンド処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param cmd アリーナ処理のID
 */
bool arena_comm(PlayerType *player_ptr, int cmd)
{
    switch (cmd) {
    case BACT_ARENA:
        return go_to_arena(player_ptr);
    case BACT_POSTER: {
        const auto &entries = ArenaEntryList::get_instance();
        msg_print(entries.get_poster_message());
        if (entries.is_player_victor() || entries.is_player_true_victor()) {
            return false;
        }

        const auto &monrace = entries.get_monrace();
        LoreTracker::get_instance().set_trackee(monrace.idx);
        handle_stuff(player_ptr);
        return false;
    }
    case BACT_ARENA_RULES:
        screen_save();
        FileDisplayer(player_ptr->name).display(true, _("arena_j.txt", "arena.txt"), 0, 0);
        screen_load();
        return false;
    default:
        THROW_EXCEPTION(std::logic_error, "Invalid building action is specified!");
    }
}
