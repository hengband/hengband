/*!
 * @brief プレイヤーのステータスを下げるか、モンスター自身のステータスを上げるスペル類の処理
 * @date 2020/05/16
 * @author Hourier
 * @todo やや過密気味。モンスター自身のステータスとプレイヤーのステータスで分けてもいいかもしれない.
 */

#include "mspell/mspell-status.h"
#include "blue-magic/blue-magic-checker.h"
#include "core/disturbance.h"
#include "effect/attribute-types.h"
#include "effect/effect-processor.h"
#include "mind/drs-types.h"
#include "monster-race/race-ability-flags.h"
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
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "timed-effect/timed-effects.h"
#include "tracking/health-bar-tracker.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

mspell_cast_msg_bad_status_to_player::mspell_cast_msg_bad_status_to_player(Format blind, Format not_blind, std::string_view resist, std::string_view saved_throw)
    : blind(blind)
    , not_blind(not_blind)
    , resist(resist)
    , saved_throw(saved_throw)
{
}

mspell_cast_msg_bad_status_to_monster::mspell_cast_msg_bad_status_to_monster(DefaultFormat default_msg, Format resist, Format saved_throw, Format success)
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
    const auto m_name = str_upcase_first(monster_name(player_ptr, m_idx));

    disturb(player_ptr, true, true);
    if (player_ptr->effects()->blindness().is_active()) {
        msg_print(msgs.blind, std::string_view(m_name));
    } else {
        msg_print(msgs.not_blind, std::string_view(m_name));
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
    auto &floor = *player_ptr->current_floor_ptr;
    bool see_either = see_monster(player_ptr, m_idx) || see_monster(player_ptr, t_idx);
    bool see_t = see_monster(player_ptr, t_idx);
    bool known = monster_near_player(floor, m_idx, t_idx);
    const auto m_name = monster_name(player_ptr, m_idx);
    const auto t_name = monster_name(player_ptr, t_idx);

    if (known) {
        if (see_either) {
            msg_print(msgs.default_msg, std::string_view(str_upcase_first(m_name)), std::string_view(t_name));
        } else {
            floor.monster_noise = true;
        }
    }

    if (resist) {
        if (see_t) {
            msg_print(msgs.resist, std::string_view(str_upcase_first(t_name)));
        }
    } else if (saved_throw) {
        if (see_t) {
            msg_print(msgs.saved_throw, std::string_view(str_upcase_first(t_name)));
        }
    } else {
        if (see_t) {
            msg_print(msgs.success, std::string_view(str_upcase_first(t_name)));
        }
    }

    set_monster_csleep(*player_ptr->current_floor_ptr, t_idx, 0);
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
        msg_print(_("{}は精神エネルギーを{}から吸いとった。", "{} draws psychic energy from {}."), str_upcase_first(m_name), t_name);
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
    const auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    bool seen = (!player_ptr->effects()->blindness().is_active() && monster.ml);
    const auto m_name = monster_name(player_ptr, m_idx);
    const auto t_name = monster_name(player_ptr, t_idx);

    if (target_type == MONSTER_TO_PLAYER) {
        disturb(player_ptr, true, true);
        if (!seen) {
            msg_print(_("何かがあなたの精神に念を放っているようだ。", "You feel something focusing on your mind."));
        } else {
            msg_print(_("{}があなたの瞳をじっとにらんでいる。", "{} gazes deep into your eyes."), str_upcase_first(m_name));
        }
    } else if (target_type == MONSTER_TO_MONSTER && see_monster(player_ptr, m_idx)) {
        msg_print(_("{}は{}をじっと睨んだ。", "{} gazes intently at {}."), str_upcase_first(m_name), t_name);
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
    const auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    bool seen = (!player_ptr->effects()->blindness().is_active() && monster.ml);
    const auto m_name = monster_name(player_ptr, m_idx);
    const auto t_name = monster_name(player_ptr, t_idx);

    if (target_type == MONSTER_TO_PLAYER) {
        disturb(player_ptr, true, true);
        if (!seen) {
            msg_print(_("何かがあなたの精神に念を放っているようだ。", "You feel something focusing on your mind."));
        } else {
            msg_print(_("{}があなたの瞳をじっとにらんでいる。", "{} gazes deep into your eyes."), str_upcase_first(m_name));
        }
    } else if (target_type == MONSTER_TO_MONSTER && see_monster(player_ptr, m_idx)) {
        msg_print(_("{}は{}をじっと睨んだ。", "{} gazes intently at {}."), str_upcase_first(m_name), t_name);
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

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster_target = floor.m_list[t_idx];
    const auto &monrace_target = monster_target.get_monrace();
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool resist, saving_throw;

    if (target_type == MONSTER_TO_PLAYER) {
        resist = (has_resist_fear(player_ptr) != 0);
        saving_throw = (randint0(100 + rlev / 2) < player_ptr->skill_sav);

        mspell_cast_msg_bad_status_to_player msg(_("{}が何かをつぶやくと、恐ろしげな音が聞こえた。", "{} mumbles, and you hear scary noises."),
            _("{}が恐ろしげな幻覚を作り出した。", "{} casts a fearful illusion."), _("しかし恐怖に侵されなかった。", "You refuse to be frightened."),
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

    resist = (monrace_target.resistance_flags.has(MonsterResistanceType::NO_FEAR));
    saving_throw = (monrace_target.level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10);

    mspell_cast_msg_bad_status_to_monster msg(_("{}が恐ろしげな幻覚を作り出した。", "{} casts a fearful illusion in front of {}."),
        _("{}は恐怖を感じない。", "{} refuses to be frightened."), _("{}は恐怖を感じない。", "{} refuses to be frightened."),
        _("{}は恐怖して逃げ出した！", "{} flees in terror!"));

    spell_badstatus_message_to_mons(player_ptr, m_idx, t_idx, msg, resist, saving_throw);

    if (!resist && !saving_throw) {
        set_monster_monfear(*player_ptr->current_floor_ptr, t_idx, monster_target.get_remaining_fear() + randint0(4) + 4);
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

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster_target = floor.m_list[t_idx];
    const auto &monrace_target = monster_target.get_monrace();
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool resist, saving_throw;

    if (target_type == MONSTER_TO_PLAYER) {
        resist = (has_resist_blind(player_ptr) != 0);
        saving_throw = (randint0(100 + rlev / 2) < player_ptr->skill_sav);

        mspell_cast_msg_bad_status_to_player msg(_("{}が何かをつぶやいた。", "{} mumbles."),
            _("{}が呪文を唱えてあなたの目をくらました！", "{} casts a spell, burning your eyes!"), _("しかし効果がなかった！", "You are unaffected!"),
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

    using DefaultFormat = mspell_cast_msg_bad_status_to_monster::DefaultFormat;
    const auto t_name = monster_name(player_ptr, t_idx);
    const auto msg_default = (t_name == "it") ? DefaultFormat(_("{}は呪文を唱えて{}の目を焼き付かせた。", "{} casts a spell, burning {}s eyes.")) : DefaultFormat(_("{}は呪文を唱えて{}の目を焼き付かせた。", "{} casts a spell, burning {}'s eyes."));

    resist = (monrace_target.resistance_flags.has(MonsterResistanceType::NO_CONF));
    saving_throw = (monrace_target.level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10);

    mspell_cast_msg_bad_status_to_monster msg(msg_default, _("{}には効果がなかった。", "{} is unaffected."),
        _("{}には効果がなかった。", "{} is unaffected."), _("{}は目が見えなくなった！ ", "{} is blinded!"));

    spell_badstatus_message_to_mons(player_ptr, m_idx, t_idx, msg, resist, saving_throw);

    if (!resist && !saving_throw) {
        (void)set_monster_confused(*player_ptr->current_floor_ptr, t_idx, monster_target.get_remaining_confusion() + 12 + randint0(4));
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

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster_target = floor.m_list[t_idx];
    const auto &monrace_target = monster_target.get_monrace();
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool resist, saving_throw;

    if (target_type == MONSTER_TO_PLAYER) {
        resist = (has_resist_conf(player_ptr) != 0);
        saving_throw = (randint0(100 + rlev / 2) < player_ptr->skill_sav);

        mspell_cast_msg_bad_status_to_player msg(_("{}が何かをつぶやくと、頭を悩ます音がした。", "{} mumbles, and you hear puzzling noises."),
            _("{}が誘惑的な幻覚を作り出した。", "{} creates a mesmerising illusion."),
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

    resist = (monrace_target.resistance_flags.has(MonsterResistanceType::NO_CONF));
    saving_throw = (monrace_target.level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10);

    mspell_cast_msg_bad_status_to_monster msg(_("{}が{}の前に幻惑的な幻をつくり出した。", "{} casts a mesmerizing illusion in front of {}."),
        _("{}は惑わされなかった。", "{} disbelieves the feeble spell."), _("{}は惑わされなかった。", "{} disbelieves the feeble spell."),
        _("{}は混乱したようだ。", "{} seems confused."));

    spell_badstatus_message_to_mons(player_ptr, m_idx, t_idx, msg, resist, saving_throw);

    if (!resist && !saving_throw) {
        (void)set_monster_confused(*player_ptr->current_floor_ptr, t_idx, monster_target.get_remaining_confusion() + 12 + randint0(4));
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

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster_target = floor.m_list[t_idx];
    const auto &monrace_target = monster_target.get_monrace();
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool resist, saving_throw;

    if (target_type == MONSTER_TO_PLAYER) {
        resist = (player_ptr->free_act != 0);
        saving_throw = (randint0(100 + rlev / 2) < player_ptr->skill_sav);

        mspell_cast_msg_bad_status_to_player msg(_("{}が何かをつぶやいた。", "{} mumbles."),
            _("{}があなたの目をじっと見つめた！", "{} stares deep into your eyes!"), _("しかし効果がなかった！", "You are unaffected!"),
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

    resist = (monrace_target.kind_flags.has(MonsterKindType::UNIQUE) || monrace_target.resistance_flags.has(MonsterResistanceType::NO_SLEEP));
    saving_throw = (monrace_target.level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10);

    mspell_cast_msg_bad_status_to_monster msg(_("{}は{}をじっと見つめた。", "{} stares intently at {}."),
        _("{}には効果がなかった。", "{} is unaffected."), _("{}には効果がなかった。", "{} is unaffected."), _("{}は麻痺した！", "{} is paralyzed!"));

    spell_badstatus_message_to_mons(player_ptr, m_idx, t_idx, msg, (bool)resist, saving_throw);

    if (!resist && !saving_throw) {
        (void)set_monster_csleep(*player_ptr->current_floor_ptr, t_idx, 500);
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
    const auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    const auto m_name = monster_name(player_ptr, m_idx);

    mspell_cast_msg msg(_("{}が何かをつぶやいた。", "{} mumbles."),
        _("{}が自分の体に念を送った。", "{0} concentrates on {3} body."),
        _("{}が自分の体に念を送った。", "{0} concentrates on {3} body."),
        _("{}が自分の体に念を送った。", "{0} concentrates on {3} body."));

    monspell_message_base(player_ptr, m_idx, t_idx, msg, player_ptr->effects()->blindness().is_active(), target_type);

    if (set_monster_fast(*player_ptr->current_floor_ptr, m_idx, monster.get_remaining_acceleration() + 100)) {
        if (target_type == MONSTER_TO_PLAYER || (target_type == MONSTER_TO_MONSTER && see_m)) {
            msg_print(_("{}の動きが速くなった。", "{} starts moving faster."), str_upcase_first(m_name));
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

    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &monster_target = floor.m_list[t_idx];
    const auto &monrace_target = monster_target.get_monrace();
    DEPTH rlev = monster_level_idx(floor, m_idx);
    bool resist, saving_throw;

    if (target_type == MONSTER_TO_PLAYER) {
        resist = (has_resist_conf(player_ptr) != 0);
        saving_throw = (randint0(100 + rlev / 2) < player_ptr->skill_sav);

        mspell_cast_msg_bad_status_to_player msg(_("{}があなたの筋力を吸い取ろうとした！", "{} drains power from your muscles!"),
            _("{}があなたの筋力を吸い取ろうとした！", "{} drains power from your muscles!"), _("しかし効果がなかった！", "You are unaffected!"),
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

    using DefaultFormat = mspell_cast_msg_bad_status_to_monster::DefaultFormat;
    const auto t_name = monster_name(player_ptr, t_idx);
    const auto msg_default = (t_name == "it") ? DefaultFormat(_("{}が{}の筋肉から力を吸いとった。", "{} drains power from {}s muscles.")) : DefaultFormat(_("{}が{}の筋肉から力を吸いとった。", "{} drains power from {}'s muscles."));

    resist = (monrace_target.kind_flags.has(MonsterKindType::UNIQUE));
    saving_throw = (monrace_target.level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10);

    mspell_cast_msg_bad_status_to_monster msg(msg_default, _("{}には効果がなかった。", "{} is unaffected."),
        _("{}には効果がなかった。", "{} is unaffected."), _("{}の動きが遅くなった。", "{} starts moving slower."));

    spell_badstatus_message_to_mons(player_ptr, m_idx, t_idx, msg, resist, saving_throw);

    if (!resist && !saving_throw) {
        set_monster_slow(*player_ptr->current_floor_ptr, t_idx, monster_target.get_remaining_deceleration() + 50);
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
    auto &floor = *player_ptr->current_floor_ptr;
    auto &monster = floor.m_list[m_idx];
    DEPTH rlev = monster_level_idx(floor, m_idx);
    const auto is_blind = player_ptr->effects()->blindness().is_active();
    const auto seen = (!is_blind && monster.ml);

    msg.to_player_true = _("{}が何かをつぶやいた。", "{} mumbles.");
    msg.to_mons_true = _("{}は自分の傷に念を集中した。", "{0} concentrates on {3} wounds.");
    msg.to_player_false = _("{}が自分の傷に集中した。", "{0} concentrates on {3} wounds.");
    msg.to_mons_false = _("{}は自分の傷に念を集中した。", "{0} concentrates on {3} wounds.");

    monspell_message_base(player_ptr, m_idx, t_idx, msg, is_blind, target_type);

    monster.hp += (rlev * 6);
    if (monster.hp >= monster.maxhp) {
        /* Fully healed */
        monster.hp = monster.maxhp;

        msg.to_player_true = _("{}は完全に治ったようだ！", "{} sounds completely healed!");
        msg.to_mons_true = _("{}は完全に治ったようだ！", "{} sounds completely healed!");
        msg.to_player_false = _("{}は完全に治った！", "{} looks completely healed!");
        msg.to_mons_false = _("{}は完全に治った！", "{} looks completely healed!");
    } else {
        msg.to_player_true = _("{}は体力を回復したようだ。", "{} sounds healthier.");
        msg.to_mons_true = _("{}は体力を回復したようだ。", "{} sounds healthier.");
        msg.to_player_false = _("{}は体力を回復したようだ。", "{} looks healthier.");
        msg.to_mons_false = _("{}は体力を回復したようだ。", "{} looks healthier.");
    }

    monspell_message_base(player_ptr, m_idx, t_idx, msg, !seen, target_type);
    HealthBarTracker::get_instance().set_flag_if_tracking(m_idx);
    if (monster.is_riding()) {
        RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::UHEALTH);
    }

    if (!monster.is_fearful()) {
        return res;
    }

    (void)set_monster_monfear(*player_ptr->current_floor_ptr, m_idx, 0);

    if (see_monster(player_ptr, m_idx)) {
        const auto m_name = monster_name(player_ptr, m_idx);
        const auto m_poss = monster_desc(player_ptr, monster, MD_PRON_VISIBLE | MD_POSSESSIVE);
        msg_print(_("{}は勇気を取り戻した。", "{} recovers {} courage."), str_upcase_first(m_name), m_poss);
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
    const auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    bool seen = (!player_ptr->effects()->blindness().is_active() && monster.ml);
    mspell_cast_msg msg(_("{}が何かを力強くつぶやいた。", "{} mumbles powerfully."),
        _("{}が何かを力強くつぶやいた。", "{} mumbles powerfully."), _("{}は無傷の球の呪文を唱えた。", "{} casts a Globe of Invulnerability."),
        _("{}は無傷の球の呪文を唱えた。", "{} casts a Globe of Invulnerability."));

    monspell_message_base(player_ptr, m_idx, t_idx, msg, !seen, target_type);

    if (monster.ml) {
        MonraceId r_idx = monster.r_idx;
        const auto m_name = monster_desc(player_ptr, monster, MD_NONE);
        switch (r_idx) {
        case MonraceId::MARIO:
        case MonraceId::LUIGI:
            msg_print(_("{}はスターを取った！", "{} got a star!"), str_upcase_first(m_name));
            break;
        case MonraceId::DIAVOLO:
            msg_print(_("『読める』………動きの『軌跡』が読める……", "'Read'......... I can read the 'trajectory' of movement..."));
            break;
        default:
            msg_print(_("{}の身体がまばゆく輝き始めた！", "The body of {} began to shine dazzlingly!"), m_name);
            break;
        }
    }

    if (!monster.is_invulnerable()) {
        (void)set_monster_invulner(*player_ptr->current_floor_ptr, m_idx, randint1(4) + 4, false);
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
    DEPTH rlev = monster_level_idx(*player_ptr->current_floor_ptr, m_idx);
    const auto m_name = monster_name(player_ptr, m_idx);

    disturb(player_ptr, true, true);

    msg_print(_("{}があなたの記憶を消去しようとしている。", "{} tries to blank your mind."), str_upcase_first(m_name));

    if (randint0(100 + rlev / 2) < player_ptr->skill_sav) {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
    } else if (lose_all_info(player_ptr)) {
        msg_print(_("記憶が薄れてしまった。", "Your memories fade away."));
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = true;

    return res;
}
