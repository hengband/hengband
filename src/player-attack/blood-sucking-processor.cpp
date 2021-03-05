﻿/*!
 * @brief 吸血処理
 * @date 2020/05/23
 * @author Hourier
 */

#include "player-attack/blood-sucking-processor.h"
#include "artifact/fixed-art-types.h"
#include "core/hp-mp-processor.h"
#include "game-option/cheat-options.h"
#include "inventory/inventory-slot-types.h"
#include "monster-race/monster-race-hook.h"
#include "object-enchant/tr-types.h"
#include "realm/realm-hex-numbers.h"
#include "spell-realm/spells-hex.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 生命のあるモンスターから吸血できるか判定する
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return なし
 */
void decide_blood_sucking(player_type *attacker_ptr, player_attack_type *pa_ptr)
{
    bool is_blood_sucker = has_flag(pa_ptr->flags, TR_VAMPIRIC);
    is_blood_sucker |= pa_ptr->chaos_effect == CE_VAMPIRIC;
    is_blood_sucker |= pa_ptr->mode == HISSATSU_DRAIN;
    is_blood_sucker |= hex_spelling(attacker_ptr, HEX_VAMP_BLADE);
    if (!is_blood_sucker)
        return;

    pa_ptr->can_drain = monster_living(pa_ptr->m_ptr->r_idx);
}

/*!
 * @brief 吸血量を計算する
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return なし
 */
void calc_drain(player_attack_type *pa_ptr)
{
    if (pa_ptr->attack_damage <= 0)
        pa_ptr->can_drain = FALSE;

    if (pa_ptr->drain_result > pa_ptr->m_ptr->hp)
        pa_ptr->drain_result = pa_ptr->m_ptr->hp;
}

/*!
 * @brief 村正による吸血処理
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param is_human モンスターが人間かどうか
 * @return なし
 */
static void drain_muramasa(player_type *attacker_ptr, player_attack_type *pa_ptr, const bool is_human)
{
    if (!is_human)
        return;

    object_type *o_ptr = &attacker_ptr->inventory_list[INVEN_MAIN_HAND + pa_ptr->hand];
    HIT_PROB to_h = o_ptr->to_h;
    HIT_POINT to_d = o_ptr->to_d;
    bool flag = TRUE;
    for (int i = 0; i < to_h + 3; i++)
        if (one_in_(4))
            flag = FALSE;

    if (flag)
        to_h++;

    flag = TRUE;
    for (int i = 0; i < to_d + 3; i++)
        if (one_in_(4))
            flag = FALSE;

    if (flag)
        to_d++;

    if ((o_ptr->to_h == to_h) && (o_ptr->to_d == to_d))
        return;

    msg_print(_("妖刀は血を吸って強くなった！", "Muramasa sucked blood, and became more powerful!"));
    o_ptr->to_h = to_h;
    o_ptr->to_d = to_d;
}

/*!
 * @brief 吸血武器による吸血処理
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param drain_msg 吸血をした旨のメッセージを表示するかどうか
 * @return なし
 * @details 1行目の5がマジックナンバーで良く分からなかったので、取り敢えず元々あったコメントをベースに定数宣言しておいた
 */
static void drain_result(player_type *attacker_ptr, player_attack_type *pa_ptr, bool *drain_msg)
{
    const int real_drain = 5;
    if (pa_ptr->drain_result <= real_drain)
        return;

    int drain_heal = damroll(2, pa_ptr->drain_result / 6);

    if (hex_spelling(attacker_ptr, HEX_VAMP_BLADE))
        drain_heal *= 2;

    if (cheat_xtra) {
        msg_format(_("Draining left: %d", "Draining left: %d"), pa_ptr->drain_left);
    }

    if (pa_ptr->drain_left == 0)
        return;

    if (drain_heal < pa_ptr->drain_left) {
        pa_ptr->drain_left -= drain_heal;
    } else {
        drain_heal = pa_ptr->drain_left;
        pa_ptr->drain_left = 0;
    }

    if (*drain_msg) {
        msg_format(_("刃が%sから生命力を吸い取った！", "Your weapon drains life from %s!"), pa_ptr->m_name);
        *drain_msg = FALSE;
    }

    drain_heal = (drain_heal * attacker_ptr->mutant_regenerate_mod) / 100;
    hp_player(attacker_ptr, drain_heal);
}

/*!
 * @brief 吸血処理のメインルーチン
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @param is_human 人間かどうか(村正用フラグ)
 * @param drain_msg 吸血をした旨のメッセージを表示するかどうか
 * @return なし
 * @details モンスターが死んだ場合、(ゲームのフレーバー的に)吸血しない
 */
void process_drain(player_type *attacker_ptr, player_attack_type *pa_ptr, const bool is_human, bool *drain_msg)
{
    if (!pa_ptr->can_drain || (pa_ptr->drain_result <= 0))
        return;

    object_type *o_ptr = &attacker_ptr->inventory_list[INVEN_MAIN_HAND + pa_ptr->hand];
    if (o_ptr->name1 == ART_MURAMASA)
        drain_muramasa(attacker_ptr, pa_ptr, is_human);
    else
        drain_result(attacker_ptr, pa_ptr, drain_msg);

    pa_ptr->m_ptr->maxhp -= (pa_ptr->attack_damage + 7) / 8;
    if (pa_ptr->m_ptr->hp > pa_ptr->m_ptr->maxhp)
        pa_ptr->m_ptr->hp = pa_ptr->m_ptr->maxhp;

    if (pa_ptr->m_ptr->maxhp < 1)
        pa_ptr->m_ptr->maxhp = 1;

    pa_ptr->weak = TRUE;
}
