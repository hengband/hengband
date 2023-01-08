/*!
 * @brief 特殊属性武器で攻撃した際の追加効果処理
 * @date 2020/05/23
 * @author Hourier
 * @details
 * カオス属性、魔術属性、ゴールデンハンマー
 */

#include "player-attack/attack-chaos-effect.h"
#include "artifact/fixed-art-types.h"
#include "core/player-redraw-types.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "lore/lore-store.h"
#include "monster-race//race-ability-mask.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-resistance-mask.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "object/object-mark-types.h"
#include "player-attack/player-attack-util.h"
#include "player/attack-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "spell-kind/spells-polymorph.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-realm/spells-hex.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief 打撃でモンスターを混乱させる処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param can_resist レベルで抵抗可能ならTRUE、できないならFALSE
 * @details
 * カオス属性や混乱の手
 */
static void attack_confuse(PlayerType *player_ptr, player_attack_type *pa_ptr, bool can_resist = true)
{
    if (player_ptr->special_attack & ATTACK_CONFUSE) {
        player_ptr->special_attack &= ~(ATTACK_CONFUSE);
        msg_print(_("手の輝きがなくなった。", "Your hands stop glowing."));
        player_ptr->redraw |= (PR_STATUS);
    }

    auto *r_ptr = pa_ptr->r_ptr;
    if (r_ptr->flags3 & RF3_NO_CONF) {
        if (is_original_ap_and_seen(player_ptr, pa_ptr->m_ptr)) {
            r_ptr->r_flags3 |= RF3_NO_CONF;
        }
        msg_format(_("%s^には効果がなかった。", "%s^ is unaffected."), pa_ptr->m_name);

    } else if (can_resist && randint0(100) < r_ptr->level) {
        msg_format(_("%s^には効果がなかった。", "%s^ is unaffected."), pa_ptr->m_name);
    } else {
        msg_format(_("%s^は混乱したようだ。", "%s^ appears confused."), pa_ptr->m_name);
        (void)set_monster_confused(player_ptr, pa_ptr->m_idx, pa_ptr->m_ptr->get_remaining_confusion() + 10 + randint0(player_ptr->lev) / 5);
    }
}

/*!
 * @brief 打撃でモンスターを朦朧とさせる処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param can_resist レベルで抵抗可能ならTRUE、できないならFALSE
 * @details
 * 魔術属性
 */
static void attack_stun(PlayerType *player_ptr, player_attack_type *pa_ptr, bool can_resist = true)
{
    auto *r_ptr = pa_ptr->r_ptr;
    if (any_bits(r_ptr->flags3, RF3_NO_STUN)) {
        if (is_original_ap_and_seen(player_ptr, pa_ptr->m_ptr)) {
            set_bits(r_ptr->flags3, RF3_NO_STUN);
        }
        msg_format(_("%s^には効果がなかった。", "%s^ is unaffected."), pa_ptr->m_name);
    } else if (can_resist && randint0(100) < r_ptr->level) {
        msg_format(_("%s^には効果がなかった。", "%s^ is unaffected."), pa_ptr->m_name);
    } else {
        msg_format(_("%s^は朦朧としたようだ。", "%s^ appears stunned."), pa_ptr->m_name);
        (void)set_monster_stunned(player_ptr, pa_ptr->m_idx, pa_ptr->m_ptr->get_remaining_stun() + 10 + randint0(player_ptr->lev) / 5);
    }
}

/*!
 * @brief 打撃でモンスターを恐怖させる処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param can_resist レベルで抵抗可能ならTRUE、できないならFALSE
 * @details
 * 魔術属性
 */
static void attack_scare(PlayerType *player_ptr, player_attack_type *pa_ptr, bool can_resist = true)
{
    auto *r_ptr = pa_ptr->r_ptr;
    if (any_bits(r_ptr->flags3, RF3_NO_FEAR)) {
        if (is_original_ap_and_seen(player_ptr, pa_ptr->m_ptr)) {
            set_bits(r_ptr->flags3, RF3_NO_FEAR);
        }
        msg_format(_("%s^には効果がなかった。", "%s^ is unaffected."), pa_ptr->m_name);
    } else if (can_resist && randint0(100) < r_ptr->level) {
        msg_format(_("%s^には効果がなかった。", "%s^ is unaffected."), pa_ptr->m_name);
    } else {
        msg_format(_("%s^は恐怖して逃げ出した！", "%s^ flees in terror!"), pa_ptr->m_name);
        (void)set_monster_monfear(player_ptr, pa_ptr->m_idx, pa_ptr->m_ptr->get_remaining_fear() + 10 + randint0(player_ptr->lev) / 5);
    }
}

/*!
 * @brief 打撃でモンスターを無力化する処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @details
 * 魔術属性
 */
static void attack_dispel(PlayerType *player_ptr, player_attack_type *pa_ptr)
{
    if (pa_ptr->r_ptr->ability_flags.has_none_of(RF_ABILITY_ATTACK_MASK) && pa_ptr->r_ptr->ability_flags.has_none_of(RF_ABILITY_INDIRECT_MASK)) {
        return;
    }

    auto dd = 2;
    if (pa_ptr->m_ptr->mtimed[MTIMED_SLOW]) {
        dd += 1;
    }
    if (pa_ptr->m_ptr->mtimed[MTIMED_FAST]) {
        dd += 2;
    }
    if (pa_ptr->m_ptr->mtimed[MTIMED_INVULNER]) {
        dd += 3;
    }

    msg_print(_("武器が敵の魔力を吸い取った！", "The weapon drains mana from your enemy!"));
    dispel_monster_status(player_ptr, pa_ptr->m_idx);

    auto sp = damroll(dd, 8);
    player_ptr->csp = std::min(player_ptr->msp, player_ptr->csp + sp);
    set_bits(player_ptr->redraw, PR_MANA);
}

/*!
 * @brief 打撃でモンスターを調査する処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @details
 * 魔術属性
 */
static void attack_probe(PlayerType *player_ptr, player_attack_type *pa_ptr)
{
    msg_print(_("刃が敵を調査した...", "The blade probed your enemy..."));
    msg_print(nullptr);
    msg_print(probed_monster_info(player_ptr, pa_ptr->m_ptr, pa_ptr->r_ptr));
    msg_print(nullptr);
    (void)lore_do_probe(player_ptr, pa_ptr->r_idx);
}

/*!
 * @breif カオス武器でのテレポート・アウェイを行うか判定する (抵抗されたら無効)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return 抵抗されたらTRUE、アウェイされるならFALSE
 */
static bool judge_tereprt_resistance(PlayerType *player_ptr, player_attack_type *pa_ptr)
{
    auto *r_ptr = pa_ptr->r_ptr;
    if (r_ptr->resistance_flags.has_not(MonsterResistanceType::RESIST_TELEPORT)) {
        return false;
    }

    if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        if (is_original_ap_and_seen(player_ptr, pa_ptr->m_ptr)) {
            r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
        }

        msg_format(_("%s^には効果がなかった。", "%s^ is unaffected!"), pa_ptr->m_name);
        return true;
    }

    if (r_ptr->level > randint1(100)) {
        if (is_original_ap_and_seen(player_ptr, pa_ptr->m_ptr)) {
            r_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
        }

        msg_format(_("%s^は抵抗力を持っている！", "%s^ resists!"), pa_ptr->m_name);
        return true;
    }

    return false;
}

/*!
 * @brief カオス武器でのテレポート・アウェイを実行する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param num 現在の攻撃回数 (テレポートしてしまったら追加攻撃できないのでその補正)
 */
static void attack_teleport_away(PlayerType *player_ptr, player_attack_type *pa_ptr, int *num)
{
    if (judge_tereprt_resistance(player_ptr, pa_ptr)) {
        return;
    }

    msg_format(_("%s^は消えた！", "%s^ disappears!"), pa_ptr->m_name);
    teleport_away(player_ptr, pa_ptr->m_idx, 50, TELEPORT_PASSIVE);
    *num = pa_ptr->num_blow + 1;
    *(pa_ptr->mdeath) = true;
}

/*!
 * @brief カオス武器でのテレポート・アウェイを実行する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param y モンスターのY座標
 * @param x モンスターのX座標
 */
static void attack_polymorph(PlayerType *player_ptr, player_attack_type *pa_ptr, POSITION y, POSITION x)
{
    auto *r_ptr = pa_ptr->r_ptr;
    if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE) || any_bits(r_ptr->flags1, RF1_QUESTOR) || r_ptr->resistance_flags.has_any_of(RFR_EFF_RESIST_CHAOS_MASK)) {
        return;
    }

    if (polymorph_monster(player_ptr, y, x)) {
        msg_format(_("%s^は変化した！", "%s^ changes!"), pa_ptr->m_name);
        *(pa_ptr->fear) = false;
        pa_ptr->weak = false;
    } else {
        msg_format(_("%s^には効果がなかった。", "%s^ is unaffected."), pa_ptr->m_name);
    }

    pa_ptr->m_ptr = &player_ptr->current_floor_ptr->m_list[pa_ptr->m_idx];
    angband_strcpy(pa_ptr->m_name, monster_desc(player_ptr, pa_ptr->m_ptr, 0).data(), sizeof(pa_ptr->m_name));
}

/*!
 * @brief ゴールデンハンマーによるアイテム奪取処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 */
static void attack_golden_hammer(PlayerType *player_ptr, player_attack_type *pa_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[pa_ptr->m_idx];
    if (m_ptr->hold_o_idx_list.empty()) {
        return;
    }

    auto *q_ptr = &floor_ptr->o_list[m_ptr->hold_o_idx_list.front()];
    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, q_ptr, OD_NAME_ONLY);
    q_ptr->held_m_idx = 0;
    q_ptr->marked.clear().set(OmType::TOUCHED);
    m_ptr->hold_o_idx_list.pop_front();
    msg_format(_("%sを奪った。", "You snatched %s."), o_name);
    store_item_to_inventory(player_ptr, q_ptr);
}

/*!
 * @brief カオス武器その他でモンスターのステータスを変化させる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param y モンスターのY座標
 * @param x モンスターのX座標
 * @param num 現在の攻撃回数
 */
void change_monster_stat(PlayerType *player_ptr, player_attack_type *pa_ptr, const POSITION y, const POSITION x, int *num)
{
    auto *r_ptr = &monraces_info[pa_ptr->m_ptr->r_idx];
    auto *o_ptr = &player_ptr->inventory_list[INVEN_MAIN_HAND + pa_ptr->hand];

    if (any_bits(player_ptr->special_attack, ATTACK_CONFUSE) || pa_ptr->chaos_effect == CE_CONFUSION || pa_ptr->mode == HISSATSU_CONF || SpellHex(player_ptr).is_spelling_specific(HEX_CONFUSION)) {
        attack_confuse(player_ptr, pa_ptr);
    }

    if (pa_ptr->magical_effect == MagicalBrandEffectType::STUN) {
        attack_stun(player_ptr, pa_ptr, false);
    }

    if (pa_ptr->magical_effect == MagicalBrandEffectType::SCARE) {
        attack_scare(player_ptr, pa_ptr, false);
    }

    if (pa_ptr->magical_effect == MagicalBrandEffectType::DISPELL) {
        attack_dispel(player_ptr, pa_ptr);
    }

    if (pa_ptr->magical_effect == MagicalBrandEffectType::PROBE) {
        attack_probe(player_ptr, pa_ptr);
    }

    if (pa_ptr->chaos_effect == CE_TELE_AWAY) {
        attack_teleport_away(player_ptr, pa_ptr, num);
    }

    if (pa_ptr->chaos_effect == CE_POLYMORPH && randint1(90) > r_ptr->level) {
        attack_polymorph(player_ptr, pa_ptr, y, x);
    }

    if (o_ptr->is_specific_artifact(FixedArtifactId::G_HAMMER)) {
        attack_golden_hammer(player_ptr, pa_ptr);
    }
}
