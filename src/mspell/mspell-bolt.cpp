#include "mspell/mspell-bolt.h"
#include "effect/effect-processor.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/drs-types.h"
#include "monster-race/race-ability-flags.h"
#include "monster/monster-info.h"
#include "monster/monster-update.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-util.h"
#include "mspell/mspell.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"

/*!
 * @brief RF4_SHOOTの処理。射撃。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF4_SHOOT(player_type *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    bool notice = monspell_message(player_ptr, m_idx, t_idx, _("%^sが奇妙な音を発した。", "%^s makes a strange noise."),
        _("%^sが矢を放った。", "%^s fires an arrow."),
        _("%^sが%sに矢を放った。", "%^s fires an arrow at %s."), TARGET_TYPE);

    if (notice) {
        sound(SOUND_SHOOT);
    }

    const auto dam = monspell_damage(player_ptr, RF_ABILITY::SHOOT, m_idx, DAM_ROLL);
    const auto proj_res = bolt(player_ptr, m_idx, y, x, GF_ARROW, dam, TARGET_TYPE);

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_BO_ACIDの処理。アシッド・ボルト。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BO_ACID(player_type *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    monspell_message(player_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがアシッド・ボルトの呪文を唱えた。", "%^s casts an acid bolt."),
        _("%sが%sに向かってアシッド・ボルトの呪文を唱えた。", "%^s casts an acid bolt at %s."), TARGET_TYPE);

    const auto dam = monspell_damage(player_ptr, RF_ABILITY::BO_ACID, m_idx, DAM_ROLL);
    const auto proj_res = bolt(player_ptr, m_idx, y, x, GF_ACID, dam, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        update_smart_learn(player_ptr, m_idx, DRS_ACID);
        update_smart_learn(player_ptr, m_idx, DRS_REFLECT);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_BO_ELECの処理。サンダー・ボルト。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BO_ELEC(player_type *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    monspell_message(player_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがサンダー・ボルトの呪文を唱えた。", "%^s casts a lightning bolt."),
        _("%^sが%sに向かってサンダー・ボルトの呪文を唱えた。", "%^s casts a lightning bolt at %s."), TARGET_TYPE);

    const auto dam = monspell_damage(player_ptr, RF_ABILITY::BO_ELEC, m_idx, DAM_ROLL);
    const auto proj_res = bolt(player_ptr, m_idx, y, x, GF_ELEC, dam, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        update_smart_learn(player_ptr, m_idx, DRS_ELEC);
        update_smart_learn(player_ptr, m_idx, DRS_REFLECT);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_BO_FIREの処理。ファイア・ボルト。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BO_FIRE(player_type *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    monspell_message(player_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがファイア・ボルトの呪文を唱えた。", "%^s casts a fire bolt."),
        _("%^sが%sに向かってファイア・ボルトの呪文を唱えた。", "%^s casts a fire bolt at %s."), TARGET_TYPE);

    const auto dam = monspell_damage(player_ptr, RF_ABILITY::BO_FIRE, m_idx, DAM_ROLL);
    const auto proj_res = bolt(player_ptr, m_idx, y, x, GF_FIRE, dam, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        update_smart_learn(player_ptr, m_idx, DRS_FIRE);
        update_smart_learn(player_ptr, m_idx, DRS_REFLECT);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_BO_COLDの処理。アイス・ボルト。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BO_COLD(player_type *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    monspell_message(player_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがアイス・ボルトの呪文を唱えた。", "%^s casts a frost bolt."),
        _("%^sが%sに向かってアイス・ボルトの呪文を唱えた。", "%^s casts a frost bolt at %s."), TARGET_TYPE);

    const auto dam = monspell_damage(player_ptr, RF_ABILITY::BO_COLD, m_idx, DAM_ROLL);
    const auto proj_res = bolt(player_ptr, m_idx, y, x, GF_COLD, dam, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        update_smart_learn(player_ptr, m_idx, DRS_COLD);
        update_smart_learn(player_ptr, m_idx, DRS_REFLECT);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_BO_NETHの処理。地獄の矢。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BO_NETH(player_type *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    monspell_message(player_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."), _("%^sが地獄の矢の呪文を唱えた。", "%^s casts a nether bolt."),
        _("%^sが%sに向かって地獄の矢の呪文を唱えた。", "%^s casts a nether bolt at %s."), TARGET_TYPE);

    const auto dam = monspell_damage(player_ptr, RF_ABILITY::BO_NETH, m_idx, DAM_ROLL);
    const auto proj_res = bolt(player_ptr, m_idx, y, x, GF_NETHER, dam, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        update_smart_learn(player_ptr, m_idx, DRS_NETH);
        update_smart_learn(player_ptr, m_idx, DRS_REFLECT);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_BO_WATEの処理。ウォーター・ボルト。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BO_WATE(player_type *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    monspell_message(player_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがウォーター・ボルトの呪文を唱えた。", "%^s casts a water bolt."),
        _("%^sが%sに向かってウォーター・ボルトの呪文を唱えた。", "%^s casts a water bolt at %s."), TARGET_TYPE);

    const auto dam = monspell_damage(player_ptr, RF_ABILITY::BO_WATE, m_idx, DAM_ROLL);
    const auto proj_res = bolt(player_ptr, m_idx, y, x, GF_WATER, dam, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        update_smart_learn(player_ptr, m_idx, DRS_REFLECT);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_BO_MANAの処理。魔力の矢。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BO_MANA(player_type *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    monspell_message(player_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."), _("%^sが魔力の矢の呪文を唱えた。", "%^s casts a mana bolt."),
        _("%^sが%sに向かって魔力の矢の呪文を唱えた。", "%^s casts a mana bolt at %s."), TARGET_TYPE);

    const auto dam = monspell_damage(player_ptr, RF_ABILITY::BO_MANA, m_idx, DAM_ROLL);
    const auto proj_res = bolt(player_ptr, m_idx, y, x, GF_MANA, dam, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        update_smart_learn(player_ptr, m_idx, DRS_REFLECT);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_BO_PLASの処理。プラズマ・ボルト。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BO_PLAS(player_type *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    monspell_message(player_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがプラズマ・ボルトの呪文を唱えた。", "%^s casts a plasma bolt."),
        _("%^sが%sに向かってプラズマ・ボルトの呪文を唱えた。", "%^s casts a plasma bolt at %s."), TARGET_TYPE);

    const auto dam = monspell_damage(player_ptr, RF_ABILITY::BO_PLAS, m_idx, DAM_ROLL);
    const auto proj_res = bolt(player_ptr, m_idx, y, x, GF_PLASMA, dam, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        update_smart_learn(player_ptr, m_idx, DRS_REFLECT);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_BO_ICEEの処理。極寒の矢。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BO_ICEE(player_type *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    monspell_message(player_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."), _("%^sが極寒の矢の呪文を唱えた。", "%^s casts an ice bolt."),
        _("%^sが%sに向かって極寒の矢の呪文を唱えた。", "%^s casts an ice bolt at %s."), TARGET_TYPE);

    const auto dam = monspell_damage(player_ptr, RF_ABILITY::BO_ICEE, m_idx, DAM_ROLL);
    const auto proj_res = bolt(player_ptr, m_idx, y, x, GF_ICE, dam, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        update_smart_learn(player_ptr, m_idx, DRS_COLD);
        update_smart_learn(player_ptr, m_idx, DRS_REFLECT);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_MISSILEの処理。マジック・ミサイル。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_MISSILE(player_type *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    monspell_message(player_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがマジック・ミサイルの呪文を唱えた。", "%^s casts a magic missile."),
        _("%^sが%sに向かってマジック・ミサイルの呪文を唱えた。", "%^s casts a magic missile at %s."), TARGET_TYPE);

    const auto dam = monspell_damage(player_ptr, RF_ABILITY::MISSILE, m_idx, DAM_ROLL);
    const auto proj_res = bolt(player_ptr, m_idx, y, x, GF_MISSILE, dam, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        update_smart_learn(player_ptr, m_idx, DRS_REFLECT);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}
