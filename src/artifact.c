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


static void curse_artifact(object_type * o_ptr)
{
	if (o_ptr->pval > 0) o_ptr->pval = 0 - (o_ptr->pval + randint1(4));
	if (o_ptr->to_a > 0) o_ptr->to_a = 0 - (o_ptr->to_a + randint1(4));
	if (o_ptr->to_h > 0) o_ptr->to_h = 0 - (o_ptr->to_h + randint1(4));
	if (o_ptr->to_d > 0) o_ptr->to_d = 0 - (o_ptr->to_d + randint1(4));

	o_ptr->art_flags3 |= (TR3_HEAVY_CURSE | TR3_CURSED);

	if (one_in_(4)) o_ptr->art_flags3 |= TR3_PERMA_CURSE;
	if (one_in_(3)) o_ptr->art_flags3 |= TR3_TY_CURSE;
	if (one_in_(2)) o_ptr->art_flags3 |= TR3_AGGRAVATE;
	if (one_in_(3)) o_ptr->art_flags3 |= TR3_DRAIN_EXP;
	if (one_in_(2)) o_ptr->art_flags3 |= TR3_TELEPORT;
	else if (one_in_(3)) o_ptr->art_flags3 |= TR3_NO_TELE;

	if ((p_ptr->pclass != CLASS_WARRIOR) && (p_ptr->pclass != CLASS_ARCHER) && (p_ptr->pclass != CLASS_CAVALRY) && (p_ptr->pclass != CLASS_BERSERKER) && (p_ptr->pclass != CLASS_SMITH) && one_in_(3))
		o_ptr->art_flags3 |= TR3_NO_MAGIC;

	o_ptr->ident |= IDENT_CURSED;
}


static void random_plus(object_type * o_ptr, bool is_scroll)
{
	int this_type = (o_ptr->tval < TV_BOOTS ? 23 : 19);

	if (artifact_bias == BIAS_WARRIOR)
	{
		if (!(o_ptr->art_flags1 & TR1_STR))
		{
			o_ptr->art_flags1 |= TR1_STR;
			if (one_in_(2)) return; /* 50% chance of being a "free" power */
		}

		if (!(o_ptr->art_flags1 & TR1_CON))
		{
			o_ptr->art_flags1 |= TR1_CON;
			if (one_in_(2)) return;
		}

		if (!(o_ptr->art_flags1 & TR1_DEX))
		{
			o_ptr->art_flags1 |= TR1_DEX;
			if (one_in_(2)) return;
		}
	}
	else if (artifact_bias == BIAS_MAGE)
	{
		if (!(o_ptr->art_flags1 & TR1_INT))
		{
			o_ptr->art_flags1 |= TR1_INT;
			if (one_in_(2)) return;
		}
		if ((o_ptr->tval == TV_GLOVES) && !(o_ptr->art_flags1 & TR1_MAGIC_MASTERY))
		{
			o_ptr->art_flags1 |= TR1_MAGIC_MASTERY;
			if (one_in_(2)) return;
		}
	}
	else if (artifact_bias == BIAS_PRIESTLY)
	{
		if (!(o_ptr->art_flags1 & TR1_WIS))
		{
			o_ptr->art_flags1 |= TR1_WIS;
			if (one_in_(2)) return;
		}
	}
	else if (artifact_bias == BIAS_RANGER)
	{
		if (!(o_ptr->art_flags1 & TR1_DEX))
		{
			o_ptr->art_flags1 |= TR1_DEX;
			if (one_in_(2)) return;
		}

		if (!(o_ptr->art_flags1 & TR1_CON))
		{
			o_ptr->art_flags1 |= TR1_CON;
			if (one_in_(2)) return; /* 50% chance of being a "free" power */
		}

		if (!(o_ptr->art_flags1 & TR1_STR))
		{
			o_ptr->art_flags1 |= TR1_STR;
			if (one_in_(2)) return;
		}
	}
	else if (artifact_bias == BIAS_ROGUE)
	{
		if (!(o_ptr->art_flags1 & TR1_STEALTH))
		{
			o_ptr->art_flags1 |= TR1_STEALTH;
			if (one_in_(2)) return;
		}
		if (!(o_ptr->art_flags1 & TR1_SEARCH))
		{
			o_ptr->art_flags1 |= TR1_SEARCH;
			if (one_in_(2)) return;
		}
	}
	else if (artifact_bias == BIAS_STR)
	{
		if (!(o_ptr->art_flags1 & TR1_STR))
		{
			o_ptr->art_flags1 |= TR1_STR;
			if (one_in_(2)) return;
		}
	}
	else if (artifact_bias == BIAS_WIS)
	{
		if (!(o_ptr->art_flags1 & TR1_WIS))
		{
			o_ptr->art_flags1 |= TR1_WIS;
			if (one_in_(2)) return;
		}
	}
	else if (artifact_bias == BIAS_INT)
	{
		if (!(o_ptr->art_flags1 & TR1_INT))
		{
			o_ptr->art_flags1 |= TR1_INT;
			if (one_in_(2)) return;
		}
	}
	else if (artifact_bias == BIAS_DEX)
	{
		if (!(o_ptr->art_flags1 & TR1_DEX))
		{
			o_ptr->art_flags1 |= TR1_DEX;
			if (one_in_(2)) return;
		}
	}
	else if (artifact_bias == BIAS_CON)
	{
		if (!(o_ptr->art_flags1 & TR1_CON))
		{
			o_ptr->art_flags1 |= TR1_CON;
			if (one_in_(2)) return;
		}
	}
	else if (artifact_bias == BIAS_CHR)
	{
		if (!(o_ptr->art_flags1 & TR1_CHR))
		{
			o_ptr->art_flags1 |= TR1_CHR;
			if (one_in_(2)) return;
		}
	}

	if ((artifact_bias == BIAS_MAGE || artifact_bias == BIAS_PRIESTLY) && (o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_ROBE))
	{
		if (!(o_ptr->art_flags3 & TR3_DEC_MANA) && one_in_(3))
		{
			o_ptr->art_flags3 |= TR3_DEC_MANA;
			if (one_in_(2)) return;
		}
	}

	switch (randint1(this_type))
	{
	case 1: case 2:
		o_ptr->art_flags1 |= TR1_STR;
		/*  if (is_scroll) msg_print("It makes you feel strong!"); */
		if (!artifact_bias && !one_in_(13))
			artifact_bias = BIAS_STR;
		else if (!artifact_bias && one_in_(7))
			artifact_bias = BIAS_WARRIOR;
		break;
	case 3: case 4:
		o_ptr->art_flags1 |= TR1_INT;
		/*  if (is_scroll) msg_print("It makes you feel smart!"); */
		if (!artifact_bias && !one_in_(13))
			artifact_bias = BIAS_INT;
		else if (!artifact_bias && one_in_(7))
			artifact_bias = BIAS_MAGE;
		break;
	case 5: case 6:
		o_ptr->art_flags1 |= TR1_WIS;
		/*  if (is_scroll) msg_print("It makes you feel wise!"); */
		if (!artifact_bias && !one_in_(13))
			artifact_bias = BIAS_WIS;
		else if (!artifact_bias && one_in_(7))
			artifact_bias = BIAS_PRIESTLY;
		break;
	case 7: case 8:
		o_ptr->art_flags1 |= TR1_DEX;
		/*  if (is_scroll) msg_print("It makes you feel nimble!"); */
		if (!artifact_bias && !one_in_(13))
			artifact_bias = BIAS_DEX;
		else if (!artifact_bias && one_in_(7))
			artifact_bias = BIAS_ROGUE;
		break;
	case 9: case 10:
		o_ptr->art_flags1 |= TR1_CON;
		/*  if (is_scroll) msg_print("It makes you feel healthy!"); */
		if (!artifact_bias && !one_in_(13))
			artifact_bias = BIAS_CON;
		else if (!artifact_bias && one_in_(9))
			artifact_bias = BIAS_RANGER;
		break;
	case 11: case 12:
		o_ptr->art_flags1 |= TR1_CHR;
		/*  if (is_scroll) msg_print("It makes you look great!"); */
		if (!artifact_bias && !one_in_(13))
			artifact_bias = BIAS_CHR;
		break;
	case 13: case 14:
		o_ptr->art_flags1 |= TR1_STEALTH;
		/*  if (is_scroll) msg_print("It looks muffled."); */
		if (!artifact_bias && one_in_(3))
			artifact_bias = BIAS_ROGUE;
		break;
	case 15: case 16:
		o_ptr->art_flags1 |= TR1_SEARCH;
		/*  if (is_scroll) msg_print("It makes you see better."); */
		if (!artifact_bias && one_in_(9))
			artifact_bias = BIAS_RANGER;
		break;
	case 17: case 18:
		o_ptr->art_flags1 |= TR1_INFRA;
		/*  if (is_scroll) msg_print("It makes you see tiny red animals.");*/
		break;
	case 19:
		o_ptr->art_flags1 |= TR1_SPEED;
		/*  if (is_scroll) msg_print("It makes you move faster!"); */
		if (!artifact_bias && one_in_(11))
			artifact_bias = BIAS_ROGUE;
		break;
	case 20: case 21:
		o_ptr->art_flags1 |= TR1_TUNNEL;
		/*  if (is_scroll) msg_print("Gravel flies from it!"); */
		break;
	case 22: case 23:
		if (o_ptr->tval == TV_BOW) random_plus(o_ptr, is_scroll);
		else
		{
			o_ptr->art_flags1 |= TR1_BLOWS;
			/*  if (is_scroll) msg_print("It seems faster!"); */
			if (!artifact_bias && one_in_(11))
				artifact_bias = BIAS_WARRIOR;
		}
		break;
	}
}


void random_resistance(object_type * o_ptr, bool is_scroll, int specific)
{
	if (!specific) /* To avoid a number of possible bugs */
	{
		if (artifact_bias == BIAS_ACID)
		{
			if (!(o_ptr->art_flags2 & TR2_RES_ACID))
			{
				o_ptr->art_flags2 |= TR2_RES_ACID;
				if (one_in_(2)) return;
			}
			if (one_in_(BIAS_LUCK) && !(o_ptr->art_flags2 & TR2_IM_ACID))
			{
				o_ptr->art_flags2 |= TR2_IM_ACID;
				if (!one_in_(IM_LUCK)) o_ptr->art_flags2 &= ~(TR2_IM_ELEC | TR2_IM_COLD | TR2_IM_FIRE);
				if (one_in_(2)) return;
			}
		}
		else if (artifact_bias == BIAS_ELEC)
		{
			if (!(o_ptr->art_flags2 & TR2_RES_ELEC))
			{
				o_ptr->art_flags2 |= TR2_RES_ELEC;
				if (one_in_(2)) return;
			}
			if ((o_ptr->tval >= TV_CLOAK) && (o_ptr->tval <= TV_HARD_ARMOR) &&
			   !(o_ptr->art_flags3 & TR3_SH_ELEC))
			{
				o_ptr->art_flags3 |= TR3_SH_ELEC;
				if (one_in_(2)) return;
			}
			if (one_in_(BIAS_LUCK) && !(o_ptr->art_flags2 & TR2_IM_ELEC))
			{
				o_ptr->art_flags2 |= TR2_IM_ELEC;
				if (!one_in_(IM_LUCK)) o_ptr->art_flags2 &= ~(TR2_IM_ACID | TR2_IM_COLD | TR2_IM_FIRE);
				if (one_in_(2)) return;
			}
		}
		else if (artifact_bias == BIAS_FIRE)
		{
			if (!(o_ptr->art_flags2 & TR2_RES_FIRE))
			{
				o_ptr->art_flags2 |= TR2_RES_FIRE;
				if (one_in_(2)) return;
			}
			if ((o_ptr->tval >= TV_CLOAK) &&
			    (o_ptr->tval <= TV_HARD_ARMOR) &&
			    !(o_ptr->art_flags3 & TR3_SH_FIRE))
			{
				o_ptr->art_flags3 |= TR3_SH_FIRE;
				if (one_in_(2)) return;
			}
			if (one_in_(BIAS_LUCK) &&
			    !(o_ptr->art_flags2 & TR2_IM_FIRE))
			{
				o_ptr->art_flags2 |= TR2_IM_FIRE;
				if (!one_in_(IM_LUCK)) o_ptr->art_flags2 &= ~(TR2_IM_ELEC | TR2_IM_COLD | TR2_IM_ACID);
				if (one_in_(2)) return;
			}
		}
		else if (artifact_bias == BIAS_COLD)
		{
			if (!(o_ptr->art_flags2 & TR2_RES_COLD))
			{
				o_ptr->art_flags2 |= TR2_RES_COLD;
				if (one_in_(2)) return;
			}
			if ((o_ptr->tval >= TV_CLOAK) &&
			    (o_ptr->tval <= TV_HARD_ARMOR) &&
			    !(o_ptr->art_flags3 & TR3_SH_COLD))
			{
				o_ptr->art_flags3 |= TR3_SH_COLD;
				if (one_in_(2)) return;
			}
			if (one_in_(BIAS_LUCK) && !(o_ptr->art_flags2 & TR2_IM_COLD))
			{
				o_ptr->art_flags2 |= TR2_IM_COLD;
				if (!one_in_(IM_LUCK)) o_ptr->art_flags2 &= ~(TR2_IM_ELEC | TR2_IM_ACID | TR2_IM_FIRE);
				if (one_in_(2)) return;
			}
		}
		else if (artifact_bias == BIAS_POIS)
		{
			if (!(o_ptr->art_flags2 & TR2_RES_POIS))
			{
				o_ptr->art_flags2 |= TR2_RES_POIS;
				if (one_in_(2)) return;
			}
		}
		else if (artifact_bias == BIAS_WARRIOR)
		{
			if (!one_in_(3) && (!(o_ptr->art_flags2 & TR2_RES_FEAR)))
			{
				o_ptr->art_flags2 |= TR2_RES_FEAR;
				if (one_in_(2)) return;
			}
			if (one_in_(3) && (!(o_ptr->art_flags3 & TR3_NO_MAGIC)))
			{
				o_ptr->art_flags3 |= TR3_NO_MAGIC;
				if (one_in_(2)) return;
			}
		}
		else if (artifact_bias == BIAS_NECROMANTIC)
		{
			if (!(o_ptr->art_flags2 & TR2_RES_NETHER))
			{
				o_ptr->art_flags2 |= TR2_RES_NETHER;
				if (one_in_(2)) return;
			}
			if (!(o_ptr->art_flags2 & TR2_RES_POIS))
			{
				o_ptr->art_flags2 |= TR2_RES_POIS;
				if (one_in_(2)) return;
			}
			if (!(o_ptr->art_flags2 & TR2_RES_DARK))
			{
				o_ptr->art_flags2 |= TR2_RES_DARK;
				if (one_in_(2)) return;
			}
		}
		else if (artifact_bias == BIAS_CHAOS)
		{
			if (!(o_ptr->art_flags2 & TR2_RES_CHAOS))
			{
				o_ptr->art_flags2 |= TR2_RES_CHAOS;
				if (one_in_(2)) return;
			}
			if (!(o_ptr->art_flags2 & TR2_RES_CONF))
			{
				o_ptr->art_flags2 |= TR2_RES_CONF;
				if (one_in_(2)) return;
			}
			if (!(o_ptr->art_flags2 & TR2_RES_DISEN))
			{
				o_ptr->art_flags2 |= TR2_RES_DISEN;
				if (one_in_(2)) return;
			}
		}
	}

	switch (specific ? specific : randint1(42))
	{
		case 1:
			if (!one_in_(WEIRD_LUCK))
				random_resistance(o_ptr, is_scroll, specific);
			else
			{
				o_ptr->art_flags2 |= TR2_IM_ACID;
				/*  if (is_scroll) msg_print("It looks totally incorruptible."); */
				if (!artifact_bias)
					artifact_bias = BIAS_ACID;
			}
			break;
		case 2:
			if (!one_in_(WEIRD_LUCK))
				random_resistance(o_ptr, is_scroll, specific);
			else
			{
				o_ptr->art_flags2 |= TR2_IM_ELEC;
				/*  if (is_scroll) msg_print("It looks completely grounded."); */
				if (!artifact_bias)
					artifact_bias = BIAS_ELEC;
			}
			break;
		case 3:
			if (!one_in_(WEIRD_LUCK))
				random_resistance(o_ptr, is_scroll, specific);
			else
			{
				o_ptr->art_flags2 |= TR2_IM_COLD;
				/*  if (is_scroll) msg_print("It feels very warm."); */
				if (!artifact_bias)
					artifact_bias = BIAS_COLD;
			}
			break;
		case 4:
			if (!one_in_(WEIRD_LUCK))
				random_resistance(o_ptr, is_scroll, specific);
			else
			{
				o_ptr->art_flags2 |= TR2_IM_FIRE;
				/*  if (is_scroll) msg_print("It feels very cool."); */
				if (!artifact_bias)
					artifact_bias = BIAS_FIRE;
			}
			break;
		case 5:
		case 6:
		case 13:
			o_ptr->art_flags2 |= TR2_RES_ACID;
			/*  if (is_scroll) msg_print("It makes your stomach rumble."); */
			if (!artifact_bias)
				artifact_bias = BIAS_ACID;
			break;
		case 7:
		case 8:
		case 14:
			o_ptr->art_flags2 |= TR2_RES_ELEC;
			/*  if (is_scroll) msg_print("It makes you feel grounded."); */
			if (!artifact_bias)
				artifact_bias = BIAS_ELEC;
			break;
		case 9:
		case 10:
		case 15:
			o_ptr->art_flags2 |= TR2_RES_FIRE;
			/*  if (is_scroll) msg_print("It makes you feel cool!");*/
			if (!artifact_bias)
				artifact_bias = BIAS_FIRE;
			break;
		case 11:
		case 12:
		case 16:
			o_ptr->art_flags2 |= TR2_RES_COLD;
			/*  if (is_scroll) msg_print("It makes you feel full of hot air!");*/
			if (!artifact_bias)
				artifact_bias = BIAS_COLD;
			break;
		case 17:
		case 18:
			o_ptr->art_flags2 |= TR2_RES_POIS;
			/*  if (is_scroll) msg_print("It makes breathing easier for you."); */
			if (!artifact_bias && !one_in_(4))
				artifact_bias = BIAS_POIS;
			else if (!artifact_bias && one_in_(2))
				artifact_bias = BIAS_NECROMANTIC;
			else if (!artifact_bias && one_in_(2))
				artifact_bias = BIAS_ROGUE;
			break;
		case 19:
		case 20:
			o_ptr->art_flags2 |= TR2_RES_FEAR;
			/*  if (is_scroll) msg_print("It makes you feel brave!"); */
			if (!artifact_bias && one_in_(3))
				artifact_bias = BIAS_WARRIOR;
			break;
		case 21:
			o_ptr->art_flags2 |= TR2_RES_LITE;
			/*  if (is_scroll) msg_print("It makes everything look darker.");*/
			break;
		case 22:
			o_ptr->art_flags2 |= TR2_RES_DARK;
			/*  if (is_scroll) msg_print("It makes everything look brighter.");*/
			break;
		case 23:
		case 24:
			o_ptr->art_flags2 |= TR2_RES_BLIND;
			/*  if (is_scroll) msg_print("It makes you feel you are wearing glasses.");*/
			break;
		case 25:
		case 26:
			o_ptr->art_flags2 |= TR2_RES_CONF;
			/*  if (is_scroll) msg_print("It makes you feel very determined.");*/
			if (!artifact_bias && one_in_(6))
				artifact_bias = BIAS_CHAOS;
			break;
		case 27:
		case 28:
			o_ptr->art_flags2 |= TR2_RES_SOUND;
			/*  if (is_scroll) msg_print("It makes you feel deaf!");*/
			break;
		case 29:
		case 30:
			o_ptr->art_flags2 |= TR2_RES_SHARDS;
			/*  if (is_scroll) msg_print("It makes your skin feel thicker.");*/
			break;
		case 31:
		case 32:
			o_ptr->art_flags2 |= TR2_RES_NETHER;
			/*  if (is_scroll) msg_print("It makes you feel like visiting a graveyard!");*/
			if (!artifact_bias && one_in_(3))
				artifact_bias = BIAS_NECROMANTIC;
			break;
		case 33:
		case 34:
			o_ptr->art_flags2 |= TR2_RES_NEXUS;
			/*  if (is_scroll) msg_print("It makes you feel normal.");*/
			break;
		case 35:
		case 36:
			o_ptr->art_flags2 |= TR2_RES_CHAOS;
			/*  if (is_scroll) msg_print("It makes you feel very firm.");*/
			if (!artifact_bias && one_in_(2))
				artifact_bias = BIAS_CHAOS;
			break;
		case 37:
		case 38:
			o_ptr->art_flags2 |= TR2_RES_DISEN;
			/*  if (is_scroll) msg_print("It is surrounded by a static feeling.");*/
			break;
		case 39:
			if (o_ptr->tval >= TV_CLOAK && o_ptr->tval <= TV_HARD_ARMOR)
				o_ptr->art_flags3 |= TR3_SH_ELEC;
			else
				random_resistance(o_ptr, is_scroll, specific);
			if (!artifact_bias)
				artifact_bias = BIAS_ELEC;
			break;
		case 40:
			if (o_ptr->tval >= TV_CLOAK && o_ptr->tval <= TV_HARD_ARMOR)
				o_ptr->art_flags3 |= TR3_SH_FIRE;
			else
				random_resistance(o_ptr, is_scroll, specific);
			if (!artifact_bias)
				artifact_bias = BIAS_FIRE;
			break;
		case 41:
			if (o_ptr->tval == TV_SHIELD || o_ptr->tval == TV_CLOAK ||
			    o_ptr->tval == TV_HELM || o_ptr->tval == TV_HARD_ARMOR)
				o_ptr->art_flags2 |= TR2_REFLECT;
			else
				random_resistance(o_ptr, is_scroll, specific);
			break;
		case 42:
			if (o_ptr->tval >= TV_CLOAK && o_ptr->tval <= TV_HARD_ARMOR)
				o_ptr->art_flags3 |= TR3_SH_COLD;
			else
				random_resistance(o_ptr, is_scroll, specific);
			if (!artifact_bias)
				artifact_bias = BIAS_COLD;
			break;
	}
}



static void random_misc(object_type * o_ptr, bool is_scroll)
{
	if (artifact_bias == BIAS_RANGER)
	{
		if (!(o_ptr->art_flags2 & TR2_SUST_CON))
		{
			o_ptr->art_flags2 |= TR2_SUST_CON;
			if (one_in_(2)) return;
		}
	}
	else if (artifact_bias == BIAS_STR)
	{
		if (!(o_ptr->art_flags2 & TR2_SUST_STR))
		{
			o_ptr->art_flags2 |= TR2_SUST_STR;
			if (one_in_(2)) return;
		}
	}
	else if (artifact_bias == BIAS_WIS)
	{
		if (!(o_ptr->art_flags2 & TR2_SUST_WIS))
		{
			o_ptr->art_flags2 |= TR2_SUST_WIS;
			if (one_in_(2)) return;
		}
	}
	else if (artifact_bias == BIAS_INT)
	{
		if (!(o_ptr->art_flags2 & TR2_SUST_INT))
		{
			o_ptr->art_flags2 |= TR2_SUST_INT;
			if (one_in_(2)) return;
		}
	}
	else if (artifact_bias == BIAS_DEX)
	{
		if (!(o_ptr->art_flags2 & TR2_SUST_DEX))
		{
			o_ptr->art_flags2 |= TR2_SUST_DEX;
			if (one_in_(2)) return;
		}
	}
	else if (artifact_bias == BIAS_CON)
	{
		if (!(o_ptr->art_flags2 & TR2_SUST_CON))
		{
			o_ptr->art_flags2 |= TR2_SUST_CON;
			if (one_in_(2)) return;
		}
	}
	else if (artifact_bias == BIAS_CHR)
	{
		if (!(o_ptr->art_flags2 & TR2_SUST_CHR))
		{
			o_ptr->art_flags2 |= TR2_SUST_CHR;
			if (one_in_(2)) return;
		}
	}
	else if (artifact_bias == BIAS_CHAOS)
	{
		if (!(o_ptr->art_flags3 & TR3_TELEPORT))
		{
			o_ptr->art_flags3 |= TR3_TELEPORT;
			if (one_in_(2)) return;
		}
	}
	else if (artifact_bias == BIAS_FIRE)
	{
		if (!(o_ptr->art_flags3 & TR3_LITE))
		{
			o_ptr->art_flags3 |= TR3_LITE; /* Freebie */
		}
	}

	switch (randint1(31))
	{
		case 1:
			o_ptr->art_flags2 |= TR2_SUST_STR;
			/*  if (is_scroll) msg_print("It makes you feel you cannot become weaker."); */
			if (!artifact_bias)
				artifact_bias = BIAS_STR;
			break;
		case 2:
			o_ptr->art_flags2 |= TR2_SUST_INT;
			/*  if (is_scroll) msg_print("It makes you feel you cannot become more stupid.");*/
			if (!artifact_bias)
				artifact_bias = BIAS_INT;
			break;
		case 3:
			o_ptr->art_flags2 |= TR2_SUST_WIS;
			/*  if (is_scroll) msg_print("It makes you feel you cannot become simpler.");*/
			if (!artifact_bias)
				artifact_bias = BIAS_WIS;
			break;
		case 4:
			o_ptr->art_flags2 |= TR2_SUST_DEX;
			/*  if (is_scroll) msg_print("It makes you feel you cannot become clumsier.");*/
			if (!artifact_bias)
				artifact_bias = BIAS_DEX;
			break;
		case 5:
			o_ptr->art_flags2 |= TR2_SUST_CON;
			/*  if (is_scroll) msg_print("It makes you feel you cannot become less healthy.");*/
			if (!artifact_bias)
				artifact_bias = BIAS_CON;
			break;
		case 6:
			o_ptr->art_flags2 |= TR2_SUST_CHR;
			/*  if (is_scroll) msg_print("It makes you feel you cannot become uglier.");*/
			if (!artifact_bias)
				artifact_bias = BIAS_CHR;
			break;
		case 7:
		case 8:
		case 14:
			o_ptr->art_flags2 |= TR2_FREE_ACT;
			/*  if (is_scroll) msg_print("It makes you feel like a young rebel!");*/
			break;
		case 9:
			o_ptr->art_flags2 |= TR2_HOLD_LIFE;
			/*  if (is_scroll) msg_print("It makes you feel immortal.");*/
			if (!artifact_bias && one_in_(5))
				artifact_bias = BIAS_PRIESTLY;
			else if (!artifact_bias && one_in_(6))
				artifact_bias = BIAS_NECROMANTIC;
			break;
		case 10:
		case 11:
			o_ptr->art_flags3 |= TR3_LITE;
			/*  if (is_scroll) msg_print("It starts shining.");*/
			break;
		case 12:
		case 13:
			o_ptr->art_flags3 |= TR3_FEATHER;
			/*  if (is_scroll) msg_print("It feels lighter.");*/
			break;
		case 15:
		case 16:
		case 17:
			o_ptr->art_flags3 |= TR3_SEE_INVIS;
			/*  if (is_scroll) msg_print("It makes you see the air!");*/
			break;
		case 18:
			if (one_in_(3)) break;
			o_ptr->art_flags3 |= TR3_TELEPATHY;
			/*  if (is_scroll) msg_print("It makes you hear voices inside your head!");*/
			if (!artifact_bias && one_in_(9))
				artifact_bias = BIAS_MAGE;
			break;
		case 19:
		case 20:
			o_ptr->art_flags3 |= TR3_SLOW_DIGEST;
			/*  if (is_scroll) msg_print("It makes you feel less hungry.");*/
			break;
		case 21:
		case 22:
			o_ptr->art_flags3 |= TR3_REGEN;
			/*  if (is_scroll) msg_print("It looks as good as new.");*/
			break;
		case 23:
			o_ptr->art_flags3 |= TR3_TELEPORT;
			/*  if (is_scroll) msg_print("Its position feels uncertain!");*/
			break;
		case 24:
		case 25:
		case 26:
			if (o_ptr->tval >= TV_BOOTS && o_ptr->tval <= TV_DRAG_ARMOR)
				random_misc(o_ptr, is_scroll);
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
			o_ptr->art_flags3 |= TR3_SHOW_MODS;
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
			o_ptr->art_flags3 |= TR3_NO_MAGIC;
			break;
		case 31:
			o_ptr->art_flags3 |= TR3_NO_TELE;
			break;
	}
}


static void random_slay(object_type *o_ptr, bool is_scroll)
{
	if (artifact_bias == BIAS_CHAOS && (o_ptr->tval != TV_BOW))
	{
		if (!(o_ptr->art_flags1 & TR1_CHAOTIC))
		{
			o_ptr->art_flags1 |= TR1_CHAOTIC;
			if (one_in_(2)) return;
		}
	}

	else if (artifact_bias == BIAS_PRIESTLY &&
	   (o_ptr->tval == TV_SWORD || o_ptr->tval == TV_POLEARM) &&
	  !(o_ptr->art_flags3 & TR3_BLESSED))
	{
		/* A free power for "priestly" random artifacts */
		o_ptr->art_flags3 |= TR3_BLESSED;
	}

	else if (artifact_bias == BIAS_NECROMANTIC && (o_ptr->tval != TV_BOW))
	{
		if (!(o_ptr->art_flags1 & TR1_VAMPIRIC))
		{
			o_ptr->art_flags1 |= TR1_VAMPIRIC;
			if (one_in_(2)) return;
		}
		if (!(o_ptr->art_flags1 & TR1_BRAND_POIS) && one_in_(2))
		{
			o_ptr->art_flags1 |= TR1_BRAND_POIS;
			if (one_in_(2)) return;
		}
	}

	else if (artifact_bias == BIAS_RANGER && (o_ptr->tval != TV_BOW))
	{
		if (!(o_ptr->art_flags1 & TR1_SLAY_ANIMAL))
		{
			o_ptr->art_flags1 |= TR1_SLAY_ANIMAL;
			if (one_in_(2)) return;
		}
	}

	else if (artifact_bias == BIAS_ROGUE && (o_ptr->tval != TV_BOW))
	{
		if ((((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_DAGGER)) ||
		     ((o_ptr->tval == TV_POLEARM) && (o_ptr->sval == SV_SPEAR))) &&
			 !(o_ptr->art_flags2 & TR2_THROW))
		{
			/* Free power for rogues... */
			o_ptr->art_flags2 |= TR2_THROW;
		}
		if (!(o_ptr->art_flags1 & TR1_BRAND_POIS))
		{
			o_ptr->art_flags1 |= TR1_BRAND_POIS;
			if (one_in_(2)) return;
		}
	}

	else if (artifact_bias == BIAS_POIS && (o_ptr->tval != TV_BOW))
	{
		if (!(o_ptr->art_flags1 & TR1_BRAND_POIS))
		{
			o_ptr->art_flags1 |= TR1_BRAND_POIS;
			if (one_in_(2)) return;
		}
	}

	else if (artifact_bias == BIAS_FIRE && (o_ptr->tval != TV_BOW))
	{
		if (!(o_ptr->art_flags1 & TR1_BRAND_FIRE))
		{
			o_ptr->art_flags1 |= TR1_BRAND_FIRE;
			if (one_in_(2)) return;
		}
	}

	else if (artifact_bias == BIAS_COLD && (o_ptr->tval != TV_BOW))
	{
		if (!(o_ptr->art_flags1 & TR1_BRAND_COLD))
		{
			o_ptr->art_flags1 |= TR1_BRAND_COLD;
			if (one_in_(2)) return;
		}
	}

	else if (artifact_bias == BIAS_ELEC && (o_ptr->tval != TV_BOW))
	{
		if (!(o_ptr->art_flags1 & TR1_BRAND_ELEC))
		{
			o_ptr->art_flags1 |= TR1_BRAND_ELEC;
			if (one_in_(2)) return;
		}
	}

	else if (artifact_bias == BIAS_ACID && (o_ptr->tval != TV_BOW))
	{
		if (!(o_ptr->art_flags1 & TR1_BRAND_ACID))
		{
			o_ptr->art_flags1 |= TR1_BRAND_ACID;
			if (one_in_(2)) return;
		}
	}

	else if (artifact_bias == BIAS_LAW && (o_ptr->tval != TV_BOW))
	{
		if (!(o_ptr->art_flags1 & TR1_SLAY_EVIL))
		{
			o_ptr->art_flags1 |= TR1_SLAY_EVIL;
			if (one_in_(2)) return;
		}
		if (!(o_ptr->art_flags1 & TR1_SLAY_UNDEAD))
		{
			o_ptr->art_flags1 |= TR1_SLAY_UNDEAD;
			if (one_in_(2)) return;
		}
		if (!(o_ptr->art_flags1 & TR1_SLAY_DEMON))
		{
			o_ptr->art_flags1 |= TR1_SLAY_DEMON;
			if (one_in_(2)) return;
		}
	}

	if (o_ptr->tval != TV_BOW)
	{
		switch (randint1(34))
		{
			case 1:
			case 2:
				o_ptr->art_flags1 |= TR1_SLAY_ANIMAL;
				/*  if (is_scroll) msg_print("You start hating animals.");*/
				break;
			case 3:
			case 4:
				o_ptr->art_flags1 |= TR1_SLAY_EVIL;
				/*  if (is_scroll) msg_print("You hate evil creatures.");*/
				if (!artifact_bias && one_in_(2))
					artifact_bias = BIAS_LAW;
				else if (!artifact_bias && one_in_(9))
					artifact_bias = BIAS_PRIESTLY;
				break;
			case 5:
			case 6:
				o_ptr->art_flags1 |= TR1_SLAY_UNDEAD;
				/*  if (is_scroll) msg_print("You hate undead creatures.");*/
				if (!artifact_bias && one_in_(9))
					artifact_bias = BIAS_PRIESTLY;
				break;
			case 7:
			case 8:
				o_ptr->art_flags1 |= TR1_SLAY_DEMON;
				/*  if (is_scroll) msg_print("You hate demons.");*/
				if (!artifact_bias && one_in_(9))
					artifact_bias = BIAS_PRIESTLY;
				break;
			case 9:
			case 10:
				o_ptr->art_flags1 |= TR1_SLAY_ORC;
				/*  if (is_scroll) msg_print("You hate orcs.");*/
				break;
			case 11:
			case 12:
				o_ptr->art_flags1 |= TR1_SLAY_TROLL;
				/*  if (is_scroll) msg_print("You hate trolls.");*/
				break;
			case 13:
			case 14:
				o_ptr->art_flags1 |= TR1_SLAY_GIANT;
				/*  if (is_scroll) msg_print("You hate giants.");*/
				break;
			case 15:
			case 16:
				o_ptr->art_flags1 |= TR1_SLAY_DRAGON;
				/*  if (is_scroll) msg_print("You hate dragons.");*/
				break;
			case 17:
				o_ptr->art_flags1 |= TR1_KILL_DRAGON;
				/*  if (is_scroll) msg_print("You feel an intense hatred of dragons.");*/
				break;
			case 18:
			case 19:
				if (o_ptr->tval == TV_SWORD)
				{
					o_ptr->art_flags1 |= TR1_VORPAL;
					/* if (is_scroll) msg_print("It looks extremely sharp!");*/
					if (!artifact_bias && one_in_(9))
						artifact_bias = BIAS_WARRIOR;
				}
				else
					random_slay(o_ptr, is_scroll);
				break;
			case 20:
				o_ptr->art_flags1 |= TR1_IMPACT;
				/* if (is_scroll) msg_print("The ground trembles beneath you.");*/
				break;
			case 21:
			case 22:
				o_ptr->art_flags1 |= TR1_BRAND_FIRE;
				/*  if (is_scroll) msg_print("It feels hot!");*/
				if (!artifact_bias)
					artifact_bias = BIAS_FIRE;
				break;
			case 23:
			case 24:
				o_ptr->art_flags1 |= TR1_BRAND_COLD;
				/*  if (is_scroll) msg_print("It feels cold!");*/
				if (!artifact_bias)
					artifact_bias = BIAS_COLD;
				break;
			case 25:
			case 26:
				o_ptr->art_flags1 |= TR1_BRAND_ELEC;
				/*  if (is_scroll) msg_print("Ouch! You get zapped!");*/
				if (!artifact_bias)
					artifact_bias = BIAS_ELEC;
				break;
			case 27:
			case 28:
				o_ptr->art_flags1 |= TR1_BRAND_ACID;
				/*  if (is_scroll) msg_print("Its smell makes you feel dizzy.");*/
				if (!artifact_bias)
					artifact_bias = BIAS_ACID;
				break;
			case 29:
			case 30:
				o_ptr->art_flags1 |= TR1_BRAND_POIS;
				/*  if (is_scroll) msg_print("It smells rotten.");*/
				if (!artifact_bias && !one_in_(3))
					artifact_bias = BIAS_POIS;
				else if (!artifact_bias && one_in_(6))
					artifact_bias = BIAS_NECROMANTIC;
				else if (!artifact_bias)
					artifact_bias = BIAS_ROGUE;
				break;
			case 31:
				o_ptr->art_flags1 |= TR1_VAMPIRIC;
				/*  if (is_scroll) msg_print("You think it bit you!");*/
				if (!artifact_bias)
					artifact_bias = BIAS_NECROMANTIC;
				break;
			case 32:
				o_ptr->art_flags1 |= TR1_FORCE_WEAPON;
				/*  if (is_scroll) msg_print("It looks consuming your MP!");*/
				if (!artifact_bias)
					artifact_bias = (one_in_(2) ? BIAS_MAGE : BIAS_PRIESTLY);
				break;
			default:
				o_ptr->art_flags1 |= TR1_CHAOTIC;
				/*  if (is_scroll) msg_print("It looks very confusing.");*/
				if (!artifact_bias)
					artifact_bias = BIAS_CHAOS;
				break;
		}
	}
	else
	{
		switch (randint1(6))
		{
			case 1:
			case 2:
			case 3:
				o_ptr->art_flags3 |= TR3_XTRA_MIGHT;
				if (!one_in_(7)) o_ptr->art_flags3 &= ~(TR3_XTRA_SHOTS);
				/*  if (is_scroll) msg_print("It looks mightier than before."); */
				if (!artifact_bias && one_in_(9))
					artifact_bias = BIAS_RANGER;
				break;
			default:
				o_ptr->art_flags3 |= TR3_XTRA_SHOTS;
				if (!one_in_(7)) o_ptr->art_flags3 &= ~(TR3_XTRA_MIGHT);
				/*  if (is_scroll) msg_print("It seems faster!"); */
				if (!artifact_bias && one_in_(9))
					artifact_bias = BIAS_RANGER;
			break;
		}
	}
}


static void give_activation_power(object_type *o_ptr)
{
	int type = 0, chance = 0;

	if (artifact_bias)
	{
		if (artifact_bias == BIAS_ELEC)
		{
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
		}
		else if (artifact_bias == BIAS_POIS)
		{
			type = ACT_BA_POIS_1;
			chance = 101;
		}
		else if (artifact_bias == BIAS_FIRE)
		{
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
		}
		else if (artifact_bias == BIAS_COLD)
		{
			chance = 101;
			if (!one_in_(3))
				type = ACT_BO_COLD_1;
			else if (!one_in_(3))
				type = ACT_BA_COLD_1;
			else if (!one_in_(3))
				type = ACT_BA_COLD_2;
			else
				type = ACT_BA_COLD_3;
		}
		else if (artifact_bias == BIAS_CHAOS)
		{
			chance = 50;
			if (one_in_(6))
				type = ACT_SUMMON_DEMON;
			else
				type = ACT_CALL_CHAOS;
		}
		else if (artifact_bias == BIAS_PRIESTLY)
		{
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
		}
		else if (artifact_bias == BIAS_NECROMANTIC)
		{
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
		}
		else if (artifact_bias == BIAS_LAW)
		{
			chance = 101;
			if (one_in_(8))
				type = ACT_BANISH_EVIL;
			else if (one_in_(4))
				type = ACT_DISP_EVIL;
			else
				type = ACT_PROT_EVIL;
		}
		else if (artifact_bias == BIAS_ROGUE)
		{
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
		}
		else if (artifact_bias == BIAS_MAGE)
		{
			chance = 66;
			if (one_in_(20))
				type = ACT_SUMMON_ELEMENTAL;
			else if (one_in_(10))
				type = ACT_SUMMON_PHANTOM;
			else if (one_in_(5))
				type = ACT_RUNE_EXPLO;
			else
				type = ACT_ESP;
		}
		else if (artifact_bias == BIAS_WARRIOR)
		{
			chance = 80;
			if (one_in_(100))
				type = ACT_INVULN;
			else
				type = ACT_BERSERK;
		}
		else if (artifact_bias == BIAS_RANGER)
		{
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
		}
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
	o_ptr->art_flags3 |= TR3_ACTIVATE;
	o_ptr->timeout = 0;
}


static void get_random_name(char *return_name, bool armour, int power)
{
	if (randint1(100) <= TABLE_NAME)
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
 if(return_name[0]==0)get_table_name(return_name);
#endif
	}
}


bool create_artifact(object_type *o_ptr, bool a_scroll)
{
	char    new_name[1024];
	int     has_pval = 0;
	int     powers = randint1(5) + 1;
	int     max_type = (o_ptr->tval < TV_BOOTS ? 7 : 5);
	int     power_level;
	s32b    total_flags;
	bool    a_cursed = FALSE;
	int     warrior_artifact_bias = 0;


	artifact_bias = 0;

	/* Nuke enchantments */
	o_ptr->name1 = 0;
	o_ptr->name2 = 0;

	o_ptr->art_flags1 |= k_info[o_ptr->k_idx].flags1;
	o_ptr->art_flags2 |= k_info[o_ptr->k_idx].flags2;
	o_ptr->art_flags3 |= k_info[o_ptr->k_idx].flags3;
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
	if (((o_ptr->tval == TV_AMULET) || (o_ptr->tval == TV_RING)) && cursed_p(o_ptr))
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
				random_plus(o_ptr, a_scroll);
				has_pval = TRUE;
				break;
			case 3: case 4:
				if (one_in_(2) && (o_ptr->tval < TV_BOOTS) && (o_ptr->tval != TV_BOW))
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
					random_resistance(o_ptr, a_scroll, FALSE);
				break;
			case 5:
				random_misc(o_ptr, a_scroll);
				break;
			case 6: case 7:
				random_slay(o_ptr, a_scroll);
				break;
			default:
				if (wizard) msg_print("Switch error in create_artifact!");
				powers++;
		}
	};

	if (has_pval)
	{
#if 0
		o_ptr->art_flags3 |= TR3_SHOW_MODS;

		/* This one commented out by gw's request... */
		if (!a_scroll)
			o_ptr->art_flags3 |= TR3_HIDE_TYPE;
#endif

		if (o_ptr->art_flags1 & TR1_BLOWS)
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
	if (o_ptr->tval >= TV_BOOTS && o_ptr->tval <= TV_DRAG_ARMOR)
		o_ptr->to_a += randint1(o_ptr->to_a > 19 ? 1 : 20 - o_ptr->to_a);
	else if (o_ptr->tval <= TV_SWORD)
	{
		o_ptr->to_h += randint1(o_ptr->to_h > 19 ? 1 : 20 - o_ptr->to_h);
		o_ptr->to_d += randint1(o_ptr->to_d > 19 ? 1 : 20 - o_ptr->to_d);
		if ((o_ptr->art_flags1 & TR1_WIS) && (o_ptr->pval > 0)) o_ptr->art_flags3 |= TR3_BLESSED;
	}

	/* Just to be sure */
	o_ptr->art_flags3 |= (TR3_IGNORE_ACID | TR3_IGNORE_ELEC |
	                      TR3_IGNORE_FIRE | TR3_IGNORE_COLD);

	total_flags = flag_cost(o_ptr, o_ptr->pval);
	if (cheat_peek) msg_format("%ld", total_flags);

	if (a_cursed) curse_artifact(o_ptr);

	if (!a_cursed &&
	    (randint1((o_ptr->tval >= TV_BOOTS)
	    ? ACTIVATION_CHANCE * 2 : ACTIVATION_CHANCE) == 1))
	{
		o_ptr->xtra2 = 0;
		give_activation_power(o_ptr);
	}

	if ((o_ptr->tval >= TV_BOOTS) && (o_ptr->tval <= TV_DRAG_ARMOR))
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

	if (((artifact_bias == BIAS_MAGE) || (artifact_bias == BIAS_INT)) && (o_ptr->tval == TV_GLOVES)) o_ptr->art_flags2 |= TR2_FREE_ACT;

	if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_DOKUBARI))
	{
		o_ptr->to_h = 0;
		o_ptr->to_d = 0;
		o_ptr->art_flags1 &= ~(TR1_BLOWS);
		o_ptr->art_flags1 &= ~(TR1_FORCE_WEAPON);
		o_ptr->art_flags1 &= ~(TR1_SLAY_ANIMAL);
		o_ptr->art_flags1 &= ~(TR1_SLAY_EVIL);
		o_ptr->art_flags1 &= ~(TR1_SLAY_UNDEAD);
		o_ptr->art_flags1 &= ~(TR1_SLAY_DEMON);
		o_ptr->art_flags1 &= ~(TR1_SLAY_ORC);
		o_ptr->art_flags1 &= ~(TR1_SLAY_TROLL);
		o_ptr->art_flags1 &= ~(TR1_SLAY_GIANT);
		o_ptr->art_flags1 &= ~(TR1_SLAY_DRAGON);
		o_ptr->art_flags1 &= ~(TR1_KILL_DRAGON);
		o_ptr->art_flags1 &= ~(TR1_VORPAL);
		o_ptr->art_flags1 &= ~(TR1_BRAND_POIS);
		o_ptr->art_flags1 &= ~(TR1_BRAND_ACID);
		o_ptr->art_flags1 &= ~(TR1_BRAND_ELEC);
		o_ptr->art_flags1 &= ~(TR1_BRAND_FIRE);
		o_ptr->art_flags1 &= ~(TR1_BRAND_COLD);
	}

	if (o_ptr->tval >= TV_BOOTS)
	{
		if (a_cursed) power_level = 0;
		else if (total_flags < 15000) power_level = 1;
		else if (total_flags < 25000) power_level = 2;
		else power_level = 3;
	}

	else
	{
		if (a_cursed) power_level = 0;
		else if (total_flags < 20000) power_level = 1;
		else if (total_flags < 35000) power_level = 2;
		else power_level = 3;
	}

	if (a_scroll)
	{
		char dummy_name[80];
		strcpy(dummy_name, "");
		(void)identify_fully_aux(o_ptr);

#ifdef JP
		if (!(get_string("このアーティファクトを何と名付けますか？", dummy_name, 80)))
#else
		if (!(get_string("What do you want to call the artifact? ", dummy_name, 80)))
#endif

		{
			get_random_name(new_name, (bool)(o_ptr->tval >= TV_BOOTS), power_level);
		}
		else
		{
#ifdef JP
			strcpy(new_name, "《");
#else
			strcpy(new_name, "'");
#endif

			strcat(new_name, dummy_name);
#ifdef JP
			strcat(new_name, "》という名の");
#else
			strcat(new_name, "'");
#endif

		}
		/* Identify it fully */
		object_aware(o_ptr);
		object_known(o_ptr);

		/* Mark the item as fully known */
		o_ptr->ident |= (IDENT_MENTAL);

		chg_virtue(V_INDIVIDUALISM, 2);
		chg_virtue(V_ENCHANT, 5);

	}
	else
	{
		get_random_name(new_name, (bool)(o_ptr->tval >= TV_BOOTS), power_level);
	}

	if (cheat_xtra)
	{
		if (artifact_bias)
#ifdef JP
msg_format("運の偏ったアーティファクト: %d。", artifact_bias);
#else
			msg_format("Biased artifact: %d.", artifact_bias);
#endif

		else
#ifdef JP
msg_print("アーティファクトに運の偏りなし。");
#else
			msg_print("No bias in artifact.");
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
					if (c_ptr->m_idx && (m_ptr->ml || cave_floor_bold(y, x)))
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
				msg_print("アーティファクトの力が邪悪を打ち倒した！");
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
			(void)summon_specific(-1, py, px, plev, SUMMON_ANIMAL_RANGER, TRUE, TRUE, TRUE, FALSE, FALSE);
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

			(void)summon_specific(-1, py, px, dun_level, SUMMON_PHANTOM, TRUE, TRUE, TRUE, FALSE, FALSE);
			o_ptr->timeout = 200 + randint1(200);
			break;
		}

		case ACT_SUMMON_ELEMENTAL:
		{
			bool pet = one_in_(3);
			bool group = !(pet && (plev < 50));

			if (summon_specific((pet ? -1 : 0), py, px, ((plev * 3) / 2), SUMMON_ELEMENTAL, group, FALSE, pet, FALSE, (bool)(!pet)))
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
			bool group = !(pet && (plev < 50));

			if (summon_specific((pet ? -1 : 0), py, px, ((plev * 3) / 2), SUMMON_DEMON, group, FALSE, pet, FALSE, (bool)(!pet)))
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
			bool group;
			bool unique_okay;
			int type;

			type = (plev > 47 ? SUMMON_HI_UNDEAD : SUMMON_UNDEAD);
			if (pet)
			{
				group = (((plev > 24) && one_in_(3)) ? TRUE : FALSE);
				unique_okay = FALSE;
			}
			else
			{
				group = TRUE;
				unique_okay = TRUE;
			}

			if (summon_specific((pet ? -1 : 0), py, px, ((plev * 3) / 2), type,
					    group, FALSE, pet, unique_okay, (bool)(!pet)))
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

			teleport_player(100);
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


void random_artifact_resistance(object_type * o_ptr)
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
			o_ptr->art_flags3 |=
			    (TR3_CURSED | TR3_HEAVY_CURSE | TR3_AGGRAVATE | TR3_TY_CURSE);
			o_ptr->ident |= IDENT_CURSED;
			return;
		}
	}
	if (o_ptr->name1 == ART_MURAMASA)
	{
		if (p_ptr->pclass != CLASS_SAMURAI)
		{
			o_ptr->art_flags3 |= (TR3_NO_TELE | TR3_NO_MAGIC | TR3_HEAVY_CURSE);
			o_ptr->ident |= IDENT_CURSED;
		}
	}

	if (o_ptr->name1 == ART_XIAOLONG)
	{
		if (p_ptr->pclass == CLASS_MONK)
			o_ptr->art_flags1 |= TR1_BLOWS;
	}

	if (o_ptr->name1 == ART_BLOOD)
	{
		int dummy, i;
		dummy = randint1(2)+randint1(2);
		for (i = 0; i < dummy; i++)
			o_ptr->art_flags1 |= (TR1_CHAOTIC << randint0(18));
		dummy = randint1(2);
		for (i = 0; i < dummy; i++)
			random_resistance(o_ptr, FALSE, randint1(34) + 4);
		dummy = 2;
		for (i = 0; i < dummy; i++)
		{
			int tmp = randint0(11);
			if (tmp < 6) o_ptr->art_flags1 |= (TR1_STR << tmp);
			else o_ptr->art_flags1 |= (TR1_STEALTH << (tmp - 6));
		}
	}

	switch (o_ptr->name1)
	{
		case ART_CELEBORN:
		case ART_ARVEDUI:
		case ART_CASPANION:
		case ART_HITHLOMIR:
		case ART_ROHIRRIM:
		case ART_CELEGORM:
		case ART_ANARION:
		case ART_THRANDUIL:
		case ART_LUTHIEN:
		case ART_THROR:
		case ART_THORIN:
		case ART_NIMTHANC:
		case ART_DETHANC:
		case ART_NARTHANC:
		case ART_STING:
		case ART_TURMIL:
		case ART_THALKETTOTH:
		case ART_JIZO:
			{
				/* Give a resistance */
				give_resistance = TRUE;
			}
			break;
		case ART_MAEDHROS:
		case ART_GLAMDRING:
		case ART_ORCRIST:
		case ART_ANDURIL:
		case ART_ZARCUTHRA:
		case ART_GURTHANG:
		case ART_HARADEKKET:
		case ART_BRAND:
		case ART_DAWN:
		case ART_BUCKLAND:
		case ART_AZAGHAL:
			{
				/* Give a resistance OR a power */
				if (one_in_(2)) give_resistance = TRUE;
				else give_power = TRUE;
			}
			break;
		case ART_NENYA:
		case ART_VILYA:
		case ART_BERUTHIEL:
		case ART_THINGOL:
		case ART_ULMO:
		case ART_OLORIN:
			{
				/* Give a power */
				give_power = TRUE;
			}
			break;
		case ART_CRIMSON:
		case ART_POWER:
		case ART_GONDOR:
		case ART_AULE:
			{
				/* Give both */
				give_power = TRUE;
				give_resistance = TRUE;
			}
			break;
	}

	if (give_power)
	{
		o_ptr->xtra1 = EGO_XTRA_ABILITY;

		/* Randomize the "xtra" power */
		if (o_ptr->xtra1) o_ptr->xtra2 = randint1(256);
	}

	artifact_bias = 0;

	if (give_resistance)
	{
		random_resistance(o_ptr, FALSE, randint1(22) + 16);
	}
}


/*
 * Create the artifact of the specified number
 */
void create_named_art(int a_idx, int y, int x)
{
	object_type forge;
	object_type *q_ptr;
	int i;

	artifact_type *a_ptr = &a_info[a_idx];

	/* Get local object */
	q_ptr = &forge;

	/* Ignore "empty" artifacts */
	if (!a_ptr->name) return;

	/* Acquire the "kind" index */
	i = lookup_kind(a_ptr->tval, a_ptr->sval);

	/* Oops */
	if (!i) return;

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

	/* Hack -- acquire "cursed" flag */
	if (a_ptr->flags3 & TR3_CURSED) q_ptr->ident |= (IDENT_CURSED);

	random_artifact_resistance(q_ptr);

	/* Drop the artifact from heaven */
	(void)drop_near(q_ptr, -1, y, x);
}
