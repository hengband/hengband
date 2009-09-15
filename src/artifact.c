/* Purpose: Artifact code */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "angband.h"


/* Chance of using syllables to form the name instead of the "template" files */
#define SINDARIN_NAME   10
#define TABLE_NAME      20
#define A_CURSED        13
#define WEIRD_LUCK      12
#define BIAS_LUCK       20
#define IM_LUCK         7

/*
 * Bias luck needs to be higher than weird luck,
 * since it is usually tested several times...
 */
#define ACTIVATION_CHANCE 3


/*
 * Use for biased artifact creation
 */
static int artifact_bias;


/*
 * Choose one random sustain
 */
void one_sustain(object_type *o_ptr)
{
	switch (randint0(6))
	{
		case 0: add_flag(o_ptr->art_flags, TR_SUST_STR); break;
		case 1: add_flag(o_ptr->art_flags, TR_SUST_INT); break;
		case 2: add_flag(o_ptr->art_flags, TR_SUST_WIS); break;
		case 3: add_flag(o_ptr->art_flags, TR_SUST_DEX); break;
		case 4: add_flag(o_ptr->art_flags, TR_SUST_CON); break;
		case 5: add_flag(o_ptr->art_flags, TR_SUST_CHR); break;
	}
}


/*
 * Choose one random high resistance
 */
void one_high_resistance(object_type *o_ptr)
{
	switch (randint0(12))
	{
		case  0: add_flag(o_ptr->art_flags, TR_RES_POIS);   break;
		case  1: add_flag(o_ptr->art_flags, TR_RES_LITE);   break;
		case  2: add_flag(o_ptr->art_flags, TR_RES_DARK);   break;
		case  3: add_flag(o_ptr->art_flags, TR_RES_SHARDS); break;
		case  4: add_flag(o_ptr->art_flags, TR_RES_BLIND);  break;
		case  5: add_flag(o_ptr->art_flags, TR_RES_CONF);   break;
		case  6: add_flag(o_ptr->art_flags, TR_RES_SOUND);  break;
		case  7: add_flag(o_ptr->art_flags, TR_RES_NETHER); break;
		case  8: add_flag(o_ptr->art_flags, TR_RES_NEXUS);  break;
		case  9: add_flag(o_ptr->art_flags, TR_RES_CHAOS);  break;
		case 10: add_flag(o_ptr->art_flags, TR_RES_DISEN);  break;
		case 11: add_flag(o_ptr->art_flags, TR_RES_FEAR);   break;
	}
}


/*
 * Choose one random high resistance ( except poison and disenchantment )
 */
void one_lordly_high_resistance(object_type *o_ptr)
{
	switch (randint0(10))
	{
		case 0: add_flag(o_ptr->art_flags, TR_RES_LITE);   break;
		case 1: add_flag(o_ptr->art_flags, TR_RES_DARK);   break;
		case 2: add_flag(o_ptr->art_flags, TR_RES_SHARDS); break;
		case 3: add_flag(o_ptr->art_flags, TR_RES_BLIND);  break;
		case 4: add_flag(o_ptr->art_flags, TR_RES_CONF);   break;
		case 5: add_flag(o_ptr->art_flags, TR_RES_SOUND);  break;
		case 6: add_flag(o_ptr->art_flags, TR_RES_NETHER); break;
		case 7: add_flag(o_ptr->art_flags, TR_RES_NEXUS);  break;
		case 8: add_flag(o_ptr->art_flags, TR_RES_CHAOS);  break;
		case 9: add_flag(o_ptr->art_flags, TR_RES_FEAR);   break;
	}
}


/*
 * Choose one random element resistance
 */
void one_ele_resistance(object_type *o_ptr)
{
	switch (randint0(4))
	{
		case  0: add_flag(o_ptr->art_flags, TR_RES_ACID); break;
		case  1: add_flag(o_ptr->art_flags, TR_RES_ELEC); break;
		case  2: add_flag(o_ptr->art_flags, TR_RES_COLD); break;
		case  3: add_flag(o_ptr->art_flags, TR_RES_FIRE); break;
	}
}


/*
 * Choose one random element or poison resistance
 */
void one_dragon_ele_resistance(object_type *o_ptr)
{
	if (one_in_(7))
	{
		add_flag(o_ptr->art_flags, TR_RES_POIS);
	}
	else
	{
		one_ele_resistance(o_ptr);
	}
}


/*
 * Choose one lower rank esp
 */
void one_low_esp(object_type *o_ptr)
{
	switch (randint1(9))
	{
	case 1: add_flag(o_ptr->art_flags, TR_ESP_ANIMAL);   break;
	case 2: add_flag(o_ptr->art_flags, TR_ESP_UNDEAD);   break;
	case 3: add_flag(o_ptr->art_flags, TR_ESP_DEMON);   break;
	case 4: add_flag(o_ptr->art_flags, TR_ESP_ORC);   break;
	case 5: add_flag(o_ptr->art_flags, TR_ESP_TROLL);   break;
	case 6: add_flag(o_ptr->art_flags, TR_ESP_GIANT);   break;
	case 7: add_flag(o_ptr->art_flags, TR_ESP_DRAGON);   break;
	case 8: add_flag(o_ptr->art_flags, TR_ESP_HUMAN);   break;
	case 9: add_flag(o_ptr->art_flags, TR_ESP_GOOD);   break;
	}
}



/*
 * Choose one random resistance
 */
void one_resistance(object_type *o_ptr)
{
	if (one_in_(3))
	{
		one_ele_resistance(o_ptr);
	}
	else
	{
		one_high_resistance(o_ptr);
	}
}


/*
 * Choose one random ability
 */
void one_ability(object_type *o_ptr)
{
	switch (randint0(10))
	{
	case 0: add_flag(o_ptr->art_flags, TR_LEVITATION);     break;
	case 1: add_flag(o_ptr->art_flags, TR_LITE);        break;
	case 2: add_flag(o_ptr->art_flags, TR_SEE_INVIS);   break;
	case 3: add_flag(o_ptr->art_flags, TR_WARNING);     break;
	case 4: add_flag(o_ptr->art_flags, TR_SLOW_DIGEST); break;
	case 5: add_flag(o_ptr->art_flags, TR_REGEN);       break;
	case 6: add_flag(o_ptr->art_flags, TR_FREE_ACT);    break;
	case 7: add_flag(o_ptr->art_flags, TR_HOLD_LIFE);   break;
	case 8:
	case 9:
		one_low_esp(o_ptr);
		break;
	}
}


static void curse_artifact(object_type * o_ptr)
{
	if (o_ptr->pval > 0) o_ptr->pval = 0 - (o_ptr->pval + randint1(4));
	if (o_ptr->to_a > 0) o_ptr->to_a = 0 - (o_ptr->to_a + randint1(4));
	if (o_ptr->to_h > 0) o_ptr->to_h = 0 - (o_ptr->to_h + randint1(4));
	if (o_ptr->to_d > 0) o_ptr->to_d = 0 - (o_ptr->to_d + randint1(4));

	o_ptr->curse_flags |= (TRC_HEAVY_CURSE | TRC_CURSED);
	remove_flag(o_ptr->art_flags, TR_BLESSED);

	if (one_in_(4)) o_ptr->curse_flags |= TRC_PERMA_CURSE;
	if (one_in_(3)) add_flag(o_ptr->art_flags, TR_TY_CURSE);
	if (one_in_(2)) add_flag(o_ptr->art_flags, TR_AGGRAVATE);
	if (one_in_(3)) add_flag(o_ptr->art_flags, TR_DRAIN_EXP);
	if (one_in_(2)) add_flag(o_ptr->art_flags, TR_TELEPORT);
	else if (one_in_(3)) add_flag(o_ptr->art_flags, TR_NO_TELE);

	if ((p_ptr->pclass != CLASS_WARRIOR) && (p_ptr->pclass != CLASS_ARCHER) && (p_ptr->pclass != CLASS_CAVALRY) && (p_ptr->pclass != CLASS_BERSERKER) && (p_ptr->pclass != CLASS_SMITH) && one_in_(3))
		add_flag(o_ptr->art_flags, TR_NO_MAGIC);
}


static void random_plus(object_type * o_ptr)
{
	int this_type = (object_is_weapon_ammo(o_ptr) ? 23 : 19);

	switch (artifact_bias)
	{
	case BIAS_WARRIOR:
		if (!(have_flag(o_ptr->art_flags, TR_STR)))
		{
			add_flag(o_ptr->art_flags, TR_STR);
			if (one_in_(2)) return;
		}

		if (!(have_flag(o_ptr->art_flags, TR_CON)))
		{
			add_flag(o_ptr->art_flags, TR_CON);
			if (one_in_(2)) return;
		}

		if (!(have_flag(o_ptr->art_flags, TR_DEX)))
		{
			add_flag(o_ptr->art_flags, TR_DEX);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_MAGE:
		if (!(have_flag(o_ptr->art_flags, TR_INT)))
		{
			add_flag(o_ptr->art_flags, TR_INT);
			if (one_in_(2)) return;
		}
		if ((o_ptr->tval == TV_GLOVES) && !(have_flag(o_ptr->art_flags, TR_MAGIC_MASTERY)))
		{
			add_flag(o_ptr->art_flags, TR_MAGIC_MASTERY);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_PRIESTLY:
		if (!(have_flag(o_ptr->art_flags, TR_WIS)))
		{
			add_flag(o_ptr->art_flags, TR_WIS);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_RANGER:
		if (!(have_flag(o_ptr->art_flags, TR_DEX)))
		{
			add_flag(o_ptr->art_flags, TR_DEX);
			if (one_in_(2)) return;
		}

		if (!(have_flag(o_ptr->art_flags, TR_CON)))
		{
			add_flag(o_ptr->art_flags, TR_CON);
			if (one_in_(2)) return;
		}

		if (!(have_flag(o_ptr->art_flags, TR_STR)))
		{
			add_flag(o_ptr->art_flags, TR_STR);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_ROGUE:
		if (!(have_flag(o_ptr->art_flags, TR_STEALTH)))
		{
			add_flag(o_ptr->art_flags, TR_STEALTH);
			if (one_in_(2)) return;
		}
		if (!(have_flag(o_ptr->art_flags, TR_SEARCH)))
		{
			add_flag(o_ptr->art_flags, TR_SEARCH);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_STR:
		if (!(have_flag(o_ptr->art_flags, TR_STR)))
		{
			add_flag(o_ptr->art_flags, TR_STR);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_WIS:
		if (!(have_flag(o_ptr->art_flags, TR_WIS)))
		{
			add_flag(o_ptr->art_flags, TR_WIS);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_INT:
		if (!(have_flag(o_ptr->art_flags, TR_INT)))
		{
			add_flag(o_ptr->art_flags, TR_INT);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_DEX:
		if (!(have_flag(o_ptr->art_flags, TR_DEX)))
		{
			add_flag(o_ptr->art_flags, TR_DEX);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_CON:
		if (!(have_flag(o_ptr->art_flags, TR_CON)))
		{
			add_flag(o_ptr->art_flags, TR_CON);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_CHR:
		if (!(have_flag(o_ptr->art_flags, TR_CHR)))
		{
			add_flag(o_ptr->art_flags, TR_CHR);
			if (one_in_(2)) return;
		}
		break;
	}

	if ((artifact_bias == BIAS_MAGE || artifact_bias == BIAS_PRIESTLY) && (o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_ROBE))
	{
		if (!(have_flag(o_ptr->art_flags, TR_DEC_MANA)) && one_in_(3))
		{
			add_flag(o_ptr->art_flags, TR_DEC_MANA);
			if (one_in_(2)) return;
		}
	}

	switch (randint1(this_type))
	{
	case 1: case 2:
		add_flag(o_ptr->art_flags, TR_STR);
		if (!artifact_bias && !one_in_(13))
			artifact_bias = BIAS_STR;
		else if (!artifact_bias && one_in_(7))
			artifact_bias = BIAS_WARRIOR;
		break;
	case 3: case 4:
		add_flag(o_ptr->art_flags, TR_INT);
		if (!artifact_bias && !one_in_(13))
			artifact_bias = BIAS_INT;
		else if (!artifact_bias && one_in_(7))
			artifact_bias = BIAS_MAGE;
		break;
	case 5: case 6:
		add_flag(o_ptr->art_flags, TR_WIS);
		if (!artifact_bias && !one_in_(13))
			artifact_bias = BIAS_WIS;
		else if (!artifact_bias && one_in_(7))
			artifact_bias = BIAS_PRIESTLY;
		break;
	case 7: case 8:
		add_flag(o_ptr->art_flags, TR_DEX);
		if (!artifact_bias && !one_in_(13))
			artifact_bias = BIAS_DEX;
		else if (!artifact_bias && one_in_(7))
			artifact_bias = BIAS_ROGUE;
		break;
	case 9: case 10:
		add_flag(o_ptr->art_flags, TR_CON);
		if (!artifact_bias && !one_in_(13))
			artifact_bias = BIAS_CON;
		else if (!artifact_bias && one_in_(9))
			artifact_bias = BIAS_RANGER;
		break;
	case 11: case 12:
		add_flag(o_ptr->art_flags, TR_CHR);
		if (!artifact_bias && !one_in_(13))
			artifact_bias = BIAS_CHR;
		break;
	case 13: case 14:
		add_flag(o_ptr->art_flags, TR_STEALTH);
		if (!artifact_bias && one_in_(3))
			artifact_bias = BIAS_ROGUE;
		break;
	case 15: case 16:
		add_flag(o_ptr->art_flags, TR_SEARCH);
		if (!artifact_bias && one_in_(9))
			artifact_bias = BIAS_RANGER;
		break;
	case 17: case 18:
		add_flag(o_ptr->art_flags, TR_INFRA);
		break;
	case 19:
		add_flag(o_ptr->art_flags, TR_SPEED);
		if (!artifact_bias && one_in_(11))
			artifact_bias = BIAS_ROGUE;
		break;
	case 20: case 21:
		add_flag(o_ptr->art_flags, TR_TUNNEL);
		break;
	case 22: case 23:
		if (o_ptr->tval == TV_BOW) random_plus(o_ptr);
		else
		{
			add_flag(o_ptr->art_flags, TR_BLOWS);
			if (!artifact_bias && one_in_(11))
				artifact_bias = BIAS_WARRIOR;
		}
		break;
	}
}


static void random_resistance(object_type * o_ptr)
{
	switch (artifact_bias)
	{
	case BIAS_ACID:
		if (!(have_flag(o_ptr->art_flags, TR_RES_ACID)))
		{
			add_flag(o_ptr->art_flags, TR_RES_ACID);
			if (one_in_(2)) return;
		}
		if (one_in_(BIAS_LUCK) && !(have_flag(o_ptr->art_flags, TR_IM_ACID)))
		{
			add_flag(o_ptr->art_flags, TR_IM_ACID);
			if (!one_in_(IM_LUCK))
			{
				remove_flag(o_ptr->art_flags, TR_IM_ELEC);
				remove_flag(o_ptr->art_flags, TR_IM_COLD);
				remove_flag(o_ptr->art_flags, TR_IM_FIRE);
			}
			if (one_in_(2)) return;
		}
		break;

	case BIAS_ELEC:
		if (!(have_flag(o_ptr->art_flags, TR_RES_ELEC)))
		{
			add_flag(o_ptr->art_flags, TR_RES_ELEC);
			if (one_in_(2)) return;
		}
		if ((o_ptr->tval >= TV_CLOAK) && (o_ptr->tval <= TV_HARD_ARMOR) &&
		    !(have_flag(o_ptr->art_flags, TR_SH_ELEC)))
		{
			add_flag(o_ptr->art_flags, TR_SH_ELEC);
			if (one_in_(2)) return;
		}
		if (one_in_(BIAS_LUCK) && !(have_flag(o_ptr->art_flags, TR_IM_ELEC)))
		{
			add_flag(o_ptr->art_flags, TR_IM_ELEC);
			if (!one_in_(IM_LUCK))
			{
				remove_flag(o_ptr->art_flags, TR_IM_ACID);
				remove_flag(o_ptr->art_flags, TR_IM_COLD);
				remove_flag(o_ptr->art_flags, TR_IM_FIRE);
			}

			if (one_in_(2)) return;
		}
		break;

	case BIAS_FIRE:
		if (!(have_flag(o_ptr->art_flags, TR_RES_FIRE)))
		{
			add_flag(o_ptr->art_flags, TR_RES_FIRE);
			if (one_in_(2)) return;
		}
		if ((o_ptr->tval >= TV_CLOAK) &&
		    (o_ptr->tval <= TV_HARD_ARMOR) &&
		    !(have_flag(o_ptr->art_flags, TR_SH_FIRE)))
		{
			add_flag(o_ptr->art_flags, TR_SH_FIRE);
			if (one_in_(2)) return;
		}
		if (one_in_(BIAS_LUCK) &&
		    !(have_flag(o_ptr->art_flags, TR_IM_FIRE)))
		{
			add_flag(o_ptr->art_flags, TR_IM_FIRE);
			if (!one_in_(IM_LUCK))
			{
				remove_flag(o_ptr->art_flags, TR_IM_ELEC);
				remove_flag(o_ptr->art_flags, TR_IM_COLD);
				remove_flag(o_ptr->art_flags, TR_IM_ACID);
			}
			if (one_in_(2)) return;
		}
		break;

	case BIAS_COLD:
		if (!(have_flag(o_ptr->art_flags, TR_RES_COLD)))
		{
			add_flag(o_ptr->art_flags, TR_RES_COLD);
			if (one_in_(2)) return;
		}
		if ((o_ptr->tval >= TV_CLOAK) &&
		    (o_ptr->tval <= TV_HARD_ARMOR) &&
		    !(have_flag(o_ptr->art_flags, TR_SH_COLD)))
		{
			add_flag(o_ptr->art_flags, TR_SH_COLD);
			if (one_in_(2)) return;
		}
		if (one_in_(BIAS_LUCK) && !(have_flag(o_ptr->art_flags, TR_IM_COLD)))
		{
			add_flag(o_ptr->art_flags, TR_IM_COLD);
			if (!one_in_(IM_LUCK))
			{
				remove_flag(o_ptr->art_flags, TR_IM_ELEC);
				remove_flag(o_ptr->art_flags, TR_IM_ACID);
				remove_flag(o_ptr->art_flags, TR_IM_FIRE);
			}
			if (one_in_(2)) return;
		}
		break;

	case BIAS_POIS:
		if (!(have_flag(o_ptr->art_flags, TR_RES_POIS)))
		{
			add_flag(o_ptr->art_flags, TR_RES_POIS);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_WARRIOR:
		if (!one_in_(3) && (!(have_flag(o_ptr->art_flags, TR_RES_FEAR))))
		{
			add_flag(o_ptr->art_flags, TR_RES_FEAR);
			if (one_in_(2)) return;
		}
		if (one_in_(3) && (!(have_flag(o_ptr->art_flags, TR_NO_MAGIC))))
		{
			add_flag(o_ptr->art_flags, TR_NO_MAGIC);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_NECROMANTIC:
		if (!(have_flag(o_ptr->art_flags, TR_RES_NETHER)))
		{
			add_flag(o_ptr->art_flags, TR_RES_NETHER);
			if (one_in_(2)) return;
		}
		if (!(have_flag(o_ptr->art_flags, TR_RES_POIS)))
		{
			add_flag(o_ptr->art_flags, TR_RES_POIS);
			if (one_in_(2)) return;
		}
		if (!(have_flag(o_ptr->art_flags, TR_RES_DARK)))
		{
			add_flag(o_ptr->art_flags, TR_RES_DARK);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_CHAOS:
		if (!(have_flag(o_ptr->art_flags, TR_RES_CHAOS)))
		{
			add_flag(o_ptr->art_flags, TR_RES_CHAOS);
			if (one_in_(2)) return;
		}
		if (!(have_flag(o_ptr->art_flags, TR_RES_CONF)))
		{
			add_flag(o_ptr->art_flags, TR_RES_CONF);
			if (one_in_(2)) return;
		}
		if (!(have_flag(o_ptr->art_flags, TR_RES_DISEN)))
		{
			add_flag(o_ptr->art_flags, TR_RES_DISEN);
			if (one_in_(2)) return;
		}
		break;
	}

	switch (randint1(42))
	{
		case 1:
			if (!one_in_(WEIRD_LUCK))
				random_resistance(o_ptr);
			else
			{
				add_flag(o_ptr->art_flags, TR_IM_ACID);
				if (!artifact_bias)
					artifact_bias = BIAS_ACID;
			}
			break;
		case 2:
			if (!one_in_(WEIRD_LUCK))
				random_resistance(o_ptr);
			else
			{
				add_flag(o_ptr->art_flags, TR_IM_ELEC);
				if (!artifact_bias)
					artifact_bias = BIAS_ELEC;
			}
			break;
		case 3:
			if (!one_in_(WEIRD_LUCK))
				random_resistance(o_ptr);
			else
			{
				add_flag(o_ptr->art_flags, TR_IM_COLD);
				if (!artifact_bias)
					artifact_bias = BIAS_COLD;
			}
			break;
		case 4:
			if (!one_in_(WEIRD_LUCK))
				random_resistance(o_ptr);
			else
			{
				add_flag(o_ptr->art_flags, TR_IM_FIRE);
				if (!artifact_bias)
					artifact_bias = BIAS_FIRE;
			}
			break;
		case 5:
		case 6:
		case 13:
			add_flag(o_ptr->art_flags, TR_RES_ACID);
			if (!artifact_bias)
				artifact_bias = BIAS_ACID;
			break;
		case 7:
		case 8:
		case 14:
			add_flag(o_ptr->art_flags, TR_RES_ELEC);
			if (!artifact_bias)
				artifact_bias = BIAS_ELEC;
			break;
		case 9:
		case 10:
		case 15:
			add_flag(o_ptr->art_flags, TR_RES_FIRE);
			if (!artifact_bias)
				artifact_bias = BIAS_FIRE;
			break;
		case 11:
		case 12:
		case 16:
			add_flag(o_ptr->art_flags, TR_RES_COLD);
			if (!artifact_bias)
				artifact_bias = BIAS_COLD;
			break;
		case 17:
		case 18:
			add_flag(o_ptr->art_flags, TR_RES_POIS);
			if (!artifact_bias && !one_in_(4))
				artifact_bias = BIAS_POIS;
			else if (!artifact_bias && one_in_(2))
				artifact_bias = BIAS_NECROMANTIC;
			else if (!artifact_bias && one_in_(2))
				artifact_bias = BIAS_ROGUE;
			break;
		case 19:
		case 20:
			add_flag(o_ptr->art_flags, TR_RES_FEAR);
			if (!artifact_bias && one_in_(3))
				artifact_bias = BIAS_WARRIOR;
			break;
		case 21:
			add_flag(o_ptr->art_flags, TR_RES_LITE);
			break;
		case 22:
			add_flag(o_ptr->art_flags, TR_RES_DARK);
			break;
		case 23:
		case 24:
			add_flag(o_ptr->art_flags, TR_RES_BLIND);
			break;
		case 25:
		case 26:
			add_flag(o_ptr->art_flags, TR_RES_CONF);
			if (!artifact_bias && one_in_(6))
				artifact_bias = BIAS_CHAOS;
			break;
		case 27:
		case 28:
			add_flag(o_ptr->art_flags, TR_RES_SOUND);
			break;
		case 29:
		case 30:
			add_flag(o_ptr->art_flags, TR_RES_SHARDS);
			break;
		case 31:
		case 32:
			add_flag(o_ptr->art_flags, TR_RES_NETHER);
			if (!artifact_bias && one_in_(3))
				artifact_bias = BIAS_NECROMANTIC;
			break;
		case 33:
		case 34:
			add_flag(o_ptr->art_flags, TR_RES_NEXUS);
			break;
		case 35:
		case 36:
			add_flag(o_ptr->art_flags, TR_RES_CHAOS);
			if (!artifact_bias && one_in_(2))
				artifact_bias = BIAS_CHAOS;
			break;
		case 37:
		case 38:
			add_flag(o_ptr->art_flags, TR_RES_DISEN);
			break;
		case 39:
			if (o_ptr->tval >= TV_CLOAK && o_ptr->tval <= TV_HARD_ARMOR)
				add_flag(o_ptr->art_flags, TR_SH_ELEC);
			else
				random_resistance(o_ptr);
			if (!artifact_bias)
				artifact_bias = BIAS_ELEC;
			break;
		case 40:
			if (o_ptr->tval >= TV_CLOAK && o_ptr->tval <= TV_HARD_ARMOR)
				add_flag(o_ptr->art_flags, TR_SH_FIRE);
			else
				random_resistance(o_ptr);
			if (!artifact_bias)
				artifact_bias = BIAS_FIRE;
			break;
		case 41:
			if (o_ptr->tval == TV_SHIELD || o_ptr->tval == TV_CLOAK ||
			    o_ptr->tval == TV_HELM || o_ptr->tval == TV_HARD_ARMOR)
				add_flag(o_ptr->art_flags, TR_REFLECT);
			else
				random_resistance(o_ptr);
			break;
		case 42:
			if (o_ptr->tval >= TV_CLOAK && o_ptr->tval <= TV_HARD_ARMOR)
				add_flag(o_ptr->art_flags, TR_SH_COLD);
			else
				random_resistance(o_ptr);
			if (!artifact_bias)
				artifact_bias = BIAS_COLD;
			break;
	}
}



static void random_misc(object_type * o_ptr)
{
	switch (artifact_bias)
	{
	case BIAS_RANGER:
		if (!(have_flag(o_ptr->art_flags, TR_SUST_CON)))
		{
			add_flag(o_ptr->art_flags, TR_SUST_CON);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_STR:
		if (!(have_flag(o_ptr->art_flags, TR_SUST_STR)))
		{
			add_flag(o_ptr->art_flags, TR_SUST_STR);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_WIS:
		if (!(have_flag(o_ptr->art_flags, TR_SUST_WIS)))
		{
			add_flag(o_ptr->art_flags, TR_SUST_WIS);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_INT:
		if (!(have_flag(o_ptr->art_flags, TR_SUST_INT)))
		{
			add_flag(o_ptr->art_flags, TR_SUST_INT);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_DEX:
		if (!(have_flag(o_ptr->art_flags, TR_SUST_DEX)))
		{
			add_flag(o_ptr->art_flags, TR_SUST_DEX);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_CON:
		if (!(have_flag(o_ptr->art_flags, TR_SUST_CON)))
		{
			add_flag(o_ptr->art_flags, TR_SUST_CON);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_CHR:
		if (!(have_flag(o_ptr->art_flags, TR_SUST_CHR)))
		{
			add_flag(o_ptr->art_flags, TR_SUST_CHR);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_CHAOS:
		if (!(have_flag(o_ptr->art_flags, TR_TELEPORT)))
		{
			add_flag(o_ptr->art_flags, TR_TELEPORT);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_FIRE:
		if (!(have_flag(o_ptr->art_flags, TR_LITE)))
		{
			add_flag(o_ptr->art_flags, TR_LITE); /* Freebie */
		}
		break;
	}

	switch (randint1(33))
	{
		case 1:
			add_flag(o_ptr->art_flags, TR_SUST_STR);
			if (!artifact_bias)
				artifact_bias = BIAS_STR;
			break;
		case 2:
			add_flag(o_ptr->art_flags, TR_SUST_INT);
			if (!artifact_bias)
				artifact_bias = BIAS_INT;
			break;
		case 3:
			add_flag(o_ptr->art_flags, TR_SUST_WIS);
			if (!artifact_bias)
				artifact_bias = BIAS_WIS;
			break;
		case 4:
			add_flag(o_ptr->art_flags, TR_SUST_DEX);
			if (!artifact_bias)
				artifact_bias = BIAS_DEX;
			break;
		case 5:
			add_flag(o_ptr->art_flags, TR_SUST_CON);
			if (!artifact_bias)
				artifact_bias = BIAS_CON;
			break;
		case 6:
			add_flag(o_ptr->art_flags, TR_SUST_CHR);
			if (!artifact_bias)
				artifact_bias = BIAS_CHR;
			break;
		case 7:
		case 8:
		case 14:
			add_flag(o_ptr->art_flags, TR_FREE_ACT);
			break;
		case 9:
			add_flag(o_ptr->art_flags, TR_HOLD_LIFE);
			if (!artifact_bias && one_in_(5))
				artifact_bias = BIAS_PRIESTLY;
			else if (!artifact_bias && one_in_(6))
				artifact_bias = BIAS_NECROMANTIC;
			break;
		case 10:
		case 11:
			add_flag(o_ptr->art_flags, TR_LITE);
			break;
		case 12:
		case 13:
			add_flag(o_ptr->art_flags, TR_LEVITATION);
			break;
		case 15:
		case 16:
		case 17:
			add_flag(o_ptr->art_flags, TR_SEE_INVIS);
			break;
		case 19:
		case 20:
			add_flag(o_ptr->art_flags, TR_SLOW_DIGEST);
			break;
		case 21:
		case 22:
			add_flag(o_ptr->art_flags, TR_REGEN);
			break;
		case 23:
			add_flag(o_ptr->art_flags, TR_TELEPORT);
			break;
		case 24:
		case 25:
		case 26:
			if (object_is_armour(o_ptr))
				random_misc(o_ptr);
			else
			{
				o_ptr->to_a = 4 + randint1(11);
			}
			break;
		case 27:
		case 28:
		case 29:
		{
			int bonus_h, bonus_d;
			add_flag(o_ptr->art_flags, TR_SHOW_MODS);
			bonus_h = 4 + (randint1(11));
			bonus_d = 4 + (randint1(11));
			if ((o_ptr->tval != TV_SWORD) && (o_ptr->tval != TV_POLEARM) && (o_ptr->tval != TV_HAFTED) && (o_ptr->tval != TV_DIGGING) && (o_ptr->tval != TV_GLOVES) && (o_ptr->tval != TV_RING))
			{
				bonus_h /= 2;
				bonus_d /= 2;
			}
			o_ptr->to_h += bonus_h;
			o_ptr->to_d += bonus_d;
			break;
		}
		case 30:
			add_flag(o_ptr->art_flags, TR_NO_MAGIC);
			break;
		case 31:
			add_flag(o_ptr->art_flags, TR_NO_TELE);
			break;
		case 32:
			add_flag(o_ptr->art_flags, TR_WARNING);
			break;

		case 18:
			switch (randint1(3))
			{
			case 1:
				add_flag(o_ptr->art_flags, TR_ESP_EVIL);
				if (!artifact_bias && one_in_(3))
					artifact_bias = BIAS_LAW;
				break;
			case 2:
				add_flag(o_ptr->art_flags, TR_ESP_NONLIVING);
				if (!artifact_bias && one_in_(3))
					artifact_bias = BIAS_MAGE;
				break;
			case 3:
				add_flag(o_ptr->art_flags, TR_TELEPATHY);
				if (!artifact_bias && one_in_(9))
					artifact_bias = BIAS_MAGE;
				break;
			}
			break;

		case 33:
		{
			int idx[3];
			int n = randint1(3);

			idx[0] = randint1(8);

			idx[1] = randint1(7);
			if (idx[1] >= idx[0]) idx[1]++;

			idx[2] = randint1(6);
			if (idx[2] >= idx[0]) idx[2]++;
			if (idx[2] >= idx[1]) idx[2]++;

			while (n--) switch (idx[n])
			{
			case 1:
				add_flag(o_ptr->art_flags, TR_ESP_ANIMAL);
				if (!artifact_bias && one_in_(4))
					artifact_bias = BIAS_RANGER;
				break;
			case 2:
				add_flag(o_ptr->art_flags, TR_ESP_UNDEAD);
				if (!artifact_bias && one_in_(3))
					artifact_bias = BIAS_PRIESTLY;
				else if (!artifact_bias && one_in_(6))
					artifact_bias = BIAS_NECROMANTIC;
				break;
			case 3:
				add_flag(o_ptr->art_flags, TR_ESP_DEMON);
				break;
			case 4:
				add_flag(o_ptr->art_flags, TR_ESP_ORC);
				break;
			case 5:
				add_flag(o_ptr->art_flags, TR_ESP_TROLL);
				break;
			case 6:
				add_flag(o_ptr->art_flags, TR_ESP_GIANT);
				break;
			case 7:
				add_flag(o_ptr->art_flags, TR_ESP_HUMAN);
				if (!artifact_bias && one_in_(6))
					artifact_bias = BIAS_ROGUE;
				break;
			case 8:
				add_flag(o_ptr->art_flags, TR_ESP_GOOD);
				if (!artifact_bias && one_in_(3))
					artifact_bias = BIAS_LAW;
				break;
			}
			break;
		}
	}
}


static void random_slay(object_type *o_ptr)
{
	if (o_ptr->tval == TV_BOW)
	{
		switch (randint1(6))
		{
			case 1:
			case 2:
			case 3:
				add_flag(o_ptr->art_flags, TR_XTRA_MIGHT);
				if (!one_in_(7)) remove_flag(o_ptr->art_flags, TR_XTRA_SHOTS);
				if (!artifact_bias && one_in_(9))
					artifact_bias = BIAS_RANGER;
				break;
			default:
				add_flag(o_ptr->art_flags, TR_XTRA_SHOTS);
				if (!one_in_(7)) remove_flag(o_ptr->art_flags, TR_XTRA_MIGHT);
				if (!artifact_bias && one_in_(9))
					artifact_bias = BIAS_RANGER;
			break;
		}

		return;
	}

	switch (artifact_bias)
	{
	case BIAS_CHAOS:
		if (!(have_flag(o_ptr->art_flags, TR_CHAOTIC)))
		{
			add_flag(o_ptr->art_flags, TR_CHAOTIC);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_PRIESTLY:
		if((o_ptr->tval == TV_SWORD || o_ptr->tval == TV_POLEARM) &&
		   !(have_flag(o_ptr->art_flags, TR_BLESSED)))
		{
			/* A free power for "priestly" random artifacts */
			add_flag(o_ptr->art_flags, TR_BLESSED);
		}
		break;

	case BIAS_NECROMANTIC:
		if (!(have_flag(o_ptr->art_flags, TR_VAMPIRIC)))
		{
			add_flag(o_ptr->art_flags, TR_VAMPIRIC);
			if (one_in_(2)) return;
		}
		if (!(have_flag(o_ptr->art_flags, TR_BRAND_POIS)) && one_in_(2))
		{
			add_flag(o_ptr->art_flags, TR_BRAND_POIS);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_RANGER:
		if (!(have_flag(o_ptr->art_flags, TR_SLAY_ANIMAL)))
		{
			add_flag(o_ptr->art_flags, TR_SLAY_ANIMAL);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_ROGUE:
		if ((((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_DAGGER)) ||
		     ((o_ptr->tval == TV_POLEARM) && (o_ptr->sval == SV_SPEAR))) &&
			 !(have_flag(o_ptr->art_flags, TR_THROW)))
		{
			/* Free power for rogues... */
			add_flag(o_ptr->art_flags, TR_THROW);
		}
		if (!(have_flag(o_ptr->art_flags, TR_BRAND_POIS)))
		{
			add_flag(o_ptr->art_flags, TR_BRAND_POIS);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_POIS:
		if (!(have_flag(o_ptr->art_flags, TR_BRAND_POIS)))
		{
			add_flag(o_ptr->art_flags, TR_BRAND_POIS);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_FIRE:
		if (!(have_flag(o_ptr->art_flags, TR_BRAND_FIRE)))
		{
			add_flag(o_ptr->art_flags, TR_BRAND_FIRE);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_COLD:
		if (!(have_flag(o_ptr->art_flags, TR_BRAND_COLD)))
		{
			add_flag(o_ptr->art_flags, TR_BRAND_COLD);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_ELEC:
		if (!(have_flag(o_ptr->art_flags, TR_BRAND_ELEC)))
		{
			add_flag(o_ptr->art_flags, TR_BRAND_ELEC);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_ACID:
		if (!(have_flag(o_ptr->art_flags, TR_BRAND_ACID)))
		{
			add_flag(o_ptr->art_flags, TR_BRAND_ACID);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_LAW:
		if (!(have_flag(o_ptr->art_flags, TR_SLAY_EVIL)))
		{
			add_flag(o_ptr->art_flags, TR_SLAY_EVIL);
			if (one_in_(2)) return;
		}
		if (!(have_flag(o_ptr->art_flags, TR_SLAY_UNDEAD)))
		{
			add_flag(o_ptr->art_flags, TR_SLAY_UNDEAD);
			if (one_in_(2)) return;
		}
		if (!(have_flag(o_ptr->art_flags, TR_SLAY_DEMON)))
		{
			add_flag(o_ptr->art_flags, TR_SLAY_DEMON);
			if (one_in_(2)) return;
		}
		break;
	}

	switch (randint1(36))
	{
		case 1:
		case 2:
			add_flag(o_ptr->art_flags, TR_SLAY_ANIMAL);
			break;
		case 3:
		case 4:
			add_flag(o_ptr->art_flags, TR_SLAY_EVIL);
			if (!artifact_bias && one_in_(2))
				artifact_bias = BIAS_LAW;
			else if (!artifact_bias && one_in_(9))
				artifact_bias = BIAS_PRIESTLY;
			break;
		case 5:
		case 6:
			add_flag(o_ptr->art_flags, TR_SLAY_UNDEAD);
			if (!artifact_bias && one_in_(9))
				artifact_bias = BIAS_PRIESTLY;
			break;
		case 7:
		case 8:
			add_flag(o_ptr->art_flags, TR_SLAY_DEMON);
			if (!artifact_bias && one_in_(9))
				artifact_bias = BIAS_PRIESTLY;
			break;
		case 9:
		case 10:
			add_flag(o_ptr->art_flags, TR_SLAY_ORC);
			break;
		case 11:
		case 12:
			add_flag(o_ptr->art_flags, TR_SLAY_TROLL);
			break;
		case 13:
		case 14:
			add_flag(o_ptr->art_flags, TR_SLAY_GIANT);
			break;
		case 15:
		case 16:
			add_flag(o_ptr->art_flags, TR_SLAY_DRAGON);
			break;
		case 17:
			add_flag(o_ptr->art_flags, TR_KILL_DRAGON);
			break;
		case 18:
		case 19:
			if (o_ptr->tval == TV_SWORD)
			{
				add_flag(o_ptr->art_flags, TR_VORPAL);
				if (!artifact_bias && one_in_(9))
					artifact_bias = BIAS_WARRIOR;
			}
			else
				random_slay(o_ptr);
			break;
		case 20:
			add_flag(o_ptr->art_flags, TR_IMPACT);
			break;
		case 21:
		case 22:
			add_flag(o_ptr->art_flags, TR_BRAND_FIRE);
			if (!artifact_bias)
				artifact_bias = BIAS_FIRE;
			break;
		case 23:
		case 24:
			add_flag(o_ptr->art_flags, TR_BRAND_COLD);
			if (!artifact_bias)
				artifact_bias = BIAS_COLD;
			break;
		case 25:
		case 26:
			add_flag(o_ptr->art_flags, TR_BRAND_ELEC);
			if (!artifact_bias)
				artifact_bias = BIAS_ELEC;
			break;
		case 27:
		case 28:
			add_flag(o_ptr->art_flags, TR_BRAND_ACID);
			if (!artifact_bias)
				artifact_bias = BIAS_ACID;
			break;
		case 29:
		case 30:
			add_flag(o_ptr->art_flags, TR_BRAND_POIS);
			if (!artifact_bias && !one_in_(3))
				artifact_bias = BIAS_POIS;
			else if (!artifact_bias && one_in_(6))
				artifact_bias = BIAS_NECROMANTIC;
			else if (!artifact_bias)
				artifact_bias = BIAS_ROGUE;
			break;
		case 31:
			add_flag(o_ptr->art_flags, TR_VAMPIRIC);
			if (!artifact_bias)
				artifact_bias = BIAS_NECROMANTIC;
			break;
		case 32:
			add_flag(o_ptr->art_flags, TR_FORCE_WEAPON);
			if (!artifact_bias)
				artifact_bias = (one_in_(2) ? BIAS_MAGE : BIAS_PRIESTLY);
			break;
		case 33:
		case 34:
			add_flag(o_ptr->art_flags, TR_SLAY_HUMAN);
			break;
		default:
			add_flag(o_ptr->art_flags, TR_CHAOTIC);
			if (!artifact_bias)
				artifact_bias = BIAS_CHAOS;
			break;
	}
}


static void give_activation_power(object_type *o_ptr)
{
	int type = 0, chance = 0;

	switch (artifact_bias)
	{
		case BIAS_ELEC:
			if (!one_in_(3))
			{
				type = ACT_BO_ELEC_1;
			}
			else if (!one_in_(5))
			{
				type = ACT_BA_ELEC_2;
			}
			else
			{
				type = ACT_BA_ELEC_3;
			}
			chance = 101;
			break;

		case BIAS_POIS:
			type = ACT_BA_POIS_1;
			chance = 101;
			break;

		case BIAS_FIRE:
			if (!one_in_(3))
			{
				type = ACT_BO_FIRE_1;
			}
			else if (!one_in_(5))
			{
				type = ACT_BA_FIRE_1;
			}
			else
			{
				type = ACT_BA_FIRE_2;
			}
			chance = 101;
			break;

		case BIAS_COLD:
			chance = 101;
			if (!one_in_(3))
				type = ACT_BO_COLD_1;
			else if (!one_in_(3))
				type = ACT_BA_COLD_1;
			else if (!one_in_(3))
				type = ACT_BA_COLD_2;
			else
				type = ACT_BA_COLD_3;
			break;

		case BIAS_CHAOS:
			chance = 50;
			if (one_in_(6))
				type = ACT_SUMMON_DEMON;
			else
				type = ACT_CALL_CHAOS;
			break;

		case BIAS_PRIESTLY:
			chance = 101;

			if (one_in_(13))
				type = ACT_CHARM_UNDEAD;
			else if (one_in_(12))
				type = ACT_BANISH_EVIL;
			else if (one_in_(11))
				type = ACT_DISP_EVIL;
			else if (one_in_(10))
				type = ACT_PROT_EVIL;
			else if (one_in_(9))
				type = ACT_CURE_1000;
			else if (one_in_(8))
				type = ACT_CURE_700;
			else if (one_in_(7))
				type = ACT_REST_ALL;
			else if (one_in_(6))
				type = ACT_REST_LIFE;
			else
				type = ACT_CURE_MW;
			break;

		case BIAS_NECROMANTIC:
			chance = 101;
			if (one_in_(66))
				type = ACT_WRAITH;
			else if (one_in_(13))
				type = ACT_DISP_GOOD;
			else if (one_in_(9))
				type = ACT_MASS_GENO;
			else if (one_in_(8))
				type = ACT_GENOCIDE;
			else if (one_in_(13))
				type = ACT_SUMMON_UNDEAD;
			else if (one_in_(9))
				type = ACT_VAMPIRE_2;
			else if (one_in_(6))
				type = ACT_CHARM_UNDEAD;
			else
				type = ACT_VAMPIRE_1;
			break;

		case BIAS_LAW:
			chance = 101;
			if (one_in_(8))
				type = ACT_BANISH_EVIL;
			else if (one_in_(4))
				type = ACT_DISP_EVIL;
			else
				type = ACT_PROT_EVIL;
			break;

		case BIAS_ROGUE:
			chance = 101;
			if (one_in_(50))
				type = ACT_SPEED;
			else if (one_in_(4))
				type = ACT_SLEEP;
			else if (one_in_(3))
				type = ACT_DETECT_ALL;
			else if (one_in_(8))
				type = ACT_ID_FULL;
			else
				type = ACT_ID_PLAIN;
			break;

		case BIAS_MAGE:
			chance = 66;
			if (one_in_(20))
				type = ACT_SUMMON_ELEMENTAL;
			else if (one_in_(10))
				type = ACT_SUMMON_PHANTOM;
			else if (one_in_(5))
				type = ACT_RUNE_EXPLO;
			else
				type = ACT_ESP;
			break;

		case BIAS_WARRIOR:
			chance = 80;
			if (one_in_(100))
				type = ACT_INVULN;
			else
				type = ACT_BERSERK;
			break;

		case BIAS_RANGER:
			chance = 101;
			if (one_in_(20))
				type = ACT_CHARM_ANIMALS;
			else if (one_in_(7))
				type = ACT_SUMMON_ANIMAL;
			else if (one_in_(6))
				type = ACT_CHARM_ANIMAL;
			else if (one_in_(4))
				type = ACT_RESIST_ALL;
			else if (one_in_(3))
				type = ACT_SATIATE;
			else
				type = ACT_CURE_POISON;
			break;
	}

	while (!type || (randint1(100) >= chance))
	{
		type = randint1(255);
		switch (type)
		{
			case ACT_SUNLIGHT:
			case ACT_BO_MISS_1:
			case ACT_BA_POIS_1:
			case ACT_BO_ELEC_1:
			case ACT_BO_ACID_1:
			case ACT_BO_COLD_1:
			case ACT_BO_FIRE_1:
			case ACT_CONFUSE:
			case ACT_SLEEP:
			case ACT_QUAKE:
			case ACT_CURE_LW:
			case ACT_CURE_MW:
			case ACT_CURE_POISON:
			case ACT_BERSERK:
			case ACT_LIGHT:
			case ACT_MAP_LIGHT:
			case ACT_DEST_DOOR:
			case ACT_STONE_MUD:
			case ACT_TELEPORT:
				chance = 101;
				break;
			case ACT_BA_COLD_1:
			case ACT_BA_FIRE_1:
			case ACT_DRAIN_1:
			case ACT_TELE_AWAY:
			case ACT_ESP:
			case ACT_RESIST_ALL:
			case ACT_DETECT_ALL:
			case ACT_RECALL:
			case ACT_SATIATE:
			case ACT_RECHARGE:
				chance = 85;
				break;
			case ACT_TERROR:
			case ACT_PROT_EVIL:
			case ACT_ID_PLAIN:
				chance = 75;
				break;
			case ACT_DRAIN_2:
			case ACT_VAMPIRE_1:
			case ACT_BO_MISS_2:
			case ACT_BA_FIRE_2:
			case ACT_REST_LIFE:
				chance = 66;
				break;
			case ACT_BA_COLD_3:
			case ACT_BA_ELEC_3:
			case ACT_WHIRLWIND:
			case ACT_VAMPIRE_2:
			case ACT_CHARM_ANIMAL:
				chance = 50;
				break;
			case ACT_SUMMON_ANIMAL:
				chance = 40;
				break;
			case ACT_DISP_EVIL:
			case ACT_BA_MISS_3:
			case ACT_DISP_GOOD:
			case ACT_BANISH_EVIL:
			case ACT_GENOCIDE:
			case ACT_MASS_GENO:
			case ACT_CHARM_UNDEAD:
			case ACT_CHARM_OTHER:
			case ACT_SUMMON_PHANTOM:
			case ACT_REST_ALL:
			case ACT_RUNE_EXPLO:
				chance = 33;
				break;
			case ACT_CALL_CHAOS:
			case ACT_ROCKET:
			case ACT_CHARM_ANIMALS:
			case ACT_CHARM_OTHERS:
			case ACT_SUMMON_ELEMENTAL:
			case ACT_CURE_700:
			case ACT_SPEED:
			case ACT_ID_FULL:
			case ACT_RUNE_PROT:
				chance = 25;
				break;
			case ACT_CURE_1000:
			case ACT_XTRA_SPEED:
			case ACT_DETECT_XTRA:
			case ACT_DIM_DOOR:
				chance = 10;
				break;
			case ACT_SUMMON_UNDEAD:
			case ACT_SUMMON_DEMON:
			case ACT_WRAITH:
			case ACT_INVULN:
			case ACT_ALCHEMY:
				chance = 5;
				break;
			default:
				chance = 0;
		}
	}

	/* A type was chosen... */
	o_ptr->xtra2 = type;
	add_flag(o_ptr->art_flags, TR_ACTIVATE);
	o_ptr->timeout = 0;
}


static void get_random_name(char *return_name, bool armour, int power)
{
	int prob = randint1(100);

	if (prob <= SINDARIN_NAME)
	{
		get_table_sindarin(return_name);
	}
	else if (prob <= TABLE_NAME)
	{
		get_table_name(return_name);
	}
	else
	{
		cptr filename;

		switch (armour)
		{
			case 1:
				switch (power)
				{
					case 0:
#ifdef JP
						filename = "a_cursed_j.txt";
#else
						filename = "a_cursed.txt";
#endif
						break;
					case 1:
#ifdef JP
						filename = "a_low_j.txt";
#else
						filename = "a_low.txt";
#endif
						break;
					case 2:
#ifdef JP
						filename = "a_med_j.txt";
#else
						filename = "a_med.txt";
#endif
						break;
					default:
#ifdef JP
						filename = "a_high_j.txt";
#else
						filename = "a_high.txt";
#endif
				}
				break;
			default:
				switch (power)
				{
					case 0:
#ifdef JP
						filename = "w_cursed_j.txt";
#else
						filename = "w_cursed.txt";
#endif
						break;
					case 1:
#ifdef JP
						filename = "w_low_j.txt";
#else
						filename = "w_low.txt";
#endif
						break;
					case 2:
#ifdef JP
						filename = "w_med_j.txt";
#else
						filename = "w_med.txt";
#endif
						break;
					default:
#ifdef JP
						filename = "w_high_j.txt";
#else
						filename = "w_high.txt";
#endif
				}
		}

		(void)get_rnd_line(filename, artifact_bias, return_name);
#ifdef JP
		 if (return_name[0] == 0) get_table_name(return_name);
#endif
	}
}


bool create_artifact(object_type *o_ptr, bool a_scroll)
{
	char    new_name[1024];
	int     has_pval = 0;
	int     powers = randint1(5) + 1;
	int     max_type = (object_is_weapon_ammo(o_ptr) ? 7 : 5);
	int     power_level;
	s32b    total_flags;
	bool    a_cursed = FALSE;
	int     warrior_artifact_bias = 0;
	int i;

	/* Reset artifact bias */
	artifact_bias = 0;

	/* Nuke enchantments */
	o_ptr->name1 = 0;
	o_ptr->name2 = 0;

	for (i = 0; i < TR_FLAG_SIZE; i++)
		o_ptr->art_flags[i] |= k_info[o_ptr->k_idx].flags[i];

	if (o_ptr->pval) has_pval = TRUE;

	if (a_scroll && one_in_(4))
	{
		switch (p_ptr->pclass)
		{
			case CLASS_WARRIOR:
			case CLASS_BERSERKER:
			case CLASS_ARCHER:
			case CLASS_SAMURAI:
			case CLASS_CAVALRY:
			case CLASS_SMITH:
				artifact_bias = BIAS_WARRIOR;
				break;
			case CLASS_MAGE:
			case CLASS_HIGH_MAGE:
			case CLASS_SORCERER:
			case CLASS_MAGIC_EATER:
			case CLASS_BLUE_MAGE:
				artifact_bias = BIAS_MAGE;
				break;
			case CLASS_PRIEST:
				artifact_bias = BIAS_PRIESTLY;
				break;
			case CLASS_ROGUE:
			case CLASS_NINJA:
				artifact_bias = BIAS_ROGUE;
				warrior_artifact_bias = 25;
				break;
			case CLASS_RANGER:
			case CLASS_SNIPER:
				artifact_bias = BIAS_RANGER;
				warrior_artifact_bias = 30;
				break;
			case CLASS_PALADIN:
				artifact_bias = BIAS_PRIESTLY;
				warrior_artifact_bias = 40;
				break;
			case CLASS_WARRIOR_MAGE:
			case CLASS_RED_MAGE:
				artifact_bias = BIAS_MAGE;
				warrior_artifact_bias = 40;
				break;
			case CLASS_CHAOS_WARRIOR:
				artifact_bias = BIAS_CHAOS;
				warrior_artifact_bias = 40;
				break;
			case CLASS_MONK:
			case CLASS_FORCETRAINER:
				artifact_bias = BIAS_PRIESTLY;
				break;
			case CLASS_MINDCRAFTER:
			case CLASS_BARD:
				if (randint1(5) > 2) artifact_bias = BIAS_PRIESTLY;
				break;
			case CLASS_TOURIST:
				if (randint1(5) > 2) artifact_bias = BIAS_WARRIOR;
				break;
			case CLASS_IMITATOR:
				if (randint1(2) > 1) artifact_bias = BIAS_RANGER;
				break;
			case CLASS_BEASTMASTER:
				artifact_bias = BIAS_CHR;
				warrior_artifact_bias = 50;
				break;
			case CLASS_MIRROR_MASTER:
				if (randint1(4) > 1) 
				{
				    artifact_bias = BIAS_MAGE;
				}
				else
				{
				    artifact_bias = BIAS_ROGUE;
				}
				break;
		}
	}

	if (a_scroll && (randint1(100) <= warrior_artifact_bias))
		artifact_bias = BIAS_WARRIOR;

	strcpy(new_name, "");

	if (!a_scroll && one_in_(A_CURSED))
		a_cursed = TRUE;
	if (((o_ptr->tval == TV_AMULET) || (o_ptr->tval == TV_RING)) && object_is_cursed(o_ptr))
		a_cursed = TRUE;

	while (one_in_(powers) || one_in_(7) || one_in_(10))
		powers++;

	if (!a_cursed && one_in_(WEIRD_LUCK))
		powers *= 2;

	if (a_cursed) powers /= 2;

	/* Main loop */
	while (powers--)
	{
		switch (randint1(max_type))
		{
			case 1: case 2:
				random_plus(o_ptr);
				has_pval = TRUE;
				break;
			case 3: case 4:
				if (one_in_(2) && object_is_weapon_ammo(o_ptr) && (o_ptr->tval != TV_BOW))
				{
					if (a_cursed && !one_in_(13)) break;
					if (one_in_(13))
					{
						if (one_in_(o_ptr->ds+4)) o_ptr->ds++;
					}
					else
					{
						if (one_in_(o_ptr->dd+1)) o_ptr->dd++;
					}
				}
				else
					random_resistance(o_ptr);
				break;
			case 5:
				random_misc(o_ptr);
				break;
			case 6: case 7:
				random_slay(o_ptr);
				break;
			default:
				if (p_ptr->wizard) msg_print("Switch error in create_artifact!");
				powers++;
		}
	};

	if (has_pval)
	{
#if 0
		add_flag(o_ptr->art_flags, TR_SHOW_MODS);

		/* This one commented out by gw's request... */
		if (!a_scroll)
			add_flag(o_ptr->art_flags, TR_HIDE_TYPE);
#endif

		if (have_flag(o_ptr->art_flags, TR_BLOWS))
		{
			o_ptr->pval = randint1(2);
			if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_HAYABUSA))
				o_ptr->pval++;
		}
		else
		{
			do
			{
				o_ptr->pval++;
			}
			while (o_ptr->pval < randint1(5) || one_in_(o_ptr->pval));
		}

		if ((o_ptr->pval > 4) && !one_in_(WEIRD_LUCK))
			o_ptr->pval = 4;
	}

	/* give it some plusses... */
	if (object_is_armour(o_ptr))
		o_ptr->to_a += randint1(o_ptr->to_a > 19 ? 1 : 20 - o_ptr->to_a);
	else if (object_is_weapon_ammo(o_ptr))
	{
		o_ptr->to_h += randint1(o_ptr->to_h > 19 ? 1 : 20 - o_ptr->to_h);
		o_ptr->to_d += randint1(o_ptr->to_d > 19 ? 1 : 20 - o_ptr->to_d);
		if ((have_flag(o_ptr->art_flags, TR_WIS)) && (o_ptr->pval > 0)) add_flag(o_ptr->art_flags, TR_BLESSED);
	}

	/* Just to be sure */
	add_flag(o_ptr->art_flags, TR_IGNORE_ACID);
	add_flag(o_ptr->art_flags, TR_IGNORE_ELEC);
	add_flag(o_ptr->art_flags, TR_IGNORE_FIRE);
	add_flag(o_ptr->art_flags, TR_IGNORE_COLD);

	total_flags = flag_cost(o_ptr, o_ptr->pval);
	if (cheat_peek) msg_format("%ld", total_flags);

	if (a_cursed) curse_artifact(o_ptr);

	if (!a_cursed &&
	    one_in_(object_is_armour(o_ptr) ? ACTIVATION_CHANCE * 2 : ACTIVATION_CHANCE))
	{
		o_ptr->xtra2 = 0;
		give_activation_power(o_ptr);
	}

	if (object_is_armour(o_ptr))
	{
		while ((o_ptr->to_d+o_ptr->to_h) > 20)
		{
			if (one_in_(o_ptr->to_d) && one_in_(o_ptr->to_h)) break;
			o_ptr->to_d -= (s16b)randint0(3);
			o_ptr->to_h -= (s16b)randint0(3);
		}
		while ((o_ptr->to_d+o_ptr->to_h) > 10)
		{
			if (one_in_(o_ptr->to_d) || one_in_(o_ptr->to_h)) break;
			o_ptr->to_d -= (s16b)randint0(3);
			o_ptr->to_h -= (s16b)randint0(3);
		}
	}

	if (((artifact_bias == BIAS_MAGE) || (artifact_bias == BIAS_INT)) && (o_ptr->tval == TV_GLOVES)) add_flag(o_ptr->art_flags, TR_FREE_ACT);

	if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_DOKUBARI))
	{
		o_ptr->to_h = 0;
		o_ptr->to_d = 0;
		remove_flag(o_ptr->art_flags, TR_BLOWS);
		remove_flag(o_ptr->art_flags, TR_FORCE_WEAPON);
		remove_flag(o_ptr->art_flags, TR_SLAY_ANIMAL);
		remove_flag(o_ptr->art_flags, TR_SLAY_EVIL);
		remove_flag(o_ptr->art_flags, TR_SLAY_UNDEAD);
		remove_flag(o_ptr->art_flags, TR_SLAY_DEMON);
		remove_flag(o_ptr->art_flags, TR_SLAY_ORC);
		remove_flag(o_ptr->art_flags, TR_SLAY_TROLL);
		remove_flag(o_ptr->art_flags, TR_SLAY_GIANT);
		remove_flag(o_ptr->art_flags, TR_SLAY_DRAGON);
		remove_flag(o_ptr->art_flags, TR_KILL_DRAGON);
		remove_flag(o_ptr->art_flags, TR_SLAY_HUMAN);
		remove_flag(o_ptr->art_flags, TR_VORPAL);
		remove_flag(o_ptr->art_flags, TR_BRAND_POIS);
		remove_flag(o_ptr->art_flags, TR_BRAND_ACID);
		remove_flag(o_ptr->art_flags, TR_BRAND_ELEC);
		remove_flag(o_ptr->art_flags, TR_BRAND_FIRE);
		remove_flag(o_ptr->art_flags, TR_BRAND_COLD);
	}

	if (!object_is_weapon_ammo(o_ptr))
	{
		/* For armors */
		if (a_cursed) power_level = 0;
		else if (total_flags < 15000) power_level = 1;
		else if (total_flags < 35000) power_level = 2;
		else power_level = 3;
	}

	else
	{
		/* For weapons */
		if (a_cursed) power_level = 0;
		else if (total_flags < 20000) power_level = 1;
		else if (total_flags < 45000) power_level = 2;
		else power_level = 3;
	}

	if (a_scroll)
	{
		char dummy_name[80] = "";
#ifdef JP
		cptr ask_msg = "このアーティファクトを何と名付けますか？";
#else
		cptr ask_msg = "What do you want to call the artifact? ";
#endif

		/* Identify it fully */
		object_aware(o_ptr);
		object_known(o_ptr);

		/* Mark the item as fully known */
		o_ptr->ident |= (IDENT_MENTAL);

		(void)screen_object(o_ptr, 0L);

		if (!get_string(ask_msg, dummy_name, sizeof dummy_name)
		    || !dummy_name[0])
		{
			/* Cancelled */
			if (one_in_(2))
			{
				get_table_sindarin_aux(dummy_name);
			}
			else
			{
				get_table_name_aux(dummy_name);
			}
		}

#ifdef JP
		sprintf(new_name, "《%s》", dummy_name);
#else
		sprintf(new_name, "'%s'", dummy_name);
#endif

		chg_virtue(V_INDIVIDUALISM, 2);
		chg_virtue(V_ENCHANT, 5);
	}
	else
	{
		get_random_name(new_name, object_is_armour(o_ptr), power_level);
	}

	if (cheat_xtra)
	{
#ifdef JP
		if (artifact_bias) msg_format("運の偏ったアーティファクト: %d。", artifact_bias);
		else msg_print("アーティファクトに運の偏りなし。");
#else
		if (artifact_bias) msg_format("Biased artifact: %d.", artifact_bias);
		else msg_print("No bias in artifact.");
#endif
	}

	/* Save the inscription */
	o_ptr->art_name = quark_add(new_name);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP);

	return TRUE;
}


bool activate_random_artifact(object_type * o_ptr)
{
	int plev = p_ptr->lev;
	int k, dir, dummy = 0;

	if (!o_ptr->art_name) return FALSE; /* oops? */

	/* Activate for attack */
	switch (o_ptr->xtra2)
	{
		case ACT_SUNLIGHT:
		{
			if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
			msg_print("太陽光線が放たれた。");
#else
			msg_print("A line of sunlight appears.");
#endif

			(void)lite_line(dir);
			o_ptr->timeout = 10;
			break;
		}

		case ACT_BO_MISS_1:
		{
#ifdef JP
			msg_print("それは眩しいくらいに明るく輝いている...");
#else
			msg_print("It glows extremely brightly...");
#endif

			if (!get_aim_dir(&dir)) return FALSE;
			fire_bolt(GF_MISSILE, dir, damroll(2, 6));
			o_ptr->timeout = 2;
			break;
		}

		case ACT_BA_POIS_1:
		{
#ifdef JP
			msg_print("それは濃緑色に脈動している...");
#else
			msg_print("It throbs deep green...");
#endif

			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_POIS, dir, 12, 3);
			o_ptr->timeout = randint0(4) + 4;
			break;
		}

		case ACT_BO_ELEC_1:
		{
#ifdef JP
			msg_print("それは火花に覆われた...");
#else
			msg_print("It is covered in sparks...");
#endif

			if (!get_aim_dir(&dir)) return FALSE;
			fire_bolt(GF_ELEC, dir, damroll(4, 8));
			o_ptr->timeout = randint0(5) + 5;
			break;
		}

		case ACT_BO_ACID_1:
		{
#ifdef JP
			msg_print("それは酸に覆われた...");
#else
			msg_print("It is covered in acid...");
#endif

			if (!get_aim_dir(&dir)) return FALSE;
			fire_bolt(GF_ACID, dir, damroll(5, 8));
			o_ptr->timeout = randint0(6) + 6;
			break;
		}

		case ACT_BO_COLD_1:
		{
#ifdef JP
			msg_print("それは霜に覆われた...");
#else
			msg_print("It is covered in frost...");
#endif

			if (!get_aim_dir(&dir)) return FALSE;
			fire_bolt(GF_COLD, dir, damroll(6, 8));
			o_ptr->timeout = randint0(7) + 7;
			break;
		}

		case ACT_BO_FIRE_1:
		{
#ifdef JP
			msg_print("それは炎に覆われた...");
#else
			msg_print("It is covered in fire...");
#endif

			if (!get_aim_dir(&dir)) return FALSE;
			fire_bolt(GF_FIRE, dir, damroll(9, 8));
			o_ptr->timeout = randint0(8) + 8;
			break;
		}

		case ACT_BA_COLD_1:
		{
#ifdef JP
			msg_print("それは霜に覆われた...");
#else
			msg_print("It is covered in frost...");
#endif

			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_COLD, dir, 48, 2);
			o_ptr->timeout = 400;
			break;
		}

		case ACT_BA_FIRE_1:
		{
#ifdef JP
			msg_print("それは赤く激しく輝いた...");
#else
			msg_print("It glows an intense red...");
#endif

			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_FIRE, dir, 72, 2);
			o_ptr->timeout = 400;
			break;
		}

		case ACT_DRAIN_1:
		{
#ifdef JP
			msg_print("それは黒く輝いた...");
#else
			msg_print("It glows black...");
#endif

			if (!get_aim_dir(&dir)) return FALSE;
			if (drain_life(dir, 100))
			o_ptr->timeout = randint0(100) + 100;
			break;
		}

		case ACT_BA_COLD_2:
		{
#ifdef JP
			msg_print("それは青く激しく輝いた...");
#else
			msg_print("It glows an intense blue...");
#endif

			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_COLD, dir, 100, 2);
			o_ptr->timeout = 300;
			break;
		}

		case ACT_BA_ELEC_2:
		{
#ifdef JP
			msg_print("電気がパチパチ音を立てた...");
#else
			msg_print("It crackles with electricity...");
#endif

			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_ELEC, dir, 100, 3);
			o_ptr->timeout = 500;
			break;
		}

		case ACT_DRAIN_2:
		{
#ifdef JP
			msg_print("黒く輝いている...");
#else
			msg_print("It glows black...");
#endif

			if (!get_aim_dir(&dir)) return FALSE;
			drain_life(dir, 120);
			o_ptr->timeout = 400;
			break;
		}

		case ACT_VAMPIRE_1:
		{
			if (!get_aim_dir(&dir)) return FALSE;
			for (dummy = 0; dummy < 3; dummy++)
			{
				if (drain_life(dir, 50))
				hp_player(50);
			}
			o_ptr->timeout = 400;
			break;
		}

		case ACT_BO_MISS_2:
		{
#ifdef JP
			msg_print("魔法のトゲが現れた...");
#else
			msg_print("It grows magical spikes...");
#endif

			if (!get_aim_dir(&dir)) return FALSE;
			fire_bolt(GF_ARROW, dir, 150);
			o_ptr->timeout = randint0(90) + 90;
			break;
		}

		case ACT_BA_FIRE_2:
		{
#ifdef JP
			msg_print("深赤色に輝いている...");
#else
			msg_print("It glows deep red...");
#endif

			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_FIRE, dir, 120, 3);
			o_ptr->timeout = randint0(225) + 225;
			break;
		}

		case ACT_BA_COLD_3:
		{
#ifdef JP
			msg_print("明るく白色に輝いている...");
#else
			msg_print("It glows bright white...");
#endif

			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_COLD, dir, 200, 3);
			o_ptr->timeout = randint0(325) + 325;
			break;
		}

		case ACT_BA_ELEC_3:
		{
#ifdef JP
			msg_print("深青色に輝いている...");
#else
			msg_print("It glows deep blue...");
#endif

			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_ELEC, dir, 250, 3);
			o_ptr->timeout = randint0(425) + 425;
			break;
		}

		case ACT_WHIRLWIND:
		{
			{
				int y = 0, x = 0;
				cave_type       *c_ptr;
				monster_type    *m_ptr;

				for (dir = 0; dir <= 9; dir++)
				{
					y = py + ddy[dir];
					x = px + ddx[dir];
					c_ptr = &cave[y][x];

					/* Get the monster */
					m_ptr = &m_list[c_ptr->m_idx];

					/* Hack -- attack monsters */
					if (c_ptr->m_idx && (m_ptr->ml || cave_have_flag_bold(y, x, FF_PROJECT)))
						py_attack(y, x, 0);
				}
			}
			o_ptr->timeout = 250;
			break;
		}

		case ACT_VAMPIRE_2:
		{
			if (!get_aim_dir(&dir)) return FALSE;
			for (dummy = 0; dummy < 3; dummy++)
			{
				if (drain_life(dir, 100))
				hp_player(100);
			}

			o_ptr->timeout = 400;
			break;
		}


		case ACT_CALL_CHAOS:
		{
#ifdef JP
			msg_print("様々な色の火花を発している...");
#else
			msg_print("It glows in scintillating colours...");
#endif

			call_chaos();
			o_ptr->timeout = 350;
			break;
		}

		case ACT_ROCKET:
		{
			if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
			msg_print("ロケットを発射した！");
#else
			msg_print("You launch a rocket!");
#endif

			fire_ball(GF_ROCKET, dir, 250 + plev*3, 2);
			o_ptr->timeout = 400;
			break;
		}

		case ACT_DISP_EVIL:
		{
#ifdef JP
			msg_print("神聖な雰囲気が充満した...");
#else
			msg_print("It floods the area with goodness...");
#endif

			dispel_evil(p_ptr->lev * 5);
			o_ptr->timeout = randint0(300) + 300;
			break;
		}

		case ACT_DISP_GOOD:
		{
#ifdef JP
			msg_print("邪悪な雰囲気が充満した...");
#else
			msg_print("It floods the area with evil...");
#endif

			dispel_good(p_ptr->lev * 5);
			o_ptr->timeout = randint0(300) + 300;
			break;
		}

		case ACT_BA_MISS_3:
		{
			if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
			msg_print("あなたはエレメントのブレスを吐いた。");
#else
			msg_print("You breathe the elements.");
#endif

			fire_ball(GF_MISSILE, dir, 300, 4);
			o_ptr->timeout = 500;
			break;
		}

		/* Activate for other offensive action */

		case ACT_CONFUSE:
		{
#ifdef JP
			msg_print("様々な色の火花を発している...");
#else
			msg_print("It glows in scintillating colours...");
#endif

			if (!get_aim_dir(&dir)) return FALSE;
			confuse_monster(dir, 20);
			o_ptr->timeout = 15;
			break;
		}

		case ACT_SLEEP:
		{
#ifdef JP
			msg_print("深青色に輝いている...");
#else
			msg_print("It glows deep blue...");
#endif

			sleep_monsters_touch();
			o_ptr->timeout = 55;
			break;
		}

		case ACT_QUAKE:
		{
			earthquake(py, px, 10);
			o_ptr->timeout = 50;
			break;
		}

		case ACT_TERROR:
		{
			turn_monsters(40 + p_ptr->lev);
			o_ptr->timeout = 3 * (p_ptr->lev + 10);
			break;
		}

		case ACT_TELE_AWAY:
		{
			if (!get_aim_dir(&dir)) return FALSE;
			(void)fire_beam(GF_AWAY_ALL, dir, plev);
			o_ptr->timeout = 200;
			break;
		}

		case ACT_BANISH_EVIL:
		{
			if (banish_evil(100))
			{
#ifdef JP
				msg_print("アーティファクトの力が邪悪を打ち払った！");
#else
				msg_print("The power of the artifact banishes evil!");
#endif

			}
			o_ptr->timeout = 250 + randint1(250);
			break;
		}

		case ACT_GENOCIDE:
		{
#ifdef JP
			msg_print("深青色に輝いている...");
#else
			msg_print("It glows deep blue...");
#endif

			(void)symbol_genocide(200, TRUE);
			o_ptr->timeout = 500;
			break;
		}

		case ACT_MASS_GENO:
		{
#ifdef JP
			msg_print("ひどく鋭い音が流れ出た...");
#else
			msg_print("It lets out a long, shrill note...");
#endif

			(void)mass_genocide(200, TRUE);
			o_ptr->timeout = 1000;
			break;
		}

		/* Activate for summoning / charming */

		case ACT_CHARM_ANIMAL:
		{
			if (!get_aim_dir(&dir)) return FALSE;
			(void)charm_animal(dir, plev);
			o_ptr->timeout = 300;
			break;
		}

		case ACT_CHARM_UNDEAD:
		{
			if (!get_aim_dir(&dir)) return FALSE;
			(void)control_one_undead(dir, plev);
			o_ptr->timeout = 333;
			break;
		}

		case ACT_CHARM_OTHER:
		{
			if (!get_aim_dir(&dir)) return FALSE;
			(void)charm_monster(dir, plev);
			o_ptr->timeout = 400;
			break;
		}

		case ACT_CHARM_ANIMALS:
		{
			(void)charm_animals(plev * 2);
			o_ptr->timeout = 500;
			break;
		}

		case ACT_CHARM_OTHERS:
		{
			charm_monsters(plev * 2);
			o_ptr->timeout = 750;
			break;
		}

		case ACT_SUMMON_ANIMAL:
		{
			(void)summon_specific(-1, py, px, plev, SUMMON_ANIMAL_RANGER, (PM_ALLOW_GROUP | PM_FORCE_PET));
			o_ptr->timeout = 200 + randint1(300);
			break;
		}

		case ACT_SUMMON_PHANTOM:
		{
#ifdef JP
			msg_print("幻霊を召喚した。");
#else
			msg_print("You summon a phantasmal servant.");
#endif

			(void)summon_specific(-1, py, px, dun_level, SUMMON_PHANTOM, (PM_ALLOW_GROUP | PM_FORCE_PET));
			o_ptr->timeout = 200 + randint1(200);
			break;
		}

		case ACT_SUMMON_ELEMENTAL:
		{
			bool pet = one_in_(3);
			u32b mode = 0L;

			if (!(pet && (plev < 50))) mode |= PM_ALLOW_GROUP;
			if (pet) mode |= PM_FORCE_PET;
			else mode |= PM_NO_PET;

			if (summon_specific((pet ? -1 : 0), py, px, ((plev * 3) / 2), SUMMON_ELEMENTAL, mode))
			{
#ifdef JP
				msg_print("エレメンタルが現れた...");
#else
				msg_print("An elemental materializes...");
#endif


				if (pet)
#ifdef JP
					msg_print("あなたに服従しているようだ。");
#else
					msg_print("It seems obedient to you.");
#endif

				else
#ifdef JP
					msg_print("それをコントロールできなかった！");
#else
					msg_print("You fail to control it!");
#endif

			}

			o_ptr->timeout = 750;
			break;
		}

		case ACT_SUMMON_DEMON:
		{
			bool pet = one_in_(3);
			u32b mode = 0L;

			if (!(pet && (plev < 50))) mode |= PM_ALLOW_GROUP;
			if (pet) mode |= PM_FORCE_PET;
			else mode |= PM_NO_PET;

			if (summon_specific((pet ? -1 : 0), py, px, ((plev * 3) / 2), SUMMON_DEMON, mode))
			{
#ifdef JP
				msg_print("硫黄の悪臭が充満した。");
#else
				msg_print("The area fills with a stench of sulphur and brimstone.");
#endif

				if (pet)
#ifdef JP
					msg_print("「ご用でございますか、ご主人様」");
#else
					msg_print("'What is thy bidding... Master?'");
#endif

				else
#ifdef JP
					msg_print("「NON SERVIAM! Wretch! お前の魂を頂くぞ！」");
#else
					msg_print("'NON SERVIAM! Wretch! I shall feast on thy mortal soul!'");
#endif

			}

			o_ptr->timeout = 666 + randint1(333);
			break;
		}

		case ACT_SUMMON_UNDEAD:
		{
			bool pet = one_in_(3);
			int type;
			u32b mode = 0L;

			type = (plev > 47 ? SUMMON_HI_UNDEAD : SUMMON_UNDEAD);

			if (!pet || ((plev > 24) && one_in_(3))) mode |= PM_ALLOW_GROUP;
			if (pet) mode |= PM_FORCE_PET;
			else mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);

			if (summon_specific((pet ? -1 : 0), py, px, ((plev * 3) / 2), type, mode))
			{
#ifdef JP
				msg_print("冷たい風があなたの周りに吹き始めた。それは腐敗臭を運んでいる...");
#else
				msg_print("Cold winds begin to blow around you, carrying with them the stench of decay...");
#endif

				if (pet)
#ifdef JP
					msg_print("古えの死せる者共があなたに仕えるため土から甦った！");
#else
					msg_print("Ancient, long-dead forms arise from the ground to serve you!");
#endif

				else
#ifdef JP
					msg_print("死者が甦った。眠りを妨げるあなたを罰するために！");
#else
					msg_print("'The dead arise... to punish you for disturbing them!'");
#endif

			}

			o_ptr->timeout = 666 + randint1(333);
			break;
		}

		/* Activate for healing */

		case ACT_CURE_LW:
		{
			(void)set_afraid(0);
			(void)hp_player(30);
			o_ptr->timeout = 10;
			break;
		}

		case ACT_CURE_MW:
		{
#ifdef JP
			msg_print("深紫色の光を発している...");
#else
			msg_print("It radiates deep purple...");
#endif

			hp_player(damroll(4, 8));
			(void)set_cut((p_ptr->cut / 2) - 50);
			o_ptr->timeout = randint0(3) + 3;
			break;
		}

		case ACT_CURE_POISON:
		{
#ifdef JP
			msg_print("深青色に輝いている...");
#else
			msg_print("It glows deep blue...");
#endif

			(void)set_afraid(0);
			(void)set_poisoned(0);
			o_ptr->timeout = 5;
			break;
		}

		case ACT_REST_LIFE:
		{
#ifdef JP
			msg_print("深紅に輝いている...");
#else
			msg_print("It glows a deep red...");
#endif

			restore_level();
			o_ptr->timeout = 450;
			break;
		}

		case ACT_REST_ALL:
		{
#ifdef JP
			msg_print("濃緑色に輝いている...");
#else
			msg_print("It glows a deep green...");
#endif

			(void)do_res_stat(A_STR);
			(void)do_res_stat(A_INT);
			(void)do_res_stat(A_WIS);
			(void)do_res_stat(A_DEX);
			(void)do_res_stat(A_CON);
			(void)do_res_stat(A_CHR);
			(void)restore_level();
			o_ptr->timeout = 750;
			break;
		}

		case ACT_CURE_700:
		{
#ifdef JP
			msg_print("深青色に輝いている...");
#else
			msg_print("It glows deep blue...");
#endif

#ifdef JP
			msg_print("体内に暖かい鼓動が感じられる...");
#else
			msg_print("You feel a warm tingling inside...");
#endif

			(void)hp_player(700);
			(void)set_cut(0);
			o_ptr->timeout = 250;
			break;
		}

		case ACT_CURE_1000:
		{
#ifdef JP
			msg_print("白く明るく輝いている...");
#else
			msg_print("It glows a bright white...");
#endif

#ifdef JP
			msg_print("ひじょうに気分がよい...");
#else
			msg_print("You feel much better...");
#endif

			(void)hp_player(1000);
			(void)set_cut(0);
			o_ptr->timeout = 888;
			break;
		}

		/* Activate for timed effect */

		case ACT_ESP:
		{
			(void)set_tim_esp(randint1(30) + 25, FALSE);
			o_ptr->timeout = 200;
			break;
		}

		case ACT_BERSERK:
		{
			(void)set_afraid(0);
			(void)set_hero(randint1(50) + 50, FALSE);
			(void)set_blessed(randint1(50) + 50, FALSE);
			o_ptr->timeout = 100 + randint1(100);
			break;
		}

		case ACT_PROT_EVIL:
		{
#ifdef JP
			msg_print("鋭い音が流れ出た...");
#else
			msg_print("It lets out a shrill wail...");
#endif

			k = 3 * p_ptr->lev;
			(void)set_protevil(randint1(25) + k, FALSE);
			o_ptr->timeout = randint0(225) + 225;
			break;
		}

		case ACT_RESIST_ALL:
		{
#ifdef JP
			msg_print("様々な色に輝いている...");
#else
			msg_print("It glows many colours...");
#endif

			(void)set_oppose_acid(randint1(40) + 40, FALSE);
			(void)set_oppose_elec(randint1(40) + 40, FALSE);
			(void)set_oppose_fire(randint1(40) + 40, FALSE);
			(void)set_oppose_cold(randint1(40) + 40, FALSE);
			(void)set_oppose_pois(randint1(40) + 40, FALSE);
			o_ptr->timeout = 200;
			break;
		}

		case ACT_SPEED:
		{
#ifdef JP
			msg_print("明るく緑色に輝いている...");
#else
			msg_print("It glows bright green...");
#endif

			(void)set_fast(randint1(20) + 20, FALSE);
			o_ptr->timeout = 250;
			break;
		}

		case ACT_XTRA_SPEED:
		{
#ifdef JP
			msg_print("明るく輝いている...");
#else
			msg_print("It glows brightly...");
#endif

			(void)set_fast(randint1(75) + 75, FALSE);
			o_ptr->timeout = randint0(200) + 200;
			break;
		}

		case ACT_WRAITH:
		{
			set_wraith_form(randint1(plev / 2) + (plev / 2), FALSE);
			o_ptr->timeout = 1000;
			break;
		}

		case ACT_INVULN:
		{
			(void)set_invuln(randint1(8) + 8, FALSE);
			o_ptr->timeout = 1000;
			break;
		}

		/* Activate for general purpose effect (detection etc.) */

		case ACT_LIGHT:
		{
#ifdef JP
			msg_print("澄んだ光があふれ出た...");
#else
			msg_print("It wells with clear light...");
#endif

			lite_area(damroll(2, 15), 3);
			o_ptr->timeout = randint0(10) + 10;
			break;
		}

		case ACT_MAP_LIGHT:
		{
#ifdef JP
			msg_print("眩しく輝いた...");
#else
			msg_print("It shines brightly...");
#endif

			map_area(DETECT_RAD_MAP);
			lite_area(damroll(2, 15), 3);
			o_ptr->timeout = randint0(50) + 50;
			break;
		}

		case ACT_DETECT_ALL:
		{
#ifdef JP
			msg_print("白く明るく輝いている...");
#else
			msg_print("It glows bright white...");
#endif

#ifdef JP
			msg_print("心にイメージが浮かんできた...");
#else
			msg_print("An image forms in your mind...");
#endif

			detect_all(DETECT_RAD_DEFAULT);
			o_ptr->timeout = randint0(55) + 55;
			break;
		}

		case ACT_DETECT_XTRA:
		{
#ifdef JP
			msg_print("明るく輝いている...");
#else
			msg_print("It glows brightly...");
#endif

			detect_all(DETECT_RAD_DEFAULT);
			probing();
			identify_fully(FALSE);
			o_ptr->timeout = 1000;
			break;
		}

		case ACT_ID_FULL:
		{
#ifdef JP
			msg_print("黄色く輝いている...");
#else
			msg_print("It glows yellow...");
#endif

			identify_fully(FALSE);
			o_ptr->timeout = 750;
			break;
		}

		case ACT_ID_PLAIN:
		{
			if (!ident_spell(FALSE)) return FALSE;
			o_ptr->timeout = 10;
			break;
		}

		case ACT_RUNE_EXPLO:
		{
#ifdef JP
			msg_print("明るい赤色に輝いている...");
#else
			msg_print("It glows bright red...");
#endif

			explosive_rune();
			o_ptr->timeout = 200;
			break;
		}

		case ACT_RUNE_PROT:
		{
#ifdef JP
			msg_print("ブルーに明るく輝いている...");
#else
			msg_print("It glows light blue...");
#endif

			warding_glyph();
			o_ptr->timeout = 400;
			break;
		}

		case ACT_SATIATE:
		{
			(void)set_food(PY_FOOD_MAX - 1);
			o_ptr->timeout = 200;
			break;
		}

		case ACT_DEST_DOOR:
		{
#ifdef JP
			msg_print("明るい赤色に輝いている...");
#else
			msg_print("It glows bright red...");
#endif

			destroy_doors_touch();
			o_ptr->timeout = 10;
			break;
		}

		case ACT_STONE_MUD:
		{
#ifdef JP
			msg_print("鼓動している...");
#else
			msg_print("It pulsates...");
#endif

			if (!get_aim_dir(&dir)) return FALSE;
			wall_to_mud(dir);
			o_ptr->timeout = 5;
			break;
		}

		case ACT_RECHARGE:
		{
			recharge(130);
			o_ptr->timeout = 70;
			break;
		}

		case ACT_ALCHEMY:
		{
#ifdef JP
			msg_print("明るい黄色に輝いている...");
#else
			msg_print("It glows bright yellow...");
#endif

			(void)alchemy();
			o_ptr->timeout = 500;
			break;
		}

		case ACT_DIM_DOOR:
		{
#ifdef JP
			msg_print("次元の扉が開いた。目的地を選んで下さい。");
#else
			msg_print("You open a dimensional gate. Choose a destination.");
#endif

			if (!dimension_door()) return FALSE;
			o_ptr->timeout = 100;
			break;
		}


		case ACT_TELEPORT:
		{
#ifdef JP
			msg_print("周りの空間が歪んでいる...");
#else
			msg_print("It twists space around you...");
#endif

			teleport_player(100, 0L);
			o_ptr->timeout = 45;
			break;
		}

		case ACT_RECALL:
		{
#ifdef JP
			msg_print("やわらかな白色に輝いている...");
#else
			msg_print("It glows soft white...");
#endif
			if (!word_of_recall()) return FALSE;
			o_ptr->timeout = 200;
			break;
		}

		default:
		{
#ifdef JP
			msg_format("Unknown activation effect: %d.", o_ptr->xtra2);
#else
			msg_format("Unknown activation effect: %d.", o_ptr->xtra2);
#endif

			return FALSE;
		}
	}

	return TRUE;
}


void get_bloody_moon_flags(object_type *o_ptr)
{
	int dummy, i;

	for (i = 0; i < TR_FLAG_SIZE; i++)
		o_ptr->art_flags[i] = a_info[ART_BLOOD].flags[i];

	dummy = randint1(2) + randint1(2);
	for (i = 0; i < dummy; i++)
	{
		int flag = randint0(26);
		if (flag >= 20) add_flag(o_ptr->art_flags, TR_KILL_UNDEAD + flag - 20);
		else if (flag == 19) add_flag(o_ptr->art_flags, TR_KILL_ANIMAL);
		else if (flag == 18) add_flag(o_ptr->art_flags, TR_SLAY_HUMAN);
		else add_flag(o_ptr->art_flags, TR_CHAOTIC + flag);
	}

	dummy = randint1(2);
	for (i = 0; i < dummy; i++) one_resistance(o_ptr);

	for (i = 0; i < 2; i++)
	{
		int tmp = randint0(11);
		if (tmp < 6) add_flag(o_ptr->art_flags, TR_STR + tmp);
		else add_flag(o_ptr->art_flags, TR_STEALTH + tmp - 6);
	}
}


void random_artifact_resistance(object_type * o_ptr, artifact_type *a_ptr)
{
	bool give_resistance = FALSE, give_power = FALSE;

	if (o_ptr->name1 == ART_TERROR) /* Terror Mask is for warriors... */
	{
		if (p_ptr->pclass == CLASS_WARRIOR || p_ptr->pclass == CLASS_ARCHER || p_ptr->pclass == CLASS_CAVALRY || p_ptr->pclass == CLASS_BERSERKER)
		{
			give_power = TRUE;
			give_resistance = TRUE;
		}
		else
		{
			add_flag(o_ptr->art_flags, TR_AGGRAVATE);
			add_flag(o_ptr->art_flags, TR_TY_CURSE);
			o_ptr->curse_flags |=
			    (TRC_CURSED | TRC_HEAVY_CURSE);
			o_ptr->curse_flags |= get_curse(2, o_ptr);
			return;
		}
	}

	if (o_ptr->name1 == ART_MURAMASA)
	{
		if (p_ptr->pclass != CLASS_SAMURAI)
		{
			add_flag(o_ptr->art_flags, TR_NO_MAGIC);
			o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
		}
	}

	if (o_ptr->name1 == ART_XIAOLONG)
	{
		if (p_ptr->pclass == CLASS_MONK)
			add_flag(o_ptr->art_flags, TR_BLOWS);
	}

	if (o_ptr->name1 == ART_BLOOD)
	{
		get_bloody_moon_flags(o_ptr);
	}

	if (o_ptr->name1 == ART_HEAVENLY_MAIDEN)
	{
		if (p_ptr->psex != SEX_FEMALE)
		{
			add_flag(o_ptr->art_flags, TR_AGGRAVATE);
		}
	}

	if (a_ptr->gen_flags & (TRG_XTRA_POWER)) give_power = TRUE;
	if (a_ptr->gen_flags & (TRG_XTRA_H_RES)) give_resistance = TRUE;
	if (a_ptr->gen_flags & (TRG_XTRA_RES_OR_POWER))
	{
		/* Give a resistance OR a power */
		if (one_in_(2)) give_resistance = TRUE;
		else give_power = TRUE;
	}

	if (give_power)
	{
		one_ability(o_ptr);
	}

	if (give_resistance)
	{
		one_high_resistance(o_ptr);
	}
}


/*
 * Create the artifact of the specified number
 */
bool create_named_art(int a_idx, int y, int x)
{
	object_type forge;
	object_type *q_ptr;
	int i;

	artifact_type *a_ptr = &a_info[a_idx];

	/* Get local object */
	q_ptr = &forge;

	/* Ignore "empty" artifacts */
	if (!a_ptr->name) return FALSE;

	/* Acquire the "kind" index */
	i = lookup_kind(a_ptr->tval, a_ptr->sval);

	/* Oops */
	if (!i) return FALSE;

	/* Create the artifact */
	object_prep(q_ptr, i);

	/* Save the name */
	q_ptr->name1 = a_idx;

	/* Extract the fields */
	q_ptr->pval = a_ptr->pval;
	q_ptr->ac = a_ptr->ac;
	q_ptr->dd = a_ptr->dd;
	q_ptr->ds = a_ptr->ds;
	q_ptr->to_a = a_ptr->to_a;
	q_ptr->to_h = a_ptr->to_h;
	q_ptr->to_d = a_ptr->to_d;
	q_ptr->weight = a_ptr->weight;

	/* Hack -- extract the "cursed" flag */
	if (a_ptr->gen_flags & TRG_CURSED) q_ptr->curse_flags |= (TRC_CURSED);
	if (a_ptr->gen_flags & TRG_HEAVY_CURSE) q_ptr->curse_flags |= (TRC_HEAVY_CURSE);
	if (a_ptr->gen_flags & TRG_PERMA_CURSE) q_ptr->curse_flags |= (TRC_PERMA_CURSE);
	if (a_ptr->gen_flags & (TRG_RANDOM_CURSE0)) q_ptr->curse_flags |= get_curse(0, q_ptr);
	if (a_ptr->gen_flags & (TRG_RANDOM_CURSE1)) q_ptr->curse_flags |= get_curse(1, q_ptr);
	if (a_ptr->gen_flags & (TRG_RANDOM_CURSE2)) q_ptr->curse_flags |= get_curse(2, q_ptr);

	random_artifact_resistance(q_ptr, a_ptr);

	/*
	 * drop_near()内で普通の固定アーティファクトが重ならない性質に依存する.
	 * 仮に2個以上存在可能かつ装備品以外の固定アーティファクトが作成されれば
	 * この関数の返り値は信用できなくなる.
	 */

	/* Drop the artifact from heaven */
	return drop_near(q_ptr, -1, y, x) ? TRUE : FALSE;
}
