#include "permanent-resistances.h"
#include "inventory/inventory-slot-types.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/tr-types.h"
#include "player/player-personalities-types.h"
#include "player/player-race-types.h"
#include "player/player-race.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief プレーヤーの職業による耐性フラグを返す
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param flags 耐性フラグの配列
 * @return なし
 * @todo 最終的にplayer-status系列と統合する
 */
static void add_class_flags(player_type *creature_ptr, BIT_FLAGS *flags)
{
	switch (creature_ptr->pclass)
	{
	case CLASS_WARRIOR:
	{
		if (creature_ptr->lev > 29)
			add_flag(flags, TR_RES_FEAR);
		if (creature_ptr->lev > 44)
			add_flag(flags, TR_REGEN);

		break;
	}
	case CLASS_SAMURAI:
	{
		if (creature_ptr->lev > 29)
			add_flag(flags, TR_RES_FEAR);

		break;
	}
	case CLASS_PALADIN:
	{
		if (creature_ptr->lev > 39)
			add_flag(flags, TR_RES_FEAR);

		break;
	}
	case CLASS_CHAOS_WARRIOR:
	{
		if (creature_ptr->lev > 29)
			add_flag(flags, TR_RES_CHAOS);
		if (creature_ptr->lev > 39)
			add_flag(flags, TR_RES_FEAR);

		break;
	}
	case CLASS_MONK:
	case CLASS_FORCETRAINER:
	{
		if ((creature_ptr->lev > 9) && !heavy_armor(creature_ptr))
			add_flag(flags, TR_SPEED);
		if ((creature_ptr->lev > 24) && !heavy_armor(creature_ptr))
			add_flag(flags, TR_FREE_ACT);

		break;
	}
	case CLASS_NINJA:
	{
		if (heavy_armor(creature_ptr))
		{
			add_flag(flags, TR_SPEED);
		}
		else
		{
			if ((!creature_ptr->inventory_list[INVEN_RARM].k_idx || has_right_hand_weapon(creature_ptr)) &&
				(!creature_ptr->inventory_list[INVEN_LARM].k_idx || has_left_hand_weapon(creature_ptr)))
				add_flag(flags, TR_SPEED);
			if (creature_ptr->lev > 24)
				add_flag(flags, TR_FREE_ACT);
		}

		add_flag(flags, TR_SLOW_DIGEST);
		add_flag(flags, TR_RES_FEAR);
		if (creature_ptr->lev > 19)
			add_flag(flags, TR_RES_POIS);
		if (creature_ptr->lev > 24)
			add_flag(flags, TR_SUST_DEX);
		if (creature_ptr->lev > 29)
			add_flag(flags, TR_SEE_INVIS);

		break;
	}
	case CLASS_MINDCRAFTER:
	{
		if (creature_ptr->lev > 9)
			add_flag(flags, TR_RES_FEAR);
		if (creature_ptr->lev > 19)
			add_flag(flags, TR_SUST_WIS);
		if (creature_ptr->lev > 29)
			add_flag(flags, TR_RES_CONF);
		if (creature_ptr->lev > 39)
			add_flag(flags, TR_TELEPATHY);

		break;
	}
	case CLASS_BARD:
	{
		add_flag(flags, TR_RES_SOUND);
		break;
	}
	case CLASS_BERSERKER:
	{
		add_flag(flags, TR_SUST_STR);
		add_flag(flags, TR_SUST_DEX);
		add_flag(flags, TR_SUST_CON);
		add_flag(flags, TR_REGEN);
		add_flag(flags, TR_FREE_ACT);
		add_flag(flags, TR_SPEED);
		if (creature_ptr->lev > 39)
			add_flag(flags, TR_REFLECT);

		break;
	}
	case CLASS_MIRROR_MASTER:
	{
		if (creature_ptr->lev > 39)
			add_flag(flags, TR_REFLECT);

		break;
	}
	default:
		break;
	}
}


/*!
 * @brief 特定の種族に擬態中の耐性フラグを返す
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param flags 耐性フラグの配列
 * @return なし
 * @todo 最終的にplayer-status系列と統合する
 */
static void add_mimic_form_flags(player_type *creature_ptr, BIT_FLAGS *flags)
{
	switch (creature_ptr->mimic_form)
	{
	case MIMIC_DEMON:
		add_flag(flags, TR_HOLD_EXP);
		add_flag(flags, TR_RES_CHAOS);
		add_flag(flags, TR_RES_NETHER);
		add_flag(flags, TR_RES_FIRE);
		add_flag(flags, TR_SEE_INVIS);
		add_flag(flags, TR_SPEED);
		break;
	case MIMIC_DEMON_LORD:
		add_flag(flags, TR_HOLD_EXP);
		add_flag(flags, TR_RES_CHAOS);
		add_flag(flags, TR_RES_NETHER);
		add_flag(flags, TR_RES_FIRE);
		add_flag(flags, TR_RES_COLD);
		add_flag(flags, TR_RES_ELEC);
		add_flag(flags, TR_RES_ACID);
		add_flag(flags, TR_RES_POIS);
		add_flag(flags, TR_RES_CONF);
		add_flag(flags, TR_RES_DISEN);
		add_flag(flags, TR_RES_NEXUS);
		add_flag(flags, TR_RES_FEAR);
		add_flag(flags, TR_IM_FIRE);
		add_flag(flags, TR_SH_FIRE);
		add_flag(flags, TR_SEE_INVIS);
		add_flag(flags, TR_TELEPATHY);
		add_flag(flags, TR_LEVITATION);
		add_flag(flags, TR_SPEED);
		break;
	case MIMIC_VAMPIRE:
		add_flag(flags, TR_HOLD_EXP);
		add_flag(flags, TR_RES_DARK);
		add_flag(flags, TR_RES_NETHER);
		if (creature_ptr->pclass != CLASS_NINJA)
			add_flag(flags, TR_LITE_1);
		add_flag(flags, TR_RES_POIS);
		add_flag(flags, TR_RES_COLD);
		add_flag(flags, TR_SEE_INVIS);
		add_flag(flags, TR_SPEED);
		break;
	}
}


/*!
 * @brief 種族ベースの耐性フラグを返す
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param flags 耐性フラグの配列
 * @return なし
 * @todo 最終的にplayer-status系列と統合する
 */
static void add_race_flags(player_type *creature_ptr, BIT_FLAGS *flags)
{
    /* Common for all races */
    if (rp_ptr->infra > 0)
        add_flag(flags, TR_INFRA);

	/* Unique for each race */
	switch (creature_ptr->prace)
	{
	case RACE_ELF:
	{
		add_flag(flags, TR_RES_LITE);
		break;
	}
	case RACE_HOBBIT:
	{
		add_flag(flags, TR_HOLD_EXP);
		break;
	}
	case RACE_GNOME:
	{
		add_flag(flags, TR_FREE_ACT);
		break;
	}
	case RACE_DWARF:
	{
		add_flag(flags, TR_RES_BLIND);
		break;
	}
	case RACE_HALF_ORC:
	{
		add_flag(flags, TR_RES_DARK);
		break;
	}
	case RACE_HALF_TROLL:
	{
		add_flag(flags, TR_SUST_STR);
		if (creature_ptr->lev <= 14)
			break;

		add_flag(flags, TR_REGEN);
		if ((creature_ptr->pclass == CLASS_WARRIOR) || (creature_ptr->pclass == CLASS_BERSERKER))
			add_flag(flags, TR_SLOW_DIGEST);

		break;
	}
	case RACE_AMBERITE:
	{
		add_flag(flags, TR_SUST_CON);
		add_flag(flags, TR_REGEN);
		break;
	}
	case RACE_HIGH_ELF:
	{
		add_flag(flags, TR_RES_LITE);
		add_flag(flags, TR_SEE_INVIS);
		break;
	}
	case RACE_BARBARIAN:
	{
		add_flag(flags, TR_RES_FEAR);
		break;
	}
	case RACE_HALF_OGRE:
	{
		add_flag(flags, TR_SUST_STR);
		add_flag(flags, TR_RES_DARK);
		break;
	}
	case RACE_HALF_GIANT:
	{
		add_flag(flags, TR_RES_SHARDS);
		add_flag(flags, TR_SUST_STR);
		break;
	}
	case RACE_HALF_TITAN:
	{
		add_flag(flags, TR_RES_CHAOS);
		break;
	}
	case RACE_CYCLOPS:
	{
		add_flag(flags, TR_RES_SOUND);
		break;
	}
	case RACE_YEEK:
	{
		add_flag(flags, TR_RES_ACID);
		if (creature_ptr->lev > 19)
			add_flag(flags, TR_IM_ACID);

		break;
	}
	case RACE_KLACKON:
	{
		add_flag(flags, TR_RES_CONF);
		add_flag(flags, TR_RES_ACID);
		if (creature_ptr->lev > 9)
			add_flag(flags, TR_SPEED);

		break;
	}
	case RACE_KOBOLD:
	{
		add_flag(flags, TR_RES_POIS);
		break;
	}
	case RACE_NIBELUNG:
	{
		add_flag(flags, TR_RES_DISEN);
		add_flag(flags, TR_RES_DARK);
		break;
	}
	case RACE_DARK_ELF:
	{
		add_flag(flags, TR_RES_DARK);
		if (creature_ptr->lev > 19)
			add_flag(flags, TR_SEE_INVIS);

		break;
	}
	case RACE_DRACONIAN:
	{
		add_flag(flags, TR_LEVITATION);
		if (creature_ptr->lev > 4)
			add_flag(flags, TR_RES_FIRE);
		if (creature_ptr->lev > 9)
			add_flag(flags, TR_RES_COLD);
		if (creature_ptr->lev > 14)
			add_flag(flags, TR_RES_ACID);
		if (creature_ptr->lev > 19)
			add_flag(flags, TR_RES_ELEC);
		if (creature_ptr->lev > 34)
			add_flag(flags, TR_RES_POIS);

		break;
	}
	case RACE_MIND_FLAYER:
	{
		add_flag(flags, TR_SUST_INT);
		add_flag(flags, TR_SUST_WIS);
		if (creature_ptr->lev > 14)
			add_flag(flags, TR_SEE_INVIS);
		if (creature_ptr->lev > 29)
			add_flag(flags, TR_TELEPATHY);

		break;
	}
	case RACE_IMP:
	{
		add_flag(flags, TR_RES_FIRE);
		if (creature_ptr->lev > 9)
			add_flag(flags, TR_SEE_INVIS);
		break;
	}
	case RACE_GOLEM:
	{
		add_flag(flags, TR_SEE_INVIS);
		add_flag(flags, TR_FREE_ACT);
		add_flag(flags, TR_RES_POIS);
		add_flag(flags, TR_SLOW_DIGEST);
		if (creature_ptr->lev > 34)
			add_flag(flags, TR_HOLD_EXP);

		break;
	}
	case RACE_SKELETON:
	{
		add_flag(flags, TR_SEE_INVIS);
		add_flag(flags, TR_RES_SHARDS);
		add_flag(flags, TR_HOLD_EXP);
		add_flag(flags, TR_RES_POIS);
		if (creature_ptr->lev > 9)
			add_flag(flags, TR_RES_COLD);

		break;
	}
	case RACE_ZOMBIE:
	{
		add_flag(flags, TR_SEE_INVIS);
		add_flag(flags, TR_HOLD_EXP);
		add_flag(flags, TR_RES_NETHER);
		add_flag(flags, TR_RES_POIS);
		add_flag(flags, TR_SLOW_DIGEST);
		if (creature_ptr->lev > 4)
			add_flag(flags, TR_RES_COLD);

		break;
	}
	case RACE_VAMPIRE:
	{
		add_flag(flags, TR_HOLD_EXP);
		add_flag(flags, TR_RES_DARK);
		add_flag(flags, TR_RES_NETHER);
		if (creature_ptr->pclass != CLASS_NINJA)
			add_flag(flags, TR_LITE_1);

		add_flag(flags, TR_RES_POIS);
		add_flag(flags, TR_RES_COLD);
		break;
	}
	case RACE_SPECTRE:
	{
		add_flag(flags, TR_LEVITATION);
		add_flag(flags, TR_FREE_ACT);
		add_flag(flags, TR_RES_COLD);
		add_flag(flags, TR_SEE_INVIS);
		add_flag(flags, TR_HOLD_EXP);
		add_flag(flags, TR_RES_NETHER);
		add_flag(flags, TR_RES_POIS);
		add_flag(flags, TR_SLOW_DIGEST);
		if (creature_ptr->lev > 34)
			add_flag(flags, TR_TELEPATHY);

		break;
	}
	case RACE_SPRITE:
	{
		add_flag(flags, TR_RES_LITE);
		add_flag(flags, TR_LEVITATION);
		if (creature_ptr->lev > 9)
			add_flag(flags, TR_SPEED);

		break;
	}
	case RACE_BEASTMAN:
	{
		add_flag(flags, TR_RES_SOUND);
		add_flag(flags, TR_RES_CONF);
		break;
	}
	case RACE_ARCHON:
	{
		add_flag(flags, TR_LEVITATION);
		add_flag(flags, TR_SEE_INVIS);
		break;
	}
	case RACE_BALROG:
	{
		add_flag(flags, TR_RES_FIRE);
		add_flag(flags, TR_RES_NETHER);
		add_flag(flags, TR_HOLD_EXP);
		if (creature_ptr->lev > 9)
			add_flag(flags, TR_SEE_INVIS);

		break;
	}
	case RACE_DUNADAN:
	{
		add_flag(flags, TR_SUST_CON);
		break;
	}
	case RACE_S_FAIRY:
	{
		add_flag(flags, TR_LEVITATION);
		break;
	}
	case RACE_KUTAR:
	{
		add_flag(flags, TR_RES_CONF);
		break;
	}
	case RACE_ANDROID:
	{
		add_flag(flags, TR_FREE_ACT);
		add_flag(flags, TR_RES_POIS);
		add_flag(flags, TR_SLOW_DIGEST);
		add_flag(flags, TR_HOLD_EXP);
		break;
	}
	default:
		break;
	}
}


/*!
 * @brief 突然変異による耐性フラグを返す
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param flags 耐性フラグの配列
 * @return なし
 * @todo 最終的にplayer-status系列と統合する
 */
static void add_mutation_flags(player_type *creature_ptr, BIT_FLAGS *flags)
{
	if (creature_ptr->muta3 == 0) return;

	if (creature_ptr->muta3 & MUT3_FLESH_ROT)
		remove_flag(flags, TR_REGEN);
	if ((creature_ptr->muta3 & MUT3_XTRA_FAT) ||
		(creature_ptr->muta3 & MUT3_XTRA_LEGS) ||
		(creature_ptr->muta3 & MUT3_SHORT_LEG))
		add_flag(flags, TR_SPEED);
	if (creature_ptr->muta3  & MUT3_ELEC_TOUC)
		add_flag(flags, TR_SH_ELEC);
	if (creature_ptr->muta3 & MUT3_FIRE_BODY)
	{
		add_flag(flags, TR_SH_FIRE);
		add_flag(flags, TR_LITE_1);
	}

	if (creature_ptr->muta3 & MUT3_WINGS)
		add_flag(flags, TR_LEVITATION);
	if (creature_ptr->muta3 & MUT3_FEARLESS)
		add_flag(flags, TR_RES_FEAR);
	if (creature_ptr->muta3 & MUT3_REGEN)
		add_flag(flags, TR_REGEN);
	if (creature_ptr->muta3 & MUT3_ESP)
		add_flag(flags, TR_TELEPATHY);
	if (creature_ptr->muta3 & MUT3_MOTION)
		add_flag(flags, TR_FREE_ACT);
}


/*!
 * @brief 性格による耐性フラグを返す
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param flags 耐性フラグの配列
 * @return なし
 * @todo 最終的にplayer-status系列と統合する
 */
static void add_personality_flags(player_type *creature_ptr, BIT_FLAGS *flags)
{
	if (creature_ptr->pseikaku == PERSONALITY_SEXY)
		add_flag(flags, TR_AGGRAVATE);
	if (creature_ptr->pseikaku == PERSONALITY_CHARGEMAN)
		add_flag(flags, TR_RES_CONF);

	if (creature_ptr->pseikaku != PERSONALITY_MUNCHKIN) return;

	add_flag(flags, TR_RES_BLIND);
	add_flag(flags, TR_RES_CONF);
	add_flag(flags, TR_HOLD_EXP);
	if (creature_ptr->pclass != CLASS_NINJA)
		add_flag(flags, TR_LITE_1);
	if (creature_ptr->lev > 9)
		add_flag(flags, TR_SPEED);
}


/*!
 * @brief 剣術家の型による耐性フラグを返す
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param flags 耐性フラグの配列
 * @todo 最終的にplayer-status系列と統合する
 * @return なし
 */
static void add_kata_flags(player_type *creature_ptr, BIT_FLAGS *flags)
{
	if (creature_ptr->special_defense & KATA_FUUJIN)
		add_flag(flags, TR_REFLECT);
	if (creature_ptr->special_defense & KAMAE_GENBU)
		add_flag(flags, TR_REFLECT);
	if (creature_ptr->special_defense & KAMAE_SUZAKU)
		add_flag(flags, TR_LEVITATION);
	if (creature_ptr->special_defense & KAMAE_SEIRYU)
	{
		add_flag(flags, TR_RES_FIRE);
		add_flag(flags, TR_RES_COLD);
		add_flag(flags, TR_RES_ACID);
		add_flag(flags, TR_RES_ELEC);
		add_flag(flags, TR_RES_POIS);
		add_flag(flags, TR_LEVITATION);
		add_flag(flags, TR_SH_FIRE);
		add_flag(flags, TR_SH_ELEC);
		add_flag(flags, TR_SH_COLD);
	}

	if ((creature_ptr->special_defense & KATA_MUSOU) == 0) return;

	add_flag(flags, TR_RES_FEAR);
	add_flag(flags, TR_RES_LITE);
	add_flag(flags, TR_RES_DARK);
	add_flag(flags, TR_RES_BLIND);
	add_flag(flags, TR_RES_CONF);
	add_flag(flags, TR_RES_SOUND);
	add_flag(flags, TR_RES_SHARDS);
	add_flag(flags, TR_RES_NETHER);
	add_flag(flags, TR_RES_NEXUS);
	add_flag(flags, TR_RES_CHAOS);
	add_flag(flags, TR_RES_DISEN);
	add_flag(flags, TR_REFLECT);
	add_flag(flags, TR_HOLD_EXP);
	add_flag(flags, TR_FREE_ACT);
	add_flag(flags, TR_SH_FIRE);
	add_flag(flags, TR_SH_ELEC);
	add_flag(flags, TR_SH_COLD);
	add_flag(flags, TR_LEVITATION);
	add_flag(flags, TR_LITE_1);
	add_flag(flags, TR_SEE_INVIS);
	add_flag(flags, TR_TELEPATHY);
	add_flag(flags, TR_SLOW_DIGEST);
	add_flag(flags, TR_REGEN);
	add_flag(flags, TR_SUST_STR);
	add_flag(flags, TR_SUST_INT);
	add_flag(flags, TR_SUST_WIS);
	add_flag(flags, TR_SUST_DEX);
	add_flag(flags, TR_SUST_CON);
	add_flag(flags, TR_SUST_CHR);
}


/*!
 * @brief プレイヤーの職業、種族に応じた耐性フラグを返す
 * Prints ratings on certain abilities
 * @param creature_ptr 参照元クリーチャーポインタ
 * @param flags フラグを保管する配列
 * @return なし
 * @details
 * Obtain the "flags" for the player as if he was an item
 * @todo 最終的にplayer-status系列と統合する
 */
void player_flags(player_type *creature_ptr, BIT_FLAGS *flags)
{
	for (int i = 0; i < TR_FLAG_SIZE; i++)
		flags[i] = 0L;

	add_class_flags(creature_ptr, flags);
	void(*race_flags_func)(player_type*, BIT_FLAGS*) = creature_ptr->mimic_form
		? add_mimic_form_flags
		: add_race_flags;
	(*race_flags_func)(creature_ptr, flags);

	add_mutation_flags(creature_ptr, flags);
	add_personality_flags(creature_ptr, flags);
	add_kata_flags(creature_ptr, flags);
}
