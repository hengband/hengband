#include "cmd-io/cmd-autopick.h"
#include "autopick/autopick-command-menu.h"
#include "autopick/autopick-commands-table.h"
#include "autopick/autopick-dirty-flags.h"
#include "autopick/autopick-drawer.h"
#include "autopick/autopick-editor-command.h"
#include "autopick/autopick-editor-util.h"
#include "autopick/autopick-entry.h"
#include "autopick/autopick-initializer.h"
#include "autopick/autopick-inserter-killer.h"
#include "autopick/autopick-pref-processor.h"
#include "autopick/autopick-reader-writer.h"
#include "autopick/autopick-util.h"
#include "cmd-io/cmd-save.h"
#include "io/input-key-acceptor.h"
#include "io/read-pref-file.h"
#include "system/item-entity.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "world/world.h"

/*
 * Check special key code and get a movement command id
 */
static int analyze_move_key(text_body_type *tb, int skey)
{
    int com_id;
    if (!(skey & SKEY_MASK)) {
        return 0;
    }

    switch (skey & ~SKEY_MOD_MASK) {
    case SKEY_DOWN:
        com_id = EC_DOWN;
        break;
    case SKEY_LEFT:
        com_id = EC_LEFT;
        break;
    case SKEY_RIGHT:
        com_id = EC_RIGHT;
        break;
    case SKEY_UP:
        com_id = EC_UP;
        break;
    case SKEY_PGUP:
        com_id = EC_PGUP;
        break;
    case SKEY_PGDOWN:
        com_id = EC_PGDOWN;
        break;
    case SKEY_TOP:
        com_id = EC_TOP;
        break;
    case SKEY_BOTTOM:
        com_id = EC_BOTTOM;
        break;
    default:
        return 0;
    }

    if (!(skey & SKEY_MOD_SHIFT)) {
        /*
         * Un-shifted cursor keys cancells
         * selection created by shift+cursor.
         */
        if (tb->mark & MARK_BY_SHIFT) {
            tb->mark = 0;
            tb->dirty_flags |= DIRTY_ALL;
        }

        return com_id;
    }

    if (tb->mark) {
        return com_id;
    }

    int len = strlen(tb->lines_list[tb->cy]);
    tb->mark = MARK_MARK | MARK_BY_SHIFT;
    tb->my = tb->cy;
    tb->mx = tb->cx;
    if (tb->cx > len) {
        tb->mx = len;
    }

    if (com_id == EC_UP || com_id == EC_DOWN) {
        tb->dirty_flags |= DIRTY_ALL;
    } else {
        tb->dirty_line = tb->cy;
    }

    return com_id;
}

/*
 * In-game editor of Object Auto-picker/Destoryer
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_edit_autopick(PlayerType *player_ptr)
{
    static int cx_save = 0;
    static int cy_save = 0;
    autopick_type an_entry, *entry = &an_entry;
    int i;
    int key = -1;
    static int32_t old_autosave_turn = 0L;
    ape_quittance quit = APE_QUIT;

    text_body_type text_body;
    text_body_type *tb = &text_body;
    tb->changed = false;
    tb->cx = cx_save;
    tb->cy = cy_save;
    tb->upper = tb->left = 0;
    tb->mark = 0;
    tb->mx = tb->my = 0;
    tb->old_cy = tb->old_upper = tb->old_left = -1;
    tb->old_wid = tb->old_hgt = -1;
    tb->old_com_id = 0;

    tb->yank = nullptr;
    tb->search_o_ptr = nullptr;
    tb->search_str = nullptr;
    tb->last_destroyed = nullptr;
    tb->dirty_flags = DIRTY_ALL | DIRTY_MODE | DIRTY_EXPRESSION;
    tb->dirty_line = -1;
    tb->filename_mode = PT_DEFAULT;

    if (w_ptr->game_turn < old_autosave_turn) {
        while (old_autosave_turn > w_ptr->game_turn) {
            old_autosave_turn -= TURNS_PER_TICK * TOWN_DAWN;
        }
    }

    if (w_ptr->game_turn > old_autosave_turn + 100L) {
        do_cmd_save_game(player_ptr, true);
        old_autosave_turn = w_ptr->game_turn;
    }

    update_playtime();
    init_autopick();
    if (autopick_last_destroyed_object.bi_id) {
        autopick_entry_from_object(player_ptr, entry, &autopick_last_destroyed_object);
        tb->last_destroyed = autopick_line_from_entry_kill(entry);
    }

    tb->lines_list = read_pickpref_text_lines(player_ptr, &tb->filename_mode);
    for (i = 0; i < tb->cy; i++) {
        if (!tb->lines_list[i]) {
            tb->cy = tb->cx = 0;
            break;
        }
    }

    TermCenteredOffsetSetter tcos(std::nullopt, std::nullopt);

    screen_save();
    while (quit == APE_QUIT) {
        int com_id = 0;
        draw_text_editor(player_ptr, tb);
        prt(_("(^Q:終了 ^W:セーブして終了, ESC:メニュー, その他:入力)",
                "(^Q:Quit, ^W:Save&Quit, ESC:Menu, Other:Input text)"),
            0, 0);
        if (!tb->mark) {
            prt(format("(%d,%d)", tb->cx, tb->cy), 0, 60);
        } else {
            prt(format("(%d,%d)-(%d,%d)", tb->mx, tb->my, tb->cx, tb->cy), 0, 60);
        }

        term_gotoxy(tb->cx - tb->left, tb->cy - tb->upper + 1);
        tb->dirty_flags = 0;
        tb->dirty_line = -1;
        tb->old_cy = tb->cy;
        tb->old_upper = tb->upper;
        tb->old_left = tb->left;
        tb->old_wid = tb->wid;
        tb->old_hgt = tb->hgt;

        key = inkey_special(true);

        if (key & SKEY_MASK) {
            com_id = analyze_move_key(tb, key);
        } else if (key == ESCAPE) {
            com_id = do_command_menu(0, 0);
            tb->dirty_flags |= DIRTY_SCREEN;
        } else if (!iscntrl((unsigned char)key)) {
            if (tb->mark) {
                tb->mark = 0;
                tb->dirty_flags |= DIRTY_ALL;
            }

            insert_single_letter(tb, key);
            continue;
        } else {
            com_id = get_com_id((char)key);
        }

        if (com_id) {
            quit = do_editor_command(player_ptr, tb, com_id);
        }
    }

    screen_load();
    const auto filename = pickpref_filename(player_ptr, tb->filename_mode);

    if (quit == APE_QUIT_AND_SAVE) {
        write_text_lines(filename.data(), tb->lines_list.data());
    }

    free_text_lines(tb->lines_list);
    string_free(tb->search_str);
    string_free(tb->last_destroyed);
    kill_yank_chain(tb);

    process_autopick_file(player_ptr, filename.data());
    w_ptr->start_time = (uint32_t)time(nullptr);
    cx_save = tb->cx;
    cy_save = tb->cy;
}
