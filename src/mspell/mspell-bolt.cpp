#include "mspell/mspell-bolt.h"
#include "effect/attribute-types.h"
#include "effect/effect-processor.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/drs-types.h"
#include "monster-race/race-ability-flags.h"
#include "monster/monster-info.h"
#include "monster/monster-update.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-result.h"
#include "mspell/mspell-util.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"

/*!
 * @brief RF4_SHOOTの処理。射撃。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF4_SHOOT(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%^sが奇妙な音を発した。", "%^s makes a strange noise."),
        _("%^sが矢を放った。", "%^s fires an arrow."),
        _("%^sが%sに矢を放った。", "%^s fires an arrow at %s."));

    bool notice = monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    if (notice) {
        sound(SOUND_SHOOT);
    }

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::SHOOT, m_idx, DAM_ROLL);
    const auto proj_res = bolt(player_ptr, m_idx, y, x, AttributeType::MONSTER_SHOOT, dam, target_type);

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
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BO_ACID(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがアシッド・ボルトの呪文を唱えた。", "%^s casts an acid bolt."),
        _("%sが%sに向かってアシッド・ボルトの呪文を唱えた。", "%^s casts an acid bolt at %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BO_ACID, m_idx, DAM_ROLL);
    const auto proj_res = bolt(player_ptr, m_idx, y, x, AttributeType::ACID, dam, target_type);
    if (target_type == MONSTER_TO_PLAYER) {
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
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BO_ELEC(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがサンダー・ボルトの呪文を唱えた。", "%^s casts a lightning bolt."),
        _("%^sが%sに向かってサンダー・ボルトの呪文を唱えた。", "%^s casts a lightning bolt at %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BO_ELEC, m_idx, DAM_ROLL);
    const auto proj_res = bolt(player_ptr, m_idx, y, x, AttributeType::ELEC, dam, target_type);
    if (target_type == MONSTER_TO_PLAYER) {
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
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BO_FIRE(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがファイア・ボルトの呪文を唱えた。", "%^s casts a fire bolt."),
        _("%^sが%sに向かってファイア・ボルトの呪文を唱えた。", "%^s casts a fire bolt at %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BO_FIRE, m_idx, DAM_ROLL);
    const auto proj_res = bolt(player_ptr, m_idx, y, x, AttributeType::FIRE, dam, target_type);
    if (target_type == MONSTER_TO_PLAYER) {
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
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BO_COLD(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがアイス・ボルトの呪文を唱えた。", "%^s casts a frost bolt."),
        _("%^sが%sに向かってアイス・ボルトの呪文を唱えた。", "%^s casts a frost bolt at %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BO_COLD, m_idx, DAM_ROLL);
    const auto proj_res = bolt(player_ptr, m_idx, y, x, AttributeType::COLD, dam, target_type);
    if (target_type == MONSTER_TO_PLAYER) {
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
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BO_NETH(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."), _("%^sが地獄の矢の呪文を唱えた。", "%^s casts a nether bolt."),
        _("%^sが%sに向かって地獄の矢の呪文を唱えた。", "%^s casts a nether bolt at %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BO_NETH, m_idx, DAM_ROLL);
    const auto proj_res = bolt(player_ptr, m_idx, y, x, AttributeType::NETHER, dam, target_type);
    if (target_type == MONSTER_TO_PLAYER) {
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
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BO_WATE(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがウォーター・ボルトの呪文を唱えた。", "%^s casts a water bolt."),
        _("%^sが%sに向かってウォーター・ボルトの呪文を唱えた。", "%^s casts a water bolt at %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BO_WATE, m_idx, DAM_ROLL);
    const auto proj_res = bolt(player_ptr, m_idx, y, x, AttributeType::WATER, dam, target_type);
    if (target_type == MONSTER_TO_PLAYER) {
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
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BO_MANA(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."), _("%^sが魔力の矢の呪文を唱えた。", "%^s casts a mana bolt."),
        _("%^sが%sに向かって魔力の矢の呪文を唱えた。", "%^s casts a mana bolt at %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BO_MANA, m_idx, DAM_ROLL);
    const auto proj_res = bolt(player_ptr, m_idx, y, x, AttributeType::MANA, dam, target_type);
    if (target_type == MONSTER_TO_PLAYER) {
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
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BO_PLAS(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがプラズマ・ボルトの呪文を唱えた。", "%^s casts a plasma bolt."),
        _("%^sが%sに向かってプラズマ・ボルトの呪文を唱えた。", "%^s casts a plasma bolt at %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BO_PLAS, m_idx, DAM_ROLL);
    const auto proj_res = bolt(player_ptr, m_idx, y, x, AttributeType::PLASMA, dam, target_type);
    if (target_type == MONSTER_TO_PLAYER) {
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
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BO_ICEE(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."), _("%^sが極寒の矢の呪文を唱えた。", "%^s casts an ice bolt."),
        _("%^sが%sに向かって極寒の矢の呪文を唱えた。", "%^s casts an ice bolt at %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BO_ICEE, m_idx, DAM_ROLL);
    const auto proj_res = bolt(player_ptr, m_idx, y, x, AttributeType::ICE, dam, target_type);
    if (target_type == MONSTER_TO_PLAYER) {
        update_smart_learn(player_ptr, m_idx, DRS_COLD);
        update_smart_learn(player_ptr, m_idx, DRS_REFLECT);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_BO_VOIDの処理。ヴォイド・ボルト。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BO_VOID(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがヴォイド・ボルトの呪文を唱えた。", "%^s casts a void bolt."),
        _("%^sが%sに向かってヴォイド・ボルトの呪文を唱えた。", "%^s casts a void bolt at %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BO_VOID, m_idx, DAM_ROLL);
    const auto proj_res = bolt(player_ptr, m_idx, y, x, AttributeType::VOID_MAGIC, dam, target_type);
    if (target_type == MONSTER_TO_PLAYER) {
        update_smart_learn(player_ptr, m_idx, DRS_REFLECT);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_BO_ABYSSの処理。アビス・ボルト。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BO_ABYSS(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがアビス・ボルトの呪文を唱えた。", "%^s casts a abyss bolt."),
        _("%^sが%sに向かってアビス・ボルトの呪文を唱えた。", "%^s casts a abyss bolt at %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BO_ABYSS, m_idx, DAM_ROLL);
    const auto proj_res = bolt(player_ptr, m_idx, y, x, AttributeType::ABYSS, dam, target_type);
    if (target_type == MONSTER_TO_PLAYER) {
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
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_MISSILE(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがマジック・ミサイルの呪文を唱えた。", "%^s casts a magic missile."),
        _("%^sが%sに向かってマジック・ミサイルの呪文を唱えた。", "%^s casts a magic missile at %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::MISSILE, m_idx, DAM_ROLL);
    const auto proj_res = bolt(player_ptr, m_idx, y, x, AttributeType::MISSILE, dam, target_type);
    if (target_type == MONSTER_TO_PLAYER) {
        update_smart_learn(player_ptr, m_idx, DRS_REFLECT);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}
