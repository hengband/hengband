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
#include "mind/drs-types.h"
#include "monster/monster-update.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-type.h"
#include "mspell/mspell-util.h"
#include "spell/spell-types.h"

/*!
 * @brief RF4_ROCKETの処理。ロケット。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF4_ROCKET(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    HIT_POINT dam;

    monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かを射った。", "%^s shoots something."), _("%^sがロケットを発射した。", "%^s fires a rocket."),
        _("%^sが%sにロケットを発射した。", "%^s fires a rocket at %s."), TARGET_TYPE);

    dam = monspell_damage(target_ptr, (MS_ROCKET), m_idx, DAM_ROLL);
    breath(target_ptr, y, x, m_idx, GF_ROCKET, dam, 2, FALSE, MS_ROCKET, TARGET_TYPE);
    if (TARGET_TYPE == MONSTER_TO_PLAYER)
        update_smart_learn(target_ptr, m_idx, DRS_SHARD);
    return dam;
}

/*!
 * @brief RF6_HAND_DOOMの処理。破滅の手。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF6_HAND_DOOM(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    simple_monspell_message(target_ptr, m_idx, t_idx, _("%^sが<破滅の手>を放った！", "%^s invokes the Hand of Doom!"),
        _("%^sが%sに<破滅の手>を放った！", "%^s invokes the Hand of Doom upon %s!"), TARGET_TYPE);

    HIT_POINT dam = 0;
    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        dam = monspell_damage(target_ptr, (MS_HAND_DOOM), m_idx, DAM_ROLL);
        breath(target_ptr, y, x, m_idx, GF_HAND_DOOM, dam, 0, FALSE, MS_HAND_DOOM, MONSTER_TO_PLAYER);
    } else if (TARGET_TYPE == MONSTER_TO_MONSTER) {
        dam = 20; /* Dummy power */
        breath(target_ptr, y, x, m_idx, GF_HAND_DOOM, dam, 0, FALSE, MS_HAND_DOOM, MONSTER_TO_MONSTER);
    }

    return dam;
}

/*!
 * @brief RF6_PSY_SPEARの処理。光の剣。 /
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF6_PSY_SPEAR(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    monspell_message(target_ptr, m_idx, t_idx, _("%^sが何かをつぶやいた。", "%^s mumbles."), _("%^sが光の剣を放った。", "%^s throw a Psycho-Spear."),
        _("%^sが%sに向かって光の剣を放った。", "%^s throw a Psycho-spear at %s."), TARGET_TYPE);

    HIT_POINT dam = monspell_damage(target_ptr, (MS_PSY_SPEAR), m_idx, DAM_ROLL);
    beam(target_ptr, m_idx, y, x, GF_PSY_SPEAR, dam, MS_PSY_SPEAR, MONSTER_TO_PLAYER);
    return dam;
}
