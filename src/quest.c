#include "angband.h"
#include "util.h"

#include "floor.h"
#include "floor-save.h"
#include "floor-events.h"
#include "grid.h"
#include "quest.h"
#include "monsterrace-hook.h"
#include "monster.h"
#include "player-status.h"
#include "artifact.h"


/*!
 * @brief クエスト突入時のメッセージテーブル / Array of places to find an inscription
 */
static concptr find_quest[] =
{
	_("床にメッセージが刻まれている:", "You find the following inscription in the floor"),
	_("壁にメッセージが刻まれている:", "You see a message inscribed in the wall"),
	_("メッセージを見つけた:", "There is a sign saying"),
	_("何かが階段の上に書いてある:", "Something is written on the staircase"),
	_("巻物を見つけた。メッセージが書いてある:", "You find a scroll with the following message"),
};

/*!
 * @brief ランダムクエストの討伐ユニークを決める / Determine the random quest uniques
 * @param q_ptr クエスト構造体の参照ポインタ
 * @return なし
 */
void determine_random_questor(quest_type *q_ptr)
{
	MONRACE_IDX r_idx;
	monster_race *r_ptr;

	get_mon_num_prep(mon_hook_quest, NULL);

	while (1)
	{
		/*
		 * Random monster 5 - 10 levels out of depth
		 * (depending on level)
		 */
		r_idx = get_mon_num(q_ptr->level + 5 + randint1(q_ptr->level / 10));
		r_ptr = &r_info[r_idx];

		if (!(r_ptr->flags1 & RF1_UNIQUE)) continue;
		if (r_ptr->flags1 & RF1_QUESTOR) continue;
		if (r_ptr->rarity > 100) continue;
		if (r_ptr->flags7 & RF7_FRIENDLY) continue;
		if (r_ptr->flags7 & RF7_AQUATIC) continue;
		if (r_ptr->flags8 & RF8_WILD_ONLY) continue;
		if (no_questor_or_bounty_uniques(r_idx)) continue;

		/*
		 * Accept monsters that are 2 - 6 levels
		 * out of depth depending on the quest level
		 */
		if (r_ptr->level > (q_ptr->level + (q_ptr->level / 20))) break;
	}

	q_ptr->r_idx = r_idx;
}

/*!
 * @brief クエストを達成状態にする /
 * @param quest_num 達成状態にしたいクエストのID
 * @return なし
 */
void complete_quest(QUEST_IDX quest_num)
{
	quest_type* const q_ptr = &quest[quest_num];

	switch (q_ptr->type)
	{
	case QUEST_TYPE_RANDOM:
		if (record_rand_quest) do_cmd_write_nikki(NIKKI_RAND_QUEST_C, quest_num, NULL);
		break;
	default:
		if (record_fix_quest) do_cmd_write_nikki(NIKKI_FIX_QUEST_C, quest_num, NULL);
		break;
	}

	q_ptr->status = QUEST_STATUS_COMPLETED;
	q_ptr->complev = p_ptr->lev;
	update_playtime();
	q_ptr->comptime = current_world_ptr->play_time;

	if (!(q_ptr->flags & QUEST_FLAG_SILENT))
	{
		play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_QUEST_CLEAR);
		msg_print(_("クエストを達成した！", "You just completed your quest!"));
		msg_print(NULL);
	}
}



/*!
 * @brief 特定の敵を倒した際にクエスト達成処理 /
 * Check for "Quest" completion when a quest monster is killed or charmed.
 * @param m_ptr 撃破したモンスターの構造体参照ポインタ
 * @return なし
 */
void check_quest_completion(monster_type *m_ptr)
{
	POSITION y, x;
	QUEST_IDX quest_num;

	bool create_stairs = FALSE;
	bool reward = FALSE;

	object_type forge;
	object_type *o_ptr;

	y = m_ptr->fy;
	x = m_ptr->fx;

	/* Inside a quest */
	quest_num = p_ptr->inside_quest;

	/* Search for an active quest on this dungeon level */
	if (!quest_num)
	{
		QUEST_IDX i;

		for (i = max_q_idx - 1; i > 0; i--)
		{
			quest_type* const q_ptr = &quest[i];

			/* Quest is not active */
			if (q_ptr->status != QUEST_STATUS_TAKEN)
				continue;

			/* Quest is not a dungeon quest */
			if (q_ptr->flags & QUEST_FLAG_PRESET)
				continue;

			/* Quest is not on this level */
			if ((q_ptr->level != current_floor_ptr->dun_level) &&
				(q_ptr->type != QUEST_TYPE_KILL_ANY_LEVEL))
				continue;

			/* Not a "kill monster" quest */
			if ((q_ptr->type == QUEST_TYPE_FIND_ARTIFACT) ||
				(q_ptr->type == QUEST_TYPE_FIND_EXIT))
				continue;

			/* Interesting quest */
			if ((q_ptr->type == QUEST_TYPE_KILL_NUMBER) ||
				(q_ptr->type == QUEST_TYPE_TOWER) ||
				(q_ptr->type == QUEST_TYPE_KILL_ALL))
				break;

			/* Interesting quest */
			if (((q_ptr->type == QUEST_TYPE_KILL_LEVEL) ||
				(q_ptr->type == QUEST_TYPE_KILL_ANY_LEVEL) ||
				(q_ptr->type == QUEST_TYPE_RANDOM)) &&
				(q_ptr->r_idx == m_ptr->r_idx))
				break;
		}

		quest_num = i;
	}

	/* Handle the current quest */
	if (quest_num && (quest[quest_num].status == QUEST_STATUS_TAKEN))
	{
		/* Current quest */
		quest_type* const q_ptr = &quest[quest_num];

		switch (q_ptr->type)
		{
		case QUEST_TYPE_KILL_NUMBER:
		{
			q_ptr->cur_num++;

			if (q_ptr->cur_num >= q_ptr->num_mon)
			{
				complete_quest(quest_num);

				q_ptr->cur_num = 0;
			}
			break;
		}
		case QUEST_TYPE_KILL_ALL:
		{
			if (!is_hostile(m_ptr)) break;

			if (count_all_hostile_monsters() == 1)
			{
				if (q_ptr->flags & QUEST_FLAG_SILENT)
				{
					q_ptr->status = QUEST_STATUS_FINISHED;
				}
				else
				{
					complete_quest(quest_num);
				}
			}
			break;
		}
		case QUEST_TYPE_KILL_LEVEL:
		case QUEST_TYPE_RANDOM:
		{
			/* Only count valid monsters */
			if (q_ptr->r_idx != m_ptr->r_idx)
				break;

			q_ptr->cur_num++;

			if (q_ptr->cur_num >= q_ptr->max_num)
			{
				complete_quest(quest_num);

				if (!(q_ptr->flags & QUEST_FLAG_PRESET))
				{
					create_stairs = TRUE;
					p_ptr->inside_quest = 0;
				}

				/* Finish the two main quests without rewarding */
				if ((quest_num == QUEST_OBERON) || (quest_num == QUEST_SERPENT))
				{
					q_ptr->status = QUEST_STATUS_FINISHED;
				}

				if (q_ptr->type == QUEST_TYPE_RANDOM)
				{
					reward = TRUE;
					q_ptr->status = QUEST_STATUS_FINISHED;
				}
			}
			break;
		}
		case QUEST_TYPE_KILL_ANY_LEVEL:
		{
			q_ptr->cur_num++;
			if (q_ptr->cur_num >= q_ptr->max_num)
			{
				complete_quest(quest_num);
				q_ptr->cur_num = 0;
			}
			break;
		}
		case QUEST_TYPE_TOWER:
		{
			if (!is_hostile(m_ptr)) break;

			if (count_all_hostile_monsters() == 1)
			{
				q_ptr->status = QUEST_STATUS_STAGE_COMPLETED;

				if ((quest[QUEST_TOWER1].status == QUEST_STATUS_STAGE_COMPLETED) &&
					(quest[QUEST_TOWER2].status == QUEST_STATUS_STAGE_COMPLETED) &&
					(quest[QUEST_TOWER3].status == QUEST_STATUS_STAGE_COMPLETED))
				{

					complete_quest(QUEST_TOWER1);
				}
			}
			break;
		}
		}
	}

	/* Create a magical staircase */
	if (create_stairs)
	{
		POSITION ny, nx;

		/* Stagger around */
		while (cave_perma_bold(y, x) || current_floor_ptr->grid_array[y][x].o_idx || (current_floor_ptr->grid_array[y][x].info & CAVE_OBJECT))
		{
			/* Pick a location */
			scatter(&ny, &nx, y, x, 1, 0);

			/* Stagger */
			y = ny; x = nx;
		}

		/* Explain the staircase */
		msg_print(_("魔法の階段が現れた...", "A magical staircase appears..."));

		/* Create stairs down */
		cave_set_feat(y, x, feat_down_stair);

		/* Remember to update everything */
		p_ptr->update |= (PU_FLOW);
	}

	/*
	 * Drop quest reward
	 */
	if (reward)
	{
		int i;

		for (i = 0; i < (current_floor_ptr->dun_level / 15) + 1; i++)
		{
			o_ptr = &forge;
			object_wipe(o_ptr);

			/* Make a great object */
			make_object(o_ptr, AM_GOOD | AM_GREAT);
			(void)drop_near(o_ptr, -1, y, x);
		}
	}
}

/*!
 * @brief 特定のアーティファクトを入手した際のクエスト達成処理 /
 * Check for "Quest" completion when a quest monster is killed or charmed.
 * @param o_ptr 入手したオブジェクトの構造体参照ポインタ
 * @return なし
 */
void check_find_art_quest_completion(object_type *o_ptr)
{
	QUEST_IDX i;
	/* Check if completed a quest */
	for (i = 0; i < max_q_idx; i++)
	{
		if ((quest[i].type == QUEST_TYPE_FIND_ARTIFACT) &&
			(quest[i].status == QUEST_STATUS_TAKEN) &&
			(quest[i].k_idx == o_ptr->name1))
		{
			complete_quest(i);
		}
	}
}


/*!
 * @brief クエストの導入メッセージを表示する / Discover quest
 * @param q_idx 開始されたクエストのID
 */
void quest_discovery(QUEST_IDX q_idx)
{
	quest_type *q_ptr = &quest[q_idx];
	monster_race *r_ptr = &r_info[q_ptr->r_idx];
	MONSTER_NUMBER q_num = q_ptr->max_num;
	GAME_TEXT name[MAX_NLEN];

	if (!q_idx) return;

	strcpy(name, (r_name + r_ptr->name));

	msg_print(find_quest[rand_range(0, 4)]);
	msg_print(NULL);

	if (q_num == 1)
	{
		if ((r_ptr->flags1 & RF1_UNIQUE) && (0 == r_ptr->max_num))
		{
			msg_print(_("この階は以前は誰かによって守られていたようだ…。", "It seems that this level was protected by someone before..."));
			/* The unique is already dead */
			quest[q_idx].status = QUEST_STATUS_FINISHED;
			q_ptr->complev = 0;
			update_playtime();
			q_ptr->comptime = current_world_ptr->play_time;
		}
		else
		{
			msg_format(_("注意せよ！この階は%sによって守られている！", "Beware, this level is protected by %s!"), name);
		}
	}
	else
	{
#ifndef JP
		plural_aux(name);
#endif
		msg_format(_("注意しろ！この階は%d体の%sによって守られている！", "Be warned, this level is guarded by %d %s!"), q_num, name);

	}
}


/*!
 * @brief 新しく入ったダンジョンの階層に固定されている一般のクエストを探し出しIDを返す。
 * / Hack -- Check if a level is a "quest" level
 * @param level 検索対象になる階
 * @return クエストIDを返す。該当がない場合0を返す。
 */
QUEST_IDX quest_number(DEPTH level)
{
	QUEST_IDX i;

	/* Check quests */
	if (p_ptr->inside_quest)
		return (p_ptr->inside_quest);

	for (i = 0; i < max_q_idx; i++)
	{
		if (quest[i].status != QUEST_STATUS_TAKEN) continue;

		if ((quest[i].type == QUEST_TYPE_KILL_LEVEL) &&
			!(quest[i].flags & QUEST_FLAG_PRESET) &&
			(quest[i].level == level) &&
			(quest[i].dungeon == p_ptr->dungeon_idx))
			return (i);
	}

	/* Check for random quest */
	return (random_quest_number(level));
}

/*!
 * @brief 新しく入ったダンジョンの階層に固定されているランダムクエストを探し出しIDを返す。
 * @param level 検索対象になる階
 * @return クエストIDを返す。該当がない場合0を返す。
 */
QUEST_IDX random_quest_number(DEPTH level)
{
	QUEST_IDX i;

	if (p_ptr->dungeon_idx != DUNGEON_ANGBAND) return 0;

	for (i = MIN_RANDOM_QUEST; i < MAX_RANDOM_QUEST + 1; i++)
	{
		if ((quest[i].type == QUEST_TYPE_RANDOM) &&
			(quest[i].status == QUEST_STATUS_TAKEN) &&
			(quest[i].level == level) &&
			(quest[i].dungeon == DUNGEON_ANGBAND))
		{
			return i;
		}
	}

	return 0;
}

/*!
 * @brief クエスト階層から離脱する際の処理
 * @return なし
 */
void leave_quest_check(void)
{
	/* Save quest number for dungeon pref file ($LEAVING_QUEST) */
	leaving_quest = p_ptr->inside_quest;

	/* Leaving an 'only once' quest marks it as failed */
	if (leaving_quest)
	{
		quest_type* const q_ptr = &quest[leaving_quest];

		if (((q_ptr->flags & QUEST_FLAG_ONCE) || (q_ptr->type == QUEST_TYPE_RANDOM)) &&
			(q_ptr->status == QUEST_STATUS_TAKEN))
		{
			q_ptr->status = QUEST_STATUS_FAILED;
			q_ptr->complev = p_ptr->lev;
			update_playtime();
			q_ptr->comptime = current_world_ptr->play_time;

			/* Additional settings */
			switch (q_ptr->type)
			{
			case QUEST_TYPE_TOWER:
				quest[QUEST_TOWER1].status = QUEST_STATUS_FAILED;
				quest[QUEST_TOWER1].complev = p_ptr->lev;
				break;
			case QUEST_TYPE_FIND_ARTIFACT:
				a_info[q_ptr->k_idx].gen_flags &= ~(TRG_QUESTITEM);
				break;
			case QUEST_TYPE_RANDOM:
				r_info[q_ptr->r_idx].flags1 &= ~(RF1_QUESTOR);

				/* Floor of random quest will be blocked */
				prepare_change_floor_mode(CFM_NO_RETURN);
				break;
			}

			/* Record finishing a quest */
			if (q_ptr->type == QUEST_TYPE_RANDOM)
			{
				if (record_rand_quest) do_cmd_write_nikki(NIKKI_RAND_QUEST_F, leaving_quest, NULL);
			}
			else
			{
				if (record_fix_quest) do_cmd_write_nikki(NIKKI_FIX_QUEST_F, leaving_quest, NULL);
			}
		}
	}
}

/*!
 * @brief 「塔」クエストの各階層から離脱する際の処理
 * @return なし
 */
void leave_tower_check(void)
{
	leaving_quest = p_ptr->inside_quest;
	/* Check for Tower Quest */
	if (leaving_quest &&
		(quest[leaving_quest].type == QUEST_TYPE_TOWER) &&
		(quest[QUEST_TOWER1].status != QUEST_STATUS_COMPLETED))
	{
		if (quest[leaving_quest].type == QUEST_TYPE_TOWER)
		{
			quest[QUEST_TOWER1].status = QUEST_STATUS_FAILED;
			quest[QUEST_TOWER1].complev = p_ptr->lev;
			update_playtime();
			quest[QUEST_TOWER1].comptime = current_world_ptr->play_time;
		}
	}
}


/*!
 * @brief クエスト入り口にプレイヤーが乗った際の処理 / Do building commands
 * @return なし
 */
void do_cmd_quest(void)
{
	if (p_ptr->wild_mode) return;

	take_turn(p_ptr, 100);

	if (!cave_have_flag_bold(p_ptr->y, p_ptr->x, FF_QUEST_ENTER))
	{
		msg_print(_("ここにはクエストの入口はない。", "You see no quest level here."));
		return;
	}
	else
	{
		msg_print(_("ここにはクエストへの入口があります。", "There is an entry of a quest."));
		if (!get_check(_("クエストに入りますか？", "Do you enter? "))) return;
		if ((p_ptr->pseikaku == SEIKAKU_COMBAT) || (inventory[INVEN_BOW].name1 == ART_CRIMSON))
			msg_print(_("『とにかく入ってみようぜぇ。』", ""));
		else if (p_ptr->pseikaku == SEIKAKU_CHARGEMAN) msg_print("『全滅してやるぞ！』");

		/* Player enters a new quest */
		p_ptr->oldpy = 0;
		p_ptr->oldpx = 0;

		leave_quest_check();

		if (quest[p_ptr->inside_quest].type != QUEST_TYPE_RANDOM) current_floor_ptr->dun_level = 1;
		p_ptr->inside_quest = current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].special;

		p_ptr->leaving = TRUE;
	}
}

