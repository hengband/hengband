#include "player-status/player-stealth.h"
#include "mind/mind-ninja.h"
#include "mutation/mutation-flag-types.h"
#include "player-info/equipment-info.h"
#include "player/mimic-info-table.h"
#include "player/player-class.h"
#include "player/player-personality.h"
#include "player/player-race-types.h"
#include "player/player-skill.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "player/race-info-table.h"
#include "spell-realm/spells-hex.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"

/*!
 * @brief 隠密能力計算 - 種族
 * @return 隠密能力の増分
 * @details
 * * 種族による加算
 */
int16_t PlayerStealth::race_value()
{
    const player_race *tmp_rp_ptr;

    if (this->owner_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[this->owner_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[enum2i(this->owner_ptr->prace)];

    return tmp_rp_ptr->r_stl;
}

/*!
 * @brief 隠密能力計算 - 性格
 * @return 隠密能力の増分
 * @details
 * * 性格による加算
 */
int16_t PlayerStealth::personality_value()
{
    const player_personality *a_ptr = &personality_info[this->owner_ptr->pseikaku];
    return a_ptr->a_stl;
}

/*!
 * @brief 隠密能力計算 - 職業(基礎値)
 * @return 隠密能力の増分
 * @details
 * * 職業による加算
 */
int16_t PlayerStealth::class_base_value()
{
    const player_class *c_ptr = &class_info[this->owner_ptr->pclass];
    return c_ptr->c_stl + (c_ptr->x_stl * this->owner_ptr->lev / 10);
}

/*!
 * @brief 隠密能力計算 - 職業(追加分)
 * @return 隠密能力の増分
 * @details
 * * 忍者がheavy_armorならば減算(-レベル/10)
 * * 忍者がheavy_armorでなく適正な武器を持っていれば加算(+レベル/10)
 */
int16_t PlayerStealth::class_value()
{
    ACTION_SKILL_POWER result = 0;

    if (this->owner_ptr->pclass == CLASS_NINJA) {
        if (heavy_armor(this->owner_ptr)) {
            result -= (this->owner_ptr->lev) / 10;
        } else if ((!this->owner_ptr->inventory_list[INVEN_MAIN_HAND].k_idx || can_attack_with_main_hand(this->owner_ptr))
            && (!this->owner_ptr->inventory_list[INVEN_SUB_HAND].k_idx || can_attack_with_sub_hand(this->owner_ptr))) {
            result += (this->owner_ptr->lev) / 10;
        }
    }

    return result;
}

/*!
 * @brief 隠密能力計算 - 変異
 * @return 隠密能力の増分
 * @details
 * * 変異MUT3_XTRA_NOISで減算(-3)
 * * 変異MUT3_MOTIONで加算(+1)
 */
int16_t PlayerStealth::mutation_value()
{
    int16_t result = 0;
    const auto &muta = this->owner_ptr->muta;
    if (muta.has(MUTA::XTRA_NOIS)) {
        result -= 3;
    }
    if (muta.has(MUTA::MOTION)) {
        result += 1;
    }
    return result;
}

/*!
 * @brief 隠密能力計算 - 一時効果
 * @return 隠密能力の増分
 * @details
 * * 呪術を唱えていると減算(-(詠唱数+1))
 * * 狂戦士化で減算(-7)
 * * 隠密の歌で加算(+999)
 */
int16_t PlayerStealth::time_effect_value()
{
    int16_t result = 0;
    if (this->owner_ptr->realm1 == REALM_HEX) {
        if (RealmHex(this->owner_ptr).is_spelling_any())
            result -= (1 + casting_hex_num(this->owner_ptr));
    }
    if (is_shero(this->owner_ptr)) {
        result -= 7;
    }
    if (is_time_limit_stealth(this->owner_ptr))
        result += 999;

    return result;
}

bool PlayerStealth::is_aggravated_s_fairy()
{
    return player_aggravate_state(this->owner_ptr) == AGGRAVATE_S_FAIRY;
}

/*!
 * @brief 隠密能力計算 - 影フェアリー反感時の例外処理
 * @return 修正後の隠密能力
 * @details
 * * セクシーギャルでない影フェアリーがTRC_AGGRAVATE持ちの時、別処理でTRC_AGGRAVATEを無効にする代わりに減算(-3か3未満なら(現在値+2)/2)
 */
int16_t PlayerStealth::set_exception_value(int16_t value)
{
    if (this->is_aggravated_s_fairy()) {
        value = MIN(value - 3, (value + 2) / 2);
    }
    return value;
}

/*!
 * @brief 隠密マイナスフラグ判定
 * @return マイナスフラグの集合体
 * @details
 * * TR_STELATHがマイナスの要素に加え、種族影フェアリーかつ反感のとき種族にマイナスフラグを与える
 */
BIT_FLAGS PlayerStealth::get_bad_flags()
{
    BIT_FLAGS result = PlayerStatusBase::get_bad_flags();

    if (this->is_aggravated_s_fairy())
        set_bits(result, FLAG_CAUSE_RACE);

    return result;
}

/*!
 * @brief 隠密値の上限と下限の設定
 * @details
 * * 初期値1
 * * 最大30、最低0に補正
 */
void PlayerStealth::set_locals()
{
    this->default_value = 1;
    this->min_value = 0;
    this->max_value = 30;
    this->tr_flag = TR_STEALTH;
    this->tr_bad_flag = TR_STEALTH;
}
