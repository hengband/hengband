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
#include <tuple>
#include <vector>

/*!
 * @brief デバグコマンド一覧表
 * @details
 * 空き: A,B,E,I,J,k,K,L,M,q,Q,R,T,U,V,W,y,Y
 */
constexpr std::array debug_menu_table = {
    std::make_tuple('a', _("全状態回復", "Restore all status")),
    std::make_tuple('b', _("現在のターゲットを引き寄せる", "Teleport target back")),
    std::make_tuple('c', _("オブジェクト生成", "Create object")),
    std::make_tuple('C', _("固定アーティファクト生成", "Create fixed artifact")),
    std::make_tuple('d', _("全感知", "Detection all")),
    std::make_tuple('D', _("次元の扉", "Dimension door")),
    std::make_tuple('e', _("能力値変更", "Modify player status")),
    std::make_tuple('E', _("青魔法を全取得/エッセンスを全取得", "Learn all blue magics / Obtain all essences")),
    std::make_tuple('f', _("*鑑定*", "*Idenfity*")),
    std::make_tuple('F', _("地形ID変更", "Modify feature type under player")),
    std::make_tuple('G', _("ゲーム設定コマンドメニュー", "Modify game configurations")),
    std::make_tuple('H', _("モンスターの群れ生成", "Summon monsters")),
    std::make_tuple('i', _("鑑定", "Idenfity")),
    std::make_tuple('I', _("アイテム設定コマンドメニュー", "Modify item configurations")),
    std::make_tuple('j', _("指定ダンジョン階にワープ", "Jump to floor depth of target dungeon")),
    std::make_tuple('k', _("指定ダメージ・半径0の指定属性のボールを自分に放つ", "Fire a zero ball to self")),
    std::make_tuple('m', _("魔法の地図", "Magic mapping")),
    std::make_tuple('n', _("指定モンスター生成", "Summon target monster")),
    std::make_tuple('N', _("指定モンスターをペットとして生成", "Summon target monster as pet")),
    std::make_tuple('o', _("オブジェクトの能力変更", "Modift object abilities")),
    std::make_tuple('O', _("オプション設定をダンプ", "Dump current options")),
    std::make_tuple('p', _("ショート・テレポート", "Phase door")),
    std::make_tuple('P', _("プレイヤー設定変更メニュー", "Modify player configurations")),
    std::make_tuple('r', _("カオスパトロンの報酬", "Get reward of chaos patron")),
    std::make_tuple('s', _("フロア相当のモンスター召喚", "Summon monster which be in target depth")),
    std::make_tuple('t', _("テレポート", "Teleport self")),
    std::make_tuple('u', _("啓蒙(忍者以外)", "Wiz-lite all floor except Ninja")),
    std::make_tuple('w', _("啓蒙(忍者配慮)", "Wiz-lite all floor")),
    std::make_tuple('x', _("経験値を得る(指定可)", "Get experience")),
    std::make_tuple('X', _("所持品を初期状態に戻す", "Return inventory to initial")),
    std::make_tuple('y', _("ダメージ100万・半径0の射撃のボールを放つ", "Cast missile ball had power a million")),
    std::make_tuple('Y', _("指定ダメージ・半径0の指定属性のボールを放つ", "Cast zero ball had power a thousand")),
    std::make_tuple('z', _("近隣のモンスター消去", "Terminate near monsters")),
    std::make_tuple('Z', _("フロアの全モンスター消去", "Terminate all monsters in floor")),
    std::make_tuple('@', _("特殊スペルの発動", "Activate specified spells")),
    std::make_tuple('"', _("スポイラーのダンプ", "Dump spoiler")),
    std::make_tuple('?', _("ヘルプ表示", "Help")),
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
        const auto &[symbol, desc] = debug_menu_table[pos];
        ss << symbol << ") " << desc;
        put_str(ss.str().c_str(), r++, c);
    }
    if (max_page > 1)
        put_str("-- more --", r++, c);
}

/*!
 * @brief デバッグコマンド選択処理への分岐
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param cmd コマンドキー
 * @return コマンド終了ならTRUE、ページ送りならFALSE
 */
bool exe_cmd_debug(player_type *player_ptr, char cmd)
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
        wiz_cure_all(player_ptr);
        break;
    case 'b':
        wiz_teleport_back(player_ptr);
        break;
    case 'c':
        wiz_create_item(player_ptr);
        break;
    case 'C':
        wiz_create_named_art(player_ptr);
        break;
    case 'd':
        detect_all(player_ptr, DETECT_RAD_ALL * 3);
        break;
    case 'D':
        wiz_dimension_door(player_ptr);
        break;
    case 'e':
        wiz_change_status(player_ptr);
        break;
    case 'E':
        switch (player_ptr->pclass) {
        case PlayerClassType::BLUE_MAGE:
            wiz_learn_blue_magic_all(player_ptr);
            break;
        case PlayerClassType::SMITH:
            wiz_fillup_all_smith_essences(player_ptr);
            break;
        default:
            break;
        }
        break;
    case 'f':
        identify_fully(player_ptr, false);
        break;
    case 'F':
        wiz_create_feature(player_ptr);
        break;
    case 'G':
        wizard_game_modifier(player_ptr);
        break;
    case 'H':
        wiz_summon_horde(player_ptr);
        break;
    case 'i':
        (void)ident_spell(player_ptr, false);
        break;
    case 'I':
        wizard_item_modifier(player_ptr);
        break;
    case 'j':
        wiz_jump_to_dungeon(player_ptr);
        break;
    case 'k':
        wiz_kill_me(player_ptr, 0, command_arg);
        break;
    case 'm':
        map_area(player_ptr, DETECT_RAD_ALL * 3);
        break;
    case 'r':
        patron_list[player_ptr->chaos_patron].gain_level_reward(player_ptr, command_arg);
        break;
    case 'N':
        wiz_summon_pet(player_ptr, command_arg);
        break;
    case 'n':
        wiz_summon_specific_enemy(player_ptr, command_arg);
        break;
    case 'O':
        wiz_dump_options();
        break;
    case 'o':
        wiz_modify_item(player_ptr);
        break;
    case 'p':
        teleport_player(player_ptr, 10, TELEPORT_SPONTANEOUS);
        break;
    case 'P':
        wizard_player_modifier(player_ptr);
        break;
    case 's':
        if (command_arg <= 0)
            command_arg = 1;

        wiz_summon_random_enemy(player_ptr, command_arg);
        break;
    case 't':
        teleport_player(player_ptr, 100, TELEPORT_SPONTANEOUS);
        break;
    case 'u':
        for (int y = 0; y < player_ptr->current_floor_ptr->height; y++)
            for (int x = 0; x < player_ptr->current_floor_ptr->width; x++)
                player_ptr->current_floor_ptr->grid_array[y][x].info |= CAVE_GLOW | CAVE_MARK;

        wiz_lite(player_ptr, false);
        break;
    case 'w':
        wiz_lite(player_ptr, (bool)(player_ptr->pclass == PlayerClassType::NINJA));
        break;
    case 'x':
        gain_exp(player_ptr, command_arg ? command_arg : (player_ptr->exp + 1));
        break;
    case 'X':
        for (INVENTORY_IDX i = INVEN_TOTAL - 1; i >= 0; i--)
            if (player_ptr->inventory_list[i].k_idx)
                drop_from_inventory(player_ptr, i, 999);

        player_outfit(player_ptr);
        break;
    case 'y':
        wiz_kill_enemy(player_ptr);
        break;
    case 'Y':
        wiz_kill_enemy(player_ptr, 0, command_arg);
        break;
    case 'z':
        wiz_zap_surrounding_monsters(player_ptr);
        break;
    case 'Z':
        wiz_zap_floor_monsters(player_ptr);
        break;
    case '_':
        probing(player_ptr);
        break;
    case '@':
        wiz_debug_spell(player_ptr);
        break;
    case '"':
        exe_output_spoilers();
        break;
    case '?':
        do_cmd_help(player_ptr);
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
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * 番号を指定するには、それをN及びデバッグコマンドをXとしてとして「0N^aX」とする
 */
void do_cmd_debug(player_type *player_ptr)
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

        if (exe_cmd_debug(player_ptr, cmd))
            break;

        page = (page + 1) % max_page;
    }
}
