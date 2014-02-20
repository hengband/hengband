/*!
 * @file mspells2.c
 * @brief モンスター魔法の実装(対モンスター処理) / Monster spells (attack monster)
 * @date 2014/01/17
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen.\n
 * @details
 */

#include "angband.h"

/*!
 * @brief モンスターが敵対モンスターにビームを当てること可能かを判定する /
 * Determine if a beam spell will hit the target.
 * @param y1 始点のY座標
 * @param x1 始点のX座標
 * @param y2 目標のY座標
 * @param x2 目標のX座標
 * @param m_ptr 使用するモンスターの構造体参照ポインタ
 * @return ビームが到達可能ならばTRUEを返す
 */
static bool direct_beam(int y1, int x1, int y2, int x2, monster_type *m_ptr)
{
	bool hit2 = FALSE;
	int i, y, x;

	int grid_n = 0;
	u16b grid_g[512];

	bool is_friend = is_pet(m_ptr);

	/* Check the projection path */
	grid_n = project_path(grid_g, MAX_RANGE, y1, x1, y2, x2, PROJECT_THRU);

	/* No grid is ever projectable from itself */
	if (!grid_n) return (FALSE);

	for (i = 0; i < grid_n; i++)
	{
		y = GRID_Y(grid_g[i]);
		x = GRID_X(grid_g[i]);

		if (y == y2 && x == x2)
			hit2 = TRUE;
		else if (is_friend && cave[y][x].m_idx > 0 &&
			 !are_enemies(m_ptr, &m_list[cave[y][x].m_idx]))
		{
			/* Friends don't shoot friends */
			return FALSE;
		}

		if (is_friend && player_bold(y, x))
			return FALSE;
	}
	if (!hit2)
		return FALSE;
	return TRUE;
}

/*!
 * @brief モンスターが敵対モンスターに直接ブレスを当てることが可能かを判定する /
 * Determine if a breath will hit the target.
 * @param y1 始点のY座標
 * @param x1 始点のX座標
 * @param y2 目標のY座標
 * @param x2 目標のX座標
 * @param rad 半径
 * @param typ 効果属性ID
 * @param is_friend TRUEならば、プレイヤーを巻き込む時にブレスの判定をFALSEにする。
 * @return ブレスを直接当てられるならばTRUEを返す
 */
static bool breath_direct(int y1, int x1, int y2, int x2, int rad, int typ, bool is_friend)
{
	/* Must be the same as projectable() */

	int i;

	/* Initial grid */
	int y = y1;
	int x = x1;

	int grid_n = 0;
	u16b grid_g[512];

	int grids = 0;
	byte gx[1024], gy[1024];
	byte gm[32];
	int gm_rad = rad;

	bool hit2 = FALSE;
	bool hityou = FALSE;

	int flg;

	switch (typ)
	{
	case GF_LITE:
	case GF_LITE_WEAK:
		flg = PROJECT_LOS;
		break;
	case GF_DISINTEGRATE:
		flg = PROJECT_DISI;
		break;
	default:
		flg = 0;
		break;
	}

	/* Check the projection path */
	grid_n = project_path(grid_g, MAX_RANGE, y1, x1, y2, x2, flg);

	/* Project along the path */
	for (i = 0; i < grid_n; ++i)
	{
		int ny = GRID_Y(grid_g[i]);
		int nx = GRID_X(grid_g[i]);

		if (flg & PROJECT_DISI)
		{
			/* Hack -- Balls explode before reaching walls */
			if (cave_stop_disintegration(ny, nx)) break;
		}
		else if (flg & PROJECT_LOS)
		{
			/* Hack -- Balls explode before reaching walls */
			if (!cave_los_bold(ny, nx)) break;
		}
		else
		{
			/* Hack -- Balls explode before reaching walls */
			if (!cave_have_flag_bold(ny, nx, FF_PROJECT)) break;
		}

		/* Save the "blast epicenter" */
		y = ny;
		x = nx;
	}

	grid_n = i;

	if (!grid_n)
	{
		if (flg & PROJECT_DISI)
		{
			if (in_disintegration_range(y1, x1, y2, x2) && (distance(y1, x1, y2, x2) <= rad)) hit2 = TRUE;
			if (in_disintegration_range(y1, x1, py, px) && (distance(y1, x1, py, px) <= rad)) hityou = TRUE;
		}
		else if (flg & PROJECT_LOS)
		{
			if (los(y1, x1, y2, x2) && (distance(y1, x1, y2, x2) <= rad)) hit2 = TRUE;
			if (los(y1, x1, py, px) && (distance(y1, x1, py, px) <= rad)) hityou = TRUE;
		}
		else
		{
			if (projectable(y1, x1, y2, x2) && (distance(y1, x1, y2, x2) <= rad)) hit2 = TRUE;
			if (projectable(y1, x1, py, px) && (distance(y1, x1, py, px) <= rad)) hityou = TRUE;
		}
	}
	else
	{
		breath_shape(grid_g, grid_n, &grids, gx, gy, gm, &gm_rad, rad, y1, x1, y, x, typ);

		for (i = 0; i < grids; i++)
		{
			/* Extract the location */
			y = gy[i];
			x = gx[i];

			if ((y == y2) && (x == x2)) hit2 = TRUE;
			if (player_bold(y, x)) hityou = TRUE;
		}
	}

	if (!hit2) return FALSE;
	if (is_friend && hityou) return FALSE;

	return TRUE;
}

/*!
 * @brief モンスターが特殊能力の目標地点を決める処理 /
 * Get the actual center point of ball spells (rad > 1) (originally from TOband)
 * @param sy 始点のY座標
 * @param sx 始点のX座標
 * @param ty 目標Y座標を返す参照ポインタ
 * @param tx 目標X座標を返す参照ポインタ
 * @param flg 判定のフラグ配列
 * @return なし
 */
void get_project_point(int sy, int sx, int *ty, int *tx, int flg)
{
	u16b path_g[128];
	int  path_n, i;

	path_n = project_path(path_g, MAX_RANGE, sy, sx, *ty, *tx, flg);

	*ty = sy;
	*tx = sx;

	/* Project along the path */
	for (i = 0; i < path_n; i++)
	{
		sy = GRID_Y(path_g[i]);
		sx = GRID_X(path_g[i]);

		/* Hack -- Balls explode before reaching walls */
		if (!cave_have_flag_bold(sy, sx, FF_PROJECT)) break;

		*ty = sy;
		*tx = sx;
	}
}

/*!
 * @brief モンスターが敵モンスターに魔力消去を使うかどうかを返す /
 * Check should monster cast dispel spell at other monster.
 * @param m_idx 術者のモンスターID
 * @param t_idx 目標のモンスターID
 * @return 魔力消去を使うべきならばTRUEを変えす。
 */
static bool dispel_check_monster(int m_idx, int t_idx)
{
	monster_type *t_ptr = &m_list[t_idx];

	/* Invulnabilty */
	if (MON_INVULNER(t_ptr)) return TRUE;

	/* Speed */
	if (t_ptr->mspeed < 135)
	{
		if (MON_FAST(t_ptr)) return TRUE;
	}

	/* Riding monster */
	if (t_idx == p_ptr->riding)
	{
		if (dispel_check(m_idx)) return TRUE;
	}

	/* No need to cast dispel spell */
	return FALSE;
}

/*!
 * @brief モンスターが敵モンスターに特殊能力を使う処理のメインルーチン /
 * Monster tries to 'cast a spell' (or breath, etc) at another monster.
 * @param m_idx 術者のモンスターID
 * @return 実際に特殊能力を使った場合TRUEを返す
 * @details
 * The player is only disturbed if able to be affected by the spell.
 */
bool monst_spell_monst(int m_idx)
{
	int y = 0, x = 0;
	int i, k, t_idx = 0;
	int thrown_spell, count = 0;
	int rlev;
	int dam = 0;
	int start;
	int plus = 1;
	u32b u_mode = 0L;
	int s_num_6 = (easy_band ? 2 : 6);
	int s_num_4 = (easy_band ? 1 : 4);
	int rad = 0; //For elemental balls

	byte spell[96], num = 0;

	char m_name[160];
	char t_name[160];

#ifndef JP
	char m_poss[160];
#endif

	monster_type *m_ptr = &m_list[m_idx];
	monster_type *t_ptr = NULL;

	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_race *tr_ptr = NULL;

	u32b f4, f5, f6;

	bool wake_up = FALSE;
	bool fear = FALSE;

	bool blind = (p_ptr->blind ? TRUE : FALSE);

	bool see_m = is_seen(m_ptr);
	bool maneable = player_has_los_bold(m_ptr->fy, m_ptr->fx);
	bool see_t;
	bool see_either;
	bool known;

	bool pet = is_pet(m_ptr);

	bool in_no_magic_dungeon = (d_info[dungeon_type].flags1 & DF1_NO_MAGIC) && dun_level
		&& (!p_ptr->inside_quest || is_fixed_quest_idx(p_ptr->inside_quest));

	bool can_use_lite_area = FALSE;

	bool can_remember;

	bool resists_tele = FALSE;

	/* Prepare flags for summoning */
	if (!pet) u_mode |= PM_ALLOW_UNIQUE;

	/* Cannot cast spells when confused */
	if (MON_CONFUSED(m_ptr)) return (FALSE);

	/* Extract the racial spell flags */
	f4 = r_ptr->flags4;
	f5 = r_ptr->flags5;
	f6 = r_ptr->flags6;

	/* Target is given for pet? */
	if (pet_t_m_idx && pet)
	{
		t_idx = pet_t_m_idx;
		t_ptr = &m_list[t_idx];

		/* Cancel if not projectable (for now) */
		if ((m_idx == t_idx) || !projectable(m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx))
		{
			t_idx = 0;
		}
	}

	/* Is there counter attack target? */
	if (!t_idx && m_ptr->target_y)
	{
		t_idx = cave[m_ptr->target_y][m_ptr->target_x].m_idx;

		if (t_idx)
		{
			t_ptr = &m_list[t_idx];

			/* Cancel if neither enemy nor a given target */
			if ((m_idx == t_idx) ||
			    ((t_idx != pet_t_m_idx) && !are_enemies(m_ptr, t_ptr)))
			{
				t_idx = 0;
			}

			/* Allow only summoning etc.. if not projectable */
			else if (!projectable(m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx))
			{
				f4 &= (RF4_INDIRECT_MASK);
				f5 &= (RF5_INDIRECT_MASK);
				f6 &= (RF6_INDIRECT_MASK);
			}
		}
	}

	/* Look for enemies normally */
	if (!t_idx)
	{
		bool success = FALSE;

		if (p_ptr->inside_battle)
		{
			start = randint1(m_max-1) + m_max;
			if (randint0(2)) plus = -1;
		}
		else start = m_max + 1;

		/* Scan thru all monsters */
		for (i = start; ((i < start + m_max) && (i > start - m_max)); i += plus)
		{
			int dummy = (i % m_max);
			if (!dummy) continue;

			t_idx = dummy;
			t_ptr = &m_list[t_idx];

			/* Skip dead monsters */
			if (!t_ptr->r_idx) continue;

			/* Monster must be 'an enemy' */
			if ((m_idx == t_idx) || !are_enemies(m_ptr, t_ptr)) continue;

			/* Monster must be projectable */
			if (!projectable(m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx)) continue;

			/* Get it */
			success = TRUE;
			break;
		}

		/* No enemy found */
		if (!success) return FALSE;
	}


	/* OK -- we've got a target */
	y = t_ptr->fy;
	x = t_ptr->fx;
	tr_ptr = &r_info[t_ptr->r_idx];

	/* Forget old counter attack target */
	reset_target(m_ptr);

	/* Extract the monster level */
	rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);

	/* Remove unimplemented spells */
	f6 &= ~(RF6_WORLD | RF6_TRAPS | RF6_FORGET);

	if (f4 & RF4_BR_LITE)
	{
		if (!los(m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx))
			f4 &= ~(RF4_BR_LITE);
	}

	/* Remove unimplemented special moves */
	if (f6 & RF6_SPECIAL)
	{
		if ((m_ptr->r_idx != MON_ROLENTO) && (r_ptr->d_char != 'B'))
			f6 &= ~(RF6_SPECIAL);
	}

	if (f6 & RF6_DARKNESS)
	{
		bool vs_ninja = (p_ptr->pclass == CLASS_NINJA) && !is_hostile(t_ptr);

		if (vs_ninja &&
		    !(r_ptr->flags3 & (RF3_UNDEAD | RF3_HURT_LITE)) &&
		    !(r_ptr->flags7 & RF7_DARK_MASK))
			can_use_lite_area = TRUE;

		if (!(r_ptr->flags2 & RF2_STUPID))
		{
			if (d_info[dungeon_type].flags1 & DF1_DARKNESS) f6 &= ~(RF6_DARKNESS);
			else if (vs_ninja && !can_use_lite_area) f6 &= ~(RF6_DARKNESS);
		}
	}

	if (in_no_magic_dungeon && !(r_ptr->flags2 & RF2_STUPID))
	{
		f4 &= (RF4_NOMAGIC_MASK);
		f5 &= (RF5_NOMAGIC_MASK);
		f6 &= (RF6_NOMAGIC_MASK);
	}

	if (p_ptr->inside_arena || p_ptr->inside_battle)
	{
		f4 &= ~(RF4_SUMMON_MASK);
		f5 &= ~(RF5_SUMMON_MASK);
		f6 &= ~(RF6_SUMMON_MASK | RF6_TELE_LEVEL);

		if (m_ptr->r_idx == MON_ROLENTO) f6 &= ~(RF6_SPECIAL);
	}

	if (p_ptr->inside_battle && !one_in_(3))
	{
		f6 &= ~(RF6_HEAL);
	}

	if (m_idx == p_ptr->riding)
	{
		f4 &= ~(RF4_RIDING_MASK);
		f5 &= ~(RF5_RIDING_MASK);
		f6 &= ~(RF6_RIDING_MASK);
	}

	if (pet)
	{
		f4 &= ~(RF4_SHRIEK);
		f6 &= ~(RF6_DARKNESS | RF6_TRAPS);

		if (!(p_ptr->pet_extra_flags & PF_TELEPORT))
		{
			f6 &= ~(RF6_BLINK | RF6_TPORT | RF6_TELE_TO | RF6_TELE_AWAY | RF6_TELE_LEVEL);
		}

		if (!(p_ptr->pet_extra_flags & PF_ATTACK_SPELL))
		{
			f4 &= ~(RF4_ATTACK_MASK);
			f5 &= ~(RF5_ATTACK_MASK);
			f6 &= ~(RF6_ATTACK_MASK);
		}

		if (!(p_ptr->pet_extra_flags & PF_SUMMON_SPELL))
		{
			f4 &= ~(RF4_SUMMON_MASK);
			f5 &= ~(RF5_SUMMON_MASK);
			f6 &= ~(RF6_SUMMON_MASK);
		}

		/* Prevent collateral damage */
		if (!(p_ptr->pet_extra_flags & PF_BALL_SPELL) && (m_idx != p_ptr->riding))
		{
			if ((f4 & (RF4_BALL_MASK & ~(RF4_ROCKET))) ||
			    (f5 & RF5_BALL_MASK) ||
			    (f6 & RF6_BALL_MASK))
			{
				int real_y = y;
				int real_x = x;

				get_project_point(m_ptr->fy, m_ptr->fx, &real_y, &real_x, 0L);

				if (projectable(real_y, real_x, py, px))
				{
					int dist = distance(real_y, real_x, py, px);

					if (dist <= 2)
					{
						f4 &= ~(RF4_BALL_MASK & ~(RF4_ROCKET));
						f5 &= ~(RF5_BALL_MASK);
						f6 &= ~(RF6_BALL_MASK);
					}
					else if (dist <= 4)
					{
						f4 &= ~(RF4_BIG_BALL_MASK);
						f5 &= ~(RF5_BIG_BALL_MASK);
						f6 &= ~(RF6_BIG_BALL_MASK);
					}
				}
				else if (f5 & RF5_BA_LITE)
				{
					if ((distance(real_y, real_x, py, px) <= 4) && los(real_y, real_x, py, px))
						f5 &= ~(RF5_BA_LITE);
				}
			}

			if (f4 & RF4_ROCKET)
			{
				int real_y = y;
				int real_x = x;

				get_project_point(m_ptr->fy, m_ptr->fx, &real_y, &real_x, PROJECT_STOP);
				if (projectable(real_y, real_x, py, px) && (distance(real_y, real_x, py, px) <= 2))
					f4 &= ~(RF4_ROCKET);
			}

			if (((f4 & RF4_BEAM_MASK) ||
			     (f5 & RF5_BEAM_MASK) ||
			     (f6 & RF6_BEAM_MASK)) &&
			    !direct_beam(m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx, m_ptr))
			{
				f4 &= ~(RF4_BEAM_MASK);
				f5 &= ~(RF5_BEAM_MASK);
				f6 &= ~(RF6_BEAM_MASK);
			}

			if ((f4 & RF4_BREATH_MASK) ||
			    (f5 & RF5_BREATH_MASK) ||
			    (f6 & RF6_BREATH_MASK))
			{
				/* Expected breath radius */
				int rad = (r_ptr->flags2 & RF2_POWERFUL) ? 3 : 2;

				if (!breath_direct(m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx, rad, 0, TRUE))
				{
					f4 &= ~(RF4_BREATH_MASK);
					f5 &= ~(RF5_BREATH_MASK);
					f6 &= ~(RF6_BREATH_MASK);
				}
				else if ((f4 & RF4_BR_LITE) &&
					 !breath_direct(m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx, rad, GF_LITE, TRUE))
				{
					f4 &= ~(RF4_BR_LITE);
				}
				else if ((f4 & RF4_BR_DISI) &&
					 !breath_direct(m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx, rad, GF_DISINTEGRATE, TRUE))
				{
					f4 &= ~(RF4_BR_DISI);
				}
			}
		}

		/* Special moves restriction */
		if (f6 & RF6_SPECIAL)
		{
			if (m_ptr->r_idx == MON_ROLENTO)
			{
				if ((p_ptr->pet_extra_flags & (PF_ATTACK_SPELL | PF_SUMMON_SPELL)) != (PF_ATTACK_SPELL | PF_SUMMON_SPELL))
					f6 &= ~(RF6_SPECIAL);
			}
			else if (r_ptr->d_char == 'B')
			{
				if ((p_ptr->pet_extra_flags & (PF_ATTACK_SPELL | PF_TELEPORT)) != (PF_ATTACK_SPELL | PF_TELEPORT))
					f6 &= ~(RF6_SPECIAL);
			}
			else f6 &= ~(RF6_SPECIAL);
		}
	}

	/* Remove some spells if necessary */

	if (!(r_ptr->flags2 & RF2_STUPID))
	{
		/* Check for a clean bolt shot */
		if (((f4 & RF4_BOLT_MASK) ||
		     (f5 & RF5_BOLT_MASK) ||
		     (f6 & RF6_BOLT_MASK)) &&
		    !clean_shot(m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx, pet))
		{
			f4 &= ~(RF4_BOLT_MASK);
			f5 &= ~(RF5_BOLT_MASK);
			f6 &= ~(RF6_BOLT_MASK);
		}

		/* Check for a possible summon */
		if (((f4 & RF4_SUMMON_MASK) ||
		     (f5 & RF5_SUMMON_MASK) ||
		     (f6 & RF6_SUMMON_MASK)) &&
		    !(summon_possible(t_ptr->fy, t_ptr->fx)))
		{
			/* Remove summoning spells */
			f4 &= ~(RF4_SUMMON_MASK);
			f5 &= ~(RF5_SUMMON_MASK);
			f6 &= ~(RF6_SUMMON_MASK);
		}

		/* Dispel magic */
		if ((f4 & RF4_DISPEL) && !dispel_check_monster(m_idx, t_idx))
		{
			/* Remove dispel spell */
			f4 &= ~(RF4_DISPEL);
		}

		/* Check for a possible raise dead */
		if ((f6 & RF6_RAISE_DEAD) && !raise_possible(m_ptr))
		{
			/* Remove raise dead spell */
			f6 &= ~(RF6_RAISE_DEAD);
		}

		/* Special moves restriction */
		if (f6 & RF6_SPECIAL)
		{
			if ((m_ptr->r_idx == MON_ROLENTO) && !summon_possible(t_ptr->fy, t_ptr->fx))
			{
				f6 &= ~(RF6_SPECIAL);
			}
		}
	}

	if (r_ptr->flags2 & RF2_SMART)
	{
		/* Hack -- allow "desperate" spells */
		if ((m_ptr->hp < m_ptr->maxhp / 10) &&
		    (randint0(100) < 50))
		{
			/* Require intelligent spells */
			f4 &= (RF4_INT_MASK);
			f5 &= (RF5_INT_MASK);
			f6 &= (RF6_INT_MASK);
		}

		/* Hack -- decline "teleport level" in some case */
		if ((f6 & RF6_TELE_LEVEL) && TELE_LEVEL_IS_INEFF((t_idx == p_ptr->riding) ? 0 : t_idx))
		{
			f6 &= ~(RF6_TELE_LEVEL);
		}
	}

	/* No spells left */
	if (!f4 && !f5 && !f6) return FALSE;

	/* Extract the "inate" spells */
	for (k = 0; k < 32; k++)
	{
		if (f4 & (1L << k)) spell[num++] = k + 32 * 3;
	}

	/* Extract the "normal" spells */
	for (k = 0; k < 32; k++)
	{
		if (f5 & (1L << k)) spell[num++] = k + 32 * 4;
	}

	/* Extract the "bizarre" spells */
	for (k = 0; k < 32; k++)
	{
		if (f6 & (1L << k)) spell[num++] = k + 32 * 5;
	}

	/* No spells left */
	if (!num) return (FALSE);

	/* Stop if player is dead or gone */
	if (!p_ptr->playing || p_ptr->is_dead) return (FALSE);

	/* Handle "leaving" */
	if (p_ptr->leaving) return (FALSE);

	/* Get the monster name (or "it") */
	monster_desc(m_name, m_ptr, 0x00);

#ifndef JP
	/* Get the monster possessive ("his"/"her"/"its") */
	monster_desc(m_poss, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE);
#endif

	/* Get the target's name (or "it") */
	monster_desc(t_name, t_ptr, 0x00);

	/* Choose a spell to cast */
	thrown_spell = spell[randint0(num)];

	see_t = is_seen(t_ptr);
	see_either = (see_m || see_t);

	/* Can the player be aware of this attack? */
	known = (m_ptr->cdis <= MAX_SIGHT) || (t_ptr->cdis <= MAX_SIGHT);

	if (p_ptr->riding && (m_idx == p_ptr->riding)) disturb(1, 1);

	/* Check for spell failure (inate attacks never fail) */
	if (!spell_is_inate(thrown_spell) && (in_no_magic_dungeon || (MON_STUNNED(m_ptr) && one_in_(2))))
	{
		disturb(1, 1);
		/* Message */
		if (see_m) msg_format(_("%^sは呪文を唱えようとしたが失敗した。", 
			                    "%^s tries to cast a spell, but fails."), m_name);

		return (TRUE);
	}

	/* Hex: Anti Magic Barrier */
	if (!spell_is_inate(thrown_spell) && magic_barrier(m_idx))
	{
		if (see_m) msg_format(_("反魔法バリアが%^sの呪文をかき消した。", 
			                    "Anti magic barrier cancels the spell which %^s casts."), m_name);
		return (TRUE);
	}

	can_remember = is_original_ap_and_seen(m_ptr);

	switch (thrown_spell)
	{
    case 96 + 0:   spell_RF4_SHRIEK(m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_SHRIEK */
    case 96 + 1:   return FALSE;  /* RF4_XXX1 */
    case 96 + 2:   spell_RF4_DISPEL(m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_DISPEL */
    case 96 + 3:   dam = spell_RF4_ROCKET(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_ROCKET */
    case 96 + 4:   dam = spell_RF4_SHOOT(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_SHOOT */
    case 96 + 5:   return FALSE; /* RF4_XXX2 */
    case 96 + 6:   return FALSE; /* RF4_XXX3 */
    case 96 + 7:   return FALSE; /* RF4_XXX4 */
    case 96 + 8:   dam = spell_RF4_BREATH(GF_ACID, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_BR_ACID */
    case 96 + 9:   dam = spell_RF4_BREATH(GF_ELEC, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_BR_ELEC */
    case 96 + 10:  dam = spell_RF4_BREATH(GF_FIRE, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_BR_FIRE */
    case 96 + 11:  dam = spell_RF4_BREATH(GF_COLD, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_BR_COLD */
    case 96 + 12:  dam = spell_RF4_BREATH(GF_POIS, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_BR_POIS */
    case 96 + 13:  dam = spell_RF4_BREATH(GF_NETHER, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_BR_NETH */
    case 96 + 14:  dam = spell_RF4_BREATH(GF_LITE, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_BR_LITE */
    case 96 + 15:  dam = spell_RF4_BREATH(GF_DARK, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_BR_DARK */
    case 96 + 16:  dam = spell_RF4_BREATH(GF_CONFUSION, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_BR_CONF */
    case 96 + 17:  dam = spell_RF4_BREATH(GF_SOUND, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_BR_SOUN */
    case 96 + 18:  dam = spell_RF4_BREATH(GF_CHAOS, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_BR_CHAO */
    case 96 + 19:  dam = spell_RF4_BREATH(GF_DISENCHANT, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_BR_DISE */
    case 96 + 20:  dam = spell_RF4_BREATH(GF_NEXUS, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_BR_NEXU */
    case 96 + 21:  dam = spell_RF4_BREATH(GF_TIME, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_BR_TIME */
    case 96 + 22:  dam = spell_RF4_BREATH(GF_INERTIA, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_BR_INER */
    case 96 + 23:  dam = spell_RF4_BREATH(GF_GRAVITY, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_BR_GRAV */
    case 96 + 24:  dam = spell_RF4_BREATH(GF_SHARDS, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_BR_SHAR */
    case 96 + 25:  dam = spell_RF4_BREATH(GF_PLASMA, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_BR_PLAS */
    case 96 + 26:  dam = spell_RF4_BREATH(GF_FORCE, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_BR_WALL */
    case 96 + 27:  dam = spell_RF4_BREATH(GF_MANA, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_BR_MANA */
    case 96 + 28:  dam = spell_RF4_BA_NUKE(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_BA_NUKE */
    case 96 + 29:  dam = spell_RF4_BREATH(GF_NUKE, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_BR_NUKE */
    case 96 + 30:  dam = spell_RF4_BA_CHAO(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_BA_CHAO */
    case 96 + 31:  dam = spell_RF4_BREATH(GF_DISINTEGRATE, y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF4_BR_DISI */
    case 128 + 0:  dam = spell_RF5_BA_ACID(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_BA_ACID */
    case 128 + 1:  dam = spell_RF5_BA_ELEC(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_BA_ELEC */
    case 128 + 2:  dam = spell_RF5_BA_FIRE(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_BA_FIRE */
    case 128 + 3:  dam = spell_RF5_BA_COLD(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_BA_COLD */
    case 128 + 4:  dam = spell_RF5_BA_POIS(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_BA_POIS */
    case 128 + 5:  dam = spell_RF5_BA_NETH(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_BA_NETH */
    case 128 + 6:  dam = spell_RF5_BA_WATE(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_BA_WATE */
    case 128 + 7:  dam = spell_RF5_BA_MANA(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_BA_MANA */
    case 128 + 8:  dam = spell_RF5_BA_DARK(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_BA_DARK */
    case 128 + 9:  dam = spell_RF5_DRAIN_MANA(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_DRAIN_MANA */
    case 128 + 10: dam = spell_RF5_MIND_BLAST(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_MIND_BLAST */
    case 128 + 11: dam = spell_RF5_BRAIN_SMASH(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_BRAIN_SMASH */
    case 128 + 12: dam = spell_RF5_CAUSE_1(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_CAUSE_1 */
    case 128 + 13: dam = spell_RF5_CAUSE_2(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_CAUSE_2 */
    case 128 + 14: dam = spell_RF5_CAUSE_3(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_CAUSE_3 */
    case 128 + 15: dam = spell_RF5_CAUSE_4(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_CAUSE_4 */
    case 128 + 16: dam = spell_RF5_BO_ACID(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_BO_ACID */
    case 128 + 17: dam = spell_RF5_BO_ELEC(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_BO_ELEC */
    case 128 + 18: dam = spell_RF5_BO_FIRE(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_BO_FIRE */
    case 128 + 19: dam = spell_RF5_BO_COLD(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_BO_COLD */
    case 128 + 20: dam = spell_RF5_BA_LITE(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_BA_LITE */
    case 128 + 21: dam = spell_RF5_BO_NETH(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_BO_NETH */
    case 128 + 22: dam = spell_RF5_BO_WATE(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_BO_WATE */
    case 128 + 23: dam = spell_RF5_BO_MANA(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_BO_MANA */
    case 128 + 24: dam = spell_RF5_BO_PLAS(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_BO_PLAS */
    case 128 + 25: dam = spell_RF5_BO_ICEE(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_BO_ICEE */
    case 128 + 26: dam = spell_RF5_MISSILE(y, x, m_idx, t_idx, MONSTER_TO_MONSTER); break; /* RF5_MISSILE */

	/* RF5_SCARE */
	case 128+27:
		if (known)
		{
			if (see_either)
			{
				msg_format(_("%^sが恐ろしげな幻覚を作り出した。",
					         "%^s casts a fearful illusion in front of %s."), m_name, t_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		if (tr_ptr->flags3 & RF3_NO_FEAR)
		{
			if (see_t) msg_format(_("%^sは恐怖を感じない。", 
				                    "%^s refuses to be frightened."), t_name);

		}
		else if (tr_ptr->level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10)
		{
			if (see_t) msg_format(_("%^sは恐怖を感じない。", 
				                    "%^s refuses to be frightened."), t_name);
		}
		else
		{
			if (set_monster_monfear(t_idx, MON_MONFEAR(t_ptr) + randint0(4) + 4)) fear = TRUE;
		}

		wake_up = TRUE;

		break;

	/* RF5_BLIND */
	case 128+28:
		if (known)
		{
			if (see_either)
			{
				_(msg_format("%sは呪文を唱えて%sの目を焼き付かせた。", m_name, t_name),
				  msg_format("%^s casts a spell, burning %s%s eyes.", m_name, t_name,
					         (streq(t_name, "it") ? "s" : "'s")));

			}
			else
			{
				mon_fight = TRUE;
			}
		}

		/* Simulate blindness with confusion */
		if (tr_ptr->flags3 & RF3_NO_CONF)
		{
			if (see_t) msg_format(_("%^sには効果がなかった。", "%^s is unaffected."), t_name);
		}
		else if (tr_ptr->level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10)
		{
			if (see_t) msg_format(_("%^sには効果がなかった。", "%^s is unaffected."), t_name);
		}
		else
		{
			if (see_t) msg_format(_("%^sは目が見えなくなった！ ", "%^s is blinded!"), t_name);

			(void)set_monster_confused(t_idx, MON_CONFUSED(t_ptr) + 12 + randint0(4));
		}

		wake_up = TRUE;

		break;

	/* RF5_CONF */
	case 128+29:
		if (known)
		{
			if (see_either)
			{
				msg_format(_("%^sが%sの前に幻惑的な幻をつくり出した。",
					         "%^s casts a mesmerizing illusion in front of %s."), m_name, t_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		if (tr_ptr->flags3 & RF3_NO_CONF)
		{
			if (see_t) msg_format(_("%^sは惑わされなかった。",
				                    "%^s disbelieves the feeble spell."), t_name);
		}
		else if (tr_ptr->level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10)
		{
			if (see_t) msg_format(_("%^sは惑わされなかった。",
				                    "%^s disbelieves the feeble spell."), t_name);
		}
		else
		{
			if (see_t) msg_format(_("%^sは混乱したようだ。",
				                    "%^s seems confused."), t_name);

			(void)set_monster_confused(t_idx, MON_CONFUSED(t_ptr) + 12 + randint0(4));
		}

		wake_up = TRUE;

		break;

	/* RF5_SLOW */
	case 128+30:
		if (known)
		{
			if (see_either)
			{
				_(msg_format("%sが%sの筋肉から力を吸いとった。", m_name, t_name),
				  msg_format("%^s drains power from %s%s muscles.", m_name, t_name,
					   (streq(t_name, "it") ? "s" : "'s")));
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		if (tr_ptr->flags1 & RF1_UNIQUE)
		{
			if (see_t) msg_format(_("%^sには効果がなかった。",
				                    "%^s is unaffected."), t_name);
		}
		else if (tr_ptr->level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10)
		{
			if (see_t) msg_format(_("%^sには効果がなかった。",
				                    "%^s is unaffected."), t_name);
		}
		else
		{
			if (set_monster_slow(t_idx, MON_SLOW(t_ptr) + 50))
			{
				if (see_t) msg_format(_("%sの動きが遅くなった。",
					                    "%^s starts moving slower."), t_name);
			}
		}

		wake_up = TRUE;

		break;

	/* RF5_HOLD */
	case 128+31:
		if (known)
		{
			if (see_either)
			{
				msg_format(_("%^sは%sをじっと見つめた。", "%^s stares intently at %s."), m_name, t_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		if ((tr_ptr->flags1 & RF1_UNIQUE) ||
		    (tr_ptr->flags3 & RF3_NO_STUN))
		{
			if (see_t) msg_format(_("%^sには効果がなかった。", "%^s is unaffected."), t_name);
		}
		else if (tr_ptr->level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10)
		{
			if (see_t) msg_format(_("%^sには効果がなかった。", "%^s is unaffected."), t_name);
		}
		else
		{
			if (see_t) msg_format(_("%^sは麻痺した！", "%^s is paralyzed!"), t_name);

			(void)set_monster_stunned(t_idx, MON_STUNNED(t_ptr) + randint1(4) + 4);
		}

		wake_up = TRUE;

		break;


	/* RF6_HASTE */
	case 160+0:
		if (known)
		{
			if (see_m)
			{
				msg_format(_("%^sが自分の体に念を送った。", "%^s concentrates on %s body."), m_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		/* Allow quick speed increases to base+10 */
		if (set_monster_fast(m_idx, MON_FAST(m_ptr) + 100))
		{
			if (see_m) msg_format(_("%^sの動きが速くなった。", "%^s starts moving faster."), m_name);
		}
		break;

	/* RF6_HAND_DOOM */
	case 160+1:
		if (known)
		{
			if (see_m)
			{
				msg_format(_("%^sが%sに<破滅の手>を放った！", "%^s invokes the Hand of Doom upon %s!"), m_name, t_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		dam = 20; /* Dummy power */
        breath(y, x, m_idx,GF_HAND_DOOM, dam, 0, FALSE, MS_HAND_DOOM, MONSTER_TO_MONSTER);

		break;

	/* RF6_HEAL */
	case 160+2:
		if (known)
		{
			if (see_m)
			{
				msg_format(_("%^sは自分の傷に念を集中した。", "%^s concentrates on %s wounds."), m_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		/* Heal some */
		m_ptr->hp += (rlev * 6);

		/* Fully healed */
		if (m_ptr->hp >= m_ptr->maxhp)
		{
			/* Fully healed */
			m_ptr->hp = m_ptr->maxhp;

			if (known)
			{
				if (see_m)
				{
					msg_format(_("%^sは完全に治った！", "%^s looks completely healed!"), m_name);
				}
				else
				{
					mon_fight = TRUE;
				}
			}
		}

		/* Partially healed */
		else if (known)
		{
			if (see_m)
			{
				msg_format(_("%^sは体力を回復したようだ。", "%^s looks healthier."), m_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		/* Redraw (later) if needed */
		if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
		if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);

		/* Cancel fear */
		if (MON_MONFEAR(m_ptr))
		{
			/* Cancel fear */
			(void)set_monster_monfear(m_idx, 0);

			/* Message */
			if (see_m) msg_format(_("%^sは勇気を取り戻した。", "%^s recovers %s courage."), m_name);
		}

		break;

	/* RF6_INVULNER */
	case 160+3:
		if (known)
		{
			if (see_m)
			{
				disturb(1, 1);
				msg_format(_("%sは無傷の球の呪文を唱えた。", "%^s casts a Globe of Invulnerability."), m_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		if (!MON_INVULNER(m_ptr)) (void)set_monster_invulner(m_idx, randint1(4) + 4, FALSE);
		break;

	/* RF6_BLINK */
	case 160+4:
		if (teleport_barrier(m_idx))
		{
			if (see_m)
			{
				msg_format(_("魔法のバリアが%^sのテレポートを邪魔した。", "Magic barrier obstructs teleporting of %^s."), m_name);
			}
		}
		else
		{
			if (see_m)
			{
				msg_format(_("%^sが瞬時に消えた。", "%^s blinks away."), m_name);
			}
			teleport_away(m_idx, 10, 0L);
		}
		break;

	/* RF6_TPORT */
	case 160+5:
		if (teleport_barrier(m_idx))
		{
			if (see_m)
			{
				msg_format(_("魔法のバリアが%^sのテレポートを邪魔した。", "Magic barrier obstructs teleporting of %^s."), m_name);
			}
		}
		else
		{
			if (see_m)
			{
				msg_format(_("%^sがテレポートした。", "%^s teleports away."), m_name);
			}
			teleport_away_followable(m_idx);
		}
		break;

	/* RF6_WORLD */
	case 160+6:
#if 0
		int who = 0;
		if(m_ptr->r_idx = MON_DIO) who == 1;
		else if(m_ptr->r_idx = MON_WONG) who == 3;
		dam = who;
		if(!process_the_world(randint1(2)+2, who, player_has_los_bold(m_ptr->fy, m_ptr->fx))) return (FALSE);
#endif
		return FALSE;

	/* RF6_SPECIAL */
	case 160+7:
		switch (m_ptr->r_idx)
		{
		case MON_OHMU:
			/* Moved to process_monster(), like multiplication */
			return FALSE;

		case MON_ROLENTO:
			if (known)
			{
				if (see_either)
				{
					disturb(1, 1);

					msg_format(_("%^sは手榴弾をばらまいた。", "%^s throws some hand grenades."), m_name);
				}
				else
				{
					mon_fight = TRUE;
				}
			}

			{
				int num = 1 + randint1(3);
				for (k = 0; k < num; k++)
				{
					count += summon_named_creature(m_idx, y, x, MON_SHURYUUDAN, 0);
				}
			}

			if (known && !see_t && count)
			{
				mon_fight = TRUE;
			}
			break;

		default:
			if (r_ptr->d_char == 'B')
			{
				if (one_in_(3))
				{
					if (see_m)
					{
						msg_format(_("%^sは突然急上昇して視界から消えた!", "%^s suddenly go out of your sight!"), m_name);
					}
					teleport_away(m_idx, 10, TELEPORT_NONMAGICAL);
					p_ptr->update |= (PU_MONSTERS);
				}
				else
				{
					if (known)
					{
						if (see_either)
						{
							msg_format(_("%^sが%sを掴んで空中から投げ落とした。", "%^s holds %s, and drops from the sky."), m_name, t_name);
						}
						else
						{
							mon_fight = TRUE;
						}
					}

					dam = damroll(4, 8);

					if (t_idx == p_ptr->riding) teleport_player_to(m_ptr->fy, m_ptr->fx, TELEPORT_NONMAGICAL | TELEPORT_PASSIVE);
					else teleport_monster_to(t_idx, m_ptr->fy, m_ptr->fx, 100, TELEPORT_NONMAGICAL | TELEPORT_PASSIVE);

					sound(SOUND_FALL);

					if (tr_ptr->flags7 & RF7_CAN_FLY)
					{
						if (see_t) msg_format(_("%^sは静かに着地した。", "%^s floats gently down to the ground."), t_name);
					}
					else
					{
						if (see_t) msg_format(_("%^sは地面に叩きつけられた。", "%^s crashed into the ground."), t_name);

						dam += damroll(6, 8);
					}

					if (p_ptr->riding == t_idx)
					{
						int get_damage = 0;

						/* Mega hack -- this special action deals damage to the player. Therefore the code of "eyeeye" is necessary.
						   -- henkma
						 */
						get_damage = take_hit(DAMAGE_NOESCAPE, dam, m_name, -1);
						if (p_ptr->tim_eyeeye && get_damage > 0 && !p_ptr->is_dead)
						{
							char m_name_self[80];

							/* hisself */
							monster_desc(m_name_self, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE | MD_OBJECTIVE);

							_(msg_format("攻撃が%s自身を傷つけた！", m_name),
							  msg_format("The attack of %s has wounded %s!", m_name, m_name_self));

							project(0, 0, m_ptr->fy, m_ptr->fx, get_damage, GF_MISSILE, PROJECT_KILL, -1);
							set_tim_eyeeye(p_ptr->tim_eyeeye-5, TRUE);
						}
					}

					mon_take_hit_mon(t_idx, dam, &fear, extract_note_dies(real_r_ptr(t_ptr)), m_idx);
				}
				break;
			}

			/* Something is wrong */
			else return FALSE;
		}

		/* done */
		break;

	/* RF6_TELE_TO */
	case 160+8:
		if (known)
		{
			if (see_either)
			{
				msg_format(_("%^sが%sを引き戻した。", "%^s commands %s to return."), m_name, t_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		if (tr_ptr->flagsr & RFR_RES_TELE)
		{
			if ((tr_ptr->flags1 & RF1_UNIQUE) || (tr_ptr->flagsr & RFR_RES_ALL))
			{
				if (is_original_ap_and_seen(t_ptr)) tr_ptr->r_flagsr |= RFR_RES_TELE;
				if (see_t)
				{
					msg_format(_("%^sには効果がなかった。", "%^s is unaffected!"), t_name);
				}

				resists_tele = TRUE;
			}
			else if (tr_ptr->level > randint1(100))
			{
				if (is_original_ap_and_seen(t_ptr)) tr_ptr->r_flagsr |= RFR_RES_TELE;
				if (see_t)
				{
					msg_format(_("%^sは耐性を持っている！", "%^s resists!"), t_name);
				}

				resists_tele = TRUE;
			}
		}

		if (!resists_tele)
		{
			if (t_idx == p_ptr->riding) teleport_player_to(m_ptr->fy, m_ptr->fx, TELEPORT_PASSIVE);
			else teleport_monster_to(t_idx, m_ptr->fy, m_ptr->fx, 100, TELEPORT_PASSIVE);
		}

		wake_up = TRUE;
		break;

	/* RF6_TELE_AWAY */
	case 160+9:
		if (known)
		{
			if (see_either)
			{
				msg_format(_("%^sは%sをテレポートさせた。", "%^s teleports %s away."), m_name, t_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		if (tr_ptr->flagsr & RFR_RES_TELE)
		{
			if ((tr_ptr->flags1 & RF1_UNIQUE) || (tr_ptr->flagsr & RFR_RES_ALL))
			{
				if (is_original_ap_and_seen(t_ptr)) tr_ptr->r_flagsr |= RFR_RES_TELE;
				if (see_t)
				{
					msg_format(_("%^sには効果がなかった。", "%^s is unaffected!"), t_name);
				}

				resists_tele = TRUE;
			}
			else if (tr_ptr->level > randint1(100))
			{
				if (is_original_ap_and_seen(t_ptr)) tr_ptr->r_flagsr |= RFR_RES_TELE;
				if (see_t)
				{
					msg_format(_("%^sは耐性を持っている！", "%^s resists!"), t_name);
				}

				resists_tele = TRUE;
			}
		}

		if (!resists_tele)
		{
			if (t_idx == p_ptr->riding) teleport_player_away(m_idx, MAX_SIGHT * 2 + 5);
			else teleport_away(t_idx, MAX_SIGHT * 2 + 5, TELEPORT_PASSIVE);
		}

		wake_up = TRUE;
		break;

	/* RF6_TELE_LEVEL */
	case 160+10:
		if (known)
		{
			if (see_either)
			{
				msg_format(_("%^sが%sの足を指さした。", "%^s gestures at %s's feet."), m_name, t_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		if (tr_ptr->flagsr & (RFR_EFF_RES_NEXU_MASK | RFR_RES_TELE))
		{
			if (see_t) msg_format(_("%^sには効果がなかった。", "%^s is unaffected!"), t_name);
		}
		else if ((tr_ptr->flags1 & RF1_QUESTOR) ||
			    (tr_ptr->level > randint1((rlev - 10) < 1 ? 1 : (rlev - 10)) + 10))
		{
			if (see_t) msg_format(_("%^sは効力を跳ね返した！", "%^s resist the effects!"), t_name);
		}
		else teleport_level((t_idx == p_ptr->riding) ? 0 : t_idx);

		wake_up = TRUE;
		break;

	/* RF6_PSY_SPEAR */
	case 160+11:
		if (known)
		{
			if (see_either)
			{
				msg_format(_("%^sが%sに向かって光の剣を放った。", "%^s throw a Psycho-spear at %s."), m_name, t_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		dam = (r_ptr->flags2 & RF2_POWERFUL) ? (randint1(rlev * 2) + 180) : (randint1(rlev * 3 / 2) + 120);
		beam(m_idx, y, x, GF_PSY_SPEAR, dam, MS_PSY_SPEAR, MONSTER_TO_MONSTER);
		break;

	/* RF6_DARKNESS */
	case 160+12:
		if (known)
		{
			if (see_m)
			{
				if (can_use_lite_area)
				{
					msg_format(_("%^sが辺りを明るく照らした。", "%^s cast a spell to light up."), m_name);
				}
				else
				{
					msg_format(_("%^sが暗闇の中で手を振った。", "%^s gestures in shadow."), m_name);
				}

				if (see_t)
				{
					if (can_use_lite_area)
					{
						msg_format(_("%^sは白い光に包まれた。", "%^s is surrounded by a white light."), t_name);
					}
					else
					{
						msg_format(_("%^sは暗闇に包まれた。", "%^s is surrounded by darkness."), t_name);
					}
				}
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		if (can_use_lite_area)
		{
			(void)project(m_idx, 3, y, x, 0, GF_LITE_WEAK, PROJECT_GRID | PROJECT_KILL, -1);
			lite_room(y, x);
		}
		else
		{
			(void)project(m_idx, 3, y, x, 0, GF_DARK_WEAK, PROJECT_GRID | PROJECT_KILL, MS_DARKNESS);
			unlite_room(y, x);
		}

		break;

	/* RF6_TRAPS */
	case 160+13:
#if 0
		if (known)
		{
			if (see_m)
			{
				msg_format(_("%^sが呪文を唱えて邪悪に微笑んだ。", "%^s casts a spell and cackles evilly."), m_name);
			}
			else
			{
				msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
			}
		}

		trap_creation(y, x);

		break;
#else
		/* Not implemented */
		return FALSE;
#endif

	/* RF6_FORGET */
	case 160+14:
		/* Not implemented */
		return FALSE;

	/* RF6_RAISE_DEAD */
	case 160+15:
		if (known)
		{
			if (see_either)
			{
				disturb(1, 1);
				if (blind)
				{
					msg_format(_("%^sが何かをつぶやいた。", "%^s mumbles."), m_name);
				}
				else
				{
					msg_format(_("%^sが死者復活の呪文を唱えた。", "%^s casts a spell to revive corpses."), m_name);
				}
			}
			else
			{
				mon_fight = TRUE;
			}
		}
		animate_dead(m_idx, m_ptr->fy, m_ptr->fx);
		break;

	/* RF6_S_KIN */
	case 160+16:
		if (known)
		{
			if (see_either)
			{
				disturb(1, 1);

				if (m_ptr->r_idx == MON_SERPENT || m_ptr->r_idx == MON_ZOMBI_SERPENT)
				{
					msg_format(_("%^sがダンジョンの主を召喚した。", "%^s magically summons guardians of dungeons."), m_name);
				}
				else
				{
					_(msg_format("%sが魔法で%sを召喚した。", m_name, ((r_ptr->flags1 & RF1_UNIQUE) ? "手下" : "仲間")),   
					  msg_format("%^s magically summons %s %s.", m_name, m_poss, ((r_ptr->flags1 & RF1_UNIQUE) ? "minions" : "kin")));
				}

			}
			else
			{
				mon_fight = TRUE;
			}
		}

		switch (m_ptr->r_idx)
		{
		case MON_MENELDOR:
		case MON_GWAIHIR:
		case MON_THORONDOR:
			{
				int num = 4 + randint1(3);
				for (k = 0; k < num; k++)
				{
					count += summon_specific(m_idx, y, x, rlev, SUMMON_EAGLES, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE));
				}
			}
			break;

		case MON_BULLGATES:
			{
				int num = 2 + randint1(3);
				for (k = 0; k < num; k++)
				{
					count += summon_named_creature(m_idx, y, x, MON_IE, 0);
				}
			}
			break;

		case MON_SERPENT:
		case MON_ZOMBI_SERPENT:
			if (r_info[MON_JORMUNGAND].cur_num < r_info[MON_JORMUNGAND].max_num && one_in_(6))
			{
				if (known && see_t)
				{
					msg_print(_("地面から水が吹き出した！", "Water blew off from the ground!"));
				}
				project(t_idx, 8, y, x, 3, GF_WATER_FLOW, PROJECT_GRID | PROJECT_HIDE, -1);
			}

			{
				int num = 2 + randint1(3);
				for (k = 0; k < num; k++)
				{
					count += summon_specific(m_idx, y, x, rlev, SUMMON_GUARDIANS, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE));
				}
			}
			break;

		case MON_CALDARM:
			{
				int num = randint1(3);
				for (k = 0; k < num; k++)
				{
					count += summon_named_creature(m_idx, y, x, MON_LOCKE_CLONE, 0);
				}
			}
			break;

		case MON_LOUSY:
			{
				int num = 2 + randint1(3);
				for (k = 0; k < num; k++)
				{
					count += summon_specific(m_idx, y, x, rlev, SUMMON_LOUSE, (PM_ALLOW_GROUP));
				}
			}
			break;

		default:
			summon_kin_type = r_ptr->d_char;

			for (k = 0; k < 4; k++)
			{
				count += summon_specific(m_idx, y, x, rlev, SUMMON_KIN, (PM_ALLOW_GROUP));
			}
			break;
		}

		if (known && !see_t && count)
		{
			mon_fight = TRUE;
		}

		break;

	/* RF6_S_CYBER */
	case 160+17:
		if (known)
		{
			if (see_either)
			{
				disturb(1, 1);

				msg_format(_("%^sがサイバーデーモンを召喚した！", "%^s magically summons Cyberdemons!"), m_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		if (is_friendly(m_ptr))
		{
			count += summon_specific(m_idx, y, x, rlev, SUMMON_CYBER, (PM_ALLOW_GROUP));
		}
		else
		{
			count += summon_cyber(m_idx, y, x);
		}

		if (known && !see_t && count)
		{
			mon_fight = TRUE;
		}

		break;

	/* RF6_S_MONSTER */
	case 160+18:
		if (known)
		{
			if (see_either)
			{
				disturb(1, 1);

				msg_format(_("%^sが魔法で仲間を召喚した！", "%^s magically summons help!"), m_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		count += summon_specific(m_idx, y, x, rlev, 0, (u_mode));

		if (known && !see_t && count)
		{
			mon_fight = TRUE;
		}

		break;

	/* RF6_S_MONSTERS */
	case 160+19:
		if (known)
		{
			if (see_either)
			{
				disturb(1, 1);

				msg_format(_("%^sが魔法でモンスターを召喚した！", "%^s magically summons monsters!"), m_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		for (k = 0; k < s_num_6; k++)
		{
			count += summon_specific(m_idx, y, x, rlev, 0, (PM_ALLOW_GROUP | u_mode));
		}

		if (known && !see_t && count)
		{
			mon_fight = TRUE;
		}

		break;

	/* RF6_S_ANT */
	case 160+20:
		if (known)
		{
			if (see_either)
			{
				disturb(1, 1);

				msg_format(_("%^sが魔法でアリを召喚した。", "%^s magically summons ants."), m_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		for (k = 0; k < s_num_6; k++)
		{
			count += summon_specific(m_idx, y, x, rlev, SUMMON_ANT, (PM_ALLOW_GROUP));
		}

		if (known && !see_t && count)
		{
			mon_fight = TRUE;
		}

		break;

	/* RF6_S_SPIDER */
	case 160+21:
		if (known)
		{
			if (see_either)
			{
				disturb(1, 1);

				msg_format(_("%^sが魔法でクモを召喚した。", "%^s magically summons spiders."), m_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		for (k = 0; k < s_num_6; k++)
		{
			count += summon_specific(m_idx, y, x, rlev, SUMMON_SPIDER, (PM_ALLOW_GROUP));
		}

		if (known && !see_t && count)
		{
			mon_fight = TRUE;
		}

		break;

	/* RF6_S_HOUND */
	case 160+22:
		if (known)
		{
			if (see_either)
			{
				disturb(1, 1);

				msg_format(_("%^sが魔法でハウンドを召喚した。", "%^s magically summons hounds."), m_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		for (k = 0; k < s_num_4; k++)
		{
			count += summon_specific(m_idx, y, x, rlev, SUMMON_HOUND, (PM_ALLOW_GROUP));
		}

		if (known && !see_t && count)
		{
			mon_fight = TRUE;
		}

		break;

	/* RF6_S_HYDRA */
	case 160+23:
		if (known)
		{
			if (see_either)
			{
				disturb(1, 1);

				msg_format(_("%^sが魔法でヒドラを召喚した。", "%^s magically summons hydras."), m_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		for (k = 0; k < s_num_4; k++)
		{
			count += summon_specific(m_idx, y, x, rlev, SUMMON_HYDRA, (PM_ALLOW_GROUP));
		}

		if (known && !see_t && count)
		{
			mon_fight = TRUE;
		}

		break;

	/* RF6_S_ANGEL */
	case 160+24:
		if (known)
		{
			if (see_either)
			{
				disturb(1, 1);

				msg_format(_("%^sが魔法で天使を召喚した！", "%^s magically summons an angel!"), m_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		{
			int num = 1;

			if ((r_ptr->flags1 & RF1_UNIQUE) && !easy_band)
			{
				num += r_ptr->level/40;
			}

			for (k = 0; k < num; k++)
			{
				count += summon_specific(m_idx, y, x, rlev, SUMMON_ANGEL, (PM_ALLOW_GROUP));
			}
		}

		if (known && !see_t && count)
		{
			mon_fight = TRUE;
		}

		break;

	/* RF6_S_DEMON */
	case 160+25:
		if (known)
		{
			if (see_either)
			{
				disturb(1, 1);

				msg_format(_("%^sが魔法で混沌の宮廷からデーモンを召喚した！", 
					         "%^s magically summons a demon from the Courts of Chaos!"), m_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		for (k = 0; k < 1; k++)
		{
			count += summon_specific(m_idx, y, x, rlev, SUMMON_DEMON, (PM_ALLOW_GROUP));
		}

		if (known && !see_t && count)
		{
			mon_fight = TRUE;
		}

		break;

	/* RF6_S_UNDEAD */
	case 160+26:
		if (known)
		{
			if (see_either)
			{
				disturb(1, 1);

				msg_format(_("%sが魔法でアンデッドを召喚した。", "%^s magically summons undead."), m_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		for (k = 0; k < 1; k++)
		{
			count += summon_specific(m_idx, y, x, rlev, SUMMON_UNDEAD, (PM_ALLOW_GROUP));
		}

		if (known && !see_t && count)
		{
			mon_fight = TRUE;
		}

		break;

	/* RF6_S_DRAGON */
	case 160+27:
		if (known)
		{
			if (see_either)
			{
				disturb(1, 1);

				msg_format(_("%^sが魔法でドラゴンを召喚した！", "%^s magically summons a dragon!"), m_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		for (k = 0; k < 1; k++)
		{
			count += summon_specific(m_idx, y, x, rlev, SUMMON_DRAGON, (PM_ALLOW_GROUP));
		}

		if (known && !see_t && count)
		{
			mon_fight = TRUE;
		}

		break;

	/* RF6_S_HI_UNDEAD */
	case 160+28:
		if (known)
		{
			if (see_either)
			{
				disturb(1, 1);

				msg_format(_("%sが魔法でアンデッドを召喚した。", "%^s magically summons undead."), m_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		for (k = 0; k < s_num_6; k++)
		{
			count += summon_specific(m_idx, y, x, rlev, SUMMON_HI_UNDEAD, (PM_ALLOW_GROUP | u_mode));
		}

		if (known && !see_t && count)
		{
			mon_fight = TRUE;
		}

		break;

	/* RF6_S_HI_DRAGON */
	case 160+29:
		if (known)
		{
			if (see_either)
			{
				disturb(1, 1);

				msg_format(_("%^sが魔法で古代ドラゴンを召喚した！", "%^s magically summons ancient dragons!"), m_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		for (k = 0; k < s_num_4; k++)
		{
			count += summon_specific(m_idx, y, x, rlev, SUMMON_HI_DRAGON, (PM_ALLOW_GROUP | u_mode));
		}

		if (known && !see_t && count)
		{
			mon_fight = TRUE;
		}

		break;

	/* RF6_S_AMBERITES */
	case 160+30:
		if (known)
		{
			if (see_either)
			{
				disturb(1, 1);

				msg_format(_("%^sがアンバーの王族を召喚した！", "%^s magically summons Lords of Amber!"), m_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		for (k = 0; k < s_num_4; k++)
		{
			count += summon_specific(m_idx, y, x, rlev, SUMMON_AMBERITES, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE));
		}

		if (known && !see_t && count)
		{
			mon_fight = TRUE;
		}

		break;

	/* RF6_S_UNIQUE */
	case 160+31:
		if (known)
		{
			if (see_either)
			{
				disturb(1, 1);

				msg_format(_("%^sが魔法で特別な強敵を召喚した！", "%^s magically summons special opponents!"), m_name);
			}
			else
			{
				mon_fight = TRUE;
			}
		}

		for (k = 0; k < s_num_4; k++)
		{
			count += summon_specific(m_idx, y, x, rlev, SUMMON_UNIQUE, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE));
		}

		{
			int non_unique_type = SUMMON_HI_UNDEAD;

			if ((m_ptr->sub_align & (SUB_ALIGN_GOOD | SUB_ALIGN_EVIL)) == (SUB_ALIGN_GOOD | SUB_ALIGN_EVIL))
				non_unique_type = 0;
			else if (m_ptr->sub_align & SUB_ALIGN_GOOD)
				non_unique_type = SUMMON_ANGEL;

			for (k = count; k < s_num_4; k++)
			{
				count += summon_specific(m_idx, y, x, rlev, non_unique_type, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE));
			}
		}

		if (known && !see_t && count)
		{
			mon_fight = TRUE;
		}

		break;
	}

	if (wake_up) (void)set_monster_csleep(t_idx, 0);

	if (fear && see_t)
	{
		msg_format(_("%^sは恐怖して逃げ出した！", "%^s flees in terror!"), t_name);
	}

	if (m_ptr->ml && maneable && !world_monster && !p_ptr->blind && (p_ptr->pclass == CLASS_IMITATOR))
	{
		if (thrown_spell != 167) /* Not RF6_SPECIAL */
		{
			if (p_ptr->mane_num == MAX_MANE)
			{
				p_ptr->mane_num--;
				for (i = 0; i < p_ptr->mane_num - 1; i++)
				{
					p_ptr->mane_spell[i] = p_ptr->mane_spell[i+1];
					p_ptr->mane_dam[i] = p_ptr->mane_dam[i+1];
				}
			}
			p_ptr->mane_spell[p_ptr->mane_num] = thrown_spell - 96;
			p_ptr->mane_dam[p_ptr->mane_num] = dam;
			p_ptr->mane_num++;
			new_mane = TRUE;

			p_ptr->redraw |= (PR_IMITATION);
		}
	}

	/* Remember what the monster did, if we saw it */
	if (can_remember)
	{
		/* Inate spell */
		if (thrown_spell < 32*4)
		{
			r_ptr->r_flags4 |= (1L << (thrown_spell - 32*3));
			if (r_ptr->r_cast_spell < MAX_UCHAR) r_ptr->r_cast_spell++;
		}

		/* Bolt or Ball */
		else if (thrown_spell < 32*5)
		{
			r_ptr->r_flags5 |= (1L << (thrown_spell - 32*4));
			if (r_ptr->r_cast_spell < MAX_UCHAR) r_ptr->r_cast_spell++;
		}

		/* Special spell */
		else if (thrown_spell < 32*6)
		{
			r_ptr->r_flags6 |= (1L << (thrown_spell - 32*5));
			if (r_ptr->r_cast_spell < MAX_UCHAR) r_ptr->r_cast_spell++;
		}
	}

	/* Always take note of monsters that kill you */
	if (p_ptr->is_dead && (r_ptr->r_deaths < MAX_SHORT) && !p_ptr->inside_arena)
	{
		r_ptr->r_deaths++; /* Ignore appearance difference */
	}

	/* A spell was cast */
	return TRUE;
}
