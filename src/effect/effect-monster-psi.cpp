#include "effect/effect-monster-psi.h"
#include "core/player-redraw-types.h"
#include "core/window-redrawer.h"
#include "effect/effect-monster-util.h"
#include "floor/line-of-sight.h"
#include "mind/mind-mirror-master.h"
#include "monster-race/monster-kind-mask.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "player-base/player-class.h"
#include "player/player-damage.h"
#include "status/bad-status-setter.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 精神のないモンスターのPsi攻撃に対する完全な耐性を発動する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果への参照ポインタ
 * @return 完全な耐性を発動した場合TRUE、そうでなければFALSE
 */
static bool resisted_psi_because_empty_mind(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
    if (none_bits(em_ptr->r_ptr->flags2, RF2_EMPTY_MIND)) {
        return false;
    }

    em_ptr->dam = 0;
    em_ptr->note = _("には完全な耐性がある！", " is immune.");
    if (is_original_ap_and_seen(player_ptr, em_ptr->m_ptr)) {
        set_bits(em_ptr->r_ptr->r_flags2, RF2_EMPTY_MIND);
    }

    return true;
}

/*!
 * @brief 異質な精神のモンスター及び強力なモンスターのPsi攻撃に対する耐性を発動する
 * @param em_ptr モンスター効果への参照ポインタ
 * @return 耐性を発動した場合TRUE、そうでなければFALSE
 * @details
 * 以下のいずれかの場合は耐性がある
 * 1) STUPIDまたはWIERD_MINDである
 * 2) ANIMALである
 * 3) レベルが d(3*ダメージ) より大きい
 */
static bool resisted_psi_because_weird_mind_or_powerful(effect_monster_type *em_ptr)
{
    bool has_resistance = em_ptr->r_ptr->behavior_flags.has(MonsterBehaviorType::STUPID);
    has_resistance |= any_bits(em_ptr->r_ptr->flags2, RF2_WEIRD_MIND);
    has_resistance |= em_ptr->r_ptr->kind_flags.has(MonsterKindType::ANIMAL);
    has_resistance |= (em_ptr->r_ptr->level > randint1(3 * em_ptr->dam));
    if (!has_resistance) {
        return false;
    }

    em_ptr->note = _("には耐性がある！", " resists!");
    em_ptr->dam /= 3;
    return true;
}

/*!
 * @brief 堕落した精神のモンスターへのPsi攻撃のダメージ反射を発動する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果への参照ポインタ
 * @return ダメージ反射を発動した場合TRUE、そうでなければFALSE
 * @details
 * 以下の条件を満たす場合に 1/2 の確率でダメージ反射する
 * 1) UNDEADまたはDEMONである
 * 2) レベルが詠唱者の レベル/2 より大きい
 */
static bool reflects_psi_with_currupted_mind(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
    bool is_corrupted = em_ptr->r_ptr->kind_flags.has_any_of(has_corrupted_mind);
    is_corrupted &= (em_ptr->r_ptr->level > player_ptr->lev / 2);
    is_corrupted &= one_in_(2);
    if (!is_corrupted) {
        return false;
    }

    em_ptr->note = nullptr;
    msg_format(_("%^sの堕落した精神は攻撃を跳ね返した！",
                   (em_ptr->seen ? "%^s's corrupted mind backlashes your attack!" : "%^ss corrupted mind backlashes your attack!")),
        em_ptr->m_name);
    return true;
}

/*!
 * @brief モンスターがPsi攻撃をダメージ反射した場合のプレイヤーへの追加効果を発動する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果への参照ポインタ
 * @details
 * 効果は、混乱、朦朧、恐怖、麻痺
 * 3/4の確率または影分身時はダメージ及び追加効果はない。
 */
static void effect_monster_psi_reflect_extra_effect(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
    if (!one_in_(4) || check_multishadow(player_ptr)) {
        return;
    }

    BadStatusSetter bss(player_ptr);
    switch (randint1(4)) {
    case 1:
        (void)bss.mod_confusion(3 + randint1(em_ptr->dam));
        return;
    case 2:
        (void)bss.mod_stun(randint1(em_ptr->dam));
        return;
    case 3:
        if (any_bits(em_ptr->r_ptr->flags3, RF3_NO_FEAR)) {
            em_ptr->note = _("には効果がなかった。", " is unaffected.");
        } else {
            (void)bss.mod_fear(3 + randint1(em_ptr->dam));
        }

        return;
    default:
        if (!player_ptr->free_act) {
            (void)bss.mod_paralysis(randint1(em_ptr->dam));
        }

        return;
    }
}

/*!
 * @brief モンスターのPsi攻撃に対する耐性を発動する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果への参照ポインタ
 * @details
 * 耐性を発動した精神の堕落したモンスターは効力を跳ね返すことがある。
 */
static void effect_monster_psi_resist(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
    if (resisted_psi_because_empty_mind(player_ptr, em_ptr)) {
        return;
    }
    if (!resisted_psi_because_weird_mind_or_powerful(em_ptr)) {
        return;
    }
    if (!reflects_psi_with_currupted_mind(player_ptr, em_ptr)) {
        return;
    }

    /* プレイヤーの反射判定 */
    if ((randint0(100 + em_ptr->r_ptr->level / 2) < player_ptr->skill_sav) && !check_multishadow(player_ptr)) {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
        em_ptr->dam = 0;
        return;
    }

    /* Injure +/- confusion */
    monster_desc(player_ptr, em_ptr->killer, em_ptr->m_ptr, MD_WRONGDOER_NAME);
    take_hit(player_ptr, DAMAGE_ATTACK, em_ptr->dam, em_ptr->killer);
    effect_monster_psi_reflect_extra_effect(player_ptr, em_ptr);
    em_ptr->dam = 0;
}

/*!
 * @brief モンスターへのPsi攻撃の追加効果を発動する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果への参照ポインタ
 * @details
 * 効果は、混乱、朦朧、恐怖、麻痺(各耐性無効)
 * ダメージがないか3/4の確率で効果なし
 */
static void effect_monster_psi_extra_effect(effect_monster_type *em_ptr)
{
    if ((em_ptr->dam <= 0) || !one_in_(4)) {
        return;
    }

    switch (randint1(4)) {
    case 1:
        em_ptr->do_conf = 3 + randint1(em_ptr->dam);
        break;
    case 2:
        em_ptr->do_stun = 3 + randint1(em_ptr->dam);
        break;
    case 3:
        em_ptr->do_fear = 3 + randint1(em_ptr->dam);
        break;
    default:
        em_ptr->note = _("は眠り込んでしまった！", " falls asleep!");
        em_ptr->do_sleep = 3 + randint1(em_ptr->dam);
        break;
    }
}

/*!
 * @brief モンスターへのPsi攻撃(PSI)の効果を発動する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果への参照ポインタ
 * @return PROICESS_CONTINUE
 * @details
 * 視界による影響を発動する。
 * モンスターの耐性とそれに不随した効果を発動する。
 */
ProcessResult effect_monster_psi(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }
    if (!(los(player_ptr, em_ptr->m_ptr->fy, em_ptr->m_ptr->fx, player_ptr->y, player_ptr->x))) {
        if (em_ptr->seen_msg) {
            msg_format(_("%sはあなたが見えないので影響されない！", "%^s can't see you, and isn't affected!"), em_ptr->m_name);
        }

        em_ptr->skipped = true;
        return ProcessResult::PROCESS_CONTINUE;
    }

    effect_monster_psi_resist(player_ptr, em_ptr);
    effect_monster_psi_extra_effect(em_ptr);
    em_ptr->note_dies = _("の精神は崩壊し、肉体は抜け殻となった。", " collapses, a mindless husk.");
    return ProcessResult::PROCESS_CONTINUE;
}

/*!
 * @brief モンスターのPsi攻撃(PSI_DRAIN)に対する耐性を発動する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果への参照ポインタ
 * @details
 * 耐性を発動した精神の堕落したモンスターは効力を跳ね返すことがある。
 */
static void effect_monster_psi_drain_resist(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
    if (resisted_psi_because_empty_mind(player_ptr, em_ptr)) {
        return;
    }
    if (!resisted_psi_because_weird_mind_or_powerful(em_ptr)) {
        return;
    }
    if (!reflects_psi_with_currupted_mind(player_ptr, em_ptr)) {
        return;
    }

    /* プレイヤーの反射判定 */
    if ((randint0(100 + em_ptr->r_ptr->level / 2) < player_ptr->skill_sav) && !check_multishadow(player_ptr)) {
        msg_print(_("あなたは効力を跳ね返した！", "You resist the effects!"));
        em_ptr->dam = 0;
        return;
    }

    monster_desc(player_ptr, em_ptr->killer, em_ptr->m_ptr, MD_WRONGDOER_NAME);
    if (check_multishadow(player_ptr)) {
        take_hit(player_ptr, DAMAGE_ATTACK, em_ptr->dam, em_ptr->killer);
        em_ptr->dam = 0;
        return;
    }

    msg_print(_("超能力パワーを吸いとられた！", "Your psychic energy is drained!"));
    player_ptr->csp -= damroll(5, em_ptr->dam) / 2;
    if (player_ptr->csp < 0) {
        player_ptr->csp = 0;
    }

    set_bits(player_ptr->redraw, PR_MANA);
    set_bits(player_ptr->window_flags, PW_SPELL);
    take_hit(player_ptr, DAMAGE_ATTACK, em_ptr->dam, em_ptr->killer);
    em_ptr->dam = 0;
}

/*!
 * @brief モンスターへのPsi攻撃(PSI_DRAIN)のダメージをMPに変換する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果への参照ポインタ
 */
static void effect_monster_psi_drain_change_power(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
    int b = damroll(5, em_ptr->dam) / 4;
    concptr str = PlayerClass(player_ptr).equals(PlayerClassType::MINDCRAFTER) ? _("超能力パワー", "psychic energy") : _("魔力", "mana");
    concptr msg = _("あなたは%sの苦痛を%sに変換した！", (em_ptr->seen ? "You convert %s's pain into %s!" : "You convert %ss pain into %s!"));
    msg_format(msg, em_ptr->m_name, str);

    b = std::min(player_ptr->msp, player_ptr->csp + b);
    player_ptr->csp = b;
    set_bits(player_ptr->redraw, PR_MANA);
    set_bits(player_ptr->window_flags, PW_SPELL);
}

/*!
 * @brief モンスターへのPsi攻撃(PSI_DRAIN)の効果を発動する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果への参照ポインタ
 * @return PROICESS_CONTINUE
 * @details
 * ダメージがないか3/4の確率で追加効果なし
 */
ProcessResult effect_monster_psi_drain(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }

    effect_monster_psi_drain_resist(player_ptr, em_ptr);
    if (em_ptr->dam > 0) {
        effect_monster_psi_drain_change_power(player_ptr, em_ptr);
    }

    em_ptr->note_dies = _("の精神は崩壊し、肉体は抜け殻となった。", " collapses, a mindless husk.");
    return ProcessResult::PROCESS_CONTINUE;
}

/*!
 * @brief モンスターへのテレキネシス(TELEKINESIS)の効果を発動する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param em_ptr モンスター効果への参照ポインタ
 * @return PROICESS_CONTINUE
 * @details
 * 朦朧＋ショートテレポートアウェイ
 */
ProcessResult effect_monster_telekinesis(PlayerType *player_ptr, effect_monster_type *em_ptr)
{
    if (em_ptr->seen) {
        em_ptr->obvious = true;
    }
    if (one_in_(4)) {
        if (player_ptr->riding && (em_ptr->g_ptr->m_idx == player_ptr->riding)) {
            em_ptr->do_dist = 0;
        } else {
            em_ptr->do_dist = 7;
        }
    }

    em_ptr->do_stun = damroll((em_ptr->caster_lev / 20) + 3, em_ptr->dam) + 1;
    if (em_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE) || (em_ptr->r_ptr->level > 5 + randint1(em_ptr->dam))) {
        em_ptr->do_stun = 0;
        em_ptr->obvious = false;
    }

    return ProcessResult::PROCESS_CONTINUE;
}
