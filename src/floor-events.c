#include "angband.h"
#include "floor.h"
#include "grid.h"
#include "monster.h"
#include "monster-status.h"
#include "quest.h"
#include "object-hook.h"
#include "player-move.h"

void day_break()
{
	POSITION y, x;
	msg_print(_("夜が明けた。", "The sun has risen."));

	if (!p_ptr->wild_mode)
	{
		/* Hack -- Scan the town */
		for (y = 0; y < current_floor_ptr->height; y++)
		{
			for (x = 0; x < current_floor_ptr->width; x++)
			{
				grid_type *g_ptr = &current_floor_ptr->grid_array[y][x];

				/* Assume lit */
				g_ptr->info |= (CAVE_GLOW);

				/* Hack -- Memorize lit grids if allowed */
				if (view_perma_grids) g_ptr->info |= (CAVE_MARK);

				/* Hack -- Notice spot */
				note_spot(y, x);
			}
		}
	}

	p_ptr->update |= (PU_MONSTERS | PU_MON_LITE);
	p_ptr->redraw |= (PR_MAP);
	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

	if (p_ptr->special_defense & NINJA_S_STEALTH)
	{
		if (current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].info & CAVE_GLOW) set_superstealth(FALSE);
	}

}

void night_falls(void)
{
	POSITION y, x;
	msg_print(_("日が沈んだ。", "The sun has fallen."));

	if (!p_ptr->wild_mode)
	{
		/* Hack -- Scan the town */
		for (y = 0; y < current_floor_ptr->height; y++)
		{
			for (x = 0; x < current_floor_ptr->width; x++)
			{
				grid_type *g_ptr = &current_floor_ptr->grid_array[y][x];

				/* Feature code (applying "mimic" field) */
				feature_type *f_ptr = &f_info[get_feat_mimic(g_ptr)];

				if (!is_mirror_grid(g_ptr) && !have_flag(f_ptr->flags, FF_QUEST_ENTER) &&
					!have_flag(f_ptr->flags, FF_ENTRANCE))
				{
					/* Assume dark */
					g_ptr->info &= ~(CAVE_GLOW);

					if (!have_flag(f_ptr->flags, FF_REMEMBER))
					{
						/* Forget the normal floor grid */
						g_ptr->info &= ~(CAVE_MARK);

						/* Hack -- Notice spot */
						note_spot(y, x);
					}
				}
			}

			/* Glow deep lava and building entrances */
			glow_deep_lava_and_bldg();
		}
	}

	p_ptr->update |= (PU_MONSTERS | PU_MON_LITE);
	p_ptr->redraw |= (PR_MAP);
	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

	if (p_ptr->special_defense & NINJA_S_STEALTH)
	{
		if (current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].info & CAVE_GLOW) set_superstealth(FALSE);
	}

}

/*!
 * @brief 現在フロアに残っている敵モンスターの数を返す /
 * @return 現在の敵モンスターの数
 */
MONSTER_NUMBER count_all_hostile_monsters(void)
{
	POSITION x, y;
	MONSTER_NUMBER number_mon = 0;

	for (x = 0; x < current_floor_ptr->width; ++x)
	{
		for (y = 0; y < current_floor_ptr->height; ++y)
		{
			MONSTER_IDX m_idx = current_floor_ptr->grid_array[y][x].m_idx;

			if (m_idx > 0 && is_hostile(&current_floor_ptr->m_list[m_idx]))
			{
				++number_mon;
			}
		}
	}

	return number_mon;
}



/*!
 * ダンジョンの雰囲気を計算するための非線形基準値 / Dungeon rating is no longer linear
 */
#define RATING_BOOST(delta) (delta * delta + 50 * delta)

 /*!
  * @brief ダンジョンの雰囲気を算出する。
  * / Examine all monsters and unidentified objects, and get the feeling of current dungeon floor
  * @return 算出されたダンジョンの雰囲気ランク
  */
byte get_dungeon_feeling(void)
{
	const int base = 10;
	int rating = 0;
	IDX i;

	/* Hack -- no feeling in the town */
	if (!current_floor_ptr->dun_level) return 0;

	/* Examine each monster */
	for (i = 1; i < m_max; i++)
	{
		monster_type *m_ptr = &current_floor_ptr->m_list[i];
		monster_race *r_ptr;
		int delta = 0;
		if (!monster_is_valid(m_ptr)) continue;

		if (is_pet(m_ptr)) continue;

		r_ptr = &r_info[m_ptr->r_idx];

		if (r_ptr->flags1 & (RF1_UNIQUE))
		{
			/* Nearly out-of-depth unique monsters */
			if (r_ptr->level + 10 > current_floor_ptr->dun_level)
			{
				/* Boost rating by twice delta-depth */
				delta += (r_ptr->level + 10 - current_floor_ptr->dun_level) * 2 * base;
			}
		}
		else
		{
			/* Out-of-depth monsters */
			if (r_ptr->level > current_floor_ptr->dun_level)
			{
				/* Boost rating by delta-depth */
				delta += (r_ptr->level - current_floor_ptr->dun_level) * base;
			}
		}

		/* Unusually crowded monsters get a little bit of rating boost */
		if (r_ptr->flags1 & RF1_FRIENDS)
		{
			if (5 <= get_monster_crowd_number(i)) delta += 1;
		}
		else
		{
			if (2 <= get_monster_crowd_number(i)) delta += 1;
		}


		rating += RATING_BOOST(delta);
	}

	/* Examine each unidentified object */
	for (i = 1; i < o_max; i++)
	{
		object_type *o_ptr = &current_floor_ptr->o_list[i];
		object_kind *k_ptr = &k_info[o_ptr->k_idx];
		int delta = 0;

		/* Skip dead objects */
		if (!o_ptr->k_idx) continue;

		/* Skip known objects */
		if (object_is_known(o_ptr))
		{
			/* Touched? */
			if (o_ptr->marked & OM_TOUCHED) continue;
		}

		/* Skip pseudo-known objects */
		if (o_ptr->ident & IDENT_SENSE) continue;

		/* Ego objects */
		if (object_is_ego(o_ptr))
		{
			ego_item_type *e_ptr = &e_info[o_ptr->name2];

			delta += e_ptr->rating * base;
		}

		/* Artifacts */
		if (object_is_artifact(o_ptr))
		{
			PRICE cost = object_value_real(o_ptr);

			delta += 10 * base;
			if (cost > 10000L) delta += 10 * base;
			if (cost > 50000L) delta += 10 * base;
			if (cost > 100000L) delta += 10 * base;

			/* Special feeling */
			if (!preserve_mode) return 1;
		}

		if (o_ptr->tval == TV_DRAG_ARMOR) delta += 30 * base;
		if (o_ptr->tval == TV_SHIELD && o_ptr->sval == SV_DRAGON_SHIELD) delta += 5 * base;
		if (o_ptr->tval == TV_GLOVES && o_ptr->sval == SV_SET_OF_DRAGON_GLOVES) delta += 5 * base;
		if (o_ptr->tval == TV_BOOTS && o_ptr->sval == SV_PAIR_OF_DRAGON_GREAVE) delta += 5 * base;
		if (o_ptr->tval == TV_HELM && o_ptr->sval == SV_DRAGON_HELM) delta += 5 * base;
		if (o_ptr->tval == TV_RING && o_ptr->sval == SV_RING_SPEED && !object_is_cursed(o_ptr)) delta += 25 * base;
		if (o_ptr->tval == TV_RING && o_ptr->sval == SV_RING_LORDLY && !object_is_cursed(o_ptr)) delta += 15 * base;
		if (o_ptr->tval == TV_AMULET && o_ptr->sval == SV_AMULET_THE_MAGI && !object_is_cursed(o_ptr)) delta += 15 * base;

		/* Out-of-depth objects */
		if (!object_is_cursed(o_ptr) && !object_is_broken(o_ptr) && k_ptr->level > current_floor_ptr->dun_level)
		{
			/* Rating increase */
			delta += (k_ptr->level - current_floor_ptr->dun_level) * base;
		}

		rating += RATING_BOOST(delta);
	}


	if (rating > RATING_BOOST(1000)) return 2;
	if (rating > RATING_BOOST(800)) return 3;
	if (rating > RATING_BOOST(600)) return 4;
	if (rating > RATING_BOOST(400)) return 5;
	if (rating > RATING_BOOST(300)) return 6;
	if (rating > RATING_BOOST(200)) return 7;
	if (rating > RATING_BOOST(100)) return 8;
	if (rating > RATING_BOOST(0)) return 9;

	return 10;
}

/*!
 * @brief ダンジョンの雰囲気を更新し、変化があった場合メッセージを表示する
 * / Update dungeon feeling, and announce it if changed
 * @return なし
 */
void update_dungeon_feeling(void)
{
	byte new_feeling;
	int quest_num;
	int delay;

	/* No feeling on the surface */
	if (!current_floor_ptr->dun_level) return;

	/* No feeling in the arena */
	if (p_ptr->inside_battle) return;

	/* Extract delay time */
	delay = MAX(10, 150 - p_ptr->skill_fos) * (150 - current_floor_ptr->dun_level) * TURNS_PER_TICK / 100;

	/* Not yet felt anything */
	if (current_world_ptr->game_turn < p_ptr->feeling_turn + delay && !cheat_xtra) return;

	/* Extract quest number (if any) */
	quest_num = quest_number(current_floor_ptr->dun_level);

	/* No feeling in a quest */
	if (quest_num &&
		(is_fixed_quest_idx(quest_num) &&
			!((quest_num == QUEST_OBERON) || (quest_num == QUEST_SERPENT) ||
				!(quest[quest_num].flags & QUEST_FLAG_PRESET)))) return;


	/* Get new dungeon feeling */
	new_feeling = get_dungeon_feeling();

	/* Remember last time updated */
	p_ptr->feeling_turn = current_world_ptr->game_turn;

	/* No change */
	if (p_ptr->feeling == new_feeling) return;

	/* Dungeon feeling is changed */
	p_ptr->feeling = new_feeling;

	/* Announce feeling */
	do_cmd_feeling();

	select_floor_music();

	/* Update the level indicator */
	p_ptr->redraw |= (PR_DEPTH);

	if (disturb_minor) disturb(FALSE, FALSE);
}