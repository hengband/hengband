#include "market/building-monster.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "game-option/game-play-options.h"
#include "io/input-key-acceptor.h"
#include "lore/lore-store.h"
#include "lore/lore-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "system/monster-race-info.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/int-char-converter.h"
#include "util/sort.h"
#include "util/string-processor.h"
#include "view/display-lore.h"

/*!
 * @brief 施設でモンスターの情報を知るメインルーチン / research_mon -KMW-
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 常にTRUEを返す。
 * @todo 返り値が意味不明なので直した方が良いかもしれない。
 */
bool research_mon(PlayerType *player_ptr)
{
    bool notpicked;
    bool recall = false;
    uint16_t why = 0;

    bool all = false;
    bool uniq = false;
    bool norm = false;
    screen_save();
    constexpr auto prompt = _("モンスターの文字を入力して下さい(記号 or ^A全,^Uユ,^N非ユ,^M名前):",
        "Enter character to be identified(^A:All,^U:Uniqs,^N:Non uniqs,^M:Name): ");
    const auto sym_opt = input_command(prompt, false);
    if (!sym_opt) {
        screen_load();
        return false;
    }

    const auto sym = sym_opt.value();
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

        monster_name = monster_name_opt.value();
        buf = format(_("名前:%sにマッチ", "Monsters' names with \"%s\""), monster_name.data());
    } else if (ident_info[ident_i]) {
        buf = format("%c - %s.", sym, ident_info[ident_i] + 2);
    } else {
        buf = format("%c - %s", sym, _("無効な文字", "Unknown Symbol"));
    }

    prt(buf, 16, 10);
    std::vector<MonsterRaceId> monraces;
    for (const auto &[monrace_id, monrace] : monraces_info) {
        if (!MonsterRace(monrace_id).is_valid()) {
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

            std::string temp2 = _(monrace.E_name, monrace.name);
            for (auto &ch : temp2) {
                if (isupper(ch)) {
                    ch = static_cast<char>(tolower(ch));
                }
            }

#ifdef JP
            if (str_find(temp2, monster_name) || str_find(monrace.name, monster_name))
#else
            if (str_find(temp2, monster_name))
#endif
                monraces.push_back(monrace_id);
        } else if (all || (monrace.d_char == sym)) {
            monraces.push_back(monrace_id);
        }
    }

    if (monraces.empty()) {
        screen_load();

        return false;
    }

    why = 2;
    char query = 'y';

    if (why) {
        ang_sort(player_ptr, monraces.data(), &why, monraces.size(), ang_sort_comp_hook, ang_sort_swap_hook);
    }

    uint i;
    static int old_sym = '\0';
    static uint old_i = 0;
    if (old_sym == sym && old_i < monraces.size()) {
        i = old_i;
    } else {
        i = monraces.size() - 1;
    }

    notpicked = true;
    while (notpicked) {
        auto r_idx = monraces[i];
        roff_top(r_idx);
        term_addstr(-1, TERM_WHITE, _(" ['r'思い出, ' 'で続行, ESC]", " [(r)ecall, ESC, space to continue]"));
        while (true) {
            if (recall) {
                lore_do_probe(player_ptr, r_idx);
                monster_race_track(player_ptr, r_idx);
                handle_stuff(player_ptr);
                screen_roff(player_ptr, r_idx, MONSTER_LORE_RESEARCH);
                notpicked = false;
                old_sym = sym;
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
            if (++i == monraces.size()) {
                i = 0;
                if (!expand_list) {
                    break;
                }
            }

            continue;
        }

        if (i-- == 0) {
            i = monraces.size() - 1;
            if (!expand_list) {
                break;
            }
        }
    }

    screen_load();
    return !notpicked;
}
