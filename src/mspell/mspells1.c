/*!
 * @file mspells1.c
 * @brief モンスター魔法の実装 / Monster spells (attack player)
 * @date 2014/01/17
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen.\n
 * @details
 * And now for Intelligent monster attacks (including spells).\n
 *\n
 * Original idea and code by "DRS" (David Reeves Sward).\n
 * Major modifications by "BEN" (Ben Harrison).\n
 *\n
 * Give monsters more intelligent attack/spell selection based on\n
 * observations of previous attacks on the player, and/or by allowing\n
 * the monster to "cheat" and know the player status.\n
 *\n
 * Maintain an idea of the player status, and use that information\n
 * to occasionally eliminate "ineffective" spell attacks.  We could\n
 * also eliminate ineffective normal attacks, but there is no reason\n
 * for the monster to do this, since he gains no benefit.\n
 * Note that MINDLESS monsters are not allowed to use this code.\n
 * And non-INTELLIGENT monsters only use it partially effectively.\n
 *\n
 * Actually learn what the player resists, and use that information\n
 * to remove attacks or spells before using them.  This will require\n
 * much less space, if I am not mistaken.  Thus, each monster gets a\n
 * set of 32 bit flags, "smart", build from the various "SM_*" flags.\n
 *\n
 * This has the added advantage that attacks and spells are related.\n
 * The "smart_learn" option means that the monster "learns" the flags\n
 * that should be set, and "smart_cheat" means that he "knows" them.\n
 * So "smart_cheat" means that the "smart" field is always up to date,\n
 * while "smart_learn" means that the "smart" field is slowly learned.\n
 * Both of them have the same effect on the "choose spell" routine.\n
 */

#include "system/angband.h"
#include "util/util.h"
#include "effect/effect-characteristics.h"
#include "dungeon/dungeon.h"
#include "grid/grid.h"
#include "object-enchant/object-curse.h"
#include "dungeon/quest.h"
#include "realm/realm-hex.h"
#include "player/player-move.h"
#include "player/player-status.h"
#include "monster/monster.h"
#include "mspell/monster-spell.h"
#include "spell/spells-type.h"
#include "world/world.h"
#include "realm/realm-song.h"
#include "view/display-main-window.h"
#include "player/player-races-table.h"
#include "player/player-class.h"
#include "spell/process-effect.h"
#include "spell/spells1.h"
#include "spell/spells3.h"
#include "mspell/mspell-learn-checker.h"
#include "mspell/assign-monster-spell.h"

#define DO_SPELL_NONE    0
#define DO_SPELL_BR_LITE 1
#define DO_SPELL_BR_DISI 2
#define DO_SPELL_BA_LITE 3

 /*!
  * @brief モンスターがプレイヤーの弱点をついた選択を取るかどうかの判定 /
  * Internal probability routine
  * @param r_ptr モンスター種族の構造体参照ポインタ
  * @param prob 基本確率(%)
  * @return 適した選択を取るならばTRUEを返す。
  */
static bool int_outof(monster_race *r_ptr, PERCENTAGE prob)
{
	/* Non-Smart monsters are half as "smart" */
	if (!(r_ptr->flags2 & RF2_SMART)) prob = prob / 2;

	/* Roll the dice */
	return (randint0(100) < prob);
}


/*!
 * @brief モンスターの魔法一覧から戦術的に適さない魔法を除外する /
 * Remove the "bad" spells from a spell list
 * @param m_idx モンスターの構造体参照ポインタ
 * @param f4p モンスター魔法のフラグリスト1
 * @param f5p モンスター魔法のフラグリスト2
 * @param f6p モンスター魔法のフラグリスト3
 * @return なし
 */
static void remove_bad_spells(MONSTER_IDX m_idx, player_type *target_ptr, u32b *f4p, u32b *f5p, u32b *f6p)
{
	monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	u32b f4 = (*f4p);
	u32b f5 = (*f5p);
	u32b f6 = (*f6p);

	u32b smart = 0L;

	/* Too stupid to know anything */
	if (r_ptr->flags2 & RF2_STUPID) return;


	/* Must be cheating or learning */
	if (!smart_cheat && !smart_learn) return;


	/* Update acquired knowledge */
	if (smart_learn)
	{
		/* Hack -- Occasionally forget player status */
		/* Only save SM_FRIENDLY, SM_PET or SM_CLONED */
		if (m_ptr->smart && (randint0(100) < 1)) m_ptr->smart &= (SM_FRIENDLY | SM_PET | SM_CLONED);

		/* Use the memorized flags */
		smart = m_ptr->smart;
	}


	/* Cheat if requested */
	if (smart_cheat)
	{
		/* Know element info */
		if (target_ptr->resist_acid) smart |= (SM_RES_ACID);
		if (is_oppose_acid(target_ptr)) smart |= (SM_OPP_ACID);
		if (target_ptr->immune_acid) smart |= (SM_IMM_ACID);
		if (target_ptr->resist_elec) smart |= (SM_RES_ELEC);
		if (is_oppose_elec(target_ptr)) smart |= (SM_OPP_ELEC);
		if (target_ptr->immune_elec) smart |= (SM_IMM_ELEC);
		if (target_ptr->resist_fire) smart |= (SM_RES_FIRE);
		if (is_oppose_fire(target_ptr)) smart |= (SM_OPP_FIRE);
		if (target_ptr->immune_fire) smart |= (SM_IMM_FIRE);
		if (target_ptr->resist_cold) smart |= (SM_RES_COLD);
		if (is_oppose_cold(target_ptr)) smart |= (SM_OPP_COLD);
		if (target_ptr->immune_cold) smart |= (SM_IMM_COLD);
		if (target_ptr->resist_pois) smart |= (SM_RES_POIS);
		if (is_oppose_pois(target_ptr)) smart |= (SM_OPP_POIS);

		/* Know special resistances */
		if (target_ptr->resist_neth) smart |= (SM_RES_NETH);
		if (target_ptr->resist_lite) smart |= (SM_RES_LITE);
		if (target_ptr->resist_dark) smart |= (SM_RES_DARK);
		if (target_ptr->resist_fear) smart |= (SM_RES_FEAR);
		if (target_ptr->resist_conf) smart |= (SM_RES_CONF);
		if (target_ptr->resist_chaos) smart |= (SM_RES_CHAOS);
		if (target_ptr->resist_disen) smart |= (SM_RES_DISEN);
		if (target_ptr->resist_blind) smart |= (SM_RES_BLIND);
		if (target_ptr->resist_nexus) smart |= (SM_RES_NEXUS);
		if (target_ptr->resist_sound) smart |= (SM_RES_SOUND);
		if (target_ptr->resist_shard) smart |= (SM_RES_SHARD);
		if (target_ptr->reflect) smart |= (SM_IMM_REFLECT);

		/* Know bizarre "resistances" */
		if (target_ptr->free_act) smart |= (SM_IMM_FREE);
		if (!target_ptr->msp) smart |= (SM_IMM_MANA);
	}

	if (!smart) return;

	if (smart & SM_IMM_ACID)
	{
		f4 &= ~(RF4_BR_ACID);
		f5 &= ~(RF5_BA_ACID);
		f5 &= ~(RF5_BO_ACID);
	}
	else if ((smart & (SM_OPP_ACID)) && (smart & (SM_RES_ACID)))
	{
		if (int_outof(r_ptr, 80)) f4 &= ~(RF4_BR_ACID);
		if (int_outof(r_ptr, 80)) f5 &= ~(RF5_BA_ACID);
		if (int_outof(r_ptr, 80)) f5 &= ~(RF5_BO_ACID);
	}
	else if ((smart & (SM_OPP_ACID)) || (smart & (SM_RES_ACID)))
	{
		if (int_outof(r_ptr, 30)) f4 &= ~(RF4_BR_ACID);
		if (int_outof(r_ptr, 30)) f5 &= ~(RF5_BA_ACID);
		if (int_outof(r_ptr, 30)) f5 &= ~(RF5_BO_ACID);
	}


	if (smart & (SM_IMM_ELEC))
	{
		f4 &= ~(RF4_BR_ELEC);
		f5 &= ~(RF5_BA_ELEC);
		f5 &= ~(RF5_BO_ELEC);
	}
	else if ((smart & (SM_OPP_ELEC)) && (smart & (SM_RES_ELEC)))
	{
		if (int_outof(r_ptr, 80)) f4 &= ~(RF4_BR_ELEC);
		if (int_outof(r_ptr, 80)) f5 &= ~(RF5_BA_ELEC);
		if (int_outof(r_ptr, 80)) f5 &= ~(RF5_BO_ELEC);
	}
	else if ((smart & (SM_OPP_ELEC)) || (smart & (SM_RES_ELEC)))
	{
		if (int_outof(r_ptr, 30)) f4 &= ~(RF4_BR_ELEC);
		if (int_outof(r_ptr, 30)) f5 &= ~(RF5_BA_ELEC);
		if (int_outof(r_ptr, 30)) f5 &= ~(RF5_BO_ELEC);
	}

	if (smart & (SM_IMM_FIRE))
	{
		f4 &= ~(RF4_BR_FIRE);
		f5 &= ~(RF5_BA_FIRE);
		f5 &= ~(RF5_BO_FIRE);
	}
	else if ((smart & (SM_OPP_FIRE)) && (smart & (SM_RES_FIRE)))
	{
		if (int_outof(r_ptr, 80)) f4 &= ~(RF4_BR_FIRE);
		if (int_outof(r_ptr, 80)) f5 &= ~(RF5_BA_FIRE);
		if (int_outof(r_ptr, 80)) f5 &= ~(RF5_BO_FIRE);
	}
	else if ((smart & (SM_OPP_FIRE)) || (smart & (SM_RES_FIRE)))
	{
		if (int_outof(r_ptr, 30)) f4 &= ~(RF4_BR_FIRE);
		if (int_outof(r_ptr, 30)) f5 &= ~(RF5_BA_FIRE);
		if (int_outof(r_ptr, 30)) f5 &= ~(RF5_BO_FIRE);
	}

	if (smart & (SM_IMM_COLD))
	{
		f4 &= ~(RF4_BR_COLD);
		f5 &= ~(RF5_BA_COLD);
		f5 &= ~(RF5_BO_COLD);
		f5 &= ~(RF5_BO_ICEE);
	}
	else if ((smart & (SM_OPP_COLD)) && (smart & (SM_RES_COLD)))
	{
		if (int_outof(r_ptr, 80)) f4 &= ~(RF4_BR_COLD);
		if (int_outof(r_ptr, 80)) f5 &= ~(RF5_BA_COLD);
		if (int_outof(r_ptr, 80)) f5 &= ~(RF5_BO_COLD);
		if (int_outof(r_ptr, 80)) f5 &= ~(RF5_BO_ICEE);
	}
	else if ((smart & (SM_OPP_COLD)) || (smart & (SM_RES_COLD)))
	{
		if (int_outof(r_ptr, 30)) f4 &= ~(RF4_BR_COLD);
		if (int_outof(r_ptr, 30)) f5 &= ~(RF5_BA_COLD);
		if (int_outof(r_ptr, 30)) f5 &= ~(RF5_BO_COLD);
		if (int_outof(r_ptr, 20)) f5 &= ~(RF5_BO_ICEE);
	}

	if ((smart & (SM_OPP_POIS)) && (smart & (SM_RES_POIS)))
	{
		if (int_outof(r_ptr, 80)) f4 &= ~(RF4_BR_POIS);
		if (int_outof(r_ptr, 80)) f5 &= ~(RF5_BA_POIS);
		if (int_outof(r_ptr, 60)) f4 &= ~(RF4_BA_NUKE);
		if (int_outof(r_ptr, 60)) f4 &= ~(RF4_BR_NUKE);
	}
	else if ((smart & (SM_OPP_POIS)) || (smart & (SM_RES_POIS)))
	{
		if (int_outof(r_ptr, 30)) f4 &= ~(RF4_BR_POIS);
		if (int_outof(r_ptr, 30)) f5 &= ~(RF5_BA_POIS);
	}

	if (smart & (SM_RES_NETH))
	{
		if (PRACE_IS_(target_ptr, RACE_SPECTRE))
		{
			f4 &= ~(RF4_BR_NETH);
			f5 &= ~(RF5_BA_NETH);
			f5 &= ~(RF5_BO_NETH);
		}
		else
		{
			if (int_outof(r_ptr, 20)) f4 &= ~(RF4_BR_NETH);
			if (int_outof(r_ptr, 50)) f5 &= ~(RF5_BA_NETH);
			if (int_outof(r_ptr, 50)) f5 &= ~(RF5_BO_NETH);
		}
	}

	if (smart & (SM_RES_LITE))
	{
		if (int_outof(r_ptr, 50)) f4 &= ~(RF4_BR_LITE);
		if (int_outof(r_ptr, 50)) f5 &= ~(RF5_BA_LITE);
	}

	if (smart & (SM_RES_DARK))
	{
		if (PRACE_IS_(target_ptr, RACE_VAMPIRE))
		{
			f4 &= ~(RF4_BR_DARK);
			f5 &= ~(RF5_BA_DARK);
		}
		else
		{
			if (int_outof(r_ptr, 50)) f4 &= ~(RF4_BR_DARK);
			if (int_outof(r_ptr, 50)) f5 &= ~(RF5_BA_DARK);
		}
	}

	if (smart & (SM_RES_FEAR))
	{
		f5 &= ~(RF5_SCARE);
	}

	if (smart & (SM_RES_CONF))
	{
		f5 &= ~(RF5_CONF);
		if (int_outof(r_ptr, 50)) f4 &= ~(RF4_BR_CONF);
	}

	if (smart & (SM_RES_CHAOS))
	{
		if (int_outof(r_ptr, 20)) f4 &= ~(RF4_BR_CHAO);
		if (int_outof(r_ptr, 50)) f4 &= ~(RF4_BA_CHAO);
	}

	if (smart & (SM_RES_DISEN))
	{
		if (int_outof(r_ptr, 40)) f4 &= ~(RF4_BR_DISE);
	}

	if (smart & (SM_RES_BLIND))
	{
		f5 &= ~(RF5_BLIND);
	}

	if (smart & (SM_RES_NEXUS))
	{
		if (int_outof(r_ptr, 50)) f4 &= ~(RF4_BR_NEXU);
		f6 &= ~(RF6_TELE_LEVEL);
	}

	if (smart & (SM_RES_SOUND))
	{
		if (int_outof(r_ptr, 50)) f4 &= ~(RF4_BR_SOUN);
	}

	if (smart & (SM_RES_SHARD))
	{
		if (int_outof(r_ptr, 40)) f4 &= ~(RF4_BR_SHAR);
	}

	if (smart & (SM_IMM_REFLECT))
	{
		if (int_outof(r_ptr, 150)) f5 &= ~(RF5_BO_COLD);
		if (int_outof(r_ptr, 150)) f5 &= ~(RF5_BO_FIRE);
		if (int_outof(r_ptr, 150)) f5 &= ~(RF5_BO_ACID);
		if (int_outof(r_ptr, 150)) f5 &= ~(RF5_BO_ELEC);
		if (int_outof(r_ptr, 150)) f5 &= ~(RF5_BO_NETH);
		if (int_outof(r_ptr, 150)) f5 &= ~(RF5_BO_WATE);
		if (int_outof(r_ptr, 150)) f5 &= ~(RF5_BO_MANA);
		if (int_outof(r_ptr, 150)) f5 &= ~(RF5_BO_PLAS);
		if (int_outof(r_ptr, 150)) f5 &= ~(RF5_BO_ICEE);
		if (int_outof(r_ptr, 150)) f5 &= ~(RF5_MISSILE);
	}

	if (smart & (SM_IMM_FREE))
	{
		f5 &= ~(RF5_HOLD);
		f5 &= ~(RF5_SLOW);
	}

	if (smart & (SM_IMM_MANA))
	{
		f5 &= ~(RF5_DRAIN_MANA);
	}

	/* No spells left? */
	/* if (!f4 && !f5 && !f6) ... */

	(*f4p) = f4;
	(*f5p) = f5;
	(*f6p) = f6;
}


/*!
 * @brief モンスターにとって所定の地点が召還に相応しい地点かどうかを返す。 /
 * Determine if there is a space near the player in which a summoned creature can appear
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y1 判定を行いたいマスのY座標
 * @param x1 判定を行いたいマスのX座標
 * @return 召還に相応しいならばTRUEを返す
 */
bool summon_possible(player_type *target_ptr, POSITION y1, POSITION x1)
{
	POSITION y, x;

	/* Start at the player's location, and check 2 grids in each dir */
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	for (y = y1 - 2; y <= y1 + 2; y++)
	{
		for (x = x1 - 2; x <= x1 + 2; x++)
		{
			/* Ignore illegal locations */
			if (!in_bounds(floor_ptr, y, x)) continue;

			/* Only check a circular area */
			if (distance(y1, x1, y, x) > 2) continue;

			/* ...nor on the Pattern */
			if (pattern_tile(floor_ptr, y, x)) continue;

			/* Require empty floor grid in line of projection */
			if (is_cave_empty_bold(target_ptr, y, x) && projectable(target_ptr, y1, x1, y, x) && projectable(target_ptr, y, x, y1, x1)) return TRUE;
		}
	}

	return FALSE;
}


/*!
 * @brief モンスターにとって死者復活を行うべき状態かどうかを返す /
 * Determine if there is a space near the player in which a summoned creature can appear
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_ptr 判定を行いたいモンスターの構造体参照ポインタ
 * @return 死者復活が有効な状態ならばTRUEを返す。
 */
bool raise_possible(player_type *target_ptr, monster_type *m_ptr)
{
	POSITION y = m_ptr->fy;
	POSITION x = m_ptr->fx;
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	for (POSITION xx = x - 5; xx <= x + 5; xx++)
	{
		grid_type *g_ptr;
		for (POSITION yy = y - 5; yy <= y + 5; yy++)
		{
			if (distance(y, x, yy, xx) > 5) continue;
			if (!los(target_ptr, y, x, yy, xx)) continue;
			if (!projectable(target_ptr, y, x, yy, xx)) continue;

			g_ptr = &floor_ptr->grid_array[yy][xx];
			/* Scan the pile of objects */
			OBJECT_IDX this_o_idx, next_o_idx = 0;
			for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
			{
				object_type *o_ptr = &floor_ptr->o_list[this_o_idx];
				next_o_idx = o_ptr->next_o_idx;

				/* Known to be worthless? */
				if (o_ptr->tval == TV_CORPSE)
				{
					if (!monster_has_hostile_align(target_ptr, m_ptr, 0, 0, &r_info[o_ptr->pval])) return TRUE;
				}
			}
		}
	}

	return FALSE;
}


/*!
 * @brief モンスターにとってボルト型魔法が有効な状態かを返す /
 * Determine if a bolt spell will hit the player.
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y1 ボルト魔法発射地点のY座標
 * @param x1 ボルト魔法発射地点のX座標
 * @param y2 ボルト魔法目標地点のY座標
 * @param x2 ボルト魔法目標地点のX座標
 * @param is_friend モンスターがプレイヤーに害意を持たない(ペットか友好的)ならばTRUEをつける
 * @return ボルト型魔法が有効ならばTRUEを返す。
 * @details
 * Originally, it was possible for a friendly to shoot another friendly.\n
 * Change it so a "clean shot" means no equally friendly monster is\n
 * between the attacker and target.\n
 *\n
 * This is exactly like "projectable", but it will\n
 * return FALSE if a monster is in the way.\n
 * no equally friendly monster is\n
 * between the attacker and target.\n
 */
bool clean_shot(player_type *target_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, bool is_friend)
{
	/* Check the projection path */
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	u16b grid_g[512];
	int grid_n = project_path(target_ptr, grid_g, MAX_RANGE, y1, x1, y2, x2, 0);

	/* No grid is ever projectable from itself */
	if (!grid_n) return FALSE;

	/* Final grid */
	POSITION y = GRID_Y(grid_g[grid_n - 1]);
	POSITION x = GRID_X(grid_g[grid_n - 1]);

	/* May not end in an unrequested grid */
	if ((y != y2) || (x != x2)) return FALSE;

	for (int i = 0; i < grid_n; i++)
	{
		y = GRID_Y(grid_g[i]);
		x = GRID_X(grid_g[i]);

		if ((floor_ptr->grid_array[y][x].m_idx > 0) && !((y == y2) && (x == x2)))
		{
			monster_type *m_ptr = &floor_ptr->m_list[floor_ptr->grid_array[y][x].m_idx];
			if (is_friend == is_pet(m_ptr))
			{
				return FALSE;
			}
		}

		/* Pets may not shoot through the character - TNB */
		if (player_bold(target_ptr, y, x) && is_friend) return FALSE;
	}

	return TRUE;
}


/*!
 * @brief モンスターのボルト型魔法処理 /
 * Cast a bolt at the player Stop if we hit a monster Affect monsters and the player
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターのID
 * @param y 目標のY座標
 * @param x 目標のX座標
 * @param typ 効果属性ID
 * @param dam_hp 威力
 * @param monspell モンスター魔法のID
 * @param target_type モンスターからモンスターへ撃つならMONSTER_TO_MONSTER、モンスターからプレイヤーならMONSTER_TO_PLAYER
 * @return なし
 */
void bolt(player_type *target_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, EFFECT_ID typ, int dam_hp, int monspell, int target_type)
{
	BIT_FLAGS flg = 0;
	switch (target_type)
	{
	case MONSTER_TO_MONSTER:
		flg = PROJECT_STOP | PROJECT_KILL;
		break;
	case MONSTER_TO_PLAYER:
		flg = PROJECT_STOP | PROJECT_KILL | PROJECT_PLAYER;
		break;
	}

	/* Target the player with a bolt attack */
	if (typ != GF_ARROW) flg |= PROJECT_REFLECTABLE;
	bool learnable = spell_learnable(target_ptr, m_idx);
	(void)project(target_ptr, m_idx, 0, y, x, dam_hp, typ, flg, (learnable ? monspell : -1));
}


/*!
 * @brief モンスターのビーム型魔法処理 /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターのID
 * @param y 目標のY座標
 * @param x 目標のX座標
 * @param typ 効果属性ID
 * @param dam_hp 威力
 * @param monspell モンスター魔法のID
 * @param target_type モンスターからモンスターへ撃つならMONSTER_TO_MONSTER、モンスターからプレイヤーならMONSTER_TO_PLAYER
 * @return なし
 */
void beam(player_type *target_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, EFFECT_ID typ, int dam_hp, int monspell, int target_type)
{
	BIT_FLAGS flg = 0;
	switch (target_type)
	{
	case MONSTER_TO_MONSTER:
		flg = PROJECT_BEAM | PROJECT_KILL | PROJECT_THRU;
		break;
	case MONSTER_TO_PLAYER:
		flg = PROJECT_BEAM | PROJECT_KILL | PROJECT_THRU | PROJECT_PLAYER;
		break;
	}

	/* Target the player with a bolt attack */
	bool learnable = spell_learnable(target_ptr, m_idx);
	(void)project(target_ptr, m_idx, 0, y, x, dam_hp, typ, flg, (learnable ? monspell : -1));
}


/*!
 * @brief モンスターのボール型＆ブレス型魔法処理 /
 * Cast a breath (or ball) attack at the player Pass over any monsters that may be in the way Affect grids, objects, monsters, and the player
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param y 目標地点のY座標
 * @param x 目標地点のX座標
 * @param m_idx モンスターのID
 * @param typ 効果属性ID
 * @param dam_hp 威力
 * @param rad 半径
 * @param breath
 * @param monspell モンスター魔法のID
 * @param target_type モンスターからモンスターへ撃つならMONSTER_TO_MONSTER、モンスターからプレイヤーならMONSTER_TO_PLAYER
 * @return なし
 */
void breath(player_type *target_ptr, POSITION y, POSITION x, MONSTER_IDX m_idx, EFFECT_ID typ, int dam_hp, POSITION rad, bool breath, int monspell, int target_type)
{
	monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	BIT_FLAGS flg = 0x00;
	switch (target_type)
	{
	case MONSTER_TO_MONSTER:
		flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
		break;
	case MONSTER_TO_PLAYER:
		flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_PLAYER;
		break;
	}

	/* Determine the radius of the blast */
	if ((rad < 1) && breath) rad = (r_ptr->flags2 & (RF2_POWERFUL)) ? 3 : 2;

	/* Handle breath attacks */
	if (breath) rad = 0 - rad;

	switch (typ)
	{
	case GF_ROCKET:
		flg |= PROJECT_STOP;
		break;
	case GF_DRAIN_MANA:
	case GF_MIND_BLAST:
	case GF_BRAIN_SMASH:
	case GF_CAUSE_1:
	case GF_CAUSE_2:
	case GF_CAUSE_3:
	case GF_CAUSE_4:
	case GF_HAND_DOOM:
		flg |= (PROJECT_HIDE | PROJECT_AIMED);
		break;
	}

	/* Target the player with a ball attack */
	bool learnable = spell_learnable(target_ptr, m_idx);
	(void)project(target_ptr, m_idx, rad, y, x, dam_hp, typ, flg, (learnable ? monspell : -1));
}


/*!
 * @brief ID値が正しいモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell is good for hurting the player (directly).
 * @param spell 判定対象のID
 * @return 正しいIDならばTRUEを返す。
 */
static bool spell_attack(byte spell)
{
	/* All RF4 spells hurt (except for shriek and dispel) */
	if (spell < 128 && spell > 98) return TRUE;

	/* Various "ball" spells */
	if (spell >= 128 && spell <= 128 + 8) return TRUE;

	/* "Cause wounds" and "bolt" spells */
	if (spell >= 128 + 12 && spell < 128 + 27) return TRUE;

	/* Hand of Doom */
	if (spell == 160 + 1) return TRUE;

	/* Psycho-Spear */
	if (spell == 160 + 11) return TRUE;

	/* Doesn't hurt */
	return FALSE;
}


/*!
 * @brief ID値が退避目的に適したモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell is good for escaping.
 * @param spell 判定対象のID
 * @return 適した魔法のIDならばTRUEを返す。
 */
static bool spell_escape(byte spell)
{
	/* Blink or Teleport */
	if (spell == 160 + 4 || spell == 160 + 5) return TRUE;

	/* Teleport the player away */
	if (spell == 160 + 9 || spell == 160 + 10) return TRUE;

	/* Isn't good for escaping */
	return FALSE;
}


/*!
 * @brief ID値が妨害目的に適したモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell is good for annoying the player.
 * @param spell 判定対象のID
 * @return 適した魔法のIDならばTRUEを返す。
 */
static bool spell_annoy(byte spell)
{
	/* Shriek */
	if (spell == 96 + 0) return TRUE;

	/* Brain smash, et al (added curses) */
	if (spell >= 128 + 9 && spell <= 128 + 14) return TRUE;

	/* Scare, confuse, blind, slow, paralyze */
	if (spell >= 128 + 27 && spell <= 128 + 31) return TRUE;

	/* Teleport to */
	if (spell == 160 + 8) return TRUE;

	/* Teleport level */
	if (spell == 160 + 10) return TRUE;

	/* Darkness, make traps, cause amnesia */
	if (spell >= 160 + 12 && spell <= 160 + 14) return TRUE;

	/* Doesn't annoy */
	return FALSE;
}


/*!
 * @brief ID値が召喚型のモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell is good for annoying the player.
 * @param spell 判定対象のID
 * @return 召喚型魔法のIDならばTRUEを返す。
 */
static bool spell_summon(byte spell)
{
	/* All summon spells */
	if (spell >= 160 + 16) return TRUE;

	/* Doesn't summon */
	return FALSE;
}


/*!
 * @brief ID値が死者復活処理かどうかを返す /
 * Return TRUE if a spell is good for annoying the player.
 * @param spell 判定対象のID
 * @return 死者復活の処理ならばTRUEを返す。
 */
static bool spell_raise(byte spell)
{
	/* All raise-dead spells */
	if (spell == 160 + 15) return TRUE;

	/* Doesn't summon */
	return FALSE;
}


/*!
 * @brief ID値が戦術的なモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell is good in a tactical situation.
 * @param spell 判定対象のID
 * @return 戦術的な魔法のIDならばTRUEを返す。
 */
static bool spell_tactic(byte spell)
{
	/* Blink */
	if (spell == 160 + 4) return TRUE;

	/* Not good */
	return FALSE;
}


/*!
 * @brief ID値が無敵化するモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell makes invulnerable.
 * @param spell 判定対象のID
 * @return 召喚型魔法のIDならばTRUEを返す。
 */
static bool spell_invulner(byte spell)
{
	/* Invulnerability */
	if (spell == 160 + 3) return TRUE;

	/* No invulnerability */
	return FALSE;
}


/*!
 * @brief ID値が加速するモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell hastes.
 * @param spell 判定対象のID
 * @return 召喚型魔法のIDならばTRUEを返す。
 */
static bool spell_haste(byte spell)
{
	/* Haste self */
	if (spell == 160 + 0) return TRUE;

	/* Not a haste spell */
	return FALSE;
}


/*!
 * @brief ID値が時間停止を行うモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell world.
 * @param spell 判定対象のID
 * @return 時間停止魔法のIDならばTRUEを返す。
 */
static bool spell_world(byte spell)
{
	if (spell == 160 + 6) return TRUE;
	return FALSE;
}


/*!
 * @brief ID値が特別効果のモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell special.
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param spell 判定対象のID
 * @return 特別効果魔法のIDならばTRUEを返す。
 */
static bool spell_special(player_type *target_ptr, byte spell)
{
	if (target_ptr->phase_out) return FALSE;
	if (spell == 160 + 7) return TRUE;
	return FALSE;
}


/*!
 * @brief ID値が光の剣のモンスター魔法IDかどうかを返す /
 * Return TRUE if a spell psycho-spear.
 * @param spell 判定対象のID
 * @return 光の剣のIDならばTRUEを返す。
 */
static bool spell_psy_spe(byte spell)
{
	/* world */
	if (spell == 160 + 11) return TRUE;

	/* Not a haste spell */
	return FALSE;
}


/*!
 * @brief ID値が治癒魔法かどうかを返す /
 * Return TRUE if a spell is good for healing.
 * @param spell 判定対象のID
 * @return 治癒魔法のIDならばTRUEを返す。
 */
static bool spell_heal(byte spell)
{
	/* Heal */
	if (spell == 160 + 2) return TRUE;

	/* No healing */
	return FALSE;
}


/*!
 * @brief ID値が魔力消去かどうかを返す /
 * Return TRUE if a spell is good for dispel.
 * @param spell 判定対象のID
 * @return 魔力消去のIDならばTRUEを返す。
 */
static bool spell_dispel(byte spell)
{
	/* Dispel */
	if (spell == 96 + 2) return TRUE;

	/* No dispel */
	return FALSE;
}


/*!
 * @brief モンスターがプレイヤーに魔力消去を与えるべきかを判定するルーチン
 * Check should monster cast dispel spell.
 * @param m_idx モンスターの構造体配列ID
 * @return 魔力消去をかけるべきならTRUEを返す。
 */
bool dispel_check(player_type *creature_ptr, MONSTER_IDX m_idx)
{
	/* Invulnabilty (including the song) */
	if (IS_INVULN(creature_ptr)) return TRUE;

	/* Wraith form */
	if (creature_ptr->wraith_form) return TRUE;

	/* Shield */
	if (creature_ptr->shield) return TRUE;

	/* Magic defence */
	if (creature_ptr->magicdef) return TRUE;

	/* Multi Shadow */
	if (creature_ptr->multishadow) return TRUE;

	/* Robe of dust */
	if (creature_ptr->dustrobe) return TRUE;

	/* Berserk Strength */
	if (creature_ptr->shero && (creature_ptr->pclass != CLASS_BERSERKER)) return TRUE;

	/* Demon Lord */
	if (creature_ptr->mimic_form == MIMIC_DEMON_LORD) return TRUE;

	/* Elemental resistances */
	monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	if (r_ptr->flags4 & RF4_BR_ACID)
	{
		if (!creature_ptr->immune_acid && (creature_ptr->oppose_acid || music_singing(creature_ptr, MUSIC_RESIST))) return TRUE;
		if (creature_ptr->special_defense & DEFENSE_ACID) return TRUE;
	}

	if (r_ptr->flags4 & RF4_BR_FIRE)
	{
		if (!((creature_ptr->prace == RACE_BALROG) && creature_ptr->lev > 44))
		{
			if (!creature_ptr->immune_fire && (creature_ptr->oppose_fire || music_singing(creature_ptr, MUSIC_RESIST))) return TRUE;
			if (creature_ptr->special_defense & DEFENSE_FIRE) return TRUE;
		}
	}

	if (r_ptr->flags4 & RF4_BR_ELEC)
	{
		if (!creature_ptr->immune_elec && (creature_ptr->oppose_elec || music_singing(creature_ptr, MUSIC_RESIST))) return TRUE;
		if (creature_ptr->special_defense & DEFENSE_ELEC) return TRUE;
	}

	if (r_ptr->flags4 & RF4_BR_COLD)
	{
		if (!creature_ptr->immune_cold && (creature_ptr->oppose_cold || music_singing(creature_ptr, MUSIC_RESIST))) return TRUE;
		if (creature_ptr->special_defense & DEFENSE_COLD) return TRUE;
	}

	if (r_ptr->flags4 & (RF4_BR_POIS | RF4_BR_NUKE))
	{
		if (!((creature_ptr->pclass == CLASS_NINJA) && creature_ptr->lev > 44))
		{
			if (creature_ptr->oppose_pois || music_singing(creature_ptr, MUSIC_RESIST)) return TRUE;
			if (creature_ptr->special_defense & DEFENSE_POIS) return TRUE;
		}
	}

	/* Ultimate resistance */
	if (creature_ptr->ult_res) return TRUE;

	/* Potion of Neo Tsuyosi special */
	if (creature_ptr->tsuyoshi) return TRUE;

	/* Elemental Brands */
	if ((creature_ptr->special_attack & ATTACK_ACID) && !(r_ptr->flagsr & RFR_EFF_IM_ACID_MASK)) return TRUE;
	if ((creature_ptr->special_attack & ATTACK_FIRE) && !(r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK)) return TRUE;
	if ((creature_ptr->special_attack & ATTACK_ELEC) && !(r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK)) return TRUE;
	if ((creature_ptr->special_attack & ATTACK_COLD) && !(r_ptr->flagsr & RFR_EFF_IM_COLD_MASK)) return TRUE;
	if ((creature_ptr->special_attack & ATTACK_POIS) && !(r_ptr->flagsr & RFR_EFF_IM_POIS_MASK)) return TRUE;

	if (creature_ptr->pspeed < 145)
	{
		if (IS_FAST(creature_ptr)) return TRUE;
	}

	/* Light speed */
	if (creature_ptr->lightspeed && (m_ptr->mspeed < 136)) return TRUE;

	if (creature_ptr->riding && (creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].mspeed < 135))
	{
		if (MON_FAST(&creature_ptr->current_floor_ptr->m_list[creature_ptr->riding])) return TRUE;
	}

	/* No need to cast dispel spell */
	return FALSE;
}


/*!
 * todo 長過ぎる。切り分けが必要
 * @brief モンスターの魔法選択ルーチン
 * Have a monster choose a spell from a list of "useful" spells.
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスターの構造体配列ID
 * @param spells 候補魔法IDをまとめた配列
 * @param num spellsの長さ
 * @return 選択したモンスター魔法のID
 * @details
 * Note that this list does NOT include spells that will just hit\n
 * other monsters, and the list is restricted when the monster is\n
 * "desperate".  Should that be the job of this function instead?\n
 *\n
 * Stupid monsters will just pick a spell randomly.  Smart monsters\n
 * will choose more "intelligently".\n
 *\n
 * Use the helper functions above to put spells into categories.\n
 *\n
 * This function may well be an efficiency bottleneck.\n
 */
static int choose_attack_spell(player_type *target_ptr, MONSTER_IDX m_idx, byte spells[], byte num)
{
	byte escape[96], escape_num = 0;
	byte attack[96], attack_num = 0;
	byte summon[96], summon_num = 0;
	byte tactic[96], tactic_num = 0;
	byte annoy[96], annoy_num = 0;
	byte invul[96], invul_num = 0;
	byte haste[96], haste_num = 0;
	byte world[96], world_num = 0;
	byte special[96], special_num = 0;
	byte psy_spe[96], psy_spe_num = 0;
	byte raise[96], raise_num = 0;
	byte heal[96], heal_num = 0;
	byte dispel[96], dispel_num = 0;

	/* Stupid monsters choose randomly */
	monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	if (r_ptr->flags2 & (RF2_STUPID)) return (spells[randint0(num)]);

	/* Categorize spells */
	for (int i = 0; i < num; i++)
	{
		/* Escape spell? */
		if (spell_escape(spells[i])) escape[escape_num++] = spells[i];

		/* Attack spell? */
		if (spell_attack(spells[i])) attack[attack_num++] = spells[i];

		/* Summon spell? */
		if (spell_summon(spells[i])) summon[summon_num++] = spells[i];

		/* Tactical spell? */
		if (spell_tactic(spells[i])) tactic[tactic_num++] = spells[i];

		/* Annoyance spell? */
		if (spell_annoy(spells[i])) annoy[annoy_num++] = spells[i];

		/* Invulnerability spell? */
		if (spell_invulner(spells[i])) invul[invul_num++] = spells[i];

		/* Haste spell? */
		if (spell_haste(spells[i])) haste[haste_num++] = spells[i];

		/* World spell? */
		if (spell_world(spells[i])) world[world_num++] = spells[i];

		/* Special spell? */
		if (spell_special(target_ptr, spells[i])) special[special_num++] = spells[i];

		/* Psycho-spear spell? */
		if (spell_psy_spe(spells[i])) psy_spe[psy_spe_num++] = spells[i];

		/* Raise-dead spell? */
		if (spell_raise(spells[i])) raise[raise_num++] = spells[i];

		/* Heal spell? */
		if (spell_heal(spells[i])) heal[heal_num++] = spells[i];

		/* Dispel spell? */
		if (spell_dispel(spells[i])) dispel[dispel_num++] = spells[i];
	}

	/*** Try to pick an appropriate spell type ***/

	/* world */
	if (world_num && (randint0(100) < 15) && !current_world_ptr->timewalk_m_idx)
	{
		/* Choose haste spell */
		return (world[randint0(world_num)]);
	}

	/* special */
	if (special_num)
	{
		bool success = FALSE;
		switch (m_ptr->r_idx)
		{
		case MON_BANOR:
		case MON_LUPART:
			if ((m_ptr->hp < m_ptr->maxhp / 2) && r_info[MON_BANOR].max_num && r_info[MON_LUPART].max_num) success = TRUE;
			break;
		default: break;
		}

		if (success) return (special[randint0(special_num)]);
	}

	/* Still hurt badly, couldn't flee, attempt to heal */
	if (m_ptr->hp < m_ptr->maxhp / 3 && one_in_(2))
	{
		/* Choose heal spell if possible */
		if (heal_num) return (heal[randint0(heal_num)]);
	}

	/* Hurt badly or afraid, attempt to flee */
	if (((m_ptr->hp < m_ptr->maxhp / 3) || MON_MONFEAR(m_ptr)) && one_in_(2))
	{
		/* Choose escape spell if possible */
		if (escape_num) return (escape[randint0(escape_num)]);
	}

	/* special */
	if (special_num)
	{
		bool success = FALSE;
		switch (m_ptr->r_idx)
		{
		case MON_OHMU:
		case MON_BANOR:
		case MON_LUPART:
			break;
		case MON_BANORLUPART:
			if (randint0(100) < 70) success = TRUE;
			break;
		case MON_ROLENTO:
			if (randint0(100) < 40) success = TRUE;
			break;
		default:
			if (randint0(100) < 50) success = TRUE;
			break;
		}
		if (success) return (special[randint0(special_num)]);
	}

	/* Player is close and we have attack spells, blink away */
	if ((distance(target_ptr->y, target_ptr->x, m_ptr->fy, m_ptr->fx) < 4) && (attack_num || (r_ptr->a_ability_flags2 & RF6_TRAPS)) && (randint0(100) < 75) && !current_world_ptr->timewalk_m_idx)
	{
		/* Choose tactical spell */
		if (tactic_num) return (tactic[randint0(tactic_num)]);
	}

	/* Summon if possible (sometimes) */
	if (summon_num && (randint0(100) < 40))
	{
		/* Choose summon spell */
		return (summon[randint0(summon_num)]);
	}

	/* dispel */
	if (dispel_num && one_in_(2))
	{
		/* Choose dispel spell if possible */
		if (dispel_check(target_ptr, m_idx))
		{
			return (dispel[randint0(dispel_num)]);
		}
	}

	/* Raise-dead if possible (sometimes) */
	if (raise_num && (randint0(100) < 40))
	{
		/* Choose raise-dead spell */
		return (raise[randint0(raise_num)]);
	}

	/* Attack spell (most of the time) */
	if (IS_INVULN(target_ptr))
	{
		if (psy_spe_num && (randint0(100) < 50))
		{
			/* Choose attack spell */
			return (psy_spe[randint0(psy_spe_num)]);
		}
		else if (attack_num && (randint0(100) < 40))
		{
			/* Choose attack spell */
			return (attack[randint0(attack_num)]);
		}
	}
	else if (attack_num && (randint0(100) < 85))
	{
		/* Choose attack spell */
		return (attack[randint0(attack_num)]);
	}

	/* Try another tactical spell (sometimes) */
	if (tactic_num && (randint0(100) < 50) && !current_world_ptr->timewalk_m_idx)
	{
		/* Choose tactic spell */
		return (tactic[randint0(tactic_num)]);
	}

	/* Cast globe of invulnerability if not already in effect */
	if (invul_num && !m_ptr->mtimed[MTIMED_INVULNER] && (randint0(100) < 50))
	{
		/* Choose Globe of Invulnerability */
		return (invul[randint0(invul_num)]);
	}

	/* We're hurt (not badly), try to heal */
	if ((m_ptr->hp < m_ptr->maxhp * 3 / 4) && (randint0(100) < 25))
	{
		/* Choose heal spell if possible */
		if (heal_num) return (heal[randint0(heal_num)]);
	}

	/* Haste self if we aren't already somewhat hasted (rarely) */
	if (haste_num && (randint0(100) < 20) && !MON_FAST(m_ptr))
	{
		/* Choose haste spell */
		return (haste[randint0(haste_num)]);
	}

	/* Annoy player (most of the time) */
	if (annoy_num && (randint0(100) < 80))
	{
		/* Choose annoyance spell */
		return (annoy[randint0(annoy_num)]);
	}

	/* Choose no spell */
	return 0;
}


/*!
 * @brief ID値が非魔術的な特殊技能かどうかを返す /
 * Return TRUE if a spell is inate spell.
 * @param spell 判定対象のID
 * @return 非魔術的な特殊技能ならばTRUEを返す。
 */
bool spell_is_inate(SPELL_IDX spell)
{
	if (spell < 32 * 4) /* Set RF4 */
	{
		if ((1L << (spell - 32 * 3)) & RF4_NOMAGIC_MASK) return TRUE;
	}

	if (spell < 32 * 5) /* Set RF5 */
	{
		if ((1L << (spell - 32 * 4)) & RF5_NOMAGIC_MASK) return TRUE;
	}

	if (spell < 32 * 6) /* Set RF6 */
	{
		if ((1L << (spell - 32 * 5)) & RF6_NOMAGIC_MASK) return TRUE;
	}

	/* This spell is not "inate" */
	return FALSE;
}


/*!
 * @brief モンスターがプレイヤーにダメージを与えるための最適な座標を算出する /
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_ptr 技能を使用するモンスター構造体の参照ポインタ
 * @param yp 最適な目標地点のY座標を返す参照ポインタ
 * @param xp 最適な目標地点のX座標を返す参照ポインタ
 * @param f_flag 射線に入れるのを避ける地形の所持フラグ
 * @param path_check 射線を判定するための関数ポインタ
 * @return 有効な座標があった場合TRUEを返す
 */
static bool adjacent_grid_check(player_type *target_ptr, monster_type *m_ptr, POSITION *yp, POSITION *xp,
	int f_flag, bool(*path_check)(player_type *, POSITION, POSITION, POSITION, POSITION))
{
	static int tonari_y[4][8] = { {-1, -1, -1,  0,  0,  1,  1,  1},
								 {-1, -1, -1,  0,  0,  1,  1,  1},
								 { 1,  1,  1,  0,  0, -1, -1, -1},
								 { 1,  1,  1,  0,  0, -1, -1, -1} };
	static int tonari_x[4][8] = { {-1,  0,  1, -1,  1, -1,  0,  1},
								 { 1,  0, -1,  1, -1,  1,  0, -1},
								 {-1,  0,  1, -1,  1, -1,  0,  1},
								 { 1,  0, -1,  1, -1,  1,  0, -1} };

	int next;
	if (m_ptr->fy < target_ptr->y && m_ptr->fx < target_ptr->x) next = 0;
	else if (m_ptr->fy < target_ptr->y) next = 1;
	else if (m_ptr->fx < target_ptr->x) next = 2;
	else next = 3;

	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	for (int i = 0; i < 8; i++)
	{
		int next_x = *xp + tonari_x[next][i];
		int next_y = *yp + tonari_y[next][i];
		grid_type *g_ptr;

		/* Access the next grid */
		g_ptr = &floor_ptr->grid_array[next_y][next_x];

		/* Skip this feature */
		if (!cave_have_flag_grid(g_ptr, f_flag)) continue;

		if (path_check(target_ptr, m_ptr->fy, m_ptr->fx, next_y, next_x))
		{
			*yp = next_y;
			*xp = next_x;
			return TRUE;
		}
	}

	return FALSE;
}


/*!
 * todo メインルーチンの割に長過ぎる。要分割
 * @brief モンスターの特殊技能メインルーチン /
 * Creatures can cast spells, shoot missiles, and breathe.
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスター構造体配列のID
 * @return 実際に特殊技能を利用したらTRUEを返す
 * @details
 * Returns "TRUE" if a spell (or whatever) was (successfully) cast.\n
 *\n
 * This function could use some work, but remember to\n
 * keep it as optimized as possible, while retaining generic code.\n
 *\n
 * Verify the various "blind-ness" checks in the code.\n
 *\n
 * Note that several effects should really not be "seen"\n
 * if the player is blind.  See also "effects.c" for other "mistakes".\n
 *\n
 * Perhaps monsters should breathe at locations *near* the player,\n
 * since this would allow them to inflict "partial" damage.\n
 *\n
 * Perhaps smart monsters should decline to use "bolt" spells if\n
 * there is a monster in the way, unless they wish to kill it.\n
 *\n
 * Note that, to allow the use of the "track_target" option at some\n
 * later time, certain non-optimal things are done in the code below,\n
 * including explicit checks against the "direct" variable, which is\n
 * currently always true by the time it is checked, but which should\n
 * really be set according to an explicit "projectable()" test, and\n
 * the use of generic "x,y" locations instead of the player location,\n
 * with those values being initialized with the player location.\n
 *\n
 * It will not be possible to "correctly" handle the case in which a\n
 * monster attempts to attack a location which is thought to contain\n
 * the player, but which in fact is nowhere near the player, since this\n
 * might induce all sorts of messages about the attack itself, and about\n
 * the effects of the attack, which the player might or might not be in\n
 * a position to observe.  Thus, for simplicity, it is probably best to\n
 * only allow "faulty" attacks by a monster if one of the important grids\n
 * (probably the initial or final grid) is in fact in view of the player.\n
 * It may be necessary to actually prevent spell attacks except when the\n
 * monster actually has line of sight to the player.  Note that a monster\n
 * could be left in a bizarre situation after the player ducked behind a\n
 * pillar and then teleported away, for example.\n
 *\n
 * @note
 * that certain spell attacks do not use the "project()" function\n
 * but "simulate" it via the "direct" variable, which is always at least\n
 * as restrictive as the "project()" function.  This is necessary to\n
 * prevent "blindness" attacks and such from bending around walls, etc,\n
 * and to allow the use of the "track_target" option in the future.\n
 *\n
 * Note that this function attempts to optimize the use of spells for the\n
 * cases in which the monster has no spells, or has spells but cannot use\n
 * them, or has spells but they will have no "useful" effect.  Note that\n
 * this function has been an efficiency bottleneck in the past.\n
 *\n
 * Note the special "MFLAG_NICE" flag, which prevents a monster from using\n
 * any spell attacks until the player has had a single chance to move.\n
 */
bool make_attack_spell(MONSTER_IDX m_idx, player_type *target_ptr)
{
#ifdef JP
#else

	char m_poss[80];
#endif
	/* Extract the "see-able-ness" */
	floor_type *floor_ptr = target_ptr->current_floor_ptr;
	monster_type *m_ptr = &floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	/* Cannot cast spells when confused */
	if (MON_CONFUSED(m_ptr))
	{
		reset_target(m_ptr);
		return FALSE;
	}

	/* Cannot cast spells when nice */
	if (m_ptr->mflag & MFLAG_NICE) return FALSE;
	if (!is_hostile(m_ptr)) return FALSE;


	/* Sometimes forbid inate attacks (breaths) */
	bool no_inate = FALSE;
	if (randint0(100) >= (r_ptr->freq_spell * 2)) no_inate = TRUE;

	/* Extract the racial spell flags */
	BIT_FLAGS f4 = r_ptr->flags4;
	BIT_FLAGS f5 = r_ptr->a_ability_flags1;
	BIT_FLAGS f6 = r_ptr->a_ability_flags2;

	/*** require projectable player ***/

	/* Check range */
	if ((m_ptr->cdis > MAX_RANGE) && !m_ptr->target_y) return FALSE;

	/* Check path for lite breath */
	POSITION x = target_ptr->x;
	POSITION y = target_ptr->y;
	POSITION x_br_lite = 0;
	POSITION y_br_lite = 0;
	if (f4 & RF4_BR_LITE)
	{
		y_br_lite = y;
		x_br_lite = x;

		if (los(target_ptr, m_ptr->fy, m_ptr->fx, y_br_lite, x_br_lite))
		{
			feature_type *f_ptr = &f_info[floor_ptr->grid_array[y_br_lite][x_br_lite].feat];

			if (!have_flag(f_ptr->flags, FF_LOS))
			{
				if (have_flag(f_ptr->flags, FF_PROJECT) && one_in_(2)) f4 &= ~(RF4_BR_LITE);
			}
		}

		/* Check path to next grid */
		else if (!adjacent_grid_check(target_ptr, m_ptr, &y_br_lite, &x_br_lite, FF_LOS, los)) f4 &= ~(RF4_BR_LITE);

		/* Don't breath lite to the wall if impossible */
		if (!(f4 & RF4_BR_LITE))
		{
			y_br_lite = 0;
			x_br_lite = 0;
		}
	}

	/* Check path */
	bool do_spell = DO_SPELL_NONE;
	if (projectable(target_ptr, m_ptr->fy, m_ptr->fx, y, x))
	{
		feature_type *f_ptr = &f_info[floor_ptr->grid_array[y][x].feat];

		if (!have_flag(f_ptr->flags, FF_PROJECT))
		{
			/* Breath disintegration to the wall if possible */
			if ((f4 & RF4_BR_DISI) && have_flag(f_ptr->flags, FF_HURT_DISI) && one_in_(2)) do_spell = DO_SPELL_BR_DISI;

			/* Breath lite to the transparent wall if possible */
			else if ((f4 & RF4_BR_LITE) && have_flag(f_ptr->flags, FF_LOS) && one_in_(2)) do_spell = DO_SPELL_BR_LITE;
		}
	}

	/* Check path to next grid */
	else
	{
		bool success = FALSE;

		if ((f4 & RF4_BR_DISI) && (m_ptr->cdis < MAX_RANGE / 2) &&
			in_disintegration_range(floor_ptr, m_ptr->fy, m_ptr->fx, y, x) &&
			(one_in_(10) || (projectable(target_ptr, y, x, m_ptr->fy, m_ptr->fx) && one_in_(2))))
		{
			do_spell = DO_SPELL_BR_DISI;
			success = TRUE;
		}
		else if ((f4 & RF4_BR_LITE) && (m_ptr->cdis < MAX_RANGE / 2) &&
			los(target_ptr, m_ptr->fy, m_ptr->fx, y, x) && one_in_(5))
		{
			do_spell = DO_SPELL_BR_LITE;
			success = TRUE;
		}
		else if ((f5 & RF5_BA_LITE) && (m_ptr->cdis <= MAX_RANGE))
		{
			POSITION by = y, bx = x;
			get_project_point(target_ptr, m_ptr->fy, m_ptr->fx, &by, &bx, 0L);
			if ((distance(by, bx, y, x) <= 3) && los(target_ptr, by, bx, y, x) && one_in_(5))
			{
				do_spell = DO_SPELL_BA_LITE;
				success = TRUE;
			}
		}

		if (!success) success = adjacent_grid_check(target_ptr, m_ptr, &y, &x, FF_PROJECT, projectable);

		if (!success)
		{
			if (m_ptr->target_y && m_ptr->target_x)
			{
				y = m_ptr->target_y;
				x = m_ptr->target_x;
				f4 &= (RF4_INDIRECT_MASK);
				f5 &= (RF5_INDIRECT_MASK);
				f6 &= (RF6_INDIRECT_MASK);
				success = TRUE;
			}

			if (y_br_lite && x_br_lite && (m_ptr->cdis < MAX_RANGE / 2) && one_in_(5))
			{
				if (!success)
				{
					y = y_br_lite;
					x = x_br_lite;
					do_spell = DO_SPELL_BR_LITE;
					success = TRUE;
				}
				else f4 |= (RF4_BR_LITE);
			}
		}

		/* No spells */
		if (!success) return FALSE;
	}

	reset_target(m_ptr);

	DEPTH rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);

	/* Forbid inate attacks sometimes */
	if (no_inate)
	{
		f4 &= ~(RF4_NOMAGIC_MASK);
		f5 &= ~(RF5_NOMAGIC_MASK);
		f6 &= ~(RF6_NOMAGIC_MASK);
	}

	bool can_use_lite_area = FALSE;
	if (f6 & RF6_DARKNESS)
	{
		if ((target_ptr->pclass == CLASS_NINJA) &&
			!(r_ptr->flags3 & (RF3_UNDEAD | RF3_HURT_LITE)) &&
			!(r_ptr->flags7 & RF7_DARK_MASK))
			can_use_lite_area = TRUE;

		if (!(r_ptr->flags2 & RF2_STUPID))
		{
			if (d_info[target_ptr->dungeon_idx].flags1 & DF1_DARKNESS) f6 &= ~(RF6_DARKNESS);
			else if ((target_ptr->pclass == CLASS_NINJA) && !can_use_lite_area) f6 &= ~(RF6_DARKNESS);
		}
	}

	bool in_no_magic_dungeon = (d_info[target_ptr->dungeon_idx].flags1 & DF1_NO_MAGIC) && floor_ptr->dun_level
		&& (!floor_ptr->inside_quest || is_fixed_quest_idx(floor_ptr->inside_quest));
	if (in_no_magic_dungeon && !(r_ptr->flags2 & RF2_STUPID))
	{
		f4 &= (RF4_NOMAGIC_MASK);
		f5 &= (RF5_NOMAGIC_MASK);
		f6 &= (RF6_NOMAGIC_MASK);
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
		if ((f6 & RF6_TELE_LEVEL) && is_teleport_level_ineffective(target_ptr, 0))
		{
			f6 &= ~(RF6_TELE_LEVEL);
		}
	}

	/* No spells left */
	if (!f4 && !f5 && !f6) return FALSE;

	/* Remove the "ineffective" spells */
	remove_bad_spells(m_idx, target_ptr, &f4, &f5, &f6);

	if (floor_ptr->inside_arena || target_ptr->phase_out)
	{
		f4 &= ~(RF4_SUMMON_MASK);
		f5 &= ~(RF5_SUMMON_MASK);
		f6 &= ~(RF6_SUMMON_MASK | RF6_TELE_LEVEL);

		if (m_ptr->r_idx == MON_ROLENTO) f6 &= ~(RF6_SPECIAL);
	}

	/* No spells left */
	if (!f4 && !f5 && !f6) return FALSE;

	if (!(r_ptr->flags2 & RF2_STUPID))
	{
		if (!target_ptr->csp) f5 &= ~(RF5_DRAIN_MANA);

		/* Check for a clean bolt shot */
		if (((f4 & RF4_BOLT_MASK) ||
			(f5 & RF5_BOLT_MASK) ||
			(f6 & RF6_BOLT_MASK)) &&
			!clean_shot(target_ptr, m_ptr->fy, m_ptr->fx, target_ptr->y, target_ptr->x, FALSE))
		{
			/* Remove spells that will only hurt friends */
			f4 &= ~(RF4_BOLT_MASK);
			f5 &= ~(RF5_BOLT_MASK);
			f6 &= ~(RF6_BOLT_MASK);
		}

		/* Check for a possible summon */
		if (((f4 & RF4_SUMMON_MASK) ||
			(f5 & RF5_SUMMON_MASK) ||
			(f6 & RF6_SUMMON_MASK)) &&
			!(summon_possible(target_ptr, y, x)))
		{
			/* Remove summoning spells */
			f4 &= ~(RF4_SUMMON_MASK);
			f5 &= ~(RF5_SUMMON_MASK);
			f6 &= ~(RF6_SUMMON_MASK);
		}

		/* Check for a possible raise dead */
		if ((f6 & RF6_RAISE_DEAD) && !raise_possible(target_ptr, m_ptr))
		{
			/* Remove raise dead spell */
			f6 &= ~(RF6_RAISE_DEAD);
		}

		/* Special moves restriction */
		if (f6 & RF6_SPECIAL)
		{
			if ((m_ptr->r_idx == MON_ROLENTO) && !summon_possible(target_ptr, y, x))
			{
				f6 &= ~(RF6_SPECIAL);
			}
		}

		/* No spells left */
		if (!f4 && !f5 && !f6) return FALSE;
	}

	/* Extract the "inate" spells */
	byte spell[96], num = 0;
	for (int k = 0; k < 32; k++)
	{
		if (f4 & (1L << k)) spell[num++] = k + RF4_SPELL_START;
	}

	/* Extract the "normal" spells */
	for (int k = 0; k < 32; k++)
	{
		if (f5 & (1L << k)) spell[num++] = k + RF5_SPELL_START;
	}

	/* Extract the "bizarre" spells */
	for (int k = 0; k < 32; k++)
	{
		if (f6 & (1L << k)) spell[num++] = k + RF6_SPELL_START;
	}

	/* No spells left */
	if (!num) return FALSE;

	/* Stop if player is dead or gone */
	if (!target_ptr->playing || target_ptr->is_dead) return FALSE;

	/* Stop if player is leaving */
	if (target_ptr->leaving) return FALSE;

	/* Get the monster name (or "it") */
	GAME_TEXT m_name[MAX_NLEN];
	monster_desc(target_ptr, m_name, m_ptr, 0x00);

#ifdef JP
#else
	/* Get the monster possessive ("his"/"her"/"its") */
	monster_desc(target_ptr, m_poss, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE);
#endif

	SPELL_IDX thrown_spell = 0;
	switch (do_spell)
	{
	case DO_SPELL_NONE:
	{
		int attempt = 10;
		while (attempt--)
		{
			thrown_spell = choose_attack_spell(target_ptr, m_idx, spell, num);
			if (thrown_spell) break;
		}
	}

	break;

	case DO_SPELL_BR_LITE:
		thrown_spell = 96 + 14; /* RF4_BR_LITE */
		break;

	case DO_SPELL_BR_DISI:
		thrown_spell = 96 + 31; /* RF4_BR_DISI */
		break;

	case DO_SPELL_BA_LITE:
		thrown_spell = 128 + 20; /* RF5_BA_LITE */
		break;

	default:
		return FALSE;
	}

	/* Abort if no spell was chosen */
	if (!thrown_spell) return FALSE;

	/* Calculate spell failure rate */
	PERCENTAGE failrate = 25 - (rlev + 3) / 4;

	/* Hack -- Stupid monsters will never fail (for jellies and such) */
	if (r_ptr->flags2 & RF2_STUPID) failrate = 0;

	/* Check for spell failure (inate attacks never fail) */
	if (!spell_is_inate(thrown_spell)
		&& (in_no_magic_dungeon || (MON_STUNNED(m_ptr) && one_in_(2)) || (randint0(100) < failrate)))
	{
		disturb(target_ptr, TRUE, TRUE);
		msg_format(_("%^sは呪文を唱えようとしたが失敗した。", "%^s tries to cast a spell, but fails."), m_name);

		return TRUE;
	}

	/* Hex: Anti Magic Barrier */
	if (!spell_is_inate(thrown_spell) && magic_barrier(target_ptr, m_idx))
	{
		msg_format(_("反魔法バリアが%^sの呪文をかき消した。", "Anti magic barrier cancels the spell which %^s casts."), m_name);
		return TRUE;
	}

	/* Projectable? */
	bool direct = player_bold(target_ptr, y, x);
	bool can_remember = is_original_ap_and_seen(target_ptr, m_ptr);
	if (!direct)
	{
		switch (thrown_spell)
		{
		case 96 + 2:    /* RF4_DISPEL */
		case 96 + 4:    /* RF4_SHOOT */
		case 128 + 9:   /* RF5_DRAIN_MANA */
		case 128 + 10:  /* RF5_MIND_BLAST */
		case 128 + 11:  /* RF5_BRAIN_SMASH */
		case 128 + 12:  /* RF5_CAUSE_1 */
		case 128 + 13:  /* RF5_CAUSE_2 */
		case 128 + 14:  /* RF5_CAUSE_3 */
		case 128 + 15:  /* RF5_CAUSE_4 */
		case 128 + 16:  /* RF5_BO_ACID */
		case 128 + 17:  /* RF5_BO_ELEC */
		case 128 + 18:  /* RF5_BO_FIRE */
		case 128 + 19:  /* RF5_BO_COLD */
		case 128 + 21:  /* RF5_BO_NETH */
		case 128 + 22:  /* RF5_BO_WATE */
		case 128 + 23:  /* RF5_BO_MANA */
		case 128 + 24:  /* RF5_BO_PLAS */
		case 128 + 25:  /* RF5_BO_ICEE */
		case 128 + 26:  /* RF5_MISSILE */
		case 128 + 27:  /* RF5_SCARE */
		case 128 + 28:  /* RF5_BLIND */
		case 128 + 29:  /* RF5_CONF */
		case 128 + 30:  /* RF5_SLOW */
		case 128 + 31:  /* RF5_HOLD */
		case 160 + 1:   /* RF6_HAND_DOOM */
		case 160 + 8:   /* RF6_TELE_TO */
		case 160 + 9:   /* RF6_TELE_AWAY */
		case 160 + 10:  /* RF6_TELE_LEVEL */
		case 160 + 11:  /* RF6_PSY_SPEAR */
		case 160 + 12:  /* RF6_DARKNESS */
		case 160 + 14:  /* RF6_FORGET */
			return FALSE;
		}
	}

	/* Cast the spell. */
	int dam = monspell_to_player(target_ptr, thrown_spell, y, x, m_idx);
	if (dam < 0) return FALSE;

	if ((target_ptr->action == ACTION_LEARN) && thrown_spell > 175)
	{
		learn_spell(target_ptr, thrown_spell - 96);
	}

	bool seen = (!target_ptr->blind && m_ptr->ml);
	bool maneable = player_has_los_bold(target_ptr, m_ptr->fy, m_ptr->fx);
	if (seen && maneable && !current_world_ptr->timewalk_m_idx && (target_ptr->pclass == CLASS_IMITATOR))
	{
		if (thrown_spell != 167) /* Not RF6_SPECIAL */
		{
			if (target_ptr->mane_num == MAX_MANE)
			{
				int i;
				target_ptr->mane_num--;
				for (i = 0; i < target_ptr->mane_num; i++)
				{
					target_ptr->mane_spell[i] = target_ptr->mane_spell[i + 1];
					target_ptr->mane_dam[i] = target_ptr->mane_dam[i + 1];
				}
			}
			target_ptr->mane_spell[target_ptr->mane_num] = thrown_spell - 96;
			target_ptr->mane_dam[target_ptr->mane_num] = dam;
			target_ptr->mane_num++;
			target_ptr->new_mane = TRUE;

			target_ptr->redraw |= (PR_IMITATION);
		}
	}

	/* Remember what the monster did to us */
	if (can_remember)
	{
		/* Inate spell */
		if (thrown_spell < 32 * 4)
		{
			r_ptr->r_flags4 |= (1L << (thrown_spell - 32 * 3));
			if (r_ptr->r_cast_spell < MAX_UCHAR) r_ptr->r_cast_spell++;
		}

		/* Bolt or Ball */
		else if (thrown_spell < 32 * 5)
		{
			r_ptr->r_flags5 |= (1L << (thrown_spell - 32 * 4));
			if (r_ptr->r_cast_spell < MAX_UCHAR) r_ptr->r_cast_spell++;
		}

		/* Special spell */
		else if (thrown_spell < 32 * 6)
		{
			r_ptr->r_flags6 |= (1L << (thrown_spell - 32 * 5));
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
