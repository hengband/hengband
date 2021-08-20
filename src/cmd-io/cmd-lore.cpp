#include "cmd-io/cmd-lore.h"
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
 * @param player_ptr プレーヤーへの参照ポインタ
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
void do_cmd_query_symbol(player_type *creature_ptr)
{
    MONRACE_IDX i;
    int n;
    MONRACE_IDX r_idx;
    char sym, query;
    char buf[256];

    bool all = false;
    bool uniq = false;
    bool norm = false;
    bool ride = false;
    char temp[MAX_MONSTER_NAME] = "";

    bool recall = false;

    uint16_t why = 0;
    MONRACE_IDX *who;

    if (!get_com(_("知りたい文字を入力して下さい(記号 or ^A全,^Uユ,^N非ユ,^R乗馬,^M名前): ",
                     "Enter character to be identified(^A:All,^U:Uniqs,^N:Non uniqs,^M:Name): "),
            &sym, false))
        return;

    for (i = 0; ident_info[i]; ++i) {
        if (sym == ident_info[i][0])
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
    } else if (ident_info[i]) {
        sprintf(buf, "%c - %s.", sym, ident_info[i] + 2);
    } else {
        sprintf(buf, "%c - %s", sym, _("無効な文字", "Unknown Symbol"));
    }

    prt(buf, 0, 0);
    C_MAKE(who, max_r_idx, MONRACE_IDX);
    for (n = 0, i = 1; i < max_r_idx; i++) {
        monster_race *r_ptr = &r_info[i];
        if (!cheat_know && !r_ptr->r_sights)
            continue;

        if (norm && (r_ptr->flags1 & (RF1_UNIQUE)))
            continue;

        if (uniq && !(r_ptr->flags1 & (RF1_UNIQUE)))
            continue;

        if (ride && !(r_ptr->flags7 & (RF7_RIDING)))
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
            strcpy(temp2, r_ptr->E_name.c_str());
#else
            strcpy(temp2, r_ptr->name.c_str());
#endif
            for (xx = 0; temp2[xx] && xx < MAX_MONSTER_NAME; xx++)
                if (isupper(temp2[xx]))
                    temp2[xx] = (char)tolower(temp2[xx]);

#ifdef JP
            if (angband_strstr(temp2, temp) || angband_strstr(r_ptr->name.c_str(), temp))
#else
            if (angband_strstr(temp2, temp))
#endif
                who[n++] = i;
        }

        else if (all || (r_ptr->d_char == sym))
            who[n++] = i;
    }

    if (!n) {
        C_KILL(who, max_r_idx, MONRACE_IDX);
        return;
    }

    put_str(_("思い出を見ますか? (k:殺害順/y/n): ", "Recall details? (k/y/n): "), 0, _(36, 40));
    query = inkey();
    prt(buf, 0, 0);
    why = 2;
    ang_sort(creature_ptr, who, &why, n, ang_sort_comp_hook, ang_sort_swap_hook);
    if (query == 'k') {
        why = 4;
        query = 'y';
    }

    if (query != 'y') {
        C_KILL(who, max_r_idx, MONRACE_IDX);
        return;
    }

    if (why == 4) {
        ang_sort(creature_ptr, who, &why, n, ang_sort_comp_hook, ang_sort_swap_hook);
    }

    i = n - 1;
    while (true) {
        r_idx = who[i];
        monster_race_track(creature_ptr, r_idx);
        handle_stuff(creature_ptr);
        while (true) {
            if (recall) {
                screen_save();
                screen_roff(creature_ptr, who[i], MONSTER_LORE_NORMAL);
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
            if (++i == n) {
                i = 0;
                if (!expand_list)
                    break;
            }
        } else {
            if (i-- == 0) {
                i = n - 1;
                if (!expand_list)
                    break;
            }
        }
    }

    C_KILL(who, max_r_idx, MONRACE_IDX);
    prt(buf, 0, 0);
}
