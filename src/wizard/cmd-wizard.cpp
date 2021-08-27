/*!
 * @brief デバッグコマンドの分岐実装
 * @date 2020/08/01
 * @author Hourier
 * @details 通常のコマンドではないのでcmd-xxx/ ではなくwizard/ 以下に置く
 */

#include "wizard/cmd-wizard.h"
#include "birth/inventory-initializer.h"
#include "cmd-io/cmd-help.h"
#include "core/asking-player.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-requester.h"
#include "mutation/mutation-investor-remover.h"
#include "player/patron.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spells-status.h"
#include "status/experience.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "wizard/wizard-game-modifier.h"
#include "wizard/wizard-item-modifier.h"
#include "wizard/wizard-player-modifier.h"
#include "wizard/wizard-special-process.h"
#include "wizard/wizard-spells.h"
#include "wizard/wizard-spoiler.h"
#include <sstream>
#include <string>
#include <vector>

/*!
 * @brief デバグコマンド一覧表
 * @details
 * 空き: A,B,E,I,J,k,K,L,M,q,Q,R,T,U,V,W,y,Y
 */
std::vector<std::vector<std::string>> debug_menu_table = {
    { "a", _("全状態回復", "Restore all status") },
    { "b", _("現在のターゲットを引き寄せる", "Teleport target back") },
    { "c", _("オブジェクト生成", "Create object") },
    { "C", _("固定アーティファクト生成", "Create fixed artifact") },
    { "d", _("全感知", "Detection all") },
    { "D", _("次元の扉", "Dimension door") },
    { "e", _("能力値変更", "Modify player status") },
    { "E", _("青魔法を全取得", "Make all blue magic learned") },
    { "f", _("*鑑定*", "*Idenfity*") },
    { "F", _("地形ID変更", "Modify feature type under player") },
    { "G", _("ゲーム設定コマンドメニュー", "Modify game configurations") },
    { "H", _("モンスターの群れ生成", "Summon monsters") },
    { "i", _("鑑定", "Idenfity") },
    { "I", _("アイテム設定コマンドメニュー", "Modify item configurations") },
    { "j", _("指定ダンジョン階にワープ", "Jump to floor depth of target dungeon") },
    { "k", _("指定ダメージ・半径0の指定属性のボールを自分に放つ", "Fire a zero ball to self") },
    { "m", _("魔法の地図", "Magic mapping") },
    { "n", _("指定モンスター生成", "Summon target monster") },
    { "N", _("指定モンスターをペットとして生成", "Summon target monster as pet") },
    { "o", _("オブジェクトの能力変更", "Modift object abilities") },
    { "O", _("オプション設定をダンプ", "Dump current options") },
    { "p", _("ショート・テレポート", "Phase door") },
    { "P", _("プレイヤー設定変更メニュー", "Modify player configurations") },
    { "r", _("カオスパトロンの報酬", "Get reward of chaos patron") },
    { "s", _("フロア相当のモンスター召喚", "Summon monster which be in target depth") },
    { "t", _("テレポート", "Teleport self") },
    { "u", _("啓蒙(忍者以外)", "Wiz-lite all floor except Ninja") },
    { "w", _("啓蒙(忍者配慮)", "Wiz-lite all floor") },
    { "x", _("経験値を得る(指定可)", "Get experience") },
    { "X", _("所持品を初期状態に戻す", "Return inventory to initial") },
    { "y", _("ダメージ100万・半径0の射撃のボールを放つ", "Cast missile ball had power a million") },
    { "Y", _("指定ダメージ・半径0の指定属性のボールを放つ", "Cast zero ball had power a thousand") },
    { "z", _("近隣のモンスター消去", "Terminate near monsters") },
    { "Z", _("フロアの全モンスター消去", "Terminate all monsters in floor") },
    { "@", _("特殊スペルの発動", "Activate specified spells") },
    { "\"", _("スポイラーのダンプ", "Dump spoiler") },
    { "?", _("ヘルプ表示", "Help") },
};

/*!
 * @brief デバグコマンドの一覧を表示する
 * @param page ページ番号
 * @param max_page ページ数
 * @param page_size 1ページ行数
 * @param max_line コマンド数
 */
void display_debug_menu(int page, int max_page, int page_size, int max_line)
{
    for (int y = 1; y < page_size + 3; y++)
        term_erase(14, y, 64);

    int r = 1;
    int c = 15;
    for (int i = 0; i < page_size; i++) {
        int pos = page * page_size + i;
        if (pos >= max_line)
            break;

        std::stringstream ss;
        ss << debug_menu_table[pos][0] << ") " << debug_menu_table[pos][1];
        put_str(ss.str().c_str(), r++, c);
    }
    if (max_page > 0)
        put_str("-- more --", r++, c);
}

/*!
 * @brief デバッグコマンド選択処理への分岐
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param cmd コマンドキー
 * @return コマンド終了ならTRUE、ページ送りならFALSE
 */
bool exe_cmd_debug(player_type *creature_ptr, char cmd)
{
    switch (cmd) {
    case ' ':
    case '<':
    case '>':
    case KTRL('a'):
        return false;
    case ESCAPE:
    case '\n':
    case '\r':
        break;
    case 'a':
        wiz_cure_all(creature_ptr);
        break;
    case 'b':
        wiz_teleport_back(creature_ptr);
        break;
    case 'c':
        wiz_create_item(creature_ptr);
        break;
    case 'C':
        wiz_create_named_art(creature_ptr, command_arg);
        break;
    case 'd':
        detect_all(creature_ptr, DETECT_RAD_ALL * 3);
        break;
    case 'D':
        wiz_dimension_door(creature_ptr);
        break;
    case 'e':
        wiz_change_status(creature_ptr);
        break;
    case 'E':
        if (creature_ptr->pclass == CLASS_BLUE_MAGE)
            wiz_learn_blue_magic_all(creature_ptr);

        break;
    case 'f':
        identify_fully(creature_ptr, false);
        break;
    case 'F':
        wiz_create_feature(creature_ptr);
        break;
    case 'G':
        wizard_game_modifier(creature_ptr);
        break;
    case 'H':
        wiz_summon_horde(creature_ptr);
        break;
    case 'i':
        (void)ident_spell(creature_ptr, false);
        break;
    case 'I':
        wizard_item_modifier(creature_ptr);
        break;
    case 'j':
        wiz_jump_to_dungeon(creature_ptr);
        break;
    case 'k':
        wiz_kill_me(creature_ptr, 0, command_arg);
        break;
    case 'm':
        map_area(creature_ptr, DETECT_RAD_ALL * 3);
        break;
    case 'r':
        gain_level_reward(creature_ptr, command_arg);
        break;
    case 'N':
        wiz_summon_pet(creature_ptr, command_arg);
        break;
    case 'n':
        wiz_summon_specific_enemy(creature_ptr, command_arg);
        break;
    case 'O':
        wiz_dump_options();
        break;
    case 'o':
        wiz_modify_item(creature_ptr);
        break;
    case 'p':
        teleport_player(creature_ptr, 10, TELEPORT_SPONTANEOUS);
        break;
    case 'P':
        wizard_player_modifier(creature_ptr);
        break;
    case 's':
        if (command_arg <= 0)
            command_arg = 1;

        wiz_summon_random_enemy(creature_ptr, command_arg);
        break;
    case 't':
        teleport_player(creature_ptr, 100, TELEPORT_SPONTANEOUS);
        break;
    case 'u':
        for (int y = 0; y < creature_ptr->current_floor_ptr->height; y++)
            for (int x = 0; x < creature_ptr->current_floor_ptr->width; x++)
                creature_ptr->current_floor_ptr->grid_array[y][x].info |= CAVE_GLOW | CAVE_MARK;

        wiz_lite(creature_ptr, false);
        break;
    case 'w':
        wiz_lite(creature_ptr, (bool)(creature_ptr->pclass == CLASS_NINJA));
        break;
    case 'x':
        gain_exp(creature_ptr, command_arg ? command_arg : (creature_ptr->exp + 1));
        break;
    case 'X':
        for (INVENTORY_IDX i = INVEN_TOTAL - 1; i >= 0; i--)
            if (creature_ptr->inventory_list[i].k_idx)
                drop_from_inventory(creature_ptr, i, 999);

        player_outfit(creature_ptr);
        break;
    case 'y':
        wiz_kill_enemy(creature_ptr);
        break;
    case 'Y':
        wiz_kill_enemy(creature_ptr, 0, command_arg);
        break;
    case 'z':
        wiz_zap_surrounding_monsters(creature_ptr);
        break;
    case 'Z':
        wiz_zap_floor_monsters(creature_ptr);
        break;
    case '_':
        probing(creature_ptr);
        break;
    case '@':
        wiz_debug_spell(creature_ptr);
        break;
    case '"':
        exe_output_spoilers();
        break;
    case '?':
        do_cmd_help(creature_ptr);
        break;
    default:
        msg_print("That is not a valid debug command.");
        break;
    }

    return true;
}

/*!
 * @brief デバッグコマンドを選択する処理のメインルーチン /
 * Ask for and parse a "debug command"
 * The "command_arg" may have been set.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @details
 * 番号を指定するには、それをN及びデバッグコマンドをXとしてとして「0N^aX」とする
 */
void do_cmd_debug(player_type *creature_ptr)
{
    TERM_LEN hgt, wid;
    term_get_size(&wid, &hgt);

    size_t max_line = debug_menu_table.size();
    int page_size = hgt - 5;
    int max_page = max_line / page_size + 1;
    int page = 0;
    char cmd;

    while (true) {
        screen_save();
        display_debug_menu(page, max_page, page_size, max_line);
        get_com("Debug Command: ", &cmd, false);
        screen_load();

        if (exe_cmd_debug(creature_ptr, cmd))
            break;

        page = (page + 1) % max_page;
    }
}
