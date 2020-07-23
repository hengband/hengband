#include "mspell/mspell-bolt.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/drs-types.h"
#include "monster/monster-info.h"
#include "monster/monster-update.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-type.h"
#include "mspell/mspell-util.h"
#include "spell/spell-types.h"

/*!
 * @brief RF4_SHOOTの処理。射撃。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF4_SHOOT(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam;

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが奇妙な音を発した。", "%^s makes a strange noise."), _("%^sが矢を放った。", "%^s fires an arrow."),
        _("%^sが%sに矢を放った。", "%^s fires an arrow at %s."), TARGET_TYPE);

    dam = monspell_damage(target_ptr, (MS_SHOOT), m_idx, DAM_ROLL);
    bolt(target_ptr, m_idx, y, x, GF_ARROW, dam, MS_SHOOT, TARGET_TYPE);
    sound(SOUND_SHOOT);

    return dam;
}

/*!
 * @brief RF5_BO_ACIDの処理。アシッド・ボルト。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_BO_ACID(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam;

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがアシッド・ボルトの呪文を唱えた。", "%^s casts a acid bolt."),
        _("%sが%sに向かってアシッド・ボルトの呪文を唱えた。", "%^s casts an acid bolt at %s."), TARGET_TYPE);

    dam = monspell_damage(target_ptr, (MS_BOLT_ACID), m_idx, DAM_ROLL);
    bolt(target_ptr, m_idx, y, x, GF_ACID, dam, MS_BOLT_ACID, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        update_smart_learn(target_ptr, m_idx, DRS_ACID);
        update_smart_learn(target_ptr, m_idx, DRS_REFLECT);
    }

    return dam;
}

/*!
 * @brief RF5_BO_ELECの処理。サンダー・ボルト。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_BO_ELEC(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam;

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがサンダー・ボルトの呪文を唱えた。", "%^s casts a lightning bolt."),
        _("%^sが%sに向かってサンダー・ボルトの呪文を唱えた。", "%^s casts a lightning bolt at %s."), TARGET_TYPE);

    dam = monspell_damage(target_ptr, (MS_BOLT_ELEC), m_idx, DAM_ROLL);
    bolt(target_ptr, m_idx, y, x, GF_ELEC, dam, MS_BOLT_ELEC, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        update_smart_learn(target_ptr, m_idx, DRS_ELEC);
        update_smart_learn(target_ptr, m_idx, DRS_REFLECT);
    }

    return dam;
}

/*!
 * @brief RF5_BO_FIREの処理。ファイア・ボルト。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_BO_FIRE(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam;

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがファイア・ボルトの呪文を唱えた。", "%^s casts a fire bolt."),
        _("%^sが%sに向かってファイア・ボルトの呪文を唱えた。", "%^s casts a fire bolt at %s."), TARGET_TYPE);

    dam = monspell_damage(target_ptr, (MS_BOLT_FIRE), m_idx, DAM_ROLL);
    bolt(target_ptr, m_idx, y, x, GF_FIRE, dam, MS_BOLT_FIRE, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        update_smart_learn(target_ptr, m_idx, DRS_FIRE);
        update_smart_learn(target_ptr, m_idx, DRS_REFLECT);
    }

    return dam;
}

/*!
 * @brief RF5_BO_COLDの処理。アイス・ボルト。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_BO_COLD(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam;

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがアイス・ボルトの呪文を唱えた。", "%^s casts a frost bolt."),
        _("%^sが%sに向かってアイス・ボルトの呪文を唱えた。", "%^s casts a frost bolt at %s."), TARGET_TYPE);

    dam = monspell_damage(target_ptr, (MS_BOLT_COLD), m_idx, DAM_ROLL);
    bolt(target_ptr, m_idx, y, x, GF_COLD, dam, MS_BOLT_COLD, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        update_smart_learn(target_ptr, m_idx, DRS_COLD);
        update_smart_learn(target_ptr, m_idx, DRS_REFLECT);
    }

    return dam;
}

/*!
 * @brief RF5_BO_NETHの処理。地獄の矢。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_BO_NETH(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam;

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."), _("%^sが地獄の矢の呪文を唱えた。", "%^s casts a nether bolt."),
        _("%^sが%sに向かって地獄の矢の呪文を唱えた。", "%^s casts a nether bolt at %s."), TARGET_TYPE);

    dam = monspell_damage(target_ptr, (MS_BOLT_NETHER), m_idx, DAM_ROLL);
    bolt(target_ptr, m_idx, y, x, GF_NETHER, dam, MS_BOLT_NETHER, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        update_smart_learn(target_ptr, m_idx, DRS_NETH);
        update_smart_learn(target_ptr, m_idx, DRS_REFLECT);
    }

    return dam;
}

/*!
 * @brief RF5_BO_WATEの処理。ウォーター・ボルト。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_BO_WATE(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam;

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがウォーター・ボルトの呪文を唱えた。", "%^s casts a water bolt."),
        _("%^sが%sに向かってウォーター・ボルトの呪文を唱えた。", "%^s casts a water bolt at %s."), TARGET_TYPE);

    dam = monspell_damage(target_ptr, (MS_BOLT_WATER), m_idx, DAM_ROLL);
    bolt(target_ptr, m_idx, y, x, GF_WATER, dam, MS_BOLT_WATER, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        update_smart_learn(target_ptr, m_idx, DRS_REFLECT);
    }

    return dam;
}

/*!
 * @brief RF5_BO_MANAの処理。魔力の矢。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_BO_MANA(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam;

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."), _("%^sが魔力の矢の呪文を唱えた。", "%^s casts a mana bolt."),
        _("%^sが%sに向かって魔力の矢の呪文を唱えた。", "%^s casts a mana bolt at %s."), TARGET_TYPE);

    dam = monspell_damage(target_ptr, (MS_BOLT_MANA), m_idx, DAM_ROLL);
    bolt(target_ptr, m_idx, y, x, GF_MANA, dam, MS_BOLT_MANA, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        update_smart_learn(target_ptr, m_idx, DRS_REFLECT);
    }

    return dam;
}

/*!
 * @brief RF5_BO_PLASの処理。プラズマ・ボルト。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_BO_PLAS(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam;

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがプラズマ・ボルトの呪文を唱えた。", "%^s casts a plasma bolt."),
        _("%^sが%sに向かってプラズマ・ボルトの呪文を唱えた。", "%^s casts a plasma bolt at %s."), TARGET_TYPE);

    dam = monspell_damage(target_ptr, (MS_BOLT_PLASMA), m_idx, DAM_ROLL);
    bolt(target_ptr, m_idx, y, x, GF_PLASMA, dam, MS_BOLT_PLASMA, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        update_smart_learn(target_ptr, m_idx, DRS_REFLECT);
    }

    return dam;
}

/*!
 * @brief RF5_BO_ICEEの処理。極寒の矢。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_BO_ICEE(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam;

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."), _("%^sが極寒の矢の呪文を唱えた。", "%^s casts an ice bolt."),
        _("%^sが%sに向かって極寒の矢の呪文を唱えた。", "%^s casts an ice bolt at %s."), TARGET_TYPE);

    dam = monspell_damage(target_ptr, (MS_BOLT_ICE), m_idx, DAM_ROLL);
    bolt(target_ptr, m_idx, y, x, GF_ICE, dam, MS_BOLT_ICE, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        update_smart_learn(target_ptr, m_idx, DRS_COLD);
        update_smart_learn(target_ptr, m_idx, DRS_REFLECT);
    }

    return dam;
}

/*!
 * @brief RF5_MISSILEの処理。マジック・ミサイル。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_MISSILE(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam;

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがマジック・ミサイルの呪文を唱えた。", "%^s casts a magic missile."),
        _("%^sが%sに向かってマジック・ミサイルの呪文を唱えた。", "%^s casts a magic missile at %s."), TARGET_TYPE);

    dam = monspell_damage(target_ptr, (MS_MAGIC_MISSILE), m_idx, DAM_ROLL);
    bolt(target_ptr, m_idx, y, x, GF_MISSILE, dam, MS_MAGIC_MISSILE, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        update_smart_learn(target_ptr, m_idx, DRS_REFLECT);
    }

    return dam;
}
