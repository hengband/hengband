/*!
 * @brief フロアの一定範囲に効果を及ぼす (悲鳴、テレポート等)スペルの効果
 * @date 2020/05/16
 * @author Hourier
 */

#include "mspell/mspell-floor.h"
#include "blue-magic/blue-magic-checker.h"
#include "core/disturbance.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "mind/drs-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-brightness-mask.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster-race/race-resistance-mask.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "mspell/mspell-result.h"
#include "mspell/mspell-status.h"
#include "mspell/mspell-util.h"
#include "player-base/player-class.h"
#include "player/player-personality-types.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-hex.h"
#include "system/floor-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "timed-effect/player-blindness.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief RF4_SHRIEKの処理。叫び。 /
 * @param m_idx 呪文を唱えるモンスターID
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * ラーニング不可。
 */
MonsterSpellResult spell_RF4_SHRIEK(MONSTER_IDX m_idx, PlayerType *player_ptr, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_simple msg(_("%s^がかん高い金切り声をあげた。", "%s^ makes a high pitched shriek."),
        _("%s^が%sに向かって叫んだ。", "%s^ shrieks at %s."));

    simple_monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    auto result = MonsterSpellResult::make_valid();
    if (m_ptr->r_idx == MonsterRaceId::LEE_QIEZI) {
        msg_print(_("しかし、その声は誰の心にも響かなかった…。", "However, that voice didn't touch anyone's heart..."));
        return result;
    }

    if (target_type == MONSTER_TO_PLAYER) {
        aggravate_monsters(player_ptr, m_idx);
    } else if (target_type == MONSTER_TO_MONSTER) {
        set_monster_csleep(player_ptr, t_idx, 0);
    }

    return result;
}

/*!
 * @brief RF6_WORLDの処理。時を止める。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 *
 * ラーニング不可。
 */
MonsterSpellResult spell_RF6_WORLD(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    disturb(player_ptr, true, true);
    (void)set_monster_timewalk(player_ptr, randint1(2) + 2, m_ptr->r_idx, true);

    return MonsterSpellResult::make_valid();
}

/*!
 * @brief RF6_BLINKの処理。ショート・テレポート。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param is_quantum_effect 量子的効果によるショート・テレポートの場合時TRUE
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * ラーニング不可。
 */
MonsterSpellResult spell_RF6_BLINK(PlayerType *player_ptr, MONSTER_IDX m_idx, int target_type, bool is_quantum_effect)
{
    const auto res = MonsterSpellResult::make_valid();
    const auto m_name = monster_name(player_ptr, m_idx);

    if (target_type == MONSTER_TO_PLAYER) {
        disturb(player_ptr, true, true);
    }

    if (!is_quantum_effect && SpellHex(player_ptr).check_hex_barrier(m_idx, HEX_ANTI_TELE)) {
        if (see_monster(player_ptr, m_idx)) {
            msg_format(_("魔法のバリアが%s^のテレポートを邪魔した。", "Magic barrier obstructs teleporting of %s^."), m_name.data());
        }
        return res;
    }

    if (see_monster(player_ptr, m_idx)) {
        msg_format(_("%s^が瞬時に消えた。", "%s^ blinks away."), m_name.data());
    }

    teleport_away(player_ptr, m_idx, 10, TELEPORT_SPONTANEOUS);

    if (target_type == MONSTER_TO_PLAYER) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
    }

    return res;
}

/*!
 * @brief RF6_TPORTの処理。テレポート。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * ラーニング不可。
 */
MonsterSpellResult spell_RF6_TPORT(PlayerType *player_ptr, MONSTER_IDX m_idx, int target_type)
{
    const auto res = MonsterSpellResult::make_valid();
    const auto m_name = monster_name(player_ptr, m_idx);

    if (target_type == MONSTER_TO_PLAYER) {
        disturb(player_ptr, true, true);
    }
    if (SpellHex(player_ptr).check_hex_barrier(m_idx, HEX_ANTI_TELE)) {
        if (see_monster(player_ptr, m_idx)) {
            msg_format(_("魔法のバリアが%s^のテレポートを邪魔した。", "Magic barrier obstructs teleporting of %s^."), m_name.data());
        }
        return res;
    }

    if (see_monster(player_ptr, m_idx)) {
        msg_format(_("%s^がテレポートした。", "%s^ teleports away."), m_name.data());
    }

    teleport_away_followable(player_ptr, m_idx);

    return res;
}

/*!
 * @brief RF6_TELE_TOの処理。テレポート・バック。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_TELE_TO(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    MonsterEntity *t_ptr = &floor_ptr->m_list[t_idx];
    MonsterRaceInfo *tr_ptr = &monraces_info[t_ptr->r_idx];

    mspell_cast_msg_simple msg(_("%s^があなたを引き戻した。", "%s^ commands you to return."),
        _("%s^が%sを引き戻した。", "%s^ commands %s to return."));

    simple_monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    if (target_type == MONSTER_TO_PLAYER) {
        teleport_player_to(player_ptr, m_ptr->fy, m_ptr->fx, TELEPORT_PASSIVE);
        return res;
    }

    if (target_type != MONSTER_TO_MONSTER) {
        return res;
    }

    bool resists_tele = false;
    const auto t_name = monster_name(player_ptr, t_idx);

    if (tr_ptr->resistance_flags.has(MonsterResistanceType::RESIST_TELEPORT)) {
        if (tr_ptr->kind_flags.has(MonsterKindType::UNIQUE) || tr_ptr->resistance_flags.has(MonsterResistanceType::RESIST_ALL)) {
            if (is_original_ap_and_seen(player_ptr, t_ptr)) {
                tr_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
            }
            if (see_monster(player_ptr, t_idx)) {
                msg_format(_("%s^には効果がなかった。", "%s^ is unaffected!"), t_name.data());
            }
            resists_tele = true;
        } else if (tr_ptr->level > randint1(100)) {
            if (is_original_ap_and_seen(player_ptr, t_ptr)) {
                tr_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
            }
            if (see_monster(player_ptr, t_idx)) {
                msg_format(_("%s^は耐性を持っている！", "%s^ resists!"), t_name.data());
            }
            resists_tele = true;
        }
    }

    if (resists_tele) {
        set_monster_csleep(player_ptr, t_idx, 0);
        return res;
    }

    if (t_idx == player_ptr->riding) {
        teleport_player_to(player_ptr, m_ptr->fy, m_ptr->fx, TELEPORT_PASSIVE);
    } else {
        teleport_monster_to(player_ptr, t_idx, m_ptr->fy, m_ptr->fx, 100, TELEPORT_PASSIVE);
    }
    set_monster_csleep(player_ptr, t_idx, 0);

    return res;
}

/*!
 * @brief RF6_TELE_AWAYの処理。テレポート・アウェイ。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF6_TELE_AWAY(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto res = MonsterSpellResult::make_valid();
    res.learnable = target_type == MONSTER_TO_PLAYER;

    auto *floor_ptr = player_ptr->current_floor_ptr;
    MonsterEntity *t_ptr = &floor_ptr->m_list[t_idx];
    MonsterRaceInfo *tr_ptr = &monraces_info[t_ptr->r_idx];

    mspell_cast_msg_simple msg(_("%s^にテレポートさせられた。", "%s^ teleports you away."),
        _("%s^は%sをテレポートさせた。", "%s^ teleports %s away."));

    simple_monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    if (target_type == MONSTER_TO_PLAYER) {
        if (is_echizen(player_ptr)) {
            msg_print(_("くっそ～", ""));
        } else if (is_chargeman(player_ptr)) {
            if (randint0(2) == 0) {
                msg_print(_("ジュラル星人め！", ""));
            } else {
                msg_print(_("弱い者いじめは止めるんだ！", ""));
            }
        }

        teleport_player_away(m_idx, player_ptr, 100, false);
        return res;
    }

    if (target_type != MONSTER_TO_MONSTER) {
        return res;
    }

    bool resists_tele = false;
    const auto t_name = monster_name(player_ptr, t_idx);

    if (tr_ptr->resistance_flags.has(MonsterResistanceType::RESIST_TELEPORT)) {
        if (tr_ptr->kind_flags.has(MonsterKindType::UNIQUE) || tr_ptr->resistance_flags.has(MonsterResistanceType::RESIST_ALL)) {
            if (is_original_ap_and_seen(player_ptr, t_ptr)) {
                tr_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
            }
            if (see_monster(player_ptr, t_idx)) {
                msg_format(_("%s^には効果がなかった。", "%s^ is unaffected!"), t_name.data());
            }
            resists_tele = true;
        } else if (tr_ptr->level > randint1(100)) {
            if (is_original_ap_and_seen(player_ptr, t_ptr)) {
                tr_ptr->r_resistance_flags.set(MonsterResistanceType::RESIST_TELEPORT);
            }
            if (see_monster(player_ptr, t_idx)) {
                msg_format(_("%s^は耐性を持っている！", "%s^ resists!"), t_name.data());
            }
            resists_tele = true;
        }
    }

    if (resists_tele) {
        set_monster_csleep(player_ptr, t_idx, 0);
        return res;
    }

    if (t_idx == player_ptr->riding) {
        teleport_player_away(m_idx, player_ptr, MAX_PLAYER_SIGHT * 2 + 5, false);
    } else {
        teleport_away(player_ptr, t_idx, MAX_PLAYER_SIGHT * 2 + 5, TELEPORT_PASSIVE);
    }
    set_monster_csleep(player_ptr, t_idx, 0);

    return res;
}

/*!
 * @brief RF6_TELE_LEVELの処理。テレポート・レベル。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * ラーニング不可。
 */
MonsterSpellResult spell_RF6_TELE_LEVEL(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    const auto res = MonsterSpellResult::make_valid();

    auto *floor_ptr = player_ptr->current_floor_ptr;
    MonsterEntity *t_ptr = &floor_ptr->m_list[t_idx];
    MonsterRaceInfo *tr_ptr = &monraces_info[t_ptr->r_idx];
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    bool resist, saving_throw;

    if (target_type == MONSTER_TO_PLAYER) {
        resist = (has_resist_nexus(player_ptr) != 0);
        saving_throw = (randint0(100 + rlev / 2) < player_ptr->skill_sav);

        mspell_cast_msg_bad_status_to_player msg(_("%s^が何か奇妙な言葉をつぶやいた。", "%s^ mumbles strangely."),
            _("%s^があなたの足を指さした。", "%s^ gestures at your feet."), _("しかし効果がなかった！", "You are unaffected!"),
            _("しかし効力を跳ね返した！", "You resist the effects!"));

        spell_badstatus_message_to_player(player_ptr, m_idx, msg, resist, saving_throw);

        if (!resist && !saving_throw) {
            teleport_level(player_ptr, 0);
        }

        update_smart_learn(player_ptr, m_idx, DRS_NEXUS);
        return res;
    }

    if (target_type != MONSTER_TO_MONSTER) {
        return res;
    }

    resist = tr_ptr->resistance_flags.has_any_of(RFR_EFF_RESIST_NEXUS_MASK) || tr_ptr->resistance_flags.has(MonsterResistanceType::RESIST_TELEPORT);
    saving_throw = (tr_ptr->flags1 & RF1_QUESTOR) || (tr_ptr->level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10);

    mspell_cast_msg_bad_status_to_monster msg(_("%s^が%sの足を指さした。", "%s^ gestures at %s's feet."),
        _("%s^には効果がなかった。", "%s^ is unaffected!"), _("%s^は効力を跳ね返した！", "%s^ resist the effects!"), "");

    spell_badstatus_message_to_mons(player_ptr, m_idx, t_idx, msg, resist, saving_throw);

    if (!resist && !saving_throw) {
        teleport_level(player_ptr, (t_idx == player_ptr->riding) ? 0 : t_idx);
    }

    return res;
}

/*!
 * @brief RF6_DARKNESSの処理。暗闇or閃光。 /
 * @param target_type プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象かつ暗闇ならラーニング可。
 */
MonsterSpellResult spell_RF6_DARKNESS(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg;
    concptr msg_done;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[m_idx];
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    bool can_use_lite_area = false;
    bool monster_to_monster = target_type == MONSTER_TO_MONSTER;
    bool monster_to_player = target_type == MONSTER_TO_PLAYER;
    const auto t_name = monster_name(player_ptr, t_idx);

    const auto is_ninja = PlayerClass(player_ptr).equals(PlayerClassType::NINJA);
    const auto is_living_monster = r_ptr->kind_flags.has_not(MonsterKindType::UNDEAD);
    const auto is_not_weak_lite = r_ptr->resistance_flags.has_not(MonsterResistanceType::HURT_LITE);
    if (is_ninja && is_living_monster && is_not_weak_lite && r_ptr->brightness_flags.has_none_of(dark_mask)) {
        can_use_lite_area = true;
    }

    const auto &t_ref = floor_ptr->m_list[t_idx];
    if (monster_to_monster && !t_ref.is_hostile()) {
        can_use_lite_area = false;
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = monster_to_player && !can_use_lite_area;

    if (can_use_lite_area) {
        msg.blind = _("%s^が何かをつぶやいた。", "%s^ mumbles.");
        msg.to_player = _("%s^が辺りを明るく照らした。", "%s^ cast a spell to light up.");
        msg.to_mons = _("%s^が辺りを明るく照らした。", "%s^ cast a spell to light up.");

        msg_done = _("%s^は白い光に包まれた。", "%s^ is surrounded by a white light.");
    } else {
        msg.blind = _("%s^が何かをつぶやいた。", "%s^ mumbles.");
        msg.to_player = _("%s^が暗闇の中で手を振った。", "%s^ gestures in shadow.");
        msg.to_mons = _("%s^が暗闇の中で手を振った。", "%s^ gestures in shadow.");

        msg_done = _("%s^は暗闇に包まれた。", "%s^ is surrounded by darkness.");
    }

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    if (see_monster(player_ptr, t_idx) && monster_to_monster) {
        msg_format(msg_done, t_name.data());
    }

    if (monster_to_player) {
        if (can_use_lite_area) {
            (void)lite_area(player_ptr, 0, 3);
        } else {
            (void)unlite_area(player_ptr, 0, 3);
        }
    } else if (monster_to_monster) {
        if (can_use_lite_area) {
            (void)project(player_ptr, m_idx, 3, y, x, 0, AttributeType::LITE_WEAK, PROJECT_GRID | PROJECT_KILL);
            lite_room(player_ptr, y, x);
        } else {
            (void)project(player_ptr, m_idx, 3, y, x, 0, AttributeType::DARK_WEAK, PROJECT_GRID | PROJECT_KILL);
            unlite_room(player_ptr, y, x);
        }
    }

    return res;
}

/*!
 * @brief RF6_TRAPSの処理。トラップ。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 *
 * ラーニング可。
 */
MonsterSpellResult spell_RF6_TRAPS(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx)
{
    const auto m_name = monster_name(player_ptr, m_idx);
    disturb(player_ptr, true, true);

    if (player_ptr->effects()->blindness()->is_blind()) {
        msg_format(_("%s^が何かをつぶやいて邪悪に微笑んだ。", "%s^ mumbles, and then cackles evilly."), m_name.data());
    } else {
        msg_format(_("%s^が呪文を唱えて邪悪に微笑んだ。", "%s^ casts a spell and cackles evilly."), m_name.data());
    }

    (void)trap_creation(player_ptr, y, x);

    auto res = MonsterSpellResult::make_valid();
    res.learnable = true;

    return res;
}

/*!
 * @brief RF6_RAISE_DEADの処理。死者復活。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * ラーニング不可。
 */
MonsterSpellResult spell_RF6_RAISE_DEAD(PlayerType *player_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    mspell_cast_msg_blind msg(_("%s^が何かをつぶやいた。", "%s^ mumbles."),
        _("%s^が死者復活の呪文を唱えた。", "%s^ casts a spell to revive corpses."), _("%s^が死者復活の呪文を唱えた。", "%s^ casts a spell to revive corpses."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    animate_dead(player_ptr, m_idx, m_ptr->fy, m_ptr->fx);

    return MonsterSpellResult::make_valid();
}
