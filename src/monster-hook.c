#include "angband.h"
#include "monster-hook.h"


/*!
* @brief モンスターがクエストの討伐対象に成り得るかを返す / Hook function for quest monsters
* @param r_idx モンスターＩＤ
* @return 討伐対象にできるならTRUEを返す。
*/
bool mon_hook_quest(MONRACE_IDX r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* Random quests are in the dungeon */
	if (r_ptr->flags8 & RF8_WILD_ONLY) return FALSE;

	/* No random quests for aquatic monsters */
	if (r_ptr->flags7 & RF7_AQUATIC) return FALSE;

	/* No random quests for multiplying monsters */
	if (r_ptr->flags2 & RF2_MULTIPLY) return FALSE;

	/* No quests to kill friendly monsters */
	if (r_ptr->flags7 & RF7_FRIENDLY) return FALSE;

	return TRUE;
}


/*!
* @brief モンスターがダンジョンに出現するかどうかを返す
* @param r_idx 判定するモンスターの種族ID
* @return ダンジョンに出現するならばTRUEを返す
*/
bool mon_hook_dungeon(MONRACE_IDX r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (!(r_ptr->flags8 & RF8_WILD_ONLY))
		return TRUE;
	else
	{
		dungeon_info_type *d_ptr = &d_info[dungeon_type];
		if ((d_ptr->mflags8 & RF8_WILD_MOUNTAIN) &&
			(r_ptr->flags8 & RF8_WILD_MOUNTAIN)) return TRUE;
		return FALSE;
	}
}


/*!
* @brief モンスターが海洋に出現するかどうかを返す
* @param r_idx 判定するモンスターの種族ID
* @return 海洋に出現するならばTRUEを返す
*/
bool mon_hook_ocean(MONRACE_IDX r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & RF8_WILD_OCEAN)
		return TRUE;
	else
		return FALSE;
}


/*!
* @brief モンスターが海岸に出現するかどうかを返す
* @param r_idx 判定するモンスターの種族ID
* @return 海岸に出現するならばTRUEを返す
*/
bool mon_hook_shore(MONRACE_IDX r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & RF8_WILD_SHORE)
		return TRUE;
	else
		return FALSE;
}


/*!
* @brief モンスターが荒地に出現するかどうかを返す
* @param r_idx 判定するモンスターの種族ID
* @return 荒地に出現するならばTRUEを返す
*/
bool mon_hook_waste(MONRACE_IDX r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & (RF8_WILD_WASTE | RF8_WILD_ALL))
		return TRUE;
	else
		return FALSE;
}


/*!
* @brief モンスターが町に出現するかどうかを返す
* @param r_idx 判定するモンスターの種族ID
* @return 荒地に出現するならばTRUEを返す
*/
bool mon_hook_town(MONRACE_IDX r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & (RF8_WILD_TOWN | RF8_WILD_ALL))
		return TRUE;
	else
		return FALSE;
}


/*!
* @brief モンスターが森林に出現するかどうかを返す
* @param r_idx 判定するモンスターの種族ID
* @return 森林に出現するならばTRUEを返す
*/
bool mon_hook_wood(MONRACE_IDX r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & (RF8_WILD_WOOD | RF8_WILD_ALL))
		return TRUE;
	else
		return FALSE;
}


/*!
* @brief モンスターが火山に出現するかどうかを返す
* @param r_idx 判定するモンスターの種族ID
* @return 火山に出現するならばTRUEを返す
*/
bool mon_hook_volcano(MONRACE_IDX r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & RF8_WILD_VOLCANO)
		return TRUE;
	else
		return FALSE;
}

/*!
* @brief モンスターが山地に出現するかどうかを返す
* @param r_idx 判定するモンスターの種族ID
* @return 山地に出現するならばTRUEを返す
*/
bool mon_hook_mountain(MONRACE_IDX r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & RF8_WILD_MOUNTAIN)
		return TRUE;
	else
		return FALSE;
}


/*!
* @brief モンスターが草原に出現するかどうかを返す
* @param r_idx 判定するモンスターの種族ID
* @return 森林に出現するならばTRUEを返す
*/
bool mon_hook_grass(MONRACE_IDX r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & (RF8_WILD_GRASS | RF8_WILD_ALL))
		return TRUE;
	else
		return FALSE;
}

/*!
* @brief モンスターが深い水地形に出現するかどうかを返す
* @param r_idx 判定するモンスターの種族ID
* @return 深い水地形に出現するならばTRUEを返す
*/
bool mon_hook_deep_water(MONRACE_IDX r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (!mon_hook_dungeon(r_idx)) return FALSE;

	if (r_ptr->flags7 & RF7_AQUATIC)
		return TRUE;
	else
		return FALSE;
}


/*!
* @brief モンスターが浅い水地形に出現するかどうかを返す
* @param r_idx 判定するモンスターの種族ID
* @return 浅い水地形に出現するならばTRUEを返す
*/
bool mon_hook_shallow_water(MONRACE_IDX r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (!mon_hook_dungeon(r_idx)) return FALSE;

	if (r_ptr->flags2 & RF2_AURA_FIRE)
		return FALSE;
	else
		return TRUE;
}


/*!
* @brief モンスターが溶岩地形に出現するかどうかを返す
* @param r_idx 判定するモンスターの種族ID
* @return 溶岩地形に出現するならばTRUEを返す
*/
bool mon_hook_lava(MONRACE_IDX r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (!mon_hook_dungeon(r_idx)) return FALSE;

	if (((r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK) ||
		(r_ptr->flags7 & RF7_CAN_FLY)) &&
		!(r_ptr->flags3 & RF3_AURA_COLD))
		return TRUE;
	else
		return FALSE;
}


/*!
* @brief モンスターが通常の床地形に出現するかどうかを返す
* @param r_idx 判定するモンスターの種族ID
* @return 通常の床地形に出現するならばTRUEを返す
*/
bool mon_hook_floor(MONRACE_IDX r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (!(r_ptr->flags7 & RF7_AQUATIC) ||
		(r_ptr->flags7 & RF7_CAN_FLY))
		return TRUE;
	else
		return FALSE;
}
