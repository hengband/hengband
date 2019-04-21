#include "angband.h"
#include "util.h"

#include "term.h"
#include "spells.h"

/*
 * Default spell color table (quark index)
 */
TERM_COLOR gf_color[MAX_GF];

/*!
 * @brief 万色表現用にランダムな色を選択する関数 /
 * Get a legal "multi-hued" color for drawing "spells"
 * @param max 色IDの最大値
 * @return 選択した色ID
 */
TERM_COLOR mh_attr(int max)
{
	switch (randint1(max))
	{
	case  1: return (TERM_RED);
	case  2: return (TERM_GREEN);
	case  3: return (TERM_BLUE);
	case  4: return (TERM_YELLOW);
	case  5: return (TERM_ORANGE);
	case  6: return (TERM_VIOLET);
	case  7: return (TERM_L_RED);
	case  8: return (TERM_L_GREEN);
	case  9: return (TERM_L_BLUE);
	case 10: return (TERM_UMBER);
	case 11: return (TERM_L_UMBER);
	case 12: return (TERM_SLATE);
	case 13: return (TERM_WHITE);
	case 14: return (TERM_L_WHITE);
	case 15: return (TERM_L_DARK);
	}

	return (TERM_WHITE);
}


/*!
 * @brief 魔法属性に応じたエフェクトの色を返す /
 * Return a color to use for the bolt/ball spells
 * @param type 魔法属性
 * @return 対応する色ID
 */
TERM_COLOR spell_color(EFFECT_ID type)
{
	/* Check if A.B.'s new graphics should be used (rr9) */
	if (streq(ANGBAND_GRAF, "new") || streq(ANGBAND_GRAF, "ne2"))
	{
		/* Analyze */
		switch (type)
		{
		case GF_PSY_SPEAR:		return (0x06);
		case GF_MISSILE:		return (0x0F);
		case GF_ACID:			return (0x04);
		case GF_ELEC:			return (0x02);
		case GF_FIRE:			return (0x00);
		case GF_COLD:			return (0x01);
		case GF_POIS:			return (0x03);
		case GF_HOLY_FIRE:		return (0x00);
		case GF_HELL_FIRE:		return (0x00);
		case GF_MANA:			return (0x0E);
			/* by henkma */
		case GF_SEEKER:			return (0x0E);
		case GF_SUPER_RAY:		return (0x0E);

		case GF_ARROW:			return (0x0F);
		case GF_WATER:			return (0x04);
		case GF_NETHER:			return (0x07);
		case GF_CHAOS:			return (mh_attr(15));
		case GF_DISENCHANT:		return (0x05);
		case GF_NEXUS:			return (0x0C);
		case GF_CONFUSION:		return (mh_attr(4));
		case GF_SOUND:			return (0x09);
		case GF_SHARDS:			return (0x08);
		case GF_FORCE:			return (0x09);
		case GF_INERTIAL:		return (0x09);
		case GF_GRAVITY:		return (0x09);
		case GF_TIME:			return (0x09);
		case GF_LITE_WEAK:		return (0x06);
		case GF_LITE:			return (0x06);
		case GF_DARK_WEAK:		return (0x07);
		case GF_DARK:			return (0x07);
		case GF_PLASMA:			return (0x0B);
		case GF_METEOR:			return (0x00);
		case GF_ICE:			return (0x01);
		case GF_ROCKET:			return (0x0F);
		case GF_DEATH_RAY:		return (0x07);
		case GF_NUKE:			return (mh_attr(2));
		case GF_DISINTEGRATE:	return (0x05);
		case GF_PSI:
		case GF_PSI_DRAIN:
		case GF_TELEKINESIS:
		case GF_DOMINATION:
		case GF_DRAIN_MANA:
		case GF_MIND_BLAST:
		case GF_BRAIN_SMASH:
			return (0x09);
		case GF_CAUSE_1:
		case GF_CAUSE_2:
		case GF_CAUSE_3:
		case GF_CAUSE_4:		return (0x0E);
		case GF_HAND_DOOM:		return (0x07);
		case GF_CAPTURE:		return (0x0E);
		case GF_IDENTIFY:		return (0x01);
		case GF_ATTACK:			return (0x0F);
		case GF_PHOTO:		return (0x06);
		}
	}
	/* Normal tiles or ASCII */
	else
	{
		TERM_COLOR a;
		SYMBOL_CODE c;

		/* Lookup the default colors for this type */
		concptr s = quark_str(gf_color[type]);

		if (!s) return (TERM_WHITE);

		/* Pick a random color */
		c = s[randint0(strlen(s))];

		/* Lookup this color */
		a = my_strchr(color_char, c) - color_char;

		/* Invalid color (note check for < 0 removed, gave a silly
		 * warning because bytes are always >= 0 -- RG) */
		if (a > 15) return (TERM_WHITE);

		/* Use this color */
		return (a);
	}

	/* Standard "color" */
	return (TERM_WHITE);
}


/*!
 * @brief 始点から終点にかけた方向毎にボルトのキャラクタを返す /
 * Find the attr/char pair to use for a spell effect
 * @param y 始点Y座標
 * @param x 始点X座標
 * @param ny 終点Y座標
 * @param nx 終点X座標
 * @param typ 魔法の効果属性
 * @return 方向キャラID
 * @details
 * <pre>
 * It is moving (or has moved) from (x,y) to (nx,ny).
 * If the distance is not "one", we (may) return "*".
 * </pre>
 */
u16b bolt_pict(POSITION y, POSITION x, POSITION ny, POSITION nx, EFFECT_ID typ)
{
	int base;

	byte k;

	TERM_COLOR a;
	SYMBOL_CODE c;

	/* No motion (*) */
	if ((ny == y) && (nx == x)) base = 0x30;

	/* Vertical (|) */
	else if (nx == x) base = 0x40;

	/* Horizontal (-) */
	else if (ny == y) base = 0x50;

	/* Diagonal (/) */
	else if ((ny - y) == (x - nx)) base = 0x60;

	/* Diagonal (\) */
	else if ((ny - y) == (nx - x)) base = 0x70;

	/* Weird (*) */
	else base = 0x30;

	/* Basic spell color */
	k = spell_color(typ);

	/* Obtain attr/char */
	a = misc_to_attr[base + k];
	c = misc_to_char[base + k];

	/* Create pict */
	return (PICT(a, c));
}




/*!
 * @brief シンボル1文字をカラーIDに変更する /
 * Convert a "color letter" into an "actual" color
 * The colors are: dwsorgbuDWvyRGBU, as shown below
 * @param c シンボル文字
 * @return カラーID
 */
TERM_COLOR color_char_to_attr(SYMBOL_CODE c)
{
	switch (c)
	{
	case 'd': return (TERM_DARK);
	case 'w': return (TERM_WHITE);
	case 's': return (TERM_SLATE);
	case 'o': return (TERM_ORANGE);
	case 'r': return (TERM_RED);
	case 'g': return (TERM_GREEN);
	case 'b': return (TERM_BLUE);
	case 'u': return (TERM_UMBER);

	case 'D': return (TERM_L_DARK);
	case 'W': return (TERM_L_WHITE);
	case 'v': return (TERM_VIOLET);
	case 'y': return (TERM_YELLOW);
	case 'R': return (TERM_L_RED);
	case 'G': return (TERM_L_GREEN);
	case 'B': return (TERM_L_BLUE);
	case 'U': return (TERM_L_UMBER);
	}

	return (255);
}


