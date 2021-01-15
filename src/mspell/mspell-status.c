/*!
 * todo やや過密気味。モンスター自身のステータスとプレーヤーのステータスで分けてもいいかもしれない.
 * @brief プレーヤーのステータスを下げるか、モンスター自身のステータスを上げるスペル類の処理
 * @date 2020/05/16
 * @author Hourier
 */

#include "mspell/mspell-status.h"
#include "blue-magic/blue-magic-checker.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "mind/drs-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-indice-types.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-util.h"
#include "player/player-personalities-types.h"
#include "player/player-status-flags.h"
#include "spell/spell-types.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "system/floor-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 状態異常呪文のメッセージ処理関数。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param msg1 対プレイヤーなら盲目時メッセージ。対モンスターなら通常時メッセージ。
 * @param msg2 対プレイヤーなら非盲目時メッセージ。対モンスターなら耐性有メッセージ。
 * @param msg3 対プレイヤーなら耐性有メッセージ。対モンスターなら抵抗時メッセージ。
 * @param msg4 対プレイヤーなら抵抗時メッセージ。対モンスターなら成功時メッセージ。
 * @param resist 耐性の有無を判別するフラグ
 * @param saving_throw 抵抗に成功したか判別するフラグ
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
void spell_badstatus_message(player_type *target_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, concptr msg1, concptr msg2, concptr msg3, concptr msg4, bool resist,
    bool saving_throw, int TARGET_TYPE)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    bool see_either = see_monster(target_ptr, m_idx) || see_monster(target_ptr, t_idx);
    bool see_t = see_monster(target_ptr, t_idx);
    bool known = monster_near_player(floor_ptr, m_idx, t_idx);
    GAME_TEXT m_name[MAX_NLEN], t_name[MAX_NLEN];
    monster_name(target_ptr, m_idx, m_name);
    monster_name(target_ptr, t_idx, t_name);

    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        disturb(target_ptr, TRUE, TRUE);
        if (target_ptr->blind)
            msg_format(msg1, m_name);
        else
            msg_format(msg2, m_name);

        if (resist) {
            msg_print(msg3);
        } else if (saving_throw) {
            msg_print(msg4);
        }

        return;
    }

    if (TARGET_TYPE != MONSTER_TO_MONSTER)
        return;

    if (known) {
        if (see_either) {
            msg_format(msg1, m_name, t_name);
        } else {
            floor_ptr->monster_noise = TRUE;
        }
    }

    if (resist) {
        if (see_t)
            msg_format(msg2, t_name);
    } else if (saving_throw) {
        if (see_t)
            msg_format(msg3, t_name);
    } else {
        if (see_t)
            msg_format(msg4, t_name);
    }

    set_monster_csleep(target_ptr, t_idx, 0);
}

/*!
 * @brief RF5_DRAIN_MANAの処理。魔力吸収。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_DRAIN_MANA(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam;
    GAME_TEXT m_name[MAX_NLEN], t_name[MAX_NLEN];
    monster_name(target_ptr, m_idx, m_name);
    monster_name(target_ptr, t_idx, t_name);

    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        disturb(target_ptr, TRUE, TRUE);
    } else if (TARGET_TYPE == MONSTER_TO_MONSTER && see_monster(target_ptr, m_idx)) {
        /* Basic message */
        msg_format(_("%^sは精神エネルギーを%sから吸いとった。", "%^s draws psychic energy from %s."), m_name, t_name);
    }

    dam = monspell_damage(target_ptr, (MS_DRAIN_MANA), m_idx, DAM_ROLL);
    breath(target_ptr, y, x, m_idx, GF_DRAIN_MANA, dam, 0, FALSE, MS_DRAIN_MANA, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER)
        update_smart_learn(target_ptr, m_idx, DRS_MANA);

    return dam;
}

/*!
 * @brief RF5_MIND_BLASTの処理。精神攻撃。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_MIND_BLAST(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    bool seen = (!target_ptr->blind && m_ptr->ml);
    HIT_POINT dam;
    GAME_TEXT m_name[MAX_NLEN], t_name[MAX_NLEN];
    monster_name(target_ptr, m_idx, m_name);
    monster_name(target_ptr, t_idx, t_name);

    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        disturb(target_ptr, TRUE, TRUE);
        if (!seen)
            msg_print(_("何かがあなたの精神に念を放っているようだ。", "You feel something focusing on your mind."));
        else
            msg_format(_("%^sがあなたの瞳をじっとにらんでいる。", "%^s gazes deep into your eyes."), m_name);
    } else if (TARGET_TYPE == MONSTER_TO_MONSTER && see_monster(target_ptr, m_idx)) {
        msg_format(_("%^sは%sをじっと睨んだ。", "%^s gazes intently at %s."), m_name, t_name);
    }

    dam = monspell_damage(target_ptr, (MS_MIND_BLAST), m_idx, DAM_ROLL);
    breath(target_ptr, y, x, m_idx, GF_MIND_BLAST, dam, 0, FALSE, MS_MIND_BLAST, TARGET_TYPE);
    return dam;
}

/*!
 * @brief RF5_BRAIN_SMASHの処理。脳攻撃。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_BRAIN_SMASH(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    bool seen = (!target_ptr->blind && m_ptr->ml);
    HIT_POINT dam;
    GAME_TEXT m_name[MAX_NLEN], t_name[MAX_NLEN];
    monster_name(target_ptr, m_idx, m_name);
    monster_name(target_ptr, t_idx, t_name);

    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        disturb(target_ptr, TRUE, TRUE);
        if (!seen)
            msg_print(_("何かがあなたの精神に念を放っているようだ。", "You feel something focusing on your mind."));
        else
            msg_format(_("%^sがあなたの瞳をじっとにらんでいる。", "%^s gazes deep into your eyes."), m_name);
    } else if (TARGET_TYPE == MONSTER_TO_MONSTER && see_monster(target_ptr, m_idx)) {
        msg_format(_("%^sは%sをじっと睨んだ。", "%^s gazes intently at %s."), m_name, t_name);
    }

    dam = monspell_damage(target_ptr, (MS_BRAIN_SMASH), m_idx, DAM_ROLL);
    breath(target_ptr, y, x, m_idx, GF_BRAIN_SMASH, dam, 0, FALSE, MS_BRAIN_SMASH, TARGET_TYPE);
    return dam;
}

/*!
 * @brief RF5_SCAREの処理。恐怖。 /
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
void spell_RF5_SCARE(MONSTER_IDX m_idx, player_type *target_ptr, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monster_type *t_ptr = &floor_ptr->m_list[t_idx];
    monster_race *tr_ptr = &r_info[t_ptr->r_idx];
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    bool resist, saving_throw;

    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        resist = (has_resist_fear(target_ptr) != 0);
        saving_throw = (randint0(100 + rlev / 2) < target_ptr->skill_sav);
        spell_badstatus_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやくと、恐ろしげな音が聞こえた。", "%^s mumbles, and you hear scary noises."),
            _("%^sが恐ろしげな幻覚を作り出した。", "%^s casts a fearful illusion."), _("しかし恐怖に侵されなかった。", "You refuse to be frightened."),
            _("しかし恐怖に侵されなかった。", "You refuse to be frightened."), resist, saving_throw, TARGET_TYPE);

        if (!resist && !saving_throw) {
            (void)set_afraid(target_ptr, target_ptr->afraid + randint0(4) + 4);
        }

        learn_spell(target_ptr, MS_SCARE);
        update_smart_learn(target_ptr, m_idx, DRS_FEAR);
        return;
    }

    if (TARGET_TYPE != MONSTER_TO_MONSTER)
        return;

    resist = ((tr_ptr->flags3 & RF3_NO_FEAR) != 0);
    saving_throw = (tr_ptr->level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10);

    spell_badstatus_message(target_ptr, m_idx, t_idx, _("%^sが恐ろしげな幻覚を作り出した。", "%^s casts a fearful illusion in front of %s."),
        _("%^sは恐怖を感じない。", "%^s refuses to be frightened."), _("%^sは恐怖を感じない。", "%^s refuses to be frightened."),
        _("%^sは恐怖して逃げ出した！", "%^s flees in terror!"), resist, saving_throw, TARGET_TYPE);

    if (!resist && !saving_throw) {
        set_monster_monfear(target_ptr, t_idx, monster_fear_remaining(t_ptr) + randint0(4) + 4);
    }
}

/*!
 * @brief RF5_BLINDの処理。盲目。 /
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
void spell_RF5_BLIND(MONSTER_IDX m_idx, player_type *target_ptr, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monster_type *t_ptr = &floor_ptr->m_list[t_idx];
    monster_race *tr_ptr = &r_info[t_ptr->r_idx];
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    bool resist, saving_throw;

    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        resist = (has_resist_blind(target_ptr) != 0);
        saving_throw = (randint0(100 + rlev / 2) < target_ptr->skill_sav);
        spell_badstatus_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."),
            _("%^sが呪文を唱えてあなたの目をくらました！", "%^s casts a spell, burning your eyes!"), _("しかし効果がなかった！", "You are unaffected!"),
            _("しかし効力を跳ね返した！", "You resist the effects!"), resist, saving_throw, TARGET_TYPE);

        if (!resist && !saving_throw) {
            (void)set_blind(target_ptr, 12 + randint0(4));
        }

        learn_spell(target_ptr, MS_BLIND);
        update_smart_learn(target_ptr, m_idx, DRS_BLIND);
        return;
    }

    if (TARGET_TYPE != MONSTER_TO_MONSTER)
        return;

    concptr msg1;
    GAME_TEXT t_name[MAX_NLEN];
    monster_name(target_ptr, t_idx, t_name);

    if (streq(t_name, "it")) {
        msg1 = _("%sは呪文を唱えて%sの目を焼き付かせた。", "%^s casts a spell, burning %ss eyes.");
    } else {
        msg1 = _("%sは呪文を唱えて%sの目を焼き付かせた。", "%^s casts a spell, burning %s's eyes.");
    }

    resist = ((tr_ptr->flags3 & RF3_NO_CONF) != 0);
    saving_throw = (tr_ptr->level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10);

    spell_badstatus_message(target_ptr, m_idx, t_idx, msg1, _("%^sには効果がなかった。", "%^s is unaffected."),
        _("%^sには効果がなかった。", "%^s is unaffected."), _("%^sは目が見えなくなった！ ", "%^s is blinded!"), resist, saving_throw, TARGET_TYPE);

    if (!resist && !saving_throw) {
        (void)set_monster_confused(target_ptr, t_idx, monster_confused_remaining(t_ptr) + 12 + randint0(4));
    }
}

/*!
 * @brief RF5_CONFの処理。混乱。/
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
void spell_RF5_CONF(MONSTER_IDX m_idx, player_type *target_ptr, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monster_type *t_ptr = &floor_ptr->m_list[t_idx];
    monster_race *tr_ptr = &r_info[t_ptr->r_idx];
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    bool resist, saving_throw;

    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        resist = (has_resist_conf(target_ptr) != 0);
        saving_throw = (randint0(100 + rlev / 2) < target_ptr->skill_sav);
        spell_badstatus_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやくと、頭を悩ます音がした。", "%^s mumbles, and you hear puzzling noises."),
            _("%^sが誘惑的な幻覚を作り出した。", "%^s creates a mesmerising illusion."),
            _("しかし幻覚にはだまされなかった。", "You disbelieve the feeble spell."),
            _("しかし幻覚にはだまされなかった。", "You disbelieve the feeble spell."), resist, saving_throw, TARGET_TYPE);

        if (!resist && !saving_throw) {
            (void)set_confused(target_ptr, target_ptr->confused + randint0(4) + 4);
        }

        learn_spell(target_ptr, MS_CONF);
        update_smart_learn(target_ptr, m_idx, DRS_CONF);
        return;
    }

    if (TARGET_TYPE != MONSTER_TO_MONSTER)
        return;

    resist = ((tr_ptr->flags3 & RF3_NO_CONF) != 0);
    saving_throw = (tr_ptr->level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10);

    spell_badstatus_message(target_ptr, m_idx, t_idx, _("%^sが%sの前に幻惑的な幻をつくり出した。", "%^s casts a mesmerizing illusion in front of %s."),
        _("%^sは惑わされなかった。", "%^s disbelieves the feeble spell."), _("%^sは惑わされなかった。", "%^s disbelieves the feeble spell."),
        _("%^sは混乱したようだ。", "%^s seems confused."), resist, saving_throw, TARGET_TYPE);

    if (!resist && !saving_throw) {
        (void)set_monster_confused(target_ptr, t_idx, monster_confused_remaining(t_ptr) + 12 + randint0(4));
    }
}

/*!
 * @brief RF5_HOLDの処理。麻痺。 /
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
void spell_RF5_HOLD(MONSTER_IDX m_idx, player_type *target_ptr, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monster_type *t_ptr = &floor_ptr->m_list[t_idx];
    monster_race *tr_ptr = &r_info[t_ptr->r_idx];
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    bool resist, saving_throw;

    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        resist = (target_ptr->free_act != 0);
        saving_throw = (randint0(100 + rlev / 2) < target_ptr->skill_sav);
        spell_badstatus_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."),
            _("%^sがあなたの目をじっと見つめた！", "%^s stares deep into your eyes!"), _("しかし効果がなかった！", "You are unaffected!"),
            _("しかし効力を跳ね返した！", "You resist the effects!"), (bool)resist, saving_throw, TARGET_TYPE);

        if (!resist && !saving_throw) {
            (void)set_paralyzed(target_ptr, target_ptr->paralyzed + randint0(4) + 4);
        }

        learn_spell(target_ptr, MS_SLEEP);
        update_smart_learn(target_ptr, m_idx, DRS_FREE);
        return;
    }

    if (TARGET_TYPE != MONSTER_TO_MONSTER)
        return;

    resist = ((tr_ptr->flags1 & RF1_UNIQUE) != 0 || (tr_ptr->flags3 & RF3_NO_STUN) != 0);
    saving_throw = (tr_ptr->level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10);

    spell_badstatus_message(target_ptr, m_idx, t_idx, _("%^sは%sをじっと見つめた。", "%^s stares intently at %s."),
        _("%^sには効果がなかった。", "%^s is unaffected."), _("%^sには効果がなかった。", "%^s is unaffected."), _("%^sは麻痺した！", "%^s is paralyzed!"),
        (bool)resist, saving_throw, TARGET_TYPE);

    if (!resist && !saving_throw) {
        (void)set_monster_stunned(target_ptr, t_idx, monster_stunned_remaining(t_ptr) + randint1(4) + 4);
    }
}

/*!
 * @brief RF6_HASTEの処理。加速。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
void spell_RF6_HASTE(player_type *target_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    bool see_m = see_monster(target_ptr, m_idx);
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    GAME_TEXT m_name[MAX_NLEN];
    monster_name(target_ptr, m_idx, m_name);
    char m_poss[10];
    monster_desc(target_ptr, m_poss, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE);

    monspell_message_base(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sが自分の体に念を送った。", format("%%^s concentrates on %s body.", m_poss)),
        _("%^sが自分の体に念を送った。", format("%%^s concentrates on %s body.", m_poss)),
        _("%^sが自分の体に念を送った。", format("%%^s concentrates on %s body.", m_poss)), target_ptr->blind > 0, TARGET_TYPE);

    if (set_monster_fast(target_ptr, m_idx, monster_fast_remaining(m_ptr) + 100)) {
        if (TARGET_TYPE == MONSTER_TO_PLAYER || (TARGET_TYPE == MONSTER_TO_MONSTER && see_m))
            msg_format(_("%^sの動きが速くなった。", "%^s starts moving faster."), m_name);
    }
}

/*!
 * @brief RF5_SLOWの処理。減速。 /
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
void spell_RF5_SLOW(MONSTER_IDX m_idx, player_type *target_ptr, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monster_type *t_ptr = &floor_ptr->m_list[t_idx];
    monster_race *tr_ptr = &r_info[t_ptr->r_idx];
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    bool resist, saving_throw;

    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        resist = (has_resist_conf(target_ptr) != 0);
        saving_throw = (randint0(100 + rlev / 2) < target_ptr->skill_sav);
        spell_badstatus_message(target_ptr, m_idx, t_idx, _("%^sがあなたの筋力を吸い取ろうとした！", "%^s drains power from your muscles!"),
            _("%^sがあなたの筋力を吸い取ろうとした！", "%^s drains power from your muscles!"), _("しかし効果がなかった！", "You are unaffected!"),
            _("しかし効力を跳ね返した！", "You resist the effects!"), resist, saving_throw, TARGET_TYPE);

        if (!resist && !saving_throw) {
            (void)set_slow(target_ptr, target_ptr->slow + randint0(4) + 4, FALSE);
        }

        learn_spell(target_ptr, MS_SLOW);
        update_smart_learn(target_ptr, m_idx, DRS_FREE);
        return;
    }

    if (TARGET_TYPE != MONSTER_TO_MONSTER)
        return;

    concptr msg1;
    GAME_TEXT t_name[MAX_NLEN];
    monster_name(target_ptr, t_idx, t_name);

    if (streq(t_name, "it")) {
        msg1 = _("%sが%sの筋肉から力を吸いとった。", "%^s drains power from %ss muscles.");
    } else {
        msg1 = _("%sが%sの筋肉から力を吸いとった。", "%^s drains power from %s's muscles.");
    }

    resist = ((tr_ptr->flags1 & RF1_UNIQUE) != 0);
    saving_throw = (tr_ptr->level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10);

    spell_badstatus_message(target_ptr, m_idx, t_idx, msg1, _("%^sには効果がなかった。", "%^s is unaffected."),
        _("%^sには効果がなかった。", "%^s is unaffected."), _("%sの動きが遅くなった。", "%^s starts moving slower."), resist, saving_throw, TARGET_TYPE);

    if (!resist && !saving_throw) {
        set_monster_slow(target_ptr, t_idx, monster_slow_remaining(t_ptr) + 50);
    }
}

/*!
 * @brief RF6_HEALの処理。治癒。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
void spell_RF6_HEAL(player_type *target_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    bool seen = (!target_ptr->blind && m_ptr->ml);
    GAME_TEXT m_name[MAX_NLEN];
    monster_name(target_ptr, m_idx, m_name);
    char m_poss[10];
    monster_desc(target_ptr, m_poss, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE);

    disturb(target_ptr, TRUE, TRUE);

    monspell_message_base(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sは自分の傷に念を集中した。", format("%%^s concentrates on %s wounds.", m_poss)),
        _("%^sが自分の傷に集中した。", format("%%^s concentrates on %s wounds.", m_poss)),
        _("%^sは自分の傷に念を集中した。", format("%%^s concentrates on %s wounds.", m_poss)), target_ptr->blind > 0, TARGET_TYPE);

    m_ptr->hp += (rlev * 6);
    if (m_ptr->hp >= m_ptr->maxhp) {
        /* Fully healed */
        m_ptr->hp = m_ptr->maxhp;

        monspell_message_base(target_ptr, m_idx, t_idx, _("%^sは完全に治ったようだ！", "%^s sounds completely healed!"),
            _("%^sは完全に治ったようだ！", "%^s sounds completely healed!"), _("%^sは完全に治った！", "%^s looks completely healed!"),
            _("%^sは完全に治った！", "%^s looks completely healed!"), !seen, TARGET_TYPE);
    } else {
        monspell_message_base(target_ptr, m_idx, t_idx, _("%^sは体力を回復したようだ。", "%^s sounds healthier."),
            _("%^sは体力を回復したようだ。", "%^s sounds healthier."), _("%^sは体力を回復したようだ。", "%^s looks healthier."),
            _("%^sは体力を回復したようだ。", "%^s looks healthier."), !seen, TARGET_TYPE);
    }

    if (target_ptr->health_who == m_idx)
        target_ptr->redraw |= (PR_HEALTH);
    if (target_ptr->riding == m_idx)
        target_ptr->redraw |= (PR_UHEALTH);

    if (!monster_fear_remaining(m_ptr))
        return;

    (void)set_monster_monfear(target_ptr, m_idx, 0);

    if (see_monster(target_ptr, m_idx))
        msg_format(_("%^sは勇気を取り戻した。", format("%%^s recovers %s courage.", m_poss)), m_name);
}

/*!
 * @brief RF6_INVULNERの処理。無敵。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
void spell_RF6_INVULNER(player_type *target_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    bool seen = (!target_ptr->blind && m_ptr->ml);

    monspell_message_base(target_ptr, m_idx, t_idx, _("%^sが何かを力強くつぶやいた。", "%^s mumbles powerfully."),
        _("%^sが何かを力強くつぶやいた。", "%^s mumbles powerfully."), _("%sは無傷の球の呪文を唱えた。", "%^s casts a Globe of Invulnerability."),
        _("%sは無傷の球の呪文を唱えた。", "%^s casts a Globe of Invulnerability."), !seen, TARGET_TYPE);

    MONRACE_IDX r_idx = m_ptr->r_idx;
    if ((r_idx == MON_MARIO) || r_idx == MON_LUIGI) {
        GAME_TEXT m_name[MAX_NLEN];
        monster_desc(target_ptr, m_name, m_ptr, MD_NONE);
        msg_format(_("%sはスターを取った！", "%^s got a star!"), m_name);
    }

    if (!monster_invulner_remaining(m_ptr))
        (void)set_monster_invulner(target_ptr, m_idx, randint1(4) + 4, FALSE);
}

/*!
 * @brief RF6_FORGETの処理。記憶消去。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param なし
 */
void spell_RF6_FORGET(player_type *target_ptr, MONSTER_IDX m_idx)
{
    DEPTH rlev = monster_level_idx(target_ptr->current_floor_ptr, m_idx);
    GAME_TEXT m_name[MAX_NLEN];
    monster_name(target_ptr, m_idx, m_name);

    disturb(target_ptr, TRUE, TRUE);

    msg_format(_("%^sがあなたの記憶を消去しようとしている。", "%^s tries to blank your mind."), m_name);

    if (randint0(100 + rlev / 2) < target_ptr->skill_sav) {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
    } else if (lose_all_info(target_ptr)) {
        msg_print(_("記憶が薄れてしまった。", "Your memories fade away."));
    }

    learn_spell(target_ptr, MS_FORGET);
}
