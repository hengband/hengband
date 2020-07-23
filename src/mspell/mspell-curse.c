#include "mspell/mspell-curse.h"
#include "core/disturbance.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "mspell/mspell-checker.h"
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-type.h"
#include "mspell/mspell-util.h"
#include "spell/spell-types.h"
#include "system/floor-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief RF5_CAUSE_*のメッセージ処理関数 /
 * @param target_ptr プレーヤーへの参照ポインタ
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
 * @return ダメージ量を返す。
 */
void spell_RF5_CAUSE(player_type *target_ptr, int GF_TYPE, HIT_POINT dam, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, concptr msg1,
    concptr msg2, concptr msg3, int MS_TYPE, int TARGET_TYPE)
{
    GAME_TEXT m_name[MAX_NLEN], t_name[MAX_NLEN];
    monster_name(target_ptr, m_idx, m_name);
    monster_name(target_ptr, t_idx, t_name);

    if (TARGET_TYPE == MONSTER_TO_PLAYER) {
        disturb(target_ptr, TRUE, TRUE);
        if (target_ptr->blind)
            msg_format(msg1, m_name);
        else
            msg_format(msg2, m_name);
        breath(target_ptr, y, x, m_idx, GF_TYPE, dam, 0, FALSE, MS_TYPE, TARGET_TYPE);
        return;
    }

    if (TARGET_TYPE == MONSTER_TO_MONSTER) {
        if (see_monster(target_ptr, m_idx)) {
            msg_format(msg3, m_name, t_name);
        } else {
            target_ptr->current_floor_ptr->monster_noise = TRUE;
        }
    }

    breath(target_ptr, y, x, m_idx, GF_TYPE, dam, 0, FALSE, MS_TYPE, TARGET_TYPE);
}

/*!
 * @brief RF5_CAUSE_1の処理。軽傷の呪い。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_CAUSE_1(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    concptr msg1, msg2, msg3;
    HIT_POINT dam;
    dam = monspell_damage(target_ptr, (MS_CAUSE_1), m_idx, DAM_ROLL);

    msg1 = _("%^sが何かをつぶやいた。", "%^s mumbles.");
    msg2 = _("%^sがあなたを指さして呪った。", "%^s points at you and curses.");
    msg3 = _("%^sは%sを指さして呪いをかけた。", "%^s points at %s and curses.");

    spell_RF5_CAUSE(target_ptr, GF_CAUSE_1, dam, y, x, m_idx, t_idx, msg1, msg2, msg3, MS_CAUSE_1, TARGET_TYPE);
    return dam;
}

/*!
 * @brief RF5_CAUSE_2の処理。重傷の呪い。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_CAUSE_2(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    concptr msg1, msg2, msg3;
    HIT_POINT dam;
    dam = monspell_damage(target_ptr, (MS_CAUSE_2), m_idx, DAM_ROLL);

    msg1 = _("%^sが何かをつぶやいた。", "%^s mumbles.");
    msg2 = _("%^sがあなたを指さして恐ろしげに呪った。", "%^s points at you and curses horribly.");
    msg3 = _("%^sは%sを指さして恐ろしげに呪いをかけた。", "%^s points at %s and curses horribly.");

    spell_RF5_CAUSE(target_ptr, GF_CAUSE_2, dam, y, x, m_idx, t_idx, msg1, msg2, msg3, MS_CAUSE_2, TARGET_TYPE);
    return dam;
}

/*!
 * @brief RF5_CAUSE_3の処理。致命傷の呪い。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_CAUSE_3(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    concptr msg1, msg2, msg3;
    HIT_POINT dam;
    dam = monspell_damage(target_ptr, (MS_CAUSE_3), m_idx, DAM_ROLL);

    msg1 = _("%^sが何かを大声で叫んだ。", "%^s mumbles loudly.");
    msg2 = _("%^sがあなたを指さして恐ろしげに呪文を唱えた！", "%^s points at you, incanting terribly!");
    msg3 = _("%^sは%sを指さし、恐ろしげに呪文を唱えた！", "%^s points at %s, incanting terribly!");

    spell_RF5_CAUSE(target_ptr, GF_CAUSE_3, dam, y, x, m_idx, t_idx, msg1, msg2, msg3, MS_CAUSE_3, TARGET_TYPE);
    return dam;
}

/*!
 * @brief RF5_CAUSE_4の処理。秘孔を突く。 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 対象の地点のy座標
 * @param x 対象の地点のx座標
 * @param m_idx 呪文を唱えるモンスターID
 * @param t_idx 呪文を受けるモンスターID。プレイヤーの場合はdummyで0とする。
 * @param TARGET_TYPE プレイヤーを対象とする場合MONSTER_TO_PLAYER、モンスターを対象とする場合MONSTER_TO_MONSTER
 * @return ダメージ量を返す。
 */
HIT_POINT spell_RF5_CAUSE_4(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, MONSTER_IDX t_idx, int TARGET_TYPE)
{
    concptr msg1, msg2, msg3;
    HIT_POINT dam;
    dam = monspell_damage(target_ptr, (MS_CAUSE_4), m_idx, DAM_ROLL);

    msg1 = _("%^sが「お前は既に死んでいる」と叫んだ。", "%^s screams the word 'DIE!'");
    msg2 = _("%^sがあなたの秘孔を突いて「お前は既に死んでいる」と叫んだ。", "%^s points at you, screaming the word DIE!");
    msg3 = _("%^sが%sの秘孔を突いて、「お前は既に死んでいる」と叫んだ。", "%^s points at %s, screaming the word, 'DIE!'");

    spell_RF5_CAUSE(target_ptr, GF_CAUSE_4, dam, y, x, m_idx, t_idx, msg1, msg2, msg3, MS_CAUSE_4, TARGET_TYPE);
    return dam;
}
