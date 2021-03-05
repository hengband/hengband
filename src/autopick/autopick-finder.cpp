﻿/*!
 * todo get_string_for_search() は長い、要分割
 * @brief 自動拾いの検索
 * @date 2020/04/26
 * @author Hourier
 */

#include "autopick/autopick-finder.h"
#include "autopick/autopick-dirty-flags.h"
#include "autopick/autopick-entry.h"
#include "autopick/autopick-matcher.h"
#include "core/show-file.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "io/input-key-acceptor.h"
#include "main/sound-of-music.h"
#include "object/item-use-flags.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"

/*
 * @brief 与えられたアイテムが自動拾いのリストに登録されているかどうかを検索する
 * @param player_ptr プレーヤーへの参照ポインタ
 * @o_ptr アイテムへの参照ポインタ
 * @return 自動拾いのリストに登録されていたらその登録番号、なかったら-1
 * @details
 * A function for Auto-picker/destroyer
 * Examine whether the object matches to the list of keywords or not.
 */
int find_autopick_list(player_type *player_ptr, object_type *o_ptr)
{
    GAME_TEXT o_name[MAX_NLEN];
    if (o_ptr->tval == TV_GOLD)
        return -1;

    describe_flavor(player_ptr, o_name, o_ptr, (OD_NO_FLAVOR | OD_OMIT_PREFIX | OD_NO_PLURAL));
    str_tolower(o_name);
    for (int i = 0; i < max_autopick; i++) {
        autopick_type *entry = &autopick_list[i];
        if (is_autopick_match(player_ptr, o_ptr, entry, o_name))
            return i;
    }

    return -1;
}

/*
 * Choose an item for search
 */
bool get_object_for_search(player_type *player_ptr, object_type **o_handle, concptr *search_strp)
{
    concptr q = _("どのアイテムを検索しますか? ", "Enter which item? ");
    concptr s = _("アイテムを持っていない。", "You have nothing to enter.");
    object_type *o_ptr;
    o_ptr = choose_object(player_ptr, NULL, q, s, USE_INVEN | USE_FLOOR | USE_EQUIP, TV_NONE);
    if (!o_ptr)
        return FALSE;

    *o_handle = o_ptr;
    string_free(*search_strp);
    char buf[MAX_NLEN + 20];
    describe_flavor(player_ptr, buf, *o_handle, (OD_NO_FLAVOR | OD_OMIT_PREFIX | OD_NO_PLURAL));
    *search_strp = string_make(format("<%s>", buf));
    return TRUE;
}

/*
 * Prepare for search by destroyed object
 */
bool get_destroyed_object_for_search(player_type *player_ptr, object_type **o_handle, concptr *search_strp)
{
    if (!autopick_last_destroyed_object.k_idx)
        return FALSE;

    *o_handle = &autopick_last_destroyed_object;
    string_free(*search_strp);
    char buf[MAX_NLEN + 20];
    describe_flavor(player_ptr, buf, *o_handle, (OD_NO_FLAVOR | OD_OMIT_PREFIX | OD_NO_PLURAL));
    *search_strp = string_make(format("<%s>", buf));
    return TRUE;
}

/*
 * Choose an item or string for search
 */
byte get_string_for_search(player_type *player_ptr, object_type **o_handle, concptr *search_strp)
{
    /*
     * Text color
     * TERM_YELLOW : Overwrite mode
     * TERM_WHITE : Insert mode
     */
    byte color = TERM_YELLOW;
    char buf[MAX_NLEN + 20];
    const int len = 80;
    char prompt[] = _("検索(^I:持ち物 ^L:破壊された物): ", "Search key(^I:inven ^L:destroyed): ");
    int col = sizeof(prompt) - 1;
    if (*search_strp)
        strcpy(buf, *search_strp);
    else
        buf[0] = '\0';

    if (*o_handle)
        color = TERM_L_GREEN;

    prt(prompt, 0, 0);
    int pos = 0;
    while (TRUE) {
        bool back = FALSE;
        term_erase(col, 0, 255);
        term_putstr(col, 0, -1, color, buf);
        term_gotoxy(col + pos, 0);

        int skey = inkey_special(TRUE);
        switch (skey) {
        case SKEY_LEFT:
        case KTRL('b'): {
            int i = 0;
            color = TERM_WHITE;
            if (pos == 0)
                break;

            while (TRUE) {
                int next_pos = i + 1;

#ifdef JP
                if (iskanji(buf[i]))
                    next_pos++;
#endif
                if (next_pos >= pos)
                    break;

                i = next_pos;
            }

            pos = i;
            break;
        }

        case SKEY_RIGHT:
        case KTRL('f'):
            color = TERM_WHITE;
            if ('\0' == buf[pos])
                break;

#ifdef JP
            if (iskanji(buf[pos]))
                pos += 2;
            else
                pos++;
#else
            pos++;
#endif
            break;

        case ESCAPE:
            return 0;

        case KTRL('r'):
            back = TRUE;
            /* Fall through */

        case '\n':
        case '\r':
        case KTRL('s'):
            if (*o_handle)
                return (back ? -1 : 1);
            string_free(*search_strp);
            *search_strp = string_make(buf);
            *o_handle = NULL;
            return (back ? -1 : 1);

        case KTRL('i'):
            return get_object_for_search(player_ptr, o_handle, search_strp);

        case KTRL('l'):
            if (get_destroyed_object_for_search(player_ptr, o_handle, search_strp))
                return 1;
            break;

        case '\010': {
            int i = 0;
            color = TERM_WHITE;
            if (pos == 0)
                break;

            while (TRUE) {
                int next_pos = i + 1;
#ifdef JP
                if (iskanji(buf[i]))
                    next_pos++;
#endif
                if (next_pos >= pos)
                    break;

                i = next_pos;
            }

            pos = i;
        }
            /* Fall through */

        case 0x7F:
        case KTRL('d'): {
            int dst, src;
            color = TERM_WHITE;
            if (buf[pos] == '\0')
                break;

            src = pos + 1;
#ifdef JP
            if (iskanji(buf[pos]))
                src++;
#endif
            dst = pos;
            while ('\0' != (buf[dst++] = buf[src++]))
                ;

            break;
        }

        default: {
            char tmp[100];
            char c;
            if (skey & SKEY_MASK)
                break;

            c = (char)skey;
            if (color != TERM_WHITE) {
                if (color == TERM_L_GREEN) {
                    *o_handle = NULL;
                    string_free(*search_strp);
                    *search_strp = NULL;
                }

                buf[0] = '\0';
                color = TERM_WHITE;
            }

            strcpy(tmp, buf + pos);
#ifdef JP
            if (iskanji(c)) {
                char next;
                inkey_base = TRUE;
                next = inkey();

                if (pos + 1 < len) {
                    buf[pos++] = c;
                    buf[pos++] = next;
                } else {
                    bell();
                }
            } else
#endif
#ifdef JP
                if (pos < len && (isprint(c) || iskana(c)))
#else
            if (pos < len && isprint(c))
#endif
            {
                buf[pos++] = c;
            } else {
                bell();
            }

            buf[pos] = '\0';
            angband_strcat(buf, tmp, len + 1);

            break;
        }
        }

        if (*o_handle == NULL || color == TERM_L_GREEN)
            continue;

        *o_handle = NULL;
        buf[0] = '\0';
        string_free(*search_strp);
        *search_strp = NULL;
    }
}

/*
 * Search next line matches for o_ptr
 */
void search_for_object(player_type *player_ptr, text_body_type *tb, object_type *o_ptr, bool forward)
{
    autopick_type an_entry, *entry = &an_entry;
    GAME_TEXT o_name[MAX_NLEN];
    int bypassed_cy = -1;
    int i = tb->cy;
    describe_flavor(player_ptr, o_name, o_ptr, (OD_NO_FLAVOR | OD_OMIT_PREFIX | OD_NO_PLURAL));
    str_tolower(o_name);

    while (TRUE) {
        bool match;
        if (forward) {
            if (!tb->lines_list[++i])
                break;
        } else {
            if (--i < 0)
                break;
        }

        if (!autopick_new_entry(entry, tb->lines_list[i], FALSE))
            continue;

        match = is_autopick_match(player_ptr, o_ptr, entry, o_name);
        autopick_free_entry(entry);
        if (!match)
            continue;

        if (tb->states[i] & LSTAT_BYPASS) {
            if (bypassed_cy == -1)
                bypassed_cy = i;
            continue;
        }

        tb->cx = 0;
        tb->cy = i;
        if (bypassed_cy != -1) {
            tb->dirty_flags |= DIRTY_SKIP_INACTIVE;
        }

        return;
    }

    if (bypassed_cy == -1) {
        tb->dirty_flags |= DIRTY_NOT_FOUND;
        return;
    }

    tb->cx = 0;
    tb->cy = bypassed_cy;
    tb->dirty_flags |= DIRTY_INACTIVE;
}

/*
 * Search next line matches to the string
 */
void search_for_string(text_body_type *tb, concptr search_str, bool forward)
{
    int bypassed_cy = -1;
    int bypassed_cx = 0;

    int i = tb->cy;
    while (TRUE) {
        concptr pos;
        if (forward) {
            if (!tb->lines_list[++i])
                break;
        } else {
            if (--i < 0)
                break;
        }

        pos = angband_strstr(tb->lines_list[i], search_str);
        if (!pos)
            continue;

        if ((tb->states[i] & LSTAT_BYPASS) && !(tb->states[i] & LSTAT_EXPRESSION)) {
            if (bypassed_cy == -1) {
                bypassed_cy = i;
                bypassed_cx = (int)(pos - tb->lines_list[i]);
            }

            continue;
        }

        tb->cx = (int)(pos - tb->lines_list[i]);
        tb->cy = i;

        if (bypassed_cy != -1) {
            tb->dirty_flags |= DIRTY_SKIP_INACTIVE;
        }

        return;
    }

    if (bypassed_cy == -1) {
        tb->dirty_flags |= DIRTY_NOT_FOUND;
        return;
    }

    tb->cx = bypassed_cx;
    tb->cy = bypassed_cy;
    tb->dirty_flags |= DIRTY_INACTIVE;
}
