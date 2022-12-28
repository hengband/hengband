/*!
 * @brief プレイヤーのステータスを下げるか、モンスター自身のステータスを上げるスペル類の処理
 * @date 2020/05/16
 * @author Hourier
 * @todo やや過密気味。モンスター自身のステータスとプレイヤーのステータスで分けてもいいかもしれない.
 */

#include "mspell/mspell-status.h"
#include "blue-magic/blue-magic-checker.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "effect/attribute-types.h"
#include "effect/effect-processor.h"
#include "mind/drs-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-result.h"
#include "mspell/mspell-util.h"
#include "player/player-personality-types.h"
#include "player/player-status-flags.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "system/floor-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "timed-effect/player-blindness.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"

mspell_cast_msg_bad_status_to_player::mspell_cast_msg_bad_status_to_player(concptr blind, concptr not_blind, concptr resist, concptr saved_throw)
    : blind(blind)
    , not_blind(not_blind)
    , resist(resist)
    , saved_throw(saved_throw)
{
}

mspell_cast_msg_bad_status_to_monster::mspell_cast_msg_bad_status_to_monster(concptr default_msg, concptr resist, concptr saved_throw, concptr success)
    : default_msg(default_msg)
    , resist(resist)
    , saved_throw(saved_throw)
    , success(success)
{
}

/*!
 * @brief 状態異常呪文のメッセージ処理関数。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param msgs メッセージの構造体
 * @param resist 耐性の有無を判別するフラグ
 * @param saved_throw 抵抗に成功したか判別するフラグ
 */
void spell_badstatus_message_to_player(PlayerType *player_ptr, MONSTER_IDX m_idx, const mspell_cast_msg_bad_status_to_player &msgs, bool resist,
    bool saved_throw)
{
    const auto m_name = monster_name(player_ptr, m_idx);

    disturb(player_ptr, true, true);
    if (player_ptr->effects()->blindness()->is_blind()) {
        msg_format(msgs.blind, m_name.data());
    } else {
        msg_format(msgs.not_blind, m_name.data());
    }

    if (resist) {
        msg_print(msgs.resist);
    } else if (saved_throw) {
        msg_print(msgs.saved_throw);
    }

    return;
}

/*!
 * @brief 状態異常呪文のメッセージ処理関数（対モンスター）。 /
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param msgs メッセージの構造体
 * @param resist 耐性の有無を判別するフラグ
 * @param saved_throw 抵抗に成功したか判別するフラグ
 */
void spell_badstatus_message_to_mons(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, const mspell_cast_msg_bad_status_to_monster &msgs, bool resist,
    bool saved_throw)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    bool see_either = see_monster(player_ptr, m_idx) || see_monster(player_ptr, t_idx);
    bool see_t = see_monster(player_ptr, t_idx);
    bool known = monster_near_player(floor_ptr, m_idx, t_idx);
    const auto m_name = monster_name(player_ptr, m_idx);
    const auto t_name = monster_name(player_ptr, t_idx);

    if (known) {
        if (see_either) {
            msg_format(msgs.default_msg, m_name.data(), t_name.data());
        } else {
            floor_ptr->monster_noise = true;
        }
    }

    if (resist) {
        if (see_t) {
            msg_format(msgs.resist, t_name.data());
        }
    } else if (saved_throw) {
        if (see_t) {
            msg_format(msgs.saved_throw, t_name.data());
        }
    } else {
        if (see_t) {
            msg_format(msgs.success, t_name.data());
        }
    }

    set_monster_csleep(player_ptr, t_idx, 0);
}

/*!
 * @brief RF5_DRAIN_MANAの処理。魔力吸収。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_DRAIN_MANA(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    const auto m_name = monster_name(player_ptr, m_idx);
    const auto t_name = monster_name(player_ptr, t_idx);

    if (target_type == MONSTER_TO_PLAYER) {
        disturb(player_ptr, true, true);
    } else if (target_type == MONSTER_TO_MONSTER && see_monster(player_ptr, m_idx)) {
        /* Basic message */
        msg_format(_("%s^は精神エネルギーを%sから吸いとった。", "%s^ draws psychic energy from %s."), m_name.data(), t_name.data());
    }

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::DRAIN_MANA, m_idx, DAM_ROLL);
    const auto proj_res = pointed(player_ptr, y, x, m_idx, AttributeType::DRAIN_MANA, dam, target_type);
    if (target_type == MONSTER_TO_PLAYER) {
        update_smart_learn(player_ptr, m_idx, DRS_MANA);
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_MIND_BLASTの処理。精神攻撃。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_MIND_BLAST(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    bool seen = (!player_ptr->effects()->blindness()->is_blind() && m_ptr->ml);
    const auto m_name = monster_name(player_ptr, m_idx);
    const auto t_name = monster_name(player_ptr, t_idx);

    if (target_type == MONSTER_TO_PLAYER) {
        disturb(player_ptr, true, true);
        if (!seen) {
            msg_print(_("何かがあなたの精神に念を放っているようだ。", "You feel something focusing on your mind."));
        } else {
            msg_format(_("%s^があなたの瞳をじっとにらんでいる。", "%s^ gazes deep into your eyes."), m_name.data());
        }
    } else if (target_type == MONSTER_TO_MONSTER && see_monster(player_ptr, m_idx)) {
        msg_format(_("%s^は%sをじっと睨んだ。", "%s^ gazes intently at %s."), m_name.data(), t_name.data());
    }

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::MIND_BLAST, m_idx, DAM_ROLL);
    const auto proj_res = pointed(player_ptr, y, x, m_idx, AttributeType::MIND_BLAST, dam, target_type);

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_BRAIN_SMASHの処理。脳攻撃。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BRAIN_SMASH(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    bool seen = (!player_ptr->effects()->blindness()->is_blind() && m_ptr->ml);
    const auto m_name = monster_name(player_ptr, m_idx);
    const auto t_name = monster_name(player_ptr, t_idx);

    if (target_type == MONSTER_TO_PLAYER) {
        disturb(player_ptr, true, true);
        if (!seen) {
            msg_print(_("何かがあなたの精神に念を放っているようだ。", "You feel something focusing on your mind."));
        } else {
            msg_format(_("%s^があなたの瞳をじっとにらんでいる。", "%s^ gazes deep into your eyes."), m_name.data());
        }
    } else if (target_type == MONSTER_TO_MONSTER && see_monster(player_ptr, m_idx)) {
        msg_format(_("%s^は%sをじっと睨んだ。", "%s^ gazes intently at %s."), m_name.data(), t_name.data());
    }

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BRAIN_SMASH, m_idx, DAM_ROLL);
    const auto proj_res = pointed(player_ptr, y, x, m_idx, AttributeType::BRAIN_SMASH, dam, target_type);

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_SCAREの処理。恐怖。 /
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF5_SCARE(MONSTER_IDX m_idx, PlayerType *player_ptr, MONSTER_IDX t_idx, int target_type)
{
    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    auto *floor_ptr = player_ptr->current_floor_ptr;
    MonsterEntity *t_ptr = &floor_ptr->m_list[t_idx];
    MonsterRaceInfo *tr_ptr = &monraces_info[t_ptr->r_idx];
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    bool resist, saving_throw;

    if (target_type == MONSTER_TO_PLAYER) {
        resist = (has_resist_fear(player_ptr) != 0);
        saving_throw = (randint0(100 + rlev / 2) < player_ptr->skill_sav);

        mspell_cast_msg_bad_status_to_player msg(_("%s^が何かをつぶやくと、恐ろしげな音が聞こえた。", "%s^ mumbles, and you hear scary noises."),
            _("%s^が恐ろしげな幻覚を作り出した。", "%s^ casts a fearful illusion."), _("しかし恐怖に侵されなかった。", "You refuse to be frightened."),
            _("しかし恐怖に侵されなかった。", "You refuse to be frightened."));

        spell_badstatus_message_to_player(player_ptr, m_idx, msg, resist, saving_throw);

        if (!resist && !saving_throw) {
            (void)BadStatusSetter(player_ptr).mod_fear(randint0(4) + 4);
        }

        update_smart_learn(player_ptr, m_idx, DRS_FEAR);
        return res;
    }

    if (target_type != MONSTER_TO_MONSTER) {
        return res;
    }

    resist = ((tr_ptr->flags3 & RF3_NO_FEAR) != 0);
    saving_throw = (tr_ptr->level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10);

    mspell_cast_msg_bad_status_to_monster msg(_("%s^が恐ろしげな幻覚を作り出した。", "%s^ casts a fearful illusion in front of %s."),
        _("%s^は恐怖を感じない。", "%s^ refuses to be frightened."), _("%s^は恐怖を感じない。", "%s^ refuses to be frightened."),
        _("%s^は恐怖して逃げ出した！", "%s^ flees in terror!"));

    spell_badstatus_message_to_mons(player_ptr, m_idx, t_idx, msg, resist, saving_throw);

    if (!resist && !saving_throw) {
        set_monster_monfear(player_ptr, t_idx, t_ptr->get_remaining_fear() + randint0(4) + 4);
    }

    return res;
}

/*!
 * @brief RF5_BLINDの処理。盲目。 /
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF5_BLIND(MONSTER_IDX m_idx, PlayerType *player_ptr, MONSTER_IDX t_idx, int target_type)
{
    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    auto *floor_ptr = player_ptr->current_floor_ptr;
    MonsterEntity *t_ptr = &floor_ptr->m_list[t_idx];
    MonsterRaceInfo *tr_ptr = &monraces_info[t_ptr->r_idx];
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    bool resist, saving_throw;

    if (target_type == MONSTER_TO_PLAYER) {
        resist = (has_resist_blind(player_ptr) != 0);
        saving_throw = (randint0(100 + rlev / 2) < player_ptr->skill_sav);

        mspell_cast_msg_bad_status_to_player msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
            _("%s^が呪文を唱えてあなたの目をくらました！", "%s^ casts a spell, burning your eyes!"), _("しかし効果がなかった！", "You are unaffected!"),
            _("しかし効力を跳ね返した！", "You resist the effects!"));

        spell_badstatus_message_to_player(player_ptr, m_idx, msg, resist, saving_throw);

        if (!resist && !saving_throw) {
            (void)BadStatusSetter(player_ptr).set_blindness(12 + randint0(4));
        }

        update_smart_learn(player_ptr, m_idx, DRS_BLIND);
        return res;
    }

    if (target_type != MONSTER_TO_MONSTER) {
        return res;
    }

    concptr msg_default;
    const auto t_name = monster_name(player_ptr, t_idx);

    if (streq(t_name.data(), "it")) {
        msg_default = _("%sは呪文を唱えて%sの目を焼き付かせた。", "%s^ casts a spell, burning %ss eyes.");
    } else {
        msg_default = _("%sは呪文を唱えて%sの目を焼き付かせた。", "%s^ casts a spell, burning %s's eyes.");
    }

    resist = ((tr_ptr->flags3 & RF3_NO_CONF) != 0);
    saving_throw = (tr_ptr->level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10);

    mspell_cast_msg_bad_status_to_monster msg(msg_default, _("%s^には効果がなかった。", "%s^ is unaffected."),
        _("%s^には効果がなかった。", "%s^ is unaffected."), _("%s^は目が見えなくなった！ ", "%s^ is blinded!"));

    spell_badstatus_message_to_mons(player_ptr, m_idx, t_idx, msg, resist, saving_throw);

    if (!resist && !saving_throw) {
        (void)set_monster_confused(player_ptr, t_idx, t_ptr->get_remaining_confusion() + 12 + randint0(4));
    }

    return res;
}

/*!
 * @brief RF5_CONFの処理。混乱。/
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF5_CONF(MONSTER_IDX m_idx, PlayerType *player_ptr, MONSTER_IDX t_idx, int target_type)
{
    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    auto *floor_ptr = player_ptr->current_floor_ptr;
    MonsterEntity *t_ptr = &floor_ptr->m_list[t_idx];
    MonsterRaceInfo *tr_ptr = &monraces_info[t_ptr->r_idx];
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    bool resist, saving_throw;

    if (target_type == MONSTER_TO_PLAYER) {
        resist = (has_resist_conf(player_ptr) != 0);
        saving_throw = (randint0(100 + rlev / 2) < player_ptr->skill_sav);

        mspell_cast_msg_bad_status_to_player msg(_("%s^が何かをつぶやくと、頭を悩ます音がした。", "%s^ mumbles, and you hear puzzling noises."),
            _("%s^が誘惑的な幻覚を作り出した。", "%s^ creates a mesmerising illusion."),
            _("しかし幻覚にはだまされなかった。", "You disbelieve the feeble spell."),
            _("しかし幻覚にはだまされなかった。", "You disbelieve the feeble spell."));

        spell_badstatus_message_to_player(player_ptr, m_idx, msg, resist, saving_throw);

        if (!resist && !saving_throw) {
            (void)BadStatusSetter(player_ptr).mod_confusion(randint0(4) + 4);
        }

        update_smart_learn(player_ptr, m_idx, DRS_CONF);
        return res;
    }

    if (target_type != MONSTER_TO_MONSTER) {
        return res;
    }

    resist = ((tr_ptr->flags3 & RF3_NO_CONF) != 0);
    saving_throw = (tr_ptr->level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10);

    mspell_cast_msg_bad_status_to_monster msg(_("%s^が%sの前に幻惑的な幻をつくり出した。", "%s^ casts a mesmerizing illusion in front of %s."),
        _("%s^は惑わされなかった。", "%s^ disbelieves the feeble spell."), _("%s^は惑わされなかった。", "%s^ disbelieves the feeble spell."),
        _("%s^は混乱したようだ。", "%s^ seems confused."));

    spell_badstatus_message_to_mons(player_ptr, m_idx, t_idx, msg, resist, saving_throw);

    if (!resist && !saving_throw) {
        (void)set_monster_confused(player_ptr, t_idx, t_ptr->get_remaining_confusion() + 12 + randint0(4));
    }

    return res;
}

/*!
 * @brief RF5_HOLDの処理。麻痺。 /
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF5_HOLD(MONSTER_IDX m_idx, PlayerType *player_ptr, MONSTER_IDX t_idx, int target_type)
{
    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    auto *floor_ptr = player_ptr->current_floor_ptr;
    MonsterEntity *t_ptr = &floor_ptr->m_list[t_idx];
    MonsterRaceInfo *tr_ptr = &monraces_info[t_ptr->r_idx];
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    bool resist, saving_throw;

    if (target_type == MONSTER_TO_PLAYER) {
        resist = (player_ptr->free_act != 0);
        saving_throw = (randint0(100 + rlev / 2) < player_ptr->skill_sav);

        mspell_cast_msg_bad_status_to_player msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
            _("%s^があなたの目をじっと見つめた！", "%s^ stares deep into your eyes!"), _("しかし効果がなかった！", "You are unaffected!"),
            _("しかし効力を跳ね返した！", "You resist the effects!"));

        spell_badstatus_message_to_player(player_ptr, m_idx, msg, (bool)resist, saving_throw);

        if (!resist && !saving_throw) {
            (void)BadStatusSetter(player_ptr).mod_paralysis(randint0(4) + 4);
        }

        update_smart_learn(player_ptr, m_idx, DRS_FREE);
        return res;
    }

    if (target_type != MONSTER_TO_MONSTER) {
        return res;
    }

    resist = (tr_ptr->kind_flags.has(MonsterKindType::UNIQUE) || (tr_ptr->flags3 & RF3_NO_STUN) != 0);
    saving_throw = (tr_ptr->level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10);

    mspell_cast_msg_bad_status_to_monster msg(_("%s^は%sをじっと見つめた。", "%s^ stares intently at %s."),
        _("%s^には効果がなかった。", "%s^ is unaffected."), _("%s^には効果がなかった。", "%s^ is unaffected."), _("%s^は麻痺した！", "%s^ is paralyzed!"));

    spell_badstatus_message_to_mons(player_ptr, m_idx, t_idx, msg, (bool)resist, saving_throw);

    if (!resist && !saving_throw) {
        (void)set_monster_stunned(player_ptr, t_idx, t_ptr->get_remaining_stun() + randint1(4) + 4);
    }

    return res;
}

/*!
 * @brief RF6_HASTEの処理。加速。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * ラーニング不可。
 */
MonsterSpellResult spell_RF6_HASTE(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    bool see_m = see_monster(player_ptr, m_idx);
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    const auto m_name = monster_name(player_ptr, m_idx);
    const auto m_poss = monster_desc(player_ptr, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE);

    mspell_cast_msg msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
        _("%s^が自分の体に念を送った。", format("%%s^ concentrates on %s body.", m_poss.data())),
        _("%s^が自分の体に念を送った。", format("%%s^ concentrates on %s body.", m_poss.data())),
        _("%s^が自分の体に念を送った。", format("%%s^ concentrates on %s body.", m_poss.data())));

    monspell_message_base(player_ptr, m_idx, t_idx, msg, player_ptr->effects()->blindness()->is_blind(), target_type);

    if (set_monster_fast(player_ptr, m_idx, m_ptr->get_remaining_acceleration() + 100)) {
        if (target_type == MONSTER_TO_PLAYER || (target_type == MONSTER_TO_MONSTER && see_m)) {
            msg_format(_("%s^の動きが速くなった。", "%s^ starts moving faster."), m_name.data());
        }
    }

    return MonsterSpellResult::make_valid();
}

/*!
 * @brief RF5_SLOWの処理。減速。 /
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF5_SLOW(MONSTER_IDX m_idx, PlayerType *player_ptr, MONSTER_IDX t_idx, int target_type)
{
    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    auto *floor_ptr = player_ptr->current_floor_ptr;
    MonsterEntity *t_ptr = &floor_ptr->m_list[t_idx];
    MonsterRaceInfo *tr_ptr = &monraces_info[t_ptr->r_idx];
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    bool resist, saving_throw;

    if (target_type == MONSTER_TO_PLAYER) {
        resist = (has_resist_conf(player_ptr) != 0);
        saving_throw = (randint0(100 + rlev / 2) < player_ptr->skill_sav);

        mspell_cast_msg_bad_status_to_player msg(_("%s^があなたの筋力を吸い取ろうとした！", "%s^ drains power from your muscles!"),
            _("%s^があなたの筋力を吸い取ろうとした！", "%s^ drains power from your muscles!"), _("しかし効果がなかった！", "You are unaffected!"),
            _("しかし効力を跳ね返した！", "You resist the effects!"));

        spell_badstatus_message_to_player(player_ptr, m_idx, msg, resist, saving_throw);

        if (!resist && !saving_throw) {
            (void)BadStatusSetter(player_ptr).mod_deceleration(randint0(4) + 4, false);
        }

        update_smart_learn(player_ptr, m_idx, DRS_FREE);
        return res;
    }

    if (target_type != MONSTER_TO_MONSTER) {
        return res;
    }

    concptr msg_default;
    const auto t_name = monster_name(player_ptr, t_idx);

    if (streq(t_name.data(), "it")) {
        msg_default = _("%sが%sの筋肉から力を吸いとった。", "%s^ drains power from %ss muscles.");
    } else {
        msg_default = _("%sが%sの筋肉から力を吸いとった。", "%s^ drains power from %s's muscles.");
    }

    resist = (tr_ptr->kind_flags.has(MonsterKindType::UNIQUE));
    saving_throw = (tr_ptr->level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10);

    mspell_cast_msg_bad_status_to_monster msg(msg_default, _("%s^には効果がなかった。", "%s^ is unaffected."),
        _("%s^には効果がなかった。", "%s^ is unaffected."), _("%sの動きが遅くなった。", "%s^ starts moving slower."));

    spell_badstatus_message_to_mons(player_ptr, m_idx, t_idx, msg, resist, saving_throw);

    if (!resist && !saving_throw) {
        set_monster_slow(player_ptr, t_idx, t_ptr->get_remaining_deceleration() + 50);
    }

    return res;
}

/*!
 * @brief RF6_HEALの処理。治癒。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * ラーニング不可。
 */
MonsterSpellResult spell_RF6_HEAL(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    const auto res = MonsterSpellResult::make_valid();

    mspell_cast_msg msg;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    const auto is_blind = player_ptr->effects()->blindness()->is_blind();
    const auto seen = (!is_blind && m_ptr->ml);
    const auto m_poss = monster_desc(player_ptr, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE);

    msg.to_player_true = _("%s^が何かをつぶやいた。", "%s^ mumbles.");
    msg.to_mons_true = _("%s^は自分の傷に念を集中した。", format("%%s^ concentrates on %s wounds.", m_poss.data()));
    msg.to_player_false = _("%s^が自分の傷に集中した。", format("%%s^ concentrates on %s wounds.", m_poss.data()));
    msg.to_mons_false = _("%s^は自分の傷に念を集中した。", format("%%s^ concentrates on %s wounds.", m_poss.data()));

    monspell_message_base(player_ptr, m_idx, t_idx, msg, is_blind, target_type);

    m_ptr->hp += (rlev * 6);
    if (m_ptr->hp >= m_ptr->maxhp) {
        /* Fully healed */
        m_ptr->hp = m_ptr->maxhp;

        msg.to_player_true = _("%s^は完全に治ったようだ！", "%s^ sounds completely healed!");
        msg.to_mons_true = _("%s^は完全に治ったようだ！", "%s^ sounds completely healed!");
        msg.to_player_false = _("%s^は完全に治った！", "%s^ looks completely healed!");
        msg.to_mons_false = _("%s^は完全に治った！", "%s^ looks completely healed!");
    } else {
        msg.to_player_true = _("%s^は体力を回復したようだ。", "%s^ sounds healthier.");
        msg.to_mons_true = _("%s^は体力を回復したようだ。", "%s^ sounds healthier.");
        msg.to_player_false = _("%s^は体力を回復したようだ。", "%s^ looks healthier.");
        msg.to_mons_false = _("%s^は体力を回復したようだ。", "%s^ looks healthier.");
    }

    monspell_message_base(player_ptr, m_idx, t_idx, msg, !seen, target_type);

    if (player_ptr->health_who == m_idx) {
        player_ptr->redraw |= (PR_HEALTH);
    }
    if (player_ptr->riding == m_idx) {
        player_ptr->redraw |= (PR_UHEALTH);
    }

    if (!m_ptr->is_fearful()) {
        return res;
    }

    (void)set_monster_monfear(player_ptr, m_idx, 0);

    if (see_monster(player_ptr, m_idx)) {
        const auto m_name = monster_name(player_ptr, m_idx);
        msg_format(_("%s^は勇気を取り戻した。", format("%%s^ recovers %s courage.", m_poss.data())), m_name.data());
    }

    return res;
}

/*!
 * @brief RF6_INVULNERの処理。無敵。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * ラーニング不可。
 */
MonsterSpellResult spell_RF6_INVULNER(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    bool seen = (!player_ptr->effects()->blindness()->is_blind() && m_ptr->ml);
    mspell_cast_msg msg(_("%s^が何かを力強くつぶやいた。", "%s^ mumbles powerfully."),
        _("%s^が何かを力強くつぶやいた。", "%s^ mumbles powerfully."), _("%sは無傷の球の呪文を唱えた。", "%s^ casts a Globe of Invulnerability."),
        _("%sは無傷の球の呪文を唱えた。", "%s^ casts a Globe of Invulnerability."));

    monspell_message_base(player_ptr, m_idx, t_idx, msg, !seen, target_type);

    if (m_ptr->ml) {
        MonsterRaceId r_idx = m_ptr->r_idx;
        const auto m_name = monster_desc(player_ptr, m_ptr, MD_NONE);
        switch (r_idx) {
        case MonsterRaceId::MARIO:
        case MonsterRaceId::LUIGI:
            msg_format(_("%sはスターを取った！", "%s^ got a star!"), m_name.data());
            break;
        case MonsterRaceId::DIAVOLO:
            msg_print(_("『読める』………動きの『軌跡』が読める……", "'Read'......... I can read the 'trajectory' of movement..."));
            break;
        default:
            msg_format(_("%sの身体がまばゆく輝き始めた！", "The body of %s^ began to shine dazzlingly!"), m_name.data());
            break;
        }
    }

    if (!m_ptr->is_invulnerable()) {
        (void)set_monster_invulner(player_ptr, m_idx, randint1(4) + 4, false);
    }

    return MonsterSpellResult::make_valid();
}

/*!
 * @brief RF6_FORGETの処理。記憶消去。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 *
 * ラーニング可。
 */
MonsterSpellResult spell_RF6_FORGET(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    DEPTH rlev = monster_level_idx(player_ptr->current_floor_ptr, m_idx);
    const auto m_name = monster_name(player_ptr, m_idx);

    disturb(player_ptr, true, true);

    msg_format(_("%s^があなたの記憶を消去しようとしている。", "%s^ tries to blank your mind."), m_name.data());

    if (randint0(100 + rlev / 2) < player_ptr->skill_sav) {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
    } else if (lose_all_info(player_ptr)) {
        msg_print(_("記憶が薄れてしまった。", "Your memories fade away."));
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = true;

    return res;
}
