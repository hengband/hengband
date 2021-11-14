#include "io/input-key-requester.h"
#include "cmd-io/cmd-menu-content-table.h"
#include "cmd-io/macro-util.h"
#include "core/asking-player.h" //!< @todo 相互依存している、後で何とかする.
#include "core/player-processor.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "game-option/game-play-options.h"
#include "game-option/input-options.h"
#include "game-option/map-screen-options.h"
#include "inventory/inventory-slot-types.h"
#include "io/cursor.h"
#include "io/input-key-acceptor.h"
#include "main/sound-of-music.h"
#include "save/save.h"
#include "system/floor-type-definition.h" //!< @todo 違和感、後で調査する.
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h" //!< @todo 相互依存している、後で何とかする.
#include "util/int-char-converter.h"
#include "util/quarks.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "window/display-sub-windows.h"
#include "window/main-window-util.h"
#include "world/world.h"

/*
 * Keymaps for each "mode" associated with each keypress.
 */
concptr keymap_act[KEYMAP_MODES][256];

bool use_menu;

int16_t command_cmd; /* Current "Angband Command" */
COMMAND_ARG command_arg; /*!< 各種コマンドの汎用的な引数として扱う / Gives argument of current command */
COMMAND_NUM command_rep; /*!< 各種コマンドの汎用的なリピート数として扱う / Gives repetition of current command */
DIRECTION command_dir; /*!< 各種コマンドの汎用的な方向値処理として扱う/ Gives direction of current command */
int16_t command_see; /* アイテム使用時等にリストを表示させるかどうか (ゲームオプションの他、様々なタイミングでONになったりOFFになったりする模様……) */
int16_t command_wrk; /* アイテムの使用許可状況 (ex. 装備品のみ、床上もOK等) */
TERM_LEN command_gap = 999; /* アイテムの表示に使う (詳細未調査) */
int16_t command_new; /* Command chaining from inven/equip view */

/*
 * Hack -- special buffer to hold the action of the current keymap
 */
static char request_command_buffer[256];

static char inkey_from_menu(PlayerType *player_ptr)
{
    char cmd;
    int basey, basex;
    int num = 0, max_num, old_num = 0;
    int menu = 0;
    bool kisuu;

    if (player_ptr->y - panel_row_min > 10)
        basey = 2;
    else
        basey = 13;
    basex = 15;

    prt("", 0, 0);
    screen_save();

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    while (true) {
        int i;
        char sub_cmd;
        concptr menu_name;
        if (!menu)
            old_num = num;
        put_str("+----------------------------------------------------+", basey, basex);
        put_str("|                                                    |", basey + 1, basex);
        put_str("|                                                    |", basey + 2, basex);
        put_str("|                                                    |", basey + 3, basex);
        put_str("|                                                    |", basey + 4, basex);
        put_str("|                                                    |", basey + 5, basex);
        put_str("+----------------------------------------------------+", basey + 6, basex);

        for (i = 0; i < 10; i++) {
            int hoge;
            if (!menu_info[menu][i].cmd)
                break;
            menu_name = menu_info[menu][i].name;
            for (hoge = 0;; hoge++) {
                if (!special_menu_info[hoge].name[0])
                    break;
                if ((menu != special_menu_info[hoge].window) || (i != special_menu_info[hoge].number))
                    continue;
                switch (special_menu_info[hoge].jouken) {
                case MENU_CLASS:
                    if (player_ptr->pclass == special_menu_info[hoge].jouken_naiyou)
                        menu_name = special_menu_info[hoge].name;
                    break;
                case MENU_WILD:
                    if (!floor_ptr->dun_level && !floor_ptr->inside_arena && !floor_ptr->inside_quest) {
                        auto can_do_in_wilderness = enum2i(special_menu_info[hoge].jouken_naiyou) > 0;
                        if (player_ptr->wild_mode == can_do_in_wilderness) {
                            menu_name = special_menu_info[hoge].name;
                        }
                    }
                    break;
                default:
                    break;
                }
            }

            put_str(menu_name, basey + 1 + i / 2, basex + 4 + (i % 2) * 24);
        }

        max_num = i;
        kisuu = (max_num % 2) == 1;
        put_str(_("》", "> "), basey + 1 + num / 2, basex + 2 + (num % 2) * 24);

        move_cursor_relative(player_ptr->y, player_ptr->x);
        sub_cmd = inkey();
        if ((sub_cmd == ' ') || (sub_cmd == 'x') || (sub_cmd == 'X') || (sub_cmd == '\r') || (sub_cmd == '\n')) {
            if (menu_info[menu][num].fin) {
                cmd = menu_info[menu][num].cmd;
                use_menu = true;
                break;
            } else {
                menu = menu_info[menu][num].cmd;
                num = 0;
                basey += 2;
                basex += 8;
            }
        } else if ((sub_cmd == ESCAPE) || (sub_cmd == 'z') || (sub_cmd == 'Z') || (sub_cmd == '0')) {
            if (!menu) {
                cmd = ESCAPE;
                break;
            } else {
                menu = 0;
                num = old_num;
                basey -= 2;
                basex -= 8;
                screen_load();
                screen_save();
            }
        } else if ((sub_cmd == '2') || (sub_cmd == 'j') || (sub_cmd == 'J')) {
            if (kisuu) {
                if (num % 2)
                    num = (num + 2) % (max_num - 1);
                else
                    num = (num + 2) % (max_num + 1);
            } else
                num = (num + 2) % max_num;
        } else if ((sub_cmd == '8') || (sub_cmd == 'k') || (sub_cmd == 'K')) {
            if (kisuu) {
                if (num % 2)
                    num = (num + max_num - 3) % (max_num - 1);
                else
                    num = (num + max_num - 1) % (max_num + 1);
            } else
                num = (num + max_num - 2) % max_num;
        } else if ((sub_cmd == '4') || (sub_cmd == '6') || (sub_cmd == 'h') || (sub_cmd == 'H') || (sub_cmd == 'l') || (sub_cmd == 'L')) {
            if ((num % 2) || (num == max_num - 1)) {
                num--;
            } else if (num < max_num - 1) {
                num++;
            }
        }
    }

    screen_load();
    if (!inkey_next)
        inkey_next = "";

    return (cmd);
}

/*
 * Request a command from the user.
 *
 * Sets player_ptr->command_cmd, player_ptr->command_dir, player_ptr->command_rep,
 * player_ptr->command_arg.  May modify player_ptr->command_new.
 *
 * Note that "caret" ("^") is treated specially, and is used to
 * allow manual input of control characters.  This can be used
 * on many machines to request repeated tunneling (Ctrl-H) and
 * on the Macintosh to request "Control-Caret".
 *
 * Note that "backslash" is treated specially, and is used to bypass any
 * keymap entry for the following character.  This is useful for macros.
 *
 * Note that this command is used both in the dungeon and in
 * stores, and must be careful to work in both situations.
 *
 * Note that "player_ptr->command_new" may not work any more.
 */
void request_command(PlayerType *player_ptr, int shopping)
{
    int16_t cmd;
    int mode;

    concptr act;

#ifdef JP
    int caretcmd = 0;
#endif
    if (rogue_like_commands) {
        mode = KEYMAP_MODE_ROGUE;
    } else {
        mode = KEYMAP_MODE_ORIG;
    }

    command_cmd = 0;
    command_arg = 0;
    command_dir = 0;
    use_menu = false;

    while (true) {
        if (!macro_running() && !command_new && auto_debug_save && (!inkey_next || *inkey_next == '\0')) {
            save_player(player_ptr, SAVE_TYPE_DEBUG);
        }
        if (fresh_once && macro_running()) {
            stop_term_fresh();
        }

        if (command_new) {
            msg_erase();
            cmd = command_new;
            command_new = 0;
        } else {
            msg_flag = false;
            num_more = 0;
            inkey_flag = true;
            term_fresh();
            cmd = inkey(true);
            if (!shopping && command_menu && ((cmd == '\r') || (cmd == '\n') || (cmd == 'x') || (cmd == 'X')) && !keymap_act[mode][(byte)(cmd)])
                cmd = inkey_from_menu(player_ptr);
        }

        prt("", 0, 0);
        if (cmd == '0') {
            COMMAND_ARG old_arg = command_arg;
            command_arg = 0;
            prt(_("回数: ", "Count: "), 0, 0);
            while (true) {
                cmd = inkey();
                if ((cmd == 0x7F) || (cmd == KTRL('H'))) {
                    command_arg = command_arg / 10;
                    prt(format(_("回数: %d", "Count: %d"), command_arg), 0, 0);
                } else if (cmd >= '0' && cmd <= '9') {
                    if (command_arg >= 1000) {
                        bell();
                        command_arg = 9999;
                    } else {
                        command_arg = command_arg * 10 + D2I(cmd);
                    }

                    prt(format(_("回数: %d", "Count: %d"), command_arg), 0, 0);
                } else {
                    break;
                }
            }

            if (command_arg == 0) {
                command_arg = 99;
                prt(format(_("回数: %d", "Count: %d"), command_arg), 0, 0);
            }

            if (old_arg != 0) {
                command_arg = old_arg;
                prt(format(_("回数: %d", "Count: %d"), command_arg), 0, 0);
            }

            if ((cmd == ' ') || (cmd == '\n') || (cmd == '\r')) {
                if (!get_com(_("コマンド: ", "Command: "), (char *)&cmd, false)) {
                    command_arg = 0;
                    continue;
                }
            }
        }

        if (cmd == '\\') {
            (void)get_com(_("コマンド: ", "Command: "), (char *)&cmd, false);
            if (!inkey_next)
                inkey_next = "";
        }

        if (cmd == '^') {
            if (get_com(_("CTRL: ", "Control: "), (char *)&cmd, false))
                cmd = KTRL(cmd);
        }

        act = keymap_act[mode][(byte)(cmd)];
        if (act && !inkey_next) {
            (void)strnfmt(request_command_buffer, 256, "%s", act);
            inkey_next = request_command_buffer;
            continue;
        }

        if (!cmd)
            continue;

        command_cmd = (byte)cmd;
        break;
    }

    if (always_repeat && (command_arg <= 0)) {
        if (angband_strchr("TBDoc+", (char)command_cmd)) {
            command_arg = 99;
        }
    }

    if (shopping == 1) {
        switch (command_cmd) {
        case 'p':
            command_cmd = 'g';
            break;

        case 'm':
            command_cmd = 'g';
            break;

        case 's':
            command_cmd = 'd';
            break;
        }
    }

#ifdef JP
    for (int i = 0; i < 256; i++) {
        concptr s;
        if ((s = keymap_act[mode][i]) != nullptr) {
            if (*s == command_cmd && *(s + 1) == 0) {
                caretcmd = i;
                break;
            }
        }
    }

    if (!caretcmd)
        caretcmd = command_cmd;
#endif

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        object_type *o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        if (!o_ptr->inscription)
            continue;

        concptr s = quark_str(o_ptr->inscription);
        s = angband_strchr(s, '^');
        while (s) {
#ifdef JP
            if ((s[1] == caretcmd) || (s[1] == '*'))
#else
            if ((s[1] == command_cmd) || (s[1] == '*'))
#endif
            {
                if (!get_check(_("本当ですか? ", "Are you sure? "))) {
                    command_cmd = ' ';
                }
            }

            s = angband_strchr(s + 1, '^');
        }
    }

    prt("", 0, 0);
}
