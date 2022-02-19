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
#include "player-base/player-class.h"
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
short command_rep; /*!< 各種コマンドの汎用的なリピート数として扱う / Gives repetition of current command */
DIRECTION command_dir; /*!< 各種コマンドの汎用的な方向値処理として扱う/ Gives direction of current command */
int16_t command_see; /* アイテム使用時等にリストを表示させるかどうか (ゲームオプションの他、様々なタイミングでONになったりOFFになったりする模様……) */
int16_t command_wrk; /* アイテムの使用許可状況 (ex. 装備品のみ、床上もOK等) */
TERM_LEN command_gap = 999; /* アイテムの表示に使う (詳細未調査) */
int16_t command_new; /* Command chaining from inven/equip view */

InputKeyRequestor::InputKeyRequestor(PlayerType *player_ptr, bool shopping)
    : player_ptr(player_ptr)
    , shopping(shopping)
    , mode(rogue_like_commands ? KEYMAP_MODE_ROGUE : KEYMAP_MODE_ORIG)
{
}

/*
 * @brief プレイヤーからのコマンド入力を受け付ける
 */
void InputKeyRequestor::request_command()
{
    command_cmd = 0;
    command_arg = 0;
    command_dir = 0;
    use_menu = false;
    this->input_command();
    if (always_repeat && (command_arg <= 0)) {
        if (angband_strchr("TBDoc+", (char)command_cmd)) {
            command_arg = 99;
        }
    }

    this->change_shopping_command();
    auto caret_command = this->get_caret_command();
    for (auto i = enum2i(INVEN_MAIN_HAND); i < INVEN_TOTAL; i++) {
        auto &o_ref = this->player_ptr->inventory_list[i];
        if ((o_ref.k_idx == 0) || (o_ref.inscription == 0)) {
            continue;
        }

        this->confirm_command(o_ref, caret_command);
    }

    prt("", 0, 0);
}

void InputKeyRequestor::input_command()
{
    while (true) {
        if (!macro_running() && !command_new && auto_debug_save && (!inkey_next || *inkey_next == '\0')) {
            save_player(this->player_ptr, SAVE_TYPE_DEBUG);
        }

        if (fresh_once && macro_running()) {
            stop_term_fresh();
        }

        auto cmd = this->get_command();
        prt("", 0, 0);
        if (this->process_repeat_num(&cmd)) {
            continue;
        }

        this->process_command_command(&cmd);
        this->process_control_command(&cmd);
        auto act = keymap_act[this->mode][(byte)(cmd)];
        if (act && !inkey_next) {
            (void)strnfmt(this->request_command_buffer, sizeof(this->request_command_buffer), "%s", act);
            inkey_next = this->request_command_buffer;
            continue;
        }

        if (cmd == 0) {
            continue;
        }

        command_cmd = (byte)cmd;
        break;
    }
}

short InputKeyRequestor::get_command()
{
    if (command_new) {
        msg_erase();
        auto cmd_back = command_new;
        command_new = 0;
        return cmd_back;
    }

    msg_flag = false;
    num_more = 0;
    inkey_flag = true;
    term_fresh();
    short cmd = inkey(true);
    if (!this->shopping && command_menu && ((cmd == '\r') || (cmd == '\n') || (cmd == 'x') || (cmd == 'X')) && !keymap_act[this->mode][(byte)(cmd)]) {
        cmd = this->inkey_from_menu();
    }

    return cmd;
}

char InputKeyRequestor::inkey_from_menu()
{
    auto basey = this->player_ptr->y - panel_row_min > 10 ? 2 : 13;
    auto basex = 15;

    prt("", 0, 0);
    screen_save();

    auto *floor_ptr = this->player_ptr->current_floor_ptr;
    char command;
    auto num = 0;
    auto old_num = 0;
    auto menu_num = 0;
    while (true) {
        char sub_cmd;
        concptr menu_name;
        if (!menu_num) {
            old_num = num;
        }

        auto line = 0;
        put_str("+----------------------------------------------------+", basey + line++, basex);
        put_str("|                                                    |", basey + line++, basex);
        put_str("|                                                    |", basey + line++, basex);
        put_str("|                                                    |", basey + line++, basex);
        put_str("|                                                    |", basey + line++, basex);
        put_str("|                                                    |", basey + line++, basex);
        put_str("+----------------------------------------------------+", basey + line++, basex);

        int cmmand_per_menu_num;
        for (cmmand_per_menu_num = 0; cmmand_per_menu_num < 10; cmmand_per_menu_num++) {
            if (!menu_info[menu_num][cmmand_per_menu_num].cmd) {
                break;
            }

            menu_name = menu_info[menu_num][cmmand_per_menu_num].name;
            for (auto special_menu_num = 0;; special_menu_num++) {
                auto special_menu = special_menu_info[special_menu_num];
                if (!special_menu.name[0]) {
                    break;
                }

                if ((menu_num != special_menu.window) || (cmmand_per_menu_num != special_menu.number)) {
                    continue;
                }

                switch (special_menu.jouken) {
                case MENU_CLASS:
                    if (PlayerClass(this->player_ptr).equals(special_menu.jouken_naiyou)) {
                        menu_name = special_menu.name;
                    }

                    break;
                case MENU_WILD: {
                    if ((floor_ptr->dun_level > 0) || floor_ptr->inside_arena || inside_quest(floor_ptr->quest_number)) {
                        break;
                    }

                    auto can_do_in_wilderness = enum2i(special_menu.jouken_naiyou) > 0;
                    if (this->player_ptr->wild_mode == can_do_in_wilderness) {
                        menu_name = special_menu.name;
                    }

                    break;
                }
                default:
                    break;
                }
            }

            put_str(menu_name, basey + 1 + cmmand_per_menu_num / 2, basex + 4 + (cmmand_per_menu_num % 2) * 24);
        }

        auto max_num = cmmand_per_menu_num;
        auto is_max_num_odd = (max_num % 2) == 1;
        put_str(_("》", "> "), basey + 1 + num / 2, basex + 2 + (num % 2) * 24);

        move_cursor_relative(this->player_ptr->y, this->player_ptr->x);
        sub_cmd = inkey();
        if ((sub_cmd == ' ') || (sub_cmd == 'x') || (sub_cmd == 'X') || (sub_cmd == '\r') || (sub_cmd == '\n')) {
            if (menu_info[menu_num][num].fin) {
                command = menu_info[menu_num][num].cmd;
                use_menu = true;
                break;
            }

            menu_num = menu_info[menu_num][num].cmd;
            num = 0;
            basey += 2;
            basex += 8;
            continue;
        }

        if ((sub_cmd == ESCAPE) || (sub_cmd == 'z') || (sub_cmd == 'Z') || (sub_cmd == '0')) {
            if (!menu_num) {
                command = ESCAPE;
                break;
            }

            menu_num = 0;
            num = old_num;
            basey -= 2;
            basex -= 8;
            screen_load();
            screen_save();
            continue;
        }

        if ((sub_cmd == '2') || (sub_cmd == 'j') || (sub_cmd == 'J')) {
            if (is_max_num_odd) {
                if (num % 2) {
                    num = (num + 2) % (max_num - 1);
                } else {
                    num = (num + 2) % (max_num + 1);
                }
            } else {
                num = (num + 2) % max_num;
            }

            continue;
        }

        if ((sub_cmd == '8') || (sub_cmd == 'k') || (sub_cmd == 'K')) {
            if (is_max_num_odd) {
                if (num % 2) {
                    num = (num + max_num - 3) % (max_num - 1);
                } else {
                    num = (num + max_num - 1) % (max_num + 1);
                }
            } else {
                num = (num + max_num - 2) % max_num;
            }

            continue;
        }

        if ((sub_cmd == '4') || (sub_cmd == '6') || (sub_cmd == 'h') || (sub_cmd == 'H') || (sub_cmd == 'l') || (sub_cmd == 'L')) {
            if ((num % 2) || (num == max_num - 1)) {
                num--;
            } else if (num < max_num - 1) {
                num++;
            }

            continue;
        }
    }

    screen_load();
    if (!inkey_next) {
        inkey_next = "";
    }

    return command;
}

char InputKeyRequestor::input_repeat_num()
{
    while (true) {
        auto cmd = inkey();
        if ((cmd == 0x7F) || (cmd == KTRL('H'))) {
            command_arg = command_arg / 10;
            prt(format(_("回数: %d", "Count: %d"), command_arg), 0, 0);
            continue;
        }

        if ((cmd >= '0') && (cmd <= '9')) {
            if (command_arg >= 1000) {
                bell();
                command_arg = 9999;
            } else {
                command_arg = command_arg * 10 + D2I(cmd);
            }

            prt(format(_("回数: %d", "Count: %d"), command_arg), 0, 0);
            continue;
        }

        return cmd;
    }
}

bool InputKeyRequestor::process_repeat_num(short *cmd)
{
    if (*cmd != '0') {
        return false;
    }

    auto old_arg = command_arg;
    command_arg = 0;
    prt(_("回数: ", "Count: "), 0, 0);
    *cmd = this->input_repeat_num();
    if (command_arg == 0) {
        command_arg = 99;
        prt(format(_("回数: %d", "Count: %d"), command_arg), 0, 0);
    }

    if (old_arg != 0) {
        command_arg = old_arg;
        prt(format(_("回数: %d", "Count: %d"), command_arg), 0, 0);
    }

    if ((*cmd != ' ') && (*cmd != '\n') && (*cmd != '\r')) {
        return false;
    }

    if (get_com(_("コマンド: ", "Command: "), (char *)cmd, false)) {
        return false;
    }

    command_arg = 0;
    return true;
}

/*
 * @brief コマンドの入力を求めるコマンドの処理
 * @param cmd 入力コマンド
 * @details 全く意味がないような気もするが元のコードにあった機能は保持しておく
 */
void InputKeyRequestor::process_command_command(short *cmd)
{
    if (*cmd != '\\') {
        return;
    }

    (void)get_com(_("コマンド: ", "Command: "), (char *)cmd, false);
    if (!inkey_next) {
        inkey_next = "";
    }
}

void InputKeyRequestor::process_control_command(short *cmd)
{
    if (*cmd != '^') {
        return;
    }

    if (get_com(_("CTRL: ", "Control: "), (char *)cmd, false)) {
        *cmd = KTRL(*cmd);
    }
}

void InputKeyRequestor::change_shopping_command()
{
    if (!this->shopping) {
        return;
    }

    switch (command_cmd) {
    case 'p':
        command_cmd = 'g';
        return;
    case 'm':
        command_cmd = 'g';
        return;
    case 's':
        command_cmd = 'd';
        return;
    }
}

int InputKeyRequestor::get_caret_command()
{
#ifdef JP
    auto caret_command = 0;
    for (auto i = 0; i < 256; i++) {
        auto s = keymap_act[this->mode][i];
        if (s == nullptr) {
            continue;
        }

        if ((*s == command_cmd) && (*(s + 1) == 0)) {
            caret_command = i;
            break;
        }
    }

    if (caret_command == 0) {
        caret_command = command_cmd;
    }

    return caret_command;
#else
    return 0;
#endif
}

void InputKeyRequestor::confirm_command(ObjectType &o_ref, const int caret_command)
{
    auto s = quark_str(o_ref.inscription);
    s = angband_strchr(s, '^');
    while (s) {
#ifdef JP
        auto sure = (s[1] == caret_command) || (s[1] == '*');
#else
        auto sure = (s[1] == command_cmd) || (s[1] == '*');
        (void)caret_command;
#endif
        if (sure) {
            if (!get_check(_("本当ですか? ", "Are you sure? "))) {
                command_cmd = ' ';
            }
        }

        s = angband_strchr(s + 1, '^');
    }
}
