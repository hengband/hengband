#include "mspell/mspell-curse.h"
#include "core/disturbance.h"
#include "effect/effect-processor.h"
#include "monster-race/race-ability-flags.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-util.h"
#include "mspell/mspell.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief RF5_CAUSE_* の処理関数
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param GF_TYPE 攻撃に使用する属性
 * @param dam 攻撃に使用するダメージ量
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param msg1 対プレイヤー、盲目時メッセージ
 * @param msg2 対プレイヤー、非盲目時メッセージ
 * @param msg3 対モンスターのメッセージ
 * @param MS_TYPE 呪文の番号
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 */
static MonsterSpellResult spell_RF5_CAUSE(player_type *player_ptr, int GF_TYPE, HIT_POINT dam, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx,
    concptr msg1, concptr msg2, concptr msg3, int TARGET_TYPE)
{
    auto res = MonsterSpellResult::make_valid(dam);
    res.learnable = TARGET_TYPE == MONSTER_TO_PLAYER;

    GAME_TEXT m_name[MAX_NLEN], t_name[MAX_NLEN];
    monster_name(player_ptr, m_idx, m_name);
    monster_name(player_ptr, t_idx, t_name);

    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        disturb(player_ptr, true, true);
        if (player_ptr->blind)
            msg_format(msg1, m_name);
        else
            msg_format(msg2, m_name);
        breath(player_ptr, y, x, m_idx, GF_TYPE, dam, 0, false, TARGET_TYPE);
        return res;
    }

    if (TARGET_TYPE == MONSTER_TO_MONSTER) {
        if (see_monster(player_ptr, m_idx)) {
            msg_format(msg3, m_name, t_name);
        } else {
            player_ptr->current_floor_ptr->monster_noise = true;
        }
    }

    breath(player_ptr, y, x, m_idx, GF_TYPE, dam, 0, false, TARGET_TYPE);

    return res;
}

/*!
 * @brief RF5_CAUSE_1の処理。軽傷の呪い。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF5_CAUSE_1(player_type *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    concptr msg1 = _("%^sが何かをつぶやいた。", "%^s mumbles.");
    concptr msg2 = _("%^sがあなたを指さして呪った。", "%^s points at you and curses.");
    concptr msg3 = _("%^sは%sを指さして呪いをかけた。", "%^s points at %s and curses.");

    const auto dam = monspell_damage(player_ptr, RF_ABILITY::CAUSE_1, m_idx, DAM_ROLL);

    return spell_RF5_CAUSE(player_ptr, GF_CAUSE_1, dam, y, x, m_idx, t_idx, msg1, msg2, msg3, TARGET_TYPE);
}

/*!
 * @brief RF5_CAUSE_2の処理。重傷の呪い。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF5_CAUSE_2(player_type *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    concptr msg1 = _("%^sが何かをつぶやいた。", "%^s mumbles.");
    concptr msg2 = _("%^sがあなたを指さして恐ろしげに呪った。", "%^s points at you and curses horribly.");
    concptr msg3 = _("%^sは%sを指さして恐ろしげに呪いをかけた。", "%^s points at %s and curses horribly.");

    const auto dam = monspell_damage(player_ptr, RF_ABILITY::CAUSE_2, m_idx, DAM_ROLL);

    return spell_RF5_CAUSE(player_ptr, GF_CAUSE_2, dam, y, x, m_idx, t_idx, msg1, msg2, msg3, TARGET_TYPE);
}

/*!
 * @brief RF5_CAUSE_3の処理。致命傷の呪い。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF5_CAUSE_3(player_type *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    concptr msg1 = _("%^sが何かを大声で叫んだ。", "%^s mumbles loudly.");
    concptr msg2 = _("%^sがあなたを指さして恐ろしげに呪文を唱えた！", "%^s points at you, incanting terribly!");
    concptr msg3 = _("%^sは%sを指さし、恐ろしげに呪文を唱えた！", "%^s points at %s, incanting terribly!");

    const auto dam = monspell_damage(player_ptr, RF_ABILITY::CAUSE_3, m_idx, DAM_ROLL);

    return spell_RF5_CAUSE(player_ptr, GF_CAUSE_3, dam, y, x, m_idx, t_idx, msg1, msg2, msg3, TARGET_TYPE);
}

/*!
 * @brief RF5_CAUSE_4の処理。秘孔を突く。 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 *
 * プレイヤーが対象ならラーニング可。
 */
MonsterSpellResult spell_RF5_CAUSE_4(player_type *player_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    concptr msg1 = _("%^sが「お前は既に死んでいる」と叫んだ。", "%^s screams the word 'DIE!'");
    concptr msg2 = _("%^sがあなたの秘孔を突いて「お前は既に死んでいる」と叫んだ。", "%^s points at you, screaming the word DIE!");
    concptr msg3 = _("%^sが%sの秘孔を突いて、「お前は既に死んでいる」と叫んだ。", "%^s points at %s, screaming the word, 'DIE!'");

    const auto dam = monspell_damage(player_ptr, RF_ABILITY::CAUSE_4, m_idx, DAM_ROLL);

    return spell_RF5_CAUSE(player_ptr, GF_CAUSE_4, dam, y, x, m_idx, t_idx, msg1, msg2, msg3, TARGET_TYPE);
}
