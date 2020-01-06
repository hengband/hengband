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
#include "core.h"
#include "util.h"

#include "creature.h"

#include "artifact.h"
#include "cmd-pet.h"
#include "cmd-dump.h"
#include "floor.h"
#include "grid.h"
#include "trap.h"
#include "monsterrace-hook.h"
#include "melee.h"
#include "world.h"
#include "spells.h"
#include "spells-summon.h"
#include "mutation.h"
#include "quest.h"
#include "avatar.h"

#include "spells-status.h"
#include "spells-floor.h"
#include "realm-hex.h"
#include "autopick.h"
#include "object-flavor.h"
#include "object-hook.h"
#include "monster-status.h"
#include "player-move.h"
#include "player-status.h"
#include "player-effects.h"
#include "player-skill.h"
#include "player-class.h"
#include "player-damage.h"
#include "player-inventory.h"

#include "dungeon.h"
#include "floor-events.h"
#include "feature.h"
#include "view-mainwindow.h"
#include "objectkind.h"
#include "monsterrace.h"
#include "targeting.h"
#include "realm-song.h"
#include "english.h"

/*!
 * @brief プレイヤー周辺の地形を感知する
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @param flag 特定地形ID
 * @param known 地形から危険フラグを外すならTRUE
 * @return 効力があった場合TRUEを返す
 */
static bool detect_feat_flag(player_type *caster_ptr, POSITION range, int flag, bool known)
{
	POSITION x, y;
	bool detect = FALSE;
	grid_type *g_ptr;

	if (d_info[caster_ptr->dungeon_idx].flags1 & DF1_DARKNESS) range /= 3;

	/* Scan the current panel */
	for (y = 1; y < caster_ptr->current_floor_ptr->height - 1; y++)
	{
		for (x = 1; x <= caster_ptr->current_floor_ptr->width - 1; x++)
		{
			int dist = distance(caster_ptr->y, caster_ptr->x, y, x);
			if (dist > range) continue;
			g_ptr = &caster_ptr->current_floor_ptr->grid_array[y][x];

			/* Hack -- Safe */
			if (flag == FF_TRAP)
			{
				/* Mark as detected */
				if (dist <= range && known)
				{
					if (dist <= range - 1) g_ptr->info |= (CAVE_IN_DETECT);

					g_ptr->info &= ~(CAVE_UNSAFE);

					lite_spot(y, x);
				}
			}

			if (cave_have_flag_grid(g_ptr, flag))
			{
				disclose_grid(y, x);
				g_ptr->info |= (CAVE_MARK);
				lite_spot(y, x);
				detect = TRUE;
			}
		}
	}

	return detect;
}


/*!
 * @brief プレイヤー周辺のトラップを感知する / Detect all traps on current panel
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @param known 感知外範囲を超える警告フラグを立てる場合TRUEを返す
 * @return 効力があった場合TRUEを返す
 */
bool detect_traps(player_type *caster_ptr, POSITION range, bool known)
{
	bool detect = detect_feat_flag(caster_ptr, range, FF_TRAP, known);

	if (known) caster_ptr->dtrap = TRUE;

	if (music_singing(caster_ptr, MUSIC_DETECT) && SINGING_COUNT(caster_ptr) > 0) detect = FALSE;
	if (detect)
	{
		msg_print(_("トラップの存在を感じとった！", "You sense the presence of traps!"));
	}

	return detect;
}


/*!
 * @brief プレイヤー周辺のドアを感知する / Detect all doors on current panel
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_doors(player_type *caster_ptr, POSITION range)
{
	bool detect = detect_feat_flag(caster_ptr, range, FF_DOOR, TRUE);

	if (music_singing(caster_ptr, MUSIC_DETECT) && SINGING_COUNT(caster_ptr) > 0) detect = FALSE;
	if (detect)
	{
		msg_print(_("ドアの存在を感じとった！", "You sense the presence of doors!"));
	}

	return detect;
}


/*!
 * @brief プレイヤー周辺の階段を感知する / Detect all stairs on current panel
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_stairs(player_type *caster_ptr, POSITION range)
{
	bool detect = detect_feat_flag(caster_ptr, range, FF_STAIRS, TRUE);

	if (music_singing(caster_ptr, MUSIC_DETECT) && SINGING_COUNT(caster_ptr) > 0) detect = FALSE;
	if (detect)
	{
		msg_print(_("階段の存在を感じとった！", "You sense the presence of stairs!"));
	}

	return detect;
}


/*!
 * @brief プレイヤー周辺の地形財宝を感知する / Detect any treasure on the current panel
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_treasure(player_type *caster_ptr, POSITION range)
{
	bool detect = detect_feat_flag(caster_ptr, range, FF_HAS_GOLD, TRUE);

	if (music_singing(caster_ptr, MUSIC_DETECT) && SINGING_COUNT(caster_ptr) > 6) detect = FALSE;
	if (detect)
	{
		msg_print(_("埋蔵された財宝の存在を感じとった！", "You sense the presence of buried treasure!"));
	}

	return detect;
}


/*!
 * @brief プレイヤー周辺のアイテム財宝を感知する / Detect all "gold" objects on the current panel
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_objects_gold(player_type *caster_ptr, POSITION range)
{
	OBJECT_IDX i;
	POSITION y, x;
	POSITION range2 = range;

	bool detect = FALSE;

	if (d_info[caster_ptr->dungeon_idx].flags1 & DF1_DARKNESS) range2 /= 3;

	/* Scan objects */
	for (i = 1; i < caster_ptr->current_floor_ptr->o_max; i++)
	{
		object_type *o_ptr = &caster_ptr->current_floor_ptr->o_list[i];

		if (!OBJECT_IS_VALID(o_ptr)) continue;
		if (OBJECT_IS_HELD_MONSTER(o_ptr)) continue;

		y = o_ptr->iy;
		x = o_ptr->ix;

		/* Only detect nearby objects */
		if (distance(caster_ptr->y, caster_ptr->x, y, x) > range2) continue;

		/* Detect "gold" objects */
		if (o_ptr->tval == TV_GOLD)
		{
			o_ptr->marked |= OM_FOUND;
			lite_spot(y, x);
			detect = TRUE;
		}
	}

	if (music_singing(caster_ptr, MUSIC_DETECT) && SINGING_COUNT(caster_ptr) > 6) detect = FALSE;
	if (detect)
	{
		msg_print(_("財宝の存在を感じとった！", "You sense the presence of treasure!"));
	}

	if (detect_monsters_string(caster_ptr, range, "$"))
	{
		detect = TRUE;
	}

	return (detect);
}


/*!
 * @brief 通常のアイテムオブジェクトを感知する / Detect all "normal" objects on the current panel
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_objects_normal(player_type *caster_ptr, POSITION range)
{
	OBJECT_IDX i;
	POSITION y, x;
	POSITION range2 = range;

	bool detect = FALSE;

	if (d_info[caster_ptr->dungeon_idx].flags1 & DF1_DARKNESS) range2 /= 3;

	/* Scan objects */
	for (i = 1; i < caster_ptr->current_floor_ptr->o_max; i++)
	{
		object_type *o_ptr = &caster_ptr->current_floor_ptr->o_list[i];

		if (!OBJECT_IS_VALID(o_ptr)) continue;
		if (OBJECT_IS_HELD_MONSTER(o_ptr)) continue;

		y = o_ptr->iy;
		x = o_ptr->ix;

		/* Only detect nearby objects */
		if (distance(caster_ptr->y, caster_ptr->x, y, x) > range2) continue;

		/* Detect "real" objects */
		if (o_ptr->tval != TV_GOLD)
		{
			o_ptr->marked |= OM_FOUND;
			lite_spot(y, x);
			detect = TRUE;
		}
	}

	if (music_singing(caster_ptr, MUSIC_DETECT) && SINGING_COUNT(caster_ptr) > 6) detect = FALSE;
	if (detect)
	{
		msg_print(_("アイテムの存在を感じとった！", "You sense the presence of objects!"));
	}

	if (detect_monsters_string(caster_ptr, range, "!=?|/`"))
	{
		detect = TRUE;
	}

	return (detect);
}


/*!
 * @brief 魔法効果のあるのアイテムオブジェクトを感知する / Detect all "magic" objects on the current panel.
 * @param caster_ptr プレーヤーへの参照ポインタ
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
bool detect_objects_magic(player_type *caster_ptr, POSITION range)
{
	OBJECT_TYPE_VALUE tv;
	OBJECT_IDX i;
	POSITION y, x;

	bool detect = FALSE;

	if (d_info[caster_ptr->dungeon_idx].flags1 & DF1_DARKNESS) range /= 3;

	/* Scan all objects */
	for (i = 1; i < caster_ptr->current_floor_ptr->o_max; i++)
	{
		object_type *o_ptr = &caster_ptr->current_floor_ptr->o_list[i];

		if (!OBJECT_IS_VALID(o_ptr)) continue;
		if (OBJECT_IS_HELD_MONSTER(o_ptr)) continue;

		y = o_ptr->iy;
		x = o_ptr->ix;

		/* Only detect nearby objects */
		if (distance(caster_ptr->y, caster_ptr->x, y, x) > range) continue;

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

	return (detect);
}


/*!
 * @brief 一般のモンスターを感知する / Detect all "normal" monsters on the current panel
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_normal(player_type *caster_ptr, POSITION range)
{
	MONSTER_IDX i;
	POSITION y, x;
	bool flag = FALSE;

	if (d_info[caster_ptr->dungeon_idx].flags1 & DF1_DARKNESS) range /= 3;

	for (i = 1; i < caster_ptr->current_floor_ptr->m_max; i++)
	{
		monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];
		if (!monster_is_valid(m_ptr)) continue;

		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Only detect nearby monsters */
		if (distance(caster_ptr->y, caster_ptr->x, y, x) > range) continue;

		/* Detect all non-invisible monsters */
		if (!(r_ptr->flags2 & RF2_INVISIBLE) || caster_ptr->see_inv)
		{
			/* Repair visibility later */
			repair_monsters = TRUE;

			m_ptr->mflag2 |= (MFLAG2_MARK | MFLAG2_SHOW);
			update_monster(caster_ptr, i, FALSE);
			flag = TRUE;
		}
	}

	if (music_singing(caster_ptr, MUSIC_DETECT) && SINGING_COUNT(caster_ptr) > 3) flag = FALSE;
	if (flag)
	{
		msg_print(_("モンスターの存在を感じとった！", "You sense the presence of monsters!"));
	}

	return (flag);
}


/*!
 * @brief 不可視のモンスターを感知する / Detect all "invisible" monsters around the player
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_invis(player_type *caster_ptr, POSITION range)
{
	MONSTER_IDX i;
	POSITION y, x;
	bool flag = FALSE;

	if (d_info[caster_ptr->dungeon_idx].flags1 & DF1_DARKNESS) range /= 3;

	for (i = 1; i < caster_ptr->current_floor_ptr->m_max; i++)
	{
		monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];

		if (!monster_is_valid(m_ptr)) continue;

		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Only detect nearby monsters */
		if (distance(caster_ptr->y, caster_ptr->x, y, x) > range) continue;

		/* Detect invisible monsters */
		if (r_ptr->flags2 & RF2_INVISIBLE)
		{
			/* Update monster recall window */
			if (caster_ptr->monster_race_idx == m_ptr->r_idx)
			{
				caster_ptr->window |= (PW_MONSTER);
			}

			/* Repair visibility later */
			repair_monsters = TRUE;

			m_ptr->mflag2 |= (MFLAG2_MARK | MFLAG2_SHOW);
			update_monster(caster_ptr, i, FALSE);
			flag = TRUE;
		}
	}

	if (music_singing(caster_ptr, MUSIC_DETECT) && SINGING_COUNT(caster_ptr) > 3) flag = FALSE;
	if (flag)
	{
		msg_print(_("透明な生物の存在を感じとった！", "You sense the presence of invisible creatures!"));
	}

	return (flag);
}

/*!
 * @brief 邪悪なモンスターを感知する / Detect all "evil" monsters on current panel
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_evil(player_type *caster_ptr, POSITION range)
{
	MONSTER_IDX i;
	POSITION y, x;
	bool flag = FALSE;

	if (d_info[caster_ptr->dungeon_idx].flags1 & DF1_DARKNESS) range /= 3;

	for (i = 1; i < caster_ptr->current_floor_ptr->m_max; i++)
	{
		monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];
		if (!monster_is_valid(m_ptr)) continue;

		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Only detect nearby monsters */
		if (distance(caster_ptr->y, caster_ptr->x, y, x) > range) continue;

		/* Detect evil monsters */
		if (r_ptr->flags3 & RF3_EVIL)
		{
			if (is_original_ap(m_ptr))
			{
				/* Take note that they are evil */
				r_ptr->r_flags3 |= (RF3_EVIL);

				/* Update monster recall window */
				if (caster_ptr->monster_race_idx == m_ptr->r_idx)
				{
					caster_ptr->window |= (PW_MONSTER);
				}
			}

			/* Repair visibility later */
			repair_monsters = TRUE;

			m_ptr->mflag2 |= (MFLAG2_MARK | MFLAG2_SHOW);
			update_monster(caster_ptr, i, FALSE);
			flag = TRUE;
		}
	}

	if (flag)
	{
		msg_print(_("邪悪なる生物の存在を感じとった！", "You sense the presence of evil creatures!"));
	}

	return (flag);
}

/*!
 * @brief 無生命のモンスターを感知する(アンデッド、悪魔系を含む) / Detect all "nonliving", "undead" or "demonic" monsters on current panel
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_nonliving(player_type *caster_ptr, POSITION range)
{
	MONSTER_IDX i;
	POSITION y, x;
	bool flag = FALSE;

	if (d_info[caster_ptr->dungeon_idx].flags1 & DF1_DARKNESS) range /= 3;

	for (i = 1; i < caster_ptr->current_floor_ptr->m_max; i++)
	{
		monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
		if (!monster_is_valid(m_ptr)) continue;

		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Only detect nearby monsters */
		if (distance(caster_ptr->y, caster_ptr->x, y, x) > range) continue;

		/* Detect non-living monsters */
		if (!monster_living(m_ptr->r_idx))
		{
			/* Update monster recall window */
			if (caster_ptr->monster_race_idx == m_ptr->r_idx)
			{
				caster_ptr->window |= (PW_MONSTER);
			}

			/* Repair visibility later */
			repair_monsters = TRUE;

			m_ptr->mflag2 |= (MFLAG2_MARK | MFLAG2_SHOW);
			update_monster(caster_ptr, i, FALSE);
			flag = TRUE;
		}
	}

	if (flag)
	{
		msg_print(_("自然でないモンスターの存在を感じた！", "You sense the presence of unnatural beings!"));
	}

	return (flag);
}

/*!
 * @brief 精神のあるモンスターを感知する / Detect all monsters it has mind on current panel
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_mind(player_type *caster_ptr, POSITION range)
{
	MONSTER_IDX i;
	POSITION y, x;
	bool flag = FALSE;

	if (d_info[caster_ptr->dungeon_idx].flags1 & DF1_DARKNESS) range /= 3;

	for (i = 1; i < caster_ptr->current_floor_ptr->m_max; i++)
	{
		monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];
		if (!monster_is_valid(m_ptr)) continue;

		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Only detect nearby monsters */
		if (distance(caster_ptr->y, caster_ptr->x, y, x) > range) continue;

		/* Detect non-living monsters */
		if (!(r_ptr->flags2 & RF2_EMPTY_MIND))
		{
			/* Update monster recall window */
			if (caster_ptr->monster_race_idx == m_ptr->r_idx)
			{
				caster_ptr->window |= (PW_MONSTER);
			}

			/* Repair visibility later */
			repair_monsters = TRUE;

			m_ptr->mflag2 |= (MFLAG2_MARK | MFLAG2_SHOW);
			update_monster(caster_ptr, i, FALSE);
			flag = TRUE;
		}
	}

	if (flag)
	{
		msg_print(_("殺気を感じとった！", "You sense the presence of someone's mind!"));
	}

	return (flag);
}


/*!
 * @brief 該当シンボルのモンスターを感知する / Detect all (string) monsters on current panel
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @param Match 対応シンボルの混じったモンスター文字列(複数指定化)
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_string(player_type *caster_ptr, POSITION range, concptr Match)
{
	MONSTER_IDX i;
	POSITION y, x;
	bool flag = FALSE;

	if (d_info[caster_ptr->dungeon_idx].flags1 & DF1_DARKNESS) range /= 3;

	for (i = 1; i < caster_ptr->current_floor_ptr->m_max; i++)
	{
		monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];
		if (!monster_is_valid(m_ptr)) continue;

		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Only detect nearby monsters */
		if (distance(caster_ptr->y, caster_ptr->x, y, x) > range) continue;

		/* Detect monsters with the same symbol */
		if (my_strchr(Match, r_ptr->d_char))
		{
			/* Update monster recall window */
			if (caster_ptr->monster_race_idx == m_ptr->r_idx)
			{
				caster_ptr->window |= (PW_MONSTER);
			}

			/* Repair visibility later */
			repair_monsters = TRUE;

			m_ptr->mflag2 |= (MFLAG2_MARK | MFLAG2_SHOW);
			update_monster(caster_ptr, i, FALSE);
			flag = TRUE;
		}
	}

	if (music_singing(caster_ptr, MUSIC_DETECT) && SINGING_COUNT(caster_ptr) > 3) flag = FALSE;
	if (flag)
	{
		msg_print(_("モンスターの存在を感じとった！", "You sense the presence of monsters!"));
	}

	return (flag);
}

/*!
 * @brief flags3に対応するモンスターを感知する / A "generic" detect monsters routine, tagged to flags3
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @param match_flag 感知フラグ
 * @return 効力があった場合TRUEを返す
 */
bool detect_monsters_xxx(player_type *caster_ptr, POSITION range, u32b match_flag)
{
	MONSTER_IDX i;
	POSITION y, x;
	bool flag = FALSE;
	concptr desc_monsters = _("変なモンスター", "weird monsters");

	if (d_info[caster_ptr->dungeon_idx].flags1 & DF1_DARKNESS) range /= 3;

	for (i = 1; i < caster_ptr->current_floor_ptr->m_max; i++)
	{
		monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];
		if (!monster_is_valid(m_ptr)) continue;

		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Only detect nearby monsters */
		if (distance(caster_ptr->y, caster_ptr->x, y, x) > range) continue;

		/* Detect evil monsters */
		if (r_ptr->flags3 & (match_flag))
		{
			if (is_original_ap(m_ptr))
			{
				/* Take note that they are something */
				r_ptr->r_flags3 |= (match_flag);

				/* Update monster recall window */
				if (caster_ptr->monster_race_idx == m_ptr->r_idx)
				{
					caster_ptr->window |= (PW_MONSTER);
				}
			}

			/* Repair visibility later */
			repair_monsters = TRUE;

			m_ptr->mflag2 |= (MFLAG2_MARK | MFLAG2_SHOW);
			update_monster(caster_ptr, i, FALSE);
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

		msg_format(_("%sの存在を感じとった！", "You sense the presence of %s!"), desc_monsters);
		msg_print(NULL);
	}

	return (flag);
}


/*!
 * @brief 全感知処理 / Detect everything
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param range 効果範囲
 * @return 効力があった場合TRUEを返す
 */
bool detect_all(player_type *caster_ptr, POSITION range)
{
	bool detect = FALSE;

	/* Detect everything */
	if (detect_traps(caster_ptr, range, TRUE)) detect = TRUE;
	if (detect_doors(caster_ptr, range)) detect = TRUE;
	if (detect_stairs(caster_ptr, range)) detect = TRUE;

	/* There are too many hidden treasure.  So... */
	/* if (detect_treasure(range)) detect = TRUE; */

	if (detect_objects_gold(caster_ptr, range)) detect = TRUE;
	if (detect_objects_normal(caster_ptr, range)) detect = TRUE;
	if (detect_monsters_invis(caster_ptr, range)) detect = TRUE;
	if (detect_monsters_normal(caster_ptr, range)) detect = TRUE;
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
bool project_all_los(player_type *caster_ptr, EFFECT_ID typ, HIT_POINT dam)
{
	MONSTER_IDX i;
	POSITION x, y;
	BIT_FLAGS flg = PROJECT_JUMP | PROJECT_KILL | PROJECT_HIDE;
	bool obvious = FALSE;

	/* Mark all (nearby) monsters */
	for (i = 1; i < caster_ptr->current_floor_ptr->m_max; i++)
	{
		monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
		if (!monster_is_valid(m_ptr)) continue;

		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Require line of sight */
		if (!player_has_los_bold(caster_ptr, y, x) || !projectable(caster_ptr->current_floor_ptr, caster_ptr->y, caster_ptr->x, y, x)) continue;

		/* Mark the monster */
		m_ptr->mflag |= (MFLAG_LOS);
	}

	/* Affect all marked monsters */
	for (i = 1; i < caster_ptr->current_floor_ptr->m_max; i++)
	{
		monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];

		/* Skip unmarked monsters */
		if (!(m_ptr->mflag & (MFLAG_LOS))) continue;

		/* Remove mark */
		m_ptr->mflag &= ~(MFLAG_LOS);

		y = m_ptr->fy;
		x = m_ptr->fx;

		/* Jump directly to the target monster */
		if (project(caster_ptr, 0, 0, y, x, dam, typ, flg, -1)) obvious = TRUE;
	}

	return (obvious);
}


/*!
 * @brief 視界内モンスターを加速する処理 / Speed monsters
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool speed_monsters(player_type *caster_ptr)
{
	return (project_all_los(caster_ptr, GF_OLD_SPEED, caster_ptr->lev));
}


/*!
 * @brief 視界内モンスターを加速する処理 / Slow monsters
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool slow_monsters(player_type *caster_ptr, int power)
{
	return (project_all_los(caster_ptr, GF_OLD_SLOW, power));
}


/*!
 * @brief 視界内モンスターを眠らせる処理 / Sleep monsters
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool sleep_monsters(player_type *caster_ptr, int power)
{
	return (project_all_los(caster_ptr, GF_OLD_SLEEP, power));
}


/*!
 * @brief 視界内の邪悪なモンスターをテレポート・アウェイさせる処理 / Banish evil monsters
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool banish_evil(player_type *caster_ptr, int dist)
{
	return (project_all_los(caster_ptr, GF_AWAY_EVIL, dist));
}


/*!
 * @brief 視界内のアンデッド・モンスターを恐怖させる処理 / Turn undead
 * @return 効力があった場合TRUEを返す
 */
bool turn_undead(player_type *caster_ptr)
{
	bool tester = (project_all_los(caster_ptr, GF_TURN_UNDEAD, caster_ptr->lev));
	if (tester)
		chg_virtue(caster_ptr, V_UNLIFE, -1);
	return tester;
}


/*!
 * @brief 視界内のアンデッド・モンスターにダメージを与える処理 / Dispel undead monsters
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool dispel_undead(player_type *caster_ptr, HIT_POINT dam)
{
	bool tester = (project_all_los(caster_ptr, GF_DISP_UNDEAD, dam));
	if (tester)
		chg_virtue(caster_ptr, V_UNLIFE, -2);
	return tester;
}


/*!
 * @brief 視界内の邪悪なモンスターにダメージを与える処理 / Dispel evil monsters
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool dispel_evil(player_type *caster_ptr, HIT_POINT dam)
{
	return (project_all_los(caster_ptr, GF_DISP_EVIL, dam));
}


/*!
 * @brief 視界内の善良なモンスターにダメージを与える処理 / Dispel good monsters
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool dispel_good(player_type *caster_ptr, HIT_POINT dam)
{
	return (project_all_los(caster_ptr, GF_DISP_GOOD, dam));
}


/*!
 * @brief 視界内のあらゆるモンスターにダメージを与える処理 / Dispel all monsters
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool dispel_monsters(player_type *caster_ptr, HIT_POINT dam)
{
	return (project_all_los(caster_ptr, GF_DISP_ALL, dam));
}


bool cleansing_nova(player_type *creature_ptr, bool magic, bool powerful)
{
	bool ident = FALSE;
	if (dispel_evil(creature_ptr, powerful ? 225 : 150)) ident = TRUE;
	int k = 3 * creature_ptr->lev;
	if (set_protevil(creature_ptr, (magic ? 0 : creature_ptr->protevil) + randint1(25) + k, FALSE)) ident = TRUE;
	if (set_poisoned(creature_ptr, 0)) ident = TRUE;
	if (set_afraid(creature_ptr, 0)) ident = TRUE;
	if (hp_player(creature_ptr, 50)) ident = TRUE;
	if (set_stun(creature_ptr, 0)) ident = TRUE;
	if (set_cut(creature_ptr,0)) ident = TRUE;
	return ident;
}


bool unleash_mana_storm(player_type *creature_ptr, bool powerful)
{
	msg_print(_("強力な魔力が敵を引き裂いた！", "Mighty magics rend your enemies!"));
	project(creature_ptr, 0, (powerful ? 7 : 5), creature_ptr->y, creature_ptr->x,
	(randint1(200) + (powerful ? 500 : 300)) * 2, GF_MANA, PROJECT_KILL | PROJECT_ITEM | PROJECT_GRID, -1);
	if ((creature_ptr->pclass != CLASS_MAGE) && (creature_ptr->pclass != CLASS_HIGH_MAGE) && (creature_ptr->pclass != CLASS_SORCERER) && (creature_ptr->pclass != CLASS_MAGIC_EATER) && (creature_ptr->pclass != CLASS_BLUE_MAGE))
	{
		(void)take_hit(creature_ptr, DAMAGE_NOESCAPE, 50, _("コントロールし難い強力な魔力の解放", "unleashing magics too mighty to control"), -1);
	}

	return TRUE;
}


/*!
 * @brief 視界内の生命のあるモンスターにダメージを与える処理 / Dispel 'living' monsters
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool dispel_living(player_type *caster_ptr, HIT_POINT dam)
{
	return (project_all_los(caster_ptr, GF_DISP_LIVING, dam));
}


/*!
 * @brief 視界内の悪魔系モンスターにダメージを与える処理 / Dispel 'living' monsters
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool dispel_demons(player_type *caster_ptr, HIT_POINT dam)
{
	return (project_all_los(caster_ptr, GF_DISP_DEMON, dam));
}


/*!
 * @brief 視界内のモンスターに「聖戦」効果を与える処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 効力があった場合TRUEを返す
 */
bool crusade(player_type *caster_ptr)
{
	return (project_all_los(caster_ptr, GF_CRUSADE, caster_ptr->lev*4));
}


/*!
 * @brief 視界内モンスターを怒らせる処理 / Wake up all monsters, and speed up "los" monsters.
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param who 怒らせる原因を起こしたモンスター(0ならばプレイヤー)
 * @return なし
 */
void aggravate_monsters(player_type *caster_ptr, MONSTER_IDX who)
{
	MONSTER_IDX i;
	bool sleep = FALSE;
	bool speed = FALSE;

	/* Aggravate everyone nearby */
	for (i = 1; i < caster_ptr->current_floor_ptr->m_max; i++)
	{
		monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
		if (!monster_is_valid(m_ptr)) continue;

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
		if (player_has_los_bold(caster_ptr, m_ptr->fy, m_ptr->fx))
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
	if (caster_ptr->riding) caster_ptr->update |= PU_BONUS;
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
bool genocide_aux(player_type *caster_ptr, MONSTER_IDX m_idx, int power, bool player_cast, int dam_side, concptr spell_name)
{
	int          msec = delay_factor * delay_factor * delay_factor;
	monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	bool         resist = FALSE;

	if (is_pet(m_ptr) && !player_cast) return FALSE;

	/* Hack -- Skip Unique Monsters or Quest Monsters */
	if (r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) resist = TRUE;
	else if (r_ptr->flags7 & RF7_UNIQUE2) resist = TRUE;
	else if (m_idx == caster_ptr->riding) resist = TRUE;
	else if ((caster_ptr->current_floor_ptr->inside_quest && !random_quest_number(caster_ptr->current_floor_ptr->dun_level)) || caster_ptr->current_floor_ptr->inside_arena || caster_ptr->phase_out) resist = TRUE;
	else if (player_cast && (r_ptr->level > randint0(power))) resist = TRUE;
	else if (player_cast && (m_ptr->mflag2 & MFLAG2_NOGENO)) resist = TRUE;

	else
	{
		if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname)
		{
			GAME_TEXT m_name[MAX_NLEN];

			monster_desc(m_name, m_ptr, MD_INDEF_VISIBLE);
			exe_write_diary(caster_ptr, NIKKI_NAMED_PET, RECORD_NAMED_PET_GENOCIDE, m_name);
		}

		delete_monster_idx(m_idx);
	}

	if (resist && player_cast)
	{
		bool see_m = is_seen(m_ptr);
		GAME_TEXT m_name[MAX_NLEN];

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
		take_hit(caster_ptr, DAMAGE_GENO, randint1(dam_side), format(_("%^sの呪文を唱えた疲労", "the strain of casting %^s"), spell_name), -1);
	}

	/* Visual feedback */
	move_cursor_relative(caster_ptr->y, caster_ptr->x);

	caster_ptr->redraw |= (PR_HP);
	caster_ptr->window |= (PW_PLAYER);

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
bool symbol_genocide(player_type *caster_ptr, int power, bool player_cast)
{
	MONSTER_IDX i;
	char typ;
	bool result = FALSE;

	/* Prevent genocide in quest levels */
	if ((caster_ptr->current_floor_ptr->inside_quest && !random_quest_number(caster_ptr->current_floor_ptr->dun_level)) || caster_ptr->current_floor_ptr->inside_arena || caster_ptr->phase_out)
	{
		msg_print(_("何も起きないようだ……", "It seems nothing happen here..."));
		return (FALSE);
	}

	/* Mega-Hack -- Get a monster symbol */
	while (!get_com(_("どの種類(文字)のモンスターを抹殺しますか: ", "Choose a monster race (by symbol) to genocide: "), &typ, FALSE)) ;

	/* Delete the monsters of that "type" */
	for (i = 1; i < caster_ptr->current_floor_ptr->m_max; i++)
	{
		monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];
		if (!monster_is_valid(m_ptr)) continue;

		/* Skip "wrong" monsters */
		if (r_ptr->d_char != typ) continue;

		result |= genocide_aux(caster_ptr, i, power, player_cast, 4, _("抹殺", "Genocide"));
	}

	if (result)
	{
		chg_virtue(caster_ptr, V_VITALITY, -2);
		chg_virtue(caster_ptr, V_CHANCE, -1);
	}

	return result;
}


/*!
 * @brief モンスターへの周辺抹殺処理ルーチン / Delete all nearby (non-unique) monsters
 * @param power 抹殺の威力
 * @param player_cast プレイヤーの魔法によるものならば TRUE
 * @return 効力があった場合TRUEを返す
 */
bool mass_genocide(player_type *caster_ptr, int power, bool player_cast)
{
	MONSTER_IDX i;
	bool result = FALSE;

	/* Prevent mass genocide in quest levels */
	if ((caster_ptr->current_floor_ptr->inside_quest && !random_quest_number(caster_ptr->current_floor_ptr->dun_level)) || caster_ptr->current_floor_ptr->inside_arena || caster_ptr->phase_out)
	{
		return (FALSE);
	}

	/* Delete the (nearby) monsters */
	for (i = 1; i < caster_ptr->current_floor_ptr->m_max; i++)
	{
		monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
		if (!monster_is_valid(m_ptr)) continue;

		/* Skip distant monsters */
		if (m_ptr->cdis > MAX_SIGHT) continue;

		/* Note effect */
		result |= genocide_aux(caster_ptr, i, power, player_cast, 3, _("周辺抹殺", "Mass Genocide"));
	}

	if (result)
	{
		chg_virtue(caster_ptr, V_VITALITY, -2);
		chg_virtue(caster_ptr, V_CHANCE, -1);
	}

	return result;
}


/*!
 * @brief アンデッド・モンスターへの周辺抹殺処理ルーチン / Delete all nearby (non-unique) undead
 * @param power 抹殺の威力
 * @param player_cast プレイヤーの魔法によるものならば TRUE
 * @return 効力があった場合TRUEを返す
 */
bool mass_genocide_undead(player_type *caster_ptr, int power, bool player_cast)
{
	MONSTER_IDX i;
	bool result = FALSE;

	/* Prevent mass genocide in quest levels */
	if ((caster_ptr->current_floor_ptr->inside_quest && !random_quest_number(caster_ptr->current_floor_ptr->dun_level)) || caster_ptr->current_floor_ptr->inside_arena || caster_ptr->phase_out)
	{
		return (FALSE);
	}

	/* Delete the (nearby) monsters */
	for (i = 1; i < caster_ptr->current_floor_ptr->m_max; i++)
	{
		monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];
		if (!monster_is_valid(m_ptr)) continue;

		if (!(r_ptr->flags3 & RF3_UNDEAD)) continue;

		/* Skip distant monsters */
		if (m_ptr->cdis > MAX_SIGHT) continue;

		/* Note effect */
		result |= genocide_aux(caster_ptr, i, power, player_cast, 3, _("アンデッド消滅", "Annihilate Undead"));
	}

	if (result)
	{
		chg_virtue(caster_ptr, V_UNLIFE, -2);
		chg_virtue(caster_ptr, V_CHANCE, -1);
	}

	return result;
}


/*!
 * @brief 周辺モンスターを調査する / Probe nearby monsters
 * @return 効力があった場合TRUEを返す
 */
bool probing(player_type *caster_ptr)
{
	int i;
	int speed; /* TODO */
	bool_hack cu, cv;
	bool probe = FALSE;
	char buf[256];
	concptr align;

	cu = Term->scr->cu;
	cv = Term->scr->cv;
	Term->scr->cu = 0;
	Term->scr->cv = 1;

	/* Probe all (nearby) monsters */
	for (i = 1; i < caster_ptr->current_floor_ptr->m_max; i++)
	{
		monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
		monster_race *r_ptr = &r_info[m_ptr->r_idx];
		if (!monster_is_valid(m_ptr)) continue;

		/* Require line of sight */
		if (!player_has_los_bold(caster_ptr, m_ptr->fy, m_ptr->fx)) continue;

		/* Probe visible monsters */
		if (m_ptr->ml)
		{
			GAME_TEXT m_name[MAX_NLEN];

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
			else if ((m_ptr->sub_align & (SUB_ALIGN_EVIL | SUB_ALIGN_GOOD)) == (SUB_ALIGN_EVIL | SUB_ALIGN_GOOD)) align = _("中立(善悪)", "neutral(good&evil)");
			else if (m_ptr->sub_align & SUB_ALIGN_EVIL) align = _("中立(邪悪)", "neutral(evil)");
			else if (m_ptr->sub_align & SUB_ALIGN_GOOD) align = _("中立(善良)", "neutral(good)");
			else align = _("中立", "neutral");

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

			caster_ptr->window |= (PW_MESSAGE);
			handle_stuff();

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
		chg_virtue(caster_ptr, V_KNOWLEDGE, 1);
		msg_print(_("これで全部です。", "That's all."));
	}

	return (probe);
}


/*!
 * @brief ペット爆破処理 /
 * @return なし
 */
void discharge_minion(player_type *caster_ptr)
{
	MONSTER_IDX i;
	bool okay = TRUE;

	for (i = 1; i < caster_ptr->current_floor_ptr->m_max; i++)
	{
		monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
		if (!m_ptr->r_idx || !is_pet(m_ptr)) continue;
		if (m_ptr->nickname) okay = FALSE;
	}

	if (!okay || caster_ptr->riding)
	{
		if (!get_check(_("本当に全ペットを爆破しますか？", "You will blast all pets. Are you sure? ")))
			return;
	}

	for (i = 1; i < caster_ptr->current_floor_ptr->m_max; i++)
	{
		HIT_POINT dam;
		monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
		monster_race *r_ptr;

		if (!m_ptr->r_idx || !is_pet(m_ptr)) continue;
		r_ptr = &r_info[m_ptr->r_idx];

		/* Uniques resist discharging */
		if (r_ptr->flags1 & RF1_UNIQUE)
		{
			GAME_TEXT m_name[MAX_NLEN];
			monster_desc(m_name, m_ptr, 0x00);
			msg_format(_("%sは爆破されるのを嫌がり、勝手に自分の世界へと帰った。", "%^s resists to be blasted, and run away."), m_name);
			delete_monster_idx(i);
			continue;
		}
		dam = m_ptr->maxhp / 2;
		if (dam > 100) dam = (dam - 100) / 2 + 100;
		if (dam > 400) dam = (dam - 400) / 2 + 400;
		if (dam > 800) dam = 800;
		project(caster_ptr, i, 2 + (r_ptr->level / 20), m_ptr->fy, m_ptr->fx, dam, GF_PLASMA, 
			PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL, -1);

		if (record_named_pet && m_ptr->nickname)
		{
			GAME_TEXT m_name[MAX_NLEN];

			monster_desc(m_name, m_ptr, MD_INDEF_VISIBLE);
			exe_write_diary(caster_ptr, NIKKI_NAMED_PET, RECORD_NAMED_PET_BLAST, m_name);
		}

		delete_monster_idx(i);
	}
}


/*!
 * todo この辺、xとyが引数になっているが、caster_ptr->xとcaster_ptr->yで全て置き換えが効くはず……
 * @brief 部屋全体を照らすサブルーチン
 * @param caster_ptr プレーヤーへの参照ポインタ
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
static void cave_temp_room_lite(player_type *caster_ptr)
{
	int i;

	/* Clear them all */
	for (i = 0; i < tmp_pos.n; i++)
	{
		POSITION y = tmp_pos.y[i];
		POSITION x = tmp_pos.x[i];

		grid_type *g_ptr = &caster_ptr->current_floor_ptr->grid_array[y][x];

		/* No longer in the array */
		g_ptr->info &= ~(CAVE_TEMP);

		/* Update only non-CAVE_GLOW grids */
		/* if (g_ptr->info & (CAVE_GLOW)) continue; */

		/* Perma-Lite */
		g_ptr->info |= (CAVE_GLOW);

		/* Process affected monsters */
		if (g_ptr->m_idx)
		{
			PERCENTAGE chance = 25;
			monster_type    *m_ptr = &caster_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
			monster_race    *r_ptr = &r_info[m_ptr->r_idx];
			update_monster(caster_ptr, g_ptr->m_idx, FALSE);

			/* Stupid monsters rarely wake up */
			if (r_ptr->flags2 & (RF2_STUPID)) chance = 10;

			/* Smart monsters always wake up */
			if (r_ptr->flags2 & (RF2_SMART)) chance = 100;

			/* Sometimes monsters wake up */
			if (MON_CSLEEP(m_ptr) && (randint0(100) < chance))
			{
				/* Wake up! */
				(void)set_monster_csleep(g_ptr->m_idx, 0);

				/* Notice the "waking up" */
				if (m_ptr->ml)
				{
					GAME_TEXT m_name[MAX_NLEN];
					monster_desc(m_name, m_ptr, 0);
					msg_format(_("%^sが目を覚ました。", "%^s wakes up."), m_name);
				}
			}
		}

		note_spot(y, x);
		lite_spot(y, x);
		update_local_illumination(caster_ptr, y, x);
	}

	tmp_pos.n = 0;
}


/*!
 * todo この辺、xとyが引数になっているが、caster_ptr->xとcaster_ptr->yで全て置き換えが効くはず……
 * @brief 部屋全体を暗くするサブルーチン
 * @param caster_ptr プレーヤーへの参照ポインタ
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
static void cave_temp_room_unlite(player_type *caster_ptr)
{
	int i;

	/* Clear them all */
	for (i = 0; i < tmp_pos.n; i++)
	{
		POSITION y = tmp_pos.y[i];
		POSITION x = tmp_pos.x[i];
		int j;

		grid_type *g_ptr = &caster_ptr->current_floor_ptr->grid_array[y][x];
		bool do_dark = !is_mirror_grid(g_ptr);

		/* No longer in the array */
		g_ptr->info &= ~(CAVE_TEMP);

		/* Darken the grid */
		if (do_dark)
		{
			if (caster_ptr->current_floor_ptr->dun_level || !is_daytime())
			{
				for (j = 0; j < 9; j++)
				{
					POSITION by = y + ddy_ddd[j];
					POSITION bx = x + ddx_ddd[j];

					if (in_bounds2(caster_ptr->current_floor_ptr, by, bx))
					{
						grid_type *cc_ptr = &caster_ptr->current_floor_ptr->grid_array[by][bx];

						if (have_flag(f_info[get_feat_mimic(cc_ptr)].flags, FF_GLOW))
						{
							do_dark = FALSE;
							break;
						}
					}
				}

				if (!do_dark) continue;
			}

			g_ptr->info &= ~(CAVE_GLOW);

			/* Hack -- Forget "boring" grids */
			if (!have_flag(f_info[get_feat_mimic(g_ptr)].flags, FF_REMEMBER))
			{
				/* Forget the grid */
				if (!view_torch_grids) g_ptr->info &= ~(CAVE_MARK);
				note_spot(y, x);
			}

			/* Process affected monsters */
			if (g_ptr->m_idx)
			{
				update_monster(caster_ptr, g_ptr->m_idx, FALSE);
			}

			lite_spot(y, x);
			update_local_illumination(caster_ptr, y, x);
		}
	}

	tmp_pos.n = 0;
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
 * @brief 部屋内にある一点の周囲に該当する地形数かいくつあるかをグローバル変数tmp_pos.nに返す / Aux function -- see below
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param y 部屋内のy座標1点
 * @param x 部屋内のx座標1点
 * @param only_room 部屋内地形のみをチェック対象にするならば TRUE
 * @param pass_bold 地形条件を返す関数ポインタ
 * @return 該当地形の数
 */
static void cave_temp_room_aux(POSITION y, POSITION x, bool only_room, bool (*pass_bold)(POSITION, POSITION))
{
	grid_type *g_ptr;
	g_ptr = &p_ptr->current_floor_ptr->grid_array[y][x];

	/* Avoid infinite recursion */
	if (g_ptr->info & (CAVE_TEMP)) return;

	/* Do not "leave" the current room */
	if (!(g_ptr->info & (CAVE_ROOM)))
	{
		if (only_room) return;

		/* Verify */
		if (!in_bounds2(p_ptr->current_floor_ptr, y, x)) return;

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
		if (in_bounds(p_ptr->current_floor_ptr, y, x) && pass_bold(y, x) &&
		    (next_to_walls_adj(y, x, pass_bold) == 6) && (next_to_open(y, x, pass_bold) <= 1)) return;
	}

	/* Paranoia -- verify space */
	if (tmp_pos.n == TEMP_MAX) return;

	/* Mark the grid as "seen" */
	g_ptr->info |= (CAVE_TEMP);

	/* Add it to the "seen" set */
	tmp_pos.y[tmp_pos.n] = y;
	tmp_pos.x[tmp_pos.n] = x;
	tmp_pos.n++;
}


/*!
 * todo このシンタックスシュガーは不要
 * @brief 指定のマスが光を通すか(LOSフラグを持つか)を返す。 / Aux function -- see below
 * @param y 指定Y座標
 * @param x 指定X座標
 * @return 光を通すならばtrueを返す。
 */
static bool cave_pass_lite_bold(POSITION y, POSITION x)
{
	return cave_los_bold(p_ptr->current_floor_ptr, y, x);
}


/*!
 * @brief 部屋内にある一点の周囲がいくつ光を通すかをグローバル変数tmp_pos.nに返す / Aux function -- see below
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
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param y 指定Y座標
 * @param x 指定X座標
 * @return 射線を通すならばtrueを返す。
 */
static bool cave_pass_dark_bold(POSITION y, POSITION x)
{
	return cave_have_flag_bold(p_ptr->current_floor_ptr, y, x, FF_PROJECT);
}


/*!
 * @brief 部屋内にある一点の周囲がいくつ射線を通すかをグローバル変数tmp_pos.nに返す / Aux function -- see below
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
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param y1 指定Y座標
 * @param x1 指定X座標
 * @return なし
 */
void lite_room(player_type *caster_ptr, POSITION y1, POSITION x1)
{
	int i;
	POSITION x, y;

	/* Add the initial grid */
	cave_temp_lite_room_aux(y1, x1);

	/* While grids are in the queue, add their neighbors */
	for (i = 0; i < tmp_pos.n; i++)
	{
		x = tmp_pos.x[i], y = tmp_pos.y[i];

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
	cave_temp_room_lite(caster_ptr);

	if (caster_ptr->special_defense & NINJA_S_STEALTH)
	{
		if (caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x].info & CAVE_GLOW) set_superstealth(caster_ptr, FALSE);
	}
}


/*!
 * @brief 指定された部屋内を暗くする / Darken all rooms containing the given location
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param y1 指定Y座標
 * @param x1 指定X座標
 * @return なし
 */
void unlite_room(player_type *caster_ptr, POSITION y1, POSITION x1)
{
	int i;
	POSITION x, y;

	/* Add the initial grid */
	cave_temp_unlite_room_aux(y1, x1);

	/* Spread, breadth first */
	for (i = 0; i < tmp_pos.n; i++)
	{
		x = tmp_pos.x[i], y = tmp_pos.y[i];

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
	cave_temp_room_unlite(caster_ptr);
}


/*!
 * @brief スターライトの効果を発生させる
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param magic 魔法による効果であればTRUE、スターライトの杖による効果であればFALSE
 * @return 常にTRUE
 */
bool starlight(player_type *caster_ptr, bool magic)
{
	HIT_POINT num = damroll(5, 3);
	POSITION y = 0, x = 0;
	int k;
	int attempts;

	if (!caster_ptr->blind && !magic)
	{
		msg_print(_("杖の先が明るく輝いた...", "The end of the staff glows brightly..."));
	}

	for (k = 0; k < num; k++)
	{
		attempts = 1000;

		while (attempts--)
		{
			scatter(&y, &x, caster_ptr->y, caster_ptr->x, 4, PROJECT_LOS);
			if (!cave_have_flag_bold(caster_ptr->current_floor_ptr, y, x, FF_PROJECT)) continue;
			if (!player_bold(caster_ptr, y, x)) break;
		}

		project(caster_ptr, 0, 0, y, x, damroll(6 + caster_ptr->lev / 8, 10), GF_LITE_WEAK,
			(PROJECT_BEAM | PROJECT_THRU | PROJECT_GRID | PROJECT_KILL | PROJECT_LOS), -1);
	}

	return TRUE;
}


/*!
 * @brief プレイヤー位置を中心にLITE_WEAK属性を通じた照明処理を行う / Hack -- call light around the player Affect all monsters in the projection radius
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dam 威力
 * @param rad 効果半径
 * @return 作用が実際にあった場合TRUEを返す
 */
bool lite_area(player_type *caster_ptr, HIT_POINT dam, POSITION rad)
{
	BIT_FLAGS flg = PROJECT_GRID | PROJECT_KILL;

	if (d_info[caster_ptr->dungeon_idx].flags1 & DF1_DARKNESS)
	{
		msg_print(_("ダンジョンが光を吸収した。", "The darkness of this dungeon absorb your light."));
		return FALSE;
	}

	if (!caster_ptr->blind)
	{
		msg_print(_("白い光が辺りを覆った。", "You are surrounded by a white light."));
	}

	/* Hook into the "project()" function */
	(void)project(caster_ptr, 0, rad, caster_ptr->y, caster_ptr->x, dam, GF_LITE_WEAK, flg, -1);

	lite_room(caster_ptr, caster_ptr->y, caster_ptr->x);

	/* Assume seen */
	return (TRUE);
}


/*!
 * @brief プレイヤー位置を中心にLITE_DARK属性を通じた消灯処理を行う / Hack -- call light around the player Affect all monsters in the projection radius
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dam 威力
 * @param rad 効果半径
 * @return 作用が実際にあった場合TRUEを返す
 */
bool unlite_area(player_type *caster_ptr, HIT_POINT dam, POSITION rad)
{
	BIT_FLAGS flg = PROJECT_GRID | PROJECT_KILL;

	if (!caster_ptr->blind)
	{
		msg_print(_("暗闇が辺りを覆った。", "Darkness surrounds you."));
	}

	/* Hook into the "project()" function */
	(void)project(caster_ptr, 0, rad, caster_ptr->y, caster_ptr->x, dam, GF_DARK_WEAK, flg, -1);

	unlite_room(caster_ptr, caster_ptr->y, caster_ptr->x);

	/* Assume seen */
	return (TRUE);
}


/*!
 * @brief ボール系スペルの発動 / Cast a ball spell
 * @param caster_ptr プレーヤーへの参照ポインタ
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
bool fire_ball(player_type *caster_ptr, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, POSITION rad)
{
	POSITION tx, ty;

	BIT_FLAGS flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;

	if (typ == GF_CHARM_LIVING) flg|= PROJECT_HIDE;
	/* Use the given direction */
	tx = caster_ptr->x + 99 * ddx[dir];
	ty = caster_ptr->y + 99 * ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay())
	{
		flg &= ~(PROJECT_STOP);
		tx = target_col;
		ty = target_row;
	}

	/* Analyze the "dir" and the "target".  Hurt items on floor. */
	return (project(caster_ptr, 0, rad, ty, tx, dam, typ, flg, -1));
}


/*!
* @brief ブレス系スペルの発動 / Cast a breath spell
* @param caster_ptr プレーヤーへの参照ポインタ
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
bool fire_breath(player_type *caster_ptr, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, POSITION rad)
{
	return fire_ball(caster_ptr, typ, dir, dam, -rad);
}


/*!
 * @brief ロケット系スペルの発動(詳細な差は確認中) / Cast a ball spell
 * @param caster_ptr プレーヤーへの参照ポインタ
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
bool fire_rocket(player_type *caster_ptr, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, POSITION rad)
{
	POSITION tx, ty;
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;

	/* Use the given direction */
	tx = caster_ptr->x + 99 * ddx[dir];
	ty = caster_ptr->y + 99 * ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay())
	{
		tx = target_col;
		ty = target_row;
	}

	/* Analyze the "dir" and the "target".  Hurt items on floor. */
	return (project(caster_ptr, 0, rad, ty, tx, dam, typ, flg, -1));
}


/*!
 * @brief ボール(ハイド)系スペルの発動 / Cast a ball spell
 * @param caster_ptr プレーヤーへの参照ポインタ
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
bool fire_ball_hide(player_type *caster_ptr, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, POSITION rad)
{
	POSITION tx, ty;
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_HIDE;

	/* Use the given direction */
	tx = caster_ptr->x + 99 * ddx[dir];
	ty = caster_ptr->y + 99 * ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay())
	{
		flg &= ~(PROJECT_STOP);
		tx = target_col;
		ty = target_row;
	}

	/* Analyze the "dir" and the "target".  Hurt items on floor. */
	return (project(caster_ptr, 0, rad, ty, tx, dam, typ, flg, -1));
}


/*!
 * @brief メテオ系スペルの発動 / Cast a meteor spell
 * @param caster_ptr プレーヤーへの参照ポインタ
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
bool fire_meteor(player_type *caster_ptr, MONSTER_IDX who, EFFECT_ID typ, POSITION y, POSITION x, HIT_POINT dam, POSITION rad)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;

	/* Analyze the "target" and the caster. */
	return (project(caster_ptr, who, rad, y, x, dam, typ, flg, -1));
}


/*!
 * @brief ブラスト系スペルの発動 / Cast a blast spell
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param typ 効果属性
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dd 威力ダイス数
 * @param ds 威力ダイス目
 * @param num 基本回数
 * @param dev 回数分散
 * @return 作用が実際にあった場合TRUEを返す
 */
bool fire_blast(player_type *caster_ptr, EFFECT_ID typ, DIRECTION dir, DICE_NUMBER dd, DICE_SID ds, int num, int dev)
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
		ly = ty = caster_ptr->y + 20 * ddy[dir];
		lx = tx = caster_ptr->x + 20 * ddx[dir];
	}

	/* Use an actual "target" */
	else /* if (dir == 5) */
	{
		tx = target_col;
		ty = target_row;

		lx = 20 * (tx - caster_ptr->x) + caster_ptr->x;
		ly = 20 * (ty - caster_ptr->y) + caster_ptr->y;
	}

	ld = distance(caster_ptr->y, caster_ptr->x, ly, lx);

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
		if (!project(caster_ptr, 0, 0, y, x, damroll(dd, ds), typ, flg, -1))
		{
			result = FALSE;
		}
	}

	return (result);
}


/*!
 * @brief モンスターとの位置交換処理 / Switch position with a monster.
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool teleport_swap(player_type *caster_ptr, DIRECTION dir)
{
	POSITION tx, ty;
	grid_type* g_ptr;
	monster_type* m_ptr;
	monster_race* r_ptr;

	if ((dir == 5) && target_okay())
	{
		tx = target_col;
		ty = target_row;
	}
	else
	{
		tx = caster_ptr->x + ddx[dir];
		ty = caster_ptr->y + ddy[dir];
	}
	g_ptr = &caster_ptr->current_floor_ptr->grid_array[ty][tx];

	if (caster_ptr->anti_tele)
	{
		msg_print(_("不思議な力がテレポートを防いだ！", "A mysterious force prevents you from teleporting!"));
		return FALSE;
	}

	if (!g_ptr->m_idx || (g_ptr->m_idx == caster_ptr->riding))
	{
		msg_print(_("それとは場所を交換できません。", "You can't trade places with that!"));
		return FALSE;
	}

	if ((g_ptr->info & CAVE_ICKY) || (distance(ty, tx, caster_ptr->y, caster_ptr->x) > caster_ptr->lev * 3 / 2 + 10))
	{
		msg_print(_("失敗した。", "Failed to swap."));
		return FALSE;
	}

	m_ptr = &caster_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
	r_ptr = &r_info[m_ptr->r_idx];

	(void)set_monster_csleep(g_ptr->m_idx, 0);

	if (r_ptr->flagsr & RFR_RES_TELE)
	{
		msg_print(_("テレポートを邪魔された！", "Your teleportation is blocked!"));
		if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= RFR_RES_TELE;
		return FALSE;
	}

	sound(SOUND_TELEPORT);

	/* Swap the player and monster */
	(void)move_player_effect(caster_ptr, ty, tx, MPE_FORGET_FLOW | MPE_HANDLE_STUFF | MPE_DONT_PICKUP);

	/* Success */
	return TRUE;
}


/*!
 * @brief 指定方向に飛び道具を飛ばす（フラグ任意指定） / Hack -- apply a "project()" in a direction (or at the target)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param typ 効果属性
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @param flg フラグ
 * @return 作用が実際にあった場合TRUEを返す
 */
bool project_hook(player_type *caster_ptr, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam, BIT_FLAGS flg)
{
	POSITION tx, ty;

	/* Pass through the target if needed */
	flg |= (PROJECT_THRU);

	/* Use the given direction */
	tx = caster_ptr->x + ddx[dir];
	ty = caster_ptr->y + ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay())
	{
		tx = target_col;
		ty = target_row;
	}

	/* Analyze the "dir" and the "target", do NOT explode */
	return (project(caster_ptr, 0, 0, ty, tx, dam, typ, flg, -1));
}


/*!
 * @brief ボルト系スペルの発動 / Cast a bolt spell.
 * @param caster_ptr プレーヤーへの参照ポインタ
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
bool fire_bolt(player_type *caster_ptr, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_GRID;
	if (typ != GF_ARROW) flg |= PROJECT_REFLECTABLE;
	return (project_hook(caster_ptr, typ, dir, dam, flg));
}


/*!
 * @brief ビーム系スペルの発動 / Cast a beam spell.
 * @param caster_ptr プレーヤーへの参照ポインタ
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
bool fire_beam(player_type *caster_ptr, EFFECT_ID typ, DIRECTION dir, HIT_POINT dam)
{
	BIT_FLAGS flg = PROJECT_BEAM | PROJECT_KILL | PROJECT_GRID | PROJECT_ITEM;
	return (project_hook(caster_ptr, typ, dir, dam, flg));
}


/*!
 * @brief 確率に応じたボルト系/ビーム系スペルの発動 / Cast a bolt spell, or rarely, a beam spell.
 * @param caster_ptr プレーヤーへの参照ポインタ
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
		return (fire_beam(p_ptr, typ, dir, dam));
	}
	else
	{
		return (fire_bolt(p_ptr, typ, dir, dam));
	}
}


/*!
 * @brief LITE_WEAK属性による光源ビーム処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool lite_line(DIRECTION dir, HIT_POINT dam)
{
	BIT_FLAGS flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_KILL;
	return (project_hook(p_ptr, GF_LITE_WEAK, dir, dam, flg));
}


/*!
 * @brief 衰弱ボルト処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool hypodynamic_bolt(DIRECTION dir, HIT_POINT dam)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(p_ptr, GF_HYPODYNAMIA, dir, dam, flg));
}


/*!
 * @brief 岩石溶解処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool wall_to_mud(DIRECTION dir, HIT_POINT dam)
{
	BIT_FLAGS flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
	return (project_hook(p_ptr, GF_KILL_WALL, dir, dam, flg));
}


/*!
 * @brief 魔法の施錠処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool wizard_lock(DIRECTION dir)
{
	BIT_FLAGS flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
	return (project_hook(p_ptr, GF_JAM_DOOR, dir, 20 + randint1(30), flg));
}


/*!
 * @brief ドア破壊処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool destroy_door(DIRECTION dir)
{
	BIT_FLAGS flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM;
	return (project_hook(p_ptr, GF_KILL_DOOR, dir, 0, flg));
}


/*!
 * @brief トラップ解除処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool disarm_trap(DIRECTION dir)
{
	BIT_FLAGS flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_ITEM;
	return (project_hook(p_ptr, GF_KILL_TRAP, dir, 0, flg));
}


/*!
 * @brief 死の光線処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev プレイヤーレベル(効力はplev*200)
 * @return 作用が実際にあった場合TRUEを返す
 */
bool death_ray(DIRECTION dir, PLAYER_LEVEL plev)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL | PROJECT_REFLECTABLE;
	return (project_hook(p_ptr, GF_DEATH_RAY, dir, plev * 200, flg));
}


/*!
 * @brief モンスター用テレポート処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param distance 移動距離
 * @return 作用が実際にあった場合TRUEを返す
 */
bool teleport_monster(DIRECTION dir, int distance)
{
	BIT_FLAGS flg = PROJECT_BEAM | PROJECT_KILL;
	return (project_hook(p_ptr, GF_AWAY_ALL, dir, distance, flg));
}


/*!
 * @brief ドア生成処理(プレイヤー中心に周囲1マス) / Hooks -- affect adjacent grids (radius 1 ball attack)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 作用が実際にあった場合TRUEを返す
 */
bool door_creation(player_type *caster_ptr, POSITION y, POSITION x)
{
	BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
	return (project(caster_ptr, 0, 1, y, x, 0, GF_MAKE_DOOR, flg, -1));
}


/*!
 * @brief トラップ生成処理(起点から周囲1マス)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param y 起点Y座標
 * @param x 起点X座標
 * @return 作用が実際にあった場合TRUEを返す
 */
bool trap_creation(player_type *caster_ptr, POSITION y, POSITION x)
{
	BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
	return (project(caster_ptr, 0, 1, y, x, 0, GF_MAKE_TRAP, flg, -1));
}


/*!
 * @brief 森林生成処理(プレイヤー中心に周囲1マス)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 作用が実際にあった場合TRUEを返す
 */
bool tree_creation(player_type *caster_ptr, POSITION y, POSITION x)
{
	BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
	return (project(caster_ptr, 0, 1, y, x, 0, GF_MAKE_TREE, flg, -1));
}


/*!
 * @brief 魔法のルーン生成処理(プレイヤー中心に周囲1マス)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 作用が実際にあった場合TRUEを返す
 */
bool glyph_creation(player_type *caster_ptr, POSITION y, POSITION x)
{
	BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM;
	return (project(caster_ptr, 0, 1, y, x, 0, GF_MAKE_GLYPH, flg, -1));
}


/*!
 * @brief 壁生成処理(プレイヤー中心に周囲1マス)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 作用が実際にあった場合TRUEを返す
 */
bool wall_stone(void)
{
	BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
	bool dummy = (project(p_ptr, 0, 1, p_ptr->y, p_ptr->x, 0, GF_STONE_WALL, flg, -1));
	p_ptr->update |= (PU_FLOW);
	p_ptr->redraw |= (PR_MAP);
	return dummy;
}


/*!
 * @brief ドア破壊処理(プレイヤー中心に周囲1マス)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 作用が実際にあった場合TRUEを返す
 */
bool destroy_doors_touch(void)
{
	BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
	return (project(p_ptr, 0, 1, p_ptr->y, p_ptr->x, 0, GF_KILL_DOOR, flg, -1));
}


/*!
 * @brief トラップ解除処理(プレイヤー中心に周囲1マス)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 作用が実際にあった場合TRUEを返す
 */
bool disarm_traps_touch(void)
{
	BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
	return (project(p_ptr, 0, 1, p_ptr->y, p_ptr->x, 0, GF_KILL_TRAP, flg, -1));
}


/*!
 * @brief スリープモンスター処理(プレイヤー中心に周囲1マス)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 作用が実際にあった場合TRUEを返す
 */
bool sleep_monsters_touch(void)
{
	BIT_FLAGS flg = PROJECT_KILL | PROJECT_HIDE;
	return (project(p_ptr, 0, 1, p_ptr->y, p_ptr->x, p_ptr->lev, GF_OLD_SLEEP, flg, -1));
}


/*!
 * @brief 死者復活処理(起点より周囲5マス)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param who 術者モンスターID(0ならばプレイヤー)
 * @param y 起点Y座標
 * @param x 起点X座標
 * @return 作用が実際にあった場合TRUEを返す
 */
bool animate_dead(MONSTER_IDX who, POSITION y, POSITION x)
{
	BIT_FLAGS flg = PROJECT_ITEM | PROJECT_HIDE;
	return (project(p_ptr, who, 5, y, x, 0, GF_ANIM_DEAD, flg, -1));
}


/*!
 * @brief 混沌招来処理
 * @return 作用が実際にあった場合TRUEを返す
 */
void call_chaos(player_type *caster_ptr)
{
	int Chaos_type, dummy, dir;
	PLAYER_LEVEL plev = caster_ptr->lev;
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
					fire_beam(caster_ptr, Chaos_type, dummy, 150);
				else
					fire_ball(caster_ptr, Chaos_type, dummy, 150, 2);
			}
		}
	}
	else if (one_in_(3))
	{
		fire_ball(caster_ptr, Chaos_type, 0, 500, 8);
	}
	else
	{
		if (!get_aim_dir(&dir)) return;
		if (line_chaos)
			fire_beam(caster_ptr, Chaos_type, dir, 250);
		else
			fire_ball(caster_ptr, Chaos_type, dir, 250, 3 + (plev / 35));
	}
}


/*!
 * @brief TY_CURSE処理発動 / Activate the evil Topi Ylinen curse
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param stop_ty 再帰処理停止フラグ
 * @param count 発動回数
 * @return 作用が実際にあった場合TRUEを返す
 * @details
 * <pre>
 * rr9: Stop the nasty things when a Cyberdemon is summoned
 * or the player gets paralyzed.
 * </pre>
 */
bool activate_ty_curse(player_type *target_ptr, bool stop_ty, int *count)
{
	BIT_FLAGS flg = (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP);

	bool is_first_curse = TRUE;
	while (is_first_curse || one_in_(3) && !stop_ty)
	{
		is_first_curse = FALSE;
		switch (randint1(34))
		{
		case 28: case 29:
			if (!(*count))
			{
				msg_print(_("地面が揺れた...", "The ground trembles..."));
				earthquake(target_ptr, target_ptr->y, target_ptr->x, 5 + randint0(10), 0);
				if (!one_in_(6)) break;
			}
		case 30: case 31:
			if (!(*count))
			{
				HIT_POINT dam = damroll(10, 10);
				msg_print(_("純粋な魔力の次元への扉が開いた！", "A portal opens to a plane of raw mana!"));
				project(target_ptr, 0, 8, target_ptr->y, target_ptr->x, dam, GF_MANA, flg, -1);
				take_hit(target_ptr, DAMAGE_NOESCAPE, dam, _("純粋な魔力の解放", "released pure mana"), -1);
				if (!one_in_(6)) break;
			}
		case 32: case 33:
			if (!(*count))
			{
				msg_print(_("周囲の空間が歪んだ！", "Space warps about you!"));
				teleport_player(target_ptr, damroll(10, 10), TELEPORT_PASSIVE);
				if (randint0(13)) (*count) += activate_hi_summon(target_ptr->y, target_ptr->x, FALSE);
				if (!one_in_(6)) break;
			}
		case 34:
			msg_print(_("エネルギーのうねりを感じた！", "You feel a surge of energy!"));
			wall_breaker(target_ptr);
			if (!randint0(7))
			{
				project(target_ptr, 0, 7, target_ptr->y, target_ptr->x, 50, GF_KILL_WALL, flg, -1);
				take_hit(target_ptr, DAMAGE_NOESCAPE, 50, _("エネルギーのうねり", "surge of energy"), -1);
			}

			if (!one_in_(6)) break;
		case 1: case 2: case 3: case 16: case 17:
			aggravate_monsters(target_ptr, 0);
			if (!one_in_(6)) break;
		case 4: case 5: case 6:
			(*count) += activate_hi_summon(target_ptr->y, target_ptr->x, FALSE);
			if (!one_in_(6)) break;
		case 7: case 8: case 9: case 18:
			(*count) += summon_specific(0, target_ptr->y, target_ptr->x, target_ptr->current_floor_ptr->dun_level, 0, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET));
			if (!one_in_(6)) break;
		case 10: case 11: case 12:
			msg_print(_("経験値が体から吸い取られた気がする！", "You feel your experience draining away..."));
			lose_exp(target_ptr, target_ptr->exp / 16);
			if (!one_in_(6)) break;
		case 13: case 14: case 15: case 19: case 20:
			if (stop_ty || (target_ptr->free_act && (randint1(125) < target_ptr->skill_sav)) || (target_ptr->pclass == CLASS_BERSERKER))
			{
				/* Do nothing */ ;
			}
			else
			{
				msg_print(_("彫像になった気分だ！", "You feel like a statue!"));
				if (target_ptr->free_act)
					set_paralyzed(target_ptr, target_ptr->paralyzed + randint1(3));
				else
					set_paralyzed(target_ptr, target_ptr->paralyzed + randint1(13));
				stop_ty = TRUE;
			}

			if (!one_in_(6)) break;
		case 21: case 22: case 23:
			(void)do_dec_stat(target_ptr, randint0(6));
			if (!one_in_(6)) break;
		case 24:
			msg_print(_("ほえ？私は誰？ここで何してる？", "Huh? Who am I? What am I doing here?"));
			lose_all_info(target_ptr);
			if (!one_in_(6)) break;
		case 25:
			/*
			 * Only summon Cyberdemons deep in the dungeon.
			 */
			if ((target_ptr->current_floor_ptr->dun_level > 65) && !stop_ty)
			{
				(*count) += summon_cyber(-1, target_ptr->y, target_ptr->x);
				stop_ty = TRUE;
				break;
			}

			if (!one_in_(6)) break;
		default:
			for (int i = 0; i < A_MAX; i++)
			{
				bool is_first_dec_stat = TRUE;
				while (is_first_dec_stat || one_in_(2))
				{
					is_first_dec_stat = FALSE;
					(void)do_dec_stat(target_ptr, i);
				}
			}
		}
	}

	return stop_ty;
}


/*!
 * @brief HI_SUMMON(上級召喚)処理発動
 * @param caster_ptr プレーヤーへの参照ポインタ
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

	summon_lev = (pet ? p_ptr->lev * 2 / 3 + randint1(p_ptr->lev / 2) : p_ptr->current_floor_ptr->dun_level);

	for (i = 0; i < (randint1(7) + (p_ptr->current_floor_ptr->dun_level / 40)); i++)
	{
		switch (randint1(25) + (p_ptr->current_floor_ptr->dun_level / 20))
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
 * @brief 周辺破壊効果(プレイヤー中心)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 作用が実際にあった場合TRUEを返す
 */
void wall_breaker(player_type *caster_ptr)
{
	int i;
	POSITION y = 0, x = 0;
	int attempts = 1000;

	if (randint1(80 + caster_ptr->lev) < 70)
	{
		while (attempts--)
		{
			scatter(&y, &x, caster_ptr->y, caster_ptr->x, 4, 0);

			if (!cave_have_flag_bold(caster_ptr->current_floor_ptr, y, x, FF_PROJECT)) continue;

			if (!player_bold(caster_ptr, y, x)) break;
		}

		project(caster_ptr, 0, 0, y, x, 20 + randint1(30), GF_KILL_WALL,
				  (PROJECT_BEAM | PROJECT_THRU | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL), -1);
	}
	else if (randint1(100) > 30)
	{
		earthquake(caster_ptr, caster_ptr->y, caster_ptr->x, 1, 0);
	}
	else
	{
		int num = damroll(5, 3);

		for (i = 0; i < num; i++)
		{
			while (1)
			{
				scatter(&y, &x, caster_ptr->y, caster_ptr->x, 10, 0);

				if (!player_bold(caster_ptr, y, x)) break;
			}

			project(caster_ptr, 0, 0, y, x, 20 + randint1(30), GF_KILL_WALL,
					  (PROJECT_BEAM | PROJECT_THRU | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL), -1);
		}
	}
}


/*!
 * @brief パニック・モンスター効果(プレイヤー視界範囲内) / Confuse monsters
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool confuse_monsters(HIT_POINT dam)
{
	return (project_all_los(p_ptr, GF_OLD_CONF, dam));
}


/*!
 * @brief チャーム・モンスター効果(プレイヤー視界範囲内) / Charm monsters
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool charm_monsters(HIT_POINT dam)
{
	return (project_all_los(p_ptr, GF_CHARM, dam));
}


/*!
 * @brief 動物魅了効果(プレイヤー視界範囲内) / Charm Animals
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool charm_animals(HIT_POINT dam)
{
	return (project_all_los(p_ptr, GF_CONTROL_ANIMAL, dam));
}


/*!
 * @brief モンスター朦朧効果(プレイヤー視界範囲内) / Stun monsters
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool stun_monsters(HIT_POINT dam)
{
	return (project_all_los(p_ptr, GF_STUN, dam));
}


/*!
 * @brief モンスター停止効果(プレイヤー視界範囲内) / Stasis monsters
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool stasis_monsters(HIT_POINT dam)
{
	return (project_all_los(p_ptr, GF_STASIS, dam));
}


/*!
 * @brief モンスター精神攻撃効果(プレイヤー視界範囲内) / Mindblast monsters
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool mindblast_monsters(HIT_POINT dam)
{
	return (project_all_los(p_ptr, GF_PSI, dam));
}


/*!
 * @brief モンスター追放効果(プレイヤー視界範囲内) / Banish all monsters
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dist 効力（距離）
 * @return 作用が実際にあった場合TRUEを返す
 */
bool banish_monsters(int dist)
{
	return (project_all_los(p_ptr, GF_AWAY_ALL, dist));
}


/*!
 * @brief 邪悪退散効果(プレイヤー視界範囲内) / Turn evil
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool turn_evil(HIT_POINT dam)
{
	return (project_all_los(p_ptr, GF_TURN_EVIL, dam));
}


/*!
 * @brief 全モンスター退散効果(プレイヤー視界範囲内) / Turn everyone
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dam 効力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool turn_monsters(HIT_POINT dam)
{
	return (project_all_los(p_ptr, GF_TURN_ALL, dam));
}


/*!
 * @brief 死の光線(プレイヤー視界範囲内) / Death-ray all monsters (note: OBSCENELY powerful)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 作用が実際にあった場合TRUEを返す
 */
bool deathray_monsters(void)
{
	return (project_all_los(p_ptr, GF_DEATH_RAY, p_ptr->lev * 200));
}


/*!
 * @brief チャーム・モンスター(1体)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev パワー
 * @return 作用が実際にあった場合TRUEを返す
 */
bool charm_monster(DIRECTION dir, PLAYER_LEVEL plev)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(p_ptr, GF_CHARM, dir, plev, flg));
}


/*!
 * @brief アンデッド支配(1体)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev パワー
 * @return 作用が実際にあった場合TRUEを返す
 */
bool control_one_undead(DIRECTION dir, PLAYER_LEVEL plev)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(p_ptr, GF_CONTROL_UNDEAD, dir, plev, flg));
}


/*!
 * @brief 悪魔支配(1体)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev パワー
 * @return 作用が実際にあった場合TRUEを返す
 */
bool control_one_demon(DIRECTION dir, PLAYER_LEVEL plev)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(p_ptr, GF_CONTROL_DEMON, dir, plev, flg));
}


/*!
 * @brief 動物支配(1体)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param plev パワー
 * @return 作用が実際にあった場合TRUEを返す
 */
bool charm_animal(DIRECTION dir, PLAYER_LEVEL plev)
{
	BIT_FLAGS flg = PROJECT_STOP | PROJECT_KILL;
	return (project_hook(p_ptr, GF_CONTROL_ANIMAL, dir, plev, flg));
}


/*!
 * @brief 変わり身処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param success 判定成功上の処理ならばTRUE
 * @return 作用が実際にあった場合TRUEを返す
 */
bool kawarimi(player_type *caster_ptr, bool success)
{
	object_type forge;
	object_type *q_ptr = &forge;
	POSITION y, x;

	if (caster_ptr->is_dead) return FALSE;
	if (caster_ptr->confused || caster_ptr->blind || caster_ptr->paralyzed || caster_ptr->image) return FALSE;
	if (randint0(200) < caster_ptr->stun) return FALSE;

	if (!success && one_in_(3))
	{
		msg_print(_("失敗！逃げられなかった。", "Failed! You couldn't run away."));
		caster_ptr->special_defense &= ~(NINJA_KAWARIMI);
		caster_ptr->redraw |= (PR_STATUS);
		return FALSE;
	}

	y = caster_ptr->y;
	x = caster_ptr->x;

	teleport_player(caster_ptr, 10 + randint1(90), 0L);
	object_wipe(q_ptr);
	object_prep(q_ptr, lookup_kind(TV_STATUE, SV_WOODEN_STATUE));

	q_ptr->pval = MON_NINJA;
	(void)drop_near(q_ptr, -1, y, x);

	if (success) msg_print(_("攻撃を受ける前に素早く身をひるがえした。", "You have turned around just before the attack hit you."));
	else msg_print(_("失敗！攻撃を受けてしまった。", "Failed! You are hit by the attack."));

	caster_ptr->special_defense &= ~(NINJA_KAWARIMI);
	caster_ptr->redraw |= (PR_STATUS);

	/* Teleported */
	return TRUE;
}


/*!
 * @brief 入身処理 / "Rush Attack" routine for Samurai or Ninja
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param mdeath 目標モンスターが死亡したかを返す
 * @return 作用が実際にあった場合TRUEを返す /  Return value is for checking "done"
 */
bool rush_attack(player_type *attacker_ptr, bool *mdeath)
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
	tx = attacker_ptr->x + project_length * ddx[dir];
	ty = attacker_ptr->y + project_length * ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay())
	{
		tx = target_col;
		ty = target_row;
	}

	if (in_bounds(attacker_ptr->current_floor_ptr, ty, tx)) tm_idx = attacker_ptr->current_floor_ptr->grid_array[ty][tx].m_idx;

	path_n = project_path(attacker_ptr->current_floor_ptr, path_g, project_length, attacker_ptr->y, attacker_ptr->x, ty, tx, PROJECT_STOP | PROJECT_KILL);
	project_length = 0;

	/* No need to move */
	if (!path_n) return TRUE;

	/* Use ty and tx as to-move point */
	ty = attacker_ptr->y;
	tx = attacker_ptr->x;

	/* Project along the path */
	for (i = 0; i < path_n; i++)
	{
		monster_type *m_ptr;

		int ny = GRID_Y(path_g[i]);
		int nx = GRID_X(path_g[i]);

		if (cave_empty_bold(attacker_ptr->current_floor_ptr, ny, nx) && player_can_enter(attacker_ptr, attacker_ptr->current_floor_ptr->grid_array[ny][nx].feat, 0))
		{
			ty = ny;
			tx = nx;

			/* Go to next grid */
			continue;
		}

		if (!attacker_ptr->current_floor_ptr->grid_array[ny][nx].m_idx)
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
		if (!player_bold(attacker_ptr, ty, tx)) teleport_player_to(attacker_ptr, ty, tx, TELEPORT_NONMAGICAL);
		update_monster(attacker_ptr, attacker_ptr->current_floor_ptr->grid_array[ny][nx].m_idx, TRUE);

		/* Found a monster */
		m_ptr = &attacker_ptr->current_floor_ptr->m_list[attacker_ptr->current_floor_ptr->grid_array[ny][nx].m_idx];

		if (tm_idx != attacker_ptr->current_floor_ptr->grid_array[ny][nx].m_idx)
		{
#ifdef JP
			msg_format("%s%sが立ちふさがっている！", tm_idx ? "別の" : "", m_ptr->ml ? "モンスター" : "何か");
#else
			msg_format("There is %s in the way!", m_ptr->ml ? (tm_idx ? "another monster" : "a monster") : "someone");
#endif
		}
		else if (!player_bold(attacker_ptr, ty, tx))
		{
			/* Hold the monster name */
			GAME_TEXT m_name[MAX_NLEN];

			/* Get the monster name (BEFORE polymorphing) */
			monster_desc(m_name, m_ptr, 0);
			msg_format(_("素早く%sの懐に入り込んだ！", "You quickly jump in and attack %s!"), m_name);
		}

		if (!player_bold(attacker_ptr, ty, tx)) teleport_player_to(attacker_ptr, ty, tx, TELEPORT_NONMAGICAL);
		moved = TRUE;
		tmp_mdeath = py_attack(attacker_ptr, ny, nx, HISSATSU_NYUSIN);

		break;
	}

	if (!moved && !player_bold(attacker_ptr, ty, tx)) teleport_player_to(attacker_ptr, ty, tx, TELEPORT_NONMAGICAL);

	if (mdeath) *mdeath = tmp_mdeath;
	return TRUE;
}


/*!
 * @brief 全鏡の消去 / Remove all mirrors in this floor
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param explode 爆発処理を伴うならばTRUE
 * @return なし
 */
void remove_all_mirrors(player_type *caster_ptr, bool explode)
{
	POSITION x, y;

	for (x = 0; x < caster_ptr->current_floor_ptr->width; x++)
	{
		for (y = 0; y < caster_ptr->current_floor_ptr->height; y++)
		{
			if (is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[y][x]))
			{
				remove_mirror(caster_ptr, y, x);
				if (explode)
					project(caster_ptr, 0, 2, y, x, caster_ptr->lev / 2 + 5, GF_SHARDS,
						(PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI), -1);
			}
		}
	}
}


/*!
 * @brief 『一つの指輪』の効果処理 /
 * Hack -- activate the ring of power
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 発動の方向ID
 * @return なし
 */
void ring_of_power(player_type *caster_ptr, DIRECTION dir)
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
		(void)dec_stat(caster_ptr, A_STR, 50, TRUE);
		(void)dec_stat(caster_ptr, A_INT, 50, TRUE);
		(void)dec_stat(caster_ptr, A_WIS, 50, TRUE);
		(void)dec_stat(caster_ptr, A_DEX, 50, TRUE);
		(void)dec_stat(caster_ptr, A_CON, 50, TRUE);
		(void)dec_stat(caster_ptr, A_CHR, 50, TRUE);

		/* Lose some experience (permanently) */
		caster_ptr->exp -= (caster_ptr->exp / 4);
		caster_ptr->max_exp -= (caster_ptr->exp / 4);
		check_experience(caster_ptr);

		break;
	}

	case 3:
	{
		msg_print(_("あなたは強力なオーラに包み込まれた。", "You are surrounded by a powerful aura."));
		dispel_monsters(caster_ptr, 1000);
		break;
	}

	case 4:
	case 5:
	case 6:
	{
		fire_ball(caster_ptr, GF_MANA, dir, 600, 3);
		break;
	}

	case 7:
	case 8:
	case 9:
	case 10:
	{
		fire_bolt(caster_ptr, GF_MANA, dir, 500);
		break;
	}
	}
}


/*!
* @brief 運命の輪、並びにカオス的な効果の発動
* @param caster_ptr プレーヤーへの参照ポインタ
* @param spell ランダムな効果を選択するための基準ID
* @return なし
*/
void wild_magic(player_type *caster_ptr, int spell)
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
		teleport_player(caster_ptr, 10, TELEPORT_PASSIVE);
		break;
	case 4:
	case 5:
	case 6:
		teleport_player(caster_ptr, 100, TELEPORT_PASSIVE);
		break;
	case 7:
	case 8:
		teleport_player(caster_ptr, 200, TELEPORT_PASSIVE);
		break;
	case 9:
	case 10:
	case 11:
		unlite_area(caster_ptr, 10, 3);
		break;
	case 12:
	case 13:
	case 14:
		lite_area(caster_ptr, damroll(2, 3), 2);
		break;
	case 15:
		destroy_doors_touch();
		break;
	case 16: case 17:
		wall_breaker(caster_ptr);
	case 18:
		sleep_monsters_touch();
		break;
	case 19:
	case 20:
		trap_creation(caster_ptr, caster_ptr->y, caster_ptr->x);
		break;
	case 21:
	case 22:
		door_creation(caster_ptr, caster_ptr->y, caster_ptr->x);
		break;
	case 23:
	case 24:
	case 25:
		aggravate_monsters(caster_ptr, 0);
		break;
	case 26:
		earthquake(caster_ptr, caster_ptr->y, caster_ptr->x, 5, 0);
		break;
	case 27:
	case 28:
		(void)gain_mutation(caster_ptr, 0);
		break;
	case 29:
	case 30:
		apply_disenchant(caster_ptr, 1);
		break;
	case 31:
		lose_all_info(caster_ptr);
		break;
	case 32:
		fire_ball(caster_ptr, GF_CHAOS, 0, spell + 5, 1 + (spell / 10));
		break;
	case 33:
		wall_stone();
		break;
	case 34:
	case 35:
		while (counter++ < 8)
		{
			(void)summon_specific(0, caster_ptr->y, caster_ptr->x, (caster_ptr->current_floor_ptr->dun_level * 3) / 2, type, (PM_ALLOW_GROUP | PM_NO_PET));
		}
		break;
	case 36:
	case 37:
		activate_hi_summon(caster_ptr->y, caster_ptr->x, FALSE);
		break;
	case 38:
		(void)summon_cyber(-1, caster_ptr->y, caster_ptr->x);
		break;
	default:
	{
		int count = 0;
		(void)activate_ty_curse(caster_ptr, FALSE, &count);
		break;
	}
	}
}


/*!
* @brief カオス魔法「流星群」の処理としてプレイヤーを中心に隕石落下処理を10+1d10回繰り返す。
* / Drop 10+1d10 meteor ball at random places near the player
* @param caster_ptr プレーヤーへの参照ポインタ
* @param dam ダメージ
* @param rad 効力の半径
* @return なし
*/
void cast_meteor(player_type *caster_ptr, HIT_POINT dam, POSITION rad)
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

			x = caster_ptr->x - 8 + randint0(17);
			y = caster_ptr->y - 8 + randint0(17);

			dx = (caster_ptr->x > x) ? (caster_ptr->x - x) : (x - caster_ptr->x);
			dy = (caster_ptr->y > y) ? (caster_ptr->y - y) : (y - caster_ptr->y);

			/* Approximate distance */
			d = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));

			if (d >= 9) continue;

			if (!in_bounds(caster_ptr->current_floor_ptr, y, x) || !projectable(caster_ptr->current_floor_ptr, caster_ptr->y, caster_ptr->x, y, x)
				|| !cave_have_flag_bold(caster_ptr->current_floor_ptr, y, x, FF_PROJECT)) continue;

			/* Valid position */
			break;
		}

		if (count > 20) continue;

		project(caster_ptr, 0, rad, y, x, dam, GF_METEOR, PROJECT_KILL | PROJECT_JUMP | PROJECT_ITEM, -1);
	}
}


/*!
* @brief 破邪魔法「神の怒り」の処理としてターゲットを指定した後分解のボールを最大20回発生させる。
* @param caster_ptr プレーヤーへの参照ポインタ
* @param dam ダメージ
* @param rad 効力の半径
* @return ターゲットを指定し、実行したならばTRUEを返す。
*/
bool cast_wrath_of_the_god(player_type *caster_ptr, HIT_POINT dam, POSITION rad)
{
	POSITION x, y, tx, ty;
	POSITION nx, ny;
	DIRECTION dir;
	int i;
	int b = 10 + randint1(10);

	if (!get_aim_dir(&dir)) return FALSE;

	/* Use the given direction */
	tx = caster_ptr->x + 99 * ddx[dir];
	ty = caster_ptr->y + 99 * ddy[dir];

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay())
	{
		tx = target_col;
		ty = target_row;
	}

	x = caster_ptr->x;
	y = caster_ptr->y;

	while (1)
	{
		/* Hack -- Stop at the target */
		if ((y == ty) && (x == tx)) break;

		ny = y;
		nx = x;
		mmove2(&ny, &nx, caster_ptr->y, caster_ptr->x, ty, tx);

		/* Stop at maximum range */
		if (MAX_RANGE <= distance(caster_ptr->y, caster_ptr->x, ny, nx)) break;

		/* Stopped by walls/doors */
		if (!cave_have_flag_bold(caster_ptr->current_floor_ptr, ny, nx, FF_PROJECT)) break;

		/* Stopped by monsters */
		if ((dir != 5) && caster_ptr->current_floor_ptr->grid_array[ny][nx].m_idx != 0) break;

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
		if (!in_bounds(caster_ptr->current_floor_ptr, y, x) ||
			cave_stop_disintegration(caster_ptr->current_floor_ptr, y, x) ||
			!in_disintegration_range(caster_ptr->current_floor_ptr, ty, tx, y, x))
			continue;

		project(caster_ptr, 0, rad, y, x, dam, GF_DISINTEGRATE, PROJECT_JUMP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL, -1);
	}

	return TRUE;
}

/*!
* @brief 「ワンダー」のランダムな効果を決定して処理する。
* @param caster_ptr プレーヤーへの参照ポインタ
* @param dir 方向ID
* @return なし
* @details
* This spell should become more useful (more controlled) as the\n
* player gains experience levels.  Thus, add 1/5 of the player's\n
* level to the die roll.  This eliminates the worst effects later on,\n
* while keeping the results quite random.  It also allows some potent\n
* effects only at high level.
*/
void cast_wonder(player_type *caster_ptr, DIRECTION dir)
{
	PLAYER_LEVEL plev = caster_ptr->lev;
	int die = randint1(100) + plev / 5;
	int vir = virtue_number(caster_ptr, V_CHANCE);

	if (vir)
	{
		if (caster_ptr->virtues[vir - 1] > 0)
		{
			while (randint1(400) < caster_ptr->virtues[vir - 1]) die++;
		}
		else
		{
			while (randint1(400) < (0 - caster_ptr->virtues[vir - 1])) die--;
		}
	}

	if (die < 26)
		chg_virtue(caster_ptr, V_CHANCE, 1);

	if (die > 100)
	{
		msg_print(_("あなたは力がみなぎるのを感じた！", "You feel a surge of power!"));
	}

	if (die < 8) clone_monster(dir);
	else if (die < 14) speed_monster(dir, plev);
	else if (die < 26) heal_monster(dir, damroll(4, 6));
	else if (die < 31) poly_monster(dir, plev);
	else if (die < 36)
		fire_bolt_or_beam(beam_chance(caster_ptr) - 10, GF_MISSILE, dir,
			damroll(3 + ((plev - 1) / 5), 4));
	else if (die < 41) confuse_monster(dir, plev);
	else if (die < 46) fire_ball(caster_ptr, GF_POIS, dir, 20 + (plev / 2), 3);
	else if (die < 51) (void)lite_line(dir, damroll(6, 8));
	else if (die < 56)
		fire_bolt_or_beam(beam_chance(caster_ptr) - 10, GF_ELEC, dir,
			damroll(3 + ((plev - 5) / 4), 8));
	else if (die < 61)
		fire_bolt_or_beam(beam_chance(caster_ptr) - 10, GF_COLD, dir,
			damroll(5 + ((plev - 5) / 4), 8));
	else if (die < 66)
		fire_bolt_or_beam(beam_chance(caster_ptr), GF_ACID, dir,
			damroll(6 + ((plev - 5) / 4), 8));
	else if (die < 71)
		fire_bolt_or_beam(beam_chance(caster_ptr), GF_FIRE, dir,
			damroll(8 + ((plev - 5) / 4), 8));
	else if (die < 76) hypodynamic_bolt(dir, 75);
	else if (die < 81) fire_ball(caster_ptr, GF_ELEC, dir, 30 + plev / 2, 2);
	else if (die < 86) fire_ball(caster_ptr, GF_ACID, dir, 40 + plev, 2);
	else if (die < 91) fire_ball(caster_ptr, GF_ICE, dir, 70 + plev, 3);
	else if (die < 96) fire_ball(caster_ptr, GF_FIRE, dir, 80 + plev, 3);
	else if (die < 101) hypodynamic_bolt(dir, 100 + plev);
	else if (die < 104)
	{
		earthquake(caster_ptr, caster_ptr->y, caster_ptr->x, 12, 0);
	}
	else if (die < 106)
	{
		(void)destroy_area(caster_ptr->current_floor_ptr, caster_ptr->y, caster_ptr->x, 13 + randint0(5), FALSE);
	}
	else if (die < 108)
	{
		symbol_genocide(caster_ptr, plev + 50, TRUE);
	}
	else if (die < 110)
	{
		dispel_monsters(caster_ptr, 120);
	}
	else /* RARE */
	{
		dispel_monsters(caster_ptr, 150);
		slow_monsters(caster_ptr, plev);
		sleep_monsters(caster_ptr, plev);
		hp_player(caster_ptr, 300);
	}
}


/*!
* @brief 「悪霊召喚」のランダムな効果を決定して処理する。
* @param caster_ptr プレーヤーへの参照ポインタ
* @param dir 方向ID
* @return なし
*/
void cast_invoke_spirits(player_type *caster_ptr, DIRECTION dir)
{
	PLAYER_LEVEL plev = caster_ptr->lev;
	int die = randint1(100) + plev / 5;
	int vir = virtue_number(caster_ptr, V_CHANCE);

	if (vir)
	{
		if (caster_ptr->virtues[vir - 1] > 0)
		{
			while (randint1(400) < caster_ptr->virtues[vir - 1]) die++;
		}
		else
		{
			while (randint1(400) < (0 - caster_ptr->virtues[vir - 1])) die--;
		}
	}

	msg_print(_("あなたは死者たちの力を招集した...", "You call on the power of the dead..."));
	if (die < 26)
		chg_virtue(caster_ptr, V_CHANCE, 1);

	if (die > 100)
	{
		msg_print(_("あなたはおどろおどろしい力のうねりを感じた！", "You feel a surge of eldritch force!"));
	}

	if (die < 8)
	{
		msg_print(_("なんてこった！あなたの周りの地面から朽ちた人影が立ち上がってきた！",
			"Oh no! Mouldering forms rise from the earth around you!"));

		(void)summon_specific(0, caster_ptr->y, caster_ptr->x, caster_ptr->current_floor_ptr->dun_level, SUMMON_UNDEAD, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET));
		chg_virtue(caster_ptr, V_UNLIFE, 1);
	}
	else if (die < 14)
	{
		msg_print(_("名状し難い邪悪な存在があなたの心を通り過ぎて行った...", "An unnamable evil brushes against your mind..."));

		set_afraid(caster_ptr, caster_ptr->afraid + randint1(4) + 4);
	}
	else if (die < 26)
	{
		msg_print(_("あなたの頭に大量の幽霊たちの騒々しい声が押し寄せてきた...",
			"Your head is invaded by a horde of gibbering spectral voices..."));

		set_confused(caster_ptr, caster_ptr->confused + randint1(4) + 4);
	}
	else if (die < 31)
	{
		poly_monster(dir, plev);
	}
	else if (die < 36)
	{
		fire_bolt_or_beam(beam_chance(caster_ptr) - 10, GF_MISSILE, dir,
			damroll(3 + ((plev - 1) / 5), 4));
	}
	else if (die < 41)
	{
		confuse_monster(dir, plev);
	}
	else if (die < 46)
	{
		fire_ball(caster_ptr, GF_POIS, dir, 20 + (plev / 2), 3);
	}
	else if (die < 51)
	{
		(void)lite_line(dir, damroll(6, 8));
	}
	else if (die < 56)
	{
		fire_bolt_or_beam(beam_chance(caster_ptr) - 10, GF_ELEC, dir,
			damroll(3 + ((plev - 5) / 4), 8));
	}
	else if (die < 61)
	{
		fire_bolt_or_beam(beam_chance(caster_ptr) - 10, GF_COLD, dir,
			damroll(5 + ((plev - 5) / 4), 8));
	}
	else if (die < 66)
	{
		fire_bolt_or_beam(beam_chance(caster_ptr), GF_ACID, dir,
			damroll(6 + ((plev - 5) / 4), 8));
	}
	else if (die < 71)
	{
		fire_bolt_or_beam(beam_chance(caster_ptr), GF_FIRE, dir,
			damroll(8 + ((plev - 5) / 4), 8));
	}
	else if (die < 76)
	{
		hypodynamic_bolt(dir, 75);
	}
	else if (die < 81)
	{
		fire_ball(caster_ptr, GF_ELEC, dir, 30 + plev / 2, 2);
	}
	else if (die < 86)
	{
		fire_ball(caster_ptr, GF_ACID, dir, 40 + plev, 2);
	}
	else if (die < 91)
	{
		fire_ball(caster_ptr, GF_ICE, dir, 70 + plev, 3);
	}
	else if (die < 96)
	{
		fire_ball(caster_ptr, GF_FIRE, dir, 80 + plev, 3);
	}
	else if (die < 101)
	{
		hypodynamic_bolt(dir, 100 + plev);
	}
	else if (die < 104)
	{
		earthquake(caster_ptr, caster_ptr->y, caster_ptr->x, 12, 0);
	}
	else if (die < 106)
	{
		(void)destroy_area(caster_ptr->current_floor_ptr, caster_ptr->y, caster_ptr->x, 13 + randint0(5), FALSE);
	}
	else if (die < 108)
	{
		symbol_genocide(caster_ptr, plev + 50, TRUE);
	}
	else if (die < 110)
	{
		dispel_monsters(caster_ptr, 120);
	}
	else
	{ /* RARE */
		dispel_monsters(caster_ptr, 150);
		slow_monsters(caster_ptr, plev);
		sleep_monsters(caster_ptr, plev);
		hp_player(caster_ptr, 300);
	}

	if (die < 31)
	{
		msg_print(_("陰欝な声がクスクス笑う。「もうすぐおまえは我々の仲間になるだろう。弱き者よ。」",
			"Sepulchral voices chuckle. 'Soon you will join us, mortal.'"));
	}
}

/*!
* @brief トランプ領域の「シャッフル」の効果をランダムに決めて処理する。
* @param caster_ptr プレーヤーへの参照ポインタ
* @return なし
*/
void cast_shuffle(player_type *caster_ptr)
{
	PLAYER_LEVEL plev = caster_ptr->lev;
	DIRECTION dir;
	int die;
	int vir = virtue_number(caster_ptr, V_CHANCE);
	int i;

	/* Card sharks and high mages get a level bonus */
	if ((caster_ptr->pclass == CLASS_ROGUE) ||
		(caster_ptr->pclass == CLASS_HIGH_MAGE) ||
		(caster_ptr->pclass == CLASS_SORCERER))
		die = (randint1(110)) + plev / 5;
	else
		die = randint1(120);
	
	if (vir)
	{
		if (caster_ptr->virtues[vir - 1] > 0)
		{
			while (randint1(400) < caster_ptr->virtues[vir - 1]) die++;
		}
		else
		{
			while (randint1(400) < (0 - caster_ptr->virtues[vir - 1])) die--;
		}
	}

	msg_print(_("あなたはカードを切って一枚引いた...", "You shuffle the deck and draw a card..."));

	if (die < 30)
		chg_virtue(caster_ptr, V_CHANCE, 1);

	if (die < 7)
	{
		msg_print(_("なんてこった！《死》だ！", "Oh no! It's Death!"));

		for (i = 0; i < randint1(3); i++)
			activate_hi_summon(caster_ptr->y, caster_ptr->x, FALSE);
	}
	else if (die < 14)
	{
		msg_print(_("なんてこった！《悪魔》だ！", "Oh no! It's the Devil!"));
		summon_specific(0, caster_ptr->y, caster_ptr->x, caster_ptr->current_floor_ptr->dun_level, SUMMON_DEMON, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET));
	}
	else if (die < 18)
	{
		int count = 0;
		msg_print(_("なんてこった！《吊られた男》だ！", "Oh no! It's the Hanged Man."));
		activate_ty_curse(caster_ptr, FALSE, &count);
	}
	else if (die < 22)
	{
		msg_print(_("《不調和の剣》だ。", "It's the swords of discord."));
		aggravate_monsters(caster_ptr, 0);
	}
	else if (die < 26)
	{
		msg_print(_("《愚者》だ。", "It's the Fool."));
		do_dec_stat(caster_ptr, A_INT);
		do_dec_stat(caster_ptr, A_WIS);
	}
	else if (die < 30)
	{
		msg_print(_("奇妙なモンスターの絵だ。", "It's the picture of a strange monster."));
		trump_summoning(caster_ptr, 1, FALSE, caster_ptr->y, caster_ptr->x, (caster_ptr->current_floor_ptr->dun_level * 3 / 2), (32 + randint1(6)), PM_ALLOW_GROUP | PM_ALLOW_UNIQUE);
	}
	else if (die < 33)
	{
		msg_print(_("《月》だ。", "It's the Moon."));
		unlite_area(caster_ptr, 10, 3);
	}
	else if (die < 38)
	{
		msg_print(_("《運命の輪》だ。", "It's the Wheel of Fortune."));
		wild_magic(caster_ptr, randint0(32));
	}
	else if (die < 40)
	{
		msg_print(_("テレポート・カードだ。", "It's a teleport trump card."));
		teleport_player(caster_ptr, 10, TELEPORT_PASSIVE);
	}
	else if (die < 42)
	{
		msg_print(_("《正義》だ。", "It's Justice."));
		set_blessed(caster_ptr, caster_ptr->lev, FALSE);
	}
	else if (die < 47)
	{
		msg_print(_("テレポート・カードだ。", "It's a teleport trump card."));
		teleport_player(caster_ptr, 100, TELEPORT_PASSIVE);
	}
	else if (die < 52)
	{
		msg_print(_("テレポート・カードだ。", "It's a teleport trump card."));
		teleport_player(caster_ptr, 200, TELEPORT_PASSIVE);
	}
	else if (die < 60)
	{
		msg_print(_("《塔》だ。", "It's the Tower."));
		wall_breaker(caster_ptr);
	}
	else if (die < 72)
	{
		msg_print(_("《節制》だ。", "It's Temperance."));
		sleep_monsters_touch();
	}
	else if (die < 80)
	{
		msg_print(_("《塔》だ。", "It's the Tower."));

		earthquake(caster_ptr, caster_ptr->y, caster_ptr->x, 5, 0);
	}
	else if (die < 82)
	{
		msg_print(_("友好的なモンスターの絵だ。", "It's the picture of a friendly monster."));
		trump_summoning(caster_ptr, 1, TRUE, caster_ptr->y, caster_ptr->x, (caster_ptr->current_floor_ptr->dun_level * 3 / 2), SUMMON_MOLD, 0L);
	}
	else if (die < 84)
	{
		msg_print(_("友好的なモンスターの絵だ。", "It's the picture of a friendly monster."));
		trump_summoning(caster_ptr, 1, TRUE, caster_ptr->y, caster_ptr->x, (caster_ptr->current_floor_ptr->dun_level * 3 / 2), SUMMON_BAT, 0L);
	}
	else if (die < 86)
	{
		msg_print(_("友好的なモンスターの絵だ。", "It's the picture of a friendly monster."));
		trump_summoning(caster_ptr, 1, TRUE, caster_ptr->y, caster_ptr->x, (caster_ptr->current_floor_ptr->dun_level * 3 / 2), SUMMON_VORTEX, 0L);
	}
	else if (die < 88)
	{
		msg_print(_("友好的なモンスターの絵だ。", "It's the picture of a friendly monster."));
		trump_summoning(caster_ptr, 1, TRUE, caster_ptr->y, caster_ptr->x, (caster_ptr->current_floor_ptr->dun_level * 3 / 2), SUMMON_COIN_MIMIC, 0L);
	}
	else if (die < 96)
	{
		msg_print(_("《恋人》だ。", "It's the Lovers."));

		if (get_aim_dir(&dir))
			charm_monster(dir, MIN(caster_ptr->lev, 20));
	}
	else if (die < 101)
	{
		msg_print(_("《隠者》だ。", "It's the Hermit."));
		wall_stone();
	}
	else if (die < 111)
	{
		msg_print(_("《審判》だ。", "It's the Judgement."));
		roll_hitdice(caster_ptr, 0L);
		lose_all_mutations(caster_ptr);
	}
	else if (die < 120)
	{
		msg_print(_("《太陽》だ。", "It's the Sun."));
		chg_virtue(caster_ptr, V_KNOWLEDGE, 1);
		chg_virtue(caster_ptr, V_ENLIGHTEN, 1);
		wiz_lite(caster_ptr, FALSE);
	}
	else
	{
		msg_print(_("《世界》だ。", "It's the World."));
		if (caster_ptr->exp < PY_MAX_EXP)
		{
			s32b ee = (caster_ptr->exp / 25) + 1;
			if (ee > 5000) ee = 5000;
			msg_print(_("更に経験を積んだような気がする。", "You feel more experienced."));
			gain_exp(caster_ptr, ee);
		}
	}
}

/*!
 * @brief 口を使う継続的な処理を中断する
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void stop_mouth(player_type *caster_ptr)
{
	if (music_singing_any(caster_ptr)) stop_singing(caster_ptr);
	if (hex_spelling_any(caster_ptr)) stop_hex_spell_all(caster_ptr);
}


bool_hack vampirism(player_type *caster_ptr)
{
	DIRECTION dir;
	POSITION x, y;
	int dummy;
	grid_type *g_ptr;

	if (d_info[caster_ptr->dungeon_idx].flags1 & DF1_NO_MELEE)
	{
		msg_print(_("なぜか攻撃することができない。", "Something prevent you from attacking."));
		return FALSE;
	}

	/* Only works on adjacent monsters */
	if (!get_direction(caster_ptr, &dir, FALSE, FALSE)) return FALSE;
	y = caster_ptr->y + ddy[dir];
	x = caster_ptr->x + ddx[dir];
	g_ptr = &caster_ptr->current_floor_ptr->grid_array[y][x];

	stop_mouth(caster_ptr);

	if (!(g_ptr->m_idx))
	{
		msg_print(_("何もない場所に噛みついた！", "You bite into thin air!"));
		return FALSE;
	}

	msg_print(_("あなたはニヤリとして牙をむいた...", "You grin and bare your fangs..."));

	dummy = caster_ptr->lev * 2;

	if (hypodynamic_bolt(dir, dummy))
	{
		if (caster_ptr->food < PY_FOOD_FULL)
			/* No heal if we are "full" */
			(void)hp_player(caster_ptr, dummy);
		else
			msg_print(_("あなたは空腹ではありません。", "You were not hungry."));

		/* Gain nutritional sustenance: 150/hp drained */
		/* A Food ration gives 5000 food points (by contrast) */
		/* Don't ever get more than "Full" this way */
		/* But if we ARE Gorged,  it won't cure us */
		dummy = caster_ptr->food + MIN(5000, 100 * dummy);
		if (caster_ptr->food < PY_FOOD_MAX)   /* Not gorged already */
			(void)set_food(caster_ptr, dummy >= PY_FOOD_MAX ? PY_FOOD_MAX - 1 : dummy);
	}
	else
		msg_print(_("げぇ！ひどい味だ。", "Yechh. That tastes foul."));
	return TRUE;
}


bool panic_hit(void)
{
	DIRECTION dir;
	POSITION x, y;

	if (!get_direction(p_ptr, &dir, FALSE, FALSE)) return FALSE;
	y = p_ptr->y + ddy[dir];
	x = p_ptr->x + ddx[dir];
	if (p_ptr->current_floor_ptr->grid_array[y][x].m_idx)
	{
		py_attack(p_ptr, y, x, 0);
		if (randint0(p_ptr->skill_dis) < 7)
			msg_print(_("うまく逃げられなかった。", "You failed to run away."));
		else
			teleport_player(p_ptr, 30, 0L);
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
* @param caster_ptr プレーヤーへの参照ポインタ
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
bool psychometry(player_type *caster_ptr)
{
	OBJECT_IDX      item;
	object_type *o_ptr;
	GAME_TEXT o_name[MAX_NLEN];
	byte            feel;
	concptr            q, s;
	bool okay = FALSE;

	q = _("どのアイテムを調べますか？", "Meditate on which item? ");
	s = _("調べるアイテムがありません。", "You have nothing appropriate.");

	o_ptr = choose_object(caster_ptr, &item, q, s, (USE_EQUIP | USE_INVEN | USE_FLOOR | IGNORE_BOTHHAND_SLOT), 0);
	if (!o_ptr) return (FALSE);

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
	msg_format("%sは%sという感じがする...", o_name, game_inscriptions[feel]);
#else
	msg_format("You feel that the %s %s %s...",
		o_name, ((o_ptr->number == 1) ? "is" : "are"), game_inscriptions[feel]);
#endif


	o_ptr->ident |= (IDENT_SENSE);
	o_ptr->feeling = feel;
	o_ptr->marked |= OM_TOUCHED;

	caster_ptr->update |= (PU_COMBINE | PU_REORDER);
	caster_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

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


bool draconian_breath(player_type *creature_ptr)
{
	DIRECTION dir;
	int Type = (one_in_(3) ? GF_COLD : GF_FIRE);
	concptr Type_desc = ((Type == GF_COLD) ? _("冷気", "cold") : _("炎", "fire"));

	if (!get_aim_dir(&dir)) return FALSE;

	if (randint1(100) < creature_ptr->lev)
	{
		switch (creature_ptr->pclass)
		{
		case CLASS_WARRIOR:
		case CLASS_BERSERKER:
		case CLASS_RANGER:
		case CLASS_TOURIST:
		case CLASS_IMITATOR:
		case CLASS_ARCHER:
		case CLASS_SMITH:
			if (one_in_(3))
			{
				Type = GF_MISSILE;
				Type_desc = _("エレメント", "the elements");
			}
			else
			{
				Type = GF_SHARDS;
				Type_desc = _("破片", "shards");
			}

			break;
		case CLASS_MAGE:
		case CLASS_WARRIOR_MAGE:
		case CLASS_HIGH_MAGE:
		case CLASS_SORCERER:
		case CLASS_MAGIC_EATER:
		case CLASS_RED_MAGE:
		case CLASS_BLUE_MAGE:
		case CLASS_MIRROR_MASTER:
			if (one_in_(3))
			{
				Type = GF_MANA;
				Type_desc = _("魔力", "mana");
			}
			else
			{
				Type = GF_DISENCHANT;
				Type_desc = _("劣化", "disenchantment");
			}

			break;
		case CLASS_CHAOS_WARRIOR:
			if (!one_in_(3))
			{
				Type = GF_CONFUSION;
				Type_desc = _("混乱", "confusion");
			}
			else
			{
				Type = GF_CHAOS;
				Type_desc = _("カオス", "chaos");
			}

			break;
		case CLASS_MONK:
		case CLASS_SAMURAI:
		case CLASS_FORCETRAINER:
			if (!one_in_(3))
			{
				Type = GF_CONFUSION;
				Type_desc = _("混乱", "confusion");
			}
			else
			{
				Type = GF_SOUND;
				Type_desc = _("轟音", "sound");
			}

			break;
		case CLASS_MINDCRAFTER:
			if (!one_in_(3))
			{
				Type = GF_CONFUSION;
				Type_desc = _("混乱", "confusion");
			}
			else
			{
				Type = GF_PSI;
				Type_desc = _("精神エネルギー", "mental energy");
			}

			break;
		case CLASS_PRIEST:
		case CLASS_PALADIN:
			if (one_in_(3))
			{
				Type = GF_HELL_FIRE;
				Type_desc = _("地獄の劫火", "hellfire");
			}
			else
			{
				Type = GF_HOLY_FIRE;
				Type_desc = _("聖なる炎", "holy fire");
			}

			break;
		case CLASS_ROGUE:
		case CLASS_NINJA:
			if (one_in_(3))
			{
				Type = GF_DARK;
				Type_desc = _("暗黒", "darkness");
			}
			else
			{
				Type = GF_POIS;
				Type_desc = _("毒", "poison");
			}

			break;
		case CLASS_BARD:
			if (!one_in_(3))
			{
				Type = GF_SOUND;
				Type_desc = _("轟音", "sound");
			}
			else
			{
				Type = GF_CONFUSION;
				Type_desc = _("混乱", "confusion");
			}

			break;
		}
	}

	stop_mouth(creature_ptr);
	msg_format(_("あなたは%sのブレスを吐いた。", "You breathe %s."), Type_desc);

	fire_breath(creature_ptr, Type, dir, creature_ptr->lev * 2, (creature_ptr->lev / 15) + 1);
	return TRUE;
}


bool android_inside_weapon(player_type *creature_ptr)
{
	DIRECTION dir;
	if (!get_aim_dir(&dir)) return FALSE;
	if (creature_ptr->lev < 10)
	{
		msg_print(_("レイガンを発射した。", "You fire your ray gun."));
		fire_bolt(creature_ptr, GF_MISSILE, dir, (creature_ptr->lev + 1) / 2);
	}
	else if (creature_ptr->lev < 25)
	{
		msg_print(_("ブラスターを発射した。", "You fire your blaster."));
		fire_bolt(creature_ptr, GF_MISSILE, dir, creature_ptr->lev);
	}
	else if (creature_ptr->lev < 35)
	{
		msg_print(_("バズーカを発射した。", "You fire your bazooka."));
		fire_ball(creature_ptr, GF_MISSILE, dir, creature_ptr->lev * 2, 2);
	}
	else if (creature_ptr->lev < 45)
	{
		msg_print(_("ビームキャノンを発射した。", "You fire a beam cannon."));
		fire_beam(creature_ptr, GF_MISSILE, dir, creature_ptr->lev * 2);
	}
	else
	{
		msg_print(_("ロケットを発射した。", "You fire a rocket."));
		fire_rocket(creature_ptr, GF_ROCKET, dir, creature_ptr->lev * 5, 2);
	}

	return TRUE;
}


bool create_ration(player_type *crature_ptr)
{
	object_type *q_ptr;
	object_type forge;
	q_ptr = &forge;

	/* Create the food ration */
	object_prep(q_ptr, lookup_kind(TV_FOOD, SV_FOOD_RATION));

	/* Drop the object from heaven */
	(void)drop_near(q_ptr, -1, crature_ptr->y, crature_ptr->x);
	msg_print(_("食事を料理して作った。", "You cook some food."));
	return TRUE;
}


void hayagake(player_type *creature_ptr)
{
	if (creature_ptr->action == ACTION_HAYAGAKE)
	{
		set_action(creature_ptr, ACTION_NONE);
	}
	else
	{
		grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x];
		feature_type *f_ptr = &f_info[g_ptr->feat];

		if (!have_flag(f_ptr->flags, FF_PROJECT) ||
			(!creature_ptr->levitation && have_flag(f_ptr->flags, FF_DEEP)))
		{
			msg_print(_("ここでは素早く動けない。", "You cannot run in here."));
		}
		else
		{
			set_action(creature_ptr, ACTION_HAYAGAKE);
		}
	}

	creature_ptr->energy_use = 0;
}


bool double_attack(player_type *creature_ptr)
{
	DIRECTION dir;
	POSITION x, y;

	if (!get_rep_dir(&dir, FALSE)) return FALSE;
	y = creature_ptr->y + ddy[dir];
	x = creature_ptr->x + ddx[dir];
	if (creature_ptr->current_floor_ptr->grid_array[y][x].m_idx)
	{
		if (one_in_(3))
			msg_print(_("あーたたたたたたたたたたたたたたたたたたたたたた！！！",
				"Ahhhtatatatatatatatatatatatatatataatatatatattaaaaa!!!!"));
		else if(one_in_(2))
			msg_print(_("無駄無駄無駄無駄無駄無駄無駄無駄無駄無駄無駄無駄！！！",
				"Mudamudamudamudamudamudamudamudamudamudamudamudamuda!!!!"));
		else
			msg_print(_("オラオラオラオラオラオラオラオラオラオラオラオラ！！！",
				"Oraoraoraoraoraoraoraoraoraoraoraoraoraoraoraoraora!!!!"));

		py_attack(creature_ptr, y, x, 0);
		if (creature_ptr->current_floor_ptr->grid_array[y][x].m_idx)
		{
			handle_stuff();
			py_attack(creature_ptr, y, x, 0);
		}
		creature_ptr->energy_need += ENERGY_NEED();
	}
	else
	{
		msg_print(_("その方向にはモンスターはいません。", "You don't see any monster in this direction"));
		msg_print(NULL);
	}

	return TRUE;
}


bool comvert_hp_to_mp(player_type *creature_ptr)
{
	int gain_sp = take_hit(p_ptr, DAMAGE_USELIFE, creature_ptr->lev, _("ＨＰからＭＰへの無謀な変換", "thoughtless convertion from HP to SP"), -1) / 5;
	if (gain_sp)
	{
		creature_ptr->csp += gain_sp;
		if (creature_ptr->csp > creature_ptr->msp)
		{
			creature_ptr->csp = creature_ptr->msp;
			creature_ptr->csp_frac = 0;
		}
	}
	else
	{
		msg_print(_("変換に失敗した。", "You failed to convert."));
	}

	creature_ptr->redraw |= (PR_HP | PR_MANA);
	return TRUE;
}


bool comvert_mp_to_hp(player_type *creature_ptr)
{
	if (creature_ptr->csp >= creature_ptr->lev / 5)
	{
		creature_ptr->csp -= creature_ptr->lev / 5;
		hp_player(p_ptr, creature_ptr->lev);
	}
	else
	{
		msg_print(_("変換に失敗した。", "You failed to convert."));
	}

	creature_ptr->redraw |= (PR_HP | PR_MANA);
	return TRUE;
}


bool demonic_breath(player_type *creature_ptr)
{
	DIRECTION dir;
	int type = (one_in_(2) ? GF_NETHER : GF_FIRE);
	if (!get_aim_dir(&dir)) return FALSE;
	stop_mouth(creature_ptr);
	msg_format(_("あなたは%sのブレスを吐いた。", "You breathe %s."), ((type == GF_NETHER) ? _("地獄", "nether") : _("火炎", "fire")));
	fire_breath(creature_ptr, type, dir, creature_ptr->lev * 3, (creature_ptr->lev / 15) + 1);
	return TRUE;
}


bool mirror_concentration(player_type *creature_ptr)
{
	if (total_friends)
	{
		msg_print(_("今はペットを操ることに集中していないと。", "You need concentration on the pets now."));
		return FALSE;
	}

	if (is_mirror_grid(&p_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x]))
	{
		msg_print(_("少し頭がハッキリした。", "You feel your head clear a little."));

		creature_ptr->csp += (5 + creature_ptr->lev * creature_ptr->lev / 100);
		if (creature_ptr->csp >= creature_ptr->msp)
		{
			creature_ptr->csp = creature_ptr->msp;
			creature_ptr->csp_frac = 0;
		}
		creature_ptr->redraw |= (PR_MANA);
	}
	else
	{
		msg_print(_("鏡の上でないと集中できない！", "Here are not any mirrors!"));
	}

	return TRUE;
}


bool sword_dancing(player_type *creature_ptr)
{
	DIRECTION dir;
	POSITION y = 0, x = 0;
	int i;
	grid_type *g_ptr;

	for (i = 0; i < 6; i++)
	{
		dir = randint0(8);
		y = creature_ptr->y + ddy_ddd[dir];
		x = creature_ptr->x + ddx_ddd[dir];
		g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];

		/* Hack -- attack monsters */
		if (g_ptr->m_idx)
			py_attack(creature_ptr, y, x, 0);
		else
		{
			msg_print(_("攻撃が空をきった。", "You attack the empty air."));
		}
	}

	return TRUE;
}

/*!
 * 幻惑の光
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return 常にTRUE
*/
bool confusing_light(player_type *creature_ptr)
{
	msg_print(_("辺りを睨んだ...", "You glare nearby monsters..."));
	slow_monsters(creature_ptr, creature_ptr->lev);
	stun_monsters(creature_ptr->lev * 4);
	confuse_monsters(creature_ptr->lev * 4);
	turn_monsters(creature_ptr->lev * 4);
	stasis_monsters(creature_ptr->lev * 4);
	return TRUE;
}


/*!
 * 荒馬慣らし
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return 結果はどうあれ騎乗したらTRUE
*/
bool rodeo(player_type *creature_ptr)
{
	GAME_TEXT m_name[MAX_NLEN];
	monster_type *m_ptr;
	monster_race *r_ptr;
	int rlev;

	if (creature_ptr->riding)
	{
		msg_print(_("今は乗馬中だ。", "You ARE riding."));
		return FALSE;
	}

	if (!do_cmd_riding(creature_ptr, TRUE)) return TRUE;

	m_ptr = &p_ptr->current_floor_ptr->m_list[creature_ptr->riding];
	r_ptr = &r_info[m_ptr->r_idx];
	monster_desc(m_name, m_ptr, 0);
	msg_format(_("%sに乗った。", "You ride on %s."), m_name);

	if (is_pet(m_ptr)) return TRUE;

	rlev = r_ptr->level;

	if (r_ptr->flags1 & RF1_UNIQUE) rlev = rlev * 3 / 2;
	if (rlev > 60) rlev = 60 + (rlev - 60) / 2;
	if ((randint1(creature_ptr->skill_exp[GINOU_RIDING] / 120 + creature_ptr->lev * 2 / 3) > rlev)
		&& one_in_(2) && !creature_ptr->current_floor_ptr->inside_arena && !creature_ptr->phase_out
		&& !(r_ptr->flags7 & (RF7_GUARDIAN)) && !(r_ptr->flags1 & (RF1_QUESTOR))
		&& (rlev < creature_ptr->lev * 3 / 2 + randint0(creature_ptr->lev / 5)))
	{
		msg_format(_("%sを手なずけた。", "You tame %s."), m_name);
		set_pet(m_ptr);
	}
	else
	{
		msg_format(_("%sに振り落とされた！", "You have thrown off by %s."), m_name);
		rakuba(p_ptr, 1, TRUE);
		/* 落馬処理に失敗してもとにかく乗馬解除 */
		creature_ptr->riding = 0;
	}

	return TRUE;
}


bool clear_mind(player_type *creature_ptr)
{
	if (total_friends)
	{
		msg_print(_("今はペットを操ることに集中していないと。", "You need concentration on the pets now."));
		return FALSE;
	}
	msg_print(_("少し頭がハッキリした。", "You feel your head clear a little."));

	creature_ptr->csp += (3 + creature_ptr->lev / 20);
	if (creature_ptr->csp >= creature_ptr->msp)
	{
		creature_ptr->csp = creature_ptr->msp;
		creature_ptr->csp_frac = 0;
	}

	creature_ptr->redraw |= (PR_MANA);
	return TRUE;
}


bool concentration(player_type *creature_ptr)
{
	int max_csp = MAX(creature_ptr->msp * 4, creature_ptr->lev * 5 + 5);

	if (total_friends)
	{
		msg_print(_("今はペットを操ることに集中していないと。", "You need concentration on the pets now."));
		return FALSE;
	}

	if (creature_ptr->special_defense & KATA_MASK)
	{
		msg_print(_("今は構えに集中している。", "You need concentration on your form."));
		return FALSE;
	}

	msg_print(_("精神を集中して気合いを溜めた。", "You concentrate to charge your power."));

	creature_ptr->csp += creature_ptr->msp / 2;
	if (creature_ptr->csp >= max_csp)
	{
		creature_ptr->csp = max_csp;
		creature_ptr->csp_frac = 0;
	}

	creature_ptr->redraw |= (PR_MANA);
	return TRUE;
}
