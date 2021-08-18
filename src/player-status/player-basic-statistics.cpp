#include "player-status/player-basic-statistics.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "mutation/mutation-flag-types.h"
#include "object/object-flags.h"
#include "player/mimic-info-table.h"
#include "player/player-class.h"
#include "player/player-personality.h"
#include "player/player-status.h"
#include "player/race-info-table.h"
#include "player/special-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "spell-realm/spells-hex.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief 基礎ステータス補正値
 * @return ステータス補正値
 * @details
 * * 各要素によるステータス修正値の合計
 */
s16b PlayerBasicStatistics::modification_value()
{
    return PlayerStatusBase::get_value();
}

/*!
 * @brief 基礎ステータスの実値
 * @return status_typeに対応するステータスの実値を返す
 */
s16b PlayerBasicStatistics::get_value()
{
    this->set_locals();
    return this->owner_ptr->stat_index[(int)this->ability_type];
}

/*!
 * @brief 基礎ステータス補正計算 - 種族
 * @param 計算するステータスの種類
 * @return ステータス補正値
 * @details
 * * 種族によるステータス修正値。
 */
s16b PlayerBasicStatistics::race_value()
{
    const player_race *tmp_rp_ptr;
    if (this->owner_ptr->mimic_form)
        tmp_rp_ptr = &mimic_info[this->owner_ptr->mimic_form];
    else
        tmp_rp_ptr = &race_info[static_cast<int>(this->owner_ptr->prace)];

    return tmp_rp_ptr->r_adj[this->ability_type];
}

/*!
 * @brief ステータス補正計算 - 職業
 * @param 計算するステータスの種類
 * @return ステータス補正値
 * @details
 * * 職業によるステータス修正値
 */
s16b PlayerBasicStatistics::class_value()
{
    const player_class *c_ptr = &class_info[this->owner_ptr->pclass];
    return c_ptr->c_adj[this->ability_type];
}

/*!
 * @brief ステータス補正計算 - 性格
 * @param 計算するステータスの種類
 * @return ステータス補正値
 * @details
 * * 性格によるステータス修正値
 */
s16b PlayerBasicStatistics::personality_value()
{
    const player_personality *a_ptr = &personality_info[this->owner_ptr->pseikaku];
    return a_ptr->a_adj[this->ability_type];
}

/*!
 * @brief ステータス更新処理
 * @details
 * * owner_ptrのステータスを更新する
 */
void PlayerBasicStatistics::update_value()
{
    this->set_locals();
    this->update_top_status();
    this->update_use_status();
    this->update_index_status();
}

/*!
 * @brief ステータス最大値更新処理
 * @details
 * * owner_ptrのステータス最大値を更新する
 * * 更新対象はset_locals()で設定したstatus_typeで決定される
 */
void PlayerBasicStatistics::update_top_status()
{
    int status = (int)this->ability_type;
    int top = modify_stat_value(this->owner_ptr->stat_max[status], this->owner_ptr->stat_add[status]);

    if (this->owner_ptr->stat_top[status] != top) {
        this->owner_ptr->stat_top[status] = (s16b)top;
        set_bits(this->owner_ptr->redraw, PR_STATS);
        set_bits(this->owner_ptr->window_flags, PW_PLAYER);
    }
}

/*!
 * @brief ステータス現在値更新の例外処理
 * @param 通常処理されたステータスの値
 * @returns 例外処理されたステータスの値
 * @details
 * * owner_ptrのステータス現在値を更新する際の例外処理
 * * 派生クラスでoverrideして使用する。
 */
s16b PlayerBasicStatistics::set_exception_use_status(s16b value)
{
    return value;
}

/*!
 * @brief ステータス現在値更新処理
 * @details
 * * owner_ptrのステータス現在値を更新する
 * * 更新対象はset_locals()で設定したstatus_typeで決定される
 */
void PlayerBasicStatistics::update_use_status()
{
    int status = (int)this->ability_type;
    s16b use = modify_stat_value(this->owner_ptr->stat_cur[status], this->owner_ptr->stat_add[status]);

    use = this->set_exception_use_status(use);

    if (this->owner_ptr->stat_use[status] != use) {
        this->owner_ptr->stat_use[status] = (s16b)use;
        set_bits(this->owner_ptr->redraw, PR_STATS);
        set_bits(this->owner_ptr->window_flags, PW_PLAYER);
    }
}

/*!
 * @brief ステータス内部値更新処理
 * @details
 * * owner_ptrのステータス内部値を更新する
 * * ステータス内部値は実際の数値処理に使われる0-37の整数値
 * * 更新対象はset_locals()で設定したstatus_typeで決定される
 */
void PlayerBasicStatistics::update_index_status()
{
    int status = (int)this->ability_type;
    int index;
    if (this->owner_ptr->stat_use[status] <= 18)
        index = (this->owner_ptr->stat_use[status] - 3);
    else if (this->owner_ptr->stat_use[status] <= 18 + 219)
        index = (15 + (this->owner_ptr->stat_use[status] - 18) / 10);
    else
        index = (37);

    if (this->owner_ptr->stat_index[status] == index)
        return;

    this->owner_ptr->stat_index[status] = (s16b)index;
    if (status == A_CON) {
        set_bits(this->owner_ptr->update, PU_HP);
    } else if (status == A_INT) {
        if (mp_ptr->spell_stat == A_INT) {
            set_bits(this->owner_ptr->update, (PU_MANA | PU_SPELLS));
        }
    } else if (status == A_WIS) {
        if (mp_ptr->spell_stat == A_WIS) {
            set_bits(this->owner_ptr->update, (PU_MANA | PU_SPELLS));
        }
    } else if (status == A_CHR) {
        if (mp_ptr->spell_stat == A_CHR) {
            set_bits(this->owner_ptr->update, (PU_MANA | PU_SPELLS));
        }
    }

    set_bits(this->owner_ptr->window_flags, PW_PLAYER);
}
