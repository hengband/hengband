#include "race-resistances.h"
#include "inventory/inventory-slot-types.h"
#include "mutation/mutation-flag-types.h"
#include "player/player-race-types.h"
#include "object/object-flags.h"
#include "object-enchant/tr-types.h"
#include "player/player-race.h"
#include "player/special-defense-types.h"
#include "mind/mind-elementalist.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief プレイヤーの職業/種族による免疫フラグを返す
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param flags フラグを保管する配列
 * @todo
 * xtra1.c周りと多重実装になっているのを何とかする
 */
void player_immunity(player_type *creature_ptr, TrFlags &flags)
{
    for (int i = 0; i < TR_FLAG_SIZE; i++) {
        flags[i] = 0L;
    }

    if (player_race_has_flag(creature_ptr, TR_IM_ACID))
        add_flag(flags, TR_RES_ACID);
    if (player_race_has_flag(creature_ptr, TR_IM_COLD))
        add_flag(flags, TR_RES_COLD);
    if (player_race_has_flag(creature_ptr, TR_IM_ELEC))
        add_flag(flags, TR_RES_ELEC);
    if (player_race_has_flag(creature_ptr, TR_IM_FIRE))
        add_flag(flags, TR_RES_FIRE);

    if (is_specific_player_race(creature_ptr, player_race_type::SPECTRE))
        add_flag(flags, TR_RES_NETHER);
    if (player_race_has_flag(creature_ptr, TR_IM_DARK))
        add_flag(flags, TR_RES_DARK);

    if (creature_ptr->pclass == CLASS_ELEMENTALIST) {
        if (has_element_resist(creature_ptr, ElementRealm::FIRE, 30))
            add_flag(flags, TR_RES_FIRE);
        if (has_element_resist(creature_ptr, ElementRealm::SKY, 30))
            add_flag(flags, TR_RES_ELEC);
        if (has_element_resist(creature_ptr, ElementRealm::SEA, 30))
            add_flag(flags, TR_RES_ACID);
        if (has_element_resist(creature_ptr, ElementRealm::ICE, 30))
            add_flag(flags, TR_RES_COLD);
    }
}

/*!
 * @brief プレイヤーの一時的魔法効果による免疫フラグを返す
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param flags フラグを保管する配列
 * @todo
 * xtra1.c周りと多重実装になっているのを何とかする
 */
void tim_player_immunity(player_type *creature_ptr, TrFlags &flags)
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
 * @todo
 * xtra1.c周りと多重実装になっているのを何とかする
 */
void known_obj_immunity(player_type *creature_ptr, TrFlags &flags)
{
	for (int i = 0; i < TR_FLAG_SIZE; i++)
		flags[i] = 0L;

	for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++)
	{
		TrFlags o_flags;
		object_type *o_ptr;
		o_ptr = &creature_ptr->inventory_list[i];
		if (!o_ptr->k_idx) continue;

                object_flags_known(o_ptr, o_flags);
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
 * @todo
 * xtra1.c周りと多重実装になっているのを何とかする
 */
void player_vulnerability_flags(player_type *creature_ptr, TrFlags &flags)
{
	for (int i = 0; i < TR_FLAG_SIZE; i++)
		flags[i] = 0L;

	if (creature_ptr->muta.has(MUTA::VULN_ELEM) || (creature_ptr->special_defense & KATA_KOUKIJIN))
	{
		add_flag(flags, TR_RES_ACID);
		add_flag(flags, TR_RES_ELEC);
		add_flag(flags, TR_RES_FIRE);
		add_flag(flags, TR_RES_COLD);
	}

	if (player_race_has_flag(creature_ptr, TR_VUL_ACID))
            add_flag(flags, TR_RES_ACID);
    if (player_race_has_flag(creature_ptr, TR_VUL_COLD))
        add_flag(flags, TR_RES_COLD);
    if (player_race_has_flag(creature_ptr, TR_VUL_ELEC))
        add_flag(flags, TR_RES_ELEC);
    if (player_race_has_flag(creature_ptr, TR_VUL_FIRE))
        add_flag(flags, TR_RES_FIRE);
    if (player_race_has_flag(creature_ptr, TR_VUL_LITE))
        add_flag(flags, TR_RES_LITE);
}
