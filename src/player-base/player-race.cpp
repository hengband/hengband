/*!
 * @brief プレーヤーの種族に基づく耐性・能力の判定処理等を行うクラス
 * @date 2021/09/08
 * @author Hourier
 * @details 本クラス作成時点で責務に対する余裕はかなりあるので、適宜ここへ移してくること.
 */
#include "player-base/player-race.h"
#include "player-info/mimic-info-table.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

PlayerRace::PlayerRace(player_type *player_ptr)
    : player_ptr(player_ptr)
{
}

bool PlayerRace::is_mimic_nonliving() const
{
    constexpr int nonliving_flag = 1;
    return any_bits(mimic_info[this->player_ptr->mimic_form].choice, nonliving_flag);
}

bool PlayerRace::equals(player_race_type prace) const
{
    return (this->player_ptr->mimic_form == MIMIC_NONE) && (this->player_ptr->prace == prace);
}
