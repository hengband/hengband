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
#include "io/gf-descriptions.h"
#include "mind/mind-blue-mage.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-indice-types.h"
#include "mutation/mutation-processor.h"
#include "object-activation/activation-others.h"
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
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "target/grid-selector.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "util/enum-converter.h"
#include "util/flag-group.h"
#include "view/display-messages.h"
#include "wizard/wizard-messages.h"
#include <string_view>
#include <vector>

static const std::vector<debug_spell_command> debug_spell_commands_list = {
    { 2, "vanish dungeon", { .spell2 = { vanish_dungeon } } },
    { 2, "unique detection", { .spell2 = { activate_unique_detection } } },
    { 3, "true healing", { .spell3 = { true_healing } } },
    { 2, "drop weapons", { .spell2 = { drop_weapons } } },
    { 4, "ty curse", { .spell4 = { activate_ty_curse } } },
    { 5, "pattern teleport", { .spell5 = { pattern_teleport } } },
};

/*!
 * @brief コマンド入力により任意にスペル効果を起こす / Wizard spells
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void wiz_debug_spell(PlayerType *player_ptr)
{
    const auto spell = input_string("SPELL: ", 50);
    if (!spell.has_value()) {
        return;
    }

    for (const auto &d : debug_spell_commands_list) {
        if (spell.value() != d.command_name) {
            continue;
        }

        switch (d.type) {
        case 2:
            (d.command_function.spell2.spell_function)(player_ptr);
            return;
        case 3: {
            const auto power = input_integer("POWER", -MAX_INT, MAX_INT);
            if (!power.has_value()) {
                return;
            }

            (d.command_function.spell3.spell_function)(player_ptr, power.value());
            return;
        }
        case 4: {
            auto count = 0;
            (d.command_function.spell4.spell_function)(player_ptr, true, &count);
            return;
        }
        case 5:
            (d.command_function.spell5.spell_function)(player_ptr);
            return;
        default:
            msg_format("Command not found.");
            return;
        }
    }
}

/*!
 * @brief 必ず成功するウィザードモード用次元の扉処理 / Wizard Dimension Door
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void wiz_dimension_door(PlayerType *player_ptr)
{
    POSITION x = 0, y = 0;
    if (!tgt_pt(player_ptr, &x, &y)) {
        return;
    }

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
        if (is_cave_empty_bold(player_ptr, wy, wx)) {
            break;
        }
    }

    (void)alloc_horde(player_ptr, wy, wx, summon_specific);
}

/*!
 * @brief ウィザードモード用処理としてターゲット中の相手をテレポートバックする / Hack -- Teleport to the target
 */
void wiz_teleport_back(PlayerType *player_ptr)
{
    if (!target_who) {
        return;
    }

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
 * @brief 現在のフロアに合ったモンスターをランダムに生成する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param num 生成処理回数
 * @details 半径5マス以内に生成する。生成場所がなかったらキャンセル。
 */
void wiz_generate_random_monster(PlayerType *player_ptr, int num)
{
    constexpr auto flags = PM_ALLOW_SLEEP | PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_QUEST;
    for (auto i = 0; i < num; i++) {
        if (!alloc_monster(player_ptr, 0, flags, summon_specific, 5)) {
            msg_print_wizard(player_ptr, 1, "Monster isn't generated correctly...");
            return;
        }
    }
}

/*!
 * @brief 現在のフロアに合ったモンスターをランダムに召喚する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param num 生成処理回数
 * @details 現在のレベル+5F からランダムに選定する。生成場所がなかったらキャンセル。
 */
void wiz_summon_random_monster(PlayerType *player_ptr, int num)
{
    const auto level = player_ptr->current_floor_ptr->dun_level;
    constexpr auto flags = PM_ALLOW_GROUP | PM_ALLOW_UNIQUE;
    const auto y = player_ptr->y;
    const auto x = player_ptr->x;
    for (auto i = 0; i < num; i++) {
        if (!summon_specific(player_ptr, 0, y, x, level, SUMMON_NONE, flags)) {
            msg_print_wizard(player_ptr, 1, "Monster isn't summoned correctly...");
            return;
        }
    }
}

/*!
 * @brief モンスターを種族IDを指定して自然生成と同じように召喚する /
 * Summon a creature of the specified type
 * @param r_idx モンスター種族ID（回数指定コマンド'0'で指定した回数がIDになる）
 * @details
 * This function is rather dangerous
 */
void wiz_summon_specific_monster(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    if (!MonsterRace(r_idx).is_valid()) {
        const auto new_monrace_id = input_numerics("MonsterID", 1, monraces_info.size() - 1, MonsterRaceId::FILTHY_URCHIN);
        if (!new_monrace_id.has_value()) {
            return;
        }

        r_idx = new_monrace_id.value();
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
void wiz_summon_pet(PlayerType *player_ptr, MonsterRaceId r_idx)
{
    if (!MonsterRace(r_idx).is_valid()) {
        const auto new_monrace_id = input_numerics("MonsterID", 1, monraces_info.size() - 1, MonsterRaceId::FILTHY_URCHIN);
        if (!new_monrace_id.has_value()) {
            return;
        }

        r_idx = new_monrace_id.value();
    }

    (void)summon_named_creature(player_ptr, 0, player_ptr->y, player_ptr->x, r_idx, PM_ALLOW_SLEEP | PM_ALLOW_GROUP | PM_FORCE_PET);
}

/*!
 * @brief ターゲットを指定して指定ダメージ・指定属性・半径0のボールを放つ
 * @param dam ダメージ量
 * @param effect_idx 属性ID
 * @param self 自分に与えるか否か
 * @details デフォルトは100万・GF_ARROW(射撃)。RES_ALL持ちも一撃で殺せる。
 */
void wiz_kill_target(PlayerType *player_ptr, int initial_dam, AttributeType effect_idx, const bool self)
{
    auto dam = initial_dam;
    if (dam <= 0) {
        const auto input_dam = input_integer("Damage", 1, 1000000, 1000000);
        if (!input_dam.has_value()) {
            return;
        }

        dam = input_dam.value();
    }

    constexpr auto max = enum2i(AttributeType::MAX);
    auto idx = effect_idx;
    if (effect_idx == AttributeType::NONE) {
        screen_save();
        for (auto i = 1; i <= 23; i++) {
            prt("", i, 0);
        }

        for (auto i = 0U; i < std::size(gf_descriptions); ++i) {
            const auto &gf_description = gf_descriptions[i];
            auto name = std::string_view(gf_description.name).substr(3); // 先頭の"GF_"を取り除く
            auto num = enum2i(gf_description.num);
            put_str(format("%03d:%-.10s^", num, name.data()), 1 + i / 5, 1 + (i % 5) * 16);
        }

        const auto input_effect_id = input_numerics("EffectID", 1, max - 1, idx);
        if (!input_effect_id.has_value()) {
            screen_load();
            return;
        }

        idx = input_effect_id.value();
        screen_load();
    }

    if (self) {
        project(player_ptr, -1, 0, player_ptr->y, player_ptr->x, dam, idx, PROJECT_KILL | PROJECT_PLAYER);
        return;
    }

    DIRECTION dir;
    if (!get_aim_dir(player_ptr, &dir)) {
        return;
    }
    fire_ball(player_ptr, idx, dir, dam, 0);
}
