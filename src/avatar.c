/* File: avatar.c */

/*
 * Purpose: Enable an Ultima IV style "avatar" game where you try to
 * achieve perfection in various virtues.
 *
 * Topi Ylinen 1998
 * f1toyl@uta.fi
 * topi.ylinen@noodi.fi
 *
 */

/*
 * Copyright (c) 1989 James E. Wilson, Christopher J. Stuart
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "angband.h"


/* The names of the virtues */

cptr virtue[MAX_VIRTUE] =
{
#ifdef JP
	"¾ð",
	"ÍÀ",
	"Àµ",
	"µ¾",
	"¼±",
	"À¿",
	"·¼",
	"Èë",
	"±¿",
	"Á³",
	"Ä´",
	"³è",
	"»à",
	"Ç¦",
	"Àá",
	"¶Ð",
	"Í¦",
	"¸Ä",
#else
	"Compassion",
	"Honour",
	"Justice",
	"Sacrifice",
	"Knowledge",
	"Faith",
	"Enlightenment",
	"Mysticism",
	"Chance",
	"Nature",
	"Harmony",
	"Vitality",
	"Unlife",
	"Patience",
	"Temperance",
	"Diligence",
	"Valour",
	"Individualism",
#endif
};


bool compare_virtue(int type, int num, int tekitou)
{
	int vir;
	if (virtue_number(type))
		vir = p_ptr->virtues[virtue_number(type) - 1];
	else
		vir = 0;

	switch (tekitou)
	{
	case VIRTUE_LARGE:
		if (vir > num) return TRUE;
		else return FALSE;
	case VIRTUE_SMALL:
		if (vir < num) return TRUE;
		else return FALSE;
	}

	return FALSE;
}


/* Aux function */

int virtue_number(int type)
{
	int i;

	/* Search */
	for (i = 0; i < 8; i++)
	{
		if (p_ptr->vir_types[i] == type) return i + 1;
	}

	/* No match */
	return 0;
}

/* Aux function */

static void get_random_virtue(int which)
{
	int type = 0;

	/* Randomly choose a type */
	while (!(type) || virtue_number(type))
	{
		switch (randint1(29))
		{
		case 1: case 2: case 3:
			type = V_SACRIFICE;
			break;
		case 4: case 5: case 6:
			type = V_COMPASSION;
			break;
		case 7: case 8: case 9: case 10: case 11: case 12:
			type = V_VALOUR;
			break;
		case 13: case 14: case 15: case 16: case 17:
			type = V_HONOUR;
			break;
		case 18: case 19: case 20: case 21:
			type = V_JUSTICE;
			break;
		case 22: case 23:
			type = V_TEMPERANCE;
			break;
		case 24: case 25:
			type = V_HARMONY;
			break;
		case 26: case 27: case 28:
			type = V_PATIENCE;
			break;
		default:
			type = V_DILIGENCE;
			break;
		}
	}

	/* Chosen */
	p_ptr->vir_types[which] = type;
}

static s16b get_realm_virtues(byte realm)
{
	switch (realm)
	{
	case REALM_LIFE:
		if (virtue_number(V_VITALITY)) return V_TEMPERANCE;
		else return V_VITALITY;
	case REALM_SORCERY:
		if (virtue_number(V_KNOWLEDGE)) return V_ENCHANT;
		else return V_KNOWLEDGE;
	case REALM_NATURE:
		if (virtue_number(V_NATURE)) return V_HARMONY;
		else return V_NATURE;
	case REALM_CHAOS:
		if (virtue_number(V_CHANCE)) return V_INDIVIDUALISM;
		else return V_CHANCE;
	case REALM_DEATH:
		return V_UNLIFE;
	case REALM_TRUMP:
		return V_KNOWLEDGE;
	case REALM_ARCANE:
		return 0;
	case REALM_CRAFT:
		if (virtue_number(V_ENCHANT)) return V_INDIVIDUALISM;
		else return V_ENCHANT;
	case REALM_DAEMON:
		if (virtue_number(V_JUSTICE)) return V_FAITH;
		else return V_JUSTICE;
	case REALM_CRUSADE:
		if (virtue_number(V_JUSTICE)) return V_HONOUR;
		else return V_JUSTICE;
	case REALM_HEX:
		if (virtue_number(V_COMPASSION)) return V_JUSTICE;
		else return V_COMPASSION;
	};

	return 0;
}

/* Select virtues & reset values for a new character */

void get_virtues(void)
{
	int i = 0, j = 0;
	s16b tmp_vir;

	/* Reset */
	for (i = 0; i < 8; i++)
	{
		p_ptr->virtues[i] = 0;
		p_ptr->vir_types[i] = 0;
	}

	i = 0;

	/* Get pre-defined types */
	/* 1 or more virtues based on class */
	switch (p_ptr->pclass)
	{
	case CLASS_WARRIOR:
	case CLASS_SAMURAI:
		p_ptr->vir_types[i++] = V_VALOUR;
		p_ptr->vir_types[i++] = V_HONOUR;
		break;
	case CLASS_MAGE:
		p_ptr->vir_types[i++] = V_KNOWLEDGE;
		p_ptr->vir_types[i++] = V_ENCHANT;
		break;
	case CLASS_PRIEST:
		p_ptr->vir_types[i++] = V_FAITH;
		p_ptr->vir_types[i++] = V_TEMPERANCE;
		break;
	case CLASS_ROGUE:
	case CLASS_SNIPER:
		p_ptr->vir_types[i++] = V_HONOUR;
		break;
	case CLASS_RANGER:
	case CLASS_ARCHER:
		p_ptr->vir_types[i++] = V_NATURE;
		p_ptr->vir_types[i++] = V_TEMPERANCE;
		break;
	case CLASS_PALADIN:
		p_ptr->vir_types[i++] = V_JUSTICE;
		p_ptr->vir_types[i++] = V_VALOUR;
		p_ptr->vir_types[i++] = V_HONOUR;
		p_ptr->vir_types[i++] = V_FAITH;
		break;
	case CLASS_WARRIOR_MAGE:
	case CLASS_RED_MAGE:
		p_ptr->vir_types[i++] = V_ENCHANT;
		p_ptr->vir_types[i++] = V_VALOUR;
		break;
	case CLASS_CHAOS_WARRIOR:
		p_ptr->vir_types[i++] = V_CHANCE;
		p_ptr->vir_types[i++] = V_INDIVIDUALISM;
		break;
	case CLASS_MONK:
	case CLASS_FORCETRAINER:
		p_ptr->vir_types[i++] = V_FAITH;
		p_ptr->vir_types[i++] = V_HARMONY;
		p_ptr->vir_types[i++] = V_TEMPERANCE;
		p_ptr->vir_types[i++] = V_PATIENCE;
		break;
	case CLASS_MINDCRAFTER:
	case CLASS_MIRROR_MASTER:
		p_ptr->vir_types[i++] = V_HARMONY;
		p_ptr->vir_types[i++] = V_ENLIGHTEN;
		p_ptr->vir_types[i++] = V_PATIENCE;
		break;
	case CLASS_HIGH_MAGE:
	case CLASS_SORCERER:
		p_ptr->vir_types[i++] = V_ENLIGHTEN;
		p_ptr->vir_types[i++] = V_ENCHANT;
		p_ptr->vir_types[i++] = V_KNOWLEDGE;
		break;
	case CLASS_TOURIST:
		p_ptr->vir_types[i++] = V_ENLIGHTEN;
		p_ptr->vir_types[i++] = V_CHANCE;
		break;
	case CLASS_IMITATOR:
		p_ptr->vir_types[i++] = V_CHANCE;
		break;
	case CLASS_BLUE_MAGE:
		p_ptr->vir_types[i++] = V_CHANCE;
		p_ptr->vir_types[i++] = V_KNOWLEDGE;
		break;
	case CLASS_BEASTMASTER:
		p_ptr->vir_types[i++] = V_NATURE;
		p_ptr->vir_types[i++] = V_CHANCE;
		p_ptr->vir_types[i++] = V_VITALITY;
		break;
	case CLASS_MAGIC_EATER:
		p_ptr->vir_types[i++] = V_ENCHANT;
		p_ptr->vir_types[i++] = V_KNOWLEDGE;
		break;
	case CLASS_BARD:
		p_ptr->vir_types[i++] = V_HARMONY;
		p_ptr->vir_types[i++] = V_COMPASSION;
		break;
	case CLASS_CAVALRY:
		p_ptr->vir_types[i++] = V_VALOUR;
		p_ptr->vir_types[i++] = V_HARMONY;
		break;
	case CLASS_BERSERKER:
		p_ptr->vir_types[i++] = V_VALOUR;
		p_ptr->vir_types[i++] = V_INDIVIDUALISM;
		break;
	case CLASS_SMITH:
		p_ptr->vir_types[i++] = V_HONOUR;
		p_ptr->vir_types[i++] = V_KNOWLEDGE;
		break;
	case CLASS_NINJA:
		p_ptr->vir_types[i++] = V_PATIENCE;
		p_ptr->vir_types[i++] = V_KNOWLEDGE;
		p_ptr->vir_types[i++] = V_FAITH;
		p_ptr->vir_types[i++] = V_UNLIFE;
		break;
	};

	/* Get one virtue based on race */
	switch (p_ptr->prace)
	{
	case RACE_HUMAN: case RACE_HALF_ELF: case RACE_DUNADAN:
		p_ptr->vir_types[i++] = V_INDIVIDUALISM;
		break;
	case RACE_ELF: case RACE_SPRITE: case RACE_ENT:
		p_ptr->vir_types[i++] = V_NATURE;
		break;
	case RACE_HOBBIT: case RACE_HALF_OGRE:
		p_ptr->vir_types[i++] = V_TEMPERANCE;
		break;
	case RACE_DWARF: case RACE_KLACKON: case RACE_ANDROID:
		p_ptr->vir_types[i++] = V_DILIGENCE;
		break;
	case RACE_GNOME: case RACE_CYCLOPS:
		p_ptr->vir_types[i++] = V_KNOWLEDGE;
		break;
	case RACE_HALF_ORC: case RACE_AMBERITE: case RACE_KOBOLD:
		p_ptr->vir_types[i++] = V_HONOUR;
		break;
	case RACE_HALF_TROLL: case RACE_BARBARIAN:
		p_ptr->vir_types[i++] = V_VALOUR;
		break;
	case RACE_HIGH_ELF: case RACE_KUTAR:
		p_ptr->vir_types[i++] = V_VITALITY;
		break;
	case RACE_HALF_GIANT: case RACE_GOLEM: case RACE_ANGEL: case RACE_DEMON:
		p_ptr->vir_types[i++] = V_JUSTICE;
		break;
	case RACE_HALF_TITAN:
		p_ptr->vir_types[i++] = V_HARMONY;
		break;
	case RACE_YEEK:
		p_ptr->vir_types[i++] = V_SACRIFICE;
		break;
	case RACE_MIND_FLAYER:
		p_ptr->vir_types[i++] = V_ENLIGHTEN;
		break;
	case RACE_DARK_ELF: case RACE_DRACONIAN: case RACE_S_FAIRY:
		p_ptr->vir_types[i++] = V_ENCHANT;
		break;
	case RACE_NIBELUNG:
		p_ptr->vir_types[i++] = V_PATIENCE;
		break;
	case RACE_IMP:
		p_ptr->vir_types[i++] = V_FAITH;
		break;
	case RACE_ZOMBIE: case RACE_SKELETON:
	case RACE_VAMPIRE: case RACE_SPECTRE:
		p_ptr->vir_types[i++] = V_UNLIFE;
		break;
	case RACE_BEASTMAN:
		p_ptr->vir_types[i++] = V_CHANCE;
		break;
	}

	/* Get a virtue for realms */
	if (p_ptr->realm1)
	{
		tmp_vir = get_realm_virtues(p_ptr->realm1);
		if (tmp_vir) p_ptr->vir_types[i++] = tmp_vir;
	}
	if (p_ptr->realm2)
	{
		tmp_vir = get_realm_virtues(p_ptr->realm2);
		if (tmp_vir) p_ptr->vir_types[i++] = tmp_vir;
	}

	/* Eliminate doubles */
	for (i = 0; i < 8; i++)
	{
		for (j = i + 1; j < 8; j++)
		{
			if ((p_ptr->vir_types[j] != 0) && (p_ptr->vir_types[j] == p_ptr->vir_types[i]))
				p_ptr->vir_types[j] = 0;
		}
	}

	/* Fill in the blanks */
	for (i = 0; i < 8; i++)
	{
		if (p_ptr->vir_types[i] == 0) get_random_virtue(i);
	}
}

void chg_virtue(int virtue, int amount)
{
	int i = 0;

	for (i = 0; i < 8; i++)
	{
		if (p_ptr->vir_types[i] == virtue)
		{
			if (amount > 0)
			{
				if ((amount + p_ptr->virtues[i] > 50) && one_in_(2))
				{
					p_ptr->virtues[i] = MAX(p_ptr->virtues[i], 50);
					return;
				}
				if ((amount + p_ptr->virtues[i] > 80) && one_in_(2))
				{
					p_ptr->virtues[i] = MAX(p_ptr->virtues[i], 80);
					return;
				}
				if ((amount + p_ptr->virtues[i] > 100) && one_in_(2))
				{
					p_ptr->virtues[i] = MAX(p_ptr->virtues[i], 100);
					return;
				}
				if (amount + p_ptr->virtues[i] > 125)
					p_ptr->virtues[i] = 125;
				else
					p_ptr->virtues[i] = p_ptr->virtues[i] + amount;
			}
			else
			{
				if ((amount + p_ptr->virtues[i] < -50) && one_in_(2))
				{
					p_ptr->virtues[i] = MIN(p_ptr->virtues[i], -50);
					return;
				}
				if ((amount + p_ptr->virtues[i] < -80) && one_in_(2))
				{
					p_ptr->virtues[i] = MIN(p_ptr->virtues[i], -80);
					return;
				}
				if ((amount + p_ptr->virtues[i] < -100) && one_in_(2))
				{
					p_ptr->virtues[i] = MIN(p_ptr->virtues[i], -100);
					return;
				}
				if (amount + p_ptr->virtues[i] < -125)
					p_ptr->virtues[i] = -125;
				else
					p_ptr->virtues[i] = p_ptr->virtues[i] + amount;
			}
			p_ptr->update |= (PU_BONUS);
			return;
		}
	}
}

void set_virtue(int virtue, int amount)
{
	int i = 0;

	for (i = 0; i < 8; i++)
	{
		if (p_ptr->vir_types[i] == virtue)
		{
			p_ptr->virtues[i] = amount;
			return;
		}
	}
}

void dump_virtues(FILE *OutFile)
{
	int v_nr = 0;

	if (!OutFile) return;

	for (v_nr = 0; v_nr < 8; v_nr++)
	{
		char v_name [20];
		int tester = p_ptr->virtues[v_nr];

		strcpy(v_name, virtue[(p_ptr->vir_types[v_nr])-1]);

		if (p_ptr->vir_types[v_nr] == 0 || p_ptr->vir_types[v_nr] > MAX_VIRTUE)
#ifdef JP
			fprintf(OutFile, "¤ª¤Ã¤È¡£%s¤Î¾ðÊó¤Ê¤·¡£", v_name);
#else
			fprintf(OutFile, "Oops. No info about %s.", v_name);
#endif

		else if (tester < -100)
#ifdef JP
			fprintf(OutFile, "[%s]¤ÎÂÐ¶Ë",
#else
			fprintf(OutFile, "You are the polar opposite of %s.",
#endif

			        v_name);
		else if (tester < -80)
#ifdef JP
			fprintf(OutFile, "[%s]¤ÎÂçÅ¨",
#else
			fprintf(OutFile, "You are an arch-enemy of %s.",
#endif

			        v_name);
		else if (tester < -60)
#ifdef JP
			fprintf(OutFile, "[%s]¤Î¶¯Å¨",
#else
			fprintf(OutFile, "You are a bitter enemy of %s.",
#endif

			        v_name);
		else if (tester < -40)
#ifdef JP
			fprintf(OutFile, "[%s]¤ÎÅ¨",
#else
			fprintf(OutFile, "You are an enemy of %s.",
#endif

			        v_name);
		else if (tester < -20)
#ifdef JP
			fprintf(OutFile, "[%s]¤Îºá¼Ô",
#else
			fprintf(OutFile, "You have sinned against %s.",
#endif

			        v_name);
		else if (tester < 0)
#ifdef JP
			fprintf(OutFile, "[%s]¤ÎÌÂÆ»¼Ô",
#else
			fprintf(OutFile, "You have strayed from the path of %s.",
#endif

			        v_name);
		else if (tester == 0)
#ifdef JP
			fprintf(OutFile,"[%s]¤ÎÃæÎ©¼Ô",
#else
			fprintf(OutFile,"You are neutral to %s.",
#endif

			        v_name);
		else if (tester < 20)
#ifdef JP
			fprintf(OutFile,"[%s]¤Î¾®ÆÁ¼Ô",
#else
			fprintf(OutFile,"You are somewhat virtuous in %s.",
#endif

			        v_name);
		else if (tester < 40)
#ifdef JP
			fprintf(OutFile,"[%s]¤ÎÃæÆÁ¼Ô",
#else
			fprintf(OutFile,"You are virtuous in %s.",
#endif

			        v_name);
		else if (tester < 60)
#ifdef JP
			fprintf(OutFile,"[%s]¤Î¹âÆÁ¼Ô",
#else
			fprintf(OutFile,"You are very virtuous in %s.",
#endif

			        v_name);
		else if (tester < 80)
#ifdef JP
			fprintf(OutFile,"[%s]¤ÎÇÆ¼Ô",
#else
			fprintf(OutFile,"You are a champion of %s.",
#endif

			        v_name);
		else if (tester < 100)
#ifdef JP
			fprintf(OutFile,"[%s]¤Î°ÎÂç¤ÊÇÆ¼Ô",
#else
			fprintf(OutFile,"You are a great champion of %s.",
#endif

			        v_name);
		else
#ifdef JP
			fprintf(OutFile,"[%s]¤Î¶ñ¸½¼Ô",
#else
			fprintf(OutFile,"You are the living embodiment of %s.",
#endif

			        v_name);

	    fprintf(OutFile, "\n");
	}
}
