#include "angband.h"


/*!
* @brief モンスターの時限ステータスを取得する
* @return m_idx モンスターの参照ID
* @return mproc_type モンスターの時限ステータスID
* @return 残りターン値
*/
int get_mproc_idx(MONSTER_IDX m_idx, int mproc_type)
{
	s16b *cur_mproc_list = mproc_list[mproc_type];
	int i;

	for (i = mproc_max[mproc_type] - 1; i >= 0; i--)
	{
		if (cur_mproc_list[i] == m_idx) return i;
	}

	return -1;
}

/*!
* @brief モンスターの時限ステータスリストを追加する
* @return m_idx モンスターの参照ID
* @return mproc_type 追加したいモンスターの時限ステータスID
* @return なし
*/
static void mproc_add(MONSTER_IDX m_idx, int mproc_type)
{
	if (mproc_max[mproc_type] < max_m_idx) mproc_list[mproc_type][mproc_max[mproc_type]++] = (s16b)m_idx;
}


/*!
* @brief モンスターの時限ステータスリストを削除
* @return m_idx モンスターの参照ID
* @return mproc_type 削除したいモンスターの時限ステータスID
* @return なし
*/
static void mproc_remove(MONSTER_IDX m_idx, int mproc_type)
{
	int mproc_idx = get_mproc_idx(m_idx, mproc_type);
	if (mproc_idx >= 0) mproc_list[mproc_type][mproc_idx] = mproc_list[mproc_type][--mproc_max[mproc_type]];
}


/*!
* @brief モンスターの時限ステータスリストを初期化する / Initialize monster process
* @return なし
*/
void mproc_init(void)
{
	monster_type *m_ptr;
	MONSTER_IDX i;
	int cmi;

	/* Reset "mproc_max[]" */
	for (cmi = 0; cmi < MAX_MTIMED; cmi++) mproc_max[cmi] = 0;

	/* Process the monsters (backwards) */
	for (i = m_max - 1; i >= 1; i--)
	{
		/* Access the monster */
		m_ptr = &m_list[i];

		/* Ignore "dead" monsters */
		if (!m_ptr->r_idx) continue;

		for (cmi = 0; cmi < MAX_MTIMED; cmi++)
		{
			if (m_ptr->mtimed[cmi]) mproc_add(i, cmi);
		}
	}
}


/*!
* @brief モンスターの睡眠状態値をセットする /
* Set "m_ptr->mtimed[MTIMED_CSLEEP]", notice observable changes
* @param m_idx モンスター参照ID
* @param v セットする値
* @return 別途更新処理が必要な場合TRUEを返す
*/
bool set_monster_csleep(MONSTER_IDX m_idx, int v)
{
	monster_type *m_ptr = &m_list[m_idx];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!MON_CSLEEP(m_ptr))
		{
			mproc_add(m_idx, MTIMED_CSLEEP);
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (MON_CSLEEP(m_ptr))
		{
			mproc_remove(m_idx, MTIMED_CSLEEP);
			notice = TRUE;
		}
	}

	/* Use the value */
	m_ptr->mtimed[MTIMED_CSLEEP] = (s16b)v;

	if (!notice) return FALSE;

	if (m_ptr->ml)
	{
		/* Update health bar as needed */
		if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
		if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);
	}

	if (r_info[m_ptr->r_idx].flags7 & RF7_HAS_LD_MASK) p_ptr->update |= (PU_MON_LITE);

	return TRUE;
}


/*!
* @brief モンスターの加速状態値をセット /
* Set "m_ptr->mtimed[MTIMED_FAST]", notice observable changes
* @param m_idx モンスター参照ID
* @param v セットする値
* @return 別途更新処理が必要な場合TRUEを返す
*/
bool set_monster_fast(MONSTER_IDX m_idx, int v)
{
	monster_type *m_ptr = &m_list[m_idx];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 200) ? 200 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!MON_FAST(m_ptr))
		{
			mproc_add(m_idx, MTIMED_FAST);
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (MON_FAST(m_ptr))
		{
			mproc_remove(m_idx, MTIMED_FAST);
			notice = TRUE;
		}
	}

	/* Use the value */
	m_ptr->mtimed[MTIMED_FAST] = (s16b)v;

	if (!notice) return FALSE;

	if ((p_ptr->riding == m_idx) && !p_ptr->leaving) p_ptr->update |= (PU_BONUS);

	return TRUE;
}


/*
* Set "m_ptr->mtimed[MTIMED_SLOW]", notice observable changes
*/
bool set_monster_slow(MONSTER_IDX m_idx, int v)
{
	monster_type *m_ptr = &m_list[m_idx];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 200) ? 200 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!MON_SLOW(m_ptr))
		{
			mproc_add(m_idx, MTIMED_SLOW);
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (MON_SLOW(m_ptr))
		{
			mproc_remove(m_idx, MTIMED_SLOW);
			notice = TRUE;
		}
	}

	/* Use the value */
	m_ptr->mtimed[MTIMED_SLOW] = (s16b)v;

	if (!notice) return FALSE;

	if ((p_ptr->riding == m_idx) && !p_ptr->leaving) p_ptr->update |= (PU_BONUS);

	return TRUE;
}


/*!
* @brief モンスターの朦朧状態値をセット /
* Set "m_ptr->mtimed[MTIMED_STUNNED]", notice observable changes
* @param m_idx モンスター参照ID
* @param v セットする値
* @return 別途更新処理が必要な場合TRUEを返す
*/
bool set_monster_stunned(MONSTER_IDX m_idx, int v)
{
	monster_type *m_ptr = &m_list[m_idx];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 200) ? 200 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!MON_STUNNED(m_ptr))
		{
			mproc_add(m_idx, MTIMED_STUNNED);
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (MON_STUNNED(m_ptr))
		{
			mproc_remove(m_idx, MTIMED_STUNNED);
			notice = TRUE;
		}
	}

	/* Use the value */
	m_ptr->mtimed[MTIMED_STUNNED] = (s16b)v;

	return notice;
}


/*!
* @brief モンスターの混乱状態値をセット /
* Set "m_ptr->mtimed[MTIMED_CONFUSED]", notice observable changes
* @param m_idx モンスター参照ID
* @param v セットする値
* @return 別途更新処理が必要な場合TRUEを返す
*/
bool set_monster_confused(MONSTER_IDX m_idx, int v)
{
	monster_type *m_ptr = &m_list[m_idx];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 200) ? 200 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!MON_CONFUSED(m_ptr))
		{
			mproc_add(m_idx, MTIMED_CONFUSED);
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (MON_CONFUSED(m_ptr))
		{
			mproc_remove(m_idx, MTIMED_CONFUSED);
			notice = TRUE;
		}
	}

	/* Use the value */
	m_ptr->mtimed[MTIMED_CONFUSED] = (s16b)v;

	return notice;
}


/*!
* @brief モンスターの恐慌状態値をセット /
* Set "m_ptr->mtimed[MTIMED_MONFEAR]", notice observable changes
* @param m_idx モンスター参照ID
* @param v セットする値
* @return 別途更新処理が必要な場合TRUEを返す
*/
bool set_monster_monfear(MONSTER_IDX m_idx, int v)
{
	monster_type *m_ptr = &m_list[m_idx];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 200) ? 200 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!MON_MONFEAR(m_ptr))
		{
			mproc_add(m_idx, MTIMED_MONFEAR);
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (MON_MONFEAR(m_ptr))
		{
			mproc_remove(m_idx, MTIMED_MONFEAR);
			notice = TRUE;
		}
	}

	/* Use the value */
	m_ptr->mtimed[MTIMED_MONFEAR] = (s16b)v;

	if (!notice) return FALSE;

	if (m_ptr->ml)
	{
		/* Update health bar as needed */
		if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
		if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);
	}

	return TRUE;
}


/*!
* @brief モンスターの無敵状態値をセット /
* Set "m_ptr->mtimed[MTIMED_INVULNER]", notice observable changes
* @param m_idx モンスター参照ID
* @param v セットする値
* @param energy_need TRUEならば無敵解除時に行動ターン消費を行う
* @return 別途更新処理が必要な場合TRUEを返す
*/
bool set_monster_invulner(MONSTER_IDX m_idx, int v, bool energy_need)
{
	monster_type *m_ptr = &m_list[m_idx];
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 200) ? 200 : (v < 0) ? 0 : v;

	/* Open */
	if (v)
	{
		if (!MON_INVULNER(m_ptr))
		{
			mproc_add(m_idx, MTIMED_INVULNER);
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (MON_INVULNER(m_ptr))
		{
			mproc_remove(m_idx, MTIMED_INVULNER);
			if (energy_need && !p_ptr->wild_mode) m_ptr->energy_need += ENERGY_NEED();
			notice = TRUE;
		}
	}

	/* Use the value */
	m_ptr->mtimed[MTIMED_INVULNER] = (s16b)v;

	if (!notice) return FALSE;

	if (m_ptr->ml)
	{
		/* Update health bar as needed */
		if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
		if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);
	}

	return TRUE;
}


static u32b csleep_noise;

/*!
* @brief モンスターの各種状態値を時間経過により更新するサブルーチン
* @param m_idx モンスター参照ID
* @param mtimed_idx 更新するモンスターの時限ステータスID
* @return なし
*/
static void process_monsters_mtimed_aux(MONSTER_IDX m_idx, int mtimed_idx)
{
	monster_type *m_ptr = &m_list[m_idx];

	switch (mtimed_idx)
	{
	case MTIMED_CSLEEP:
	{
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Assume does not wake up */
		bool test = FALSE;

		/* Hack -- Require proximity */
		if (m_ptr->cdis < AAF_LIMIT)
		{
			/* Handle "sensing radius" */
			if (m_ptr->cdis <= (is_pet(m_ptr) ? ((r_ptr->aaf > MAX_SIGHT) ? MAX_SIGHT : r_ptr->aaf) : r_ptr->aaf))
			{
				/* We may wake up */
				test = TRUE;
			}

			/* Handle "sight" and "aggravation" */
			else if ((m_ptr->cdis <= MAX_SIGHT) && (player_has_los_bold(m_ptr->fy, m_ptr->fx)))
			{
				/* We may wake up */
				test = TRUE;
			}
		}

		if (test)
		{
			u32b notice = randint0(1024);

			/* Nightmare monsters are more alert */
			if (ironman_nightmare) notice /= 2;

			/* Hack -- See if monster "notices" player */
			if ((notice * notice * notice) <= csleep_noise)
			{
				/* Hack -- amount of "waking" */
				/* Wake up faster near the player */
				int d = (m_ptr->cdis < AAF_LIMIT / 2) ? (AAF_LIMIT / m_ptr->cdis) : 1;

				/* Hack -- amount of "waking" is affected by speed of player */
				d = (d * SPEED_TO_ENERGY(p_ptr->pspeed)) / 10;
				if (d < 0) d = 1;

				/* Monster wakes up "a little bit" */

				/* Still asleep */
				if (!set_monster_csleep(m_idx, MON_CSLEEP(m_ptr) - d))
				{
					/* Notice the "not waking up" */
					if (is_original_ap_and_seen(m_ptr))
					{
						/* Hack -- Count the ignores */
						if (r_ptr->r_ignore < MAX_UCHAR) r_ptr->r_ignore++;
					}
				}

				/* Just woke up */
				else
				{
					/* Notice the "waking up" */
					if (m_ptr->ml)
					{
						char m_name[80];

						/* Acquire the monster name */
						monster_desc(m_name, m_ptr, 0);

						/* Dump a message */
						msg_format(_("%^sが目を覚ました。", "%^s wakes up."), m_name);
					}

					if (is_original_ap_and_seen(m_ptr))
					{
						/* Hack -- Count the wakings */
						if (r_ptr->r_wake < MAX_UCHAR) r_ptr->r_wake++;
					}
				}
			}
		}
		break;
	}

	case MTIMED_FAST:
		/* Reduce by one, note if expires */
		if (set_monster_fast(m_idx, MON_FAST(m_ptr) - 1))
		{
			if (is_seen(m_ptr))
			{
				char m_name[80];

				/* Acquire the monster name */
				monster_desc(m_name, m_ptr, 0);

				/* Dump a message */
				msg_format(_("%^sはもう加速されていない。", "%^s is no longer fast."), m_name);
			}
		}
		break;

	case MTIMED_SLOW:
		/* Reduce by one, note if expires */
		if (set_monster_slow(m_idx, MON_SLOW(m_ptr) - 1))
		{
			if (is_seen(m_ptr))
			{
				char m_name[80];

				/* Acquire the monster name */
				monster_desc(m_name, m_ptr, 0);

				/* Dump a message */
				msg_format(_("%^sはもう減速されていない。", "%^s is no longer slow."), m_name);
			}
		}
		break;

	case MTIMED_STUNNED:
	{
		int rlev = r_info[m_ptr->r_idx].level;

		/* Recover from stun */
		if (set_monster_stunned(m_idx, (randint0(10000) <= rlev * rlev) ? 0 : (MON_STUNNED(m_ptr) - 1)))
		{
			/* Message if visible */
			if (is_seen(m_ptr))
			{
				char m_name[80];

				/* Acquire the monster name */
				monster_desc(m_name, m_ptr, 0);

				/* Dump a message */
				msg_format(_("%^sは朦朧状態から立ち直った。", "%^s is no longer stunned."), m_name);
			}
		}
		break;
	}

	case MTIMED_CONFUSED:
		/* Reduce the confusion */
		if (set_monster_confused(m_idx, MON_CONFUSED(m_ptr) - randint1(r_info[m_ptr->r_idx].level / 20 + 1)))
		{
			/* Message if visible */
			if (is_seen(m_ptr))
			{
				char m_name[80];

				/* Acquire the monster name */
				monster_desc(m_name, m_ptr, 0);

				/* Dump a message */
				msg_format(_("%^sは混乱から立ち直った。", "%^s is no longer confused."), m_name);
			}
		}
		break;

	case MTIMED_MONFEAR:
		/* Reduce the fear */
		if (set_monster_monfear(m_idx, MON_MONFEAR(m_ptr) - randint1(r_info[m_ptr->r_idx].level / 20 + 1)))
		{
			/* Visual note */
			if (is_seen(m_ptr))
			{
				char m_name[80];
#ifndef JP
				char m_poss[80];

				/* Acquire the monster possessive */
				monster_desc(m_poss, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE);
#endif

				/* Acquire the monster name */
				monster_desc(m_name, m_ptr, 0);

				/* Dump a message */
#ifdef JP
				msg_format("%^sは勇気を取り戻した。", m_name);
#else
				msg_format("%^s recovers %s courage.", m_name, m_poss);
#endif
			}
		}
		break;

	case MTIMED_INVULNER:
		/* Reduce by one, note if expires */
		if (set_monster_invulner(m_idx, MON_INVULNER(m_ptr) - 1, TRUE))
		{
			if (is_seen(m_ptr))
			{
				char m_name[80];

				/* Acquire the monster name */
				monster_desc(m_name, m_ptr, 0);

				/* Dump a message */
				msg_format(_("%^sはもう無敵でない。", "%^s is no longer invulnerable."), m_name);
			}
		}
		break;
	}
}


/*!
* @brief 全モンスターの各種状態値を時間経過により更新するメインルーチン
* @param mtimed_idx 更新するモンスターの時限ステータスID
* @return なし
* @details
* Process the counters of monsters (once per 10 game turns)\n
* These functions are to process monsters' counters same as player's.
*/
void process_monsters_mtimed(int mtimed_idx)
{
	int  i;
	s16b *cur_mproc_list = mproc_list[mtimed_idx];

	/* Hack -- calculate the "player noise" */
	if (mtimed_idx == MTIMED_CSLEEP) csleep_noise = (1L << (30 - p_ptr->skill_stl));

	/* Process the monsters (backwards) */
	for (i = mproc_max[mtimed_idx] - 1; i >= 0; i--)
	{
		/* Access the monster */
		process_monsters_mtimed_aux(cur_mproc_list[i], mtimed_idx);
	}
}

/*!
* @brief モンスターへの魔力消去処理
* @param m_idx 魔力消去を受けるモンスターの参照ID
* @return なし
*/
void dispel_monster_status(MONSTER_IDX m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];
	char         m_name[80];

	monster_desc(m_name, m_ptr, 0);
	if (set_monster_invulner(m_idx, 0, TRUE))
	{
		if (m_ptr->ml) msg_format(_("%sはもう無敵ではない。", "%^s is no longer invulnerable."), m_name);
	}
	if (set_monster_fast(m_idx, 0))
	{
		if (m_ptr->ml) msg_format(_("%sはもう加速されていない。", "%^s is no longer fast."), m_name);
	}
	if (set_monster_slow(m_idx, 0))
	{
		if (m_ptr->ml) msg_format(_("%sはもう減速されていない。", "%^s is no longer slow."), m_name);
	}
}

/*!
* @brief モンスターの時間停止処理
* @param num 時間停止を行った敵が行動できる回数
* @param who 時間停止処理の主体ID
* @param vs_player TRUEならば時間停止開始処理を行う
* @return 時間停止が行われている状態ならばTRUEを返す
*/
bool process_the_world(int num, int who, bool vs_player)
{
	monster_type *m_ptr = &m_list[hack_m_idx];  /* the world monster */

	if (world_monster) return (FALSE);

	if (vs_player)
	{
		char m_name[80];
		monster_desc(m_name, m_ptr, 0);

		if (who == 1)
			msg_format(_("「『ザ・ワールド』！時は止まった！」", "%s yells 'The World! Time has stopped!'"), m_name);
		else if (who == 3)
			msg_format(_("「時よ！」", "%s yells 'Time!'"), m_name);
		else msg_print("hek!");

		msg_print(NULL);
	}

	/* This monster cast spells */
	world_monster = hack_m_idx;

	if (vs_player) do_cmd_redraw();

	while (num--)
	{
		if (!m_ptr->r_idx) break;
		process_monster(world_monster);

		reset_target(m_ptr);

		/* Notice stuff */
		if (p_ptr->notice) notice_stuff();

		/* Update stuff */
		if (p_ptr->update) update_stuff();

		/* Redraw stuff */
		if (p_ptr->redraw) redraw_stuff();

		/* Redraw stuff */
		if (p_ptr->window) window_stuff();

		/* Delay */
		if (vs_player) Term_xtra(TERM_XTRA_DELAY, 500);
	}

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Update monsters */
	p_ptr->update |= (PU_MONSTERS);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

	world_monster = 0;
	if (vs_player || (player_has_los_bold(m_ptr->fy, m_ptr->fx) && projectable(p_ptr->y, p_ptr->x, m_ptr->fy, m_ptr->fx)))
	{
		msg_print(_("「時は動きだす…」", "You feel time flowing around you once more."));
		msg_print(NULL);
	}

	handle_stuff();

	return (TRUE);
}

/*!
* @brief モンスターの経験値取得処理
* @param m_idx 経験値を得るモンスターの参照ID
* @param s_idx 撃破されたモンスター種族の参照ID
* @return なし
*/
void monster_gain_exp(MONSTER_IDX m_idx, IDX s_idx)
{
	monster_type *m_ptr;
	monster_race *r_ptr;
	monster_race *s_ptr;
	int new_exp;

	/* Paranoia */
	if (m_idx <= 0 || s_idx <= 0) return;

	m_ptr = &m_list[m_idx];

	/* Paranoia -- Skip dead monsters */
	if (!m_ptr->r_idx) return;

	r_ptr = &r_info[m_ptr->r_idx];
	s_ptr = &r_info[s_idx];

	if (p_ptr->inside_battle) return;

	if (!r_ptr->next_exp) return;

	new_exp = s_ptr->mexp * s_ptr->level / (r_ptr->level + 2);
	if (m_idx == p_ptr->riding) new_exp = (new_exp + 1) / 2;
	if (!dun_level) new_exp /= 5;
	m_ptr->exp += new_exp;
	if (m_ptr->mflag2 & MFLAG2_CHAMELEON) return;

	if (m_ptr->exp >= r_ptr->next_exp)
	{
		char m_name[80];
		int old_hp = m_ptr->hp;
		int old_maxhp = m_ptr->max_maxhp;
		int old_r_idx = m_ptr->r_idx;
		byte old_sub_align = m_ptr->sub_align;

		/* Hack -- Reduce the racial counter of previous monster */
		real_r_ptr(m_ptr)->cur_num--;

		monster_desc(m_name, m_ptr, 0);
		m_ptr->r_idx = r_ptr->next_r_idx;

		/* Count the monsters on the level */
		real_r_ptr(m_ptr)->cur_num++;

		m_ptr->ap_r_idx = m_ptr->r_idx;
		r_ptr = &r_info[m_ptr->r_idx];

		if (r_ptr->flags1 & RF1_FORCE_MAXHP)
		{
			m_ptr->max_maxhp = maxroll(r_ptr->hdice, r_ptr->hside);
		}
		else
		{
			m_ptr->max_maxhp = damroll(r_ptr->hdice, r_ptr->hside);
		}
		if (ironman_nightmare)
		{
			u32b hp = m_ptr->max_maxhp * 2L;

			m_ptr->max_maxhp = (s16b)MIN(30000, hp);
		}
		m_ptr->maxhp = m_ptr->max_maxhp;
		m_ptr->hp = old_hp * m_ptr->maxhp / old_maxhp;

		/* dealt damage is 0 at initial*/
		m_ptr->dealt_damage = 0;

		/* Extract the monster base speed */
		m_ptr->mspeed = get_mspeed(r_ptr);

		/* Sub-alignment of a monster */
		if (!is_pet(m_ptr) && !(r_ptr->flags3 & (RF3_EVIL | RF3_GOOD)))
			m_ptr->sub_align = old_sub_align;
		else
		{
			m_ptr->sub_align = SUB_ALIGN_NEUTRAL;
			if (r_ptr->flags3 & RF3_EVIL) m_ptr->sub_align |= SUB_ALIGN_EVIL;
			if (r_ptr->flags3 & RF3_GOOD) m_ptr->sub_align |= SUB_ALIGN_GOOD;
		}

		m_ptr->exp = 0;

		if (is_pet(m_ptr) || m_ptr->ml)
		{
			if (!ignore_unview || player_can_see_bold(m_ptr->fy, m_ptr->fx))
			{
				if (p_ptr->image)
				{
					monster_race *hallu_race;

					do
					{
						hallu_race = &r_info[randint1(max_r_idx - 1)];
					} while (!hallu_race->name || (hallu_race->flags1 & RF1_UNIQUE));
					msg_format(_("%sは%sに進化した。", "%^s evolved into %s."), m_name, r_name + hallu_race->name);
				}
				else
				{
					msg_format(_("%sは%sに進化した。", "%^s evolved into %s."), m_name, r_name + r_ptr->name);
				}
			}

			if (!p_ptr->image) r_info[old_r_idx].r_xtra1 |= MR1_SINKA;

			/* Now you feel very close to this pet. */
			m_ptr->parent_m_idx = 0;
		}
		update_mon(m_idx, FALSE);
		lite_spot(m_ptr->fy, m_ptr->fx);
	}
	if (m_idx == p_ptr->riding) p_ptr->update |= PU_BONUS;
}
