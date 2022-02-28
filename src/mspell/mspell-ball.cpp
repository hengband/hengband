#include "mspell/mspell-ball.h"
#include "effect/attribute-types.h"
#include "effect/effect-processor.h"
#include "main/sound-of-music.h"
#include "mind/drs-types.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-result.h"
#include "mspell/mspell-util.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief RF4_BA_NUKEの処理。放射能球。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF4_BA_NUKE(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."), _("%^sが放射能球を放った。", "%^s casts a ball of radiation."),
        _("%^sが%sに放射能球を放った。", "%^s casts a ball of radiation at %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BA_NUKE, m_idx, DAM_ROLL);
    const auto proj_res = ball(player_ptr, y, x, m_idx, AttributeType::NUKE, dam, 2, target_type);
    if (target_type == MONSTER_TO_PLAYER) {
        update_smart_learn(player_ptr, m_idx, DRS_POIS);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF4_BA_CHAOの処理。純ログルス。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF4_BA_CHAO(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%^sが恐ろしげにつぶやいた。", "%^s mumbles frighteningly."),
        _("%^sが純ログルスを放った。", "%^s invokes a raw Logrus."), _("%^sが%sに純ログルスを放った。", "%^s invokes raw Logrus upon %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BA_CHAO, m_idx, DAM_ROLL);
    const auto proj_res = ball(player_ptr, y, x, m_idx, AttributeType::CHAOS, dam, 4, target_type);
    if (target_type == MONSTER_TO_PLAYER) {
        update_smart_learn(player_ptr, m_idx, DRS_CHAOS);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_BA_ACIDの処理。アシッド・ボール。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BA_ACID(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがアシッド・ボールの呪文を唱えた。", "%^s casts an acid ball."),
        _("%^sが%sに向かってアシッド・ボールの呪文を唱えた。", "%^s casts an acid ball at %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    const auto rad = monster_is_powerful(player_ptr->current_floor_ptr, m_idx) ? 4 : 2;
    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BA_ACID, m_idx, DAM_ROLL);
    const auto proj_res = ball(player_ptr, y, x, m_idx, AttributeType::ACID, dam, rad, target_type);
    if (target_type == MONSTER_TO_PLAYER) {
        update_smart_learn(player_ptr, m_idx, DRS_ACID);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_BA_ELECの処理。サンダー・ボール。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BA_ELEC(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがサンダー・ボールの呪文を唱えた。", "%^s casts a lightning ball."),
        _("%^sが%sに向かってサンダー・ボールの呪文を唱えた。", "%^s casts a lightning ball at %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    const auto rad = monster_is_powerful(player_ptr->current_floor_ptr, m_idx) ? 4 : 2;
    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BA_ELEC, m_idx, DAM_ROLL);
    const auto proj_res = ball(player_ptr, y, x, m_idx, AttributeType::ELEC, dam, rad, target_type);
    if (target_type == MONSTER_TO_PLAYER) {
        update_smart_learn(player_ptr, m_idx, DRS_ELEC);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_BA_FIREの処理。ファイア・ボール。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BA_FIRE(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg;
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];

    if (m_ptr->r_idx == MON_ROLENTO) {
        msg.blind = _("%sが何かを投げた。", "%^s throws something.");
        msg.to_player = _("%sは手榴弾を投げた。", "%^s throws a hand grenade.");
        msg.to_mons = _("%^sが%^sに向かって手榴弾を投げた。", "%^s throws a hand grenade.");
    } else {
        msg.blind = _("%^sが何かをつぶやいた。", "%^s mumbles.");
        msg.to_player = _("%^sがファイア・ボールの呪文を唱えた。", "%^s casts a fire ball.");
        msg.to_mons = _("%^sが%sに向かってファイア・ボールの呪文を唱えた。", "%^s casts a fire ball at %s.");
    }

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    const auto rad = monster_is_powerful(player_ptr->current_floor_ptr, m_idx) ? 4 : 2;
    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BA_FIRE, m_idx, DAM_ROLL);
    const auto proj_res = ball(player_ptr, y, x, m_idx, AttributeType::FIRE, dam, rad, target_type);
    if (target_type == MONSTER_TO_PLAYER) {
        update_smart_learn(player_ptr, m_idx, DRS_FIRE);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_BA_COLDの処理。アイス・ボール。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BA_COLD(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."),
        _("%^sがアイス・ボールの呪文を唱えた。", "%^s casts a frost ball."),
        _("%^sが%sに向かってアイス・ボールの呪文を唱えた。", "%^s casts a frost ball at %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    const auto rad = monster_is_powerful(player_ptr->current_floor_ptr, m_idx) ? 4 : 2;
    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BA_COLD, m_idx, DAM_ROLL);
    const auto proj_res = ball(player_ptr, y, x, m_idx, AttributeType::COLD, dam, rad, target_type);
    if (target_type == MONSTER_TO_PLAYER) {
        update_smart_learn(player_ptr, m_idx, DRS_COLD);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_BA_POISの処理。悪臭雲。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BA_POIS(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."), _("%^sが悪臭雲の呪文を唱えた。", "%^s casts a stinking cloud."),
        _("%^sが%sに向かって悪臭雲の呪文を唱えた。", "%^s casts a stinking cloud at %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BA_POIS, m_idx, DAM_ROLL);
    const auto proj_res = ball(player_ptr, y, x, m_idx, AttributeType::POIS, dam, 2, target_type);
    if (target_type == MONSTER_TO_PLAYER) {
        update_smart_learn(player_ptr, m_idx, DRS_POIS);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_BA_NETHの処理。地獄球。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BA_NETH(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."), _("%^sが地獄球の呪文を唱えた。", "%^s casts a nether ball."),
        _("%^sが%sに向かって地獄球の呪文を唱えた。", "%^s casts a nether ball at %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BA_NETH, m_idx, DAM_ROLL);
    const auto proj_res = ball(player_ptr, y, x, m_idx, AttributeType::NETHER, dam, 2, target_type);
    if (target_type == MONSTER_TO_PLAYER) {
        update_smart_learn(player_ptr, m_idx, DRS_NETH);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_BA_WATEの処理。ウォーター・ボール。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BA_WATE(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    bool known = monster_near_player(player_ptr->current_floor_ptr, m_idx, t_idx);
    bool see_either = see_monster(player_ptr, m_idx) || see_monster(player_ptr, t_idx);
    bool mon_to_mon = (target_type == MONSTER_TO_MONSTER);
    bool mon_to_player = (target_type == MONSTER_TO_PLAYER);
    GAME_TEXT t_name[MAX_NLEN];
    monster_name(player_ptr, t_idx, t_name);

    mspell_cast_msg_blind msg(_("%^sが何かをつぶやいた。", "%^s mumbles."), _("%^sが流れるような身振りをした。", "%^s gestures fluidly."),
        _("%^sが%sに対して流れるような身振りをした。", "%^s gestures fluidly at %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    if (mon_to_player) {
        msg_format(_("あなたは渦巻きに飲み込まれた。", "You are engulfed in a whirlpool."));
    } else if (mon_to_mon && known && see_either && !player_ptr->blind) {
        msg_format(_("%^sは渦巻に飲み込まれた。", "%^s is engulfed in a whirlpool."), t_name);
    }

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BA_WATE, m_idx, DAM_ROLL);
    const auto proj_res = ball(player_ptr, y, x, m_idx, AttributeType::WATER, dam, 4, target_type);

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_BA_MANAの処理。魔力の嵐。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BA_MANA(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%^sが何かを力強くつぶやいた。", "%^s mumbles powerfully."),
        _("%^sが魔力の嵐の呪文を念じた。", "%^s invokes a mana storm."), _("%^sが%sに対して魔力の嵐の呪文を念じた。", "%^s invokes a mana storm upon %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BA_MANA, m_idx, DAM_ROLL);
    const auto proj_res = ball(player_ptr, y, x, m_idx, AttributeType::MANA, dam, 4, target_type);

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_BA_DARKの処理。暗黒の嵐。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BA_DARK(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%^sが何かを力強くつぶやいた。", "%^s mumbles powerfully."),
        _("%^sが暗黒の嵐の呪文を念じた。", "%^s invokes a darkness storm."),
        _("%^sが%sに対して暗黒の嵐の呪文を念じた。", "%^s invokes a darkness storm upon %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BA_DARK, m_idx, DAM_ROLL);
    const auto proj_res = ball(player_ptr, y, x, m_idx, AttributeType::DARK, dam, 4, target_type);
    if (target_type == MONSTER_TO_PLAYER) {
        update_smart_learn(player_ptr, m_idx, DRS_DARK);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_BA_LITEの処理。スターバースト。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BA_LITE(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%^sが何かを力強くつぶやいた。", "%^s mumbles powerfully."),
        _("%^sがスターバーストの呪文を念じた。", "%^s invokes a starburst."),
        _("%^sが%sに対してスターバーストの呪文を念じた。", "%^s invokes a starburst upon %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BA_LITE, m_idx, DAM_ROLL);
    const auto proj_res = ball(player_ptr, y, x, m_idx, AttributeType::LITE, dam, 4, target_type);
    if (target_type == MONSTER_TO_PLAYER) {
        update_smart_learn(player_ptr, m_idx, DRS_LITE);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_BA_VOIDの処理。虚無の嵐。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BA_VOID(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%^sが何かを力強くつぶやいた。", "%^s mumbles powerfully."),
        _("%^sが虚無の嵐の呪文を念じた。", "%^s invokes a void storm."),
        _("%^sが%sに対して虚無の嵐の呪文を念じた。", "%^s invokes a void storm upon %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BA_VOID, m_idx, DAM_ROLL);
    const auto proj_res = ball(player_ptr, y, x, m_idx, AttributeType::VOID_MAGIC, dam, 4, target_type);

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF5_BA_ABYSSの処理。アビス・ボール。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param target_type プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF5_BA_ABYSS(PlayerType *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int target_type)
{
    mspell_cast_msg_blind msg(_("%^sが何かを力強くつぶやいた。", "%^s mumbles powerfully."),
        _("%^sが深淵の嵐の呪文を念じた。", "%^s invokes a abyss storm."),
        _("%^sが%sに対して深淵の嵐の呪文を念じた。", "%^s invokes a void abyss upon %s."));

    monspell_message(player_ptr, m_idx, t_idx, msg, target_type);

    const auto dam = monspell_damage(player_ptr, MonsterAbilityType::BA_ABYSS, m_idx, DAM_ROLL);
    const auto proj_res = ball(player_ptr, y, x, m_idx, AttributeType::ABYSS, dam, 4, target_type);
    if (target_type == MONSTER_TO_PLAYER) {
        update_smart_learn(player_ptr, m_idx, DRS_DARK);
    }

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}
