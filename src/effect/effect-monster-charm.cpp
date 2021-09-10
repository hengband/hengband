#include "effect/effect-monster-charm.h"
#include "avatar/avatar.h"
#include "dungeon/quest.h"
#include "effect/effect-monster-util.h"
#include "effect/spells-effect-util.h"
#include "monster-floor/monster-remover.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "object-enchant/trc-types.h"
#include "pet/pet-fall-off.h"
#include "pet/pet-util.h"
#include "player/player-status-flags.h"
#include "spell/spells-diceroll.h"
#include "status/bad-status-setter.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

static void effect_monster_charm_resist(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (common_saving_throw_charm(player_ptr, em_ptr->dam, em_ptr->m_ptr)) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->obvious = false;

        if (one_in_(4))
            em_ptr->m_ptr->mflag2.set(MFLAG2::NOPET);
    } else if (has_aggravate(player_ptr)) {
        em_ptr->note = _("はあなたに敵意を抱いている！", " hates you too much!");
        if (one_in_(4))
            em_ptr->m_ptr->mflag2.set(MFLAG2::NOPET);
    } else {
        em_ptr->note = _("は突然友好的になったようだ！", " suddenly seems friendly!");
        set_pet(player_ptr, em_ptr->m_ptr);

        chg_virtue(player_ptr, V_INDIVIDUALISM, -1);
        if (em_ptr->r_ptr->flags3 & RF3_ANIMAL)
            chg_virtue(player_ptr, V_NATURE, 1);
    }
}

process_result effect_monster_charm(player_type *player_ptr, effect_monster_type *em_ptr)
{
    int vir = virtue_number(player_ptr, V_HARMONY);
    if (vir) {
        em_ptr->dam += player_ptr->virtues[vir - 1] / 10;
    }

    vir = virtue_number(player_ptr, V_INDIVIDUALISM);
    if (vir) {
        em_ptr->dam -= player_ptr->virtues[vir - 1] / 20;
    }

    if (em_ptr->seen)
        em_ptr->obvious = true;

    effect_monster_charm_resist(player_ptr, em_ptr);
    em_ptr->dam = 0;
    return PROCESS_CONTINUE;
}

process_result effect_monster_control_undead(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    int vir = virtue_number(player_ptr, V_UNLIFE);
    if (vir) {
        em_ptr->dam += player_ptr->virtues[vir - 1] / 10;
    }

    vir = virtue_number(player_ptr, V_INDIVIDUALISM);
    if (vir) {
        em_ptr->dam -= player_ptr->virtues[vir - 1] / 20;
    }

    if (common_saving_throw_control(player_ptr, em_ptr->dam, em_ptr->m_ptr) || !(em_ptr->r_ptr->flags3 & RF3_UNDEAD)) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->obvious = false;
        if (one_in_(4))
            em_ptr->m_ptr->mflag2.set(MFLAG2::NOPET);
    } else if (has_aggravate(player_ptr)) {
        em_ptr->note = _("はあなたに敵意を抱いている！", " hates you too much!");
        if (one_in_(4))
            em_ptr->m_ptr->mflag2.set(MFLAG2::NOPET);
    } else {
        em_ptr->note = _("は既にあなたの奴隷だ！", " is in your thrall!");
        set_pet(player_ptr, em_ptr->m_ptr);
    }

    em_ptr->dam = 0;
    return PROCESS_CONTINUE;
}

process_result effect_monster_control_demon(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    int vir = virtue_number(player_ptr, V_UNLIFE);
    if (vir) {
        em_ptr->dam += player_ptr->virtues[vir - 1] / 10;
    }

    vir = virtue_number(player_ptr, V_INDIVIDUALISM);
    if (vir) {
        em_ptr->dam -= player_ptr->virtues[vir - 1] / 20;
    }

    if (common_saving_throw_control(player_ptr, em_ptr->dam, em_ptr->m_ptr) || !(em_ptr->r_ptr->flags3 & RF3_DEMON)) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->obvious = false;
        if (one_in_(4))
            em_ptr->m_ptr->mflag2.set(MFLAG2::NOPET);
    } else if (has_aggravate(player_ptr)) {
        em_ptr->note = _("はあなたに敵意を抱いている！", " hates you too much!");
        if (one_in_(4))
            em_ptr->m_ptr->mflag2.set(MFLAG2::NOPET);
    } else {
        em_ptr->note = _("は既にあなたの奴隷だ！", " is in your thrall!");
        set_pet(player_ptr, em_ptr->m_ptr);
    }

    em_ptr->dam = 0;
    return PROCESS_CONTINUE;
}

process_result effect_monster_control_animal(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;

    int vir = virtue_number(player_ptr, V_NATURE);
    if (vir) {
        em_ptr->dam += player_ptr->virtues[vir - 1] / 10;
    }

    vir = virtue_number(player_ptr, V_INDIVIDUALISM);
    if (vir) {
        em_ptr->dam -= player_ptr->virtues[vir - 1] / 20;
    }

    if (common_saving_throw_control(player_ptr, em_ptr->dam, em_ptr->m_ptr) || !(em_ptr->r_ptr->flags3 & RF3_ANIMAL)) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->obvious = false;
        if (one_in_(4))
            em_ptr->m_ptr->mflag2.set(MFLAG2::NOPET);
    } else if (has_aggravate(player_ptr)) {
        em_ptr->note = _("はあなたに敵意を抱いている！", " hates you too much!");
        if (one_in_(4))
            em_ptr->m_ptr->mflag2.set(MFLAG2::NOPET);
    } else {
        em_ptr->note = _("はなついた。", " is tamed!");
        set_pet(player_ptr, em_ptr->m_ptr);
        if (em_ptr->r_ptr->flags3 & RF3_ANIMAL)
            chg_virtue(player_ptr, V_NATURE, 1);
    }

    em_ptr->dam = 0;
    return PROCESS_CONTINUE;
}

process_result effect_monster_charm_living(player_type *player_ptr, effect_monster_type *em_ptr)
{
    int vir = virtue_number(player_ptr, V_UNLIFE);
    if (em_ptr->seen)
        em_ptr->obvious = true;

    vir = virtue_number(player_ptr, V_UNLIFE);
    if (vir) {
        em_ptr->dam -= player_ptr->virtues[vir - 1] / 10;
    }

    vir = virtue_number(player_ptr, V_INDIVIDUALISM);
    if (vir) {
        em_ptr->dam -= player_ptr->virtues[vir - 1] / 20;
    }

    msg_format(_("%sを見つめた。", "You stare at %s."), em_ptr->m_name);

    if (common_saving_throw_charm(player_ptr, em_ptr->dam, em_ptr->m_ptr) || !monster_living(em_ptr->m_ptr->r_idx)) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->obvious = false;
        if (one_in_(4))
            em_ptr->m_ptr->mflag2.set(MFLAG2::NOPET);
    } else if (has_aggravate(player_ptr)) {
        em_ptr->note = _("はあなたに敵意を抱いている！", " hates you too much!");
        if (one_in_(4))
            em_ptr->m_ptr->mflag2.set(MFLAG2::NOPET);
    } else {
        em_ptr->note = _("を支配した。", " is tamed!");
        set_pet(player_ptr, em_ptr->m_ptr);
        if (em_ptr->r_ptr->flags3 & RF3_ANIMAL)
            chg_virtue(player_ptr, V_NATURE, 1);
    }

    em_ptr->dam = 0;
    return PROCESS_CONTINUE;
}

static void effect_monster_domination_corrupted_addition(player_type *player_ptr, effect_monster_type *em_ptr)
{
    switch (randint1(4)) {
    case 1:
        set_stun(player_ptr, player_ptr->stun + em_ptr->dam / 2);
        break;
    case 2:
        set_confused(player_ptr, player_ptr->confused + em_ptr->dam / 2);
        break;
    default: {
        if (em_ptr->r_ptr->flags3 & RF3_NO_FEAR)
            em_ptr->note = _("には効果がなかった。", " is unaffected.");
        else
            set_afraid(player_ptr, player_ptr->afraid + em_ptr->dam);
    }
    }
}

// Powerful demons & undead can turn a mindcrafter's attacks back on them.
static void effect_monster_domination_corrupted(player_type *player_ptr, effect_monster_type *em_ptr)
{
    bool is_corrupted = ((em_ptr->r_ptr->flags3 & (RF3_UNDEAD | RF3_DEMON)) != 0) && (em_ptr->r_ptr->level > player_ptr->lev / 2) && (one_in_(2));
    if (!is_corrupted) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->obvious = false;
        return;
    }

    em_ptr->note = nullptr;
    msg_format(_("%^sの堕落した精神は攻撃を跳ね返した！",
                   (em_ptr->seen ? "%^s's corrupted mind backlashes your attack!" : "%^ss corrupted mind backlashes your attack!")),
        em_ptr->m_name);
    if (randint0(100 + em_ptr->r_ptr->level / 2) < player_ptr->skill_sav) {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
        return;
    }

    effect_monster_domination_corrupted_addition(player_ptr, em_ptr);
}

static void effect_monster_domination_addition(effect_monster_type *em_ptr)
{
    switch (randint1(4)) {
    case 1:
        em_ptr->do_stun = em_ptr->dam / 2;
        break;
    case 2:
        em_ptr->do_conf = em_ptr->dam / 2;
        break;
    default:
        em_ptr->do_fear = em_ptr->dam;
    }
}

process_result effect_monster_domination(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (!is_hostile(em_ptr->m_ptr))
        return PROCESS_CONTINUE;

    if (em_ptr->seen)
        em_ptr->obvious = true;

    if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) || (em_ptr->r_ptr->flags3 & RF3_NO_CONF)
        || (em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10)) {
        if (((em_ptr->r_ptr->flags3 & RF3_NO_CONF) != 0) && is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
            em_ptr->r_ptr->r_flags3 |= (RF3_NO_CONF);

        em_ptr->do_conf = 0;
        effect_monster_domination_corrupted(player_ptr, em_ptr);
        em_ptr->dam = 0;
        return PROCESS_CONTINUE;
    }

    if (!common_saving_throw_charm(player_ptr, em_ptr->dam, em_ptr->m_ptr)) {
        em_ptr->note = _("があなたに隷属した。", " is in your thrall!");
        set_pet(player_ptr, em_ptr->m_ptr);
        em_ptr->dam = 0;
        return PROCESS_CONTINUE;
    }

    effect_monster_domination_addition(em_ptr);
    em_ptr->dam = 0;
    return PROCESS_CONTINUE;
}

static bool effect_monster_crusade_domination(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (((em_ptr->r_ptr->flags3 & RF3_GOOD) == 0) || player_ptr->current_floor_ptr->inside_arena)
        return false;

    if (em_ptr->r_ptr->flags3 & RF3_NO_CONF)
        em_ptr->dam -= 50;
    if (em_ptr->dam < 1)
        em_ptr->dam = 1;

    if (is_pet(em_ptr->m_ptr)) {
        em_ptr->note = _("の動きが速くなった。", " starts moving faster.");
        (void)set_monster_fast(player_ptr, em_ptr->g_ptr->m_idx, monster_fast_remaining(em_ptr->m_ptr) + 100);
        return true;
    }

    if ((em_ptr->r_ptr->flags1 & RF1_QUESTOR) || (em_ptr->r_ptr->flags1 & RF1_UNIQUE) || em_ptr->m_ptr->mflag2.has(MFLAG2::NOPET) || has_aggravate(player_ptr)
        || ((em_ptr->r_ptr->level + 10) > randint1(em_ptr->dam))) {
        if (one_in_(4))
            em_ptr->m_ptr->mflag2.set(MFLAG2::NOPET);

        return false;
    }

    em_ptr->note = _("を支配した。", " is tamed!");
    set_pet(player_ptr, em_ptr->m_ptr);
    (void)set_monster_fast(player_ptr, em_ptr->g_ptr->m_idx, monster_fast_remaining(em_ptr->m_ptr) + 100);
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
        em_ptr->r_ptr->r_flags3 |= RF3_GOOD;

    return true;
}

process_result effect_monster_crusade(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen)
        em_ptr->obvious = true;
    bool success = effect_monster_crusade_domination(player_ptr, em_ptr);
    if (success) {
        em_ptr->dam = 0;
        return PROCESS_CONTINUE;
    }

    if ((em_ptr->r_ptr->flags3 & RF3_NO_FEAR) == 0)
        em_ptr->do_fear = randint1(90) + 10;
    else if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr))
        em_ptr->r_ptr->r_flags3 |= RF3_NO_FEAR;

    em_ptr->dam = 0;
    return PROCESS_CONTINUE;
}

/*!
 * @brief モンスターボールで捕まえられる最大HPを計算する
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param m_ptr モンスター情報への参照ポインタ
 * @param hp 計算対象のHP
 * @return 捕まえられる最大HP
 */
static HIT_POINT calcutate_capturable_hp(player_type *player_ptr, monster_type *m_ptr, HIT_POINT hp)
{
    if (is_pet(m_ptr))
        return hp * 4L;

    if ((player_ptr->pclass == CLASS_BEASTMASTER) && monster_living(m_ptr->r_idx))
        return hp * 3 / 10;

    return hp * 3 / 20;
}

/*!
 * @brief モンスターボールで捕らえた処理
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param em_ptr 効果情報への参照ポインタ
 */
static void effect_monster_captured(player_type *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->m_ptr->mflag2.has(MFLAG2::CHAMELEON))
        choose_new_monster(player_ptr, em_ptr->g_ptr->m_idx, false, MON_CHAMELEON);

    msg_format(_("%sを捕えた！", "You capture %^s!"), em_ptr->m_name);
    cap_mon = em_ptr->m_ptr->r_idx;
    cap_mspeed = em_ptr->m_ptr->mspeed;
    cap_hp = em_ptr->m_ptr->hp;
    cap_maxhp = em_ptr->m_ptr->max_maxhp;
    cap_nickname = em_ptr->m_ptr->nickname;
    if ((em_ptr->g_ptr->m_idx == player_ptr->riding) && process_fall_off_horse(player_ptr, -1, false))
        msg_format(_("地面に落とされた。", "You have fallen from %s."), em_ptr->m_name);

    delete_monster_idx(player_ptr, em_ptr->g_ptr->m_idx);
    calculate_upkeep(player_ptr);
}

/*!
 * @brief モンスターボールで捕らえる効果(GF_CAPTURE)
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param em_ptr 効果情報への参照ポインタ
 * @return 効果発動結果
 */
process_result effect_monster_capture(player_type *player_ptr, effect_monster_type *em_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if ((floor_ptr->inside_quest && (quest[floor_ptr->inside_quest].type == QUEST_TYPE_KILL_ALL) && !is_pet(em_ptr->m_ptr))
        || any_bits(em_ptr->r_ptr->flags1, RF1_UNIQUE | RF1_QUESTOR) || any_bits(em_ptr->r_ptr->flags7, RF7_NAZGUL | RF7_UNIQUE2)
        || em_ptr->m_ptr->parent_m_idx) {
        msg_format(_("%sには効果がなかった。", "%s is unaffected."), em_ptr->m_name);
        em_ptr->skipped = true;
        return PROCESS_CONTINUE;
    }

    auto r_max_hp = em_ptr->r_ptr->hdice * em_ptr->r_ptr->hside;
    auto threshold_hp = calcutate_capturable_hp(player_ptr, em_ptr->m_ptr, r_max_hp);
    auto capturable_hp = MAX(2, calcutate_capturable_hp(player_ptr, em_ptr->m_ptr, em_ptr->m_ptr->max_maxhp));

    if (threshold_hp < 2 || em_ptr->m_ptr->hp >= capturable_hp) {
        msg_format(_("もっと弱らせないと。", "You need to weaken %s more."), em_ptr->m_name);
        em_ptr->skipped = true;
        return PROCESS_CONTINUE;
    }

    if (em_ptr->m_ptr->hp <= randint1(capturable_hp)) {
        effect_monster_captured(player_ptr, em_ptr);
        return PROCESS_TRUE;
    }

    msg_format(_("うまく捕まえられなかった。", "You failed to capture %s."), em_ptr->m_name);
    em_ptr->skipped = true;
    return PROCESS_CONTINUE;
}
