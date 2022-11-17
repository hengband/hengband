#include "player-status/player-stealth.h"
#include "mind/mind-ninja.h"
#include "mutation/mutation-flag-types.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/class-info.h"
#include "player-info/equipment-info.h"
#include "player-info/mimic-info-table.h"
#include "player-info/race-types.h"
#include "player/player-personality.h"
#include "player/player-skill.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "player/race-info-table.h"
#include "spell-realm/spells-hex.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"

PlayerStealth::PlayerStealth(PlayerType *player_ptr)
    : PlayerStatusBase(player_ptr)
{
}

/*!
 * @brief 隠密能力計算 - 種族
 * @return 隠密能力の増分
 * @details
 * * 種族による加算
 */
int16_t PlayerStealth::race_bonus()
{
    return PlayerRace(this->player_ptr).get_info()->r_stl;
}

/*!
 * @brief 隠密能力計算 - 性格
 * @return 隠密能力の増分
 * @details
 * * 性格による加算
 */
int16_t PlayerStealth::personality_bonus()
{
    const player_personality *a_ptr = &personality_info[this->player_ptr->ppersonality];
    return a_ptr->a_stl;
}

/*!
 * @brief 隠密能力計算 - 職業(基礎値)
 * @return 隠密能力の増分
 * @details
 * * 職業による加算
 */
int16_t PlayerStealth::class_base_bonus()
{
    const player_class_info *c_ptr = &class_info[enum2i(this->player_ptr->pclass)];
    return c_ptr->c_stl + (c_ptr->x_stl * this->player_ptr->lev / 10);
}

/*!
 * @brief 隠密能力計算 - 職業(追加分)
 * @return 隠密能力の増分
 * @details
 * * 忍者がheavy_armorならば減算(-レベル/10)
 * * 忍者がheavy_armorでなく適正な武器を持っていれば加算(+レベル/10)
 */
int16_t PlayerStealth::class_bonus()
{
    if (!PlayerClass(this->player_ptr).equals(PlayerClassType::NINJA)) {
        return 0;
    }

    int16_t bonus = 0;
    if (heavy_armor(this->player_ptr)) {
        bonus -= (this->player_ptr->lev) / 10;
    } else if ((!this->player_ptr->inventory_list[INVEN_MAIN_HAND].k_idx || can_attack_with_main_hand(this->player_ptr)) && (!this->player_ptr->inventory_list[INVEN_SUB_HAND].k_idx || can_attack_with_sub_hand(this->player_ptr))) {
        bonus += (this->player_ptr->lev) / 10;
    }

    return bonus;
}

/*!
 * @brief 隠密能力計算 - 変異
 * @return 隠密能力の増分
 * @details
 * * 変異MUT3_XTRA_NOISで減算(-3)
 * * 変異MUT3_MOTIONで加算(+1)
 */
int16_t PlayerStealth::mutation_bonus()
{
    int16_t bonus = 0;
    const auto &muta = this->player_ptr->muta;
    if (muta.has(PlayerMutationType::XTRA_NOIS)) {
        bonus -= 3;
    }

    if (muta.has(PlayerMutationType::MOTION)) {
        bonus += 1;
    }

    return bonus;
}

/*!
 * @brief 隠密能力計算 - 一時効果
 * @return 隠密能力の増分
 * @details
 * * 呪術を唱えていると減算(-(詠唱数+1))
 * * 狂戦士化で減算(-7)
 * * 隠密の歌で加算(+999)
 */
int16_t PlayerStealth::time_effect_bonus()
{
    int16_t bonus = 0;
    if (this->player_ptr->realm1 == REALM_HEX) {
        SpellHex spell_hex(this->player_ptr);
        if (spell_hex.is_spelling_any()) {
            bonus -= spell_hex.get_casting_num() + 1;
        }
    }

    if (is_shero(this->player_ptr)) {
        bonus -= 7;
    }

    if (is_time_limit_stealth(this->player_ptr)) {
        bonus += 999;
    }

    return bonus;
}

bool PlayerStealth::is_aggravated_s_fairy()
{
    return player_aggravate_state(this->player_ptr) == AGGRAVATE_S_FAIRY;
}

/*!
 * @brief 隠密能力計算 - 影フェアリー反感時の例外処理
 * @return 修正後の隠密能力
 * @details
 * * セクシーギャルでない影フェアリーがTRC_AGGRAVATE持ちの時、別処理でTRC_AGGRAVATEを無効にする代わりに減算(-3か3未満なら(現在値+2)/2)
 */
int16_t PlayerStealth::set_exception_bonus(int16_t value)
{
    if (this->is_aggravated_s_fairy()) {
        value = std::min<int16_t>(value - 3, (value + 2) / 2);
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
    auto flags = PlayerStatusBase::get_bad_flags();
    if (this->is_aggravated_s_fairy()) {
        set_bits(flags, FLAG_CAUSE_RACE);
    }

    return flags;
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
