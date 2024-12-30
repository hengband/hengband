#include "cmd-io/cmd-lore.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "game-option/cheat-options.h"
#include "game-option/game-play-options.h"
#include "io/input-key-acceptor.h"
#include "lore/lore-util.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "tracking/lore-tracker.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-lore.h"

namespace {
std::pair<std::string, std::vector<MonraceId>> collect_monraces(char symbol)
{
    const auto &monraces = MonraceList::get_instance();
    const auto is_known_only = !cheat_know;

    switch (symbol) {
    case KTRL('A'): {
        constexpr auto msg = _("全モンスターのリスト", "Full monster list.");
        auto filter = [](const MonraceDefinition &) { return true; };
        return std::make_pair(msg, monraces.search(std::move(filter), is_known_only));
    }
    case KTRL('U'): {
        constexpr auto msg = _("ユニーク・モンスターのリスト", "Unique monster list.");
        auto filter = [](const MonraceDefinition &monrace) { return monrace.kind_flags.has(MonsterKindType::UNIQUE); };
        return std::make_pair(msg, monraces.search(std::move(filter), is_known_only));
    }
    case KTRL('N'): {
        constexpr auto msg = _("ユニーク外モンスターのリスト", "Non-unique monster list.");
        auto filter = [](const MonraceDefinition &monrace) { return monrace.kind_flags.has_not(MonsterKindType::UNIQUE); };
        return std::make_pair(msg, monraces.search(std::move(filter), is_known_only));
    }
    case KTRL('R'): {
        constexpr auto msg = _("乗馬可能モンスターのリスト", "Ridable monster list.");
        auto filter = [](const MonraceDefinition &monrace) { return monrace.misc_flags.has(MonsterMiscType::RIDING); };
        return std::make_pair(msg, monraces.search(std::move(filter), is_known_only));
    }
    case KTRL('M'): {
        const auto monster_name = input_string(_("名前(英語の場合小文字で可)", "Enter name:"), MAX_MONSTER_NAME);
        if (!monster_name) {
            return { "", {} };
        }

        auto msg = format(_("名前:%sにマッチ", "Monsters' names with \"%s\""), monster_name->data());
        return std::make_pair(msg, monraces.search_by_name(*monster_name, is_known_only));
    }
    default: {
        int ident_i;
        for (ident_i = 0; ident_info[ident_i]; ++ident_i) {
            if (symbol == ident_info[ident_i][0]) {
                break;
            }
        }

        if (ident_info[ident_i]) {
            auto msg = format("%c - %s.", symbol, ident_info[ident_i] + 2);
            return std::make_pair(msg, monraces.search_by_symbol(symbol, is_known_only));
        }

        return std::make_pair(_("無効な文字", "Unknown Symbol"), std::vector<MonraceId>{});
    }
    }
}
}

/*!
 * @brief モンスターの思い出を見るコマンドのメインルーチン
 * Identify a character, allow recall of monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * <pre>
 * Several "special" responses recall "multiple" monsters:
 *   ^A (all monsters)
 *   ^U (all unique monsters)
 *   ^N (all non-unique monsters)
 *
 * The responses may be sorted in several ways, see below.
 *
 * Note that the player ghosts are ignored.
 * </pre>
 */
void do_cmd_query_symbol(PlayerType *player_ptr)
{
    constexpr auto prompt = _("知りたい文字を入力して下さい(記号 or ^A全,^Uユ,^N非ユ,^R乗馬,^M名前): ",
        "Enter character to be identified(^A:All,^U:Uniqs,^N:Non uniqs,^M:Name): ");
    const auto symbol = input_command(prompt);
    if (!symbol) {
        return;
    }

    auto [buf, monrace_ids] = collect_monraces(*symbol);
    prt(buf, 0, 0);

    if (monrace_ids.empty()) {
        return;
    }

    put_str(_("思い出を見ますか? (k:殺害順/y/n): ", "Recall details? (k/y/n): "), 0, _(36, 40));
    auto query = inkey();
    prt(buf, 0, 0);
    auto is_detailed = false;
    if (query == 'k') {
        is_detailed = true;
        query = 'y';
    }

    if (query != 'y') {
        return;
    }

    const auto &monraces = MonraceList::get_instance();
    std::stable_sort(monrace_ids.begin(), monrace_ids.end(),
        [&monraces, is_detailed](auto x, auto y) { return monraces.order(x, y, is_detailed); });
    auto i = std::ssize(monrace_ids) - 1;
    auto &tracker = LoreTracker::get_instance();
    auto recall = false;
    while (true) {
        const auto monrace_id = monrace_ids[i];
        tracker.set_trackee(monrace_id);
        handle_stuff(player_ptr);
        while (true) {
            if (recall) {
                screen_save();
                screen_roff(player_ptr, monrace_ids[i], MONSTER_LORE_NORMAL);
            }

            roff_top(monrace_id);
            term_addstr(-1, TERM_WHITE, _(" ['r'思い出, ESC]", " [(r)ecall, ESC]"));
            query = inkey();
            if (recall) {
                screen_load();
            }

            if (query != 'r') {
                break;
            }
            recall = !recall;
        }

        if (query == ESCAPE) {
            break;
        }

        if (query == '-') {
            if (++i == std::ssize(monrace_ids)) {
                i = 0;
                if (!expand_list) {
                    break;
                }
            }
        } else {
            if (i-- == 0) {
                i = monrace_ids.size() - 1;
                if (!expand_list) {
                    break;
                }
            }
        }
    }

    prt(buf, 0, 0);
}
