/*!
 * @brief プレーヤーのアイテム・お金・明かりの残りターンを盗んだり減少させたりする処理
 * @date 2020/05/31
 * @author Hourier
 */

#include "combat/monster-eating.h"
#include "floor/floor.h"
#include "mind/racial-mirror-master.h"
#include "object/object2.h"
#include "object/object-flavor.h"
#include "object/object-hook.h"
#include "object/object-mark-types.h"
#include "player/avatar.h"

void process_eat_gold(player_type *target_ptr, monap_type *monap_ptr)
{
    if (!target_ptr->paralyzed && (randint0(100) < (adj_dex_safe[target_ptr->stat_ind[A_DEX]] + target_ptr->lev))) {
        msg_print(_("しかし素早く財布を守った！", "You quickly protect your money pouch!"));
        if (randint0(3))
            monap_ptr->blinked = TRUE;

        return;
    }

    PRICE gold = (target_ptr->au / 10) + randint1(25);
    if (gold < 2)
        gold = 2;

    if (gold > 5000)
        gold = (target_ptr->au / 20) + randint1(3000);

    if (gold > target_ptr->au)
        gold = target_ptr->au;

    target_ptr->au -= gold;
    if (gold <= 0) {
        msg_print(_("しかし何も盗まれなかった。", "Nothing was stolen."));
    } else if (target_ptr->au > 0) {
        msg_print(_("財布が軽くなった気がする。", "Your purse feels lighter."));
        msg_format(_("$%ld のお金が盗まれた！", "%ld coins were stolen!"), (long)gold);
        chg_virtue(target_ptr, V_SACRIFICE, 1);
    } else {
        msg_print(_("財布が軽くなった気がする。", "Your purse feels lighter."));
        msg_print(_("お金が全部盗まれた！", "All of your coins were stolen!"));
        chg_virtue(target_ptr, V_SACRIFICE, 2);
    }

    target_ptr->redraw |= (PR_GOLD);
    target_ptr->window |= (PW_PLAYER);
    monap_ptr->blinked = TRUE;
}

/*!
 * @brief 盗み打撃の時にアイテムが盗まれるかどうかを判定する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @monap_ptr モンスターからモンスターへの直接攻撃構造体への参照ポインタ
 * @return 盗まれたらTRUE、何も盗まれなかったらFALSE
 */
bool check_eat_item(player_type *target_ptr, monap_type *monap_ptr)
{
    if (MON_CONFUSED(monap_ptr->m_ptr))
        return FALSE;

    if (target_ptr->is_dead || check_multishadow(target_ptr))
        return FALSE;

    if (!target_ptr->paralyzed && (randint0(100) < (adj_dex_safe[target_ptr->stat_ind[A_DEX]] + target_ptr->lev))) {
        msg_print(_("しかしあわててザックを取り返した！", "You grab hold of your backpack!"));
        monap_ptr->blinked = TRUE;
        monap_ptr->obvious = TRUE;
        return FALSE;
    }

    return TRUE;
}

/*!
 * @brief プレーヤーが持っているアイテムをモンスターに移す
 * @param target_ptr プレーヤーへの参照ポインタ
 * @monap_ptr モンスターからモンスターへの直接攻撃構造体への参照ポインタ
 * @return なし
 */
static void move_item_to_monster(player_type *target_ptr, monap_type *monap_ptr, const OBJECT_IDX o_idx)
{
    if (o_idx == 0)
        return;

    object_type *j_ptr;
    j_ptr = &target_ptr->current_floor_ptr->o_list[o_idx];
    object_copy(j_ptr, monap_ptr->o_ptr);
    j_ptr->number = 1;
    if ((monap_ptr->o_ptr->tval == TV_ROD) || (monap_ptr->o_ptr->tval == TV_WAND)) {
        j_ptr->pval = monap_ptr->o_ptr->pval / monap_ptr->o_ptr->number;
        monap_ptr->o_ptr->pval -= j_ptr->pval;
    }

    j_ptr->marked = OM_TOUCHED;
    j_ptr->held_m_idx = monap_ptr->m_idx;
    j_ptr->next_o_idx = monap_ptr->m_ptr->hold_o_idx;
    monap_ptr->m_ptr->hold_o_idx = o_idx;
}

/*!
 * @brief アイテム盗み処理
 * @param target_ptr プレーヤーへの参照ポインタ
 * @monap_ptr モンスターからモンスターへの直接攻撃構造体への参照ポインタ
 * @return なし
 * @details eatとあるがお金や食べ物と違ってなくならない、盗んだモンスターを倒せば取り戻せる
 */
void process_eat_item(player_type *target_ptr, monap_type *monap_ptr)
{
    for (int i = 0; i < 10; i++) {
        OBJECT_IDX o_idx;
        INVENTORY_IDX i_idx = (INVENTORY_IDX)randint0(INVEN_PACK);
        monap_ptr->o_ptr = &target_ptr->inventory_list[i_idx];
        if (!monap_ptr->o_ptr->k_idx)
            continue;

        if (object_is_artifact(monap_ptr->o_ptr))
            continue;

        object_desc(target_ptr, monap_ptr->o_name, monap_ptr->o_ptr, OD_OMIT_PREFIX);
#ifdef JP
        msg_format("%s(%c)を%s盗まれた！", monap_ptr->o_name, index_to_label(i_idx), ((monap_ptr->o_ptr->number > 1) ? "一つ" : ""));
#else
        msg_format("%sour %s (%c) was stolen!", ((o_ptr->number > 1) ? "One of y" : "Y"), monap_ptr->o_name, index_to_label(i_idx));
#endif
        chg_virtue(target_ptr, V_SACRIFICE, 1);
        o_idx = o_pop(target_ptr->current_floor_ptr);
        move_item_to_monster(target_ptr, monap_ptr, o_idx);
        inven_item_increase(target_ptr, i_idx, -1);
        inven_item_optimize(target_ptr, i_idx);
        monap_ptr->obvious = TRUE;
        monap_ptr->blinked = TRUE;
        break;
    }
}

void process_eat_food(player_type *target_ptr, monap_type *monap_ptr)
{
    for (int i = 0; i < 10; i++) {
        INVENTORY_IDX i_idx = (INVENTORY_IDX)randint0(INVEN_PACK);
        monap_ptr->o_ptr = &target_ptr->inventory_list[i_idx];
        if (!monap_ptr->o_ptr->k_idx)
            continue;

        if ((monap_ptr->o_ptr->tval != TV_FOOD) && !((monap_ptr->o_ptr->tval == TV_CORPSE) && (monap_ptr->o_ptr->sval)))
            continue;

        object_desc(target_ptr, monap_ptr->o_name, monap_ptr->o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
#ifdef JP
        msg_format("%s(%c)を%s食べられてしまった！", monap_ptr->o_name, index_to_label(i_idx), ((monap_ptr->o_ptr->number > 1) ? "一つ" : ""));
#else
        msg_format("%sour %s (%c) was eaten!", ((o_ptr->number > 1) ? "One of y" : "Y"), monap_ptr->o_name, index_to_label(i_idx));
#endif
        inven_item_increase(target_ptr, i_idx, -1);
        inven_item_optimize(target_ptr, i_idx);
        monap_ptr->obvious = TRUE;
        break;
    }
}

void process_eat_lite(player_type *target_ptr, monap_type *monap_ptr)
{
    if ((monap_ptr->o_ptr->xtra4 <= 0) || object_is_fixed_artifact(monap_ptr->o_ptr))
        return;

    monap_ptr->o_ptr->xtra4 -= (s16b)(250 + randint1(250));
    if (monap_ptr->o_ptr->xtra4 < 1)
        monap_ptr->o_ptr->xtra4 = 1;

    if (!target_ptr->blind) {
        msg_print(_("明かりが暗くなってしまった。", "Your light dims."));
        monap_ptr->obvious = TRUE;
    }

    target_ptr->window |= (PW_EQUIP);
}
