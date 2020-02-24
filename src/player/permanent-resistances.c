#include "permanent-resistances.h"
#include "player-personality.h"

/*!
 * @brief プレイヤーの職業、種族に応じた耐性フラグを返す
 * Prints ratings on certain abilities
 * @param creature_ptr 参照元クリーチャーポインタ
 * @param flgs フラグを保管する配列
 * @return なし
 * @details
 * Obtain the "flags" for the player as if he was an item
 * @todo
 * xtra1.c周りと多重実装になっているのを何とかする
 */
void player_flags(player_type *creature_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE])
{
	for (int i = 0; i < TR_FLAG_SIZE; i++)
		flgs[i] = 0L;

	switch (creature_ptr->pclass)
	{
	case CLASS_WARRIOR:
		if (creature_ptr->lev > 44)
			add_flag(flgs, TR_REGEN);
	case CLASS_SAMURAI:
		if (creature_ptr->lev > 29)
			add_flag(flgs, TR_RES_FEAR);
		break;
	case CLASS_PALADIN:
		if (creature_ptr->lev > 39)
			add_flag(flgs, TR_RES_FEAR);
		break;
	case CLASS_CHAOS_WARRIOR:
		if (creature_ptr->lev > 29)
			add_flag(flgs, TR_RES_CHAOS);
		if (creature_ptr->lev > 39)
			add_flag(flgs, TR_RES_FEAR);
		break;
	case CLASS_MONK:
	case CLASS_FORCETRAINER:
		if ((creature_ptr->lev > 9) && !heavy_armor(creature_ptr))
			add_flag(flgs, TR_SPEED);
		if ((creature_ptr->lev > 24) && !heavy_armor(creature_ptr))
			add_flag(flgs, TR_FREE_ACT);
		break;
	case CLASS_NINJA:
		if (heavy_armor(creature_ptr))
			add_flag(flgs, TR_SPEED);
		else
		{
			if ((!creature_ptr->inventory_list[INVEN_RARM].k_idx || creature_ptr->migite) &&
				(!creature_ptr->inventory_list[INVEN_LARM].k_idx || creature_ptr->hidarite))
				add_flag(flgs, TR_SPEED);
			if (creature_ptr->lev > 24)
				add_flag(flgs, TR_FREE_ACT);
		}

		add_flag(flgs, TR_SLOW_DIGEST);
		add_flag(flgs, TR_RES_FEAR);
		if (creature_ptr->lev > 19) add_flag(flgs, TR_RES_POIS);
		if (creature_ptr->lev > 24) add_flag(flgs, TR_SUST_DEX);
		if (creature_ptr->lev > 29) add_flag(flgs, TR_SEE_INVIS);
		break;
	case CLASS_MINDCRAFTER:
		if (creature_ptr->lev > 9)
			add_flag(flgs, TR_RES_FEAR);
		if (creature_ptr->lev > 19)
			add_flag(flgs, TR_SUST_WIS);
		if (creature_ptr->lev > 29)
			add_flag(flgs, TR_RES_CONF);
		if (creature_ptr->lev > 39)
			add_flag(flgs, TR_TELEPATHY);
		break;
	case CLASS_BARD:
		add_flag(flgs, TR_RES_SOUND);
		break;
	case CLASS_BERSERKER:
		add_flag(flgs, TR_SUST_STR);
		add_flag(flgs, TR_SUST_DEX);
		add_flag(flgs, TR_SUST_CON);
		add_flag(flgs, TR_REGEN);
		add_flag(flgs, TR_FREE_ACT);
		add_flag(flgs, TR_SPEED);
		if (creature_ptr->lev > 39) add_flag(flgs, TR_REFLECT);
		break;
	case CLASS_MIRROR_MASTER:
		if (creature_ptr->lev > 39)add_flag(flgs, TR_REFLECT);
		break;
	default:
		break;
	}

	/* Races */
	if (creature_ptr->mimic_form)
	{
		switch (creature_ptr->mimic_form)
		{
		case MIMIC_DEMON:
			add_flag(flgs, TR_HOLD_EXP);
			add_flag(flgs, TR_RES_CHAOS);
			add_flag(flgs, TR_RES_NETHER);
			add_flag(flgs, TR_RES_FIRE);
			add_flag(flgs, TR_SEE_INVIS);
			add_flag(flgs, TR_SPEED);
			break;
		case MIMIC_DEMON_LORD:
			add_flag(flgs, TR_HOLD_EXP);
			add_flag(flgs, TR_RES_CHAOS);
			add_flag(flgs, TR_RES_NETHER);
			add_flag(flgs, TR_RES_FIRE);
			add_flag(flgs, TR_RES_COLD);
			add_flag(flgs, TR_RES_ELEC);
			add_flag(flgs, TR_RES_ACID);
			add_flag(flgs, TR_RES_POIS);
			add_flag(flgs, TR_RES_CONF);
			add_flag(flgs, TR_RES_DISEN);
			add_flag(flgs, TR_RES_NEXUS);
			add_flag(flgs, TR_RES_FEAR);
			add_flag(flgs, TR_IM_FIRE);
			add_flag(flgs, TR_SH_FIRE);
			add_flag(flgs, TR_SEE_INVIS);
			add_flag(flgs, TR_TELEPATHY);
			add_flag(flgs, TR_LEVITATION);
			add_flag(flgs, TR_SPEED);
			break;
		case MIMIC_VAMPIRE:
			add_flag(flgs, TR_HOLD_EXP);
			add_flag(flgs, TR_RES_DARK);
			add_flag(flgs, TR_RES_NETHER);
			if (creature_ptr->pclass != CLASS_NINJA) add_flag(flgs, TR_LITE_1);
			add_flag(flgs, TR_RES_POIS);
			add_flag(flgs, TR_RES_COLD);
			add_flag(flgs, TR_SEE_INVIS);
			add_flag(flgs, TR_SPEED);
			break;
		}
	}
	else
	{
		switch (creature_ptr->prace)
		{
		case RACE_ELF:
			add_flag(flgs, TR_RES_LITE);
			break;
		case RACE_HOBBIT:
			add_flag(flgs, TR_HOLD_EXP);
			break;
		case RACE_GNOME:
			add_flag(flgs, TR_FREE_ACT);
			break;
		case RACE_DWARF:
			add_flag(flgs, TR_RES_BLIND);
			break;
		case RACE_HALF_ORC:
			add_flag(flgs, TR_RES_DARK);
			break;
		case RACE_HALF_TROLL:
			add_flag(flgs, TR_SUST_STR);
			if (creature_ptr->lev <= 14) break;

			add_flag(flgs, TR_REGEN);
			if ((creature_ptr->pclass == CLASS_WARRIOR) || (creature_ptr->pclass == CLASS_BERSERKER))
			{
				add_flag(flgs, TR_SLOW_DIGEST);
				/*
				 * Let's not make Regeneration a disadvantage
				 * for the poor warriors who can never learn
				 * a spell that satisfies hunger (actually
				 * neither can rogues, but half-trolls are not
				 * supposed to play rogues)
				 */
			}

			break;
		case RACE_AMBERITE:
			add_flag(flgs, TR_SUST_CON);
			add_flag(flgs, TR_REGEN);
			break;
		case RACE_HIGH_ELF:
			add_flag(flgs, TR_RES_LITE);
			add_flag(flgs, TR_SEE_INVIS);
			break;
		case RACE_BARBARIAN:
			add_flag(flgs, TR_RES_FEAR);
			break;
		case RACE_HALF_OGRE:
			add_flag(flgs, TR_SUST_STR);
			add_flag(flgs, TR_RES_DARK);
			break;
		case RACE_HALF_GIANT:
			add_flag(flgs, TR_RES_SHARDS);
			add_flag(flgs, TR_SUST_STR);
			break;
		case RACE_HALF_TITAN:
			add_flag(flgs, TR_RES_CHAOS);
			break;
		case RACE_CYCLOPS:
			add_flag(flgs, TR_RES_SOUND);
			break;
		case RACE_YEEK:
			add_flag(flgs, TR_RES_ACID);
			if (creature_ptr->lev > 19)
				add_flag(flgs, TR_IM_ACID);
			break;
		case RACE_KLACKON:
			add_flag(flgs, TR_RES_CONF);
			add_flag(flgs, TR_RES_ACID);
			if (creature_ptr->lev > 9)
				add_flag(flgs, TR_SPEED);
			break;
		case RACE_KOBOLD:
			add_flag(flgs, TR_RES_POIS);
			break;
		case RACE_NIBELUNG:
			add_flag(flgs, TR_RES_DISEN);
			add_flag(flgs, TR_RES_DARK);
			break;
		case RACE_DARK_ELF:
			add_flag(flgs, TR_RES_DARK);
			if (creature_ptr->lev > 19)
				add_flag(flgs, TR_SEE_INVIS);
			break;
		case RACE_DRACONIAN:
			add_flag(flgs, TR_LEVITATION);
			if (creature_ptr->lev > 4)
				add_flag(flgs, TR_RES_FIRE);
			if (creature_ptr->lev > 9)
				add_flag(flgs, TR_RES_COLD);
			if (creature_ptr->lev > 14)
				add_flag(flgs, TR_RES_ACID);
			if (creature_ptr->lev > 19)
				add_flag(flgs, TR_RES_ELEC);
			if (creature_ptr->lev > 34)
				add_flag(flgs, TR_RES_POIS);
			break;
		case RACE_MIND_FLAYER:
			add_flag(flgs, TR_SUST_INT);
			add_flag(flgs, TR_SUST_WIS);
			if (creature_ptr->lev > 14)
				add_flag(flgs, TR_SEE_INVIS);
			if (creature_ptr->lev > 29)
				add_flag(flgs, TR_TELEPATHY);
			break;
		case RACE_IMP:
			add_flag(flgs, TR_RES_FIRE);
			if (creature_ptr->lev > 9)
				add_flag(flgs, TR_SEE_INVIS);
			break;
		case RACE_GOLEM:
			add_flag(flgs, TR_SEE_INVIS);
			add_flag(flgs, TR_FREE_ACT);
			add_flag(flgs, TR_RES_POIS);
			add_flag(flgs, TR_SLOW_DIGEST);
			if (creature_ptr->lev > 34)
				add_flag(flgs, TR_HOLD_EXP);
			break;
		case RACE_SKELETON:
			add_flag(flgs, TR_SEE_INVIS);
			add_flag(flgs, TR_RES_SHARDS);
			add_flag(flgs, TR_HOLD_EXP);
			add_flag(flgs, TR_RES_POIS);
			if (creature_ptr->lev > 9)
				add_flag(flgs, TR_RES_COLD);
			break;
		case RACE_ZOMBIE:
			add_flag(flgs, TR_SEE_INVIS);
			add_flag(flgs, TR_HOLD_EXP);
			add_flag(flgs, TR_RES_NETHER);
			add_flag(flgs, TR_RES_POIS);
			add_flag(flgs, TR_SLOW_DIGEST);
			if (creature_ptr->lev > 4)
				add_flag(flgs, TR_RES_COLD);
			break;
		case RACE_VAMPIRE:
			add_flag(flgs, TR_HOLD_EXP);
			add_flag(flgs, TR_RES_DARK);
			add_flag(flgs, TR_RES_NETHER);
			if (creature_ptr->pclass != CLASS_NINJA) add_flag(flgs, TR_LITE_1);
			add_flag(flgs, TR_RES_POIS);
			add_flag(flgs, TR_RES_COLD);
			break;
		case RACE_SPECTRE:
			add_flag(flgs, TR_LEVITATION);
			add_flag(flgs, TR_FREE_ACT);
			add_flag(flgs, TR_RES_COLD);
			add_flag(flgs, TR_SEE_INVIS);
			add_flag(flgs, TR_HOLD_EXP);
			add_flag(flgs, TR_RES_NETHER);
			add_flag(flgs, TR_RES_POIS);
			add_flag(flgs, TR_SLOW_DIGEST);
			if (creature_ptr->lev > 34)
				add_flag(flgs, TR_TELEPATHY);
			break;
		case RACE_SPRITE:
			add_flag(flgs, TR_RES_LITE);
			add_flag(flgs, TR_LEVITATION);
			if (creature_ptr->lev > 9)
				add_flag(flgs, TR_SPEED);
			break;
		case RACE_BEASTMAN:
			add_flag(flgs, TR_RES_SOUND);
			add_flag(flgs, TR_RES_CONF);
			break;
		case RACE_ANGEL:
			add_flag(flgs, TR_LEVITATION);
			add_flag(flgs, TR_SEE_INVIS);
			break;
		case RACE_DEMON:
			add_flag(flgs, TR_RES_FIRE);
			add_flag(flgs, TR_RES_NETHER);
			add_flag(flgs, TR_HOLD_EXP);
			if (creature_ptr->lev > 9)
				add_flag(flgs, TR_SEE_INVIS);
			break;
		case RACE_DUNADAN:
			add_flag(flgs, TR_SUST_CON);
			break;
		case RACE_S_FAIRY:
			add_flag(flgs, TR_LEVITATION);
			break;
		case RACE_KUTAR:
			add_flag(flgs, TR_RES_CONF);
			break;
		case RACE_ANDROID:
			add_flag(flgs, TR_FREE_ACT);
			add_flag(flgs, TR_RES_POIS);
			add_flag(flgs, TR_SLOW_DIGEST);
			add_flag(flgs, TR_HOLD_EXP);
			break;
		default:
			break;
		}
	}

	if (creature_ptr->muta3)
	{
		if (creature_ptr->muta3 & MUT3_FLESH_ROT)
		{
			remove_flag(flgs, TR_REGEN);
		}

		if ((creature_ptr->muta3 & MUT3_XTRA_FAT) ||
			(creature_ptr->muta3 & MUT3_XTRA_LEGS) ||
			(creature_ptr->muta3 & MUT3_SHORT_LEG))
		{
			add_flag(flgs, TR_SPEED);
		}

		if (creature_ptr->muta3  & MUT3_ELEC_TOUC)
		{
			add_flag(flgs, TR_SH_ELEC);
		}

		if (creature_ptr->muta3 & MUT3_FIRE_BODY)
		{
			add_flag(flgs, TR_SH_FIRE);
			add_flag(flgs, TR_LITE_1);
		}

		if (creature_ptr->muta3 & MUT3_WINGS)
		{
			add_flag(flgs, TR_LEVITATION);
		}

		if (creature_ptr->muta3 & MUT3_FEARLESS)
		{
			add_flag(flgs, TR_RES_FEAR);
		}

		if (creature_ptr->muta3 & MUT3_REGEN)
		{
			add_flag(flgs, TR_REGEN);
		}

		if (creature_ptr->muta3 & MUT3_ESP)
		{
			add_flag(flgs, TR_TELEPATHY);
		}

		if (creature_ptr->muta3 & MUT3_MOTION)
		{
			add_flag(flgs, TR_FREE_ACT);
		}
	}

	if (creature_ptr->pseikaku == SEIKAKU_SEXY)
		add_flag(flgs, TR_AGGRAVATE);
	if (creature_ptr->pseikaku == SEIKAKU_CHARGEMAN)
		add_flag(flgs, TR_RES_CONF);
	if (creature_ptr->pseikaku == SEIKAKU_MUNCHKIN)
	{
		add_flag(flgs, TR_RES_BLIND);
		add_flag(flgs, TR_RES_CONF);
		add_flag(flgs, TR_HOLD_EXP);
		if (creature_ptr->pclass != CLASS_NINJA) add_flag(flgs, TR_LITE_1);
		if (creature_ptr->lev > 9)
			add_flag(flgs, TR_SPEED);
	}

	if (creature_ptr->special_defense & KATA_FUUJIN)
		add_flag(flgs, TR_REFLECT);

	if (creature_ptr->special_defense & KAMAE_GENBU)
		add_flag(flgs, TR_REFLECT);

	if (creature_ptr->special_defense & KAMAE_SUZAKU)
		add_flag(flgs, TR_LEVITATION);

	if (creature_ptr->special_defense & KAMAE_SEIRYU)
	{
		add_flag(flgs, TR_RES_FIRE);
		add_flag(flgs, TR_RES_COLD);
		add_flag(flgs, TR_RES_ACID);
		add_flag(flgs, TR_RES_ELEC);
		add_flag(flgs, TR_RES_POIS);
		add_flag(flgs, TR_LEVITATION);
		add_flag(flgs, TR_SH_FIRE);
		add_flag(flgs, TR_SH_ELEC);
		add_flag(flgs, TR_SH_COLD);
	}

	if (creature_ptr->special_defense & KATA_MUSOU)
	{
		add_flag(flgs, TR_RES_FEAR);
		add_flag(flgs, TR_RES_LITE);
		add_flag(flgs, TR_RES_DARK);
		add_flag(flgs, TR_RES_BLIND);
		add_flag(flgs, TR_RES_CONF);
		add_flag(flgs, TR_RES_SOUND);
		add_flag(flgs, TR_RES_SHARDS);
		add_flag(flgs, TR_RES_NETHER);
		add_flag(flgs, TR_RES_NEXUS);
		add_flag(flgs, TR_RES_CHAOS);
		add_flag(flgs, TR_RES_DISEN);
		add_flag(flgs, TR_REFLECT);
		add_flag(flgs, TR_HOLD_EXP);
		add_flag(flgs, TR_FREE_ACT);
		add_flag(flgs, TR_SH_FIRE);
		add_flag(flgs, TR_SH_ELEC);
		add_flag(flgs, TR_SH_COLD);
		add_flag(flgs, TR_LEVITATION);
		add_flag(flgs, TR_LITE_1);
		add_flag(flgs, TR_SEE_INVIS);
		add_flag(flgs, TR_TELEPATHY);
		add_flag(flgs, TR_SLOW_DIGEST);
		add_flag(flgs, TR_REGEN);
		add_flag(flgs, TR_SUST_STR);
		add_flag(flgs, TR_SUST_INT);
		add_flag(flgs, TR_SUST_WIS);
		add_flag(flgs, TR_SUST_DEX);
		add_flag(flgs, TR_SUST_CON);
		add_flag(flgs, TR_SUST_CHR);
	}
}
