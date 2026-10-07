/*!
 * @brief メッセージ消去の入力待ち中に追加された描画要求の回帰プローブ
 * @details ゲーム本体の処理とメモリ上の端末を使う独立プロセス。doctest の対象ではない。
 */
#include "core/window-redrawer.h"
#include "game-option/input-options.h"
#include "game-option/text-display-options.h"
#include "io/input-key-acceptor.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/gameterm.h"
#include "term/z-term.h"
#include "view/display-messages.h"
#include "window/main-window-row-column.h"
#include "world/world.h"
#include <iostream>
#include <string>

namespace {
bool injected = false;
bool reenter = false;
int event_count = 0;

errr event_hook(int action, int)
{
    if (action != TERM_XTRA_EVENT) {
        return 0;
    }

    ++event_count;
    if (!injected) {
        injected = true;
        term_resize(120, 36);
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        rfu.set_flag(MainWindowRedrawingFlag::WIPE);
        rfu.set_flag(MainWindowRedrawingFlag::GOLD);
        // 旧実装でも無限待ちせず失敗を検出できるよう、再入前に応答キーを用意する。
        term_key_push(' ');
        if (reenter) {
            redraw_stuff(p_ptr);
        }
    }

    term_key_push(' ');
    return 0;
}

bool gold_visible(const term_type &term, const std::string &gold)
{
    const auto &row = term.scr->c[ROW_GOLD];
    return std::string(row.begin(), row.end()).find(gold) != std::string::npos;
}
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cerr << "Usage: main-window-wipe-probe plain|deferred|reenter\n";
        return 2;
    }

    const std::string mode(argv[1]);
    if ((mode != "plain") && (mode != "deferred") && (mode != "reenter")) {
        std::cerr << "Unknown mode: " << mode << '\n';
        return 2;
    }

    reenter = mode == "reenter";
    PlayerType player;
    p_ptr = &player;
    player.au = 123456;
    player.playing = true;
    auto &world = AngbandWorld::get_instance();
    world.character_generated = true;
    world.character_dungeon = false;
    world.character_icky_depth = 0;
    world.timewalk_m_idx = 0;
    auto_more = false;
    skip_more = false;
    quick_messages = false;
    num_more = 0;
    msg_flag = false;

    term_type term;
    term_init(&term, 80, 24, 64);
    term.xtra_hook = event_hook;
    term.never_bored = true;
    term.never_fresh = true;
    angband_terms[0] = &term;
    term_activate(&term);
    if (mode != "plain") {
        msg_print("pending message before wipe");
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::WIPE);
    rfu.set_flag(MainWindowRedrawingFlag::GOLD);
    redraw_stuff(&player);

    const auto consumed = !rfu.has(MainWindowRedrawingFlag::WIPE);
    const auto drawn = gold_visible(term, "123456");
    redraw_stuff(&player);
    const auto preserved = gold_visible(term, "123456");
    player.au = 654321;
    rfu.set_flag(MainWindowRedrawingFlag::GOLD);
    redraw_stuff(&player);
    const auto guard_released = gold_visible(term, "654321");
    const auto expected_input = injected == (mode != "plain");
    const auto passed = expected_input && consumed && drawn && preserved && guard_released;
    std::cout << "mode=" << mode << " injected=" << injected << " wipe_consumed=" << consumed
              << " drawn=" << drawn << " preserved=" << preserved
              << " guard_released=" << guard_released << " events=" << event_count
              << " result=" << (passed ? "PASS" : "FAIL") << '\n';
    angband_terms[0] = nullptr;
    game_term = nullptr;
    return passed ? 0 : 1;
}
