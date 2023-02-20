/*!
 * @brief プレイヤーのHP/MP、アイテム、お金・明かりの残りターン、充填魔力を盗んだり減少させたりする処理
 * @date 2020/05/31
 * @author Hourier
 */

#include "monster-attack/monster-eating.h"
#include "avatar/avatar.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "mind/mind-mirror-master.h"
#include "monster-attack/monster-attack-player.h"
#include "monster/monster-status.h"
#include "object/object-info.h"
#include "object/object-mark-types.h"
#include "player-base/player-race.h"
#include "player-info/race-info.h"
#include "player/digestion-processor.h"
#include "player/player-status-flags.h"
#include "player/player-status-table.h"
#include "status/experience.h"
#include "system/baseitem-info.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "timed-effect/player-blindness.h"
#include "timed-effect/player-paralysis.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world-object.h"

void process_eat_gold(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    auto is_paralyzed = player_ptr->effects()->paralysis()->is_paralyzed();
    if (!is_paralyzed && (randint0(100) < (adj_dex_safe[player_ptr->stat_index[A_DEX]] + player_ptr->lev))) {
        msg_print(_("しかし素早く財布を守った！", "You quickly protect your money pouch!"));
        if (randint0(3)) {
            monap_ptr->blinked = true;
        }

        return;
    }

    PRICE gold = (player_ptr->au / 10) + randint1(25);
    if (gold < 2) {
        gold = 2;
    }

    if (gold > 5000) {
        gold = (player_ptr->au / 20) + randint1(3000);
    }

    if (gold > player_ptr->au) {
        gold = player_ptr->au;
    }

    player_ptr->au -= gold;
    if (gold <= 0) {
        msg_print(_("しかし何も盗まれなかった。", "Nothing was stolen."));
    } else if (player_ptr->au > 0) {
        msg_print(_("財布が軽くなった気がする。", "Your purse feels lighter."));
        msg_format(_("$%ld のお金が盗まれた！", "%ld coins were stolen!"), (long)gold);
        chg_virtue(player_ptr, V_SACRIFICE, 1);
    } else {
        msg_print(_("財布が軽くなった気がする。", "Your purse feels lighter."));
        msg_print(_("お金が全部盗まれた！", "All of your coins were stolen!"));
        chg_virtue(player_ptr, V_SACRIFICE, 2);
    }

    player_ptr->redraw |= (PR_GOLD);
    player_ptr->window_flags |= (PW_PLAYER);
    monap_ptr->blinked = true;
}

/*!
 * @brief 盗み打撃の時にアイテムが盗まれるかどうかを判定する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @monap_ptr モンスターからモンスターへの直接攻撃構造体への参照ポインタ
 * @return 盗まれたらTRUE、何も盗まれなかったらFALSE
 */
bool check_eat_item(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    if (monap_ptr->m_ptr->is_confused()) {
        return false;
    }

    if (player_ptr->is_dead || check_multishadow(player_ptr)) {
        return false;
    }

    auto is_paralyzed = player_ptr->effects()->paralysis()->is_paralyzed();
    if (!is_paralyzed && (randint0(100) < (adj_dex_safe[player_ptr->stat_index[A_DEX]] + player_ptr->lev))) {
        msg_print(_("しかしあわててザックを取り返した！", "You grab hold of your backpack!"));
        monap_ptr->blinked = true;
        monap_ptr->obvious = true;
        return false;
    }

    return true;
}

/*!
 * @brief プレイヤーが持っているアイテムをモンスターに移す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @monap_ptr モンスターからモンスターへの直接攻撃構造体への参照ポインタ
 */
static void move_item_to_monster(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr, const OBJECT_IDX o_idx)
{
    if (o_idx == 0) {
        return;
    }

    auto *j_ptr = &player_ptr->current_floor_ptr->o_list[o_idx];
    j_ptr->copy_from(monap_ptr->o_ptr);
    j_ptr->number = 1;
    if (monap_ptr->o_ptr->is_wand_rod()) {
        j_ptr->pval = monap_ptr->o_ptr->pval / monap_ptr->o_ptr->number;
        monap_ptr->o_ptr->pval -= j_ptr->pval;
    }

    j_ptr->marked.clear().set(OmType::TOUCHED);
    j_ptr->held_m_idx = monap_ptr->m_idx;
    monap_ptr->m_ptr->hold_o_idx_list.add(player_ptr->current_floor_ptr, o_idx);
}

/*!
 * @brief アイテム盗み処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @monap_ptr モンスターからモンスターへの直接攻撃構造体への参照ポインタ
 * @details eatとあるがお金や食べ物と違ってなくならない、盗んだモンスターを倒せば取り戻せる
 */
void process_eat_item(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    for (int i = 0; i < 10; i++) {
        OBJECT_IDX o_idx;
        INVENTORY_IDX i_idx = (INVENTORY_IDX)randint0(INVEN_PACK);
        monap_ptr->o_ptr = &player_ptr->inventory_list[i_idx];
        if (!monap_ptr->o_ptr->bi_id) {
            continue;
        }

        if (monap_ptr->o_ptr->is_fixed_or_random_artifact()) {
            continue;
        }

        describe_flavor(player_ptr, monap_ptr->o_name, monap_ptr->o_ptr, OD_OMIT_PREFIX);
#ifdef JP
        msg_format("%s(%c)を%s盗まれた！", monap_ptr->o_name, index_to_label(i_idx), ((monap_ptr->o_ptr->number > 1) ? "一つ" : ""));
#else
        msg_format("%sour %s (%c) was stolen!", ((monap_ptr->o_ptr->number > 1) ? "One of y" : "Y"), monap_ptr->o_name, index_to_label(i_idx));
#endif
        chg_virtue(player_ptr, V_SACRIFICE, 1);
        o_idx = o_pop(player_ptr->current_floor_ptr);
        move_item_to_monster(player_ptr, monap_ptr, o_idx);
        inven_item_increase(player_ptr, i_idx, -1);
        inven_item_optimize(player_ptr, i_idx);
        monap_ptr->obvious = true;
        monap_ptr->blinked = true;
        break;
    }
}

void process_eat_food(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    for (int i = 0; i < 10; i++) {
        INVENTORY_IDX i_idx = (INVENTORY_IDX)randint0(INVEN_PACK);
        monap_ptr->o_ptr = &player_ptr->inventory_list[i_idx];
        if (!monap_ptr->o_ptr->bi_id) {
            continue;
        }

        const auto tval = monap_ptr->o_ptr->bi_key.tval();
        if ((tval != ItemKindType::FOOD) && !((tval == ItemKindType::CORPSE) && (monap_ptr->o_ptr->bi_key.sval() != 0))) {
            continue;
        }

        describe_flavor(player_ptr, monap_ptr->o_name, monap_ptr->o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
#ifdef JP
        msg_format("%s(%c)を%s食べられてしまった！", monap_ptr->o_name, index_to_label(i_idx), ((monap_ptr->o_ptr->number > 1) ? "一つ" : ""));
#else
        msg_format("%sour %s (%c) was eaten!", ((monap_ptr->o_ptr->number > 1) ? "One of y" : "Y"), monap_ptr->o_name, index_to_label(i_idx));
#endif
        inven_item_increase(player_ptr, i_idx, -1);
        inven_item_optimize(player_ptr, i_idx);
        monap_ptr->obvious = true;
        break;
    }
}

void process_eat_lite(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    if ((monap_ptr->o_ptr->fuel <= 0) || monap_ptr->o_ptr->is_fixed_artifact()) {
        return;
    }

    monap_ptr->o_ptr->fuel -= 250 + randint1(250);
    if (monap_ptr->o_ptr->fuel < 1) {
        monap_ptr->o_ptr->fuel = 1;
    }

    if (!player_ptr->effects()->blindness()->is_blind()) {
        msg_print(_("明かりが暗くなってしまった。", "Your light dims."));
        monap_ptr->obvious = true;
    }

    player_ptr->window_flags |= (PW_EQUIP);
}

/*!
 * @brief モンスターからの攻撃による充填魔力吸収処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @monap_ptr モンスターからモンスターへの直接攻撃構造体への参照ポインタ
 * @return 吸収されたらTRUE、されなかったらFALSE
 * @details 魔道具使用能力向上フラグがあれば、吸収量は全部ではない
 * 詳細はOSDN #40911の議論を参照のこと
 */
bool process_un_power(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    if (!monap_ptr->o_ptr->is_wand_staff() || (monap_ptr->o_ptr->pval == 0)) {
        return false;
    }

    const auto is_magic_mastery = has_magic_mastery(player_ptr) != 0;
    const auto &baseitem = baseitems_info[monap_ptr->o_ptr->bi_id];
    const auto pval = baseitem.pval;
    const auto level = monap_ptr->rlev;
    auto drain = is_magic_mastery ? std::min<short>(pval, pval * level / 400 + pval * randint1(level) / 400) : pval;
    drain = std::min(drain, monap_ptr->o_ptr->pval);
    if (drain <= 0) {
        return false;
    }

    msg_print(_("ザックからエネルギーが吸い取られた！", "Energy was drained from your pack!"));
    if (is_magic_mastery && (drain != monap_ptr->o_ptr->pval)) {
        msg_print(_("しかし、あなたの魔法を操る力がその一部を取り返した！", "However, your skill of magic mastery got back the part of energy!"));
    }

    monap_ptr->obvious = true;
    auto recovery = drain * baseitem.level;
    const auto tval = monap_ptr->o_ptr->bi_key.tval();
    if (tval == ItemKindType::STAFF) {
        recovery *= monap_ptr->o_ptr->number;
    }

    recovery = std::min(recovery, monap_ptr->m_ptr->maxhp - monap_ptr->m_ptr->hp);
    monap_ptr->m_ptr->hp += recovery;

    if (player_ptr->health_who == monap_ptr->m_idx) {
        player_ptr->redraw |= PR_HEALTH;
    }

    if (player_ptr->riding == monap_ptr->m_idx) {
        player_ptr->redraw |= PR_UHEALTH;
    }

    monap_ptr->o_ptr->pval = !is_magic_mastery || (monap_ptr->o_ptr->pval == 1) ? 0 : monap_ptr->o_ptr->pval - drain;
    player_ptr->update |= PU_COMBINE | PU_REORDER;
    player_ptr->window_flags |= PW_INVEN;
    return true;
}

bool check_drain_hp(PlayerType *player_ptr, const int32_t d)
{
    bool resist_drain = !drain_exp(player_ptr, d, d / 10, 50);
    if (player_ptr->mimic_form != MimicKindType::NONE) {
        return PlayerRace(player_ptr).is_mimic_nonliving() ? true : resist_drain;
    }

    switch (player_ptr->prace) {
    case PlayerRaceType::ZOMBIE:
    case PlayerRaceType::VAMPIRE:
    case PlayerRaceType::SPECTRE:
    case PlayerRaceType::SKELETON:
    case PlayerRaceType::BALROG:
    case PlayerRaceType::GOLEM:
    case PlayerRaceType::ANDROID:
        return true;
    default:
        return resist_drain;
    }
}

void process_drain_life(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr, const bool resist_drain)
{
    if ((monap_ptr->damage <= 5) || resist_drain) {
        return;
    }

    bool did_heal = monap_ptr->m_ptr->hp < monap_ptr->m_ptr->maxhp;
    monap_ptr->m_ptr->hp += damroll(4, monap_ptr->damage / 6);
    if (monap_ptr->m_ptr->hp > monap_ptr->m_ptr->maxhp) {
        monap_ptr->m_ptr->hp = monap_ptr->m_ptr->maxhp;
    }

    if (player_ptr->health_who == monap_ptr->m_idx) {
        player_ptr->redraw |= (PR_HEALTH);
    }

    if (player_ptr->riding == monap_ptr->m_idx) {
        player_ptr->redraw |= (PR_UHEALTH);
    }

    if (monap_ptr->m_ptr->ml && did_heal) {
        msg_format(_("%sは体力を回復したようだ。", "%s^ appears healthier."), monap_ptr->m_name);
    }
}

void process_drain_mana(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    if (check_multishadow(player_ptr)) {
        msg_print(_("攻撃は幻影に命中し、あなたには届かなかった。", "The attack hits Shadow, but you are unharmed!"));
        return;
    }

    monap_ptr->do_cut = 0;
    player_ptr->csp -= monap_ptr->damage;
    if (player_ptr->csp < 0) {
        player_ptr->csp = 0;
        player_ptr->csp_frac = 0;
    }

    player_ptr->redraw |= (PR_MANA);
}

/*!
 * @brief モンスターからの空腹進行処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @monap_ptr モンスターからモンスターへの直接攻撃構造体への参照ポインタ
 * @details 空腹、衰弱の一歩手前で止める優しさは残す。
 */
void process_monster_attack_hungry(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    msg_format(_("あなたは腹が減った！", "You feel hungry!"));
    auto subtracted_food = static_cast<int16_t>(player_ptr->food - monap_ptr->damage);
    if ((player_ptr->food >= PY_FOOD_ALERT) && (PY_FOOD_ALERT > subtracted_food)) {
        set_food(player_ptr, PY_FOOD_ALERT - 1);
    } else if ((player_ptr->food > PY_FOOD_FAINT) && (PY_FOOD_FAINT >= subtracted_food)) {
        set_food(player_ptr, PY_FOOD_FAINT);
    } else {
        set_food(player_ptr, subtracted_food);
    }
}
