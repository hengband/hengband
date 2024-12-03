#include "market/building-monster.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "game-option/game-play-options.h"
#include "io/input-key-acceptor.h"
#include "lore/lore-util.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-race-info.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "tracking/lore-tracker.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-lore.h"
#include "view/display-messages.h"
#include <algorithm>

/*!
 * @brief 施設でモンスターの情報を知るメインルーチン / research_mon -KMW-
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 常にTRUEを返す。
 * @todo 返り値が意味不明なので直した方が良いかもしれない。
 */
bool research_mon(PlayerType *player_ptr)
{
    bool recall = false;
    bool all = false;
    bool uniq = false;
    bool norm = false;
    screen_save();
    constexpr auto prompt = _("モンスターの文字を入力して下さい(記号 or ^A全,^Uユ,^N非ユ,^M名前):",
        "Enter character to be identified(^A:All,^U:Uniqs,^N:Non uniqs,^M:Name): ");
    const auto sym = input_command(prompt, false);
    if (!sym) {
        screen_load();
        return false;
    }

    IDX ident_i;
    for (ident_i = 0; ident_info[ident_i]; ++ident_i) {
        if (sym == ident_info[ident_i][0]) {
            break;
        }
    }

    std::string buf;
    std::string monster_name("");
    if (sym == KTRL('A')) {
        all = true;
        buf = _("全モンスターのリスト", "Full monster list.");
    } else if (sym == KTRL('U')) {
        all = uniq = true;
        buf = _("ユニーク・モンスターのリスト", "Unique monster list.");
    } else if (sym == KTRL('N')) {
        all = norm = true;
        buf = _("ユニーク外モンスターのリスト", "Non-unique monster list.");
    } else if (sym == KTRL('M')) {
        all = true;
        const auto monster_name_opt = input_string(_("名前(英語の場合小文字で可)", "Enter name:"), MAX_MONSTER_NAME);
        if (!monster_name_opt) {
            screen_load();
            return false;
        }

        monster_name = *monster_name_opt;
        buf = format(_("名前:%sにマッチ", "Monsters' names with \"%s\""), monster_name.data());
    } else if (ident_info[ident_i]) {
        buf = format("%c - %s.", *sym, ident_info[ident_i] + 2);
    } else {
        buf = format("%c - %s", *sym, _("無効な文字", "Unknown Symbol"));
    }

    prt(buf, 16, 10);
    std::vector<MonraceId> monrace_ids;
    auto &monraces = MonraceList::get_instance();
    for (const auto &[monrace_id, monrace] : monraces) {
        if (!monrace.is_valid()) {
            continue;
        }

        /* Require non-unique monsters if needed */
        if (norm && monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            continue;
        }

        /* Require unique monsters if needed */
        if (uniq && monrace.kind_flags.has_not(MonsterKindType::UNIQUE)) {
            continue;
        }

        /* 名前検索 */
        if (!monster_name.empty()) {
            for (size_t xx = 0; (xx < monster_name.length()); xx++) {
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
            for (auto &ch : temp2) {
                if (isupper(ch)) {
                    ch = static_cast<char>(tolower(ch));
                }
            }

#ifdef JP
            if (str_find(temp2, monster_name) || str_find(monrace.name.string(), monster_name))
#else
            if (str_find(temp2, monster_name))
#endif
                monrace_ids.push_back(monrace_id);
        } else if (all || (monrace.symbol_definition.character == sym)) {
            monrace_ids.push_back(monrace_id);
        }
    }

    if (monrace_ids.empty()) {
        screen_load();
        return false;
    }

    std::stable_sort(monrace_ids.begin(), monrace_ids.end(), [&monraces](auto x, auto y) { return monraces.order(x, y); });

    uint i;
    static int old_sym = '\0';
    static uint old_i = 0;
    if (old_sym == sym && old_i < monrace_ids.size()) {
        i = old_i;
    } else {
        i = monrace_ids.size() - 1;
    }

    auto &tracker = LoreTracker::get_instance();
    auto notpicked = true;
    auto query = 'y';
    while (notpicked) {
        auto monrace_id = monrace_ids[i];
        roff_top(monrace_id);
        term_addstr(-1, TERM_WHITE, _(" ['r'思い出, ' 'で続行, ESC]", " [(r)ecall, ESC, space to continue]"));
        while (true) {
            if (recall) {
                const auto mes = monraces.probe_lore(monrace_id);
                if (mes) {
                    msg_print(*mes);
                    msg_print(nullptr);
                }

                tracker.set_trackee(monrace_id);
                handle_stuff(player_ptr);
                screen_roff(player_ptr, monrace_id, MONSTER_LORE_RESEARCH);
                notpicked = false;
                old_sym = *sym;
                old_i = i;
            }

            query = inkey();
            if (query != 'r') {
                break;
            }

            recall = !recall;
        }

        if (query == ESCAPE) {
            break;
        }

        if (query == '-') {
            if (++i == monrace_ids.size()) {
                i = 0;
                if (!expand_list) {
                    break;
                }
            }

            continue;
        }

        if (i-- == 0) {
            i = monrace_ids.size() - 1;
            if (!expand_list) {
                break;
            }
        }
    }

    screen_load();
    return !notpicked;
}
