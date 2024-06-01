/*!
 * @brief 内部ゲームデータ定義
 * @author Hourier
 * @date 2024/06/01
 */

#include "system/inner-game-data.h"
#include "player-info/race-types.h"
#include "system/gamevalue.h"

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
        this->game_turn_limit = TURNS_PER_TICK * TOWN_DAWN * MAX_DAYS + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
        return;
    default:
        this->game_turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
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
        return turns - (TURNS_PER_TICK * TOWN_DAWN * 3 / 4);
    default:
        return turns;
    }
}
