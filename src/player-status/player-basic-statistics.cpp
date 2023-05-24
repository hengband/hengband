#include "player-status/player-basic-statistics.h"
#include "core/window-redrawer.h"
#include "mutation/mutation-flag-types.h"
#include "object/object-flags.h"
#include "player-base/player-race.h"
#include "player-info/class-info.h"
#include "player-info/mimic-info-table.h"
#include "player/player-personality.h"
#include "player/player-status.h"
#include "player/race-info-table.h"
#include "player/special-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "spell-realm/spells-hex.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"

PlayerBasicStatistics::PlayerBasicStatistics(PlayerType *player_ptr)
    : PlayerStatusBase(player_ptr)
{
}

/*!
 * @brief 基礎ステータス補正値
 * @return ステータス補正値
 * @details
 * * 各要素によるステータス修正値の合計
 */
int16_t PlayerBasicStatistics::modification_value()
{
    return PlayerStatusBase::get_value();
}

/*!
 * @brief 基礎ステータスの実値
 * @return status_typeに対応するステータスの実値を返す
 */
int16_t PlayerBasicStatistics::get_value()
{
    this->set_locals();
    return this->player_ptr->stat_index[(int)this->ability_type];
}

/*!
 * @brief 基礎ステータス補正計算 - 種族
 * @param 計算するステータスの種類
 * @return ステータス補正値
 * @details
 * * 種族によるステータス修正値。
 */
int16_t PlayerBasicStatistics::race_bonus()
{
    return PlayerRace(this->player_ptr).get_info()->r_adj[this->ability_type];
}

/*!
 * @brief ステータス補正計算 - 職業
 * @param 計算するステータスの種類
 * @return ステータス補正値
 * @details
 * * 職業によるステータス修正値
 */
int16_t PlayerBasicStatistics::class_bonus()
{
    const player_class_info *c_ptr = &class_info[enum2i(this->player_ptr->pclass)];
    return c_ptr->c_adj[this->ability_type];
}

/*!
 * @brief ステータス補正計算 - 性格
 * @param 計算するステータスの種類
 * @return ステータス補正値
 * @details
 * * 性格によるステータス修正値
 */
int16_t PlayerBasicStatistics::personality_bonus()
{
    const player_personality *a_ptr = &personality_info[this->player_ptr->ppersonality];
    return a_ptr->a_adj[this->ability_type];
}

/*!
 * @brief ステータス更新処理
 * @details
 * * player_ptrのステータスを更新する
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
 * * player_ptrのステータス最大値を更新する
 * * 更新対象はset_locals()で設定したstatus_typeで決定される
 */
void PlayerBasicStatistics::update_top_status()
{
    int status = (int)this->ability_type;
    int top = modify_stat_value(this->player_ptr->stat_max[status], this->player_ptr->stat_add[status]);

    if (this->player_ptr->stat_top[status] != top) {
        this->player_ptr->stat_top[status] = (int16_t)top;
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        rfu.set_flag(MainWindowRedrawingFlag::ABILITY_SCORE);
        set_bits(this->player_ptr->window_flags, PW_PLAYER);
    }
}

/*!
 * @brief ステータス現在値更新の例外処理
 * @param 通常処理されたステータスの値
 * @returns 例外処理されたステータスの値
 * @details
 * * player_ptrのステータス現在値を更新する際の例外処理
 * * 派生クラスでoverrideして使用する。
 */
int16_t PlayerBasicStatistics::set_exception_use_status(int16_t value)
{
    return value;
}

/*!
 * @brief ステータス現在値更新処理
 * @details
 * * player_ptrのステータス現在値を更新する
 * * 更新対象はset_locals()で設定したstatus_typeで決定される
 */
void PlayerBasicStatistics::update_use_status()
{
    int status = (int)this->ability_type;
    int16_t use = modify_stat_value(this->player_ptr->stat_cur[status], this->player_ptr->stat_add[status]);

    use = this->set_exception_use_status(use);

    if (this->player_ptr->stat_use[status] != use) {
        this->player_ptr->stat_use[status] = (int16_t)use;
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        rfu.set_flag(MainWindowRedrawingFlag::ABILITY_SCORE);
        set_bits(this->player_ptr->window_flags, PW_PLAYER);
    }
}

/*!
 * @brief ステータス内部値更新処理
 * @details
 * * player_ptrのステータス内部値を更新する
 * * ステータス内部値は実際の数値処理に使われる0-37の整数値
 * * 更新対象はset_locals()で設定したstatus_typeで決定される
 */
void PlayerBasicStatistics::update_index_status()
{
    int status = (int)this->ability_type;
    int index;
    if (this->player_ptr->stat_use[status] <= 18) {
        index = (this->player_ptr->stat_use[status] - 3);
    } else if (this->player_ptr->stat_use[status] <= 18 + 219) {
        index = (15 + (this->player_ptr->stat_use[status] - 18) / 10);
    } else {
        index = (37);
    }

    if (this->player_ptr->stat_index[status] == index) {
        return;
    }

    this->player_ptr->stat_index[status] = (int16_t)index;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    const auto flags = {
        StatusRedrawingFlag::MP,
        StatusRedrawingFlag::SPELLS,
    };
    if (status == A_CON) {
        rfu.set_flag(StatusRedrawingFlag::HP);
    } else if (status == A_INT) {
        if (mp_ptr->spell_stat == A_INT) {
            rfu.set_flags(flags);
        }
    } else if (status == A_WIS) {
        if (mp_ptr->spell_stat == A_WIS) {
            rfu.set_flags(flags);
        }
    } else if (status == A_CHR) {
        if (mp_ptr->spell_stat == A_CHR) {
            rfu.set_flags(flags);
        }
    }

    set_bits(this->player_ptr->window_flags, PW_PLAYER);
}
