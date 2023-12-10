#include "world/world.h"
#include "player-info/race-types.h"
#include "system/player-type-definition.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"

AngbandWorld world;
AngbandWorld *w_ptr = &world;

/*!
 * @brief アリーナへの入場/退出状態を更新する
 * @param 入場ならばtrue、退出ならばfalse
 */
void AngbandWorld::set_arena(const bool new_status)
{
    this->is_out_arena = new_status;
}

/*!
 * @brief アリーナへの入場/退出状態を取得する
 * @return アリーナ状態
 */
bool AngbandWorld::get_arena() const
{
    return this->is_out_arena;
}

/*!
 * @brief ゲーム時間が日中かどうかを返す
 * @return 日中ならばTRUE、夜ならばFALSE
 */
bool AngbandWorld::is_daytime() const
{
    int a_day = TURNS_PER_TICK * TOWN_DAWN;
    return (this->game_turn % a_day) < (a_day / 2);
}

/*!
 * @brief プレイ開始からの経過時間を計算する
 * @param start_race 開始時のプレイヤー種族
 * @return 日数、時間、分
 */
std::tuple<int, int, int> AngbandWorld::extract_date_time(PlayerRaceType start_race) const
{
    const auto a_day = TURNS_PER_TICK * TOWN_DAWN;
    const auto turn_in_today = (this->game_turn + a_day / 4) % a_day;
    int day;
    switch (start_race) {
    case PlayerRaceType::VAMPIRE:
    case PlayerRaceType::SKELETON:
    case PlayerRaceType::ZOMBIE:
    case PlayerRaceType::SPECTRE:
        day = (this->game_turn - a_day * 3 / 4) / a_day + 1;
        break;
    default:
        day = (this->game_turn + a_day / 4) / a_day + 1;
        break;
    }

    auto hour = (24 * turn_in_today / a_day) % 24;
    auto min = (1440 * turn_in_today / a_day) % 60;
    return { day, hour, min };
}

/*!
 * @brief 実ゲームプレイ時間を更新する
 */
void AngbandWorld::update_playtime()
{
    if (this->start_time == 0) {
        return;
    }

    const auto current_time = static_cast<uint32_t>(time(nullptr));
    this->play_time += (current_time - this->start_time);
    this->start_time = current_time;
}

/*!
 * @brief 勝利したクラスを追加する
 */
void AngbandWorld::add_winner_class(PlayerClassType c)
{
    if (!this->noscore) {
        this->sf_winner.set(c);
    }
}

/*!
 * @brief 引退したクラスを追加する
 */
void AngbandWorld::add_retired_class(PlayerClassType c)
{
    if (!this->noscore) {
        this->sf_retired.set(c);
    }
}

term_color_type AngbandWorld::get_birth_class_color(PlayerClassType c) const
{
    if (c >= PlayerClassType::MAX) {
        return TERM_WHITE;
    }

    if (this->is_retired_class(c)) {
        return TERM_L_DARK;
    }

    return this->is_winner_class(c) ? TERM_SLATE : TERM_WHITE;
}

/*!
 * @brief 勝利したクラスか判定する
 */
bool AngbandWorld::is_winner_class(PlayerClassType c) const
{
    if (c == PlayerClassType::MAX) {
        return false;
    }

    return this->sf_winner.has(c);
}

/*!
 * @brief 引退したクラスか判定する
 */
bool AngbandWorld::is_retired_class(PlayerClassType c) const
{
    if (c == PlayerClassType::MAX) {
        return false;
    }

    return this->sf_retired.has(c);
}
