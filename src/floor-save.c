/*!
 * @file floors.c
 * @brief 保存された階の管理 / management of the saved floor
 * @date 2014/01/04
 * @author
 * Copyright (c) 2002  Mogami \n
 * This software may be copied and distributed for educational, research, and \n
 * not for profit purposes provided that this copyright and statement are \n
 * included in all such copies. \n
 * 2014 Deskull rearranged comment for Doxygen. \n
 */

#include "angband.h"
#include "bldg.h"
#include "core.h"
#include "load.h"
#include "util.h"

#include "artifact.h"
#include "dungeon.h"
#include "floor.h"
#include "floor-events.h"
#include "floor-generate.h"
#include "feature.h"
#include "grid.h"
#include "monster.h"
#include "quest.h"
#include "wild.h"
#include "spells-floor.h"
#include "monster-status.h"
#include "object-hook.h"
#include "cmd-pet.h"
#include "cmd-basic.h"
#include "files.h"
#include "player-effects.h"
#include "player-class.h"
#include "player-personality.h"
#include "world.h"
#include "spells.h"
#include "cmd-dump.h"
#include "save.h"

#include "view-mainwindow.h"


static FLOOR_IDX new_floor_id;  /*!<次のフロアのID / floor_id of the destination */
static u32b latest_visit_mark;  /*!<フロアを渡った回数？(確認中) / Max number of visit_mark */

/*
 * Number of floor_id used from birth
 */
FLOOR_IDX max_floor_id;

/*
 * Sign for current process used in temporal files.
 * Actually it is the start time of current process.
 */
u32b saved_floor_file_sign;

/*!
 * @brief 保存フロア配列を初期化する / Initialize saved_floors array. 
 * @param force テンポラリファイルが残っていた場合も警告なしで強制的に削除する。
 * @details Make sure that old temporal files are not remaining as gurbages.
 * @return なし
 */
void init_saved_floors(bool force)
{
	char floor_savefile[1024];
	int i;
	int fd = -1;
	BIT_FLAGS mode = 0644;

#ifdef SET_UID
# ifdef SECURE
	/* Get "games" permissions */
	beGames();
# endif
#endif

	for (i = 0; i < MAX_SAVED_FLOORS; i++)
	{
		saved_floor_type *sf_ptr = &saved_floors[i];

		/* File name */
		sprintf(floor_savefile, "%s.F%02d", savefile, i);

		/* Grab permissions */
		safe_setuid_grab();

		/* Try to create the file */
		fd = fd_make(floor_savefile, mode);

		/* Drop permissions */
		safe_setuid_drop();

		/* Failed! */
		if (fd < 0)
		{
			if (!force)
			{
				msg_print(_("エラー：古いテンポラリ・ファイルが残っています。", "Error: There are old temporal files."));
				msg_print(_("変愚蛮怒を二重に起動していないか確認してください。", "Make sure you are not running two game processes simultaneously."));
				msg_print(_("過去に変愚蛮怒がクラッシュした場合は一時ファイルを", "If the temporal files are garbages of old crashed process, "));
				msg_print(_("強制的に削除して実行を続けられます。", "you can delete it safely."));
				if (!get_check(_("強制的に削除してもよろしいですか？", "Do you delete old temporal files? ")))
					quit(_("実行中止", "Aborted."));
				force = TRUE;
			}
		}
		else
		{
			/* Close the "fd" */
			(void)fd_close(fd);
		}

		/* Grab permissions */
		safe_setuid_grab();

		/* Simply kill the temporal file */ 
		(void)fd_kill(floor_savefile);

		/* Drop permissions */
		safe_setuid_drop();

		sf_ptr->floor_id = 0;
	}

	/* No floor_id used yet (No.0 is reserved to indicate non existance) */
	max_floor_id = 1;

	/* vist_mark is from 1 */
	latest_visit_mark = 1;

	/* A sign to mark temporal files */
	saved_floor_file_sign = (u32b)time(NULL);

	/* No next floor yet */
	new_floor_id = 0;

	/* No change floor mode yet */
	p_ptr->change_floor_mode = 0;

#ifdef SET_UID
# ifdef SECURE
	/* Drop "games" permissions */
	bePlayer();
# endif
#endif
}

/*!
 * @brief 保存フロア用テンポラリファイルを削除する / Kill temporal files
 * @details Should be called just before the game quit.
 * @return なし
 */
void clear_saved_floor_files(void)
{
	char floor_savefile[1024];
	int i;

#ifdef SET_UID
# ifdef SECURE
	/* Get "games" permissions */
	beGames();
# endif
#endif

	for (i = 0; i < MAX_SAVED_FLOORS; i++)
	{
		saved_floor_type *sf_ptr = &saved_floors[i];

		/* No temporal file */
		if (!sf_ptr->floor_id) continue;
		if (sf_ptr->floor_id == p_ptr->floor_id) continue;

		/* File name */
		sprintf(floor_savefile, "%s.F%02d", savefile, i);

		/* Grab permissions */
		safe_setuid_grab();

		/* Simply kill the temporal file */ 
		(void)fd_kill(floor_savefile);

		/* Drop permissions */
		safe_setuid_drop();
	}

#ifdef SET_UID
# ifdef SECURE
	/* Drop "games" permissions */
	bePlayer();
# endif
#endif
}

/*!
 * @brief 保存フロアIDから参照ポインタを得る / Get a pointer for an item of the saved_floors array.
 * @param floor_id 保存フロアID
 * @return IDに対応する保存フロアのポインタ、ない場合はNULLを返す。
 */
saved_floor_type *get_sf_ptr(FLOOR_IDX floor_id)
{
	int i;

	/* floor_id No.0 indicates no floor */
	if (!floor_id) return NULL;

	for (i = 0; i < MAX_SAVED_FLOORS; i++)
	{
		saved_floor_type *sf_ptr = &saved_floors[i];

		if (sf_ptr->floor_id == floor_id) return sf_ptr;
	}

	/* None found */
	return NULL;
}


/*!
 * @brief 参照ポインタ先の保存フロアを抹消する / kill a saved floor and get an empty space
 * @param sf_ptr 保存フロアの参照ポインタ
 * @return なし
 */
static void kill_saved_floor(saved_floor_type *sf_ptr)
{
	char floor_savefile[1024];
	if (!sf_ptr) return;

	/* Already empty */
	if (!sf_ptr->floor_id) return;

	if (sf_ptr->floor_id == p_ptr->floor_id)
	{
		/* Kill current floor */
		p_ptr->floor_id = 0;

		/* Current floor doesn't have temporal file */
	}
	else 
	{
		/* File name */
		sprintf(floor_savefile, "%s.F%02d", savefile, (int)sf_ptr->savefile_id);

		/* Grab permissions */
		safe_setuid_grab();

		/* Simply kill the temporal file */ 
		(void)fd_kill(floor_savefile);

		/* Drop permissions */
		safe_setuid_drop();
	}

	/* No longer exists */
	sf_ptr->floor_id = 0;
}


/*!
 * @brief 新規に利用可能な保存フロアを返す / Initialize new saved floor and get its floor id.
 * @return 利用可能な保存フロアID
 * @details
 * If number of saved floors are already MAX_SAVED_FLOORS, kill the oldest one.
 */
FLOOR_IDX get_new_floor_id(void)
{
	saved_floor_type *sf_ptr = NULL;
	FLOOR_IDX i;

	/* Look for empty space */
	for (i = 0; i < MAX_SAVED_FLOORS; i++)
	{
		sf_ptr = &saved_floors[i];

		if (!sf_ptr->floor_id) break;
	}

	/* None found */
	if (i == MAX_SAVED_FLOORS)
	{
		s16b oldest = 0;
		u32b oldest_visit = 0xffffffffL;

		/* Search for oldest */
		for (i = 0; i < MAX_SAVED_FLOORS; i++)
		{
			sf_ptr = &saved_floors[i];

			/* Don't kill current floor */
			if (sf_ptr->floor_id == p_ptr->floor_id) continue;

			/* Don't kill newer */
			if (sf_ptr->visit_mark > oldest_visit) continue;

			oldest = i;
			oldest_visit = sf_ptr->visit_mark;
		}

		/* Kill oldest saved floor */
		sf_ptr = &saved_floors[oldest];
		kill_saved_floor(sf_ptr);

		/* Use it */
		i = oldest;
	}

	/* Prepare new floor data */
	sf_ptr->savefile_id = i;
	sf_ptr->floor_id = max_floor_id;
	sf_ptr->last_visit = 0;
	sf_ptr->upper_floor_id = 0;
	sf_ptr->lower_floor_id = 0;
	sf_ptr->visit_mark = latest_visit_mark++;

	/* sf_ptr->dun_level may be changed later */
	sf_ptr->dun_level = current_floor_ptr->dun_level;


	/* Increment number of floor_id */
	if (max_floor_id < MAX_SHORT) max_floor_id++;

	/* 32767 floor_ids are all used up!  Re-use ancient IDs */
	else max_floor_id = 1;

	return sf_ptr->floor_id;
}


/*!
 * @brief フロア切り替え時の処理フラグを追加する / Prepare mode flags of changing floor
 * @param mode 追加したい所持フラグ
 * @return なし
 */
void prepare_change_floor_mode(BIT_FLAGS mode)
{
	p_ptr->change_floor_mode |= mode;
}

/*!
 * @brief 階段移動先のフロアが生成できない時に簡単な行き止まりマップを作成する / Builds the dead end
 * @return なし
 */
static void build_dead_end(void)
{
	POSITION x, y;

	/* Clear and empty the current_floor_ptr->grid_array */
	clear_cave();

	/* Fill the arrays of floors and walls in the good proportions */
	set_floor_and_wall(0);

	/* Smallest area */
	current_floor_ptr->height = SCREEN_HGT;
	current_floor_ptr->width = SCREEN_WID;

	/* Filled with permanent walls */
	for (y = 0; y < MAX_HGT; y++)
	{
		for (x = 0; x < MAX_WID; x++)
		{
			/* Create "solid" perma-wall */
			place_solid_perm_bold(y, x);
		}
	}

	/* Place at center of the floor */
	p_ptr->y = current_floor_ptr->height / 2;
	p_ptr->x = current_floor_ptr->width / 2;

	/* Give one square */
	place_floor_bold(p_ptr->y, p_ptr->x);

	wipe_generate_random_floor_flags();
}



#define MAX_PARTY_MON 21 /*!< フロア移動時に先のフロアに連れて行けるペットの最大数 Maximum number of preservable pets */
static monster_type party_mon[MAX_PARTY_MON]; /*!< フロア移動に保存するペットモンスターの配列 */

/*!
 * @brief フロア移動時のペット保存処理 / Preserve_pets
 * @return なし
 */
static void preserve_pet(void)
{
	int num;
	MONSTER_IDX i;

	for (num = 0; num < MAX_PARTY_MON; num++)
	{
		party_mon[num].r_idx = 0;
	}

	if (p_ptr->riding)
	{
		monster_type *m_ptr = &current_floor_ptr->m_list[p_ptr->riding];

		/* Pet of other pet don't follow. */
		if (m_ptr->parent_m_idx)
		{
			p_ptr->riding = 0;
			p_ptr->pet_extra_flags &= ~(PF_RYOUTE);
			p_ptr->riding_ryoute = p_ptr->old_riding_ryoute = FALSE;
		}
		else
		{
			/* Preserve the mount */
			(void)COPY(&party_mon[0], m_ptr, monster_type);

			/* Delete from this floor */
			delete_monster_idx(p_ptr->riding);
		}
	}

	/*
	 * If player is in wild mode, no pets are preserved
	 * except a monster whom player riding
	 */
	if (!p_ptr->wild_mode && !p_ptr->inside_arena && !p_ptr->phase_out)
	{
		for (i = current_floor_ptr->m_max - 1, num = 1; (i >= 1 && num < MAX_PARTY_MON); i--)
		{
			monster_type *m_ptr = &current_floor_ptr->m_list[i];

			if (!monster_is_valid(m_ptr)) continue;
			if (!is_pet(m_ptr)) continue;
			if (i == p_ptr->riding) continue;

			if (reinit_wilderness)
			{
				/* Don't lose sight of pets when getting a Quest */
			}
			else
			{
				POSITION dis = distance(p_ptr->y, p_ptr->x, m_ptr->fy, m_ptr->fx);

				/* Confused (etc.) monsters don't follow. */
				if (MON_CONFUSED(m_ptr) || MON_STUNNED(m_ptr) || MON_CSLEEP(m_ptr)) continue;

				/* Pet of other pet don't follow. */
				if (m_ptr->parent_m_idx) continue;

				/*
				 * Pets with nickname will follow even from 3 blocks away
				 * when you or the pet can see the other.
				 */
				if (m_ptr->nickname && 
				    ((player_has_los_bold(m_ptr->fy, m_ptr->fx) && projectable(p_ptr->y, p_ptr->x, m_ptr->fy, m_ptr->fx)) ||
				     (los(m_ptr->fy, m_ptr->fx, p_ptr->y, p_ptr->x) && projectable(m_ptr->fy, m_ptr->fx, p_ptr->y, p_ptr->x))))
				{
					if (dis > 3) continue;
				}
				else
				{
					if (dis > 1) continue;
				}
			}

			(void)COPY(&party_mon[num], &current_floor_ptr->m_list[i], monster_type);

			num++;

			/* Delete from this floor */
			delete_monster_idx(i);
		}
	}

	if (record_named_pet)
	{
		for (i = current_floor_ptr->m_max - 1; i >=1; i--)
		{
			monster_type *m_ptr = &current_floor_ptr->m_list[i];
			GAME_TEXT m_name[MAX_NLEN];

			if (!monster_is_valid(m_ptr)) continue;
			if (!is_pet(m_ptr)) continue;
			if (!m_ptr->nickname) continue;
			if (p_ptr->riding == i) continue;

			monster_desc(m_name, m_ptr, MD_ASSUME_VISIBLE | MD_INDEF_VISIBLE);
			exe_write_diary(p_ptr, NIKKI_NAMED_PET, RECORD_NAMED_PET_MOVED, m_name);
		}
	}


	/* Pet of other pet may disappear. */
	for (i = current_floor_ptr->m_max - 1; i >=1; i--)
	{
		monster_type *m_ptr = &current_floor_ptr->m_list[i];

		/* Are there its parent? */
		if (m_ptr->parent_m_idx && !current_floor_ptr->m_list[m_ptr->parent_m_idx].r_idx)
		{
			/* Its parent have gone, it also goes away. */

			if (is_seen(m_ptr))
			{
				GAME_TEXT m_name[MAX_NLEN];
				monster_desc(m_name, m_ptr, 0);
				msg_format(_("%sは消え去った！", "%^s disappears!"), m_name);
			}

			delete_monster_idx(i);
		}
	}
}


/*!
 * @brief フロア移動時にペットを伴った場合の準備処理 / Pre-calculate the racial counters of preserved pets
 * @return なし
 * @details
 * To prevent multiple generation of unique monster who is the minion of player
 */
void precalc_cur_num_of_pet(void)
{
	monster_type *m_ptr;
	int i;
	int max_num = p_ptr->wild_mode ? 1 : MAX_PARTY_MON;

	for (i = 0; i < max_num; i++)
	{
		m_ptr = &party_mon[i];

		/* Skip empty monsters */
		if (!monster_is_valid(m_ptr)) continue;

		/* Hack -- Increase the racial counter */
		real_r_ptr(m_ptr)->cur_num++;
	}
}

/*!
 * @brief 移動先のフロアに伴ったペットを配置する / Place preserved pet monsters on new floor
 * @return なし
 */
static void place_pet(void)
{
	int i;
	int max_num = p_ptr->wild_mode ? 1 : MAX_PARTY_MON;

	for (i = 0; i < max_num; i++)
	{
		POSITION cy = 0, cx = 0;
		MONSTER_IDX m_idx;

		if (!(party_mon[i].r_idx)) continue;

		if (i == 0)
		{
			m_idx = m_pop();
			p_ptr->riding = m_idx;
			if (m_idx)
			{
				cy = p_ptr->y;
				cx = p_ptr->x;
			}
		}
		else
		{
			int j;
			POSITION d;

			for (d = 1; d < A_MAX; d++)
			{
				for (j = 1000; j > 0; j--)
				{
					scatter(&cy, &cx, p_ptr->y, p_ptr->x, d, 0);
					if (monster_can_enter(cy, cx, &r_info[party_mon[i].r_idx], 0)) break;
				}
				if (j) break;
			}
			m_idx = (d == 6) ? 0 : m_pop();
		}

		if (m_idx)
		{
			monster_type *m_ptr = &current_floor_ptr->m_list[m_idx];
			monster_race *r_ptr;

			current_floor_ptr->grid_array[cy][cx].m_idx = m_idx;

			m_ptr->r_idx = party_mon[i].r_idx;

			/* Copy all member of the structure */
			*m_ptr = party_mon[i];
			r_ptr = real_r_ptr(m_ptr);

			m_ptr->fy = cy;
			m_ptr->fx = cx;
			m_ptr->ml = TRUE;
			m_ptr->mtimed[MTIMED_CSLEEP] = 0;
			m_ptr->hold_o_idx = 0;
			m_ptr->target_y = 0;

			if ((r_ptr->flags1 & RF1_FORCE_SLEEP) && !ironman_nightmare)
			{
				/* Monster is still being nice */
				m_ptr->mflag |= (MFLAG_NICE);

				/* Must repair monsters */
				repair_monsters = TRUE;
			}
			update_monster(m_idx, TRUE);
			lite_spot(cy, cx);

			/* Pre-calculated in precalc_cur_num_of_pet() */
			/* r_ptr->cur_num++; */

			/* Hack -- Count the number of "reproducers" */
			if (r_ptr->flags2 & RF2_MULTIPLY) current_floor_ptr->num_repro++;

		}
		else
		{
			monster_type *m_ptr = &party_mon[i];
			monster_race *r_ptr = real_r_ptr(m_ptr);
			GAME_TEXT m_name[MAX_NLEN];

			monster_desc(m_name, m_ptr, 0);
			msg_format(_("%sとはぐれてしまった。", "You have lost sight of %s."), m_name);
			if (record_named_pet && m_ptr->nickname)
			{
				monster_desc(m_name, m_ptr, MD_INDEF_VISIBLE);
				exe_write_diary(p_ptr, NIKKI_NAMED_PET, RECORD_NAMED_PET_LOST_SIGHT, m_name);
			}

			/* Pre-calculated in precalc_cur_num_of_pet(), but need to decrease */
			if (r_ptr->cur_num) r_ptr->cur_num--;
		}
	}

	/* For accuracy of precalc_cur_num_of_pet() */
	(void)C_WIPE(party_mon, MAX_PARTY_MON, monster_type);
}


/*!
 * @brief ユニークモンスターやアーティファクトの所在フロアを更新する / Hack -- Update location of unique monsters and artifacts
 * @param cur_floor_id 現在のフロアID
 * @return なし
 * @details 
 * The r_ptr->floor_id and a_ptr->floor_id are not updated correctly\n
 * while new floor creation since dungeons may be re-created by\n
 * auto-scum option.\n
 */
static void update_unique_artifact(s16b cur_floor_id)
{
	int i;

	/* Maintain unique monsters */
	for (i = 1; i < current_floor_ptr->m_max; i++)
	{
		monster_race *r_ptr;
		monster_type *m_ptr = &current_floor_ptr->m_list[i];

		if (!monster_is_valid(m_ptr)) continue;

		/* Extract real monster race */
		r_ptr = real_r_ptr(m_ptr);

		/* Memorize location of the unique monster */
		if ((r_ptr->flags1 & RF1_UNIQUE) ||
		    (r_ptr->flags7 & RF7_NAZGUL))
		{
			r_ptr->floor_id = cur_floor_id;
		}
	}

	/* Maintain artifatcs */
	for (i = 1; i < current_floor_ptr->o_max; i++)
	{
		object_type *o_ptr = &current_floor_ptr->o_list[i];

		if (!OBJECT_IS_VALID(o_ptr)) continue;

		/* Memorize location of the artifact */
		if (object_is_fixed_artifact(o_ptr))
		{
			a_info[o_ptr->name1].floor_id = cur_floor_id;
		}
	}
}


/*!
 * @brief フロア移動時、プレイヤーの移動先モンスターが既にいた場合ランダムな近隣に移動させる / When a monster is at a place where player will return,
 * @return なし
 */
static void get_out_monster(void)
{
	int tries = 0;
	POSITION dis = 1;
	POSITION oy = p_ptr->y;
	POSITION ox = p_ptr->x;
	MONSTER_IDX m_idx = current_floor_ptr->grid_array[oy][ox].m_idx;

	/* Nothing to do if no monster */
	if (!m_idx) return;

	/* Look until done */
	while (TRUE)
	{
		monster_type *m_ptr;

		/* Pick a (possibly illegal) location */
		POSITION ny = rand_spread(oy, dis);
		POSITION nx = rand_spread(ox, dis);

		tries++;

		/* Stop after 1000 tries */
		if (tries > 10000) return;

		/*
		 * Increase distance after doing enough tries
		 * compared to area of possible space
		 */
		if (tries > 20 * dis * dis) dis++;

		/* Ignore illegal locations */
		if (!in_bounds(ny, nx)) continue;

		/* Require "empty" floor space */
		if (!cave_empty_bold(ny, nx)) continue;

		/* Hack -- no teleport onto glyph of warding */
		if (is_glyph_grid(&current_floor_ptr->grid_array[ny][nx])) continue;
		if (is_explosive_rune_grid(&current_floor_ptr->grid_array[ny][nx])) continue;

		/* ...nor onto the Pattern */
		if (pattern_tile(ny, nx)) continue;

		/*** It's a good place ***/

		m_ptr = &current_floor_ptr->m_list[m_idx];

		/* Update the old location */
		current_floor_ptr->grid_array[oy][ox].m_idx = 0;

		/* Update the new location */
		current_floor_ptr->grid_array[ny][nx].m_idx = m_idx;

		/* Move the monster */
		m_ptr->fy = ny;
		m_ptr->fx = nx; 

		/* No need to do update_monster() */

		/* Success */
		return;
	}
}

/*!
 * @brief 新フロアに移動元フロアに繋がる階段を配置する / Virtually teleport onto the stairs that is connecting between two floors.
 * @param sf_ptr 移動元の保存フロア構造体参照ポインタ
 * @return なし
 */
static void locate_connected_stairs(saved_floor_type *sf_ptr, BIT_FLAGS floor_mode)
{
	POSITION x, y, sx = 0, sy = 0;
	POSITION x_table[20];
	POSITION y_table[20];
	int num = 0;
	int i;

	/* Search usable stairs */
	for (y = 0; y < current_floor_ptr->height; y++)
	{
		for (x = 0; x < current_floor_ptr->width; x++)
		{
			grid_type *g_ptr = &current_floor_ptr->grid_array[y][x];
			feature_type *f_ptr = &f_info[g_ptr->feat];
			bool ok = FALSE;

			if (floor_mode & CFM_UP)
			{
				if (have_flag(f_ptr->flags, FF_LESS) && have_flag(f_ptr->flags, FF_STAIRS) &&
				    !have_flag(f_ptr->flags, FF_SPECIAL))
				{
					ok = TRUE;

					/* Found fixed stairs? */
					if (g_ptr->special &&
					    g_ptr->special == sf_ptr->upper_floor_id)
					{
						sx = x;
						sy = y;
					}
				}
			}

			else if (floor_mode & CFM_DOWN)
			{
				if (have_flag(f_ptr->flags, FF_MORE) && have_flag(f_ptr->flags, FF_STAIRS) &&
				    !have_flag(f_ptr->flags, FF_SPECIAL))
				{
					ok = TRUE;

					/* Found fixed stairs */
					if (g_ptr->special &&
					    g_ptr->special == sf_ptr->lower_floor_id)
					{
						sx = x;
						sy = y;
					}
				}
			}

			else
			{
				if (have_flag(f_ptr->flags, FF_BLDG))
				{
					ok = TRUE;
				}
			}

			if (ok && (num < 20))
			{
				x_table[num] = x;
				y_table[num] = y;
				num++;
			}
		}
	}

	if (sx)
	{
		/* Already fixed */
		p_ptr->y = sy;
		p_ptr->x = sx;
	}
	else if (!num)
	{
		/* No stairs found! -- No return */
		prepare_change_floor_mode(CFM_RAND_PLACE | CFM_NO_RETURN);

		/* Mega Hack -- It's not the stairs you enter.  Disable it.  */
		if (!feat_uses_special(current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].feat)) current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].special = 0;
	}
	else
	{
		/* Choose random one */
		i = randint0(num);

		/* Point stair location */
		p_ptr->y = y_table[i];
		p_ptr->x = x_table[i];
	}
}

/*!
 * @brief 現在のフロアを離れるに伴って行なわれる保存処理
 * / Maintain quest monsters, mark next floor_id at stairs, save current floor, and prepare to enter next floor.
 * @return なし
 */
void leave_floor(BIT_FLAGS floor_mode)
{
	grid_type *g_ptr = NULL;
	feature_type *f_ptr;
	saved_floor_type *sf_ptr;
	MONRACE_IDX quest_r_idx = 0;
	DUNGEON_IDX i;

	/* Preserve pets and prepare to take these to next floor */
	preserve_pet();

	/* Remove all mirrors without explosion */
	remove_all_mirrors(FALSE);

	if (p_ptr->special_defense & NINJA_S_STEALTH) set_superstealth(p_ptr, FALSE);

	/* New floor is not yet prepared */
	new_floor_id = 0;

	/* Temporary get a floor_id (for Arena) */
	if (!p_ptr->floor_id &&
	    (floor_mode & CFM_SAVE_FLOORS) &&
	    !(floor_mode & CFM_NO_RETURN))
	{
	    /* Get temporal floor_id */
	    p_ptr->floor_id = get_new_floor_id();
	}

	/* Search the quest monster index */
	for (i = 0; i < max_q_idx; i++)
	{
		if ((quest[i].status == QUEST_STATUS_TAKEN) &&
		    ((quest[i].type == QUEST_TYPE_KILL_LEVEL) ||
		    (quest[i].type == QUEST_TYPE_RANDOM)) &&
		    (quest[i].level == current_floor_ptr->dun_level) &&
		    (p_ptr->dungeon_idx == quest[i].dungeon) &&
		    !(quest[i].flags & QUEST_FLAG_PRESET))
		{
			quest_r_idx = quest[i].r_idx;
		}
	}

	/* Maintain quest monsters */
	for (i = 1; i < current_floor_ptr->m_max; i++)
	{
		monster_race *r_ptr;
		monster_type *m_ptr = &current_floor_ptr->m_list[i];

		if (!monster_is_valid(m_ptr)) continue;

		/* Only maintain quest monsters */
		if (quest_r_idx != m_ptr->r_idx) continue;

		/* Extract real monster race */
		r_ptr = real_r_ptr(m_ptr);

		/* Ignore unique monsters */
		if ((r_ptr->flags1 & RF1_UNIQUE) ||
		    (r_ptr->flags7 & RF7_NAZGUL)) continue;

		/* Delete non-unique quest monsters */
		delete_monster_idx(i);
	}

	/* Check if there is a same item */
	for (i = 0; i < INVEN_PACK; i++)
	{
		object_type *o_ptr = &p_ptr->inventory_list[i];

		if (!OBJECT_IS_VALID(o_ptr)) continue;

		/* Delete old memorized location of the artifact */
		if (object_is_fixed_artifact(o_ptr))
		{
			a_info[o_ptr->name1].floor_id = 0;
		}
	}

	/* Extract current floor info or NULL */
	sf_ptr = get_sf_ptr(p_ptr->floor_id);

	/* Choose random stairs */
	if ((floor_mode & CFM_RAND_CONNECT) && p_ptr->floor_id)
	{
		locate_connected_stairs(sf_ptr, floor_mode);
	}

	/* Extract new dungeon level */
	if (floor_mode & CFM_SAVE_FLOORS)
	{
		/* Extract stair position */
		g_ptr = &current_floor_ptr->grid_array[p_ptr->y][p_ptr->x];
		f_ptr = &f_info[g_ptr->feat];

		/* Get back to old saved floor? */
		if (g_ptr->special && !have_flag(f_ptr->flags, FF_SPECIAL) && get_sf_ptr(g_ptr->special))
		{
			/* Saved floor is exist.  Use it. */
			new_floor_id = g_ptr->special;
		}

		/* Mark shaft up/down */
		if (have_flag(f_ptr->flags, FF_STAIRS) && have_flag(f_ptr->flags, FF_SHAFT))
		{
			prepare_change_floor_mode(CFM_SHAFT);
		}
	}

	/* Climb up/down some sort of stairs */
	if (floor_mode & (CFM_DOWN | CFM_UP))
	{
		int move_num = 0;

		/* Extract level movement number */
		if (floor_mode & CFM_DOWN) move_num = 1;
		else if (floor_mode & CFM_UP) move_num = -1;

		/* Shafts are deeper than normal stairs */
		if (floor_mode & CFM_SHAFT)
			move_num += SGN(move_num);

		/* Get out from or Enter the dungeon */
		if (floor_mode & CFM_DOWN)
		{
			if (!current_floor_ptr->dun_level)
				move_num = d_info[p_ptr->dungeon_idx].mindepth;
		}
		else if (floor_mode & CFM_UP)
		{
			if (current_floor_ptr->dun_level + move_num < d_info[p_ptr->dungeon_idx].mindepth)
				move_num = -current_floor_ptr->dun_level;
		}

		current_floor_ptr->dun_level += move_num;
	}

	/* Leaving the dungeon to town */
	if (!current_floor_ptr->dun_level && p_ptr->dungeon_idx)
	{
		p_ptr->leaving_dungeon = TRUE;
		if (!vanilla_town && !lite_town)
		{
			p_ptr->wilderness_y = d_info[p_ptr->dungeon_idx].dy;
			p_ptr->wilderness_x = d_info[p_ptr->dungeon_idx].dx;
		}
		p_ptr->recall_dungeon = p_ptr->dungeon_idx;
		p_ptr->dungeon_idx = 0;

		/* Reach to the surface -- Clear all saved floors */
		floor_mode &= ~CFM_SAVE_FLOORS;
	}

	/* Kill some old saved floors */
	if (!(floor_mode & CFM_SAVE_FLOORS))
	{
		/* Kill all saved floors */
		for (i = 0; i < MAX_SAVED_FLOORS; i++)
			kill_saved_floor(&saved_floors[i]);

		/* Reset visit_mark count */
		latest_visit_mark = 1;
	}
	else if (floor_mode & CFM_NO_RETURN)
	{
		/* Kill current floor */
		kill_saved_floor(sf_ptr);
	}

	/* No current floor -- Left/Enter dungeon etc... */
	if (!p_ptr->floor_id)
	{
		/* No longer need to save current floor */
		return;
	}


	/* Mark next floor_id on the previous floor */
	if (!new_floor_id)
	{
		/* Get new id */
		new_floor_id = get_new_floor_id();

		/* Connect from here */
		if (g_ptr && !feat_uses_special(g_ptr->feat))
		{
			g_ptr->special = new_floor_id;
		}
	}

	/* Fix connection -- level teleportation or trap door */
	if (floor_mode & CFM_RAND_CONNECT)
	{
		if (floor_mode & CFM_UP)
			sf_ptr->upper_floor_id = new_floor_id;
		else if (floor_mode & CFM_DOWN)
			sf_ptr->lower_floor_id = new_floor_id;
	}

	/* If you can return, you need to save previous floor */
	if ((floor_mode & CFM_SAVE_FLOORS) &&
	    !(floor_mode & CFM_NO_RETURN))
	{
		/* Get out of the my way! */
		get_out_monster();

		/* Record the last visit current_world_ptr->game_turn of current floor */
		sf_ptr->last_visit = current_world_ptr->game_turn;

		forget_lite();
		forget_view();
		clear_mon_lite();

		/* Save current floor */
		if (!save_floor(sf_ptr, 0))
		{
			/* Save failed -- No return */
			prepare_change_floor_mode(CFM_NO_RETURN);

			/* Kill current floor */
			kill_saved_floor(get_sf_ptr(p_ptr->floor_id));
		}
	}
}


/*!
 * @brief フロアの切り替え処理 / Enter new floor.
 * @return なし
 * @details
 * If the floor is an old saved floor, it will be\n
 * restored from the temporal file.  If the floor is new one, new current_floor_ptr->grid_array\n
 * will be generated.\n
 */
void change_floor(BIT_FLAGS floor_mode)
{
	saved_floor_type *sf_ptr;
	bool loaded = FALSE;

	/* The dungeon is not ready */
	current_world_ptr->character_dungeon = FALSE;

	/* No longer in the trap detecteded region */
	p_ptr->dtrap = FALSE;

	/* Mega-Hack -- no panel yet */
	panel_row_min = 0;
	panel_row_max = 0;
	panel_col_min = 0;
	panel_col_max = 0;

	/* Mega-Hack -- not ambushed on the wildness? */
	p_ptr->ambush_flag = FALSE;

	/* No saved floors (On the surface etc.) */
	if (!(floor_mode & CFM_SAVE_FLOORS) &&
	    !(floor_mode & CFM_FIRST_FLOOR))
	{
		/* Create current_floor_ptr->grid_array */
		generate_random_floor();

		/* Paranoia -- No new saved floor */
		new_floor_id = 0;
	}

	/* In the dungeon */
	else
	{
		/* No floor_id yet */
		if (!new_floor_id)
		{
			/* Get new id */
			new_floor_id = get_new_floor_id();
		}

		/* Pointer for infomations of new floor */
		sf_ptr = get_sf_ptr(new_floor_id);

		/* Try to restore old floor */
		if (sf_ptr->last_visit)
		{
			/* Old saved floor is exist */
			if (load_floor(sf_ptr, 0))
			{
				loaded = TRUE;

				/* Forbid return stairs */
				if (floor_mode & CFM_NO_RETURN)
				{
					grid_type *g_ptr = &current_floor_ptr->grid_array[p_ptr->y][p_ptr->x];

					if (!feat_uses_special(g_ptr->feat))
					{
						if (floor_mode & (CFM_DOWN | CFM_UP))
						{
							/* Reset to floor */
							g_ptr->feat = feat_ground_type[randint0(100)];
						}

						g_ptr->special = 0;
					}
				}
			}
		}

		/*
		 * Set lower/upper_floor_id of new floor when the new
		 * floor is right-above/right-under the current floor.
		 *
		 * Stair creation/Teleport level/Trap door will take
		 * you the same floor when you used it later again.
		 */
		if (p_ptr->floor_id)
		{
			saved_floor_type *cur_sf_ptr = get_sf_ptr(p_ptr->floor_id);

			if (floor_mode & CFM_UP)
			{
				/* New floor is right-above */
				if (cur_sf_ptr->upper_floor_id == new_floor_id)
					sf_ptr->lower_floor_id = p_ptr->floor_id;
			}
			else if (floor_mode & CFM_DOWN)
			{
				/* New floor is right-under */
				if (cur_sf_ptr->lower_floor_id == new_floor_id)
					sf_ptr->upper_floor_id = p_ptr->floor_id;
			}
		}

		/* Break connection to killed floor */
		else
		{
			if (floor_mode & CFM_UP)
				sf_ptr->lower_floor_id = 0;
			else if (floor_mode & CFM_DOWN)
				sf_ptr->upper_floor_id = 0;
		}

		/* Maintain monsters and artifacts */
		if (loaded)
		{
			MONSTER_IDX i;
			GAME_TURN tmp_last_visit = sf_ptr->last_visit;
			GAME_TURN absence_ticks;
			int alloc_chance = d_info[p_ptr->dungeon_idx].max_m_alloc_chance;
			GAME_TURN alloc_times;

			while (tmp_last_visit > current_world_ptr->game_turn) tmp_last_visit -= TURNS_PER_TICK * TOWN_DAWN;
			absence_ticks = (current_world_ptr->game_turn - tmp_last_visit) / TURNS_PER_TICK;

			/* Maintain monsters */
			for (i = 1; i < current_floor_ptr->m_max; i++)
			{
				monster_race *r_ptr;
				monster_type *m_ptr = &current_floor_ptr->m_list[i];

				if (!monster_is_valid(m_ptr)) continue;

				if (!is_pet(m_ptr))
				{
					/* Restore HP */
					m_ptr->hp = m_ptr->maxhp = m_ptr->max_maxhp;

					/* Remove timed status (except MTIMED_CSLEEP) */
					(void)set_monster_fast(i, 0);
					(void)set_monster_slow(i, 0);
					(void)set_monster_stunned(i, 0);
					(void)set_monster_confused(i, 0);
					(void)set_monster_monfear(i, 0);
					(void)set_monster_invulner(i, 0, FALSE);
				}

				/* Extract real monster race */
				r_ptr = real_r_ptr(m_ptr);

				/* Ignore non-unique */
				if (!(r_ptr->flags1 & RF1_UNIQUE) &&
				    !(r_ptr->flags7 & RF7_NAZGUL)) continue;

				/* Appear at a different floor? */
				if (r_ptr->floor_id != new_floor_id)
				{
					/* Disapper from here */
					delete_monster_idx(i);
				}
			}

			/* Maintain artifatcs */
			for (i = 1; i < current_floor_ptr->o_max; i++)
			{
				object_type *o_ptr = &current_floor_ptr->o_list[i];

				if (!OBJECT_IS_VALID(o_ptr)) continue;

				/* Ignore non-artifact */
				if (!object_is_fixed_artifact(o_ptr)) continue;

				/* Appear at a different floor? */
				if (a_info[o_ptr->name1].floor_id != new_floor_id)
				{
					/* Disappear from here */
					delete_object_idx(i);
				}
				else
				{
					/* Cancel preserve */
					a_info[o_ptr->name1].cur_num = 1;
				}
			}

			(void)place_quest_monsters();

			/* Place some random monsters */
			alloc_times = absence_ticks / alloc_chance;

			if (randint0(alloc_chance) < (absence_ticks % alloc_chance))
				alloc_times++;

			for (i = 0; i < alloc_times; i++)
			{
				/* Make a (group of) new monster */
				(void)alloc_monster(0, 0);
			}

		}

		/* New floor_id or failed to restore */
		else /* if (!loaded) */
		{
			if (sf_ptr->last_visit)
			{
				/* Temporal file is broken? */
				msg_print(_("階段は行き止まりだった。", "The staircases come to a dead end..."));

				/* Create simple dead end */
				build_dead_end();

				/* Break connection */
				if (floor_mode & CFM_UP)
				{
					sf_ptr->upper_floor_id = 0;
				}
				else if (floor_mode & CFM_DOWN)
				{
					sf_ptr->lower_floor_id = 0;
				}
			}
			else
			{
				/* Newly create current_floor_ptr->grid_array */
				generate_random_floor();
			}

			/* Record last visit current_world_ptr->game_turn */
			sf_ptr->last_visit = current_world_ptr->game_turn;

			/* Set correct current_floor_ptr->dun_level value */
			sf_ptr->dun_level = current_floor_ptr->dun_level;

			/* Create connected stairs */
			if (!(floor_mode & CFM_NO_RETURN))
			{
				/* Extract stair position */
				grid_type *g_ptr = &current_floor_ptr->grid_array[p_ptr->y][p_ptr->x];

				/*** Create connected stairs ***/

				/* No stairs down from Quest */
				if ((floor_mode & CFM_UP) && !quest_number(current_floor_ptr->dun_level))
				{
					g_ptr->feat = (floor_mode & CFM_SHAFT) ? feat_state(feat_down_stair, FF_SHAFT) : feat_down_stair;
				}

				/* No stairs up when ironman_downward */
				else if ((floor_mode & CFM_DOWN) && !ironman_downward)
				{
					g_ptr->feat = (floor_mode & CFM_SHAFT) ? feat_state(feat_up_stair, FF_SHAFT) : feat_up_stair;
				}

				/* Paranoia -- Clear mimic */
				g_ptr->mimic = 0;

				/* Connect to previous floor */
				g_ptr->special = p_ptr->floor_id;
			}
		}

		/* Arrive at random grid */
		if (floor_mode & (CFM_RAND_PLACE))
		{
			(void)new_player_spot();
		}

		/* You see stairs blocked */
		else if ((floor_mode & CFM_NO_RETURN) && (floor_mode & (CFM_DOWN | CFM_UP)))
		{
			if (!p_ptr->blind)
			{
				msg_print(_("突然階段が塞がれてしまった。", "Suddenly the stairs is blocked!"));
			}
			else
			{
				msg_print(_("ゴトゴトと何か音がした。", "You hear some noises."));
			}
		}

		/*
		 * Update visit mark
		 *
		 * The "current_world_ptr->game_turn" is not always different number because
		 * the level teleport doesn't take any current_world_ptr->game_turn.  Use
		 * visit mark instead of last visit current_world_ptr->game_turn to find the
		 * oldest saved floor.
		 */
		sf_ptr->visit_mark = latest_visit_mark++;
	}

	/* Place preserved pet monsters */
	place_pet();

	/* Reset travel target place */
	forget_travel_flow();

	/* Hack -- maintain unique and artifacts */
	update_unique_artifact(new_floor_id);

	/* Now the player is in new floor */
	p_ptr->floor_id = new_floor_id;

	/* The dungeon is ready */
	current_world_ptr->character_dungeon = TRUE;

	/* Hack -- Munchkin characters always get whole map */
	if (p_ptr->pseikaku == SEIKAKU_MUNCHKIN)
		wiz_lite((bool)(p_ptr->pclass == CLASS_NINJA));

	/* Remember when this level was "created" */
	current_floor_ptr->generated_turn = current_world_ptr->game_turn;

	/* No dungeon feeling yet */
	p_ptr->feeling_turn = current_floor_ptr->generated_turn;
	p_ptr->feeling = 0;

	/* Clear all flags */
	floor_mode = 0L;

	select_floor_music();
	p_ptr->change_floor_mode = 0;
}
