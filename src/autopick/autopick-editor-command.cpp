/*!
 * @brief 自動拾いエディタ画面でキーを押した時の挙動一式
 * @date 2020/04/26
 * @author Hourier
 * @todo これ単体で700行を超えているので要分割
 */

#include "autopick/autopick-editor-command.h"
#include "autopick/autopick-commands-table.h"
#include "autopick/autopick-dirty-flags.h"
#include "autopick/autopick-drawer.h"
#include "autopick/autopick-editor-util.h"
#include "autopick/autopick-entry.h"
#include "autopick/autopick-finder.h"
#include "autopick/autopick-flags-table.h"
#include "autopick/autopick-inserter-killer.h"
#include "autopick/autopick-methods-table.h"
#include "autopick/autopick-reader-writer.h"
#include "autopick/autopick-util.h"
#include "core/asking-player.h"
#include "core/show-file.h"
#include "game-option/input-options.h"
#include "game-option/keymap-directory-getter.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "system/player-type-definition.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/string-processor.h"
#include <sstream>
#include <string>
#include <string_view>

/*!
 * @brief
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param tb 自動拾いの構文
 * @param com_id エディタ内で打ったコマンド
 * @return
 * @details Execute a single editor command
 */
ape_quittance do_editor_command(PlayerType *player_ptr, text_body_type *tb, int com_id)
{
    switch (com_id) {
    case EC_QUIT: {
        if (tb->changed) {
            if (!input_check(_("全ての変更を破棄してから終了します。よろしいですか？ ", "Discard all changes and quit. Are you sure? "))) {
                break;
            }
        }

        return APE_QUIT_WITHOUT_SAVE;
    }
    case EC_SAVEQUIT:
        return APE_QUIT_AND_SAVE;
    case EC_REVERT: {
        if (!input_check(_("全ての変更を破棄して元の状態に戻します。よろしいですか？ ", "Discard all changes and revert to original file. Are you sure? "))) {
            break;
        }

        tb->lines_list = read_pickpref_text_lines(player_ptr->base_name, &tb->filename_mode);
        tb->dirty_flags |= DIRTY_ALL | DIRTY_MODE | DIRTY_EXPRESSION;
        tb->cx = tb->cy = 0;
        tb->mark = 0;
        tb->changed = false;
        break;
    }
    case EC_HELP: {
        FileDisplayer(player_ptr->name).display(true, _("jeditor.txt", "editor.txt"), 0, 0);
        tb->dirty_flags |= DIRTY_SCREEN;
        break;
    }
    case EC_RETURN: {
        if (tb->mark) {
            tb->mark = 0;
            tb->dirty_flags |= DIRTY_ALL;
        }

        if (!insert_return_code(tb)) {
            break;
        }
        tb->cy++;
        tb->cx = 0;
        tb->dirty_flags |= DIRTY_ALL;
        break;
    }
    case EC_LEFT: {
        if (0 < tb->cx) {
            tb->cx--;
            const int len = tb->lines_list[tb->cy]->length();
            if (len < tb->cx) {
                tb->cx = len;
            }
#ifdef JP
            for (auto i = 0; (*tb->lines_list[tb->cy])[i] != '\0'; i++) {
                if (iskanji((*tb->lines_list[tb->cy])[i])) {
                    i++;
                    if (i == tb->cx) {
                        tb->cx--;
                        break;
                    }
                }
            }
#endif
        } else if (tb->cy > 0) {
            tb->cy--;
            tb->cx = tb->lines_list[tb->cy]->length();
        }

        break;
    }
    case EC_DOWN: {
        if (!tb->lines_list[tb->cy + 1]) {
            if (!add_empty_line(tb)) {
                break;
            }
        }

        tb->cy++;
        break;
    }
    case EC_UP:
        if (tb->cy > 0) {
            tb->cy--;
        }
        break;
    case EC_RIGHT: {
        const int len = tb->lines_list[tb->cy]->length();
#ifdef JP
        if ((tb->cx + 1 < len) && iskanji((*tb->lines_list[tb->cy])[tb->cx])) {
            tb->cx++;
        }
#endif
        tb->cx++;

        if (len < tb->cx) {
            tb->cx = len;
            if (!tb->lines_list[tb->cy + 1]) {
                if (!add_empty_line(tb)) {
                    break;
                }
            }

            tb->cy++;
            tb->cx = 0;
        }

        break;
    }
    case EC_BOL:
        tb->cx = 0;
        break;
    case EC_EOL:
        tb->cx = tb->lines_list[tb->cy]->length();
        break;
    case EC_PGUP:
        while (0 < tb->cy && tb->upper <= tb->cy) {
            tb->cy--;
        }

        while (0 < tb->upper && tb->cy + 1 < tb->upper + tb->hgt) {
            tb->upper--;
        }

        break;
    case EC_PGDOWN:
        while (tb->cy < tb->upper + tb->hgt) {
            if (!tb->lines_list[tb->cy + 1]) {
                if (!add_empty_line(tb)) {
                    break;
                }
            }

            tb->cy++;
        }

        tb->upper = tb->cy;
        break;
    case EC_TOP:
        tb->cy = 0;
        break;
    case EC_BOTTOM:
        while (true) {
            if (!tb->lines_list[tb->cy + 1]) {
                if (!add_empty_line(tb)) {
                    break;
                }
            }

            tb->cy++;
        }

        tb->cx = 0;
        break;
    case EC_CUT: {
        copy_text_to_yank(tb);
        if (tb->my == tb->cy) {
            const auto bx1 = std::min(tb->mx, tb->cx);
            auto bx2 = std::max(tb->mx, tb->cx);
            const int len = tb->lines_list[tb->cy]->length();
            if (bx2 > len) {
                bx2 = len;
            }

            kill_line_segment(tb, tb->cy, bx1, bx2, true);
            tb->cx = bx1;
        } else {
            const auto by1 = std::min(tb->my, tb->cy);
            const auto by2 = std::max(tb->my, tb->cy);
            for (auto y = by2; y >= by1; y--) {
                const int len = tb->lines_list[y]->length();
                kill_line_segment(tb, y, 0, len, true);
            }

            tb->cy = by1;
            tb->cx = 0;
        }

        tb->mark = 0;
        tb->dirty_flags |= DIRTY_ALL;
        tb->changed = true;
        break;
    }
    case EC_COPY: {
        copy_text_to_yank(tb);

        /*
         * Move cursor position to the end of the selection
         *
         * Pressing ^C ^V correctly duplicates the selection.
         */
        if (tb->my != tb->cy) {
            tb->cy = std::max(tb->cy, tb->my);
            if (!tb->lines_list[tb->cy + 1]) {
                if (!add_empty_line(tb)) {
                    break;
                }
            }

            tb->cy++;
            break;
        }

        tb->cx = std::max(tb->cx, tb->mx);
        if ((*tb->lines_list[tb->cy])[tb->cx] == '\0') {
            if (!tb->lines_list[tb->cy + 1]) {
                if (!add_empty_line(tb)) {
                    break;
                }
            }

            tb->cy++;
            tb->cx = 0;
        }

        break;
    }
    case EC_PASTE: {
        const int len = tb->lines_list[tb->cy]->length();
        if (tb->yank.empty()) {
            break;
        }
        if (tb->cx > len) {
            tb->cx = len;
        }

        if (tb->mark) {
            tb->mark = 0;
            tb->dirty_flags |= DIRTY_ALL;
        }

        for (auto i = 0U; i < tb->yank.size(); ++i) {
            const std::string_view line(*tb->lines_list[tb->cy]);
            std::string buf(line.substr(0, tb->cx));
            buf.append(tb->yank[i], 0, MAX_LINELEN - buf.length() - 1);

            const auto is_last_line = i + 1 == tb->yank.size();
            if (!is_last_line || tb->yank_eol) {
                if (!insert_return_code(tb)) {
                    break;
                }

                tb->lines_list[tb->cy] = std::make_unique<std::string>(std::move(buf));
                tb->cx = 0;
                tb->cy++;
                continue;
            }

            const auto rest = line.substr(tb->cx);
            tb->cx = buf.length();
            buf.append(rest, 0, MAX_LINELEN - buf.length() - 1);

            tb->lines_list[tb->cy] = std::make_unique<std::string>(std::move(buf));
            break;
        }

        tb->dirty_flags |= DIRTY_ALL;
        tb->dirty_flags |= DIRTY_EXPRESSION;
        tb->changed = true;
        break;
    }
    case EC_BLOCK: {
        if (tb->mark) {
            tb->mark = 0;
            tb->dirty_flags |= DIRTY_ALL;
            break;
        }

        tb->mark = MARK_MARK;
        if (com_id == tb->old_com_id) {
            int tmp = tb->cy;
            tb->cy = tb->my;
            tb->my = tmp;
            tmp = tb->cx;
            tb->cx = tb->mx;
            tb->mx = tmp;
            tb->dirty_flags |= DIRTY_ALL;
            break;
        }

        const int len = tb->lines_list[tb->cy]->length();

        tb->my = tb->cy;
        tb->mx = tb->cx;
        if (tb->cx > len) {
            tb->mx = len;
        }
        break;
    }
    case EC_KILL_LINE: {
        const int len = tb->lines_list[tb->cy]->length();
        if (tb->cx > len) {
            tb->cx = len;
        }

        if (tb->mark) {
            tb->mark = 0;
            tb->dirty_flags |= DIRTY_ALL;
        }

        if (tb->old_com_id != com_id) {
            kill_yank_chain(tb);
        }

        if (tb->cx < len) {
            add_str_to_yank(tb, &((*tb->lines_list[tb->cy])[tb->cx]));
            kill_line_segment(tb, tb->cy, tb->cx, len, false);
            tb->dirty_line = tb->cy;
            break;
        }

        if (tb->yank_eol) {
            add_str_to_yank(tb, "");
        }

        tb->yank_eol = true;
        do_editor_command(player_ptr, tb, EC_DELETE_CHAR);
        break;
    }
    case EC_DELETE_CHAR: {
        if (tb->mark) {
            tb->mark = 0;
            tb->dirty_flags |= DIRTY_ALL;
        }

#ifdef JP
        if (iskanji((*tb->lines_list[tb->cy])[tb->cx])) {
            tb->cx++;
        }
#endif
        tb->cx++;
        const int len = tb->lines_list[tb->cy]->length();
        if (len >= tb->cx) {
            do_editor_command(player_ptr, tb, EC_BACKSPACE);
            break;
        }

        if (tb->lines_list[tb->cy + 1]) {
            tb->cy++;
            tb->cx = 0;
        } else {
            tb->cx = len;
            break;
        }

        do_editor_command(player_ptr, tb, EC_BACKSPACE);
        break;
    }
    case EC_BACKSPACE: {
        if (tb->mark) {
            tb->mark = 0;
            tb->dirty_flags |= DIRTY_ALL;
        }

        const int len = tb->lines_list[tb->cy]->length();
        if (len < tb->cx) {
            tb->cx = len;
        }

        if (tb->cx == 0) {
            if (tb->cy == 0) {
                break;
            }
            tb->cx = tb->lines_list[tb->cy - 1]->length();
            tb->lines_list[tb->cy - 1]->append(*tb->lines_list[tb->cy]);

            int i;
            for (i = tb->cy; tb->lines_list[i + 1]; i++) {
                std::swap(tb->lines_list[i], tb->lines_list[i + 1]);
            }

            tb->lines_list[i].reset();
            tb->cy--;
            tb->dirty_flags |= DIRTY_ALL;
            tb->dirty_flags |= DIRTY_EXPRESSION;
            tb->changed = true;
            break;
        }

        const auto mb_chars = str_find_all_multibyte_chars(*tb->lines_list[tb->cy]);
        const auto delete_bytes = mb_chars.contains(tb->cx - 2) ? 2 : 1;
        tb->cx -= delete_bytes;
        tb->lines_list[tb->cy]->erase(tb->cx, delete_bytes);
        tb->dirty_line = tb->cy;
        check_expression_line(tb, tb->cy);
        tb->changed = true;
        break;
    }
    case EC_SEARCH_STR: {
        tb->dirty_flags |= DIRTY_SCREEN;
        const auto as_result = get_string_for_search(player_ptr, { tb->search_o_ptr, tb->search_str });
        tb->search_o_ptr = as_result.item_ptr;
        tb->search_str = as_result.search_str;
        if (as_result.result == AutopickSearchResult::CANCEL) {
            break;
        }

        const auto command = as_result.result == AutopickSearchResult::FORWARD ? EC_SEARCH_FORW : EC_SEARCH_BACK;
        do_editor_command(player_ptr, tb, command);
        break;
    }
    case EC_SEARCH_FORW:
        if (tb->search_o_ptr) {
            search_for_object(player_ptr, tb, tb->search_o_ptr, true);
            break;
        }

        if (!tb->search_str.empty()) {
            search_for_string(tb, tb->search_str, true);
            break;
        }

        tb->dirty_flags |= DIRTY_NO_SEARCH;
        break;

    case EC_SEARCH_BACK: {
        if (tb->search_o_ptr) {
            search_for_object(player_ptr, tb, tb->search_o_ptr, false);
            break;
        }

        if (!tb->search_str.empty()) {
            search_for_string(tb, tb->search_str, false);
            break;
        }

        tb->dirty_flags |= DIRTY_NO_SEARCH;
        break;
    }
    case EC_SEARCH_OBJ: {
        tb->dirty_flags |= DIRTY_SCREEN;
        AutopickSearch as(tb->search_o_ptr, tb->search_str);
        if (!get_object_for_search(player_ptr, as)) {
            break;
        }

        tb->search_str = as.search_str;
        do_editor_command(player_ptr, tb, EC_SEARCH_FORW);
        break;
    }
    case EC_SEARCH_DESTROYED: {
        AutopickSearch as(tb->search_o_ptr, tb->search_str);
        if (!get_destroyed_object_for_search(player_ptr, as)) {
            tb->dirty_flags |= DIRTY_NO_SEARCH;
            break;
        }

        tb->search_str = as.search_str;
        do_editor_command(player_ptr, tb, EC_SEARCH_FORW);
        break;
    }
    case EC_INSERT_OBJECT: {
        autopick_type an_entry, *entry = &an_entry;
        if (!entry_from_choosed_object(player_ptr, entry)) {
            tb->dirty_flags |= DIRTY_SCREEN;
            break;
        }

        tb->cx = 0;
        if (!insert_return_code(tb)) {
            break;
        }

        tb->lines_list[tb->cy] = std::make_unique<std::string>(autopick_line_from_entry(*entry));
        tb->dirty_flags |= DIRTY_SCREEN;
        break;
    }
    case EC_INSERT_DESTROYED: {
        if (tb->last_destroyed.empty()) {
            break;
        }

        tb->cx = 0;
        if (!insert_return_code(tb)) {
            break;
        }

        tb->lines_list[tb->cy] = std::make_unique<std::string>(tb->last_destroyed);
        tb->dirty_flags |= DIRTY_ALL;
        tb->changed = true;
        break;
    }
    case EC_INSERT_BLOCK: {
        if (!can_insert_line(tb, 2)) {
            break;
        }
        const auto expression = format("?:[AND [EQU $RACE %s] [EQU $CLASS %s] [GEQ $LEVEL %02d]]",
            rp_ptr->title.en_string().data(), cp_ptr->title.en_string().data(),
            player_ptr->lev);
        tb->cx = 0;
        insert_return_code(tb);
        tb->lines_list[tb->cy] = std::make_unique<std::string>(expression);
        tb->cy++;
        insert_return_code(tb);
        tb->lines_list[tb->cy] = std::make_unique<std::string>("?:1");
        tb->dirty_flags |= DIRTY_ALL;
        tb->changed = true;
        break;
    }
    case EC_INSERT_MACRO: {
        draw_text_editor(player_ptr, tb);
        term_erase(0, tb->cy - tb->upper + 1, tb->wid);
        term_putstr(0, tb->cy - tb->upper + 1, tb->wid - 1, TERM_YELLOW, _("P:<トリガーキー>: ", "P:<Trigger key>: "));
        if (!insert_macro_line(tb)) {
            break;
        }

        tb->cx = 2;
        tb->dirty_flags |= DIRTY_ALL;
        tb->changed = true;
        break;
    }
    case EC_INSERT_KEYMAP: {
        draw_text_editor(player_ptr, tb);
        term_erase(0, tb->cy - tb->upper + 1, tb->wid);
        term_putstr(0, tb->cy - tb->upper + 1, tb->wid - 1, TERM_YELLOW,
            format(_("C:%d:<コマンドキー>: ", "C:%d:<Keypress>: "), (rogue_like_commands ? KEYMAP_MODE_ROGUE : KEYMAP_MODE_ORIG)));

        if (!insert_keymap_line(tb)) {
            break;
        }

        tb->cx = 2;
        tb->dirty_flags |= DIRTY_ALL;
        tb->changed = true;
        break;
    }
    case EC_CL_AUTOPICK:
        toggle_command_letter(tb, DO_AUTOPICK);
        break;
    case EC_CL_DESTROY:
        toggle_command_letter(tb, DO_AUTODESTROY);
        break;
    case EC_CL_LEAVE:
        toggle_command_letter(tb, DONT_AUTOPICK);
        break;
    case EC_CL_QUERY:
        toggle_command_letter(tb, DO_QUERY_AUTOPICK);
        break;
    case EC_CL_NO_DISP:
        toggle_command_letter(tb, DO_DISPLAY);
        break;
    case EC_IK_UNAWARE:
        toggle_keyword(tb, FLG_UNAWARE);
        break;
    case EC_IK_UNIDENTIFIED:
        toggle_keyword(tb, FLG_UNIDENTIFIED);
        break;
    case EC_IK_IDENTIFIED:
        toggle_keyword(tb, FLG_IDENTIFIED);
        break;
    case EC_IK_STAR_IDENTIFIED:
        toggle_keyword(tb, FLG_STAR_IDENTIFIED);
        break;
    case EC_KK_WEAPONS:
        toggle_keyword(tb, FLG_WEAPONS);
        break;
    case EC_KK_FAVORITE_WEAPONS:
        toggle_keyword(tb, FLG_FAVORITE_WEAPONS);
        break;
    case EC_KK_ARMORS:
        toggle_keyword(tb, FLG_ARMORS);
        break;
    case EC_KK_MISSILES:
        toggle_keyword(tb, FLG_MISSILES);
        break;
    case EC_KK_DEVICES:
        toggle_keyword(tb, FLG_DEVICES);
        break;
    case EC_KK_LIGHTS:
        toggle_keyword(tb, FLG_LIGHTS);
        break;
    case EC_KK_JUNKS:
        toggle_keyword(tb, FLG_JUNKS);
        break;
    case EC_KK_CORPSES:
        toggle_keyword(tb, FLG_CORPSES);
        break;
    case EC_KK_SPELLBOOKS:
        toggle_keyword(tb, FLG_SPELLBOOKS);
        break;
    case EC_KK_SHIELDS:
        toggle_keyword(tb, FLG_SHIELDS);
        break;
    case EC_KK_BOWS:
        toggle_keyword(tb, FLG_BOWS);
        break;
    case EC_KK_RINGS:
        toggle_keyword(tb, FLG_RINGS);
        break;
    case EC_KK_AMULETS:
        toggle_keyword(tb, FLG_AMULETS);
        break;
    case EC_KK_SUITS:
        toggle_keyword(tb, FLG_SUITS);
        break;
    case EC_KK_CLOAKS:
        toggle_keyword(tb, FLG_CLOAKS);
        break;
    case EC_KK_HELMS:
        toggle_keyword(tb, FLG_HELMS);
        break;
    case EC_KK_GLOVES:
        toggle_keyword(tb, FLG_GLOVES);
        break;
    case EC_KK_BOOTS:
        toggle_keyword(tb, FLG_BOOTS);
        break;
    case EC_OK_COLLECTING:
        toggle_keyword(tb, FLG_COLLECTING);
        break;
    case EC_OK_BOOSTED:
        toggle_keyword(tb, FLG_BOOSTED);
        break;
    case EC_OK_MORE_DICE:
        toggle_keyword(tb, FLG_MORE_DICE);
        break;
    case EC_OK_MORE_BONUS:
        toggle_keyword(tb, FLG_MORE_BONUS);
        break;
    case EC_OK_WORTHLESS:
        toggle_keyword(tb, FLG_WORTHLESS);
        break;
    case EC_OK_ARTIFACT:
        toggle_keyword(tb, FLG_ARTIFACT);
        break;
    case EC_OK_EGO:
        toggle_keyword(tb, FLG_EGO);
        break;
    case EC_OK_GOOD:
        toggle_keyword(tb, FLG_GOOD);
        break;
    case EC_OK_NAMELESS:
        toggle_keyword(tb, FLG_NAMELESS);
        break;
    case EC_OK_AVERAGE:
        toggle_keyword(tb, FLG_AVERAGE);
        break;
    case EC_OK_RARE:
        toggle_keyword(tb, FLG_RARE);
        break;
    case EC_OK_COMMON:
        toggle_keyword(tb, FLG_COMMON);
        break;
    case EC_OK_WANTED:
        toggle_keyword(tb, FLG_WANTED);
        break;
    case EC_OK_UNIQUE:
        toggle_keyword(tb, FLG_UNIQUE);
        break;
    case EC_OK_HUMAN:
        toggle_keyword(tb, FLG_HUMAN);
        break;
    case EC_OK_UNREADABLE:
        toggle_keyword(tb, FLG_UNREADABLE);
        add_keyword(tb, FLG_SPELLBOOKS);
        break;
    case EC_OK_REALM1:
        toggle_keyword(tb, FLG_REALM1);
        add_keyword(tb, FLG_SPELLBOOKS);
        break;
    case EC_OK_REALM2:
        toggle_keyword(tb, FLG_REALM2);
        add_keyword(tb, FLG_SPELLBOOKS);
        break;
    case EC_OK_FIRST:
        toggle_keyword(tb, FLG_FIRST);
        add_keyword(tb, FLG_SPELLBOOKS);
        break;
    case EC_OK_SECOND:
        toggle_keyword(tb, FLG_SECOND);
        add_keyword(tb, FLG_SPELLBOOKS);
        break;
    case EC_OK_THIRD:
        toggle_keyword(tb, FLG_THIRD);
        add_keyword(tb, FLG_SPELLBOOKS);
        break;
    case EC_OK_FOURTH:
        toggle_keyword(tb, FLG_FOURTH);
        add_keyword(tb, FLG_SPELLBOOKS);
        break;
    }

    tb->old_com_id = com_id;
    return APE_QUIT;
}
