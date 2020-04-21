#include "angband.h"
#include "player-race.h"

/*!
 * @brief 変身種族情報
 */
const player_race mimic_info[] =
{
	{
#ifdef JP
		"[標準形態]",
#endif
		"Default",

		{  0,  0,  0,  0,  0,  0 },
		0,  0,  0,  0,  0,  10,  0,  0,
		10,  100,
		0,  0,
		0,  0, 0, 0,
		0,  0, 0, 0,
		0,
		0x000000,
	},
	{
#ifdef JP
		"[悪魔]",
#endif
		"[Demon]",

		{  5,  3,  2,  3,  4,  -6 },
		-5,  18, 20, -2,  3,  10, 40, 20,
		12,  0,
		0,  0,
		0,  0, 0, 0,
		0,  0, 0, 0,
		5,
		0x000003,
	},
	{
#ifdef JP
		"[魔王]",
#endif
		"[Demon lord]",

		{  20,  20,  20,  20,  20,  20 },
		20,  20, 25, -2,  3,  10, 70, 40,
		14,  0,
		0,  0,
		0,  0, 0, 0,
		0,  0, 0, 0,
		20,
		0x000003,
	},
	{
#ifdef JP
		"[吸血鬼]",
#endif
		"[Vampire]",

		{ 4, 4, 1, 1, 2, 3 },
		6, 12, 8, 6, 2, 12, 30, 20,
		11,  0,
		0,  0,
		0,  0, 0, 0,
		0,  0, 0, 0,
		5,
		0x000005,
	},
};

const player_race *rp_ptr;

SYMBOL_CODE get_summon_symbol_from_player(player_type *creature_ptr)
{
	SYMBOL_CODE symbol = 'N';
	switch (creature_ptr->mimic_form)
	{
	case MIMIC_NONE:
		switch (creature_ptr->prace)
		{
		case RACE_HUMAN:
		case RACE_AMBERITE:
		case RACE_BARBARIAN:
		case RACE_BEASTMAN:
		case RACE_DUNADAN:
			symbol = 'p';
			break;
		case RACE_HALF_ELF:
		case RACE_ELF:
		case RACE_HOBBIT:
		case RACE_GNOME:
		case RACE_DWARF:
		case RACE_HIGH_ELF:
		case RACE_NIBELUNG:
		case RACE_DARK_ELF:
		case RACE_MIND_FLAYER:
		case RACE_KUTAR:
		case RACE_S_FAIRY:
			symbol = 'h';
			break;
		case RACE_HALF_ORC:
			symbol = 'o';
			break;
		case RACE_HALF_TROLL:
			symbol = 'T';
			break;
		case RACE_HALF_OGRE:
			symbol = 'O';
			break;
		case RACE_HALF_GIANT:
		case RACE_HALF_TITAN:
		case RACE_CYCLOPS:
			symbol = 'P';
			break;
		case RACE_YEEK:
			symbol = 'y';
			break;
		case RACE_KLACKON:
			symbol = 'K';
			break;
		case RACE_KOBOLD:
			symbol = 'k';
			break;
		case RACE_IMP:
			if (one_in_(13)) symbol = 'U';
			else symbol = 'u';
			break;
		case RACE_DRACONIAN:
			symbol = 'd';
			break;
		case RACE_GOLEM:
		case RACE_ANDROID:
			symbol = 'g';
			break;
		case RACE_SKELETON:
			if (one_in_(13)) symbol = 'L';
			else symbol = 's';
			break;
		case RACE_ZOMBIE:
			symbol = 'z';
			break;
		case RACE_VAMPIRE:
			symbol = 'V';
			break;
		case RACE_SPECTRE:
			symbol = 'G';
			break;
		case RACE_SPRITE:
			symbol = 'I';
			break;
		case RACE_ENT:
			symbol = '#';
			break;
		case RACE_ANGEL:
			symbol = 'A';
			break;
		case RACE_DEMON:
			symbol = 'U';
			break;
		default:
			symbol = 'p';
			break;
		}
		break;
	case MIMIC_DEMON:
		if (one_in_(13)) symbol = 'U';
		else symbol = 'u';
		break;
	case MIMIC_DEMON_LORD:
		symbol = 'U';
		break;
	case MIMIC_VAMPIRE:
		symbol = 'V';
		break;
	}
	return symbol;
}
