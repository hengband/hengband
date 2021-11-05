﻿#include "cmd-io/cmd-lore.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "game-option/cheat-options.h"
#include "game-option/game-play-options.h"
#include "io/input-key-acceptor.h"
#include "lore/lore-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags7.h"
#include "system/monster-race-definition.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/int-char-converter.h"
#include "util/sort.h"
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
    char sym, query;
    char buf[256];

    bool all = false;
    bool uniq = false;
    bool norm = false;
    bool ride = false;
    char temp[MAX_MONSTER_NAME] = "";

    bool recall = false;

    uint16_t why = 0;

    if (!get_com(_("知りたい文字を入力して下さい(記号 or ^A全,^Uユ,^N非ユ,^R乗馬,^M名前): ",
                     "Enter character to be identified(^A:All,^U:Uniqs,^N:Non uniqs,^M:Name): "),
            &sym, false))
        return;

    int ident_i;
    for (ident_i = 0; ident_info[ident_i]; ++ident_i) {
        if (sym == ident_info[ident_i][0])
            break;
    }

    if (sym == KTRL('A')) {
        all = true;
        strcpy(buf, _("全モンスターのリスト", "Full monster list."));
    } else if (sym == KTRL('U')) {
        all = uniq = true;
        strcpy(buf, _("ユニーク・モンスターのリスト", "Unique monster list."));
    } else if (sym == KTRL('N')) {
        all = norm = true;
        strcpy(buf, _("ユニーク外モンスターのリスト", "Non-unique monster list."));
    } else if (sym == KTRL('R')) {
        all = ride = true;
        strcpy(buf, _("乗馬可能モンスターのリスト", "Ridable monster list."));
    } else if (sym == KTRL('M')) {
        all = true;
        if (!get_string(_("名前(英語の場合小文字で可)", "Enter name:"), temp, 70)) {
            temp[0] = 0;
            return;
        }
        sprintf(buf, _("名前:%sにマッチ", "Monsters' names with \"%s\""), temp);
    } else if (ident_info[ident_i]) {
        sprintf(buf, "%c - %s.", sym, ident_info[ident_i] + 2);
    } else {
        sprintf(buf, "%c - %s", sym, _("無効な文字", "Unknown Symbol"));
    }

    prt(buf, 0, 0);
    std::vector<MONRACE_IDX> who;
    for (const auto &r_ref : r_info) {
        if (!cheat_know && !r_ref.r_sights)
            continue;

        if (norm && (r_ref.flags1 & (RF1_UNIQUE)))
            continue;

        if (uniq && !(r_ref.flags1 & (RF1_UNIQUE)))
            continue;

        if (ride && !(r_ref.flags7 & (RF7_RIDING)))
            continue;

        if (temp[0]) {
            TERM_LEN xx;
            char temp2[MAX_MONSTER_NAME];

            for (xx = 0; temp[xx] && xx < MAX_MONSTER_NAME; xx++) {
#ifdef JP
                if (iskanji(temp[xx])) {
                    xx++;
                    continue;
                }
#endif
                if (isupper(temp[xx]))
                    temp[xx] = (char)tolower(temp[xx]);
            }

#ifdef JP
            strcpy(temp2, r_ref.E_name.c_str());
#else
            strcpy(temp2, r_ref.name.c_str());
#endif
            for (xx = 0; temp2[xx] && xx < MAX_MONSTER_NAME; xx++)
                if (isupper(temp2[xx]))
                    temp2[xx] = (char)tolower(temp2[xx]);

#ifdef JP
            if (angband_strstr(temp2, temp) || angband_strstr(r_ref.name.c_str(), temp))
#else
            if (angband_strstr(temp2, temp))
#endif
                who.push_back(r_ref.idx);
        }

        else if (all || (r_ref.d_char == sym))
            who.push_back(r_ref.idx);
    }

    if (who.empty()) {
        return;
    }

    put_str(_("思い出を見ますか? (k:殺害順/y/n): ", "Recall details? (k/y/n): "), 0, _(36, 40));
    query = inkey();
    prt(buf, 0, 0);
    why = 2;
    ang_sort(player_ptr, who.data(), &why, who.size(), ang_sort_comp_hook, ang_sort_swap_hook);
    if (query == 'k') {
        why = 4;
        query = 'y';
    }

    if (query != 'y') {
        return;
    }

    if (why == 4) {
        ang_sort(player_ptr, who.data(), &why, who.size(), ang_sort_comp_hook, ang_sort_swap_hook);
    }

    auto i = who.size() - 1;
    while (true) {
        auto r_idx = who[i];
        monster_race_track(player_ptr, r_idx);
        handle_stuff(player_ptr);
        while (true) {
            if (recall) {
                screen_save();
                screen_roff(player_ptr, who[i], MONSTER_LORE_NORMAL);
            }

            roff_top(r_idx);
            term_addstr(-1, TERM_WHITE, _(" ['r'思い出, ESC]", " [(r)ecall, ESC]"));
            query = inkey();
            if (recall) {
                screen_load();
            }

            if (query != 'r')
                break;
            recall = !recall;
        }

        if (query == ESCAPE)
            break;

        if (query == '-') {
            if (++i == who.size()) {
                i = 0;
                if (!expand_list)
                    break;
            }
        } else {
            if (i-- == 0) {
                i = who.size() - 1;
                if (!expand_list)
                    break;
            }
        }
    }

    prt(buf, 0, 0);
}
