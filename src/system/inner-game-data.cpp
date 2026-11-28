/*!
 * @brief 内部ゲームデータ定義
 * @author Hourier
 * @date 2024/06/01
 */

#include "system/inner-game-data.h"
#include "player-info/race-types.h"
#include "system/gamevalue.h"
#include "term/term-color-types.h"

InnerGameData InnerGameData::instance{};

InnerGameData::InnerGameData()
    : start_race(PlayerRaceType::HUMAN)
{
}

InnerGameData &InnerGameData::get_instance()
{
    return instance;
}

PlayerRaceType InnerGameData::get_start_race() const
{
    return this->start_race;
}

int InnerGameData::get_game_turn_limit() const
{
    return this->game_turn_limit;
}

void InnerGameData::init_turn_limit()
{
    switch (this->start_race) {
    case PlayerRaceType::VAMPIRE:
    case PlayerRaceType::SKELETON:
    case PlayerRaceType::ZOMBIE:
    case PlayerRaceType::SPECTRE:
        this->game_turn_limit = TURNS_PER_DAY * MAX_DAYS + TURNS_PER_DAY * 3 / 4;
        return;
    default:
        this->game_turn_limit = TURNS_PER_DAY * (MAX_DAYS - 1) + TURNS_PER_DAY * 3 / 4;
        return;
    }
}

void InnerGameData::set_start_race(PlayerRaceType race)
{
    this->start_race = race;
}

/*!
 * @brief ゲームターンからの実時間換算を行うための補正をかける
 * @param turns ゲームターン
 * @details アンデッド種族は18:00からゲームを開始するので、この修正を予め行う。
 * @return 修正をかけた後のゲームターン
 */
int InnerGameData::get_real_turns(int turns) const
{
    switch (this->start_race) {
    case PlayerRaceType::VAMPIRE:
    case PlayerRaceType::SKELETON:
    case PlayerRaceType::ZOMBIE:
    case PlayerRaceType::SPECTRE:
        return turns - (TURNS_PER_DAY * 3 / 4);
    default:
        return turns;
    }
}

uint32_t InnerGameData::get_total_play_time() const
{
    return this->total_play_time;
}

void InnerGameData::set_total_play_time(uint32_t time)
{
    this->total_play_time = time;
}

void InnerGameData::add_play_time(uint32_t time)
{
    this->total_play_time += time;
}

bool InnerGameData::is_no_score() const
{
    return this->noscore > 0;
}

uint16_t InnerGameData::get_no_score() const
{
    return this->noscore;
}

void InnerGameData::initialize_no_score()
{
    this->noscore = 0;
}

void InnerGameData::add_no_score(uint16_t new_state)
{
    this->noscore |= new_state;
}

term_color_type InnerGameData::get_birth_class_color(PlayerClassType c) const
{
    if (c >= PlayerClassType::MAX) {
        return TERM_WHITE;
    }

    if (this->is_retired_class(c)) {
        return TERM_L_DARK;
    }

    return this->is_winner_class(c) ? TERM_SLATE : TERM_WHITE;
}

size_t InnerGameData::get_won_classes_count() const
{
    return this->won_classes.count();
}

bool InnerGameData::has_not_won_class(PlayerClassType pc) const
{
    return this->won_classes.has_not(pc);
}

const EnumClassFlagGroup<PlayerClassType> &InnerGameData::get_won_classes() const
{
    return this->won_classes;
}

void InnerGameData::set_won_classes(const EnumClassFlagGroup<PlayerClassType> &classes)
{
    this->won_classes = classes;
}

/*!
 * @brief 勝利したクラスを追加する
 */
void InnerGameData::add_winner_class(PlayerClassType c)
{
    if (!this->noscore) {
        this->won_classes.set(c);
    }
}

bool InnerGameData::has_not_retired_class(PlayerClassType pc) const
{
    return this->retired_classes.has_not(pc);
}

const EnumClassFlagGroup<PlayerClassType> &InnerGameData::get_retired_classes() const
{
    return this->retired_classes;
}

/*!
 * @brief 引退したクラスを追加する
 */
void InnerGameData::add_retired_class(PlayerClassType c)
{
    if (!this->noscore) {
        this->retired_classes.set(c);
    }
}

void InnerGameData::set_retired_classes(const EnumClassFlagGroup<PlayerClassType> &classes)
{
    this->retired_classes = classes;
}

/*!
 * @brief 勝利したクラスか判定する
 */
bool InnerGameData::is_winner_class(PlayerClassType c) const
{
    if (c >= PlayerClassType::MAX) {
        return false;
    }

    return this->won_classes.has(c);
}

/*!
 * @brief 引退したクラスか判定する
 */
bool InnerGameData::is_retired_class(PlayerClassType c) const
{
    if (c >= PlayerClassType::MAX) {
        return false;
    }

    return this->retired_classes.has(c);
}
