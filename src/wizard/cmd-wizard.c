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
#include "dungeon/quest.h"
#include "grid/grid.h"
#include "info-reader/fixed-map-parser.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-requester.h"
#include "market/arena.h"
#include "mutation/mutation-investor-remover.h"
#include "player-info/self-info.h"
#include "player/patron.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spells-object.h"
#include "spell/spells-status.h"
#include "status/experience.h"
#include "system/floor-type-definition.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "wizard/wizard-item-modifier.h"
#include "wizard/wizard-special-process.h"
#include "wizard/wizard-spells.h"
#include "wizard/wizard-spoiler.h"

/*!
 * @brief デバッグコマンドを選択する処理のメインルーチン /
 * Ask for and parse a "debug command"
 * The "command_arg" may have been set.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * 番号を指定するには、それをN及びデバッグコマンドをXとしてとして「0N^aX」とする
 * a：全状態回復 / Cure all maladies
 * A：善悪の属性表示 / Know alignment
 * b：相手をテレポバック / Teleport to target
 * B：モンスター闘技場のモンスターを更新する / Update gambling monster
 * c：アイテム生成 / Create any object
 * C：指定番号の固定アーティファクトを生成する / Create a named artifact
 * d：全感知 / Detect everything
 * D：次元の扉 / Dimension_door
 * e：能力変更 / Edit character
 * E：全てのスペルをラーニング状態にする / Blue Mage Only
 * f：*鑑定* / Fully identification
 * F：地形ID変更 / Create desired feature
 * g：上質なアイテムを生成 / Good Objects
 * G：なし / Nothing
 * h：新生 / Hitpoint rerating
 * H：モンスターの群れ生成 / Generate monster group
 * i：鑑定 / Identification
 * I：なし / Nothing
 * j：ダンジョンの指定フロアへテレポート (ウィザードあり) / Jump to dungeon
 * J：なし / Nothing
 * k：自己分析 / Self info
 * K：なし / Nothing
 * l：番号指定したアイテムまで鑑定済にする / Learn about objects
 * L：なし / Nothing
 * m：魔法の地図 / Magic Mapping
 * M：突然変異 / Mutation / TODO: 指定した突然変異の除外機能を追加したい
 * n：番号指定したモンスターを生成 / Generate a monster
 * N：番号指定したペットを生成 / Generate a pet
 * o：アイテムのtval等を編集する / Edit object
 * O：現在のオプション設定をダンプ出力 / Output option settings
 * p：ショートテレポ / Blink
 * P：なし / Nothing
 * q：クエストを完了させる / Finish quest
 * Q：クエストに突入する (ウィザードあり) / Jump to quest
 * r：カオスパトロンから報酬を貰う / Gain reward from chaos patron
 * R：クラス変更 / Change class
 * s：フロア相応のモンスター召喚 / Summon a monster
 * S：高級品獲得ドロップ / Get a great item
 * t：テレポート / Teleport
 * T：プレイ日時変更 / Change time
 * u：啓蒙 (強制的に忍者以外) / Lite floor without ninja classified
 * U：なし / Nothing
 * v：特別品獲得ドロップ / Get a special item
 * V：クラス変更 / Change class / TODO: Rと同じなので何か変えたい
 * w：啓蒙 (忍者かどうか考慮) / Lite floor with ninja classified
 * W：なし / Nothing
 * x：経験値を得る / Gain experience
 * X：アイテムを初期状態に戻す / Return items to the initial ones
 * y：なし / Nothing
 * Y：なし / Nothing
 * z：近隣のモンスター消去 / Zap monsters around
 * Z：フロア中のモンスター消去 / Zap all monsters in the floor
 * @：特殊スペルの発動 / Special spell
 * "：スポイラーのダンプ / Dump spoiler
 * ?：ヘルプ表示 (通常の？と同じ) / Show help (same as normal help)
 */
void do_cmd_debug(player_type *creature_ptr)
{
    char cmd;
    get_com("Debug Command: ", &cmd, FALSE);
    switch (cmd) {
    case ESCAPE:
    case ' ':
    case '\n':
    case '\r':
        break;
    case 'a':
        wiz_cure_all(creature_ptr);
        break;
    case 'A':
        msg_format("Your alignment is %d.", creature_ptr->align);
        break;
    case 'b':
        wiz_teleport_back(creature_ptr);
        break;
    case 'B':
        update_gambling_monsters(creature_ptr);
        break;
    case 'c':
        wiz_create_item(creature_ptr);
        break;
    case 'C':
        wiz_create_named_art(creature_ptr);
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
        identify_fully(creature_ptr, FALSE, 0);
        break;
    case 'F':
        wiz_create_feature(creature_ptr);
        break;
    case 'g':
        if (command_arg <= 0)
            command_arg = 1;

        acquirement(creature_ptr, creature_ptr->y, creature_ptr->x, command_arg, FALSE, FALSE, TRUE);
        break;
    case 'h':
        roll_hitdice(creature_ptr, SPOP_DISPLAY_MES | SPOP_DEBUG);
        break;
    case 'H':
        wiz_summon_horde(creature_ptr);
        break;
    case 'i':
        (void)ident_spell(creature_ptr, FALSE, 0);
        break;
    case 'j':
        wiz_jump_to_dungeon(creature_ptr);
        break;
    case 'k':
        self_knowledge(creature_ptr);
        break;
    case 'l':
        wiz_learn_items_all(creature_ptr);
        break;
    case 'm':
        map_area(creature_ptr, DETECT_RAD_ALL * 3);
        break;
    case 'M':
        (void)gain_mutation(creature_ptr, command_arg);
        break;
    case 'R':
        wiz_reset_class(creature_ptr);
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
    case 'Q': {
        char ppp[30];
        char tmp_val[5];
        int tmp_int;
        sprintf(ppp, "QuestID (0-%d):", max_q_idx - 1);
        sprintf(tmp_val, "%d", 0);

        if (!get_string(ppp, tmp_val, 3))
            return;

        tmp_int = atoi(tmp_val);
        if ((tmp_int < 0) || (tmp_int >= max_q_idx))
            break;

        creature_ptr->current_floor_ptr->inside_quest = (QUEST_IDX)tmp_int;
        parse_fixed_map(creature_ptr, "q_info.txt", 0, 0, 0, 0);
        quest[tmp_int].status = QUEST_STATUS_TAKEN;
        creature_ptr->current_floor_ptr->inside_quest = 0;
        break;
    }
    case 'q':
        if (!creature_ptr->current_floor_ptr->inside_quest) {
            msg_print("No current quest");
            msg_print(NULL);
            break;
        }

        if (quest[creature_ptr->current_floor_ptr->inside_quest].status == QUEST_STATUS_TAKEN)
            complete_quest(creature_ptr, creature_ptr->current_floor_ptr->inside_quest);

        break;
    case 's':
        if (command_arg <= 0)
            command_arg = 1;

        wiz_summon_random_enemy(creature_ptr, command_arg);
        break;
    case 'S':
        if (command_arg <= 0)
            command_arg = 1;

        acquirement(creature_ptr, creature_ptr->y, creature_ptr->x, command_arg, TRUE, TRUE, TRUE);
        break;
    case 't':
        teleport_player(creature_ptr, 100, TELEPORT_SPONTANEOUS);
        break;
    case 'T':
        set_gametime();
        break;
    case 'u':
        for (int y = 0; y < creature_ptr->current_floor_ptr->height; y++)
            for (int x = 0; x < creature_ptr->current_floor_ptr->width; x++)
                creature_ptr->current_floor_ptr->grid_array[y][x].info |= CAVE_GLOW | CAVE_MARK;

        wiz_lite(creature_ptr, FALSE);
        break;
    case 'v':
        if (command_arg <= 0)
            command_arg = 1;

        acquirement(creature_ptr, creature_ptr->y, creature_ptr->x, command_arg, TRUE, FALSE, TRUE);
        break;
    case 'V':
        wiz_reset_class(creature_ptr);
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
        exe_output_spoilers(creature_ptr);
        break;
    case '?':
        do_cmd_help(creature_ptr);
        break;
    default:
        msg_print("That is not a valid debug command.");
        break;
    }
}
