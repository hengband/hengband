/*!
 * @file spells1.c
 * @brief 魔法による遠隔処理の実装 / Spell projection
 * @date 2014/07/10
 * @author
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * </pre>
 */

#include "angband.h"
#include "core.h"
#include "util.h"

#include "cmd-pet.h"
#include "cmd-dump.h"
#include "floor.h"
#include "trap.h"
#include "autopick.h"
#include "object-curse.h"
#include "player-damage.h"
#include "player-effects.h"
#include "player-race.h"
#include "player-class.h"

#include "monster.h"
#include "monster-status.h"
#include "monster-spell.h"
#include "spells.h"
#include "spells-status.h"
#include "spells-diceroll.h"
#include "spells-summon.h"
#include "monsterrace-hook.h"

#include "melee.h"
#include "world.h"
#include "mutation.h"
#include "rooms.h"
#include "artifact.h"
#include "avatar.h"
#include "player-status.h"
#include "player-move.h"
#include "realm-hex.h"
#include "realm-song.h"
#include "object-hook.h"
#include "object-broken.h"
#include "object-flavor.h"
#include "quest.h"
#include "term.h"
#include "grid.h"
#include "feature.h"
#include "view-mainwindow.h"
#include "dungeon.h"


static int rakubadam_m; /*!< 振り落とされた際のダメージ量 */
static int rakubadam_p; /*!< 落馬した際のダメージ量 */
bool sukekaku;

int project_length = 0; /*!< 投射の射程距離 */

int cap_mon;
int cap_mspeed;
HIT_POINT cap_hp;
HIT_POINT cap_maxhp;
STR_OFFSET cap_nickname;

/*!
 * @brief 歌、剣術、呪術領域情報テーブル
 */
const magic_type technic_info[NUM_TECHNIC][32] =
{
	{
		/* Music */
		{ 1,  1,  10,   2},
		{ 2,  1,  10,   2},
		{ 3,  2,  20,   3},
		{ 4,  2,  20,   4},
		{ 5,  2,  20,   6},
		{ 7,  4,  30,   8},
		{ 9,  3,  30,   10},
		{ 10, 2,  30,   12},

		{ 12,  3,   40,   20},
		{ 15, 16,  42,   35},
		{ 17, 18,  40,   25},
		{ 18,  2,  45,   30},
		{ 23,  8,  50,   38},
		{ 28, 30,  50,   41},
		{ 33, 35,  60,   42},
		{ 38, 35,  70,   46},

		{ 10,  4,  20,   13},
		{ 22,  5,  30,   26},
		{ 23,  3,  35,   27},
		{ 26,  28,  37,   29},
		{ 32,  37,  41,   36},
		{ 33,  22,  43,   40},
		{ 37,  35,  46,   42},
		{ 45,  60,  50,   56},

		{ 23,  18,  20,   23},
		{ 30,  30,  30,   26},
		{ 33,  65,  41,   30},
		{ 37,  35,  43,   35},
		{ 40,  30,  46,   50},
		{ 42,  75,  50,   68},
		{ 45,  58,  62,   73},
		{ 49,  48,  70,  200}
	},

	{
		/* Hissatsu */
		{ 1,   15,   0,   0},
		{ 3,   10,   0,   0},
		{ 6,   15,   0,   0},
		{ 9,    8,   0,   0},
		{ 10,  12,   0,   0},
		{ 12,  25,   0,   0},
		{ 14,   7,   0,   0},
		{ 17,  20,   0,   0},

		{ 19,  10,   0,   0},
		{ 22,  20,   0,   0},
		{ 24,  30,   0,   0},
		{ 25,  10,   0,   0},
		{ 27,  15,   0,   0},
		{ 29,  45,   0,   0},
		{ 32,  70,   0,   0},
		{ 35,  50,   0,   0},

		{ 18,  40,   0,   0},
		{ 22,  22,   0,   0},
		{ 24,  30,   0,   0},
		{ 26,  35,   0,   0},
		{ 30,  30,   0,   0},
		{ 32,  60,   0,   0},
		{ 36,  40,   0,   0},
		{ 39,  80,   0,   0},

		{ 26,  20,   0,   0},
		{ 29,  40,   0,   0},
		{ 31,  35,   0,   0},
		{ 36,  80,   0,   0},
		{ 39, 100,   0,   0},
		{ 42, 110,   0,   0},
		{ 45, 130,   0,   0},
		{ 50, 255,   0,   0}
	},

	{
		/* Hex */
		{  1,  2, 20,   2},
		{  1,  2, 20,   2},
		{  3,  2, 30,   3},
		{  5,  3, 30,   4},
		{  7,  3, 40,   6},
		{  8, 10, 60,   8},
		{  9,  3, 30,  10},
		{ 10,  5, 40,  12},

		{ 12,  8, 40,  15},
		{ 12,  9, 35,  15},
		{ 15, 10, 50,  20},
		{ 20, 12, 45,  35},
		{ 25, 15, 50,  50},
		{ 30, 12, 60,  70},
		{ 35, 10, 60,  80},
		{ 40, 16, 70, 100},

		{ 15,  8, 20,  20},
		{ 18, 15, 50,  20},
		{ 22, 10, 65,  35},
		{ 25, 28, 70,  50},
		{ 28, 10, 70,  60},
		{ 30, 20, 60,  60},
		{ 36, 22, 70,  80},
		{ 40, 28, 70, 100},

		{  5,  6, 35,   5},
		{ 22, 24, 70,  40},
		{ 25,  2, 65,  50},
		{ 32, 20, 50,  70},
		{ 35, 35, 70,  80},
		{ 38, 32, 70,  90},
		{ 42, 24, 70, 120},
		{ 46, 45, 80, 200}
	},
};



/*!
 * @brief 配置した鏡リストの次を取得する /
 * Get another mirror. for SEEKER 
 * @param next_y 次の鏡のy座標を返す参照ポインタ
 * @param next_x 次の鏡のx座標を返す参照ポインタ
 * @param cury 現在の鏡のy座標
 * @param curx 現在の鏡のx座標
 */
static void next_mirror(player_type *creature_ptr, POSITION* next_y, POSITION* next_x, POSITION cury, POSITION curx)
{
	POSITION mirror_x[10], mirror_y[10]; /* 鏡はもっと少ない */
	int mirror_num = 0;			  /* 鏡の数 */
	POSITION x, y;
	int num;

	for (x = 0; x < creature_ptr->current_floor_ptr->width; x++)
	{
		for (y = 0; y < creature_ptr->current_floor_ptr->height; y++)
		{
			if (is_mirror_grid(&creature_ptr->current_floor_ptr->grid_array[y][x])) {
				mirror_y[mirror_num] = y;
				mirror_x[mirror_num] = x;
				mirror_num++;
			}
		}
	}
	if (mirror_num)
	{
		num = randint0(mirror_num);
		*next_y = mirror_y[num];
		*next_x = mirror_x[num];
		return;
	}
	*next_y = cury + randint0(5) - 2;
	*next_x = curx + randint0(5) - 2;
	return;
}


/*
 * Mega-Hack -- track "affected" monsters (see "project()" comments)
 */
static int project_m_n; /*!< 魔法効果範囲内にいるモンスターの数 */
static POSITION project_m_x; /*!< 処理中のモンスターX座標 */
static POSITION project_m_y; /*!< 処理中のモンスターY座標 */
/* Mega-Hack -- monsters target */
static POSITION monster_target_x; /*!< モンスターの攻撃目標X座標 */
static POSITION monster_target_y; /*!< モンスターの攻撃目標Y座標 */


/*!
 * @brief 汎用的なビーム/ボルト/ボール系による地形効果処理 / We are called from "project()" to "damage" terrain features
 * @param who 魔法を発動したモンスター(0ならばプレイヤー) / Index of "source" monster (zero for "player")
 * @param r 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標Y座標 / Target y location (or location to travel "towards")
 * @param x 目標X座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param typ 効果属性 / Type of damage to apply to monsters (and objects)
 * @return 何か一つでも効力があればTRUEを返す / TRUE if any "effects" of the projection were observed, else FALSE
 * @details
 * <pre>
 * We are called both for "beam" effects and "ball" effects.
 *
 * The "r" parameter is the "distance from ground zero".
 *
 * Note that we determine if the player can "see" anything that happens
 * by taking into account: blindness, line-of-sight, and illumination.
 *
 * We return "TRUE" if the effect of the projection is "obvious".
 *
 * We also "see" grids which are "memorized", probably a hack
 *
 * Perhaps we should affect doors?
 * </pre>
 */
static bool project_f(floor_type *floor_ptr, MONSTER_IDX who, POSITION r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID typ)
{
	grid_type *g_ptr = &floor_ptr->grid_array[y][x];
	feature_type *f_ptr = &f_info[g_ptr->feat];

	bool obvious = FALSE;
	bool known = player_has_los_bold(p_ptr, y, x);

	who = who ? who : 0;

	/* Reduce damage by distance */
	dam = (dam + r) / (r + 1);


	if (have_flag(f_ptr->flags, FF_TREE))
	{
		concptr message;
		switch (typ)
		{
		case GF_POIS:
		case GF_NUKE:
		case GF_DEATH_RAY:
			message = _("枯れた", "was blasted."); break;
		case GF_TIME:
			message = _("縮んだ", "shrank."); break;
		case GF_ACID:
			message = _("溶けた", "melted."); break;
		case GF_COLD:
		case GF_ICE:
			message = _("凍り、砕け散った", "was frozen and smashed."); break;
		case GF_FIRE:
		case GF_ELEC:
		case GF_PLASMA:
			message = _("燃えた", "burns up!"); break;
		case GF_METEOR:
		case GF_CHAOS:
		case GF_MANA:
		case GF_SEEKER:
		case GF_SUPER_RAY:
		case GF_SHARDS:
		case GF_ROCKET:
		case GF_SOUND:
		case GF_DISENCHANT:
		case GF_FORCE:
		case GF_GRAVITY:
			message = _("粉砕された", "was crushed."); break;
		default:
			message = NULL; break;
		}
		if (message)
		{
			msg_format(_("木は%s。", "A tree %s"), message);
			cave_set_feat(y, x, one_in_(3) ? feat_brake : feat_grass);

			/* Observe */
			if (g_ptr->info & (CAVE_MARK)) obvious = TRUE;
		}
	}

	/* Analyze the type */
	switch (typ)
	{
		/* Ignore most effects */
		case GF_CAPTURE:
		case GF_HAND_DOOM:
		case GF_CAUSE_1:
		case GF_CAUSE_2:
		case GF_CAUSE_3:
		case GF_CAUSE_4:
		case GF_MIND_BLAST:
		case GF_BRAIN_SMASH:
		case GF_DRAIN_MANA:
		case GF_PSY_SPEAR:
		case GF_FORCE:
		case GF_HOLY_FIRE:
		case GF_HELL_FIRE:
		case GF_PSI:
		case GF_PSI_DRAIN:
		case GF_TELEKINESIS:
		case GF_DOMINATION:
		case GF_IDENTIFY:
		case GF_ATTACK:
		case GF_ACID:
		case GF_ELEC:
		case GF_COLD:
		case GF_ICE:
		case GF_FIRE:
		case GF_PLASMA:
		case GF_METEOR:
		case GF_CHAOS:
		case GF_MANA:
		case GF_SEEKER:
		case GF_SUPER_RAY:
		{
			break;
		}

		/* Destroy Traps (and Locks) */
		case GF_KILL_TRAP:
		{
			/* Reveal secret doors */
			if (is_hidden_door(g_ptr))
			{
				/* Pick a door */
				disclose_grid(y, x);

				/* Check line of sight */
				if (known)
				{
					obvious = TRUE;
				}
			}

			/* Destroy traps */
			if (is_trap(g_ptr->feat))
			{
				/* Check line of sight */
				if (known)
				{
					msg_print(_("まばゆい閃光が走った！", "There is a bright flash of light!"));
					obvious = TRUE;
				}

				/* Destroy the trap */
				cave_alter_feat(y, x, FF_DISARM);
			}

			/* Locked doors are unlocked */
			if (is_closed_door(g_ptr->feat) && f_ptr->power && have_flag(f_ptr->flags, FF_OPEN))
			{
				FEAT_IDX old_feat = g_ptr->feat;

				/* Unlock the door */
				cave_alter_feat(y, x, FF_DISARM);

				/* Check line of sound */
				if (known && (old_feat != g_ptr->feat))
				{
					msg_print(_("カチッと音がした！", "Click!"));
					obvious = TRUE;
				}
			}

			/* Remove "unsafe" flag if player is not blind */
			if (!p_ptr->blind && player_has_los_bold(p_ptr, y, x))
			{
				g_ptr->info &= ~(CAVE_UNSAFE);
				lite_spot(y, x);
				obvious = TRUE;
			}

			break;
		}

		/* Destroy Doors (and traps) */
		case GF_KILL_DOOR:
		{
			/* Destroy all doors and traps */
			if (is_trap(g_ptr->feat) || have_flag(f_ptr->flags, FF_DOOR))
			{
				/* Check line of sight */
				if (known)
				{
					msg_print(_("まばゆい閃光が走った！", "There is a bright flash of light!"));
					obvious = TRUE;
				}

				/* Destroy the feature */
				cave_alter_feat(y, x, FF_TUNNEL);
			}

			/* Remove "unsafe" flag if player is not blind */
			if (!p_ptr->blind && player_has_los_bold(p_ptr, y, x))
			{
				g_ptr->info &= ~(CAVE_UNSAFE);
				lite_spot(y, x);
				obvious = TRUE;
			}

			break;
		}

		case GF_JAM_DOOR: /* Jams a door (as if with a spike) */
		{
			if (have_flag(f_ptr->flags, FF_SPIKE))
			{
				s16b old_mimic = g_ptr->mimic;
				feature_type *mimic_f_ptr = &f_info[get_feat_mimic(g_ptr)];

				cave_alter_feat(y, x, FF_SPIKE);
				g_ptr->mimic = old_mimic;

				note_spot(y, x);
				lite_spot(y, x);

				/* Check line of sight */
				if (known && have_flag(mimic_f_ptr->flags, FF_OPEN))
				{
					msg_format(_("%sに何かがつっかえて開かなくなった。", "The %s seems stuck."), f_name + mimic_f_ptr->name);
					obvious = TRUE;
				}
			}
			break;
		}

		/* Destroy walls (and doors) */
		case GF_KILL_WALL:
		{
			if (have_flag(f_ptr->flags, FF_HURT_ROCK))
			{
				if (known && (g_ptr->info & (CAVE_MARK)))
				{
					msg_format(_("%sが溶けて泥になった！", "The %s turns into mud!"), f_name + f_info[get_feat_mimic(g_ptr)].name);
					obvious = TRUE;
				}

				/* Destroy the wall */
				cave_alter_feat(y, x, FF_HURT_ROCK);
				p_ptr->update |= (PU_FLOW);
			}

			break;
		}

		case GF_MAKE_DOOR:
		{
			if (!cave_naked_bold(p_ptr, p_ptr->current_floor_ptr, y, x)) break;
			if (player_bold(p_ptr, y, x)) break;
			cave_set_feat(y, x, feat_door[DOOR_DOOR].closed);
			if (g_ptr->info & (CAVE_MARK)) obvious = TRUE;
			break;
		}

		case GF_MAKE_TRAP:
		{
			place_trap(y, x);
			break;
		}

		case GF_MAKE_TREE:
		{
			if (!cave_naked_bold(p_ptr, p_ptr->current_floor_ptr, y, x)) break;
			if (player_bold(p_ptr, y, x)) break;
			cave_set_feat(y, x, feat_tree);
			if (g_ptr->info & (CAVE_MARK)) obvious = TRUE;
			break;
		}

		case GF_MAKE_GLYPH:
		{
			if (!cave_naked_bold(p_ptr, p_ptr->current_floor_ptr, y, x)) break;
			g_ptr->info |= CAVE_OBJECT;
			g_ptr->mimic = feat_glyph;
			note_spot(y, x);
			lite_spot(y, x);
			break;
		}

		case GF_STONE_WALL:
		{
			if (!cave_naked_bold(p_ptr, p_ptr->current_floor_ptr, y, x)) break;
			if (player_bold(p_ptr, y, x)) break;
			cave_set_feat(y, x, feat_granite);
			break;
		}

		case GF_LAVA_FLOW:
		{
			if (have_flag(f_ptr->flags, FF_PERMANENT)) break;
			if (dam == 1)
			{
				if (!have_flag(f_ptr->flags, FF_FLOOR)) break;
				cave_set_feat(y, x, feat_shallow_lava);
			}
			else if (dam)
			{
				cave_set_feat(y, x, feat_deep_lava);
			}
			break;
		}

		case GF_WATER_FLOW:
		{
			if (have_flag(f_ptr->flags, FF_PERMANENT)) break;
			if (dam == 1)
			{
				if (!have_flag(f_ptr->flags, FF_FLOOR)) break;
				cave_set_feat(y, x, feat_shallow_water);
			}
			else if (dam)
			{
				cave_set_feat(y, x, feat_deep_water);
			}
			break;
		}

		/* Lite up the grid */
		case GF_LITE_WEAK:
		case GF_LITE:
		{
			/* Turn on the light */
			if (!(d_info[p_ptr->dungeon_idx].flags1 & DF1_DARKNESS))
			{
				g_ptr->info |= (CAVE_GLOW);
				note_spot(y, x);
				lite_spot(y, x);
				update_local_illumination(p_ptr, y, x);

				/* Observe */
				if (player_can_see_bold(y, x)) obvious = TRUE;

				/* Mega-Hack -- Update the monster in the affected grid */
				/* This allows "spear of light" (etc) to work "correctly" */
				if (g_ptr->m_idx) update_monster(g_ptr->m_idx, FALSE);

				if (p_ptr->special_defense & NINJA_S_STEALTH)
				{
					if (player_bold(p_ptr, y, x)) set_superstealth(p_ptr, FALSE);
				}
			}

			break;
		}

		/* Darken the grid */
		case GF_DARK_WEAK:
		case GF_DARK:
		{
			bool do_dark = !p_ptr->phase_out && !is_mirror_grid(g_ptr);
			int j;

			/* Turn off the light. */
			if (do_dark)
			{
				if (floor_ptr->dun_level || !is_daytime())
				{
					for (j = 0; j < 9; j++)
					{
						int by = y + ddy_ddd[j];
						int bx = x + ddx_ddd[j];

						if (in_bounds2(floor_ptr, by, bx))
						{
							grid_type *cc_ptr = &floor_ptr->grid_array[by][bx];

							if (have_flag(f_info[get_feat_mimic(cc_ptr)].flags, FF_GLOW))
							{
								do_dark = FALSE;
								break;
							}
						}
					}

					if (!do_dark) break;
				}

				g_ptr->info &= ~(CAVE_GLOW);

				/* Hack -- Forget "boring" grids */
				if (!have_flag(f_ptr->flags, FF_REMEMBER))
				{
					/* Forget */
					g_ptr->info &= ~(CAVE_MARK);

					note_spot(y, x);
				}

				lite_spot(y, x);

				update_local_illumination(p_ptr, y, x);

				if (player_can_see_bold(y, x)) obvious = TRUE;

				/* Mega-Hack -- Update the monster in the affected grid */
				/* This allows "spear of light" (etc) to work "correctly" */
				if (g_ptr->m_idx) update_monster(g_ptr->m_idx, FALSE);
			}

			/* All done */
			break;
		}

		case GF_SHARDS:
		case GF_ROCKET:
		{
			if (is_mirror_grid(g_ptr))
			{
				msg_print(_("鏡が割れた！", "The mirror was crashed!"));
				sound(SOUND_GLASS);
				remove_mirror(y, x);
				project(0, 2, y, x, p_ptr->lev / 2 + 5, GF_SHARDS, (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI), -1);
			}

			if (have_flag(f_ptr->flags, FF_GLASS) && !have_flag(f_ptr->flags, FF_PERMANENT) && (dam >= 50))
			{
				if (known && (g_ptr->info & CAVE_MARK))
				{
					msg_format(_("%sが割れた！", "The %s was crashed!"), f_name + f_info[get_feat_mimic(g_ptr)].name);
					sound(SOUND_GLASS);
				}

				/* Destroy the wall */
				cave_alter_feat(y, x, FF_HURT_ROCK);
				p_ptr->update |= (PU_FLOW);
			}
			break;
		}

		case GF_SOUND:
		{
			if (is_mirror_grid(g_ptr) && p_ptr->lev < 40)
			{
				msg_print(_("鏡が割れた！", "The mirror was crashed!"));
				sound(SOUND_GLASS);
				remove_mirror(y, x);
				project(0, 2, y, x, p_ptr->lev / 2 + 5, GF_SHARDS, (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI), -1);
			}

			if (have_flag(f_ptr->flags, FF_GLASS) && !have_flag(f_ptr->flags, FF_PERMANENT) && (dam >= 200))
			{
				if (known && (g_ptr->info & CAVE_MARK))
				{
					msg_format(_("%sが割れた！", "The %s was crashed!"), f_name + f_info[get_feat_mimic(g_ptr)].name);
					sound(SOUND_GLASS);
				}

				/* Destroy the wall */
				cave_alter_feat(y, x, FF_HURT_ROCK);
				p_ptr->update |= (PU_FLOW);
			}
			break;
		}

		case GF_DISINTEGRATE:
		{
			/* Destroy mirror/glyph */
			if (is_mirror_grid(g_ptr) || is_glyph_grid(g_ptr) || is_explosive_rune_grid(g_ptr))
				remove_mirror(y, x);

			/* Permanent features don't get effect */
			/* But not protect monsters and other objects */
			if (have_flag(f_ptr->flags, FF_HURT_DISI) && !have_flag(f_ptr->flags, FF_PERMANENT))
			{
				cave_alter_feat(y, x, FF_HURT_DISI);

				/* Update some things -- similar to GF_KILL_WALL */
				p_ptr->update |= (PU_FLOW);
			}
			break;
		}
	}

	lite_spot(y, x);
	/* Return "Anything seen?" */
	return (obvious);
}



/*!
 * @brief 汎用的なビーム/ボルト/ボール系によるアイテムオブジェクトへの効果処理 / Handle a beam/bolt/ball causing damage to a monster.
 * @param who 魔法を発動したモンスター(0ならばプレイヤー) / Index of "source" monster (zero for "player")
 * @param r 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標Y座標 / Target y location (or location to travel "towards")
 * @param x 目標X座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param typ 効果属性 / Type of damage to apply to monsters (and objects)
 * @return 何か一つでも効力があればTRUEを返す / TRUE if any "effects" of the projection were observed, else FALSE
 * @details
 * <pre>
 * We are called from "project()" to "damage" objects
 *
 * We are called both for "beam" effects and "ball" effects.
 *
 * Perhaps we should only SOMETIMES damage things on the ground.
 *
 * The "r" parameter is the "distance from ground zero".
 *
 * Note that we determine if the player can "see" anything that happens
 * by taking into account: blindness, line-of-sight, and illumination.
 *
 * We also "see" grids which are "memorized", probably a hack
 *
 * We return "TRUE" if the effect of the projection is "obvious".
 * </pre>
 */
static bool project_o(MONSTER_IDX who, POSITION r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID typ)
{
	grid_type *g_ptr = &p_ptr->current_floor_ptr->grid_array[y][x];

	OBJECT_IDX this_o_idx, next_o_idx = 0;

	bool obvious = FALSE;
	bool known = player_has_los_bold(p_ptr, y, x);

	BIT_FLAGS flgs[TR_FLAG_SIZE];

	GAME_TEXT o_name[MAX_NLEN];

	KIND_OBJECT_IDX k_idx = 0;
	bool is_potion = FALSE;


	who = who ? who : 0;

	/* Reduce damage by distance */
	dam = (dam + r) / (r + 1);


	/* Scan all objects in the grid */
	for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr = &p_ptr->current_floor_ptr->o_list[this_o_idx];

		bool is_art = FALSE;
		bool ignore = FALSE;
		bool do_kill = FALSE;

		concptr note_kill = NULL;

#ifndef JP
		/* Get the "plural"-ness */
		bool plural = (o_ptr->number > 1);
#endif
		next_o_idx = o_ptr->next_o_idx;
		object_flags(o_ptr, flgs);

		/* Check for artifact */
		if (object_is_artifact(o_ptr)) is_art = TRUE;

		/* Analyze the type */
		switch (typ)
		{
			/* Acid -- Lots of things */
			case GF_ACID:
			{
				if (hates_acid(o_ptr))
				{
					do_kill = TRUE;
					note_kill = _("融けてしまった！", (plural ? " melt!" : " melts!"));
					if (have_flag(flgs, TR_IGNORE_ACID)) ignore = TRUE;
				}
				break;
			}

			/* Elec -- Rings and Wands */
			case GF_ELEC:
			{
				if (hates_elec(o_ptr))
				{
					do_kill = TRUE;
					note_kill = _("壊れてしまった！", (plural ? " are destroyed!" : " is destroyed!"));
					if (have_flag(flgs, TR_IGNORE_ELEC)) ignore = TRUE;
				}
				break;
			}

			/* Fire -- Flammable objects */
			case GF_FIRE:
			{
				if (hates_fire(o_ptr))
				{
					do_kill = TRUE;
					note_kill = _("燃えてしまった！", (plural ? " burn up!" : " burns up!"));
					if (have_flag(flgs, TR_IGNORE_FIRE)) ignore = TRUE;
				}
				break;
			}

			/* Cold -- potions and flasks */
			case GF_COLD:
			{
				if (hates_cold(o_ptr))
				{
					note_kill = _("砕け散ってしまった！", (plural ? " shatter!" : " shatters!"));
					do_kill = TRUE;
					if (have_flag(flgs, TR_IGNORE_COLD)) ignore = TRUE;
				}
				break;
			}

			/* Fire + Elec */
			case GF_PLASMA:
			{
				if (hates_fire(o_ptr))
				{
					do_kill = TRUE;
					note_kill = _("燃えてしまった！", (plural ? " burn up!" : " burns up!"));
					if (have_flag(flgs, TR_IGNORE_FIRE)) ignore = TRUE;
				}
				if (hates_elec(o_ptr))
				{
					ignore = FALSE;
					do_kill = TRUE;
					note_kill = _("壊れてしまった！", (plural ? " are destroyed!" : " is destroyed!"));
					if (have_flag(flgs, TR_IGNORE_ELEC)) ignore = TRUE;
				}
				break;
			}

			/* Fire + Cold */
			case GF_METEOR:
			{
				if (hates_fire(o_ptr))
				{
					do_kill = TRUE;
					note_kill = _("燃えてしまった！", (plural ? " burn up!" : " burns up!"));
					if (have_flag(flgs, TR_IGNORE_FIRE)) ignore = TRUE;
				}
				if (hates_cold(o_ptr))
				{
					ignore = FALSE;
					do_kill = TRUE;
					note_kill = _("砕け散ってしまった！", (plural ? " shatter!" : " shatters!"));
					if (have_flag(flgs, TR_IGNORE_COLD)) ignore = TRUE;
				}
				break;
			}

			/* Hack -- break potions and such */
			case GF_ICE:
			case GF_SHARDS:
			case GF_FORCE:
			case GF_SOUND:
			{
				if (hates_cold(o_ptr))
				{
					note_kill = _("砕け散ってしまった！", (plural ? " shatter!" : " shatters!"));
					do_kill = TRUE;
				}
				break;
			}

			/* Mana and Chaos -- destroy everything */
			case GF_MANA:
			case GF_SEEKER:
			case GF_SUPER_RAY:
			{
				do_kill = TRUE;
				note_kill = _("壊れてしまった！", (plural ? " are destroyed!" : " is destroyed!"));
				break;
			}

			case GF_DISINTEGRATE:
			{
				do_kill = TRUE;
				note_kill = _("蒸発してしまった！", (plural ? " evaporate!" : " evaporates!"));
				break;
			}

			case GF_CHAOS:
			{
				do_kill = TRUE;
				note_kill = _("壊れてしまった！", (plural ? " are destroyed!" : " is destroyed!"));
				if (have_flag(flgs, TR_RES_CHAOS)) ignore = TRUE;
				else if ((o_ptr->tval == TV_SCROLL) && (o_ptr->sval == SV_SCROLL_CHAOS)) ignore = TRUE;
				break;
			}

			/* Holy Fire and Hell Fire -- destroys cursed non-artifacts */
			case GF_HOLY_FIRE:
			case GF_HELL_FIRE:
			{
				if (object_is_cursed(o_ptr))
				{
					do_kill = TRUE;
					note_kill = _("壊れてしまった！", (plural ? " are destroyed!" : " is destroyed!"));
				}
				break;
			}

			case GF_IDENTIFY:
			{
				identify_item(o_ptr);

				/* Auto-inscription */
				autopick_alter_item((-this_o_idx), FALSE);
				break;
			}

			/* Unlock chests */
			case GF_KILL_TRAP:
			case GF_KILL_DOOR:
			{
				/* Chests are noticed only if trapped or locked */
				if (o_ptr->tval == TV_CHEST)
				{
					/* Disarm/Unlock traps */
					if (o_ptr->pval > 0)
					{
						/* Disarm or Unlock */
						o_ptr->pval = (0 - o_ptr->pval);

						/* Identify */
						object_known(o_ptr);

						if (known && (o_ptr->marked & OM_FOUND))
						{
							msg_print(_("カチッと音がした！", "Click!"));
							obvious = TRUE;
						}
					}
				}

				break;
			}
			case GF_ANIM_DEAD:
			{
				if (o_ptr->tval == TV_CORPSE)
				{
					int i;
					BIT_FLAGS mode = 0L;

					if (!who || is_pet(&p_ptr->current_floor_ptr->m_list[who]))
						mode |= PM_FORCE_PET;

					for (i = 0; i < o_ptr->number ; i++)
					{
						if (((o_ptr->sval == SV_CORPSE) && (randint1(100) > 80)) ||
							((o_ptr->sval == SV_SKELETON) && (randint1(100) > 60)))
						{
							if (!note_kill)
							{
								note_kill = _("灰になった。", (plural ? " become dust." : " becomes dust."));
							}
							continue;
						}
						else if (summon_named_creature(who, y, x, o_ptr->pval, mode))
						{
							note_kill = _("生き返った。", " revived.");
						}
						else if (!note_kill)
						{
							note_kill = _("灰になった。", (plural ? " become dust." : " becomes dust."));
						}
					}
					do_kill = TRUE;
					obvious = TRUE;
				}
				break;
			}
		}


		/* Attempt to destroy the object */
		if (do_kill)
		{
			/* Effect "observed" */
			if (known && (o_ptr->marked & OM_FOUND))
			{
				obvious = TRUE;
				object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
			}

			/* Artifacts, and other objects, get to resist */
			if (is_art || ignore)
			{
				/* Observe the resist */
				if (known && (o_ptr->marked & OM_FOUND))
				{
					msg_format(_("%sは影響を受けない！", 
					   (plural ? "The %s are unaffected!" : "The %s is unaffected!")), o_name);
				}
			}

			/* Kill it */
			else
			{
				/* Describe if needed */
				if (known && (o_ptr->marked & OM_FOUND) && note_kill)
				{
					msg_format(_("%sは%s", "The %s%s"), o_name, note_kill);
				}

				k_idx = o_ptr->k_idx;
				is_potion = object_is_potion(o_ptr);
				delete_object_idx(this_o_idx);

				/* Potions produce effects when 'shattered' */
				if (is_potion)
				{
					(void)potion_smash_effect(who, y, x, k_idx);
				}

				lite_spot(y, x);
			}
		}
	}

	/* Return "Anything seen?" */
	return (obvious);
}


/*!
 * @brief 汎用的なビーム/ボルト/ボール系によるモンスターへの効果処理 / Handle a beam/bolt/ball causing damage to a monster.
 * @param who 魔法を発動したモンスター(0ならばプレイヤー) / Index of "source" monster (zero for "player")
 * @param r 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標Y座標 / Target y location (or location to travel "towards")
 * @param x 目標X座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param typ 効果属性 / Type of damage to apply to monsters (and objects)
 * @param flg 効果フラグ
 * @param see_s_msg TRUEならばメッセージを表示する
 * @return 何か一つでも効力があればTRUEを返す / TRUE if any "effects" of the projection were observed, else FALSE
 * @details
 * <pre>
 * This routine takes a "source monster" (by index) which is mostly used to
 * determine if the player is causing the damage, and a "radius" (see below),
 * which is used to decrease the power of explosions with distance, and a
 * location, via integers which are modified by certain types of attacks
 * (polymorph and teleport being the obvious ones), a default damage, which
 * is modified as needed based on various properties, and finally a "damage
 * type" (see below).
 * </pre>
 * <pre>
 * Note that this routine can handle "no damage" attacks (like teleport) by
 * taking a "zero" damage, and can even take "parameters" to attacks (like
 * confuse) by accepting a "damage", using it to calculate the effect, and
 * then setting the damage to zero.  Note that the "damage" parameter is
 * divided by the radius, so monsters not at the "epicenter" will not take
 * as much damage (or whatever)...
 * </pre>
 * <pre>
 * Note that "polymorph" is dangerous, since a failure in "place_monster()"'
 * may result in a dereference of an invalid pointer.  
 * </pre>
 * <pre>
 * Various messages are produced, and damage is applied.
 * </pre>
 * <pre>
 * Just "casting" a substance (i.e. plasma) does not make you immune, you must
 * actually be "made" of that substance, or "breathe" big balls of it.
 * We assume that "Plasma" monsters, and "Plasma" breathers, are immune
 * to plasma.
 * We assume "Nether" is an evil, necromantic force, so it doesn't hurt undead,
 * and hurts evil less.  If can breath nether, then it resists it as well.
 * </pre>
 * <pre>
 * Damage reductions use the following formulas:
 *   Note that "dam = dam * 6 / (randint1(6) + 6);"
 *	 gives avg damage of .655, ranging from .858 to .500
 *   Note that "dam = dam * 5 / (randint1(6) + 6);"
 *	 gives avg damage of .544, ranging from .714 to .417
 *   Note that "dam = dam * 4 / (randint1(6) + 6);"
 *	 gives avg damage of .444, ranging from .556 to .333
 *   Note that "dam = dam * 3 / (randint1(6) + 6);"
 *	 gives avg damage of .327, ranging from .427 to .250
 *   Note that "dam = dam * 2 / (randint1(6) + 6);"
 *	 gives something simple.
 * </pre>
 * <pre>
 * In this function, "result" messages are postponed until the end, where
 * the "note" string is appended to the monster name, if not NULL.  So,
 * to make a spell have "no effect" just set "note" to NULL.  You should
 * also set "notice" to FALSE, or the player will learn what the spell does.
 * </pre>
 * <pre>
 * We attempt to return "TRUE" if the player saw anything "useful" happen.
 * "flg" was added.
 * </pre>
 */
static bool project_m(MONSTER_IDX who, POSITION r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID typ, BIT_FLAGS flg, bool see_s_msg)
{
	int tmp;

	grid_type *g_ptr = &p_ptr->current_floor_ptr->grid_array[y][x];

	monster_type *m_ptr = &p_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
	monster_type *caster_ptr = (who > 0) ? &p_ptr->current_floor_ptr->m_list[who] : NULL;

	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	char killer[80];

	/* Is the monster "seen"? */
	bool seen = m_ptr->ml;
	bool seen_msg = is_seen(m_ptr);

	bool slept = (bool)MON_CSLEEP(m_ptr);

	/* Were the effects "obvious" (if seen)? */
	bool obvious = FALSE;

	/* Can the player know about this effect? */
	bool known = ((m_ptr->cdis <= MAX_SIGHT) || p_ptr->phase_out);

	/* Were the effects "irrelevant"? */
	bool skipped = FALSE;

	/* Gets the monster angry at the source of the effect? */
	bool get_angry = FALSE;

	/* Polymorph setting (true or false) */
	bool do_poly = FALSE;

	/* Teleport setting (max distance) */
	int do_dist = 0;

	/* Confusion setting (amount to confuse) */
	int do_conf = 0;

	/* Stunning setting (amount to stun) */
	int do_stun = 0;

	/* Sleep amount (amount to sleep) */
	int do_sleep = 0;

	/* Fear amount (amount to fear) */
	int do_fear = 0;

	/* Time amount (amount to time) */
	int do_time = 0;

	bool heal_leper = FALSE;

	/* Hold the monster name */
	GAME_TEXT m_name[MAX_NLEN];
	char m_poss[10];

	PARAMETER_VALUE photo = 0;

	/* Assume no note */
	concptr note = NULL;

	/* Assume a default death */
	concptr note_dies = extract_note_dies(real_r_idx(m_ptr));

	DEPTH caster_lev = (who > 0) ? r_info[caster_ptr->r_idx].level : (p_ptr->lev * 2);

	/* Nobody here */
	if (!g_ptr->m_idx) return (FALSE);

	/* Never affect projector */
	if (who && (g_ptr->m_idx == who)) return (FALSE);
	if ((g_ptr->m_idx == p_ptr->riding) && !who && !(typ == GF_OLD_HEAL) && !(typ == GF_OLD_SPEED) && !(typ == GF_STAR_HEAL)) return (FALSE);
	if (sukekaku && ((m_ptr->r_idx == MON_SUKE) || (m_ptr->r_idx == MON_KAKU))) return FALSE;

	/* Don't affect already death monsters */
	/* Prevents problems with chain reactions of exploding monsters */
	if (m_ptr->hp < 0) return (FALSE);

	/* Reduce damage by distance */
	dam = (dam + r) / (r + 1);


	/* Get the monster name (BEFORE polymorphing) */
	monster_desc(m_name, m_ptr, 0);

	/* Get the monster possessive ("his"/"her"/"its") */
	monster_desc(m_poss, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE);

	if (p_ptr->riding && (g_ptr->m_idx == p_ptr->riding)) disturb(p_ptr, TRUE, TRUE);

	if (r_ptr->flagsr & RFR_RES_ALL &&
		typ != GF_OLD_CLONE && typ != GF_STAR_HEAL && typ != GF_OLD_HEAL
		&& typ != GF_OLD_SPEED && typ != GF_CAPTURE && typ != GF_PHOTO)
	{
		note = _("には完全な耐性がある！", " is immune.");
		dam = 0;
		if(is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (RFR_RES_ALL);
		if(typ == GF_LITE_WEAK || typ == GF_KILL_WALL) skipped = TRUE;
	}
	else
	{
		/* Analyze the damage type */
		switch (typ)
		{
			/* Magic Missile -- pure damage */
			case GF_MISSILE:
			{
				if (seen) obvious = TRUE;
				break;
			}

			/* Acid */
			case GF_ACID:
			{
				if (seen) obvious = TRUE;
				if (r_ptr->flagsr & RFR_IM_ACID)
				{
					note = _("にはかなり耐性がある！", " resists a lot.");
					dam /= 9;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (RFR_IM_ACID);
				}
				break;
			}

			/* Electricity */
			case GF_ELEC:
			{
				if (seen) obvious = TRUE;
				if (r_ptr->flagsr & RFR_IM_ELEC)
				{
					note = _("にはかなり耐性がある！", " resists a lot.");
					dam /= 9;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (RFR_IM_ELEC);
				}
				break;
			}

			/* Fire damage */
			case GF_FIRE:
			{
				if (seen) obvious = TRUE;
				if (r_ptr->flagsr & RFR_IM_FIRE)
				{
					note = _("にはかなり耐性がある！", " resists a lot.");
					dam /= 9;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (RFR_IM_FIRE);
				}
				else if (r_ptr->flags3 & (RF3_HURT_FIRE))
				{
					note = _("はひどい痛手をうけた。", " is hit hard.");
					dam *= 2;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_HURT_FIRE);
				}
				break;
			}

			/* Cold */
			case GF_COLD:
			{
				if (seen) obvious = TRUE;
				if (r_ptr->flagsr & RFR_IM_COLD)
				{
					note = _("にはかなり耐性がある！", " resists a lot.");
					dam /= 9;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (RFR_IM_COLD);
				}
				else if (r_ptr->flags3 & (RF3_HURT_COLD))
				{
					note = _("はひどい痛手をうけた。", " is hit hard.");
					dam *= 2;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_HURT_COLD);
				}
				break;
			}

			/* Poison */
			case GF_POIS:
			{
				if (seen) obvious = TRUE;
				if (r_ptr->flagsr & RFR_IM_POIS)
				{
					note = _("にはかなり耐性がある！", " resists a lot.");
					dam /= 9;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (RFR_IM_POIS);
				}
				break;
			}

			/* Nuclear waste */
			case GF_NUKE:
			{
				if (seen) obvious = TRUE;
				if (r_ptr->flagsr & RFR_IM_POIS)
				{
					note = _("には耐性がある。", " resists.");
					dam *= 3; dam /= randint1(6) + 6;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (RFR_IM_POIS);
				}
				else if (one_in_(3)) do_poly = TRUE;
				break;
			}

			/* Hellfire -- hurts Evil */
			case GF_HELL_FIRE:
			{
				if (seen) obvious = TRUE;
				if (r_ptr->flags3 & RF3_GOOD)
				{
					note = _("はひどい痛手をうけた。", " is hit hard.");
					dam *= 2;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_GOOD);
				}
				break;
			}

			/* Holy Fire -- hurts Evil, Good are immune, others _resist_ */
			case GF_HOLY_FIRE:
			{
				if (seen) obvious = TRUE;
				if (r_ptr->flags3 & RF3_EVIL)
				{
					dam *= 2;
					note = _("はひどい痛手をうけた。", " is hit hard.");
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= RF3_EVIL;
				}
				else
				{
					note = _("には耐性がある。", " resists.");
					dam *= 3; dam /= randint1(6) + 6;
				}
				break;
			}

			/* Arrow -- XXX no defense */
			case GF_ARROW:
			{
				if (seen) obvious = TRUE;
				break;
			}

			/* Plasma -- XXX perhaps check ELEC or FIRE */
			case GF_PLASMA:
			{
				if (seen) obvious = TRUE;
				if (r_ptr->flagsr & RFR_RES_PLAS)
				{
					note = _("には耐性がある。", " resists.");
					dam *= 3; dam /= randint1(6) + 6;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (RFR_RES_PLAS);
				}
				break;
			}

			/* Nether -- see above */
			case GF_NETHER:
			{
				if (seen) obvious = TRUE;
				if (r_ptr->flagsr & RFR_RES_NETH)
				{
					if (r_ptr->flags3 & RF3_UNDEAD)
					{
						note = _("には完全な耐性がある！", " is immune.");
						dam = 0;
						if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_UNDEAD);
					}
					else
					{
						note = _("には耐性がある。", " resists.");
						dam *= 3; dam /= randint1(6) + 6;
					}
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (RFR_RES_NETH);
				}
				else if (r_ptr->flags3 & RF3_EVIL)
				{
					note = _("はいくらか耐性を示した。", " resists somewhat.");
					dam /= 2;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_EVIL);
				}
				break;
			}

			/* Water (acid) damage -- Water spirits/elementals are immune */
			case GF_WATER:
			{
				if (seen) obvious = TRUE;
				if (r_ptr->flagsr & RFR_RES_WATE)
				{
					if ((m_ptr->r_idx == MON_WATER_ELEM) || (m_ptr->r_idx == MON_UNMAKER))
					{
						note = _("には完全な耐性がある！", " is immune.");
						dam = 0;
					}
					else
					{
						note = _("には耐性がある。", " resists.");
						dam *= 3; dam /= randint1(6) + 6;
					}
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (RFR_RES_WATE);
				}
				break;
			}

			/* Chaos -- Chaos breathers resist */
			case GF_CHAOS:
			{
				if (seen) obvious = TRUE;
				if (r_ptr->flagsr & RFR_RES_CHAO)
				{
					note = _("には耐性がある。", " resists.");
					dam *= 3; dam /= randint1(6) + 6;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (RFR_RES_CHAO);
				}
				else if ((r_ptr->flags3 & RF3_DEMON) && one_in_(3))
				{
					note = _("はいくらか耐性を示した。", " resists somewhat.");
					dam *= 3; dam /= randint1(6) + 6;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_DEMON);
				}
				else
				{
					do_poly = TRUE;
					do_conf = (5 + randint1(11) + r) / (r + 1);
				}
				break;
			}

			/* Shards -- Shard breathers resist */
			case GF_SHARDS:
			{
				if (seen) obvious = TRUE;
				if (r_ptr->flagsr & RFR_RES_SHAR)
				{
					note = _("には耐性がある。", " resists.");
					dam *= 3; dam /= randint1(6) + 6;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (RFR_RES_SHAR);
				}
				break;
			}

			/* Rocket: Shard resistance helps */
			case GF_ROCKET:
			{
				if (seen) obvious = TRUE;
				if (r_ptr->flagsr & RFR_RES_SHAR)
				{
					note = _("はいくらか耐性を示した。", " resists somewhat.");
					dam /= 2;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (RFR_RES_SHAR);
				}
				break;
			}


			/* Sound -- Sound breathers resist */
			case GF_SOUND:
			{
				if (seen) obvious = TRUE;
				if (r_ptr->flagsr & RFR_RES_SOUN)
				{
					note = _("には耐性がある。", " resists.");
					dam *= 2; dam /= randint1(6) + 6;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (RFR_RES_SOUN);
				}
				else do_stun = (10 + randint1(15) + r) / (r + 1);
				break;
			}

			/* Confusion */
			case GF_CONFUSION:
			{
				if (seen) obvious = TRUE;
				if (r_ptr->flags3 & RF3_NO_CONF)
				{
					note = _("には耐性がある。", " resists.");
					dam *= 3; dam /= randint1(6) + 6;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_NO_CONF);
				}
				else do_conf = (10 + randint1(15) + r) / (r + 1);
				break;
			}

			/* Disenchantment -- Breathers and Disenchanters resist */
			case GF_DISENCHANT:
			{
				if (seen) obvious = TRUE;
				if (r_ptr->flagsr & RFR_RES_DISE)
				{
					note = _("には耐性がある。", " resists.");
					dam *= 3; dam /= randint1(6) + 6;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (RFR_RES_DISE);
				}
				break;
			}

			/* Nexus -- Breathers and Existers resist */
			case GF_NEXUS:
			{
				if (seen) obvious = TRUE;
				if (r_ptr->flagsr & RFR_RES_NEXU)
				{
					note = _("には耐性がある。", " resists.");
					dam *= 3; dam /= randint1(6) + 6;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (RFR_RES_NEXU);
				}
				break;
			}

			/* Force */
			case GF_FORCE:
			{
				if (seen) obvious = TRUE;
				if (r_ptr->flagsr & RFR_RES_WALL)
				{
					note = _("には耐性がある。", " resists.");
					dam *= 3; dam /= randint1(6) + 6;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (RFR_RES_WALL);
				}
				else do_stun = (randint1(15) + r) / (r + 1);
				break;
			}

			/* Inertia -- breathers resist */
			case GF_INERTIAL:
			{
				if (seen) obvious = TRUE;
				if (r_ptr->flagsr & RFR_RES_INER)
				{
					note = _("には耐性がある。", " resists.");
					dam *= 3; dam /= randint1(6) + 6;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (RFR_RES_INER);
				}
				else
				{
					/* Powerful monsters can resist */
					if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
						(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
					{
						obvious = FALSE;
					}
					/* Normal monsters slow down */
					else
					{
						if (set_monster_slow(g_ptr->m_idx, MON_SLOW(m_ptr) + 50))
						{
							note = _("の動きが遅くなった。", " starts moving slower.");
						}
					}
				}
				break;
			}

			/* Time -- breathers resist */
			case GF_TIME:
			{
				if (seen) obvious = TRUE;
				if (r_ptr->flagsr & RFR_RES_TIME)
				{
					note = _("には耐性がある。", " resists.");
					dam *= 3; dam /= randint1(6) + 6;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (RFR_RES_TIME);
				}
				else do_time = (dam + 1) / 2;
				break;
			}

			/* Gravity -- breathers resist */
			case GF_GRAVITY:
			{
				bool resist_tele = FALSE;

				if (seen) obvious = TRUE;
				if (r_ptr->flagsr & RFR_RES_TELE)
				{
					if (r_ptr->flags1 & (RF1_UNIQUE))
					{
						if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= RFR_RES_TELE;
						note = _("には効果がなかった。", " is unaffected!");
						resist_tele = TRUE;
					}
					else if (r_ptr->level > randint1(100))
					{
						if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= RFR_RES_TELE;
						note = _("には耐性がある！", " resists!");
						resist_tele = TRUE;
					}
				}

				if (!resist_tele) do_dist = 10;
				else do_dist = 0;
				if (p_ptr->riding && (g_ptr->m_idx == p_ptr->riding)) do_dist = 0;

				if (r_ptr->flagsr & RFR_RES_GRAV)
				{
					note = _("には耐性がある！", " resists!");
					dam *= 3; dam /= randint1(6) + 6;
					do_dist = 0;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (RFR_RES_GRAV);
				}
				else
				{
					/* 1. slowness */
					/* Powerful monsters can resist */
					if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
						(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
					{
						obvious = FALSE;
					}
					/* Normal monsters slow down */
					else
					{
						if (set_monster_slow(g_ptr->m_idx, MON_SLOW(m_ptr) + 50))
						{
							note = _("の動きが遅くなった。", " starts moving slower.");
						}
					}

					/* 2. stun */
					do_stun = damroll((caster_lev / 20) + 3 , (dam)) + 1;

					/* Attempt a saving throw */
					if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
						(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
					{
						/* Resist */
						do_stun = 0;
						/* No obvious effect */
						note = _("には効果がなかった。", " is unaffected!");
						obvious = FALSE;
					}
				}
				break;
			}

			/* Pure damage */
			case GF_MANA:
			case GF_SEEKER:
			case GF_SUPER_RAY:
			{
				if (seen) obvious = TRUE;
				break;
			}


			/* Pure damage */
			case GF_DISINTEGRATE:
			{
				if (seen) obvious = TRUE;
				if (r_ptr->flags3 & RF3_HURT_ROCK)
				{
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_HURT_ROCK);
					note = _("の皮膚がただれた！", " loses some skin!");
					note_dies = _("は蒸発した！", " evaporates!");
					dam *= 2;
				}
				break;
			}

			case GF_PSI:
			{
				if (seen) obvious = TRUE;

				/* PSI only works if the monster can see you! -- RG */
				if (!(los(m_ptr->fy, m_ptr->fx, p_ptr->y, p_ptr->x)))
				{
					if (seen_msg) 
						msg_format(_("%sはあなたが見えないので影響されない！", "%^s can't see you, and isn't affected!"), m_name);
					skipped = TRUE;
					break;
				}
				if (r_ptr->flags2 & RF2_EMPTY_MIND)
				{
					dam = 0;
					note = _("には完全な耐性がある！", " is immune.");
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags2 |= (RF2_EMPTY_MIND);

				}
				else if ((r_ptr->flags2 & (RF2_STUPID | RF2_WEIRD_MIND)) ||
						 (r_ptr->flags3 & RF3_ANIMAL) ||
						 (r_ptr->level > randint1(3 * dam)))
				{
					note = _("には耐性がある！", " resists!");
					dam /= 3;

					/*
					 * Powerful demons & undead can turn a mindcrafter's
					 * attacks back on them
					 */
					if ((r_ptr->flags3 & (RF3_UNDEAD | RF3_DEMON)) &&
						(r_ptr->level > p_ptr->lev / 2) &&
						one_in_(2))
					{
						note = NULL;
						msg_format(_("%^sの堕落した精神は攻撃を跳ね返した！", 
							(seen ? "%^s's corrupted mind backlashes your attack!" : 
									"%^ss corrupted mind backlashes your attack!")), m_name);

						/* Saving throw */
						if ((randint0(100 + r_ptr->level / 2) < p_ptr->skill_sav) && !CHECK_MULTISHADOW(p_ptr))
						{
							msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
						}
						else
						{
							/* Injure +/- confusion */
							monster_desc(killer, m_ptr, MD_WRONGDOER_NAME);
							take_hit(p_ptr, DAMAGE_ATTACK, dam, killer, -1);  /* has already been /3 */
							if (one_in_(4) && !CHECK_MULTISHADOW(p_ptr))
							{
								switch (randint1(4))
								{
									case 1:
										set_confused(p_ptr, p_ptr->confused + 3 + randint1(dam));
										break;
									case 2:
										set_stun(p_ptr, p_ptr->stun + randint1(dam));
										break;
									case 3:
									{
										if (r_ptr->flags3 & RF3_NO_FEAR)
											note = _("には効果がなかった。", " is unaffected.");
										else
											set_afraid(p_ptr, p_ptr->afraid + 3 + randint1(dam));
										break;
									}
									default:
										if (!p_ptr->free_act)
											(void)set_paralyzed(p_ptr, p_ptr->paralyzed + randint1(dam));
										break;
								}
							}
						}
						dam = 0;
					}
				}

				if ((dam > 0) && one_in_(4))
				{
					switch (randint1(4))
					{
						case 1:
							do_conf = 3 + randint1(dam);
							break;
						case 2:
							do_stun = 3 + randint1(dam);
							break;
						case 3:
							do_fear = 3 + randint1(dam);
							break;
						default:
							note = _("は眠り込んでしまった！", " falls asleep!");
							do_sleep = 3 + randint1(dam);
							break;
					}
				}

				note_dies = _("の精神は崩壊し、肉体は抜け殻となった。", " collapses, a mindless husk.");
				break;
			}

			case GF_PSI_DRAIN:
			{
				if (seen) obvious = TRUE;
				if (r_ptr->flags2 & RF2_EMPTY_MIND)
				{
					dam = 0;
					note = _("には完全な耐性がある！", " is immune.");
				}
				else if ((r_ptr->flags2 & (RF2_STUPID | RF2_WEIRD_MIND)) ||
						 (r_ptr->flags3 & RF3_ANIMAL) ||
						 (r_ptr->level > randint1(3 * dam)))
				{
					note = _("には耐性がある！", " resists!");
					dam /= 3;

					/*
					 * Powerful demons & undead can turn a mindcrafter's
					 * attacks back on them
					 */
					if ((r_ptr->flags3 & (RF3_UNDEAD | RF3_DEMON)) &&
						 (r_ptr->level > p_ptr->lev / 2) &&
						 (one_in_(2)))
					{
						note = NULL;
						msg_format(_("%^sの堕落した精神は攻撃を跳ね返した！", 
							(seen ? "%^s's corrupted mind backlashes your attack!" : 
									"%^ss corrupted mind backlashes your attack!")), m_name);
						/* Saving throw */
						if ((randint0(100 + r_ptr->level / 2) < p_ptr->skill_sav) && !CHECK_MULTISHADOW(p_ptr))
						{
							msg_print(_("あなたは効力を跳ね返した！", "You resist the effects!"));
						}
						else
						{
							/* Injure + mana drain */
							monster_desc(killer, m_ptr, MD_WRONGDOER_NAME);
							if (!CHECK_MULTISHADOW(p_ptr))
							{
								msg_print(_("超能力パワーを吸いとられた！", "Your psychic energy is drained!"));
								p_ptr->csp -= damroll(5, dam) / 2;
								if (p_ptr->csp < 0) p_ptr->csp = 0;
								p_ptr->redraw |= PR_MANA;
								p_ptr->window |= (PW_SPELL);
							}
							take_hit(p_ptr, DAMAGE_ATTACK, dam, killer, -1);  /* has already been /3 */
						}
						dam = 0;
					}
				}
				else if (dam > 0)
				{
					int b = damroll(5, dam) / 4;
					concptr str = (p_ptr->pclass == CLASS_MINDCRAFTER) ? _("超能力パワー", "psychic energy") : _("魔力", "mana");
					concptr msg = _("あなたは%sの苦痛を%sに変換した！", 
						 (seen ? "You convert %s's pain into %s!" : 
								 "You convert %ss pain into %s!"));
					msg_format(msg, m_name, str);

					b = MIN(p_ptr->msp, p_ptr->csp + b);
					p_ptr->csp = b;
					p_ptr->redraw |= PR_MANA;
					p_ptr->window |= (PW_SPELL);
				}
				note_dies = _("の精神は崩壊し、肉体は抜け殻となった。", " collapses, a mindless husk.");
				break;
			}

			case GF_TELEKINESIS:
			{
				if (seen) obvious = TRUE;
				if (one_in_(4))
				{
					if (p_ptr->riding && (g_ptr->m_idx == p_ptr->riding)) do_dist = 0;
					else do_dist = 7;
				}

				/* 1. stun */
				do_stun = damroll((caster_lev / 20) + 3 , dam) + 1;

				/* Attempt a saving throw */
				if ((r_ptr->flags1 & RF1_UNIQUE) ||
					(r_ptr->level > 5 + randint1(dam)))
				{
					/* Resist */
					do_stun = 0;
					/* No obvious effect */
					obvious = FALSE;
				}
				break;
			}

			/* Psycho-spear -- powerful magic missile */
			case GF_PSY_SPEAR:
			{
				if (seen) obvious = TRUE;
				break;
			}

			/* Meteor -- powerful magic missile */
			case GF_METEOR:
			{
				if (seen) obvious = TRUE;
				break;
			}

			case GF_DOMINATION:
			{
				if (!is_hostile(m_ptr)) break;
				if (seen) obvious = TRUE;
				/* Attempt a saving throw */
				if ((r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) ||
					(r_ptr->flags3 & RF3_NO_CONF) ||
					(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
				{
					/* Memorize a flag */
					if (r_ptr->flags3 & RF3_NO_CONF)
					{
						if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_NO_CONF);
					}

					/* Resist */
					do_conf = 0;

					/*
					 * Powerful demons & undead can turn a mindcrafter's
					 * attacks back on them
					 */
					if ((r_ptr->flags3 & (RF3_UNDEAD | RF3_DEMON)) &&
						(r_ptr->level > p_ptr->lev / 2) &&
						(one_in_(2)))
					{
						note = NULL;
						msg_format(_("%^sの堕落した精神は攻撃を跳ね返した！",
							(seen ? "%^s's corrupted mind backlashes your attack!" :
							"%^ss corrupted mind backlashes your attack!")), m_name);

						/* Saving throw */
						if (randint0(100 + r_ptr->level/2) < p_ptr->skill_sav)
						{
							msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
						}
						else
						{
							/* Confuse, stun, terrify */
							switch (randint1(4))
							{
								case 1:
									set_stun(p_ptr, p_ptr->stun + dam / 2);
									break;
								case 2:
									set_confused(p_ptr, p_ptr->confused + dam / 2);
									break;
								default:
								{
									if (r_ptr->flags3 & RF3_NO_FEAR)
										note = _("には効果がなかった。", " is unaffected.");
									else
										set_afraid(p_ptr, p_ptr->afraid + dam);
								}
							}
						}
					}
					else
					{
						/* No obvious effect */
						note = _("には効果がなかった。", " is unaffected.");
						obvious = FALSE;
					}
				}
				else
				{
					if (!common_saving_throw_charm(p_ptr, dam, m_ptr))
					{
						note = _("があなたに隷属した。", " is in your thrall!");
						set_pet(m_ptr);
					}
					else
					{
						switch (randint1(4))
						{
							case 1:
								do_stun = dam / 2;
								break;
							case 2:
								do_conf = dam / 2;
								break;
							default:
								do_fear = dam;
						}
					}
				}

				/* No "real" damage */
				dam = 0;
				break;
			}

			/* Ice -- Cold + Cuts + Stun */
			case GF_ICE:
			{
				if (seen) obvious = TRUE;
				do_stun = (randint1(15) + 1) / (r + 1);
				if (r_ptr->flagsr & RFR_IM_COLD)
				{
					note = _("にはかなり耐性がある！", " resists a lot.");
					dam /= 9;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (RFR_IM_COLD);
				}
				else if (r_ptr->flags3 & (RF3_HURT_COLD))
				{
					note = _("はひどい痛手をうけた。", " is hit hard.");
					dam *= 2;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_HURT_COLD);
				}
				break;
			}


			/* Drain Life */
			case GF_HYPODYNAMIA:
			{
				if (seen) obvious = TRUE;
				if (!monster_living(m_ptr->r_idx))
				{
					if (is_original_ap_and_seen(m_ptr))
					{
						if (r_ptr->flags3 & RF3_DEMON) r_ptr->r_flags3 |= (RF3_DEMON);
						if (r_ptr->flags3 & RF3_UNDEAD) r_ptr->r_flags3 |= (RF3_UNDEAD);
						if (r_ptr->flags3 & RF3_NONLIVING) r_ptr->r_flags3 |= (RF3_NONLIVING);
					}
					note = _("には効果がなかった。", " is unaffected.");
					obvious = FALSE;
					dam = 0;
				}
				else do_time = (dam+7)/8;

				break;
			}

			/* Death Ray */
			case GF_DEATH_RAY:
			{
				if (seen) obvious = TRUE;
				if (!monster_living(m_ptr->r_idx))
				{
					if (is_original_ap_and_seen(m_ptr))
					{
						if (r_ptr->flags3 & RF3_DEMON) r_ptr->r_flags3 |= (RF3_DEMON);
						if (r_ptr->flags3 & RF3_UNDEAD) r_ptr->r_flags3 |= (RF3_UNDEAD);
						if (r_ptr->flags3 & RF3_NONLIVING) r_ptr->r_flags3 |= (RF3_NONLIVING);
					}
					note = _("には完全な耐性がある！", " is immune.");
					obvious = FALSE;
					dam = 0;
				}
				else if (((r_ptr->flags1 & RF1_UNIQUE) &&
					 (randint1(888) != 666)) ||
					 (((r_ptr->level + randint1(20)) > randint1((caster_lev / 2) + randint1(10))) &&
					 randint1(100) != 66))
				{
					note = _("には耐性がある！", " resists!");
					obvious = FALSE;
					dam = 0;
				}

				break;
			}

			/* Polymorph monster (Use "dam" as "power") */
			case GF_OLD_POLY:
			{
				if (seen) obvious = TRUE;
				/* Attempt to polymorph (see below) */
				do_poly = TRUE;

				/* Powerful monsters can resist */
				if ((r_ptr->flags1 & RF1_UNIQUE) ||
					(r_ptr->flags1 & RF1_QUESTOR) ||
					(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
				{
					note = _("には効果がなかった。", " is unaffected.");
					do_poly = FALSE;
					obvious = FALSE;
				}

				/* No "real" damage */
				dam = 0;

				break;
			}


			/* Clone monsters (Ignore "dam") */
			case GF_OLD_CLONE:
			{
				if (seen) obvious = TRUE;

				if ((p_ptr->inside_arena) || is_pet(m_ptr) || (r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) || (r_ptr->flags7 & (RF7_NAZGUL | RF7_UNIQUE2)))
				{
					note = _("には効果がなかった。", " is unaffected.");
				}
				else
				{
					/* Heal fully */
					m_ptr->hp = m_ptr->maxhp;

					/* Attempt to clone. */
					if (multiply_monster(g_ptr->m_idx, TRUE, 0L))
					{
						note = _("が分裂した！", " spawns!");
					}
				}

				/* No "real" damage */
				dam = 0;

				break;
			}


			/* Heal Monster (use "dam" as amount of healing) */
			case GF_STAR_HEAL:
			{
				if (seen) obvious = TRUE;

				/* Wake up */
				(void)set_monster_csleep(g_ptr->m_idx, 0);

				if (m_ptr->maxhp < m_ptr->max_maxhp)
				{
					if (seen_msg) msg_format(_("%^sの強さが戻った。", "%^s recovers %s vitality."), m_name, m_poss);
					m_ptr->maxhp = m_ptr->max_maxhp;
				}

				if (!dam)
				{
					/* Redraw (later) if needed */
					if (p_ptr->health_who == g_ptr->m_idx) p_ptr->redraw |= (PR_HEALTH);
					if (p_ptr->riding == g_ptr->m_idx) p_ptr->redraw |= (PR_UHEALTH);
					break;
				}

				/* Fall through */
			}
			case GF_OLD_HEAL:
			{
				if (seen) obvious = TRUE;

				/* Wake up */
				(void)set_monster_csleep(g_ptr->m_idx, 0);
				if (MON_STUNNED(m_ptr))
				{
					if (seen_msg) msg_format(_("%^sは朦朧状態から立ち直った。", "%^s is no longer stunned."), m_name);
					(void)set_monster_stunned(g_ptr->m_idx, 0);
				}
				if (MON_CONFUSED(m_ptr))
				{
					if (seen_msg) msg_format(_("%^sは混乱から立ち直った。", "%^s is no longer confused."), m_name);
					(void)set_monster_confused(g_ptr->m_idx, 0);
				}
				if (MON_MONFEAR(m_ptr))
				{
					if (seen_msg) msg_format(_("%^sは勇気を取り戻した。", "%^s recovers %s courage."), m_name);
					(void)set_monster_monfear(g_ptr->m_idx, 0);
				}

				/* Heal */
				if (m_ptr->hp < 30000) m_ptr->hp += dam;

				/* No overflow */
				if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

				if (!who)
				{
					chg_virtue(p_ptr, V_VITALITY, 1);

					if (r_ptr->flags1 & RF1_UNIQUE)
						chg_virtue(p_ptr, V_INDIVIDUALISM, 1);

					if (is_friendly(m_ptr))
						chg_virtue(p_ptr, V_HONOUR, 1);
					else if (!(r_ptr->flags3 & RF3_EVIL))
					{
						if (r_ptr->flags3 & RF3_GOOD)
							chg_virtue(p_ptr, V_COMPASSION, 2);
						else
							chg_virtue(p_ptr, V_COMPASSION, 1);
					}

					if (r_ptr->flags3 & RF3_ANIMAL)
						chg_virtue(p_ptr, V_NATURE, 1);
				}

				if (m_ptr->r_idx == MON_LEPER)
				{
					heal_leper = TRUE;
					if (!who) chg_virtue(p_ptr, V_COMPASSION, 5);
				}

				/* Redraw (later) if needed */
				if (p_ptr->health_who == g_ptr->m_idx) p_ptr->redraw |= (PR_HEALTH);
				if (p_ptr->riding == g_ptr->m_idx) p_ptr->redraw |= (PR_UHEALTH);

				note = _("は体力を回復したようだ。", " looks healthier.");

				/* No "real" damage */
				dam = 0;
				break;
			}


			/* Speed Monster (Ignore "dam") */
			case GF_OLD_SPEED:
			{
				if (seen) obvious = TRUE;

				/* Speed up */
				if (set_monster_fast(g_ptr->m_idx, MON_FAST(m_ptr) + 100))
				{
					note = _("の動きが速くなった。", " starts moving faster.");
				}

				if (!who)
				{
					if (r_ptr->flags1 & RF1_UNIQUE)
						chg_virtue(p_ptr, V_INDIVIDUALISM, 1);
					if (is_friendly(m_ptr))
						chg_virtue(p_ptr, V_HONOUR, 1);
				}

				/* No "real" damage */
				dam = 0;
				break;
			}


			/* Slow Monster (Use "dam" as "power") */
			case GF_OLD_SLOW:
			{
				if (seen) obvious = TRUE;

				/* Powerful monsters can resist */
				if ((r_ptr->flags1 & RF1_UNIQUE) ||
					(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
				{
					note = _("には効果がなかった。", " is unaffected.");
					obvious = FALSE;
				}

				/* Normal monsters slow down */
				else
				{
					if (set_monster_slow(g_ptr->m_idx, MON_SLOW(m_ptr) + 50))
					{
						note = _("の動きが遅くなった。", " starts moving slower.");
					}
				}

				/* No "real" damage */
				dam = 0;
				break;
			}


			/* Sleep (Use "dam" as "power") */
			case GF_OLD_SLEEP:
			{
				if (seen) obvious = TRUE;

				/* Attempt a saving throw */
				if ((r_ptr->flags1 & RF1_UNIQUE) ||
					(r_ptr->flags3 & RF3_NO_SLEEP) ||
					(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
				{
					/* Memorize a flag */
					if (r_ptr->flags3 & RF3_NO_SLEEP)
					{
						if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_NO_SLEEP);
					}
					/* No obvious effect */
					note = _("には効果がなかった。", " is unaffected.");
					obvious = FALSE;
				}
				else
				{
					/* Go to sleep (much) later */
					note = _("は眠り込んでしまった！", " falls asleep!");
					do_sleep = 500;
				}

				/* No "real" damage */
				dam = 0;
				break;
			}


			/* Sleep (Use "dam" as "power") */
			case GF_STASIS_EVIL:
			{
				if (seen) obvious = TRUE;

				/* Attempt a saving throw */
				if ((r_ptr->flags1 & RF1_UNIQUE) ||
					!(r_ptr->flags3 & RF3_EVIL) ||
					(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
				{
					note = _("には効果がなかった。", " is unaffected.");
					obvious = FALSE;
				}
				else
				{
					/* Go to sleep (much) later */
					note = _("は動けなくなった！", " is suspended!");
					do_sleep = 500;
				}

				/* No "real" damage */
				dam = 0;
				break;
			}

			/* Sleep (Use "dam" as "power") */
			case GF_STASIS:
			{
				if (seen) obvious = TRUE;

				/* Attempt a saving throw */
				if ((r_ptr->flags1 & RF1_UNIQUE) ||
					(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
				{
					note = _("には効果がなかった。", " is unaffected.");
					obvious = FALSE;
				}
				else
				{
					/* Go to sleep (much) later */
					note = _("は動けなくなった！", " is suspended!");
					do_sleep = 500;
				}

				/* No "real" damage */
				dam = 0;
				break;
			}

			/* Charm monster */
			case GF_CHARM:
			{
				int vir;
				vir = virtue_number(p_ptr, V_HARMONY);
				if (vir)
				{
					dam += p_ptr->virtues[vir-1]/10;
				}

				vir = virtue_number(p_ptr, V_INDIVIDUALISM);
				if (vir)
				{
					dam -= p_ptr->virtues[vir-1]/20;
				}

				if (seen) obvious = TRUE;

				/* Attempt a saving throw */
				if (common_saving_throw_charm(p_ptr, dam, m_ptr))
				{

					/* Resist */
					/* No obvious effect */
					note = _("には効果がなかった。", " is unaffected.");
					obvious = FALSE;

					if (one_in_(4)) m_ptr->mflag2 |= MFLAG2_NOPET;
				}
				else if (p_ptr->cursed & TRC_AGGRAVATE)
				{
					note = _("はあなたに敵意を抱いている！", " hates you too much!");
					if (one_in_(4)) m_ptr->mflag2 |= MFLAG2_NOPET;
				}
				else
				{
					note = _("は突然友好的になったようだ！", " suddenly seems friendly!");
					set_pet(m_ptr);

					chg_virtue(p_ptr, V_INDIVIDUALISM, -1);
					if (r_ptr->flags3 & RF3_ANIMAL)
						chg_virtue(p_ptr, V_NATURE, 1);
				}

				/* No "real" damage */
				dam = 0;
				break;
			}

			/* Control undead */
			case GF_CONTROL_UNDEAD:
			{
				int vir;
				if (seen) obvious = TRUE;

				vir = virtue_number(p_ptr, V_UNLIFE);
				if (vir)
				{
					dam += p_ptr->virtues[vir-1]/10;
				}

				vir = virtue_number(p_ptr, V_INDIVIDUALISM);
				if (vir)
				{
					dam -= p_ptr->virtues[vir-1]/20;
				}

				/* Attempt a saving throw */
				if (common_saving_throw_control(p_ptr, dam, m_ptr) ||
					!(r_ptr->flags3 & RF3_UNDEAD))
				{
					/* No obvious effect */
					note = _("には効果がなかった。", " is unaffected.");
					obvious = FALSE;
					if (one_in_(4)) m_ptr->mflag2 |= MFLAG2_NOPET;
				}
				else if (p_ptr->cursed & TRC_AGGRAVATE)
				{
					note = _("はあなたに敵意を抱いている！", " hates you too much!");
					if (one_in_(4)) m_ptr->mflag2 |= MFLAG2_NOPET;
				}
				else
				{
					note = _("は既にあなたの奴隷だ！", " is in your thrall!");
					set_pet(m_ptr);
				}

				/* No "real" damage */
				dam = 0;
				break;
			}

			/* Control demon */
			case GF_CONTROL_DEMON:
			{
				int vir;
				if (seen) obvious = TRUE;

				vir = virtue_number(p_ptr, V_UNLIFE);
				if (vir)
				{
					dam += p_ptr->virtues[vir-1]/10;
				}

				vir = virtue_number(p_ptr, V_INDIVIDUALISM);
				if (vir)
				{
					dam -= p_ptr->virtues[vir-1]/20;
				}

				/* Attempt a saving throw */
				if (common_saving_throw_control(p_ptr, dam, m_ptr) ||
					!(r_ptr->flags3 & RF3_DEMON))
				{
					/* No obvious effect */
					note = _("には効果がなかった。", " is unaffected.");
					obvious = FALSE;
					if (one_in_(4)) m_ptr->mflag2 |= MFLAG2_NOPET;
				}
				else if (p_ptr->cursed & TRC_AGGRAVATE)
				{
					note = _("はあなたに敵意を抱いている！", " hates you too much!");
					if (one_in_(4)) m_ptr->mflag2 |= MFLAG2_NOPET;
				}
				else
				{
					note = _("は既にあなたの奴隷だ！", " is in your thrall!");
					set_pet(m_ptr);
				}

				/* No "real" damage */
				dam = 0;
				break;
			}

			/* Tame animal */
			case GF_CONTROL_ANIMAL:
			{
				int vir;
				if (seen) obvious = TRUE;

				vir = virtue_number(p_ptr, V_NATURE);
				if (vir)
				{
					dam += p_ptr->virtues[vir-1]/10;
				}

				vir = virtue_number(p_ptr, V_INDIVIDUALISM);
				if (vir)
				{
					dam -= p_ptr->virtues[vir-1]/20;
				}

				/* Attempt a saving throw */
				if (common_saving_throw_control(p_ptr, dam, m_ptr) ||
					!(r_ptr->flags3 & RF3_ANIMAL))
				{
					/* Resist */
					/* No obvious effect */
					note = _("には効果がなかった。", " is unaffected.");
					obvious = FALSE;
					if (one_in_(4)) m_ptr->mflag2 |= MFLAG2_NOPET;
				}
				else if (p_ptr->cursed & TRC_AGGRAVATE)
				{
					note = _("はあなたに敵意を抱いている！", " hates you too much!");
					if (one_in_(4)) m_ptr->mflag2 |= MFLAG2_NOPET;
				}
				else
				{
					note = _("はなついた。", " is tamed!");
					set_pet(m_ptr);
					if (r_ptr->flags3 & RF3_ANIMAL)
						chg_virtue(p_ptr, V_NATURE, 1);
				}

				/* No "real" damage */
				dam = 0;
				break;
			}

			/* Tame animal */
			case GF_CHARM_LIVING:
			{
				int vir;

				vir = virtue_number(p_ptr, V_UNLIFE);
				if (seen) obvious = TRUE;

				vir = virtue_number(p_ptr, V_UNLIFE);
				if (vir)
				{
					dam -= p_ptr->virtues[vir-1]/10;
				}

				vir = virtue_number(p_ptr, V_INDIVIDUALISM);
				if (vir)
				{
					dam -= p_ptr->virtues[vir-1]/20;
				}

				msg_format(_("%sを見つめた。", "You stare into %s."), m_name);

				/* Attempt a saving throw */
				if (common_saving_throw_charm(p_ptr, dam, m_ptr) ||
					!monster_living(m_ptr->r_idx))
				{
					/* Resist */
					/* No obvious effect */
					note = _("には効果がなかった。", " is unaffected.");
					obvious = FALSE;
					if (one_in_(4)) m_ptr->mflag2 |= MFLAG2_NOPET;
				}
				else if (p_ptr->cursed & TRC_AGGRAVATE)
				{
					note = _("はあなたに敵意を抱いている！", " hates you too much!");
					if (one_in_(4)) m_ptr->mflag2 |= MFLAG2_NOPET;
				}
				else
				{
					note = _("を支配した。", " is tamed!");
					set_pet(m_ptr);
					if (r_ptr->flags3 & RF3_ANIMAL)
						chg_virtue(p_ptr, V_NATURE, 1);
				}

				/* No "real" damage */
				dam = 0;
				break;
			}

			/* Confusion (Use "dam" as "power") */
			case GF_OLD_CONF:
			{
				if (seen) obvious = TRUE;

				/* Get confused later */
				do_conf = damroll(3, (dam / 2)) + 1;

				/* Attempt a saving throw */
				if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
					(r_ptr->flags3 & (RF3_NO_CONF)) ||
					(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
				{
					/* Memorize a flag */
					if (r_ptr->flags3 & (RF3_NO_CONF))
					{
						if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_NO_CONF);
					}

					/* Resist */
					do_conf = 0;

					/* No obvious effect */
					note = _("には効果がなかった。", " is unaffected.");
					obvious = FALSE;
				}

				/* No "real" damage */
				dam = 0;
				break;
			}

			case GF_STUN:
			{
				if (seen) obvious = TRUE;

				do_stun = damroll((caster_lev / 20) + 3 , (dam)) + 1;

				/* Attempt a saving throw */
				if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
					(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
				{
					/* Resist */
					do_stun = 0;

					/* No obvious effect */
					note = _("には効果がなかった。", " is unaffected.");
					obvious = FALSE;
				}

				/* No "real" damage */
				dam = 0;
				break;
			}

			/* Lite, but only hurts susceptible creatures */
			case GF_LITE_WEAK:
			{
				if (!dam)
				{
					skipped = TRUE;
					break;
				}
				/* Hurt by light */
				if (r_ptr->flags3 & (RF3_HURT_LITE))
				{
					/* Obvious effect */
					if (seen) obvious = TRUE;

					/* Memorize the effects */
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_HURT_LITE);

					/* Special effect */
					note = _("は光に身をすくめた！", " cringes from the light!");
					note_dies = _("は光を受けてしぼんでしまった！", " shrivels away in the light!");
				}

				/* Normally no damage */
				else
				{
					/* No damage */
					dam = 0;
				}

				break;
			}



			/* Lite -- opposite of Dark */
			case GF_LITE:
			{
				if (seen) obvious = TRUE;

				if (r_ptr->flagsr & RFR_RES_LITE)
				{
					note = _("には耐性がある！", " resists!");
					dam *= 2; dam /= (randint1(6)+6);
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (RFR_RES_LITE);
				}
				else if (r_ptr->flags3 & (RF3_HURT_LITE))
				{
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_HURT_LITE);
					note = _("は光に身をすくめた！", " cringes from the light!");
					note_dies = _("は光を受けてしぼんでしまった！", " shrivels away in the light!");
					dam *= 2;
				}
				break;
			}


			/* Dark -- opposite of Lite */
			case GF_DARK:
			{
				if (seen) obvious = TRUE;

				if (r_ptr->flagsr & RFR_RES_DARK)
				{
					note = _("には耐性がある！", " resists!");
					dam *= 2; dam /= (randint1(6)+6);
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= (RFR_RES_DARK);
				}
				break;
			}


			/* Stone to Mud */
			case GF_KILL_WALL:
			{
				/* Hurt by rock remover */
				if (r_ptr->flags3 & (RF3_HURT_ROCK))
				{
					/* Notice effect */
					if (seen) obvious = TRUE;

					/* Memorize the effects */
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_HURT_ROCK);

					/* Cute little message */
					note = _("の皮膚がただれた！", " loses some skin!");
					note_dies = _("はドロドロに溶けた！", " dissolves!");
				}

				/* Usually, ignore the effects */
				else
				{
					/* No damage */
					dam = 0;
				}

				break;
			}


			/* Teleport undead (Use "dam" as "power") */
			case GF_AWAY_UNDEAD:
			{
				/* Only affect undead */
				if (r_ptr->flags3 & (RF3_UNDEAD))
				{
					bool resists_tele = FALSE;

					if (r_ptr->flagsr & RFR_RES_TELE)
					{
						if ((r_ptr->flags1 & (RF1_UNIQUE)) || (r_ptr->flagsr & RFR_RES_ALL))
						{
							if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= RFR_RES_TELE;
							note = _("には効果がなかった。", " is unaffected.");
							resists_tele = TRUE;
						}
						else if (r_ptr->level > randint1(100))
						{
							if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= RFR_RES_TELE;
							note = _("には耐性がある！", " resists!");
							resists_tele = TRUE;
						}
					}

					if (!resists_tele)
					{
						if (seen) obvious = TRUE;
						if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_UNDEAD);
						do_dist = dam;
					}
				}

				/* Others ignore */
				else
				{
					/* Irrelevant */
					skipped = TRUE;
				}

				/* No "real" damage */
				dam = 0;
				break;
			}


			/* Teleport evil (Use "dam" as "power") */
			case GF_AWAY_EVIL:
			{
				/* Only affect evil */
				if (r_ptr->flags3 & (RF3_EVIL))
				{
					bool resists_tele = FALSE;

					if (r_ptr->flagsr & RFR_RES_TELE)
					{
						if ((r_ptr->flags1 & (RF1_UNIQUE)) || (r_ptr->flagsr & RFR_RES_ALL))
						{
							if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= RFR_RES_TELE;
							note = _("には効果がなかった。", " is unaffected.");
							resists_tele = TRUE;
						}
						else if (r_ptr->level > randint1(100))
						{
							if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= RFR_RES_TELE;
							note = _("には耐性がある！", " resists!");
							resists_tele = TRUE;
						}
					}

					if (!resists_tele)
					{
						if (seen) obvious = TRUE;
						if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_EVIL);
						do_dist = dam;
					}
				}

				/* Others ignore */
				else
				{
					/* Irrelevant */
					skipped = TRUE;
				}

				/* No "real" damage */
				dam = 0;
				break;
			}


			/* Teleport monster (Use "dam" as "power") */
			case GF_AWAY_ALL:
			{
				bool resists_tele = FALSE;
				if (r_ptr->flagsr & RFR_RES_TELE)
				{
					if ((r_ptr->flags1 & (RF1_UNIQUE)) || (r_ptr->flagsr & RFR_RES_ALL))
					{
						if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= RFR_RES_TELE;
						note = _("には効果がなかった。", " is unaffected.");
						resists_tele = TRUE;
					}
					else if (r_ptr->level > randint1(100))
					{
						if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= RFR_RES_TELE;
						note = _("には耐性がある！", " resists!");
						resists_tele = TRUE;
					}
				}

				if (!resists_tele)
				{
					if (seen) obvious = TRUE;

					/* Prepare to teleport */
					do_dist = dam;
				}

				/* No "real" damage */
				dam = 0;
				break;
			}


			/* Turn undead (Use "dam" as "power") */
			case GF_TURN_UNDEAD:
			{
				/* Only affect undead */
				if (r_ptr->flags3 & (RF3_UNDEAD))
				{
					if (seen) obvious = TRUE;

					/* Learn about type */
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_UNDEAD);

					/* Apply some fear */
					do_fear = damroll(3, (dam / 2)) + 1;

					/* Attempt a saving throw */
					if (r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10)
					{
						/* No obvious effect */
						note = _("には効果がなかった。", " is unaffected.");
						obvious = FALSE;
						do_fear = 0;
					}
				}

				/* Others ignore */
				else
				{
					/* Irrelevant */
					skipped = TRUE;
				}

				/* No "real" damage */
				dam = 0;
				break;
			}


			/* Turn evil (Use "dam" as "power") */
			case GF_TURN_EVIL:
			{
				/* Only affect evil */
				if (r_ptr->flags3 & (RF3_EVIL))
				{
					if (seen) obvious = TRUE;

					/* Learn about type */
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_EVIL);

					/* Apply some fear */
					do_fear = damroll(3, (dam / 2)) + 1;

					/* Attempt a saving throw */
					if (r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10)
					{
						/* No obvious effect */
						note = _("には効果がなかった。", " is unaffected.");
						obvious = FALSE;
						do_fear = 0;
					}
				}

				/* Others ignore */
				else
				{
					/* Irrelevant */
					skipped = TRUE;
				}

				/* No "real" damage */
				dam = 0;
				break;
			}


			/* Turn monster (Use "dam" as "power") */
			case GF_TURN_ALL:
			{
				if (seen) obvious = TRUE;

				/* Apply some fear */
				do_fear = damroll(3, (dam / 2)) + 1;

				/* Attempt a saving throw */
				if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
					(r_ptr->flags3 & (RF3_NO_FEAR)) ||
					(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
				{
					/* No obvious effect */
					note = _("には効果がなかった。", " is unaffected.");
					obvious = FALSE;
					do_fear = 0;
				}

				/* No "real" damage */
				dam = 0;
				break;
			}


			/* Dispel undead */
			case GF_DISP_UNDEAD:
			{
				/* Only affect undead */
				if (r_ptr->flags3 & (RF3_UNDEAD))
				{
					if (seen) obvious = TRUE;

					/* Learn about type */
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_UNDEAD);

					note = _("は身震いした。", " shudders.");
					note_dies = _("はドロドロに溶けた！", " dissolves!");
				}

				/* Others ignore */
				else
				{
					/* Irrelevant */
					skipped = TRUE;

					/* No damage */
					dam = 0;
				}

				break;
			}


			/* Dispel evil */
			case GF_DISP_EVIL:
			{
				/* Only affect evil */
				if (r_ptr->flags3 & (RF3_EVIL))
				{
					if (seen) obvious = TRUE;

					/* Learn about type */
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_EVIL);

					note = _("は身震いした。", " shudders.");
					note_dies = _("はドロドロに溶けた！", " dissolves!");
				}

				/* Others ignore */
				else
				{
					/* Irrelevant */
					skipped = TRUE;

					/* No damage */
					dam = 0;
				}

				break;
			}

			/* Dispel good */
			case GF_DISP_GOOD:
			{
				/* Only affect good */
				if (r_ptr->flags3 & (RF3_GOOD))
				{
					if (seen) obvious = TRUE;

					/* Learn about type */
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_GOOD);

					note = _("は身震いした。", " shudders.");
					note_dies = _("はドロドロに溶けた！", " dissolves!");
				}

				/* Others ignore */
				else
				{
					/* Irrelevant */
					skipped = TRUE;

					/* No damage */
					dam = 0;
				}

				break;
			}

			/* Dispel living */
			case GF_DISP_LIVING:
			{
				/* Only affect non-undead */
				if (monster_living(m_ptr->r_idx))
				{
					if (seen) obvious = TRUE;

					note = _("は身震いした。", " shudders.");
					note_dies = _("はドロドロに溶けた！", " dissolves!");
				}

				/* Others ignore */
				else
				{
					/* Irrelevant */
					skipped = TRUE;

					/* No damage */
					dam = 0;
				}

				break;
			}

			/* Dispel demons */
			case GF_DISP_DEMON:
			{
				/* Only affect demons */
				if (r_ptr->flags3 & (RF3_DEMON))
				{
					if (seen) obvious = TRUE;

					/* Learn about type */
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_DEMON);

					note = _("は身震いした。", " shudders.");
					note_dies = _("はドロドロに溶けた！", " dissolves!");
				}

				/* Others ignore */
				else
				{
					/* Irrelevant */
					skipped = TRUE;

					/* No damage */
					dam = 0;
				}

				break;
			}

			/* Dispel monster */
			case GF_DISP_ALL:
			{
				if (seen) obvious = TRUE;
				note = _("は身震いした。", " shudders.");
				note_dies = _("はドロドロに溶けた！", " dissolves!");
				break;
			}

			/* Drain mana */
			case GF_DRAIN_MANA:
			{
				if (seen) obvious = TRUE;
				if ((r_ptr->flags4 & ~(RF4_NOMAGIC_MASK)) || (r_ptr->a_ability_flags1 & ~(RF5_NOMAGIC_MASK)) || (r_ptr->a_ability_flags2 & ~(RF6_NOMAGIC_MASK)))
				{
					if (who > 0)
					{
						/* Heal the monster */
						if (caster_ptr->hp < caster_ptr->maxhp)
						{
							/* Heal */
							caster_ptr->hp += dam;
							if (caster_ptr->hp > caster_ptr->maxhp) caster_ptr->hp = caster_ptr->maxhp;

							/* Redraw (later) if needed */
							if (p_ptr->health_who == who) p_ptr->redraw |= (PR_HEALTH);
							if (p_ptr->riding == who) p_ptr->redraw |= (PR_UHEALTH);

							/* Special message */
							if (see_s_msg)
							{
								monster_desc(killer, caster_ptr, 0);
								msg_format(_("%^sは気分が良さそうだ。", "%^s appears healthier."), killer);
							}
						}
					}
					else
					{
						msg_format(_("%sから精神エネルギーを吸いとった。", "You draw psychic energy from %s."), m_name);
						(void)hp_player(p_ptr, dam);
					}
				}
				else
				{
					if (see_s_msg) msg_format(_("%sには効果がなかった。", "%s is unaffected."), m_name);
				}
				dam = 0;
				break;
			}

			/* Mind blast */
			case GF_MIND_BLAST:
			{
				if (seen) obvious = TRUE;
				if (!who) msg_format(_("%sをじっと睨んだ。", "You gaze intently at %s."), m_name);
				/* Attempt a saving throw */
				if ((r_ptr->flags1 & RF1_UNIQUE) ||
					 (r_ptr->flags3 & RF3_NO_CONF) ||
					 (r_ptr->level > randint1((caster_lev - 10) < 1 ? 1 : (caster_lev - 10)) + 10))
				{
					/* Memorize a flag */
					if (r_ptr->flags3 & (RF3_NO_CONF))
					{
						if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_NO_CONF);
					}
					note = _("には効果がなかった。", " is unaffected.");
					dam = 0;
				}
				else if (r_ptr->flags2 & RF2_EMPTY_MIND)
				{
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags2 |= (RF2_EMPTY_MIND);
					note = _("には完全な耐性がある！", " is immune.");
					dam = 0;
				}
				else if (r_ptr->flags2 & RF2_WEIRD_MIND)
				{
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags2 |= (RF2_WEIRD_MIND);
					note = _("には耐性がある。", " resists.");
					dam /= 3;
				}
				else
				{
					note = _("は精神攻撃を食らった。", " is blasted by psionic energy.");
					note_dies = _("の精神は崩壊し、肉体は抜け殻となった。", " collapses, a mindless husk.");

					if (who > 0) do_conf = randint0(4) + 4;
					else do_conf = randint0(8) + 8;
				}
				break;
			}

			/* Brain smash */
			case GF_BRAIN_SMASH:
			{
				if (seen) obvious = TRUE;
				if (!who) msg_format(_("%sをじっと睨んだ。", "You gaze intently at %s."), m_name);

				/* Attempt a saving throw */
				if ((r_ptr->flags1 & RF1_UNIQUE) ||
					 (r_ptr->flags3 & RF3_NO_CONF) ||
					 (r_ptr->level > randint1((caster_lev - 10) < 1 ? 1 : (caster_lev - 10)) + 10))
				{
					/* Memorize a flag */
					if (r_ptr->flags3 & (RF3_NO_CONF))
					{
						if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_NO_CONF);
					}
					note = _("には効果がなかった。", " is unaffected.");
					dam = 0;
				}
				else if (r_ptr->flags2 & RF2_EMPTY_MIND)
				{
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags2 |= (RF2_EMPTY_MIND);
					note = _("には完全な耐性がある！", " is immune.");
					dam = 0;
				}
				else if (r_ptr->flags2 & RF2_WEIRD_MIND)
				{
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags2 |= (RF2_WEIRD_MIND);
					note = _("には耐性がある！", " resists!");
					dam /= 3;
				}
				else
				{
					note = _("は精神攻撃を食らった。", " is blasted by psionic energy.");
					note_dies = _("の精神は崩壊し、肉体は抜け殻となった。", " collapses, a mindless husk.");

					if (who > 0)
					{
						do_conf = randint0(4) + 4;
						do_stun = randint0(4) + 4;
					}
					else
					{
						do_conf = randint0(8) + 8;
						do_stun = randint0(8) + 8;
					}
					(void)set_monster_slow(g_ptr->m_idx, MON_SLOW(m_ptr) + 10);
				}
				break;
			}

			/* CAUSE_1 */
			case GF_CAUSE_1:
			{
				if (seen) obvious = TRUE;
				if (!who) msg_format(_("%sを指差して呪いをかけた。", "You point at %s and curse."), m_name);
				/* Attempt a saving throw */
				if (randint0(100 + (caster_lev / 2)) < (r_ptr->level + 35))
				{
					note = _("には効果がなかった。", " is unaffected.");
					dam = 0;
				}
				break;
			}

			/* CAUSE_2 */
			case GF_CAUSE_2:
			{
				if (seen) obvious = TRUE;
				if (!who) msg_format(_("%sを指差して恐ろしげに呪いをかけた。", "You point at %s and curse horribly."), m_name);
				/* Attempt a saving throw */
				if (randint0(100 + (caster_lev / 2)) < (r_ptr->level + 35))
				{
					note = _("には効果がなかった。", " is unaffected.");
					dam = 0;
				}
				break;
			}

			/* CAUSE_3 */
			case GF_CAUSE_3:
			{
				if (seen) obvious = TRUE;
				if (!who) msg_format(_("%sを指差し、恐ろしげに呪文を唱えた！", "You point at %s, incanting terribly!"), m_name);
				/* Attempt a saving throw */
				if (randint0(100 + (caster_lev / 2)) < (r_ptr->level + 35))
				{
					note = _("には効果がなかった。", " is unaffected.");
					dam = 0;
				}
				break;
			}

			/* CAUSE_4 */
			case GF_CAUSE_4:
			{
				if (seen) obvious = TRUE;
				if (!who) 
					msg_format(_("%sの秘孔を突いて、「お前は既に死んでいる」と叫んだ。", 
								 "You point at %s, screaming the word, 'DIE!'."), m_name);
				/* Attempt a saving throw */
				if ((randint0(100 + (caster_lev / 2)) < (r_ptr->level + 35)) && ((who <= 0) || (caster_ptr->r_idx != MON_KENSHIROU)))
				{
					note = _("には効果がなかった。", " is unaffected.");
					dam = 0;
				}
				break;
			}

			/* HAND_DOOM */
			case GF_HAND_DOOM:
			{
				if (seen) obvious = TRUE;
				if (r_ptr->flags1 & RF1_UNIQUE)
				{
					note = _("には効果がなかった。", " is unaffected.");
					dam = 0;
				}
				else
				{
					if ((who > 0) ? ((caster_lev + randint1(dam)) > (r_ptr->level + 10 + randint1(20))) :
					   (((caster_lev / 2) + randint1(dam)) > (r_ptr->level + randint1(200))))
					{
						dam = ((40 + randint1(20)) * m_ptr->hp) / 100;

						if (m_ptr->hp < dam) dam = m_ptr->hp - 1;
					}
					else
					{
						note = _("は耐性を持っている！", "resists!");
						dam = 0;
					}
				}
				break;
			}

			/* Capture monster */
			case GF_CAPTURE:
			{
				int nokori_hp;
				if ((p_ptr->inside_quest && (quest[p_ptr->inside_quest].type == QUEST_TYPE_KILL_ALL) && !is_pet(m_ptr)) ||
					(r_ptr->flags1 & (RF1_UNIQUE)) || (r_ptr->flags7 & (RF7_NAZGUL)) || (r_ptr->flags7 & (RF7_UNIQUE2)) || (r_ptr->flags1 & RF1_QUESTOR) || m_ptr->parent_m_idx)
				{
					msg_format(_("%sには効果がなかった。", "%s is unaffected."), m_name);
					skipped = TRUE;
					break;
				}

				if (is_pet(m_ptr)) nokori_hp = m_ptr->maxhp * 4L;
				else if ((p_ptr->pclass == CLASS_BEASTMASTER) && monster_living(m_ptr->r_idx))
					nokori_hp = m_ptr->maxhp * 3 / 10;
				else
					nokori_hp = m_ptr->maxhp * 3 / 20;

				if (m_ptr->hp >= nokori_hp)
				{
					msg_format(_("もっと弱らせないと。", "You need to weaken %s more."), m_name);
					skipped = TRUE;
				}
				else if (m_ptr->hp < randint0(nokori_hp))
				{
					if (m_ptr->mflag2 & MFLAG2_CHAMELEON) choose_new_monster(g_ptr->m_idx, FALSE, MON_CHAMELEON);
					msg_format(_("%sを捕えた！", "You capture %^s!"), m_name);
					cap_mon = m_ptr->r_idx;
					cap_mspeed = m_ptr->mspeed;
					cap_hp = m_ptr->hp;
					cap_maxhp = m_ptr->max_maxhp;
					cap_nickname = m_ptr->nickname; /* Quark transfer */
					if (g_ptr->m_idx == p_ptr->riding)
					{
						if (rakuba(p_ptr, -1, FALSE))
						{
							msg_format(_("地面に落とされた。", "You have fallen from %s."), m_name);
						}
					}

					delete_monster_idx(g_ptr->m_idx);

					return (TRUE);
				}
				else
				{
					msg_format(_("うまく捕まえられなかった。", "You failed to capture %s."), m_name);
					skipped = TRUE;
				}
				break;
			}

			/* Attack (Use "dam" as attack type) */
			case GF_ATTACK:
			{
				/* Return this monster's death */
				return py_attack(p_ptr, y, x, dam);
			}

			/* Sleep (Use "dam" as "power") */
			case GF_ENGETSU:
			{
				int effect = 0;
				bool done = TRUE;

				if (seen) obvious = TRUE;
				if (r_ptr->flags2 & RF2_EMPTY_MIND)
				{
					note = _("には効果がなかった。", " is unaffected.");
					dam = 0;
					skipped = TRUE;
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags2 |= (RF2_EMPTY_MIND);
					break;
				}
				if (MON_CSLEEP(m_ptr))
				{
					note = _("には効果がなかった。", " is unaffected.");
					dam = 0;
					skipped = TRUE;
					break;
				}

				if (one_in_(5)) effect = 1;
				else if (one_in_(4)) effect = 2;
				else if (one_in_(3)) effect = 3;
				else done = FALSE;

				if (effect == 1)
				{
					/* Powerful monsters can resist */
					if ((r_ptr->flags1 & RF1_UNIQUE) ||
						(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
					{
						note = _("には効果がなかった。", " is unaffected.");
						obvious = FALSE;
					}

					/* Normal monsters slow down */
					else
					{
						if (set_monster_slow(g_ptr->m_idx, MON_SLOW(m_ptr) + 50))
						{
							note = _("の動きが遅くなった。", " starts moving slower.");
						}
					}
				}

				else if (effect == 2)
				{
					do_stun = damroll((p_ptr->lev / 10) + 3 , (dam)) + 1;

					/* Attempt a saving throw */
					if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
						(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
					{
						/* Resist */
						do_stun = 0;

						/* No obvious effect */
						note = _("には効果がなかった。", " is unaffected.");
						obvious = FALSE;
					}
				}

				else if (effect == 3)
				{
					/* Attempt a saving throw */
					if ((r_ptr->flags1 & RF1_UNIQUE) ||
						(r_ptr->flags3 & RF3_NO_SLEEP) ||
						(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
					{
						/* Memorize a flag */
						if (r_ptr->flags3 & RF3_NO_SLEEP)
						{
							if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_NO_SLEEP);
						}

						/* No obvious effect */
						note = _("には効果がなかった。", " is unaffected.");
						obvious = FALSE;
					}
					else
					{
						/* Go to sleep (much) later */
						note = _("は眠り込んでしまった！", " falls asleep!");
						do_sleep = 500;
					}
				}

				if (!done)
				{
					note = _("には効果がなかった。", " is unaffected.");
				}

				/* No "real" damage */
				dam = 0;
				break;
			}

			/* GENOCIDE */
			case GF_GENOCIDE:
			{
				if (seen) obvious = TRUE;
				if (genocide_aux(g_ptr->m_idx, dam, !who, (r_ptr->level + 1) / 2, _("モンスター消滅", "Genocide One")))
				{
					if (seen_msg) msg_format(_("%sは消滅した！", "%^s disappered!"), m_name);
					chg_virtue(p_ptr, V_VITALITY, -1);
					return TRUE;
				}

				skipped = TRUE;
				break;
			}

			case GF_PHOTO:
			{
				if (!who) msg_format(_("%sを写真に撮った。", "You take a photograph of %s."), m_name);
				/* Hurt by light */
				if (r_ptr->flags3 & (RF3_HURT_LITE))
				{
					/* Obvious effect */
					if (seen) obvious = TRUE;

					/* Memorize the effects */
					if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_HURT_LITE);

					/* Special effect */
					note = _("は光に身をすくめた！", " cringes from the light!");
					note_dies = _("は光を受けてしぼんでしまった！", " shrivels away in the light!");
				}

				/* Normally no damage */
				else
				{
					/* No damage */
					dam = 0;
				}

				photo = m_ptr->r_idx;

				break;
			}


			/* blood curse */
			case GF_BLOOD_CURSE:
			{
				if (seen) obvious = TRUE;
				break;
			}

			case GF_CRUSADE:
			{
				bool success = FALSE;
				if (seen) obvious = TRUE;

				if ((r_ptr->flags3 & (RF3_GOOD)) && !p_ptr->inside_arena)
				{
					if (r_ptr->flags3 & (RF3_NO_CONF)) dam -= 50;
					if (dam < 1) dam = 1;

					/* No need to tame your pet */
					if (is_pet(m_ptr))
					{
						note = _("の動きが速くなった。", " starts moving faster.");
						(void)set_monster_fast(g_ptr->m_idx, MON_FAST(m_ptr) + 100);
						success = TRUE;
					}

					/* Attempt a saving throw */
					else if ((r_ptr->flags1 & (RF1_QUESTOR)) ||
						(r_ptr->flags1 & (RF1_UNIQUE)) ||
						(m_ptr->mflag2 & MFLAG2_NOPET) ||
						(p_ptr->cursed & TRC_AGGRAVATE) ||
						 ((r_ptr->level+10) > randint1(dam)))
					{
						/* Resist */
						if (one_in_(4)) m_ptr->mflag2 |= MFLAG2_NOPET;
					}
					else
					{
						note = _("を支配した。", " is tamed!");
						set_pet(m_ptr);
						(void)set_monster_fast(g_ptr->m_idx, MON_FAST(m_ptr) + 100);

						/* Learn about type */
						if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_GOOD);
						success = TRUE;
					}
				}

				if (!success)
				{
					if (!(r_ptr->flags3 & RF3_NO_FEAR))
					{
						do_fear = randint1(90)+10;
					}
					else if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= (RF3_NO_FEAR);
				}

				/* No "real" damage */
				dam = 0;
				break;
			}

			case GF_WOUNDS:
			{
				if (seen) obvious = TRUE;
				/* Attempt a saving throw */
				if (randint0(100 + dam) < (r_ptr->level + 50))
				{
					note = _("には効果がなかった。", " is unaffected.");
					dam = 0;
				}
				break;
			}

			/* Default */
			default:
			{
				/* Irrelevant */
				skipped = TRUE;

				/* No damage */
				dam = 0;

				break;
			}
		}
	}

	/* Absolutely no effect */
	if (skipped) return (FALSE);

	/* "Unique" monsters cannot be polymorphed */
	if (r_ptr->flags1 & (RF1_UNIQUE)) do_poly = FALSE;

	/* Quest monsters cannot be polymorphed */
	if (r_ptr->flags1 & RF1_QUESTOR) do_poly = FALSE;

	if (p_ptr->riding && (g_ptr->m_idx == p_ptr->riding)) do_poly = FALSE;

	/* "Unique" and "quest" monsters can only be "killed" by the player. */
	if (((r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) || (r_ptr->flags7 & RF7_NAZGUL)) && !p_ptr->phase_out)
	{
		if (who && (dam > m_ptr->hp)) dam = m_ptr->hp;
	}

	if (!who && slept)
	{
		if (!(r_ptr->flags3 & RF3_EVIL) || one_in_(5)) chg_virtue(p_ptr, V_COMPASSION, -1);
		if (!(r_ptr->flags3 & RF3_EVIL) || one_in_(5)) chg_virtue(p_ptr, V_HONOUR, -1);
	}

	/* Modify the damage */
	tmp = dam;
	dam = mon_damage_mod(m_ptr, dam, (bool)(typ == GF_PSY_SPEAR));
	if ((tmp > 0) && (dam == 0)) note = _("はダメージを受けていない。", " is unharmed.");

	/* Check for death */
	if (dam > m_ptr->hp)
	{
		/* Extract method of death */
		note = note_dies;
	}
	else
	{
		/* Sound and Impact resisters never stun */
		if (do_stun &&
			!(r_ptr->flagsr & (RFR_RES_SOUN | RFR_RES_WALL)) &&
			!(r_ptr->flags3 & RF3_NO_STUN))
		{
			if (seen) obvious = TRUE;

			/* Get stunned */
			if (MON_STUNNED(m_ptr))
			{
				note = _("はひどくもうろうとした。", " is more dazed.");
				tmp = MON_STUNNED(m_ptr) + (do_stun / 2);
			}
			else
			{
				note = _("はもうろうとした。", " is dazed.");
				tmp = do_stun;
			}

			/* Apply stun */
			(void)set_monster_stunned(g_ptr->m_idx, tmp);

			/* Get angry */
			get_angry = TRUE;
		}

		/* Confusion and Chaos resisters (and sleepers) never confuse */
		if (do_conf &&
			 !(r_ptr->flags3 & RF3_NO_CONF) &&
			 !(r_ptr->flagsr & RFR_EFF_RES_CHAO_MASK))
		{
			if (seen) obvious = TRUE;

			/* Already partially confused */
			if (MON_CONFUSED(m_ptr))
			{
				note = _("はさらに混乱したようだ。", " looks more confused.");
				tmp = MON_CONFUSED(m_ptr) + (do_conf / 2);
			}

			/* Was not confused */
			else
			{
				note = _("は混乱したようだ。", " looks confused.");
				tmp = do_conf;
			}

			/* Apply confusion */
			(void)set_monster_confused(g_ptr->m_idx, tmp);

			/* Get angry */
			get_angry = TRUE;
		}

		if (do_time)
		{
			if (seen) obvious = TRUE;

			if (do_time >= m_ptr->maxhp) do_time = m_ptr->maxhp - 1;

			if (do_time)
			{
				note = _("は弱くなったようだ。", " seems weakened.");
				m_ptr->maxhp -= do_time;
				if ((m_ptr->hp - dam) > m_ptr->maxhp) dam = m_ptr->hp - m_ptr->maxhp;
			}
			get_angry = TRUE;
		}

		/* Mega-Hack -- Handle "polymorph" -- monsters get a saving throw */
		if (do_poly && (randint1(90) > r_ptr->level))
		{
			if (polymorph_monster(p_ptr, y, x))
			{
				if (seen) obvious = TRUE;

				/* Monster polymorphs */
				note = _("が変身した！", " changes!");

				/* Turn off the damage */
				dam = 0;
			}
			else
			{
				/* No polymorph */
				note = _("には効果がなかった。", " is unaffected.");
			}

			/* Hack -- Get new monster */
			m_ptr = &p_ptr->current_floor_ptr->m_list[g_ptr->m_idx];

			/* Hack -- Get new race */
			r_ptr = &r_info[m_ptr->r_idx];
		}

		/* Handle "teleport" */
		if (do_dist)
		{
			if (seen) obvious = TRUE;

			note = _("が消え去った！", " disappears!");

			if (!who) chg_virtue(p_ptr, V_VALOUR, -1);

			/* Teleport */
			teleport_away(g_ptr->m_idx, do_dist,
						(!who ? TELEPORT_DEC_VALOUR : 0L) | TELEPORT_PASSIVE);

			/* Hack -- get new location */
			y = m_ptr->fy;
			x = m_ptr->fx;

			/* Hack -- get new grid */
			g_ptr = &p_ptr->current_floor_ptr->grid_array[y][x];
		}

		/* Fear */
		if (do_fear)
		{
			/* Set fear */
			(void)set_monster_monfear(g_ptr->m_idx, MON_MONFEAR(m_ptr) + do_fear);

			/* Get angry */
			get_angry = TRUE;
		}
	}

	if (typ == GF_DRAIN_MANA)
	{
		/* Drain mana does nothing */
	}

	/* If another monster did the damage, hurt the monster by hand */
	else if (who)
	{
		/* Redraw (later) if needed */
		if (p_ptr->health_who == g_ptr->m_idx) p_ptr->redraw |= (PR_HEALTH);
		if (p_ptr->riding == g_ptr->m_idx) p_ptr->redraw |= (PR_UHEALTH);

		/* Wake the monster up */
		(void)set_monster_csleep(g_ptr->m_idx, 0);

		/* Hurt the monster */
		m_ptr->hp -= dam;

		/* Dead monster */
		if (m_ptr->hp < 0)
		{
			bool sad = FALSE;

			if (is_pet(m_ptr) && !(m_ptr->ml))
				sad = TRUE;

			/* Give detailed messages if destroyed */
			if (known && note)
			{
				monster_desc(m_name, m_ptr, MD_TRUE_NAME);
				if (see_s_msg)
				{
					msg_format("%^s%s", m_name, note);
				}
				else
				{
					p_ptr->current_floor_ptr->monster_noise = TRUE;
				}
			}

			if (who > 0) monster_gain_exp(who, m_ptr->r_idx);

			/* Generate treasure, etc */
			monster_death(g_ptr->m_idx, FALSE);


			delete_monster_idx(g_ptr->m_idx);

			if (sad)
			{
				msg_print(_("少し悲しい気分がした。", "You feel sad for a moment."));
			}
		}

		/* Damaged monster */
		else
		{
			/* Give detailed messages if visible or destroyed */
			if (note && seen_msg) msg_format("%^s%s", m_name, note);

			/* Hack -- Pain message */
			else if (see_s_msg)
			{
				message_pain(g_ptr->m_idx, dam);
			}
			else
			{
				p_ptr->current_floor_ptr->monster_noise = TRUE;
			}

			/* Hack -- handle sleep */
			if (do_sleep) (void)set_monster_csleep(g_ptr->m_idx, do_sleep);
		}
	}

	else if (heal_leper)
	{
		if (seen_msg) msg_print(_("不潔な病人は病気が治った！", "The Mangy looking leper is healed!"));

		if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname)
		{
			char m2_name[MAX_NLEN];

			monster_desc(m2_name, m_ptr, MD_INDEF_VISIBLE);
			exe_write_diary(p_ptr, NIKKI_NAMED_PET, RECORD_NAMED_PET_HEAL_LEPER, m2_name);
		}

		delete_monster_idx(g_ptr->m_idx);
	}

	/* If the player did it, give him experience, check fear */
	else
	{
		bool fear = FALSE;

		/* Hurt the monster, check for fear and death */
		if (mon_take_hit(g_ptr->m_idx, dam, &fear, note_dies))
		{
			/* Dead monster */
		}

		/* Damaged monster */
		else
		{
			/* HACK - anger the monster before showing the sleep message */
			if (do_sleep) anger_monster(m_ptr);

			/* Give detailed messages if visible or destroyed */
			if (note && seen_msg)
				msg_format(_("%s%s", "%^s%s"), m_name, note);

			/* Hack -- Pain message */
			else if (known && (dam || !do_fear))
			{
				message_pain(g_ptr->m_idx, dam);
			}

			/* Anger monsters */
			if (((dam > 0) || get_angry) && !do_sleep)
				anger_monster(m_ptr);

			if ((fear || do_fear) && seen)
			{
				sound(SOUND_FLEE);
				msg_format(_("%^sは恐怖して逃げ出した！", "%^s flees in terror!"), m_name);
			}

			/* Hack -- handle sleep */
			if (do_sleep) (void)set_monster_csleep(g_ptr->m_idx, do_sleep);
		}
	}

	if ((typ == GF_BLOOD_CURSE) && one_in_(4))
	{
		blood_curse_to_enemy(who);
	}

	if (p_ptr->phase_out)
	{
		p_ptr->health_who = g_ptr->m_idx;
		p_ptr->redraw |= (PR_HEALTH);
		handle_stuff();
	}

	/* Verify this code */
	if (m_ptr->r_idx) update_monster(g_ptr->m_idx, FALSE);

	/* Redraw the monster grid */
	lite_spot(y, x);

	/* Update monster recall window */
	if ((p_ptr->monster_race_idx == m_ptr->r_idx) && (seen || !m_ptr->r_idx))
	{
		p_ptr->window |= (PW_MONSTER);
	}

	if ((dam > 0) && !is_pet(m_ptr) && !is_friendly(m_ptr))
	{
		if (!who)
		{
			if (!(flg & PROJECT_NO_HANGEKI))
			{
				set_target(m_ptr, monster_target_y, monster_target_x);
			}
		}
		else if ((who > 0) && is_pet(caster_ptr) && !player_bold(p_ptr, m_ptr->target_y, m_ptr->target_x))
		{
			set_target(m_ptr, caster_ptr->fy, caster_ptr->fx);
		}
	}

	if (p_ptr->riding && (p_ptr->riding == g_ptr->m_idx) && (dam > 0))
	{
		if (m_ptr->hp > m_ptr->maxhp/3) dam = (dam + 1) / 2;
		rakubadam_m = (dam > 200) ? 200 : dam;
	}


	if (photo)
	{
		object_type *q_ptr;
		object_type forge;
		q_ptr = &forge;

		/* Prepare to make a Blade of Chaos */
		object_prep(q_ptr, lookup_kind(TV_STATUE, SV_PHOTO));

		q_ptr->pval = photo;

		/* Mark the item as fully known */
		q_ptr->ident |= (IDENT_MENTAL);
		(void)drop_near(q_ptr, -1, p_ptr->y, p_ptr->x);
	}

	/* Track it */
	project_m_n++;
	project_m_x = x;
	project_m_y = y;

	/* Return "Anything seen?" */
	return (obvious);
}

/*!
 * @brief 汎用的なビーム/ボルト/ボール系によるプレイヤーへの効果処理 / Helper function for "project()" below.
 * @param who 魔法を発動したモンスター(0ならばプレイヤー) / Index of "source" monster (zero for "player")
 * @param who_name 効果を起こしたモンスターの名前
 * @param r 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標Y座標 / Target y location (or location to travel "towards")
 * @param x 目標X座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param typ 効果属性 / Type of damage to apply to monsters (and objects)
 * @param flg 効果フラグ
 * @param monspell 効果元のモンスター魔法ID
 * @return 何か一つでも効力があればTRUEを返す / TRUE if any "effects" of the projection were observed, else FALSE
 * @details
 * Handle a beam/bolt/ball causing damage to the player.
 * This routine takes a "source monster" (by index), a "distance", a default
 * "damage", and a "damage type".  See "project_m()" above.
 * If "rad" is non-zero, then the blast was centered elsewhere, and the damage
 * is reduced (see "project_m()" above).  This can happen if a monster breathes
 * at the player and hits a wall instead.
 * NOTE (Zangband): 'Bolt' attacks can be reflected back, so we need
 * to know if this is actually a ball or a bolt spell
 * We return "TRUE" if any "obvious" effects were observed.  XXX XXX Actually,
 * we just assume that the effects were obvious, for historical reasons.
 */
static bool project_p(MONSTER_IDX who, player_type *target_ptr, concptr who_name, int r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID typ, BIT_FLAGS flg, int monspell)
{
	int k = 0;
	DEPTH rlev = 0;

	/* Hack -- assume obvious */
	bool obvious = TRUE;

	/* Player blind-ness */
	bool blind = (target_ptr->blind ? TRUE : FALSE);

	/* Player needs a "description" (he is blind) */
	bool fuzzy = FALSE;

	/* Source monster */
	monster_type *m_ptr = NULL;

	/* Monster name (for attacks) */
	GAME_TEXT m_name[MAX_NLEN];

	/* Monster name (for damage) */
	char killer[80];

	/* Hack -- messages */
	concptr act = NULL;

	int get_damage = 0;


	/* Player is not here */
	if (!player_bold(p_ptr, y, x)) return (FALSE);

	if ((target_ptr->special_defense & NINJA_KAWARIMI) && dam && (randint0(55) < (target_ptr->lev * 3 / 5 + 20)) && who && (who != target_ptr->riding))
	{
		if (kawarimi(TRUE)) return FALSE;
	}

	/* Player cannot hurt himself */
	if (!who) return (FALSE);
	if (who == target_ptr->riding) return (FALSE);

	if ((target_ptr->reflect || ((target_ptr->special_defense & KATA_FUUJIN) && !target_ptr->blind)) && (flg & PROJECT_REFLECTABLE) && !one_in_(10))
	{
		POSITION t_y, t_x;
		int max_attempts = 10;
		sound(SOUND_REFLECT);

		if (blind)
			msg_print(_("何かが跳ね返った！", "Something bounces!"));
		else if (target_ptr->special_defense & KATA_FUUJIN)
			msg_print(_("風の如く武器を振るって弾き返した！", "The attack bounces!"));
		else
			msg_print(_("攻撃が跳ね返った！", "The attack bounces!"));


		/* Choose 'new' target */
		if (who > 0)
		{
			do
			{
				t_y = target_ptr->current_floor_ptr->m_list[who].fy - 1 + randint1(3);
				t_x = target_ptr->current_floor_ptr->m_list[who].fx - 1 + randint1(3);
				max_attempts--;
			} while (max_attempts && in_bounds2u(target_ptr->current_floor_ptr, t_y, t_x) && !projectable(target_ptr->y, target_ptr->x, t_y, t_x));

			if (max_attempts < 1)
			{
				t_y = target_ptr->current_floor_ptr->m_list[who].fy;
				t_x = target_ptr->current_floor_ptr->m_list[who].fx;
			}
		}
		else
		{
			t_y = target_ptr->y - 1 + randint1(3);
			t_x = target_ptr->x - 1 + randint1(3);
		}

		project(0, 0, t_y, t_x, dam, typ, (PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE), monspell);

		disturb(target_ptr, TRUE, TRUE);
		return TRUE;
	}

	/* Limit maximum damage */
	if (dam > 1600) dam = 1600;

	/* Reduce damage by distance */
	dam = (dam + r) / (r + 1);


	/* If the player is blind, be more descriptive */
	if (blind) fuzzy = TRUE;


	if (who > 0)
	{
		m_ptr = &target_ptr->current_floor_ptr->m_list[who];
		rlev = (((&r_info[m_ptr->r_idx])->level >= 1) ? (&r_info[m_ptr->r_idx])->level : 1);
		monster_desc(m_name, m_ptr, 0);

		/* Get the monster's real name (gotten before polymorph!) */
		strcpy(killer, who_name);
	}
	else
	{
		switch (who)
		{
		case PROJECT_WHO_UNCTRL_POWER:
			strcpy(killer, _("制御できない力の氾流", "uncontrollable power storm"));
			break;

		case PROJECT_WHO_GLASS_SHARDS:
			strcpy(killer, _("ガラスの破片", "shards of glass"));
			break;

		default:
			strcpy(killer, _("罠", "a trap"));
			break;
		}
		strcpy(m_name, killer);
	}

	/* Analyze the damage */
	switch (typ)
	{
		/* Standard damage -- hurts target_ptr->inventory_list too */
	case GF_ACID:
	{
		if (fuzzy) msg_print(_("酸で攻撃された！", "You are hit by acid!"));
		get_damage = acid_dam(dam, killer, monspell, FALSE);
		break;
	}

	/* Standard damage -- hurts target_ptr->inventory_list too */
	case GF_FIRE:
	{
		if (fuzzy) msg_print(_("火炎で攻撃された！", "You are hit by fire!"));
		get_damage = fire_dam(dam, killer, monspell, FALSE);
		break;
	}

	/* Standard damage -- hurts target_ptr->inventory_list too */
	case GF_COLD:
	{
		if (fuzzy) msg_print(_("冷気で攻撃された！", "You are hit by cold!"));
		get_damage = cold_dam(dam, killer, monspell, FALSE);
		break;
	}

	/* Standard damage -- hurts target_ptr->inventory_list too */
	case GF_ELEC:
	{
		if (fuzzy) msg_print(_("電撃で攻撃された！", "You are hit by lightning!"));
		get_damage = elec_dam(dam, killer, monspell, FALSE);
		break;
	}

	/* Standard damage -- also poisons player */
	case GF_POIS:
	{
		bool double_resist = IS_OPPOSE_POIS();
		if (fuzzy) msg_print(_("毒で攻撃された！", "You are hit by poison!"));

		if (target_ptr->resist_pois) dam = (dam + 2) / 3;
		if (double_resist) dam = (dam + 2) / 3;

		if ((!(double_resist || target_ptr->resist_pois)) && one_in_(HURT_CHANCE) && !CHECK_MULTISHADOW(target_ptr))
		{
			do_dec_stat(target_ptr, A_CON);
		}

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);

		if (!(double_resist || target_ptr->resist_pois) && !CHECK_MULTISHADOW(target_ptr))
		{
			set_poisoned(target_ptr, target_ptr->poisoned + randint0(dam) + 10);
		}
		break;
	}

	/* Standard damage -- also poisons / mutates player */
	case GF_NUKE:
	{
		bool double_resist = IS_OPPOSE_POIS();
		if (fuzzy) msg_print(_("放射能で攻撃された！", "You are hit by radiation!"));

		if (target_ptr->resist_pois) dam = (2 * dam + 2) / 5;
		if (double_resist) dam = (2 * dam + 2) / 5;
		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		if (!(double_resist || target_ptr->resist_pois) && !CHECK_MULTISHADOW(target_ptr))
		{
			set_poisoned(target_ptr, target_ptr->poisoned + randint0(dam) + 10);

			if (one_in_(5)) /* 6 */
			{
				msg_print(_("奇形的な変身を遂げた！", "You undergo a freakish metamorphosis!"));
				if (one_in_(4)) /* 4 */
					do_poly_self(target_ptr);
				else
					status_shuffle(target_ptr);
			}

			if (one_in_(6))
			{
				inven_damage(set_acid_destroy, 2);
			}
		}
		break;
	}

	/* Standard damage */
	case GF_MISSILE:
	{
		if (fuzzy) msg_print(_("何かで攻撃された！", "You are hit by something!"));
		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}

	/* Holy Orb -- Player only takes partial damage */
	case GF_HOLY_FIRE:
	{
		if (fuzzy) msg_print(_("何かで攻撃された！", "You are hit by something!"));
		if (target_ptr->align > 10)
			dam /= 2;
		else if (target_ptr->align < -10)
			dam *= 2;
		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}

	case GF_HELL_FIRE:
	{
		if (fuzzy) msg_print(_("何かで攻撃された！", "You are hit by something!"));
		if (target_ptr->align > 10)
			dam *= 2;
		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}

	/* Arrow -- XXX no dodging */
	case GF_ARROW:
	{
		if (fuzzy)
		{
			msg_print(_("何か鋭いもので攻撃された！", "You are hit by something sharp!"));
		}
		else if ((target_ptr->inventory_list[INVEN_RARM].name1 == ART_ZANTETSU) || (target_ptr->inventory_list[INVEN_LARM].name1 == ART_ZANTETSU))
		{
			msg_print(_("矢を斬り捨てた！", "You cut down the arrow!"));
			break;
		}
		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}

	/* Plasma -- XXX No resist */
	case GF_PLASMA:
	{
		if (fuzzy) msg_print(_("何かとても熱いもので攻撃された！", "You are hit by something *HOT*!"));
		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);

		if (!target_ptr->resist_sound && !CHECK_MULTISHADOW(target_ptr))
		{
			int plus_stun = (randint1((dam > 40) ? 35 : (dam * 3 / 4 + 5)));
			(void)set_stun(target_ptr, target_ptr->stun + plus_stun);
		}

		if (!(target_ptr->resist_fire || IS_OPPOSE_FIRE() || target_ptr->immune_fire))
		{
			inven_damage(set_acid_destroy, 3);
		}

		break;
	}

	/* Nether -- drain experience */
	case GF_NETHER:
	{
		if (fuzzy) msg_print(_("地獄の力で攻撃された！", "You are hit by nether forces!"));
		if (target_ptr->resist_neth)
		{
			if (!PRACE_IS_(target_ptr, RACE_SPECTRE))
			{
				dam *= 6; dam /= (randint1(4) + 7);
			}
		}
		else if (!CHECK_MULTISHADOW(target_ptr)) drain_exp(target_ptr, 200 + (target_ptr->exp / 100), 200 + (target_ptr->exp / 1000), 75);

		if (PRACE_IS_(target_ptr, RACE_SPECTRE) && !CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("気分がよくなった。", "You feel invigorated!"));
			hp_player(target_ptr, dam / 4);
			learn_spell(monspell);
		}
		else
		{
			get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		}

		break;
	}

	/* Water -- stun/confuse */
	case GF_WATER:
	{
		if (fuzzy) msg_print(_("何か湿ったもので攻撃された！", "You are hit by something wet!"));
		if (!CHECK_MULTISHADOW(target_ptr))
		{
			if (!target_ptr->resist_sound && !target_ptr->resist_water)
			{
				set_stun(target_ptr, target_ptr->stun + randint1(40));
			}
			if (!target_ptr->resist_conf && !target_ptr->resist_water)
			{
				set_confused(target_ptr, target_ptr->confused + randint1(5) + 5);
			}

			if (one_in_(5) && !target_ptr->resist_water)
			{
				inven_damage(set_cold_destroy, 3);
			}

			if (target_ptr->resist_water) get_damage /= 4;
		}

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}

	/* Chaos -- many effects */
	case GF_CHAOS:
	{
		if (fuzzy) msg_print(_("無秩序の波動で攻撃された！", "You are hit by a wave of anarchy!"));
		if (target_ptr->resist_chaos)
		{
			dam *= 6; dam /= (randint1(4) + 7);
		}

		if (!CHECK_MULTISHADOW(target_ptr))
		{
			if (!target_ptr->resist_conf)
			{
				(void)set_confused(target_ptr, target_ptr->confused + randint0(20) + 10);
			}
			if (!target_ptr->resist_chaos)
			{
				(void)set_image(target_ptr, target_ptr->image + randint1(10));
				if (one_in_(3))
				{
					msg_print(_("あなたの身体はカオスの力で捻じ曲げられた！", "Your body is twisted by chaos!"));
					(void)gain_mutation(target_ptr, 0);
				}
			}
			if (!target_ptr->resist_neth && !target_ptr->resist_chaos)
			{
				drain_exp(target_ptr, 5000 + (target_ptr->exp / 100), 500 + (target_ptr->exp / 1000), 75);
			}

			if (!target_ptr->resist_chaos || one_in_(9))
			{
				inven_damage(set_elec_destroy, 2);
				inven_damage(set_fire_destroy, 2);
			}
		}

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}

	/* Shards -- mostly cutting */
	case GF_SHARDS:
	{
		if (fuzzy) msg_print(_("何か鋭いもので攻撃された！", "You are hit by something sharp!"));
		if (target_ptr->resist_shard)
		{
			dam *= 6; dam /= (randint1(4) + 7);
		}
		else if (!CHECK_MULTISHADOW(target_ptr))
		{
			(void)set_cut(target_ptr, target_ptr->cut + dam);
		}

		if (!target_ptr->resist_shard || one_in_(13))
		{
			inven_damage(set_cold_destroy, 2);
		}

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}

	/* Sound -- mostly stunning */
	case GF_SOUND:
	{
		if (fuzzy) msg_print(_("轟音で攻撃された！", "You are hit by a loud noise!"));
		if (target_ptr->resist_sound)
		{
			dam *= 5; dam /= (randint1(4) + 7);
		}
		else if (!CHECK_MULTISHADOW(target_ptr))
		{
			int plus_stun = (randint1((dam > 90) ? 35 : (dam / 3 + 5)));
			(void)set_stun(target_ptr, target_ptr->stun + plus_stun);
		}

		if (!target_ptr->resist_sound || one_in_(13))
		{
			inven_damage(set_cold_destroy, 2);
		}

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}

	/* Pure confusion */
	case GF_CONFUSION:
	{
		if (fuzzy) msg_print(_("何か混乱するもので攻撃された！", "You are hit by something puzzling!"));
		if (target_ptr->resist_conf)
		{
			dam *= 5; dam /= (randint1(4) + 7);
		}
		else if (!CHECK_MULTISHADOW(target_ptr))
		{
			(void)set_confused(target_ptr, target_ptr->confused + randint1(20) + 10);
		}
		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}

	/* Disenchantment -- see above */
	case GF_DISENCHANT:
	{
		if (fuzzy) msg_print(_("何かさえないもので攻撃された！", "You are hit by something static!"));
		if (target_ptr->resist_disen)
		{
			dam *= 6; dam /= (randint1(4) + 7);
		}
		else if (!CHECK_MULTISHADOW(target_ptr))
		{
			(void)apply_disenchant(target_ptr, 0);
		}
		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}

	/* Nexus -- see above */
	case GF_NEXUS:
	{
		if (fuzzy) msg_print(_("何か奇妙なもので攻撃された！", "You are hit by something strange!"));
		if (target_ptr->resist_nexus)
		{
			dam *= 6; dam /= (randint1(4) + 7);
		}
		else if (!CHECK_MULTISHADOW(target_ptr))
		{
			apply_nexus(m_ptr, target_ptr);
		}
		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}

	/* Force -- mostly stun */
	case GF_FORCE:
	{
		if (fuzzy) msg_print(_("運動エネルギーで攻撃された！", "You are hit by kinetic force!"));
		if (!target_ptr->resist_sound && !CHECK_MULTISHADOW(target_ptr))
		{
			(void)set_stun(target_ptr, target_ptr->stun + randint1(20));
		}
		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}


	/* Rocket -- stun, cut */
	case GF_ROCKET:
	{
		if (fuzzy) msg_print(_("爆発があった！", "There is an explosion!"));
		if (!target_ptr->resist_sound && !CHECK_MULTISHADOW(target_ptr))
		{
			(void)set_stun(target_ptr, target_ptr->stun + randint1(20));
		}

		if (target_ptr->resist_shard)
		{
			dam /= 2;
		}
		else if (!CHECK_MULTISHADOW(target_ptr))
		{
			(void)set_cut(target_ptr, target_ptr->cut + (dam / 2));
		}

		if (!target_ptr->resist_shard || one_in_(12))
		{
			inven_damage(set_cold_destroy, 3);
		}

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}

	/* Inertia -- slowness */
	case GF_INERTIAL:
	{
		if (fuzzy) msg_print(_("何か遅いもので攻撃された！", "You are hit by something slow!"));
		if (!CHECK_MULTISHADOW(target_ptr)) (void)set_slow(target_ptr, target_ptr->slow + randint0(4) + 4, FALSE);
		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}

	/* Lite -- blinding */
	case GF_LITE:
	{
		if (fuzzy) msg_print(_("何かで攻撃された！", "You are hit by something!"));
		if (target_ptr->resist_lite)
		{
			dam *= 4; dam /= (randint1(4) + 7);
		}
		else if (!blind && !target_ptr->resist_blind && !CHECK_MULTISHADOW(target_ptr))
		{
			(void)set_blind(target_ptr, target_ptr->blind + randint1(5) + 2);
		}

		if (PRACE_IS_(target_ptr, RACE_VAMPIRE) || (target_ptr->mimic_form == MIMIC_VAMPIRE))
		{
			if (!CHECK_MULTISHADOW(target_ptr)) msg_print(_("光で肉体が焦がされた！", "The light scorches your flesh!"));
			dam *= 2;
		}
		else if (PRACE_IS_(target_ptr, RACE_S_FAIRY))
		{
			dam = dam * 4 / 3;
		}

		if (target_ptr->wraith_form) dam *= 2;
		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);

		if (target_ptr->wraith_form && !CHECK_MULTISHADOW(target_ptr))
		{
			target_ptr->wraith_form = 0;
			msg_print(_("閃光のため非物質的な影の存在でいられなくなった。",
				"The light forces you out of your incorporeal shadow form."));

			target_ptr->redraw |= (PR_MAP | PR_STATUS);
			target_ptr->update |= (PU_MONSTERS);
			target_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
		}

		break;
	}

	/* Dark -- blinding */
	case GF_DARK:
	{
		if (fuzzy) msg_print(_("何かで攻撃された！", "You are hit by something!"));
		if (target_ptr->resist_dark)
		{
			dam *= 4; dam /= (randint1(4) + 7);

			if (PRACE_IS_(target_ptr, RACE_VAMPIRE) || (target_ptr->mimic_form == MIMIC_VAMPIRE) || target_ptr->wraith_form) dam = 0;
		}
		else if (!blind && !target_ptr->resist_blind && !CHECK_MULTISHADOW(target_ptr))
		{
			(void)set_blind(target_ptr, target_ptr->blind + randint1(5) + 2);
		}
		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}

	/* Time -- bolt fewer effects XXX */
	case GF_TIME:
	{
		if (fuzzy) msg_print(_("過去からの衝撃に攻撃された！", "You are hit by a blast from the past!"));
		if (target_ptr->resist_time)
		{
			dam *= 4;
			dam /= (randint1(4) + 7);
			msg_print(_("時間が通り過ぎていく気がする。", "You feel as if time is passing you by."));
		}
		else if (!CHECK_MULTISHADOW(target_ptr))
		{
			switch (randint1(10))
			{
			case 1: case 2: case 3: case 4: case 5:
			{
				if (target_ptr->prace == RACE_ANDROID) break;
				msg_print(_("人生が逆戻りした気がする。", "You feel life has clocked back."));
				lose_exp(target_ptr, 100 + (target_ptr->exp / 100) * MON_DRAIN_LIFE);
				break;
			}

			case 6: case 7: case 8: case 9:
			{
				switch (randint1(6))
				{
				case 1: k = A_STR; act = _("強く", "strong"); break;
				case 2: k = A_INT; act = _("聡明で", "bright"); break;
				case 3: k = A_WIS; act = _("賢明で", "wise"); break;
				case 4: k = A_DEX; act = _("器用で", "agile"); break;
				case 5: k = A_CON; act = _("健康で", "hale"); break;
				case 6: k = A_CHR; act = _("美しく", "beautiful"); break;
				}

				msg_format(_("あなたは以前ほど%sなくなってしまった...。",
					"You're not as %s as you used to be..."), act);

				target_ptr->stat_cur[k] = (target_ptr->stat_cur[k] * 3) / 4;
				if (target_ptr->stat_cur[k] < 3) target_ptr->stat_cur[k] = 3;
				target_ptr->update |= (PU_BONUS);
				break;
			}

			case 10:
			{
				msg_print(_("あなたは以前ほど力強くなくなってしまった...。",
					"You're not as powerful as you used to be..."));

				for (k = 0; k < A_MAX; k++)
				{
					target_ptr->stat_cur[k] = (target_ptr->stat_cur[k] * 7) / 8;
					if (target_ptr->stat_cur[k] < 3) target_ptr->stat_cur[k] = 3;
				}
				target_ptr->update |= (PU_BONUS);
				break;
			}
			}
		}

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}

	/* Gravity -- stun plus slowness plus teleport */
	case GF_GRAVITY:
	{
		if (fuzzy) msg_print(_("何か重いもので攻撃された！", "You are hit by something heavy!"));
		msg_print(_("周辺の重力がゆがんだ。", "Gravity warps around you."));

		if (!CHECK_MULTISHADOW(target_ptr))
		{
			teleport_player(5, TELEPORT_PASSIVE);
			if (!target_ptr->levitation)
				(void)set_slow(target_ptr, target_ptr->slow + randint0(4) + 4, FALSE);
			if (!(target_ptr->resist_sound || target_ptr->levitation))
			{
				int plus_stun = (randint1((dam > 90) ? 35 : (dam / 3 + 5)));
				(void)set_stun(target_ptr, target_ptr->stun + plus_stun);
			}
		}
		if (target_ptr->levitation)
		{
			dam = (dam * 2) / 3;
		}

		if (!target_ptr->levitation || one_in_(13))
		{
			inven_damage(set_cold_destroy, 2);
		}

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}

	/* Standard damage */
	case GF_DISINTEGRATE:
	{
		if (fuzzy) msg_print(_("純粋なエネルギーで攻撃された！", "You are hit by pure energy!"));

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}

	case GF_OLD_HEAL:
	{
		if (fuzzy) msg_print(_("何らかの攻撃によって気分がよくなった。", "You are hit by something invigorating!"));

		(void)hp_player(target_ptr, dam);
		dam = 0;
		break;
	}

	case GF_OLD_SPEED:
	{
		if (fuzzy) msg_print(_("何かで攻撃された！", "You are hit by something!"));
		(void)set_fast(target_ptr, target_ptr->fast + randint1(5), FALSE);
		dam = 0;
		break;
	}

	case GF_OLD_SLOW:
	{
		if (fuzzy) msg_print(_("何か遅いもので攻撃された！", "You are hit by something slow!"));
		(void)set_slow(target_ptr, target_ptr->slow + randint0(4) + 4, FALSE);
		break;
	}

	case GF_OLD_SLEEP:
	{
		if (target_ptr->free_act)  break;
		if (fuzzy) msg_print(_("眠ってしまった！", "You fall asleep!"));

		if (ironman_nightmare)
		{
			msg_print(_("恐ろしい光景が頭に浮かんできた。", "A horrible vision enters your mind."));
			/* Have some nightmares */
			sanity_blast(target_ptr, NULL, FALSE);
		}

		set_paralyzed(target_ptr, target_ptr->paralyzed + dam);
		dam = 0;
		break;
	}

	/* Pure damage */
	case GF_MANA:
	case GF_SEEKER:
	case GF_SUPER_RAY:
	{
		if (fuzzy) msg_print(_("魔法のオーラで攻撃された！", "You are hit by an aura of magic!"));
		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		break;
	}

	/* Pure damage */
	case GF_PSY_SPEAR:
	{
		if (fuzzy) msg_print(_("エネルギーの塊で攻撃された！", "You are hit by an energy!"));
		get_damage = take_hit(target_ptr, DAMAGE_FORCE, dam, killer, monspell);
		break;
	}

	/* Pure damage */
	case GF_METEOR:
	{
		if (fuzzy) msg_print(_("何かが空からあなたの頭上に落ちてきた！", "Something falls from the sky on you!"));

		get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		if (!target_ptr->resist_shard || one_in_(13))
		{
			if (!target_ptr->immune_fire) inven_damage(set_fire_destroy, 2);
			inven_damage(set_cold_destroy, 2);
		}

		break;
	}

	/* Ice -- cold plus stun plus cuts */
	case GF_ICE:
	{
		if (fuzzy) msg_print(_("何か鋭く冷たいもので攻撃された！", "You are hit by something sharp and cold!"));
		get_damage = cold_dam(dam, killer, monspell, FALSE);
		if (!CHECK_MULTISHADOW(target_ptr))
		{
			if (!target_ptr->resist_shard)
			{
				(void)set_cut(target_ptr, target_ptr->cut + damroll(5, 8));
			}
			if (!target_ptr->resist_sound)
			{
				(void)set_stun(target_ptr, target_ptr->stun + randint1(15));
			}

			if ((!(target_ptr->resist_cold || IS_OPPOSE_COLD())) || one_in_(12))
			{
				if (!target_ptr->immune_cold) inven_damage(set_cold_destroy, 3);
			}
		}

		break;
	}

	/* Death Ray */
	case GF_DEATH_RAY:
	{
		if (fuzzy) msg_print(_("何か非常に冷たいもので攻撃された！", "You are hit by something extremely cold!"));

		if (target_ptr->mimic_form)
		{
			if (!(mimic_info[target_ptr->mimic_form].MIMIC_FLAGS & MIMIC_IS_NONLIVING))
				get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		}
		else
		{

			switch (target_ptr->prace)
			{
				/* Some races are immune */
			case RACE_GOLEM:
			case RACE_SKELETON:
			case RACE_ZOMBIE:
			case RACE_VAMPIRE:
			case RACE_DEMON:
			case RACE_SPECTRE:
			{
				dam = 0;
				break;
			}
			/* Hurt a lot */
			default:
			{
				get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
				break;
			}
			}
		}

		break;
	}

	/* Drain mana */
	case GF_DRAIN_MANA:
	{
		if (CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("攻撃は幻影に命中し、あなたには届かなかった。", "The attack hits Shadow, you are unharmed!"));
		}
		else if (target_ptr->csp)
		{
			/* Basic message */
			if (who > 0)
				msg_format(_("%^sに精神エネルギーを吸い取られてしまった！", "%^s draws psychic energy from you!"), m_name);
			else
				msg_print(_("精神エネルギーを吸い取られてしまった！", "Your psychic energy is drawn!"));

			/* Full drain */
			if (dam >= target_ptr->csp)
			{
				dam = target_ptr->csp;
				target_ptr->csp = 0;
				target_ptr->csp_frac = 0;
			}

			/* Partial drain */
			else
			{
				target_ptr->csp -= dam;
			}

			learn_spell(monspell);
			target_ptr->redraw |= (PR_MANA);
			target_ptr->window |= (PW_PLAYER | PW_SPELL);

			if (who > 0)
			{
				/* Heal the monster */
				if (m_ptr->hp < m_ptr->maxhp)
				{
					/* Heal */
					m_ptr->hp += dam;
					if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

					/* Redraw (later) if needed */
					if (target_ptr->health_who == who) target_ptr->redraw |= (PR_HEALTH);
					if (target_ptr->riding == who) target_ptr->redraw |= (PR_UHEALTH);

					/* Special message */
					if (m_ptr->ml)
					{
						msg_format(_("%^sは気分が良さそうだ。", "%^s appears healthier."), m_name);
					}
				}
			}
		}

		dam = 0;
		break;
	}

	/* Mind blast */
	case GF_MIND_BLAST:
	{
		if ((randint0(100 + rlev / 2) < MAX(5, target_ptr->skill_sav)) && !CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
			learn_spell(monspell);
		}
		else
		{
			if (!CHECK_MULTISHADOW(target_ptr))
			{
				msg_print(_("霊的エネルギーで精神が攻撃された。", "Your mind is blasted by psyonic energy."));

				if (!target_ptr->resist_conf)
				{
					(void)set_confused(target_ptr, target_ptr->confused + randint0(4) + 4);
				}

				if (!target_ptr->resist_chaos && one_in_(3))
				{
					(void)set_image(target_ptr, target_ptr->image + randint0(250) + 150);
				}

				target_ptr->csp -= 50;
				if (target_ptr->csp < 0)
				{
					target_ptr->csp = 0;
					target_ptr->csp_frac = 0;
				}
				target_ptr->redraw |= PR_MANA;
			}

			get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		}
		break;
	}

	/* Brain smash */
	case GF_BRAIN_SMASH:
	{
		if ((randint0(100 + rlev / 2) < MAX(5, target_ptr->skill_sav)) && !CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
			learn_spell(monspell);
		}
		else
		{
			if (!CHECK_MULTISHADOW(target_ptr))
			{
				msg_print(_("霊的エネルギーで精神が攻撃された。", "Your mind is blasted by psyonic energy."));

				target_ptr->csp -= 100;
				if (target_ptr->csp < 0)
				{
					target_ptr->csp = 0;
					target_ptr->csp_frac = 0;
				}
				target_ptr->redraw |= PR_MANA;
			}

			get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
			if (!CHECK_MULTISHADOW(target_ptr))
			{
				if (!target_ptr->resist_blind)
				{
					(void)set_blind(target_ptr, target_ptr->blind + 8 + randint0(8));
				}
				if (!target_ptr->resist_conf)
				{
					(void)set_confused(target_ptr, target_ptr->confused + randint0(4) + 4);
				}
				if (!target_ptr->free_act)
				{
					(void)set_paralyzed(target_ptr, target_ptr->paralyzed + randint0(4) + 4);
				}
				(void)set_slow(target_ptr, target_ptr->slow + randint0(4) + 4, FALSE);

				while (randint0(100 + rlev / 2) > (MAX(5, target_ptr->skill_sav)))
					(void)do_dec_stat(target_ptr, A_INT);
				while (randint0(100 + rlev / 2) > (MAX(5, target_ptr->skill_sav)))
					(void)do_dec_stat(target_ptr, A_WIS);

				if (!target_ptr->resist_chaos)
				{
					(void)set_image(target_ptr, target_ptr->image + randint0(250) + 150);
				}
			}
		}
		break;
	}

	/* cause 1 */
	case GF_CAUSE_1:
	{
		if ((randint0(100 + rlev / 2) < target_ptr->skill_sav) && !CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
			learn_spell(monspell);
		}
		else
		{
			if (!CHECK_MULTISHADOW(target_ptr)) curse_equipment(15, 0);
			get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		}
		break;
	}

	/* cause 2 */
	case GF_CAUSE_2:
	{
		if ((randint0(100 + rlev / 2) < target_ptr->skill_sav) && !CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
			learn_spell(monspell);
		}
		else
		{
			if (!CHECK_MULTISHADOW(target_ptr)) curse_equipment(25, MIN(rlev / 2 - 15, 5));
			get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		}
		break;
	}

	/* cause 3 */
	case GF_CAUSE_3:
	{
		if ((randint0(100 + rlev / 2) < target_ptr->skill_sav) && !CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
			learn_spell(monspell);
		}
		else
		{
			if (!CHECK_MULTISHADOW(target_ptr)) curse_equipment(33, MIN(rlev / 2 - 15, 15));
			get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
		}
		break;
	}

	/* cause 4 */
	case GF_CAUSE_4:
	{
		if ((randint0(100 + rlev / 2) < target_ptr->skill_sav) && !(m_ptr->r_idx == MON_KENSHIROU) && !CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("しかし秘孔を跳ね返した！", "You resist the effects!"));
			learn_spell(monspell);
		}
		else
		{
			get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, killer, monspell);
			if (!CHECK_MULTISHADOW(target_ptr)) (void)set_cut(target_ptr, target_ptr->cut + damroll(10, 10));
		}
		break;
	}

	/* Hand of Doom */
	case GF_HAND_DOOM:
	{
		if ((randint0(100 + rlev / 2) < target_ptr->skill_sav) && !CHECK_MULTISHADOW(target_ptr))
		{
			msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
			learn_spell(monspell);
		}
		else
		{
			if (!CHECK_MULTISHADOW(target_ptr))
			{
				msg_print(_("あなたは命が薄まっていくように感じた！", "You feel your life fade away!"));
				curse_equipment(40, 20);
			}

			get_damage = take_hit(target_ptr, DAMAGE_ATTACK, dam, m_name, monspell);

			if (target_ptr->chp < 1) target_ptr->chp = 1;
		}
		break;
	}

	/* Default */
	default:
	{
		/* No damage */
		dam = 0;

		break;
	}
	}

	/* Hex - revenge damage stored */
	revenge_store(get_damage);

	if ((target_ptr->tim_eyeeye || hex_spelling(HEX_EYE_FOR_EYE))
		&& (get_damage > 0) && !target_ptr->is_dead && (who > 0))
	{
		GAME_TEXT m_name_self[80];

		/* hisself */
		monster_desc(m_name_self, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE | MD_OBJECTIVE);

		msg_format(_("攻撃が%s自身を傷つけた！", "The attack of %s has wounded %s!"), m_name, m_name_self);
		project(0, 0, m_ptr->fy, m_ptr->fx, get_damage, GF_MISSILE, PROJECT_KILL, -1);
		if (target_ptr->tim_eyeeye) set_tim_eyeeye(target_ptr, target_ptr->tim_eyeeye - 5, TRUE);
	}

	if (target_ptr->riding && dam > 0)
	{
		rakubadam_p = (dam > 200) ? 200 : dam;
	}


	disturb(target_ptr, TRUE, TRUE);


	if ((target_ptr->special_defense & NINJA_KAWARIMI) && dam && who && (who != target_ptr->riding))
	{
		(void)kawarimi(FALSE);
	}

	/* Return "Anything seen?" */
	return (obvious);
}


/*
 * Find the distance from (x, y) to a line.
 */
POSITION dist_to_line(POSITION y, POSITION x, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
	/* Vector from (x, y) to (x1, y1) */
	POSITION py = y1 - y;
	POSITION px = x1 - x;

	/* Normal vector */
	POSITION ny = x2 - x1;
	POSITION nx = y1 - y2;

	/* Length of N */
	POSITION pd = distance(y1, x1, y, x);
	POSITION nd = distance(y1, x1, y2, x2);

	if (pd > nd) return distance(y, x, y2, x2);

	/* Component of P on N */
	nd = ((nd) ? ((py * ny + px * nx) / nd) : 0);

	/* Absolute value */
	return((nd >= 0) ? nd : 0 - nd);
}



/*
 * 
 * Modified version of los() for calculation of disintegration balls.
 * Disintegration effects are stopped by permanent walls.
 */
bool in_disintegration_range(POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
	POSITION dx, dy; /* Delta */
	POSITION ax, ay; /* Absolute */
	POSITION sx, sy; /* Signs */
	POSITION qx, qy; /* Fractions */
	POSITION tx, ty; /* Scanners */
	POSITION f1, f2; /* Scale factors */
	POSITION m; /* Slope, or 1/Slope, of LOS */

	/* Extract the offset */
	dy = y2 - y1;
	dx = x2 - x1;

	/* Extract the absolute offset */
	ay = ABS(dy);
	ax = ABS(dx);

	/* Handle adjacent (or identical) grids */
	if ((ax < 2) && (ay < 2)) return (TRUE);

	/* Paranoia -- require "safe" origin */
	/* if (!in_bounds(p_ptr->current_floor_ptr, y1, x1)) return (FALSE); */

	/* Directly South/North */
	if (!dx)
	{
		/* South -- check for walls */
		if (dy > 0)
		{
			for (ty = y1 + 1; ty < y2; ty++)
			{
				if (cave_stop_disintegration(ty, x1)) return (FALSE);
			}
		}

		/* North -- check for walls */
		else
		{
			for (ty = y1 - 1; ty > y2; ty--)
			{
				if (cave_stop_disintegration(ty, x1)) return (FALSE);
			}
		}

		/* Assume los */
		return (TRUE);
	}

	/* Directly East/West */
	if (!dy)
	{
		/* East -- check for walls */
		if (dx > 0)
		{
			for (tx = x1 + 1; tx < x2; tx++)
			{
				if (cave_stop_disintegration(y1, tx)) return (FALSE);
			}
		}

		/* West -- check for walls */
		else
		{
			for (tx = x1 - 1; tx > x2; tx--)
			{
				if (cave_stop_disintegration(y1, tx)) return (FALSE);
			}
		}

		/* Assume los */
		return (TRUE);
	}

	/* Extract some signs */
	sx = (dx < 0) ? -1 : 1;
	sy = (dy < 0) ? -1 : 1;

	/* Vertical "knights" */
	if (ax == 1)
	{
		if (ay == 2)
		{
			if (!cave_stop_disintegration(y1 + sy, x1)) return (TRUE);
		}
	}

	/* Horizontal "knights" */
	else if (ay == 1)
	{
		if (ax == 2)
		{
			if (!cave_stop_disintegration(y1, x1 + sx)) return (TRUE);
		}
	}

	/* Calculate scale factor div 2 */
	f2 = (ax * ay);

	/* Calculate scale factor */
	f1 = f2 << 1;


	/* Travel horizontally */
	if (ax >= ay)
	{
		/* Let m = dy / dx * 2 * (dy * dx) = 2 * dy * dy */
		qy = ay * ay;
		m = qy << 1;

		tx = x1 + sx;

		/* Consider the special case where slope == 1. */
		if (qy == f2)
		{
			ty = y1 + sy;
			qy -= f1;
		}
		else
		{
			ty = y1;
		}

		/* Note (below) the case (qy == f2), where */
		/* the LOS exactly meets the corner of a tile. */
		while (x2 - tx)
		{
			if (cave_stop_disintegration(ty, tx)) return (FALSE);

			qy += m;

			if (qy < f2)
			{
				tx += sx;
			}
			else if (qy > f2)
			{
				ty += sy;
				if (cave_stop_disintegration(ty, tx)) return (FALSE);
				qy -= f1;
				tx += sx;
			}
			else
			{
				ty += sy;
				qy -= f1;
				tx += sx;
			}
		}
	}

	/* Travel vertically */
	else
	{
		/* Let m = dx / dy * 2 * (dx * dy) = 2 * dx * dx */
		qx = ax * ax;
		m = qx << 1;

		ty = y1 + sy;

		if (qx == f2)
		{
			tx = x1 + sx;
			qx -= f1;
		}
		else
		{
			tx = x1;
		}

		/* Note (below) the case (qx == f2), where */
		/* the LOS exactly meets the corner of a tile. */
		while (y2 - ty)
		{
			if (cave_stop_disintegration(ty, tx)) return (FALSE);

			qx += m;

			if (qx < f2)
			{
				ty += sy;
			}
			else if (qx > f2)
			{
				tx += sx;
				if (cave_stop_disintegration(ty, tx)) return (FALSE);
				qx -= f1;
				ty += sy;
			}
			else
			{
				tx += sx;
				qx -= f1;
				ty += sy;
			}
		}
	}

	/* Assume los */
	return (TRUE);
}


/*
 * breath shape
 */
void breath_shape(u16b *path_g, int dist, int *pgrids, POSITION *gx, POSITION *gy, POSITION *gm, POSITION *pgm_rad, POSITION rad, POSITION y1, POSITION x1, POSITION y2, POSITION x2, EFFECT_ID typ)
{
	POSITION by = y1;
	POSITION bx = x1;
	int brad = 0;
	int brev = rad * rad / dist;
	int bdis = 0;
	int cdis;
	int path_n = 0;
	int mdis = distance(y1, x1, y2, x2) + rad;

	while (bdis <= mdis)
	{
		POSITION x, y;

		if ((0 < dist) && (path_n < dist))
		{
			POSITION ny = GRID_Y(path_g[path_n]);
			POSITION nx = GRID_X(path_g[path_n]);
			POSITION nd = distance(ny, nx, y1, x1);

			/* Get next base point */
			if (bdis >= nd)
			{
				by = ny;
				bx = nx;
				path_n++;
			}
		}

		/* Travel from center outward */
		for (cdis = 0; cdis <= brad; cdis++)
		{
			/* Scan the maximal blast area of radius "cdis" */
			for (y = by - cdis; y <= by + cdis; y++)
			{
				for (x = bx - cdis; x <= bx + cdis; x++)
				{
					/* Ignore "illegal" locations */
					if (!in_bounds(p_ptr->current_floor_ptr, y, x)) continue;

					/* Enforce a circular "ripple" */
					if (distance(y1, x1, y, x) != bdis) continue;

					/* Enforce an arc */
					if (distance(by, bx, y, x) != cdis) continue;

					switch (typ)
					{
					case GF_LITE:
					case GF_LITE_WEAK:
						/* Lights are stopped by opaque terrains */
						if (!los(by, bx, y, x)) continue;
						break;
					case GF_DISINTEGRATE:
						/* Disintegration are stopped only by perma-walls */
						if (!in_disintegration_range(by, bx, y, x)) continue;
						break;
					default:
						/* Ball explosions are stopped by walls */
						if (!projectable(by, bx, y, x)) continue;
						break;
					}

					/* Save this grid */
					gy[*pgrids] = y;
					gx[*pgrids] = x;
					(*pgrids)++;
				}
			}
		}

		/* Encode some more "radius" info */
		gm[bdis + 1] = *pgrids;

		/* Increase the size */
		brad = rad * (path_n + brev) / (dist + brev);

		/* Find the next ripple */
		bdis++;
	}

	/* Store the effect size */
	*pgm_rad = bdis;
}


/*!
 * @brief 汎用的なビーム/ボルト/ボール系処理のルーチン Generic "beam"/"bolt"/"ball" projection routine.
 * @param who 魔法を発動したモンスター(0ならばプレイヤー) / Index of "source" monster (zero for "player")
 * @param rad 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標Y座標 / Target y location (or location to travel "towards")
 * @param x 目標X座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param typ 効果属性 / Type of damage to apply to monsters (and objects)
 * @param flg 効果フラグ / Extra bit flags (see PROJECT_xxxx)
 * @param monspell 効果元のモンスター魔法ID
 * @return 何か一つでも効力があればTRUEを返す / TRUE if any "effects" of the projection were observed, else FALSE
 * @details
 * <pre>
 * Allows a monster (or player) to project a beam/bolt/ball of a given kind
 * towards a given location (optionally passing over the heads of interposing
 * monsters), and have it do a given amount of damage to the monsters (and
 * optionally objects) within the given radius of the final location.
 *
 * A "bolt" travels from source to target and affects only the target grid.
 * A "beam" travels from source to target, affecting all grids passed through.
 * A "ball" travels from source to the target, exploding at the target, and
 *   affecting everything within the given radius of the target location.
 *
 * Traditionally, a "bolt" does not affect anything on the ground, and does
 * not pass over the heads of interposing monsters, much like a traditional
 * missile, and will "stop" abruptly at the "target" even if no monster is
 * positioned there, while a "ball", on the other hand, passes over the heads
 * of monsters between the source and target, and affects everything except
 * the source monster which lies within the final radius, while a "beam"
 * affects every monster between the source and target, except for the casting
 * monster (or player), and rarely affects things on the ground.
 *
 * Two special flags allow us to use this function in special ways, the
 * "PROJECT_HIDE" flag allows us to perform "invisible" projections, while
 * the "PROJECT_JUMP" flag allows us to affect a specific grid, without
 * actually projecting from the source monster (or player).
 *
 * The player will only get "experience" for monsters killed by himself
 * Unique monsters can only be destroyed by attacks from the player
 *
 * Only 256 grids can be affected per projection, limiting the effective
 * "radius" of standard ball attacks to nine units (diameter nineteen).
 *
 * One can project in a given "direction" by combining PROJECT_THRU with small
 * offsets to the initial location (see "line_spell()"), or by calculating
 * "virtual targets" far away from the player.
 *
 * One can also use PROJECT_THRU to send a beam/bolt along an angled path,
 * continuing until it actually hits somethings (useful for "stone to mud").
 *
 * Bolts and Beams explode INSIDE walls, so that they can destroy doors.
 *
 * Balls must explode BEFORE hitting walls, or they would affect monsters
 * on both sides of a wall.  Some bug reports indicate that this is still
 * happening in 2.7.8 for Windows, though it appears to be impossible.
 *
 * We "pre-calculate" the blast area only in part for efficiency.
 * More importantly, this lets us do "explosions" from the "inside" out.
 * This results in a more logical distribution of "blast" treasure.
 * It also produces a better (in my opinion) animation of the explosion.
 * It could be (but is not) used to have the treasure dropped by monsters
 * in the middle of the explosion fall "outwards", and then be damaged by
 * the blast as it spreads outwards towards the treasure drop location.
 *
 * Walls and doors are included in the blast area, so that they can be
 * "burned" or "melted" in later versions.
 *
 * This algorithm is intended to maximize simplicity, not necessarily
 * efficiency, since this function is not a bottleneck in the code.
 *
 * We apply the blast effect from ground zero outwards, in several passes,
 * first affecting features, then objects, then monsters, then the player.
 * This allows walls to be removed before checking the object or monster
 * in the wall, and protects objects which are dropped by monsters killed
 * in the blast, and allows the player to see all affects before he is
 * killed or teleported away.  The semantics of this method are open to
 * various interpretations, but they seem to work well in practice.
 *
 * We process the blast area from ground-zero outwards to allow for better
 * distribution of treasure dropped by monsters, and because it provides a
 * pleasing visual effect at low cost.
 *
 * Note that the damage done by "ball" explosions decreases with distance.
 * This decrease is rapid, grids at radius "dist" take "1/dist" damage.
 *
 * Notice the "napalm" effect of "beam" weapons.  First they "project" to
 * the target, and then the damage "flows" along this beam of destruction.
 * The damage at every grid is the same as at the "center" of a "ball"
 * explosion, since the "beam" grids are treated as if they ARE at the
 * center of a "ball" explosion.
 *
 * Currently, specifying "beam" plus "ball" means that locations which are
 * covered by the initial "beam", and also covered by the final "ball", except
 * for the final grid (the epicenter of the ball), will be "hit twice", once
 * by the initial beam, and once by the exploding ball.  For the grid right
 * next to the epicenter, this results in 150% damage being done.  The center
 * does not have this problem, for the same reason the final grid in a "beam"
 * plus "bolt" does not -- it is explicitly removed.  Simply removing "beam"
 * grids which are covered by the "ball" will NOT work, as then they will
 * receive LESS damage than they should.  Do not combine "beam" with "ball".
 *
 * The array "gy[],gx[]" with current size "grids" is used to hold the
 * collected locations of all grids in the "blast area" plus "beam path".
 *
 * Note the rather complex usage of the "gm[]" array.  First, gm[0] is always
 * zero.  Second, for N>1, gm[N] is always the index (in gy[],gx[]) of the
 * first blast grid (see above) with radius "N" from the blast center.  Note
 * that only the first gm[1] grids in the blast area thus take full damage.
 * Also, note that gm[rad+1] is always equal to "grids", which is the total
 * number of blast grids.
 *
 * Note that once the projection is complete, (y2,x2) holds the final location
 * of bolts/beams, and the "epicenter" of balls.
 *
 * Note also that "rad" specifies the "inclusive" radius of projection blast,
 * so that a "rad" of "one" actually covers 5 or 9 grids, depending on the
 * implementation of the "distance" function.  Also, a bolt can be properly
 * viewed as a "ball" with a "rad" of "zero".
 *
 * Note that if no "target" is reached before the beam/bolt/ball travels the
 * maximum distance allowed (MAX_RANGE), no "blast" will be induced.  This
 * may be relevant even for bolts, since they have a "1x1" mini-blast.
 *
 * Note that for consistency, we "pretend" that the bolt actually takes "time"
 * to move from point A to point B, even if the player cannot see part of the
 * projection path.  Note that in general, the player will *always* see part
 * of the path, since it either starts at the player or ends on the player.
 *
 * Hack -- we assume that every "projection" is "self-illuminating".
 *
 * Hack -- when only a single monster is affected, we automatically track
 * (and recall) that monster, unless "PROJECT_JUMP" is used.
 *
 * Note that all projections now "explode" at their final destination, even
 * if they were being projected at a more distant destination.  This means
 * that "ball" spells will *always* explode.
 *
 * Note that we must call "handle_stuff()" after affecting terrain features
 * in the blast radius, in case the "illumination" of the grid was changed,
 * and "update_view()" and "update_monsters()" need to be called.
 * </pre>
 */
bool project(MONSTER_IDX who, POSITION rad, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID typ, BIT_FLAGS flg, int monspell)
{
	int i, t, dist;

	POSITION y1, x1;
	POSITION y2, x2;
	POSITION by, bx;

	int dist_hack = 0;

	POSITION y_saver, x_saver; /* For reflecting monsters */

	int msec = delay_factor * delay_factor * delay_factor;

	/* Assume the player sees nothing */
	bool notice = FALSE;

	/* Assume the player has seen nothing */
	bool visual = FALSE;

	/* Assume the player has seen no blast grids */
	bool drawn = FALSE;

	/* Assume to be a normal ball spell */
	bool breath = FALSE;

	/* Is the player blind? */
	bool blind = (p_ptr->blind ? TRUE : FALSE);

	bool old_hide = FALSE;

	/* Number of grids in the "path" */
	int path_n = 0;

	/* Actual grids in the "path" */
	u16b path_g[512];

	/* Number of grids in the "blast area" (including the "beam" path) */
	int grids = 0;

	/* Coordinates of the affected grids */
	POSITION gx[1024], gy[1024];

	/* Encoded "radius" info (see above) */
	POSITION gm[32];

	/* Actual radius encoded in gm[] */
	POSITION gm_rad = rad;

	bool jump = FALSE;

	/* Attacker's name (prepared before polymorph)*/
	GAME_TEXT who_name[MAX_NLEN];

	/* Can the player see the source of this effect? */
	bool see_s_msg = TRUE;

	/* Initialize by null string */
	who_name[0] = '\0';

	rakubadam_p = 0;
	rakubadam_m = 0;

	/* Default target of monsterspell is player */
	monster_target_y = p_ptr->y;
	monster_target_x = p_ptr->x;

	/* Hack -- Jump to target */
	if (flg & (PROJECT_JUMP))
	{
		x1 = x;
		y1 = y;

		/* Clear the flag */
		flg &= ~(PROJECT_JUMP);

		jump = TRUE;
	}

	/* Start at player */
	else if (who <= 0)
	{
		x1 = p_ptr->x;
		y1 = p_ptr->y;
	}

	/* Start at monster */
	else if (who > 0)
	{
		x1 = p_ptr->current_floor_ptr->m_list[who].fx;
		y1 = p_ptr->current_floor_ptr->m_list[who].fy;
		monster_desc(who_name, &p_ptr->current_floor_ptr->m_list[who], MD_WRONGDOER_NAME);
	}

	else
	{
		x1 = x;
		y1 = y;
	}

	y_saver = y1;
	x_saver = x1;

	/* Default "destination" */
	y2 = y;
	x2 = x;


	/* Hack -- verify stuff */
	if (flg & (PROJECT_THRU))
	{
		if ((x1 == x2) && (y1 == y2))
		{
			flg &= ~(PROJECT_THRU);
		}
	}

	/* Handle a breath attack */
	if (rad < 0)
	{
		rad = 0 - rad;
		breath = TRUE;
		if (flg & PROJECT_HIDE) old_hide = TRUE;
		flg |= PROJECT_HIDE;
	}


	/* Hack -- Assume there will be no blast (max radius 32) */
	for (dist = 0; dist < 32; dist++) gm[dist] = 0;


	/* Initial grid */
	y = y1;
	x = x1;
	dist = 0;

	/* Collect beam grids */
	if (flg & (PROJECT_BEAM))
	{
		gy[grids] = y;
		gx[grids] = x;
		grids++;
	}

	switch (typ)
	{
	case GF_LITE:
	case GF_LITE_WEAK:
		if (breath || (flg & PROJECT_BEAM)) flg |= (PROJECT_LOS);
		break;
	case GF_DISINTEGRATE:
		flg |= (PROJECT_GRID);
		if (breath || (flg & PROJECT_BEAM)) flg |= (PROJECT_DISI);
		break;
	}

	/* Calculate the projection path */

	path_n = project_path(path_g, (project_length ? project_length : MAX_RANGE), y1, x1, y2, x2, flg);
	handle_stuff();

	/* Giga-Hack SEEKER & SUPER_RAY */

	if( typ == GF_SEEKER )
	{
		int j;
		int last_i=0;

		/* Mega-Hack */
		project_m_n = 0;
		project_m_x = 0;
		project_m_y = 0;

		for (i = 0; i < path_n; ++i)
		{
			POSITION oy = y;
			POSITION ox = x;

			POSITION ny = GRID_Y(path_g[i]);
			POSITION nx = GRID_X(path_g[i]);

			/* Advance */
			y = ny;
			x = nx;

			gy[grids] = y;
			gx[grids] = x;
			grids++;


			/* Only do visuals if requested */
			if (!blind && !(flg & (PROJECT_HIDE)))
			{
				/* Only do visuals if the player can "see" the bolt */
				if (panel_contains(y, x) && player_has_los_bold(p_ptr, y, x))
				{
					u16b p;

					TERM_COLOR a;
					SYMBOL_CODE c;

					/* Obtain the bolt pict */
					p = bolt_pict(oy, ox, y, x, typ);

					/* Extract attr/char */
					a = PICT_A(p);
					c = PICT_C(p);

					/* Visual effects */
					print_rel(c, a, y, x);
					move_cursor_relative(y, x);
					/*if (fresh_before)*/ Term_fresh();
					Term_xtra(TERM_XTRA_DELAY, msec);
					lite_spot(y, x);
					/*if (fresh_before)*/ Term_fresh();

					/* Display "beam" grids */
					if (flg & (PROJECT_BEAM))
					{
						/* Obtain the explosion pict */
						p = bolt_pict(y, x, y, x, typ);

						/* Extract attr/char */
						a = PICT_A(p);
						c = PICT_C(p);

						/* Visual effects */
						print_rel(c, a, y, x);
					}

					/* Hack -- Activate delay */
					visual = TRUE;
				}

				/* Hack -- delay anyway for consistency */
				else if (visual)
				{
					/* Delay for consistency */
					Term_xtra(TERM_XTRA_DELAY, msec);
				}
			}
			if (project_o(0, 0, y, x, dam, GF_SEEKER))notice = TRUE;
			if (is_mirror_grid(&p_ptr->current_floor_ptr->grid_array[y][x]))
			{
				/* The target of monsterspell becomes tha mirror(broken) */
				monster_target_y = y;
				monster_target_x = x;

				remove_mirror(y, x);
				next_mirror(p_ptr, &oy, &ox, y, x);

				path_n = i + project_path(&(path_g[i + 1]), (project_length ? project_length : MAX_RANGE), y, x, oy, ox, flg);
				for (j = last_i; j <= i; j++)
				{
					y = GRID_Y(path_g[j]);
					x = GRID_X(path_g[j]);
					if (project_m(0, 0, y, x, dam, GF_SEEKER, flg, TRUE)) notice = TRUE;
					if (!who && (project_m_n == 1) && !jump) {
						if (p_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx > 0) {
							monster_type *m_ptr = &p_ptr->current_floor_ptr->m_list[p_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx];

							if (m_ptr->ml)
							{
								if (!p_ptr->image) monster_race_track(m_ptr->ap_r_idx);
								health_track(p_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx);
							}
						}
					}
					(void)project_f(p_ptr->current_floor_ptr, 0, 0, y, x, dam, GF_SEEKER);
				}
				last_i = i;
			}
		}
		for(i = last_i ; i < path_n ; i++)
		{
			POSITION py, px;
			py = GRID_Y(path_g[i]);
			px = GRID_X(path_g[i]);
			if (project_m(0, 0, py, px, dam, GF_SEEKER, flg, TRUE))
				notice = TRUE;
			if (!who && (project_m_n == 1) && !jump) {
				if (p_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx > 0)
				{
					monster_type *m_ptr = &p_ptr->current_floor_ptr->m_list[p_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx];

					if (m_ptr->ml)
					{
						if (!p_ptr->image) monster_race_track(m_ptr->ap_r_idx);
						health_track(p_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx);
					}
				}
			}
			(void)project_f(p_ptr->current_floor_ptr, 0, 0, py, px, dam, GF_SEEKER);
		}
		return notice;
	}
	else if(typ == GF_SUPER_RAY){
		int j;
		int second_step = 0;

		/* Mega-Hack */
		project_m_n = 0;
		project_m_x = 0;
		project_m_y = 0;

		for (i = 0; i < path_n; ++i)
		{
			POSITION oy = y;
			POSITION ox = x;

			POSITION ny = GRID_Y(path_g[i]);
			POSITION nx = GRID_X(path_g[i]);

			/* Advance */
			y = ny;
			x = nx;

			gy[grids] = y;
			gx[grids] = x;
			grids++;


			/* Only do visuals if requested */
			if (!blind && !(flg & (PROJECT_HIDE)))
			{
				/* Only do visuals if the player can "see" the bolt */
				if (panel_contains(y, x) && player_has_los_bold(p_ptr, y, x))
				{
					u16b p;

					TERM_COLOR a;
					SYMBOL_CODE c;

					/* Obtain the bolt pict */
					p = bolt_pict(oy, ox, y, x, typ);

					/* Extract attr/char */
					a = PICT_A(p);
					c = PICT_C(p);

					/* Visual effects */
					print_rel(c, a, y, x);
					move_cursor_relative(y, x);
					/*if (fresh_before)*/ Term_fresh();
					Term_xtra(TERM_XTRA_DELAY, msec);
					lite_spot(y, x);
					/*if (fresh_before)*/ Term_fresh();

					/* Display "beam" grids */
					if (flg & (PROJECT_BEAM))
					{
						/* Obtain the explosion pict */
						p = bolt_pict(y, x, y, x, typ);

						/* Extract attr/char */
						a = PICT_A(p);
						c = PICT_C(p);

						/* Visual effects */
						print_rel(c, a, y, x);
					}

					/* Hack -- Activate delay */
					visual = TRUE;
				}

				/* Hack -- delay anyway for consistency */
				else if (visual)
				{
					/* Delay for consistency */
					Term_xtra(TERM_XTRA_DELAY, msec);
				}
			}
			if(project_o(0,0,y,x,dam,GF_SUPER_RAY) )notice=TRUE;
			if (!cave_have_flag_bold(y, x, FF_PROJECT))
			{
				if( second_step )continue;
				break;
			}
			if( is_mirror_grid(&p_ptr->current_floor_ptr->grid_array[y][x]) && !second_step )
			{
			  /* The target of monsterspell becomes tha mirror(broken) */
				monster_target_y = y;
				monster_target_x = x;

				remove_mirror(y,x);
				for( j = 0; j <=i ; j++ )
				{
					y = GRID_Y(path_g[j]);
					x = GRID_X(path_g[j]);
					(void)project_f(p_ptr->current_floor_ptr, 0,0,y,x,dam,GF_SUPER_RAY);
				}
				path_n = i;
				second_step =i+1;
				path_n += project_path(&(path_g[path_n+1]), (project_length ? project_length : MAX_RANGE), y, x, y-1, x-1, flg);
				path_n += project_path(&(path_g[path_n+1]), (project_length ? project_length : MAX_RANGE), y, x, y-1, x  , flg);
				path_n += project_path(&(path_g[path_n+1]), (project_length ? project_length : MAX_RANGE), y, x, y-1, x+1, flg);
				path_n += project_path(&(path_g[path_n+1]), (project_length ? project_length : MAX_RANGE), y, x, y  , x-1, flg);
				path_n += project_path(&(path_g[path_n+1]), (project_length ? project_length : MAX_RANGE), y, x, y  , x+1, flg);
				path_n += project_path(&(path_g[path_n+1]), (project_length ? project_length : MAX_RANGE), y, x, y+1, x-1, flg);
				path_n += project_path(&(path_g[path_n+1]), (project_length ? project_length : MAX_RANGE), y, x, y+1, x  , flg);
				path_n += project_path(&(path_g[path_n+1]), (project_length ? project_length : MAX_RANGE), y, x, y+1, x+1, flg);
			}
		}
		for( i = 0; i < path_n ; i++ )
		{
			POSITION py, px;
			py = GRID_Y(path_g[i]);
			px = GRID_X(path_g[i]);
			(void)project_m(0, 0, py, px, dam, GF_SUPER_RAY, flg, TRUE);
			if(!who && (project_m_n == 1) && !jump){
				if(p_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx >0 ){
					monster_type *m_ptr = &p_ptr->current_floor_ptr->m_list[p_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx];

					if (m_ptr->ml)
					{
						if (!p_ptr->image) monster_race_track(m_ptr->ap_r_idx);
						health_track(p_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx);
					}
				}
			}
			(void)project_f(p_ptr->current_floor_ptr, 0, 0, py, px, dam, GF_SUPER_RAY);
		}
		return notice;
	}

	/* Project along the path */
	for (i = 0; i < path_n; ++i)
	{
		POSITION oy = y;
		POSITION ox = x;

		POSITION ny = GRID_Y(path_g[i]);
		POSITION nx = GRID_X(path_g[i]);

		if (flg & PROJECT_DISI)
		{
			/* Hack -- Balls explode before reaching walls */
			if (cave_stop_disintegration(ny, nx) && (rad > 0)) break;
		}
		else if (flg & PROJECT_LOS)
		{
			/* Hack -- Balls explode before reaching walls */
			if (!cave_los_bold(p_ptr->current_floor_ptr, ny, nx) && (rad > 0)) break;
		}
		else
		{
			/* Hack -- Balls explode before reaching walls */
			if (!cave_have_flag_bold(ny, nx, FF_PROJECT) && (rad > 0)) break;
		}

		/* Advance */
		y = ny;
		x = nx;

		/* Collect beam grids */
		if (flg & (PROJECT_BEAM))
		{
			gy[grids] = y;
			gx[grids] = x;
			grids++;
		}

		/* Only do visuals if requested */
		if (!blind && !(flg & (PROJECT_HIDE | PROJECT_FAST)))
		{
			/* Only do visuals if the player can "see" the bolt */
			if (panel_contains(y, x) && player_has_los_bold(p_ptr, y, x))
			{
				u16b p;

				TERM_COLOR a;
				SYMBOL_CODE c;

				/* Obtain the bolt pict */
				p = bolt_pict(oy, ox, y, x, typ);

				/* Extract attr/char */
				a = PICT_A(p);
				c = PICT_C(p);

				/* Visual effects */
				print_rel(c, a, y, x);
				move_cursor_relative(y, x);
				/*if (fresh_before)*/ Term_fresh();
				Term_xtra(TERM_XTRA_DELAY, msec);
				lite_spot(y, x);
				/*if (fresh_before)*/ Term_fresh();

				/* Display "beam" grids */
				if (flg & (PROJECT_BEAM))
				{
					/* Obtain the explosion pict */
					p = bolt_pict(y, x, y, x, typ);

					/* Extract attr/char */
					a = PICT_A(p);
					c = PICT_C(p);

					/* Visual effects */
					print_rel(c, a, y, x);
				}

				/* Hack -- Activate delay */
				visual = TRUE;
			}

			/* Hack -- delay anyway for consistency */
			else if (visual)
			{
				/* Delay for consistency */
				Term_xtra(TERM_XTRA_DELAY, msec);
			}
		}
	}

	path_n = i;

	/* Save the "blast epicenter" */
	by = y;
	bx = x;

	if (breath && !path_n)
	{
		breath = FALSE;
		gm_rad = rad;
		if (!old_hide)
		{
			flg &= ~(PROJECT_HIDE);
		}
	}

	/* Start the "explosion" */
	gm[0] = 0;

	/* Hack -- make sure beams get to "explode" */
	gm[1] = grids;

	dist = path_n;
	dist_hack = dist;

	project_length = 0;

	/* If we found a "target", explode there */
	if (dist <= MAX_RANGE)
	{
		/* Mega-Hack -- remove the final "beam" grid */
		if ((flg & (PROJECT_BEAM)) && (grids > 0)) grids--;

		/*
		 * Create a conical breath attack
		 *
		 *       ***
		 *   ********
		 * D********@**
		 *   ********
		 *       ***
		 */

		if (breath)
		{
			flg &= ~(PROJECT_HIDE);

			breath_shape(path_g, dist, &grids, gx, gy, gm, &gm_rad, rad, y1, x1, by, bx, typ);
		}
		else
		{
			/* Determine the blast area, work from the inside out */
			for (dist = 0; dist <= rad; dist++)
			{
				/* Scan the maximal blast area of radius "dist" */
				for (y = by - dist; y <= by + dist; y++)
				{
					for (x = bx - dist; x <= bx + dist; x++)
					{
						/* Ignore "illegal" locations */
						if (!in_bounds2(p_ptr->current_floor_ptr, y, x)) continue;

						/* Enforce a "circular" explosion */
						if (distance(by, bx, y, x) != dist) continue;

						switch (typ)
						{
						case GF_LITE:
						case GF_LITE_WEAK:
							/* Lights are stopped by opaque terrains */
							if (!los(by, bx, y, x)) continue;
							break;
						case GF_DISINTEGRATE:
							/* Disintegration are stopped only by perma-walls */
							if (!in_disintegration_range(by, bx, y, x)) continue;
							break;
						default:
							/* Ball explosions are stopped by walls */
							if (!projectable(by, bx, y, x)) continue;
							break;
						}

						/* Save this grid */
						gy[grids] = y;
						gx[grids] = x;
						grids++;
					}
				}

				/* Encode some more "radius" info */
				gm[dist+1] = grids;
			}
		}
	}

	/* Speed -- ignore "non-explosions" */
	if (!grids) return (FALSE);


	/* Display the "blast area" if requested */
	if (!blind && !(flg & (PROJECT_HIDE)))
	{
		/* Then do the "blast", from inside out */
		for (t = 0; t <= gm_rad; t++)
		{
			/* Dump everything with this radius */
			for (i = gm[t]; i < gm[t+1]; i++)
			{
				y = gy[i];
				x = gx[i];

				/* Only do visuals if the player can "see" the blast */
				if (panel_contains(y, x) && player_has_los_bold(p_ptr, y, x))
				{
					u16b p;

					TERM_COLOR a;
					SYMBOL_CODE c;

					drawn = TRUE;

					/* Obtain the explosion pict */
					p = bolt_pict(y, x, y, x, typ);

					/* Extract attr/char */
					a = PICT_A(p);
					c = PICT_C(p);

					/* Visual effects -- Display */
					print_rel(c, a, y, x);
				}
			}

			/* Hack -- center the cursor */
			move_cursor_relative(by, bx);

			/* Flush each "radius" seperately */
			/*if (fresh_before)*/ Term_fresh();

			/* Delay (efficiently) */
			if (visual || drawn)
			{
				Term_xtra(TERM_XTRA_DELAY, msec);
			}
		}

		/* Flush the erasing */
		if (drawn)
		{
			/* Erase the explosion drawn above */
			for (i = 0; i < grids; i++)
			{
				y = gy[i];
				x = gx[i];

				/* Hack -- Erase if needed */
				if (panel_contains(y, x) && player_has_los_bold(p_ptr, y, x))
				{
					lite_spot(y, x);
				}
			}

			/* Hack -- center the cursor */
			move_cursor_relative(by, bx);

			/* Flush the explosion */
			/*if (fresh_before)*/ Term_fresh();
		}
	}

	update_creature(p_ptr);

	if (flg & PROJECT_KILL)
	{
		see_s_msg = (who > 0) ? is_seen(&p_ptr->current_floor_ptr->m_list[who]) :
			(!who ? TRUE : (player_can_see_bold(y1, x1) && projectable(p_ptr->y, p_ptr->x, y1, x1)));
	}


	/* Check features */
	if (flg & (PROJECT_GRID))
	{
		/* Start with "dist" of zero */
		dist = 0;

		/* Scan for features */
		for (i = 0; i < grids; i++)
		{
			/* Hack -- Notice new "dist" values */
			if (gm[dist+1] == i) dist++;

			/* Get the grid location */
			y = gy[i];
			x = gx[i];

			/* Find the closest point in the blast */
			if (breath)
			{
				int d = dist_to_line(y, x, y1, x1, by, bx);

				/* Affect the grid */
				if (project_f(p_ptr->current_floor_ptr, who, d, y, x, dam, typ)) notice = TRUE;
			}
			else
			{
				/* Affect the grid */
				if (project_f(p_ptr->current_floor_ptr, who, dist, y, x, dam, typ)) notice = TRUE;
			}
		}
	}

	update_creature(p_ptr);

	/* Check objects */
	if (flg & (PROJECT_ITEM))
	{
		/* Start with "dist" of zero */
		dist = 0;

		/* Scan for objects */
		for (i = 0; i < grids; i++)
		{
			/* Hack -- Notice new "dist" values */
			if (gm[dist+1] == i) dist++;

			/* Get the grid location */
			y = gy[i];
			x = gx[i];

			/* Find the closest point in the blast */
			if (breath)
			{
				int d = dist_to_line(y, x, y1, x1, by, bx);

				/* Affect the object in the grid */
				if (project_o(who, d, y, x, dam, typ)) notice = TRUE;
			}
			else
			{
				/* Affect the object in the grid */
				if (project_o(who, dist, y, x, dam, typ)) notice = TRUE;
			}
		}
	}


	/* Check monsters */
	if (flg & (PROJECT_KILL))
	{
		/* Mega-Hack */
		project_m_n = 0;
		project_m_x = 0;
		project_m_y = 0;

		/* Start with "dist" of zero */
		dist = 0;

		/* Scan for monsters */
		for (i = 0; i < grids; i++)
		{
			int effective_dist;

			/* Hack -- Notice new "dist" values */
			if (gm[dist + 1] == i) dist++;

			/* Get the grid location */
			y = gy[i];
			x = gx[i];

			/* A single bolt may be reflected */
			if (grids <= 1)
			{
				monster_type *m_ptr = &p_ptr->current_floor_ptr->m_list[p_ptr->current_floor_ptr->grid_array[y][x].m_idx];
				monster_race *ref_ptr = &r_info[m_ptr->r_idx];

				if ((flg & PROJECT_REFLECTABLE) && p_ptr->current_floor_ptr->grid_array[y][x].m_idx && (ref_ptr->flags2 & RF2_REFLECTING) &&
					((p_ptr->current_floor_ptr->grid_array[y][x].m_idx != p_ptr->riding) || !(flg & PROJECT_PLAYER)) &&
					(!who || dist_hack > 1) && !one_in_(10))
				{
					POSITION t_y, t_x;
					int max_attempts = 10;

					/* Choose 'new' target */
					do
					{
						t_y = y_saver - 1 + randint1(3);
						t_x = x_saver - 1 + randint1(3);
						max_attempts--;
					}
					while (max_attempts && in_bounds2u(p_ptr->current_floor_ptr, t_y, t_x) && !projectable(y, x, t_y, t_x));

					if (max_attempts < 1)
					{
						t_y = y_saver;
						t_x = x_saver;
					}

					sound(SOUND_REFLECT);
					if (is_seen(m_ptr))
					{
						if ((m_ptr->r_idx == MON_KENSHIROU) || (m_ptr->r_idx == MON_RAOU))
							msg_print(_("「北斗神拳奥義・二指真空把！」", "The attack bounces!"));
						else if (m_ptr->r_idx == MON_DIO) 
							msg_print(_("ディオ・ブランドーは指一本で攻撃を弾き返した！", "The attack bounces!"));
						else 
							msg_print(_("攻撃は跳ね返った！", "The attack bounces!"));
					}
					if (is_original_ap_and_seen(m_ptr)) ref_ptr->r_flags2 |= RF2_REFLECTING;

					/* Reflected bolts randomly target either one */
					if (player_bold(p_ptr, y, x) || one_in_(2)) flg &= ~(PROJECT_PLAYER);
					else flg |= PROJECT_PLAYER;

					/* The bolt is reflected */
					project(p_ptr->current_floor_ptr->grid_array[y][x].m_idx, 0, t_y, t_x, dam, typ, flg, monspell);

					/* Don't affect the monster any longer */
					continue;
				}
			}


			/* Find the closest point in the blast */
			if (breath)
			{
				effective_dist = dist_to_line(y, x, y1, x1, by, bx);
			}
			else
			{
				effective_dist = dist;
			}


			/* There is the riding player on this monster */
			if (p_ptr->riding && player_bold(p_ptr, y, x))
			{
				/* Aimed on the player */
				if (flg & PROJECT_PLAYER)
				{
					if (flg & (PROJECT_BEAM | PROJECT_REFLECTABLE | PROJECT_AIMED))
					{
						/*
						 * A beam or bolt is well aimed
						 * at the PLAYER!
						 * So don't affects the mount.
						 */
						continue;
					}
					else
					{
						/*
						 * The spell is not well aimed, 
						 * So partly affect the mount too.
						 */
						effective_dist++;
					}
				}

				/*
				 * This grid is the original target.
				 * Or aimed on your horse.
				 */
				else if (((y == y2) && (x == x2)) || (flg & PROJECT_AIMED))
				{
					/* Hit the mount with full damage */
				}

				/*
				 * Otherwise this grid is not the
				 * original target, it means that line
				 * of fire is obstructed by this
				 * monster.
				 */
				/*
				 * A beam or bolt will hit either
				 * player or mount.  Choose randomly.
				 */
				else if (flg & (PROJECT_BEAM | PROJECT_REFLECTABLE))
				{
					if (one_in_(2))
					{
						/* Hit the mount with full damage */
					}
					else
					{
						/* Hit the player later */
						flg |= PROJECT_PLAYER;

						/* Don't affect the mount */
						continue;
					}
				}

				/*
				 * The spell is not well aimed, so
				 * partly affect both player and
				 * mount.
				 */
				else
				{
					effective_dist++;
				}
			}

			/* Affect the monster in the grid */
			if (project_m(who, effective_dist, y, x, dam, typ, flg, see_s_msg)) notice = TRUE;
		}


		/* Player affected one monster (without "jumping") */
		if (!who && (project_m_n == 1) && !jump)
		{
			x = project_m_x;
			y = project_m_y;

			/* Track if possible */
			if (p_ptr->current_floor_ptr->grid_array[y][x].m_idx > 0)
			{
				monster_type *m_ptr = &p_ptr->current_floor_ptr->m_list[p_ptr->current_floor_ptr->grid_array[y][x].m_idx];

				if (m_ptr->ml)
				{
					if (!p_ptr->image) monster_race_track(m_ptr->ap_r_idx);
					health_track(p_ptr->current_floor_ptr->grid_array[y][x].m_idx);
				}
			}
		}
	}


	/* Check player */
	if (flg & (PROJECT_KILL))
	{
		/* Start with "dist" of zero */
		dist = 0;

		/* Scan for player */
		for (i = 0; i < grids; i++)
		{
			int effective_dist;

			/* Hack -- Notice new "dist" values */
			if (gm[dist+1] == i) dist++;

			/* Get the grid location */
			y = gy[i];
			x = gx[i];

			/* Affect the player? */
			if (!player_bold(p_ptr, y, x)) continue;

			/* Find the closest point in the blast */
			if (breath)
			{
				effective_dist = dist_to_line(y, x, y1, x1, by, bx);
			}
			else
			{
				effective_dist = dist;
			}

			/* Target may be your horse */
			if (p_ptr->riding)
			{
				/* Aimed on the player */
				if (flg & PROJECT_PLAYER)
				{
					/* Hit the player with full damage */
				}

				/*
				 * Hack -- When this grid was not the
				 * original target, a beam or bolt
				 * would hit either player or mount,
				 * and should be choosen randomly.
				 *
				 * But already choosen to hit the
				 * mount at this point.
				 *
				 * Or aimed on your horse.
				 */
				else if (flg & (PROJECT_BEAM | PROJECT_REFLECTABLE | PROJECT_AIMED))
				{
					/*
					 * A beam or bolt is well aimed
					 * at the mount!
					 * So don't affects the player.
					 */
					continue;
				}
				else
				{
					/*
					 * The spell is not well aimed, 
					 * So partly affect the player too.
					 */
					effective_dist++;
				}
			}

			/* Affect the player */
			if (project_p(who, p_ptr, who_name, effective_dist, y, x, dam, typ, flg, monspell)) notice = TRUE;
		}
	}

	if (p_ptr->riding)
	{
		GAME_TEXT m_name[MAX_NLEN];

		monster_desc(m_name, &p_ptr->current_floor_ptr->m_list[p_ptr->riding], 0);

		if (rakubadam_m > 0)
		{
			if (rakuba(p_ptr, rakubadam_m, FALSE))
			{
				msg_format(_("%^sに振り落とされた！", "%^s has thrown you off!"), m_name);
			}
		}
		if (p_ptr->riding && rakubadam_p > 0)
		{
			if(rakuba(p_ptr, rakubadam_p, FALSE))
			{
				msg_format(_("%^sから落ちてしまった！", "You have fallen from %s."), m_name);
			}
		}
	}

	/* Return "something was noticed" */
	return (notice);
}

/*!
 * @brief 鏡魔法「封魔結界」の効果処理
 * @param dam ダメージ量
 * @return 効果があったらTRUEを返す
 */
bool binding_field(HIT_POINT dam)
{
	POSITION mirror_x[10], mirror_y[10]; /* 鏡はもっと少ない */
	int mirror_num = 0;			  /* 鏡の数 */
	POSITION x, y;
	POSITION centersign;
	POSITION x1, x2, y1, y2;
	u16b p;
	int msec = delay_factor*delay_factor*delay_factor;

	/* 三角形の頂点 */
	POSITION point_x[3];
	POSITION point_y[3];

	/* Default target of monsterspell is player */
	monster_target_y = p_ptr->y;
	monster_target_x = p_ptr->x;

	for (x = 0; x < p_ptr->current_floor_ptr->width; x++)
	{
		for (y = 0; y < p_ptr->current_floor_ptr->height; y++)
		{
			if (is_mirror_grid(&p_ptr->current_floor_ptr->grid_array[y][x]) &&
				distance(p_ptr->y, p_ptr->x, y, x) <= MAX_RANGE &&
				distance(p_ptr->y, p_ptr->x, y, x) != 0 &&
				player_has_los_bold(p_ptr, y, x) &&
				projectable(p_ptr->y, p_ptr->x, y, x)
				) {
				mirror_y[mirror_num] = y;
				mirror_x[mirror_num] = x;
				mirror_num++;
			}
		}
	}

	if (mirror_num < 2)return FALSE;

	point_x[0] = randint0(mirror_num);
	do {
		point_x[1] = randint0(mirror_num);
	} while (point_x[0] == point_x[1]);

	point_y[0] = mirror_y[point_x[0]];
	point_x[0] = mirror_x[point_x[0]];
	point_y[1] = mirror_y[point_x[1]];
	point_x[1] = mirror_x[point_x[1]];
	point_y[2] = p_ptr->y;
	point_x[2] = p_ptr->x;

	x = point_x[0] + point_x[1] + point_x[2];
	y = point_y[0] + point_y[1] + point_y[2];

	centersign = (point_x[0] * 3 - x)*(point_y[1] * 3 - y)
		- (point_y[0] * 3 - y)*(point_x[1] * 3 - x);
	if (centersign == 0)return FALSE;

	x1 = point_x[0] < point_x[1] ? point_x[0] : point_x[1];
	x1 = x1 < point_x[2] ? x1 : point_x[2];
	y1 = point_y[0] < point_y[1] ? point_y[0] : point_y[1];
	y1 = y1 < point_y[2] ? y1 : point_y[2];

	x2 = point_x[0] > point_x[1] ? point_x[0] : point_x[1];
	x2 = x2 > point_x[2] ? x2 : point_x[2];
	y2 = point_y[0] > point_y[1] ? point_y[0] : point_y[1];
	y2 = y2 > point_y[2] ? y2 : point_y[2];

	for (y = y1; y <= y2; y++) {
		for (x = x1; x <= x2; x++) {
			if (centersign*((point_x[0] - x)*(point_y[1] - y)
				- (point_y[0] - y)*(point_x[1] - x)) >= 0 &&
				centersign*((point_x[1] - x)*(point_y[2] - y)
					- (point_y[1] - y)*(point_x[2] - x)) >= 0 &&
				centersign*((point_x[2] - x)*(point_y[0] - y)
					- (point_y[2] - y)*(point_x[0] - x)) >= 0)
			{
				if (player_has_los_bold(p_ptr, y, x) && projectable(p_ptr->y, p_ptr->x, y, x)) {
					/* Visual effects */
					if (!(p_ptr->blind)
						&& panel_contains(y, x)) {
						p = bolt_pict(y, x, y, x, GF_MANA);
						print_rel(PICT_C(p), PICT_A(p), y, x);
						move_cursor_relative(y, x);
						/*if (fresh_before)*/ Term_fresh();
						Term_xtra(TERM_XTRA_DELAY, msec);
					}
				}
			}
		}
	}
	for (y = y1; y <= y2; y++) {
		for (x = x1; x <= x2; x++) {
			if (centersign*((point_x[0] - x)*(point_y[1] - y)
				- (point_y[0] - y)*(point_x[1] - x)) >= 0 &&
				centersign*((point_x[1] - x)*(point_y[2] - y)
					- (point_y[1] - y)*(point_x[2] - x)) >= 0 &&
				centersign*((point_x[2] - x)*(point_y[0] - y)
					- (point_y[2] - y)*(point_x[0] - x)) >= 0)
			{
				if (player_has_los_bold(p_ptr, y, x) && projectable(p_ptr->y, p_ptr->x, y, x)) {
					(void)project_f(p_ptr->current_floor_ptr, 0, 0, y, x, dam, GF_MANA);
				}
			}
		}
	}
	for (y = y1; y <= y2; y++) {
		for (x = x1; x <= x2; x++) {
			if (centersign*((point_x[0] - x)*(point_y[1] - y)
				- (point_y[0] - y)*(point_x[1] - x)) >= 0 &&
				centersign*((point_x[1] - x)*(point_y[2] - y)
					- (point_y[1] - y)*(point_x[2] - x)) >= 0 &&
				centersign*((point_x[2] - x)*(point_y[0] - y)
					- (point_y[2] - y)*(point_x[0] - x)) >= 0)
			{
				if (player_has_los_bold(p_ptr, y, x) && projectable(p_ptr->y, p_ptr->x, y, x)) {
					(void)project_o(0, 0, y, x, dam, GF_MANA);
				}
			}
		}
	}
	for (y = y1; y <= y2; y++) {
		for (x = x1; x <= x2; x++) {
			if (centersign*((point_x[0] - x)*(point_y[1] - y)
				- (point_y[0] - y)*(point_x[1] - x)) >= 0 &&
				centersign*((point_x[1] - x)*(point_y[2] - y)
					- (point_y[1] - y)*(point_x[2] - x)) >= 0 &&
				centersign*((point_x[2] - x)*(point_y[0] - y)
					- (point_y[2] - y)*(point_x[0] - x)) >= 0)
			{
				if (player_has_los_bold(p_ptr, y, x) && projectable(p_ptr->y, p_ptr->x, y, x)) {
					(void)project_m(0, 0, y, x, dam, GF_MANA,
						(PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP), TRUE);
				}
			}
		}
	}
	if (one_in_(7)) {
		msg_print(_("鏡が結界に耐えきれず、壊れてしまった。", "The field broke a mirror"));
		remove_mirror(point_y[0], point_x[0]);
	}

	return TRUE;
}

/*!
 * @brief 鏡魔法「鏡の封印」の効果処理
 * @param dam ダメージ量
 * @return 効果があったらTRUEを返す
 */
void seal_of_mirror(HIT_POINT dam)
{
	POSITION x, y;

	for (x = 0; x < p_ptr->current_floor_ptr->width; x++)
	{
		for (y = 0; y < p_ptr->current_floor_ptr->height; y++)
		{
			if (is_mirror_grid(&p_ptr->current_floor_ptr->grid_array[y][x]))
			{
				if (project_m(0, 0, y, x, dam, GF_GENOCIDE,
					(PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP), TRUE))
				{
					if (!p_ptr->current_floor_ptr->grid_array[y][x].m_idx)
					{
						remove_mirror(y, x);
					}
				}
			}
		}
	}
	return;
}



/*!
 * @brief 領域魔法に応じて技能の名称を返す。
 * @param tval 魔法書のtval
 * @return 領域魔法の技能名称を保管した文字列ポインタ
 */
concptr spell_category_name(OBJECT_TYPE_VALUE tval)
{
	switch (tval)
	{
	case TV_HISSATSU_BOOK:
		return _("必殺技", "art");
	case TV_LIFE_BOOK:
		return _("祈り", "prayer");
	case TV_MUSIC_BOOK:
		return _("歌", "song");
	default:
		return _("呪文", "spell");
	}
}

