#include "race-resistances.h"
#include "inventory/inventory-slot-types.h"
#include "mutation/mutation-flag-types.h"
#include "player/player-race-types.h"
#include "object/object-flags.h"
#include "object-enchant/tr-types.h"
#include "player/player-race.h"
#include "player/special-defense-types.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief プレイヤーの種族による免疫フラグを返す
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param flags フラグを保管する配列
 * @return なし
 * @todo
 * xtra1.c周りと多重実装になっているのを何とかする
 */
void player_immunity(player_type *creature_ptr, BIT_FLAGS *flags)
{
	for (int i = 0; i < TR_FLAG_SIZE; i++)
		flags[i] = 0L;

	if (is_specific_player_race(creature_ptr, RACE_SPECTRE))
		add_flag(flags, TR_RES_NETHER);
	if (creature_ptr->mimic_form == MIMIC_VAMPIRE || is_specific_player_race(creature_ptr, RACE_VAMPIRE))
		add_flag(flags, TR_RES_DARK);
	if (creature_ptr->mimic_form == MIMIC_DEMON_LORD)
		add_flag(flags, TR_RES_FIRE);
	else if (is_specific_player_race(creature_ptr, RACE_YEEK) && creature_ptr->lev > 19)
		add_flag(flags, TR_RES_ACID);
}


/*!
 * @brief プレイヤーの一時的魔法効果による免疫フラグを返す
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param flags フラグを保管する配列
 * @return なし
 * @todo
 * xtra1.c周りと多重実装になっているのを何とかする
 */
void tim_player_immunity(player_type *creature_ptr, BIT_FLAGS *flags)
{
	for (int i = 0; i < TR_FLAG_SIZE; i++)
		flags[i] = 0L;

	if (creature_ptr->special_defense & DEFENSE_ACID)
		add_flag(flags, TR_RES_ACID);
	if (creature_ptr->special_defense & DEFENSE_ELEC)
		add_flag(flags, TR_RES_ELEC);
	if (creature_ptr->special_defense & DEFENSE_FIRE)
		add_flag(flags, TR_RES_FIRE);
	if (creature_ptr->special_defense & DEFENSE_COLD)
		add_flag(flags, TR_RES_COLD);
	if (creature_ptr->wraith_form)
		add_flag(flags, TR_RES_DARK);
}


/*!
 * @brief プレイヤーの装備による免疫フラグを返す
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param flags フラグを保管する配列
 * @return なし
 * @todo
 * xtra1.c周りと多重実装になっているのを何とかする
 */
void known_obj_immunity(player_type *creature_ptr, BIT_FLAGS *flags)
{
	for (int i = 0; i < TR_FLAG_SIZE; i++)
		flags[i] = 0L;

	for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++)
	{
		u32b o_flags[TR_FLAG_SIZE];
		object_type *o_ptr;
		o_ptr = &creature_ptr->inventory_list[i];
		if (!o_ptr->k_idx) continue;

		object_flags_known(creature_ptr, o_ptr, o_flags);
		if (has_flag(o_flags, TR_IM_ACID)) add_flag(flags, TR_RES_ACID);
		if (has_flag(o_flags, TR_IM_ELEC)) add_flag(flags, TR_RES_ELEC);
		if (has_flag(o_flags, TR_IM_FIRE)) add_flag(flags, TR_RES_FIRE);
		if (has_flag(o_flags, TR_IM_COLD)) add_flag(flags, TR_RES_COLD);
	}
}


/*!
 * @brief プレイヤーの種族による弱点フラグを返す
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param flags フラグを保管する配列
 * @return なし
 * @todo
 * xtra1.c周りと多重実装になっているのを何とかする
 */
void player_vulnerability_flags(player_type *creature_ptr, BIT_FLAGS *flags)
{
	for (int i = 0; i < TR_FLAG_SIZE; i++)
		flags[i] = 0L;

	if ((creature_ptr->muta3 & MUT3_VULN_ELEM) || (creature_ptr->special_defense & KATA_KOUKIJIN))
	{
		add_flag(flags, TR_RES_ACID);
		add_flag(flags, TR_RES_ELEC);
		add_flag(flags, TR_RES_FIRE);
		add_flag(flags, TR_RES_COLD);
	}

	if (is_specific_player_race(creature_ptr, RACE_ANDROID))
		add_flag(flags, TR_RES_ELEC);
	if (is_specific_player_race(creature_ptr, RACE_ENT))
		add_flag(flags, TR_RES_FIRE);
	if (is_specific_player_race(creature_ptr, RACE_VAMPIRE) || is_specific_player_race(creature_ptr, RACE_S_FAIRY) ||
		(creature_ptr->mimic_form == MIMIC_VAMPIRE))
		add_flag(flags, TR_RES_LITE);
}
