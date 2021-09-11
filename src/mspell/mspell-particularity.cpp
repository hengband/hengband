/*!
 * @brief ボルトでもボールでもブレスでもなく、ダメージを与える特殊なスペルの実行 /
 * Performing special spells that take damage, not bolts, balls or breaths
 * @date 2020/05/16
 * @author Hourier
 * @details 肥大化しやすいファイル名なので、関数の追加時は共通部分を別ファイルに抜き出せるか検討すること /
 * This is a filename that tends to be bloated.
 * So when adding a function, please consider whether you can extract the common part to another file.
 */

#include "mspell/mspell-particularity.h"
#include "effect/effect-processor.h"
#include "mind/drs-types.h"
#include "monster-race/race-ability-flags.h"
#include "monster/monster-update.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-util.h"
#include "mspell/mspell.h"
#include "spell/spell-types.h"
#include "system/player-type-definition.h"

/*!
 * @brief RF4_ROCKETの処理。ロケット。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF4_ROCKET(player_type *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    monspell_message(player_ptr, m_idx, t_idx, _("%^sが何かを射った。", "%^s shoots something."), _("%^sがロケットを発射した。", "%^s fires a rocket."),
        _("%^sが%sにロケットを発射した。", "%^s fires a rocket at %s."), TARGET_TYPE);

    const auto dam = monspell_damage(player_ptr, RF_ABILITY::ROCKET, m_idx, DAM_ROLL);
    const auto proj_res = breath(player_ptr, y, x, m_idx, GF_ROCKET, dam, 2, false, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER)
        update_smart_learn(player_ptr, m_idx, DRS_SHARD);

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF6_HAND_DOOMの処理。破滅の手。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF6_HAND_DOOM(player_type *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    simple_monspell_message(player_ptr, m_idx, t_idx, _("%^sが<破滅の手>を放った！", "%^s invokes the Hand of Doom!"),
        _("%^sが%sに<破滅の手>を放った！", "%^s invokes the Hand of Doom upon %s!"), TARGET_TYPE);

    ProjectResult proj_res;
    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        const auto dam = monspell_damage(player_ptr, RF_ABILITY::HAND_DOOM, m_idx, DAM_ROLL);
        proj_res = breath(player_ptr, y, x, m_idx, GF_HAND_DOOM, dam, 0, false, MONSTER_TO_PLAYER);
    } else if (TARGET_TYPE == MONSTER_TO_MONSTER) {
        const auto dam = 20; /* Dummy power */
        proj_res = breath(player_ptr, y, x, m_idx, GF_HAND_DOOM, dam, 0, false, MONSTER_TO_MONSTER);
    }

    auto res = MonsterSpellResult::make_valid();
    res.learnable = proj_res.affected_player;

    return res;
}

/*!
 * @brief RF6_PSY_SPEARの処理。光の剣。 /
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーに当たったらラーニング可。
 */
MonsterSpellResult spell_RF6_PSY_SPEAR(player_type *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    monspell_message(player_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."), _("%^sが光の剣を放った。", "%^s throw a Psycho-Spear."),
        _("%^sが%sに向かって光の剣を放った。", "%^s throw a Psycho-spear at %s."), TARGET_TYPE);

    const auto dam = monspell_damage(player_ptr, RF_ABILITY::PSY_SPEAR, m_idx, DAM_ROLL);
    const auto proj_res = beam(player_ptr, m_idx, y, x, GF_PSY_SPEAR, dam, MONSTER_TO_PLAYER);

    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = proj_res.affected_player;

    return res;
}
