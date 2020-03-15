/*!
 * @file monster2.c
 * @brief モンスター処理 / misc code for monsters
 * @date 2014/07/08
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "angband.h"
#include "util.h"
#include "core.h"

#include "io/write-diary.h"
#include "cmd/cmd-dump.h"
#include "cmd-pet.h"
#include "dungeon.h"
#include "floor.h"
#include "object-flavor.h"
#include "monsterrace-hook.h"
#include "monster-status.h"
#include "monster.h"
#include "spells.h"
#include "spells-summon.h"
#include "quest.h"
#include "grid.h"
#include "player-move.h"
#include "player-status.h"
#include "player-race.h"
#include "player-class.h"
#include "player-personality.h"
#include "wild.h"
#include "warning.h"
#include "monster-spell.h"
#include "files.h"
#include "view-mainwindow.h"
#include "world.h"
#include "monsterrace.h"
#include "creature.h"
#include "targeting.h"
#include "melee.h"

#define HORDE_NOGOOD 0x01 /*!< (未実装フラグ)HORDE生成でGOODなモンスターの生成を禁止する？ */
#define HORDE_NOEVIL 0x02 /*!< (未実装フラグ)HORDE生成でEVILなモンスターの生成を禁止する？ */
#define MON_SCAT_MAXD 10 /*!< mon_scatter()関数によるモンスター配置で許される中心からの最大距離 */

MONSTER_IDX hack_m_idx = 0;	/* Hack -- see "process_monsters()" */
MONSTER_IDX hack_m_idx_ii = 0;

bool is_friendly_idx(player_type *player_ptr, MONSTER_IDX m_idx);

/*!
 * @brief モンスターの目標地点をセットする / Set the target of counter attack
 * @param m_ptr モンスターの参照ポインタ
 * @param y 目標y座標
 * @param x 目標x座標
 * @return なし
 */
void set_target(monster_type *m_ptr, POSITION y, POSITION x)
{
	m_ptr->target_y = y;
	m_ptr->target_x = x;
}


/*!
 * @brief モンスターの目標地点をリセットする / Reset the target of counter attack
 * @param m_ptr モンスターの参照ポインタ
 * @return なし
 */
void reset_target(monster_type *m_ptr)
{
	set_target(m_ptr, 0, 0);
}


/*!
 * @brief モンスターの真の種族を返す / Extract monster race pointer of a monster's true form
 * @param m_ptr モンスターの参照ポインタ
 * @return 本当のモンスター種族参照ポインタ
 */
monster_race *real_r_ptr(monster_type *m_ptr)
{
	return &r_info[real_r_idx(m_ptr)];
}


MONRACE_IDX real_r_idx(monster_type *m_ptr)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	if (m_ptr->mflag2 & MFLAG2_CHAMELEON)
	{
		if (r_ptr->flags1 & RF1_UNIQUE)
			return MON_CHAMELEON_K;
		else
			return MON_CHAMELEON;
	}

	return m_ptr->r_idx;
}


/*!
 * @brief モンスター配列からモンスターを消去する / Delete a monster by index.
 * @param i 消去するモンスターのID
 * @return なし
 * @details
 * モンスターを削除するとそのモンスターが拾っていたアイテムも同時に削除される。 /
 * When a monster is deleted, all of its objects are deleted.
 */
void delete_monster_idx(player_type *player_ptr, MONSTER_IDX i)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	monster_type *m_ptr = &floor_ptr->m_list[i];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	POSITION y = m_ptr->fy;
	POSITION x = m_ptr->fx;

	real_r_ptr(m_ptr)->cur_num--;
	if (r_ptr->flags2 & (RF2_MULTIPLY)) floor_ptr->num_repro--;

	if (MON_CSLEEP(m_ptr)) (void)set_monster_csleep(player_ptr, i, 0);
	if (MON_FAST(m_ptr)) (void)set_monster_fast(player_ptr, i, 0);
	if (MON_SLOW(m_ptr)) (void)set_monster_slow(player_ptr, i, 0);
	if (MON_STUNNED(m_ptr)) (void)set_monster_stunned(player_ptr, i, 0);
	if (MON_CONFUSED(m_ptr)) (void)set_monster_confused(player_ptr, i, 0);
	if (MON_MONFEAR(m_ptr)) (void)set_monster_monfear(player_ptr, i, 0);
	if (MON_INVULNER(m_ptr)) (void)set_monster_invulner(player_ptr, i, 0, FALSE);

	if (i == target_who) target_who = 0;

	if (i == player_ptr->health_who) health_track(player_ptr, 0);

	if (player_ptr->pet_t_m_idx == i) player_ptr->pet_t_m_idx = 0;
	if (player_ptr->riding_t_m_idx == i) player_ptr->riding_t_m_idx = 0;
	if (player_ptr->riding == i) player_ptr->riding = 0;

	floor_ptr->grid_array[y][x].m_idx = 0;
	OBJECT_IDX next_o_idx = 0;
	for (OBJECT_IDX this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;
		o_ptr = &floor_ptr->o_list[this_o_idx];
		next_o_idx = o_ptr->next_o_idx;
		delete_object_idx(player_ptr, this_o_idx);
	}

	(void)WIPE(m_ptr, monster_type);
	floor_ptr->m_cnt--;
	lite_spot(player_ptr, y, x);
	if (r_ptr->flags7 & (RF7_LITE_MASK | RF7_DARK_MASK))
	{
		player_ptr->update |= (PU_MON_LITE);
	}
}


/*!
 * @brief モンスター情報を配列内移動する / Move an object from index i1 to index i2 in the object list
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param i1 配列移動元添字
 * @param i2 配列移動先添字
 * @return なし
 */
static void compact_monsters_aux(player_type *player_ptr, MONSTER_IDX i1, MONSTER_IDX i2)
{
	if (i1 == i2) return;

	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	monster_type *m_ptr;
	m_ptr = &floor_ptr->m_list[i1];

	POSITION y = m_ptr->fy;
	POSITION x = m_ptr->fx;
	grid_type *g_ptr;
	g_ptr = &floor_ptr->grid_array[y][x];
	g_ptr->m_idx = i2;

	OBJECT_IDX next_o_idx = 0;
	for (OBJECT_IDX this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;
		o_ptr = &floor_ptr->o_list[this_o_idx];
		next_o_idx = o_ptr->next_o_idx;
		o_ptr->held_m_idx = i2;
	}

	if (target_who == i1) target_who = i2;

	if (player_ptr->pet_t_m_idx == i1) player_ptr->pet_t_m_idx = i2;
	if (player_ptr->riding_t_m_idx == i1) player_ptr->riding_t_m_idx = i2;

	if (player_ptr->riding == i1) player_ptr->riding = i2;

	if (player_ptr->health_who == i1) health_track(player_ptr, i2);

	if (is_pet(m_ptr))
	{
		for (int i = 1; i < floor_ptr->m_max; i++)
		{
			monster_type *m2_ptr = &floor_ptr->m_list[i];

			if (m2_ptr->parent_m_idx == i1)
				m2_ptr->parent_m_idx = i2;
		}
	}

	(void)COPY(&floor_ptr->m_list[i2], &floor_ptr->m_list[i1], monster_type);
	(void)WIPE(&floor_ptr->m_list[i1], monster_type);

	for (int i = 0; i < MAX_MTIMED; i++)
	{
		int mproc_idx = get_mproc_idx(floor_ptr, i1, i);
		if (mproc_idx >= 0) floor_ptr->mproc_list[i][mproc_idx] = i2;
	}
}


/*!
 * @brief モンスター情報配列を圧縮する / Compact and Reorder the monster list
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param size 圧縮後のモンスター件数目標
 * @return なし
 * @details
 * This function can be very dangerous, use with caution!
 *
 * When actually "compacting" monsters, we base the saving throw
 * on a combination of monster level, distance from player, and
 * current "desperation".
 *
 * After "compacting" (if needed), we "reorder" the monsters into a more
 * compact order, and we reset the allocation info, and the "live" array.
 */
void compact_monsters(player_type *player_ptr, int size)
{
	if (size) msg_print(_("モンスター情報を圧縮しています...", "Compacting monsters..."));

	/* Compact at least 'size' objects */
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	for (int num = 0, cnt = 1; num < size; cnt++)
	{
		int cur_lev = 5 * cnt;
		int cur_dis = 5 * (20 - cnt);
		for (MONSTER_IDX i = 1; i < floor_ptr->m_max; i++)
		{
			monster_type *m_ptr = &floor_ptr->m_list[i];
			monster_race *r_ptr = &r_info[m_ptr->r_idx];
			if (!monster_is_valid(m_ptr)) continue;
			if (r_ptr->level > cur_lev) continue;
			if (i == player_ptr->riding) continue;
			if ((cur_dis > 0) && (m_ptr->cdis < cur_dis)) continue;

			int chance = 90;
			if ((r_ptr->flags1 & (RF1_QUESTOR)) && (cnt < 1000)) chance = 100;

			if (r_ptr->flags1 & (RF1_UNIQUE)) chance = 100;

			if (randint0(100) < chance) continue;

			if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname)
			{
				GAME_TEXT m_name[MAX_NLEN];
				monster_desc(player_ptr, m_name, m_ptr, MD_INDEF_VISIBLE);
				exe_write_diary(player_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_COMPACT, m_name);
			}

			delete_monster_idx(player_ptr, i);
			num++;
		}
	}

	/* Excise dead monsters (backwards!) */
	for (MONSTER_IDX i = floor_ptr->m_max - 1; i >= 1; i--)
	{
		monster_type *m_ptr = &floor_ptr->m_list[i];
		if (m_ptr->r_idx) continue;
		compact_monsters_aux(player_ptr, floor_ptr->m_max - 1, i);
		floor_ptr->m_max--;
	}
}


/*!
 * todo ここには本来floor_type*を追加したいが、monster.hにfloor.hの参照を追加するとコンパイルエラーが出るので保留
 * @brief プレイヤーのフロア離脱に伴う全モンスター配列の消去 / Delete/Remove all the monsters when the player leaves the level
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * This is an efficient method of simulating multiple calls to the
 * "delete_monster()" function, with no visual effects.
 */
void wipe_monsters_list(player_type *player_ptr)
{
	if (!r_info[MON_BANORLUPART].max_num)
	{
		if (r_info[MON_BANOR].max_num)
		{
			r_info[MON_BANOR].max_num = 0;
			r_info[MON_BANOR].r_pkills++;
			r_info[MON_BANOR].r_akills++;
			if (r_info[MON_BANOR].r_tkills < MAX_SHORT)
				r_info[MON_BANOR].r_tkills++;
		}

		if (r_info[MON_LUPART].max_num)
		{
			r_info[MON_LUPART].max_num = 0;
			r_info[MON_LUPART].r_pkills++;
			r_info[MON_LUPART].r_akills++;
			if (r_info[MON_LUPART].r_tkills < MAX_SHORT)
				r_info[MON_LUPART].r_tkills++;
		}
	}

	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	for (int i = floor_ptr->m_max - 1; i >= 1; i--)
	{
		monster_type *m_ptr = &floor_ptr->m_list[i];
		if (!monster_is_valid(m_ptr)) continue;

		floor_ptr->grid_array[m_ptr->fy][m_ptr->fx].m_idx = 0;
		(void)WIPE(m_ptr, monster_type);
	}

	/*
	 * Wiping racial counters of all monsters and incrementing of racial
	 * counters of monsters in party_mon[] are required to prevent multiple
	 * generation of unique monster who is the minion of player.
	 */
	for (int i = 1; i < max_r_idx; i++) r_info[i].cur_num = 0;

	floor_ptr->m_max = 1;
	floor_ptr->m_cnt = 0;
	for (int i = 0; i < MAX_MTIMED; i++) floor_ptr->mproc_max[i] = 0;

	floor_ptr->num_repro = 0;
	target_who = 0;
	player_ptr->pet_t_m_idx = 0;
	player_ptr->riding_t_m_idx = 0;
	health_track(player_ptr, 0);
}


/*!
 * todo ここには本来floor_type*を追加したいが、monster.hにfloor.hの参照を追加するとコンパイルエラーが出るので保留
 * @brief モンスター配列の空きを探す / Acquires and returns the index of a "free" monster.
 * @return 利用可能なモンスター配列の添字
 * @details
 * This routine should almost never fail, but it *can* happen.
 */
MONSTER_IDX m_pop(player_type *player_ptr)
{
	/* Normal allocation */
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	if (floor_ptr->m_max < current_world_ptr->max_m_idx)
	{
		MONSTER_IDX i = floor_ptr->m_max;
		floor_ptr->m_max++;
		floor_ptr->m_cnt++;
		return i;
	}

	/* Recycle dead monsters */
	for (MONSTER_IDX i = 1; i < floor_ptr->m_max; i++)
	{
		monster_type *m_ptr;
		m_ptr = &floor_ptr->m_list[i];
		if (m_ptr->r_idx) continue;
		floor_ptr->m_cnt++;
		return i;
	}

	if (current_world_ptr->character_dungeon) msg_print(_("モンスターが多すぎる！", "Too many monsters!"));
	return 0;
}


/*!
 * @var summon_specific_type
 * @brief 召喚条件を指定するグローバル変数 / Hack -- the "type" of the current "summon specific"
 * @todo summon_specific_typeグローバル変数の除去と関数引数への代替を行う
 */
static int summon_specific_type = 0;


/*!
 * @var summon_specific_who
 * @brief 召喚を行ったプレイヤーあるいはモンスターのIDを示すグローバル変数 / Hack -- the index of the summoning monster
 * @todo summon_specific_who グローバル変数の除去と関数引数への代替を行う
 */
static int summon_specific_who = -1;


/*!
 * @var summon_unique_okay
 * @brief 召喚対象にユニークを含めるかを示すグローバル変数 / summoning unique enable
 * @todo summon_unique_okay グローバル変数の除去と関数引数への代替を行う
 */
static bool summon_unique_okay = FALSE;


/*!
 * @brief 指定されたモンスター種族がsummon_specific_typeで指定された召喚条件に合うかどうかを返す
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return 召喚条件が一致するならtrue
 * @details
 */
static bool summon_specific_aux(player_type *player_ptr, MONRACE_IDX summoner_idx, MONRACE_IDX r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];
	bool is_match = FALSE;

	switch (summon_specific_type)
	{
	case SUMMON_ANT:
	{
		is_match = (r_ptr->d_char == 'a');
		break;
	}
	case SUMMON_SPIDER:
	{
		is_match = (r_ptr->d_char == 'S');
		break;
	}
	case SUMMON_HOUND:
	{
		is_match = ((r_ptr->d_char == 'C') || (r_ptr->d_char == 'Z'));
		break;
	}
	case SUMMON_HYDRA:
	{
		is_match = (r_ptr->d_char == 'M');
		break;
	}
	case SUMMON_ANGEL:
	{
		is_match = (r_ptr->d_char == 'A' && ((r_ptr->flags3 & RF3_EVIL) || (r_ptr->flags3 & RF3_GOOD)));
		break;
	}
	case SUMMON_DEMON:
	{
		is_match = (r_ptr->flags3 & RF3_DEMON);
		break;
	}
	case SUMMON_UNDEAD:
	{
		is_match = (r_ptr->flags3 & RF3_UNDEAD);
		break;
	}
	case SUMMON_DRAGON:
	{
		is_match = (r_ptr->flags3 & RF3_DRAGON);
		break;
	}
	case SUMMON_HI_UNDEAD:
	{
		is_match = ((r_ptr->d_char == 'L') ||
			(r_ptr->d_char == 'V') ||
			(r_ptr->d_char == 'W'));
		break;
	}
	case SUMMON_HI_DRAGON:
	{
		is_match = (r_ptr->d_char == 'D');
		break;
	}
	case SUMMON_HI_DEMON:
	{
		is_match = (((r_ptr->d_char == 'U') ||
			(r_ptr->d_char == 'H') ||
			(r_ptr->d_char == 'B')) &&
			(r_ptr->flags3 & RF3_DEMON)) ? TRUE : FALSE;
		break;
	}
	case SUMMON_AMBERITES:
	{
		is_match = (r_ptr->flags3 & (RF3_AMBERITE)) ? TRUE : FALSE;
		break;
	}
	case SUMMON_UNIQUE:
	{
		is_match = (r_ptr->flags1 & (RF1_UNIQUE)) ? TRUE : FALSE;
		break;
	}
	case SUMMON_MOLD:
	{
		is_match = (r_ptr->d_char == 'm');
		break;
	}
	case SUMMON_BAT:
	{
		is_match = (r_ptr->d_char == 'b');
		break;
	}
	case SUMMON_QUYLTHULG:
	{
		is_match = (r_ptr->d_char == 'Q');
		break;
	}
	case SUMMON_COIN_MIMIC:
	{
		is_match = (r_ptr->d_char == '$');
		break;
	}
	case SUMMON_MIMIC:
	{
		is_match = ((r_ptr->d_char == '!') ||
			(r_ptr->d_char == '?') ||
			(r_ptr->d_char == '=') ||
			(r_ptr->d_char == '$') ||
			(r_ptr->d_char == '|'));
		break;
	}
	case SUMMON_GOLEM:
	{
		is_match = (r_ptr->d_char == 'g');
		break;
	}
	case SUMMON_CYBER:
	{
		is_match = ((r_ptr->d_char == 'U') &&
			(r_ptr->flags4 & RF4_ROCKET));
		break;
	}
	case SUMMON_KIN:
	{
		SYMBOL_CODE summon_kin_type;
		if (summoner_idx)
		{
			summon_kin_type = r_info[summoner_idx].d_char;
		}
		else
		{
			summon_kin_type = get_summon_symbol_from_player(player_ptr);
		}

		is_match = ((r_ptr->d_char == summon_kin_type) && (r_idx != MON_HAGURE));
		break;
	}
	case SUMMON_DAWN:
	{
		is_match = (r_idx == MON_DAWN);
		break;
	}
	case SUMMON_ANIMAL:
	{
		is_match = (r_ptr->flags3 & (RF3_ANIMAL));
		break;
	}
	case SUMMON_ANIMAL_RANGER:
	{
		is_match = ((r_ptr->flags3 & (RF3_ANIMAL)) &&
			(my_strchr("abcflqrwBCHIJKMRS", r_ptr->d_char)) &&
			!(r_ptr->flags3 & (RF3_DRAGON)) &&
			!(r_ptr->flags3 & (RF3_EVIL)) &&
			!(r_ptr->flags3 & (RF3_UNDEAD)) &&
			!(r_ptr->flags3 & (RF3_DEMON)) &&
			!(r_ptr->flags2 & (RF2_MULTIPLY)) &&
			!(r_ptr->flags4 || r_ptr->a_ability_flags1 || r_ptr->a_ability_flags2));
		break;
	}
	case SUMMON_HI_DRAGON_LIVING:
	{
		is_match = ((r_ptr->d_char == 'D') && monster_living(r_idx));
		break;
	}
	case SUMMON_LIVING:
	{
		is_match = monster_living(r_idx);
		break;
	}
	case SUMMON_PHANTOM:
	{
		is_match = (r_idx == MON_PHANTOM_B || r_idx == MON_PHANTOM_W);
		break;
	}
	case SUMMON_BLUE_HORROR:
	{
		is_match = (r_idx == MON_BLUE_HORROR);
		break;
	}
	case SUMMON_ELEMENTAL:
	{
		is_match = (r_ptr->d_char == 'E');
		break;
	}
	case SUMMON_VORTEX:
	{
		is_match = (r_ptr->d_char == 'v');
		break;
	}
	case SUMMON_HYBRID:
	{
		is_match = (r_ptr->d_char == 'H');
		break;
	}
	case SUMMON_BIRD:
	{
		is_match = (r_ptr->d_char == 'B');
		break;
	}
	case SUMMON_KAMIKAZE:
	{
		int i;
		for (i = 0; i < 4; i++)
			if (r_ptr->blow[i].method == RBM_EXPLODE) is_match = TRUE;
		break;
	}
	case SUMMON_KAMIKAZE_LIVING:
	{
		int i;

		for (i = 0; i < 4; i++)
			if (r_ptr->blow[i].method == RBM_EXPLODE) is_match = TRUE;
		is_match = (is_match && monster_living(r_idx));
		break;
	}
	case SUMMON_MANES:
	{
		is_match = (r_idx == MON_MANES);
		break;
	}
	case SUMMON_LOUSE:
	{
		is_match = (r_idx == MON_LOUSE);
		break;
	}
	case SUMMON_GUARDIANS:
	{
		is_match = (r_ptr->flags7 & RF7_GUARDIAN);
		break;
	}
	case SUMMON_KNIGHTS:
	{
		is_match = ((r_idx == MON_NOV_PALADIN) ||
			(r_idx == MON_NOV_PALADIN_G) ||
			(r_idx == MON_PALADIN) ||
			(r_idx == MON_W_KNIGHT) ||
			(r_idx == MON_ULTRA_PALADIN) ||
			(r_idx == MON_KNI_TEMPLAR));
		break;
	}
	case SUMMON_EAGLES:
	{
		is_match = (r_ptr->d_char == 'B' &&
			(r_ptr->flags8 & RF8_WILD_MOUNTAIN) &&
			(r_ptr->flags8 & RF8_WILD_ONLY));
		break;
	}
	case SUMMON_PIRANHAS:
	{
		is_match = (r_idx == MON_PIRANHA);
		break;
	}
	case SUMMON_ARMAGE_GOOD:
	{
		is_match = (r_ptr->d_char == 'A' && (r_ptr->flags3 & RF3_GOOD));
		break;
	}
	case SUMMON_ARMAGE_EVIL:
	{
		is_match = ((r_ptr->flags3 & RF3_DEMON) ||
			(r_ptr->d_char == 'A' && (r_ptr->flags3 & RF3_EVIL)));
		break;
	}
	}

	return is_match;
}


/*!
 * @var chameleon_change_m_idx
 * @brief カメレオンの変身先モンスターIDを受け渡すためのグローバル変数
 * @todo 変数渡しの問題などもあるができればchameleon_change_m_idxのグローバル変数を除去し、関数引き渡しに移行すること
 */
static int chameleon_change_m_idx = 0;

/*!
 * todo ここには本来floor_type*を追加したいが、monster.hにfloor.hの参照を追加するとコンパイルエラーが出るので保留
 * @brief 指定されたモンスター種族がダンジョンの制限にかかるかどうかをチェックする / Some dungeon types restrict the possible monsters.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param r_idx チェックするモンスター種族ID
 * @return 召喚条件が一致するならtrue / Return TRUE is the monster is OK and FALSE otherwise
 */
static bool restrict_monster_to_dungeon(player_type *player_ptr, MONRACE_IDX r_idx)
{
	DUNGEON_IDX d_idx = player_ptr->dungeon_idx;
	dungeon_type *d_ptr = &d_info[d_idx];
	monster_race *r_ptr = &r_info[r_idx];

	if (d_ptr->flags1 & DF1_CHAMELEON)
	{
		if (chameleon_change_m_idx) return TRUE;
	}

	if (d_ptr->flags1 & DF1_NO_MAGIC)
	{
		if (r_idx != MON_CHAMELEON &&
			r_ptr->freq_spell &&
			!(r_ptr->flags4 & RF4_NOMAGIC_MASK) &&
			!(r_ptr->a_ability_flags1 & RF5_NOMAGIC_MASK) &&
			!(r_ptr->a_ability_flags2 & RF6_NOMAGIC_MASK))
			return FALSE;
	}

	if (d_ptr->flags1 & DF1_NO_MELEE)
	{
		if (r_idx == MON_CHAMELEON) return TRUE;
		if (!(r_ptr->flags4 & (RF4_BOLT_MASK | RF4_BEAM_MASK | RF4_BALL_MASK)) &&
			!(r_ptr->a_ability_flags1 & (RF5_BOLT_MASK | RF5_BEAM_MASK | RF5_BALL_MASK | RF5_CAUSE_1 | RF5_CAUSE_2 | RF5_CAUSE_3 | RF5_CAUSE_4 | RF5_MIND_BLAST | RF5_BRAIN_SMASH)) &&
			!(r_ptr->a_ability_flags2 & (RF6_BOLT_MASK | RF6_BEAM_MASK | RF6_BALL_MASK)))
			return FALSE;
	}

	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	if (d_ptr->flags1 & DF1_BEGINNER)
	{
		if (r_ptr->level > floor_ptr->dun_level)
			return FALSE;
	}

	if (d_ptr->special_div >= 64) return TRUE;
	if (summon_specific_type && !(d_ptr->flags1 & DF1_CHAMELEON)) return TRUE;

	byte a;
	switch (d_ptr->mode)
	{
	case DUNGEON_MODE_AND:
	{
		if (d_ptr->mflags1)
		{
			if ((d_ptr->mflags1 & r_ptr->flags1) != d_ptr->mflags1)
				return FALSE;
		}

		if (d_ptr->mflags2)
		{
			if ((d_ptr->mflags2 & r_ptr->flags2) != d_ptr->mflags2)
				return FALSE;
		}

		if (d_ptr->mflags3)
		{
			if ((d_ptr->mflags3 & r_ptr->flags3) != d_ptr->mflags3)
				return FALSE;
		}

		if (d_ptr->mflags4)
		{
			if ((d_ptr->mflags4 & r_ptr->flags4) != d_ptr->mflags4)
				return FALSE;
		}

		if (d_ptr->m_a_ability_flags1)
		{
			if ((d_ptr->m_a_ability_flags1 & r_ptr->a_ability_flags1) != d_ptr->m_a_ability_flags1)
				return FALSE;
		}

		if (d_ptr->m_a_ability_flags2)
		{
			if ((d_ptr->m_a_ability_flags2 & r_ptr->a_ability_flags2) != d_ptr->m_a_ability_flags2)
				return FALSE;
		}

		if (d_ptr->mflags7)
		{
			if ((d_ptr->mflags7 & r_ptr->flags7) != d_ptr->mflags7)
				return FALSE;
		}

		if (d_ptr->mflags8)
		{
			if ((d_ptr->mflags8 & r_ptr->flags8) != d_ptr->mflags8)
				return FALSE;
		}

		if (d_ptr->mflags9)
		{
			if ((d_ptr->mflags9 & r_ptr->flags9) != d_ptr->mflags9)
				return FALSE;
		}

		if (d_ptr->mflagsr)
		{
			if ((d_ptr->mflagsr & r_ptr->flagsr) != d_ptr->mflagsr)
				return FALSE;
		}

		for (a = 0; a < 5; a++)
			if (d_ptr->r_char[a] && (d_ptr->r_char[a] != r_ptr->d_char)) return FALSE;

		return TRUE;
	}
	case DUNGEON_MODE_NAND:
	{
		if (d_ptr->mflags1)
		{
			if ((d_ptr->mflags1 & r_ptr->flags1) != d_ptr->mflags1)
				return TRUE;
		}

		if (d_ptr->mflags2)
		{
			if ((d_ptr->mflags2 & r_ptr->flags2) != d_ptr->mflags2)
				return TRUE;
		}

		if (d_ptr->mflags3)
		{
			if ((d_ptr->mflags3 & r_ptr->flags3) != d_ptr->mflags3)
				return TRUE;
		}

		if (d_ptr->mflags4)
		{
			if ((d_ptr->mflags4 & r_ptr->flags4) != d_ptr->mflags4)
				return TRUE;
		}

		if (d_ptr->m_a_ability_flags1)
		{
			if ((d_ptr->m_a_ability_flags1 & r_ptr->a_ability_flags1) != d_ptr->m_a_ability_flags1)
				return TRUE;
		}

		if (d_ptr->m_a_ability_flags2)
		{
			if ((d_ptr->m_a_ability_flags2 & r_ptr->a_ability_flags2) != d_ptr->m_a_ability_flags2)
				return TRUE;
		}

		if (d_ptr->mflags7)
		{
			if ((d_ptr->mflags7 & r_ptr->flags7) != d_ptr->mflags7)
				return TRUE;
		}

		if (d_ptr->mflags8)
		{
			if ((d_ptr->mflags8 & r_ptr->flags8) != d_ptr->mflags8)
				return TRUE;
		}

		if (d_ptr->mflags9)
		{
			if ((d_ptr->mflags9 & r_ptr->flags9) != d_ptr->mflags9)
				return TRUE;
		}

		if (d_ptr->mflagsr)
		{
			if ((d_ptr->mflagsr & r_ptr->flagsr) != d_ptr->mflagsr)
				return TRUE;
		}

		for (a = 0; a < 5; a++)
			if (d_ptr->r_char[a] && (d_ptr->r_char[a] != r_ptr->d_char)) return TRUE;

		return FALSE;
	}
	case DUNGEON_MODE_OR:
	{
		if (r_ptr->flags1 & d_ptr->mflags1) return TRUE;
		if (r_ptr->flags2 & d_ptr->mflags2) return TRUE;
		if (r_ptr->flags3 & d_ptr->mflags3) return TRUE;
		if (r_ptr->flags4 & d_ptr->mflags4) return TRUE;
		if (r_ptr->a_ability_flags1 & d_ptr->m_a_ability_flags1) return TRUE;
		if (r_ptr->a_ability_flags2 & d_ptr->m_a_ability_flags2) return TRUE;
		if (r_ptr->flags7 & d_ptr->mflags7) return TRUE;
		if (r_ptr->flags8 & d_ptr->mflags8) return TRUE;
		if (r_ptr->flags9 & d_ptr->mflags9) return TRUE;
		if (r_ptr->flagsr & d_ptr->mflagsr) return TRUE;
		for (a = 0; a < 5; a++)
			if (d_ptr->r_char[a] == r_ptr->d_char) return TRUE;

		return FALSE;
	}
	case DUNGEON_MODE_NOR:
	{
		if (r_ptr->flags1 & d_ptr->mflags1) return FALSE;
		if (r_ptr->flags2 & d_ptr->mflags2) return FALSE;
		if (r_ptr->flags3 & d_ptr->mflags3) return FALSE;
		if (r_ptr->flags4 & d_ptr->mflags4) return FALSE;
		if (r_ptr->a_ability_flags1 & d_ptr->m_a_ability_flags1) return FALSE;
		if (r_ptr->a_ability_flags2 & d_ptr->m_a_ability_flags2) return FALSE;
		if (r_ptr->flags7 & d_ptr->mflags7) return FALSE;
		if (r_ptr->flags8 & d_ptr->mflags8) return FALSE;
		if (r_ptr->flags9 & d_ptr->mflags9) return FALSE;
		if (r_ptr->flagsr & d_ptr->mflagsr) return FALSE;
		for (a = 0; a < 5; a++)
			if (d_ptr->r_char[a] == r_ptr->d_char) return FALSE;

		return TRUE;
	}
	}

	return TRUE;
}

/*
 * Hack -- function hooks to restrict "get_mon_num_prep()" function
 */
monsterrace_hook_type get_mon_num_hook;
monsterrace_hook_type get_mon_num2_hook;

/*!
 * @brief モンスター生成制限関数最大2つから / Apply a "monster restriction function" to the "monster allocation table"
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param monster_hook 制限関数1
 * @param monster_hook2 制限関数2
 * @return エラーコード
 */
errr get_mon_num_prep(player_type *player_ptr, monsterrace_hook_type monster_hook, monsterrace_hook_type monster_hook2)
{
	/* Todo: Check the hooks for non-changes */
	get_mon_num_hook = monster_hook;
	get_mon_num2_hook = monster_hook2;

	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	for (int i = 0; i < alloc_race_size; i++)
	{
		monster_race *r_ptr;
		alloc_entry *entry = &alloc_race_table[i];
		entry->prob2 = 0;
		r_ptr = &r_info[entry->index];

		if ((get_mon_num_hook && !((*get_mon_num_hook)(entry->index))) ||
			(get_mon_num2_hook && !((*get_mon_num2_hook)(entry->index))))
			continue;

		if (!player_ptr->phase_out && !chameleon_change_m_idx &&
			summon_specific_type != SUMMON_GUARDIANS)
		{
			if (r_ptr->flags1 & RF1_QUESTOR)
				continue;

			if (r_ptr->flags7 & RF7_GUARDIAN)
				continue;

			if ((r_ptr->flags1 & (RF1_FORCE_DEPTH)) &&
				(r_ptr->level > floor_ptr->dun_level))
				continue;
		}

		entry->prob2 = entry->prob1;
		if (floor_ptr->dun_level && (!floor_ptr->inside_quest || is_fixed_quest_idx(floor_ptr->inside_quest)) &&
			!restrict_monster_to_dungeon(player_ptr, entry->index) && !player_ptr->phase_out)
		{
			int hoge = entry->prob2 * d_info[player_ptr->dungeon_idx].special_div;
			entry->prob2 = hoge / 64;
			if (randint0(64) < (hoge & 0x3f)) entry->prob2++;
		}
	}

	return 0;
}


/*!
 * @brief 生成モンスター種族を1種生成テーブルから選択する
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param level 生成階
 * @return 選択されたモンスター生成種族
 * @details
 * Choose a monster race that seems "appropriate" to the given level
 *
 * This function uses the "prob2" field of the "monster allocation table",
 * and various local information, to calculate the "prob3" field of the
 * same table, which is then used to choose an "appropriate" monster, in
 * a relatively efficient manner.
 *
 * Note that "town" monsters will *only* be created in the town, and
 * "normal" monsters will *never* be created in the town, unless the
 * "level" is "modified", for example, by polymorph or summoning.
 *
 * There is a small chance (1/50) of "boosting" the given depth by
 * a small amount (up to four levels), except in the town.
 *
 * It is (slightly) more likely to acquire a monster of the given level
 * than one of a lower level.  This is done by choosing several monsters
 * appropriate to the given level and keeping the "hardest" one.
 *
 * Note that if no monsters are "appropriate", then this function will
 * fail, and return zero, but this should *almost* never happen.
 */
MONRACE_IDX get_mon_num(player_type *player_ptr, DEPTH level, BIT_FLAGS option)
{
	int delay = mysqrt(level * 10000L) + 400L;
	int reinforcement_possibility = MAX(NASTY_MON_MAX, NASTY_MON_BASE - ((current_world_ptr->dungeon_turn / (TURNS_PER_TICK * 5000L) - delay / 10)));
	int reinforcement_level = MIN(NASTY_MON_PLUS_MAX, 3 + current_world_ptr->dungeon_turn / (TURNS_PER_TICK * 40000L) - delay / 40 + MIN(5, level / 10));

	if (d_info[player_ptr->dungeon_idx].flags1 & DF1_MAZE)
	{
		reinforcement_possibility = MIN(reinforcement_possibility / 2, reinforcement_possibility - 10);
		if (reinforcement_possibility < 2) reinforcement_possibility = 2;
		reinforcement_level += 2;
		level += 3;
	}

	if (!(option & GMN_ARENA) && !(d_info[player_ptr->dungeon_idx].flags1 & DF1_BEGINNER))
	{
		if (ironman_nightmare && !randint0(reinforcement_possibility))
		{
			level = 1 + (level * MAX_DEPTH / randint1(MAX_DEPTH));
		}
		else
		{
			if (!randint0(reinforcement_possibility))
			{
				level += reinforcement_level;
			}
		}
	}

	if (level > MAX_DEPTH - 1) level = MAX_DEPTH - 1;
	if (level < 0) level = 0;

	long total = 0L;

	/* Process probabilities */
	alloc_entry *table = alloc_race_table;
	for (int i = 0; i < alloc_race_size; i++)
	{
		if (table[i].level > level) break;
		table[i].prob3 = 0;
		MONRACE_IDX r_idx = table[i].index;
		monster_race *r_ptr;
		r_ptr = &r_info[r_idx];
		if (!(option & GMN_ARENA) && !chameleon_change_m_idx)
		{
			if (((r_ptr->flags1 & (RF1_UNIQUE)) ||
				(r_ptr->flags7 & (RF7_NAZGUL))) &&
				(r_ptr->cur_num >= r_ptr->max_num))
			{
				continue;
			}

			if ((r_ptr->flags7 & (RF7_UNIQUE2)) &&
				(r_ptr->cur_num >= 1))
			{
				continue;
			}

			if (r_idx == MON_BANORLUPART)
			{
				if (r_info[MON_BANOR].cur_num > 0) continue;
				if (r_info[MON_LUPART].cur_num > 0) continue;
			}
		}

		table[i].prob3 = table[i].prob2;
		total += table[i].prob3;
	}

	if (total <= 0) return 0;

	long value = randint0(total);
	int found_count = 0;
	for (int i = 0; i < alloc_race_size; i++)
	{
		if (value < table[i].prob3) break;
		value = value - table[i].prob3;
		found_count++;
	}

	int p = randint0(100);

	/* Try for a "harder" monster once (50%) or twice (10%) */
	if (p < 60)
	{
		int j = found_count;
		value = randint0(total);
		for (found_count = 0; found_count < alloc_race_size; found_count++)
		{
			if (value < table[found_count].prob3) break;

			value = value - table[found_count].prob3;
		}

		if (table[found_count].level < table[j].level)
			found_count = j;
	}

	/* Try for a "harder" monster twice (10%) */
	if (p < 10)
	{
		int j = found_count;
		value = randint0(total);
		for (found_count = 0; found_count < alloc_race_size; found_count++)
		{
			if (value < table[found_count].prob3) break;

			value = value - table[found_count].prob3;
		}

		if (table[found_count].level < table[j].level)
			found_count = j;
	}

	return (table[found_count].index);
}


/*!
 * @brief モンスターの呼称を作成する / Build a string describing a monster in some way.
 * @param desc 記述出力先の文字列参照ポインタ
 * @param m_ptr モンスターの参照ポインタ
 * @param mode 呼称オプション
 * @return なし
 * @details
 * We can correctly describe monsters based on their visibility.
 * We can force all monsters to be treated as visible or invisible.
 * We can build nominatives, objectives, possessives, or reflexives.
 * We can selectively pronominalize hidden, visible, or all monsters.
 * We can use definite or indefinite descriptions for hidden monsters.
 * We can use definite or indefinite descriptions for visible monsters.
 *
 * Pronominalization involves the gender whenever possible and allowed,
 * so that by cleverly requesting pronominalization / visibility, you
 * can get messages like "You hit someone.  She screams in agony!".
 *
 * Reflexives are acquired by requesting Objective plus Possessive.
 *
 * If no m_ptr arg is given (?), the monster is assumed to be hidden,
 * unless the "Assume Visible" mode is requested.
 *
 * If no r_ptr arg is given, it is extracted from m_ptr and r_info
 * If neither m_ptr nor r_ptr is given, the monster is assumed to
 * be neuter, singular, and hidden (unless "Assume Visible" is set),
 * in which case you may be in trouble... :-)
 *
 * I am assuming that no monster name is more than 70 characters long,
 * so that "char desc[80];" is sufficiently large for any result.
 *
 * Mode Flags:
 *  MD_OBJECTIVE      --> Objective (or Reflexive)
 *  MD_POSSESSIVE     --> Possessive (or Reflexive)
 *  MD_INDEF_HIDDEN   --> Use indefinites for hidden monsters ("something")
 *  MD_INDEF_VISIBLE  --> Use indefinites for visible monsters ("a kobold")
 *  MD_PRON_HIDDEN    --> Pronominalize hidden monsters
 *  MD_PRON_VISIBLE   --> Pronominalize visible monsters
 *  MD_ASSUME_HIDDEN  --> Assume the monster is hidden
 *  MD_ASSUME_VISIBLE --> Assume the monster is visible
 *  MD_TRUE_NAME      --> Chameleon's true name
 *  MD_IGNORE_HALLU   --> Ignore hallucination, and penetrate shape change
 *
 * Useful Modes:
 *  0x00 --> Full nominative name ("the kobold") or "it"
 *  MD_INDEF_HIDDEN --> Full nominative name ("the kobold") or "something"
 *  MD_ASSUME_VISIBLE --> Genocide resistance name ("the kobold")
 *  MD_ASSUME_VISIBLE | MD_INDEF_VISIBLE --> Killing name ("a kobold")
 *  MD_PRON_VISIBLE | MD_POSSESSIVE
 *    --> Possessive, genderized if visable ("his") or "its"
 *  MD_PRON_VISIBLE | MD_POSSESSIVE | MD_OBJECTIVE
 *    --> Reflexive, genderized if visable ("himself") or "itself"
 */
void monster_desc(player_type *player_ptr, char *desc, monster_type *m_ptr, BIT_FLAGS mode)
{
	monster_race *r_ptr;
	r_ptr = &r_info[m_ptr->ap_r_idx];
	concptr name = (mode & MD_TRUE_NAME) ? (r_name + real_r_ptr(m_ptr)->name) : (r_name + r_ptr->name);
	GAME_TEXT silly_name[1024];
	bool named = FALSE;
	if (player_ptr->image && !(mode & MD_IGNORE_HALLU))
	{
		if (one_in_(2))
		{
			if (!get_rnd_line(_("silly_j.txt", "silly.txt"), m_ptr->r_idx, silly_name))
				named = TRUE;
		}

		if (!named)
		{
			monster_race *hallu_race;

			do
			{
				hallu_race = &r_info[randint1(max_r_idx - 1)];
			} while (!hallu_race->name || (hallu_race->flags1 & RF1_UNIQUE));

			strcpy(silly_name, (r_name + hallu_race->name));
		}

		name = silly_name;
	}

	bool seen = (m_ptr && ((mode & MD_ASSUME_VISIBLE) || (!(mode & MD_ASSUME_HIDDEN) && m_ptr->ml)));
	bool pron = (m_ptr && ((seen && (mode & MD_PRON_VISIBLE)) || (!seen && (mode & MD_PRON_HIDDEN))));

	/* First, try using pronouns, or describing hidden monsters */
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	if (!seen || pron)
	{
		int kind = 0x00;
		if (r_ptr->flags1 & (RF1_FEMALE)) kind = 0x20;
		else if (r_ptr->flags1 & (RF1_MALE)) kind = 0x10;

		if (!m_ptr || !pron) kind = 0x00;

		concptr res = _("何か", "it");
		switch (kind + (mode & (MD_INDEF_HIDDEN | MD_POSSESSIVE | MD_OBJECTIVE)))
		{
#ifdef JP
		case 0x00:                                                    res = "何か"; break;
			case 0x00 + (MD_OBJECTIVE) : res = "何か"; break;
				case 0x00 + (MD_POSSESSIVE) : res = "何かの"; break;
					case 0x00 + (MD_POSSESSIVE | MD_OBJECTIVE) : res = "何か自身"; break;
						case 0x00 + (MD_INDEF_HIDDEN) : res = "何か"; break;
							case 0x00 + (MD_INDEF_HIDDEN | MD_OBJECTIVE) : res = "何か"; break;
								case 0x00 + (MD_INDEF_HIDDEN | MD_POSSESSIVE) : res = "何か"; break;
									case 0x00 + (MD_INDEF_HIDDEN | MD_POSSESSIVE | MD_OBJECTIVE) : res = "それ自身"; break;
#else
		case 0x00:                                                    res = "it"; break;
			case 0x00 + (MD_OBJECTIVE) : res = "it"; break;
				case 0x00 + (MD_POSSESSIVE) : res = "its"; break;
					case 0x00 + (MD_POSSESSIVE | MD_OBJECTIVE) : res = "itself"; break;
						case 0x00 + (MD_INDEF_HIDDEN) : res = "something"; break;
							case 0x00 + (MD_INDEF_HIDDEN | MD_OBJECTIVE) : res = "something"; break;
								case 0x00 + (MD_INDEF_HIDDEN | MD_POSSESSIVE) : res = "something's"; break;
									case 0x00 + (MD_INDEF_HIDDEN | MD_POSSESSIVE | MD_OBJECTIVE) : res = "itself"; break;
#endif

#ifdef JP
									case 0x10:                                                    res = "彼"; break;
										case 0x10 + (MD_OBJECTIVE) : res = "彼"; break;
											case 0x10 + (MD_POSSESSIVE) : res = "彼の"; break;
												case 0x10 + (MD_POSSESSIVE | MD_OBJECTIVE) : res = "彼自身"; break;
													case 0x10 + (MD_INDEF_HIDDEN) : res = "誰か"; break;
														case 0x10 + (MD_INDEF_HIDDEN | MD_OBJECTIVE) : res = "誰か"; break;
															case 0x10 + (MD_INDEF_HIDDEN | MD_POSSESSIVE) : res = "誰かの"; break;
																case 0x10 + (MD_INDEF_HIDDEN | MD_POSSESSIVE | MD_OBJECTIVE) : res = "彼自身"; break;
#else
									case 0x10:                                                    res = "he"; break;
										case 0x10 + (MD_OBJECTIVE) : res = "him"; break;
											case 0x10 + (MD_POSSESSIVE) : res = "his"; break;
												case 0x10 + (MD_POSSESSIVE | MD_OBJECTIVE) : res = "himself"; break;
													case 0x10 + (MD_INDEF_HIDDEN) : res = "someone"; break;
														case 0x10 + (MD_INDEF_HIDDEN | MD_OBJECTIVE) : res = "someone"; break;
															case 0x10 + (MD_INDEF_HIDDEN | MD_POSSESSIVE) : res = "someone's"; break;
																case 0x10 + (MD_INDEF_HIDDEN | MD_POSSESSIVE | MD_OBJECTIVE) : res = "himself"; break;
#endif

#ifdef JP
																case 0x20:                                                    res = "彼女"; break;
																	case 0x20 + (MD_OBJECTIVE) : res = "彼女"; break;
																		case 0x20 + (MD_POSSESSIVE) : res = "彼女の"; break;
																			case 0x20 + (MD_POSSESSIVE | MD_OBJECTIVE) : res = "彼女自身"; break;
																				case 0x20 + (MD_INDEF_HIDDEN) : res = "誰か"; break;
																					case 0x20 + (MD_INDEF_HIDDEN | MD_OBJECTIVE) : res = "誰か"; break;
																						case 0x20 + (MD_INDEF_HIDDEN | MD_POSSESSIVE) : res = "誰かの"; break;
																							case 0x20 + (MD_INDEF_HIDDEN | MD_POSSESSIVE | MD_OBJECTIVE) : res = "彼女自身"; break;
#else
																case 0x20:                                                    res = "she"; break;
																	case 0x20 + (MD_OBJECTIVE) : res = "her"; break;
																		case 0x20 + (MD_POSSESSIVE) : res = "her"; break;
																			case 0x20 + (MD_POSSESSIVE | MD_OBJECTIVE) : res = "herself"; break;
																				case 0x20 + (MD_INDEF_HIDDEN) : res = "someone"; break;
																					case 0x20 + (MD_INDEF_HIDDEN | MD_OBJECTIVE) : res = "someone"; break;
																						case 0x20 + (MD_INDEF_HIDDEN | MD_POSSESSIVE) : res = "someone's"; break;
																							case 0x20 + (MD_INDEF_HIDDEN | MD_POSSESSIVE | MD_OBJECTIVE) : res = "herself"; break;
#endif
		}

		(void)strcpy(desc, res);
		return;
	}

	/* Handle visible monsters, "reflexive" request */
	if ((mode & (MD_POSSESSIVE | MD_OBJECTIVE)) == (MD_POSSESSIVE | MD_OBJECTIVE))
	{
		/* The monster is visible, so use its gender */
		if (r_ptr->flags1 & (RF1_FEMALE)) strcpy(desc, _("彼女自身", "herself"));
		else if (r_ptr->flags1 & (RF1_MALE)) strcpy(desc, _("彼自身", "himself"));
		else strcpy(desc, _("それ自身", "itself"));
		return;
	}

	/* Handle all other visible monster requests */
	/* Tanuki? */
	if (is_pet(m_ptr) && !is_original_ap(m_ptr))
	{
#ifdef JP
		char *t;
		char buf[128];
		strcpy(buf, name);
		t = buf;
		while (strncmp(t, "』", 2) && *t) t++;
		if (*t)
		{
			*t = '\0';
			(void)sprintf(desc, "%s？』", buf);
		}
		else
			(void)sprintf(desc, "%s？", name);
#else
		(void)sprintf(desc, "%s?", name);
#endif
	}
	else
	{
		if ((r_ptr->flags1 & RF1_UNIQUE) && !(player_ptr->image && !(mode & MD_IGNORE_HALLU)))
		{
			if ((m_ptr->mflag2 & MFLAG2_CHAMELEON) && !(mode & MD_TRUE_NAME))
			{
#ifdef JP
				char *t;
				char buf[128];
				strcpy(buf, name);
				t = buf;
				while (strncmp(t, "』", 2) && *t) t++;
				if (*t)
				{
					*t = '\0';
					(void)sprintf(desc, "%s？』", buf);
				}
				else
					(void)sprintf(desc, "%s？", name);
#else
				(void)sprintf(desc, "%s?", name);
#endif
			}
			else if (player_ptr->phase_out &&
				!(player_ptr->riding && (&floor_ptr->m_list[player_ptr->riding] == m_ptr)))
			{
				(void)sprintf(desc, _("%sもどき", "fake %s"), name);
			}
			else
			{
				(void)strcpy(desc, name);
			}
		}
		else if (mode & MD_INDEF_VISIBLE)
		{
#ifdef JP
			(void)strcpy(desc, "");
#else
			(void)strcpy(desc, is_a_vowel(name[0]) ? "an " : "a ");
#endif
			(void)strcat(desc, name);
		}
		else
		{
			if (is_pet(m_ptr))
				(void)strcpy(desc, _("あなたの", "your "));
			else
				(void)strcpy(desc, _("", "the "));

			(void)strcat(desc, name);
		}
	}

	if (m_ptr->nickname)
	{
		char buf[128];
		sprintf(buf, _("「%s」", " called %s"), quark_str(m_ptr->nickname));
		strcat(desc, buf);
	}

	if (player_ptr->riding && (&floor_ptr->m_list[player_ptr->riding] == m_ptr))
	{
		strcat(desc, _("(乗馬中)", "(riding)"));
	}

	if ((mode & MD_IGNORE_HALLU) && (m_ptr->mflag2 & MFLAG2_CHAMELEON))
	{
		if (r_ptr->flags1 & RF1_UNIQUE)
		{
			strcat(desc, _("(カメレオンの王)", "(Chameleon Lord)"));
		}
		else
		{
			strcat(desc, _("(カメレオン)", "(Chameleon)"));
		}
	}

	if ((mode & MD_IGNORE_HALLU) && !is_original_ap(m_ptr))
	{
		strcat(desc, format("(%s)", r_name + r_info[m_ptr->r_idx].name));
	}

	/* Handle the Possessive as a special afterthought */
	if (mode & MD_POSSESSIVE)
	{
		(void)strcat(desc, _("の", "'s"));
	}
}


/*!
* @brief モンスターIDを取り、モンスター名をm_nameに代入する /
* @param player_ptr プレーヤーへの参照ポインタ
* @param m_idx モンスターID
* @param m_name モンスター名を入力する配列
*/
void monster_name(player_type *player_ptr, MONSTER_IDX m_idx, char* m_name)
{
	monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
	monster_desc(player_ptr, m_name, m_ptr, 0x00);
}


/*!
 * @brief モンスターの調査による思い出補完処理 / Learn about a monster (by "probing" it)
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param r_idx 補完されるモンスター種族ID
 * @return 明らかになった情報の度数
 * @details
 * Return the number of new flags learnt.  -Mogami-
 */
int lore_do_probe(player_type *player_ptr, MONRACE_IDX r_idx)
{
	int n = 0;
	monster_race *r_ptr = &r_info[r_idx];
	if (r_ptr->r_wake != MAX_UCHAR) n++;
	if (r_ptr->r_ignore != MAX_UCHAR) n++;
	r_ptr->r_wake = r_ptr->r_ignore = MAX_UCHAR;

	for (int i = 0; i < 4; i++)
	{
		if (r_ptr->blow[i].effect || r_ptr->blow[i].method)
		{
			if (r_ptr->r_blows[i] != MAX_UCHAR) n++;
			r_ptr->r_blows[i] = MAX_UCHAR;
		}
	}

	byte tmp_byte =
		(((r_ptr->flags1 & RF1_DROP_4D2) ? 8 : 0) +
		((r_ptr->flags1 & RF1_DROP_3D2) ? 6 : 0) +
			((r_ptr->flags1 & RF1_DROP_2D2) ? 4 : 0) +
			((r_ptr->flags1 & RF1_DROP_1D2) ? 2 : 0) +
			((r_ptr->flags1 & RF1_DROP_90) ? 1 : 0) +
			((r_ptr->flags1 & RF1_DROP_60) ? 1 : 0));

	if (!(r_ptr->flags1 & RF1_ONLY_GOLD))
	{
		if (r_ptr->r_drop_item != tmp_byte) n++;
		r_ptr->r_drop_item = tmp_byte;
	}
	if (!(r_ptr->flags1 & RF1_ONLY_ITEM))
	{
		if (r_ptr->r_drop_gold != tmp_byte) n++;
		r_ptr->r_drop_gold = tmp_byte;
	}

	if (r_ptr->r_cast_spell != MAX_UCHAR) n++;
	r_ptr->r_cast_spell = MAX_UCHAR;

	for (int i = 0; i < 32; i++)
	{
		if (!(r_ptr->r_flags1 & (1L << i)) &&
			(r_ptr->flags1 & (1L << i))) n++;
		if (!(r_ptr->r_flags2 & (1L << i)) &&
			(r_ptr->flags2 & (1L << i))) n++;
		if (!(r_ptr->r_flags3 & (1L << i)) &&
			(r_ptr->flags3 & (1L << i))) n++;
		if (!(r_ptr->r_flags4 & (1L << i)) &&
			(r_ptr->flags4 & (1L << i))) n++;
		if (!(r_ptr->r_flags5 & (1L << i)) &&
			(r_ptr->a_ability_flags1 & (1L << i))) n++;
		if (!(r_ptr->r_flags6 & (1L << i)) &&
			(r_ptr->a_ability_flags2 & (1L << i))) n++;
		if (!(r_ptr->r_flagsr & (1L << i)) &&
			(r_ptr->flagsr & (1L << i))) n++;
	}

	r_ptr->r_flags1 = r_ptr->flags1;
	r_ptr->r_flags2 = r_ptr->flags2;
	r_ptr->r_flags3 = r_ptr->flags3;
	r_ptr->r_flags4 = r_ptr->flags4;
	r_ptr->r_flags5 = r_ptr->a_ability_flags1;
	r_ptr->r_flags6 = r_ptr->a_ability_flags2;
	r_ptr->r_flagsr = r_ptr->flagsr;

	if (!(r_ptr->r_xtra1 & MR1_SINKA)) n++;
	r_ptr->r_xtra1 |= MR1_SINKA;

	if (player_ptr->monster_race_idx == r_idx)
	{
		player_ptr->window |= (PW_MONSTER);
	}

	return n;
}


/*!
 * @brief モンスターの撃破に伴うドロップ情報の保管処理 / Take note that the given monster just dropped some treasure
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスター情報のID
 * @param num_item 手に入れたアイテム数
 * @param num_gold 手に入れた財宝の単位数
 * @return なし
 * @details
 * Note that learning the "GOOD"/"GREAT" flags gives information
 * about the treasure (even when the monster is killed for the first
 * time, such as uniques, and the treasure has not been examined yet).
 *
 * This "indirect" method is used to prevent the player from learning
 * exactly how much treasure a monster can drop from observing only
 * a single example of a drop.  This method actually observes how much
 * gold and items are dropped, and remembers that information to be
 * described later by the monster recall code.
 */
void lore_treasure(player_type *player_ptr, MONSTER_IDX m_idx, ITEM_NUMBER num_item, ITEM_NUMBER num_gold)
{
	monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	if (!is_original_ap(m_ptr)) return;

	if (num_item > r_ptr->r_drop_item) r_ptr->r_drop_item = num_item;
	if (num_gold > r_ptr->r_drop_gold) r_ptr->r_drop_gold = num_gold;

	if (r_ptr->flags1 & (RF1_DROP_GOOD)) r_ptr->r_flags1 |= (RF1_DROP_GOOD);
	if (r_ptr->flags1 & (RF1_DROP_GREAT)) r_ptr->r_flags1 |= (RF1_DROP_GREAT);
	if (player_ptr->monster_race_idx == m_ptr->r_idx) player_ptr->window |= (PW_MONSTER);
}


/*!
 * @brief モンスターの各情報を更新する / This function updates the monster record of the given monster
 * @param m_idx 更新するモンスター情報のID
 * @param full プレイヤーとの距離更新を行うならばtrue
 * @return なし
 * @details
 * This involves extracting the distance to the player (if requested),
 * and then checking for visibility (natural, infravision, see-invis,
 * telepathy), updating the monster visibility flag, redrawing (or
 * erasing) the monster when its visibility changes, and taking note
 * of any interesting monster flags (cold-blooded, invisible, etc).
 *
 * Note the new "mflag" field which encodes several monster state flags,
 * including "view" for when the monster is currently in line of sight,
 * and "mark" for when the monster is currently visible via detection.
 *
 * The only monster fields that are changed here are "cdis" (the
 * distance from the player), "ml" (visible to the player), and
 * "mflag" (to maintain the "MFLAG_VIEW" flag).
 *
 * Note the special "update_monsters()" function which can be used to
 * call this function once for every monster.
 *
 * Note the "full" flag which requests that the "cdis" field be updated,
 * this is only needed when the monster (or the player) has moved.
 *
 * Every time a monster moves, we must call this function for that
 * monster, and update the distance, and the visibility.  Every time
 * the player moves, we must call this function for every monster, and
 * update the distance, and the visibility.  Whenever the player "state"
 * changes in certain ways ("blindness", "infravision", "telepathy",
 * and "see invisible"), we must call this function for every monster,
 * and update the visibility.
 *
 * Routines that change the "illumination" of a grid must also call this
 * function for any monster in that grid, since the "visibility" of some
 * monsters may be based on the illumination of their grid.
 *
 * Note that this function is called once per monster every time the
 * player moves.  When the player is running, this function is one
 * of the primary bottlenecks, along with "update_view()" and the
 * "process_monsters()" code, so efficiency is important.
 *
 * Note the optimized "inline" version of the "distance()" function.
 *
 * A monster is "visible" to the player if (1) it has been detected
 * by the player, (2) it is close to the player and the player has
 * telepathy, or (3) it is close to the player, and in line of sight
 * of the player, and it is "illuminated" by some combination of
 * infravision, torch light, or permanent light (invisible monsters
 * are only affected by "light" if the player can see invisible).
 *
 * Monsters which are not on the current panel may be "visible" to
 * the player, and their descriptions will include an "offscreen"
 * reference.  Currently, offscreen monsters cannot be targetted
 * or viewed directly, but old targets will remain set.  XXX XXX
 *
 * The player can choose to be disturbed by several things, including
 * "disturb_move" (monster which is viewable moves in some way), and
 * "disturb_near" (monster which is "easily" viewable moves in some
 * way).  Note that "moves" includes "appears" and "disappears".
 */
void update_monster(player_type *subject_ptr, MONSTER_IDX m_idx, bool full)
{
	monster_type *m_ptr = &subject_ptr->current_floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	bool do_disturb = disturb_move;

	POSITION fy = m_ptr->fy;
	POSITION fx = m_ptr->fx;

	bool flag = FALSE;
	bool easy = FALSE;
	bool in_darkness = (d_info[subject_ptr->dungeon_idx].flags1 & DF1_DARKNESS) && !subject_ptr->see_nocto;
	if (disturb_high)
	{
		monster_race *ap_r_ptr = &r_info[m_ptr->ap_r_idx];
		if (ap_r_ptr->r_tkills && ap_r_ptr->level >= subject_ptr->lev)
			do_disturb = TRUE;
	}

	POSITION distance;
	if (full)
	{
		int dy = (subject_ptr->y > fy) ? (subject_ptr->y - fy) : (fy - subject_ptr->y);
		int dx = (subject_ptr->x > fx) ? (subject_ptr->x - fx) : (fx - subject_ptr->x);

		distance = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));
		if (distance > 255) distance = 255;
		if (!distance) distance = 1;

		m_ptr->cdis = distance;
	}
	else
	{
		distance = m_ptr->cdis;
	}

	if (m_ptr->mflag2 & (MFLAG2_MARK)) flag = TRUE;

	if (distance <= (in_darkness ? MAX_SIGHT / 2 : MAX_SIGHT))
	{
		if (!in_darkness || (distance <= MAX_SIGHT / 4))
		{
			if (subject_ptr->special_defense & KATA_MUSOU)
			{
				flag = TRUE;
				if (is_original_ap(m_ptr) && !subject_ptr->image)
				{
					if (r_ptr->flags2 & (RF2_SMART)) r_ptr->r_flags2 |= (RF2_SMART);
					if (r_ptr->flags2 & (RF2_STUPID)) r_ptr->r_flags2 |= (RF2_STUPID);
				}
			}
			else if (subject_ptr->telepathy)
			{
				if (r_ptr->flags2 & (RF2_EMPTY_MIND))
				{
					if (is_original_ap(m_ptr) && !subject_ptr->image) r_ptr->r_flags2 |= (RF2_EMPTY_MIND);
				}
				else if (r_ptr->flags2 & (RF2_WEIRD_MIND))
				{
					if ((m_idx % 10) == 5)
					{
						flag = TRUE;
						if (is_original_ap(m_ptr) && !subject_ptr->image)
						{
							r_ptr->r_flags2 |= (RF2_WEIRD_MIND);
							if (r_ptr->flags2 & (RF2_SMART)) r_ptr->r_flags2 |= (RF2_SMART);
							if (r_ptr->flags2 & (RF2_STUPID)) r_ptr->r_flags2 |= (RF2_STUPID);
						}
					}
				}
				else
				{
					flag = TRUE;
					if (is_original_ap(m_ptr) && !subject_ptr->image)
					{
						if (r_ptr->flags2 & (RF2_SMART)) r_ptr->r_flags2 |= (RF2_SMART);
						if (r_ptr->flags2 & (RF2_STUPID)) r_ptr->r_flags2 |= (RF2_STUPID);
					}
				}
			}

			if ((subject_ptr->esp_animal) && (r_ptr->flags3 & (RF3_ANIMAL)))
			{
				flag = TRUE;
				if (is_original_ap(m_ptr) && !subject_ptr->image) r_ptr->r_flags3 |= (RF3_ANIMAL);
			}

			if ((subject_ptr->esp_undead) && (r_ptr->flags3 & (RF3_UNDEAD)))
			{
				flag = TRUE;
				if (is_original_ap(m_ptr) && !subject_ptr->image) r_ptr->r_flags3 |= (RF3_UNDEAD);
			}

			if ((subject_ptr->esp_demon) && (r_ptr->flags3 & (RF3_DEMON)))
			{
				flag = TRUE;
				if (is_original_ap(m_ptr) && !subject_ptr->image) r_ptr->r_flags3 |= (RF3_DEMON);
			}

			if ((subject_ptr->esp_orc) && (r_ptr->flags3 & (RF3_ORC)))
			{
				flag = TRUE;
				if (is_original_ap(m_ptr) && !subject_ptr->image) r_ptr->r_flags3 |= (RF3_ORC);
			}

			if ((subject_ptr->esp_troll) && (r_ptr->flags3 & (RF3_TROLL)))
			{
				flag = TRUE;
				if (is_original_ap(m_ptr) && !subject_ptr->image) r_ptr->r_flags3 |= (RF3_TROLL);
			}

			if ((subject_ptr->esp_giant) && (r_ptr->flags3 & (RF3_GIANT)))
			{
				flag = TRUE;
				if (is_original_ap(m_ptr) && !subject_ptr->image) r_ptr->r_flags3 |= (RF3_GIANT);
			}

			if ((subject_ptr->esp_dragon) && (r_ptr->flags3 & (RF3_DRAGON)))
			{
				flag = TRUE;
				if (is_original_ap(m_ptr) && !subject_ptr->image) r_ptr->r_flags3 |= (RF3_DRAGON);
			}

			if ((subject_ptr->esp_human) && (r_ptr->flags2 & (RF2_HUMAN)))
			{
				flag = TRUE;
				if (is_original_ap(m_ptr) && !subject_ptr->image) r_ptr->r_flags2 |= (RF2_HUMAN);
			}

			if ((subject_ptr->esp_evil) && (r_ptr->flags3 & (RF3_EVIL)))
			{
				flag = TRUE;
				if (is_original_ap(m_ptr) && !subject_ptr->image) r_ptr->r_flags3 |= (RF3_EVIL);
			}

			if ((subject_ptr->esp_good) && (r_ptr->flags3 & (RF3_GOOD)))
			{
				flag = TRUE;
				if (is_original_ap(m_ptr) && !subject_ptr->image) r_ptr->r_flags3 |= (RF3_GOOD);
			}

			if ((subject_ptr->esp_nonliving) &&
				((r_ptr->flags3 & (RF3_DEMON | RF3_UNDEAD | RF3_NONLIVING)) == RF3_NONLIVING))
			{
				flag = TRUE;
				if (is_original_ap(m_ptr) && !subject_ptr->image) r_ptr->r_flags3 |= (RF3_NONLIVING);
			}

			if ((subject_ptr->esp_unique) && (r_ptr->flags1 & (RF1_UNIQUE)))
			{
				flag = TRUE;
				if (is_original_ap(m_ptr) && !subject_ptr->image) r_ptr->r_flags1 |= (RF1_UNIQUE);
			}
		}

		if (player_has_los_bold(subject_ptr, fy, fx) && !subject_ptr->blind)
		{
			bool do_invisible = FALSE;
			bool do_cold_blood = FALSE;

			if (subject_ptr->concent >= CONCENT_RADAR_THRESHOLD)
			{
				easy = flag = TRUE;
			}

			if (distance <= subject_ptr->see_infra)
			{
				if ((r_ptr->flags2 & (RF2_COLD_BLOOD | RF2_AURA_FIRE)) == RF2_COLD_BLOOD)
				{
					do_cold_blood = TRUE;
				}
				else
				{
					easy = flag = TRUE;
				}
			}

			if (player_can_see_bold(subject_ptr, fy, fx))
			{
				if (r_ptr->flags2 & (RF2_INVISIBLE))
				{
					do_invisible = TRUE;
					if (subject_ptr->see_inv)
					{
						easy = flag = TRUE;
					}
				}
				else
				{
					easy = flag = TRUE;
				}
			}

			if (flag)
			{
				if (is_original_ap(m_ptr) && !subject_ptr->image)
				{
					if (do_invisible) r_ptr->r_flags2 |= (RF2_INVISIBLE);
					if (do_cold_blood) r_ptr->r_flags2 |= (RF2_COLD_BLOOD);
				}
			}
		}
	}

	/* The monster is now visible */
	if (flag)
	{
		if (!m_ptr->ml)
		{
			m_ptr->ml = TRUE;
			lite_spot(subject_ptr, fy, fx);

			if (subject_ptr->health_who == m_idx) subject_ptr->redraw |= (PR_HEALTH);
			if (subject_ptr->riding == m_idx) subject_ptr->redraw |= (PR_UHEALTH);

			if (!subject_ptr->image)
			{
				if ((m_ptr->ap_r_idx == MON_KAGE) && (r_info[MON_KAGE].r_sights < MAX_SHORT))
					r_info[MON_KAGE].r_sights++;
				else if (is_original_ap(m_ptr) && (r_ptr->r_sights < MAX_SHORT))
					r_ptr->r_sights++;
			}

			if (r_info[m_ptr->ap_r_idx].flags2 & RF2_ELDRITCH_HORROR)
			{
				sanity_blast(subject_ptr, m_ptr, FALSE);
			}

			if (disturb_near && (projectable(subject_ptr, m_ptr->fy, m_ptr->fx, subject_ptr->y, subject_ptr->x) && projectable(subject_ptr, subject_ptr->y, subject_ptr->x, m_ptr->fy, m_ptr->fx)))
			{
				if (disturb_pets || is_hostile(m_ptr))
					disturb(subject_ptr, TRUE, TRUE);
			}
		}
	}

	/* The monster is not visible */
	else
	{
		if (m_ptr->ml)
		{
			m_ptr->ml = FALSE;
			lite_spot(subject_ptr, fy, fx);

			if (subject_ptr->health_who == m_idx) subject_ptr->redraw |= (PR_HEALTH);
			if (subject_ptr->riding == m_idx) subject_ptr->redraw |= (PR_UHEALTH);
			if (do_disturb)
			{
				if (disturb_pets || is_hostile(m_ptr))
					disturb(subject_ptr, TRUE, TRUE);
			}
		}
	}

	/* The monster is now easily visible */
	if (easy)
	{
		if (!(m_ptr->mflag & (MFLAG_VIEW)))
		{
			m_ptr->mflag |= (MFLAG_VIEW);
			if (do_disturb)
			{
				if (disturb_pets || is_hostile(m_ptr))
					disturb(subject_ptr, TRUE, TRUE);
			}
		}

		return;
	}

	/* The monster is not easily visible */
	/* Change */
	if (!(m_ptr->mflag & (MFLAG_VIEW))) return;

	/* Mark as not easily visible */
	m_ptr->mflag &= ~(MFLAG_VIEW);

	if (do_disturb)
	{
		if (disturb_pets || is_hostile(m_ptr))
			disturb(subject_ptr, TRUE, TRUE);
	}
}


/*!
 * @param player_ptr プレーヤーへの参照ポインタ
 * @brief 単純に生存している全モンスターの更新処理を行う / This function simply updates all the (non-dead) monsters (see above).
 * @param full 距離更新を行うならtrue
 * @return なし
 */
void update_monsters(player_type *player_ptr, bool full)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	for (MONSTER_IDX i = 1; i < floor_ptr->m_max; i++)
	{
		monster_type *m_ptr = &floor_ptr->m_list[i];
		if (!monster_is_valid(m_ptr)) continue;
		update_monster(player_ptr, i, full);
	}
}


/*!
 * todo ここにplayer_typeを追加すると関数ポインタ周りの収拾がつかなくなるので保留
 * @param player_ptr プレーヤーへの参照ポインタ
 * @brief カメレオンの王の変身対象となるモンスターかどうか判定する / Hack -- the index of the summoning monster
 * @param r_idx モンスター種族ID
 * @return 対象にできるならtrueを返す
 */
static bool monster_hook_chameleon_lord(MONRACE_IDX r_idx)
{
	floor_type *floor_ptr = p_ptr->current_floor_ptr;
	monster_race *r_ptr = &r_info[r_idx];
	monster_type *m_ptr = &floor_ptr->m_list[chameleon_change_m_idx];
	monster_race *old_r_ptr = &r_info[m_ptr->r_idx];

	if (!(r_ptr->flags1 & (RF1_UNIQUE))) return FALSE;
	if (r_ptr->flags7 & (RF7_FRIENDLY | RF7_CHAMELEON)) return FALSE;

	if (ABS(r_ptr->level - r_info[MON_CHAMELEON_K].level) > 5) return FALSE;

	if ((r_ptr->blow[0].method == RBM_EXPLODE) || (r_ptr->blow[1].method == RBM_EXPLODE) || (r_ptr->blow[2].method == RBM_EXPLODE) || (r_ptr->blow[3].method == RBM_EXPLODE))
		return FALSE;

	if (!monster_can_cross_terrain(p_ptr, floor_ptr->grid_array[m_ptr->fy][m_ptr->fx].feat, r_ptr, 0)) return FALSE;

	if (!(old_r_ptr->flags7 & RF7_CHAMELEON))
	{
		if (monster_has_hostile_align(p_ptr, m_ptr, 0, 0, r_ptr)) return FALSE;
	}
	else if (summon_specific_who > 0)
	{
		if (monster_has_hostile_align(p_ptr, &floor_ptr->m_list[summon_specific_who], 0, 0, r_ptr)) return FALSE;
	}

	return TRUE;
}


/*!
 * todo ここにplayer_typeを追加すると関数ポインタ周りの収拾がつかなくなるので保留
 * @brief カメレオンの変身対象となるモンスターかどうか判定する / Hack -- the index of the summoning monster
 * @param r_idx モンスター種族ID
 * @return 対象にできるならtrueを返す
 * @todo グローバル変数対策の上 monster_hook.cへ移す。
 */
static bool monster_hook_chameleon(MONRACE_IDX r_idx)
{
	floor_type *floor_ptr = p_ptr->current_floor_ptr;
	monster_race *r_ptr = &r_info[r_idx];
	monster_type *m_ptr = &floor_ptr->m_list[chameleon_change_m_idx];
	monster_race *old_r_ptr = &r_info[m_ptr->r_idx];

	if (r_ptr->flags1 & (RF1_UNIQUE)) return FALSE;
	if (r_ptr->flags2 & RF2_MULTIPLY) return FALSE;
	if (r_ptr->flags7 & (RF7_FRIENDLY | RF7_CHAMELEON)) return FALSE;

	if ((r_ptr->blow[0].method == RBM_EXPLODE) || (r_ptr->blow[1].method == RBM_EXPLODE) || (r_ptr->blow[2].method == RBM_EXPLODE) || (r_ptr->blow[3].method == RBM_EXPLODE))
		return FALSE;

	if (!monster_can_cross_terrain(p_ptr, floor_ptr->grid_array[m_ptr->fy][m_ptr->fx].feat, r_ptr, 0)) return FALSE;

	if (!(old_r_ptr->flags7 & RF7_CHAMELEON))
	{
		if ((old_r_ptr->flags3 & RF3_GOOD) && !(r_ptr->flags3 & RF3_GOOD)) return FALSE;
		if ((old_r_ptr->flags3 & RF3_EVIL) && !(r_ptr->flags3 & RF3_EVIL)) return FALSE;
		if (!(old_r_ptr->flags3 & (RF3_GOOD | RF3_EVIL)) && (r_ptr->flags3 & (RF3_GOOD | RF3_EVIL))) return FALSE;
	}
	else if (summon_specific_who > 0)
	{
		if (monster_has_hostile_align(p_ptr, &floor_ptr->m_list[summon_specific_who], 0, 0, r_ptr)) return FALSE;
	}

	return (*(get_monster_hook(p_ptr)))(r_idx);
}


/*!
 * @brief モンスターの変身処理
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param m_idx 変身処理を受けるモンスター情報のID
 * @param born 生成時の初変身先指定ならばtrue
 * @param r_idx 旧モンスター種族のID
 * @return なし
 */
void choose_new_monster(player_type *player_ptr, MONSTER_IDX m_idx, bool born, MONRACE_IDX r_idx)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	monster_type *m_ptr = &floor_ptr->m_list[m_idx];
	monster_race *r_ptr;

	bool old_unique = FALSE;
	if (r_info[m_ptr->r_idx].flags1 & RF1_UNIQUE)
		old_unique = TRUE;
	if (old_unique && (r_idx == MON_CHAMELEON)) r_idx = MON_CHAMELEON_K;
	r_ptr = &r_info[r_idx];

	char old_m_name[MAX_NLEN];
	monster_desc(player_ptr, old_m_name, m_ptr, 0);

	if (!r_idx)
	{
		DEPTH level;

		chameleon_change_m_idx = m_idx;
		if (old_unique)
			get_mon_num_prep(player_ptr, monster_hook_chameleon_lord, NULL);
		else
			get_mon_num_prep(player_ptr, monster_hook_chameleon, NULL);

		if (old_unique)
			level = r_info[MON_CHAMELEON_K].level;
		else if (!floor_ptr->dun_level)
			level = wilderness[player_ptr->wilderness_y][player_ptr->wilderness_x].level;
		else
			level = floor_ptr->dun_level;

		if (d_info[player_ptr->dungeon_idx].flags1 & DF1_CHAMELEON) level += 2 + randint1(3);

		r_idx = get_mon_num(player_ptr, level, 0);
		r_ptr = &r_info[r_idx];

		chameleon_change_m_idx = 0;
		if (!r_idx) return;
	}

	m_ptr->r_idx = r_idx;
	m_ptr->ap_r_idx = r_idx;
	update_monster(player_ptr, m_idx, FALSE);
	lite_spot(player_ptr, m_ptr->fy, m_ptr->fx);

	int old_r_idx = m_ptr->r_idx;
	if ((r_info[old_r_idx].flags7 & (RF7_LITE_MASK | RF7_DARK_MASK)) ||
		(r_ptr->flags7 & (RF7_LITE_MASK | RF7_DARK_MASK)))
		player_ptr->update |= (PU_MON_LITE);

	if (born)
	{
		if (r_ptr->flags3 & (RF3_EVIL | RF3_GOOD))
		{
			m_ptr->sub_align = SUB_ALIGN_NEUTRAL;
			if (r_ptr->flags3 & RF3_EVIL) m_ptr->sub_align |= SUB_ALIGN_EVIL;
			if (r_ptr->flags3 & RF3_GOOD) m_ptr->sub_align |= SUB_ALIGN_GOOD;
		}

		return;
	}

	if (m_idx == player_ptr->riding)
	{
		GAME_TEXT m_name[MAX_NLEN];
		monster_desc(player_ptr, m_name, m_ptr, 0);
		msg_format(_("突然%sが変身した。", "Suddenly, %s transforms!"), old_m_name);
		if (!(r_ptr->flags7 & RF7_RIDING))
			if (rakuba(player_ptr, 0, TRUE)) msg_format(_("地面に落とされた。", "You have fallen from %s."), m_name);
	}

	m_ptr->mspeed = get_mspeed(player_ptr, r_ptr);

	int oldmaxhp = m_ptr->max_maxhp;
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
		m_ptr->max_maxhp = (HIT_POINT)MIN(30000, hp);
	}

	m_ptr->maxhp = (long)(m_ptr->maxhp * m_ptr->max_maxhp) / oldmaxhp;
	if (m_ptr->maxhp < 1) m_ptr->maxhp = 1;
	m_ptr->hp = (long)(m_ptr->hp * m_ptr->max_maxhp) / oldmaxhp;
	m_ptr->dealt_damage = 0;
}


/*!
 * todo ここにplayer_typeを追加すると関数ポインタ周りの収拾がつかなくなるので保留
 * @brief たぬきの変身対象となるモンスターかどうか判定する / Hook for Tanuki
 * @param r_idx モンスター種族ID
 * @return 対象にできるならtrueを返す
 * @todo グローバル変数対策の上 monster_hook.cへ移す。
 */
static bool monster_hook_tanuki(MONRACE_IDX r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags1 & (RF1_UNIQUE)) return FALSE;
	if (r_ptr->flags2 & RF2_MULTIPLY) return FALSE;
	if (r_ptr->flags7 & (RF7_FRIENDLY | RF7_CHAMELEON)) return FALSE;
	if (r_ptr->flags7 & RF7_AQUATIC) return FALSE;

	if ((r_ptr->blow[0].method == RBM_EXPLODE) || (r_ptr->blow[1].method == RBM_EXPLODE) || (r_ptr->blow[2].method == RBM_EXPLODE) || (r_ptr->blow[3].method == RBM_EXPLODE))
		return FALSE;

	return (*(get_monster_hook(p_ptr)))(r_idx);
}


/*!
 * @param player_ptr プレーヤーへの参照ポインタ
 * @brief モンスターの表層IDを設定する / Set initial racial appearance of a monster
 * @param r_idx モンスター種族ID
 * @return モンスター種族の表層ID
 */
static MONRACE_IDX initial_r_appearance(player_type *player_ptr, MONRACE_IDX r_idx, BIT_FLAGS generate_mode)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	if ((generate_mode | PM_JURAL) && !(generate_mode & (PM_MULTIPLY | PM_KAGE)))
	{
		return MON_ALIEN_JURAL;
	}

	if (!(r_info[r_idx].flags7 & RF7_TANUKI))
		return r_idx;

	get_mon_num_prep(player_ptr, monster_hook_tanuki, NULL);

	int attempts = 1000;
	DEPTH min = MIN(floor_ptr->base_level - 5, 50);
	while (--attempts)
	{
		MONRACE_IDX ap_r_idx = get_mon_num(player_ptr, floor_ptr->base_level + 10, 0);
		if (r_info[ap_r_idx].level >= min) return ap_r_idx;
	}

	return r_idx;
}


/*!
 * todo ここには本来floor_type*を追加したいが、monster.hにfloor.hの参照を追加するとコンパイルエラーが出るので保留
 * @brief モンスターの個体加速を設定する / Get initial monster speed
 * @param r_ptr モンスター種族の参照ポインタ
 * @return 加速値
 */
SPEED get_mspeed(player_type *player_ptr, monster_race *r_ptr)
{
	SPEED mspeed = r_ptr->speed;
	if (!(r_ptr->flags1 & RF1_UNIQUE) && !player_ptr->current_floor_ptr->inside_arena)
	{
		/* Allow some small variation per monster */
		int i = SPEED_TO_ENERGY(r_ptr->speed) / (one_in_(4) ? 3 : 10);
		if (i) mspeed += rand_spread(0, i);
	}

	if (mspeed > 199) mspeed = 199;

	return mspeed;
}


/*!
 * @brief モンスターを一体生成する / Attempt to place a monster of the given race at the given location.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param who 召喚を行ったモンスターID
 * @param y 生成位置y座標
 * @param x 生成位置x座標
 * @param r_idx 生成モンスター種族
 * @param mode 生成オプション
 * @return 成功したらtrue
 * @details
 * To give the player a sporting chance, any monster that appears in
 * line-of-sight and is extremely dangerous can be marked as
 * "FORCE_SLEEP", which will cause them to be placed with low energy,
 * which often (but not always) lets the player move before they do.
 *
 * This routine refuses to place out-of-depth "FORCE_DEPTH" monsters.
 *
 * Use special "here" and "dead" flags for unique monsters,
 * remove old "cur_num" and "max_num" fields.
 *
 * Actually, do something similar for artifacts, to simplify
 * the "preserve" mode, and to make the "what artifacts" flag more useful.
 *
 * This is the only function which may place a monster in the dungeon,
 * except for the savefile loading code.
 */
static bool place_monster_one(player_type *player_ptr, MONSTER_IDX who, POSITION y, POSITION x, MONRACE_IDX r_idx, BIT_FLAGS mode)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	grid_type *g_ptr = &floor_ptr->grid_array[y][x];
	monster_type *m_ptr;
	monster_race *r_ptr = &r_info[r_idx];
	concptr name = (r_name + r_ptr->name);

	if (player_ptr->wild_mode) return FALSE;
	if (!in_bounds(floor_ptr, y, x)) return FALSE;
	if (!r_idx) return FALSE;
	if (!r_ptr->name) return FALSE;

	if (!(mode & PM_IGNORE_TERRAIN))
	{
		if (pattern_tile(floor_ptr, y, x)) return FALSE;
		if (!monster_can_enter(player_ptr, y, x, r_ptr, 0)) return FALSE;
	}

	if (!player_ptr->phase_out)
	{
		if (((r_ptr->flags1 & (RF1_UNIQUE)) ||
			(r_ptr->flags7 & (RF7_NAZGUL))) &&
			(r_ptr->cur_num >= r_ptr->max_num))
		{
			return FALSE;
		}

		if ((r_ptr->flags7 & (RF7_UNIQUE2)) &&
			(r_ptr->cur_num >= 1))
		{
			return FALSE;
		}

		if (r_idx == MON_BANORLUPART)
		{
			if (r_info[MON_BANOR].cur_num > 0) return FALSE;
			if (r_info[MON_LUPART].cur_num > 0) return FALSE;
		}

		if ((r_ptr->flags1 & (RF1_FORCE_DEPTH)) && (floor_ptr->dun_level < r_ptr->level) &&
			(!ironman_nightmare || (r_ptr->flags1 & (RF1_QUESTOR))))
		{
			return FALSE;
		}
	}

	if (quest_number(player_ptr, floor_ptr->dun_level))
	{
		int hoge = quest_number(player_ptr, floor_ptr->dun_level);
		if ((quest[hoge].type == QUEST_TYPE_KILL_LEVEL) || (quest[hoge].type == QUEST_TYPE_RANDOM))
		{
			if (r_idx == quest[hoge].r_idx)
			{
				int number_mon, i2, j2;
				number_mon = 0;

				for (i2 = 0; i2 < floor_ptr->width; ++i2)
					for (j2 = 0; j2 < floor_ptr->height; j2++)
						if (floor_ptr->grid_array[j2][i2].m_idx > 0)
							if (floor_ptr->m_list[floor_ptr->grid_array[j2][i2].m_idx].r_idx == quest[hoge].r_idx)
								number_mon++;
				if (number_mon + quest[hoge].cur_num >= quest[hoge].max_num)
					return FALSE;
			}
		}
	}

	if (is_glyph_grid(g_ptr))
	{
		if (randint1(BREAK_GLYPH) < (r_ptr->level + 20))
		{
			if (g_ptr->info & CAVE_MARK)
			{
				msg_print(_("守りのルーンが壊れた！", "The rune of protection is broken!"));
			}

			g_ptr->info &= ~(CAVE_MARK);
			g_ptr->info &= ~(CAVE_OBJECT);
			g_ptr->mimic = 0;

			note_spot(player_ptr, y, x);
		}
		else return FALSE;
	}

	msg_format_wizard(CHEAT_MONSTER, _("%s(Lv%d)を生成しました。", "%s(Lv%d) was generated."), name, r_ptr->level);
	if ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags7 & RF7_NAZGUL) || (r_ptr->level < 10)) mode &= ~PM_KAGE;

	g_ptr->m_idx = m_pop(player_ptr);
	hack_m_idx_ii = g_ptr->m_idx;
	if (!g_ptr->m_idx) return FALSE;

	m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
	m_ptr->r_idx = r_idx;
	m_ptr->ap_r_idx = initial_r_appearance(player_ptr, r_idx, mode);

	m_ptr->mflag = 0;
	m_ptr->mflag2 = 0;
	if ((mode & PM_MULTIPLY) && (who > 0) && !is_original_ap(&floor_ptr->m_list[who]))
	{
		m_ptr->ap_r_idx = floor_ptr->m_list[who].ap_r_idx;
		if (floor_ptr->m_list[who].mflag2 & MFLAG2_KAGE) m_ptr->mflag2 |= MFLAG2_KAGE;
	}

	if ((who > 0) && !(r_ptr->flags3 & (RF3_EVIL | RF3_GOOD)))
		m_ptr->sub_align = floor_ptr->m_list[who].sub_align;
	else
	{
		m_ptr->sub_align = SUB_ALIGN_NEUTRAL;
		if (r_ptr->flags3 & RF3_EVIL) m_ptr->sub_align |= SUB_ALIGN_EVIL;
		if (r_ptr->flags3 & RF3_GOOD) m_ptr->sub_align |= SUB_ALIGN_GOOD;
	}

	m_ptr->fy = y;
	m_ptr->fx = x;
	m_ptr->current_floor_ptr = floor_ptr;

	for (int cmi = 0; cmi < MAX_MTIMED; cmi++) m_ptr->mtimed[cmi] = 0;

	m_ptr->cdis = 0;
	reset_target(m_ptr);
	m_ptr->nickname = 0;
	m_ptr->exp = 0;

	if (who > 0 && is_pet(&floor_ptr->m_list[who]))
	{
		mode |= PM_FORCE_PET;
		m_ptr->parent_m_idx = who;
	}
	else
	{
		m_ptr->parent_m_idx = 0;
	}

	if (r_ptr->flags7 & RF7_CHAMELEON)
	{
		choose_new_monster(player_ptr, g_ptr->m_idx, TRUE, 0);
		r_ptr = &r_info[m_ptr->r_idx];
		m_ptr->mflag2 |= MFLAG2_CHAMELEON;
		if ((r_ptr->flags1 & RF1_UNIQUE) && (who <= 0))
			m_ptr->sub_align = SUB_ALIGN_NEUTRAL;
	}
	else if ((mode & PM_KAGE) && !(mode & PM_FORCE_PET))
	{
		m_ptr->ap_r_idx = MON_KAGE;
		m_ptr->mflag2 |= MFLAG2_KAGE;
	}

	if (mode & PM_NO_PET) m_ptr->mflag2 |= MFLAG2_NOPET;

	m_ptr->ml = FALSE;
	if (mode & PM_FORCE_PET)
	{
		set_pet(player_ptr, m_ptr);
	}
	else if ((r_ptr->flags7 & RF7_FRIENDLY) ||
		(mode & PM_FORCE_FRIENDLY) || is_friendly_idx(player_ptr, who))
	{
		if (!monster_has_hostile_align(player_ptr, NULL, 0, -1, r_ptr)) set_friendly(m_ptr);
	}

	m_ptr->mtimed[MTIMED_CSLEEP] = 0;
	if ((mode & PM_ALLOW_SLEEP) && r_ptr->sleep && !ironman_nightmare)
	{
		int val = r_ptr->sleep;
		(void)set_monster_csleep(player_ptr, g_ptr->m_idx, (val * 2) + randint1(val * 10));
	}

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

		m_ptr->max_maxhp = (HIT_POINT)MIN(30000, hp);
	}

	m_ptr->maxhp = m_ptr->max_maxhp;
	if (m_ptr->r_idx == MON_WOUNDED_BEAR)
		m_ptr->hp = m_ptr->maxhp / 2;
	else m_ptr->hp = m_ptr->maxhp;

	m_ptr->dealt_damage = 0;

	m_ptr->mspeed = get_mspeed(player_ptr, r_ptr);

	if (mode & PM_HASTE) (void)set_monster_fast(player_ptr, g_ptr->m_idx, 100);

	if (!ironman_nightmare)
	{
		m_ptr->energy_need = ENERGY_NEED() - (s16b)randint0(100);
	}
	else
	{
		m_ptr->energy_need = ENERGY_NEED() - (s16b)randint0(100) * 2;
	}

	if ((r_ptr->flags1 & RF1_FORCE_SLEEP) && !ironman_nightmare)
	{
		m_ptr->mflag |= (MFLAG_NICE);
		repair_monsters = TRUE;
	}

	if (g_ptr->m_idx < hack_m_idx)
	{
		m_ptr->mflag |= (MFLAG_BORN);
	}

	if (r_ptr->flags7 & RF7_SELF_LD_MASK)
		player_ptr->update |= (PU_MON_LITE);
	else if ((r_ptr->flags7 & RF7_HAS_LD_MASK) && !MON_CSLEEP(m_ptr))
		player_ptr->update |= (PU_MON_LITE);
	update_monster(player_ptr, g_ptr->m_idx, TRUE);

	real_r_ptr(m_ptr)->cur_num++;

	/*
	 * Memorize location of the unique monster in saved floors.
	 * A unique monster move from old saved floor.
	 */
	if (current_world_ptr->character_dungeon &&
		((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags7 & RF7_NAZGUL)))
		real_r_ptr(m_ptr)->floor_id = player_ptr->floor_id;

	if (r_ptr->flags2 & RF2_MULTIPLY) floor_ptr->num_repro++;

	if (player_ptr->warning && current_world_ptr->character_dungeon)
	{
		if (r_ptr->flags1 & RF1_UNIQUE)
		{
			concptr color;
			object_type *o_ptr;
			GAME_TEXT o_name[MAX_NLEN];

			if (r_ptr->level > player_ptr->lev + 30)
				color = _("黒く", "black");
			else if (r_ptr->level > player_ptr->lev + 15)
				color = _("紫色に", "purple");
			else if (r_ptr->level > player_ptr->lev + 5)
				color = _("ルビー色に", "deep red");
			else if (r_ptr->level > player_ptr->lev - 5)
				color = _("赤く", "red");
			else if (r_ptr->level > player_ptr->lev - 15)
				color = _("ピンク色に", "pink");
			else
				color = _("白く", "white");

			o_ptr = choose_warning_item(player_ptr);
			if (o_ptr)
			{
				object_desc(player_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
				msg_format(_("%sは%s光った。", "%s glows %s."), o_name, color);
			}
			else
			{
				msg_format(_("%s光る物が頭に浮かんだ。", "An %s image forms in your mind."), color);
			}
		}
	}

	if (!is_explosive_rune_grid(g_ptr)) return TRUE;

	if (randint1(BREAK_MINOR_GLYPH) > r_ptr->level)
	{
		if (g_ptr->info & CAVE_MARK)
		{
			msg_print(_("ルーンが爆発した！", "The rune explodes!"));
			project(player_ptr, 0, 2, y, x, 2 * (player_ptr->lev + damroll(7, 7)), GF_MANA, (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI), -1);
		}
	}
	else
	{
		msg_print(_("爆発のルーンは解除された。", "An explosive rune was disarmed."));
	}

	g_ptr->info &= ~(CAVE_MARK);
	g_ptr->info &= ~(CAVE_OBJECT);
	g_ptr->mimic = 0;

	note_spot(player_ptr, y, x);
	lite_spot(player_ptr, y, x);

	return TRUE;
}


/*!
 * @brief モンスター1体を目標地点に可能な限り近い位置に生成する / improved version of scatter() for place monster
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param r_idx 生成モンスター種族
 * @param yp 結果生成位置y座標
 * @param xp 結果生成位置x座標
 * @param y 中心生成位置y座標
 * @param x 中心生成位置x座標
 * @param max_dist 生成位置の最大半径
 * @return 成功したらtrue
 *
 */
static bool mon_scatter(player_type *player_ptr, MONRACE_IDX r_idx, POSITION *yp, POSITION *xp, POSITION y, POSITION x, POSITION max_dist)
{
	POSITION place_x[MON_SCAT_MAXD];
	POSITION place_y[MON_SCAT_MAXD];
	int num[MON_SCAT_MAXD];

	if (max_dist >= MON_SCAT_MAXD)
		return FALSE;

	int i;
	for (i = 0; i < MON_SCAT_MAXD; i++)
		num[i] = 0;

	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	for (POSITION nx = x - max_dist; nx <= x + max_dist; nx++)
	{
		for (POSITION ny = y - max_dist; ny <= y + max_dist; ny++)
		{
			if (!in_bounds(floor_ptr, ny, nx)) continue;
			if (!projectable(player_ptr, y, x, ny, nx)) continue;
			if (r_idx > 0)
			{
				monster_race *r_ptr = &r_info[r_idx];
				if (!monster_can_enter(player_ptr, ny, nx, r_ptr, 0))
					continue;
			}
			else
			{
				if (!is_cave_empty_bold2(player_ptr, ny, nx)) continue;
				if (pattern_tile(floor_ptr, ny, nx)) continue;
			}

			i = distance(y, x, ny, nx);
			if (i > max_dist)
				continue;

			num[i]++;
			if (one_in_(num[i]))
			{
				place_x[i] = nx;
				place_y[i] = ny;
			}
		}
	}

	i = 0;
	while (i < MON_SCAT_MAXD && 0 == num[i])
		i++;
	if (i >= MON_SCAT_MAXD)
		return FALSE;

	*xp = place_x[i];
	*yp = place_y[i];

	return TRUE;
}


/*!
 * @brief モンスターを目標地点に集団生成する / Attempt to place a "group" of monsters around the given location
 * @param who 召喚主のモンスター情報ID
 * @param y 中心生成位置y座標
 * @param x 中心生成位置x座標
 * @param r_idx 生成モンスター種族
 * @param mode 生成オプション
 * @return 成功したらtrue
 */
static bool place_monster_group(player_type *player_ptr, MONSTER_IDX who, POSITION y, POSITION x, MONRACE_IDX r_idx, BIT_FLAGS mode)
{
	monster_race *r_ptr = &r_info[r_idx];
	int total = randint1(10);

	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	int extra = 0;
	if (r_ptr->level > floor_ptr->dun_level)
	{
		extra = r_ptr->level - floor_ptr->dun_level;
		extra = 0 - randint1(extra);
	}
	else if (r_ptr->level < floor_ptr->dun_level)
	{
		extra = floor_ptr->dun_level - r_ptr->level;
		extra = randint1(extra);
	}

	if (extra > 9) extra = 9;

	total += extra;

	if (total < 1) total = 1;
	if (total > GROUP_MAX) total = GROUP_MAX;

	int hack_n = 1;
	POSITION hack_x[GROUP_MAX];
	hack_x[0] = x;
	POSITION hack_y[GROUP_MAX];
	hack_y[0] = y;

	for (int n = 0; (n < hack_n) && (hack_n < total); n++)
	{
		POSITION hx = hack_x[n];
		POSITION hy = hack_y[n];
		for (int i = 0; (i < 8) && (hack_n < total); i++)
		{
			POSITION mx, my;
			scatter(player_ptr, &my, &mx, hy, hx, 4, 0);
			if (!is_cave_empty_bold2(player_ptr, my, mx)) continue;

			if (place_monster_one(player_ptr, who, my, mx, r_idx, mode))
			{
				hack_y[hack_n] = my;
				hack_x[hack_n] = mx;
				hack_n++;
			}
		}
	}

	return TRUE;
}


/*!
 * @var place_monster_idx
 * @brief 護衛対象となるモンスター種族IDを渡すグローバル変数 / Hack -- help pick an escort type
 * @todo 関数ポインタの都合を配慮しながら、グローバル変数place_monster_idxを除去し、関数引数化する
 */
static MONSTER_IDX place_monster_idx = 0;

/*!
 * @var place_monster_m_idx
 * @brief 護衛対象となるモンスターIDを渡すグローバル変数 / Hack -- help pick an escort type
 * @todo 関数ポインタの都合を配慮しながら、グローバル変数place_monster_m_idxを除去し、関数引数化する
 */
static MONSTER_IDX place_monster_m_idx = 0;

/*!
 * todo ここには本来floor_type*を追加したいが、monster.hにfloor.hの参照を追加するとコンパイルエラーが出るので保留
 * todo ここにplayer_typeを追加すると関数ポインタ周りの収拾がつかなくなるので保留
 * @brief モンスター種族が召喚主の護衛となれるかどうかをチェックする / Hack -- help pick an escort type
 * @param r_idx チェックするモンスター種族のID
 * @return 護衛にできるならばtrue
 */
static bool place_monster_can_escort(MONRACE_IDX r_idx)
{
	monster_race *r_ptr = &r_info[place_monster_idx];
	monster_type *m_ptr = &p_ptr->current_floor_ptr->m_list[place_monster_m_idx];
	monster_race *z_ptr = &r_info[r_idx];

	if (mon_hook_dungeon(place_monster_idx) != mon_hook_dungeon(r_idx)) return FALSE;
	if (z_ptr->d_char != r_ptr->d_char) return FALSE;
	if (z_ptr->level > r_ptr->level) return FALSE;
	if (z_ptr->flags1 & RF1_UNIQUE) return FALSE;
	if (place_monster_idx == r_idx) return FALSE;
	if (monster_has_hostile_align(p_ptr, m_ptr, 0, 0, z_ptr)) return FALSE;

	if (r_ptr->flags7 & RF7_FRIENDLY)
	{
		if (monster_has_hostile_align(p_ptr, NULL, 1, -1, z_ptr)) return FALSE;
	}

	if ((r_ptr->flags7 & RF7_CHAMELEON) && !(z_ptr->flags7 & RF7_CHAMELEON))
		return FALSE;

	return TRUE;
}


/*!
 * @brief 一般的なモンスター生成処理のサブルーチン / Attempt to place a monster of the given race at the given location
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param who 召喚主のモンスター情報ID
 * @param y 生成地点y座標
 * @param x 生成地点x座標
 * @param r_idx 生成するモンスターの種族ID
 * @param mode 生成オプション
 * @return 生成に成功したらtrue
 * @details
 * Note that certain monsters are now marked as requiring "friends".
 * These monsters, if successfully placed, and if the "grp" parameter
 * is TRUE, will be surrounded by a "group" of identical monsters.
 *
 * Note that certain monsters are now marked as requiring an "escort",
 * which is a collection of monsters with similar "race" but lower level.
 *
 * Some monsters induce a fake "group" flag on their escorts.
 *
 * Note the "bizarre" use of non-recursion to prevent annoying output
 * when running a code profiler.
 *
 * Note the use of the new "monster allocation table" code to restrict
 * the "get_mon_num()" function to "legal" escort types.
 */
bool place_monster_aux(player_type *player_ptr, MONSTER_IDX who, POSITION y, POSITION x, MONRACE_IDX r_idx, BIT_FLAGS mode)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (!(mode & PM_NO_KAGE) && one_in_(333))
		mode |= PM_KAGE;

	if (!place_monster_one(player_ptr, who, y, x, r_idx, mode)) return FALSE;
	if (!(mode & PM_ALLOW_GROUP)) return TRUE;

	place_monster_m_idx = hack_m_idx_ii;

	/* Reinforcement */
	for (int i = 0; i < 6; i++)
	{
		if (!r_ptr->reinforce_id[i]) break;
		int n = damroll(r_ptr->reinforce_dd[i], r_ptr->reinforce_ds[i]);
		for (int j = 0; j < n; j++)
		{
			POSITION nx, ny, d = 7;
			scatter(player_ptr, &ny, &nx, y, x, d, 0);
			(void)place_monster_one(player_ptr, place_monster_m_idx, ny, nx, r_ptr->reinforce_id[i], mode);
		}
	}

	if (r_ptr->flags1 & (RF1_FRIENDS))
	{
		(void)place_monster_group(player_ptr, who, y, x, r_idx, mode);
	}

	if (!(r_ptr->flags1 & (RF1_ESCORT))) return TRUE;

	place_monster_idx = r_idx;
	for (int i = 0; i < 32; i++)
	{
		POSITION nx, ny, d = 3;
		MONRACE_IDX z;
		scatter(player_ptr, &ny, &nx, y, x, d, 0);
		if (!is_cave_empty_bold2(player_ptr, ny, nx)) continue;

		get_mon_num_prep(player_ptr, place_monster_can_escort, get_monster_hook2(player_ptr, ny, nx));
		z = get_mon_num(player_ptr, r_ptr->level, 0);
		if (!z) break;

		(void)place_monster_one(player_ptr, place_monster_m_idx, ny, nx, z, mode);
		if ((r_info[z].flags1 & RF1_FRIENDS) ||
			(r_ptr->flags1 & RF1_ESCORTS))
		{
			(void)place_monster_group(player_ptr, place_monster_m_idx, ny, nx, z, mode);
		}
	}

	return TRUE;
}


/*!
 * @brief 一般的なモンスター生成処理のメインルーチン / Attempt to place a monster of the given race at the given location
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y 生成地点y座標
 * @param x 生成地点x座標
 * @param mode 生成オプション
 * @return 生成に成功したらtrue
 */
bool place_monster(player_type *player_ptr, POSITION y, POSITION x, BIT_FLAGS mode)
{
	MONRACE_IDX r_idx;
	get_mon_num_prep(player_ptr, get_monster_hook(player_ptr), get_monster_hook2(player_ptr, y, x));
	r_idx = get_mon_num(player_ptr, player_ptr->current_floor_ptr->monster_level, 0);
	if (!r_idx) return FALSE;

	if ((one_in_(5) || (player_ptr->current_floor_ptr->base_level == 0)) &&
		!(r_info[r_idx].flags1 & RF1_UNIQUE) && my_strchr("hkoptuyAHLOPTUVY", r_info[r_idx].d_char))
	{
		mode |= PM_JURAL;
	}

	if (place_monster_aux(player_ptr, 0, y, x, r_idx, mode)) return TRUE;

	return FALSE;
}


/*!
 * @brief 指定地点に1種類のモンスター種族による群れを生成する
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y 生成地点y座標
 * @param x 生成地点x座標
 * @return 生成に成功したらtrue
 */
bool alloc_horde(player_type *player_ptr, POSITION y, POSITION x)
{
	get_mon_num_prep(player_ptr, get_monster_hook(player_ptr), get_monster_hook2(player_ptr, y, x));

	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	MONRACE_IDX r_idx = 0;
	int attempts = 1000;
	monster_race *r_ptr = NULL;
	while (--attempts)
	{
		r_idx = get_mon_num(player_ptr, floor_ptr->monster_level, 0);
		if (!r_idx) return FALSE;

		r_ptr = &r_info[r_idx];
		if (r_ptr->flags1 & RF1_UNIQUE) continue;

		if (r_idx == MON_HAGURE) continue;
		break;
	}

	if (attempts < 1) return FALSE;

	attempts = 1000;

	while (--attempts)
	{
		if (place_monster_aux(player_ptr, 0, y, x, r_idx, 0L)) break;
	}

	if (attempts < 1) return FALSE;

	MONSTER_IDX m_idx = floor_ptr->grid_array[y][x].m_idx;
	if (floor_ptr->m_list[m_idx].mflag2 & MFLAG2_CHAMELEON) r_ptr = &r_info[floor_ptr->m_list[m_idx].r_idx];

	POSITION cy = y;
	POSITION cx = x;
	for (attempts = randint1(10) + 5; attempts; attempts--)
	{
		scatter(player_ptr, &cy, &cx, y, x, 5, 0);
		(void)summon_specific(player_ptr, m_idx, cy, cx, floor_ptr->dun_level + 5, SUMMON_KIN, PM_ALLOW_GROUP);
		y = cy;
		x = cx;
	}

	if (cheat_hear) msg_format(_("モンスターの大群(%c)", "Monster horde (%c)."), r_ptr->d_char);
	return TRUE;
}


/*!
 * @brief ダンジョンの主生成を試みる / Put the Guardian
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param def_val 現在の主の生成状態
 * @return 生成に成功したらtrue
 */
bool alloc_guardian(player_type *player_ptr, bool def_val)
{
	MONRACE_IDX guardian = d_info[player_ptr->dungeon_idx].final_guardian;
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	bool is_guardian_applicable = guardian > 0;
	is_guardian_applicable &= d_info[player_ptr->dungeon_idx].maxdepth == floor_ptr->dun_level;
	is_guardian_applicable &= r_info[guardian].cur_num < r_info[guardian].max_num;
	if (!is_guardian_applicable) return def_val;

	int try_count = 4000;
	while (try_count)
	{
		POSITION oy = randint1(floor_ptr->height - 4) + 2;
		POSITION ox = randint1(floor_ptr->width - 4) + 2;
		if (!is_cave_empty_bold2(player_ptr, oy, ox))
		{
			try_count++;
			continue;
		}

		if (!monster_can_cross_terrain(player_ptr, floor_ptr->grid_array[oy][ox].feat, &r_info[guardian], 0))
		{
			try_count++;
			continue;
		}

		if (place_monster_aux(player_ptr, 0, oy, ox, guardian, (PM_ALLOW_GROUP | PM_NO_KAGE | PM_NO_PET)))
			return TRUE;

		try_count--;
	}

	return FALSE;
}


/*!
 * @brief ダンジョンの初期配置モンスターを生成1回生成する / Attempt to allocate a random monster in the dungeon.
 * @param dis プレイヤーから離れるべき最低距離
 * @param mode 生成オプション
 * @return 生成に成功したらtrue
 * @details
 * Place the monster at least "dis" distance from the player.
 * Use "slp" to choose the initial "sleep" status
 * Use "floor_ptr->monster_level" for the monster level
 */
bool alloc_monster(player_type *player_ptr, POSITION dis, BIT_FLAGS mode)
{
	if (alloc_guardian(player_ptr, FALSE)) return TRUE;

	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	POSITION y = 0, x = 0;
	int attempts_left = 10000;
	while (attempts_left--)
	{
		y = randint0(floor_ptr->height);
		x = randint0(floor_ptr->width);

		if (floor_ptr->dun_level)
		{
			if (!is_cave_empty_bold2(player_ptr, y, x)) continue;
		}
		else
		{
			if (!is_cave_empty_bold(player_ptr, y, x)) continue;
		}

		if (distance(y, x, player_ptr->y, player_ptr->x) > dis) break;
	}

	if (!attempts_left)
	{
		if (cheat_xtra || cheat_hear)
		{
			msg_print(_("警告！新たなモンスターを配置できません。小さい階ですか？", "Warning! Could not allocate a new monster. Small level?"));
		}

		return FALSE;
	}


	if (randint1(5000) <= floor_ptr->dun_level)
	{
		if (alloc_horde(player_ptr, y, x))
		{
			return TRUE;
		}
	}
	else
	{
		if (place_monster(player_ptr, y, x, (mode | PM_ALLOW_GROUP))) return TRUE;
	}

	return FALSE;
}


/*!
 * todo ここにplayer_typeを追加すると関数ポインタ周りの収拾がつかなくなるので保留
 * @brief モンスターが召喚の基本条件に合っているかをチェックする / Hack -- help decide if a monster race is "okay" to summon
 * @param r_idx チェックするモンスター種族ID
 * @return 召喚対象にできるならばTRUE
 */
static bool summon_specific_okay(MONRACE_IDX r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];
	monster_type *m_ptr = &p_ptr->current_floor_ptr->m_list[summon_specific_who];
	if (!mon_hook_dungeon(r_idx)) return FALSE;

	if (summon_specific_who > 0)
	{
		if (monster_has_hostile_align(p_ptr, m_ptr, 0, 0, r_ptr)) return FALSE;
	}
	else if (summon_specific_who < 0)
	{
		if (monster_has_hostile_align(p_ptr, NULL, 10, -10, r_ptr))
		{
			if (!one_in_(ABS(p_ptr->align) / 2 + 1)) return FALSE;
		}
	}

	if (!summon_unique_okay && ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags7 & RF7_NAZGUL))) return FALSE;

	if (!summon_specific_type) return TRUE;

	if ((summon_specific_who < 0) &&
		((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags7 & RF7_NAZGUL)) &&
		monster_has_hostile_align(p_ptr, NULL, 10, -10, r_ptr))
		return FALSE;

	if ((r_ptr->flags7 & RF7_CHAMELEON) && (d_info[p_ptr->dungeon_idx].flags1 & DF1_CHAMELEON)) return TRUE;

	return (summon_specific_aux(p_ptr, m_ptr->r_idx, r_idx));
}


/*!
 * @brief モンスターを召喚により配置する / Place a monster (of the specified "type") near the given location. Return TRUE if a monster was actually summoned.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param who 召喚主のモンスター情報ID
 * @param y1 目標地点y座標
 * @param x1 目標地点x座標
 * @param lev 相当生成階
 * @param type 召喚種別
 * @param mode 生成オプション
 * @return 召喚できたらtrueを返す
 * @details
 *
 * We will attempt to place the monster up to 10 times before giving up.
 *
 * Note: SUMMON_UNIQUE and SUMMON_AMBERITES will summon Unique's
 * Note: SUMMON_HI_UNDEAD and SUMMON_HI_DRAGON may summon Unique's
 * Note: None of the other summon codes will ever summon Unique's.
 *
 * This function has been changed.  We now take the "monster level"
 * of the summoning monster as a parameter, and use that, along with
 * the current dungeon level, to help determine the level of the
 * desired monster.  Note that this is an upper bound, and also
 * tends to "prefer" monsters of that level.  Currently, we use
 * the average of the dungeon and monster levels, and then add
 * five to allow slight increases in monster power.
 *
 * Note that we use the new "monster allocation table" creation code
 * to restrict the "get_mon_num()" function to the set of "legal"
 * monsters, making this function much faster and more reliable.
 *
 * Note that this function may not succeed, though this is very rare.
 */
bool summon_specific(player_type *player_ptr, MONSTER_IDX who, POSITION y1, POSITION x1, DEPTH lev, int type, BIT_FLAGS mode)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	if (floor_ptr->inside_arena) return FALSE;

	POSITION x, y;
	if (!mon_scatter(player_ptr, 0, &y, &x, y1, x1, 2)) return FALSE;

	summon_specific_who = who;
	summon_specific_type = type;
	summon_unique_okay = (mode & PM_ALLOW_UNIQUE) ? TRUE : FALSE;
	get_mon_num_prep(player_ptr, summon_specific_okay, get_monster_hook2(player_ptr, y, x));

	MONRACE_IDX r_idx = get_mon_num(player_ptr, (floor_ptr->dun_level + lev) / 2 + 5, 0);
	if (!r_idx)
	{
		summon_specific_type = 0;
		return FALSE;
	}

	if ((type == SUMMON_BLUE_HORROR) || (type == SUMMON_DAWN)) mode |= PM_NO_KAGE;

	if (!place_monster_aux(player_ptr, who, y, x, r_idx, mode))
	{
		summon_specific_type = 0;
		return FALSE;
	}

	summon_specific_type = 0;
	sound(SOUND_SUMMON);
	return TRUE;
}


/*!
 * @brief 特定モンスター種族を召喚により生成する / A "dangerous" function, creates a pet of the specified type
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param who 召喚主のモンスター情報ID
 * @param oy 目標地点y座標
 * @param ox 目標地点x座標
 * @param r_idx 生成するモンスター種族ID
 * @param mode 生成オプション
 * @return 召喚できたらtrueを返す
 */
bool summon_named_creature(player_type *player_ptr, MONSTER_IDX who, POSITION oy, POSITION ox, MONRACE_IDX r_idx, BIT_FLAGS mode)
{
	if (r_idx >= max_r_idx) return FALSE;

	POSITION x, y;
	if (player_ptr->current_floor_ptr->inside_arena) return FALSE;

	if (!mon_scatter(player_ptr, r_idx, &y, &x, oy, ox, 2)) return FALSE;

	return place_monster_aux(player_ptr, who, y, x, r_idx, (mode | PM_NO_KAGE));
}


/*!
 * @brief モンスターを増殖生成する / Let the given monster attempt to reproduce.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param m_idx 増殖するモンスター情報ID
 * @param clone クローン・モンスター処理ならばtrue
 * @param mode 生成オプション
 * @return 生成できたらtrueを返す
 * @details
 * Note that "reproduction" REQUIRES empty space.
 */
bool multiply_monster(player_type *player_ptr, MONSTER_IDX m_idx, bool clone, BIT_FLAGS mode)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	monster_type *m_ptr = &floor_ptr->m_list[m_idx];
	POSITION y, x;
	if (!mon_scatter(player_ptr, m_ptr->r_idx, &y, &x, m_ptr->fy, m_ptr->fx, 1))
		return FALSE;

	if (m_ptr->mflag2 & MFLAG2_NOPET) mode |= PM_NO_PET;

	if (!place_monster_aux(player_ptr, m_idx, y, x, m_ptr->r_idx, (mode | PM_NO_KAGE | PM_MULTIPLY)))
		return FALSE;

	if (clone || (m_ptr->smart & SM_CLONED))
	{
		floor_ptr->m_list[hack_m_idx_ii].smart |= SM_CLONED;
		floor_ptr->m_list[hack_m_idx_ii].mflag2 |= MFLAG2_NOPET;
	}

	return TRUE;
}


/*!
 * @brief ダメージを受けたモンスターの様子を記述する / Dump a message describing a monster's reaction to damage
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param m_idx モンスター情報ID
 * @param dam 与えたダメージ
 * @return なし
 * @details
 * Technically should attempt to treat "Beholder"'s as jelly's
 */
void message_pain(player_type *player_ptr, MONSTER_IDX m_idx, HIT_POINT dam)
{
	monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	GAME_TEXT m_name[MAX_NLEN];

	monster_desc(player_ptr, m_name, m_ptr, 0);

	if (dam == 0)
	{
		msg_format(_("%^sはダメージを受けていない。", "%^s is unharmed."), m_name);
		return;
	}

	HIT_POINT newhp = m_ptr->hp;
	HIT_POINT oldhp = newhp + dam;
	HIT_POINT tmp = (newhp * 100L) / oldhp;
	PERCENTAGE percentage = tmp;

	if (my_strchr(",ejmvwQ", r_ptr->d_char))
	{
#ifdef JP
		if (percentage > 95) msg_format("%^sはほとんど気にとめていない。", m_name);
		else if (percentage > 75) msg_format("%^sはしり込みした。", m_name);
		else if (percentage > 50) msg_format("%^sは縮こまった。", m_name);
		else if (percentage > 35) msg_format("%^sは痛みに震えた。", m_name);
		else if (percentage > 20) msg_format("%^sは身もだえした。", m_name);
		else if (percentage > 10) msg_format("%^sは苦痛で身もだえした。", m_name);
		else msg_format("%^sはぐにゃぐにゃと痙攣した。", m_name);
		return;
#else
		if (percentage > 95) msg_format("%^s barely notices.", m_name);
		else if (percentage > 75) msg_format("%^s flinches.", m_name);
		else if (percentage > 50) msg_format("%^s squelches.", m_name);
		else if (percentage > 35) msg_format("%^s quivers in pain.", m_name);
		else if (percentage > 20) msg_format("%^s writhes about.", m_name);
		else if (percentage > 10) msg_format("%^s writhes in agony.", m_name);
		else msg_format("%^s jerks limply.", m_name);
		return;
#endif
	}

	if (my_strchr("l", r_ptr->d_char))
	{
#ifdef JP
		if (percentage > 95) msg_format("%^sはほとんど気にとめていない。", m_name);
		else if (percentage > 75) msg_format("%^sはしり込みした。", m_name);
		else if (percentage > 50) msg_format("%^sは躊躇した。", m_name);
		else if (percentage > 35) msg_format("%^sは痛みに震えた。", m_name);
		else if (percentage > 20) msg_format("%^sは身もだえした。", m_name);
		else if (percentage > 10) msg_format("%^sは苦痛で身もだえした。", m_name);
		else msg_format("%^sはぐにゃぐにゃと痙攣した。", m_name);
		return;
#else
		if (percentage > 95) msg_format("%^s barely notices.", m_name);
		else if (percentage > 75) msg_format("%^s flinches.", m_name);
		else if (percentage > 50) msg_format("%^s hesitates.", m_name);
		else if (percentage > 35) msg_format("%^s quivers in pain.", m_name);
		else if (percentage > 20) msg_format("%^s writhes about.", m_name);
		else if (percentage > 10) msg_format("%^s writhes in agony.", m_name);
		else msg_format("%^s jerks limply.", m_name);
		return;
#endif		
	}

	if (my_strchr("g#+<>", r_ptr->d_char))
	{
#ifdef JP
		if (percentage > 95) msg_format("%sは攻撃を気にとめていない。", m_name);
		else if (percentage > 75) msg_format("%sは攻撃に肩をすくめた。", m_name);
		else if (percentage > 50) msg_format("%^sは雷鳴のように吠えた。", m_name);
		else if (percentage > 35) msg_format("%^sは苦しげに吠えた。", m_name);
		else if (percentage > 20) msg_format("%^sはうめいた。", m_name);
		else if (percentage > 10) msg_format("%^sは躊躇した。", m_name);
		else msg_format("%^sはくしゃくしゃになった。", m_name);
		return;
#else
		if (percentage > 95) msg_format("%^s ignores the attack.", m_name);
		else if (percentage > 75) msg_format("%^s shrugs off the attack.", m_name);
		else if (percentage > 50) msg_format("%^s roars thunderously.", m_name);
		else if (percentage > 35) msg_format("%^s rumbles.", m_name);
		else if (percentage > 20) msg_format("%^s grunts.", m_name);
		else if (percentage > 10) msg_format("%^s hesitates.", m_name);
		else msg_format("%^s crumples.", m_name);
		return;
#endif
	}

	if (my_strchr("JMR", r_ptr->d_char) || !isalpha(r_ptr->d_char))
	{
#ifdef JP
		if (percentage > 95) msg_format("%^sはほとんど気にとめていない。", m_name);
		else if (percentage > 75) msg_format("%^sはシーッと鳴いた。", m_name);
		else if (percentage > 50) msg_format("%^sは怒って頭を上げた。", m_name);
		else if (percentage > 35) msg_format("%^sは猛然と威嚇した。", m_name);
		else if (percentage > 20) msg_format("%^sは身もだえした。", m_name);
		else if (percentage > 10) msg_format("%^sは苦痛で身もだえした。", m_name);
		else msg_format("%^sはぐにゃぐにゃと痙攣した。", m_name);
		return;
#else
		if (percentage > 95) msg_format("%^s barely notices.", m_name);
		else if (percentage > 75) msg_format("%^s hisses.", m_name);
		else if (percentage > 50) msg_format("%^s rears up in anger.", m_name);
		else if (percentage > 35) msg_format("%^s hisses furiously.", m_name);
		else if (percentage > 20) msg_format("%^s writhes about.", m_name);
		else if (percentage > 10) msg_format("%^s writhes in agony.", m_name);
		else msg_format("%^s jerks limply.", m_name);
		return;
#endif
	}

	if (my_strchr("f", r_ptr->d_char))
	{
#ifdef JP
		if (percentage > 95) msg_format("%sは攻撃に肩をすくめた。", m_name);
		else if (percentage > 75) msg_format("%^sは吠えた。", m_name);
		else if (percentage > 50) msg_format("%^sは怒って吠えた。", m_name);
		else if (percentage > 35) msg_format("%^sは痛みでシーッと鳴いた。", m_name);
		else if (percentage > 20) msg_format("%^sは痛みで弱々しく鳴いた。", m_name);
		else if (percentage > 10) msg_format("%^sは苦痛にうめいた。", m_name);
		else msg_format("%sは哀れな鳴き声を出した。", m_name);
		return;
#else
		if (percentage > 95) msg_format("%^s shrugs off the attack.", m_name);
		else if (percentage > 75) msg_format("%^s roars.", m_name);
		else if (percentage > 50) msg_format("%^s growls angrily.", m_name);
		else if (percentage > 35) msg_format("%^s hisses with pain.", m_name);
		else if (percentage > 20) msg_format("%^s mewls in pain.", m_name);
		else if (percentage > 10) msg_format("%^s hisses in agony.", m_name);
		else msg_format("%^s mewls pitifully.", m_name);
		return;
#endif
	}

	if (my_strchr("acFIKS", r_ptr->d_char))
	{
#ifdef JP
		if (percentage > 95) msg_format("%sは攻撃を気にとめていない。", m_name);
		else if (percentage > 75) msg_format("%^sはキーキー鳴いた。", m_name);
		else if (percentage > 50) msg_format("%^sはヨロヨロ逃げ回った。", m_name);
		else if (percentage > 35) msg_format("%^sはうるさく鳴いた。", m_name);
		else if (percentage > 20) msg_format("%^sは痛みに痙攣した。", m_name);
		else if (percentage > 10) msg_format("%^sは苦痛で痙攣した。", m_name);
		else msg_format("%^sはピクピクひきつった。", m_name);
		return;
#else
		if (percentage > 95)	msg_format("%^s ignores the attack.", m_name);
		else if (percentage > 75) msg_format("%^s chitters.", m_name);
		else if (percentage > 50) msg_format("%^s scuttles about.", m_name);
		else if (percentage > 35) msg_format("%^s twitters.", m_name);
		else if (percentage > 20) msg_format("%^s jerks in pain.", m_name);
		else if (percentage > 10) msg_format("%^s jerks in agony.", m_name);
		else msg_format("%^s twitches.", m_name);
		return;
#endif
	}

	if (my_strchr("B", r_ptr->d_char))
	{
#ifdef JP
		if (percentage > 95) msg_format("%^sはさえずった。", m_name);
		else if (percentage > 75) msg_format("%^sはピーピー鳴いた。", m_name);
		else if (percentage > 50) msg_format("%^sはギャーギャー鳴いた。", m_name);
		else if (percentage > 35) msg_format("%^sはギャーギャー鳴きわめいた。", m_name);
		else if (percentage > 20) msg_format("%^sは苦しんだ。", m_name);
		else if (percentage > 10) msg_format("%^sはのたうち回った。", m_name);
		else msg_format("%^sはキーキーと鳴き叫んだ。", m_name);
		return;
#else
		if (percentage > 95)	msg_format("%^s chirps.", m_name);
		else if (percentage > 75) msg_format("%^s twitters.", m_name);
		else if (percentage > 50) msg_format("%^s squawks.", m_name);
		else if (percentage > 35) msg_format("%^s chatters.", m_name);
		else if (percentage > 20) msg_format("%^s jeers.", m_name);
		else if (percentage > 10) msg_format("%^s flutters about.", m_name);
		else msg_format("%^s squeaks.", m_name);
		return;
#endif
	}

	if (my_strchr("duDLUW", r_ptr->d_char))
	{
#ifdef JP
		if (percentage > 95) msg_format("%sは攻撃を気にとめていない。", m_name);
		else if (percentage > 75) msg_format("%^sはしり込みした。", m_name);
		else if (percentage > 50) msg_format("%^sは痛みでシーッと鳴いた。", m_name);
		else if (percentage > 35) msg_format("%^sは痛みでうなった。", m_name);
		else if (percentage > 20) msg_format("%^sは痛みに吠えた。", m_name);
		else if (percentage > 10) msg_format("%^sは苦しげに叫んだ。", m_name);
		else msg_format("%^sは弱々しくうなった。", m_name);
		return;
#else
		if (percentage > 95) msg_format("%^s ignores the attack.", m_name);
		else if (percentage > 75) msg_format("%^s flinches.", m_name);
		else if (percentage > 50) msg_format("%^s hisses in pain.", m_name);
		else if (percentage > 35) msg_format("%^s snarls with pain.", m_name);
		else if (percentage > 20) msg_format("%^s roars with pain.", m_name);
		else if (percentage > 10) msg_format("%^s gasps.", m_name);
		else msg_format("%^s snarls feebly.", m_name);
		return;
#endif
	}

	if (my_strchr("s", r_ptr->d_char))
	{
#ifdef JP
		if (percentage > 95) msg_format("%sは攻撃を気にとめていない。", m_name);
		else if (percentage > 75) msg_format("%sは攻撃に肩をすくめた。", m_name);
		else if (percentage > 50) msg_format("%^sはカタカタと笑った。", m_name);
		else if (percentage > 35) msg_format("%^sはよろめいた。", m_name);
		else if (percentage > 20) msg_format("%^sはカタカタ言った。", m_name);
		else if (percentage > 10) msg_format("%^sはよろめいた。", m_name);
		else msg_format("%^sはガタガタ言った。", m_name);
		return;
#else
		if (percentage > 95) msg_format("%^s ignores the attack.", m_name);
		else if (percentage > 75) msg_format("%^s shrugs off the attack.", m_name);
		else if (percentage > 50) msg_format("%^s rattles.", m_name);
		else if (percentage > 35) msg_format("%^s stumbles.", m_name);
		else if (percentage > 20) msg_format("%^s rattles.", m_name);
		else if (percentage > 10) msg_format("%^s staggers.", m_name);
		else msg_format("%^s clatters.", m_name);
		return;
#endif
	}

	if (my_strchr("z", r_ptr->d_char))
	{
#ifdef JP
		if (percentage > 95) msg_format("%sは攻撃を気にとめていない。", m_name);
		else if (percentage > 75) msg_format("%sは攻撃に肩をすくめた。", m_name);
		else if (percentage > 50) msg_format("%^sはうめいた。", m_name);
		else if (percentage > 35) msg_format("%sは苦しげにうめいた。", m_name);
		else if (percentage > 20) msg_format("%^sは躊躇した。", m_name);
		else if (percentage > 10) msg_format("%^sはうなった。", m_name);
		else msg_format("%^sはよろめいた。", m_name);
		return;
#else
		if (percentage > 95) msg_format("%^s ignores the attack.", m_name);
		else if (percentage > 75) msg_format("%^s shrugs off the attack.", m_name);
		else if (percentage > 50) msg_format("%^s groans.", m_name);
		else if (percentage > 35) msg_format("%^s moans.", m_name);
		else if (percentage > 20) msg_format("%^s hesitates.", m_name);
		else if (percentage > 10) msg_format("%^s grunts.", m_name);
		else msg_format("%^s staggers.", m_name);
		return;
#endif
	}

	if (my_strchr("G", r_ptr->d_char))
	{
#ifdef JP
		if (percentage > 95) msg_format("%sは攻撃を気にとめていない。", m_name);
		else if (percentage > 75) msg_format("%sは攻撃に肩をすくめた。", m_name);
		else if (percentage > 50) msg_format("%sはうめいた。", m_name);
		else if (percentage > 35) msg_format("%^sは泣きわめいた。", m_name);
		else if (percentage > 20) msg_format("%^sは吠えた。", m_name);
		else if (percentage > 10) msg_format("%sは弱々しくうめいた。", m_name);
		else msg_format("%^sはかすかにうめいた。", m_name);
		return;
#else
		if (percentage > 95) msg_format("%^s ignores the attack.", m_name);
		else if (percentage > 75) msg_format("%^s shrugs off the attack.", m_name);
		else if (percentage > 50)  msg_format("%^s moans.", m_name);
		else if (percentage > 35) msg_format("%^s wails.", m_name);
		else if (percentage > 20) msg_format("%^s howls.", m_name);
		else if (percentage > 10) msg_format("%^s moans softly.", m_name);
		else msg_format("%^s sighs.", m_name);
		return;
#endif
	}

	if (my_strchr("CZ", r_ptr->d_char))
	{
#ifdef JP
		if (percentage > 95) msg_format("%^sは攻撃に肩をすくめた。", m_name);
		else if (percentage > 75) msg_format("%^sは痛みでうなった。", m_name);
		else if (percentage > 50) msg_format("%^sは痛みでキャンキャン吠えた。", m_name);
		else if (percentage > 35) msg_format("%^sは痛みで鳴きわめいた。", m_name);
		else if (percentage > 20) msg_format("%^sは苦痛のあまり鳴きわめいた。", m_name);
		else if (percentage > 10) msg_format("%^sは苦痛でもだえ苦しんだ。", m_name);
		else msg_format("%^sは弱々しく吠えた。", m_name);
		return;
#else
		if (percentage > 95) msg_format("%^s shrugs off the attack.", m_name);
		else if (percentage > 75) msg_format("%^s snarls with pain.", m_name);
		else if (percentage > 50) msg_format("%^s yelps in pain.", m_name);
		else if (percentage > 35) msg_format("%^s howls in pain.", m_name);
		else if (percentage > 20) msg_format("%^s howls in agony.", m_name);
		else if (percentage > 10) msg_format("%^s writhes in agony.", m_name);
		else msg_format("%^s yelps feebly.", m_name);
		return;
#endif
	}

	if (my_strchr("Xbilqrt", r_ptr->d_char))
	{
#ifdef JP
		if (percentage > 95) msg_format("%^sは攻撃を気にとめていない。", m_name);
		else if (percentage > 75) msg_format("%^sは痛みでうなった。", m_name);
		else if (percentage > 50) msg_format("%^sは痛みで叫んだ。", m_name);
		else if (percentage > 35) msg_format("%^sは痛みで絶叫した。", m_name);
		else if (percentage > 20) msg_format("%^sは苦痛のあまり絶叫した。", m_name);
		else if (percentage > 10) msg_format("%^sは苦痛でもだえ苦しんだ。", m_name);
		else msg_format("%^sは弱々しく叫んだ。", m_name);
		return;
#else
		if (percentage > 95) msg_format("%^s ignores the attack.", m_name);
		else if (percentage > 75) msg_format("%^s grunts with pain.", m_name);
		else if (percentage > 50) msg_format("%^s squeals in pain.", m_name);
		else if (percentage > 35) msg_format("%^s shrieks in pain.", m_name);
		else if (percentage > 20) msg_format("%^s shrieks in agony.", m_name);
		else if (percentage > 10) msg_format("%^s writhes in agony.", m_name);
		else msg_format("%^s cries out feebly.", m_name);
		return;
#endif
	}

#ifdef JP
	if (percentage > 95) msg_format("%^sは攻撃に肩をすくめた。", m_name);
	else if (percentage > 75) msg_format("%^sは痛みでうなった。", m_name);
	else if (percentage > 50) msg_format("%^sは痛みで叫んだ。", m_name);
	else if (percentage > 35) msg_format("%^sは痛みで絶叫した。", m_name);
	else if (percentage > 20) msg_format("%^sは苦痛のあまり絶叫した。", m_name);
	else if (percentage > 10) msg_format("%^sは苦痛でもだえ苦しんだ。", m_name);
	else msg_format("%^sは弱々しく叫んだ。", m_name);
#else
	if (percentage > 95) msg_format("%^s shrugs off the attack.", m_name);
	else if (percentage > 75) msg_format("%^s grunts with pain.", m_name);
	else if (percentage > 50) msg_format("%^s cries out in pain.", m_name);
	else if (percentage > 35) msg_format("%^s screams in pain.", m_name);
	else if (percentage > 20) msg_format("%^s screams in agony.", m_name);
	else if (percentage > 10) msg_format("%^s writhes in agony.", m_name);
	else msg_format("%^s cries out feebly.", m_name);
#endif
}


/*!
 * @brief SMART(適格に攻撃を行う)モンスターの学習状況を更新する / Learn about an "observed" resistance.
 * @param m_idx 更新を行う「モンスター情報ID
 * @param what 学習対象ID
 * @return なし
 */
void update_smart_learn(player_type *player_ptr, MONSTER_IDX m_idx, int what)
{
	monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	if (!smart_learn) return;
	if (r_ptr->flags2 & (RF2_STUPID)) return;
	if (!(r_ptr->flags2 & (RF2_SMART)) && (randint0(100) < 50)) return;

	switch (what)
	{
	case DRS_ACID:
		if (player_ptr->resist_acid) m_ptr->smart |= (SM_RES_ACID);
		if (is_oppose_acid(player_ptr)) m_ptr->smart |= (SM_OPP_ACID);
		if (player_ptr->immune_acid) m_ptr->smart |= (SM_IMM_ACID);
		break;

	case DRS_ELEC:
		if (player_ptr->resist_elec) m_ptr->smart |= (SM_RES_ELEC);
		if (is_oppose_elec(player_ptr)) m_ptr->smart |= (SM_OPP_ELEC);
		if (player_ptr->immune_elec) m_ptr->smart |= (SM_IMM_ELEC);
		break;

	case DRS_FIRE:
		if (player_ptr->resist_fire) m_ptr->smart |= (SM_RES_FIRE);
		if (is_oppose_fire(player_ptr)) m_ptr->smart |= (SM_OPP_FIRE);
		if (player_ptr->immune_fire) m_ptr->smart |= (SM_IMM_FIRE);
		break;

	case DRS_COLD:
		if (player_ptr->resist_cold) m_ptr->smart |= (SM_RES_COLD);
		if (is_oppose_cold(player_ptr)) m_ptr->smart |= (SM_OPP_COLD);
		if (player_ptr->immune_cold) m_ptr->smart |= (SM_IMM_COLD);
		break;

	case DRS_POIS:
		if (player_ptr->resist_pois) m_ptr->smart |= (SM_RES_POIS);
		if (is_oppose_pois(player_ptr)) m_ptr->smart |= (SM_OPP_POIS);
		break;


	case DRS_NETH:
		if (player_ptr->resist_neth) m_ptr->smart |= (SM_RES_NETH);
		break;

	case DRS_LITE:
		if (player_ptr->resist_lite) m_ptr->smart |= (SM_RES_LITE);
		break;

	case DRS_DARK:
		if (player_ptr->resist_dark) m_ptr->smart |= (SM_RES_DARK);
		break;

	case DRS_FEAR:
		if (player_ptr->resist_fear) m_ptr->smart |= (SM_RES_FEAR);
		break;

	case DRS_CONF:
		if (player_ptr->resist_conf) m_ptr->smart |= (SM_RES_CONF);
		break;

	case DRS_CHAOS:
		if (player_ptr->resist_chaos) m_ptr->smart |= (SM_RES_CHAOS);
		break;

	case DRS_DISEN:
		if (player_ptr->resist_disen) m_ptr->smart |= (SM_RES_DISEN);
		break;

	case DRS_BLIND:
		if (player_ptr->resist_blind) m_ptr->smart |= (SM_RES_BLIND);
		break;

	case DRS_NEXUS:
		if (player_ptr->resist_nexus) m_ptr->smart |= (SM_RES_NEXUS);
		break;

	case DRS_SOUND:
		if (player_ptr->resist_sound) m_ptr->smart |= (SM_RES_SOUND);
		break;

	case DRS_SHARD:
		if (player_ptr->resist_shard) m_ptr->smart |= (SM_RES_SHARD);
		break;

	case DRS_FREE:
		if (player_ptr->free_act) m_ptr->smart |= (SM_IMM_FREE);
		break;

	case DRS_MANA:
		if (!player_ptr->msp) m_ptr->smart |= (SM_IMM_MANA);
		break;

	case DRS_REFLECT:
		if (player_ptr->reflect) m_ptr->smart |= (SM_IMM_REFLECT);
		break;
	}
}


/*!
 * @brief モンスターが盗みや拾いで確保していたアイテムを全てドロップさせる / Drop all items carried by a monster
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param m_ptr モンスター参照ポインタ
 * @return なし
 */
void monster_drop_carried_objects(player_type *player_ptr, monster_type *m_ptr)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	OBJECT_IDX next_o_idx = 0;
	for (OBJECT_IDX this_o_idx = m_ptr->hold_o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type forge;
		object_type *o_ptr;
		object_type *q_ptr;
		o_ptr = &floor_ptr->o_list[this_o_idx];
		next_o_idx = o_ptr->next_o_idx;
		q_ptr = &forge;

		object_copy(q_ptr, o_ptr);
		q_ptr->held_m_idx = 0;
		delete_object_idx(player_ptr, this_o_idx);
		(void)drop_near(player_ptr, q_ptr, -1, m_ptr->fy, m_ptr->fx);
	}

	m_ptr->hold_o_idx = 0;
}


/*!
 * todo ここには本来floor_type*を追加したいが、monster.hにfloor.hの参照を追加するとコンパイルエラーが出るので保留
 * @brief 指定したモンスターに隣接しているモンスターの数を返す。
 * / Count number of adjacent monsters
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param m_idx 隣接数を調べたいモンスターのID
 * @return 隣接しているモンスターの数
 */
int get_monster_crowd_number(player_type *player_ptr, MONSTER_IDX m_idx)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	monster_type *m_ptr = &floor_ptr->m_list[m_idx];
	POSITION my = m_ptr->fy;
	POSITION mx = m_ptr->fx;
	int count = 0;
	for (int i = 0; i < 7; i++)
	{
		int ay = my + ddy_ddd[i];
		int ax = mx + ddx_ddd[i];

		if (!in_bounds(floor_ptr, ay, ax)) continue;
		if (floor_ptr->grid_array[ay][ax].m_idx > 0) count++;
	}

	return count;
}


bool is_friendly_idx(player_type *player_ptr, MONSTER_IDX m_idx)
{
	return m_idx > 0 && is_friendly(&player_ptr->current_floor_ptr->m_list[(m_idx)]);
}
