/*!
 * @brief フロアの一定範囲に効果を及ぼす (悲鳴、テレポート等)スペルの効果
 * @date 2020/05/16
 * @author Hourier
 */

#include "mspell/mspell-floor.h"
#include "blue-magic/blue-magic-checker.h"
#include "core/disturbance.h"
#include "core/player-update-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "mind/drs-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "mspell/mspell-status.h"
#include "mspell/mspell-type.h"
#include "mspell/mspell-util.h"
#include "player/player-personalities-types.h"
#include "player/player-status-flags.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-hex.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief RF4_SHRIEKの処理。叫び。 /
 * @param m_idx 呪文を唱えるモンスターID
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
void spell_RF4_SHRIEK(MONSTER_IDX m_idx, player_type *target_ptr, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    simple_monspell_message(target_ptr, m_idx, t_idx, _("%^sがかん高い金切り声をあげた。", "%^s makes a high pitched shriek."),
        _("%^sが%sに向かって叫んだ。", "%^s shrieks at %s."), TARGET_TYPE);

    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        aggravate_monsters(target_ptr, m_idx);
        return;
    }

    if (TARGET_TYPE == MONSTER_TO_MONSTER) {
        set_monster_csleep(target_ptr, t_idx, 0);
    }
}

/*!
 * @brief RF6_WORLDの処理。時を止める。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 */
HIT_POINT spell_RF6_WORLD(player_type *target_ptr, MONSTER_IDX m_idx)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    MONSTER_IDX who = 0;
    GAME_TEXT m_name[MAX_NLEN];
    monster_name(target_ptr, m_idx, m_name);

    disturb(target_ptr, TRUE, TRUE);
    if (m_ptr->r_idx == MON_DIO)
        who = 1;
    else if (m_ptr->r_idx == MON_WONG)
        who = 3;
    if (!set_monster_timewalk(target_ptr, randint1(2) + 2, who, TRUE))
        return FALSE;
    return who;
}

/*!
 * @brief RF6_BLINKの処理。ショート・テレポート。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param is_quantum_effect 量子的効果によるショート・テレポートの場合時TRUE
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
void spell_RF6_BLINK(player_type *target_ptr, MONSTER_IDX m_idx, int TARGET_TYPE, bool is_quantum_effect)
{
    GAME_TEXT m_name[MAX_NLEN];
    monster_name(target_ptr, m_idx, m_name);

    if (TARGET_TYPE == MONSTER_TO_PLAYER)
        disturb(target_ptr, TRUE, TRUE);

    if (!is_quantum_effect && teleport_barrier(target_ptr, m_idx)) {
        if (see_monster(target_ptr, m_idx))
            msg_format(_("魔法のバリアが%^sのテレポートを邪魔した。", "Magic barrier obstructs teleporting of %^s."), m_name);
        return;
    }

    if (see_monster(target_ptr, m_idx))
        msg_format(_("%^sが瞬時に消えた。", "%^s blinks away."), m_name);

    teleport_away(target_ptr, m_idx, 10, TELEPORT_SPONTANEOUS);

    if (TARGET_TYPE == MONSTER_TO_PLAYER)
        target_ptr->update |= (PU_MONSTERS);
}

/*!
 * @brief RF6_TPORTの処理。テレポート。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
void spell_RF6_TPORT(player_type *target_ptr, MONSTER_IDX m_idx, int TARGET_TYPE)
{
    GAME_TEXT m_name[MAX_NLEN];
    monster_name(target_ptr, m_idx, m_name);

    if (TARGET_TYPE == MONSTER_TO_PLAYER)
        disturb(target_ptr, TRUE, TRUE);
    if (teleport_barrier(target_ptr, m_idx)) {
        if (see_monster(target_ptr, m_idx))
            msg_format(_("魔法のバリアが%^sのテレポートを邪魔した。", "Magic barrier obstructs teleporting of %^s."), m_name);
        return;
    }

    if (see_monster(target_ptr, m_idx))
        msg_format(_("%^sがテレポートした。", "%^s teleports away."), m_name);

    teleport_away_followable(target_ptr, m_idx);
}

/*!
 * @brief RF6_TELE_TOの処理。テレポート・バック。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
void spell_RF6_TELE_TO(player_type *target_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    monster_type *t_ptr = &floor_ptr->m_list[t_idx];
    monster_race *tr_ptr = &r_info[t_ptr->r_idx];

    simple_monspell_message(target_ptr, m_idx, t_idx, _("%^sがあなたを引き戻した。", "%^s commands you to return."),
        _("%^sが%sを引き戻した。", "%^s commands %s to return."), TARGET_TYPE);

    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        teleport_player_to(target_ptr, m_ptr->fy, m_ptr->fx, TELEPORT_PASSIVE);
        learn_spell(target_ptr, MS_TELE_TO);
        return;
    }

    if (TARGET_TYPE != MONSTER_TO_MONSTER)
        return;

    bool resists_tele = FALSE;
    GAME_TEXT t_name[MAX_NLEN];
    monster_name(target_ptr, t_idx, t_name);

    if (tr_ptr->flagsr & RFR_RES_TELE) {
        if ((tr_ptr->flags1 & RF1_UNIQUE) || (tr_ptr->flagsr & RFR_RES_ALL)) {
            if (is_original_ap_and_seen(target_ptr, t_ptr))
                tr_ptr->r_flagsr |= RFR_RES_TELE;
            if (see_monster(target_ptr, t_idx)) {
                msg_format(_("%^sには効果がなかった。", "%^s is unaffected!"), t_name);
            }
            resists_tele = TRUE;
        } else if (tr_ptr->level > randint1(100)) {
            if (is_original_ap_and_seen(target_ptr, t_ptr))
                tr_ptr->r_flagsr |= RFR_RES_TELE;
            if (see_monster(target_ptr, t_idx)) {
                msg_format(_("%^sは耐性を持っている！", "%^s resists!"), t_name);
            }
            resists_tele = TRUE;
        }
    }

    if (resists_tele) {
        set_monster_csleep(target_ptr, t_idx, 0);
        return;
    }

    if (t_idx == target_ptr->riding)
        teleport_player_to(target_ptr, m_ptr->fy, m_ptr->fx, TELEPORT_PASSIVE);
    else
        teleport_monster_to(target_ptr, t_idx, m_ptr->fy, m_ptr->fx, 100, TELEPORT_PASSIVE);
    set_monster_csleep(target_ptr, t_idx, 0);
}

/*!
 * @brief RF6_TELE_AWAYの処理。テレポート・アウェイ。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
void spell_RF6_TELE_AWAY(player_type *target_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monster_type *t_ptr = &floor_ptr->m_list[t_idx];
    monster_race *tr_ptr = &r_info[t_ptr->r_idx];

    simple_monspell_message(target_ptr, m_idx, t_idx, _("%^sにテレポートさせられた。", "%^s teleports you away."),
        _("%^sは%sをテレポートさせた。", "%^s teleports %s away."), TARGET_TYPE);

    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        if (is_echizen(target_ptr))
            msg_print(_("くっそ～", ""));
        else if (target_ptr->pseikaku == PERSONALITY_CHARGEMAN) {
            if (randint0(2) == 0)
                msg_print(_("ジュラル星人め！", ""));
            else
                msg_print(_("弱い者いじめは止めるんだ！", ""));
        }

        learn_spell(target_ptr, MS_TELE_AWAY);
        teleport_player_away(m_idx, target_ptr, 100, FALSE);
        return;
    }

    if (TARGET_TYPE != MONSTER_TO_MONSTER)
        return;

    bool resists_tele = FALSE;
    GAME_TEXT t_name[MAX_NLEN];
    monster_name(target_ptr, t_idx, t_name);

    if (tr_ptr->flagsr & RFR_RES_TELE) {
        if ((tr_ptr->flags1 & RF1_UNIQUE) || (tr_ptr->flagsr & RFR_RES_ALL)) {
            if (is_original_ap_and_seen(target_ptr, t_ptr))
                tr_ptr->r_flagsr |= RFR_RES_TELE;
            if (see_monster(target_ptr, t_idx)) {
                msg_format(_("%^sには効果がなかった。", "%^s is unaffected!"), t_name);
            }
            resists_tele = TRUE;
        } else if (tr_ptr->level > randint1(100)) {
            if (is_original_ap_and_seen(target_ptr, t_ptr))
                tr_ptr->r_flagsr |= RFR_RES_TELE;
            if (see_monster(target_ptr, t_idx)) {
                msg_format(_("%^sは耐性を持っている！", "%^s resists!"), t_name);
            }
            resists_tele = TRUE;
        }
    }

    if (resists_tele) {
        set_monster_csleep(target_ptr, t_idx, 0);
        return;
    }

    if (t_idx == target_ptr->riding)
        teleport_player_away(m_idx, target_ptr, MAX_SIGHT * 2 + 5, FALSE);
    else
        teleport_away(target_ptr, t_idx, MAX_SIGHT * 2 + 5, TELEPORT_PASSIVE);
    set_monster_csleep(target_ptr, t_idx, 0);
}

/*!
 * @brief RF6_TELE_LEVELの処理。テレポート・レベル。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
void spell_RF6_TELE_LEVEL(player_type *target_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monster_type *t_ptr = &floor_ptr->m_list[t_idx];
    monster_race *tr_ptr = &r_info[t_ptr->r_idx];
    DEPTH rlev = monster_level_idx(floor_ptr, m_idx);
    bool resist, saving_throw;

    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        resist = (has_resist_nexus(target_ptr) != 0);
        saving_throw = (randint0(100 + rlev / 2) < target_ptr->skill_sav);
        spell_badstatus_message(target_ptr, m_idx, t_idx, _("%^sが何か奇妙な言葉をつぶやいた。", "%^s mumbles strangely."),
            _("%^sがあなたの足を指さした。", "%^s gestures at your feet."), _("しかし効果がなかった！", "You are unaffected!"),
            _("しかし効力を跳ね返した！", "You resist the effects!"), resist, saving_throw, TARGET_TYPE);

        if (!resist && !saving_throw) {
            teleport_level(target_ptr, 0);
        }

        learn_spell(target_ptr, MS_TELE_LEVEL);
        update_smart_learn(target_ptr, m_idx, DRS_NEXUS);
        return;
    }

    if (TARGET_TYPE != MONSTER_TO_MONSTER)
        return;

    resist = tr_ptr->flagsr & (RFR_EFF_RES_NEXU_MASK | RFR_RES_TELE);
    saving_throw = (tr_ptr->flags1 & RF1_QUESTOR) || (tr_ptr->level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10);

    spell_badstatus_message(target_ptr, m_idx, t_idx, _("%^sが%sの足を指さした。", "%^s gestures at %s's feet."),
        _("%^sには効果がなかった。", "%^s is unaffected!"), _("%^sは効力を跳ね返した！", "%^s resist the effects!"), "", resist, saving_throw, TARGET_TYPE);

    if (!resist && !saving_throw) {
        teleport_level(target_ptr, (t_idx == target_ptr->riding) ? 0 : t_idx);
    }
}

/*!
 * @brief RF6_DARKNESSの処理。暗闇or閃光。 /
 * @param target_type プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
void spell_RF6_DARKNESS(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    monster_type *t_ptr = &floor_ptr->m_list[t_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    bool can_use_lite_area = FALSE;
    bool monster_to_monster = TARGET_TYPE == MONSTER_TO_MONSTER;
    bool monster_to_player = TARGET_TYPE == MONSTER_TO_PLAYER;
    GAME_TEXT t_name[MAX_NLEN];
    monster_name(target_ptr, t_idx, t_name);

    if ((target_ptr->pclass == CLASS_NINJA) && !(r_ptr->flags3 & (RF3_UNDEAD | RF3_HURT_LITE)) && !(r_ptr->flags7 & RF7_DARK_MASK))
        can_use_lite_area = TRUE;

    if (monster_to_monster && !is_hostile(t_ptr))
        can_use_lite_area = FALSE;

    if (can_use_lite_area) {
        monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."),
            _("%^sが辺りを明るく照らした。", "%^s cast a spell to light up."), _("%^sが辺りを明るく照らした。", "%^s cast a spell to light up."), TARGET_TYPE);

        if (see_monster(target_ptr, t_idx) && monster_to_monster) {
            msg_format(_("%^sは白い光に包まれた。", "%^s is surrounded by a white light."), t_name);
        }
    } else {
        monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."), _("%^sが暗闇の中で手を振った。", "%^s gestures in shadow."),
            _("%^sが暗闇の中で手を振った。", "%^s gestures in shadow."), TARGET_TYPE);

        if (see_monster(target_ptr, t_idx) && monster_to_monster) {
            msg_format(_("%^sは暗闇に包まれた。", "%^s is surrounded by darkness."), t_name);
        }
    }

    if (monster_to_player) {
        if (can_use_lite_area) {
            (void)lite_area(target_ptr, 0, 3);
        } else {
            learn_spell(target_ptr, MS_DARKNESS);
            (void)unlite_area(target_ptr, 0, 3);
        }

        return;
    }

    if (!monster_to_monster)
        return;

    int lite_area = can_use_lite_area ? -1 : MS_DARKNESS;
    (void)project(target_ptr, m_idx, 3, y, x, 0, GF_LITE_WEAK, PROJECT_GRID | PROJECT_KILL, lite_area);
    lite_room(target_ptr, y, x);
}

/*!
 * @brief RF6_TRAPSの処理。トラップ。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param なし
 */
void spell_RF6_TRAPS(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx)
{
    GAME_TEXT m_name[MAX_NLEN];
    monster_name(target_ptr, m_idx, m_name);
    disturb(target_ptr, TRUE, TRUE);

    if (target_ptr->blind)
        msg_format(_("%^sが何かをつぶやいて邪悪に微笑んだ。", "%^s mumbles, and then cackles evilly."), m_name);
    else
        msg_format(_("%^sが呪文を唱えて邪悪に微笑んだ。", "%^s casts a spell and cackles evilly."), m_name);

    learn_spell(target_ptr, MS_MAKE_TRAP);
    (void)trap_creation(target_ptr, y, x);
}

/*!
 * @brief RF6_RAISE_DEADの処理。死者復活。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
void spell_RF6_RAISE_DEAD(player_type *target_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sが死者復活の呪文を唱えた。", "%^s casts a spell to revive corpses."), _("%^sが死者復活の呪文を唱えた。", "%^s casts a spell to revive corpses."),
        TARGET_TYPE);

    animate_dead(target_ptr, m_idx, m_ptr->fy, m_ptr->fx);
}
