#include "mspell/mspell-ball.h"
#include "main/sound-of-music.h"
#include "mind/drs-types.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-type.h"
#include "mspell/mspell-util.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief RF4_BA_NUKEの処理。放射能球。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF4_BA_NUKE(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam;

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."), _("%^sが放射能球を放った。", "%^s casts a ball of radiation."),
        _("%^sが%sに放射能球を放った。", "%^s casts a ball of radiation at %s."), TARGET_TYPE);

    dam = monspell_damage(target_ptr, (MS_BALL_NUKE), m_idx, DAM_ROLL);
    breath(target_ptr, y, x, m_idx, GF_NUKE, dam, 2, FALSE, MS_BALL_NUKE, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER)
        update_smart_learn(target_ptr, m_idx, DRS_POIS);

    return dam;
}

/*!
 * @brief RF4_BA_CHAOの処理。純ログルス。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF4_BA_CHAO(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam;

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが恐ろしげにつぶやいた。", "%^s mumbles frighteningly."),
        _("%^sが純ログルスを放った。", "%^s invokes a raw Logrus."), _("%^sが%sに純ログルスを放った。", "%^s invokes raw Logrus upon %s."), TARGET_TYPE);

    dam = monspell_damage(target_ptr, (MS_BALL_CHAOS), m_idx, DAM_ROLL);
    breath(target_ptr, y, x, m_idx, GF_CHAOS, dam, 4, FALSE, MS_BALL_CHAOS, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER)
        update_smart_learn(target_ptr, m_idx, DRS_CHAOS);

    return dam;
}

/*!
 * @brief RF5_BA_ACIDの処理。アシッド・ボール。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_BA_ACID(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam, rad;

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがアシッド・ボールの呪文を唱えた。", "%^s casts an acid ball."),
        _("%^sが%sに向かってアシッド・ボールの呪文を唱えた。", "%^s casts an acid ball at %s."), TARGET_TYPE);

    rad = monster_is_powerful(target_ptr->current_floor_ptr, m_idx) ? 4 : 2;
    dam = monspell_damage(target_ptr, (MS_BALL_ACID), m_idx, DAM_ROLL);
    breath(target_ptr, y, x, m_idx, GF_ACID, dam, rad, FALSE, MS_BALL_ACID, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER)
        update_smart_learn(target_ptr, m_idx, DRS_ACID);

    return dam;
}

/*!
 * @brief RF5_BA_ELECの処理。サンダー・ボール。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_BA_ELEC(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam, rad;

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがサンダー・・ボールの呪文を唱えた。", "%^s casts a lightning ball."),
        _("%^sが%sに向かってサンダー・ボールの呪文を唱えた。", "%^s casts a lightning ball at %s."), TARGET_TYPE);

    rad = monster_is_powerful(target_ptr->current_floor_ptr, m_idx) ? 4 : 2;
    dam = monspell_damage(target_ptr, (MS_BALL_ELEC), m_idx, DAM_ROLL);
    breath(target_ptr, y, x, m_idx, GF_ELEC, dam, rad, FALSE, MS_BALL_ELEC, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER)
        update_smart_learn(target_ptr, m_idx, DRS_ELEC);

    return dam;
}

/*!
 * @brief RF5_BA_FIREの処理。ファイア・ボール。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_BA_FIRE(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam, rad;
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];

    if (m_ptr->r_idx == MON_ROLENTO) {
        monspell_message(target_ptr, m_idx, t_idx, _("%sが何かを投げた。", "%^s throws something."), _("%sは手榴弾を投げた。", "%^s throws a hand grenade."),
            _("%^sが%^sに向かって手榴弾を投げた。", "%^s throws a hand grenade."), TARGET_TYPE);
    } else {
        monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."),
            _("%^sがファイア・ボールの呪文を唱えた。", "%^s casts a fire ball."),
            _("%^sが%sに向かってファイア・ボールの呪文を唱えた。", "%^s casts a fire ball at %s."), TARGET_TYPE);
    }
    rad = monster_is_powerful(target_ptr->current_floor_ptr, m_idx) ? 4 : 2;
    dam = monspell_damage(target_ptr, (MS_BALL_FIRE), m_idx, DAM_ROLL);
    breath(target_ptr, y, x, m_idx, GF_FIRE, dam, rad, FALSE, MS_BALL_FIRE, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER)
        update_smart_learn(target_ptr, m_idx, DRS_FIRE);

    return dam;
}

/*!
 * @brief RF5_BA_COLDの処理。アイス・ボール。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_BA_COLD(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam, rad;

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがアイス・ボールの呪文を唱えた。", "%^s casts a frost ball."),
        _("%^sが%sに向かってアイス・ボールの呪文を唱えた。", "%^s casts a frost ball at %s."), TARGET_TYPE);

    rad = monster_is_powerful(target_ptr->current_floor_ptr, m_idx) ? 4 : 2;
    dam = monspell_damage(target_ptr, (MS_BALL_COLD), m_idx, DAM_ROLL);
    breath(target_ptr, y, x, m_idx, GF_COLD, dam, rad, FALSE, MS_BALL_COLD, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER)
        update_smart_learn(target_ptr, m_idx, DRS_COLD);

    return dam;
}

/*!
 * @brief RF5_BA_POISの処理。悪臭雲。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_BA_POIS(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam;

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."), _("%^sが悪臭雲の呪文を唱えた。", "%^s casts a stinking cloud."),
        _("%^sが%sに向かって悪臭雲の呪文を唱えた。", "%^s casts a stinking cloud at %s."), TARGET_TYPE);

    dam = monspell_damage(target_ptr, (MS_BALL_POIS), m_idx, DAM_ROLL);
    breath(target_ptr, y, x, m_idx, GF_POIS, dam, 2, FALSE, MS_BALL_POIS, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER)
        update_smart_learn(target_ptr, m_idx, DRS_POIS);

    return dam;
}

/*!
 * @brief RF5_BA_NETHの処理。地獄球。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_BA_NETH(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam;

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."), _("%^sが地獄球の呪文を唱えた。", "%^s casts a nether ball."),
        _("%^sが%sに向かって地獄球の呪文を唱えた。", "%^s casts a nether ball at %s."), TARGET_TYPE);

    dam = monspell_damage(target_ptr, (MS_BALL_NETHER), m_idx, DAM_ROLL);
    breath(target_ptr, y, x, m_idx, GF_NETHER, dam, 2, FALSE, MS_BALL_NETHER, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER)
        update_smart_learn(target_ptr, m_idx, DRS_NETH);

    return dam;
}

/*!
 * @brief RF5_BA_WATEの処理。ウォーター・ボール。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_BA_WATE(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam;
    bool known = monster_near_player(target_ptr->current_floor_ptr, m_idx, t_idx);
    bool see_either = see_monster(target_ptr, m_idx) || see_monster(target_ptr, t_idx);
    bool mon_to_mon = (TARGET_TYPE == MONSTER_TO_MONSTER);
    bool mon_to_player = (TARGET_TYPE == MONSTER_TO_PLAYER);
    GAME_TEXT t_name[MAX_NLEN];
    monster_name(target_ptr, t_idx, t_name);

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."), _("%^sが流れるような身振りをした。", "%^s gestures fluidly."),
        _("%^sが%sに対して流れるような身振りをした。", "%^s gestures fluidly at %s."), TARGET_TYPE);

    if (mon_to_player) {
        msg_format(_("あなたは渦巻きに飲み込まれた。", "You are engulfed in a whirlpool."));
    } else if (mon_to_mon && known && see_either && !target_ptr->blind) {
        msg_format(_("%^sは渦巻に飲み込まれた。", "%^s is engulfed in a whirlpool."), t_name);
    }

    dam = monspell_damage(target_ptr, (MS_BALL_WATER), m_idx, DAM_ROLL);
    breath(target_ptr, y, x, m_idx, GF_WATER, dam, 4, FALSE, MS_BALL_WATER, TARGET_TYPE);
    return dam;
}

/*!
 * @brief RF5_BA_MANAの処理。魔力の嵐。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_BA_MANA(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam;

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かを力強くつぶやいた。", "%^s mumbles powerfully."),
        _("%^sが魔力の嵐の呪文を念じた。", "%^s invokes a mana storm."), _("%^sが%sに対して魔力の嵐の呪文を念じた。", "%^s invokes a mana storm upon %s."),
        TARGET_TYPE);

    dam = monspell_damage(target_ptr, (MS_BALL_MANA), m_idx, DAM_ROLL);
    breath(target_ptr, y, x, m_idx, GF_MANA, dam, 4, FALSE, MS_BALL_MANA, TARGET_TYPE);
    return dam;
}

/*!
 * @brief RF5_BA_DARKの処理。暗黒の嵐。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_BA_DARK(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam;

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かを力強くつぶやいた。", "%^s mumbles powerfully."),
        _("%^sが暗黒の嵐の呪文を念じた。", "%^s invokes a darkness storm."),
        _("%^sが%sに対して暗黒の嵐の呪文を念じた。", "%^s invokes a darkness storm upon %s."), TARGET_TYPE);

    dam = monspell_damage(target_ptr, (MS_BALL_DARK), m_idx, DAM_ROLL);
    breath(target_ptr, y, x, m_idx, GF_DARK, dam, 4, FALSE, MS_BALL_DARK, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER)
        update_smart_learn(target_ptr, m_idx, DRS_DARK);

    return dam;
}

/*!
 * @brief RF5_BA_LITEの処理。スターバースト。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_BA_LITE(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam;

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かを力強くつぶやいた。", "%^s mumbles powerfully."),
        _("%^sがスターバーストの呪文を念じた。", "%^s invokes a starburst."),
        _("%^sが%sに対してスターバーストの呪文を念じた。", "%^s invokes a starburst upon %s."), TARGET_TYPE);

    dam = monspell_damage(target_ptr, (MS_STARBURST), m_idx, DAM_ROLL);
    breath(target_ptr, y, x, m_idx, GF_LITE, dam, 4, FALSE, MS_STARBURST, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER)
        update_smart_learn(target_ptr, m_idx, DRS_LITE);

    return dam;
}
