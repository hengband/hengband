/*!
 * @brief カオス武器で攻撃した際の追加効果処理
 * @date 2020/05/23
 * @author Hourier
 * @details 不可分な処理であるゴールデンハンマーによるアイテム奪取処理も入っている
 */

#include "player-attack/attack-chaos-effect.h"
#include "artifact/fixed-art-types.h"
#include "core/player-redraw-types.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "object/object-mark-types.h"
#include "player/attack-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "spell-kind/spells-polymorph.h"
#include "spell-kind/spells-teleport.h"
#include "spell-realm/spells-hex.h"
#include "system/floor-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief カオス武器か混乱の手でモンスターを混乱させる処理
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return 死んだらTRUE、生きていたらFALSE
 * @return なし
 */
static void attack_confuse(player_type *attacker_ptr, player_attack_type *pa_ptr)
{
    if (attacker_ptr->special_attack & ATTACK_CONFUSE) {
        attacker_ptr->special_attack &= ~(ATTACK_CONFUSE);
        msg_print(_("手の輝きがなくなった。", "Your hands stop glowing."));
        attacker_ptr->redraw |= (PR_STATUS);
    }

    monster_race *r_ptr = &r_info[pa_ptr->m_ptr->r_idx];
    if (r_ptr->flags3 & RF3_NO_CONF) {
        if (is_original_ap_and_seen(attacker_ptr, pa_ptr->m_ptr))
            r_ptr->r_flags3 |= RF3_NO_CONF;
        msg_format(_("%^sには効果がなかった。", "%^s is unaffected."), pa_ptr->m_name);

    } else if (randint0(100) < r_ptr->level) {
        msg_format(_("%^sには効果がなかった。", "%^s is unaffected."), pa_ptr->m_name);
    } else {
        msg_format(_("%^sは混乱したようだ。", "%^s appears confused."), pa_ptr->m_name);
        (void)set_monster_confused(attacker_ptr, pa_ptr->g_ptr->m_idx, monster_confused_remaining(pa_ptr->m_ptr) + 10 + randint0(attacker_ptr->lev) / 5);
    }
}

/*!
 * @breif カオス武器でのテレポート・アウェイを行うか判定する (抵抗されたら無効)
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return 抵抗されたらTRUE、アウェイされるならFALSE
 */
static bool judge_tereprt_resistance(player_type *attacker_ptr, player_attack_type *pa_ptr)
{
    monster_race *r_ptr = &r_info[pa_ptr->m_ptr->r_idx];
    if ((r_ptr->flagsr & RFR_RES_TELE) == 0)
        return FALSE;

    if (r_ptr->flags1 & RF1_UNIQUE) {
        if (is_original_ap_and_seen(attacker_ptr, pa_ptr->m_ptr))
            r_ptr->r_flagsr |= RFR_RES_TELE;

        msg_format(_("%^sには効果がなかった。", "%^s is unaffected!"), pa_ptr->m_name);
        return TRUE;
    }

    if (r_ptr->level > randint1(100)) {
        if (is_original_ap_and_seen(attacker_ptr, pa_ptr->m_ptr))
            r_ptr->r_flagsr |= RFR_RES_TELE;

        msg_format(_("%^sは抵抗力を持っている！", "%^s resists!"), pa_ptr->m_name);
        return TRUE;
    }

    return FALSE;
}

/*!
 * @brief カオス武器でのテレポート・アウェイを実行する
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param num 現在の攻撃回数 (テレポートしてしまったら追加攻撃できないのでその補正)
 * @return なし
 */
static void attack_teleport_away(player_type *attacker_ptr, player_attack_type *pa_ptr, int *num)
{
    if (judge_tereprt_resistance(attacker_ptr, pa_ptr))
        return;

    msg_format(_("%^sは消えた！", "%^s disappears!"), pa_ptr->m_name);
    teleport_away(attacker_ptr, pa_ptr->g_ptr->m_idx, 50, TELEPORT_PASSIVE);
    *num = pa_ptr->num_blow + 1;
    *(pa_ptr->mdeath) = TRUE;
}

/*!
 * @brief カオス武器でのテレポート・アウェイを実行する
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param y モンスターのY座標
 * @param x モンスターのX座標
 * @return なし
 */
static void attack_polymorph(player_type *attacker_ptr, player_attack_type *pa_ptr, POSITION y, POSITION x)
{
    monster_race *r_ptr = &r_info[pa_ptr->m_ptr->r_idx];
    if (((r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) != 0) || ((r_ptr->flagsr & RFR_EFF_RES_CHAO_MASK) != 0))
        return;

    if (polymorph_monster(attacker_ptr, y, x)) {
        msg_format(_("%^sは変化した！", "%^s changes!"), pa_ptr->m_name);
        *(pa_ptr->fear) = FALSE;
        pa_ptr->weak = FALSE;
    } else
        msg_format(_("%^sには効果がなかった。", "%^s is unaffected."), pa_ptr->m_name);

    pa_ptr->m_ptr = &attacker_ptr->current_floor_ptr->m_list[pa_ptr->g_ptr->m_idx];
    monster_desc(attacker_ptr, pa_ptr->m_name, pa_ptr->m_ptr, 0);
}

/*!
 * @brief ゴールデンハンマーによるアイテム奪取処理
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return なし
 */
static void attack_golden_hammer(player_type *attacker_ptr, player_attack_type *pa_ptr)
{
    floor_type *floor_ptr = attacker_ptr->current_floor_ptr;
    monster_type *target_ptr = &floor_ptr->m_list[pa_ptr->g_ptr->m_idx];
    if (target_ptr->hold_o_idx == 0)
        return;

    object_type *q_ptr = &floor_ptr->o_list[target_ptr->hold_o_idx];
    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(attacker_ptr, o_name, q_ptr, OD_NAME_ONLY);
    q_ptr->held_m_idx = 0;
    q_ptr->marked = OM_TOUCHED;
    target_ptr->hold_o_idx = q_ptr->next_o_idx;
    q_ptr->next_o_idx = 0;
    msg_format(_("%sを奪った。", "You snatched %s."), o_name);
    store_item_to_inventory(attacker_ptr, q_ptr);
}

/*!
 * @brief カオス武器その他でモンスターのステータスを変化させる
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param y モンスターのY座標
 * @param x モンスターのX座標
 * @param num 現在の攻撃回数
 * @return なし
 */
void change_monster_stat(player_type *attacker_ptr, player_attack_type *pa_ptr, const POSITION y, const POSITION x, int *num)
{
    monster_race *r_ptr = &r_info[pa_ptr->m_ptr->r_idx];
    object_type *o_ptr = &attacker_ptr->inventory_list[INVEN_RARM + pa_ptr->hand];
    if ((attacker_ptr->special_attack & ATTACK_CONFUSE) || (pa_ptr->chaos_effect == CE_CONFUSION) || (pa_ptr->mode == HISSATSU_CONF)
        || hex_spelling(attacker_ptr, HEX_CONFUSION))
        attack_confuse(attacker_ptr, pa_ptr);
    else if (pa_ptr->chaos_effect == CE_TELE_AWAY)
        attack_teleport_away(attacker_ptr, pa_ptr, num);
    else if ((pa_ptr->chaos_effect == CE_POLYMORPH) && (randint1(90) > r_ptr->level))
        attack_polymorph(attacker_ptr, pa_ptr, y, x);
    else if (o_ptr->name1 == ART_G_HAMMER)
        attack_golden_hammer(attacker_ptr, pa_ptr);
}
