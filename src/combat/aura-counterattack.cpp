/*!
 * @brief モンスターから直接攻撃を受けた時に、プレイヤーのオーラダメージで反撃する処理
 * @date 2020/05/31
 * @author Hourier
 */

#include "combat/aura-counterattack.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "monster-attack/monster-attack-player.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-resistance-mask.h"
#include "monster/monster-damage.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "object-hook/hook-armor.h"
#include "player/player-status-flags.h"
#include "realm/realm-hex-numbers.h"
#include "spell-kind/spells-teleport.h"
#include "spell-realm/spells-hex.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include <fmt/format.h>
#include <string>
#include <string_view>

/*!
 * @brief オーラの反撃を受けない耐性をモンスターが持つかを調べる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃の情報
 * @param immune_mask 反撃を受けない耐性
 * @return 耐性を持つか
 * @details 耐性を持っていれば、見えているときに思い出に記録する
 */
static bool is_immune_to_aura(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr, const EnumClassFlagGroup<MonsterResistanceType> &immune_mask)
{
    auto &monrace = monap_ptr->m_ptr->get_monrace();
    if (monrace.resistance_flags.has_none_of(immune_mask)) {
        return false;
    }

    if (is_original_ap_and_seen(player_ptr, *monap_ptr->m_ptr)) {
        monrace.r_resistance_flags.set(monrace.resistance_flags & immune_mask);
    }

    return true;
}

/*!
 * @brief オーラの反撃でモンスターにダメージを与える
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃の情報
 * @param dam ダメージ (モンスターによる補正の前)
 * @param message 反撃したときのメッセージ
 * @param attribute 反撃の属性
 * @param note_dies モンスターが倒れたときのメッセージ
 * @return モンスターが倒れたか
 */
static bool damage_by_aura(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr, int dam, std::string_view message, AttributeType attribute, std::string_view note_dies)
{
    dam = mon_damage_mod(player_ptr, *monap_ptr->m_ptr, dam, false);
    msg_print(message);
    MonsterDamageProcessor mdp(player_ptr, monap_ptr->m_idx, dam, &monap_ptr->fear, attribute);
    if (!mdp.mon_take_hit(note_dies)) {
        return false;
    }

    monap_ptr->blinked = false;
    monap_ptr->alive = false;
    return true;
}

/*!
 * @brief 2d6 のダメージを与えるオーラで反撃する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃の情報
 * @param immune_mask 反撃を受けない耐性
 * @param message 反撃したときのメッセージ (モンスターの名前を埋め込む)
 * @param attribute 反撃の属性
 * @param note_dies モンスターが倒れたときのメッセージ
 * @return 反撃したか (モンスターが倒れているか、耐性を持っていれば反撃しない)
 */
static bool aura_by_monster_attack(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr, const EnumClassFlagGroup<MonsterResistanceType> &immune_mask,
    fmt::format_string<std::string> message, AttributeType attribute, std::string_view note_dies)
{
    if (!monap_ptr->alive || player_ptr->is_dead) {
        return false;
    }

    if (is_immune_to_aura(player_ptr, monap_ptr, immune_mask)) {
        return false;
    }

    const auto dam = Dice::roll(2, 6);
    damage_by_aura(player_ptr, monap_ptr, dam, fmt::format(message, str_upcase_first(monap_ptr->m_name)), attribute, note_dies);
    return true;
}

static void aura_shards_by_monster_attack(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    if (!player_ptr->dustrobe || !monap_ptr->alive || player_ptr->is_dead) {
        return;
    }

    aura_by_monster_attack(player_ptr, monap_ptr, RFR_EFF_RESIST_SHARDS_MASK, _("{}は鏡の破片をくらった！", "{} gets sliced!"), AttributeType::SHARDS,
        _("はズタズタになった。", " is torn to pieces."));

    if (player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x].is_mirror()) {
        teleport_player(player_ptr, 10, TELEPORT_SPONTANEOUS);
    }
}

static void aura_holy_by_monster_attack(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    if (!player_ptr->tim_sh_holy || !monap_ptr->alive || player_ptr->is_dead) {
        return;
    }

    auto &monrace = monap_ptr->m_ptr->get_monrace();
    if (monrace.kind_flags.has_not(MonsterKindType::EVIL)) {
        return;
    }

    if (!aura_by_monster_attack(player_ptr, monap_ptr, { MonsterResistanceType::RESIST_ALL }, _("{}は聖なるオーラで傷ついた！", "{} is injured by holy power!"),
            AttributeType::HOLY_FIRE, _("は倒れた。", " is destroyed."))) {
        return;
    }

    if (is_original_ap_and_seen(player_ptr, *monap_ptr->m_ptr)) {
        monrace.r_kind_flags.set(MonsterKindType::EVIL);
    }
}

static void aura_shadow_by_monster_attack(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    if (!SpellHex(player_ptr).is_spelling_specific(HEX_SHADOW_CLOAK) || !monap_ptr->alive || player_ptr->is_dead) {
        return;
    }

    if (is_immune_to_aura(player_ptr, monap_ptr, { MonsterResistanceType::RESIST_ALL, MonsterResistanceType::RESIST_DARK })) {
        return;
    }

    int dam = 1;
    ItemEntity *o_armed_ptr = player_ptr->inventory[INVEN_MAIN_HAND].get();

    if (o_armed_ptr->is_valid()) {
        const auto num = (o_armed_ptr->damage_dice.num + player_ptr->damage_dice_bonus[0].num);
        const auto sides = (o_armed_ptr->damage_dice.sides + player_ptr->damage_dice_bonus[0].sides);
        dam = Dice::floored_expected_value(num, sides) + o_armed_ptr->to_d + player_ptr->to_d[0];
    }

    o_armed_ptr = player_ptr->inventory[INVEN_BODY].get();
    if (o_armed_ptr->is_valid() && o_armed_ptr->is_cursed()) {
        dam *= 2;
    }

    const auto message = fmt::format(_("影のオーラが{}に反撃した！", "Enveloping shadows attack {}."), monap_ptr->m_name);
    if (damage_by_aura(player_ptr, monap_ptr, dam, message, AttributeType::DARK, _("は倒れた。", " is destroyed."))) {
        return;
    }

    struct attribute_table {
        inventory_slot_type slot;
        AttributeType type;
    };

    const int TABLE_SIZE = 4;
    attribute_table table[TABLE_SIZE] = { { INVEN_HEAD, AttributeType::OLD_CONF }, { INVEN_SUB_HAND, AttributeType::OLD_SLEEP },
        { INVEN_ARMS, AttributeType::TURN_ALL }, { INVEN_FEET, AttributeType::OLD_SLOW } };

    BIT_FLAGS flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;

    /* Some cursed armours gives an extra effect */
    for (int j = 0; j < TABLE_SIZE; j++) {
        o_armed_ptr = player_ptr->inventory[table[j].slot].get();
        if (o_armed_ptr->is_valid() && o_armed_ptr->is_cursed() && o_armed_ptr->is_protector()) {
            project(player_ptr, 0, 0, monap_ptr->m_ptr->fy, monap_ptr->m_ptr->fx, (player_ptr->lev * 2), table[j].type, flg);
        }
    }
}

void process_aura_counterattack(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    if (!monap_ptr->touched) {
        return;
    }

    if (has_sh_fire(player_ptr)) {
        aura_by_monster_attack(player_ptr, monap_ptr, RFR_EFF_IM_FIRE_MASK, _("{}は突然熱くなった！", "{} is suddenly very hot!"), AttributeType::FIRE,
            _("は灰の山になった。", " turns into a pile of ash."));
    }

    if (has_sh_elec(player_ptr)) {
        aura_by_monster_attack(player_ptr, monap_ptr, RFR_EFF_IM_ELEC_MASK, _("{}は電撃をくらった！", "{} gets zapped!"), AttributeType::ELEC,
            _("は燃え殻の山になった。", " turns into a pile of cinders."));
    }

    if (has_sh_cold(player_ptr)) {
        aura_by_monster_attack(player_ptr, monap_ptr, RFR_EFF_IM_COLD_MASK, _("{}は冷気をくらった！", "{} is very cold!"), AttributeType::COLD,
            _("は凍りついた。", " was frozen."));
    }

    aura_shards_by_monster_attack(player_ptr, monap_ptr);
    aura_holy_by_monster_attack(player_ptr, monap_ptr);
    if (player_ptr->tim_sh_touki) {
        aura_by_monster_attack(player_ptr, monap_ptr, { MonsterResistanceType::RESIST_ALL }, _("{}が鋭い闘気のオーラで傷ついた！", "{} is injured by the Force"),
            AttributeType::MANA, _("は倒れた。", " is destroyed."));
    }
    aura_shadow_by_monster_attack(player_ptr, monap_ptr);
}
