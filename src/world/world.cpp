#include "world/world.h"
#include "player/player-race-types.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

world_type world;
world_type *current_world_ptr = &world;

/*!
 * @brief ゲーム時間が日中かどうかを返す /
 * Whether daytime or not
 * @return 日中ならばTRUE、夜ならばFALSE
 */
bool is_daytime(void)
{
	s32b len = TURNS_PER_TICK * TOWN_DAWN;
	if ((current_world_ptr->game_turn % len) < (len / 2))
		return true;
	else
		return false;
}

/*!
 * @brief 現在の日数、時刻を返す /
 * Extract day, hour, min
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param day 日数を返すための参照ポインタ
 * @param hour 時数を返すための参照ポインタ
 * @param min 分数を返すための参照ポインタ
 */
void extract_day_hour_min(player_type *player_ptr, int *day, int *hour, int *min)
{
	const s32b A_DAY = TURNS_PER_TICK * TOWN_DAWN;
	s32b turn_in_today = (current_world_ptr->game_turn + A_DAY / 4) % A_DAY;

	switch (player_ptr->start_race)
	{
	case player_race_type::VAMPIRE:
	case player_race_type::SKELETON:
	case player_race_type::ZOMBIE:
	case player_race_type::SPECTRE:
		*day = (current_world_ptr->game_turn - A_DAY * 3 / 4) / A_DAY + 1;
		break;
	default:
		*day = (current_world_ptr->game_turn + A_DAY / 4) / A_DAY + 1;
		break;
	}
	*hour = (24 * turn_in_today / A_DAY) % 24;
	*min = (1440 * turn_in_today / A_DAY) % 60;
}

/*!
 * @brief 実ゲームプレイ時間を更新する
 */
void update_playtime(void)
{
    if (current_world_ptr->start_time != 0) {
        u32b tmp = (u32b)time(NULL);
        current_world_ptr->play_time += (tmp - current_world_ptr->start_time);
        current_world_ptr->start_time = tmp;
    }
}

/*!
 * @brief 勝利したクラスを追加する
 */
void add_winner_class(player_class_type c)
{
    if (!current_world_ptr->noscore)
        current_world_ptr->sf_winner.set(c);
}

/*!
 * @brief 引退したクラスを追加する
 */
void add_retired_class(player_class_type c)
{
    if (!current_world_ptr->noscore)
        current_world_ptr->sf_retired.set(c);
}

/*!
 * @brief 勝利したクラスか判定する
 */
bool is_winner_class(player_class_type c)
{
    if (c == MAX_CLASS)
        return false;
    return current_world_ptr->sf_winner.has(c);
}

/*!
 * @brief 引退したクラスか判定する
 */
bool is_retired_class(player_class_type c)
{
    if (c == MAX_CLASS)
        return false;
    return current_world_ptr->sf_retired.has(c);
}
