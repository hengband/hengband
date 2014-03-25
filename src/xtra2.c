/*!
 * @file xtra2.c
 * @brief 雑多なその他の処理2 / effects of various "objects"
 * @date 2014/02/06
 * @author
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research, and\n
 * not for profit purposes provided that this copyright and statement are\n
 * included in all such copies.\n
 * 2014 Deskull rearranged comment for Doxygen.
 */


#include "angband.h"

#define REWARD_CHANCE 10


/*!
 * @brief プレイヤーの経験値について整合性のためのチェックと調整を行う /
 * Advance experience levels and print experience
 * @return なし
 */
void check_experience(void)
{
	bool level_reward = FALSE;
	bool level_mutation = FALSE;
	bool level_inc_stat = FALSE;
	bool android = (p_ptr->prace == RACE_ANDROID ? TRUE : FALSE);
	int  old_lev = p_ptr->lev;

	/* Hack -- lower limit */
	if (p_ptr->exp < 0) p_ptr->exp = 0;
	if (p_ptr->max_exp < 0) p_ptr->max_exp = 0;
	if (p_ptr->max_max_exp < 0) p_ptr->max_max_exp = 0;

	/* Hack -- upper limit */
	if (p_ptr->exp > PY_MAX_EXP) p_ptr->exp = PY_MAX_EXP;
	if (p_ptr->max_exp > PY_MAX_EXP) p_ptr->max_exp = PY_MAX_EXP;
	if (p_ptr->max_max_exp > PY_MAX_EXP) p_ptr->max_max_exp = PY_MAX_EXP;

	/* Hack -- maintain "max" experience */
	if (p_ptr->exp > p_ptr->max_exp) p_ptr->max_exp = p_ptr->exp;

	/* Hack -- maintain "max max" experience */
	if (p_ptr->max_exp > p_ptr->max_max_exp) p_ptr->max_max_exp = p_ptr->max_exp;

	/* Redraw experience */
	p_ptr->redraw |= (PR_EXP);

	/* Handle stuff */
	handle_stuff();


	/* Lose levels while possible */
	while ((p_ptr->lev > 1) &&
	       (p_ptr->exp < ((android ? player_exp_a : player_exp)[p_ptr->lev - 2] * p_ptr->expfact / 100L)))
	{
		/* Lose a level */
		p_ptr->lev--;

		/* Update some stuff */
		p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);

		/* Redraw some stuff */
		p_ptr->redraw |= (PR_LEV | PR_TITLE);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

		/* Handle stuff */
		handle_stuff();
	}


	/* Gain levels while possible */
	while ((p_ptr->lev < PY_MAX_LEVEL) &&
	       (p_ptr->exp >= ((android ? player_exp_a : player_exp)[p_ptr->lev-1] * p_ptr->expfact / 100L)))
	{
		/* Gain a level */
		p_ptr->lev++;

		/* Save the highest level */
		if (p_ptr->lev > p_ptr->max_plv)
		{
			p_ptr->max_plv = p_ptr->lev;

			if ((p_ptr->pclass == CLASS_CHAOS_WARRIOR) ||
			    (p_ptr->muta2 & MUT2_CHAOS_GIFT))
			{
				level_reward = TRUE;
			}
			if (p_ptr->prace == RACE_BEASTMAN)
			{
				if (one_in_(5)) level_mutation = TRUE;
			}
			level_inc_stat = TRUE;

			do_cmd_write_nikki(NIKKI_LEVELUP, p_ptr->lev, NULL);
		}

		/* Sound */
		sound(SOUND_LEVEL);

		/* Message */
#ifdef JP
msg_format("レベル %d にようこそ。", p_ptr->lev);
#else
		msg_format("Welcome to level %d.", p_ptr->lev);

#endif

		/* Update some stuff */
		p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);

		/* Redraw some stuff */
		p_ptr->redraw |= (PR_LEV | PR_TITLE | PR_EXP);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER | PW_SPELL | PW_INVEN);

		/* HPとMPの上昇量を表示 */
		level_up = 1;

		/* Handle stuff */
		handle_stuff();

		level_up = 0;

		if (level_inc_stat)
		{
			if(!(p_ptr->max_plv % 10))
			{
				int choice;
				screen_save();
				while(1)
				{
					int n;
					char tmp[32];

#ifdef JP
					cnv_stat(p_ptr->stat_max[0], tmp);
					prt(format("        a) 腕力 (現在値 %s)", tmp), 2, 14);
					cnv_stat(p_ptr->stat_max[1], tmp);
					prt(format("        b) 知能 (現在値 %s)", tmp), 3, 14);
					cnv_stat(p_ptr->stat_max[2], tmp);
					prt(format("        c) 賢さ (現在値 %s)", tmp), 4, 14);
					cnv_stat(p_ptr->stat_max[3], tmp);
					prt(format("        d) 器用 (現在値 %s)", tmp), 5, 14);
					cnv_stat(p_ptr->stat_max[4], tmp);
					prt(format("        e) 耐久 (現在値 %s)", tmp), 6, 14);
					cnv_stat(p_ptr->stat_max[5], tmp);
					prt(format("        f) 魅力 (現在値 %s)", tmp), 7, 14);
					prt("", 8, 14);
					prt("        どの能力値を上げますか？", 1, 14);
#else
					cnv_stat(p_ptr->stat_max[0], tmp);
					prt(format("        a) Str (cur %s)", tmp), 2, 14);
					cnv_stat(p_ptr->stat_max[1], tmp);
					prt(format("        b) Int (cur %s)", tmp), 3, 14);
					cnv_stat(p_ptr->stat_max[2], tmp);
					prt(format("        c) Wis (cur %s)", tmp), 4, 14);
					cnv_stat(p_ptr->stat_max[3], tmp);
					prt(format("        d) Dex (cur %s)", tmp), 5, 14);
					cnv_stat(p_ptr->stat_max[4], tmp);
					prt(format("        e) Con (cur %s)", tmp), 6, 14);
					cnv_stat(p_ptr->stat_max[5], tmp);
					prt(format("        f) Chr (cur %s)", tmp), 7, 14);
					prt("", 8, 14);
					prt("        Which stat do you want to raise?", 1, 14);
#endif
					while(1)
					{
						choice = inkey();
						if ((choice >= 'a') && (choice <= 'f')) break;
					}
					for(n = 0; n < 6; n++)
						if (n != choice - 'a')
							prt("",n+2,14);
					if (get_check(_("よろしいですか？", "Are you sure? "))) break;
				}
				do_inc_stat(choice - 'a');
				screen_load();
			}
			else if(!(p_ptr->max_plv % 2))
				do_inc_stat(randint0(6));
		}

		if (level_mutation)
		{
			msg_print(_("あなたは変わった気がする...", "You feel different..."));
			(void)gain_random_mutation(0);
			level_mutation = FALSE;
		}

		/*
		 * 報酬でレベルが上ると再帰的に check_experience() が
		 * 呼ばれるので順番を最後にする。
		 */
		if (level_reward)
		{
			gain_level_reward(0);
			level_reward = FALSE;
		}

		/* Update some stuff */
		p_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);

		/* Redraw some stuff */
		p_ptr->redraw |= (PR_LEV | PR_TITLE);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER | PW_SPELL);

		/* Handle stuff */
		handle_stuff();
	}

	/* Load an autopick preference file */
	if (old_lev != p_ptr->lev) autopick_load_pref(FALSE);
}


/*!
 * @brief モンスターを倒した際の財宝svalを返す
 * @param r_idx 倒したモンスターの種族ID
 * @return 財宝のsval
 * @details
 * Hack -- Return the "automatic coin type" of a monster race
 * Used to allocate proper treasure when "Creeping coins" die
 * XXX XXX XXX Note the use of actual "monster names"
 */
static int get_coin_type(int r_idx)
{
	/* Analyze monsters */
	switch (r_idx)
	{
	case MON_COPPER_COINS: return 2;
	case MON_SILVER_COINS: return 5;
	case MON_GOLD_COINS: return 10;
	case MON_MITHRIL_COINS:
	case MON_MITHRIL_GOLEM: return 16;
	case MON_ADAMANT_COINS: return 17;
	}

	/* Assume nothing */
	return 0;
}


/*!
 * @brief オブジェクトがクロークかどうかを判定する /
 * Hack -- determine if a template is Cloak
 * @param k_idx 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトがクロークならばTRUEを返す
 */
static bool kind_is_cloak(int k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	/* Analyze the item type */
	if (k_ptr->tval == TV_CLOAK)
	{
		return (TRUE);
	}

	/* Assume not good */
	return (FALSE);
}


/*!
 * @brief オブジェクトが竿状武器かどうかを判定する /
 * Hack -- determine if a template is Polearm
 * @param k_idx 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが竿状武器ならばTRUEを返す
 */
static bool kind_is_polearm(int k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	/* Analyze the item type */
	if (k_ptr->tval == TV_POLEARM)
	{
		return (TRUE);
	}

	/* Assume not good */
	return (FALSE);
}


/*!
 * @brief オブジェクトが剣かどうかを判定する /
 * Hack -- determine if a template is Sword
 * @param k_idx 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが剣ならばTRUEを返す
 */
static bool kind_is_sword(int k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	/* Analyze the item type */
	if ((k_ptr->tval == TV_SWORD) && (k_ptr->sval > 2))
	{
		return (TRUE);
	}

	/* Assume not good */
	return (FALSE);
}


/*!
 * @brief オブジェクトが魔法書かどうかを判定する /
 * Hack -- determine if a template is Book
 * @param k_idx 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが魔法書ならばTRUEを返す
 */
static bool kind_is_book(int k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	/* Analyze the item type */
	if ((k_ptr->tval >= TV_LIFE_BOOK) && (k_ptr->tval <= TV_CRUSADE_BOOK))
	{
		return (TRUE);
	}

	/* Assume not good */
	return (FALSE);
}


/*!
 * @brief オブジェクトがベースアイテム時点でGOODかどうかを判定する /
 * Hack -- determine if a template is Good book
 * @param k_idx 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトがベースアイテム時点でGOODなアイテムならばTRUEを返す
 */
static bool kind_is_good_book(int k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	/* Analyze the item type */
	if ((k_ptr->tval >= TV_LIFE_BOOK) && (k_ptr->tval <= TV_CRUSADE_BOOK) && (k_ptr->tval != TV_ARCANE_BOOK) && (k_ptr->sval > 1))
	{
		return (TRUE);
	}

	/* Assume not good */
	return (FALSE);
}


/*!
 * @brief オブジェクトが鎧かどうかを判定する /
 * Hack -- determine if a template is Armor
 * @param k_idx 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが鎧ならばTRUEを返す
 */
static bool kind_is_armor(int k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	/* Analyze the item type */
	if (k_ptr->tval == TV_HARD_ARMOR)
	{
		return (TRUE);
	}

	/* Assume not good */
	return (FALSE);
}


/*!
 * @brief オブジェクトが打撃武器かどうかを判定する /
 * Hack -- determine if a template is hafted weapon
 * @param k_idx 判定したいオブジェクトのベースアイテムID
 * @return オブジェクトが打撃武器ならばTRUEを返す
 */
static bool kind_is_hafted(int k_idx)
{
	object_kind *k_ptr = &k_info[k_idx];

	/* Analyze the item type */
	if (k_ptr->tval == TV_HAFTED)
	{
		return (TRUE);
	}

	/* Assume not good */
	return (FALSE);
}

/*!
 * @brief クエストを達成状態にする /
 * @param quest_num 達成状態にしたいクエストのID
 * @return なし
 */
void complete_quest(int quest_num)
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
	q_ptr->complev = (byte)p_ptr->lev;
	update_playtime();
	q_ptr->comptime = playtime;

	if (!(q_ptr->flags & QUEST_FLAG_SILENT))
	{
		msg_print(_("クエストを達成した！", "You just completed your quest!"));
		msg_print(NULL);
	}
}

/*!
 * @brief 現在フロアに残っている敵モンスターの数を返す /
 * @return 現在の敵モンスターの数
 */
static int count_all_hostile_monsters(void)
{
	int x, y;
	int number_mon = 0;

	for (x = 0; x < cur_wid; ++ x)
	{
		for (y = 0; y < cur_hgt; ++ y)
		{
			int m_idx = cave[y][x].m_idx;

			if (m_idx > 0 && is_hostile(&m_list[m_idx]))
			{
				++ number_mon;
			}
		}
	}

	return number_mon;
}

/*!
 * @brief 特定の敵を倒した際にクエスト達成処理 /
 * Check for "Quest" completion when a quest monster is killed or charmed.
 * @param m_ptr 撃破したモンスターの構造体参照ポインタ
 * @return なし
 */
void check_quest_completion(monster_type *m_ptr)
{
	int y, x;

	int quest_num;

	bool create_stairs = FALSE;
	bool reward = FALSE;

	object_type forge;
	object_type *q_ptr;

	/* Get the location */
	y = m_ptr->fy;
	x = m_ptr->fx;

	/* Inside a quest */
	quest_num = p_ptr->inside_quest;		

	/* Search for an active quest on this dungeon level */
	if (!quest_num)
	{
		int i;

		for (i = max_quests - 1; i > 0; i--)
		{
			quest_type* const q_ptr = &quest[i];
			
			/* Quest is not active */
			if (q_ptr->status != QUEST_STATUS_TAKEN)
				continue;

			/* Quest is not a dungeon quest */
			if (q_ptr->flags & QUEST_FLAG_PRESET)
				continue;

			/* Quest is not on this level */
			if ((q_ptr->level != dun_level) &&
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

					if((quest[QUEST_TOWER1].status == QUEST_STATUS_STAGE_COMPLETED) &&
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
		int ny, nx;

		/* Stagger around */
		while (cave_perma_bold(y, x) || cave[y][x].o_idx || (cave[y][x].info & CAVE_OBJECT) )
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

		for (i = 0; i < (dun_level / 15)+1; i++)
		{
			/* Get local object */
			q_ptr = &forge;

			/* Wipe the object */
			object_wipe(q_ptr);

			/* Make a great object */
			make_object(q_ptr, AM_GOOD | AM_GREAT);

			/* Drop it in the dungeon */
			(void)drop_near(q_ptr, -1, y, x);
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
	int i;
	/* Check if completed a quest */
	for (i = 0; i < max_quests; i++)
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
 * @brief モンスターを撃破した際の述語メッセージを返す /
 * Return monster death string
 * @param r_ptr 撃破されたモンスターの種族情報を持つ構造体の参照ポインタ
 * @return 撃破されたモンスターの述語
 */
cptr extract_note_dies(monster_race *r_ptr)
{
	/* Some monsters get "destroyed" */
	if (!monster_living(r_ptr))
	{
		int i;

		for (i = 0; i < 4; i++)
		{
			if (r_ptr->blow[i].method == RBM_EXPLODE)
			{
				return _("は爆発して粉々になった。", " explodes into tiny shreds.");
			}
		}
		return _("を倒した。", " is destroyed.");
	}

	/* Assume a default death */
	return _("は死んだ。", " dies.");
}


/*!
 * @brief モンスターが死亡した時の処理 /
 * Handle the "death" of a monster.
 * @param m_idx 死亡したモンスターのID
 * @param drop_item TRUEならばモンスターのドロップ処理を行う
 * @return 撃破されたモンスターの述語
 * @details
 * <pre>
 * Disperse treasures centered at the monster location based on the
 * various flags contained in the monster flags fields.
 * Check for "Quest" completion when a quest monster is killed.
 * Note that only the player can induce "monster_death()" on Uniques.
 * Thus (for now) all Quest monsters should be Uniques.
 * Note that monsters can now carry objects, and when a monster dies,
 * it drops all of its objects, which may disappear in crowded rooms.
 * </pre>
 */
void monster_death(int m_idx, bool drop_item)
{
	int i, j, y, x;

	int dump_item = 0;
	int dump_gold = 0;

	int number = 0;

	monster_type *m_ptr = &m_list[m_idx];

	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	bool visible = ((m_ptr->ml && !p_ptr->image) || (r_ptr->flags1 & RF1_UNIQUE));

	u32b mo_mode = 0L;

	bool do_gold = (!(r_ptr->flags1 & RF1_ONLY_ITEM));
	bool do_item = (!(r_ptr->flags1 & RF1_ONLY_GOLD));
	bool cloned = (m_ptr->smart & SM_CLONED) ? TRUE : FALSE;
	int force_coin = get_coin_type(m_ptr->r_idx);

	object_type forge;
	object_type *q_ptr;

	bool drop_chosen_item = drop_item && !cloned && !p_ptr->inside_arena
		&& !p_ptr->inside_battle && !is_pet(m_ptr);

	/* The caster is dead? */
	if (world_monster && world_monster == m_idx) world_monster = 0;

	/* Notice changes in view */
	if (r_ptr->flags7 & (RF7_LITE_MASK | RF7_DARK_MASK))
	{
		/* Update some things */
		p_ptr->update |= (PU_MON_LITE);
	}

	/* Get the location */
	y = m_ptr->fy;
	x = m_ptr->fx;

	if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname)
	{
		char m_name[80];

		monster_desc(m_name, m_ptr, MD_INDEF_VISIBLE);
		do_cmd_write_nikki(NIKKI_NAMED_PET, 3, m_name);
	}

	/* Let monsters explode! */
	for (i = 0; i < 4; i++)
	{
		if (r_ptr->blow[i].method == RBM_EXPLODE)
		{
			int flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
			int typ = mbe_info[r_ptr->blow[i].effect].explode_type;
			int d_dice = r_ptr->blow[i].d_dice;
			int d_side = r_ptr->blow[i].d_side;
			int damage = damroll(d_dice, d_side);

			project(m_idx, 3, y, x, damage, typ, flg, -1);
			break;
		}
	}

	if (m_ptr->mflag2 & MFLAG2_CHAMELEON)
	{
		choose_new_monster(m_idx, TRUE, MON_CHAMELEON);
		r_ptr = &r_info[m_ptr->r_idx];
	}

	/* Check for quest completion */
	check_quest_completion(m_ptr);

	/* Handle the possibility of player vanquishing arena combatant -KMW- */
	if (p_ptr->inside_arena && !is_pet(m_ptr))
	{
		p_ptr->exit_bldg = TRUE;

		if (p_ptr->arena_number > MAX_ARENA_MONS)
		{
			msg_print(_("素晴らしい！君こそ真の勝利者だ。", "You are a Genuine Champion!"));
		}
		else
		{
			msg_print(_("勝利！チャンピオンへの道を進んでいる。", "Victorious! You're on your way to becoming Champion."));
		}

		if (arena_info[p_ptr->arena_number].tval)
		{
			/* Get local object */
			q_ptr = &forge;

			/* Prepare to make a prize */
			object_prep(q_ptr, lookup_kind(arena_info[p_ptr->arena_number].tval, arena_info[p_ptr->arena_number].sval));

			apply_magic(q_ptr, object_level, AM_NO_FIXED_ART);

			/* Drop it in the dungeon */
			(void)drop_near(q_ptr, -1, y, x);
		}

		if (p_ptr->arena_number > MAX_ARENA_MONS) p_ptr->arena_number++;
		p_ptr->arena_number++;
		if (record_arena)
		{
			char m_name[80];
			
			/* Extract monster name */
			monster_desc(m_name, m_ptr, MD_IGNORE_HALLU | MD_ASSUME_VISIBLE | MD_INDEF_VISIBLE);
			
			do_cmd_write_nikki(NIKKI_ARENA, p_ptr->arena_number, m_name);
		}
	}

	if (m_idx == p_ptr->riding)
	{
		if (rakuba(-1, FALSE))
		{
			msg_print(_("地面に落とされた。", "You have fallen from your riding pet."));
		}
	}

	/* Drop a dead corpse? */
	if (one_in_(r_ptr->flags1 & RF1_UNIQUE ? 1 : 4) &&
	    (r_ptr->flags9 & (RF9_DROP_CORPSE | RF9_DROP_SKELETON)) &&
	    !(p_ptr->inside_arena || p_ptr->inside_battle || cloned || ((m_ptr->r_idx == today_mon) && is_pet(m_ptr))))
	{
		/* Assume skeleton */
		bool corpse = FALSE;

		/*
		 * We cannot drop a skeleton? Note, if we are in this check,
		 * we *know* we can drop at least a corpse or a skeleton
		 */
		if (!(r_ptr->flags9 & RF9_DROP_SKELETON))
			corpse = TRUE;
		else if ((r_ptr->flags9 & RF9_DROP_CORPSE) && (r_ptr->flags1 & RF1_UNIQUE))
			corpse = TRUE;

		/* Else, a corpse is more likely unless we did a "lot" of damage */
		else if (r_ptr->flags9 & RF9_DROP_CORPSE)
		{
			/* Lots of damage in one blow */
			if ((0 - ((m_ptr->maxhp) / 4)) > m_ptr->hp)
			{
				if (one_in_(5)) corpse = TRUE;
			}
			else
			{
				if (!one_in_(5)) corpse = TRUE;
			}
		}

		/* Get local object */
		q_ptr = &forge;

		/* Prepare to make an object */
		object_prep(q_ptr, lookup_kind(TV_CORPSE, (corpse ? SV_CORPSE : SV_SKELETON)));

		apply_magic(q_ptr, object_level, AM_NO_FIXED_ART);

		q_ptr->pval = m_ptr->r_idx;

		/* Drop it in the dungeon */
		(void)drop_near(q_ptr, -1, y, x);
	}

	/* Drop objects being carried */
	monster_drop_carried_objects(m_ptr);

	if (r_ptr->flags1 & RF1_DROP_GOOD) mo_mode |= AM_GOOD;
	if (r_ptr->flags1 & RF1_DROP_GREAT) mo_mode |= AM_GREAT;

	switch (m_ptr->r_idx)
	{
	case MON_PINK_HORROR:
		/* Pink horrors are replaced with 2 Blue horrors */
		if (!(p_ptr->inside_arena || p_ptr->inside_battle))
		{
			bool notice = FALSE;

			for (i = 0; i < 2; i++)
			{
				int wy = y, wx = x;
				bool pet = is_pet(m_ptr);
				u32b mode = 0L;

				if (pet) mode |= PM_FORCE_PET;

				if (summon_specific((pet ? -1 : m_idx), wy, wx, 100, SUMMON_BLUE_HORROR, mode))
				{
					if (player_can_see_bold(wy, wx))
						notice = TRUE;
				}
			}

			if (notice)
				msg_print(_("ピンク・ホラーは分裂した！", "The Pink horror divides!"));
		}
		break;

	case MON_BLOODLETTER:
		/* Bloodletters of Khorne may drop a blade of chaos */
		if (drop_chosen_item && (randint1(100) < 15))
		{
			/* Get local object */
			q_ptr = &forge;

			/* Prepare to make a Blade of Chaos */
			object_prep(q_ptr, lookup_kind(TV_SWORD, SV_BLADE_OF_CHAOS));

			apply_magic(q_ptr, object_level, AM_NO_FIXED_ART | mo_mode);

			/* Drop it in the dungeon */
			(void)drop_near(q_ptr, -1, y, x);
		}
		break;

	case MON_RAAL:
		if (drop_chosen_item && (dun_level > 9))
		{
			/* Get local object */
			q_ptr = &forge;

			/* Wipe the object */
			object_wipe(q_ptr);

			/* Activate restriction */
			if ((dun_level > 49) && one_in_(5))
				get_obj_num_hook = kind_is_good_book;
			else
				get_obj_num_hook = kind_is_book;

			/* Make a book */
			make_object(q_ptr, mo_mode);

			/* Drop it in the dungeon */
			(void)drop_near(q_ptr, -1, y, x);
		}
		break;

	case MON_DAWN:
		/*
		 * Mega^3-hack: killing a 'Warrior of the Dawn' is likely to
		 * spawn another in the fallen one's place!
		 */
		if (!p_ptr->inside_arena && !p_ptr->inside_battle)
		{
			if (!one_in_(7))
			{
				int wy = y, wx = x;
				int attempts = 100;
				bool pet = is_pet(m_ptr);

				do
				{
					scatter(&wy, &wx, y, x, 20, 0);
				}
				while (!(in_bounds(wy, wx) && cave_empty_bold2(wy, wx)) && --attempts);

				if (attempts > 0)
				{
					u32b mode = 0L;
					if (pet) mode |= PM_FORCE_PET;

					if (summon_specific((pet ? -1 : m_idx), wy, wx, 100, SUMMON_DAWN, mode))
					{
						if (player_can_see_bold(wy, wx))
							msg_print(_("新たな戦士が現れた！", "A new warrior steps forth!"));
					}
				}
			}
		}
		break;

	case MON_UNMAKER:
		/* One more ultra-hack: An Unmaker goes out with a big bang! */
		{
			int flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
			(void)project(m_idx, 6, y, x, 100, GF_CHAOS, flg, -1);
		}
		break;

	case MON_UNICORN_ORD:
	case MON_MORGOTH:
	case MON_ONE_RING:
		/* Reward for "lazy" player */
		if (p_ptr->pseikaku == SEIKAKU_NAMAKE)
		{
			int a_idx = 0;
			artifact_type *a_ptr = NULL;

			if (!drop_chosen_item) break;

			do
			{
				switch (randint0(3))
				{
				case 0:
					a_idx = ART_NAMAKE_HAMMER;
					break;
				case 1:
					a_idx = ART_NAMAKE_BOW;
					break;
				case 2:
					a_idx = ART_NAMAKE_ARMOR;
					break;
				}

				a_ptr = &a_info[a_idx];
			}
			while (a_ptr->cur_num);

			/* Create the artifact */
			if (create_named_art(a_idx, y, x))
			{
				a_ptr->cur_num = 1;

				/* Hack -- Memorize location of artifact in saved floors */
				if (character_dungeon) a_ptr->floor_id = p_ptr->floor_id;
			}
			else if (!preserve_mode) a_ptr->cur_num = 1;
		}
		break;

	case MON_SERPENT:
		if (!drop_chosen_item) break;

		/* Get local object */
		q_ptr = &forge;

		/* Mega-Hack -- Prepare to make "Grond" */
		object_prep(q_ptr, lookup_kind(TV_HAFTED, SV_GROND));

		/* Mega-Hack -- Mark this item as "Grond" */
		q_ptr->name1 = ART_GROND;

		/* Mega-Hack -- Actually create "Grond" */
		apply_magic(q_ptr, -1, AM_GOOD | AM_GREAT);

		/* Drop it in the dungeon */
		(void)drop_near(q_ptr, -1, y, x);

		/* Get local object */
		q_ptr = &forge;

		/* Mega-Hack -- Prepare to make "Chaos" */
		object_prep(q_ptr, lookup_kind(TV_CROWN, SV_CHAOS));

		/* Mega-Hack -- Mark this item as "Chaos" */
		q_ptr->name1 = ART_CHAOS;

		/* Mega-Hack -- Actually create "Chaos" */
		apply_magic(q_ptr, -1, AM_GOOD | AM_GREAT);

		/* Drop it in the dungeon */
		(void)drop_near(q_ptr, -1, y, x);
		break;

	case MON_B_DEATH_SWORD:
		if (drop_chosen_item)
		{
			/* Get local object */
			q_ptr = &forge;

			/* Prepare to make a broken sword */
			object_prep(q_ptr, lookup_kind(TV_SWORD, randint1(2)));

			/* Drop it in the dungeon */
			(void)drop_near(q_ptr, -1, y, x);
		}
		break;

	case MON_A_GOLD:
	case MON_A_SILVER:
		if (drop_chosen_item && ((m_ptr->r_idx == MON_A_GOLD) ||
		     ((m_ptr->r_idx == MON_A_SILVER) && (r_ptr->r_akills % 5 == 0))))
		{
			/* Get local object */
			q_ptr = &forge;

			/* Prepare to make a Can of Toys */
			object_prep(q_ptr, lookup_kind(TV_CHEST, SV_CHEST_KANDUME));

			apply_magic(q_ptr, object_level, AM_NO_FIXED_ART);

			/* Drop it in the dungeon */
			(void)drop_near(q_ptr, -1, y, x);
		}
		break;

	case MON_ROLENTO:
		{
			int flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
			(void)project(m_idx, 3, y, x, damroll(20, 10), GF_FIRE, flg, -1);
		}
		break;

	default:
		if (!drop_chosen_item) break;

		switch (r_ptr->d_char)
		{
		case '(':
			if (dun_level > 0)
			{
				/* Get local object */
				q_ptr = &forge;

				/* Wipe the object */
				object_wipe(q_ptr);

				/* Activate restriction */
				get_obj_num_hook = kind_is_cloak;

				/* Make a cloak */
				make_object(q_ptr, mo_mode);

				/* Drop it in the dungeon */
				(void)drop_near(q_ptr, -1, y, x);
			}
			break;

		case '/':
			if (dun_level > 4)
			{
				/* Get local object */
				q_ptr = &forge;

				/* Wipe the object */
				object_wipe(q_ptr);

				/* Activate restriction */
				get_obj_num_hook = kind_is_polearm;

				/* Make a poleweapon */
				make_object(q_ptr, mo_mode);

				/* Drop it in the dungeon */
				(void)drop_near(q_ptr, -1, y, x);
			}
			break;

		case '[':
			if (dun_level > 19)
			{
				/* Get local object */
				q_ptr = &forge;

				/* Wipe the object */
				object_wipe(q_ptr);

				/* Activate restriction */
				get_obj_num_hook = kind_is_armor;

				/* Make a hard armor */
				make_object(q_ptr, mo_mode);

				/* Drop it in the dungeon */
				(void)drop_near(q_ptr, -1, y, x);
			}
			break;

		case '\\':
			if (dun_level > 4)
			{
				/* Get local object */
				q_ptr = &forge;

				/* Wipe the object */
				object_wipe(q_ptr);

				/* Activate restriction */
				get_obj_num_hook = kind_is_hafted;

				/* Make a hafted weapon */
				make_object(q_ptr, mo_mode);

				/* Drop it in the dungeon */
				(void)drop_near(q_ptr, -1, y, x);
			}
			break;

		case '|':
			if (m_ptr->r_idx != MON_STORMBRINGER)
			{
				/* Get local object */
				q_ptr = &forge;

				/* Wipe the object */
				object_wipe(q_ptr);

				/* Activate restriction */
				get_obj_num_hook = kind_is_sword;

				/* Make a sword */
				make_object(q_ptr, mo_mode);

				/* Drop it in the dungeon */
				(void)drop_near(q_ptr, -1, y, x);
			}
			break;
		}
		break;
	}

	/* Mega-Hack -- drop fixed items */
	if (drop_chosen_item)
	{
		int a_idx = 0;
		int chance = 0;

		switch (m_ptr->r_idx)
		{
		case MON_OBERON:
			if (one_in_(3))
			{
				a_idx = ART_JUDGE;
				chance = 33;
			}
			else
			{
				a_idx = ART_AMBER;
				chance = 50;
			}
			break;

		case MON_GHB:
			a_idx = ART_GHB;
			chance = 100;
			break;

		case MON_STORMBRINGER:
			a_idx = ART_STORMBRINGER;
			chance = 100;
			break;

		case MON_ECHIZEN:
			a_idx = ART_CRIMSON;
			chance = 50;
			break;

		case MON_GANDALF:
			a_idx = ART_ICANUS;
			chance = 20;
			break;

		case MON_OROCHI:
			a_idx = ART_KUSANAGI;
			chance = 25;
			break;

		case MON_DWORKIN:
			a_idx = ART_JUDGE;
			chance = 20;
			break;

		case MON_SAURON:
			if (one_in_(10))
			{
				a_idx = ART_POWER;
				chance = 100;
			}
			else
			{
				a_idx = ART_AHO;
				chance = 100;
			}
			break;

		case MON_BRAND:
			if (!one_in_(3))
			{
				a_idx = ART_BRAND;
				chance = 25;
			}
			else
			{
				a_idx = ART_WEREWINDLE;
				chance = 33;
			}
			break;

		case MON_CORWIN:
			if (!one_in_(3))
			{
				a_idx = ART_GRAYSWANDIR;
				chance = 33;
			}
			else
			{
				a_idx = ART_CORWIN;
				chance = 33;
			}
			break;

		case MON_SURTUR:
			if (!one_in_(3))
			{
				a_idx = ART_TWILIGHT;
				chance = 100;
			}
			else
			{
				a_idx = ART_ORB_OF_FATE;
				chance = 100;
			}
			break;

		case MON_SARUMAN:
			a_idx = ART_ELENDIL;
			chance = 33;
			break;

		case MON_FIONA:
			a_idx = ART_FIONA;
			chance = 50;
			break;

		case MON_JULIAN:
			a_idx = ART_JULIAN;
			chance = 45;
			break;

		case MON_KLING:
			a_idx = ART_DESTINY;
			chance = 40;
			break;

		case MON_GOEMON:
			a_idx = ART_ZANTETSU;
			chance = 100;
			break;

		case MON_HAGEN:
			a_idx = ART_HAGEN;
			chance = 66;
			break;

		case MON_CAINE:
			a_idx = ART_CAINE;
			chance = 50;
			break;

		case MON_BULLGATES:
			a_idx = ART_WINBLOWS;
			chance = 66;
			break;

		case MON_LUNGORTHIN:
			a_idx = ART_CALRIS;
			chance = 50;
			break;

		case MON_JACK_SHADOWS:
			a_idx = ART_JACK;
			chance = 15;
			break;

		case MON_DIO:
			a_idx = ART_STONEMASK;
			chance = 20;
			break;

		case MON_BELD:
			a_idx = ART_SOULCRUSH;
			chance = 10;
			break;

		case MON_PIP:
			a_idx = ART_EXCALIBUR_J;
			chance = 50;
			break;

		case MON_SHUTEN:
			a_idx = ART_SHUTEN_DOJI;
			chance = 33;
			break;

		case MON_GOTHMOG:
			a_idx = ART_GOTHMOG;
			chance = 33;
			break;

		case MON_FUNDIN:
			a_idx = ART_FUNDIN;
			chance = 5;
			break;

		case MON_ROBIN_HOOD:
			a_idx = ART_ROBIN_HOOD;
			chance = 5;
			break;

		case MON_KOGAN:
			a_idx = ART_NANACHO;
			chance = 80;
			break;

		}

		if ((a_idx > 0) && ((randint0(100) < chance) || p_ptr->wizard))
		{
			artifact_type *a_ptr = &a_info[a_idx];

			if (!a_ptr->cur_num)
			{
				/* Create the artifact */
				if (create_named_art(a_idx, y, x))
				{
					a_ptr->cur_num = 1;

					/* Hack -- Memorize location of artifact in saved floors */
					if (character_dungeon) a_ptr->floor_id = p_ptr->floor_id;
				}
				else if (!preserve_mode) a_ptr->cur_num = 1;
			}
		}

		if ((r_ptr->flags7 & RF7_GUARDIAN) && (d_info[dungeon_type].final_guardian == m_ptr->r_idx))
		{
			int k_idx = d_info[dungeon_type].final_object ? d_info[dungeon_type].final_object
				: lookup_kind(TV_SCROLL, SV_SCROLL_ACQUIREMENT);

			if (d_info[dungeon_type].final_artifact)
			{
				int a_idx = d_info[dungeon_type].final_artifact;
				artifact_type *a_ptr = &a_info[a_idx];

				if (!a_ptr->cur_num)
				{
					/* Create the artifact */
					if (create_named_art(a_idx, y, x))
					{
						a_ptr->cur_num = 1;

						/* Hack -- Memorize location of artifact in saved floors */
						if (character_dungeon) a_ptr->floor_id = p_ptr->floor_id;
					}
					else if (!preserve_mode) a_ptr->cur_num = 1;

					/* Prevent rewarding both artifact and "default" object */
					if (!d_info[dungeon_type].final_object) k_idx = 0;
				}
			}

			if (k_idx)
			{
				/* Get local object */
				q_ptr = &forge;

				/* Prepare to make a reward */
				object_prep(q_ptr, k_idx);

				apply_magic(q_ptr, object_level, AM_NO_FIXED_ART | AM_GOOD);

				/* Drop it in the dungeon */
				(void)drop_near(q_ptr, -1, y, x);
			}
			msg_format(_("あなたは%sを制覇した！", "You have conquered %s!"),d_name+d_info[dungeon_type].name);
		}
	}

	/* Determine how much we can drop */
	if ((r_ptr->flags1 & RF1_DROP_60) && (randint0(100) < 60)) number++;
	if ((r_ptr->flags1 & RF1_DROP_90) && (randint0(100) < 90)) number++;
	if  (r_ptr->flags1 & RF1_DROP_1D2) number += damroll(1, 2);
	if  (r_ptr->flags1 & RF1_DROP_2D2) number += damroll(2, 2);
	if  (r_ptr->flags1 & RF1_DROP_3D2) number += damroll(3, 2);
	if  (r_ptr->flags1 & RF1_DROP_4D2) number += damroll(4, 2);

	if (cloned && !(r_ptr->flags1 & RF1_UNIQUE))
		number = 0; /* Clones drop no stuff unless Cloning Pits */

	if (is_pet(m_ptr) || p_ptr->inside_battle || p_ptr->inside_arena)
		number = 0; /* Pets drop no stuff */
	if (!drop_item && (r_ptr->d_char != '$')) number = 0;
	
	if((r_ptr->flags2 & (RF2_MULTIPLY)) && (r_ptr->r_akills > 1024))
		number = 0; /* Limit of Multiply monster drop */

	/* Hack -- handle creeping coins */
	coin_type = force_coin;

	/* Average dungeon and monster levels */
	object_level = (dun_level + r_ptr->level) / 2;

	/* Drop some objects */
	for (j = 0; j < number; j++)
	{
		/* Get local object */
		q_ptr = &forge;

		/* Wipe the object */
		object_wipe(q_ptr);

		/* Make Gold */
		if (do_gold && (!do_item || (randint0(100) < 50)))
		{
			/* Make some gold */
			if (!make_gold(q_ptr)) continue;

			/* XXX XXX XXX */
			dump_gold++;
		}

		/* Make Object */
		else
		{
			/* Make an object */
			if (!make_object(q_ptr, mo_mode)) continue;

			/* XXX XXX XXX */
			dump_item++;
		}

		/* Drop it in the dungeon */
		(void)drop_near(q_ptr, -1, y, x);
	}

	/* Reset the object level */
	object_level = base_level;

	/* Reset "coin" type */
	coin_type = 0;


	/* Take note of any dropped treasure */
	if (visible && (dump_item || dump_gold))
	{
		/* Take notes on treasure */
		lore_treasure(m_idx, dump_item, dump_gold);
	}

	/* Only process "Quest Monsters" */
	if (!(r_ptr->flags1 & RF1_QUESTOR)) return;
	if (p_ptr->inside_battle) return;

	/* Winner? */
	if ((m_ptr->r_idx == MON_SERPENT) && !cloned)
	{
		/* Total winner */
		p_ptr->total_winner = TRUE;

		/* Redraw the "title" */
		p_ptr->redraw |= (PR_TITLE);

		do_cmd_write_nikki(NIKKI_BUNSHOU, 0, _("見事に変愚蛮怒の勝利者となった！", "become *WINNER* of Hengband finely!"));

		if ((p_ptr->pclass == CLASS_CHAOS_WARRIOR) || (p_ptr->muta2 & MUT2_CHAOS_GIFT))
		{
			msg_format(_("%sからの声が響いた。", "The voice of %s booms out:"), chaos_patrons[p_ptr->chaos_patron]);
			msg_print(_("『よくやった、定命の者よ！』", "'Thou art donst well, mortal!'"));
		}

		/* Congratulations */
		msg_print(_("*** おめでとう ***", "*** CONGRATULATIONS ***"));
		msg_print(_("あなたはゲームをコンプリートしました。", "You have won the game!"));
		msg_print(_("準備が整ったら引退(自殺コマンド)しても結構です。", "You may retire (commit suicide) when you are ready."));
	}
}

/*!
 * @brief モンスターに与えたダメージの修正処理 /
 * Modify the physical damage done to the monster.
 * @param m_ptr ダメージを受けるモンスターの構造体参照ポインタ
 * @param dam ダメージ基本値
 * @param is_psy_spear 攻撃手段が光の剣ならばTRUE
 * @return 修正を行った結果のダメージ量
 * @details
 * <pre>
 * (for example when it's invulnerable or shielded)
 * ToDo: Accept a damage-type to calculate the modified damage from
 * things like fire, frost, lightning, poison, ... attacks.
 * "type" is not yet used and should be 0.
 * </pre>
 */
int mon_damage_mod(monster_type *m_ptr, int dam, bool is_psy_spear)
{
	monster_race    *r_ptr = &r_info[m_ptr->r_idx];

	if ((r_ptr->flagsr & RFR_RES_ALL) && dam > 0)
	{
		dam /= 100;
		if ((dam == 0) && one_in_(3)) dam = 1;
	}

	if (MON_INVULNER(m_ptr))
	{
		if (is_psy_spear)
		{
			if (!p_ptr->blind && is_seen(m_ptr))
			{
				msg_print(_("バリアを切り裂いた！", "The barrier is penetrated!"));
			}
		}
		else if (!one_in_(PENETRATE_INVULNERABILITY))
		{
			return (0);
		}
	}
	return (dam);
}


/*!
 * @brief モンスターに与えたダメージを元に経験値を加算する /
 * Calculate experience point to be get
 * @param dam 与えたダメージ量
 * @param m_ptr ダメージを与えたモンスターの構造体参照ポインタ
 * @return なし
 * @details
 * <pre>
 * Even the 64 bit operation is not big enough to avoid overflaw
 * unless we carefully choose orders of multiplication and division.
 * Get the coefficient first, and multiply (potentially huge) base
 * experience point of a monster later.
 * </pre>
 */
static void get_exp_from_mon(int dam, monster_type *m_ptr)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	s32b new_exp;
	u32b new_exp_frac;
	s32b div_h;
	u32b div_l;

	if (!m_ptr->r_idx) return;
	if (is_pet(m_ptr) || p_ptr->inside_battle) return;

	/*
	 * - Ratio of monster's level to player's level effects
	 * - Varying speed effects
	 * - Get a fraction in proportion of damage point
	 */
	new_exp = r_ptr->level * SPEED_TO_ENERGY(m_ptr->mspeed) * dam;
	new_exp_frac = 0;
	div_h = 0L;
	div_l = (p_ptr->max_plv+2) * SPEED_TO_ENERGY(r_ptr->speed);

	/* Use (average maxhp * 2) as a denominator */
	if (!(r_ptr->flags1 & RF1_FORCE_MAXHP))
		s64b_mul(&div_h, &div_l, 0, r_ptr->hdice * (ironman_nightmare ? 2 : 1) * (r_ptr->hside + 1));
	else
		s64b_mul(&div_h, &div_l, 0, r_ptr->hdice * (ironman_nightmare ? 2 : 1) * r_ptr->hside * 2);

	/* Special penalty in the wilderness */
	if (!dun_level && (!(r_ptr->flags8 & RF8_WILD_ONLY) || !(r_ptr->flags1 & RF1_UNIQUE)))
		s64b_mul(&div_h, &div_l, 0, 5);

	/* Do division first to prevent overflaw */
	s64b_div(&new_exp, &new_exp_frac, div_h, div_l);

	/* Special penalty for mutiply-monster */
	if ((r_ptr->flags2 & RF2_MULTIPLY) || (m_ptr->r_idx == MON_DAWN))
	{
		int monnum_penarty = r_ptr->r_akills / 400;
		if (monnum_penarty > 8) monnum_penarty = 8;

		while (monnum_penarty--)
		{
			/* Divide by 4 */
			s64b_RSHIFT(new_exp, new_exp_frac, 2);
		}
	}
	
	/* Special penalty for rest_and_shoot exp scum */
	if ((m_ptr->dealt_damage > (u32b)m_ptr->max_maxhp) && (m_ptr->hp >= 0))
	{
		int over_damage = m_ptr->dealt_damage / m_ptr->max_maxhp;
		if (over_damage > 32) over_damage = 32;

		while (over_damage--)
		{
			/* 9/10 for once */
			s64b_mul(&new_exp, &new_exp_frac, 0, 9);
			s64b_div(&new_exp, &new_exp_frac, 0, 10);
		}
	}

	/* Finally multiply base experience point of the monster */
	s64b_mul(&new_exp, &new_exp_frac, 0, r_ptr->mexp);

	/* Gain experience */
	gain_exp_64(new_exp, new_exp_frac);
}


/*!
 * @brief モンスターのHPをダメージに応じて減算する /
 * Decreases monsters hit points, handling monster death.
 * @param dam 与えたダメージ量
 * @param m_idx ダメージを与えたモンスターのID
 * @param fear ダメージによってモンスターが恐慌状態に陥ったならばTRUEを返す
 * @param note モンスターが倒された際の特別なメッセージ述語
 * @return なし
 * @details
 * <pre>
 * We return TRUE if the monster has been killed (and deleted).
 * We announce monster death (using an optional "death message"
 * if given, and a otherwise a generic killed/destroyed message).
 * Only "physical attacks" can induce the "You have slain" message.
 * Missile and Spell attacks will induce the "dies" message, or
 * various "specialized" messages.  Note that "You have destroyed"
 * and "is destroyed" are synonyms for "You have slain" and "dies".
 * Hack -- unseen monsters yield "You have killed it." message.
 * Added fear (DGK) and check whether to print fear messages -CWS
 * Made name, sex, and capitalization generic -BEN-
 * As always, the "ghost" processing is a total hack.
 * Hack -- we "delay" fear messages by passing around a "fear" flag.
 * XXX XXX XXX Consider decreasing monster experience over time, say,
 * by using "(m_exp * m_lev * (m_lev)) / (p_lev * (m_lev + n_killed))"
 * instead of simply "(m_exp * m_lev) / (p_lev)", to make the first
 * monster worth more than subsequent monsters.  This would also need
 * to induce changes in the monster recall code.
 * </pre>
 */
bool mon_take_hit(int m_idx, int dam, bool *fear, cptr note)
{
	monster_type    *m_ptr = &m_list[m_idx];
	monster_race    *r_ptr = &r_info[m_ptr->r_idx];

	monster_type    exp_mon;

	/* Innocent until proven otherwise */
	bool        innocent = TRUE, thief = FALSE;
	int         i;
	int         expdam;
	int			dealt_damage;

	(void)COPY(&exp_mon, m_ptr, monster_type);
	
	expdam = (m_ptr->hp > dam) ? dam : m_ptr->hp;

	get_exp_from_mon(expdam, &exp_mon);

	/* Genocided by chaos patron */
	if (!m_ptr->r_idx) m_idx = 0;
	
	/* Redraw (later) if needed */
	if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
	if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);

	/* Wake it up */
	(void)set_monster_csleep(m_idx, 0);

	/* Hack - Cancel any special player stealth magics. -LM- */
	if (p_ptr->special_defense & NINJA_S_STEALTH)
	{
		set_superstealth(FALSE);
	}

	/* Genocided by chaos patron */
	if (!m_idx) return TRUE;
	
	/* Remember dealt_damage before this attack*/
	dealt_damage = m_ptr->dealt_damage;

	/* Hurt it */
	m_ptr->hp -= dam;
	
	m_ptr->dealt_damage += dam;
	if(m_ptr->dealt_damage > (u32b)m_ptr->max_maxhp * 100) m_ptr->dealt_damage = m_ptr->max_maxhp * 100;
	if (p_ptr->wizard)
	{
		msg_format( _("合計%d/%dのダメージを与えた。","You do %d (out of %d) damage."),
					m_ptr->dealt_damage, m_ptr->maxhp);
	}

	/* It is dead now */
	if (m_ptr->hp < 0)
	{
		char m_name[80];

		if (r_info[m_ptr->r_idx].flags7 & RF7_TANUKI)
		{
			/* You might have unmasked Tanuki first time */
			r_ptr = &r_info[m_ptr->r_idx];
			m_ptr->ap_r_idx = m_ptr->r_idx;
			if (r_ptr->r_sights < MAX_SHORT) r_ptr->r_sights++;
		}

		if (m_ptr->mflag2 & MFLAG2_CHAMELEON)
		{
			/* You might have unmasked Chameleon first time */
			r_ptr = real_r_ptr(m_ptr);
			if (r_ptr->r_sights < MAX_SHORT) r_ptr->r_sights++;
		}

		if (!(m_ptr->smart & SM_CLONED))
		{
			/* When the player kills a Unique, it stays dead */
			if (r_ptr->flags1 & RF1_UNIQUE)
			{
				r_ptr->max_num = 0;

				/* Mega-Hack -- Banor & Lupart */
				if ((m_ptr->r_idx == MON_BANOR) || (m_ptr->r_idx == MON_LUPART))
				{
					r_info[MON_BANORLUPART].max_num = 0;
					r_info[MON_BANORLUPART].r_pkills++;
					r_info[MON_BANORLUPART].r_akills++;
					if (r_info[MON_BANORLUPART].r_tkills < MAX_SHORT) r_info[MON_BANORLUPART].r_tkills++;
				}
				else if (m_ptr->r_idx == MON_BANORLUPART)
				{
					r_info[MON_BANOR].max_num = 0;
					r_info[MON_BANOR].r_pkills++;
					r_info[MON_BANOR].r_akills++;
					if (r_info[MON_BANOR].r_tkills < MAX_SHORT) r_info[MON_BANOR].r_tkills++;
					r_info[MON_LUPART].max_num = 0;
					r_info[MON_LUPART].r_pkills++;
					r_info[MON_LUPART].r_akills++;
					if (r_info[MON_LUPART].r_tkills < MAX_SHORT) r_info[MON_LUPART].r_tkills++;
				}
			}

			/* When the player kills a Nazgul, it stays dead */
			else if (r_ptr->flags7 & RF7_NAZGUL) r_ptr->max_num--;
		}

		/* Count all monsters killed */
		if (r_ptr->r_akills < MAX_SHORT) r_ptr->r_akills++;

		/* Recall even invisible uniques or winners */
		if ((m_ptr->ml && !p_ptr->image) || (r_ptr->flags1 & RF1_UNIQUE))
		{
			/* Count kills this life */
			if ((m_ptr->mflag2 & MFLAG2_KAGE) && (r_info[MON_KAGE].r_pkills < MAX_SHORT)) r_info[MON_KAGE].r_pkills++;
			else if (r_ptr->r_pkills < MAX_SHORT) r_ptr->r_pkills++;

			/* Count kills in all lives */
			if ((m_ptr->mflag2 & MFLAG2_KAGE) && (r_info[MON_KAGE].r_tkills < MAX_SHORT)) r_info[MON_KAGE].r_tkills++;
			else if (r_ptr->r_tkills < MAX_SHORT) r_ptr->r_tkills++;

			/* Hack -- Auto-recall */
			monster_race_track(m_ptr->ap_r_idx);
		}

		/* Extract monster name */
		monster_desc(m_name, m_ptr, MD_TRUE_NAME);

		/* Don't kill Amberites */
		if ((r_ptr->flags3 & RF3_AMBERITE) && one_in_(2))
		{
			int curses = 1 + randint1(3);
			bool stop_ty = FALSE;
			int count = 0;

			msg_format(_("%^sは恐ろしい血の呪いをあなたにかけた！", "%^s puts a terrible blood curse on you!"), m_name);
			curse_equipment(100, 50);

			do
			{
				stop_ty = activate_ty_curse(stop_ty, &count);
			}
			while (--curses);
		}

		if (r_ptr->flags2 & RF2_CAN_SPEAK)
		{
			char line_got[1024];

			/* Dump a message */
			if (!get_rnd_line(_("mondeath_j.txt", "mondeath.txt"), m_ptr->r_idx, line_got))

				msg_format("%^s %s", m_name, line_got);

#ifdef WORLD_SCORE
			if (m_ptr->r_idx == MON_SERPENT)
			{
				/* Make screen dump */
				screen_dump = make_screen_dump();
			}
#endif
		}

		if (!(d_info[dungeon_type].flags1 & DF1_BEGINNER))
		{
			if (!dun_level && !ambush_flag && !p_ptr->inside_arena)
			{
				chg_virtue(V_VALOUR, -1);
			}
			else if (r_ptr->level > dun_level)
			{
				if (randint1(10) <= (r_ptr->level - dun_level))
					chg_virtue(V_VALOUR, 1);
			}
			if (r_ptr->level > 60)
			{
				chg_virtue(V_VALOUR, 1);
			}
			if (r_ptr->level >= 2 * (p_ptr->lev+1))
				chg_virtue(V_VALOUR, 2);
		}

		if (r_ptr->flags1 & RF1_UNIQUE)
		{
			if (r_ptr->flags3 & (RF3_EVIL | RF3_GOOD)) chg_virtue(V_HARMONY, 2);

			if (r_ptr->flags3 & RF3_GOOD)
			{
				chg_virtue(V_UNLIFE, 2);
				chg_virtue(V_VITALITY, -2);
			}

			if (one_in_(3)) chg_virtue(V_INDIVIDUALISM, -1);
		}

		if (m_ptr->r_idx == MON_BEGGAR || m_ptr->r_idx == MON_LEPER)
		{
			chg_virtue(V_COMPASSION, -1);
		}

		if ((r_ptr->flags3 & RF3_GOOD) &&
			((r_ptr->level) / 10 + (3 * dun_level) >= randint1(100)))
			chg_virtue(V_UNLIFE, 1);

		if (r_ptr->d_char == 'A')
		{
			if (r_ptr->flags1 & RF1_UNIQUE)
				chg_virtue(V_FAITH, -2);
			else if ((r_ptr->level) / 10 + (3 * dun_level) >= randint1(100))
			{
				if (r_ptr->flags3 & RF3_GOOD) chg_virtue(V_FAITH, -1);
				else chg_virtue(V_FAITH, 1);
			}
		}
		else if (r_ptr->flags3 & RF3_DEMON)
		{
			if (r_ptr->flags1 & RF1_UNIQUE)
				chg_virtue(V_FAITH, 2);
			else if ((r_ptr->level) / 10 + (3 * dun_level) >= randint1(100))
				chg_virtue(V_FAITH, 1);
		}

		if ((r_ptr->flags3 & RF3_UNDEAD) && (r_ptr->flags1 & RF1_UNIQUE))
			chg_virtue(V_VITALITY, 2);

		if (r_ptr->r_deaths)
		{
			if (r_ptr->flags1 & RF1_UNIQUE)
			{
				chg_virtue(V_HONOUR, 10);
			}
			else if ((r_ptr->level) / 10 + (2 * dun_level) >= randint1(100))
			{
				chg_virtue(V_HONOUR, 1);
			}
		}
		if ((r_ptr->flags2 & RF2_MULTIPLY) && (r_ptr->r_akills > 1000) && one_in_(10))
		{
			chg_virtue(V_VALOUR, -1);
		}

		for (i = 0; i < 4; i++)
		{
			if (r_ptr->blow[i].d_dice != 0) innocent = FALSE; /* Murderer! */

			if ((r_ptr->blow[i].effect == RBE_EAT_ITEM)
				|| (r_ptr->blow[i].effect == RBE_EAT_GOLD))

				thief = TRUE; /* Thief! */
		}

		/* The new law says it is illegal to live in the dungeon */
		if (r_ptr->level != 0) innocent = FALSE;

		if (thief)
		{
			if (r_ptr->flags1 & RF1_UNIQUE)
				chg_virtue(V_JUSTICE, 3);
			else if (1+((r_ptr->level) / 10 + (2 * dun_level))
				>= randint1(100))
				chg_virtue(V_JUSTICE, 1);
		}
		else if (innocent)
		{
			chg_virtue (V_JUSTICE, -1);
		}

		if ((r_ptr->flags3 & RF3_ANIMAL) && !(r_ptr->flags3 & RF3_EVIL) && !(r_ptr->flags4 & ~(RF4_NOMAGIC_MASK))  && !(r_ptr->flags5 & ~(RF5_NOMAGIC_MASK)) && !(r_ptr->flags6 & ~(RF6_NOMAGIC_MASK)))
		{
			if (one_in_(4)) chg_virtue(V_NATURE, -1);
		}

		if ((r_ptr->flags1 & RF1_UNIQUE) && record_destroy_uniq)
		{
			char note_buf[160];
#ifdef JP
			sprintf(note_buf, "%s%s", r_name + r_ptr->name, (m_ptr->smart & SM_CLONED) ? "(クローン)" : "");
#else
			sprintf(note_buf, "%s%s", r_name + r_ptr->name, (m_ptr->smart & SM_CLONED) ? "(Clone)" : "");
#endif
			do_cmd_write_nikki(NIKKI_UNIQUE, 0, note_buf);
		}

		/* Make a sound */
		sound(SOUND_KILL);

		/* Death by Missile/Spell attack */
		if (note)
		{
			msg_format("%^s%s", m_name, note);
		}

		/* Death by physical attack -- invisible monster */
		else if (!m_ptr->ml)
		{
#ifdef JP
			if ((p_ptr->pseikaku == SEIKAKU_COMBAT) || (inventory[INVEN_BOW].name1 == ART_CRIMSON))
				msg_format("せっかくだから%sを殺した。", m_name);
			else
msg_format("%sを殺した。", m_name);
#else
				msg_format("You have killed %s.", m_name);
#endif

		}

		/* Death by Physical attack -- non-living monster */
		else if (!monster_living(r_ptr))
		{
			int i;
			bool explode = FALSE;

			for (i = 0; i < 4; i++)
			{
				if (r_ptr->blow[i].method == RBM_EXPLODE) explode = TRUE;
			}

			/* Special note at death */
			if (explode)
				msg_format(_("%sは爆発して粉々になった。", "%^s explodes into tiny shreds."), m_name);
			else
			{
#ifdef JP
				if ((p_ptr->pseikaku == SEIKAKU_COMBAT) || (inventory[INVEN_BOW].name1 == ART_CRIMSON))
					msg_format("せっかくだから%sを倒した。", m_name);
				else
msg_format("%sを倒した。", m_name);
#else
				msg_format("You have destroyed %s.", m_name);
#endif
			}
		}

		/* Death by Physical attack -- living monster */
		else
		{
#ifdef JP
			if ((p_ptr->pseikaku == SEIKAKU_COMBAT) || (inventory[INVEN_BOW].name1 == ART_CRIMSON))
				msg_format("せっかくだから%sを葬り去った。", m_name);
			else
msg_format("%sを葬り去った。", m_name);
#else
				msg_format("You have slain %s.", m_name);
#endif

		}
		if ((r_ptr->flags1 & RF1_UNIQUE) && !(m_ptr->smart & SM_CLONED) && !vanilla_town)
		{
			for (i = 0; i < MAX_KUBI; i++)
			{
				if ((kubi_r_idx[i] == m_ptr->r_idx) && !(m_ptr->mflag2 & MFLAG2_CHAMELEON))
				{
					msg_format(_("%sの首には賞金がかかっている。", "There is a price on %s's head."), m_name);
					break;
				}
			}
		}

		/* Generate treasure */
		monster_death(m_idx, TRUE);

		/* Mega hack : replace IKETA to BIKETAL */
		if ((m_ptr->r_idx == MON_IKETA) &&
		    !(p_ptr->inside_arena || p_ptr->inside_battle))
		{
			int dummy_y = m_ptr->fy;
			int dummy_x = m_ptr->fx;
			u32b mode = 0L;

			if (is_pet(m_ptr)) mode |= PM_FORCE_PET;

			/* Delete the monster */
			delete_monster_idx(m_idx);

			if (summon_named_creature(0, dummy_y, dummy_x, MON_BIKETAL, mode))
			{
				msg_print(_("「ハァッハッハッハ！！私がバイケタルだ！！」", "Uwa-hahaha!  *I* am Biketal!"));
			}
		}
		else
		{
			/* Delete the monster */
			delete_monster_idx(m_idx);
		}

		get_exp_from_mon((long)exp_mon.max_maxhp*2, &exp_mon);

		/* Not afraid */
		(*fear) = FALSE;

		/* Monster is dead */
		return (TRUE);
	}


#ifdef ALLOW_FEAR

	/* Mega-Hack -- Pain cancels fear */
	if (MON_MONFEAR(m_ptr) && (dam > 0))
	{
		/* Cure fear */
		if (set_monster_monfear(m_idx, MON_MONFEAR(m_ptr) - randint1(dam)))
		{
			/* No more fear */
			(*fear) = FALSE;
		}
	}

	/* Sometimes a monster gets scared by damage */
	if (!MON_MONFEAR(m_ptr) && !(r_ptr->flags3 & (RF3_NO_FEAR)))
	{
		/* Percentage of fully healthy */
		int percentage = (100L * m_ptr->hp) / m_ptr->maxhp;

		/*
		 * Run (sometimes) if at 10% or less of max hit points,
		 * or (usually) when hit for half its current hit points
		 */
		if ((randint1(10) >= percentage) ||
		    ((dam >= m_ptr->hp) && (randint0(100) < 80)))
		{
			/* Hack -- note fear */
			(*fear) = TRUE;

			/* XXX XXX XXX Hack -- Add some timed fear */
			(void)set_monster_monfear(m_idx, (randint1(10) +
					  (((dam >= m_ptr->hp) && (percentage > 7)) ?
					   20 : ((11 - percentage) * 5))));
		}
	}

#endif

#if 0
	if (p_ptr->riding && (p_ptr->riding == m_idx) && (dam > 0))
	{
		char m_name[80];

		/* Extract monster name */
		monster_desc(m_name, m_ptr, 0);

		if (m_ptr->hp > m_ptr->maxhp/3) dam = (dam + 1) / 2;
		if (rakuba((dam > 200) ? 200 : dam, FALSE))
		{
		msg_format(_("%^sに振り落とされた！", "%^s has thrown you off!"), m_name);
		}
	}
#endif

	/* Not dead yet */
	return (FALSE);
}


/*!
 * @brief 現在のコンソール表示の縦横を返す。 /
 * Get term size and calculate screen size
 * @param wid_p コンソールの表示幅文字数を返す
 * @param hgt_p コンソールの表示行数を返す
 * @return なし
 */
void get_screen_size(int *wid_p, int *hgt_p)
{
	Term_get_size(wid_p, hgt_p);
	*hgt_p -= ROW_MAP + 2;
	*wid_p -= COL_MAP + 2;
	if (use_bigtile) *wid_p /= 2;
}


/*!
 * @brief コンソール上におけるマップ表示の左上位置を返す /
 * Calculates current boundaries Called below and from "do_cmd_locate()".
 * @return なし
 */
void panel_bounds_center(void)
{
	int wid, hgt;

	/* Get size */
	get_screen_size(&wid, &hgt);

	panel_row_max = panel_row_min + hgt - 1;
	panel_row_prt = panel_row_min - 1;
	panel_col_max = panel_col_min + wid - 1;
	panel_col_prt = panel_col_min - 13;
}


/*!
 * @brief コンソールのリサイズに合わせてマップを再描画する /
 * Map resizing whenever the main term changes size
 * @return なし
 */
void resize_map(void)
{
	/* Only if the dungeon exists */
	if (!character_dungeon) return;
	
	/* Mega-Hack -- no panel yet */
	panel_row_max = 0;
	panel_col_max = 0;

	/* Reset the panels */
	panel_row_min = cur_hgt;
	panel_col_min = cur_wid;
				
	verify_panel();

	/* Update stuff */
	p_ptr->update |= (PU_TORCH | PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);

	/* Forget lite/view */
	p_ptr->update |= (PU_UN_VIEW | PU_UN_LITE);

	/* Update lite/view */
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_MON_LITE);

	/* Update monsters */
	p_ptr->update |= (PU_MONSTERS);

	/* Redraw everything */
	p_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_MAP | PR_EQUIPPY);

	/* Hack -- update */
	handle_stuff();
	
	/* Redraw */
	Term_redraw();

	/*
	 * Waiting command;
	 * Place the cursor on the player
	 */
	if (can_save) move_cursor_relative(py, px);

	/* Refresh */
	Term_fresh();
}

/*!
 * @brief コンソールを再描画する /
 * Redraw a term when it is resized
 * @return なし
 */
void redraw_window(void)
{
	/* Only if the dungeon exists */
	if (!character_dungeon) return;

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER);

	/* Window stuff */
	p_ptr->window |= (PW_MESSAGE | PW_OVERHEAD | PW_DUNGEON | PW_MONSTER | PW_OBJECT);

	/* Hack -- update */
	handle_stuff();

	/* Redraw */
	Term_redraw();
}


/*!
 * @brief フォーカスを当てるべきマップ描画の基準座標を指定する（サブルーチン）
 * @param dy 変更先のフロアY座標
 * @param dx 変更先のフロアX座標
 * Handle a request to change the current panel
 * Return TRUE if the panel was changed.
 * Also used in do_cmd_locate
 * @return 実際に再描画が必要だった場合TRUEを返す
 */
bool change_panel(int dy, int dx)
{
	int y, x;
	int wid, hgt;

	/* Get size */
	get_screen_size(&wid, &hgt);

	/* Apply the motion */
	y = panel_row_min + dy * hgt / 2;
	x = panel_col_min + dx * wid / 2;

	/* Verify the row */
	if (y > cur_hgt - hgt) y = cur_hgt - hgt;
	if (y < 0) y = 0;

	/* Verify the col */
	if (x > cur_wid - wid) x = cur_wid - wid;
	if (x < 0) x = 0;

	/* Handle "changes" */
	if ((y != panel_row_min) || (x != panel_col_min))
	{
		/* Save the new panel info */
		panel_row_min = y;
		panel_col_min = x;

		/* Recalculate the boundaries */
		panel_bounds_center();

		/* Update stuff */
		p_ptr->update |= (PU_MONSTERS);

		/* Redraw map */
		p_ptr->redraw |= (PR_MAP);

		/* Handle stuff */
		handle_stuff();

		/* Success */
		return (TRUE);
	}

	/* No change */
	return (FALSE);
}

/*!
 * @brief フォーカスを当てるべきマップ描画の基準座標を指定する
 * @param y 変更先のフロアY座標
 * @param x 変更先のフロアX座標
 * @details
 * Handle a request to change the current panel
 * Return TRUE if the panel was changed.
 * Also used in do_cmd_locate
 * @return 実際に再描画が必要だった場合TRUEを返す
 */
bool change_panel_xy(int y, int x)
{
	int dy = 0, dx = 0;
	int wid, hgt;

	/* Get size */
	get_screen_size(&wid, &hgt);

	if (y < panel_row_min) dy = -1;
	if (y > panel_row_max) dy = 1;
	if (x < panel_col_min) dx = -1;
	if (x > panel_col_max) dx = 1;

	if (!dy && !dx) return (FALSE);

	return change_panel(dy, dx);
}


/*!
 * @brief マップ描画のフォーカスを当てるべき座標を更新する
 * @details
 * Given an row (y) and col (x), this routine detects when a move
 * off the screen has occurred and figures new borders. -RAK-
 * "Update" forces a "full update" to take place.
 * The map is reprinted if necessary, and "TRUE" is returned.
 * @return 実際に再描画が必要だった場合TRUEを返す
 */
void verify_panel(void)
{
	int y = py;
	int x = px;
	int wid, hgt;

	int prow_min;
	int pcol_min;
	int max_prow_min;
	int max_pcol_min;

	/* Get size */
	get_screen_size(&wid, &hgt);

	max_prow_min = cur_hgt - hgt;
	max_pcol_min = cur_wid - wid;

	/* Bounds checking */
	if (max_prow_min < 0) max_prow_min = 0;
	if (max_pcol_min < 0) max_pcol_min = 0;

		/* Center on player */
	if (center_player && (center_running || !running))
	{
		/* Center vertically */
		prow_min = y - hgt / 2;
		if (prow_min < 0) prow_min = 0;
		else if (prow_min > max_prow_min) prow_min = max_prow_min;

		/* Center horizontally */
		pcol_min = x - wid / 2;
		if (pcol_min < 0) pcol_min = 0;
		else if (pcol_min > max_pcol_min) pcol_min = max_pcol_min;
	}
	else
	{
		prow_min = panel_row_min;
		pcol_min = panel_col_min;

		/* Scroll screen when 2 grids from top/bottom edge */
		if (y > panel_row_max - 2)
		{
			while (y > prow_min + hgt-1 - 2)
			{
				prow_min += (hgt / 2);
			}
		}

		if (y < panel_row_min + 2)
		{
			while (y < prow_min + 2)
			{
				prow_min -= (hgt / 2);
			}
		}

		if (prow_min > max_prow_min) prow_min = max_prow_min;
		if (prow_min < 0) prow_min = 0;

		/* Scroll screen when 4 grids from left/right edge */
		if (x > panel_col_max - 4)
		{
			while (x > pcol_min + wid-1 - 4)
			{
				pcol_min += (wid / 2);
			}
		}
		
		if (x < panel_col_min + 4)
		{
			while (x < pcol_min + 4)
			{
				pcol_min -= (wid / 2);
			}
		}

		if (pcol_min > max_pcol_min) pcol_min = max_pcol_min;
		if (pcol_min < 0) pcol_min = 0;
	}

	/* Check for "no change" */
	if ((prow_min == panel_row_min) && (pcol_min == panel_col_min)) return;

	/* Save the new panel info */
	panel_row_min = prow_min;
	panel_col_min = pcol_min;

	/* Hack -- optional disturb on "panel change" */
	if (disturb_panel && !center_player) disturb(0, 0);

	/* Recalculate the boundaries */
	panel_bounds_center();

	/* Update stuff */
	p_ptr->update |= (PU_MONSTERS);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
}


/*
 * Monster health description
 */
cptr look_mon_desc(monster_type *m_ptr, u32b mode)
{
	monster_race *ap_r_ptr = &r_info[m_ptr->ap_r_idx];
	bool         living;
	int          perc;
	cptr desc;
	cptr attitude;
	cptr clone;

	/* Determine if the monster is "living" */
	living = monster_living(ap_r_ptr);

	/* Calculate a health "percentage" */
	perc = 100L * m_ptr->hp / m_ptr->maxhp;

	/* Healthy monsters */
	if (m_ptr->hp >= m_ptr->maxhp)
	{
		/* No damage */
#ifdef JP
		desc = living ? "無傷" : "無ダメージ";
#else
		desc = living ? "unhurt" : "undamaged";
#endif

	}

	else if (perc >= 60)
	{
#ifdef JP
		desc = living ? "軽傷" : "小ダメージ";
#else
		desc = living ? "somewhat wounded" : "somewhat damaged";
#endif

	}

	else if (perc >= 25)
	{
#ifdef JP
		desc = living ? "負傷" : "中ダメージ";
#else
		desc = living ? "wounded" : "damaged";
#endif

	}

	else if (perc >= 10)
	{
#ifdef JP
		desc = living ? "重傷" : "大ダメージ";
#else
		desc = living ? "badly wounded" : "badly damaged";
#endif

	}

	else 
	{
#ifdef JP
		desc = living ? "半死半生" : "倒れかけ";
#else
		desc = living ? "almost dead" : "almost destroyed";
#endif
	}


	/* Need attitude information? */
	if (!(mode & 0x01))
	{
		/* Full information is not needed */
		attitude = "";
	}
	else if (is_pet(m_ptr))
	{
		attitude = _(", ペット", ", pet");
	}
	else if (is_friendly(m_ptr))
	{
		attitude = _(", 友好的", ", friendly");
	}
	else
	{
		attitude = _("", "");
	}


	/* Clone monster? */
	if (m_ptr->smart & SM_CLONED)
	{
		clone = ", clone";
	}
	else
	{
		clone = "";
	}

	/* Display monster's level --- idea borrowed from ToME */
	if (ap_r_ptr->r_tkills && !(m_ptr->mflag2 & MFLAG2_KAGE))
	{
		return format(_("レベル%d, %s%s%s", "Level %d, %s%s%s"), ap_r_ptr->level, desc, attitude, clone);
	}
	else 
	{
		return format(_("レベル???, %s%s%s", "Level ???, %s%s%s"), desc, attitude, clone);
	}
}



/*
 * Angband sorting algorithm -- quick sort in place
 *
 * Note that the details of the data we are sorting is hidden,
 * and we rely on the "ang_sort_comp()" and "ang_sort_swap()"
 * function hooks to interact with the data, which is given as
 * two pointers, and which may have any user-defined form.
 */
void ang_sort_aux(vptr u, vptr v, int p, int q)
{
	int z, a, b;

	/* Done sort */
	if (p >= q) return;

	/* Pivot */
	z = p;

	/* Begin */
	a = p;
	b = q;

	/* Partition */
	while (TRUE)
	{
		/* Slide i2 */
		while (!(*ang_sort_comp)(u, v, b, z)) b--;

		/* Slide i1 */
		while (!(*ang_sort_comp)(u, v, z, a)) a++;

		/* Done partition */
		if (a >= b) break;

		/* Swap */
		(*ang_sort_swap)(u, v, a, b);

		/* Advance */
		a++, b--;
	}

	/* Recurse left side */
	ang_sort_aux(u, v, p, b);

	/* Recurse right side */
	ang_sort_aux(u, v, b+1, q);
}


/*
 * Angband sorting algorithm -- quick sort in place
 *
 * Note that the details of the data we are sorting is hidden,
 * and we rely on the "ang_sort_comp()" and "ang_sort_swap()"
 * function hooks to interact with the data, which is given as
 * two pointers, and which may have any user-defined form.
 */
void ang_sort(vptr u, vptr v, int n)
{
	/* Sort the array */
	ang_sort_aux(u, v, 0, n-1);
}



/*** Targeting Code ***/


/*
 * Determine is a monster makes a reasonable target
 *
 * The concept of "targeting" was stolen from "Morgul" (?)
 *
 * The player can target any location, or any "target-able" monster.
 *
 * Currently, a monster is "target_able" if it is visible, and if
 * the player can hit it with a projection, and the player is not
 * hallucinating.  This allows use of "use closest target" macros.
 *
 * Future versions may restrict the ability to target "trappers"
 * and "mimics", but the semantics is a little bit weird.
 */
bool target_able(int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];

	/* Monster must be alive */
	if (!m_ptr->r_idx) return (FALSE);

	/* Hack -- no targeting hallucinations */
	if (p_ptr->image) return (FALSE);

	/* Monster must be visible */
	if (!m_ptr->ml) return (FALSE);

	if (p_ptr->riding && (p_ptr->riding == m_idx)) return (TRUE);

	/* Monster must be projectable */
	if (!projectable(py, px, m_ptr->fy, m_ptr->fx)) return (FALSE);

	/* XXX XXX XXX Hack -- Never target trappers */
	/* if (CLEAR_ATTR && (CLEAR_CHAR)) return (FALSE); */

	/* Assume okay */
	return (TRUE);
}




/*
 * Update (if necessary) and verify (if possible) the target.
 *
 * We return TRUE if the target is "okay" and FALSE otherwise.
 */
bool target_okay(void)
{
	/* Accept stationary targets */
	if (target_who < 0) return (TRUE);

	/* Check moving targets */
	if (target_who > 0)
	{
		/* Accept reasonable targets */
		if (target_able(target_who))
		{
			monster_type *m_ptr = &m_list[target_who];

			/* Acquire monster location */
			target_row = m_ptr->fy;
			target_col = m_ptr->fx;

			/* Good target */
			return (TRUE);
		}
	}

	/* Assume no target */
	return (FALSE);
}


/*
 * Sorting hook -- comp function -- by "distance to player"
 *
 * We use "u" and "v" to point to arrays of "x" and "y" positions,
 * and sort the arrays by double-distance to the player.
 */
static bool ang_sort_comp_distance(vptr u, vptr v, int a, int b)
{
	byte *x = (byte*)(u);
	byte *y = (byte*)(v);

	int da, db, kx, ky;

	/* Absolute distance components */
	kx = x[a]; kx -= px; kx = ABS(kx);
	ky = y[a]; ky -= py; ky = ABS(ky);

	/* Approximate Double Distance to the first point */
	da = ((kx > ky) ? (kx + kx + ky) : (ky + ky + kx));

	/* Absolute distance components */
	kx = x[b]; kx -= px; kx = ABS(kx);
	ky = y[b]; ky -= py; ky = ABS(ky);

	/* Approximate Double Distance to the first point */
	db = ((kx > ky) ? (kx + kx + ky) : (ky + ky + kx));

	/* Compare the distances */
	return (da <= db);
}


/*
 * Sorting hook -- comp function -- by importance level of grids
 *
 * We use "u" and "v" to point to arrays of "x" and "y" positions,
 * and sort the arrays by level of monster
 */
static bool ang_sort_comp_importance(vptr u, vptr v, int a, int b)
{
	byte *x = (byte*)(u);
	byte *y = (byte*)(v);
	cave_type *ca_ptr = &cave[y[a]][x[a]];
	cave_type *cb_ptr = &cave[y[b]][x[b]];
	monster_type *ma_ptr = &m_list[ca_ptr->m_idx];
	monster_type *mb_ptr = &m_list[cb_ptr->m_idx];
	monster_race *ap_ra_ptr, *ap_rb_ptr;

	/* The player grid */
	if (y[a] == py && x[a] == px) return TRUE;
	if (y[b] == py && x[b] == px) return FALSE;

	/* Extract monster race */
	if (ca_ptr->m_idx && ma_ptr->ml) ap_ra_ptr = &r_info[ma_ptr->ap_r_idx];
	else ap_ra_ptr = NULL;
	if (cb_ptr->m_idx && mb_ptr->ml) ap_rb_ptr = &r_info[mb_ptr->ap_r_idx];
	else ap_rb_ptr = NULL;

	if (ap_ra_ptr && !ap_rb_ptr) return TRUE;
	if (!ap_ra_ptr && ap_rb_ptr) return FALSE;

	/* Compare two monsters */
	if (ap_ra_ptr && ap_rb_ptr)
	{
		/* Unique monsters first */
		if ((ap_ra_ptr->flags1 & RF1_UNIQUE) && !(ap_rb_ptr->flags1 & RF1_UNIQUE)) return TRUE;
		if (!(ap_ra_ptr->flags1 & RF1_UNIQUE) && (ap_rb_ptr->flags1 & RF1_UNIQUE)) return FALSE;

		/* Shadowers first (あやしい影) */
		if ((ma_ptr->mflag2 & MFLAG2_KAGE) && !(mb_ptr->mflag2 & MFLAG2_KAGE)) return TRUE;
		if (!(ma_ptr->mflag2 & MFLAG2_KAGE) && (mb_ptr->mflag2 & MFLAG2_KAGE)) return FALSE;

 		/* Unknown monsters first */
		if (!ap_ra_ptr->r_tkills && ap_rb_ptr->r_tkills) return TRUE;
		if (ap_ra_ptr->r_tkills && !ap_rb_ptr->r_tkills) return FALSE;

		/* Higher level monsters first (if known) */
		if (ap_ra_ptr->r_tkills && ap_rb_ptr->r_tkills)
		{
			if (ap_ra_ptr->level > ap_rb_ptr->level) return TRUE;
			if (ap_ra_ptr->level < ap_rb_ptr->level) return FALSE;
		}

		/* Sort by index if all conditions are same */
		if (ma_ptr->ap_r_idx > mb_ptr->ap_r_idx) return TRUE;
		if (ma_ptr->ap_r_idx < mb_ptr->ap_r_idx) return FALSE;
	}

	/* An object get higher priority */
	if (cave[y[a]][x[a]].o_idx && !cave[y[b]][x[b]].o_idx) return TRUE;
	if (!cave[y[a]][x[a]].o_idx && cave[y[b]][x[b]].o_idx) return FALSE;

	/* Priority from the terrain */
	if (f_info[ca_ptr->feat].priority > f_info[cb_ptr->feat].priority) return TRUE;
	if (f_info[ca_ptr->feat].priority < f_info[cb_ptr->feat].priority) return FALSE;

	/* If all conditions are same, compare distance */
	return ang_sort_comp_distance(u, v, a, b);
}


/*
 * Sorting hook -- swap function -- by "distance to player"
 *
 * We use "u" and "v" to point to arrays of "x" and "y" positions,
 * and sort the arrays by distance to the player.
 */
static void ang_sort_swap_distance(vptr u, vptr v, int a, int b)
{
	byte *x = (byte*)(u);
	byte *y = (byte*)(v);

	byte temp;

	/* Swap "x" */
	temp = x[a];
	x[a] = x[b];
	x[b] = temp;

	/* Swap "y" */
	temp = y[a];
	y[a] = y[b];
	y[b] = temp;
}



/*
 * Hack -- help "select" a location (see below)
 */
static s16b target_pick(int y1, int x1, int dy, int dx)
{
	int i, v;

	int x2, y2, x3, y3, x4, y4;

	int b_i = -1, b_v = 9999;


	/* Scan the locations */
	for (i = 0; i < temp_n; i++)
	{
		/* Point 2 */
		x2 = temp_x[i];
		y2 = temp_y[i];

		/* Directed distance */
		x3 = (x2 - x1);
		y3 = (y2 - y1);

		/* Verify quadrant */
		if (dx && (x3 * dx <= 0)) continue;
		if (dy && (y3 * dy <= 0)) continue;

		/* Absolute distance */
		x4 = ABS(x3);
		y4 = ABS(y3);

		/* Verify quadrant */
		if (dy && !dx && (x4 > y4)) continue;
		if (dx && !dy && (y4 > x4)) continue;

		/* Approximate Double Distance */
		v = ((x4 > y4) ? (x4 + x4 + y4) : (y4 + y4 + x4));

		/* XXX XXX XXX Penalize location */

		/* Track best */
		if ((b_i >= 0) && (v >= b_v)) continue;

		/* Track best */
		b_i = i; b_v = v;
	}

	/* Result */
	return (b_i);
}


/*
 * Hack -- determine if a given location is "interesting"
 */
static bool target_set_accept(int y, int x)
{
	cave_type *c_ptr;

	s16b this_o_idx, next_o_idx = 0;

	/* Bounds */
	if (!(in_bounds(y, x))) return (FALSE);

	/* Player grid is always interesting */
	if (player_bold(y, x)) return (TRUE);


	/* Handle hallucination */
	if (p_ptr->image) return (FALSE);


	/* Examine the grid */
	c_ptr = &cave[y][x];

	/* Visible monsters */
	if (c_ptr->m_idx)
	{
		monster_type *m_ptr = &m_list[c_ptr->m_idx];

		/* Visible monsters */
		if (m_ptr->ml) return (TRUE);
	}

	/* Scan all objects in the grid */
	for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;

		/* Acquire object */
		o_ptr = &o_list[this_o_idx];

		/* Acquire next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Memorized object */
		if (o_ptr->marked & OM_FOUND) return (TRUE);
	}

	/* Interesting memorized features */
	if (c_ptr->info & (CAVE_MARK))
	{
		/* Notice object features */
		if (c_ptr->info & CAVE_OBJECT) return (TRUE);

		/* Feature code (applying "mimic" field) */
		if (have_flag(f_info[get_feat_mimic(c_ptr)].flags, FF_NOTICE)) return TRUE;
	}

	/* Nope */
	return (FALSE);
}


/*
 * Prepare the "temp" array for "target_set"
 *
 * Return the number of target_able monsters in the set.
 */
static void target_set_prepare(int mode)
{
	int y, x;
	int min_hgt, max_hgt, min_wid, max_wid;

	if (mode & TARGET_KILL)
	{
		/* Inner range */
		min_hgt = MAX((py - MAX_RANGE), 0);
		max_hgt = MIN((py + MAX_RANGE), cur_hgt - 1);
		min_wid = MAX((px - MAX_RANGE), 0);
		max_wid = MIN((px + MAX_RANGE), cur_wid - 1);
	}
	else /* not targetting */
	{
		/* Inner panel */
		min_hgt = panel_row_min;
		max_hgt = panel_row_max;
		min_wid = panel_col_min;
		max_wid = panel_col_max;
	}

	/* Reset "temp" array */
	temp_n = 0;

	/* Scan the current panel */
	for (y = min_hgt; y <= max_hgt; y++)
	{
		for (x = min_wid; x <= max_wid; x++)
		{
			cave_type *c_ptr;

			/* Require "interesting" contents */
			if (!target_set_accept(y, x)) continue;

			c_ptr = &cave[y][x];

			/* Require target_able monsters for "TARGET_KILL" */
			if ((mode & (TARGET_KILL)) && !target_able(c_ptr->m_idx)) continue;

			if ((mode & (TARGET_KILL)) && !target_pet && is_pet(&m_list[c_ptr->m_idx])) continue;

			/* Save the location */
			temp_x[temp_n] = x;
			temp_y[temp_n] = y;
			temp_n++;
		}
	}

	/* Set the sort hooks */
	if (mode & (TARGET_KILL))
	{
		/* Target the nearest monster for shooting */
		ang_sort_comp = ang_sort_comp_distance;
		ang_sort_swap = ang_sort_swap_distance;
	}
	else
	{
		/* Look important grids first in Look command */
		ang_sort_comp = ang_sort_comp_importance;
		ang_sort_swap = ang_sort_swap_distance;
	}

	/* Sort the positions */
	ang_sort(temp_x, temp_y, temp_n);

	if (p_ptr->riding && target_pet && (temp_n > 1) && (mode & (TARGET_KILL)))
	{
		byte tmp;

		tmp = temp_y[0];
		temp_y[0] = temp_y[1];
		temp_y[1] = tmp;
		tmp = temp_x[0];
		temp_x[0] = temp_x[1];
		temp_x[1] = tmp;
	}
}
void target_set_prepare_look(){
	target_set_prepare(TARGET_LOOK);
}


/*
 * Evaluate number of kill needed to gain level
 */
static void evaluate_monster_exp(char *buf, monster_type *m_ptr)
{
	monster_race *ap_r_ptr = &r_info[m_ptr->ap_r_idx];
	u32b num;
	s32b exp_mon, exp_adv;
	u32b exp_mon_frac, exp_adv_frac;

	if ((p_ptr->lev >= PY_MAX_LEVEL) || (p_ptr->prace == RACE_ANDROID))
	{
		sprintf(buf,"**");
		return;
	}
	else if (!ap_r_ptr->r_tkills || (m_ptr->mflag2 & MFLAG2_KAGE))
	{
		if (!p_ptr->wizard)
		{
			sprintf(buf,"??");
			return;
		}
	}


	/* The monster's experience point (assuming average monster speed) */
	exp_mon = ap_r_ptr->mexp * ap_r_ptr->level;
	exp_mon_frac = 0;
	s64b_div(&exp_mon, &exp_mon_frac, 0, (p_ptr->max_plv + 2));


	/* Total experience value for next level */
	exp_adv = player_exp[p_ptr->lev -1] * p_ptr->expfact;
	exp_adv_frac = 0;
	s64b_div(&exp_adv, &exp_adv_frac, 0, 100);

	/* Experience value need to get */
	s64b_sub(&exp_adv, &exp_adv_frac, p_ptr->exp, p_ptr->exp_frac);


	/* You need to kill at least one monster to get any experience */
	s64b_add(&exp_adv, &exp_adv_frac, exp_mon, exp_mon_frac);
	s64b_sub(&exp_adv, &exp_adv_frac, 0, 1);

	/* Extract number of monsters needed */
	s64b_div(&exp_adv, &exp_adv_frac, exp_mon, exp_mon_frac);

	/* If 999 or more monsters needed, only display "999". */
	num = MIN(999, exp_adv_frac);

	/* Display the number */
	sprintf(buf,"%03ld", (long int)num);
}


bool show_gold_on_floor = FALSE;

/*
 * Examine a grid, return a keypress.
 *
 * The "mode" argument contains the "TARGET_LOOK" bit flag, which
 * indicates that the "space" key should scan through the contents
 * of the grid, instead of simply returning immediately.  This lets
 * the "look" command get complete information, without making the
 * "target" command annoying.
 *
 * The "info" argument contains the "commands" which should be shown
 * inside the "[xxx]" text.  This string must never be empty, or grids
 * containing monsters will be displayed with an extra comma.
 *
 * Note that if a monster is in the grid, we update both the monster
 * recall info and the health bar info to track that monster.
 *
 * Eventually, we may allow multiple objects per grid, or objects
 * and terrain features in the same grid. XXX XXX XXX
 *
 * This function must handle blindness/hallucination.
 */
static int target_set_aux(int y, int x, int mode, cptr info)
{
	cave_type *c_ptr = &cave[y][x];
	s16b this_o_idx, next_o_idx = 0;
	cptr s1 = "", s2 = "", s3 = "", x_info = "";
	bool boring = TRUE;
	s16b feat;
	feature_type *f_ptr;
	int query = '\001';
	char out_val[MAX_NLEN+80];

#ifdef ALLOW_EASY_FLOOR
	int floor_list[23], floor_num = 0;

	/* Scan all objects in the grid */
	if (easy_floor)
	{
		floor_num = scan_floor(floor_list, y, x, 0x02);

		if (floor_num)
		{
			x_info = _("x物 ", "x,");
		}
	}

#endif /* ALLOW_EASY_FLOOR */

	/* Hack -- under the player */
	if (player_bold(y, x))
	{
		/* Description */
#ifdef JP
		s1 = "あなたは";
		s2 = "の上";
		s3 = "にいる";
#else
		s1 = "You are ";

		/* Preposition */
		s2 = "on ";
#endif
	}
	else
	{
		s1 = _("ターゲット:", "Target:");
	}

	/* Hack -- hallucination */
	if (p_ptr->image)
	{
		cptr name = _("何か奇妙な物", "something strange");

		/* Display a message */
#ifdef JP
		sprintf(out_val, "%s%s%s%s [%s]", s1, name, s2, s3, info);
#else
		sprintf(out_val, "%s%s%s%s [%s]", s1, s2, s3, name, info);
#endif

		prt(out_val, 0, 0);
		move_cursor_relative(y, x);
		query = inkey();

		/* Stop on everything but "return" */
		if ((query != '\r') && (query != '\n')) return query;

		/* Repeat forever */
		return 0;
	}


	/* Actual monsters */
	if (c_ptr->m_idx && m_list[c_ptr->m_idx].ml)
	{
		monster_type *m_ptr = &m_list[c_ptr->m_idx];
		monster_race *ap_r_ptr = &r_info[m_ptr->ap_r_idx];
		char m_name[80];
		bool recall = FALSE;

		/* Not boring */
		boring = FALSE;

		/* Get the monster name ("a kobold") */
		monster_desc(m_name, m_ptr, MD_INDEF_VISIBLE);

		/* Hack -- track this monster race */
		monster_race_track(m_ptr->ap_r_idx);

		/* Hack -- health bar for this monster */
		health_track(c_ptr->m_idx);

		/* Hack -- handle stuff */
		handle_stuff();

		/* Interact */
		while (1)
		{
			char acount[10];

			/* Recall */
			if (recall)
			{
				/* Save */
				screen_save();

				/* Recall on screen */
				screen_roff(m_ptr->ap_r_idx, 0);

				/* Hack -- Complete the prompt (again) */
				Term_addstr(-1, TERM_WHITE, format(_("  [r思 %s%s]", "  [r,%s%s]"), x_info, info));

				/* Command */
				query = inkey();

				/* Restore */
				screen_load();

				/* Normal commands */
				if (query != 'r') break;

				/* Toggle recall */
				recall = FALSE;

				/* Cleare recall text and repeat */
				continue;
			}

			/*** Normal ***/

			/* Describe, and prompt for recall */
			evaluate_monster_exp(acount, m_ptr);

#ifdef JP
			sprintf(out_val, "[%s]%s%s(%s)%s%s [r思 %s%s]", acount, s1, m_name, look_mon_desc(m_ptr, 0x01), s2, s3, x_info, info);
#else
			sprintf(out_val, "[%s]%s%s%s%s(%s) [r, %s%s]", acount, s1, s2, s3, m_name, look_mon_desc(m_ptr, 0x01), x_info, info);
#endif

			prt(out_val, 0, 0);

			/* Place cursor */
			move_cursor_relative(y, x);

			/* Command */
			query = inkey();

			/* Normal commands */
			if (query != 'r') break;

			/* Toggle recall */
			recall = TRUE;
		}

		/* Always stop at "normal" keys */
		if ((query != '\r') && (query != '\n') && (query != ' ') && (query != 'x')) return query;

		/* Sometimes stop at "space" key */
		if ((query == ' ') && !(mode & (TARGET_LOOK))) return query;

		/* Change the intro */
		s1 = _("それは", "It is ");

		/* Hack -- take account of gender */
		if (ap_r_ptr->flags1 & (RF1_FEMALE)) s1 = _("彼女は", "She is ");
		else if (ap_r_ptr->flags1 & (RF1_MALE)) s1 = _("彼は", "He is ");

		/* Use a preposition */
#ifdef JP
		s2 = "を";
		s3 = "持っている";
#else
		s2 = "carrying ";
#endif


		/* Scan all objects being carried */
		for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx)
		{
			char o_name[MAX_NLEN];

			object_type *o_ptr;

			/* Acquire object */
			o_ptr = &o_list[this_o_idx];

			/* Acquire next object */
			next_o_idx = o_ptr->next_o_idx;

			/* Obtain an object description */
			object_desc(o_name, o_ptr, 0);

			/* Describe the object */
#ifdef JP
			sprintf(out_val, "%s%s%s%s[%s]", s1, o_name, s2, s3, info);
#else
			sprintf(out_val, "%s%s%s%s [%s]", s1, s2, s3, o_name, info);
#endif

			prt(out_val, 0, 0);
			move_cursor_relative(y, x);
			query = inkey();

			/* Always stop at "normal" keys */
			if ((query != '\r') && (query != '\n') && (query != ' ') && (query != 'x')) return query;

			/* Sometimes stop at "space" key */
			if ((query == ' ') && !(mode & (TARGET_LOOK))) return query;

			/* Change the intro */
			s2 = _("をまた", "also carrying ");
		}

		/* Use a preposition */
#ifdef JP
		s2 = "の上";
		s3 = "にいる";
#else
		s2 = "on ";
#endif
	}


#ifdef ALLOW_EASY_FLOOR
	if (floor_num)
	{
		int min_width = 0;

		while (1)
		{
			if (floor_num == 1)
			{
				char o_name[MAX_NLEN];

				object_type *o_ptr;

				/* Acquire object */
				o_ptr = &o_list[floor_list[0]];

				/* Describe the object */
				object_desc(o_name, o_ptr, 0);

				/* Message */
#ifdef JP
				sprintf(out_val, "%s%s%s%s[%s]",
					s1, o_name, s2, s3, info);
#else
				sprintf(out_val, "%s%s%s%s [%s]",
					s1, s2, s3, o_name, info);
#endif

				prt(out_val, 0, 0);
				move_cursor_relative(y, x);

				/* Command */
				query = inkey();

				/* End this grid */
				return query;
			}

			/* Provide one cushion before item listing  */
			if (boring)
			{
				/* Display rough information about items */
#ifdef JP
				sprintf(out_val, "%s %d個のアイテム%s%s ['x'で一覧, %s]",
					s1, floor_num, s2, s3, info);
#else
				sprintf(out_val, "%s%s%sa pile of %d items [x,%s]",
					s1, s2, s3, floor_num, info);
#endif

				prt(out_val, 0, 0);
				move_cursor_relative(y, x);

				/* Command */
				query = inkey();

				/* No request for listing */
				if (query != 'x' && query != ' ') return query;
			}


			/** Display list of items **/

			/* Continue scrolling list if requested */
			while (1)
			{
				int i, o_idx;

				/* Save screen */
				screen_save();

				/* Display */
				show_gold_on_floor = TRUE;
				(void)show_floor(0, y, x, &min_width);
				show_gold_on_floor = FALSE;

				/* Prompt */
#ifdef JP
				sprintf(out_val, "%s %d個のアイテム%s%s [Enterで次へ, %s]",
					s1, floor_num, s2, s3, info);
#else
				sprintf(out_val, "%s%s%sa pile of %d items [Enter,%s]",
					s1, s2, s3, floor_num, info);
#endif
				prt(out_val, 0, 0);


				/* Wait */
				query = inkey();

				/* Load screen */
				screen_load();

				/* Exit unless 'Enter' */
				if (query != '\n' && query != '\r')
				{
					return query;
				}

				/* Get the object being moved. */
				o_idx = c_ptr->o_idx;
 
				/* Only rotate a pile of two or more objects. */
				if (!(o_idx && o_list[o_idx].next_o_idx)) continue;

				/* Remove the first object from the list. */
				excise_object_idx(o_idx);

				/* Find end of the list. */
				i = c_ptr->o_idx;
				while (o_list[i].next_o_idx)
					i = o_list[i].next_o_idx;

				/* Add after the last object. */
				o_list[i].next_o_idx = o_idx;

				/* Loop and re-display the list */
			}
		}

		/* NOTREACHED */
	}
#endif /* ALLOW_EASY_FLOOR */


	/* Scan all objects in the grid */
	for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;

		/* Acquire object */
		o_ptr = &o_list[this_o_idx];

		/* Acquire next object */
		next_o_idx = o_ptr->next_o_idx;

		/* Describe it */
		if (o_ptr->marked & OM_FOUND)
		{
			char o_name[MAX_NLEN];

			/* Not boring */
			boring = FALSE;

			/* Obtain an object description */
			object_desc(o_name, o_ptr, 0);

			/* Describe the object */
#ifdef JP
			sprintf(out_val, "%s%s%s%s[%s]", s1, o_name, s2, s3, info);
#else
			sprintf(out_val, "%s%s%s%s [%s]", s1, s2, s3, o_name, info);
#endif

			prt(out_val, 0, 0);
			move_cursor_relative(y, x);
			query = inkey();

			/* Always stop at "normal" keys */
			if ((query != '\r') && (query != '\n') && (query != ' ') && (query != 'x')) return query;

			/* Sometimes stop at "space" key */
			if ((query == ' ') && !(mode & TARGET_LOOK)) return query;

			/* Change the intro */
			s1 = _("それは", "It is ");

			/* Plurals */
			if (o_ptr->number != 1) s1 = _("それらは", "They are ");

			/* Preposition */
#ifdef JP
			s2 = "の上";
			s3 = "に見える";
#else
			s2 = "on ";
#endif

		}
	}


	/* Feature code (applying "mimic" field) */
	feat = get_feat_mimic(c_ptr);

	/* Require knowledge about grid, or ability to see grid */
	if (!(c_ptr->info & CAVE_MARK) && !player_can_see_bold(y, x))
	{
		/* Forget feature */
		feat = feat_none;
	}

	f_ptr = &f_info[feat];

	/* Terrain feature if needed */
	if (boring || have_flag(f_ptr->flags, FF_REMEMBER))
	{
		cptr name;

		/* Hack -- special handling for quest entrances */
		if (have_flag(f_ptr->flags, FF_QUEST_ENTER))
		{
			/* Set the quest number temporary */
			int old_quest = p_ptr->inside_quest;
			int j;

			/* Clear the text */
			for (j = 0; j < 10; j++) quest_text[j][0] = '\0';
			quest_text_line = 0;

			p_ptr->inside_quest = c_ptr->special;

			/* Get the quest text */
			init_flags = INIT_NAME_ONLY;

			process_dungeon_file("q_info.txt", 0, 0, 0, 0);

			name = format(_("クエスト「%s」(%d階相当)", "the entrance to the quest '%s'(level %d)"), 
						quest[c_ptr->special].name, quest[c_ptr->special].level);

			/* Reset the old quest number */
			p_ptr->inside_quest = old_quest;
		}

		/* Hack -- special handling for building doors */
		else if (have_flag(f_ptr->flags, FF_BLDG) && !p_ptr->inside_arena)
		{
			name = building[f_ptr->subtype].name;
		}
		else if (have_flag(f_ptr->flags, FF_ENTRANCE))
		{
			name = format(_("%s(%d階相当)", "%s(level %d)"), d_text + d_info[c_ptr->special].text, d_info[c_ptr->special].mindepth);
		}
		else if (have_flag(f_ptr->flags, FF_TOWN))
		{
			name = town[c_ptr->special].name;
		}
		else if (p_ptr->wild_mode && (feat == feat_floor))
		{
			name = _("道", "road");
		}
		else
		{
			name = f_name + f_ptr->name;
		}


		/* Pick a prefix */
		if (*s2 &&
		    ((!have_flag(f_ptr->flags, FF_MOVE) && !have_flag(f_ptr->flags, FF_CAN_FLY)) ||
		     (!have_flag(f_ptr->flags, FF_LOS) && !have_flag(f_ptr->flags, FF_TREE)) ||
		     have_flag(f_ptr->flags, FF_TOWN)))
		{
			s2 = _("の中", "in ");
		}

		/* Hack -- special introduction for store & building doors -KMW- */
		if (have_flag(f_ptr->flags, FF_STORE) ||
		    have_flag(f_ptr->flags, FF_QUEST_ENTER) ||
		    (have_flag(f_ptr->flags, FF_BLDG) && !p_ptr->inside_arena) ||
		    have_flag(f_ptr->flags, FF_ENTRANCE))
		{
#ifdef JP
			s2 = "の入口";
#else
			s3 = "";
#endif
		}
#ifndef JP
		else if (have_flag(f_ptr->flags, FF_FLOOR) ||
			 have_flag(f_ptr->flags, FF_TOWN) ||
			 have_flag(f_ptr->flags, FF_SHALLOW) ||
			 have_flag(f_ptr->flags, FF_DEEP))
		{
			s3 ="";
		}
		else
		{
			/* Pick proper indefinite article */
			s3 = (is_a_vowel(name[0])) ? "an " : "a ";
		}
#endif

		/* Display a message */
		if (p_ptr->wizard)
		{
			char f_idx_str[32];
			if (c_ptr->mimic) sprintf(f_idx_str, "%d/%d", c_ptr->feat, c_ptr->mimic);
			else sprintf(f_idx_str, "%d", c_ptr->feat);
#ifdef JP
			sprintf(out_val, "%s%s%s%s[%s] %x %s %d %d %d (%d,%d) %d", s1, name, s2, s3, info, c_ptr->info, f_idx_str, c_ptr->dist, c_ptr->cost, c_ptr->when, y, x, travel.cost[y][x]);
#else
			sprintf(out_val, "%s%s%s%s [%s] %x %s %d %d %d (%d,%d)", s1, s2, s3, name, info, c_ptr->info, f_idx_str, c_ptr->dist, c_ptr->cost, c_ptr->when, y, x);
#endif
		}
		else
#ifdef JP
			sprintf(out_val, "%s%s%s%s[%s]", s1, name, s2, s3, info);
#else
			sprintf(out_val, "%s%s%s%s [%s]", s1, s2, s3, name, info);
#endif

		prt(out_val, 0, 0);
		move_cursor_relative(y, x);
		query = inkey();

		/* Always stop at "normal" keys */
		if ((query != '\r') && (query != '\n') && (query != ' ')) return query;
	}

	/* Stop on everything but "return" */
	if ((query != '\r') && (query != '\n')) return query;

	/* Repeat forever */
	return 0;
}


/*
 * Handle "target" and "look".
 *
 * Note that this code can be called from "get_aim_dir()".
 *
 * All locations must be on the current panel.  Consider the use of
 * "panel_bounds()" to allow "off-panel" targets, perhaps by using
 * some form of "scrolling" the map around the cursor.  XXX XXX XXX
 * That is, consider the possibility of "auto-scrolling" the screen
 * while the cursor moves around.  This may require changes in the
 * "update_mon()" code to allow "visibility" even if off panel, and
 * may require dynamic recalculation of the "temp" grid set.
 *
 * Hack -- targeting/observing an "outer border grid" may induce
 * problems, so this is not currently allowed.
 *
 * The player can use the direction keys to move among "interesting"
 * grids in a heuristic manner, or the "space", "+", and "-" keys to
 * move through the "interesting" grids in a sequential manner, or
 * can enter "location" mode, and use the direction keys to move one
 * grid at a time in any direction.  The "t" (set target) command will
 * only target a monster (as opposed to a location) if the monster is
 * target_able and the "interesting" mode is being used.
 *
 * The current grid is described using the "look" method above, and
 * a new command may be entered at any time, but note that if the
 * "TARGET_LOOK" bit flag is set (or if we are in "location" mode,
 * where "space" has no obvious meaning) then "space" will scan
 * through the description of the current grid until done, instead
 * of immediately jumping to the next "interesting" grid.  This
 * allows the "target" command to retain its old semantics.
 *
 * The "*", "+", and "-" keys may always be used to jump immediately
 * to the next (or previous) interesting grid, in the proper mode.
 *
 * The "return" key may always be used to scan through a complete
 * grid description (forever).
 *
 * This command will cancel any old target, even if used from
 * inside the "look" command.
 */
bool target_set(int mode)
{
	int		i, d, m, t, bd;
	int		y = py;
	int		x = px;

	bool	done = FALSE;

	bool	flag = TRUE;

	char	query;

	char	info[80];

	cave_type		*c_ptr;

	int wid, hgt;
	
	/* Get size */
	get_screen_size(&wid, &hgt);

	/* Cancel target */
	target_who = 0;


	/* Cancel tracking */
	/* health_track(0); */


	/* Prepare the "temp" array */
	target_set_prepare(mode);

	/* Start near the player */
	m = 0;

	/* Interact */
	while (!done)
	{
		/* Interesting grids */
		if (flag && temp_n)
		{
			y = temp_y[m];
			x = temp_x[m];

			/* Set forcus */
			change_panel_xy(y, x);

			if (!(mode & TARGET_LOOK)) prt_path(y, x);

			/* Access */
			c_ptr = &cave[y][x];

			/* Allow target */
			if (target_able(c_ptr->m_idx))
			{
				strcpy(info, _("q止 t決 p自 o現 +次 -前", "q,t,p,o,+,-,<dir>"));
			}

			/* Dis-allow target */
			else
			{
				strcpy(info, _("q止 p自 o現 +次 -前", "q,p,o,+,-,<dir>"));
			}
			
			/* Describe and Prompt */
			while (TRUE){
				query = target_set_aux(y, x, mode, info);
				if(query)break;
			}

			/* Cancel tracking */
			/* health_track(0); */

			/* Assume no "direction" */
			d = 0;

			if (use_menu)
			{
				if (query == '\r') query = 't';
			}  

			/* Analyze */
			switch (query)
			{
				case ESCAPE:
				case 'q':
				{
					done = TRUE;
					break;
				}

				case 't':
				case '.':
				case '5':
				case '0':
				{
					if (target_able(c_ptr->m_idx))
					{
						health_track(c_ptr->m_idx);
						target_who = c_ptr->m_idx;
						target_row = y;
						target_col = x;
						done = TRUE;
					}
					else
					{
						bell();
					}
					break;
				}

				case ' ':
				case '*':
				case '+':
				{
					if (++m == temp_n)
					{
						m = 0;
						if (!expand_list) done = TRUE;
					}
					break;
				}

				case '-':
				{
					if (m-- == 0)
					{
						m = temp_n - 1;
						if (!expand_list) done = TRUE;
					}
					break;
				}

				case 'p':
				{
					/* Recenter the map around the player */
					verify_panel();

					/* Update stuff */
					p_ptr->update |= (PU_MONSTERS);

					/* Redraw map */
					p_ptr->redraw |= (PR_MAP);

					/* Window stuff */
					p_ptr->window |= (PW_OVERHEAD);

					/* Handle stuff */
					handle_stuff();

					/* Recalculate interesting grids */
					target_set_prepare(mode);

					y = py;
					x = px;
				}

				case 'o':
				{
					flag = FALSE;
					break;
				}

				case 'm':
				{
					break;
				}

				default:
				{
					/* Extract the action (if any) */
					d = get_keymap_dir(query);

					if (!d) bell();
					break;
				}
			}

			/* Hack -- move around */
			if (d)
			{
				/* Modified to scroll to monster */
				int y2 = panel_row_min;
				int x2 = panel_col_min;

				/* Find a new monster */
				i = target_pick(temp_y[m], temp_x[m], ddy[d], ddx[d]);

				/* Request to target past last interesting grid */
				while (flag && (i < 0))
				{
					/* Note the change */
					if (change_panel(ddy[d], ddx[d]))
					{
						int v = temp_y[m];
						int u = temp_x[m];

						/* Recalculate interesting grids */
						target_set_prepare(mode);

						/* Look at interesting grids */
						flag = TRUE;

						/* Find a new monster */
						i = target_pick(v, u, ddy[d], ddx[d]);

						/* Use that grid */
						if (i >= 0) m = i;
					}

					/* Nothing interesting */
					else
					{
						int dx = ddx[d];
						int dy = ddy[d];

						/* Restore previous position */
						panel_row_min = y2;
						panel_col_min = x2;
						panel_bounds_center();

						/* Update stuff */
						p_ptr->update |= (PU_MONSTERS);

						/* Redraw map */
						p_ptr->redraw |= (PR_MAP);

						/* Window stuff */
						p_ptr->window |= (PW_OVERHEAD);

						/* Handle stuff */
						handle_stuff();

						/* Recalculate interesting grids */
						target_set_prepare(mode);

						/* Look at boring grids */
						flag = FALSE;

						/* Move */
						x += dx;
						y += dy;

						/* Do not move horizontally if unnecessary */
						if (((x < panel_col_min + wid / 2) && (dx > 0)) ||
							 ((x > panel_col_min + wid / 2) && (dx < 0)))
						{
							dx = 0;
						}

						/* Do not move vertically if unnecessary */
						if (((y < panel_row_min + hgt / 2) && (dy > 0)) ||
							 ((y > panel_row_min + hgt / 2) && (dy < 0)))
						{
							dy = 0;
						}

						/* Apply the motion */
						if ((y >= panel_row_min+hgt) || (y < panel_row_min) ||
						    (x >= panel_col_min+wid) || (x < panel_col_min))
						{
							if (change_panel(dy, dx)) target_set_prepare(mode);
						}

						/* Slide into legality */
						if (x >= cur_wid-1) x = cur_wid - 2;
						else if (x <= 0) x = 1;

						/* Slide into legality */
						if (y >= cur_hgt-1) y = cur_hgt- 2;
						else if (y <= 0) y = 1;
					}
				}

				/* Use that grid */
				m = i;
			}
		}

		/* Arbitrary grids */
		else
		{
			bool move_fast = FALSE;

			if (!(mode & TARGET_LOOK)) prt_path(y, x);

			/* Access */
			c_ptr = &cave[y][x];

			/* Default prompt */
			strcpy(info, _("q止 t決 p自 m近 +次 -前", "q,t,p,m,+,-,<dir>"));

			/* Describe and Prompt (enable "TARGET_LOOK") */
			while (!(query = target_set_aux(y, x, mode | TARGET_LOOK, info)));

			/* Cancel tracking */
			/* health_track(0); */

			/* Assume no direction */
			d = 0;

			if (use_menu)
			{
				if (query == '\r') query = 't';
			}  

			/* Analyze the keypress */
			switch (query)
			{
				case ESCAPE:
				case 'q':
				{
					done = TRUE;
					break;
				}

				case 't':
				case '.':
				case '5':
				case '0':
				{
					target_who = -1;
					target_row = y;
					target_col = x;
					done = TRUE;
					break;
				}

				case 'p':
				{
					/* Recenter the map around the player */
					verify_panel();

					/* Update stuff */
					p_ptr->update |= (PU_MONSTERS);

					/* Redraw map */
					p_ptr->redraw |= (PR_MAP);

					/* Window stuff */
					p_ptr->window |= (PW_OVERHEAD);

					/* Handle stuff */
					handle_stuff();

					/* Recalculate interesting grids */
					target_set_prepare(mode);

					y = py;
					x = px;
				}

				case 'o':
				{
					break;
				}

				case ' ':
				case '*':
				case '+':
				case '-':
				case 'm':
				{
					flag = TRUE;

					m = 0;
					bd = 999;

					/* Pick a nearby monster */
					for (i = 0; i < temp_n; i++)
					{
						t = distance(y, x, temp_y[i], temp_x[i]);

						/* Pick closest */
						if (t < bd)
						{
							m = i;
							bd = t;
						}
					}

					/* Nothing interesting */
					if (bd == 999) flag = FALSE;

					break;
				}

				default:
				{
					/* Extract the action (if any) */
					d = get_keymap_dir(query);

					/* XTRA HACK MOVEFAST */
					if (isupper(query)) move_fast = TRUE;

					if (!d) bell();
					break;
				}
			}

			/* Handle "direction" */
			if (d)
			{
				int dx = ddx[d];
				int dy = ddy[d];

				/* XTRA HACK MOVEFAST */
				if (move_fast)
				{
					int mag = MIN(wid / 2, hgt / 2);
					x += dx * mag;
					y += dy * mag;
				}
				else
				{
					x += dx;
					y += dy;
				}

				/* Do not move horizontally if unnecessary */
				if (((x < panel_col_min + wid / 2) && (dx > 0)) ||
					 ((x > panel_col_min + wid / 2) && (dx < 0)))
				{
					dx = 0;
				}

				/* Do not move vertically if unnecessary */
				if (((y < panel_row_min + hgt / 2) && (dy > 0)) ||
					 ((y > panel_row_min + hgt / 2) && (dy < 0)))
				{
					dy = 0;
				}

				/* Apply the motion */
				if ((y >= panel_row_min + hgt) || (y < panel_row_min) ||
					 (x >= panel_col_min + wid) || (x < panel_col_min))
				{
					if (change_panel(dy, dx)) target_set_prepare(mode);
				}

				/* Slide into legality */
				if (x >= cur_wid-1) x = cur_wid - 2;
				else if (x <= 0) x = 1;

				/* Slide into legality */
				if (y >= cur_hgt-1) y = cur_hgt- 2;
				else if (y <= 0) y = 1;
			}
		}
	}

	/* Forget */
	temp_n = 0;

	/* Clear the top line */
	prt("", 0, 0);

	/* Recenter the map around the player */
	verify_panel();

	/* Update stuff */
	p_ptr->update |= (PU_MONSTERS);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);

	/* Handle stuff */
	handle_stuff();

	/* Failure to set target */
	if (!target_who) return (FALSE);

	/* Success */
	return (TRUE);
}


/*
 * Get an "aiming direction" from the user.
 *
 * The "dir" is loaded with 1,2,3,4,6,7,8,9 for "actual direction", and
 * "0" for "current target", and "-1" for "entry aborted".
 *
 * Note that "Force Target", if set, will pre-empt user interaction,
 * if there is a usable target already set.
 *
 * Note that confusion over-rides any (explicit?) user choice.
 */
bool get_aim_dir(int *dp)
{
	int		dir;

	char	command;

	cptr	p;

	/* Initialize */
	(*dp) = 0;

	/* Global direction */
	dir = command_dir;

	/* Hack -- auto-target if requested */
	if (use_old_target && target_okay()) dir = 5;

#ifdef ALLOW_REPEAT /* TNB */

	if (repeat_pull(dp))
	{
		/* Confusion? */

		/* Verify */
		if (!(*dp == 5 && !target_okay()))
		{
/*			return (TRUE); */
			dir = *dp;
		}
	}

#endif /* ALLOW_REPEAT -- TNB */

	/* Ask until satisfied */
	while (!dir)
	{
		/* Choose a prompt */
		if (!target_okay())
		{
			p = _("方向 ('*'でターゲット選択, ESCで中断)? ", "Direction ('*' to choose a target, Escape to cancel)? ");
		}
		else
		{
			p = _("方向 ('5'でターゲットへ, '*'でターゲット再選択, ESCで中断)? ", "Direction ('5' for target, '*' to re-target, Escape to cancel)? ");
		}

		/* Get a command (or Cancel) */
		if (!get_com(p, &command, TRUE)) break;

		if (use_menu)
		{
			if (command == '\r') command = 't';
		}  

		/* Convert various keys to "standard" keys */
		switch (command)
		{
			/* Use current target */
			case 'T':
			case 't':
			case '.':
			case '5':
			case '0':
			{
				dir = 5;
				break;
			}

			/* Set new target */
			case '*':
			case ' ':
			case '\r':
			{
				if (target_set(TARGET_KILL)) dir = 5;
				break;
			}

			default:
			{
				/* Extract the action (if any) */
				dir = get_keymap_dir(command);

				break;
			}
		}

		/* Verify requested targets */
		if ((dir == 5) && !target_okay()) dir = 0;

		/* Error */
		if (!dir) bell();
	}

	/* No direction */
	if (!dir)
	{
		project_length = 0; /* reset to default */
		return (FALSE);
	}

	/* Save the direction */
	command_dir = dir;

	/* Check for confusion */
	if (p_ptr->confused)
	{
		/* XXX XXX XXX */
		/* Random direction */
		dir = ddd[randint0(8)];
	}

	/* Notice confusion */
	if (command_dir != dir)
	{
		/* Warn the user */
		msg_print(_("あなたは混乱している。", "You are confused."));
	}

	/* Save direction */
	(*dp) = dir;

#ifdef ALLOW_REPEAT /* TNB */

/*	repeat_push(dir); */
	repeat_push(command_dir);

#endif /* ALLOW_REPEAT -- TNB */

	/* A "valid" direction was entered */
	return (TRUE);
}



/*
 * Request a "movement" direction (1,2,3,4,6,7,8,9) from the user,
 * and place it into "command_dir", unless we already have one.
 *
 * This function should be used for all "repeatable" commands, such as
 * run, walk, open, close, bash, disarm, spike, tunnel, etc, as well
 * as all commands which must reference a grid adjacent to the player,
 * and which may not reference the grid under the player.  Note that,
 * for example, it is no longer possible to "disarm" or "open" chests
 * in the same grid as the player.
 *
 * Direction "5" is illegal and will (cleanly) abort the command.
 *
 * This function tracks and uses the "global direction", and uses
 * that as the "desired direction", to which "confusion" is applied.
 */
bool get_rep_dir(int *dp, bool under)
{
	int dir;
	cptr prompt;

	/* Initialize */
	(*dp) = 0;

	/* Global direction */
	dir = command_dir;

#ifdef ALLOW_REPEAT /* TNB */

	if (repeat_pull(dp))
	{
		dir = *dp;
/*		return (TRUE); */
	}

#endif /* ALLOW_REPEAT -- TNB */

	if (under)
	{
		prompt = _("方向 ('.'足元, ESCで中断)? ", "Direction ('.' at feet, Escape to cancel)? ");
	}
	else
	{
		prompt = _("方向 (ESCで中断)? ", "Direction (Escape to cancel)? ");
	}
	
	/* Get a direction */
	while (!dir)
	{
		char ch;

		/* Get a command (or Cancel) */
		if (!get_com(prompt, &ch, TRUE)) break;

		/* Look down */
		if ((under) && ((ch == '5') || (ch == '-') || (ch == '.')))
		{
			dir = 5;
		}
		else
		{
			/* Look up the direction */
			dir = get_keymap_dir(ch);

			/* Oops */
			if (!dir) bell();
		}
	}

	/* Prevent weirdness */
	if ((dir == 5) && (!under)) dir = 0;

	/* Aborted */
	if (!dir) return (FALSE);

	/* Save desired direction */
	command_dir = dir;

	/* Apply "confusion" */
	if (p_ptr->confused)
	{
		/* Standard confusion */
		if (randint0(100) < 75)
		{
			/* Random direction */
			dir = ddd[randint0(8)];
		}
	}
	else if (p_ptr->riding)
	{
		monster_type *m_ptr = &m_list[p_ptr->riding];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		if (MON_CONFUSED(m_ptr))
		{
			/* Standard confusion */
			if (randint0(100) < 75)
			{
				/* Random direction */
				dir = ddd[randint0(8)];
			}
		}
		else if ((r_ptr->flags1 & RF1_RAND_50) && (r_ptr->flags1 & RF1_RAND_25) && (randint0(100) < 50))
		{
			/* Random direction */
			dir = ddd[randint0(8)];
		}
		else if ((r_ptr->flags1 & RF1_RAND_50) && (randint0(100) < 25))
		{
			/* Random direction */
			dir = ddd[randint0(8)];
		}
	}

	/* Notice confusion */
	if (command_dir != dir)
	{
		if (p_ptr->confused)
		{
			/* Warn the user */
			msg_print(_("あなたは混乱している。", "You are confused."));
		}
		else
		{
			char m_name[80];
			monster_type *m_ptr = &m_list[p_ptr->riding];

			monster_desc(m_name, m_ptr, 0);
			if (MON_CONFUSED(m_ptr))
			{
				msg_format(_("%sは混乱している。", "%^s is confusing."), m_name);
			}
			else
			{
				msg_format(_("%sは思い通りに動いてくれない。", "You cannot control %s."), m_name);
			}
		}
	}

	/* Save direction */
	(*dp) = dir;

#ifdef ALLOW_REPEAT /* TNB */

/*	repeat_push(dir); */
	repeat_push(command_dir);

#endif /* ALLOW_REPEAT -- TNB */

	/* Success */
	return (TRUE);
}


bool get_rep_dir2(int *dp)
{
	int dir;

	/* Initialize */
	(*dp) = 0;

	/* Global direction */
	dir = command_dir;

#ifdef ALLOW_REPEAT /* TNB */

	if (repeat_pull(dp))
	{
		dir = *dp;
/*		return (TRUE); */
	}

#endif /* ALLOW_REPEAT -- TNB */

	/* Get a direction */
	while (!dir)
	{
		char ch;

		/* Get a command (or Cancel) */
		if (!get_com(_("方向 (ESCで中断)? ", "Direction (Escape to cancel)? "), &ch, TRUE)) break;

		/* Look up the direction */
		dir = get_keymap_dir(ch);

		/* Oops */
		if (!dir) bell();
	}

	/* Prevent weirdness */
	if (dir == 5) dir = 0;

	/* Aborted */
	if (!dir) return (FALSE);

	/* Save desired direction */
	command_dir = dir;

	/* Apply "confusion" */
	if (p_ptr->confused)
	{
		/* Standard confusion */
		if (randint0(100) < 75)
		{
			/* Random direction */
			dir = ddd[randint0(8)];
		}
	}

	/* Notice confusion */
	if (command_dir != dir)
	{
		/* Warn the user */
		msg_print(_("あなたは混乱している。", "You are confused."));
	}

	/* Save direction */
	(*dp) = dir;

#ifdef ALLOW_REPEAT /* TNB */

/*	repeat_push(dir); */
	repeat_push(command_dir);

#endif /* ALLOW_REPEAT -- TNB */

	/* Success */
	return (TRUE);
}

void gain_level_reward(int chosen_reward)
{
	object_type *q_ptr;
	object_type forge;
	char        wrath_reason[32] = "";
	int         nasty_chance = 6;
	int         dummy = 0, dummy2 = 0;
	int         type, effect;
	cptr        reward = NULL;
	char o_name[MAX_NLEN];

	int count = 0;

	if (!chosen_reward)
	{
		if (multi_rew) return;
		else multi_rew = TRUE;
	}


	if (p_ptr->lev == 13) nasty_chance = 2;
	else if (!(p_ptr->lev % 13)) nasty_chance = 3;
	else if (!(p_ptr->lev % 14)) nasty_chance = 12;

	if (one_in_(nasty_chance))
		type = randint1(20); /* Allow the 'nasty' effects */
	else
		type = randint1(15) + 5; /* Or disallow them */

	if (type < 1) type = 1;
	if (type > 20) type = 20;
	type--;


#ifdef JP
sprintf(wrath_reason, "%sの怒り",
		chaos_patrons[p_ptr->chaos_patron]);
#else
	sprintf(wrath_reason, "the Wrath of %s",
		chaos_patrons[p_ptr->chaos_patron]);
#endif


	effect = chaos_rewards[p_ptr->chaos_patron][type];

	if (one_in_(6) && !chosen_reward)
	{
#ifdef JP
msg_format("%^sは褒美としてあなたを突然変異させた。",
			chaos_patrons[p_ptr->chaos_patron]);
#else
		msg_format("%^s rewards you with a mutation!",
			chaos_patrons[p_ptr->chaos_patron]);
#endif

		(void)gain_random_mutation(0);
		reward = _("変異した。", "mutation");
	}
	else
	{
	switch (chosen_reward ? chosen_reward : effect)
	{
		case REW_POLY_SLF:
#ifdef JP
msg_format("%sの声が響き渡った:",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("The voice of %s booms out:",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			msg_print(_("「汝、新たなる姿を必要とせり！」", "'Thou needst a new form, mortal!'"));

			do_poly_self();
			reward = _("変異した。", "polymorphing");
			break;
		case REW_GAIN_EXP:
#ifdef JP
msg_format("%sの声が響き渡った:",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("The voice of %s booms out:",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			msg_print(_("「汝は良く行いたり！続けよ！」", "'Well done, mortal! Lead on!'"));

			if (p_ptr->prace == RACE_ANDROID)
			{
				msg_print(_("しかし何も起こらなかった。", "But, nothing happen."));
			}
			else if (p_ptr->exp < PY_MAX_EXP)
			{
				s32b ee = (p_ptr->exp / 2) + 10;
				if (ee > 100000L) ee = 100000L;
				msg_print(_("更に経験を積んだような気がする。", "You feel more experienced."));

				gain_exp(ee);
				reward = _("経験値を得た", "experience");
			}
			break;
		case REW_LOSE_EXP:
#ifdef JP
msg_format("%sの声が響き渡った:",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("The voice of %s booms out:",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			msg_print(_("「下僕よ、汝それに値せず。」", "'Thou didst not deserve that, slave.'"));

			if (p_ptr->prace == RACE_ANDROID)
			{
				msg_print(_("しかし何も起こらなかった。", "But, nothing happen."));
			}
			else
			{
				lose_exp(p_ptr->exp / 6);
				reward = _("経験値を失った。", "losing experience");
			}
			break;
		case REW_GOOD_OBJ:
#ifdef JP
msg_format("%sの声がささやいた:",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("The voice of %s whispers:",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			msg_print(_("「我が与えし物を賢明に使うべし。」", "'Use my gift wisely.'"));

			acquirement(py, px, 1, FALSE, FALSE, FALSE);
			reward = _("上質なアイテムを手に入れた。", "a good item");
			break;
		case REW_GREA_OBJ:
#ifdef JP
msg_format("%sの声が響き渡った:",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("The voice of %s booms out:",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			msg_print(_("「我が与えし物を賢明に使うべし。」", "'Use my gift wisely.'"));

			acquirement(py, px, 1, TRUE, FALSE, FALSE);
			reward = _("高級品のアイテムを手に入れた。", "an excellent item");
			break;
		case REW_CHAOS_WP:
#ifdef JP
msg_format("%sの声が響き渡った:",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("The voice of %s booms out:",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			msg_print(_("「汝の行いは貴き剣に値せり。」", "'Thy deed hath earned thee a worthy blade.'"));

			/* Get local object */
			q_ptr = &forge;
			dummy = TV_SWORD;
			switch (randint1(p_ptr->lev))
			{
				case 0: case 1:
					dummy2 = SV_DAGGER;
					break;
				case 2: case 3:
					dummy2 = SV_MAIN_GAUCHE;
					break;
				case 4:
					dummy2 = SV_TANTO;
					break;
				case 5: case 6:
					dummy2 = SV_RAPIER;
					break;
				case 7: case 8:
					dummy2 = SV_SMALL_SWORD;
					break;
				case 9: case 10:
					dummy2 = SV_BASILLARD;
					break;
				case 11: case 12: case 13:
					dummy2 = SV_SHORT_SWORD;
					break;
				case 14: case 15:
					dummy2 = SV_SABRE;
					break;
				case 16: case 17:
					dummy2 = SV_CUTLASS;
					break;
				case 18:
					dummy2 = SV_WAKIZASHI;
					break;
				case 19:
					dummy2 = SV_KHOPESH;
					break;
				case 20:
					dummy2 = SV_TULWAR;
					break;
				case 21:
					dummy2 = SV_BROAD_SWORD;
					break;
				case 22: case 23:
					dummy2 = SV_LONG_SWORD;
					break;
				case 24: case 25:
					dummy2 = SV_SCIMITAR;
					break;
				case 26:
					dummy2 = SV_NINJATO;
					break;
				case 27:
					dummy2 = SV_KATANA;
					break;
				case 28: case 29:
					dummy2 = SV_BASTARD_SWORD;
					break;
				case 30:
					dummy2 = SV_GREAT_SCIMITAR;
					break;
				case 31:
					dummy2 = SV_CLAYMORE;
					break;
				case 32:
					dummy2 = SV_ESPADON;
					break;
				case 33:
					dummy2 = SV_TWO_HANDED_SWORD;
					break;
				case 34:
					dummy2 = SV_FLAMBERGE;
					break;
				case 35:
					dummy2 = SV_NO_DACHI;
					break;
				case 36:
					dummy2 = SV_EXECUTIONERS_SWORD;
					break;
				case 37:
					dummy2 = SV_ZWEIHANDER;
					break;
				case 38:
					dummy2 = SV_HAYABUSA;
					break;
				default:
					dummy2 = SV_BLADE_OF_CHAOS;
			}

			object_prep(q_ptr, lookup_kind(dummy, dummy2));
			q_ptr->to_h = 3 + randint1(dun_level) % 10;
			q_ptr->to_d = 3 + randint1(dun_level) % 10;
			one_resistance(q_ptr);
			q_ptr->name2 = EGO_CHAOTIC;

			/* Drop it in the dungeon */
			(void)drop_near(q_ptr, -1, py, px);
			reward = _("(混沌)の武器を手に入れた。", "chaos weapon");
			break;
		case REW_GOOD_OBS:
#ifdef JP
msg_format("%sの声が響き渡った:",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("The voice of %s booms out:",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			msg_print(_("「汝の行いは貴き報いに値せり。」", "'Thy deed hath earned thee a worthy reward.'"));

			acquirement(py, px, randint1(2) + 1, FALSE, FALSE, FALSE);
			reward = _("上質なアイテムを手に入れた。", "good items");
			break;
		case REW_GREA_OBS:
#ifdef JP
msg_format("%sの声が響き渡った:",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("The voice of %s booms out:",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			msg_print(_("「下僕よ、汝の献身への我が惜しみ無き報いを見るがよい。」", "'Behold, mortal, how generously I reward thy loyalty.'"));

			acquirement(py, px, randint1(2) + 1, TRUE, FALSE, FALSE);
			reward = _("高級品のアイテムを手に入れた。", "excellent items");
			break;
		case REW_TY_CURSE:
#ifdef JP
msg_format("%sの声が轟き渡った:",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("The voice of %s thunders:",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			msg_print(_("「下僕よ、汝傲慢なり。」", "'Thou art growing arrogant, mortal.'"));

			(void)activate_ty_curse(FALSE, &count);
			reward = _("禍々しい呪いをかけられた。", "cursing");
			break;
		case REW_SUMMON_M:
#ifdef JP
msg_format("%sの声が響き渡った:",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("The voice of %s booms out:",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			msg_print(_("「我が下僕たちよ、かの傲慢なる者を倒すべし！」", "'My pets, destroy the arrogant mortal!'"));

			for (dummy = 0; dummy < randint1(5) + 1; dummy++)
			{
				(void)summon_specific(0, py, px, dun_level, 0, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET));
			}
			reward = _("モンスターを召喚された。", "summoning hostile monsters");
			break;
		case REW_H_SUMMON:
#ifdef JP
msg_format("%sの声が響き渡った:",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("The voice of %s booms out:",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			msg_print(_("「汝、より強き敵を必要とせり！」", "'Thou needst worthier opponents!'"));

			activate_hi_summon(py, px, FALSE);
			reward = _("モンスターを召喚された。", "summoning many hostile monsters");
			break;
		case REW_DO_HAVOC:
#ifdef JP
msg_format("%sの声が響き渡った:",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("The voice of %s booms out:",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			msg_print(_("「死と破壊こそ我が喜びなり！」", "'Death and destruction! This pleaseth me!'"));

			call_chaos();
			reward = _("カオスの力が渦巻いた。", "calling chaos");
			break;
		case REW_GAIN_ABL:
#ifdef JP
msg_format("%sの声が鳴り響いた:",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("The voice of %s rings out:",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			msg_print(_("「留まるのだ、下僕よ。余が汝の肉体を鍛えん。」", "'Stay, mortal, and let me mold thee.'"));

			if (one_in_(3) && !(chaos_stats[p_ptr->chaos_patron] < 0))
				do_inc_stat(chaos_stats[p_ptr->chaos_patron]);
			else
				do_inc_stat(randint0(6));
			reward = _("能力値が上がった。", "increasing a stat");
			break;
		case REW_LOSE_ABL:
#ifdef JP
msg_format("%sの声が響き渡った:",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("The voice of %s booms out:",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			msg_print(_("「下僕よ、余は汝に飽みたり。」", "'I grow tired of thee, mortal.'"));

			if (one_in_(3) && !(chaos_stats[p_ptr->chaos_patron] < 0))
				do_dec_stat(chaos_stats[p_ptr->chaos_patron]);
			else
				(void)do_dec_stat(randint0(6));
			reward = _("能力値が下がった。", "decreasing a stat");
			break;
		case REW_RUIN_ABL:
#ifdef JP
msg_format("%sの声が轟き渡った:",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("The voice of %s thunders:",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			msg_print(_("「汝、謙虚たることを学ぶべし！」", "'Thou needst a lesson in humility, mortal!'"));
			msg_print(_("あなたは以前より弱くなった！", "You feel less powerful!"));

			for (dummy = 0; dummy < 6; dummy++)
			{
				(void)dec_stat(dummy, 10 + randint1(15), TRUE);
			}
			reward = _("全能力値が下がった。", "decreasing all stats");
			break;
		case REW_POLY_WND:
			msg_format(_("%sの力が触れるのを感じた。", "You feel the power of %s touch you."),

				chaos_patrons[p_ptr->chaos_patron]);
			do_poly_wounds();
			reward = _("傷が変化した。", "polymorphing wounds");
			break;
		case REW_AUGM_ABL:
#ifdef JP
msg_format("%sの声が響き渡った:",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("The voice of %s booms out:",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			msg_print(_("「我がささやかなる賜物を受けとるがよい！」", "'Receive this modest gift from me!'"));

			for (dummy = 0; dummy < 6; dummy++)
			{
				(void)do_inc_stat(dummy);
			}
			reward = _("全能力値が上がった。", "increasing all stats");
			break;
		case REW_HURT_LOT:
#ifdef JP
msg_format("%sの声が響き渡った:",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("The voice of %s booms out:",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			msg_print(_("「苦しむがよい、無能な愚か者よ！」", "'Suffer, pathetic fool!'"));

			fire_ball(GF_DISINTEGRATE, 0, p_ptr->lev * 4, 4);
			take_hit(DAMAGE_NOESCAPE, p_ptr->lev * 4, wrath_reason, -1);
			reward = _("分解の球が発生した。", "generating disintegration ball");
			break;
	   case REW_HEAL_FUL:
#ifdef JP
msg_format("%sの声が響き渡った:",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("The voice of %s booms out:",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			msg_print(_("「甦るがよい、我が下僕よ！」", "'Rise, my servant!'"));

			restore_level();
			(void)set_poisoned(0);
			(void)set_blind(0);
			(void)set_confused(0);
			(void)set_image(0);
			(void)set_stun(0);
			(void)set_cut(0);
			hp_player(5000);
			for (dummy = 0; dummy < 6; dummy++)
			{
				(void)do_res_stat(dummy);
			}
			reward = _("体力が回復した。", "healing");
			break;
		case REW_CURSE_WP:
			if (!buki_motteruka(INVEN_RARM) && !buki_motteruka(INVEN_LARM)) break;
#ifdef JP
msg_format("%sの声が響き渡った:",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("The voice of %s booms out:",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			msg_print(_("「汝、武器に頼ることなかれ。」", "'Thou reliest too much on thy weapon.'"));

			dummy = INVEN_RARM;
			if (buki_motteruka(INVEN_LARM))
			{
				dummy = INVEN_LARM;
				if (buki_motteruka(INVEN_RARM) && one_in_(2)) dummy = INVEN_RARM;
			}
			object_desc(o_name, &inventory[dummy], OD_NAME_ONLY);
			(void)curse_weapon(FALSE, dummy);
			reward = format(_("%sが破壊された。", "destroying %s"), o_name);
			break;
		case REW_CURSE_AR:
			if (!inventory[INVEN_BODY].k_idx) break;
#ifdef JP
msg_format("%sの声が響き渡った:",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("The voice of %s booms out:",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			msg_print(_("「汝、防具に頼ることなかれ。」", "'Thou reliest too much on thine equipment.'"));

			object_desc(o_name, &inventory[INVEN_BODY], OD_NAME_ONLY);
			(void)curse_armor();
			reward = format(_("%sが破壊された。", "destroying %s"), o_name);
			break;
		case REW_PISS_OFF:
#ifdef JP
msg_format("%sの声がささやいた:",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("The voice of %s whispers:",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			msg_print(_("「我を怒りしめた罪を償うべし。」", "'Now thou shalt pay for annoying me.'"));

			switch (randint1(4))
			{
				case 1:
					(void)activate_ty_curse(FALSE, &count);
					reward = _("禍々しい呪いをかけられた。", "cursing");
					break;
				case 2:
					activate_hi_summon(py, px, FALSE);
					reward = _("モンスターを召喚された。", "summoning hostile monsters");
					break;
				case 3:
					if (one_in_(2))
					{
						if (!buki_motteruka(INVEN_RARM) && !buki_motteruka(INVEN_LARM)) break;
						dummy = INVEN_RARM;
						if (buki_motteruka(INVEN_LARM))
						{
							dummy = INVEN_LARM;
							if (buki_motteruka(INVEN_RARM) && one_in_(2)) dummy = INVEN_RARM;
						}
						object_desc(o_name, &inventory[dummy], OD_NAME_ONLY);
						(void)curse_weapon(FALSE, dummy);
						reward = format(_("%sが破壊された。", "destroying %s"), o_name);
					}
					else
					{
						if (!inventory[INVEN_BODY].k_idx) break;
						object_desc(o_name, &inventory[INVEN_BODY], OD_NAME_ONLY);
						(void)curse_armor();
						reward = format(_("%sが破壊された。", "destroying %s"), o_name);
					}
					break;
				default:
					for (dummy = 0; dummy < 6; dummy++)
					{
						(void)dec_stat(dummy, 10 + randint1(15), TRUE);
					}
					reward = _("全能力値が下がった。", "decreasing all stats");
					break;
			}
			break;
		case REW_WRATH:
			msg_format(_("%sの声が轟き渡った:", "The voice of %s thunders:"),

				chaos_patrons[p_ptr->chaos_patron]);
			msg_print(_("「死ぬがよい、下僕よ！」", "'Die, mortal!'"));

			take_hit(DAMAGE_LOSELIFE, p_ptr->lev * 4, wrath_reason, -1);
			for (dummy = 0; dummy < 6; dummy++)
			{
				(void)dec_stat(dummy, 10 + randint1(15), FALSE);
			}
			activate_hi_summon(py, px, FALSE);
			(void)activate_ty_curse(FALSE, &count);
			if (one_in_(2))
			{
				dummy = 0;

				if (buki_motteruka(INVEN_RARM))
				{
					dummy = INVEN_RARM;
					if (buki_motteruka(INVEN_LARM) && one_in_(2)) dummy = INVEN_LARM;
				}
				else if (buki_motteruka(INVEN_LARM)) dummy = INVEN_LARM;

				if (dummy) (void)curse_weapon(FALSE, dummy);
			}
			if (one_in_(2)) (void)curse_armor();
			break;
		case REW_DESTRUCT:
#ifdef JP
msg_format("%sの声が響き渡った:",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("The voice of %s booms out:",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			msg_print(_("「死と破壊こそ我が喜びなり！」", "'Death and destruction! This pleaseth me!'"));

			(void)destroy_area(py, px, 25, FALSE);
			reward = _("ダンジョンが*破壊*された。", "*destruct*ing dungeon");
			break;
		case REW_GENOCIDE:
#ifdef JP
msg_format("%sの声が響き渡った:",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("The voice of %s booms out:",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			msg_print(_("「我、汝の敵を抹殺せん！」", "'Let me relieve thee of thine oppressors!'"));

			(void)symbol_genocide(0, FALSE);
			reward = _("モンスターが抹殺された。", "genociding monsters");
			break;
		case REW_MASS_GEN:
#ifdef JP
msg_format("%sの声が響き渡った:",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("The voice of %s booms out:",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			msg_print(_("「我、汝の敵を抹殺せん！」", "'Let me relieve thee of thine oppressors!'"));

			(void)mass_genocide(0, FALSE);
			reward = _("モンスターが抹殺された。", "genociding nearby monsters");
			break;
		case REW_DISPEL_C:
#ifdef JP
msg_format("%sの力が敵を攻撃するのを感じた！",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("You can feel the power of %s assault your enemies!",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			(void)dispel_monsters(p_ptr->lev * 4);
			break;
		case REW_IGNORE:
#ifdef JP
msg_format("%sはあなたを無視した。",
				chaos_patrons[p_ptr->chaos_patron]);
#else
			msg_format("%s ignores you.",
				chaos_patrons[p_ptr->chaos_patron]);
#endif

			break;
		case REW_SER_DEMO:
			msg_format(_("%sは褒美として悪魔の使いをよこした！", "%s rewards you with a demonic servant!"),chaos_patrons[p_ptr->chaos_patron]);

			if (!summon_specific(-1, py, px, dun_level, SUMMON_DEMON, PM_FORCE_PET))
				msg_print(_("何も現れなかった...", "Nobody ever turns up..."));
			else
				reward = _("悪魔がペットになった。", "a demonic servant");

			break;
		case REW_SER_MONS:
			msg_format(_("%sは褒美として使いをよこした！", "%s rewards you with a servant!"),chaos_patrons[p_ptr->chaos_patron]);

			if (!summon_specific(-1, py, px, dun_level, 0, PM_FORCE_PET))
				msg_print(_("何も現れなかった...", "Nobody ever turns up..."));
			else
				reward = _("モンスターがペットになった。", "a servant");

			break;
		case REW_SER_UNDE:
			msg_format(_("%sは褒美としてアンデッドの使いをよこした。", "%s rewards you with an undead servant!"),chaos_patrons[p_ptr->chaos_patron]);

			if (!summon_specific(-1, py, px, dun_level, SUMMON_UNDEAD, PM_FORCE_PET))
				msg_print(_("何も現れなかった...", "Nobody ever turns up..."));
			else
				reward = _("アンデッドがペットになった。", "an undead servant");

			break;
		default:
			msg_format(_("%sの声がどもった:", "The voice of %s stammers:"),

				chaos_patrons[p_ptr->chaos_patron]);
			msg_format(_("「あー、あー、答えは %d/%d。質問は何？」", "'Uh... uh... the answer's %d/%d, what's the question?'"), type, effect);

	}
	}
	if (reward)
	{
		do_cmd_write_nikki(NIKKI_BUNSHOU, 0, format(_("パトロンの報酬で%s", "The patron rewards you with %s."), reward));
	}
}


/*
 * XAngband: determine if a given location is "interesting"
 * based on target_set_accept function.
 */
static bool tgt_pt_accept(int y, int x)
{
	cave_type *c_ptr;

	/* Bounds */
	if (!(in_bounds(y, x))) return (FALSE);

	/* Player grid is always interesting */
	if ((y == py) && (x == px)) return (TRUE);

	/* Handle hallucination */
	if (p_ptr->image) return (FALSE);

	/* Examine the grid */
	c_ptr = &cave[y][x];

	/* Interesting memorized features */
	if (c_ptr->info & (CAVE_MARK))
	{
		/* Notice stairs */
		if (cave_have_flag_grid(c_ptr, FF_LESS)) return (TRUE);
		if (cave_have_flag_grid(c_ptr, FF_MORE)) return (TRUE);

		/* Notice quest features */
		if (cave_have_flag_grid(c_ptr, FF_QUEST_ENTER)) return (TRUE);
		if (cave_have_flag_grid(c_ptr, FF_QUEST_EXIT)) return (TRUE);
	}

	/* Nope */
	return (FALSE);
}


/*
 * XAngband: Prepare the "temp" array for "tget_pt"
 * based on target_set_prepare funciton.
 */
static void tgt_pt_prepare(void)
{
	int y, x;

	/* Reset "temp" array */
	temp_n = 0;

	if (!expand_list) return;

	/* Scan the current panel */
	for (y = 1; y < cur_hgt; y++)
	{
		for (x = 1; x < cur_wid; x++)
		{
			/* Require "interesting" contents */
			if (!tgt_pt_accept(y, x)) continue;

			/* Save the location */
			temp_x[temp_n] = x;
			temp_y[temp_n] = y;
			temp_n++;
		}
	}

	/* Target the nearest monster for shooting */
	ang_sort_comp = ang_sort_comp_distance;
	ang_sort_swap = ang_sort_swap_distance;

	/* Sort the positions */
	ang_sort(temp_x, temp_y, temp_n);
}

/*
 * old -- from PsiAngband.
 */
bool tgt_pt(int *x_ptr, int *y_ptr)
{
	char ch = 0;
	int d, x, y, n;
	bool success = FALSE;

	int wid, hgt;

	/* Get size */
	get_screen_size(&wid, &hgt);

	x = px;
	y = py;

	if (expand_list) 
	{
		tgt_pt_prepare();
		n = 0;
	}

	msg_print(_("場所を選んでスペースキーを押して下さい。", "Select a point and press space."));
	msg_flag = FALSE; /* prevents "-more-" message. */

	while ((ch != ESCAPE) && !success)
	{
		bool move_fast = FALSE;

		move_cursor_relative(y, x);
		ch = inkey();
		switch (ch)
		{
		case ESCAPE:
			break;
		case ' ':
		case 't':
		case '.':
		case '5':
		case '0':
			/* illegal place */
			if (player_bold(y, x)) ch = 0;

			/* okay place */
			else success = TRUE;

			break;

		/* XAngband: Move cursor to stairs */
		case '>':
		case '<':
			if (expand_list && temp_n)
			{
				int dx, dy;
				int cx = (panel_col_min + panel_col_max) / 2;
				int cy = (panel_row_min + panel_row_max) / 2;

				n++;

				/* Skip stairs which have defferent distance */
				for (; n < temp_n; ++ n)
				{
					cave_type *c_ptr = &cave[temp_y[n]][temp_x[n]];

					if (cave_have_flag_grid(c_ptr, FF_STAIRS) &&
					    cave_have_flag_grid(c_ptr, ch == '>' ? FF_MORE : FF_LESS))
					{
						/* Found */
						break;
					}
				}

				if (n == temp_n)	/* Loop out taget list */
				{
					n = 0;
					y = py;
					x = px;
					verify_panel();	/* Move cursor to player */

					/* Update stuff */
					p_ptr->update |= (PU_MONSTERS);

					/* Redraw map */
					p_ptr->redraw |= (PR_MAP);

					/* Window stuff */
					p_ptr->window |= (PW_OVERHEAD);

					/* Handle stuff */
					handle_stuff();
				}
				else	/* move cursor to next stair and change panel */
				{
					y = temp_y[n];
					x = temp_x[n];

					dy = 2 * (y - cy) / hgt;
					dx = 2 * (x - cx) / wid;
					if (dy || dx) change_panel(dy, dx);
				}
			}
			break;

		default:
			/* Look up the direction */
			d = get_keymap_dir(ch);

			/* XTRA HACK MOVEFAST */
			if (isupper(ch)) move_fast = TRUE;

			/* Handle "direction" */
			if (d)
			{
				int dx = ddx[d];
				int dy = ddy[d];

				/* XTRA HACK MOVEFAST */
				if (move_fast)
				{
					int mag = MIN(wid / 2, hgt / 2);
					x += dx * mag;
					y += dy * mag;
				}
				else
				{
					x += dx;
					y += dy;
				}

				/* Do not move horizontally if unnecessary */
				if (((x < panel_col_min + wid / 2) && (dx > 0)) ||
					 ((x > panel_col_min + wid / 2) && (dx < 0)))
				{
					dx = 0;
				}

				/* Do not move vertically if unnecessary */
				if (((y < panel_row_min + hgt / 2) && (dy > 0)) ||
					 ((y > panel_row_min + hgt / 2) && (dy < 0)))
				{
					dy = 0;
				}

				/* Apply the motion */
				if ((y >= panel_row_min + hgt) || (y < panel_row_min) ||
					 (x >= panel_col_min + wid) || (x < panel_col_min))
				{
					/* if (change_panel(dy, dx)) target_set_prepare(mode); */
					change_panel(dy, dx);
				}

				/* Slide into legality */
				if (x >= cur_wid-1) x = cur_wid - 2;
				else if (x <= 0) x = 1;

				/* Slide into legality */
				if (y >= cur_hgt-1) y = cur_hgt- 2;
				else if (y <= 0) y = 1;

			}
			break;
		}
	}

	/* Clear the top line */
	prt("", 0, 0);

	/* Recenter the map around the player */
	verify_panel();

	/* Update stuff */
	p_ptr->update |= (PU_MONSTERS);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD);

	/* Handle stuff */
	handle_stuff();

	*x_ptr = x;
	*y_ptr = y;
	return success;
}


bool get_hack_dir(int *dp)
{
	int		dir;
	cptr    p;
	char    command;


	/* Initialize */
	(*dp) = 0;

	/* Global direction */
	dir = 0;

	/* (No auto-targeting) */

	/* Ask until satisfied */
	while (!dir)
	{
		/* Choose a prompt */
		if (!target_okay())
		{
			p = _("方向 ('*'でターゲット選択, ESCで中断)? ", "Direction ('*' to choose a target, Escape to cancel)? ");
		}
		else
		{
			p = _("方向 ('5'でターゲットへ, '*'でターゲット再選択, ESCで中断)? ", "Direction ('5' for target, '*' to re-target, Escape to cancel)? ");
		}

		/* Get a command (or Cancel) */
		if (!get_com(p, &command, TRUE)) break;

		if (use_menu)
		{
			if (command == '\r') command = 't';
		}  

		/* Convert various keys to "standard" keys */
		switch (command)
		{
			/* Use current target */
			case 'T':
			case 't':
			case '.':
			case '5':
			case '0':
			{
				dir = 5;
				break;
			}

			/* Set new target */
			case '*':
			case ' ':
			case '\r':
			{
				if (target_set(TARGET_KILL)) dir = 5;
				break;
			}

			default:
			{
				/* Look up the direction */
				dir = get_keymap_dir(command);

				break;
			}
		}

		/* Verify requested targets */
		if ((dir == 5) && !target_okay()) dir = 0;

		/* Error */
		if (!dir) bell();
	}

	/* No direction */
	if (!dir) return (FALSE);

	/* Save the direction */
	command_dir = dir;

	/* Check for confusion */
	if (p_ptr->confused)
	{
		/* XXX XXX XXX */
		/* Random direction */
		dir = ddd[randint0(8)];
	}

	/* Notice confusion */
	if (command_dir != dir)
	{
		/* Warn the user */
		msg_print(_("あなたは混乱している。", "You are confused."));
	}

	/* Save direction */
	(*dp) = dir;

	/* A "valid" direction was entered */
	return (TRUE);
}


/*
 * エネルギーの増加量10d5を速く計算するための関数
 */

#define Go_no_JuuJou 5*5*5*5*5*5*5*5*5*5

s16b gain_energy(void)
{
	int i;
	s32b energy_result = 10;
	s32b tmp;

	tmp = randint0(Go_no_JuuJou);

	for (i = 0; i < 9; i ++){
		energy_result += tmp % 5;
		tmp /= 5;
	}

	return (s16b)(energy_result + tmp);
}


/*
 * Return bow energy 
 */
s16b bow_energy(int sval)
{
	int energy = 100;

	/* Analyze the launcher */
	switch (sval)
	{
		/* Sling and ammo */
		case SV_SLING:
		{
			energy = 8000;
			break;
		}

		/* Short Bow and Arrow */
		case SV_SHORT_BOW:
		{
			energy = 10000;
			break;
		}

		/* Long Bow and Arrow */
		case SV_LONG_BOW:
		{
			energy = 10000;
			break;
		}

		/* Bow of irresponsiblity and Arrow */
		case SV_NAMAKE_BOW:
		{
			energy = 7777;
			break;
		}

		/* Light Crossbow and Bolt */
		case SV_LIGHT_XBOW:
		{
			energy = 12000;
			break;
		}

		/* Heavy Crossbow and Bolt */
		case SV_HEAVY_XBOW:
		{
			energy = 13333;
			break;
		}
	}

	return (energy);
}


/*
 * Return bow tmul
 */
int bow_tmul(int sval)
{
	int tmul = 0;

	/* Analyze the launcher */
	switch (sval)
	{
		/* Sling and ammo */
		case SV_SLING:
		{
			tmul = 2;
			break;
		}

		/* Short Bow and Arrow */
		case SV_SHORT_BOW:
		{
			tmul = 2;
			break;
		}

		/* Long Bow and Arrow */
		case SV_LONG_BOW:
		{
			tmul = 3;
			break;
		}

		/* Bow of irresponsiblity and Arrow */
		case SV_NAMAKE_BOW:
		{
			tmul = 3;
			break;
		}

		/* Light Crossbow and Bolt */
		case SV_LIGHT_XBOW:
		{
			tmul = 3;
			break;
		}

		/* Heavy Crossbow and Bolt */
		case SV_HEAVY_XBOW:
		{
			tmul = 4;
			break;
		}
	}

	return (tmul);
}

/*
 * Return alignment title
 */
cptr your_alignment(void)
{
#ifdef JP
	if (p_ptr->align > 150) return "大善";
	else if (p_ptr->align > 50) return "中善";
	else if (p_ptr->align > 10) return "小善";
	else if (p_ptr->align > -11) return "中立";
	else if (p_ptr->align > -51) return "小悪";
	else if (p_ptr->align > -151) return "中悪";
	else return "大悪";
#else
	if (p_ptr->align > 150) return "Lawful";
	else if (p_ptr->align > 50) return "Good";
	else if (p_ptr->align > 10) return "Neutral Good";
	else if (p_ptr->align > -11) return "Neutral";
	else if (p_ptr->align > -51) return "Neutral Evil";
	else if (p_ptr->align > -151) return "Evil";
	else return "Chaotic";
#endif
}


/*
 * Return proficiency level of weapons and misc. skills (except riding)
 */
int weapon_exp_level(int weapon_exp)
{
	if (weapon_exp < WEAPON_EXP_BEGINNER) return EXP_LEVEL_UNSKILLED;
	else if (weapon_exp < WEAPON_EXP_SKILLED) return EXP_LEVEL_BEGINNER;
	else if (weapon_exp < WEAPON_EXP_EXPERT) return EXP_LEVEL_SKILLED;
	else if (weapon_exp < WEAPON_EXP_MASTER) return EXP_LEVEL_EXPERT;
	else return EXP_LEVEL_MASTER;
}


/*
 * Return proficiency level of riding
 */
int riding_exp_level(int riding_exp)
{
	if (riding_exp < RIDING_EXP_BEGINNER) return EXP_LEVEL_UNSKILLED;
	else if (riding_exp < RIDING_EXP_SKILLED) return EXP_LEVEL_BEGINNER;
	else if (riding_exp < RIDING_EXP_EXPERT) return EXP_LEVEL_SKILLED;
	else if (riding_exp < RIDING_EXP_MASTER) return EXP_LEVEL_EXPERT;
	else return EXP_LEVEL_MASTER;
}


/*
 * Return proficiency level of spells
 */
int spell_exp_level(int spell_exp)
{
	if (spell_exp < SPELL_EXP_BEGINNER) return EXP_LEVEL_UNSKILLED;
	else if (spell_exp < SPELL_EXP_SKILLED) return EXP_LEVEL_BEGINNER;
	else if (spell_exp < SPELL_EXP_EXPERT) return EXP_LEVEL_SKILLED;
	else if (spell_exp < SPELL_EXP_MASTER) return EXP_LEVEL_EXPERT;
	else return EXP_LEVEL_MASTER;
}


/*
 * Display a rumor and apply its effects
 */

int rumor_num(char *zz, int max_idx)
{
	if (strcmp(zz, "*") == 0) return randint1(max_idx - 1);
	return atoi(zz);
}

cptr rumor_bind_name(char *base, cptr fullname)
{
	char *s, *v;

	s = strstr(base, "{Name}");
	if (s)
	{
		s[0] = '\0';
		v = format("%s%s%s", base, fullname, (s + 6));
	}
	else
	{
		v = base;
	}

	return v;
}

void display_rumor(bool ex)
{
	bool err;
	int section = 0;
	char Rumor[1024];

	if (ex)
	{
		if (randint0(3) == 0) section = 1;
	}

	err = _(get_rnd_line_jonly("rumors_j.txt", section, Rumor, 10),
			get_rnd_line("rumors.txt", section, Rumor));
	if (err) strcpy(Rumor, _("嘘の噂もある。", "Some rumors are wrong."));

	err = TRUE;

	if (strncmp(Rumor, "R:", 2) == 0)
	{
		char *zz[4];
		cptr rumor_msg = NULL;
		cptr rumor_eff_format = NULL;
		char fullname[1024] = "";

		if (tokenize(Rumor + 2, 3, zz, TOKENIZE_CHECKQUOTE) == 3)
		{
			if (strcmp(zz[0], "ARTIFACT") == 0)
			{
				int a_idx, k_idx;
				object_type forge;
				object_type *q_ptr = &forge;
				artifact_type *a_ptr;

				while (1)
				{
					a_idx = rumor_num(zz[1], max_a_idx);

					a_ptr = &a_info[a_idx];
					if (a_ptr->name) break;
				}

				k_idx = lookup_kind(a_ptr->tval, a_ptr->sval);
				object_prep(q_ptr, k_idx);
				q_ptr->name1 = a_idx;
				q_ptr->ident = IDENT_STORE;
				object_desc(fullname, q_ptr, OD_NAME_ONLY);
			}
			else if  (strcmp(zz[0], "MONSTER") == 0)
			{
				int r_idx;
				monster_race *r_ptr;

				while(1)
				{
					r_idx = rumor_num(zz[1], max_r_idx);
					r_ptr = &r_info[r_idx];
					if (r_ptr->name) break;
				}

				strcpy(fullname, r_name + r_ptr->name);

				/* Remember this monster */
				if (!r_ptr->r_sights)
				{
					r_ptr->r_sights++;
				}
			}
			else if  (strcmp(zz[0], "DUNGEON") == 0)
			{
				int d_idx;
				dungeon_info_type *d_ptr;

				while (1)
				{
					d_idx = rumor_num(zz[1], max_d_idx);
					d_ptr = &d_info[d_idx];
					if (d_ptr->name) break;
				}

				strcpy(fullname, d_name + d_ptr->name);

				if (!max_dlv[d_idx])
				{
					max_dlv[d_idx] = d_ptr->mindepth;
					rumor_eff_format = _("%sに帰還できるようになった。", "You can recall to %s.");
				}
			}
			else if  (strcmp(zz[0], "TOWN") == 0)
			{
				int t_idx;
				s32b visit;

				while(1)
				{
					t_idx = rumor_num(zz[1], NO_TOWN);
					if (town[t_idx].name) break;
				}

				strcpy(fullname, town[t_idx].name);

				visit = (1L << (t_idx - 1));
				if ((t_idx != SECRET_TOWN) && !(p_ptr->visit & visit))
				{
					p_ptr->visit |= visit;
					rumor_eff_format = _("%sに行ったことがある気がする。", "You feel you have been to %s.");
				}
			}

			rumor_msg = rumor_bind_name(zz[2], fullname);
			msg_print(rumor_msg);
			if (rumor_eff_format)
			{
				msg_print(NULL);
				msg_format(rumor_eff_format, fullname);
			}
			err = FALSE;
		}
	/* error */
	if (err) msg_print(_("この情報は間違っている。", "This information is wrong."));
	}
			else
	{
		msg_format("%s", Rumor);
	}
}
