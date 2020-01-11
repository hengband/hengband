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
#include "util.h"

#include "cmd-pet.h"
#include "floor.h"
#include "grid.h"
#include "quest.h"
#include "realm-hex.h"
#include "player-move.h"
#include "player-class.h"
#include "monster.h"
#include "monster-status.h"
#include "monster-spell.h"
#include "spells.h"
#include "dungeon.h"
#include "world.h"
#include "view-mainwindow.h"

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
static bool direct_beam(POSITION y1, POSITION x1, POSITION y2, POSITION x2, monster_type *m_ptr)
{
	bool hit2 = FALSE;
	int i;
	POSITION y, x;

	int grid_n = 0;
	u16b grid_g[512];

	bool is_friend = is_pet(m_ptr);

	/* Check the projection path */
	grid_n = project_path(p_ptr->current_floor_ptr, grid_g, MAX_RANGE, y1, x1, y2, x2, PROJECT_THRU);

	/* No grid is ever projectable from itself */
	if (!grid_n) return FALSE;

	for (i = 0; i < grid_n; i++)
	{
		y = GRID_Y(grid_g[i]);
		x = GRID_X(grid_g[i]);

		if (y == y2 && x == x2)
			hit2 = TRUE;
		else if (is_friend && p_ptr->current_floor_ptr->grid_array[y][x].m_idx > 0 &&
			 !are_enemies(m_ptr, &p_ptr->current_floor_ptr->m_list[p_ptr->current_floor_ptr->grid_array[y][x].m_idx]))
		{
			/* Friends don't shoot friends */
			return FALSE;
		}

		if (is_friend && player_bold(p_ptr, y, x))
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
static bool breath_direct(player_type *master_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, POSITION rad, EFFECT_ID typ, bool is_friend)
{
	/* Must be the same as projectable() */

	int i;

	/* Initial grid */
	POSITION y = y1;
	POSITION x = x1;

	int grid_n = 0;
	u16b grid_g[512];

	int grids = 0;
	POSITION gx[1024], gy[1024];
	POSITION gm[32];
	POSITION gm_rad = rad;

	bool hit2 = FALSE;
	bool hityou = FALSE;

	BIT_FLAGS flg;

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
	grid_n = project_path(master_ptr->current_floor_ptr, grid_g, MAX_RANGE, y1, x1, y2, x2, flg);

	/* Project along the path */
	for (i = 0; i < grid_n; ++i)
	{
		int ny = GRID_Y(grid_g[i]);
		int nx = GRID_X(grid_g[i]);

		if (flg & PROJECT_DISI)
		{
			/* Hack -- Balls explode before reaching walls */
			if (cave_stop_disintegration(master_ptr->current_floor_ptr, ny, nx)) break;
		}
		else if (flg & PROJECT_LOS)
		{
			/* Hack -- Balls explode before reaching walls */
			if (!cave_los_bold(master_ptr->current_floor_ptr, ny, nx)) break;
		}
		else
		{
			/* Hack -- Balls explode before reaching walls */
			if (!cave_have_flag_bold(master_ptr->current_floor_ptr, ny, nx, FF_PROJECT)) break;
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
			if (in_disintegration_range(master_ptr->current_floor_ptr, y1, x1, y2, x2) && (distance(y1, x1, y2, x2) <= rad)) hit2 = TRUE;
			if (in_disintegration_range(master_ptr->current_floor_ptr, y1, x1, master_ptr->y, master_ptr->x) && (distance(y1, x1, master_ptr->y, master_ptr->x) <= rad)) hityou = TRUE;
		}
		else if (flg & PROJECT_LOS)
		{
			if (los(master_ptr->current_floor_ptr, y1, x1, y2, x2) && (distance(y1, x1, y2, x2) <= rad)) hit2 = TRUE;
			if (los(master_ptr->current_floor_ptr, y1, x1, master_ptr->y, master_ptr->x) && (distance(y1, x1, master_ptr->y, master_ptr->x) <= rad)) hityou = TRUE;
		}
		else
		{
			if (projectable(master_ptr->current_floor_ptr, y1, x1, y2, x2) && (distance(y1, x1, y2, x2) <= rad)) hit2 = TRUE;
			if (projectable(master_ptr->current_floor_ptr, y1, x1, master_ptr->y, master_ptr->x) && (distance(y1, x1, master_ptr->y, master_ptr->x) <= rad)) hityou = TRUE;
		}
	}
	else
	{
		breath_shape(master_ptr->current_floor_ptr, grid_g, grid_n, &grids, gx, gy, gm, &gm_rad, rad, y1, x1, y, x, typ);

		for (i = 0; i < grids; i++)
		{
			y = gy[i];
			x = gx[i];
			if ((y == y2) && (x == x2)) hit2 = TRUE;
			if (player_bold(master_ptr, y, x)) hityou = TRUE;
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
void get_project_point(floor_type *floor_ptr, POSITION sy, POSITION sx, POSITION *ty, POSITION *tx, BIT_FLAGS flg)
{
	u16b path_g[128];
	int  path_n, i;

	path_n = project_path(floor_ptr, path_g, MAX_RANGE, sy, sx, *ty, *tx, flg);

	*ty = sy;
	*tx = sx;

	/* Project along the path */
	for (i = 0; i < path_n; i++)
	{
		sy = GRID_Y(path_g[i]);
		sx = GRID_X(path_g[i]);

		/* Hack -- Balls explode before reaching walls */
		if (!cave_have_flag_bold(floor_ptr, sy, sx, FF_PROJECT)) break;

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
static bool dispel_check_monster(MONSTER_IDX m_idx, MONSTER_IDX t_idx)
{
	monster_type *t_ptr = &p_ptr->current_floor_ptr->m_list[t_idx];

	if (MON_INVULNER(t_ptr)) return TRUE;

	if (t_ptr->mspeed < 135)
	{
		if (MON_FAST(t_ptr)) return TRUE;
	}

	/* Riding monster */
	if (t_idx == p_ptr->riding)
	{
		if (dispel_check(p_ptr, m_idx)) return TRUE;
	}

	/* No need to cast dispel spell */
	return FALSE;
}


/*!
 * todo モンスターからモンスターへの呪文なのにplayer_typeが引数になり得るのは間違っている……
 * @brief モンスターが敵モンスターに特殊能力を使う処理のメインルーチン /
 * Monster tries to 'cast a spell' (or breath, etc) at another monster.
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx 術者のモンスターID
 * @return 実際に特殊能力を使った場合TRUEを返す
 * @details
 * The player is only disturbed if able to be affected by the spell.
 */
bool monst_spell_monst(player_type *target_ptr, MONSTER_IDX m_idx)
{
	POSITION y = 0, x = 0;
	int i, k;
	MONSTER_IDX target_idx = 0;
	int thrown_spell;
	HIT_POINT dam = 0;
	int start;
	int plus = 1;

	byte spell[96], num = 0;

	GAME_TEXT m_name[160];
	GAME_TEXT t_name[160];

#ifndef JP
	char m_poss[160];
#endif

	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	monster_type *m_ptr = &floor_ptr->m_list[m_idx];
	monster_type *t_ptr = NULL;

	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	u32b f4, f5, f6;

	bool see_m = is_seen(m_ptr);
	bool maneable = player_has_los_bold(target_ptr, m_ptr->fy, m_ptr->fx);
	bool pet = is_pet(m_ptr);

	bool in_no_magic_dungeon = (d_info[target_ptr->dungeon_idx].flags1 & DF1_NO_MAGIC) && floor_ptr->dun_level
		&& (!floor_ptr->inside_quest || is_fixed_quest_idx(floor_ptr->inside_quest));

	bool can_use_lite_area = FALSE;
	bool can_remember;

	/* Cannot cast spells when confused */
	if (MON_CONFUSED(m_ptr)) return FALSE;

	/* Extract the racial spell flags */
	f4 = r_ptr->flags4;
	f5 = r_ptr->a_ability_flags1;
	f6 = r_ptr->a_ability_flags2;

	/* Target is given for pet? */
	if (target_ptr->pet_t_m_idx && pet)
	{
		target_idx = target_ptr->pet_t_m_idx;
		t_ptr = &floor_ptr->m_list[target_idx];

		/* Cancel if not projectable (for now) */
		if ((m_idx == target_idx) || !projectable(floor_ptr, m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx))
		{
			target_idx = 0;
		}
	}

	/* Is there counter attack target? */
	if (!target_idx && m_ptr->target_y)
	{
		target_idx = floor_ptr->grid_array[m_ptr->target_y][m_ptr->target_x].m_idx;

		if (target_idx)
		{
			t_ptr = &floor_ptr->m_list[target_idx];

			/* Cancel if neither enemy nor a given target */
			if ((m_idx == target_idx) ||
			    ((target_idx != target_ptr->pet_t_m_idx) && !are_enemies(m_ptr, t_ptr)))
			{
				target_idx = 0;
			}

			/* Allow only summoning etc.. if not projectable */
			else if (!projectable(floor_ptr, m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx))
			{
				f4 &= (RF4_INDIRECT_MASK);
				f5 &= (RF5_INDIRECT_MASK);
				f6 &= (RF6_INDIRECT_MASK);
			}
		}
	}

	/* Look for enemies normally */
	if (!target_idx)
	{
		bool success = FALSE;

		if (target_ptr->phase_out)
		{
			start = randint1(floor_ptr->m_max-1) + floor_ptr->m_max;
			if (randint0(2)) plus = -1;
		}
		else start = floor_ptr->m_max + 1;

		/* Scan thru all monsters */
		for (i = start; ((i < start + floor_ptr->m_max) && (i > start - floor_ptr->m_max)); i += plus)
		{
			MONSTER_IDX dummy = (i % floor_ptr->m_max);
			if (!dummy) continue;

			target_idx = dummy;
			t_ptr = &floor_ptr->m_list[target_idx];

			if (!monster_is_valid(t_ptr)) continue;

			/* Monster must be 'an enemy' */
			if ((m_idx == target_idx) || !are_enemies(m_ptr, t_ptr)) continue;

			/* Monster must be projectable */
			if (!projectable(floor_ptr, m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx)) continue;

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

	/* Forget old counter attack target */
	reset_target(m_ptr);

	/* Remove unimplemented spells */
	f6 &= ~(RF6_WORLD | RF6_TRAPS | RF6_FORGET);

	if (f4 & RF4_BR_LITE)
	{
		if (!los(floor_ptr, m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx))
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
		bool vs_ninja = (target_ptr->pclass == CLASS_NINJA) && !is_hostile(t_ptr);

		if (vs_ninja &&
		    !(r_ptr->flags3 & (RF3_UNDEAD | RF3_HURT_LITE)) &&
		    !(r_ptr->flags7 & RF7_DARK_MASK))
			can_use_lite_area = TRUE;

		if (!(r_ptr->flags2 & RF2_STUPID))
		{
			if (d_info[target_ptr->dungeon_idx].flags1 & DF1_DARKNESS) f6 &= ~(RF6_DARKNESS);
			else if (vs_ninja && !can_use_lite_area) f6 &= ~(RF6_DARKNESS);
		}
	}

	if (in_no_magic_dungeon && !(r_ptr->flags2 & RF2_STUPID))
	{
		f4 &= (RF4_NOMAGIC_MASK);
		f5 &= (RF5_NOMAGIC_MASK);
		f6 &= (RF6_NOMAGIC_MASK);
	}

	if (floor_ptr->inside_arena || target_ptr->phase_out)
	{
		f4 &= ~(RF4_SUMMON_MASK);
		f5 &= ~(RF5_SUMMON_MASK);
		f6 &= ~(RF6_SUMMON_MASK | RF6_TELE_LEVEL);

		if (m_ptr->r_idx == MON_ROLENTO) f6 &= ~(RF6_SPECIAL);
	}

	if (target_ptr->phase_out && !one_in_(3))
	{
		f6 &= ~(RF6_HEAL);
	}

	if (m_idx == target_ptr->riding)
	{
		f4 &= ~(RF4_RIDING_MASK);
		f5 &= ~(RF5_RIDING_MASK);
		f6 &= ~(RF6_RIDING_MASK);
	}

	if (pet)
	{
		f4 &= ~(RF4_SHRIEK);
		f6 &= ~(RF6_DARKNESS | RF6_TRAPS);

		if (!(target_ptr->pet_extra_flags & PF_TELEPORT))
		{
			f6 &= ~(RF6_BLINK | RF6_TPORT | RF6_TELE_TO | RF6_TELE_AWAY | RF6_TELE_LEVEL);
		}

		if (!(target_ptr->pet_extra_flags & PF_ATTACK_SPELL))
		{
			f4 &= ~(RF4_ATTACK_MASK);
			f5 &= ~(RF5_ATTACK_MASK);
			f6 &= ~(RF6_ATTACK_MASK);
		}

		if (!(target_ptr->pet_extra_flags & PF_SUMMON_SPELL))
		{
			f4 &= ~(RF4_SUMMON_MASK);
			f5 &= ~(RF5_SUMMON_MASK);
			f6 &= ~(RF6_SUMMON_MASK);
		}

		/* Prevent collateral damage */
		if (!(target_ptr->pet_extra_flags & PF_BALL_SPELL) && (m_idx != target_ptr->riding))
		{
			if ((f4 & (RF4_BALL_MASK & ~(RF4_ROCKET))) ||
			    (f5 & RF5_BALL_MASK) ||
			    (f6 & RF6_BALL_MASK))
			{
				POSITION real_y = y;
				POSITION real_x = x;

				get_project_point(floor_ptr, m_ptr->fy, m_ptr->fx, &real_y, &real_x, 0L);

				if (projectable(floor_ptr, real_y, real_x, target_ptr->y, target_ptr->x))
				{
					int dist = distance(real_y, real_x, target_ptr->y, target_ptr->x);

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
					if ((distance(real_y, real_x, target_ptr->y, target_ptr->x) <= 4) && los(floor_ptr, real_y, real_x, target_ptr->y, target_ptr->x))
						f5 &= ~(RF5_BA_LITE);
				}
			}

			if (f4 & RF4_ROCKET)
			{
				POSITION real_y = y;
				POSITION real_x = x;

				get_project_point(floor_ptr, m_ptr->fy, m_ptr->fx, &real_y, &real_x, PROJECT_STOP);
				if (projectable(floor_ptr, real_y, real_x, target_ptr->y, target_ptr->x) && (distance(real_y, real_x, target_ptr->y, target_ptr->x) <= 2))
					f4 &= ~(RF4_ROCKET);
			}

			if (((f4 & RF4_BEAM_MASK) || (f5 & RF5_BEAM_MASK) || (f6 & RF6_BEAM_MASK)) &&
			    !direct_beam(m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx, m_ptr))
			{
				f4 &= ~(RF4_BEAM_MASK);
				f5 &= ~(RF5_BEAM_MASK);
				f6 &= ~(RF6_BEAM_MASK);
			}

			if ((f4 & RF4_BREATH_MASK) || (f5 & RF5_BREATH_MASK) || (f6 & RF6_BREATH_MASK))
			{
				/* Expected breath radius */
				POSITION rad = (r_ptr->flags2 & RF2_POWERFUL) ? 3 : 2;

				if (!breath_direct(target_ptr, m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx, rad, 0, TRUE))
				{
					f4 &= ~(RF4_BREATH_MASK);
					f5 &= ~(RF5_BREATH_MASK);
					f6 &= ~(RF6_BREATH_MASK);
				}
				else if ((f4 & RF4_BR_LITE) &&
					 !breath_direct(target_ptr, m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx, rad, GF_LITE, TRUE))
				{
					f4 &= ~(RF4_BR_LITE);
				}
				else if ((f4 & RF4_BR_DISI) &&
					 !breath_direct(target_ptr, m_ptr->fy, m_ptr->fx, t_ptr->fy, t_ptr->fx, rad, GF_DISINTEGRATE, TRUE))
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
				if ((target_ptr->pet_extra_flags & (PF_ATTACK_SPELL | PF_SUMMON_SPELL)) != (PF_ATTACK_SPELL | PF_SUMMON_SPELL))
					f6 &= ~(RF6_SPECIAL);
			}
			else if (r_ptr->d_char == 'B')
			{
				if ((target_ptr->pet_extra_flags & (PF_ATTACK_SPELL | PF_TELEPORT)) != (PF_ATTACK_SPELL | PF_TELEPORT))
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
		if ((f4 & RF4_DISPEL) && !dispel_check_monster(m_idx, target_idx))
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
		if ((f6 & RF6_TELE_LEVEL) && is_teleport_level_ineffective(target_ptr, (target_idx == target_ptr->riding) ? 0 : target_idx))
		{
			f6 &= ~(RF6_TELE_LEVEL);
		}
	}

	/* No spells left */
	if (!f4 && !f5 && !f6) return FALSE;

	/* Extract the "inate" spells */
	for (k = 0; k < 32; k++)
	{
		if (f4 & (1L << k)) spell[num++] = k + RF4_SPELL_START;
	}

	/* Extract the "normal" spells */
	for (k = 0; k < 32; k++)
	{
        if (f5 & (1L << k)) spell[num++] = k + RF5_SPELL_START;
	}

	/* Extract the "bizarre" spells */
	for (k = 0; k < 32; k++)
	{
        if (f6 & (1L << k)) spell[num++] = k + RF6_SPELL_START;
	}

	/* No spells left */
	if (!num) return FALSE;

	/* Stop if player is dead or gone */
	if (!target_ptr->playing || target_ptr->is_dead) return FALSE;

	/* Handle "leaving" */
	if (target_ptr->leaving) return FALSE;

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

	if (target_ptr->riding && (m_idx == target_ptr->riding)) disturb(target_ptr, TRUE, TRUE);

	/* Check for spell failure (inate attacks never fail) */
	if (!spell_is_inate(thrown_spell) && (in_no_magic_dungeon || (MON_STUNNED(m_ptr) && one_in_(2))))
	{
		disturb(target_ptr, TRUE, TRUE);
		if (see_m) msg_format(_("%^sは呪文を唱えようとしたが失敗した。", 
			                    "%^s tries to cast a spell, but fails."), m_name);

		return TRUE;
	}

	/* Hex: Anti Magic Barrier */
	if (!spell_is_inate(thrown_spell) && magic_barrier(target_ptr, m_idx))
	{
		if (see_m) msg_format(_("反魔法バリアが%^sの呪文をかき消した。", 
			                    "Anti magic barrier cancels the spell which %^s casts."), m_name);
		return TRUE;
	}

	can_remember = is_original_ap_and_seen(m_ptr);

	dam = monspell_to_monster(target_ptr, thrown_spell, y, x, m_idx, target_idx);
	if (dam < 0) return FALSE;

	if (m_ptr->ml && maneable && !current_world_ptr->timewalk_m_idx && !target_ptr->blind && (target_ptr->pclass == CLASS_IMITATOR))
	{
		if (thrown_spell != 167) /* Not RF6_SPECIAL */
		{
			if (target_ptr->mane_num == MAX_MANE)
			{
				target_ptr->mane_num--;
				for (i = 0; i < target_ptr->mane_num - 1; i++)
				{
					target_ptr->mane_spell[i] = target_ptr->mane_spell[i+1];
					target_ptr->mane_dam[i] = target_ptr->mane_dam[i+1];
				}
			}

			target_ptr->mane_spell[target_ptr->mane_num] = thrown_spell - RF4_SPELL_START;
			target_ptr->mane_dam[target_ptr->mane_num] = dam;
			target_ptr->mane_num++;
			target_ptr->new_mane = TRUE;

			target_ptr->redraw |= (PR_IMITATION);
		}
	}

	/* Remember what the monster did, if we saw it */
	if (can_remember)
	{
		/* Inate spell */
        if (thrown_spell < RF4_SPELL_START + RF4_SPELL_SIZE)
		{
            r_ptr->r_flags4 |= (1L << (thrown_spell - RF4_SPELL_START));
			if (r_ptr->r_cast_spell < MAX_UCHAR) r_ptr->r_cast_spell++;
		}

		/* Bolt or Ball */
        else if (thrown_spell < RF5_SPELL_START + RF5_SPELL_SIZE)
		{
            r_ptr->r_flags5 |= (1L << (thrown_spell - RF5_SPELL_START));
			if (r_ptr->r_cast_spell < MAX_UCHAR) r_ptr->r_cast_spell++;
		}

		/* Special spell */
        else if (thrown_spell < RF6_SPELL_START + RF6_SPELL_SIZE)
		{
            r_ptr->r_flags6 |= (1L << (thrown_spell - RF6_SPELL_START));
			if (r_ptr->r_cast_spell < MAX_UCHAR) r_ptr->r_cast_spell++;
		}
	}

	/* Always take note of monsters that kill you */
	if (target_ptr->is_dead && (r_ptr->r_deaths < MAX_SHORT) && !floor_ptr->inside_arena)
	{
		r_ptr->r_deaths++; /* Ignore appearance difference */
	}

	/* A spell was cast */
	return TRUE;
}
