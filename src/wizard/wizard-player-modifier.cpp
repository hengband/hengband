/*!
 * @brief プレイヤー属性を変更するデバッグコマンド
 * @date 2021/03/07
 */

#include "wizard/wizard-player-modifier.h"
#include "core/asking-player.h"
#include "io/input-key-requester.h"
#include "mutation/mutation-investor-remover.h"
#include "player-info/self-info.h"
#include "spell/spells-status.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "wizard/wizard-special-process.h"
#include <string>
#include <vector>
#include <sstream>

/*!
 * @brief プレイヤー設定コマンド一覧表
 */
std::vector<std::vector<std::string>> wizard_player_modifier_menu_table = {
    { "r", _("種族変更", "Change race") },
    { "c", _("職業変更", "Change class") },
    { "R", _("領域変更", "Change realms") },
    { "e", _("能力変更", "Change status") },
    { "k", _("自己分析", "Self knowledge") },
    { "l", _("ライフレート変更", "Set new life rate") },
    { "m", _("突然変異", "Get mutation") },
    { "a", _("属性表示", "Print your alignment") },
};

/*!
 * @brief プレイヤー設定コマンドの一覧を表示する
 */
void display_wizard_player_modifier_menu()
{
    for (int y = 1; y < 9; y++)
        term_erase(14, y, 64);

    int r = 1;
    int c = 15;
    int sz = wizard_player_modifier_menu_table.size();
    for (int i = 0; i < sz; i++) {
        std::stringstream ss;
        ss << wizard_player_modifier_menu_table[i][0] << ") " << wizard_player_modifier_menu_table[i][1];
        put_str(ss.str().c_str(), r++, c);
    }
}

/*!
 * @brief プレイヤー設定コマンドの入力を受け付ける
 * @param player_ptr プレイヤーの情報へのポインタ
 */
void wizard_player_modifier(player_type *player_ptr)
{
    screen_save();
    display_wizard_player_modifier_menu();

    char cmd;
    get_com("Player Command: ", &cmd, false);
    screen_load();

    switch (cmd) {
    case ESCAPE:
    case ' ':
    case '\n':
    case '\r':
        break;
    case 'a':
        msg_format("Your alignment is %d.", player_ptr->alignment);
        break;
    case 'c':
        wiz_reset_class(player_ptr);
        break;
    case 'e':
        wiz_change_status(player_ptr);
        break;
    case 'k':
        self_knowledge(player_ptr);
        break;
    case 'm':
        (void)gain_mutation(player_ptr, command_arg);
        break;
    case 'n':
        roll_hitdice(player_ptr, static_cast<spell_operation>(SPOP_DISPLAY_MES | SPOP_DEBUG));
        break;
    case 'r':
        wiz_reset_race(player_ptr);
        break;
    case 'R':
        wiz_reset_realms(player_ptr);
        break;
    default:
        msg_print("That is not a valid debug command.");
        break;
    }
}
