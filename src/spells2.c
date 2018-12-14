/*!
 * @file spells2.c
 * @brief 魔法効果の実装/ Spell code (part 2)
 * @date 2014/07/15
 * @author
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * </pre>
 */

#include "angband.h"
#include "grid.h"
#include "trap.h"
#include "monsterrace-hook.h"


/*!
 * @brief プレイヤー周辺の地形を感知する
 * @param range 効果範囲
 * @param flag 特定地形ID
 * @param known 地形から危険フラグを外すならTRUE
 * @return 効力があった場合TRUEを返す
 */
static bool detect_feat_flag(POSITION range, int flag, bool known)
{
	POSITION x, y;
	bool detect = FALSE;
	cave_type *c_ptr;

	if (d_info[dungeon_type].flags1 & DF1_DARKNESS) range /= 3;

	/* Scan the current panel */
	for (y = 1; y < cur_hgt - 1; y++)
	{
		for (x = 1; x <= cur_wid - 1; x++)
		{
			int dist = distance(p_ptr->y, p_ptr->x, y, x);
			if (dist > range) continue;
			c_ptr = &cave[y][x];

			/* Hack -- Safe */
			if (flag == FF_TRAP)
			{
				/* Mark as detected */
				if (dist <= range && known)
				{
					if (dist <= range - 1) c_ptr->info |= (CAVE_IN_DETECT);

					c_ptr->info &= ~(CAVE_UNSAFE);

					lite_spot(y, x);
				}
			}

			/* Detect flags */
			if (cave_have_flag_grid(c_ptr, flag))
			{
				/* Detect secrets */
				disclose_grid(y, x);

				/* Hack -- Memorize */
				c_ptr->info |= (CAVE_MARK);

				lite_spot(y, x);

				detect = TRUE;
			}
		}
	}
	return detect;
}


/*!
 * @brief プレイヤー周辺のトラップを感知する / Detect all traps on current panel
 * @param range 効果範囲
 * @param known 感知外範囲を超える警告フラグを立てる場合TRUEを返す
 * @return 効力があった場合TRUEを返す
 */
bool detect_traps(POSITION range, bool known)
{
	bool detect = detect_feat_flag(range, FF_TRAP, known);

	if (known) p_ptr->dtrap = TRUE;

	if (music_singing(MUSIC_DETECT) && SINGING_COUNT(p_ptr) > 0) detect = FALSE;
	if (detect)
	{
		msg_print(_("トラップの存在を感じとった！", "You sense the presence of traps!"));
	}
	return detect;
}


/*!
 * @brief プレイヤー周辺のドアを感知する / Detect all doors on current panel
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_doors(POSITION range)
{
	bool detect = detect_feat_flag(range, FF_DOOR, TRUE);

	if (music_singing(MUSIC_DETECT) && SINGING_COUNT(p_ptr) > 0) detect = FALSE;
	if (detect)
	{
		msg_print(_("ドアの存在を感じとった！", "You sense the presence of doors!"));
	}
	return detect;
}


/*!
 * @brief プレイヤー周辺の階段を感知する / Detect all stairs on current panel
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_stairs(POSITION range)
{
	bool detect = detect_feat_flag(range, FF_STAIRS, TRUE);

	if (music_singing(MUSIC_DETECT) && SINGING_COUNT(p_ptr) > 0) detect = FALSE;
	if (detect)
	{
		msg_print(_("階段の存在を感じとった！", "You sense the presence of stairs!"));
	}
	return detect;
}


/*!
 * @brief プレイヤー周辺の地形財宝を感知する / Detect any treasure on the current panel
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_treasure(POSITION range)
{
	bool detect = detect_feat_flag(range, FF_HAS_GOLD, TRUE);

	if (music_singing(MUSIC_DETECT) && SINGING_COUNT(p_ptr) > 6) detect = FALSE;
	if (detect)
	{
		msg_print(_("埋蔵された財宝の存在を感じとった！", "You sense the presence of buried treasure!"));
	}
	return detect;
}


/*!
 * @brief プレイヤー周辺のアイテム財宝を感知する / Detect all "gold" objects on the current panel
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_objects_gold(POSITION range)
{
	OBJECT_IDX i;
	POSITION y, x;
	POSITION range2 = range;

	bool detect = FALSE;

	if (d_info[dungeon_type].flags1 & DF1_DARKNESS) range2 /= 3;

	/* Scan objects */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr = &o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Skip held objects */
		if (o_ptr->held_m_idx) continue;

		y = o_ptr->iy;
		x = o_ptr->ix;

		/* Only detect nearby objects */
		if (distance(p_ptr->y, p_ptr->x, y, x) > range2) continue;

		/* Detect "gold" objects */
		if (o_ptr->tval == TV_GOLD)
		{
			o_ptr->marked |= OM_FOUND;
			lite_spot(y, x);
			detect = TRUE;
		}
	}

	if (music_singing(MUSIC_DETECT) && SINGING_COUNT(p_ptr) > 6) detect = FALSE;
	if (detect)
	{
		msg_print(_("財宝の存在を感じとった！", "You sense the presence of treasure!"));
	}

	if (detect_monsters_string(range, "$"))
	{
		detect = TRUE;
	}
	return (detect);
}


/*!
 * @brief 通常のアイテムオブジェクトを感知する / Detect all "normal" objects on the current panel
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_objects_normal(POSITION range)
{
	OBJECT_IDX i;
	POSITION y, x;
	POSITION range2 = range;

	bool detect = FALSE;

	if (d_info[dungeon_type].flags1 & DF1_DARKNESS) range2 /= 3;

	/* Scan objects */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr = &o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Skip held objects */
		if (o_ptr->held_m_idx) continue;

		y = o_ptr->iy;
		x = o_ptr->ix;

		/* Only detect nearby objects */
		if (distance(p_ptr->y, p_ptr->x, y, x) > range2) continue;

		/* Detect "real" objects */
		if (o_ptr->tval != TV_GOLD)
		{
			o_ptr->marked |= OM_FOUND;
			lite_spot(y, x);
			detect = TRUE;
		}
	}

	if (music_singing(MUSIC_DETECT) && SINGING_COUNT(p_ptr) > 6) detect = FALSE;
	if (detect)
	{
		msg_print(_("アイテムの存在を感じとった！", "You sense the presence of objects!"));
	}

	if (detect_monsters_string(range, "!=?|/`"))
	{
		detect = TRUE;
	}
	return (detect);
}


/*!
 * @brief 魔法効果のあるのアイテムオブジェクトを感知する / Detect all "magic" objects on the current panel.
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 * @details
 * <pre>
 * This will light up all spaces with "magic" items, including artifacts,
 * ego-items, potions, scrolls, books, rods, wands, staves, amulets, rings,
 * and "enchanted" items of the "good" variety.
 *
 * It can probably be argued that this function is now too powerful.
 * </pre>
 */
bool detect_objects_magic(POSITION range)
{
	OBJECT_TYPE_VALUE tv;
	OBJECT_IDX i;
	POSITION y, x;

	bool detect = FALSE;

	if (d_info[dungeon_type].flags1 & DF1_DARKNESS) range /= 3;

	/* Scan all objects */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr = &o_list[i];

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Skip held objects */
		if (o_ptr->held_m_idx) continue;

		y = o_ptr->iy;
		x = o_ptr->ix;

		/* Only detect nearby objects */
		if (distance(p_ptr->y, p_ptr->x, y, x) > range) continue;

		/* Examine the tval */
		tv = o_ptr->tval;

		/* Artifacts, misc magic items, or enchanted wearables */
		if (object_is_artifact(o_ptr) ||
			object_is_ego(o_ptr) ||
		    (tv == TV_WHISTLE) ||
		    (tv == TV_AMULET) ||
			(tv == TV_RING) ||
		    (tv == TV_STAFF) ||
			(tv == TV_WAND) ||
			(tv == TV_ROD) ||
		    (tv == TV_SCROLL) ||
			(tv == TV_POTION) ||
		    (tv == TV_LIFE_BOOK) ||
			(tv == TV_SORCERY_BOOK) ||
		    (tv == TV_NATURE_BOOK) ||
			(tv == TV_CHAOS_BOOK) ||
		    (tv == TV_DEATH_BOOK) ||
		    (tv == TV_TRUMP_BOOK) ||
			(tv == TV_ARCANE_BOOK) ||
			(tv == TV_CRAFT_BOOK) ||
			(tv == TV_DAEMON_BOOK) ||
			(tv == TV_CRUSADE_BOOK) ||
			(tv == TV_MUSIC_BOOK) ||
			(tv == TV_HISSATSU_BOOK) ||
			(tv == TV_HEX_BOOK) ||
		    ((o_ptr->to_a > 0) || (o_ptr->to_h + o_ptr->to_d > 0)))
		{
			/* Memorize the item */
			o_ptr->marked |= OM_FOUND;
			lite_spot(y, x);
			detect = TRUE;
		}
	}
	if (detect)
	{
		msg_print(_("魔法のアイテムの存在を感じとった！", "You sense the presence of magic objects!"));
	}

	/* Return result */
	return (detect);
}


/*!
 * @brief 一般のモンスターを感知する / Detect all "normal" monsters on the current panel
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_normal(POSITION range)
{
	MONSTER_IDX i;
	POSITION y, x;

	bool flag = FALSE;

	if (d_info[dungeon_type].flags1 & DF1_DARKNESS) range /= 3;

	/* Scan monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Only detect nearby monsters */
		if (distance(p_ptr->y, p_ptr->x, y, x) > range) continue;

		/* Detect all non-invisible monsters */
		if (!(r_ptr->flags2 & RF2_INVISIBLE) || p_ptr->see_inv)
		{
			/* Repair visibility later */
			repair_monsters = TRUE;

			/* Hack -- Detect monster */
			m_ptr->mflag2 |= (MFLAG2_MARK | MFLAG2_SHOW);
			update_monster(i, FALSE);
			flag = TRUE;
		}
	}

	if (music_singing(MUSIC_DETECT) && SINGING_COUNT(p_ptr) > 3) flag = FALSE;
	if (flag)
	{
		/* Describe result */
		msg_print(_("モンスターの存在を感じとった！", "You sense the presence of monsters!"));
	}
	return (flag);
}


/*!
 * @brief 不可視のモンスターを感知する / Detect all "invisible" monsters around the player
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_invis(POSITION range)
{
	MONSTER_IDX i;
	POSITION y, x;
	bool flag = FALSE;

	if (d_info[dungeon_type].flags1 & DF1_DARKNESS) range /= 3;

	/* Scan monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Only detect nearby monsters */
		if (distance(p_ptr->y, p_ptr->x, y, x) > range) continue;

		/* Detect invisible monsters */
		if (r_ptr->flags2 & RF2_INVISIBLE)
		{
			/* Update monster recall window */
			if (p_ptr->monster_race_idx == m_ptr->r_idx)
			{
				p_ptr->window |= (PW_MONSTER);
			}

			/* Repair visibility later */
			repair_monsters = TRUE;

			/* Hack -- Detect monster */
			m_ptr->mflag2 |= (MFLAG2_MARK | MFLAG2_SHOW);
			update_monster(i, FALSE);
			flag = TRUE;
		}
	}

	if (music_singing(MUSIC_DETECT) && SINGING_COUNT(p_ptr) > 3) flag = FALSE;
	if (flag)
	{
		/* Describe result */
		msg_print(_("透明な生物の存在を感じとった！", "You sense the presence of invisible creatures!"));
	}
	return (flag);
}

/*!
 * @brief 邪悪なモンスターを感知する / Detect all "evil" monsters on current panel
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_evil(POSITION range)
{
	MONSTER_IDX i;
	POSITION y, x;
	bool flag = FALSE;

	if (d_info[dungeon_type].flags1 & DF1_DARKNESS) range /= 3;

	/* Scan monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Only detect nearby monsters */
		if (distance(p_ptr->y, p_ptr->x, y, x) > range) continue;

		/* Detect evil monsters */
		if (r_ptr->flags3 & RF3_EVIL)
		{
			if (is_original_ap(m_ptr))
			{
				/* Take note that they are evil */
				r_ptr->r_flags3 |= (RF3_EVIL);

				/* Update monster recall window */
				if (p_ptr->monster_race_idx == m_ptr->r_idx)
				{
					p_ptr->window |= (PW_MONSTER);
				}
			}

			/* Repair visibility later */
			repair_monsters = TRUE;

			/* Hack -- Detect monster */
			m_ptr->mflag2 |= (MFLAG2_MARK | MFLAG2_SHOW);
			update_monster(i, FALSE);
			flag = TRUE;
		}
	}
	if (flag)
	{
		/* Describe result */
		msg_print(_("邪悪なる生物の存在を感じとった！", "You sense the presence of evil creatures!"));
	}
	return (flag);
}

/*!
 * @brief 無生命のモンスターを感知する(アンデッド、悪魔系を含む) / Detect all "nonliving", "undead" or "demonic" monsters on current panel
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_nonliving(POSITION range)
{
	MONSTER_IDX i;
	POSITION y, x;
	bool flag = FALSE;

	if (d_info[dungeon_type].flags1 & DF1_DARKNESS) range /= 3;

	/* Scan monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Only detect nearby monsters */
		if (distance(p_ptr->y, p_ptr->x, y, x) > range) continue;

		/* Detect non-living monsters */
		if (!monster_living(m_ptr->r_idx))
		{
			/* Update monster recall window */
			if (p_ptr->monster_race_idx == m_ptr->r_idx)
			{
				p_ptr->window |= (PW_MONSTER);
			}

			/* Repair visibility later */
			repair_monsters = TRUE;

			/* Hack -- Detect monster */
			m_ptr->mflag2 |= (MFLAG2_MARK | MFLAG2_SHOW);
			update_monster(i, FALSE);
			flag = TRUE;
		}
	}
	if (flag)
	{
		/* Describe result */
		msg_print(_("自然でないモンスターの存在を感じた！", "You sense the presence of unnatural beings!"));
	}
	return (flag);
}

/*!
 * @brief 精神のあるモンスターを感知する / Detect all monsters it has mind on current panel
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_mind(POSITION range)
{
	MONSTER_IDX i;
	POSITION y, x;
	bool flag = FALSE;

	if (d_info[dungeon_type].flags1 & DF1_DARKNESS) range /= 3;

	/* Scan monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Only detect nearby monsters */
		if (distance(p_ptr->y, p_ptr->x, y, x) > range) continue;

		/* Detect non-living monsters */
		if (!(r_ptr->flags2 & RF2_EMPTY_MIND))
		{
			/* Update monster recall window */
			if (p_ptr->monster_race_idx == m_ptr->r_idx)
			{
				p_ptr->window |= (PW_MONSTER);
			}

			/* Repair visibility later */
			repair_monsters = TRUE;

			/* Hack -- Detect monster */
			m_ptr->mflag2 |= (MFLAG2_MARK | MFLAG2_SHOW);
			update_monster(i, FALSE);
			flag = TRUE;
		}
	}
	if (flag)
	{
		/* Describe result */
		msg_print(_("殺気を感じとった！", "You sense the presence of someone's mind!"));
	}
	return (flag);
}


/*!
 * @brief 該当シンボルのモンスターを感知する / Detect all (string) monsters on current panel
 * @param range 効果範囲
 * @param Match 対応シンボルの混じったモンスター文字列(複数指定化)
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_string(POSITION range, cptr Match)
{
	MONSTER_IDX i;
	POSITION y, x;
	bool flag = FALSE;

	if (d_info[dungeon_type].flags1 & DF1_DARKNESS) range /= 3;

	/* Scan monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Only detect nearby monsters */
		if (distance(p_ptr->y, p_ptr->x, y, x) > range) continue;

		/* Detect monsters with the same symbol */
		if (my_strchr(Match, r_ptr->d_char))
		{
			/* Update monster recall window */
			if (p_ptr->monster_race_idx == m_ptr->r_idx)
			{
				p_ptr->window |= (PW_MONSTER);
			}

			/* Repair visibility later */
			repair_monsters = TRUE;

			/* Hack -- Detect monster */
			m_ptr->mflag2 |= (MFLAG2_MARK | MFLAG2_SHOW);
			update_monster(i, FALSE);
			flag = TRUE;
		}
	}

	if (music_singing(MUSIC_DETECT) && SINGING_COUNT(p_ptr) > 3) flag = FALSE;
	if (flag)
	{
		/* Describe result */
		msg_print(_("モンスターの存在を感じとった！", "You sense the presence of monsters!"));
	}
	return (flag);
}

/*!
 * @brief flags3に対応するモンスターを感知する / A "generic" detect monsters routine, tagged to flags3
 * @param range 効果範囲
 * @param match_flag 感知フラグ
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_xxx(POSITION range, u32b match_flag)
{
	MONSTER_IDX i;
	POSITION y, x;
	bool flag = FALSE;
	cptr desc_monsters = _("変なモンスター", "weird monsters");

	if (d_info[dungeon_type].flags1 & DF1_DARKNESS) range /= 3;

	/* Scan monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Only detect nearby monsters */
		if (distance(p_ptr->y, p_ptr->x, y, x) > range) continue;

		/* Detect evil monsters */
		if (r_ptr->flags3 & (match_flag))
		{
			if (is_original_ap(m_ptr))
			{
				/* Take note that they are something */
				r_ptr->r_flags3 |= (match_flag);

				/* Update monster recall window */
				if (p_ptr->monster_race_idx == m_ptr->r_idx)
				{
					p_ptr->window |= (PW_MONSTER);
				}
			}

			/* Repair visibility later */
			repair_monsters = TRUE;

			/* Hack -- Detect monster */
			m_ptr->mflag2 |= (MFLAG2_MARK | MFLAG2_SHOW);
			update_monster(i, FALSE);
			flag = TRUE;
		}
	}
	if (flag)
	{
		switch (match_flag)
		{
			case RF3_DEMON:
			desc_monsters = _("デーモン", "demons");
				break;
			case RF3_UNDEAD:
			desc_monsters = _("アンデッド", "the undead");
				break;
		}

		/* Describe result */
		msg_format(_("%sの存在を感じとった！", "You sense the presence of %s!"), desc_monsters);
		msg_print(NULL);
	}
	return (flag);
}


/*!
 * @brief 全感知処理 / Detect everything
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_all(POSITION range)
{
	bool detect = FALSE;

	/* Detect everything */
	if (detect_traps(range, TRUE)) detect = TRUE;
	if (detect_doors(range)) detect = TRUE;
	if (detect_stairs(range)) detect = TRUE;

	/* There are too many hidden treasure.  So... */
	/* if (detect_treasure(range)) detect = TRUE; */

	if (detect_objects_gold(range)) detect = TRUE;
	if (detect_objects_normal(range)) detect = TRUE;
	if (detect_monsters_invis(range)) detect = TRUE;
	if (detect_monsters_normal(range)) detect = TRUE;
	return (detect);
}


/*!
 * @brief 視界内モンスターに魔法効果を与える / Apply a "project()" directly to all viewable monsters
 * @param typ 属性効果
 * @param dam 効果量
 * @return 効力があった場合TRUEを返す
 * @details
 * <pre>
 * Note that affected monsters are NOT auto-tracked by this usage.
 *
 * To avoid misbehavior when monster deaths have side-effects,
 * this is done in two passes. -- JDL
 * </pre>
 */
bool project_hack(EFFECT_ID typ, HIT_POINT dam)
{
	MONSTER_IDX i;
	POSITION x, y;
	BIT_FLAGS flg = PROJECT_JUMP | PROJECT_KILL | PROJECT_HIDE;
	bool obvious = FALSE;


	/* Mark all (nearby) monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Require line of sight */
		if (!player_has_los_bold(y, x) || !projectable(p_ptr->y, p_ptr->x, y, x)) continue;

		/* Mark the monster */
		m_ptr->mflag |= (MFLAG_TEMP);
	}

	/* Affect all marked monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];

		/* Skip unmarked monsters */
		if (!(m_ptr->mflag & (MFLAG_TEMP))) continue;

		/* Remove mark */
		m_ptr->mflag &= ~(MFLAG_TEMP);

		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Jump directly to the target monster */
		if (project(0, 0, y, x, dam, typ, flg, -1)) obvious = TRUE;
	}
	return (obvious);
}


/*!
 * @brief 視界内モンスターを加速する処理 / Speed monsters
 * @return 効力があった場合TRUEを返す
 */
bool speed_monsters(void)
{
	return (project_hack(GF_OLD_SPEED, p_ptr->lev));
}

/*!
 * @brief 視界内モンスターを加速する処理 / Slow monsters
 * @return 効力があった場合TRUEを返す
 */
bool slow_monsters(int power)
{
	return (project_hack(GF_OLD_SLOW, power));
}

/*!
 * @brief 視界内モンスターを眠らせる処理 / Sleep monsters
 * @return 効力があった場合TRUEを返す
 */
bool sleep_monsters(int power)
{
	return (project_hack(GF_OLD_SLEEP, power));
}

/*!
 * @brief 視界内の邪悪なモンスターをテレポート・アウェイさせる処理 / Banish evil monsters
 * @return 効力があった場合TRUEを返す
 */
bool banish_evil(int dist)
{
	return (project_hack(GF_AWAY_EVIL, dist));
}

/*!
 * @brief 視界内のアンデッド・モンスターを恐怖させる処理 / Turn undead
 * @return 効力があった場合TRUEを返す
 */
bool turn_undead(void)
{
	bool tester = (project_hack(GF_TURN_UNDEAD, p_ptr->lev));
	if (tester)
		chg_virtue(V_UNLIFE, -1);
	return tester;
}

/*!
 * @brief 視界内のアンデッド・モンスターにダメージを与える処理 / Dispel undead monsters
 * @return 効力があった場合TRUEを返す
 */
bool dispel_undead(HIT_POINT dam)
{
	bool tester = (project_hack(GF_DISP_UNDEAD, dam));
	if (tester)
		chg_virtue(V_UNLIFE, -2);
	return tester;
}

/*!
 * @brief 視界内の邪悪なモンスターにダメージを与える処理 / Dispel evil monsters
 * @return 効力があった場合TRUEを返す
 */
bool dispel_evil(HIT_POINT dam)
{
	return (project_hack(GF_DISP_EVIL, dam));
}

/*!
 * @brief 視界内の善良なモンスターにダメージを与える処理 / Dispel good monsters
 * @return 効力があった場合TRUEを返す
 */
bool dispel_good(HIT_POINT dam)
{
	return (project_hack(GF_DISP_GOOD, dam));
}

/*!
 * @brief 視界内のあらゆるモンスターにダメージを与える処理 / Dispel all monsters
 * @return 効力があった場合TRUEを返す
 */
bool dispel_monsters(HIT_POINT dam)
{
	return (project_hack(GF_DISP_ALL, dam));
}

/*!
 * @brief 視界内の生命のあるモンスターにダメージを与える処理 / Dispel 'living' monsters
 * @return 効力があった場合TRUEを返す
 */
bool dispel_living(HIT_POINT dam)
{
	return (project_hack(GF_DISP_LIVING, dam));
}

/*!
 * @brief 視界内の悪魔系モンスターにダメージを与える処理 / Dispel 'living' monsters
 * @return 効力があった場合TRUEを返す
 */
bool dispel_demons(HIT_POINT dam)
{
	return (project_hack(GF_DISP_DEMON, dam));
}

/*!
 * @brief 視界内のモンスターに「聖戦」効果を与える処理
 * @return 効力があった場合TRUEを返す
 */
bool crusade(void)
{
	return (project_hack(GF_CRUSADE, p_ptr->lev*4));
}

/*!
 * @brief 視界内モンスターを怒らせる処理 / Wake up all monsters, and speed up "los" monsters.
 * @param who 怒らせる原因を起こしたモンスター(0ならばプレイヤー)
 * @return なし
 */
void aggravate_monsters(MONSTER_IDX who)
{
	MONSTER_IDX i;
	bool    sleep = FALSE;
	bool    speed = FALSE;

	/* Aggravate everyone nearby */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip aggravating monster (or player) */
		if (i == who) continue;

		/* Wake up nearby sleeping monsters */
		if (m_ptr->cdis < MAX_SIGHT * 2)
		{
			/* Wake up */
			if (MON_CSLEEP(m_ptr))
			{
				(void)set_monster_csleep(i, 0);
				sleep = TRUE;
			}
			if (!is_pet(m_ptr)) m_ptr->mflag2 |= MFLAG2_NOPET;
		}

		/* Speed up monsters in line of sight */
		if (player_has_los_bold(m_ptr->fy, m_ptr->fx))
		{
			if (!is_pet(m_ptr))
			{
				(void)set_monster_fast(i, MON_FAST(m_ptr) + 100);
				speed = TRUE;
			}
		}
	}

	if (speed) msg_print(_("付近で何かが突如興奮したような感じを受けた！", "You feel a sudden stirring nearby!"));
	else if (sleep) msg_print(_("何かが突如興奮したような騒々しい音が遠くに聞こえた！", "You hear a sudden stirring in the distance!"));
	if (p_ptr->riding) p_ptr->update |= PU_BONUS;
}


/*!
 * @brief モンスターへの単体抹殺処理サブルーチン / Delete a non-unique/non-quest monster
 * @param m_idx 抹殺するモンスターID
 * @param power 抹殺の威力
 * @param player_cast プレイヤーの魔法によるものならば TRUE
 * @param dam_side プレイヤーへの負担ダメージ量(1d(dam_side))
 * @param spell_name 抹殺効果を起こした魔法の名前
 * @return 効力があった場合TRUEを返す
 */
bool genocide_aux(MONSTER_IDX m_idx, int power, bool player_cast, int dam_side, cptr spell_name)
{
	int          msec = delay_factor * delay_factor * delay_factor;
	monster_type *m_ptr = &m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	bool         resist = FALSE;

	if (is_pet(m_ptr) && !player_cast) return FALSE;

	/* Hack -- Skip Unique Monsters or Quest Monsters */
	if (r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) resist = TRUE;
	else if (r_ptr->flags7 & RF7_UNIQUE2) resist = TRUE;
	else if (m_idx == p_ptr->riding) resist = TRUE;
	else if ((p_ptr->inside_quest && !random_quest_number(dun_level)) || p_ptr->inside_arena || p_ptr->inside_battle) resist = TRUE;
	else if (player_cast && (r_ptr->level > randint0(power))) resist = TRUE;
	else if (player_cast && (m_ptr->mflag2 & MFLAG2_NOGENO)) resist = TRUE;


	else
	{
		if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname)
		{
			char m_name[80];

			monster_desc(m_name, m_ptr, MD_INDEF_VISIBLE);
			do_cmd_write_nikki(NIKKI_NAMED_PET, RECORD_NAMED_PET_GENOCIDE, m_name);
		}

		delete_monster_idx(m_idx);
	}

	if (resist && player_cast)
	{
		bool see_m = is_seen(m_ptr);
		char m_name[80];

		monster_desc(m_name, m_ptr, 0);
		if (see_m)
		{
			msg_format(_("%^sには効果がなかった。", "%^s is unaffected."), m_name);
		}
		if (MON_CSLEEP(m_ptr))
		{
			(void)set_monster_csleep(m_idx, 0);
			if (m_ptr->ml)
			{
				msg_format(_("%^sが目を覚ました。", "%^s wakes up."), m_name);
			}
		}
		if (is_friendly(m_ptr) && !is_pet(m_ptr))
		{
			if (see_m)
			{
				msg_format(_("%sは怒った！", "%^s gets angry!"), m_name);
			}
			set_hostile(m_ptr);
		}
		if (one_in_(13)) m_ptr->mflag2 |= MFLAG2_NOGENO;
	}

	if (player_cast)
	{
		take_hit(DAMAGE_GENO, randint1(dam_side), format(_("%^sの呪文を唱えた疲労", "the strain of casting %^s"), spell_name), -1);
	}

	/* Visual feedback */
	move_cursor_relative(p_ptr->y, p_ptr->x);

	p_ptr->redraw |= (PR_HP);
	p_ptr->window |= (PW_PLAYER);

	/* Handle */
	handle_stuff();
	Term_fresh();

	Term_xtra(TERM_XTRA_DELAY, msec);

	return !resist;
}


/*!
 * @brief モンスターへのシンボル抹殺処理ルーチン / Delete all non-unique/non-quest monsters of a given "type" from the level
 * @param power 抹殺の威力
 * @param player_cast プレイヤーの魔法によるものならば TRUE
 * @return 効力があった場合TRUEを返す
 */
bool symbol_genocide(int power, bool player_cast)
{
	MONSTER_IDX i;
	char typ;
	bool result = FALSE;

	/* Prevent genocide in quest levels */
	if ((p_ptr->inside_quest && !random_quest_number(dun_level)) || p_ptr->inside_arena || p_ptr->inside_battle)
	{
		return (FALSE);
	}

	/* Mega-Hack -- Get a monster symbol */
	while (!get_com(_("どの種類(文字)のモンスターを抹殺しますか: ", "Choose a monster race (by symbol) to genocide: "), &typ, FALSE)) ;

	/* Delete the monsters of that "type" */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip "wrong" monsters */
		if (r_ptr->d_char != typ) continue;

		/* Take note */
		result |= genocide_aux(i, power, player_cast, 4, _("抹殺", "Genocide"));
	}

	if (result)
	{
		chg_virtue(V_VITALITY, -2);
		chg_virtue(V_CHANCE, -1);
	}

	return result;
}


/*!
 * @brief モンスターへの周辺抹殺処理ルーチン / Delete all nearby (non-unique) monsters
 * @param power 抹殺の威力
 * @param player_cast プレイヤーの魔法によるものならば TRUE
 * @return 効力があった場合TRUEを返す
 */
bool mass_genocide(int power, bool player_cast)
{
	MONSTER_IDX i;
	bool result = FALSE;

	/* Prevent mass genocide in quest levels */
	if ((p_ptr->inside_quest && !random_quest_number(dun_level)) || p_ptr->inside_arena || p_ptr->inside_battle)
	{
		return (FALSE);
	}

	/* Delete the (nearby) monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Skip distant monsters */
		if (m_ptr->cdis > MAX_SIGHT) continue;

		/* Note effect */
		result |= genocide_aux(i, power, player_cast, 3, _("周辺抹殺", "Mass Genocide"));
	}

	if (result)
	{
		chg_virtue(V_VITALITY, -2);
		chg_virtue(V_CHANCE, -1);
	}

	return result;
}


/*!
 * @brief アンデッド・モンスターへの周辺抹殺処理ルーチン / Delete all nearby (non-unique) undead
 * @param power 抹殺の威力
 * @param player_cast プレイヤーの魔法によるものならば TRUE
 * @return 効力があった場合TRUEを返す
 */
bool mass_genocide_undead(int power, bool player_cast)
{
	MONSTER_IDX i;
	bool result = FALSE;

	/* Prevent mass genocide in quest levels */
	if ((p_ptr->inside_quest && !random_quest_number(dun_level)) || p_ptr->inside_arena || p_ptr->inside_battle)
	{
		return (FALSE);
	}

	/* Delete the (nearby) monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		if (!(r_ptr->flags3 & RF3_UNDEAD)) continue;

		/* Skip distant monsters */
		if (m_ptr->cdis > MAX_SIGHT) continue;

		/* Note effect */
		result |= genocide_aux(i, power, player_cast, 3, _("アンデッド消滅", "Annihilate Undead"));
	}

	if (result)
	{
		chg_virtue(V_UNLIFE, -2);
		chg_virtue(V_CHANCE, -1);
	}

	return result;
}


/*!
 * @brief 周辺モンスターを調査する / Probe nearby monsters
 * @return 効力があった場合TRUEを返す
 */
bool probing(void)
{
	int i;
	SPEED speed;
	bool_hack cu, cv;
	bool probe = FALSE;
	char buf[256];
	cptr align;

	cu = Term->scr->cu;
	cv = Term->scr->cv;
	Term->scr->cu = 0;
	Term->scr->cv = 1;

	/* Probe all (nearby) monsters */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		/* Paranoia -- Skip dead monsters */
		if (!m_ptr->r_idx) continue;

		/* Require line of sight */
		if (!player_has_los_bold(m_ptr->fy, m_ptr->fx)) continue;

		/* Probe visible monsters */
		if (m_ptr->ml)
		{
			char m_name[80];

			/* Start the message */
			if (!probe)
			{
				msg_print(_("調査中...", "Probing..."));
			}

			msg_print(NULL);

			if (!is_original_ap(m_ptr))
			{
				if (m_ptr->mflag2 & MFLAG2_KAGE)
					m_ptr->mflag2 &= ~(MFLAG2_KAGE);

				m_ptr->ap_r_idx = m_ptr->r_idx;
				lite_spot(m_ptr->fy, m_ptr->fx);
			}
			/* Get "the monster" or "something" */
			monster_desc(m_name, m_ptr, MD_IGNORE_HALLU | MD_INDEF_HIDDEN);

			speed = m_ptr->mspeed - 110;
			if (MON_FAST(m_ptr)) speed += 10;
			if (MON_SLOW(m_ptr)) speed -= 10;
			if (ironman_nightmare) speed += 5;

			/* Get the monster's alignment */
			if ((r_ptr->flags3 & (RF3_EVIL | RF3_GOOD)) == (RF3_EVIL | RF3_GOOD)) align = _("善悪", "good&evil");
			else if (r_ptr->flags3 & RF3_EVIL) align = _("邪悪", "evil");
			else if (r_ptr->flags3 & RF3_GOOD) align = _("善良", "good");
			else if ((m_ptr->sub_align & (SUB_ALIGN_EVIL | SUB_ALIGN_GOOD)) == (SUB_ALIGN_EVIL | SUB_ALIGN_GOOD)) _(align = "中立(善悪)", "neutral(good&evil)");
			else if (m_ptr->sub_align & SUB_ALIGN_EVIL) align = _("中立(邪悪)", "neutral(evil)");
			else if (m_ptr->sub_align & SUB_ALIGN_GOOD) align = _("中立(善良)", "neutral(good)");
			else align = _("中立", "neutral");

			/* Describe the monster */
			sprintf(buf,_("%s ... 属性:%s HP:%d/%d AC:%d 速度:%s%d 経験:", "%s ... align:%s HP:%d/%d AC:%d speed:%s%d exp:"),
				m_name, align, (int)m_ptr->hp, (int)m_ptr->maxhp, r_ptr->ac, (speed > 0) ? "+" : "", speed);

			if (r_ptr->next_r_idx)
			{
				strcat(buf, format("%d/%d ", m_ptr->exp, r_ptr->next_exp));
			}
			else
			{
				strcat(buf, "xxx ");
			}

			if (MON_CSLEEP(m_ptr)) strcat(buf,_("睡眠 ", "sleeping "));
			if (MON_STUNNED(m_ptr)) strcat(buf, _("朦朧 ", "stunned "));
			if (MON_MONFEAR(m_ptr)) strcat(buf, _("恐怖 ", "scared "));
			if (MON_CONFUSED(m_ptr)) strcat(buf, _("混乱 ", "confused "));
			if (MON_INVULNER(m_ptr)) strcat(buf, _("無敵 ", "invulnerable "));
			buf[strlen(buf)-1] = '\0';
			prt(buf, 0, 0);

			/* HACK : Add the line to message buffer */
			message_add(buf);
			p_ptr->window |= (PW_MESSAGE);
			window_stuff();

			if (m_ptr->ml) move_cursor_relative(m_ptr->fy, m_ptr->fx);
			inkey();

			Term_erase(0, 0, 255);

			/* Learn everything about this monster */
			if (lore_do_probe(m_ptr->r_idx))
			{
				/* Get base name of monster */
				strcpy(buf, (r_name + r_ptr->name));

#ifdef JP
				/* Note that we learnt some new flags  -Mogami- */
				msg_format("%sについてさらに詳しくなった気がする。", buf);
#else
				/* Pluralize it */
				plural_aux(buf);

				/* Note that we learnt some new flags  -Mogami- */
				msg_format("You now know more about %s.", buf);
#endif
				/* Clear -more- prompt */
				msg_print(NULL);
			}

			/* Probe worked */
			probe = TRUE;
		}
	}

	Term->scr->cu = cu;
	Term->scr->cv = cv;
	Term_fresh();

	if (probe)
	{
		chg_virtue(V_KNOWLEDGE, 1);
		msg_print(_("これで全部です。", "That's all."));
	}
	return (probe);
}



/*!
 * @brief *破壊*処理を行う / The spell of destruction
 * @param y1 破壊の中心Y座標
 * @param x1 破壊の中心X座標 
 * @param r 破壊の半径
 * @param in_generate ダンジョンフロア生成中の処理ならばTRUE
 * @return 効力があった場合TRUEを返す
 * @details
 * <pre>
 * This spell "deletes" monsters (instead of "killing" them).
 *
 * Later we may use one function for both "destruction" and
 * "earthquake" by using the "full" to select "destruction".
 * </pre>
 */
bool destroy_area(POSITION y1, POSITION x1, POSITION r, bool in_generate)
{
	POSITION y, x;
	int k, t;
	cave_type *c_ptr;
	bool flag = FALSE;

	/* Prevent destruction of quest levels and town */
	if ((p_ptr->inside_quest && is_fixed_quest_idx(p_ptr->inside_quest)) || !dun_level)
	{
		return (FALSE);
	}

	/* Lose monster light */
	if (!in_generate) clear_mon_lite();

	/* Big area of affect */
	for (y = (y1 - r); y <= (y1 + r); y++)
	{
		for (x = (x1 - r); x <= (x1 + r); x++)
		{
			/* Skip illegal grids */
			if (!in_bounds(y, x)) continue;

			/* Extract the distance */
			k = distance(y1, x1, y, x);

			/* Stay in the circle of death */
			if (k > r) continue;
			c_ptr = &cave[y][x];

			/* Lose room and vault */
			c_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY);

			/* Lose light and knowledge */
			c_ptr->info &= ~(CAVE_MARK | CAVE_GLOW | CAVE_KNOWN);

			if (!in_generate) /* Normal */
			{
				/* Lose unsafety */
				c_ptr->info &= ~(CAVE_UNSAFE);

				/* Hack -- Notice player affect */
				if (player_bold(y, x))
				{
					/* Hurt the player later */
					flag = TRUE;

					/* Do not hurt this grid */
					continue;
				}
			}

			/* Hack -- Skip the epicenter */
			if ((y == y1) && (x == x1)) continue;

			if (c_ptr->m_idx)
			{
				monster_type *m_ptr = &m_list[c_ptr->m_idx];
				monster_race *r_ptr = &r_info[m_ptr->r_idx];

				if (in_generate) /* In generation */
				{
					/* Delete the monster (if any) */
					delete_monster(y, x);
				}
				else if (r_ptr->flags1 & RF1_QUESTOR)
				{
					/* Heal the monster */
					m_ptr->hp = m_ptr->maxhp;

					/* Try to teleport away quest monsters */
					if (!teleport_away(c_ptr->m_idx, (r * 2) + 1, TELEPORT_DEC_VALOUR)) continue;
				}
				else
				{
					if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname)
					{
						char m_name[80];

						monster_desc(m_name, m_ptr, MD_INDEF_VISIBLE);
						do_cmd_write_nikki(NIKKI_NAMED_PET, RECORD_NAMED_PET_DESTROY, m_name);
					}

					/* Delete the monster (if any) */
					delete_monster(y, x);
				}
			}

			/* During generation, destroyed artifacts are "preserved" */
			if (preserve_mode || in_generate)
			{
				OBJECT_IDX this_o_idx, next_o_idx = 0;

				/* Scan all objects in the grid */
				for (this_o_idx = c_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
				{
					object_type *o_ptr;
					o_ptr = &o_list[this_o_idx];

					/* Acquire next object */
					next_o_idx = o_ptr->next_o_idx;

					/* Hack -- Preserve unknown artifacts */
					if (object_is_fixed_artifact(o_ptr) && (!object_is_known(o_ptr) || in_generate))
					{
						/* Mega-Hack -- Preserve the artifact */
						a_info[o_ptr->name1].cur_num = 0;

						if (in_generate && cheat_peek)
						{
							char o_name[MAX_NLEN];
							object_desc(o_name, o_ptr, (OD_NAME_ONLY | OD_STORE));
							msg_format(_("伝説のアイテム (%s) は生成中に*破壊*された。", "Artifact (%s) was *destroyed* during generation."), o_name);
						}
					}
					else if (in_generate && cheat_peek && o_ptr->art_name)
					{
						msg_print(_("ランダム・アーティファクトの1つは生成中に*破壊*された。", 
									"One of the random artifacts was *destroyed* during generation."));
					}
				}
			}

			/* Delete objects */
			delete_object(y, x);

			/* Destroy "non-permanent" grids */
			if (!cave_perma_grid(c_ptr))
			{
				/* Wall (or floor) type */
				t = randint0(200);

				if (!in_generate) /* Normal */
				{
					if (t < 20)
					{
						/* Create granite wall */
						cave_set_feat(y, x, feat_granite);
					}
					else if (t < 70)
					{
						/* Create quartz vein */
						cave_set_feat(y, x, feat_quartz_vein);
					}
					else if (t < 100)
					{
						/* Create magma vein */
						cave_set_feat(y, x, feat_magma_vein);
					}
					else
					{
						/* Create floor */
						cave_set_feat(y, x, floor_type[randint0(100)]);
					}
				}
				else /* In generation */
				{
					if (t < 20)
					{
						/* Create granite wall */
						place_extra_grid(c_ptr);
					}
					else if (t < 70)
					{
						/* Create quartz vein */
						c_ptr->feat = feat_quartz_vein;
					}
					else if (t < 100)
					{
						/* Create magma vein */
						c_ptr->feat = feat_magma_vein;
					}
					else
					{
						/* Create floor */
						place_floor_grid(c_ptr);
					}

					/* Clear garbage of hidden trap or door */
					c_ptr->mimic = 0;
				}
			}
		}
	}

	if (!in_generate)
	{
		/* Process "re-glowing" */
		for (y = (y1 - r); y <= (y1 + r); y++)
		{
			for (x = (x1 - r); x <= (x1 + r); x++)
			{
				/* Skip illegal grids */
				if (!in_bounds(y, x)) continue;

				/* Extract the distance */
				k = distance(y1, x1, y, x);

				/* Stay in the circle of death */
				if (k > r) continue;
				c_ptr = &cave[y][x];

				if (is_mirror_grid(c_ptr)) c_ptr->info |= CAVE_GLOW;
				else if (!(d_info[dungeon_type].flags1 & DF1_DARKNESS))
				{
					int i, yy, xx;
					cave_type *cc_ptr;

					for (i = 0; i < 9; i++)
					{
						yy = y + ddy_ddd[i];
						xx = x + ddx_ddd[i];
						if (!in_bounds2(yy, xx)) continue;
						cc_ptr = &cave[yy][xx];
						if (have_flag(f_info[get_feat_mimic(cc_ptr)].flags, FF_GLOW))
						{
							c_ptr->info |= CAVE_GLOW;
							break;
						}
					}
				}
			}
		}

		/* Hack -- Affect player */
		if (flag)
		{
			msg_print(_("燃えるような閃光が発生した！", "There is a searing blast of light!"));

			/* Blind the player */
			if (!p_ptr->resist_blind && !p_ptr->resist_lite)
			{
				/* Become blind */
				(void)set_blind(p_ptr->blind + 10 + randint1(10));
			}
		}

		forget_flow();

		/* Mega-Hack -- Forget the view and lite */
		p_ptr->update |= (PU_UN_VIEW | PU_UN_LITE);

		p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MON_LITE | PU_MONSTERS);

		p_ptr->redraw |= (PR_MAP);

		p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

		if (p_ptr->special_defense & NINJA_S_STEALTH)
		{
			if (cave[p_ptr->y][p_ptr->x].info & CAVE_GLOW) set_superstealth(FALSE);
		}
	}

	/* Success */
	return (TRUE);
}


/*!
 * @brief 地震処理(サブルーチン) /
 * Induce an "earthquake" of the given radius at the given location.
 * @return 効力があった場合TRUEを返す
 * @param cy 中心Y座標
 * @param cx 中心X座標
 * @param r 効果半径
 * @param m_idx 地震を起こしたモンスターID(0ならばプレイヤー)
 * @details
 * <pre>
 *
 * This will turn some walls into floors and some floors into walls.
 *
 * The player will take damage and "jump" into a safe grid if possible,
 * otherwise, he will "tunnel" through the rubble instantaneously.
 *
 * Monsters will take damage, and "jump" into a safe grid if possible,
 * otherwise they will be "buried" in the rubble, disappearing from
 * the level in the same way that they do when genocided.
 *
 * Note that thus the player and monsters (except eaters of walls and
 * passers through walls) will never occupy the same grid as a wall.
 * Note that as of now (2.7.8) no monster may occupy a "wall" grid, even
 * for a single turn, unless that monster can pass_walls or kill_walls.
 * This has allowed massive simplification of the "monster" code.
 * </pre>
 */
bool earthquake_aux(POSITION cy, POSITION cx, POSITION r, MONSTER_IDX m_idx)
{
	DIRECTION i;
	int t;
	POSITION y, x, yy, xx, dy, dx;
	int             damage = 0;
	int             sn = 0;
	POSITION        sy = 0, sx = 0;
	bool            hurt = FALSE;
	cave_type       *c_ptr;
	bool            map[32][32];


	/* Prevent destruction of quest levels and town */
	if ((p_ptr->inside_quest && is_fixed_quest_idx(p_ptr->inside_quest)) || !dun_level)
	{
		return (FALSE);
	}

	/* Paranoia -- Enforce maximum range */
	if (r > 12) r = 12;

	/* Clear the "maximal blast" area */
	for (y = 0; y < 32; y++)
	{
		for (x = 0; x < 32; x++)
		{
			map[y][x] = FALSE;
		}
	}

	/* Check around the epicenter */
	for (dy = -r; dy <= r; dy++)
	{
		for (dx = -r; dx <= r; dx++)
		{
			/* Extract the location */
			yy = cy + dy;
			xx = cx + dx;

			/* Skip illegal grids */
			if (!in_bounds(yy, xx)) continue;

			/* Skip distant grids */
			if (distance(cy, cx, yy, xx) > r) continue;
			c_ptr = &cave[yy][xx];

			/* Lose room and vault */
			c_ptr->info &= ~(CAVE_ROOM | CAVE_ICKY | CAVE_UNSAFE);

			/* Lose light and knowledge */
			c_ptr->info &= ~(CAVE_GLOW | CAVE_MARK | CAVE_KNOWN);

			/* Skip the epicenter */
			if (!dx && !dy) continue;

			/* Skip most grids */
			if (randint0(100) < 85) continue;

			/* Damage this grid */
			map[16+yy-cy][16+xx-cx] = TRUE;

			/* Hack -- Take note of player damage */
			if (player_bold(yy, xx)) hurt = TRUE;
		}
	}

	/* First, affect the player (if necessary) */
	if (hurt && !p_ptr->pass_wall && !p_ptr->kill_wall)
	{
		/* Check around the player */
		for (i = 0; i < 8; i++)
		{
			/* Access the location */
			y = p_ptr->y + ddy_ddd[i];
			x = p_ptr->x + ddx_ddd[i];

			/* Skip non-empty grids */
			if (!cave_empty_bold(y, x)) continue;

			/* Important -- Skip "quake" grids */
			if (map[16+y-cy][16+x-cx]) continue;

			if (cave[y][x].m_idx) continue;

			/* Count "safe" grids */
			sn++;

			/* Randomize choice */
			if (randint0(sn) > 0) continue;

			/* Save the safe location */
			sy = y; sx = x;
		}

		/* Random message */
		switch (randint1(3))
		{
			case 1:
			{
				msg_print(_("ダンジョンの壁が崩れた！", "The cave ceiling collapses!"));
				break;
			}
			case 2:
			{
				msg_print(_("ダンジョンの床が不自然にねじ曲がった！", "The cave floor twists in an unnatural way!"));
				break;
			}
			default:
			{
				msg_print(_("ダンジョンが揺れた！崩れた岩が頭に降ってきた！", "The cave quakes!  You are pummeled with debris!"));
				break;
			}
		}

		/* Hurt the player a lot */
		if (!sn)
		{
			/* Message and damage */
			msg_print(_("あなたはひどい怪我を負った！", "You are severely crushed!"));
			damage = 200;
		}

		/* Destroy the grid, and push the player to safety */
		else
		{
			/* Calculate results */
			switch (randint1(3))
			{
				case 1:
				{
					msg_print(_("降り注ぐ岩をうまく避けた！", "You nimbly dodge the blast!"));
					damage = 0;
					break;
				}
				case 2:
				{
					msg_print(_("岩石があなたに直撃した!", "You are bashed by rubble!"));
					damage = damroll(10, 4);
					(void)set_stun(p_ptr->stun + randint1(50));
					break;
				}
				case 3:
				{
					msg_print(_("あなたは床と壁との間に挟まれてしまった！", "You are crushed between the floor and ceiling!"));
					damage = damroll(10, 4);
					(void)set_stun(p_ptr->stun + randint1(50));
					break;
				}
			}

			/* Move the player to the safe location */
			(void)move_player_effect(sy, sx, MPE_DONT_PICKUP);
		}

		/* Important -- no wall on player */
		map[16+p_ptr->y-cy][16+p_ptr->x-cx] = FALSE;

		if (damage)
		{
			cptr killer;

			if (m_idx)
			{
				char m_name[80];
				monster_type *m_ptr = &m_list[m_idx];

				/* Get the monster's real name */
				monster_desc(m_name, m_ptr, MD_IGNORE_HALLU | MD_ASSUME_VISIBLE | MD_INDEF_VISIBLE);

				killer = format(_("%sの起こした地震", "an earthquake caused by %s"), m_name);
			}
			else
			{
				killer = _("地震", "an earthquake");
			}

			take_hit(DAMAGE_ATTACK, damage, killer, -1);
		}
	}

	/* Examine the quaked region */
	for (dy = -r; dy <= r; dy++)
	{
		for (dx = -r; dx <= r; dx++)
		{
			/* Extract the location */
			yy = cy + dy;
			xx = cx + dx;

			/* Skip unaffected grids */
			if (!map[16+yy-cy][16+xx-cx]) continue;
			c_ptr = &cave[yy][xx];

			if (c_ptr->m_idx == p_ptr->riding) continue;

			/* Process monsters */
			if (c_ptr->m_idx)
			{
				monster_type *m_ptr = &m_list[c_ptr->m_idx];
				monster_race *r_ptr = &r_info[m_ptr->r_idx];

				/* Quest monsters */
				if (r_ptr->flags1 & RF1_QUESTOR)
				{
					/* No wall on quest monsters */
					map[16+yy-cy][16+xx-cx] = FALSE;

					continue;
				}

				/* Most monsters cannot co-exist with rock */
				if (!(r_ptr->flags2 & (RF2_KILL_WALL)) &&
				    !(r_ptr->flags2 & (RF2_PASS_WALL)))
				{
					char m_name[80];

					/* Assume not safe */
					sn = 0;

					/* Monster can move to escape the wall */
					if (!(r_ptr->flags1 & (RF1_NEVER_MOVE)))
					{
						/* Look for safety */
						for (i = 0; i < 8; i++)
						{
							y = yy + ddy_ddd[i];
							x = xx + ddx_ddd[i];

							/* Skip non-empty grids */
							if (!cave_empty_bold(y, x)) continue;

							/* Hack -- no safety on glyph of warding */
							if (is_glyph_grid(&cave[y][x])) continue;
							if (is_explosive_rune_grid(&cave[y][x])) continue;

							/* ... nor on the Pattern */
							if (pattern_tile(y, x)) continue;

							/* Important -- Skip "quake" grids */
							if (map[16+y-cy][16+x-cx]) continue;

							if (cave[y][x].m_idx) continue;
							if (player_bold(y, x)) continue;

							/* Count "safe" grids */
							sn++;

							/* Randomize choice */
							if (randint0(sn) > 0) continue;

							/* Save the safe grid */
							sy = y; sx = x;
						}
					}

					/* Describe the monster */
					monster_desc(m_name, m_ptr, 0);

					/* Scream in pain */
					if (!ignore_unview || is_seen(m_ptr)) msg_format(_("%^sは苦痛で泣きわめいた！", "%^s wails out in pain!"), m_name);

					/* Take damage from the quake */
					damage = (sn ? damroll(4, 8) : (m_ptr->hp + 1));

					/* Monster is certainly awake */
					(void)set_monster_csleep(c_ptr->m_idx, 0);

					/* Apply damage directly */
					m_ptr->hp -= damage;

					/* Delete (not kill) "dead" monsters */
					if (m_ptr->hp < 0)
					{
						if (!ignore_unview || is_seen(m_ptr)) 
							msg_format(_("%^sは岩石に埋もれてしまった！", "%^s is embedded in the rock!"), m_name);

						if (c_ptr->m_idx)
						{
							if (record_named_pet && is_pet(&m_list[c_ptr->m_idx]) && m_list[c_ptr->m_idx].nickname)
							{
								char m2_name[80];

								monster_desc(m2_name, m_ptr, MD_INDEF_VISIBLE);
								do_cmd_write_nikki(NIKKI_NAMED_PET, RECORD_NAMED_PET_EARTHQUAKE, m2_name);
							}
						}


						delete_monster(yy, xx);

						/* No longer safe */
						sn = 0;
					}

					/* Hack -- Escape from the rock */
					if (sn)
					{
						IDX m_idx_aux = cave[yy][xx].m_idx;

						/* Update the old location */
						cave[yy][xx].m_idx = 0;

						/* Update the new location */
						cave[sy][sx].m_idx = m_idx_aux;

						/* Move the monster */
						m_ptr->fy = sy;
						m_ptr->fx = sx;

						/* Update the monster (new location) */
						update_monster(m_idx, TRUE);

						/* Redraw the old grid */
						lite_spot(yy, xx);

						/* Redraw the new grid */
						lite_spot(sy, sx);
					}
				}
			}
		}
	}

	/* Lose monster light */
	clear_mon_lite();

	/* Examine the quaked region */
	for (dy = -r; dy <= r; dy++)
	{
		for (dx = -r; dx <= r; dx++)
		{
			/* Extract the location */
			yy = cy + dy;
			xx = cx + dx;

			/* Skip unaffected grids */
			if (!map[16+yy-cy][16+xx-cx]) continue;

			/* Access the cave grid */
			c_ptr = &cave[yy][xx];

			/* Paranoia -- never affect player */
/*			if (player_bold(yy, xx)) continue; */

			/* Destroy location (if valid) */
			if (cave_valid_bold(yy, xx))
			{
				/* Delete objects */
				delete_object(yy, xx);

				/* Wall (or floor) type */
				t = cave_have_flag_bold(yy, xx, FF_PROJECT) ? randint0(100) : 200;

				/* Granite */
				if (t < 20)
				{
					/* Create granite wall */
					cave_set_feat(yy, xx, feat_granite);
				}

				/* Quartz */
				else if (t < 70)
				{
					/* Create quartz vein */
					cave_set_feat(yy, xx, feat_quartz_vein);
				}

				/* Magma */
				else if (t < 100)
				{
					/* Create magma vein */
					cave_set_feat(yy, xx, feat_magma_vein);
				}

				/* Floor */
				else
				{
					/* Create floor */
					cave_set_feat(yy, xx, floor_type[randint0(100)]);
				}
			}
		}
	}


	/* Process "re-glowing" */
	for (dy = -r; dy <= r; dy++)
	{
		for (dx = -r; dx <= r; dx++)
		{
			/* Extract the location */
			yy = cy + dy;
			xx = cx + dx;

			/* Skip illegal grids */
			if (!in_bounds(yy, xx)) continue;

			/* Skip distant grids */
			if (distance(cy, cx, yy, xx) > r) continue;
			c_ptr = &cave[yy][xx];

			if (is_mirror_grid(c_ptr)) c_ptr->info |= CAVE_GLOW;
			else if (!(d_info[dungeon_type].flags1 & DF1_DARKNESS))
			{
				int ii, yyy, xxx;
				cave_type *cc_ptr;

				for (ii = 0; ii < 9; ii++)
				{
					yyy = yy + ddy_ddd[ii];
					xxx = xx + ddx_ddd[ii];
					if (!in_bounds2(yyy, xxx)) continue;
					cc_ptr = &cave[yyy][xxx];
					if (have_flag(f_info[get_feat_mimic(cc_ptr)].flags, FF_GLOW))
					{
						c_ptr->info |= CAVE_GLOW;
						break;
					}
				}
			}
		}
	}


	/* Mega-Hack -- Forget the view and lite */
	p_ptr->update |= (PU_UN_VIEW | PU_UN_LITE);

	p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MON_LITE | PU_MONSTERS);

	/* Update the health bar */
	p_ptr->redraw |= (PR_HEALTH | PR_UHEALTH);

	p_ptr->redraw |= (PR_MAP);

	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

	if (p_ptr->special_defense & NINJA_S_STEALTH)
	{
		if (cave[p_ptr->y][p_ptr->x].info & CAVE_GLOW) set_superstealth(FALSE);
	}

	/* Success */
	return (TRUE);
}

/*!
 * @brief 地震処理(プレイヤーの中心発動) /
 * Induce an "earthquake" of the given radius at the given location.
 * @return 効力があった場合TRUEを返す
 * @param cy 中心Y座標
 * @param cx 中心X座標
 * @param r 効果半径
 */
bool earthquake(POSITION cy, POSITION cx, POSITION r)
{
	return earthquake_aux(cy, cx, r, 0);
}

/*!
 * @brief ペット爆破処理 /
 * @return なし
 */
void discharge_minion(void)
{
	MONSTER_IDX i;
	bool okay = TRUE;

	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &m_list[i];
		if (!m_ptr->r_idx || !is_pet(m_ptr)) continue;
		if (m_ptr->nickname) okay = FALSE;
	}
	if (!okay || p_ptr->riding)
	{
		if (!get_check(_("本当に全ペットを爆破しますか？", "You will blast all pets. Are you sure? ")))
			return;
	}
	for (i = 1; i < m_max; i++)
	{
		HIT_POINT dam;
		monster_type *m_ptr = &m_list[i];
		monster_race *r_ptr;

		if (!m_ptr->r_idx || !is_pet(m_ptr)) continue;
		r_ptr = &r_info[m_ptr->r_idx];

		/* Uniques resist discharging */
		if (r_ptr->flags1 & RF1_UNIQUE)
		{
			char m_name[80];
			monster_desc(m_name, m_ptr, 0x00);
			msg_format(_("%sは爆破されるのを嫌がり、勝手に自分の世界へと帰った。", "%^s resists to be blasted, and run away."), m_name);
			delete_monster_idx(i);
			continue;
		}
		dam = m_ptr->maxhp / 2;
		if (dam > 100) dam = (dam-100)/2 + 100;
		if (dam > 400) dam = (dam-400)/2 + 400;
		if (dam > 800) dam = 800;
		project(i, 2+(r_ptr->level/20), m_ptr->fy,
			m_ptr->fx, dam, GF_PLASMA, 
			PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL, -1);

		if (record_named_pet && m_ptr->nickname)
		{
			char m_name[80];

			monster_desc(m_name, m_ptr, MD_INDEF_VISIBLE);
			do_cmd_write_nikki(NIKKI_NAMED_PET, RECORD_NAMED_PET_BLAST, m_name);
		}

		delete_monster_idx(i);
	}
}


/*!
 * @brief 部屋全体を照らすサブルーチン
 * @return なし
 * @details
 * <pre>
 * This routine clears the entire "temp" set.
 * This routine will Perma-Lite all "temp" grids.
 * This routine is used (only) by "lite_room()"
 * Dark grids are illuminated.
 * Also, process all affected monsters.
 *
 * SMART monsters always wake up when illuminated
 * NORMAL monsters wake up 1/4 the time when illuminated
 * STUPID monsters wake up 1/10 the time when illuminated
 * </pre>
 */
static void cave_temp_room_lite(void)
{
	int i;

	/* Clear them all */
	for (i = 0; i < temp_n; i++)
	{
		POSITION y = temp_y[i];
		POSITION x = temp_x[i];

		cave_type *c_ptr = &cave[y][x];

		/* No longer in the array */
		c_ptr->info &= ~(CAVE_TEMP);

		/* Update only non-CAVE_GLOW grids */
		/* if (c_ptr->info & (CAVE_GLOW)) continue; */

		/* Perma-Lite */
		c_ptr->info |= (CAVE_GLOW);

		/* Process affected monsters */
		if (c_ptr->m_idx)
		{
			int chance = 25;

			monster_type    *m_ptr = &m_list[c_ptr->m_idx];

			monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			update_monster(c_ptr->m_idx, FALSE);

			/* Stupid monsters rarely wake up */
			if (r_ptr->flags2 & (RF2_STUPID)) chance = 10;

			/* Smart monsters always wake up */
			if (r_ptr->flags2 & (RF2_SMART)) chance = 100;

			/* Sometimes monsters wake up */
			if (MON_CSLEEP(m_ptr) && (randint0(100) < chance))
			{
				/* Wake up! */
				(void)set_monster_csleep(c_ptr->m_idx, 0);

				/* Notice the "waking up" */
				if (m_ptr->ml)
				{
					char m_name[80];
					monster_desc(m_name, m_ptr, 0);
					msg_format(_("%^sが目を覚ました。", "%^s wakes up."), m_name);
				}
			}
		}

		/* Note */
		note_spot(y, x);

		lite_spot(y, x);

		update_local_illumination(y, x);
	}

	/* None left */
	temp_n = 0;
}



/*!
 * @brief 部屋全体を暗くするサブルーチン
 * @return なし
 * @details
 * <pre>
 * This routine clears the entire "temp" set.
 * This routine will "darken" all "temp" grids.
 * In addition, some of these grids will be "unmarked".
 * This routine is used (only) by "unlite_room()"
 * Also, process all affected monsters
 * </pre>
 */
static void cave_temp_room_unlite(void)
{
	int i;

	/* Clear them all */
	for (i = 0; i < temp_n; i++)
	{
		POSITION y = temp_y[i];
		POSITION x = temp_x[i];
		int j;

		cave_type *c_ptr = &cave[y][x];
		bool do_dark = !is_mirror_grid(c_ptr);

		/* No longer in the array */
		c_ptr->info &= ~(CAVE_TEMP);

		/* Darken the grid */
		if (do_dark)
		{
			if (dun_level || !is_daytime())
			{
				for (j = 0; j < 9; j++)
				{
					int by = y + ddy_ddd[j];
					int bx = x + ddx_ddd[j];

					if (in_bounds2(by, bx))
					{
						cave_type *cc_ptr = &cave[by][bx];

						if (have_flag(f_info[get_feat_mimic(cc_ptr)].flags, FF_GLOW))
						{
							do_dark = FALSE;
							break;
						}
					}
				}

				if (!do_dark) continue;
			}

			c_ptr->info &= ~(CAVE_GLOW);

			/* Hack -- Forget "boring" grids */
			if (!have_flag(f_info[get_feat_mimic(c_ptr)].flags, FF_REMEMBER))
			{
				/* Forget the grid */
				if (!view_torch_grids) c_ptr->info &= ~(CAVE_MARK);

				note_spot(y, x);
			}

			/* Process affected monsters */
			if (c_ptr->m_idx)
			{
				update_monster(c_ptr->m_idx, FALSE);
			}

			lite_spot(y, x);

			update_local_illumination(y, x);
		}
	}

	/* None left */
	temp_n = 0;
}


/*!
 * @brief 周辺に関数ポインタの条件に該当する地形がいくつあるかを計算する / Determine how much contiguous open space this grid is next to
 * @param cy Y座標
 * @param cx X座標
 * @param pass_bold 地形条件を返す関数ポインタ
 * @return 該当地形の数
 */
static int next_to_open(POSITION cy, POSITION cx, bool (*pass_bold)(POSITION, POSITION))
{
	int i;
	POSITION y, x;
	int len = 0;
	int blen = 0;

	for (i = 0; i < 16; i++)
	{
		y = cy + ddy_cdd[i % 8];
		x = cx + ddx_cdd[i % 8];

		/* Found a wall, break the length */
		if (!pass_bold(y, x))
		{
			/* Track best length */
			if (len > blen)
			{
				blen = len;
			}

			len = 0;
		}
		else
		{
			len++;
		}
	}

	return (MAX(len, blen));
}

/*!
 * @brief 周辺に関数ポインタの条件に該当する地形がいくつあるかを計算する / Determine how much contiguous open space this grid is next to
 * @param cy Y座標
 * @param cx X座標
 * @param pass_bold 地形条件を返す関数ポインタ
 * @return 該当地形の数
 */
static int next_to_walls_adj(POSITION cy, POSITION cx, bool (*pass_bold)(POSITION, POSITION))
{
	DIRECTION i;
	POSITION y, x;
	int c = 0;

	for (i = 0; i < 8; i++)
	{
		y = cy + ddy_ddd[i];
		x = cx + ddx_ddd[i];

		if (!pass_bold(y, x)) c++;
	}

	return c;
}


/*!
 * @brief 部屋内にある一点の周囲に該当する地形数かいくつあるかをグローバル変数temp_nに返す / Aux function -- see below
 * @param y 部屋内のy座標1点
 * @param x 部屋内のx座標1点
 * @param only_room 部屋内地形のみをチェック対象にするならば TRUE
 * @param pass_bold 地形条件を返す関数ポインタ
 * @return 該当地形の数
 */
static void cave_temp_room_aux(POSITION y, POSITION x, bool only_room, bool (*pass_bold)(POSITION, POSITION))
{
	cave_type *c_ptr;

	/* Get the grid */
	c_ptr = &cave[y][x];

	/* Avoid infinite recursion */
	if (c_ptr->info & (CAVE_TEMP)) return;

	/* Do not "leave" the current room */
	if (!(c_ptr->info & (CAVE_ROOM)))
	{
		if (only_room) return;

		/* Verify */
		if (!in_bounds2(y, x)) return;

		/* Do not exceed the maximum spell range */
		if (distance(p_ptr->y, p_ptr->x, y, x) > MAX_RANGE) return;

		/* Verify this grid */
		/*
		 * The reason why it is ==6 instead of >5 is that 8 is impossible
		 * due to the check for cave_bold above.
		 * 7 lights dead-end corridors (you need to do this for the
		 * checkboard interesting rooms, so that the boundary is lit
		 * properly.
		 * This leaves only a check for 6 bounding walls!
		 */
		if (in_bounds(y, x) && pass_bold(y, x) &&
		    (next_to_walls_adj(y, x, pass_bold) == 6) && (next_to_open(y, x, pass_bold) <= 1)) return;
	}

	/* Paranoia -- verify space */
	if (temp_n == TEMP_MAX) return;

	/* Mark the grid as "seen" */
	c_ptr->info |= (CAVE_TEMP);

	/* Add it to the "seen" set */
	temp_y[temp_n] = y;
	temp_x[temp_n] = x;
	temp_n++;
}

/*!
 * @brief 指定のマスが光を通すか(LOSフラグを持つか)を返す。 / Aux function -- see below
 * @param y 指定Y座標
 * @param x 指定X座標
 * @return 光を通すならばtrueを返す。
 */
static bool cave_pass_lite_bold(POSITION y, POSITION x)
{
	return cave_los_bold(y, x);
}

/*!
 * @brief 部屋内にある一点の周囲がいくつ光を通すかをグローバル変数temp_nに返す / Aux function -- see below
 * @param y 指定Y座標
 * @param x 指定X座標
 * @return なし
 */
static void cave_temp_lite_room_aux(POSITION y, POSITION x)
{
	cave_temp_room_aux(y, x, FALSE, cave_pass_lite_bold);
}

/*!
 * @brief 指定のマスが光を通さず射線のみを通すかを返す。 / Aux function -- see below
 * @param y 指定Y座標
 * @param x 指定X座標
 * @return 射線を通すならばtrueを返す。
 */
static bool cave_pass_dark_bold(POSITION y, POSITION x)
{
	return cave_have_flag_bold(y, x, FF_PROJECT);
}


/*!
 * @brief 部屋内にある一点の周囲がいくつ射線を通すかをグローバル変数temp_nに返す / Aux function -- see below
 * @param y 指定Y座標
 * @param x 指定X座標
 * @return なし
 */
static void cave_temp_unlite_room_aux(POSITION y, POSITION x)
{
	cave_temp_room_aux(y, x, TRUE, cave_pass_dark_bold);
}


/*!
 * @brief 指定された部屋内を照らす / Illuminate any room containing the given location.
 * @param y1 指定Y座標
 * @param x1 指定X座標
 * @return なし
 */
void lite_room(POSITION y1, POSITION x1)
{
	int i;
	POSITION x, y;

	/* Add the initial grid */
	cave_temp_lite_room_aux(y1, x1);

	/* While grids are in the queue, add their neighbors */
	for (i = 0; i < temp_n; i++)
	{
		x = temp_x[i], y = temp_y[i];

		/* Walls get lit, but stop light */
		if (!cave_pass_lite_bold(y, x)) continue;

		/* Spread adjacent */
		cave_temp_lite_room_aux(y + 1, x);
		cave_temp_lite_room_aux(y - 1, x);
		cave_temp_lite_room_aux(y, x + 1);
		cave_temp_lite_room_aux(y, x - 1);

		/* Spread diagonal */
		cave_temp_lite_room_aux(y + 1, x + 1);
		cave_temp_lite_room_aux(y - 1, x - 1);
		cave_temp_lite_room_aux(y - 1, x + 1);
		cave_temp_lite_room_aux(y + 1, x - 1);
	}

	/* Now, lite them all up at once */
	cave_temp_room_lite();

	if (p_ptr->special_defense & NINJA_S_STEALTH)
	{
		if (cave[p_ptr->y][p_ptr->x].info & CAVE_GLOW) set_superstealth(FALSE);
	}
}


/*!
 * @brief 指定された部屋内を暗くする / Darken all rooms containing the given location
 * @param y1 指定Y座標
 * @param x1 指定X座標
 * @return なし
 */
void unlite_room(POSITION y1, POSITION x1)
{
	int i;
	POSITION x, y;

	/* Add the initial grid */
	cave_temp_unlite_room_aux(y1, x1);

	/* Spread, breadth first */
	for (i = 0; i < temp_n; i++)
	{
		x = temp_x[i], y = temp_y[i];

		/* Walls get dark, but stop darkness */
		if (!cave_pass_dark_bold(y, x)) continue;

		/* Spread adjacent */
		cave_temp_unlite_room_aux(y + 1, x);
		cave_temp_unlite_room_aux(y - 1, x);
		cave_temp_unlite_room_aux(y, x + 1);
		cave_temp_unlite_room_aux(y, x - 1);

		/* Spread diagonal */
		cave_temp_unlite_room_aux(y + 1, x + 1);
		cave_temp_unlite_room_aux(y - 1, x - 1);
		cave_temp_unlite_room_aux(y - 1, x + 1);
		cave_temp_unlite_room_aux(y + 1, x - 1);
	}

	/* Now, darken them all at once */
	cave_temp_room_unlite();
}



/*!
 * @brief プレイヤー位置を中心にLITE_WEAK属性を通じた照明処理を行う / Hack -- call light around the player Affect all monsters in the projection radius
 * @param dam 威力
 * @param rad 効果半径
 * @return 作用が実際にあった場合TRUEを返す
 */
bool lite_area(HIT_POINT dam, POSITION rad)
{
	BIT_FLAGS flg = PROJECT_GRID | PROJECT_KILL;

	if (d_info[dungeon_type].flags1 & DF1_DARKNESS)
	{
		msg_print(_("ダンジョンが光を吸収した。", "The darkness of this dungeon absorb your light."));
		return FALSE;
	}

	/* Hack -- Message */
	if (!p_ptr->blind)
	{
		msg_print(_("白い光が辺りを覆った。", "You are surrounded by a white light."));
	}

	/* Hook into the "project()" function */
	(void)project(0, rad, p_ptr->y, p_ptr->x, dam, GF_LITE_WEAK, flg, -1);

	/* Lite up the room */
	lite_room(p_ptr->y, p_ptr->x);

	/* Assume seen */
	return (TRUE);
}


/*!
 * @brief プレイヤー位置を中心にLITE_DARK属性を通じた消灯処理を行う / Hack -- call light around the player Affect all monsters in the projection radius
 * @param dam 威力
 * @param rad 効果半径
 * @return 作用が実際にあった場合TRUEを返す
 */
bool unlite_area(HIT_POINT dam, POSITION rad)
{
	BIT_FLAGS flg = PROJECT_GRID | PROJECT_KILL;

	/* Hack -- Message */
	if (!p_ptr->blind)
	{
		msg_print(_("暗闇が辺りを覆った。", "Darkness surrounds you."));
	}

	/* Hook into the "project()" function */
	(void)project(0, rad, p_ptr->y, p_ptr->x, dam, GF_DARK_WEAK, flg, -1);

	/* Lite up the room */
	unlite_room(p_ptr->y, p_ptr->x);

	/* Assume seen */
	return (TRUE);
}



/*!
 * @brief ボール系スペルの発動 / Cast a ball spell
 * @param typ 効果属性
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @param rad 半径
 * @return 作用が実際にあった場合TRUEを返す
 * @details
 * <pre>
 * Stop if we hit a monster, act as a "ball"
 * Allow "target" mode to pass over monsters
 * Affect grids, objects, and monsters
 * </pre>
 */
bool fire_ball(EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, POSITION rad)
{
	POSITION tx, ty;

	BIT_FLAGS flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;

	if (typ == GF_CHARM_LIVING) flg|= PROJECT_HIDE;
	/* Use the given direction */
	tx = p_ptr->x + 99 * ddx[dir];
	ty = p_ptr->y + 99 * ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay())
	{
		flg &= ~(PROJECT_STOP);
		tx = target_col;
		ty = target_row;
	}

	/* Analyze the "dir" and the "target".  Hurt items on floor. */
	return (project(0, rad, ty, tx, dam, typ, flg, -1));
}

/*!
* @brief ブレス系スペルの発動 / Cast a breath spell
* @param typ 効果属性
* @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
* @param dam 威力
* @param rad 半径
* @return 作用が実際にあった場合TRUEを返す
* @details
* <pre>
* Stop if we hit a monster, act as a "ball"
* Allow "target" mode to pass over monsters
* Affect grids, objects, and monsters
* </pre>
*/
bool fire_breath(EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, POSITION rad)
{
	return fire_ball(typ, dir, dam, -rad);
}


/*!
 * @brief ロケット系スペルの発動(詳細な差は確認中) / Cast a ball spell
 * @param typ 効果属性
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @param rad 半径
 * @return 作用が実際にあった場合TRUEを返す
 * @details
 * <pre>
 * Stop if we hit a monster, act as a "ball"
 * Allow "target" mode to pass over monsters
 * Affect grids, objects, and monsters
 * </pre>
 */
bool fire_rocket(EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, POSITION rad)
{
	POSITION tx, ty;
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;

	/* Use the given direction */
	tx = p_ptr->x + 99 * ddx[dir];
	ty = p_ptr->y + 99 * ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay())
	{
		tx = target_col;
		ty = target_row;
	}

	/* Analyze the "dir" and the "target".  Hurt items on floor. */
	return (project(0, rad, ty, tx, dam, typ, flg, -1));
}


/*!
 * @brief ボール(ハイド)系スペルの発動 / Cast a ball spell
 * @param typ 効果属性
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @param rad 半径
 * @return 作用が実際にあった場合TRUEを返す
 * @details
 * <pre>
 * Stop if we hit a monster, act as a "ball"
 * Allow "target" mode to pass over monsters
 * Affect grids, objects, and monsters
 * </pre>
 */
bool fire_ball_hide(EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, POSITION rad)
{
	POSITION tx, ty;
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_HIDE;

	/* Use the given direction */
	tx = p_ptr->x + 99 * ddx[dir];
	ty = p_ptr->y + 99 * ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay())
	{
		flg &= ~(PROJECT_STOP);
		tx = target_col;
		ty = target_row;
	}

	/* Analyze the "dir" and the "target".  Hurt items on floor. */
	return (project(0, rad, ty, tx, dam, typ, flg, -1));
}


/*!
 * @brief メテオ系スペルの発動 / Cast a meteor spell
 * @param who スぺル詠唱者のモンスターID(0=プレイヤー)
 * @param typ 効果属性
 * @param dam 威力
 * @param rad 半径
 * @param y 中心点Y座標
 * @param x 中心点X座標
 * @return 作用が実際にあった場合TRUEを返す
 * @details
 * <pre>
 * Cast a meteor spell, defined as a ball spell cast by an arbitary monster, 
 * player, or outside source, that starts out at an arbitrary location, and 
 * leaving no trail from the "caster" to the target.  This function is 
 * especially useful for bombardments and similar. -LM-
 * Option to hurt the player.
 * </pre>
 */
bool fire_meteor(MONSTER_IDX who, EFFECT_ID typ, POSITION y, POSITION x, HIT_POINT dam, POSITION rad)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;

	/* Analyze the "target" and the caster. */
	return (project(who, rad, y, x, dam, typ, flg, -1));
}


/*!
 * @brief ブラスト系スペルの発動 / Cast a blast spell
 * @param typ 効果属性
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dd 威力ダイス数
 * @param ds 威力ダイス目
 * @param num 基本回数
 * @param dev 回数分散
 * @return 作用が実際にあった場合TRUEを返す
 */
bool fire_blast(EFFECT_ID typ, DIRECTION dir, int dd, int ds, int num, int dev)
{
	POSITION ly, lx;
	int ld;
	POSITION ty, tx, y, x;
	int i;

	BIT_FLAGS flg = PROJECT_FAST | PROJECT_THRU | PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE | PROJECT_GRID;

	/* Assume okay */
	bool result = TRUE;

	/* Use the given direction */
	if (dir != 5)
	{
		ly = ty = p_ptr->y + 20 * ddy[dir];
		lx = tx = p_ptr->x + 20 * ddx[dir];
	}

	/* Use an actual "target" */
	else /* if (dir == 5) */
	{
		tx = target_col;
		ty = target_row;

		lx = 20 * (tx - p_ptr->x) + p_ptr->x;
		ly = 20 * (ty - p_ptr->y) + p_ptr->y;
	}

	ld = distance(p_ptr->y, p_ptr->x, ly, lx);

	/* Blast */
	for (i = 0; i < num; i++)
	{
		while (1)
		{
			/* Get targets for some bolts */
			y = rand_spread(ly, ld * dev / 20);
			x = rand_spread(lx, ld * dev / 20);

			if (distance(ly, lx, y, x) <= ld * dev / 20) break;
		}

		/* Analyze the "dir" and the "target". */
		if (!project(0, 0, y, x, damroll(dd, ds), typ, flg, -1))
		{
			result = FALSE;
		}
	}

	return (result);
}


/*!
 * @brief モンスターとの位置交換処理 / Switch position with a monster.
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool teleport_swap(DIRECTION dir)
{
	POSITION tx, ty;
	cave_type* c_ptr;
	monster_type* m_ptr;
	monster_race* r_ptr;

	if ((dir == 5) && target_okay())
	{
		tx = target_col;
		ty = target_row;
	}
	else
	{
		tx = p_ptr->x + ddx[dir];
		ty = p_ptr->y + ddy[dir];
	}
	c_ptr = &cave[ty][tx];

	if (p_ptr->anti_tele)
	{
		msg_print(_("不思議な力がテレポートを防いだ！", "A mysterious force prevents you from teleporting!"));
		return FALSE;
	}

	if (!c_ptr->m_idx || (c_ptr->m_idx == p_ptr->riding))
	{
		msg_print(_("それとは場所を交換できません。", "You can't trade places with that!"));

		/* Failure */
		return FALSE;
	}

	if ((c_ptr->info & CAVE_ICKY) || (distance(ty, tx, p_ptr->y, p_ptr->x) > p_ptr->lev * 3 / 2 + 10))
	{
		msg_print(_("失敗した。", "Failed to swap."));

		/* Failure */
		return FALSE;
	}

	m_ptr = &m_list[c_ptr->m_idx];
	r_ptr = &r_info[m_ptr->r_idx];

	(void)set_monster_csleep(c_ptr->m_idx, 0);

	if (r_ptr->flagsr & RFR_RES_TELE)
	{
		msg_print(_("テレポートを邪魔された！", "Your teleportation is blocked!"));

		if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= RFR_RES_TELE;

		/* Failure */
		return FALSE;
	}

	sound(SOUND_TELEPORT);

	/* Swap the player and monster */
	(void)move_player_effect(ty, tx, MPE_FORGET_FLOW | MPE_HANDLE_STUFF | MPE_DONT_PICKUP);

	/* Success */
	return TRUE;
}


/*!
 * @brief 指定方向に飛び道具を飛ばす（フラグ任意指定） / Hack -- apply a "projection()" in a direction (or at the target)
 * @param typ 効果属性
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @param flg フラグ
 * @return 作用が実際にあった場合TRUEを返す
 */
bool project_hook(EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, BIT_FLAGS flg)
{
	int tx, ty;

	/* Pass through the target if needed */
	flg |= (PROJECT_THRU);

	/* Use the given direction */
	tx = p_ptr->x + ddx[dir];
	ty = p_ptr->y + ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay())
	{
		tx = target_col;
		ty = target_row;
	}

	/* Analyze the "dir" and the "target", do NOT explode */
	return (project(0, 0, ty, tx, dam, typ, flg, -1));
}


/*!
 * @brief ボルト系スペルの発動 / Cast a bolt spell.
 * @param typ 効果属性
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @return 作用が実際にあった場合TRUEを返す
 * @details
 * <pre>
 * Stop if we hit a monster, as a "bolt".
 * Affect monsters and grids (not objects).
 * </pre>
 */
bool fire_bolt(EFFECT_ID typ, DIRECTION dir, HIT_POINT dam)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_GRID;
	if (typ != GF_ARROW) flg |= PROJECT_REFLECTABLE;
	return (project_hook(typ, dir, dam, flg));
}


/*!
 * @brief ビーム系スペルの発動 / Cast a beam spell.
 * @param typ 効果属性
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @return 作用が実際にあった場合TRUEを返す
 * @details
 * <pre>
 * Pass through monsters, as a "beam".
 * Affect monsters, grids and objects.
 * </pre>
 */
bool fire_beam(EFFECT_ID typ, DIRECTION dir, HIT_POINT dam)
{
	BIT_FLAGS flg = PROJECT_BEAM | PROJECT_KILL | PROJECT_GRID | PROJECT_ITEM;
	return (project_hook(typ, dir, dam, flg));
}


/*!
 * @brief 確率に応じたボルト系/ビーム系スペルの発動 / Cast a bolt spell, or rarely, a beam spell.
 * @param prob ビーム化する確率(%)
 * @param typ 効果属性
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @return 作用が実際にあった場合TRUEを返す
 * @details
 * <pre>
 * Pass through monsters, as a "beam".
 * Affect monsters, grids and objects.
 * </pre>
 */
bool fire_bolt_or_beam(PERCENTAGE prob, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam)
{
	if (randint0(100) < prob)
	{
		return (fire_beam(typ, dir, dam));
	}
	else
	{
		return (fire_bolt(typ, dir, dam));
	}
}

/*!
 * @brief LITE_WEAK属性による光源ビーム処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool lite_line(DIRECTION dir, HIT_POINT dam)
{
	BIT_FLAGS flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_KILL;
	return (project_hook(GF_LITE_WEAK, dir, dam, flg));
}

/*!
 * @brief 衰弱ボルト処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool hypodynamic_bolt(DIRECTION dir, HIT_POINT dam)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_HYPODYNAMIA, dir, dam, flg));
}

/*!
 * @brief 岩石溶解処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool wall_to_mud(DIRECTION dir, HIT_POINT dam)
{
	BIT_FLAGS flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
	return (project_hook(GF_KILL_WALL, dir, dam, flg));
}

/*!
 * @brief 魔法の施錠処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool wizard_lock(DIRECTION dir)
{
	BIT_FLAGS flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
	return (project_hook(GF_JAM_DOOR, dir, 20 + randint1(30), flg));
}

/*!
 * @brief ドア破壊処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool destroy_door(DIRECTION dir)
{
	BIT_FLAGS flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM;
	return (project_hook(GF_KILL_DOOR, dir, 0, flg));
}

/*!
 * @brief トラップ解除処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool disarm_trap(DIRECTION dir)
{
	BIT_FLAGS flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM;
	return (project_hook(GF_KILL_TRAP, dir, 0, flg));
}

/*!
 * @brief モンスター回復処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool heal_monster(DIRECTION dir, HIT_POINT dam)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_OLD_HEAL, dir, dam, flg));
}

/*!
 * @brief モンスター加速処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param power 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool speed_monster(DIRECTION dir, int power)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_OLD_SPEED, dir, power, flg));
}

/*!
 * @brief モンスター減速処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param power 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool slow_monster(DIRECTION dir, int power)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_OLD_SLOW, dir, power, flg));
}

/*!
 * @brief モンスター催眠処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param power 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool sleep_monster(DIRECTION dir, int power)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_OLD_SLEEP, dir, power, flg));
}

/*!
 * @brief モンスター拘束(STASIS)処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 * @details 威力はプレイヤーレベル*2に固定
 */
bool stasis_monster(DIRECTION dir)
{
	return (fire_ball_hide(GF_STASIS, dir, p_ptr->lev*2, 0));
}

/*!
 * @brief 邪悪なモンスター拘束(STASIS)処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 * @details 威力はプレイヤーレベル*2に固定
 */
bool stasis_evil(DIRECTION dir)
{
	return (fire_ball_hide(GF_STASIS_EVIL, dir, p_ptr->lev*2, 0));
}

/*!
 * @brief モンスター混乱処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev プレイヤーレベル(=効力)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool confuse_monster(DIRECTION dir, PLAYER_LEVEL plev)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_OLD_CONF, dir, plev, flg));
}

/*!
 * @brief モンスター朦朧処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev プレイヤーレベル(=効力)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool stun_monster(DIRECTION dir, PLAYER_LEVEL plev)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_STUN, dir, plev, flg));
}

/*!
 * @brief チェンジモンスター処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param power 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool poly_monster(DIRECTION dir, int power)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	bool tester = (project_hook(GF_OLD_POLY, dir, power, flg));
	if (tester)
		chg_virtue(V_CHANCE, 1);
	return(tester);
}

/*!
 * @brief クローンモンスター処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool clone_monster(DIRECTION dir)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_OLD_CLONE, dir, 0, flg));
}

/*!
 * @brief モンスター恐慌処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev プレイヤーレベル(=効力)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool fear_monster(DIRECTION dir, PLAYER_LEVEL plev)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_TURN_ALL, dir, plev, flg));
}

/*!
 * @brief 死の光線処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev プレイヤーレベル(効力はplev*200)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool death_ray(DIRECTION dir, PLAYER_LEVEL plev)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(GF_DEATH_RAY, dir, plev * 200, flg));
}

/*!
 * @brief モンスター用テレポート処理
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param distance 移動距離
 * @return 作用が実際にあった場合TRUEを返す
 */
bool teleport_monster(DIRECTION dir, int distance)
{
	BIT_FLAGS flg = PROJECT_BEAM | PROJECT_KILL;
	return (project_hook(GF_AWAY_ALL, dir, distance, flg));
}

/*!
 * @brief ドア生成処理(プレイヤー中心に周囲1マス) / Hooks -- affect adjacent grids (radius 1 ball attack)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool door_creation(void)
{
	BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
	return (project(0, 1, p_ptr->y, p_ptr->x, 0, GF_MAKE_DOOR, flg, -1));
}

/*!
 * @brief トラップ生成処理(起点から周囲1マス)
 * @param y 起点Y座標
 * @param x 起点X座標
 * @return 作用が実際にあった場合TRUEを返す
 */
bool trap_creation(POSITION y, POSITION x)
{
	BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
	return (project(0, 1, y, x, 0, GF_MAKE_TRAP, flg, -1));
}

/*!
 * @brief 森林生成処理(プレイヤー中心に周囲1マス)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool tree_creation(void)
{
	BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
	return (project(0, 1, p_ptr->y, p_ptr->x, 0, GF_MAKE_TREE, flg, -1));
}

/*!
 * @brief 魔法のルーン生成処理(プレイヤー中心に周囲1マス)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool glyph_creation(void)
{
	BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM;
	return (project(0, 1, p_ptr->y, p_ptr->x, 0, GF_MAKE_GLYPH, flg, -1));
}

/*!
 * @brief 壁生成処理(プレイヤー中心に周囲1マス)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool wall_stone(void)
{
	BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;

	bool dummy = (project(0, 1, p_ptr->y, p_ptr->x, 0, GF_STONE_WALL, flg, -1));

	p_ptr->update |= (PU_FLOW);

	p_ptr->redraw |= (PR_MAP);

	return dummy;
}

/*!
 * @brief ドア破壊処理(プレイヤー中心に周囲1マス)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool destroy_doors_touch(void)
{
	BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
	return (project(0, 1, p_ptr->y, p_ptr->x, 0, GF_KILL_DOOR, flg, -1));
}

/*!
 * @brief トラップ解除処理(プレイヤー中心に周囲1マス)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool disarm_traps_touch(void)
{
	BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
	return (project(0, 1, p_ptr->y, p_ptr->x, 0, GF_KILL_TRAP, flg, -1));
}

/*!
 * @brief スリープモンスター処理(プレイヤー中心に周囲1マス)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool sleep_monsters_touch(void)
{
	BIT_FLAGS flg = PROJECT_KILL | PROJECT_HIDE;
	return (project(0, 1, p_ptr->y, p_ptr->x, p_ptr->lev, GF_OLD_SLEEP, flg, -1));
}


/*!
 * @brief 死者復活処理(起点より周囲5マス)
 * @param who 術者モンスターID(0ならばプレイヤー)
 * @param y 起点Y座標
 * @param x 起点X座標
 * @return 作用が実際にあった場合TRUEを返す
 */
bool animate_dead(MONSTER_IDX who, POSITION y, POSITION x)
{
	BIT_FLAGS flg = PROJECT_ITEM | PROJECT_HIDE;
	return (project(who, 5, y, x, 0, GF_ANIM_DEAD, flg, -1));
}

/*!
 * @brief 混沌招来処理
 * @return 作用が実際にあった場合TRUEを返す
 */
void call_chaos(void)
{
	int Chaos_type, dummy, dir;
	PLAYER_LEVEL plev = p_ptr->lev;
	bool line_chaos = FALSE;

	int hurt_types[31] =
	{
		GF_ELEC,      GF_POIS,    GF_ACID,    GF_COLD,
		GF_FIRE,      GF_MISSILE, GF_ARROW,   GF_PLASMA,
		GF_HOLY_FIRE, GF_WATER,   GF_LITE,    GF_DARK,
		GF_FORCE,     GF_INERTIAL, GF_MANA,    GF_METEOR,
		GF_ICE,       GF_CHAOS,   GF_NETHER,  GF_DISENCHANT,
		GF_SHARDS,    GF_SOUND,   GF_NEXUS,   GF_CONFUSION,
		GF_TIME,      GF_GRAVITY, GF_ROCKET,  GF_NUKE,
		GF_HELL_FIRE, GF_DISINTEGRATE, GF_PSY_SPEAR
	};

	Chaos_type = hurt_types[randint0(31)];
	if (one_in_(4)) line_chaos = TRUE;

	if (one_in_(6))
	{
		for (dummy = 1; dummy < 10; dummy++)
		{
			if (dummy - 5)
			{
				if (line_chaos)
					fire_beam(Chaos_type, dummy, 150);
				else
					fire_ball(Chaos_type, dummy, 150, 2);
			}
		}
	}
	else if (one_in_(3))
	{
		fire_ball(Chaos_type, 0, 500, 8);
	}
	else
	{
		if (!get_aim_dir(&dir)) return;
		if (line_chaos)
			fire_beam(Chaos_type, dir, 250);
		else
			fire_ball(Chaos_type, dir, 250, 3 + (plev / 35));
	}
}

/*!
 * @brief TY_CURSE処理発動 / Activate the evil Topi Ylinen curse
 * @param stop_ty 再帰処理停止フラグ
 * @param count 発動回数
 * @return 作用が実際にあった場合TRUEを返す
 * @details
 * <pre>
 * rr9: Stop the nasty things when a Cyberdemon is summoned
 * or the player gets paralyzed.
 * </pre>
 */
bool activate_ty_curse(bool stop_ty, int *count)
{
	int     i = 0;

	BIT_FLAGS flg = (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP);

	do
	{
		switch (randint1(34))
		{
		case 28: case 29:
			if (!(*count))
			{
				msg_print(_("地面が揺れた...", "The ground trembles..."));
				earthquake(p_ptr->y, p_ptr->x, 5 + randint0(10));
				if (!one_in_(6)) break;
			}
		case 30: case 31:
			if (!(*count))
			{
				HIT_POINT dam = damroll(10, 10);
				msg_print(_("純粋な魔力の次元への扉が開いた！", "A portal opens to a plane of raw mana!"));
				project(0, 8, p_ptr->y, p_ptr->x, dam, GF_MANA, flg, -1);
				take_hit(DAMAGE_NOESCAPE, dam, _("純粋な魔力の解放", "released pure mana"), -1);
				if (!one_in_(6)) break;
			}
		case 32: case 33:
			if (!(*count))
			{
				msg_print(_("周囲の空間が歪んだ！", "Space warps about you!"));
				teleport_player(damroll(10, 10), TELEPORT_PASSIVE);
				if (randint0(13)) (*count) += activate_hi_summon(p_ptr->y, p_ptr->x, FALSE);
				if (!one_in_(6)) break;
			}
		case 34:
			msg_print(_("エネルギーのうねりを感じた！", "You feel a surge of energy!"));
			wall_breaker();
			if (!randint0(7))
			{
				project(0, 7, p_ptr->y, p_ptr->x, 50, GF_KILL_WALL, flg, -1);
				take_hit(DAMAGE_NOESCAPE, 50, _("エネルギーのうねり", "surge of energy"), -1);
			}
			if (!one_in_(6)) break;
		case 1: case 2: case 3: case 16: case 17:
			aggravate_monsters(0);
			if (!one_in_(6)) break;
		case 4: case 5: case 6:
			(*count) += activate_hi_summon(p_ptr->y, p_ptr->x, FALSE);
			if (!one_in_(6)) break;
		case 7: case 8: case 9: case 18:
			(*count) += summon_specific(0, p_ptr->y, p_ptr->x, dun_level, 0, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET));
			if (!one_in_(6)) break;
		case 10: case 11: case 12:
			msg_print(_("経験値が体から吸い取られた気がする！", "You feel your experience draining away..."));
			lose_exp(p_ptr->exp / 16);
			if (!one_in_(6)) break;
		case 13: case 14: case 15: case 19: case 20:
			if (stop_ty || (p_ptr->free_act && (randint1(125) < p_ptr->skill_sav)) || (p_ptr->pclass == CLASS_BERSERKER))
			{
				/* Do nothing */ ;
			}
			else
			{
				msg_print(_("彫像になった気分だ！", "You feel like a statue!"));
				if (p_ptr->free_act)
					set_paralyzed(p_ptr->paralyzed + randint1(3));
				else
					set_paralyzed(p_ptr->paralyzed + randint1(13));
				stop_ty = TRUE;
			}
			if (!one_in_(6)) break;
		case 21: case 22: case 23:
			(void)do_dec_stat(randint0(6));
			if (!one_in_(6)) break;
		case 24:
			msg_print(_("ほえ？私は誰？ここで何してる？", "Huh? Who am I? What am I doing here?"));
			lose_all_info();
			if (!one_in_(6)) break;
		case 25:
			/*
			 * Only summon Cyberdemons deep in the dungeon.
			 */
			if ((dun_level > 65) && !stop_ty)
			{
				(*count) += summon_cyber(-1, p_ptr->y, p_ptr->x);
				stop_ty = TRUE;
				break;
			}
			if (!one_in_(6)) break;
		default:
			while (i < 6)
			{
				do
				{
					(void)do_dec_stat(i);
				}
				while (one_in_(2));

				i++;
			}
		}
	}
	while (one_in_(3) && !stop_ty);

	return stop_ty;
}

/*!
 * @brief HI_SUMMON(上級召喚)処理発動
 * @param y 召喚位置Y座標
 * @param x 召喚位置X座標
 * @param can_pet プレイヤーのペットとなる可能性があるならばTRUEにする
 * @return 作用が実際にあった場合TRUEを返す
 */
int activate_hi_summon(POSITION y, POSITION x, bool can_pet)
{
	int i;
	int count = 0;
	DEPTH summon_lev;
	BIT_FLAGS mode = PM_ALLOW_GROUP;
	bool pet = FALSE;

	if (can_pet)
	{
		if (one_in_(4))
		{
			mode |= PM_FORCE_FRIENDLY;
		}
		else
		{
			mode |= PM_FORCE_PET;
			pet = TRUE;
		}
	}

	if (!pet) mode |= PM_NO_PET;

	summon_lev = (pet ? p_ptr->lev * 2 / 3 + randint1(p_ptr->lev / 2) : dun_level);

	for (i = 0; i < (randint1(7) + (dun_level / 40)); i++)
	{
		switch (randint1(25) + (dun_level / 20))
		{
			case 1: case 2:
				count += summon_specific((pet ? -1 : 0), y, x, summon_lev, SUMMON_ANT, mode);
				break;
			case 3: case 4:
				count += summon_specific((pet ? -1 : 0), y, x, summon_lev, SUMMON_SPIDER, mode);
				break;
			case 5: case 6:
				count += summon_specific((pet ? -1 : 0), y, x, summon_lev, SUMMON_HOUND, mode);
				break;
			case 7: case 8:
				count += summon_specific((pet ? -1 : 0), y, x, summon_lev, SUMMON_HYDRA, mode);
				break;
			case 9: case 10:
				count += summon_specific((pet ? -1 : 0), y, x, summon_lev, SUMMON_ANGEL, mode);
				break;
			case 11: case 12:
				count += summon_specific((pet ? -1 : 0), y, x, summon_lev, SUMMON_UNDEAD, mode);
				break;
			case 13: case 14:
				count += summon_specific((pet ? -1 : 0), y, x, summon_lev, SUMMON_DRAGON, mode);
				break;
			case 15: case 16:
				count += summon_specific((pet ? -1 : 0), y, x, summon_lev, SUMMON_DEMON, mode);
				break;
			case 17:
				if (can_pet) break;
				count += summon_specific((pet ? -1 : 0), y, x, summon_lev, SUMMON_AMBERITES, (mode | PM_ALLOW_UNIQUE));
				break;
			case 18: case 19:
				if (can_pet) break;
				count += summon_specific((pet ? -1 : 0), y, x, summon_lev, SUMMON_UNIQUE, (mode | PM_ALLOW_UNIQUE));
				break;
			case 20: case 21:
				if (!can_pet) mode |= PM_ALLOW_UNIQUE;
				count += summon_specific((pet ? -1 : 0), y, x, summon_lev, SUMMON_HI_UNDEAD, mode);
				break;
			case 22: case 23:
				if (!can_pet) mode |= PM_ALLOW_UNIQUE;
				count += summon_specific((pet ? -1 : 0), y, x, summon_lev, SUMMON_HI_DRAGON, mode);
				break;
			case 24:
				count += summon_specific((pet ? -1 : 0), y, x, 100, SUMMON_CYBER, mode);
				break;
			default:
				if (!can_pet) mode |= PM_ALLOW_UNIQUE;
				count += summon_specific((pet ? -1 : 0), y, x,pet ? summon_lev : (((summon_lev * 3) / 2) + 5), 0, mode);
		}
	}

	return count;
}


/*!
 * @brief サイバーデーモンの召喚
 * @param who 召喚主のモンスターID(0ならばプレイヤー)
 * @param y 召喚位置Y座標
 * @param x 召喚位置X座標
 * @return 作用が実際にあった場合TRUEを返す
 */
int summon_cyber(MONSTER_IDX who, POSITION y, POSITION x)
{
	int i;
	int max_cyber = (easy_band ? 1 : (dun_level / 50) + randint1(2));
	int count = 0;
	BIT_FLAGS mode = PM_ALLOW_GROUP;

	/* Summoned by a monster */
	if (who > 0)
	{
		monster_type *m_ptr = &m_list[who];
		if (is_pet(m_ptr)) mode |= PM_FORCE_PET;
	}

	if (max_cyber > 4) max_cyber = 4;

	for (i = 0; i < max_cyber; i++)
	{
		count += summon_specific(who, y, x, 100, SUMMON_CYBER, mode);
	}

	return count;
}

/*!
 * @brief 周辺破壊効果(プレイヤー中心)
 * @return 作用が実際にあった場合TRUEを返す
 */
void wall_breaker(void)
{
	int i;
	POSITION y = 0, x = 0;
	int attempts = 1000;

	if (randint1(80 + p_ptr->lev) < 70)
	{
		while (attempts--)
		{
			scatter(&y, &x, p_ptr->y, p_ptr->x, 4, 0);

			if (!cave_have_flag_bold(y, x, FF_PROJECT)) continue;

			if (!player_bold(y, x)) break;
		}

		project(0, 0, y, x, 20 + randint1(30), GF_KILL_WALL,
				  (PROJECT_BEAM | PROJECT_THRU | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL), -1);
	}
	else if (randint1(100) > 30)
	{
		earthquake(p_ptr->y, p_ptr->x, 1);
	}
	else
	{
		int num = damroll(5, 3);

		for (i = 0; i < num; i++)
		{
			while (1)
			{
				scatter(&y, &x, p_ptr->y, p_ptr->x, 10, 0);

				if (!player_bold(y, x)) break;
			}

			project(0, 0, y, x, 20 + randint1(30), GF_KILL_WALL,
					  (PROJECT_BEAM | PROJECT_THRU | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL), -1);
		}
	}
}


/*!
 * @brief パニック・モンスター効果(プレイヤー視界範囲内) / Confuse monsters
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool confuse_monsters(HIT_POINT dam)
{
	return (project_hack(GF_OLD_CONF, dam));
}


/*!
 * @brief チャーム・モンスター効果(プレイヤー視界範囲内) / Charm monsters
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool charm_monsters(HIT_POINT dam)
{
	return (project_hack(GF_CHARM, dam));
}


/*!
 * @brief 動物魅了効果(プレイヤー視界範囲内) / Charm Animals
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool charm_animals(HIT_POINT dam)
{
	return (project_hack(GF_CONTROL_ANIMAL, dam));
}


/*!
 * @brief モンスター朦朧効果(プレイヤー視界範囲内) / Stun monsters
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool stun_monsters(HIT_POINT dam)
{
	return (project_hack(GF_STUN, dam));
}


/*!
 * @brief モンスター停止効果(プレイヤー視界範囲内) / Stasis monsters
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool stasis_monsters(HIT_POINT dam)
{
	return (project_hack(GF_STASIS, dam));
}


/*!
 * @brief モンスター精神攻撃効果(プレイヤー視界範囲内) / Mindblast monsters
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool mindblast_monsters(HIT_POINT dam)
{
	return (project_hack(GF_PSI, dam));
}


/*!
 * @brief モンスター追放効果(プレイヤー視界範囲内) / Banish all monsters
 * @param dist 効力（距離）
 * @return 作用が実際にあった場合TRUEを返す
 */
bool banish_monsters(int dist)
{
	return (project_hack(GF_AWAY_ALL, dist));
}


/*!
 * @brief 邪悪退散効果(プレイヤー視界範囲内) / Turn evil
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool turn_evil(HIT_POINT dam)
{
	return (project_hack(GF_TURN_EVIL, dam));
}


/*!
 * @brief 全モンスター退散効果(プレイヤー視界範囲内) / Turn everyone
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool turn_monsters(HIT_POINT dam)
{
	return (project_hack(GF_TURN_ALL, dam));
}


/*!
 * @brief 死の光線(プレイヤー視界範囲内) / Death-ray all monsters (note: OBSCENELY powerful)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool deathray_monsters(void)
{
	return (project_hack(GF_DEATH_RAY, p_ptr->lev * 200));
}

/*!
 * @brief チャーム・モンスター(1体)
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev パワー
 * @return 作用が実際にあった場合TRUEを返す
 */
bool charm_monster(DIRECTION dir, PLAYER_LEVEL plev)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(GF_CHARM, dir, plev, flg));
}

/*!
 * @brief アンデッド支配(1体)
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev パワー
 * @return 作用が実際にあった場合TRUEを返す
 */
bool control_one_undead(DIRECTION dir, PLAYER_LEVEL plev)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(GF_CONTROL_UNDEAD, dir, plev, flg));
}

/*!
 * @brief 悪魔支配(1体)
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev パワー
 * @return 作用が実際にあった場合TRUEを返す
 */
bool control_one_demon(DIRECTION dir, PLAYER_LEVEL plev)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(GF_CONTROL_DEMON, dir, plev, flg));
}

/*!
 * @brief 動物支配(1体)
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev パワー
 * @return 作用が実際にあった場合TRUEを返す
 */
bool charm_animal(DIRECTION dir, PLAYER_LEVEL plev)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(GF_CONTROL_ANIMAL, dir, plev, flg));
}


/*!
 * @brief 変わり身処理
 * @param success 判定成功上の処理ならばTRUE
 * @return 作用が実際にあった場合TRUEを返す
 */
bool kawarimi(bool success)
{
	object_type forge;
	object_type *q_ptr = &forge;
	POSITION y, x;

	if (p_ptr->is_dead) return FALSE;
	if (p_ptr->confused || p_ptr->blind || p_ptr->paralyzed || p_ptr->image) return FALSE;
	if (randint0(200) < p_ptr->stun) return FALSE;

	if (!success && one_in_(3))
	{
		msg_print(_("失敗！逃げられなかった。", "Failed! You couldn't run away."));
		p_ptr->special_defense &= ~(NINJA_KAWARIMI);
		p_ptr->redraw |= (PR_STATUS);
		return FALSE;
	}

	y = p_ptr->y;
	x = p_ptr->x;

	teleport_player(10 + randint1(90), 0L);

	object_wipe(q_ptr);

	object_prep(q_ptr, lookup_kind(TV_STATUE, SV_WOODEN_STATUE));

	q_ptr->pval = MON_NINJA;

	/* Drop it in the dungeon */
	(void)drop_near(q_ptr, -1, y, x);

#ifdef JP
	if (success) msg_print("攻撃を受ける前に素早く身をひるがえした。");
	else msg_print("失敗！攻撃を受けてしまった。");
#else
	if (success) msg_print("You have turned around just before the attack hit you.");
	else msg_print("Failed! You are hit by the attack.");
#endif

	p_ptr->special_defense &= ~(NINJA_KAWARIMI);
	p_ptr->redraw |= (PR_STATUS);

	/* Teleported */
	return TRUE;
}


/*!
 * @brief 入身処理 / "Rush Attack" routine for Samurai or Ninja
 * @param mdeath 目標モンスターが死亡したかを返す
 * @return 作用が実際にあった場合TRUEを返す /  Return value is for checking "done"
 */
bool rush_attack(bool *mdeath)
{
	DIRECTION dir;
	int tx, ty;
	int tm_idx = 0;
	u16b path_g[32];
	int path_n, i;
	bool tmp_mdeath = FALSE;
	bool moved = FALSE;

	if (mdeath) *mdeath = FALSE;

	project_length = 5;
	if (!get_aim_dir(&dir)) return FALSE;

	/* Use the given direction */
	tx = p_ptr->x + project_length * ddx[dir];
	ty = p_ptr->y + project_length * ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay())
	{
		tx = target_col;
		ty = target_row;
	}

	if (in_bounds(ty, tx)) tm_idx = cave[ty][tx].m_idx;

	path_n = project_path(path_g, project_length, p_ptr->y, p_ptr->x, ty, tx, PROJECT_STOP | PROJECT_KILL);
	project_length = 0;

	/* No need to move */
	if (!path_n) return TRUE;

	/* Use ty and tx as to-move point */
	ty = p_ptr->y;
	tx = p_ptr->x;

	/* Project along the path */
	for (i = 0; i < path_n; i++)
	{
		monster_type *m_ptr;

		int ny = GRID_Y(path_g[i]);
		int nx = GRID_X(path_g[i]);

		if (cave_empty_bold(ny, nx) && player_can_enter(cave[ny][nx].feat, 0))
		{
			ty = ny;
			tx = nx;

			/* Go to next grid */
			continue;
		}

		if (!cave[ny][nx].m_idx)
		{
			if (tm_idx)
			{
				msg_print(_("失敗！", "Failed!"));
			}
			else
			{
				msg_print(_("ここには入身では入れない。", "You can't move to that place."));
			}

			/* Exit loop */
			break;
		}

		/* Move player before updating the monster */
		if (!player_bold(ty, tx)) teleport_player_to(ty, tx, TELEPORT_NONMAGICAL);
		update_monster(cave[ny][nx].m_idx, TRUE);

		/* Found a monster */
		m_ptr = &m_list[cave[ny][nx].m_idx];

		if (tm_idx != cave[ny][nx].m_idx)
		{
#ifdef JP
			msg_format("%s%sが立ちふさがっている！", tm_idx ? "別の" : "",
				   m_ptr->ml ? "モンスター" : "何か");
#else
			msg_format("There is %s in the way!", m_ptr->ml ? (tm_idx ? "another monster" : "a monster") : "someone");
#endif
		}
		else if (!player_bold(ty, tx))
		{
			/* Hold the monster name */
			char m_name[80];

			/* Get the monster name (BEFORE polymorphing) */
			monster_desc(m_name, m_ptr, 0);
			msg_format(_("素早く%sの懐に入り込んだ！", "You quickly jump in and attack %s!"), m_name);
		}

		if (!player_bold(ty, tx)) teleport_player_to(ty, tx, TELEPORT_NONMAGICAL);
		moved = TRUE;
		tmp_mdeath = py_attack(ny, nx, HISSATSU_NYUSIN);

		break;
	}

	if (!moved && !player_bold(ty, tx)) teleport_player_to(ty, tx, TELEPORT_NONMAGICAL);

	if (mdeath) *mdeath = tmp_mdeath;
	return TRUE;
}


/*!
 * @brief 全鏡の消去 / Remove all mirrors in this floor
 * @param explode 爆発処理を伴うならばTRUE
 * @return なし
 */
void remove_all_mirrors(bool explode)
{
	POSITION x, y;

	for (x = 0; x < cur_wid; x++)
	{
		for (y = 0; y < cur_hgt; y++)
		{
			if (is_mirror_grid(&cave[y][x]))
			{
				remove_mirror(y, x);
				if (explode)
					project(0, 2, y, x, p_ptr->lev / 2 + 5, GF_SHARDS,
						(PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI), -1);
			}
		}
	}
}

/*!
 * @brief 『一つの指輪』の効果処理 /
 * Hack -- activate the ring of power
 * @param dir 発動の方向ID
 * @return なし
 */
void ring_of_power(DIRECTION dir)
{
	/* Pick a random effect */
	switch (randint1(10))
	{
	case 1:
	case 2:
	{
		msg_print(_("あなたは悪性のオーラに包み込まれた。", "You are surrounded by a malignant aura."));
		sound(SOUND_EVIL);

		/* Decrease all stats (permanently) */
		(void)dec_stat(A_STR, 50, TRUE);
		(void)dec_stat(A_INT, 50, TRUE);
		(void)dec_stat(A_WIS, 50, TRUE);
		(void)dec_stat(A_DEX, 50, TRUE);
		(void)dec_stat(A_CON, 50, TRUE);
		(void)dec_stat(A_CHR, 50, TRUE);

		/* Lose some experience (permanently) */
		p_ptr->exp -= (p_ptr->exp / 4);
		p_ptr->max_exp -= (p_ptr->exp / 4);
		check_experience();

		break;
	}

	case 3:
	{
		msg_print(_("あなたは強力なオーラに包み込まれた。", "You are surrounded by a powerful aura."));

		/* Dispel monsters */
		dispel_monsters(1000);

		break;
	}

	case 4:
	case 5:
	case 6:
	{
		/* Mana Ball */
		fire_ball(GF_MANA, dir, 600, 3);

		break;
	}

	case 7:
	case 8:
	case 9:
	case 10:
	{
		/* Mana Bolt */
		fire_bolt(GF_MANA, dir, 500);

		break;
	}
	}
}

/*!
* @brief 運命の輪、並びにカオス的な効果の発動
* @param spell ランダムな効果を選択するための基準ID
* @return なし
*/
void wild_magic(int spell)
{
	int counter = 0;
	int type = SUMMON_MOLD + randint0(6);

	if (type < SUMMON_MOLD) type = SUMMON_MOLD;
	else if (type > SUMMON_MIMIC) type = SUMMON_MIMIC;

	switch (randint1(spell) + randint1(8) + 1)
	{
	case 1:
	case 2:
	case 3:
		teleport_player(10, TELEPORT_PASSIVE);
		break;
	case 4:
	case 5:
	case 6:
		teleport_player(100, TELEPORT_PASSIVE);
		break;
	case 7:
	case 8:
		teleport_player(200, TELEPORT_PASSIVE);
		break;
	case 9:
	case 10:
	case 11:
		unlite_area(10, 3);
		break;
	case 12:
	case 13:
	case 14:
		lite_area(damroll(2, 3), 2);
		break;
	case 15:
		destroy_doors_touch();
		break;
	case 16: case 17:
		wall_breaker();
	case 18:
		sleep_monsters_touch();
		break;
	case 19:
	case 20:
		trap_creation(p_ptr->y, p_ptr->x);
		break;
	case 21:
	case 22:
		door_creation();
		break;
	case 23:
	case 24:
	case 25:
		aggravate_monsters(0);
		break;
	case 26:
		earthquake(p_ptr->y, p_ptr->x, 5);
		break;
	case 27:
	case 28:
		(void)gain_random_mutation(0);
		break;
	case 29:
	case 30:
		apply_disenchant(1);
		break;
	case 31:
		lose_all_info();
		break;
	case 32:
		fire_ball(GF_CHAOS, 0, spell + 5, 1 + (spell / 10));
		break;
	case 33:
		wall_stone();
		break;
	case 34:
	case 35:
		while (counter++ < 8)
		{
			(void)summon_specific(0, p_ptr->y, p_ptr->x, (dun_level * 3) / 2, type, (PM_ALLOW_GROUP | PM_NO_PET));
		}
		break;
	case 36:
	case 37:
		activate_hi_summon(p_ptr->y, p_ptr->x, FALSE);
		break;
	case 38:
		(void)summon_cyber(-1, p_ptr->y, p_ptr->x);
		break;
	default:
	{
		int count = 0;
		(void)activate_ty_curse(FALSE, &count);
		break;
	}
	}

	return;
}

/*!
* @brief カオス魔法「流星群」の処理としてプレイヤーを中心に隕石落下処理を10+1d10回繰り返す。
* / Drop 10+1d10 meteor ball at random places near the player
* @param dam ダメージ
* @param rad 効力の半径
* @return なし
*/
void cast_meteor(HIT_POINT dam, POSITION rad)
{
	int i;
	int b = 10 + randint1(10);

	for (i = 0; i < b; i++)
	{
		POSITION y = 0, x = 0;
		int count;

		for (count = 0; count <= 20; count++)
		{
			int dy, dx, d;

			x = p_ptr->x - 8 + randint0(17);
			y = p_ptr->y - 8 + randint0(17);

			dx = (p_ptr->x > x) ? (p_ptr->x - x) : (x - p_ptr->x);
			dy = (p_ptr->y > y) ? (p_ptr->y - y) : (y - p_ptr->y);

			/* Approximate distance */
			d = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));

			if (d >= 9) continue;

			if (!in_bounds(y, x) || !projectable(p_ptr->y, p_ptr->x, y, x)
				|| !cave_have_flag_bold(y, x, FF_PROJECT)) continue;

			/* Valid position */
			break;
		}

		if (count > 20) continue;

		project(0, rad, y, x, dam, GF_METEOR, PROJECT_KILL | PROJECT_JUMP | PROJECT_ITEM, -1);
	}
}


/*!
* @brief 破邪魔法「神の怒り」の処理としてターゲットを指定した後分解のボールを最大20回発生させる。
* @param dam ダメージ
* @param rad 効力の半径
* @return ターゲットを指定し、実行したならばTRUEを返す。
*/
bool cast_wrath_of_the_god(HIT_POINT dam, POSITION rad)
{
	POSITION x, y, tx, ty;
	POSITION nx, ny;
	DIRECTION dir;
	int i;
	int b = 10 + randint1(10);

	if (!get_aim_dir(&dir)) return FALSE;

	/* Use the given direction */
	tx = p_ptr->x + 99 * ddx[dir];
	ty = p_ptr->y + 99 * ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay())
	{
		tx = target_col;
		ty = target_row;
	}

	x = p_ptr->x;
	y = p_ptr->y;

	while (1)
	{
		/* Hack -- Stop at the target */
		if ((y == ty) && (x == tx)) break;

		ny = y;
		nx = x;
		mmove2(&ny, &nx, p_ptr->y, p_ptr->x, ty, tx);

		/* Stop at maximum range */
		if (MAX_RANGE <= distance(p_ptr->y, p_ptr->x, ny, nx)) break;

		/* Stopped by walls/doors */
		if (!cave_have_flag_bold(ny, nx, FF_PROJECT)) break;

		/* Stopped by monsters */
		if ((dir != 5) && cave[ny][nx].m_idx != 0) break;

		/* Save the new location */
		x = nx;
		y = ny;
	}
	tx = x;
	ty = y;

	for (i = 0; i < b; i++)
	{
		int count = 20, d = 0;

		while (count--)
		{
			int dx, dy;

			x = tx - 5 + randint0(11);
			y = ty - 5 + randint0(11);

			dx = (tx > x) ? (tx - x) : (x - tx);
			dy = (ty > y) ? (ty - y) : (y - ty);

			/* Approximate distance */
			d = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));
			/* Within the radius */
			if (d < 5) break;
		}

		if (count < 0) continue;

		/* Cannot penetrate perm walls */
		if (!in_bounds(y, x) ||
			cave_stop_disintegration(y, x) ||
			!in_disintegration_range(ty, tx, y, x))
			continue;

		project(0, rad, y, x, dam, GF_DISINTEGRATE, PROJECT_JUMP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL, -1);
	}

	return TRUE;
}

/*!
* @brief 「ワンダー」のランダムな効果を決定して処理する。
* @param dir 方向ID
* @return なし
* @details
* This spell should become more useful (more controlled) as the\n
* player gains experience levels.  Thus, add 1/5 of the player's\n
* level to the die roll.  This eliminates the worst effects later on,\n
* while keeping the results quite random.  It also allows some potent\n
* effects only at high level.
*/
void cast_wonder(DIRECTION dir)
{
	PLAYER_LEVEL plev = p_ptr->lev;
	int die = randint1(100) + plev / 5;
	int vir = virtue_number(V_CHANCE);

	if (vir)
	{
		if (p_ptr->virtues[vir - 1] > 0)
		{
			while (randint1(400) < p_ptr->virtues[vir - 1]) die++;
		}
		else
		{
			while (randint1(400) < (0 - p_ptr->virtues[vir - 1])) die--;
		}
	}

	if (die < 26)
		chg_virtue(V_CHANCE, 1);

	if (die > 100)
	{
		msg_print(_("あなたは力がみなぎるのを感じた！", "You feel a surge of power!"));
	}

	if (die < 8) clone_monster(dir);
	else if (die < 14) speed_monster(dir, plev);
	else if (die < 26) heal_monster(dir, damroll(4, 6));
	else if (die < 31) poly_monster(dir, plev);
	else if (die < 36)
		fire_bolt_or_beam(beam_chance() - 10, GF_MISSILE, dir,
			damroll(3 + ((plev - 1) / 5), 4));
	else if (die < 41) confuse_monster(dir, plev);
	else if (die < 46) fire_ball(GF_POIS, dir, 20 + (plev / 2), 3);
	else if (die < 51) (void)lite_line(dir, damroll(6, 8));
	else if (die < 56)
		fire_bolt_or_beam(beam_chance() - 10, GF_ELEC, dir,
			damroll(3 + ((plev - 5) / 4), 8));
	else if (die < 61)
		fire_bolt_or_beam(beam_chance() - 10, GF_COLD, dir,
			damroll(5 + ((plev - 5) / 4), 8));
	else if (die < 66)
		fire_bolt_or_beam(beam_chance(), GF_ACID, dir,
			damroll(6 + ((plev - 5) / 4), 8));
	else if (die < 71)
		fire_bolt_or_beam(beam_chance(), GF_FIRE, dir,
			damroll(8 + ((plev - 5) / 4), 8));
	else if (die < 76) hypodynamic_bolt(dir, 75);
	else if (die < 81) fire_ball(GF_ELEC, dir, 30 + plev / 2, 2);
	else if (die < 86) fire_ball(GF_ACID, dir, 40 + plev, 2);
	else if (die < 91) fire_ball(GF_ICE, dir, 70 + plev, 3);
	else if (die < 96) fire_ball(GF_FIRE, dir, 80 + plev, 3);
	else if (die < 101) hypodynamic_bolt(dir, 100 + plev);
	else if (die < 104)
	{
		earthquake(p_ptr->y, p_ptr->x, 12);
	}
	else if (die < 106)
	{
		(void)destroy_area(p_ptr->y, p_ptr->x, 13 + randint0(5), FALSE);
	}
	else if (die < 108)
	{
		symbol_genocide(plev + 50, TRUE);
	}
	else if (die < 110) dispel_monsters(120);
	else /* RARE */
	{
		dispel_monsters(150);
		slow_monsters(plev);
		sleep_monsters(plev);
		hp_player(300);
	}
}


/*!
* @brief 「悪霊召喚」のランダムな効果を決定して処理する。
* @param dir 方向ID
* @return なし
*/
void cast_invoke_spirits(DIRECTION dir)
{
	PLAYER_LEVEL plev = p_ptr->lev;
	int die = randint1(100) + plev / 5;
	int vir = virtue_number(V_CHANCE);

	if (vir)
	{
		if (p_ptr->virtues[vir - 1] > 0)
		{
			while (randint1(400) < p_ptr->virtues[vir - 1]) die++;
		}
		else
		{
			while (randint1(400) < (0 - p_ptr->virtues[vir - 1])) die--;
		}
	}

	msg_print(_("あなたは死者たちの力を招集した...", "You call on the power of the dead..."));
	if (die < 26)
		chg_virtue(V_CHANCE, 1);

	if (die > 100)
	{
		msg_print(_("あなたはおどろおどろしい力のうねりを感じた！", "You feel a surge of eldritch force!"));
	}

	if (die < 8)
	{
		msg_print(_("なんてこった！あなたの周りの地面から朽ちた人影が立ち上がってきた！",
			"Oh no! Mouldering forms rise from the earth around you!"));

		(void)summon_specific(0, p_ptr->y, p_ptr->x, dun_level, SUMMON_UNDEAD, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET));
		chg_virtue(V_UNLIFE, 1);
	}
	else if (die < 14)
	{
		msg_print(_("名状し難い邪悪な存在があなたの心を通り過ぎて行った...", "An unnamable evil brushes against your mind..."));

		set_afraid(p_ptr->afraid + randint1(4) + 4);
	}
	else if (die < 26)
	{
		msg_print(_("あなたの頭に大量の幽霊たちの騒々しい声が押し寄せてきた...",
			"Your head is invaded by a horde of gibbering spectral voices..."));

		set_confused(p_ptr->confused + randint1(4) + 4);
	}
	else if (die < 31)
	{
		poly_monster(dir, plev);
	}
	else if (die < 36)
	{
		fire_bolt_or_beam(beam_chance() - 10, GF_MISSILE, dir,
			damroll(3 + ((plev - 1) / 5), 4));
	}
	else if (die < 41)
	{
		confuse_monster(dir, plev);
	}
	else if (die < 46)
	{
		fire_ball(GF_POIS, dir, 20 + (plev / 2), 3);
	}
	else if (die < 51)
	{
		(void)lite_line(dir, damroll(6, 8));
	}
	else if (die < 56)
	{
		fire_bolt_or_beam(beam_chance() - 10, GF_ELEC, dir,
			damroll(3 + ((plev - 5) / 4), 8));
	}
	else if (die < 61)
	{
		fire_bolt_or_beam(beam_chance() - 10, GF_COLD, dir,
			damroll(5 + ((plev - 5) / 4), 8));
	}
	else if (die < 66)
	{
		fire_bolt_or_beam(beam_chance(), GF_ACID, dir,
			damroll(6 + ((plev - 5) / 4), 8));
	}
	else if (die < 71)
	{
		fire_bolt_or_beam(beam_chance(), GF_FIRE, dir,
			damroll(8 + ((plev - 5) / 4), 8));
	}
	else if (die < 76)
	{
		hypodynamic_bolt(dir, 75);
	}
	else if (die < 81)
	{
		fire_ball(GF_ELEC, dir, 30 + plev / 2, 2);
	}
	else if (die < 86)
	{
		fire_ball(GF_ACID, dir, 40 + plev, 2);
	}
	else if (die < 91)
	{
		fire_ball(GF_ICE, dir, 70 + plev, 3);
	}
	else if (die < 96)
	{
		fire_ball(GF_FIRE, dir, 80 + plev, 3);
	}
	else if (die < 101)
	{
		hypodynamic_bolt(dir, 100 + plev);
	}
	else if (die < 104)
	{
		earthquake(p_ptr->y, p_ptr->x, 12);
	}
	else if (die < 106)
	{
		(void)destroy_area(p_ptr->y, p_ptr->x, 13 + randint0(5), FALSE);
	}
	else if (die < 108)
	{
		symbol_genocide(plev + 50, TRUE);
	}
	else if (die < 110)
	{
		dispel_monsters(120);
	}
	else
	{ /* RARE */
		dispel_monsters(150);
		slow_monsters(plev);
		sleep_monsters(plev);
		hp_player(300);
	}

	if (die < 31)
	{
		msg_print(_("陰欝な声がクスクス笑う。「もうすぐおまえは我々の仲間になるだろう。弱き者よ。」",
			"Sepulchral voices chuckle. 'Soon you will join us, mortal.'"));
	}
}

/*!
* @brief トランプ領域の「シャッフル」の効果をランダムに決めて処理する。
* @return なし
*/
void cast_shuffle(void)
{
	PLAYER_LEVEL plev = p_ptr->lev;
	DIRECTION dir;
	int die;
	int vir = virtue_number(V_CHANCE);
	int i;

	/* Card sharks and high mages get a level bonus */
	if ((p_ptr->pclass == CLASS_ROGUE) ||
		(p_ptr->pclass == CLASS_HIGH_MAGE) ||
		(p_ptr->pclass == CLASS_SORCERER))
		die = (randint1(110)) + plev / 5;
	else
		die = randint1(120);


	if (vir)
	{
		if (p_ptr->virtues[vir - 1] > 0)
		{
			while (randint1(400) < p_ptr->virtues[vir - 1]) die++;
		}
		else
		{
			while (randint1(400) < (0 - p_ptr->virtues[vir - 1])) die--;
		}
	}

	msg_print(_("あなたはカードを切って一枚引いた...", "You shuffle the deck and draw a card..."));

	if (die < 30)
		chg_virtue(V_CHANCE, 1);

	if (die < 7)
	{
		msg_print(_("なんてこった！《死》だ！", "Oh no! It's Death!"));

		for (i = 0; i < randint1(3); i++)
			activate_hi_summon(p_ptr->y, p_ptr->x, FALSE);
	}
	else if (die < 14)
	{
		msg_print(_("なんてこった！《悪魔》だ！", "Oh no! It's the Devil!"));
		summon_specific(0, p_ptr->y, p_ptr->x, dun_level, SUMMON_DEMON, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET));
	}
	else if (die < 18)
	{
		int count = 0;
		msg_print(_("なんてこった！《吊られた男》だ！", "Oh no! It's the Hanged Man."));
		activate_ty_curse(FALSE, &count);
	}
	else if (die < 22)
	{
		msg_print(_("《不調和の剣》だ。", "It's the swords of discord."));
		aggravate_monsters(0);
	}
	else if (die < 26)
	{
		msg_print(_("《愚者》だ。", "It's the Fool."));
		do_dec_stat(A_INT);
		do_dec_stat(A_WIS);
	}
	else if (die < 30)
	{
		msg_print(_("奇妙なモンスターの絵だ。", "It's the picture of a strange monster."));
		trump_summoning(1, FALSE, p_ptr->y, p_ptr->x, (dun_level * 3 / 2), (32 + randint1(6)), PM_ALLOW_GROUP | PM_ALLOW_UNIQUE);
	}
	else if (die < 33)
	{
		msg_print(_("《月》だ。", "It's the Moon."));
		unlite_area(10, 3);
	}
	else if (die < 38)
	{
		msg_print(_("《運命の輪》だ。", "It's the Wheel of Fortune."));
		wild_magic(randint0(32));
	}
	else if (die < 40)
	{
		msg_print(_("テレポート・カードだ。", "It's a teleport trump card."));
		teleport_player(10, TELEPORT_PASSIVE);
	}
	else if (die < 42)
	{
		msg_print(_("《正義》だ。", "It's Justice."));
		set_blessed(p_ptr->lev, FALSE);
	}
	else if (die < 47)
	{
		msg_print(_("テレポート・カードだ。", "It's a teleport trump card."));
		teleport_player(100, TELEPORT_PASSIVE);
	}
	else if (die < 52)
	{
		msg_print(_("テレポート・カードだ。", "It's a teleport trump card."));
		teleport_player(200, TELEPORT_PASSIVE);
	}
	else if (die < 60)
	{
		msg_print(_("《塔》だ。", "It's the Tower."));
		wall_breaker();
	}
	else if (die < 72)
	{
		msg_print(_("《節制》だ。", "It's Temperance."));
		sleep_monsters_touch();
	}
	else if (die < 80)
	{
		msg_print(_("《塔》だ。", "It's the Tower."));

		earthquake(p_ptr->y, p_ptr->x, 5);
	}
	else if (die < 82)
	{
		msg_print(_("友好的なモンスターの絵だ。", "It's the picture of a friendly monster."));
		trump_summoning(1, TRUE, p_ptr->y, p_ptr->x, (dun_level * 3 / 2), SUMMON_MOLD, 0L);
	}
	else if (die < 84)
	{
		msg_print(_("友好的なモンスターの絵だ。", "It's the picture of a friendly monster."));
		trump_summoning(1, TRUE, p_ptr->y, p_ptr->x, (dun_level * 3 / 2), SUMMON_BAT, 0L);
	}
	else if (die < 86)
	{
		msg_print(_("友好的なモンスターの絵だ。", "It's the picture of a friendly monster."));
		trump_summoning(1, TRUE, p_ptr->y, p_ptr->x, (dun_level * 3 / 2), SUMMON_VORTEX, 0L);
	}
	else if (die < 88)
	{
		msg_print(_("友好的なモンスターの絵だ。", "It's the picture of a friendly monster."));
		trump_summoning(1, TRUE, p_ptr->y, p_ptr->x, (dun_level * 3 / 2), SUMMON_COIN_MIMIC, 0L);
	}
	else if (die < 96)
	{
		msg_print(_("《恋人》だ。", "It's the Lovers."));

		if (get_aim_dir(&dir))
			charm_monster(dir, MIN(p_ptr->lev, 20));
	}
	else if (die < 101)
	{
		msg_print(_("《隠者》だ。", "It's the Hermit."));
		wall_stone();
	}
	else if (die < 111)
	{
		msg_print(_("《審判》だ。", "It's the Judgement."));
		do_cmd_rerate(FALSE);
		lose_all_mutations();
	}
	else if (die < 120)
	{
		msg_print(_("《太陽》だ。", "It's the Sun."));
		chg_virtue(V_KNOWLEDGE, 1);
		chg_virtue(V_ENLIGHTEN, 1);
		wiz_lite(FALSE);
	}
	else
	{
		msg_print(_("《世界》だ。", "It's the World."));
		if (p_ptr->exp < PY_MAX_EXP)
		{
			s32b ee = (p_ptr->exp / 25) + 1;
			if (ee > 5000) ee = 5000;
			msg_print(_("更に経験を積んだような気がする。", "You feel more experienced."));
			gain_exp(ee);
		}
	}
}

bool_hack life_stream(bool_hack message, bool_hack virtue)
{
	if(virtue)
	{
		chg_virtue(V_VITALITY, 1);
		chg_virtue(V_UNLIFE, -5);
	}
	if(message)
	{
		msg_print(_("体中に生命力が満ちあふれてきた！", "You feel life flow through your body!"));
	}
	restore_level();
	(void)set_poisoned(0);
	(void)set_blind(0);
	(void)set_confused(0);
	(void)set_image(0);
	(void)set_stun(0);
	(void)set_cut(0);
	(void)restore_all_status();
	(void)set_shero(0, TRUE);
	update_stuff();
	hp_player(5000);

	return TRUE;
}

bool_hack heroism(int base)
{
	bool_hack ident = FALSE;
	if(set_afraid(0)) ident = TRUE;
	if(set_hero(p_ptr->hero + randint1(base) + base, FALSE)) ident = TRUE;
	if(hp_player(10)) ident = TRUE;
	return ident;
}

bool_hack berserk(int base)
{
	bool_hack ident = FALSE;
	if (set_afraid(0)) ident = TRUE;
	if (set_shero(p_ptr->hero + randint1(base) + base, FALSE)) ident = TRUE;
	if (hp_player(30)) ident = TRUE;
	return ident;
}

bool_hack cure_light_wounds(DICE_NUMBER dice, DICE_SID sides)
{
	bool_hack ident = FALSE;
	if (hp_player(damroll(dice, sides))) ident = TRUE;
	if (set_blind(0)) ident = TRUE;
	if (set_cut(p_ptr->cut - 10)) ident = TRUE;
	if (set_shero(0, TRUE)) ident = TRUE;
	return ident;
}

bool_hack cure_serious_wounds(DICE_NUMBER dice, DICE_SID sides)
{
	bool_hack ident = FALSE;
	if (hp_player(damroll(dice, sides))) ident = TRUE;
	if (set_blind(0)) ident = TRUE;
	if (set_confused(0)) ident = TRUE;
	if (set_cut((p_ptr->cut / 2) - 50)) ident = TRUE;
	if (set_shero(0, TRUE)) ident = TRUE;
	return ident;
}

bool_hack cure_critical_wounds(HIT_POINT pow)
{
	bool_hack ident = FALSE;
	if (hp_player(pow)) ident = TRUE;
	if (set_blind(0)) ident = TRUE;
	if (set_confused(0)) ident = TRUE;
	if (set_poisoned(0)) ident = TRUE;
	if (set_stun(0)) ident = TRUE;
	if (set_cut(0)) ident = TRUE;
	if (set_shero(0, TRUE)) ident = TRUE;
	return ident;
}

bool_hack true_healing(HIT_POINT pow)
{
	bool_hack ident = FALSE;
	if (hp_player(pow)) ident = TRUE;
	if (set_blind(0)) ident = TRUE;
	if (set_confused(0)) ident = TRUE;
	if (set_poisoned(0)) ident = TRUE;
	if (set_stun(0)) ident = TRUE;
	if (set_cut(0)) ident = TRUE;
	if (set_image(0)) ident = TRUE;
	return ident;
}

bool_hack restore_mana(bool_hack magic_eater)
{
	bool_hack ident = FALSE;

	if (p_ptr->pclass == CLASS_MAGIC_EATER && magic_eater)
	{
		int i;
		for (i = 0; i < EATER_EXT * 2; i++)
		{
			p_ptr->magic_num1[i] += (p_ptr->magic_num2[i] < 10) ? EATER_CHARGE * 3 : p_ptr->magic_num2[i] * EATER_CHARGE / 3;
			if (p_ptr->magic_num1[i] > p_ptr->magic_num2[i] * EATER_CHARGE) p_ptr->magic_num1[i] = p_ptr->magic_num2[i] * EATER_CHARGE;
		}
		for (; i < EATER_EXT * 3; i++)
		{
			KIND_OBJECT_IDX k_idx = lookup_kind(TV_ROD, i - EATER_EXT * 2);
			p_ptr->magic_num1[i] -= ((p_ptr->magic_num2[i] < 10) ? EATER_ROD_CHARGE * 3 : p_ptr->magic_num2[i] * EATER_ROD_CHARGE / 3)*k_info[k_idx].pval;
			if (p_ptr->magic_num1[i] < 0) p_ptr->magic_num1[i] = 0;
		}
		msg_print(_("頭がハッキリとした。", "You feel your head clear."));
		p_ptr->window |= (PW_PLAYER);
		ident = TRUE;
	}
	else if (p_ptr->csp < p_ptr->msp)
	{
		p_ptr->csp = p_ptr->msp;
		p_ptr->csp_frac = 0;
		msg_print(_("頭がハッキリとした。", "You feel your head clear."));
		p_ptr->redraw |= (PR_MANA);
		p_ptr->window |= (PW_PLAYER);
		p_ptr->window |= (PW_SPELL);
		ident = TRUE;
	}

	return ident;
}

bool restore_all_status(void)
{
	bool ident = FALSE; 
	if (do_res_stat(A_STR)) ident = TRUE;
	if (do_res_stat(A_INT)) ident = TRUE;
	if (do_res_stat(A_WIS)) ident = TRUE;
	if (do_res_stat(A_DEX)) ident = TRUE;
	if (do_res_stat(A_CON)) ident = TRUE;
	if (do_res_stat(A_CHR)) ident = TRUE;
	return ident;
}

/*!
 * @brief 口を使う継続的な処理を中断する
 * @return なし
 */
void stop_mouth(void)
{
	if (music_singing_any()) stop_singing();
	if (hex_spelling_any()) stop_hex_spell_all();
}


bool_hack vampirism(void)
{
	DIRECTION dir;
	POSITION x, y;
	int dummy;
	cave_type *c_ptr;

	if (d_info[dungeon_type].flags1 & DF1_NO_MELEE)
	{
		msg_print(_("なぜか攻撃することができない。", "Something prevent you from attacking."));
		return FALSE;
	}

	/* Only works on adjacent monsters */
	if (!get_direction(&dir, FALSE, FALSE)) return FALSE;
	y = p_ptr->y + ddy[dir];
	x = p_ptr->x + ddx[dir];
	c_ptr = &cave[y][x];

	stop_mouth();

	if (!(c_ptr->m_idx))
	{
		msg_print(_("何もない場所に噛みついた！", "You bite into thin air!"));
		return FALSE;
	}

	msg_print(_("あなたはニヤリとして牙をむいた...", "You grin and bare your fangs..."));

	dummy = p_ptr->lev * 2;

	if (hypodynamic_bolt(dir, dummy))
	{
		if (p_ptr->food < PY_FOOD_FULL)
			/* No heal if we are "full" */
			(void)hp_player(dummy);
		else
			msg_print(_("あなたは空腹ではありません。", "You were not hungry."));

		/* Gain nutritional sustenance: 150/hp drained */
		/* A Food ration gives 5000 food points (by contrast) */
		/* Don't ever get more than "Full" this way */
		/* But if we ARE Gorged,  it won't cure us */
		dummy = p_ptr->food + MIN(5000, 100 * dummy);
		if (p_ptr->food < PY_FOOD_MAX)   /* Not gorged already */
			(void)set_food(dummy >= PY_FOOD_MAX ? PY_FOOD_MAX - 1 : dummy);
	}
	else
		msg_print(_("げぇ！ひどい味だ。", "Yechh. That tastes foul."));
	return TRUE;
}

bool panic_hit(void)
{
	DIRECTION dir;
	POSITION x, y;

	if (!get_direction(&dir, FALSE, FALSE)) return FALSE;
	y = p_ptr->y + ddy[dir];
	x = p_ptr->x + ddx[dir];
	if (cave[y][x].m_idx)
	{
		py_attack(y, x, 0);
		if (randint0(p_ptr->skill_dis) < 7)
			msg_print(_("うまく逃げられなかった。", "You failed to run away."));
		else
			teleport_player(30, 0L);
		return TRUE;
	}
	else
	{
		msg_print(_("その方向にはモンスターはいません。", "You don't see any monster in this direction"));
		msg_print(NULL);
		return FALSE;
	}

}

/*!
* @brief 超能力者のサイコメトリー処理/ Forcibly pseudo-identify an object in the inventory (or on the floor)
* @return なし
* @note
* currently this function allows pseudo-id of any object,
* including silly ones like potions & scrolls, which always
* get '{average}'. This should be changed, either to stop such
* items from being pseudo-id'd, or to allow psychometry to
* detect whether the unidentified potion/scroll/etc is
* good (Cure Light Wounds, Restore Strength, etc) or
* bad (Poison, Weakness etc) or 'useless' (Slime Mold Juice, etc).
*/
bool psychometry(void)
{
	OBJECT_IDX      item;
	object_type     *o_ptr;
	char            o_name[MAX_NLEN];
	byte            feel;
	cptr            q, s;
	bool okay = FALSE;

	item_tester_no_ryoute = TRUE;
	q = _("どのアイテムを調べますか？", "Meditate on which item? ");
	s = _("調べるアイテムがありません。", "You have nothing appropriate.");

	if (!get_item(&item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR))) return (FALSE);

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	/* It is fully known, no information needed */
	if (object_is_known(o_ptr))
	{
		msg_print(_("何も新しいことは判らなかった。", "You cannot find out anything more about that."));
		return TRUE;
	}

	/* Check for a feeling */
	feel = value_check_aux1(o_ptr);

	/* Get an object description */
	object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

	/* Skip non-feelings */
	if (!feel)
	{
		msg_format(_("%sからは特に変わった事は感じとれなかった。", "You do not perceive anything unusual about the %s."), o_name);
		return TRUE;
	}

#ifdef JP
	msg_format("%sは%sという感じがする...",
		o_name, game_inscriptions[feel]);
#else
	msg_format("You feel that the %s %s %s...",
		o_name, ((o_ptr->number == 1) ? "is" : "are"),
		game_inscriptions[feel]);
#endif


	/* We have "felt" it */
	o_ptr->ident |= (IDENT_SENSE);

	/* "Inscribe" it */
	o_ptr->feeling = feel;

	/* Player touches it */
	o_ptr->marked |= OM_TOUCHED;

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Valid "tval" codes */
	switch (o_ptr->tval)
	{
	case TV_SHOT:
	case TV_ARROW:
	case TV_BOLT:
	case TV_BOW:
	case TV_DIGGING:
	case TV_HAFTED:
	case TV_POLEARM:
	case TV_SWORD:
	case TV_BOOTS:
	case TV_GLOVES:
	case TV_HELM:
	case TV_CROWN:
	case TV_SHIELD:
	case TV_CLOAK:
	case TV_SOFT_ARMOR:
	case TV_HARD_ARMOR:
	case TV_DRAG_ARMOR:
	case TV_CARD:
	case TV_RING:
	case TV_AMULET:
	case TV_LITE:
	case TV_FIGURINE:
		okay = TRUE;
		break;
	}

	/* Auto-inscription/destroy */
	autopick_alter_item(item, (bool)(okay && destroy_feeling));

	/* Something happened */
	return (TRUE);
}
