/*!
 * @brief ウィザードモード専用のスペル処理
 * @date 2020/06/27
 * @author Hourier
 */

#include "wizard/wizard-spells.h"
#include "blue-magic/blue-magic-checker.h"
#include "core/asking-player.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
#include "floor/floor-util.h"
#include "floor/pattern-walk.h"
#include "mind/mind-blue-mage.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-ability-flags.h"
#include "mutation/mutation-processor.h"
#include "player-base/player-class.h"
#include "player-info/bluemage-data-type.h"
#include "player-info/smith-data-type.h"
#include "smith/object-smith.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-random.h"
#include "spell-kind/spells-teleport.h"
#include "spell-realm/spells-chaos.h"
#include "spell/spells-status.h"
#include "spell/summon-types.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/player-type-definition.h"
#include "target/grid-selector.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "util/enum-converter.h"
#include "util/flag-group.h"
#include "view/display-messages.h"

#include <vector>

debug_spell_command debug_spell_commands_list[SPELL_MAX] = {
    { 2, "vanish dungeon", { .spell2 = { vanish_dungeon } } },
    { 3, "true healing", { .spell3 = { true_healing } } },
    { 2, "drop weapons", { .spell2 = { drop_weapons } } },
    { 4, "ty curse", { .spell4 = { activate_ty_curse } } },
    { 5, "pattern teleport", { .spell5 = { pattern_teleport } } },
};

/*!
 * @brief コマンド入力により任意にスペル効果を起こす / Wizard spells
 * @return 実際にテレポートを行ったらTRUEを返す
 */
bool wiz_debug_spell(PlayerType *player_ptr)
{
    char tmp_val[50] = "\0";
    int tmp_int;

    if (!get_string("SPELL: ", tmp_val, 32))
        return false;

    for (int i = 0; i < SPELL_MAX; i++) {
        if (strcmp(tmp_val, debug_spell_commands_list[i].command_name) != 0)
            continue;

        switch (debug_spell_commands_list[i].type) {
        case 2:
            (*(debug_spell_commands_list[i].command_function.spell2.spell_function))(player_ptr);
            return true;
            break;
        case 3:
            tmp_val[0] = '\0';
            if (!get_string("POWER:", tmp_val, 32))
                return false;
            tmp_int = atoi(tmp_val);
            (*(debug_spell_commands_list[i].command_function.spell3.spell_function))(player_ptr, tmp_int);
            return true;
            break;
        case 4:
            (*(debug_spell_commands_list[i].command_function.spell4.spell_function))(player_ptr, true, &tmp_int);
            return true;
            break;
        case 5:
            (*(debug_spell_commands_list[i].command_function.spell5.spell_function))(player_ptr);
            return true;
            break;
        default:
            break;
        }
    }

    msg_format("Command not found.");
    return false;
}

/*!
 * @brief 必ず成功するウィザードモード用次元の扉処理 / Wizard Dimension Door
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void wiz_dimension_door(PlayerType *player_ptr)
{
    POSITION x = 0, y = 0;
    if (!tgt_pt(player_ptr, &x, &y))
        return;

    teleport_player_to(player_ptr, y, x, TELEPORT_NONMAGICAL);
}

/*!
 * @brief ウィザードモード用モンスターの群れ生成 / Summon a horde of monsters
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void wiz_summon_horde(PlayerType *player_ptr)
{
    POSITION wy = player_ptr->y, wx = player_ptr->x;
    int attempts = 1000;

    while (--attempts) {
        scatter(player_ptr, &wy, &wx, player_ptr->y, player_ptr->x, 3, PROJECT_NONE);
        if (is_cave_empty_bold(player_ptr, wy, wx))
            break;
    }

    (void)alloc_horde(player_ptr, wy, wx, summon_specific);
}

/*!
 * @brief ウィザードモード用処理としてターゲット中の相手をテレポートバックする / Hack -- Teleport to the target
 */
void wiz_teleport_back(PlayerType *player_ptr)
{
    if (!target_who)
        return;

    teleport_player_to(player_ptr, target_row, target_col, TELEPORT_NONMAGICAL);
}

/*!
 * @brief 青魔導師の魔法を全て習得済みにする /
 * debug command for blue mage
 */
void wiz_learn_blue_magic_all(PlayerType *player_ptr)
{
    auto bluemage_data = PlayerClass(player_ptr).get_specific_data<bluemage_data_type>();
    if (!bluemage_data) {
        return;
    }

    for (auto type : BLUE_MAGIC_TYPE_LIST) {
        EnumClassFlagGroup<MonsterAbilityType> ability_flags;
        set_rf_masks(ability_flags, type);
        bluemage_data->learnt_blue_magics.set(ability_flags);
    }
}

/*!
 * @brief 鍛冶師の全てのエッセンスを最大所持量にする
 */
void wiz_fillup_all_smith_essences(PlayerType *player_ptr)
{
    auto smith_data = PlayerClass(player_ptr).get_specific_data<smith_data_type>();
    if (!smith_data) {
        return;
    }

    for (auto essence : Smith::get_essence_list()) {
        smith_data->essences[essence] = Smith::ESSENCE_AMOUNT_MAX;
    }
}

/*!
 * @brief 現在のフロアに合ったモンスターをランダムに召喚する /
 * Summon some creatures
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param num 生成処理回数
 */
void wiz_summon_random_enemy(PlayerType *player_ptr, int num)
{
    for (int i = 0; i < num; i++)
        (void)summon_specific(player_ptr, 0, player_ptr->y, player_ptr->x, player_ptr->current_floor_ptr->dun_level, SUMMON_NONE, PM_ALLOW_GROUP | PM_ALLOW_UNIQUE);
}

/*!
 * @brief モンスターを種族IDを指定して自然生成と同じように召喚する /
 * Summon a creature of the specified type
 * @param r_idx モンスター種族ID（回数指定コマンド'0'で指定した回数がIDになる）
 * @details
 * This function is rather dangerous
 */
void wiz_summon_specific_enemy(PlayerType *player_ptr, MONRACE_IDX r_idx)
{
    if (r_idx <= 0) {
        int val = 1;
        if (!get_value("MonsterID", 1, r_info.size() - 1, &val)) {
            return;
        }
        r_idx = static_cast<MONRACE_IDX>(val);
    }
    (void)summon_named_creature(player_ptr, 0, player_ptr->y, player_ptr->x, r_idx, PM_ALLOW_SLEEP | PM_ALLOW_GROUP);
}

/*!
 * @brief モンスターを種族IDを指定してペット召喚する /
 * Summon a creature of the specified type
 * @param r_idx モンスター種族ID（回数指定コマンド'0'で指定した回数がIDになる）
 * @details
 * This function is rather dangerous
 */
void wiz_summon_pet(PlayerType *player_ptr, MONRACE_IDX r_idx)
{
    if (r_idx <= 0) {
        int val = 1;
        if (!get_value("MonsterID", 1, r_info.size() - 1, &val)) {
            return;
        }
        r_idx = static_cast<MONRACE_IDX>(val);
    }
    (void)summon_named_creature(player_ptr, 0, player_ptr->y, player_ptr->x, r_idx, PM_ALLOW_SLEEP | PM_ALLOW_GROUP | PM_FORCE_PET);
}

/*!
 * @brief ターゲットを指定して指定ダメージ・指定属性・半径0のボールを放つ
 * @param dam ダメージ量
 * @param effect_idx 属性ID
 * @details デフォルトは100万・GF_ARROW(射撃)。RES_ALL持ちも一撃で殺せる。
 */
void wiz_kill_enemy(PlayerType *player_ptr, HIT_POINT dam, AttributeType effect_idx)
{
    if (dam <= 0) {
        char tmp[80] = "";
        sprintf(tmp, "Damage (1-999999): ");
        char tmp_val[10] = "1000";
        if (!get_string(tmp, tmp_val, 6))
            return;

        dam = (HIT_POINT)atoi(tmp_val);
    }
    int max = (int)AttributeType::MAX;
    int idx = (int)effect_idx;

    if (idx <= 0) {
        char tmp[80] = "";
        sprintf(tmp, "Effect ID (1-%d): ", max - 1);
        char tmp_val[10] = "";
        if (!get_string(tmp, tmp_val, 3))
            return;

        effect_idx = (AttributeType)atoi(tmp_val);
    }

    if (idx <= 0 || idx >= max) {
        msg_format(_("番号は1から%dの間で指定して下さい。", "ID must be between 1 to %d."), max - 1);
        return;
    }

    DIRECTION dir;

    if (!get_aim_dir(player_ptr, &dir))
        return;

    fire_ball(player_ptr, effect_idx, dir, dam, 0);
}

/*!
 * @brief 自分に指定ダメージ・指定属性・半径0のボールを放つ
 * @param dam ダメージ量
 * @param effect_idx 属性ID
 */
void wiz_kill_me(PlayerType *player_ptr, HIT_POINT dam, AttributeType effect_idx)
{
    if (dam <= 0) {
        char tmp[80] = "";
        sprintf(tmp, "Damage (1-999999): ");
        char tmp_val[10] = "1000";
        if (!get_string(tmp, tmp_val, 6))
            return;

        dam = (HIT_POINT)atoi(tmp_val);
    }
    int max = (int)AttributeType::MAX;
    int idx = (int)effect_idx;

    if (idx <= 0) {
        char tmp[80] = "";
        sprintf(tmp, "Effect ID (1-%d): ", max - 1);
        char tmp_val[10] = "1";
        if (!get_string(tmp, tmp_val, 3))
            return;

        effect_idx = (AttributeType)atoi(tmp_val);
    }

    if (idx <= 0 || idx >= max) {
        msg_format(_("番号は1から%dの間で指定して下さい。", "ID must be between 1 to %d."), max - 1);
        return;
    }

    project(player_ptr, -1, 0, player_ptr->y, player_ptr->x, dam, effect_idx, PROJECT_KILL | PROJECT_PLAYER);
}
