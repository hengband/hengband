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
    char temp[MAX_MONSTER_NAME] = "";

    screen_save();

    char sym;
    if (!get_com(
            _("モンスターの文字を入力して下さい(記号 or ^A全,^Uユ,^N非ユ,^M名前):", "Enter character to be identified(^A:All,^U:Uniqs,^N:Non uniqs,^M:Name): "),
            &sym, false))

    {
        screen_load();
        return false;
    }

    IDX ident_i;
    for (ident_i = 0; ident_info[ident_i]; ++ident_i) {
        if (sym == ident_info[ident_i][0]) {
            break;
        }
    }

    /* XTRA HACK WHATSEARCH */
    std::string buf;
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
        if (!get_string(_("名前(英語の場合小文字で可)", "Enter name:"), temp, 70)) {
            temp[0] = 0;
            screen_load();

            return false;
        }

        buf = format(_("名前:%sにマッチ", "Monsters' names with \"%s\""), temp);
    } else if (ident_info[ident_i]) {
        buf = format("%c - %s.", sym, ident_info[ident_i] + 2);
    } else {
        buf = format("%c - %s", sym, _("無効な文字", "Unknown Symbol"));
    }

    /* Display the result */
    prt(buf, 16, 10);

    /* Allocate the "who" array */
    std::vector<MonsterRaceId> who;

    /* Collect matching monsters */
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
        if (temp[0]) {
            for (int xx = 0; temp[xx] && xx < 80; xx++) {
#ifdef JP
                if (iskanji(temp[xx])) {
                    xx++;
                    continue;
                }
#endif
                if (isupper(temp[xx])) {
                    temp[xx] = (char)tolower(temp[xx]);
                }
            }

            std::string temp2 = _(monrace.E_name, monrace.name);
            for (auto &ch : temp2) {
                if (isupper(ch)) {
                    ch = static_cast<char>(tolower(ch));
                }
            }

#ifdef JP
            if (str_find(temp2, temp) || str_find(monrace.name, temp))
#else
            if (str_find(temp2, temp))
#endif
                who.push_back(monrace_id);
        } else if (all || (monrace.d_char == sym)) {
            who.push_back(monrace_id);
        }
    }

    if (who.empty()) {
        screen_load();

        return false;
    }

    why = 2;
    char query = 'y';

    if (why) {
        ang_sort(player_ptr, who.data(), &why, who.size(), ang_sort_comp_hook, ang_sort_swap_hook);
    }

    uint i;
    static int old_sym = '\0';
    static uint old_i = 0;
    if (old_sym == sym && old_i < who.size()) {
        i = old_i;
    } else {
        i = who.size() - 1;
    }

    notpicked = true;
    while (notpicked) {
        auto r_idx = who[i];
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
            if (++i == who.size()) {
                i = 0;
                if (!expand_list) {
                    break;
                }
            }

            continue;
        }

        if (i-- == 0) {
            i = who.size() - 1;
            if (!expand_list) {
                break;
            }
        }
    }

    screen_load();
    return !notpicked;
}
