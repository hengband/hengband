#include "cmd-io/cmd-lore.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "game-option/cheat-options.h"
#include "game-option/game-play-options.h"
#include "io/input-key-acceptor.h"
#include "lore/lore-util.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "tracking/lore-tracker.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-lore.h"

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
    bool all = false;
    bool uniq = false;
    bool norm = false;
    bool ride = false;
    bool recall = false;

    constexpr auto prompt = _("知りたい文字を入力して下さい(記号 or ^A全,^Uユ,^N非ユ,^R乗馬,^M名前): ",
        "Enter character to be identified(^A:All,^U:Uniqs,^N:Non uniqs,^M:Name): ");
    const auto symbol = input_command(prompt);
    if (!symbol) {
        return;
    }

    int ident_i;
    for (ident_i = 0; ident_info[ident_i]; ++ident_i) {
        if (symbol == ident_info[ident_i][0]) {
            break;
        }
    }

    std::string buf;
    std::string monster_name("");
    if (symbol == KTRL('A')) {
        all = true;
        buf = _("全モンスターのリスト", "Full monster list.");
    } else if (symbol == KTRL('U')) {
        all = uniq = true;
        buf = _("ユニーク・モンスターのリスト", "Unique monster list.");
    } else if (symbol == KTRL('N')) {
        all = norm = true;
        buf = _("ユニーク外モンスターのリスト", "Non-unique monster list.");
    } else if (symbol == KTRL('R')) {
        all = ride = true;
        buf = _("乗馬可能モンスターのリスト", "Ridable monster list.");
    } else if (symbol == KTRL('M')) {
        all = true;
        const auto monster_name_opt = input_string(_("名前(英語の場合小文字で可)", "Enter name:"), MAX_MONSTER_NAME);
        if (!monster_name_opt) {
            return;
        }

        monster_name = *monster_name_opt;
        buf = format(_("名前:%sにマッチ", "Monsters' names with \"%s\""), monster_name.data());
    } else if (ident_info[ident_i]) {
        buf = format("%c - %s.", *symbol, ident_info[ident_i] + 2);
    } else {
        buf = format("%c - %s", *symbol, _("無効な文字", "Unknown Symbol"));
    }

    prt(buf, 0, 0);
    std::vector<MonraceId> monrace_ids;
    const auto &monraces = MonraceList::get_instance();
    for (const auto &[monrace_id, monrace] : monraces) {
        if (!cheat_know && !monrace.r_sights) {
            continue;
        }

        if (norm && monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            continue;
        }

        if (uniq && monrace.kind_flags.has_not(MonsterKindType::UNIQUE)) {
            continue;
        }

        if (ride && monrace.misc_flags.has_not(MonsterMiscType::RIDING)) {
            continue;
        }

        if (!monster_name.empty()) {
            for (size_t xx = 0; xx < monster_name.length(); xx++) {
#ifdef JP
                if (iskanji(monster_name[xx])) {
                    xx++;
                    continue;
                }
#endif
                if (isupper(monster_name[xx])) {
                    monster_name[xx] = (char)tolower(monster_name[xx]);
                }
            }

            std::string temp2 = monrace.name.en_string();
            for (size_t xx = 0; xx < temp2.length(); xx++) {
                if (isupper(temp2[xx])) {
                    temp2[xx] = (char)tolower(temp2[xx]);
                }
            }

#ifdef JP
            if (str_find(temp2, monster_name) || str_find(monrace.name.string(), monster_name))
#else
            if (str_find(temp2, monster_name))
#endif
                monrace_ids.push_back(monrace_id);
        }

        else if (all || (monrace.symbol_definition.character == symbol)) {
            monrace_ids.push_back(monrace_id);
        }
    }

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

    std::stable_sort(monrace_ids.begin(), monrace_ids.end(),
        [&monraces, is_detailed](auto x, auto y) { return monraces.order(x, y, is_detailed); });
    auto i = std::ssize(monrace_ids) - 1;
    auto &tracker = LoreTracker::get_instance();
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
