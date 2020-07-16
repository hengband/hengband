#include "market/building-monster.h"
#include "core/asking-player.h"
#include "util/sort.h"
#include "core/stuff-handler.h"
#include "game-option/game-play-options.h"
#include "io/input-key-acceptor.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "lore/lore-store.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-lore.h"

/*!
 * @brief 施設でモンスターの情報を知るメインルーチン / research_mon -KMW-
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return 常にTRUEを返す。
 * @todo 返り値が意味不明なので直した方が良いかもしれない。
 */
bool research_mon(player_type *player_ptr)
{
    char buf[128];
    bool notpicked;
    bool recall = FALSE;
    u16b why = 0;
    MONSTER_IDX *who;

    bool all = FALSE;
    bool uniq = FALSE;
    bool norm = FALSE;
    char temp[80] = "";

    static int old_sym = '\0';
    static IDX old_i = 0;
    screen_save();

    char sym;
    if (!get_com(
            _("モンスターの文字を入力して下さい(記号 or ^A全,^Uユ,^N非ユ,^M名前):", "Enter character to be identified(^A:All,^U:Uniqs,^N:Non uniqs,^M:Name): "),
            &sym, FALSE))

    {
        screen_load();
        return FALSE;
    }

    IDX i;
    for (i = 0; ident_info[i]; ++i) {
        if (sym == ident_info[i][0])
            break;
    }

    /* XTRA HACK WHATSEARCH */
    if (sym == KTRL('A')) {
        all = TRUE;
        strcpy(buf, _("全モンスターのリスト", "Full monster list."));
    } else if (sym == KTRL('U')) {
        all = uniq = TRUE;
        strcpy(buf, _("ユニーク・モンスターのリスト", "Unique monster list."));
    } else if (sym == KTRL('N')) {
        all = norm = TRUE;
        strcpy(buf, _("ユニーク外モンスターのリスト", "Non-unique monster list."));
    } else if (sym == KTRL('M')) {
        all = TRUE;
        if (!get_string(_("名前(英語の場合小文字で可)", "Enter name:"), temp, 70)) {
            temp[0] = 0;
            screen_load();

            return FALSE;
        }

        sprintf(buf, _("名前:%sにマッチ", "Monsters with a name \"%s\""), temp);
    } else if (ident_info[i]) {
        sprintf(buf, "%c - %s.", sym, ident_info[i] + 2);
    } else {
        sprintf(buf, "%c - %s", sym, _("無効な文字", "Unknown Symbol"));
    }

    /* Display the result */
    prt(buf, 16, 10);

    /* Allocate the "who" array */
    C_MAKE(who, max_r_idx, MONRACE_IDX);

    /* Collect matching monsters */
    int n = 0;
    for (i = 1; i < max_r_idx; i++) {
        monster_race *r_ptr = &r_info[i];

        /* Empty monster */
        if (!r_ptr->name)
            continue;

        /* XTRA HACK WHATSEARCH */
        /* Require non-unique monsters if needed */
        if (norm && (r_ptr->flags1 & (RF1_UNIQUE)))
            continue;

        /* Require unique monsters if needed */
        if (uniq && !(r_ptr->flags1 & (RF1_UNIQUE)))
            continue;

        /* 名前検索 */
        if (temp[0]) {
            for (int xx = 0; temp[xx] && xx < 80; xx++) {
#ifdef JP
                if (iskanji(temp[xx])) {
                    xx++;
                    continue;
                }
#endif
                if (isupper(temp[xx]))
                    temp[xx] = (char)tolower(temp[xx]);
            }

            char temp2[80];
#ifdef JP
            strcpy(temp2, r_name + r_ptr->E_name);
#else
            strcpy(temp2, r_name + r_ptr->name);
#endif
            for (int xx = 0; temp2[xx] && xx < 80; xx++) {
                if (isupper(temp2[xx]))
                    temp2[xx] = (char)tolower(temp2[xx]);
            }

#ifdef JP
            if (angband_strstr(temp2, temp) || angband_strstr(r_name + r_ptr->name, temp))
#else
            if (angband_strstr(temp2, temp))
#endif
                who[n++] = i;
        } else if (all || (r_ptr->d_char == sym)) {
            who[n++] = i;
        }
    }

    if (n == 0) {
        C_KILL(who, max_r_idx, MONRACE_IDX);
        screen_load();

        return FALSE;
    }

    why = 2;
    char query = 'y';

    if (why) {
        ang_sort(player_ptr, who, &why, n, ang_sort_comp_hook, ang_sort_swap_hook);
    }

    if (old_sym == sym && old_i < n)
        i = old_i;
    else
        i = n - 1;

    notpicked = TRUE;
    MONRACE_IDX r_idx;
    while (notpicked) {
        r_idx = who[i];
        roff_top(r_idx);
        term_addstr(-1, TERM_WHITE, _(" ['r'思い出, ' 'で続行, ESC]", " [(r)ecall, ESC, space to continue]"));
        while (TRUE) {
            if (recall) {
                lore_do_probe(player_ptr, r_idx);
                monster_race_track(player_ptr, r_idx);
                handle_stuff(player_ptr);
                screen_roff(player_ptr, r_idx, 0x01);
                notpicked = FALSE;
                old_sym = sym;
                old_i = i;
            }

            query = inkey();
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

            continue;
        }

        if (i-- == 0) {
            i = n - 1;
            if (!expand_list)
                break;
        }
    }

    C_KILL(who, max_r_idx, MONRACE_IDX);
    screen_load();
    return !notpicked;
}
