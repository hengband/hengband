/*!
 * @brief 自動拾いの検索
 * @date 2020/04/26
 * @author Hourier
 * @todo get_string_for_search() は長い、要分割
 */

#include "autopick/autopick-finder.h"
#include "autopick/autopick-dirty-flags.h"
#include "autopick/autopick-entry.h"
#include "autopick/autopick-matcher.h"
#include "autopick/autopick-util.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "io/input-key-acceptor.h"
#include "main/sound-of-music.h"
#include "object/item-use-flags.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"

/*!
 * @brief 与えられたアイテムが自動拾いのリストに登録されているかどうかを検索する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @o_ptr アイテムへの参照ポインタ
 * @return 自動拾いのリストに登録されていたらその登録番号、なかったら-1
 * @details
 * A function for Auto-picker/destroyer
 * Examine whether the object matches to the list of keywords or not.
 */
int find_autopick_list(PlayerType *player_ptr, const ItemEntity *o_ptr)
{
    if (o_ptr->bi_key.tval() == ItemKindType::GOLD) {
        return -1;
    }

    const auto item_name = str_tolower(describe_flavor(player_ptr, *o_ptr, (OD_NO_FLAVOR | OD_OMIT_PREFIX | OD_NO_PLURAL)));
    for (auto i = 0U; i < autopick_list.size(); i++) {
        const auto &entry = autopick_list[i];
        if (is_autopick_match(player_ptr, o_ptr, entry, item_name)) {
            return i;
        }
    }

    return -1;
}

/*!
 * @brief Choose an item for search
 */
bool get_object_for_search(PlayerType *player_ptr, AutopickSearch &as)
{
    constexpr auto q = _("どのアイテムを検索しますか? ", "Enter which item? ");
    constexpr auto s = _("アイテムを持っていない。", "You have nothing to enter.");
    ItemEntity *o_ptr;
    o_ptr = choose_object(player_ptr, nullptr, q, s, USE_INVEN | USE_FLOOR | USE_EQUIP);
    if (!o_ptr) {
        return false;
    }

    as.item_ptr = o_ptr;
    const auto item_name = describe_flavor(player_ptr, *as.item_ptr, (OD_NO_FLAVOR | OD_OMIT_PREFIX | OD_NO_PLURAL));
    as.search_str = format("<%s>", item_name.data());
    return true;
}

/*!
 * @brief Prepare for search by destroyed object
 */
bool get_destroyed_object_for_search(PlayerType *player_ptr, AutopickSearch &as)
{
    if (!autopick_last_destroyed_object.is_valid()) {
        return false;
    }

    as.item_ptr = &autopick_last_destroyed_object;
    const auto item_name = describe_flavor(player_ptr, *as.item_ptr, (OD_NO_FLAVOR | OD_OMIT_PREFIX | OD_NO_PLURAL));
    as.search_str = format("<%s>", item_name.data());
    return true;
}

/*!
 * @brief Choose an item or string for search
 * @details
 * Text color
 * TERM_YELLOW : Overwrite mode
 * TERM_WHITE : Insert mode
 */
AutopickSearch get_string_for_search(PlayerType *player_ptr, const AutopickSearch &as_initial)
{
    AutopickSearch as = as_initial;
    std::string buf = as.search_str;
    const int len = 80;
    uint8_t color = TERM_YELLOW;
    if (as.item_ptr != nullptr) {
        color = TERM_L_GREEN;
    }

    const std::string prompt(_("検索(^I:持ち物 ^L:破壊された物): ", "Search key(^I:inven ^L:destroyed): "));
    const auto col = prompt.length();
    prt(prompt, 0, 0);
    int pos = 0;
    while (true) {
        bool back = false;
        term_erase(col, 0);
        term_putstr(col, 0, -1, color, buf);
        term_gotoxy(col + pos, 0);

        int skey = inkey_special(true);
        switch (skey) {
        case SKEY_LEFT:
        case KTRL('b'): {
            int i = 0;
            color = TERM_WHITE;
            if (pos == 0) {
                break;
            }

            while (true) {
                int next_pos = i + 1;

#ifdef JP
                if (iskanji(buf[i])) {
                    next_pos++;
                }
#endif
                if (next_pos >= pos) {
                    break;
                }

                i = next_pos;
            }

            pos = i;
            break;
        }

        case SKEY_RIGHT:
        case KTRL('f'):
            color = TERM_WHITE;
            if ('\0' == buf[pos]) {
                break;
            }

#ifdef JP
            if (iskanji(buf[pos])) {
                pos += 2;
            } else {
                pos++;
            }
#else
            pos++;
#endif
            break;

        case ESCAPE:
            return as;

        case KTRL('r'):
            back = true;
            [[fallthrough]];

        case '\n':
        case '\r':
        case KTRL('s'): {
            as.result = back ? AutopickSearchResult::BACK : AutopickSearchResult::FORWARD;
            if (as.item_ptr != nullptr) {
                return as;
            }

            as.search_str = buf;
            as.item_ptr = nullptr;
            return as;
        }
        case KTRL('i'):
            as.result = get_object_for_search(player_ptr, as) ? AutopickSearchResult::FORWARD : AutopickSearchResult::CANCEL;
            return as;
        case KTRL('l'):
            if (get_destroyed_object_for_search(player_ptr, as)) {
                as.result = AutopickSearchResult::FORWARD;
                return as;
            }

            break;
        case '\010': {
            int i = 0;
            color = TERM_WHITE;
            if (pos == 0) {
                break;
            }

            while (true) {
                int next_pos = i + 1;
#ifdef JP
                if (iskanji(buf[i])) {
                    next_pos++;
                }
#endif
                if (next_pos >= pos) {
                    break;
                }

                i = next_pos;
            }

            pos = i;
        }
            [[fallthrough]];

        case 0x7F:
        case KTRL('d'): {
            int dst, src;
            color = TERM_WHITE;
            if (buf[pos] == '\0') {
                break;
            }

            src = pos + 1;
#ifdef JP
            if (iskanji(buf[pos])) {
                src++;
            }
#endif
            dst = pos;
            while ('\0' != (buf[dst++] = buf[src++])) {
                ;
            }

            break;
        }

        default: {
            if (skey & SKEY_MASK) {
                break;
            }

            const auto c = static_cast<char>(skey);
            if (color != TERM_WHITE) {
                if (color == TERM_L_GREEN) {
                    as.item_ptr = nullptr;
                    as.search_str = "";
                }

                buf = "";
                color = TERM_WHITE;
            }

            const auto tmp = buf.substr(pos);
#ifdef JP
            if (iskanji(c)) {
                inkey_base = true;
                const auto next = inkey();
                if (pos + 1 < len) {
                    if (static_cast<int>(buf.length()) < pos) {
                        buf[pos++] = c;
                        buf[pos++] = next;
                    } else {
                        buf.push_back(c);
                        buf.push_back(next);
                        pos += 2;
                    }
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
                buf.push_back(c);
                pos++;
            } else {
                bell();
            }

            buf += tmp.substr(0, len + 1);
            break;
        }
        }

        if (as.item_ptr == nullptr || color == TERM_L_GREEN) {
            continue;
        }

        as.item_ptr = nullptr;
        buf = "";
        as.search_str = "";
    }
}

/*!
 * @brief Search next line matches for o_ptr
 */
void search_for_object(PlayerType *player_ptr, text_body_type *tb, const ItemEntity *o_ptr, bool forward)
{
    autopick_type an_entry, *entry = &an_entry;
    int bypassed_cy = -1;
    int i = tb->cy;
    const auto item_name = str_tolower(describe_flavor(player_ptr, *o_ptr, (OD_NO_FLAVOR | OD_OMIT_PREFIX | OD_NO_PLURAL)));

    while (true) {
        bool match;
        if (forward) {
            if (!tb->lines_list[++i]) {
                break;
            }
        } else {
            if (--i < 0) {
                break;
            }
        }

        if (!autopick_new_entry(entry, *tb->lines_list[i], false)) {
            continue;
        }

        match = is_autopick_match(player_ptr, o_ptr, *entry, item_name);
        if (!match) {
            continue;
        }

        if (tb->states[i] & LSTAT_BYPASS) {
            if (bypassed_cy == -1) {
                bypassed_cy = i;
            }
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

/*!
 * @brief Search next line matches to the string
 */
void search_for_string(text_body_type *tb, std::string_view search_str, bool forward)
{
    int bypassed_cy = -1;
    int bypassed_cx = 0;

    int i = tb->cy;
    while (true) {
        if (forward) {
            if (!tb->lines_list[++i]) {
                break;
            }
        } else {
            if (--i < 0) {
                break;
            }
        }

        const auto line_data = tb->lines_list[i]->data();
        const auto *pos = angband_strstr(line_data, search_str);
        if (!pos) {
            continue;
        }

        if ((tb->states[i] & LSTAT_BYPASS) && !(tb->states[i] & LSTAT_EXPRESSION)) {
            if (bypassed_cy == -1) {
                bypassed_cy = i;
                bypassed_cx = (int)(pos - line_data);
            }

            continue;
        }

        tb->cx = (int)(pos - line_data);
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
