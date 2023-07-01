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
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h" //!< @todo 相互依存している、後で何とかする.
#include "util/int-char-converter.h"
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

static char request_command_buffer[256]{}; /*!< Special buffer to hold the action of the current keymap */

InputKeyRequestor::InputKeyRequestor(PlayerType *player_ptr, bool shopping)
    : player_ptr(player_ptr)
    , shopping(shopping)
    , mode(rogue_like_commands ? KEYMAP_MODE_ROGUE : KEYMAP_MODE_ORIG)
    , base_y(player_ptr->y - panel_row_min > 10 ? 2 : 13)
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
    this->process_input_command();
    if (always_repeat && (command_arg <= 0)) {
        if (angband_strchr("TBDoc+", (char)command_cmd)) {
            command_arg = 99;
        }
    }

    this->change_shopping_command();
    this->sweep_confirmation_equipments();
    prt("", 0, 0);
}

void InputKeyRequestor::process_input_command()
{
    while (true) {
        if (!this->shopping && !macro_running() && !command_new && auto_debug_save && (!inkey_next || *inkey_next == '\0')) {
            save_player(this->player_ptr, SaveType::DEBUG);
        }

        if (fresh_once && macro_running()) {
            stop_term_fresh();
        }

        auto cmd = this->get_command();
        prt("", 0, 0);
        if (this->process_repeat_num(cmd)) {
            continue;
        }

        this->process_command_command(cmd);
        this->process_control_command(cmd);
        auto act = keymap_act[this->mode][(byte)(cmd)];
        if (act && !inkey_next) {
            (void)strnfmt(request_command_buffer, sizeof(request_command_buffer), "%s", act);
            inkey_next = request_command_buffer;
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
    prt("", 0, 0);
    screen_save();
    auto old_num = 0;
    while (true) {
        if (this->menu_num == 0) {
            old_num = this->num;
        }

        this->make_commands_frame();
        this->max_num = this->get_command_per_menu_num();
        this->is_max_num_odd = (max_num % 2) == 1;
        put_str(_("》", "> "), this->base_y + 1 + this->num / 2, this->base_x + 2 + (this->num % 2) * 24);
        move_cursor_relative(this->player_ptr->y, this->player_ptr->x);
        this->sub_cmd = inkey();
        if ((this->sub_cmd == ' ') || (this->sub_cmd == 'x') || (this->sub_cmd == 'X') || (this->sub_cmd == '\r') || (this->sub_cmd == '\n')) {
            if (this->check_continuous_command()) {
                break;
            }

            continue;
        }

        if ((this->sub_cmd == ESCAPE) || (this->sub_cmd == 'z') || (this->sub_cmd == 'Z') || (this->sub_cmd == '0')) {
            if (this->check_escape_key(old_num)) {
                break;
            }

            continue;
        }

        if (this->process_down_cursor() || this->process_up_cursor()) {
            continue;
        }

        this->process_right_left_cursor();
    }

    screen_load();
    if (inkey_next == nullptr) {
        inkey_next = "";
    }

    return this->command;
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

bool InputKeyRequestor::process_repeat_num(short &cmd)
{
    if (cmd != '0') {
        return false;
    }

    auto old_arg = command_arg;
    command_arg = 0;
    prt(_("回数: ", "Count: "), 0, 0);
    cmd = this->input_repeat_num();
    if (command_arg == 0) {
        command_arg = 99;
        prt(format(_("回数: %d", "Count: %d"), command_arg), 0, 0);
    }

    if (old_arg != 0) {
        command_arg = old_arg;
        prt(format(_("回数: %d", "Count: %d"), command_arg), 0, 0);
    }

    if ((cmd != ' ') && (cmd != '\n') && (cmd != '\r')) {
        return false;
    }

    const auto ret_cmd = input_command(_("コマンド: ", "Command: "));
    cmd = ret_cmd.value_or(ESCAPE);
    command_arg = 0;
    return true;
}

/*
 * @brief コマンドの入力を求めるコマンドの処理
 * @param cmd 入力コマンド
 * @details 全く意味がないような気もするが元のコードにあった機能は保持しておく
 */
void InputKeyRequestor::process_command_command(short &cmd)
{
    if (cmd != '\\') {
        return;
    }

    const auto new_command = input_command(_("コマンド: ", "Command: "));
    cmd = new_command.value_or(ESCAPE);
    if (inkey_next == nullptr) {
        inkey_next = "";
    }
}

void InputKeyRequestor::process_control_command(short &cmd)
{
    if (cmd != '^') {
        return;
    }

    const auto new_command = input_command(_("CTRL: ", "Control: "));
    const auto is_input = new_command.has_value();
    cmd = new_command.value_or(ESCAPE);
    if (is_input) {
        cmd = KTRL(cmd);
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

void InputKeyRequestor::sweep_confirmation_equipments()
{
    auto caret_command = this->get_caret_command();
    for (auto i = enum2i(INVEN_MAIN_HAND); i < INVEN_TOTAL; i++) {
        auto &item = this->player_ptr->inventory_list[i];
        if (!item.is_valid() || !item.is_inscribed()) {
            continue;
        }

        this->confirm_command(item.inscription, caret_command);
    }
}

void InputKeyRequestor::confirm_command(const std::optional<std::string> &inscription, const int caret_command)
{
    if (!inscription.has_value()) {
        return;
    }

    auto s = inscription->data();
    s = angband_strchr(s, '^');
    while (s != nullptr) {
#ifdef JP
        auto sure = s[1] == caret_command;
#else
        auto sure = s[1] == command_cmd;
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

void InputKeyRequestor::make_commands_frame()
{
    auto line = 0;
    put_str("+----------------------------------------------------+", this->base_y + line++, this->base_x);
    put_str("|                                                    |", this->base_y + line++, this->base_x);
    put_str("|                                                    |", this->base_y + line++, this->base_x);
    put_str("|                                                    |", this->base_y + line++, this->base_x);
    put_str("|                                                    |", this->base_y + line++, this->base_x);
    put_str("|                                                    |", this->base_y + line++, this->base_x);
    put_str("+----------------------------------------------------+", this->base_y + line++, this->base_x);
}

std::string InputKeyRequestor::switch_special_menu_condition(const SpecialMenuContent &special_menu)
{
    switch (special_menu.menu_condition) {
    case SpecialMenuType::NONE:
        return "";
    case SpecialMenuType::CLASS:
        if (PlayerClass(this->player_ptr).equals(special_menu.class_condition.value())) {
            return std::string(special_menu.name);
        }

        return "";
    case SpecialMenuType::WILD: {
        auto floor_ptr = this->player_ptr->current_floor_ptr;
        if ((floor_ptr->dun_level > 0) || floor_ptr->inside_arena || inside_quest(floor_ptr->quest_number)) {
            return "";
        }

        if (this->player_ptr->wild_mode == special_menu.wild_mode) {
            return std::string(special_menu.name);
        }

        return "";
    }
    default:
        throw("Invalid SpecialMenuType is specified!");
    }
}

int InputKeyRequestor::get_command_per_menu_num()
{
    int command_per_menu_num;
    for (command_per_menu_num = 0; command_per_menu_num < MAX_COMMAND_PER_SCREEN; command_per_menu_num++) {
        if (menu_info[this->menu_num][command_per_menu_num].cmd == 0) {
            break;
        }

        std::string menu_name(menu_info[this->menu_num][command_per_menu_num].name);
        for (const auto &special_menu : special_menu_info) {
            if (special_menu.name[0] == '\0') {
                break;
            }

            if ((this->menu_num != special_menu.window) || (command_per_menu_num != special_menu.number)) {
                continue;
            }

            auto tmp_menu_name = this->switch_special_menu_condition(special_menu);
            if (tmp_menu_name != "") {
                menu_name = tmp_menu_name;
            }
        }

        put_str(menu_name, this->base_y + 1 + command_per_menu_num / 2, this->base_x + 4 + (command_per_menu_num % 2) * 24);
    }

    return command_per_menu_num;
}

bool InputKeyRequestor::check_continuous_command()
{
    if (menu_info[this->menu_num][this->num].fin) {
        this->command = menu_info[this->menu_num][this->num].cmd;
        use_menu = true;
        return true;
    }

    this->menu_num = menu_info[this->menu_num][this->num].cmd;
    this->num = 0;
    this->base_y += 2;
    this->base_x += 8;
    return false;
}

bool InputKeyRequestor::check_escape_key(const int old_num)
{
    if (this->menu_num == 0) {
        this->command = ESCAPE;
        return true;
    }

    this->menu_num = 0;
    this->num = old_num;
    this->base_y -= 2;
    this->base_x -= 8;
    screen_load();
    screen_save();
    return false;
}

bool InputKeyRequestor::process_down_cursor()
{
    if ((this->sub_cmd != '2') && (this->sub_cmd != 'j') && (this->sub_cmd != 'J')) {
        return false;
    }

    if (!this->is_max_num_odd) {
        this->num = (this->num + 2) % this->max_num;
        return true;
    }

    auto tmp_num = this->num % 2 ? this->max_num - 1 : this->max_num + 1;
    this->num = (this->num + 2) % tmp_num;
    return true;
}

bool InputKeyRequestor::process_up_cursor()
{
    if ((this->sub_cmd != '8') && (this->sub_cmd != 'k') && (this->sub_cmd != 'K')) {
        return false;
    }

    if (!this->is_max_num_odd) {
        this->num = (this->num + this->max_num - 2) % this->max_num;
        return true;
    }

    auto is_num_odd = (this->num % 2) != 0;
    auto tmp_num1 = is_num_odd ? (this->num + max_num - 3) : (this->num + this->max_num - 1);
    auto tmp_num2 = is_num_odd ? (this->max_num - 1) : (this->max_num + 1);
    this->num = tmp_num1 % tmp_num2;
    return true;
}

void InputKeyRequestor::process_right_left_cursor()
{
    auto orig_key_right_left = (this->sub_cmd == '4') || (this->sub_cmd == '6');
    auto rogue_key_right_left = (this->sub_cmd == 'h') || (this->sub_cmd == 'H') || (this->sub_cmd == 'l') || (this->sub_cmd == 'L');
    if (!orig_key_right_left && !rogue_key_right_left) {
        return;
    }

    if ((this->num % 2) || (this->num == max_num - 1)) {
        this->num--;
    } else if (this->num < max_num - 1) {
        this->num++;
    }
}
