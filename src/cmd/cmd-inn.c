#include "angband.h"
#include "cmd/cmd-inn.h"
#include "cmd/cmd-magiceat.h"
#include "io/write-diary.h"
#include "world.h"
#include "player-effects.h"
#include "core.h" // 暫定、後で消す.
#include "rumor.h"

/*!
 * @brief 宿屋で食事を摂る
 * @param customer_ptr プレーヤーへの参照ポインタ
 * @return 満腹ならFALSE、そうでないならTRUE
 */
static bool buy_food(player_type *customer_ptr)
{
	if (customer_ptr->food >= PY_FOOD_FULL)
	{
		msg_print(_("今は満腹だ。", "You are full now."));
		return FALSE;
	}

	msg_print(_("バーテンはいくらかの食べ物とビールをくれた。", "The barkeep gives you some gruel and a beer."));
	(void)set_food(customer_ptr, PY_FOOD_MAX - 1);
	return TRUE;
}


/*!
 * @brief 健康体しか宿屋に泊めない処理
 * @param customer_ptr プレーヤーへの参照ポインタ
 * @return 毒でも切り傷でもないならTRUE、そうでないならFALSE
 */
static bool is_healthy_stay(player_type *customer_ptr)
{
	if (!customer_ptr->poisoned && !customer_ptr->cut) return TRUE;

	msg_print(_("あなたに必要なのは部屋ではなく、治療者です。", "You need a healer, not a room."));
	msg_print(NULL);
	msg_print(_("すみません、でもうちで誰かに死なれちゃ困りますんで。", "Sorry, but don't want anyone dying in here."));
	return FALSE;
}


/*!
 * @brief 宿屋に泊まったことを日記に残す
 * @param customer_ptr プレーヤーへの参照ポインタ
 * @param prev_hour 宿屋に入った直後の時刻の時刻
 * @return なし
 */
static void stay_inn(player_type *customer_ptr, int prev_hour)
{
	bool is_player_undead = PRACE_IS_(customer_ptr, RACE_SKELETON) ||
		PRACE_IS_(customer_ptr, RACE_ZOMBIE) ||
		PRACE_IS_(customer_ptr, RACE_VAMPIRE) ||
		PRACE_IS_(customer_ptr, RACE_SPECTRE);
	if ((prev_hour >= 6) && (prev_hour < 18))
	{
		concptr stay_message_jp = is_player_undead ? "宿屋に泊まった" : "日が暮れるまで宿屋で過ごした";
		exe_write_diary(customer_ptr, DIARY_DESCRIPTION, 0, _(stay_message_jp, "stayed during the day at the inn."));
		return;
	}

	concptr stay_message_jp = is_player_undead ? "夜が明けるまで宿屋で過ごした" : "宿屋に泊まった";
	exe_write_diary(customer_ptr, DIARY_DESCRIPTION, 0, _(stay_message_jp, "stayed overnight at the inn."));
}


/*!
 * @brief 宿泊によってゲーム内ターンを経過させる
 * @param なし
 * @return なし
 */
static void pass_game_turn_by_stay(void)
{
	s32b oldturn = current_world_ptr->game_turn;
	current_world_ptr->game_turn =
		(current_world_ptr->game_turn / (TURNS_PER_TICK * TOWN_DAWN / 2) + 1) *
		(TURNS_PER_TICK * TOWN_DAWN / 2);
	if (current_world_ptr->dungeon_turn >= current_world_ptr->dungeon_turn_limit)
		return;

	current_world_ptr->dungeon_turn += MIN((current_world_ptr->game_turn - oldturn), TURNS_PER_TICK * 250) * INN_DUNGEON_TURN_ADJ;
	if (current_world_ptr->dungeon_turn > current_world_ptr->dungeon_turn_limit)
		current_world_ptr->dungeon_turn = current_world_ptr->dungeon_turn_limit;
}


/*!
 * @brief 悪夢モードなら悪夢を見せる
 * @param customer_ptr プレーヤーへの参照ポインタ
 * @return 悪夢モードならばTRUE
 */
static bool have_a_nightmare(player_type *customer_ptr)
{
	if (!ironman_nightmare) return FALSE;

	msg_print(_("眠りに就くと恐ろしい光景が心をよぎった。", "Horrible visions flit through your mind as you sleep."));

	while (TRUE)
	{
		sanity_blast(customer_ptr, NULL, FALSE);
		if (!one_in_(3)) break;
	}

	msg_print(_("あなたは絶叫して目を覚ました。", "You awake screaming."));
	exe_write_diary(customer_ptr, DIARY_DESCRIPTION, 0, _("悪夢にうなされてよく眠れなかった。", "had a nightmare."));
	return TRUE;
}


/*!
 * @brief 体調を元に戻す
 * @param customer_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void back_to_health(player_type *customer_ptr)
{
	set_blind(customer_ptr, 0);
	set_confused(customer_ptr, 0);
	customer_ptr->stun = 0;
	customer_ptr->chp = customer_ptr->mhp;
	customer_ptr->csp = customer_ptr->msp;
}


/*!
 * todo 悪夢を見る前後に全回復しているが、何か意図がある？
 * @brief 宿屋を利用する
 * @param customer_ptr プレーヤーへの参照ポインタ
 * @param cmd 宿屋の利用施設ID
 * @return 施設の利用が実際に行われたらTRUE
 * @details inn commands
 * Note that resting for the night was a perfect way to avoid player
 * ghosts in the town *if* you could only make it to the inn in time (-:
 * Now that the ghosts are temporarily disabled in 2.8.X, this function
 * will not be that useful.  I will keep it in the hopes the player
 * ghost code does become a reality again. Does help to avoid filthy urchins.
 * Resting at night is also a quick way to restock stores -KMW-
 */
bool inn_comm(player_type *customer_ptr, int cmd)
{
	switch (cmd)
	{
	case BACT_FOOD:
		return buy_food(customer_ptr);
	case BACT_REST: /* Rest for the night */
	{
		if (!is_healthy_stay(customer_ptr)) return FALSE;

		int prev_day, prev_hour, prev_min;
		extract_day_hour_min(customer_ptr, &prev_day, &prev_hour, &prev_min);
		stay_inn(customer_ptr, prev_hour);

		pass_game_turn_by_stay();
		prevent_turn_overflow(customer_ptr);

		if ((prev_hour >= 18) && (prev_hour <= 23))
			exe_write_diary(customer_ptr, DIARY_DIALY, 0, NULL);

		customer_ptr->chp = customer_ptr->mhp;
		if (have_a_nightmare(customer_ptr)) return TRUE;

		back_to_health(customer_ptr);
		if (customer_ptr->pclass == CLASS_MAGIC_EATER)
		{
			int i;
			for (i = 0; i < 72; i++)
			{
				customer_ptr->magic_num1[i] = customer_ptr->magic_num2[i] * EATER_CHARGE;
			}

			for (; i < 108; i++)
			{
				customer_ptr->magic_num1[i] = 0;
			}
		}

		if ((prev_hour >= 6) && (prev_hour <= 17))
		{
			msg_print(_("あなたはリフレッシュして目覚め、夕方を迎えた。", "You awake refreshed for the evening."));
			exe_write_diary(customer_ptr, DIARY_DESCRIPTION, 0, _("夕方を迎えた。", "awoke refreshed."));
			break;
		}

		msg_print(_("あなたはリフレッシュして目覚め、新たな日を迎えた。", "You awake refreshed for the new day."));
		exe_write_diary(customer_ptr, DIARY_DESCRIPTION, 0, _("すがすがしい朝を迎えた。", "awoke refreshed."));
		break;
	}
	case BACT_RUMORS: /* Listen for rumors */
	{
		display_rumor(customer_ptr, TRUE);
		break;
	}
	}

	return TRUE;
}
