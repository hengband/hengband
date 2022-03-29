#include "effect/effect-monster-charm.h"
#include "avatar/avatar.h"
#include "dungeon/quest.h"
#include "effect/effect-monster-util.h"
#include "effect/spells-effect-util.h"
#include "monster-floor/monster-remover.h"
#include "monster-race/monster-kind-mask.h"
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
#include "player-base/player-class.h"
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

static void effect_monster_charm_resist(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
    if (common_saving_throw_charm(player_ptr, em_ptr->dam, em_ptr->m_ptr)) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->obvious = false;

        if (one_in_(4)) {
            em_ptr->m_ptr->mflag2.set(MonsterConstantFlagType::NOPET);
        }
    } else if (has_aggravate(player_ptr)) {
        em_ptr->note = _("はあなたに敵意を抱いている！", " hates you too much!");
        if (one_in_(4)) {
            em_ptr->m_ptr->mflag2.set(MonsterConstantFlagType::NOPET);
        }
    } else {
        em_ptr->note = _("は突然友好的になったようだ！", " suddenly seems friendly!");
        set_pet(player_ptr, em_ptr->m_ptr);

        chg_virtue(player_ptr, V_INDIVIDUALISM, -1);
        if (em_ptr->r_ptr->kind_flags.has(MonsterKindType::ANIMAL)) {
            chg_virtue(player_ptr, V_NATURE, 1);
        }
    }
}

ProcessResult effect_monster_charm(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
    int vir = virtue_number(player_ptr, V_HARMONY);
    if (vir) {
        em_ptr->dam += player_ptr->virtues[vir - 1] / 10;
    }

    vir = virtue_number(player_ptr, V_INDIVIDUALISM);
    if (vir) {
        em_ptr->dam -= player_ptr->virtues[vir - 1] / 20;
    }

    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    effect_monster_charm_resist(player_ptr, em_ptr);
    em_ptr->dam = 0;
    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_control_undead(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    int vir = virtue_number(player_ptr, V_UNLIFE);
    if (vir) {
        em_ptr->dam += player_ptr->virtues[vir - 1] / 10;
    }

    vir = virtue_number(player_ptr, V_INDIVIDUALISM);
    if (vir) {
        em_ptr->dam -= player_ptr->virtues[vir - 1] / 20;
    }

    if (common_saving_throw_control(player_ptr, em_ptr->dam, em_ptr->m_ptr) || em_ptr->r_ptr->kind_flags.has_not(MonsterKindType::UNDEAD)) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->obvious = false;
        if (one_in_(4)) {
            em_ptr->m_ptr->mflag2.set(MonsterConstantFlagType::NOPET);
        }
    } else if (has_aggravate(player_ptr)) {
        em_ptr->note = _("はあなたに敵意を抱いている！", " hates you too much!");
        if (one_in_(4)) {
            em_ptr->m_ptr->mflag2.set(MonsterConstantFlagType::NOPET);
        }
    } else {
        em_ptr->note = _("は既にあなたの奴隷だ！", " is in your thrall!");
        set_pet(player_ptr, em_ptr->m_ptr);
    }

    em_ptr->dam = 0;
    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_control_demon(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    int vir = virtue_number(player_ptr, V_UNLIFE);
    if (vir) {
        em_ptr->dam += player_ptr->virtues[vir - 1] / 10;
    }

    vir = virtue_number(player_ptr, V_INDIVIDUALISM);
    if (vir) {
        em_ptr->dam -= player_ptr->virtues[vir - 1] / 20;
    }

    if (common_saving_throw_control(player_ptr, em_ptr->dam, em_ptr->m_ptr) || em_ptr->r_ptr->kind_flags.has_not(MonsterKindType::DEMON)) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->obvious = false;
        if (one_in_(4)) {
            em_ptr->m_ptr->mflag2.set(MonsterConstantFlagType::NOPET);
        }
    } else if (has_aggravate(player_ptr)) {
        em_ptr->note = _("はあなたに敵意を抱いている！", " hates you too much!");
        if (one_in_(4)) {
            em_ptr->m_ptr->mflag2.set(MonsterConstantFlagType::NOPET);
        }
    } else {
        em_ptr->note = _("は既にあなたの奴隷だ！", " is in your thrall!");
        set_pet(player_ptr, em_ptr->m_ptr);
    }

    em_ptr->dam = 0;
    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_control_animal(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    int vir = virtue_number(player_ptr, V_NATURE);
    if (vir) {
        em_ptr->dam += player_ptr->virtues[vir - 1] / 10;
    }

    vir = virtue_number(player_ptr, V_INDIVIDUALISM);
    if (vir) {
        em_ptr->dam -= player_ptr->virtues[vir - 1] / 20;
    }

    if (common_saving_throw_control(player_ptr, em_ptr->dam, em_ptr->m_ptr) || em_ptr->r_ptr->kind_flags.has_not(MonsterKindType::ANIMAL)) {
        em_ptr->note = _("には効果がなかった。", " is unaffected.");
        em_ptr->obvious = false;
        if (one_in_(4)) {
            em_ptr->m_ptr->mflag2.set(MonsterConstantFlagType::NOPET);
        }
    } else if (has_aggravate(player_ptr)) {
        em_ptr->note = _("はあなたに敵意を抱いている！", " hates you too much!");
        if (one_in_(4)) {
            em_ptr->m_ptr->mflag2.set(MonsterConstantFlagType::NOPET);
        }
    } else {
        em_ptr->note = _("はなついた。", " is tamed!");
        set_pet(player_ptr, em_ptr->m_ptr);
        if (em_ptr->r_ptr->kind_flags.has(MonsterKindType::ANIMAL)) {
            chg_virtue(player_ptr, V_NATURE, 1);
        }
    }

    em_ptr->dam = 0;
    return ProcessResult::PROCESS_CONTINUE;
}

ProcessResult effect_monster_charm_living(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
    int vir = virtue_number(player_ptr, V_UNLIFE);
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

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
        if (one_in_(4)) {
            em_ptr->m_ptr->mflag2.set(MonsterConstantFlagType::NOPET);
        }
    } else if (has_aggravate(player_ptr)) {
        em_ptr->note = _("はあなたに敵意を抱いている！", " hates you too much!");
        if (one_in_(4)) {
            em_ptr->m_ptr->mflag2.set(MonsterConstantFlagType::NOPET);
        }
    } else {
        em_ptr->note = _("を支配した。", " is tamed!");
        set_pet(player_ptr, em_ptr->m_ptr);
        if (em_ptr->r_ptr->kind_flags.has(MonsterKindType::ANIMAL)) {
            chg_virtue(player_ptr, V_NATURE, 1);
        }
    }

    em_ptr->dam = 0;
    return ProcessResult::PROCESS_CONTINUE;
}

static void effect_monster_domination_corrupted_addition(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
    BadStatusSetter bss(player_ptr);
    switch (randint1(4)) {
    case 1:
        (void)bss.mod_stun(em_ptr->dam / 2);
        return;
    case 2:
        (void)bss.mod_confusion(em_ptr->dam / 2);
        return;
    default:
        if (any_bits(em_ptr->r_ptr->flags3, RF3_NO_FEAR)) {
            em_ptr->note = _("には効果がなかった。", " is unaffected.");
        } else {
            (void)bss.mod_fear(static_cast<TIME_EFFECT>(em_ptr->dam));
        }

        return;
    }
}

// Powerful demons & undead can turn a mindcrafter's attacks back on them.
static void effect_monster_domination_corrupted(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
    bool is_corrupted = em_ptr->r_ptr->kind_flags.has_any_of(has_corrupted_mind) && (em_ptr->r_ptr->level > player_ptr->lev / 2) && (one_in_(2));
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

ProcessResult effect_monster_domination(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
    if (!is_hostile(em_ptr->m_ptr)) {
        return ProcessResult::PROCESS_CONTINUE;
    }

    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    if (em_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE) || any_bits(em_ptr->r_ptr->flags1, RF1_QUESTOR) || (em_ptr->r_ptr->flags3 & RF3_NO_CONF) || (em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10)) {
        if (((em_ptr->r_ptr->flags3 & RF3_NO_CONF) != 0) && is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
            em_ptr->r_ptr->r_flags3 |= (RF3_NO_CONF);
        }

        em_ptr->do_conf = 0;
        effect_monster_domination_corrupted(player_ptr, em_ptr);
        em_ptr->dam = 0;
        return ProcessResult::PROCESS_CONTINUE;
    }

    if (!common_saving_throw_charm(player_ptr, em_ptr->dam, em_ptr->m_ptr)) {
        em_ptr->note = _("があなたに隷属した。", " is in your thrall!");
        set_pet(player_ptr, em_ptr->m_ptr);
        em_ptr->dam = 0;
        return ProcessResult::PROCESS_CONTINUE;
    }

    effect_monster_domination_addition(em_ptr);
    em_ptr->dam = 0;
    return ProcessResult::PROCESS_CONTINUE;
}

static bool effect_monster_crusade_domination(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
    if ((em_ptr->r_ptr->kind_flags.has_not(MonsterKindType::GOOD)) || player_ptr->current_floor_ptr->inside_arena) {
        return false;
    }

    if (em_ptr->r_ptr->flags3 & RF3_NO_CONF) {
        em_ptr->dam -= 50;
    }
    if (em_ptr->dam < 1) {
        em_ptr->dam = 1;
    }

    if (is_pet(em_ptr->m_ptr)) {
        em_ptr->note = _("の動きが速くなった。", " starts moving faster.");
        (void)set_monster_fast(player_ptr, em_ptr->g_ptr->m_idx, monster_fast_remaining(em_ptr->m_ptr) + 100);
        return true;
    }

    bool failed = any_bits(em_ptr->r_ptr->flags1, RF1_QUESTOR);
    failed |= em_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE);
    failed |= em_ptr->m_ptr->mflag2.has(MonsterConstantFlagType::NOPET);
    failed |= has_aggravate(player_ptr);
    failed |= (em_ptr->r_ptr->level + 10) > randint1(em_ptr->dam);

    if (failed) {
        if (one_in_(4)) {
            em_ptr->m_ptr->mflag2.set(MonsterConstantFlagType::NOPET);
        }

        return false;
    }

    em_ptr->note = _("を支配した。", " is tamed!");
    set_pet(player_ptr, em_ptr->m_ptr);
    (void)set_monster_fast(player_ptr, em_ptr->g_ptr->m_idx, monster_fast_remaining(em_ptr->m_ptr) + 100);
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_kind_flags.set(MonsterKindType::GOOD);
    }

    return true;
}

ProcessResult effect_monster_crusade(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }
    bool success = effect_monster_crusade_domination(player_ptr, em_ptr);
    if (success) {
        em_ptr->dam = 0;
        return ProcessResult::PROCESS_CONTINUE;
    }

    if ((em_ptr->r_ptr->flags3 & RF3_NO_FEAR) == 0) {
        em_ptr->do_fear = randint1(90) + 10;
    } else if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        em_ptr->r_ptr->r_flags3 |= RF3_NO_FEAR;
    }

    em_ptr->dam = 0;
    return ProcessResult::PROCESS_CONTINUE;
}

/*!
 * @brief モンスターボールで捕まえられる最大HPを計算する
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param m_ptr モンスター情報への参照ポインタ
 * @param hp 計算対象のHP
 * @return 捕まえられる最大HP
 */
static int calcutate_capturable_hp(PlayerType *player_ptr, monster_type *m_ptr, int hp)
{
    if (is_pet(m_ptr)) {
        return hp * 4L;
    }

    if (PlayerClass(player_ptr).equals(PlayerClassType::BEASTMASTER) && monster_living(m_ptr->r_idx)) {
        return hp * 3 / 10;
    }

    return hp * 3 / 20;
}

/*!
 * @brief モンスターボールで捕らえた処理
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param em_ptr 効果情報への参照ポインタ
 */
static void effect_monster_captured(PlayerType *player_ptr, effect_monster_type *em_ptr, std::optional<CapturedMonsterType *> tmp_cap_mon_ptr)
{
    if (em_ptr->m_ptr->mflag2.has(MonsterConstantFlagType::CHAMELEON)) {
        choose_new_monster(player_ptr, em_ptr->g_ptr->m_idx, false, MonsterRaceId::CHAMELEON);
    }

    msg_format(_("%sを捕えた！", "You capture %^s!"), em_ptr->m_name);
    auto cap_mon_ptr = tmp_cap_mon_ptr.value();
    cap_mon_ptr->r_idx = em_ptr->m_ptr->r_idx;
    cap_mon_ptr->speed = em_ptr->m_ptr->mspeed;
    cap_mon_ptr->current_hp = static_cast<short>(em_ptr->m_ptr->hp);
    cap_mon_ptr->max_hp = static_cast<short>(em_ptr->m_ptr->max_maxhp);
    cap_mon_ptr->nickname = em_ptr->m_ptr->nickname;
    if ((em_ptr->g_ptr->m_idx == player_ptr->riding) && process_fall_off_horse(player_ptr, -1, false)) {
        msg_format(_("地面に落とされた。", "You have fallen from %s."), em_ptr->m_name);
    }

    delete_monster_idx(player_ptr, em_ptr->g_ptr->m_idx);
    calculate_upkeep(player_ptr);
}

/*!
 * @brief モンスターボールで捕らえる効果(CAPTURE)
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param em_ptr 効果情報への参照ポインタ
 * @return 効果発動結果
 */
ProcessResult effect_monster_capture(PlayerType *player_ptr, effect_monster_type *em_ptr, std::optional<CapturedMonsterType *> cap_mon_ptr)
{
    const auto &quest_list = QuestList::get_instance();
    auto *floor_ptr = player_ptr->current_floor_ptr;

    auto quest_monster = inside_quest(floor_ptr->quest_number);
    quest_monster &= (quest_list[floor_ptr->quest_number].type == QuestKindType::KILL_ALL);
    quest_monster &= !is_pet(em_ptr->m_ptr);

    auto cannot_capture = quest_monster;
    cannot_capture |= em_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE);
    cannot_capture |= any_bits(em_ptr->r_ptr->flags1, RF1_QUESTOR);
    cannot_capture |= any_bits(em_ptr->r_ptr->flags7, RF7_NAZGUL | RF7_UNIQUE2);
    cannot_capture |= (em_ptr->m_ptr->parent_m_idx != 0);
    if (cannot_capture) {
        msg_format(_("%sには効果がなかった。", "%s is unaffected."), em_ptr->m_name);
        em_ptr->skipped = true;
        return ProcessResult::PROCESS_CONTINUE;
    }

    auto r_max_hp = em_ptr->r_ptr->hdice * em_ptr->r_ptr->hside;
    auto threshold_hp = calcutate_capturable_hp(player_ptr, em_ptr->m_ptr, r_max_hp);
    auto capturable_hp = std::max(2, calcutate_capturable_hp(player_ptr, em_ptr->m_ptr, em_ptr->m_ptr->max_maxhp));

    if (threshold_hp < 2 || em_ptr->m_ptr->hp >= capturable_hp) {
        msg_format(_("もっと弱らせないと。", "You need to weaken %s more."), em_ptr->m_name);
        em_ptr->skipped = true;
        return ProcessResult::PROCESS_CONTINUE;
    }

    if (em_ptr->m_ptr->hp <= randint1(capturable_hp)) {
        effect_monster_captured(player_ptr, em_ptr, cap_mon_ptr);
        return ProcessResult::PROCESS_TRUE;
    }

    msg_format(_("うまく捕まえられなかった。", "You failed to capture %s."), em_ptr->m_name);
    em_ptr->skipped = true;
    return ProcessResult::PROCESS_CONTINUE;
}
