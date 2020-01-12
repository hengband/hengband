/*!
 * @file files.c
 * @brief ファイル入出力管理 / Purpose: code dealing with files (and death)
 * @date 2014/01/28
 * @author
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2014 Deskull rearranged comment for Doxygen.\n
 * </pre>
 */

#include "angband.h"
#include "util.h"
#include "files.h"
#include "core.h"

#include "birth.h"
#include "files.h"

#include "bldg.h"

#include "cmd-magiceat.h"
#include "cmd-dump.h"
#include "world.h"
#include "player-inventory.h"
#include "player-race.h"
#include "player-status.h"
#include "player-move.h"
#include "player-class.h"
#include "player-skill.h"
#include "player-personality.h"
#include "player-sex.h"
#include "player-effects.h"
#include "sort.h"
#include "mutation.h"
#include "quest.h"
#include "store.h"
#include "artifact.h"
#include "avatar.h"
#include "shoot.h"
#include "patron.h"
#include "monster.h"
#include "monster-process.h"
#include "monster-status.h"
#include "object-flavor.h"
#include "object-hook.h"
#include "realm.h"
#include "realm-hex.h"
#include "cmd-pet.h"
#include "spells.h"
#include "term.h"
#include "view-mainwindow.h"
#include "floor-events.h"
#include "floor-town.h"
#include "dungeon-file.h"
#include "init.h"
#include "monster-spell.h"
#include "floor.h"
#include "dungeon.h"
#include "melee.h"
#include "objectkind.h"
#include "autopick.h"
#include "save.h"
#include "realm-song.h"

#define PREF_TYPE_NORMAL   0
#define PREF_TYPE_AUTOPICK 1
#define PREF_TYPE_HISTPREF 2

 /* Mode flags for displaying player flags */
#define DP_CURSE   0x01
#define DP_IMM     0x02
#define DP_WP      0x08

#define GRAVE_LINE_WIDTH 31

concptr ANGBAND_DIR; //!< Path name: The main "lib" directory This variable is not actually used anywhere in the code
concptr ANGBAND_DIR_APEX; //!< High score files (binary) These files may be portable between platforms
concptr ANGBAND_DIR_BONE; //!< Bone files for player ghosts (ascii) These files are portable between platforms
concptr ANGBAND_DIR_DATA; //!< Binary image files for the "*_info" arrays (binary) These files are not portable between platforms
concptr ANGBAND_DIR_EDIT; //!< Textual template files for the "*_info" arrays (ascii) These files are portable between platforms
concptr ANGBAND_DIR_SCRIPT; //!< Script files These files are portable between platforms.
concptr ANGBAND_DIR_FILE; //!< Various extra files (ascii) These files may be portable between platforms
concptr ANGBAND_DIR_HELP; //!< Help files (normal) for the online help (ascii) These files are portable between platforms
concptr ANGBAND_DIR_INFO; //!< Help files (spoilers) for the online help (ascii) These files are portable between platforms
concptr ANGBAND_DIR_PREF; //!< Default user "preference" files (ascii) These files are rarely portable between platforms
concptr ANGBAND_DIR_SAVE; //!< Savefiles for current characters (binary)
concptr ANGBAND_DIR_USER; //!< User "preference" files (ascii) These files are rarely portable between platforms
concptr ANGBAND_DIR_XTRA; //!< Various extra files (binary) These files are rarely portable between platforms

s16b signal_count;		/* Hack -- Count interupts */

/*
 * Buffer to hold the current savefile name
 * 'savefile' holds full path name. 'savefile_base' holds only base name.
 */
char savefile[1024];
char savefile_base[40];

/*
 * You may or may not want to use the following "#undef".
 */
 /* #undef _POSIX_SAVED_IDS */

 /*!
  * @brief ファイルのドロップパーミッションチェック / Hack -- drop permissions
  */
void safe_setuid_drop(void)
{

#ifdef SET_UID

# ifdef SAFE_SETUID

#  ifdef SAFE_SETUID_POSIX

	if (setuid(getuid()) != 0)
	{
		quit(_("setuid(): 正しく許可が取れません！", "setuid(): cannot set permissions correctly!"));
	}
	if (setgid(getgid()) != 0)
	{
		quit(_("setgid(): 正しく許可が取れません！", "setgid(): cannot set permissions correctly!"));
	}

#  else

	if (setreuid(geteuid(), getuid()) != 0)
	{
		quit(_("setreuid(): 正しく許可が取れません！", "setreuid(): cannot set permissions correctly!"));
	}
	if (setregid(getegid(), getgid()) != 0)
	{
		quit(_("setregid(): 正しく許可が取れません！", "setregid(): cannot set permissions correctly!"));
	}

#  endif

# endif

#endif

}


/*!
 * @brief ファイルのグラブパーミッションチェック / Hack -- grab permissions
 */
void safe_setuid_grab(void)
{

#ifdef SET_UID

# ifdef SAFE_SETUID

#  ifdef SAFE_SETUID_POSIX

	if (setuid(p_ptr->player_egid) != 0)
	{
		quit(_("setuid(): 正しく許可が取れません！", "setuid(): cannot set permissions correctly!"));
	}
	if (setgid(p_ptr->player_egid) != 0)
	{
		quit(_("setgid(): 正しく許可が取れません！", "setgid(): cannot set permissions correctly!"));
	}

#  else

	if (setreuid(geteuid(), getuid()) != 0)
	{
		quit(_("setreuid(): 正しく許可が取れません！", "setreuid(): cannot set permissions correctly!"));
	}
	if (setregid(getegid(), getgid()) != 0)
	{
		quit(_("setregid(): 正しく許可が取れません！", "setregid(): cannot set permissions correctly!"));
	}

#  endif /* SAFE_SETUID_POSIX */

# endif /* SAFE_SETUID */

#endif /* SET_UID */

}


/*!
 * @brief 各種データテキストをトークン単位に分解する / Extract the first few "tokens" from a buffer
 * @param buf データテキストの参照ポインタ
 * @param num トークンの数
 * @param tokens トークンを保管する文字列参照ポインタ配列
 * @param mode オプション
 * @return 解釈した文字列数
 * @details
 * <pre>
 * This function uses "colon" and "slash" as the delimeter characters.
 * We never extract more than "num" tokens.  The "last" token may include
 * "delimeter" characters, allowing the buffer to include a "string" token.
 * We save pointers to the tokens in "tokens", and return the number found.
 * Hack -- Attempt to handle the 'c' character formalism
 * Hack -- An empty buffer, or a final delimeter, yields an "empty" token.
 * Hack -- We will always extract at least one token
 * </pre>
 */
s16b tokenize(char *buf, s16b num, char **tokens, BIT_FLAGS mode)
{
	s16b i = 0;
	char *s = buf;
	while (i < num - 1)
	{
		char *t;
		for (t = s; *t; t++)
		{
			/* Found a delimiter */
			if ((*t == ':') || (*t == '/')) break;

			/* Handle single quotes */
			if ((mode & TOKENIZE_CHECKQUOTE) && (*t == '\''))
			{
				/* Advance */
				t++;

				/* Handle backslash */
				if (*t == '\\') t++;

				/* Require a character */
				if (!*t) break;

				/* Advance */
				t++;

				/* Hack -- Require a close quote */
				if (*t != '\'') *t = '\'';
			}

			/* Handle back-slash */
			if (*t == '\\') t++;
		}

		/* Nothing left */
		if (!*t) break;

		/* Nuke and advance */
		*t++ = '\0';

		/* Save the token */
		tokens[i++] = s;

		/* Advance */
		s = t;
	}

	/* Save the token */
	tokens[i++] = s;
	return i;
}


/* A number with a name */
typedef struct named_num named_num;

struct named_num
{
	concptr name;		/* The name of this thing */
	int num;			/* A number associated with it */
};


/* Index of spell type names */
static named_num gf_desc[] =
{
	{"GF_ELEC", 				GF_ELEC				},
	{"GF_POIS", 				GF_POIS				},
	{"GF_ACID", 				GF_ACID				},
	{"GF_COLD", 				GF_COLD				},
	{"GF_FIRE",		 			GF_FIRE				},
	{"GF_PSY_SPEAR",			GF_PSY_SPEAR		},
	{"GF_MISSILE",				GF_MISSILE			},
	{"GF_ARROW",				GF_ARROW			},
	{"GF_PLASMA",				GF_PLASMA			},
	{"GF_WATER",				GF_WATER			},
	{"GF_LITE",					GF_LITE				},
	{"GF_DARK",					GF_DARK				},
	{"GF_LITE_WEAK",			GF_LITE_WEAK		},
	{"GF_DARK_WEAK",			GF_DARK_WEAK		},
	{"GF_SHARDS",				GF_SHARDS			},
	{"GF_SOUND",				GF_SOUND			},
	{"GF_CONFUSION",			GF_CONFUSION		},
	{"GF_FORCE",				GF_FORCE			},
	{"GF_INERTIA",				GF_INERTIAL			},
	{"GF_MANA",					GF_MANA				},
	{"GF_METEOR",				GF_METEOR			},
	{"GF_ICE",					GF_ICE				},
	{"GF_CHAOS",				GF_CHAOS			},
	{"GF_NETHER",				GF_NETHER			},
	{"GF_DISENCHANT",			GF_DISENCHANT		},
	{"GF_NEXUS",				GF_NEXUS			},
	{"GF_TIME",					GF_TIME				},
	{"GF_GRAVITY",				GF_GRAVITY			},
	{"GF_KILL_WALL",			GF_KILL_WALL		},
	{"GF_KILL_DOOR",			GF_KILL_DOOR		},
	{"GF_KILL_TRAP",			GF_KILL_TRAP		},
	{"GF_MAKE_WALL",			GF_MAKE_WALL		},
	{"GF_MAKE_DOOR",			GF_MAKE_DOOR		},
	{"GF_MAKE_TRAP",			GF_MAKE_TRAP		},
	{"GF_MAKE_TREE",			GF_MAKE_TREE		},
	{"GF_OLD_CLONE",			GF_OLD_CLONE		},
	{"GF_OLD_POLY",				GF_OLD_POLY			},
	{"GF_OLD_HEAL",				GF_OLD_HEAL			},
	{"GF_OLD_SPEED",			GF_OLD_SPEED		},
	{"GF_OLD_SLOW",				GF_OLD_SLOW			},
	{"GF_OLD_CONF",				GF_OLD_CONF			},
	{"GF_OLD_SLEEP",			GF_OLD_SLEEP		},
	{"GF_HYPODYNAMIA",			GF_HYPODYNAMIA		},
	{"GF_AWAY_UNDEAD",			GF_AWAY_UNDEAD		},
	{"GF_AWAY_EVIL",			GF_AWAY_EVIL		},
	{"GF_AWAY_ALL",				GF_AWAY_ALL			},
	{"GF_TURN_UNDEAD",			GF_TURN_UNDEAD		},
	{"GF_TURN_EVIL",			GF_TURN_EVIL		},
	{"GF_TURN_ALL",				GF_TURN_ALL			},
	{"GF_DISP_UNDEAD",			GF_DISP_UNDEAD		},
	{"GF_DISP_EVIL",			GF_DISP_EVIL		},
	{"GF_DISP_ALL",				GF_DISP_ALL			},
	{"GF_DISP_DEMON",			GF_DISP_DEMON		},
	{"GF_DISP_LIVING",			GF_DISP_LIVING		},
	{"GF_ROCKET",				GF_ROCKET			},
	{"GF_NUKE",					GF_NUKE				},
	{"GF_MAKE_GLYPH",			GF_MAKE_GLYPH		},
	{"GF_STASIS",				GF_STASIS			},
	{"GF_STONE_WALL",			GF_STONE_WALL		},
	{"GF_DEATH_RAY",			GF_DEATH_RAY		},
	{"GF_STUN",					GF_STUN				},
	{"GF_HOLY_FIRE",			GF_HOLY_FIRE		},
	{"GF_HELL_FIRE",			GF_HELL_FIRE		},
	{"GF_DISINTEGRATE",			GF_DISINTEGRATE		},
	{"GF_CHARM",				GF_CHARM			},
	{"GF_CONTROL_UNDEAD",		GF_CONTROL_UNDEAD	},
	{"GF_CONTROL_ANIMAL",		GF_CONTROL_ANIMAL	},
	{"GF_PSI",					GF_PSI				},
	{"GF_PSI_DRAIN",			GF_PSI_DRAIN		},
	{"GF_TELEKINESIS",			GF_TELEKINESIS		},
	{"GF_JAM_DOOR",				GF_JAM_DOOR			},
	{"GF_DOMINATION",			GF_DOMINATION		},
	{"GF_DISP_GOOD",			GF_DISP_GOOD		},
	{"GF_DRAIN_MANA",			GF_DRAIN_MANA		},
	{"GF_MIND_BLAST",			GF_MIND_BLAST		},
	{"GF_BRAIN_SMASH",			GF_BRAIN_SMASH		},
	{"GF_CAUSE_1",				GF_CAUSE_1			},
	{"GF_CAUSE_2",				GF_CAUSE_2			},
	{"GF_CAUSE_3",				GF_CAUSE_3			},
	{"GF_CAUSE_4",				GF_CAUSE_4			},
	{"GF_HAND_DOOM",			GF_HAND_DOOM		},
	{"GF_CAPTURE",				GF_CAPTURE			},
	{"GF_ANIM_DEAD",			GF_ANIM_DEAD		},
	{"GF_CHARM_LIVING",			GF_CHARM_LIVING		},
	{"GF_IDENTIFY",				GF_IDENTIFY			},
	{"GF_ATTACK",				GF_ATTACK			},
	{"GF_ENGETSU",				GF_ENGETSU			},
	{"GF_GENOCIDE",				GF_GENOCIDE			},
	{"GF_PHOTO",				GF_PHOTO			},
	{"GF_CONTROL_DEMON",		GF_CONTROL_DEMON	},
	{"GF_LAVA_FLOW",			GF_LAVA_FLOW		},
	{"GF_BLOOD_CURSE",			GF_BLOOD_CURSE		},
	{"GF_SEEKER",				GF_SEEKER			},
	{"GF_SUPER_RAY",			GF_SUPER_RAY		},
	{"GF_STAR_HEAL",			GF_STAR_HEAL		},
	{"GF_WATER_FLOW",			GF_WATER_FLOW		},
	{"GF_CRUSADE",				GF_CRUSADE			},
	{"GF_STASIS_EVIL",			GF_STASIS_EVIL		},
	{"GF_WOUNDS",				GF_WOUNDS			},
	{NULL, 						0					}
};


/*!
 * @brief 設定ファイルの各行から各種テキスト情報を取得する /
 * Parse a sub-file of the "extra info" (format shown below)
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param buf データテキストの参照ポインタ
 * @return エラーコード
 * @details
 * <pre>
 * Each "action" line has an "action symbol" in the first column,
 * followed by a colon, followed by some command specific info,
 * usually in the form of "tokens" separated by colons or slashes.
 * Blank lines, lines starting with white space, and lines starting
 * with pound signs ("#") are ignored (as comments).
 * Note the use of "tokenize()" to allow the use of both colons and
 * slashes as delimeters, while still allowing final tokens which
 * may contain any characters including "delimiters".
 * Note the use of "strtol()" to allow all "integers" to be encoded
 * in decimal, hexidecimal, or octal form.
 * Note that "monster zero" is used for the "player" attr/char, "object
 * zero" will be used for the "stack" attr/char, and "feature zero" is
 * used for the "nothing" attr/char.
 * Parse another file recursively, see below for details
 *   %:\<filename\>
 * Specify the attr/char values for "monsters" by race index
 *   R:\<num\>:\<a\>:\<c\>
 * Specify the attr/char values for "objects" by kind index
 *   K:\<num\>:\<a\>:\<c\>
 * Specify the attr/char values for "features" by feature index
 *   F:\<num\>:\<a\>:\<c\>
 * Specify the attr/char values for unaware "objects" by kind tval
 *   U:\<tv\>:\<a\>:\<c\>
 * Specify the attr/char values for inventory "objects" by kind tval
 *   E:\<tv\>:\<a\>:\<c\>
 * Define a macro action, given an encoded macro action
 *   A:\<str\>
 * Create a normal macro, given an encoded macro trigger
 *   P:\<str\>
 * Create a command macro, given an encoded macro trigger
 *   C:\<str\>
 * Create a keyset mapping
 *   S:\<key\>:\<key\>:\<dir\>
 * Turn an option off, given its name
 *   X:\<str\>
 * Turn an option on, given its name
 *   Y:\<str\>
 * Specify visual information, given an index, and some data
 *   V:\<num\>:\<kv\>:\<rv\>:\<gv\>:\<bv\>
 * Specify the set of colors to use when drawing a zapped spell
 *   Z:\<type\>:\<str\>
 * Specify a macro trigger template and macro trigger names.
 *   T:\<template\>:\<modifier chr\>:\<modifier name1\>:\<modifier name2\>:...
 *   T:\<trigger\>:\<keycode\>:\<shift-keycode\>
 * </pre>
 */
errr process_pref_file_command(player_type *creature_ptr, char *buf)
{
	if (buf[1] != ':') return 1;

	char *zz[16];
	switch (buf[0])
	{
		/* Mega-Hack -- read external player's history file */
		/* Process "H:<history>" */
	case 'H':
		add_history_from_pref_line(buf + 2);
		return 0;

		/* Process "R:<num>:<a>/<c>" -- attr/char for monster races */
	case 'R':
	{
		if (tokenize(buf + 2, 3, zz, TOKENIZE_CHECKQUOTE) != 3) break;
		monster_race *r_ptr;
		int i = (huge)strtol(zz[0], NULL, 0);
		TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], NULL, 0);
		SYMBOL_CODE n2 = (SYMBOL_CODE)strtol(zz[2], NULL, 0);
		if (i >= max_r_idx) return 1;
		r_ptr = &r_info[i];
		if (n1 || (!(n2 & 0x80) && n2)) r_ptr->x_attr = n1; /* Allow TERM_DARK text */
		if (n2) r_ptr->x_char = n2;
		return 0;
	}

	/* Process "K:<num>:<a>/<c>"  -- attr/char for object kinds */
	case 'K':
	{
		if (tokenize(buf + 2, 3, zz, TOKENIZE_CHECKQUOTE) != 3) break;

		object_kind *k_ptr;
		int i = (huge)strtol(zz[0], NULL, 0);
		TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], NULL, 0);
		SYMBOL_CODE n2 = (SYMBOL_CODE)strtol(zz[2], NULL, 0);
		if (i >= max_k_idx) return 1;
		k_ptr = &k_info[i];
		if (n1 || (!(n2 & 0x80) && n2)) k_ptr->x_attr = n1; /* Allow TERM_DARK text */
		if (n2) k_ptr->x_char = n2;
		return 0;
	}

	/* Process "F:<num>:<a>/<c>" -- attr/char for terrain features */
	/* "F:<num>:<a>/<c>" */
	/* "F:<num>:<a>/<c>:LIT" */
	/* "F:<num>:<a>/<c>:<la>/<lc>:<da>/<dc>" */
	case 'F':
	{
		feature_type *f_ptr;
		int num = tokenize(buf + 2, F_LIT_MAX * 2 + 1, zz, TOKENIZE_CHECKQUOTE);

		if ((num != 3) && (num != 4) && (num != F_LIT_MAX * 2 + 1)) return 1;
		else if ((num == 4) && !streq(zz[3], "LIT")) return 1;

		int i = (huge)strtol(zz[0], NULL, 0);
		if (i >= max_f_idx) return 1;
		f_ptr = &f_info[i];

		TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], NULL, 0);
		SYMBOL_CODE n2 = (SYMBOL_CODE)strtol(zz[2], NULL, 0);
		if (n1 || (!(n2 & 0x80) && n2)) f_ptr->x_attr[F_LIT_STANDARD] = n1; /* Allow TERM_DARK text */
		if (n2) f_ptr->x_char[F_LIT_STANDARD] = n2;

		/* Mega-hack -- feat supports lighting */
		switch (num)
		{
			/* No lighting support */
		case 3:
			n1 = f_ptr->x_attr[F_LIT_STANDARD];
			n2 = f_ptr->x_char[F_LIT_STANDARD];
			for (int j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++)
			{
				f_ptr->x_attr[j] = n1;
				f_ptr->x_char[j] = n2;
			}

			break;

			/* Use default lighting */
		case 4:
			apply_default_feat_lighting(f_ptr->x_attr, f_ptr->x_char);
			break;

			/* Use desired lighting */
		case F_LIT_MAX * 2 + 1:
			for (int j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++)
			{
				n1 = (TERM_COLOR)strtol(zz[j * 2 + 1], NULL, 0);
				n2 = (SYMBOL_CODE)strtol(zz[j * 2 + 2], NULL, 0);
				if (n1 || (!(n2 & 0x80) && n2)) f_ptr->x_attr[j] = n1; /* Allow TERM_DARK text */
				if (n2) f_ptr->x_char[j] = n2;
			}

			break;
		}
	}

	return 0;

	/* Process "S:<num>:<a>/<c>" -- attr/char for special things */
	case 'S':
	{
		if (tokenize(buf + 2, 3, zz, TOKENIZE_CHECKQUOTE) != 3) break;

		int j = (byte)strtol(zz[0], NULL, 0);
		TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], NULL, 0);
		SYMBOL_CODE n2 = (SYMBOL_CODE)strtol(zz[2], NULL, 0);
		misc_to_attr[j] = n1;
		misc_to_char[j] = n2;
		return 0;
	}

	/* Process "U:<tv>:<a>/<c>" -- attr/char for unaware items */
	case 'U':
	{
		if (tokenize(buf + 2, 3, zz, TOKENIZE_CHECKQUOTE) != 3) break;

		int j = (huge)strtol(zz[0], NULL, 0);
		TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], NULL, 0);
		SYMBOL_CODE n2 = (SYMBOL_CODE)strtol(zz[2], NULL, 0);
		for (int i = 1; i < max_k_idx; i++)
		{
			object_kind *k_ptr = &k_info[i];
			if (k_ptr->tval == j)
			{
				if (n1) k_ptr->d_attr = n1;
				if (n2) k_ptr->d_char = n2;
			}
		}

		return 0;
	}

	/* Process "E:<tv>:<a>" -- attribute for inventory objects */
	case 'E':
	{
		if (tokenize(buf + 2, 2, zz, TOKENIZE_CHECKQUOTE) != 2) break;

		int j = (byte)strtol(zz[0], NULL, 0) % 128;
		TERM_COLOR n1 = (TERM_COLOR)strtol(zz[1], NULL, 0);
		if (n1) tval_to_attr[j] = n1;
		return 0;
	}

	/* Process "A:<str>" -- save an "action" for later */
	case 'A':
		text_to_ascii(macro__buf, buf + 2);
		return 0;

		/* Process "P:<str>" -- normal macro */
	case 'P':
	{
		char tmp[1024];

		text_to_ascii(tmp, buf + 2);
		macro_add(tmp, macro__buf);
		return 0;
	}

	/* Process "C:<str>" -- create keymap */
	case 'C':
	{
		if (tokenize(buf + 2, 2, zz, TOKENIZE_CHECKQUOTE) != 2) return 1;

		int mode = strtol(zz[0], NULL, 0);
		if ((mode < 0) || (mode >= KEYMAP_MODES)) return 1;

		char tmp[1024];
		text_to_ascii(tmp, zz[1]);
		if (!tmp[0] || tmp[1]) return 1;
		int i = (byte)(tmp[0]);

		string_free(keymap_act[mode][i]);

		keymap_act[mode][i] = string_make(macro__buf);

		return 0;
	}

	/* Process "V:<num>:<kv>:<rv>:<gv>:<bv>" -- visual info */
	case 'V':
	{
		if (tokenize(buf + 2, 5, zz, TOKENIZE_CHECKQUOTE) != 5) break;

		int i = (byte)strtol(zz[0], NULL, 0);
		angband_color_table[i][0] = (byte)strtol(zz[1], NULL, 0);
		angband_color_table[i][1] = (byte)strtol(zz[2], NULL, 0);
		angband_color_table[i][2] = (byte)strtol(zz[3], NULL, 0);
		angband_color_table[i][3] = (byte)strtol(zz[4], NULL, 0);
		return 0;
	}

	/* Process "X:<str>" -- turn option off */
	/* Process "Y:<str>" -- turn option on */
	case 'X':
	case 'Y':
	{
		for (int i = 0; option_info[i].o_desc; i++)
		{
			bool is_option = option_info[i].o_var != NULL;
			is_option &= option_info[i].o_text != NULL;
			is_option &= streq(option_info[i].o_text, buf + 2);
			if (!is_option) continue;

			int os = option_info[i].o_set;
			int ob = option_info[i].o_bit;

			if ((creature_ptr->playing || current_world_ptr->character_xtra) &&
				(OPT_PAGE_BIRTH == option_info[i].o_page) && !current_world_ptr->wizard)
			{
				msg_format(_("初期オプションは変更できません! '%s'", "Birth options can not changed! '%s'"), buf);
				msg_print(NULL);
				return 0;
			}

			if (buf[0] == 'X')
			{
				option_flag[os] &= ~(1L << ob);
				(*option_info[i].o_var) = FALSE;
				return 0;
			}

			option_flag[os] |= (1L << ob);
			(*option_info[i].o_var) = TRUE;
			return 0;
		}

		/* don't know that option. ignore it.*/
		msg_format(_("オプションの名前が正しくありません： %s", "Ignored invalid option: %s"), buf);
		msg_print(NULL);
		return 0;
	}

	/* Process "Z:<type>:<str>" -- set spell color */
	case 'Z':
	{
		/* Find the colon */
		char *t = my_strchr(buf + 2, ':');

		if (!t) return 1;

		/* Nuke the colon */
		*(t++) = '\0';

		for (int i = 0; gf_desc[i].name; i++)
		{
			/* Match this type */
			if (streq(gf_desc[i].name, buf + 2))
			{
				/* Remember this color set */
				gf_color[gf_desc[i].num] = (TERM_COLOR)quark_add(t);

				/* Success */
				return 0;
			}
		}

		break;
	}

	/* Initialize macro trigger names and a template */
	/* Process "T:<trigger>:<keycode>:<shift-keycode>" */
	/* Process "T:<template>:<modifier chr>:<modifier name>:..." */
	case 'T':
	{
		int tok = tokenize(buf + 2, 2 + MAX_MACRO_MOD, zz, 0);

		/* Process "T:<template>:<modifier chr>:<modifier name>:..." */
		if (tok >= 4)
		{
			int num;

			if (macro_template != NULL)
			{
				num = strlen(macro_modifier_chr);

				/* Kill the template string */
				string_free(macro_template);
				macro_template = NULL;

				/* Kill flag characters of modifier keys */
				string_free(macro_modifier_chr);

				/* Kill corresponding modifier names */
				for (int i = 0; i < num; i++)
				{
					string_free(macro_modifier_name[i]);
				}

				/* Kill trigger name strings */
				for (int i = 0; i < max_macrotrigger; i++)
				{
					string_free(macro_trigger_name[i]);
					string_free(macro_trigger_keycode[0][i]);
					string_free(macro_trigger_keycode[1][i]);
				}

				max_macrotrigger = 0;
			}

			if (*zz[0] == '\0') return 0; /* clear template */

			/* Number of modifier flags */
			num = strlen(zz[1]);

			/* Limit the number */
			num = MIN(MAX_MACRO_MOD, num);

			/* Stop if number of modifier is not correct */
			if (2 + num != tok) return 1;

			/* Get a template string */
			macro_template = string_make(zz[0]);

			/* Get flag characters of modifier keys */
			macro_modifier_chr = string_make(zz[1]);

			/* Get corresponding modifier names */
			for (int i = 0; i < num; i++)
			{
				macro_modifier_name[i] = string_make(zz[2 + i]);
			}

			return 0;
		}

		/* Process "T:<trigger>:<keycode>:<shift-keycode>" */
		if (tok < 2) return 0;

		char buf_aux[1024];
		char *t, *s;
		if (max_macrotrigger >= MAX_MACRO_TRIG)
		{
			msg_print(_("マクロトリガーの設定が多すぎます!", "Too many macro triggers!"));
			return 1;
		}

		int m = max_macrotrigger;
		max_macrotrigger++;

		/* Take into account the escape character  */
		t = buf_aux;
		s = zz[0];
		while (*s)
		{
			if ('\\' == *s) s++;
			*t++ = *s++;
		}

		*t = '\0';

		/* Get a trigger name */
		macro_trigger_name[m] = string_make(buf_aux);

		/* Get the corresponding key code */
		macro_trigger_keycode[0][m] = string_make(zz[1]);

		if (tok == 3)
		{
			macro_trigger_keycode[1][m] = string_make(zz[2]);
			return 0;
		}

		macro_trigger_keycode[1][m] = string_make(zz[1]);
		return 0;
	}
	}

	return 1;
}


/*!
 * @brief process_pref_fileのサブルーチンとして条件分岐処理の解釈と結果を返す /
 * Helper function for "process_pref_file()"
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param sp テキスト文字列の参照ポインタ
 * @param fp 再帰中のポインタ参照
 * @return
 * @details
 * <pre>
 * Input:
 *   v: output buffer array
 *   f: final character
 * Output:
 *   result
 * </pre>
 */
concptr process_pref_file_expr(player_type *creature_ptr, char **sp, char *fp)
{
	char *s;
	s = (*sp);

	/* Skip spaces */
	while (iswspace(*s)) s++;

	/* Save start */
	char *b;
	b = s;

	concptr v = "?o?o?";

	/* Analyze */
	char b1 = '[';
	char b2 = ']';
	char f = ' ';
	static char tmp[16];
	if (*s == b1)
	{
		concptr p;
		concptr t;

		/* Skip b1 */
		s++;

		/* First */
		t = process_pref_file_expr(creature_ptr, &s, &f);

		if (!*t)
		{
			/* Nothing */
		}

		/* Function: IOR */
		else if (streq(t, "IOR"))
		{
			v = "0";
			while (*s && (f != b2))
			{
				t = process_pref_file_expr(creature_ptr, &s, &f);
				if (*t && !streq(t, "0")) v = "1";
			}
		}

		/* Function: AND */
		else if (streq(t, "AND"))
		{
			v = "1";
			while (*s && (f != b2))
			{
				t = process_pref_file_expr(creature_ptr, &s, &f);
				if (*t && streq(t, "0")) v = "0";
			}
		}

		/* Function: NOT */
		else if (streq(t, "NOT"))
		{
			v = "1";
			while (*s && (f != b2))
			{
				t = process_pref_file_expr(creature_ptr, &s, &f);
				if (*t && streq(t, "1")) v = "0";
			}
		}

		/* Function: EQU */
		else if (streq(t, "EQU"))
		{
			v = "0";
			if (*s && (f != b2))
			{
				t = process_pref_file_expr(creature_ptr, &s, &f);
			}
			while (*s && (f != b2))
			{
				p = process_pref_file_expr(creature_ptr, &s, &f);
				if (streq(t, p)) v = "1";
			}
		}

		/* Function: LEQ */
		else if (streq(t, "LEQ"))
		{
			v = "1";
			if (*s && (f != b2))
			{
				t = process_pref_file_expr(creature_ptr, &s, &f);
			}
			while (*s && (f != b2))
			{
				p = t;
				t = process_pref_file_expr(creature_ptr, &s, &f);
				if (*t && atoi(p) > atoi(t)) v = "0";
			}
		}

		/* Function: GEQ */
		else if (streq(t, "GEQ"))
		{
			v = "1";
			if (*s && (f != b2))
			{
				t = process_pref_file_expr(creature_ptr, &s, &f);
			}
			while (*s && (f != b2))
			{
				p = t;
				t = process_pref_file_expr(creature_ptr, &s, &f);

				/* Compare two numbers instead of string */
				if (*t && atoi(p) < atoi(t)) v = "0";
			}
		}

		else
		{
			while (*s && (f != b2))
			{
				t = process_pref_file_expr(creature_ptr, &s, &f);
			}
		}

		/* Verify ending */
		if (f != b2) v = "?x?x?";

		/* Extract final and Terminate */
		if ((f = *s) != '\0') *s++ = '\0';

		*fp = f;
		*sp = s;
		return v;
	}

	/* Other */
	/* Accept all printables except spaces and brackets */
#ifdef JP
	while (iskanji(*s) || (isprint(*s) && !my_strchr(" []", *s)))
	{
		if (iskanji(*s)) s++;
		s++;
	}
#else
	while (isprint(*s) && !my_strchr(" []", *s)) ++s;
#endif

	/* Extract final and Terminate */
	if ((f = *s) != '\0') *s++ = '\0';

	if (*b != '$')
	{
		v = b;
		*fp = f;
		*sp = s;
		return v;
	}

	/* System */
	if (streq(b + 1, "SYS"))
	{
		v = ANGBAND_SYS;
	}

	else if (streq(b + 1, "KEYBOARD"))
	{
		v = ANGBAND_KEYBOARD;
	}

	/* Graphics */
	else if (streq(b + 1, "GRAF"))
	{
		v = ANGBAND_GRAF;
	}

	/* Monochrome mode */
	else if (streq(b + 1, "MONOCHROME"))
	{
		if (arg_monochrome)
			v = "ON";
		else
			v = "OFF";
	}

	/* Race */
	else if (streq(b + 1, "RACE"))
	{
#ifdef JP
		v = rp_ptr->E_title;
#else
		v = rp_ptr->title;
#endif
	}

	/* Class */
	else if (streq(b + 1, "CLASS"))
	{
#ifdef JP
		v = cp_ptr->E_title;
#else
		v = cp_ptr->title;
#endif
	}

	/* Player */
	else if (streq(b + 1, "PLAYER"))
	{
		static char tmp_player_name[32];
		char *pn, *tpn;
		for (pn = creature_ptr->name, tpn = tmp_player_name; *pn; pn++, tpn++)
		{
#ifdef JP
			if (iskanji(*pn))
			{
				*(tpn++) = *(pn++);
				*tpn = *pn;
				continue;
			}
#endif
			*tpn = my_strchr(" []", *pn) ? '_' : *pn;
		}
		*tpn = '\0';
		v = tmp_player_name;
	}

	/* First realm */
	else if (streq(b + 1, "REALM1"))
	{
#ifdef JP
		v = E_realm_names[creature_ptr->realm1];
#else
		v = realm_names[creature_ptr->realm1];
#endif
	}

	/* Second realm */
	else if (streq(b + 1, "REALM2"))
	{
#ifdef JP
		v = E_realm_names[creature_ptr->realm2];
#else
		v = realm_names[creature_ptr->realm2];
#endif
	}

	/* Level */
	else if (streq(b + 1, "LEVEL"))
	{
		sprintf(tmp, "%02d", creature_ptr->lev);
		v = tmp;
	}

	/* Autopick auto-register is in-use or not? */
	else if (streq(b + 1, "AUTOREGISTER"))
	{
		if (creature_ptr->autopick_autoregister)
			v = "1";
		else
			v = "0";
	}

	/* Money */
	else if (streq(b + 1, "MONEY"))
	{
		sprintf(tmp, "%09ld", (long int)creature_ptr->au);
		v = tmp;
	}

	*fp = f;
	*sp = s;
	return v;
}


/*!
 * @brief process_pref_fileのサブルーチン /
 * Open the "user pref file" and parse it.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param name 読み込むファイル名
 * @param preftype prefファイルのタイプ
 * @return エラーコード
 * @details
 * <pre>
 * Input:
 *   v: output buffer array
 *   f: final character
 * Output:
 *   result
 * </pre>
 */
static errr process_pref_file_aux(player_type *creature_ptr, concptr name, int preftype)
{
	FILE *fp;
	fp = my_fopen(name, "r");

	/* No such file */
	if (!fp) return (-1);

	/* Process the file */
	char buf[1024];
	char old[1024];
	int line = -1;
	errr err = 0;
	bool bypass = FALSE;
	while (my_fgets(fp, buf, sizeof(buf)) == 0)
	{
		line++;
		if (!buf[0]) continue;

#ifdef JP
		if (!iskanji(buf[0]))
#endif
			if (iswspace(buf[0])) continue;

		/* Skip comments */
		if (buf[0] == '#') continue;

		strcpy(old, buf);

		/* Process "?:<expr>" */
		if ((buf[0] == '?') && (buf[1] == ':'))
		{
			char f;
			concptr v;
			char *s;

			/* Start */
			s = buf + 2;

			/* Parse the expr */
			v = process_pref_file_expr(creature_ptr, &s, &f);

			/* Set flag */
			bypass = (streq(v, "0") ? TRUE : FALSE);
			continue;
		}

		if (bypass) continue;

		/* Process "%:<file>" */
		if (buf[0] == '%')
		{
			static int depth_count = 0;

			/* Ignore if deeper than 20 level */
			if (depth_count > 20) continue;

			/* Count depth level */
			depth_count++;

			/* Process that file if allowed */
			switch (preftype)
			{
			case PREF_TYPE_AUTOPICK:
				(void)process_autopick_file(creature_ptr, buf + 2);
				break;
			case PREF_TYPE_HISTPREF:
				(void)process_histpref_file(creature_ptr, buf + 2);
				break;
			default:
				(void)process_pref_file(creature_ptr, buf + 2);
				break;
			}

			/* Set back depth level */
			depth_count--;
			continue;
		}

		/* Process the line */
		err = process_pref_file_command(creature_ptr, buf);

		/* This is not original pref line... */
		if (err)
		{
			if (preftype != PREF_TYPE_AUTOPICK)
				break;
			err = process_autopick_file_command(buf);
		}
	}

	if (err)
	{
		/* Print error message */
		/* ToDo: Add better error messages */
		msg_format(_("ファイル'%s'の%d行でエラー番号%dのエラー。", "Error %d in line %d of file '%s'."),
			_(name, err), line, _(err, name));
		msg_format(_("('%s'を解析中)", "Parsing '%s'"), old);
		msg_print(NULL);
	}

	my_fclose(fp);
	return (err);
}


/*!
 * @brief pref設定ファイルを読み込み設定を反映させる /
 * Process the "user pref file" with the given name
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param name 読み込むファイル名
 * @return エラーコード
 * @details
 * <pre>
 * See the functions above for a list of legal "commands".
 * We also accept the special "?" and "%" directives, which
 * allow conditional evaluation and filename inclusion.
 * </pre>
 */
errr process_pref_file(player_type *creature_ptr, concptr name)
{
	char buf[1024];
	path_build(buf, sizeof(buf), ANGBAND_DIR_PREF, name);

	/* Process the system pref file */
	errr err1 = process_pref_file_aux(creature_ptr, buf, PREF_TYPE_NORMAL);

	/* Stop at parser errors, but not at non-existing file */
	if (err1 > 0) return err1;

	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, name);

	/* Process the user pref file */
	errr err2 = process_pref_file_aux(creature_ptr, buf, PREF_TYPE_NORMAL);

	/* User file does not exist, but read system pref file */
	if (err2 < 0 && !err1)
		return -2;

	/* Result of user file processing */
	return err2;
}


#ifdef CHECK_TIME

/*
 * Operating hours for ANGBAND (defaults to non-work hours)
 */
static char days[7][29] =
{
	"SUN:XXXXXXXXXXXXXXXXXXXXXXXX",
	"MON:XXXXXXXX.........XXXXXXX",
	"TUE:XXXXXXXX.........XXXXXXX",
	"WED:XXXXXXXX.........XXXXXXX",
	"THU:XXXXXXXX.........XXXXXXX",
	"FRI:XXXXXXXX.........XXXXXXX",
	"SAT:XXXXXXXXXXXXXXXXXXXXXXXX"
};

/*
 * Restict usage (defaults to no restrictions)
 */
static bool check_time_flag = FALSE;

#endif


/*!
 * @brief Angbandプレイ禁止時刻をチェック /
 * Handle CHECK_TIME
 * @return エラーコード
 */
errr check_time(void)
{

#ifdef CHECK_TIME

	time_t      c;
	struct tm   *tp;

	/* No restrictions */
	if (!check_time_flag) return (0);

	/* Check for time violation */
	c = time((time_t *)0);
	tp = localtime(&c);

	/* Violation */
	if (days[tp->tm_wday][tp->tm_hour + 4] != 'X') return (1);

#endif

	/* Success */
	return (0);
}


/*!
 * @brief Angbandプレイ禁止時刻の初期化 /
 * Initialize CHECK_TIME
 * @return エラーコード
 */
errr check_time_init(void)
{

#ifdef CHECK_TIME

	FILE        *fp;

	char	buf[1024];

	path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, "time.txt");
	fp = my_fopen(buf, "r");

	/* No file, no restrictions */
	if (!fp) return (0);

	/* Assume restrictions */
	check_time_flag = TRUE;

	/* Parse the file */
	while (0 == my_fgets(fp, buf, sizeof(buf)))
	{
		/* Skip comments and blank lines */
		if (!buf[0] || (buf[0] == '#')) continue;

		/* Chop the buffer */
		buf[29] = '\0';

		/* Extract the info */
		if (prefix(buf, "SUN:")) strcpy(days[0], buf);
		if (prefix(buf, "MON:")) strcpy(days[1], buf);
		if (prefix(buf, "TUE:")) strcpy(days[2], buf);
		if (prefix(buf, "WED:")) strcpy(days[3], buf);
		if (prefix(buf, "THU:")) strcpy(days[4], buf);
		if (prefix(buf, "FRI:")) strcpy(days[5], buf);
		if (prefix(buf, "SAT:")) strcpy(days[6], buf);
	}
	my_fclose(fp);

#endif

	/* Success */
	return (0);
}


#ifdef CHECK_LOAD

#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN  64
#endif

typedef struct statstime statstime;

struct statstime
{
	int                 cp_time[4];
	int                 dk_xfer[4];
	unsigned int        v_pgpgin;
	unsigned int        v_pgpgout;
	unsigned int        v_pswpin;
	unsigned int        v_pswpout;
	unsigned int        v_intr;
	int                 if_ipackets;
	int                 if_ierrors;
	int                 if_opackets;
	int                 if_oerrors;
	int                 if_collisions;
	unsigned int        v_swtch;
	long                avenrun[3];
	struct timeval      boottime;
	struct timeval      curtime;
};

/*
 * Maximal load (if any).
 */
static int check_load_value = 0;

#endif


/*!
 * @brief Angbandプレイ禁止ホストのチェック /
 * Handle CHECK_LOAD
 * @return エラーコード
 */
errr check_load(void)
{

#ifdef CHECK_LOAD

	struct statstime    st;

	/* Success if not checking */
	if (!check_load_value) return (0);

	/* Check the load */
	if (0 == rstat("localhost", &st))
	{
		long val1 = (long)(st.avenrun[2]);
		long val2 = (long)(check_load_value)* FSCALE;

		/* Check for violation */
		if (val1 >= val2) return (1);
	}

#endif

	/* Success */
	return (0);
}


/*!
 * @brief Angbandプレイ禁止ホストの設定初期化 /
 * Initialize CHECK_LOAD
 * @return エラーコード
 */
errr check_load_init(void)
{

#ifdef CHECK_LOAD

	FILE        *fp;

	char	buf[1024];

	char	temphost[MAXHOSTNAMELEN + 1];
	char	thishost[MAXHOSTNAMELEN + 1];

	path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, "load.txt");

	/* Open the "load" file */
	fp = my_fopen(buf, "r");

	/* No file, no restrictions */
	if (!fp) return (0);

	/* Default load */
	check_load_value = 100;

	/* Get the host name */
	(void)gethostname(thishost, (sizeof thishost) - 1);

	/* Parse it */
	while (0 == my_fgets(fp, buf, sizeof(buf)))
	{
		int value;

		/* Skip comments and blank lines */
		if (!buf[0] || (buf[0] == '#')) continue;

		/* Parse, or ignore */
		if (sscanf(buf, "%s%d", temphost, &value) != 2) continue;

		/* Skip other hosts */
		if (!streq(temphost, thishost) &&
			!streq(temphost, "localhost")) continue;

		/* Use that value */
		check_load_value = value;

		break;
	}
	my_fclose(fp);

#endif

	/* Success */
	return (0);
}


#define ENTRY_BARE_HAND 0
#define ENTRY_TWO_HANDS 1
#define ENTRY_RIGHT_HAND1 2
#define ENTRY_LEFT_HAND1 3
#define ENTRY_LEFT_HAND2 4
#define ENTRY_RIGHT_HAND2 5
#define ENTRY_POSTURE 6
#define ENTRY_SHOOT_HIT_DAM 7
#define ENTRY_SHOOT_POWER 8
#define ENTRY_SPEED 9
#define ENTRY_BASE_AC 10
#define ENTRY_LEVEL 11
#define ENTRY_CUR_EXP 12
#define ENTRY_MAX_EXP 13
#define ENTRY_EXP_TO_ADV 14
#define ENTRY_GOLD 15
#define ENTRY_DAY 16
#define ENTRY_HP 17
#define ENTRY_SP 18
#define ENTRY_PLAY_TIME 19
#define ENTRY_SKILL_FIGHT 20
#define ENTRY_SKILL_SHOOT 21
#define ENTRY_SKILL_SAVING 22
#define ENTRY_SKILL_STEALTH 23
#define ENTRY_SKILL_PERCEP 24
#define ENTRY_SKILL_SEARCH 25
#define ENTRY_SKILL_DISARM 26
#define ENTRY_SKILL_DEVICE 27
#define ENTRY_SKILL_DIG 45


#define ENTRY_BLOWS 28
#define ENTRY_SHOTS 29
#define ENTRY_AVG_DMG 30
#define ENTRY_INFRA 31

#define ENTRY_NAME 32
#define ENTRY_SEX 33
#define ENTRY_RACE 34
#define ENTRY_CLASS 35
#define ENTRY_REALM 36
#define ENTRY_PATRON 37
#define ENTRY_AGE 38
#define ENTRY_HEIGHT 39
#define ENTRY_WEIGHT 40
#define ENTRY_SOCIAL 41
#define ENTRY_ALIGN 42

#define ENTRY_EXP_ANDR 43
#define ENTRY_EXP_TO_ADV_ANDR 44

static struct
{
	int col;
	int row;
	int len;
	char header[20];
} disp_player_line[]
#ifdef JP
= {
	{ 1, 10, 25, "打撃修正(格闘)"},
	{ 1, 10, 25, "打撃修正(両手)"},
	{ 1, 10, 25, "打撃修正(右手)"},
	{ 1, 10, 25, "打撃修正(左手)"},
	{ 1, 11, 25, "打撃修正(左手)"},
	{ 1, 11, 25, "打撃修正(右手)"},
	{ 1, 11, 25, ""},
	{ 1, 15, 25, "射撃攻撃修正"},
	{ 1, 16, 25, "射撃武器倍率"},
	{ 1, 20, 25, "加速"},
	{ 1, 19, 25, "ＡＣ"},
	{29, 13, 21, "レベル"},
	{29, 14, 21, "経験値"},
	{29, 15, 21, "最大経験"},
	{29, 16, 21, "次レベル"},
	{29, 17, 21, "所持金"},
	{29, 19, 21, "日付"},
	{29, 10, 21, "ＨＰ"},
	{29, 11, 21, "ＭＰ"},
	{29, 20, 21, "プレイ時間"},
	{53, 10, -1, "打撃命中  :"},
	{53, 11, -1, "射撃命中  :"},
	{53, 12, -1, "魔法防御  :"},
	{53, 13, -1, "隠密行動  :"},
	{53, 15, -1, "知覚      :"},
	{53, 16, -1, "探索      :"},
	{53, 17, -1, "解除      :"},
	{53, 18, -1, "魔法道具  :"},
	{ 1, 12, 25, "打撃回数"},
	{ 1, 17, 25, "射撃回数"},
	{ 1, 13, 25, "平均ダメージ"},
	{53, 20, -1, "赤外線視力:"},
	{26,  1, -1, "名前  : "},
	{ 1,  3, -1, "性別     : "},
	{ 1,  4, -1, "種族     : "},
	{ 1,  5, -1, "職業     : "},
	{ 1,  6, -1, "魔法     : "},
	{ 1,  7, -1, "守護魔神 : "},
	{29,  3, 21, "年齢"},
	{29,  4, 21, "身長"},
	{29,  5, 21, "体重"},
	{29,  6, 21, "社会的地位"},
	{29,  7, 21, "属性"},
	{29, 14, 21, "強化度"},
	{29, 16, 21, "次レベル"},
	{53, 19, -1, "掘削      :" },
};
#else
= {
	{ 1, 10, 25, "Bare hand"},
	{ 1, 10, 25, "Two hands"},
	{ 1, 10, 25, "Right hand"},
	{ 1, 10, 25, "Left hand"},
	{ 1, 11, 25, "Left hand"},
	{ 1, 11, 25, "Right hand"},
	{ 1, 11, 25, "Posture"},
	{ 1, 15, 25, "Shooting"},
	{ 1, 16, 25, "Multiplier"},
	{ 1, 20, 25, "Speed"},
	{ 1, 19, 25, "AC"},
	{29, 13, 21, "Level"},
	{29, 14, 21, "Experience"},
	{29, 15, 21, "Max Exp"},
	{29, 16, 21, "Exp to Adv"},
	{29, 17, 21, "Gold"},
	{29, 19, 21, "Time"},
	{29, 10, 21, "Hit point"},
	{29, 11, 21, "SP (Mana)"},
	{29, 20, 21, "Play time"},
	{53, 10, -1, "Fighting   : "},
	{53, 11, -1, "Bows/Throw : "},
	{53, 12, -1, "SavingThrow: "},
	{53, 13, -1, "Stealth    : "},
	{53, 15, -1, "Perception : "},
	{53, 16, -1, "Searching  : "},
	{53, 17, -1, "Disarming  : "},
	{53, 18, -1, "MagicDevice: "},
	{ 1, 12, 25, "Blows/Round"},
	{ 1, 17, 25, "Shots/Round"},
	{ 1, 13, 25, "AverageDmg/Rnd"},
	{53, 20, -1, "Infra-Vision: "},
	{26,  1, -1, "Name  : "},
	{ 1,  3, -1, "Sex      : "},
	{ 1,  4, -1, "Race     : "},
	{ 1,  5, -1, "Class    : "},
	{ 1,  6, -1, "Magic    : "},
	{ 1,  7, -1, "Patron   : "},
	{29,  3, 21, "Age"},
	{29,  4, 21, "Height"},
	{29,  5, 21, "Weight"},
	{29,  6, 21, "Social Class"},
	{29,  7, 21, "Align"},
	{29, 14, 21, "Construction"},
	{29, 16, 21, "Const to Adv"},
	{53, 19, -1, "Digging    :" },
};
#endif


/*!
 * @brief プレイヤーのステータス1種を出力する
 * @param entry 項目ID
 * @param val 値を保管した文字列ポインタ
 * @param attr 項目表示の色
 * @return なし
 */
static void display_player_one_line(int entry, concptr val, TERM_COLOR attr)
{
	concptr head = disp_player_line[entry].header;
	int head_len = strlen(head);
	int row = disp_player_line[entry].row;
	int col = disp_player_line[entry].col;
	int len = disp_player_line[entry].len;
	Term_putstr(col, row, -1, TERM_WHITE, head);

	if (!val) return;

	if (len <= 0)
	{
		Term_putstr(col + head_len, row, -1, attr, val);
		return;
	}

	int val_len = len - head_len;
	char buf[40];
	sprintf(buf, "%*.*s", val_len, val_len, val);
	Term_putstr(col + head_len, row, -1, attr, buf);
}


/*!
 * @brief プレイヤーの打撃能力修正を表示する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param hand 武器の装備部位ID
 * @param hand_entry 項目ID
 * @return なし
 */
static void display_player_melee_bonus(player_type *creature_ptr, int hand, int hand_entry)
{
	HIT_PROB show_tohit = creature_ptr->dis_to_h[hand];
	HIT_POINT show_todam = creature_ptr->dis_to_d[hand];
	object_type *o_ptr = &creature_ptr->inventory_list[INVEN_RARM + hand];

	/* Hack -- add in weapon info if known */
	if (object_is_known(o_ptr)) show_tohit += o_ptr->to_h;
	if (object_is_known(o_ptr)) show_todam += o_ptr->to_d;

	show_tohit += creature_ptr->skill_thn / BTH_PLUS_ADJ;

	char buf[160];
	sprintf(buf, "(%+d,%+d)", (int)show_tohit, (int)show_todam);

	/* Dump the bonuses to hit/dam */
	if (!has_melee_weapon(creature_ptr, INVEN_RARM) && !has_melee_weapon(creature_ptr, INVEN_LARM))
		display_player_one_line(ENTRY_BARE_HAND, buf, TERM_L_BLUE);
	else if (creature_ptr->ryoute)
		display_player_one_line(ENTRY_TWO_HANDS, buf, TERM_L_BLUE);
	else
		display_player_one_line(hand_entry, buf, TERM_L_BLUE);
}


/*!
 * @brief プレイヤーステータス表示の中央部分を表示するサブルーチン
 * @param creature_ptr プレーヤーへの参照ポインタ
 * Prints the following information on the screen.
 * @return なし
 */
static void display_player_middle(player_type *creature_ptr)
{
	HIT_PROB show_tohit = creature_ptr->dis_to_h_b;
	HIT_POINT show_todam = 0;

	if (creature_ptr->migite)
	{
		display_player_melee_bonus(creature_ptr, 0, left_hander ? ENTRY_LEFT_HAND1 : ENTRY_RIGHT_HAND1);
	}

	if (creature_ptr->hidarite)
	{
		display_player_melee_bonus(creature_ptr, 1, left_hander ? ENTRY_RIGHT_HAND2 : ENTRY_LEFT_HAND2);
	}
	else if ((creature_ptr->pclass == CLASS_MONK) && (empty_hands(creature_ptr, TRUE) & EMPTY_HAND_RARM))
	{
		int i;
		if (creature_ptr->special_defense & KAMAE_MASK)
		{
			for (i = 0; i < MAX_KAMAE; i++)
			{
				if ((creature_ptr->special_defense >> i) & KAMAE_GENBU) break;
			}
			if (i < MAX_KAMAE)
			{
				display_player_one_line(ENTRY_POSTURE, format(_("%sの構え", "%s form"), kamae_shurui[i].desc), TERM_YELLOW);
			}
		}
		else
		{
			display_player_one_line(ENTRY_POSTURE, _("構えなし", "none"), TERM_YELLOW);
		}
	}

	/* Apply weapon bonuses */
	object_type *o_ptr = &creature_ptr->inventory_list[INVEN_BOW];
	if (object_is_known(o_ptr)) show_tohit += o_ptr->to_h;
	if (object_is_known(o_ptr)) show_todam += o_ptr->to_d;

	if ((o_ptr->sval == SV_LIGHT_XBOW) || (o_ptr->sval == SV_HEAVY_XBOW))
		show_tohit += creature_ptr->weapon_exp[0][o_ptr->sval] / 400;
	else
		show_tohit += (creature_ptr->weapon_exp[0][o_ptr->sval] - (WEAPON_EXP_MASTER / 2)) / 200;

	show_tohit += creature_ptr->skill_thb / BTH_PLUS_ADJ;

	/* Range attacks */
	display_player_one_line(ENTRY_SHOOT_HIT_DAM, format("(%+d,%+d)", show_tohit, show_todam), TERM_L_BLUE);

	int tmul = 0;
	if (creature_ptr->inventory_list[INVEN_BOW].k_idx)
	{
		tmul = bow_tmul(creature_ptr->inventory_list[INVEN_BOW].sval);

		/* Get extra "power" from "extra might" */
		if (creature_ptr->xtra_might) tmul++;

		tmul = tmul * (100 + (int)(adj_str_td[creature_ptr->stat_ind[A_STR]]) - 128);
	}

	/* shoot power */
	display_player_one_line(ENTRY_SHOOT_POWER, format("x%d.%02d", tmul / 100, tmul % 100), TERM_L_BLUE);

	/* Dump the armor class */
	display_player_one_line(ENTRY_BASE_AC, format("[%d,%+d]", creature_ptr->dis_ac, creature_ptr->dis_to_a), TERM_L_BLUE);

	int i = creature_ptr->pspeed - 110;

	/* Hack -- Visually "undo" the Search Mode Slowdown */
	if (creature_ptr->action == ACTION_SEARCH) i += 10;

	TERM_COLOR attr;
	if (i > 0)
	{
		if (!creature_ptr->riding)
			attr = TERM_L_GREEN;
		else
			attr = TERM_GREEN;
	}
	else if (i == 0)
	{
		if (!creature_ptr->riding)
			attr = TERM_L_BLUE;
		else
			attr = TERM_GREEN;
	}
	else
	{
		if (!creature_ptr->riding)
			attr = TERM_L_UMBER;
		else
			attr = TERM_RED;
	}

	int tmp_speed = 0;
	if (!creature_ptr->riding)
	{
		if (IS_FAST(creature_ptr)) tmp_speed += 10;
		if (creature_ptr->slow) tmp_speed -= 10;
		if (creature_ptr->lightspeed) tmp_speed = 99;
	}
	else
	{
		if (MON_FAST(&creature_ptr->current_floor_ptr->m_list[creature_ptr->riding])) tmp_speed += 10;
		if (MON_SLOW(&creature_ptr->current_floor_ptr->m_list[creature_ptr->riding])) tmp_speed -= 10;
	}

	char buf[160];
	if (tmp_speed)
	{
		if (!creature_ptr->riding)
			sprintf(buf, "(%+d%+d)", i - tmp_speed, tmp_speed);
		else
			sprintf(buf, _("乗馬中 (%+d%+d)", "Riding (%+d%+d)"), i - tmp_speed, tmp_speed);

		if (tmp_speed > 0)
			attr = TERM_YELLOW;
		else
			attr = TERM_VIOLET;
	}
	else
	{
		if (!creature_ptr->riding)
			sprintf(buf, "(%+d)", i);
		else
			sprintf(buf, _("乗馬中 (%+d)", "Riding (%+d)"), i);
	}

	display_player_one_line(ENTRY_SPEED, buf, attr);

	/* Dump character level */
	display_player_one_line(ENTRY_LEVEL, format("%d", creature_ptr->lev), TERM_L_GREEN);

	/* Dump experience */
	int e;
	if (creature_ptr->prace == RACE_ANDROID) e = ENTRY_EXP_ANDR;
	else e = ENTRY_CUR_EXP;

	if (creature_ptr->exp >= creature_ptr->max_exp)
		display_player_one_line(e, format("%ld", creature_ptr->exp), TERM_L_GREEN);
	else
		display_player_one_line(e, format("%ld", creature_ptr->exp), TERM_YELLOW);

	/* Dump max experience */
	if (creature_ptr->prace == RACE_ANDROID)
		/* Nothing */;
	else
		display_player_one_line(ENTRY_MAX_EXP, format("%ld", creature_ptr->max_exp), TERM_L_GREEN);

	/* Dump exp to advance */
	if (creature_ptr->prace == RACE_ANDROID) e = ENTRY_EXP_TO_ADV_ANDR;
	else e = ENTRY_EXP_TO_ADV;

	if (creature_ptr->lev >= PY_MAX_LEVEL)
		display_player_one_line(e, "*****", TERM_L_GREEN);
	else if (creature_ptr->prace == RACE_ANDROID)
		display_player_one_line(e, format("%ld", (s32b)(player_exp_a[creature_ptr->lev - 1] * creature_ptr->expfact / 100L)), TERM_L_GREEN);
	else
		display_player_one_line(e, format("%ld", (s32b)(player_exp[creature_ptr->lev - 1] * creature_ptr->expfact / 100L)), TERM_L_GREEN);

	/* Dump gold */
	display_player_one_line(ENTRY_GOLD, format("%ld", creature_ptr->au), TERM_L_GREEN);

	/* Dump Day */
	int day, hour, min;
	extract_day_hour_min(&day, &hour, &min);

	if (day < MAX_DAYS) sprintf(buf, _("%d日目 %2d:%02d", "Day %d %2d:%02d"), day, hour, min);
	else sprintf(buf, _("*****日目 %2d:%02d", "Day ***** %2d:%02d"), hour, min);
	display_player_one_line(ENTRY_DAY, buf, TERM_L_GREEN);

	/* Dump hit point */
	if (creature_ptr->chp >= creature_ptr->mhp)
		display_player_one_line(ENTRY_HP, format("%4d/%4d", creature_ptr->chp, creature_ptr->mhp), TERM_L_GREEN);
	else if (creature_ptr->chp > (creature_ptr->mhp * hitpoint_warn) / 10)
		display_player_one_line(ENTRY_HP, format("%4d/%4d", creature_ptr->chp, creature_ptr->mhp), TERM_YELLOW);
	else
		display_player_one_line(ENTRY_HP, format("%4d/%4d", creature_ptr->chp, creature_ptr->mhp), TERM_RED);

	/* Dump mana power */
	if (creature_ptr->csp >= creature_ptr->msp)
		display_player_one_line(ENTRY_SP, format("%4d/%4d", creature_ptr->csp, creature_ptr->msp), TERM_L_GREEN);
	else if (creature_ptr->csp > (creature_ptr->msp * mana_warn) / 10)
		display_player_one_line(ENTRY_SP, format("%4d/%4d", creature_ptr->csp, creature_ptr->msp), TERM_YELLOW);
	else
		display_player_one_line(ENTRY_SP, format("%4d/%4d", creature_ptr->csp, creature_ptr->msp), TERM_RED);

	/* Dump play time */
	display_player_one_line(ENTRY_PLAY_TIME, format("%.2lu:%.2lu:%.2lu", current_world_ptr->play_time / (60 * 60), (current_world_ptr->play_time / 60) % 60, current_world_ptr->play_time % 60), TERM_L_GREEN);
}


/*
 * Hack -- pass color info around this file
 */
static TERM_COLOR likert_color = TERM_WHITE;


/*!
 * @brief 技能ランクの表示基準を定める
 * Returns a "rating" of x depending on y
 * @param x 技能値
 * @param y 技能値に対するランク基準比
 * @return なし
 */
static concptr likert(int x, int y)
{
	static char dummy[20] = "", dummy2[20] = "";
	memset(dummy, 0, strlen(dummy));
	memset(dummy2, 0, strlen(dummy2));
	if (y <= 0) y = 1;

	if (show_actual_value)
	{
		sprintf(dummy, "%3d-", x);
	}

	/* Negative value */
	if (x < 0)
	{
		likert_color = TERM_L_DARK;
		strcat(dummy, _("最低", "Very Bad"));
		return dummy;
	}

	switch ((x / y))
	{
	case 0:
	case 1:
		likert_color = TERM_RED;
		strcat(dummy, _("悪い", "Bad"));
		break;
	case 2:
		likert_color = TERM_L_RED;
		strcat(dummy, _("劣る", "Poor"));
		break;

	case 3:
	case 4:
		likert_color = TERM_ORANGE;
		strcat(dummy, _("普通", "Fair"));
		break;

	case 5:
		likert_color = TERM_YELLOW;
		strcat(dummy, _("良い", "Good"));
		break;

	case 6:
		likert_color = TERM_YELLOW;
		strcat(dummy, _("大変良い", "Very Good"));
		break;

	case 7:
	case 8:
		likert_color = TERM_L_GREEN;
		strcat(dummy, _("卓越", "Excellent"));
		break;

	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
		likert_color = TERM_GREEN;
		strcat(dummy, _("超越", "Superb"));
		break;

	case 14:
	case 15:
	case 16:
	case 17:
		likert_color = TERM_BLUE;
		strcat(dummy, _("英雄的", "Heroic"));
		break;

	default:
		likert_color = TERM_VIOLET;
		sprintf(dummy2, _("伝説的[%d]", "Legendary[%d]"),
			(int)((((x / y) - 17) * 5) / 2));
		strcat(dummy, dummy2);

		break;
	}

	return dummy;
}


/*!
 * @brief プレイヤーステータスの1ページ目各種詳細をまとめて表示するサブルーチン
 * Prints ratings on certain abilities
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * This code is "imitated" elsewhere to "dump" a character sheet.
 */
static void display_player_various(player_type *creature_ptr)
{
	int muta_att = 0;
	if (creature_ptr->muta2 & MUT2_HORNS)     muta_att++;
	if (creature_ptr->muta2 & MUT2_SCOR_TAIL) muta_att++;
	if (creature_ptr->muta2 & MUT2_BEAK)      muta_att++;
	if (creature_ptr->muta2 & MUT2_TRUNK)     muta_att++;
	if (creature_ptr->muta2 & MUT2_TENTACLES) muta_att++;

	int xthn = creature_ptr->skill_thn + (creature_ptr->to_h_m * BTH_PLUS_ADJ);

	/* Shooting Skill (with current bow and normal missile) */
	object_type *o_ptr;
	o_ptr = &creature_ptr->inventory_list[INVEN_BOW];
	int tmp = creature_ptr->to_h_b + o_ptr->to_h;
	int	xthb = creature_ptr->skill_thb + (tmp * BTH_PLUS_ADJ);

	/* If the player is wielding one? */
	int	shots;
	int shot_frac;
	if (o_ptr->k_idx)
	{
		ENERGY energy_fire = bow_energy(o_ptr->sval);

		/* Calculate shots per round */
		shots = creature_ptr->num_fire * 100;
		shot_frac = (shots * 100 / energy_fire) % 100;
		shots = shots / energy_fire;
		if (o_ptr->name1 == ART_CRIMSON)
		{
			shots = 1;
			shot_frac = 0;
			if (creature_ptr->pclass == CLASS_ARCHER)
			{
				/* Extra shot at level 10 */
				if (creature_ptr->lev >= 10) shots++;

				/* Extra shot at level 30 */
				if (creature_ptr->lev >= 30) shots++;

				/* Extra shot at level 45 */
				if (creature_ptr->lev >= 45) shots++;
			}
		}
	}
	else
	{
		shots = 0;
		shot_frac = 0;
	}

	int damage[2];
	int to_h[2];
	int basedam;
	BIT_FLAGS flgs[TR_FLAG_SIZE];
	bool poison_needle;
	for (int i = 0; i < 2; i++)
	{
		damage[i] = creature_ptr->dis_to_d[i] * 100;
		if (((creature_ptr->pclass == CLASS_MONK) || (creature_ptr->pclass == CLASS_FORCETRAINER)) && (empty_hands(creature_ptr, TRUE) & EMPTY_HAND_RARM))
		{
			PLAYER_LEVEL level = creature_ptr->lev;
			if (i)
			{
				damage[i] = 0;
				break;
			}
			if (creature_ptr->pclass == CLASS_FORCETRAINER) level = MAX(1, level - 3);
			if (creature_ptr->special_defense & KAMAE_BYAKKO)
				basedam = monk_ave_damage[level][1];
			else if (creature_ptr->special_defense & (KAMAE_GENBU | KAMAE_SUZAKU))
				basedam = monk_ave_damage[level][2];
			else
				basedam = monk_ave_damage[level][0];

			damage[i] += basedam;
			if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_POISON_NEEDLE)) damage[i] = 1;
			if (damage[i] < 0) damage[i] = 0;
			continue;
		}

		o_ptr = &creature_ptr->inventory_list[INVEN_RARM + i];

		/* Average damage per round */
		if (o_ptr->k_idx == 0)
		{
			basedam = 0;
			damage[i] += basedam;
			if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_POISON_NEEDLE)) damage[i] = 1;
			if (damage[i] < 0) damage[i] = 0;
			continue;
		}

		to_h[i] = 0;
		poison_needle = FALSE;

		if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_POISON_NEEDLE)) poison_needle = TRUE;
		if (object_is_known(o_ptr))
		{
			damage[i] += o_ptr->to_d * 100;
			to_h[i] += o_ptr->to_h;
		}

		basedam = ((o_ptr->dd + creature_ptr->to_dd[i]) * (o_ptr->ds + creature_ptr->to_ds[i] + 1)) * 50;
		object_flags_known(o_ptr, flgs);

		basedam = calc_expect_crit(creature_ptr, o_ptr->weight, to_h[i], basedam, creature_ptr->dis_to_h[i], poison_needle);
		if ((o_ptr->ident & IDENT_MENTAL) && ((o_ptr->name1 == ART_VORPAL_BLADE) || (o_ptr->name1 == ART_CHAINSWORD)))
		{
			/* vorpal blade */
			basedam *= 5;
			basedam /= 3;
		}
		else if (have_flag(flgs, TR_VORPAL))
		{
			/* vorpal flag only */
			basedam *= 11;
			basedam /= 9;
		}
		if ((creature_ptr->pclass != CLASS_SAMURAI) && have_flag(flgs, TR_FORCE_WEAPON) && (creature_ptr->csp > (o_ptr->dd * o_ptr->ds / 5)))
			basedam = basedam * 7 / 2;
		damage[i] += basedam;
		if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_POISON_NEEDLE)) damage[i] = 1;
		if (damage[i] < 0) damage[i] = 0;
	}

	int blows1 = creature_ptr->migite ? creature_ptr->num_blow[0] : 0;
	int blows2 = creature_ptr->hidarite ? creature_ptr->num_blow[1] : 0;

	/* Basic abilities */

	int xdis = creature_ptr->skill_dis;
	int xdev = creature_ptr->skill_dev;
	int xsav = creature_ptr->skill_sav;
	int xstl = creature_ptr->skill_stl;
	int	xsrh = creature_ptr->skill_srh;
	int	xfos = creature_ptr->skill_fos;
	int xdig = creature_ptr->skill_dig;

	concptr desc = likert(xthn, 12);
	display_player_one_line(ENTRY_SKILL_FIGHT, desc, likert_color);

	desc = likert(xthb, 12);
	display_player_one_line(ENTRY_SKILL_SHOOT, desc, likert_color);

	desc = likert(xsav, 7);
	display_player_one_line(ENTRY_SKILL_SAVING, desc, likert_color);

	/* Hack -- 0 is "minimum stealth value", so print "Very Bad" */
	desc = likert((xstl > 0) ? xstl : -1, 1);
	display_player_one_line(ENTRY_SKILL_STEALTH, desc, likert_color);

	desc = likert(xfos, 6);
	display_player_one_line(ENTRY_SKILL_PERCEP, desc, likert_color);

	desc = likert(xsrh, 6);
	display_player_one_line(ENTRY_SKILL_SEARCH, desc, likert_color);

	desc = likert(xdis, 8);
	display_player_one_line(ENTRY_SKILL_DISARM, desc, likert_color);

	desc = likert(xdev, 6);
	display_player_one_line(ENTRY_SKILL_DEVICE, desc, likert_color);

	desc = likert(xdev, 6);
	display_player_one_line(ENTRY_SKILL_DEVICE, desc, likert_color);

	desc = likert(xdig, 4);
	display_player_one_line(ENTRY_SKILL_DIG, desc, likert_color);

	if (!muta_att)
		display_player_one_line(ENTRY_BLOWS, format("%d+%d", blows1, blows2), TERM_L_BLUE);
	else
		display_player_one_line(ENTRY_BLOWS, format("%d+%d+%d", blows1, blows2, muta_att), TERM_L_BLUE);

	display_player_one_line(ENTRY_SHOTS, format("%d.%02d", shots, shot_frac), TERM_L_BLUE);


	if ((damage[0] + damage[1]) == 0)
		desc = "nil!";
	else
		desc = format("%d+%d", blows1 * damage[0] / 100, blows2 * damage[1] / 100);

	display_player_one_line(ENTRY_AVG_DMG, desc, TERM_L_BLUE);

	display_player_one_line(ENTRY_INFRA, format("%d feet", creature_ptr->see_infra * 10), TERM_WHITE);
}


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
static void player_flags(player_type *creature_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE])
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
		break; /* Do nothing */
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
			if (creature_ptr->lev > 14)
			{
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
			}

			break;
		case RACE_AMBERITE:
			add_flag(flgs, TR_SUST_CON);
			add_flag(flgs, TR_REGEN); /* Amberites heal fast */
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
			/* XXX pass_wall */
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
			; /* Do nothing */
		}
	}

	/* Mutations */
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


/*!
 * @brief プレイヤーの一時的魔法効果による耐性を返す
 * Prints ratings on certain abilities
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param flgs フラグを保管する配列
 * @return なし
 * @todo
 * xtra1.c周りと多重実装になっているのを何とかする
 */
static void tim_player_flags(player_type *creature_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE])
{
	for (int i = 0; i < TR_FLAG_SIZE; i++)
		flgs[i] = 0L;

	if (IS_HERO(creature_ptr) || creature_ptr->shero)
		add_flag(flgs, TR_RES_FEAR);
	if (creature_ptr->tim_invis)
		add_flag(flgs, TR_SEE_INVIS);
	if (creature_ptr->tim_regen)
		add_flag(flgs, TR_REGEN);
	if (is_time_limit_esp(creature_ptr))
		add_flag(flgs, TR_TELEPATHY);
	if (IS_FAST(creature_ptr) || creature_ptr->slow)
		add_flag(flgs, TR_SPEED);

	if (is_oppose_acid(creature_ptr) && !(creature_ptr->special_defense & DEFENSE_ACID) && !(PRACE_IS_(creature_ptr, RACE_YEEK) && (creature_ptr->lev > 19)))
		add_flag(flgs, TR_RES_ACID);
	if (is_oppose_elec(creature_ptr) && !(creature_ptr->special_defense & DEFENSE_ELEC))
		add_flag(flgs, TR_RES_ELEC);
	if (is_oppose_fire(creature_ptr) && !(creature_ptr->special_defense & DEFENSE_FIRE))
		add_flag(flgs, TR_RES_FIRE);
	if (is_oppose_cold(creature_ptr) && !(creature_ptr->special_defense & DEFENSE_COLD))
		add_flag(flgs, TR_RES_COLD);
	if (is_oppose_pois(creature_ptr))
		add_flag(flgs, TR_RES_POIS);

	if (creature_ptr->special_attack & ATTACK_ACID)
		add_flag(flgs, TR_BRAND_ACID);
	if (creature_ptr->special_attack & ATTACK_ELEC)
		add_flag(flgs, TR_BRAND_ELEC);
	if (creature_ptr->special_attack & ATTACK_FIRE)
		add_flag(flgs, TR_BRAND_FIRE);
	if (creature_ptr->special_attack & ATTACK_COLD)
		add_flag(flgs, TR_BRAND_COLD);
	if (creature_ptr->special_attack & ATTACK_POIS)
		add_flag(flgs, TR_BRAND_POIS);
	if (creature_ptr->special_defense & DEFENSE_ACID)
		add_flag(flgs, TR_IM_ACID);
	if (creature_ptr->special_defense & DEFENSE_ELEC)
		add_flag(flgs, TR_IM_ELEC);
	if (creature_ptr->special_defense & DEFENSE_FIRE)
		add_flag(flgs, TR_IM_FIRE);
	if (creature_ptr->special_defense & DEFENSE_COLD)
		add_flag(flgs, TR_IM_COLD);
	if (creature_ptr->wraith_form)
		add_flag(flgs, TR_REFLECT);
	/* by henkma */
	if (creature_ptr->tim_reflect)
		add_flag(flgs, TR_REFLECT);

	if (creature_ptr->magicdef)
	{
		add_flag(flgs, TR_RES_BLIND);
		add_flag(flgs, TR_RES_CONF);
		add_flag(flgs, TR_REFLECT);
		add_flag(flgs, TR_FREE_ACT);
		add_flag(flgs, TR_LEVITATION);
	}

	if (creature_ptr->tim_res_nether) add_flag(flgs, TR_RES_NETHER);

	if (creature_ptr->tim_sh_fire) add_flag(flgs, TR_SH_FIRE);

	if (creature_ptr->ult_res)
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

	if (creature_ptr->realm1 != REALM_HEX) return;

	if (hex_spelling(HEX_DEMON_AURA))
	{
		add_flag(flgs, TR_SH_FIRE);
		add_flag(flgs, TR_REGEN);
	}

	if (hex_spelling(HEX_ICE_ARMOR)) add_flag(flgs, TR_SH_COLD);
	if (hex_spelling(HEX_SHOCK_CLOAK)) add_flag(flgs, TR_SH_ELEC);
}


/*!
 * @brief プレイヤーの装備一覧をシンボルで並べる
 * Equippy chars
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param y 表示するコンソールの行
 * @param x 表示するコンソールの列
 * @param mode オプション
 * @return なし
 */
void display_player_equippy(player_type *creature_ptr, TERM_LEN y, TERM_LEN x, BIT_FLAGS16 mode)
{
	/* Weapon flags need only two column */
	int max_i = (mode & DP_WP) ? INVEN_LARM + 1 : INVEN_TOTAL;

	/* Dump equippy chars */
	for (int i = INVEN_RARM; i < max_i; i++)
	{
		object_type *o_ptr;
		o_ptr = &creature_ptr->inventory_list[i];

		TERM_COLOR a = object_attr(o_ptr);
		char c = object_char(o_ptr);

		/* Clear the part of the screen */
		if (!equippy_chars || !o_ptr->k_idx)
		{
			c = ' ';
			a = TERM_DARK;
		}

		/* Dump */
		Term_putch(x + i - INVEN_RARM, y, a, c);
	}
}


/*!
 * @brief プレイヤーの装備による免疫フラグを返す
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param flgs フラグを保管する配列
 * @return なし
 * @todo
 * xtra1.c周りと多重実装になっているのを何とかする
 */
static void known_obj_immunity(player_type *creature_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE])
{
	for (int i = 0; i < TR_FLAG_SIZE; i++)
		flgs[i] = 0L;

	/* Check equipment */
	for (int i = INVEN_RARM; i < INVEN_TOTAL; i++)
	{
		u32b o_flgs[TR_FLAG_SIZE];

		object_type *o_ptr;

		/* Object */
		o_ptr = &creature_ptr->inventory_list[i];

		if (!o_ptr->k_idx) continue;

		/* Known flags */
		object_flags_known(o_ptr, o_flgs);

		if (have_flag(o_flgs, TR_IM_ACID)) add_flag(flgs, TR_RES_ACID);
		if (have_flag(o_flgs, TR_IM_ELEC)) add_flag(flgs, TR_RES_ELEC);
		if (have_flag(o_flgs, TR_IM_FIRE)) add_flag(flgs, TR_RES_FIRE);
		if (have_flag(o_flgs, TR_IM_COLD)) add_flag(flgs, TR_RES_COLD);
	}
}

/*!
 * @brief プレイヤーの種族による免疫フラグを返す
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param flgs フラグを保管する配列
 * @return なし
 * @todo
 * xtra1.c周りと多重実装になっているのを何とかする
 */
static void player_immunity(player_type *creature_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE])
{
	for (int i = 0; i < TR_FLAG_SIZE; i++)
		flgs[i] = 0L;

	if (PRACE_IS_(creature_ptr, RACE_SPECTRE))
		add_flag(flgs, TR_RES_NETHER);
	if (creature_ptr->mimic_form == MIMIC_VAMPIRE || PRACE_IS_(creature_ptr, RACE_VAMPIRE))
		add_flag(flgs, TR_RES_DARK);
	if (creature_ptr->mimic_form == MIMIC_DEMON_LORD)
		add_flag(flgs, TR_RES_FIRE);
	else if (PRACE_IS_(creature_ptr, RACE_YEEK) && creature_ptr->lev > 19)
		add_flag(flgs, TR_RES_ACID);
}

/*!
 * @brief プレイヤーの一時的魔法効果による免疫フラグを返す
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param flgs フラグを保管する配列
 * @return なし
 * @todo
 * xtra1.c周りと多重実装になっているのを何とかする
 */
static void tim_player_immunity(player_type *creature_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE])
{
	for (int i = 0; i < TR_FLAG_SIZE; i++)
		flgs[i] = 0L;

	if (creature_ptr->special_defense & DEFENSE_ACID)
		add_flag(flgs, TR_RES_ACID);
	if (creature_ptr->special_defense & DEFENSE_ELEC)
		add_flag(flgs, TR_RES_ELEC);
	if (creature_ptr->special_defense & DEFENSE_FIRE)
		add_flag(flgs, TR_RES_FIRE);
	if (creature_ptr->special_defense & DEFENSE_COLD)
		add_flag(flgs, TR_RES_COLD);
	if (creature_ptr->wraith_form)
		add_flag(flgs, TR_RES_DARK);
}

/*!
 * @brief プレイヤーの種族による弱点フラグを返す
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param flgs フラグを保管する配列
 * @return なし
 * @todo
 * xtra1.c周りと多重実装になっているのを何とかする
 */
static void player_vuln_flags(player_type *creature_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE])
{
	for (int i = 0; i < TR_FLAG_SIZE; i++)
		flgs[i] = 0L;

	if ((creature_ptr->muta3 & MUT3_VULN_ELEM) || (creature_ptr->special_defense & KATA_KOUKIJIN))
	{
		add_flag(flgs, TR_RES_ACID);
		add_flag(flgs, TR_RES_ELEC);
		add_flag(flgs, TR_RES_FIRE);
		add_flag(flgs, TR_RES_COLD);
	}
	if (PRACE_IS_(creature_ptr, RACE_ANDROID))
		add_flag(flgs, TR_RES_ELEC);
	if (PRACE_IS_(creature_ptr, RACE_ENT))
		add_flag(flgs, TR_RES_FIRE);
	if (PRACE_IS_(creature_ptr, RACE_VAMPIRE) || PRACE_IS_(creature_ptr, RACE_S_FAIRY) ||
		(creature_ptr->mimic_form == MIMIC_VAMPIRE))
		add_flag(flgs, TR_RES_LITE);
}


/*
 * A struct for storing misc. flags
 */
typedef struct {
	BIT_FLAGS player_flags[TR_FLAG_SIZE];
	BIT_FLAGS tim_player_flags[TR_FLAG_SIZE];
	BIT_FLAGS player_imm[TR_FLAG_SIZE];
	BIT_FLAGS tim_player_imm[TR_FLAG_SIZE];
	BIT_FLAGS player_vuln[TR_FLAG_SIZE];
	BIT_FLAGS known_obj_imm[TR_FLAG_SIZE];
} all_player_flags;


/*!
 * @brief プレイヤーの特性フラグ一種を表示するサブルーチン /
 * Helper function, see below
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param row コンソール表示位置の左上行
 * @param col コンソール表示位置の左上列
 * @param header コンソール上で表示する特性名
 * @param flag1 参照する特性ID
 * @param f プレイヤーの特性情報構造体
 * @param mode 表示オプション
 * @return なし
 */
static void display_flag_aux(player_type *creature_ptr, TERM_LEN row, TERM_LEN col, concptr header, int flag1, all_player_flags *f, u16b mode)
{
	byte header_color = TERM_L_DARK;
	int header_col = col;

	bool vuln = FALSE;
	if (have_flag(f->player_vuln, flag1) &&
		!(have_flag(f->known_obj_imm, flag1) ||
			have_flag(f->player_imm, flag1) ||
			have_flag(f->tim_player_imm, flag1)))
		vuln = TRUE;

	/* Advance */
	col += strlen(header) + 1;

	/* Weapon flags need only two column */
	int max_i = (mode & DP_WP) ? INVEN_LARM + 1 : INVEN_TOTAL;

	/* Check equipment */
	for (int i = INVEN_RARM; i < max_i; i++)
	{
		BIT_FLAGS flgs[TR_FLAG_SIZE];
		object_type *o_ptr;

		/* Object */
		o_ptr = &creature_ptr->inventory_list[i];

		/* Known flags */
		object_flags_known(o_ptr, flgs);

		/* Default */
		if (!(mode & DP_IMM))
			c_put_str((byte)(vuln ? TERM_RED : TERM_SLATE), ".", row, col);

		/* Check flags */
		if (mode & DP_CURSE)
		{
			if (have_flag(flgs, TR_ADD_L_CURSE) || have_flag(flgs, TR_ADD_H_CURSE))
			{
				c_put_str(TERM_L_DARK, "+", row, col);
				header_color = TERM_WHITE;
			}

			if (o_ptr->curse_flags & (TRC_CURSED | TRC_HEAVY_CURSE))
			{
				c_put_str(TERM_WHITE, "+", row, col);
				header_color = TERM_WHITE;
			}

			if (o_ptr->curse_flags & TRC_PERMA_CURSE)
			{
				c_put_str(TERM_WHITE, "*", row, col);
				header_color = TERM_WHITE;
			}

			col++;
			continue;
		}

		if (flag1 == TR_LITE_1)
		{
			if (HAVE_DARK_FLAG(flgs))
			{
				c_put_str(TERM_L_DARK, "+", row, col);
				header_color = TERM_WHITE;
			}
			else if (HAVE_LITE_FLAG(flgs))
			{
				c_put_str(TERM_WHITE, "+", row, col);
				header_color = TERM_WHITE;
			}

			col++;
			continue;
		}

		if (have_flag(flgs, flag1))
		{
			c_put_str((byte)(vuln ? TERM_L_RED : TERM_WHITE),
				(mode & DP_IMM) ? "*" : "+", row, col);
			header_color = TERM_WHITE;
		}

		col++;
	}

	/* Assume that player flag is already written */
	if (mode & DP_IMM)
	{
		if (header_color != TERM_L_DARK)
		{
			/* Overwrite Header Color */
			c_put_str(header_color, header, row, header_col);
		}

		return;
	}

	/* Default */
	c_put_str((byte)(vuln ? TERM_RED : TERM_SLATE), ".", row, col);

	/* Player flags */
	if (have_flag(f->player_flags, flag1))
	{
		c_put_str((byte)(vuln ? TERM_L_RED : TERM_WHITE), "+", row, col);
		header_color = TERM_WHITE;
	}

	/* Timed player flags */
	if (have_flag(f->tim_player_flags, flag1))
	{
		c_put_str((byte)(vuln ? TERM_ORANGE : TERM_YELLOW), "#", row, col);
		header_color = TERM_WHITE;
	}

	/* Immunity */
	if (have_flag(f->tim_player_imm, flag1))
	{
		c_put_str(TERM_YELLOW, "*", row, col);
		header_color = TERM_WHITE;
	}
	if (have_flag(f->player_imm, flag1))
	{
		c_put_str(TERM_WHITE, "*", row, col);
		header_color = TERM_WHITE;
	}

	/* Vulnerability */
	if (vuln) c_put_str(TERM_RED, "v", row, col + 1);

	/* Header */
	c_put_str(header_color, header, row, header_col);
}


/*!
 * @brief プレイヤーの特性フラグ一覧表示１ /
 * @param creature_ptr プレーヤーへの参照ポインタ
 * Special display, part 1
 * @return なし
 */
static void display_player_flag_info(player_type *creature_ptr)
{
	/* Extract flags and store */
	all_player_flags f;
	player_flags(creature_ptr, f.player_flags);
	tim_player_flags(creature_ptr, f.tim_player_flags);
	player_immunity(creature_ptr, f.player_imm);
	tim_player_immunity(creature_ptr, f.tim_player_imm);
	known_obj_immunity(creature_ptr, f.known_obj_imm);
	player_vuln_flags(creature_ptr, f.player_vuln);

	/*** Set 1 ***/
	TERM_LEN row = 12;
	TERM_LEN col = 1;
	display_player_equippy(creature_ptr, row - 2, col + 8, 0);
	c_put_str(TERM_WHITE, "abcdefghijkl@", row - 1, col + 8);

#ifdef JP
	display_flag_aux(creature_ptr, row + 0, col, "耐酸  :", TR_RES_ACID, &f, 0);
	display_flag_aux(creature_ptr, row + 0, col, "耐酸  :", TR_IM_ACID, &f, DP_IMM);
	display_flag_aux(creature_ptr, row + 1, col, "耐電撃:", TR_RES_ELEC, &f, 0);
	display_flag_aux(creature_ptr, row + 1, col, "耐電撃:", TR_IM_ELEC, &f, DP_IMM);
	display_flag_aux(creature_ptr, row + 2, col, "耐火炎:", TR_RES_FIRE, &f, 0);
	display_flag_aux(creature_ptr, row + 2, col, "耐火炎:", TR_IM_FIRE, &f, DP_IMM);
	display_flag_aux(creature_ptr, row + 3, col, "耐冷気:", TR_RES_COLD, &f, 0);
	display_flag_aux(creature_ptr, row + 3, col, "耐冷気:", TR_IM_COLD, &f, DP_IMM);
	display_flag_aux(creature_ptr, row + 4, col, "耐毒  :", TR_RES_POIS, &f, 0);
	display_flag_aux(creature_ptr, row + 5, col, "耐閃光:", TR_RES_LITE, &f, 0);
	display_flag_aux(creature_ptr, row + 6, col, "耐暗黒:", TR_RES_DARK, &f, 0);
	display_flag_aux(creature_ptr, row + 7, col, "耐破片:", TR_RES_SHARDS, &f, 0);
	display_flag_aux(creature_ptr, row + 8, col, "耐盲目:", TR_RES_BLIND, &f, 0);
	display_flag_aux(creature_ptr, row + 9, col, "耐混乱:", TR_RES_CONF, &f, 0);
#else
	display_flag_aux(creature_ptr, row + 0, col, "Acid  :", TR_RES_ACID, &f, 0);
	display_flag_aux(creature_ptr, row + 0, col, "Acid  :", TR_IM_ACID, &f, DP_IMM);
	display_flag_aux(creature_ptr, row + 1, col, "Elec  :", TR_RES_ELEC, &f, 0);
	display_flag_aux(creature_ptr, row + 1, col, "Elec  :", TR_IM_ELEC, &f, DP_IMM);
	display_flag_aux(creature_ptr, row + 2, col, "Fire  :", TR_RES_FIRE, &f, 0);
	display_flag_aux(creature_ptr, row + 2, col, "Fire  :", TR_IM_FIRE, &f, DP_IMM);
	display_flag_aux(creature_ptr, row + 3, col, "Cold  :", TR_RES_COLD, &f, 0);
	display_flag_aux(creature_ptr, row + 3, col, "Cold  :", TR_IM_COLD, &f, DP_IMM);
	display_flag_aux(creature_ptr, row + 4, col, "Poison:", TR_RES_POIS, &f, 0);
	display_flag_aux(creature_ptr, row + 5, col, "Light :", TR_RES_LITE, &f, 0);
	display_flag_aux(creature_ptr, row + 6, col, "Dark  :", TR_RES_DARK, &f, 0);
	display_flag_aux(creature_ptr, row + 7, col, "Shard :", TR_RES_SHARDS, &f, 0);
	display_flag_aux(creature_ptr, row + 8, col, "Blind :", TR_RES_BLIND, &f, 0);
	display_flag_aux(creature_ptr, row + 9, col, "Conf  :", TR_RES_CONF, &f, 0);
#endif

	/*** Set 2 ***/
	row = 12;
	col = 26;
	display_player_equippy(creature_ptr, row - 2, col + 8, 0);
	c_put_str(TERM_WHITE, "abcdefghijkl@", row - 1, col + 8);

#ifdef JP
	display_flag_aux(creature_ptr, row + 0, col, "耐轟音:", TR_RES_SOUND, &f, 0);
	display_flag_aux(creature_ptr, row + 1, col, "耐地獄:", TR_RES_NETHER, &f, 0);
	display_flag_aux(creature_ptr, row + 2, col, "耐因混:", TR_RES_NEXUS, &f, 0);
	display_flag_aux(creature_ptr, row + 3, col, "耐カオ:", TR_RES_CHAOS, &f, 0);
	display_flag_aux(creature_ptr, row + 4, col, "耐劣化:", TR_RES_DISEN, &f, 0);
	display_flag_aux(creature_ptr, row + 5, col, "耐恐怖:", TR_RES_FEAR, &f, 0);
	display_flag_aux(creature_ptr, row + 6, col, "反射  :", TR_REFLECT, &f, 0);
	display_flag_aux(creature_ptr, row + 7, col, "火炎オ:", TR_SH_FIRE, &f, 0);
	display_flag_aux(creature_ptr, row + 8, col, "電気オ:", TR_SH_ELEC, &f, 0);
	display_flag_aux(creature_ptr, row + 9, col, "冷気オ:", TR_SH_COLD, &f, 0);
#else
	display_flag_aux(creature_ptr, row + 0, col, "Sound :", TR_RES_SOUND, &f, 0);
	display_flag_aux(creature_ptr, row + 1, col, "Nether:", TR_RES_NETHER, &f, 0);
	display_flag_aux(creature_ptr, row + 2, col, "Nexus :", TR_RES_NEXUS, &f, 0);
	display_flag_aux(creature_ptr, row + 3, col, "Chaos :", TR_RES_CHAOS, &f, 0);
	display_flag_aux(creature_ptr, row + 4, col, "Disnch:", TR_RES_DISEN, &f, 0);
	display_flag_aux(creature_ptr, row + 5, col, "Fear  :", TR_RES_FEAR, &f, 0);
	display_flag_aux(creature_ptr, row + 6, col, "Reflct:", TR_REFLECT, &f, 0);
	display_flag_aux(creature_ptr, row + 7, col, "AuFire:", TR_SH_FIRE, &f, 0);
	display_flag_aux(creature_ptr, row + 8, col, "AuElec:", TR_SH_ELEC, &f, 0);
	display_flag_aux(creature_ptr, row + 9, col, "AuCold:", TR_SH_COLD, &f, 0);
#endif

	/*** Set 3 ***/
	row = 12;
	col = 51;
	display_player_equippy(creature_ptr, row - 2, col + 12, 0);
	c_put_str(TERM_WHITE, "abcdefghijkl@", row - 1, col + 12);

#ifdef JP
	display_flag_aux(creature_ptr, row + 0, col, "加速      :", TR_SPEED, &f, 0);
	display_flag_aux(creature_ptr, row + 1, col, "耐麻痺    :", TR_FREE_ACT, &f, 0);
	display_flag_aux(creature_ptr, row + 2, col, "透明体視認:", TR_SEE_INVIS, &f, 0);
	display_flag_aux(creature_ptr, row + 3, col, "経験値保持:", TR_HOLD_EXP, &f, 0);
	display_flag_aux(creature_ptr, row + 4, col, "警告      :", TR_WARNING, &f, 0);
	display_flag_aux(creature_ptr, row + 5, col, "遅消化    :", TR_SLOW_DIGEST, &f, 0);
	display_flag_aux(creature_ptr, row + 6, col, "急回復    :", TR_REGEN, &f, 0);
	display_flag_aux(creature_ptr, row + 7, col, "浮遊      :", TR_LEVITATION, &f, 0);
	display_flag_aux(creature_ptr, row + 8, col, "永遠光源  :", TR_LITE_1, &f, 0);
	display_flag_aux(creature_ptr, row + 9, col, "呪い      :", 0, &f, DP_CURSE);
#else
	display_flag_aux(creature_ptr, row + 0, col, "Speed     :", TR_SPEED, &f, 0);
	display_flag_aux(creature_ptr, row + 1, col, "FreeAction:", TR_FREE_ACT, &f, 0);
	display_flag_aux(creature_ptr, row + 2, col, "SeeInvisi.:", TR_SEE_INVIS, &f, 0);
	display_flag_aux(creature_ptr, row + 3, col, "Hold Exp  :", TR_HOLD_EXP, &f, 0);
	display_flag_aux(creature_ptr, row + 4, col, "Warning   :", TR_WARNING, &f, 0);
	display_flag_aux(creature_ptr, row + 5, col, "SlowDigest:", TR_SLOW_DIGEST, &f, 0);
	display_flag_aux(creature_ptr, row + 6, col, "Regene.   :", TR_REGEN, &f, 0);
	display_flag_aux(creature_ptr, row + 7, col, "Levitation:", TR_LEVITATION, &f, 0);
	display_flag_aux(creature_ptr, row + 8, col, "Perm Lite :", TR_LITE_1, &f, 0);
	display_flag_aux(creature_ptr, row + 9, col, "Cursed    :", 0, &f, DP_CURSE);
#endif
}


/*!
 * @brief プレイヤーの特性フラグ一覧表示2 /
 * @param creature_ptr プレーヤーへの参照ポインタ
 * Special display, part 2
 * @return なし
 */
static void display_player_other_flag_info(player_type *creature_ptr)
{
	/* Extract flags and store */
	all_player_flags f;
	player_flags(creature_ptr, f.player_flags);
	tim_player_flags(creature_ptr, f.tim_player_flags);
	player_immunity(creature_ptr, f.player_imm);
	tim_player_immunity(creature_ptr, f.tim_player_imm);
	known_obj_immunity(creature_ptr, f.known_obj_imm);
	player_vuln_flags(creature_ptr, f.player_vuln);

	/*** Set 1 ***/
	TERM_LEN row = 3;
	TERM_LEN col = 1;
	display_player_equippy(creature_ptr, row - 2, col + 12, DP_WP);
	c_put_str(TERM_WHITE, "ab@", row - 1, col + 12);

#ifdef JP
	display_flag_aux(creature_ptr, row + 0, col, "邪悪 倍打 :", TR_SLAY_EVIL, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 0, col, "邪悪 倍打 :", TR_KILL_EVIL, &f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 1, col, "不死 倍打 :", TR_SLAY_UNDEAD, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 1, col, "不死 倍打 :", TR_KILL_UNDEAD, &f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 2, col, "悪魔 倍打 :", TR_SLAY_DEMON, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 2, col, "悪魔 倍打 :", TR_KILL_DEMON, &f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 3, col, "龍 倍打   :", TR_SLAY_DRAGON, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 3, col, "龍 倍打   :", TR_KILL_DRAGON, &f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 4, col, "人間 倍打 :", TR_SLAY_HUMAN, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 4, col, "人間 倍打 :", TR_KILL_HUMAN, &f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 5, col, "動物 倍打 :", TR_SLAY_ANIMAL, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 5, col, "動物 倍打 :", TR_KILL_ANIMAL, &f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 6, col, "オーク倍打:", TR_SLAY_ORC, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 6, col, "オーク倍打:", TR_KILL_ORC, &f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 7, col, "トロル倍打:", TR_SLAY_TROLL, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 7, col, "トロル倍打:", TR_KILL_TROLL, &f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 8, col, "巨人 倍打 :", TR_SLAY_GIANT, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 8, col, "巨人 倍打 :", TR_KILL_GIANT, &f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 9, col, "溶解      :", TR_BRAND_ACID, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 10, col, "電撃      :", TR_BRAND_ELEC, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 11, col, "焼棄      :", TR_BRAND_FIRE, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 12, col, "凍結      :", TR_BRAND_COLD, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 13, col, "毒殺      :", TR_BRAND_POIS, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 14, col, "切れ味    :", TR_VORPAL, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 15, col, "地震      :", TR_IMPACT, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 16, col, "吸血      :", TR_VAMPIRIC, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 17, col, "カオス効果:", TR_CHAOTIC, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 18, col, "理力      :", TR_FORCE_WEAPON, &f, DP_WP);
#else
	display_flag_aux(creature_ptr, row + 0, col, "Slay Evil :", TR_SLAY_EVIL, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 0, col, "Slay Evil :", TR_KILL_EVIL, &f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 1, col, "Slay Und. :", TR_SLAY_UNDEAD, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 1, col, "Slay Und. :", TR_KILL_UNDEAD, &f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 2, col, "Slay Demon:", TR_SLAY_DEMON, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 2, col, "Slay Demon:", TR_KILL_DEMON, &f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 3, col, "Slay Drag.:", TR_SLAY_DRAGON, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 3, col, "Slay Drag.:", TR_KILL_DRAGON, &f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 4, col, "Slay Human:", TR_SLAY_HUMAN, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 4, col, "Slay Human:", TR_KILL_HUMAN, &f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 5, col, "Slay Anim.:", TR_SLAY_ANIMAL, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 5, col, "Slay Anim.:", TR_KILL_ANIMAL, &f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 6, col, "Slay Orc  :", TR_SLAY_ORC, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 6, col, "Slay Orc  :", TR_KILL_ORC, &f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 7, col, "Slay Troll:", TR_SLAY_TROLL, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 7, col, "Slay Troll:", TR_KILL_TROLL, &f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 8, col, "Slay Giant:", TR_SLAY_GIANT, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 8, col, "Slay Giant:", TR_KILL_GIANT, &f, (DP_WP | DP_IMM));
	display_flag_aux(creature_ptr, row + 9, col, "Acid Brand:", TR_BRAND_ACID, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 10, col, "Elec Brand:", TR_BRAND_ELEC, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 11, col, "Fire Brand:", TR_BRAND_FIRE, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 12, col, "Cold Brand:", TR_BRAND_COLD, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 13, col, "Poison Brd:", TR_BRAND_POIS, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 14, col, "Sharpness :", TR_VORPAL, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 15, col, "Quake     :", TR_IMPACT, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 16, col, "Vampiric  :", TR_VAMPIRIC, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 17, col, "Chaotic   :", TR_CHAOTIC, &f, DP_WP);
	display_flag_aux(creature_ptr, row + 18, col, "Force Wep.:", TR_FORCE_WEAPON, &f, DP_WP);
#endif

	/*** Set 2 ***/
	row = 3;
	col = col + 12 + 7;
	display_player_equippy(creature_ptr, row - 2, col + 13, 0);
	c_put_str(TERM_WHITE, "abcdefghijkl@", row - 1, col + 13);

#ifdef JP
	display_flag_aux(creature_ptr, row + 0, col, "テレパシー :", TR_TELEPATHY, &f, 0);
	display_flag_aux(creature_ptr, row + 1, col, "邪悪ESP    :", TR_ESP_EVIL, &f, 0);
	display_flag_aux(creature_ptr, row + 2, col, "無生物ESP  :", TR_ESP_NONLIVING, &f, 0);
	display_flag_aux(creature_ptr, row + 3, col, "善良ESP    :", TR_ESP_GOOD, &f, 0);
	display_flag_aux(creature_ptr, row + 4, col, "不死ESP    :", TR_ESP_UNDEAD, &f, 0);
	display_flag_aux(creature_ptr, row + 5, col, "悪魔ESP    :", TR_ESP_DEMON, &f, 0);
	display_flag_aux(creature_ptr, row + 6, col, "龍ESP      :", TR_ESP_DRAGON, &f, 0);
	display_flag_aux(creature_ptr, row + 7, col, "人間ESP    :", TR_ESP_HUMAN, &f, 0);
	display_flag_aux(creature_ptr, row + 8, col, "動物ESP    :", TR_ESP_ANIMAL, &f, 0);
	display_flag_aux(creature_ptr, row + 9, col, "オークESP  :", TR_ESP_ORC, &f, 0);
	display_flag_aux(creature_ptr, row + 10, col, "トロルESP  :", TR_ESP_TROLL, &f, 0);
	display_flag_aux(creature_ptr, row + 11, col, "巨人ESP    :", TR_ESP_GIANT, &f, 0);
	display_flag_aux(creature_ptr, row + 12, col, "ユニークESP:", TR_ESP_UNIQUE, &f, 0);
	display_flag_aux(creature_ptr, row + 13, col, "腕力維持   :", TR_SUST_STR, &f, 0);
	display_flag_aux(creature_ptr, row + 14, col, "知力維持   :", TR_SUST_INT, &f, 0);
	display_flag_aux(creature_ptr, row + 15, col, "賢さ維持   :", TR_SUST_WIS, &f, 0);
	display_flag_aux(creature_ptr, row + 16, col, "器用維持   :", TR_SUST_DEX, &f, 0);
	display_flag_aux(creature_ptr, row + 17, col, "耐久維持   :", TR_SUST_CON, &f, 0);
	display_flag_aux(creature_ptr, row + 18, col, "魅力維持   :", TR_SUST_CHR, &f, 0);
#else
	display_flag_aux(creature_ptr, row + 0, col, "Telepathy  :", TR_TELEPATHY, &f, 0);
	display_flag_aux(creature_ptr, row + 1, col, "ESP Evil   :", TR_ESP_EVIL, &f, 0);
	display_flag_aux(creature_ptr, row + 2, col, "ESP Noliv. :", TR_ESP_NONLIVING, &f, 0);
	display_flag_aux(creature_ptr, row + 3, col, "ESP Good   :", TR_ESP_GOOD, &f, 0);
	display_flag_aux(creature_ptr, row + 4, col, "ESP Undead :", TR_ESP_UNDEAD, &f, 0);
	display_flag_aux(creature_ptr, row + 5, col, "ESP Demon  :", TR_ESP_DEMON, &f, 0);
	display_flag_aux(creature_ptr, row + 6, col, "ESP Dragon :", TR_ESP_DRAGON, &f, 0);
	display_flag_aux(creature_ptr, row + 7, col, "ESP Human  :", TR_ESP_HUMAN, &f, 0);
	display_flag_aux(creature_ptr, row + 8, col, "ESP Animal :", TR_ESP_ANIMAL, &f, 0);
	display_flag_aux(creature_ptr, row + 9, col, "ESP Orc    :", TR_ESP_ORC, &f, 0);
	display_flag_aux(creature_ptr, row + 10, col, "ESP Troll  :", TR_ESP_TROLL, &f, 0);
	display_flag_aux(creature_ptr, row + 11, col, "ESP Giant  :", TR_ESP_GIANT, &f, 0);
	display_flag_aux(creature_ptr, row + 12, col, "ESP Unique :", TR_ESP_UNIQUE, &f, 0);
	display_flag_aux(creature_ptr, row + 13, col, "Sust Str   :", TR_SUST_STR, &f, 0);
	display_flag_aux(creature_ptr, row + 14, col, "Sust Int   :", TR_SUST_INT, &f, 0);
	display_flag_aux(creature_ptr, row + 15, col, "Sust Wis   :", TR_SUST_WIS, &f, 0);
	display_flag_aux(creature_ptr, row + 16, col, "Sust Dex   :", TR_SUST_DEX, &f, 0);
	display_flag_aux(creature_ptr, row + 17, col, "Sust Con   :", TR_SUST_CON, &f, 0);
	display_flag_aux(creature_ptr, row + 18, col, "Sust Chr   :", TR_SUST_CHR, &f, 0);
#endif

	/*** Set 3 ***/
	row = 3;
	col = col + 12 + 17;
	display_player_equippy(creature_ptr, row - 2, col + 14, 0);
	c_put_str(TERM_WHITE, "abcdefghijkl@", row - 1, col + 14);

#ifdef JP
	display_flag_aux(creature_ptr, row + 0, col, "追加攻撃    :", TR_BLOWS, &f, 0);
	display_flag_aux(creature_ptr, row + 1, col, "採掘        :", TR_TUNNEL, &f, 0);
	display_flag_aux(creature_ptr, row + 2, col, "赤外線視力  :", TR_INFRA, &f, 0);
	display_flag_aux(creature_ptr, row + 3, col, "魔法道具支配:", TR_MAGIC_MASTERY, &f, 0);
	display_flag_aux(creature_ptr, row + 4, col, "隠密        :", TR_STEALTH, &f, 0);
	display_flag_aux(creature_ptr, row + 5, col, "探索        :", TR_SEARCH, &f, 0);

	display_flag_aux(creature_ptr, row + 7, col, "乗馬        :", TR_RIDING, &f, 0);
	display_flag_aux(creature_ptr, row + 8, col, "投擲        :", TR_THROW, &f, 0);
	display_flag_aux(creature_ptr, row + 9, col, "祝福        :", TR_BLESSED, &f, 0);
	display_flag_aux(creature_ptr, row + 10, col, "反テレポート:", TR_NO_TELE, &f, 0);
	display_flag_aux(creature_ptr, row + 11, col, "反魔法      :", TR_NO_MAGIC, &f, 0);
	display_flag_aux(creature_ptr, row + 12, col, "消費魔力減少:", TR_DEC_MANA, &f, 0);

	display_flag_aux(creature_ptr, row + 14, col, "経験値減少  :", TR_DRAIN_EXP, &f, 0);
	display_flag_aux(creature_ptr, row + 15, col, "乱テレポート:", TR_TELEPORT, &f, 0);
	display_flag_aux(creature_ptr, row + 16, col, "反感        :", TR_AGGRAVATE, &f, 0);
	display_flag_aux(creature_ptr, row + 17, col, "太古の怨念  :", TR_TY_CURSE, &f, 0);
#else
	display_flag_aux(creature_ptr, row + 0, col, "Add Blows   :", TR_BLOWS, &f, 0);
	display_flag_aux(creature_ptr, row + 1, col, "Add Tunnel  :", TR_TUNNEL, &f, 0);
	display_flag_aux(creature_ptr, row + 2, col, "Add Infra   :", TR_INFRA, &f, 0);
	display_flag_aux(creature_ptr, row + 3, col, "Add Device  :", TR_MAGIC_MASTERY, &f, 0);
	display_flag_aux(creature_ptr, row + 4, col, "Add Stealth :", TR_STEALTH, &f, 0);
	display_flag_aux(creature_ptr, row + 5, col, "Add Search  :", TR_SEARCH, &f, 0);

	display_flag_aux(creature_ptr, row + 7, col, "Riding      :", TR_RIDING, &f, 0);
	display_flag_aux(creature_ptr, row + 8, col, "Throw       :", TR_THROW, &f, 0);
	display_flag_aux(creature_ptr, row + 9, col, "Blessed     :", TR_BLESSED, &f, 0);
	display_flag_aux(creature_ptr, row + 10, col, "No Teleport :", TR_NO_TELE, &f, 0);
	display_flag_aux(creature_ptr, row + 11, col, "Anti Magic  :", TR_NO_MAGIC, &f, 0);
	display_flag_aux(creature_ptr, row + 12, col, "Econom. Mana:", TR_DEC_MANA, &f, 0);

	display_flag_aux(creature_ptr, row + 14, col, "Drain Exp   :", TR_DRAIN_EXP, &f, 0);
	display_flag_aux(creature_ptr, row + 15, col, "Rnd.Teleport:", TR_TELEPORT, &f, 0);
	display_flag_aux(creature_ptr, row + 16, col, "Aggravate   :", TR_AGGRAVATE, &f, 0);
	display_flag_aux(creature_ptr, row + 17, col, "TY Curse    :", TR_TY_CURSE, &f, 0);
#endif

}


/*!
 * @brief プレイヤーの特性フラグ一覧表示2a /
 * @param creature_ptr プレーヤーへの参照ポインタ
 * Special display, part 2a
 * @return なし
 */
static void display_player_misc_info(player_type *creature_ptr)
{
	/* Display basics */
#ifdef JP
	put_str("名前  :", 1, 26);
	put_str("性別  :", 3, 1);
	put_str("種族  :", 4, 1);
	put_str("職業  :", 5, 1);
#else
	put_str("Name  :", 1, 26);
	put_str("Sex   :", 3, 1);
	put_str("Race  :", 4, 1);
	put_str("Class :", 5, 1);
#endif

	char	buf[80];
	char	tmp[80];
	strcpy(tmp, ap_ptr->title);
#ifdef JP
	if (ap_ptr->no == 1)
		strcat(tmp, "の");
#else
	strcat(tmp, " ");
#endif
	strcat(tmp, creature_ptr->name);

	c_put_str(TERM_L_BLUE, tmp, 1, 34);
	c_put_str(TERM_L_BLUE, sp_ptr->title, 3, 9);
	c_put_str(TERM_L_BLUE, (creature_ptr->mimic_form ? mimic_info[creature_ptr->mimic_form].title : rp_ptr->title), 4, 9);
	c_put_str(TERM_L_BLUE, cp_ptr->title, 5, 9);

	/* Display extras */
#ifdef JP
	put_str("レベル:", 6, 1);
	put_str("ＨＰ  :", 7, 1);
	put_str("ＭＰ  :", 8, 1);
#else
	put_str("Level :", 6, 1);
	put_str("Hits  :", 7, 1);
	put_str("Mana  :", 8, 1);
#endif

	(void)sprintf(buf, "%d", (int)creature_ptr->lev);
	c_put_str(TERM_L_BLUE, buf, 6, 9);
	(void)sprintf(buf, "%d/%d", (int)creature_ptr->chp, (int)creature_ptr->mhp);
	c_put_str(TERM_L_BLUE, buf, 7, 9);
	(void)sprintf(buf, "%d/%d", (int)creature_ptr->csp, (int)creature_ptr->msp);
	c_put_str(TERM_L_BLUE, buf, 8, 9);
}


/*!
 * @brief プレイヤーの特性フラグ一覧表示2b /
 * Special display, part 2b
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * <pre>
 * How to print out the modifications and sustains.
 * Positive mods with no sustain will be light green.
 * Positive mods with a sustain will be dark green.
 * Sustains (with no modification) will be a dark green 's'.
 * Negative mods (from a curse) will be red.
 * Huge mods (>9), like from MICoMorgoth, will be a '*'
 * No mod, no sustain, will be a slate '.'
 * </pre>
 */
static void display_player_stat_info(player_type *creature_ptr)
{
	/* Print out the labels for the columns */
	int stat_col = 22;
	int row = 3;
	c_put_str(TERM_WHITE, _("能力", "Stat"), row, stat_col + 1);
	c_put_str(TERM_BLUE, _("  基本", "  Base"), row, stat_col + 7);
	c_put_str(TERM_L_BLUE, _(" 種 職 性 装 ", "RacClaPerMod"), row, stat_col + 13);
	c_put_str(TERM_L_GREEN, _("合計", "Actual"), row, stat_col + 28);
	c_put_str(TERM_YELLOW, _("現在", "Current"), row, stat_col + 35);

	/* Display the stats */
	char buf[80];
	for (int i = 0; i < A_MAX; i++)
	{
		int r_adj;
		if (creature_ptr->mimic_form) r_adj = mimic_info[creature_ptr->mimic_form].r_adj[i];
		else r_adj = rp_ptr->r_adj[i];

		/* Calculate equipment adjustment */
		int e_adj = 0;

		/* Icky formula to deal with the 18 barrier */
		if ((creature_ptr->stat_max[i] > 18) && (creature_ptr->stat_top[i] > 18))
			e_adj = (creature_ptr->stat_top[i] - creature_ptr->stat_max[i]) / 10;
		if ((creature_ptr->stat_max[i] <= 18) && (creature_ptr->stat_top[i] <= 18))
			e_adj = creature_ptr->stat_top[i] - creature_ptr->stat_max[i];
		if ((creature_ptr->stat_max[i] <= 18) && (creature_ptr->stat_top[i] > 18))
			e_adj = (creature_ptr->stat_top[i] - 18) / 10 - creature_ptr->stat_max[i] + 18;

		if ((creature_ptr->stat_max[i] > 18) && (creature_ptr->stat_top[i] <= 18))
			e_adj = creature_ptr->stat_top[i] - (creature_ptr->stat_max[i] - 19) / 10 - 19;

		if (PRACE_IS_(creature_ptr, RACE_ENT))
		{
			switch (i)
			{
			case A_STR:
			case A_CON:
				if (creature_ptr->lev > 25) r_adj++;
				if (creature_ptr->lev > 40) r_adj++;
				if (creature_ptr->lev > 45) r_adj++;
				break;
			case A_DEX:
				if (creature_ptr->lev > 25) r_adj--;
				if (creature_ptr->lev > 40) r_adj--;
				if (creature_ptr->lev > 45) r_adj--;
				break;
			}
		}

		e_adj -= r_adj;
		e_adj -= cp_ptr->c_adj[i];
		e_adj -= ap_ptr->a_adj[i];

		if (creature_ptr->stat_cur[i] < creature_ptr->stat_max[i])
			/* Reduced name of stat */
			c_put_str(TERM_WHITE, stat_names_reduced[i], row + i + 1, stat_col + 1);
		else
			c_put_str(TERM_WHITE, stat_names[i], row + i + 1, stat_col + 1);


		/* Internal "natural" max value.  Maxes at 18/100 */
		/* This is useful to see if you are maxed out */
		cnv_stat(creature_ptr->stat_max[i], buf);
		if (creature_ptr->stat_max[i] == creature_ptr->stat_max_max[i])
		{
			c_put_str(TERM_WHITE, "!", row + i + 1, _(stat_col + 6, stat_col + 4));
		}

		c_put_str(TERM_BLUE, buf, row + i + 1, stat_col + 13 - strlen(buf));

		/* Race, class, and equipment modifiers */
		(void)sprintf(buf, "%3d", r_adj);
		c_put_str(TERM_L_BLUE, buf, row + i + 1, stat_col + 13);
		(void)sprintf(buf, "%3d", (int)cp_ptr->c_adj[i]);
		c_put_str(TERM_L_BLUE, buf, row + i + 1, stat_col + 16);
		(void)sprintf(buf, "%3d", (int)ap_ptr->a_adj[i]);
		c_put_str(TERM_L_BLUE, buf, row + i + 1, stat_col + 19);
		(void)sprintf(buf, "%3d", (int)e_adj);
		c_put_str(TERM_L_BLUE, buf, row + i + 1, stat_col + 22);

		/* Actual maximal modified value */
		cnv_stat(creature_ptr->stat_top[i], buf);
		c_put_str(TERM_L_GREEN, buf, row + i + 1, stat_col + 26);

		/* Only display stat_use if not maximal */
		if (creature_ptr->stat_use[i] < creature_ptr->stat_top[i])
		{
			cnv_stat(creature_ptr->stat_use[i], buf);
			c_put_str(TERM_YELLOW, buf, row + i + 1, stat_col + 33);
		}
	}

	/* Header and Footer */
	int col = stat_col + 41;
	c_put_str(TERM_WHITE, "abcdefghijkl@", row, col);
	c_put_str(TERM_L_GREEN, _("能力修正", "Modification"), row - 1, col);

	/* Process equipment */
	BIT_FLAGS flgs[TR_FLAG_SIZE];
	for (int i = INVEN_RARM; i < INVEN_TOTAL; i++)
	{
		object_type *o_ptr;
		o_ptr = &creature_ptr->inventory_list[i];

		/* Acquire "known" flags */
		object_flags_known(o_ptr, flgs);

		/* Initialize color based of sign of pval. */
		for (int stat = 0; stat < A_MAX; stat++)
		{
			byte a = TERM_SLATE;
			char c = '.';

			/* Boost */
			if (have_flag(flgs, stat))
			{
				/* Default */
				c = '*';

				/* Good */
				if (o_ptr->pval > 0)
				{
					/* Good */
					a = TERM_L_GREEN;

					/* Label boost */
					if (o_ptr->pval < 10) c = '0' + o_ptr->pval;
				}

				if (have_flag(flgs, stat + TR_SUST_STR))
				{
					/* Dark green for sustained stats */
					a = TERM_GREEN;
				}

				/* Bad */
				if (o_ptr->pval < 0)
				{
					/* Bad */
					a = TERM_RED;

					/* Label boost */
					if (o_ptr->pval > -10) c = '0' - o_ptr->pval;
				}
			}

			/* Sustain */
			else if (have_flag(flgs, stat + TR_SUST_STR))
			{
				/* Dark green "s" */
				a = TERM_GREEN;
				c = 's';
			}

			/* Dump proper character */
			Term_putch(col, row + stat + 1, a, c);
		}

		/* Advance */
		col++;
	}

	player_flags(creature_ptr, flgs);

	/* Check stats */
	for (int stat = 0; stat < A_MAX; stat++)
	{
		byte a = TERM_SLATE;
		char c = '.';

		if (creature_ptr->muta3 || creature_ptr->tsuyoshi)
		{
			int dummy = 0;

			if (stat == A_STR)
			{
				if (creature_ptr->muta3 & MUT3_HYPER_STR) dummy += 4;
				if (creature_ptr->muta3 & MUT3_PUNY) dummy -= 4;
				if (creature_ptr->tsuyoshi) dummy += 4;
			}
			else if (stat == A_WIS || stat == A_INT)
			{
				if (creature_ptr->muta3 & MUT3_HYPER_INT) dummy += 4;
				if (creature_ptr->muta3 & MUT3_MORONIC) dummy -= 4;
			}
			else if (stat == A_DEX)
			{
				if (creature_ptr->muta3 & MUT3_IRON_SKIN) dummy -= 1;
				if (creature_ptr->muta3 & MUT3_LIMBER) dummy += 3;
				if (creature_ptr->muta3 & MUT3_ARTHRITIS) dummy -= 3;
			}
			else if (stat == A_CON)
			{
				if (creature_ptr->muta3 & MUT3_RESILIENT) dummy += 4;
				if (creature_ptr->muta3 & MUT3_XTRA_FAT) dummy += 2;
				if (creature_ptr->muta3 & MUT3_ALBINO) dummy -= 4;
				if (creature_ptr->muta3 & MUT3_FLESH_ROT) dummy -= 2;
				if (creature_ptr->tsuyoshi) dummy += 4;
			}
			else if (stat == A_CHR)
			{
				if (creature_ptr->muta3 & MUT3_SILLY_VOI) dummy -= 4;
				if (creature_ptr->muta3 & MUT3_BLANK_FAC) dummy -= 1;
				if (creature_ptr->muta3 & MUT3_FLESH_ROT) dummy -= 1;
				if (creature_ptr->muta3 & MUT3_SCALES) dummy -= 1;
				if (creature_ptr->muta3 & MUT3_WART_SKIN) dummy -= 2;
				if (creature_ptr->muta3 & MUT3_ILL_NORM) dummy = 0;
			}

			/* Boost */
			if (dummy)
			{
				c = '*';

				/* Good */
				if (dummy > 0)
				{
					/* Good */
					a = TERM_L_GREEN;

					/* Label boost */
					if (dummy < 10) c = '0' + dummy;
				}

				/* Bad */
				if (dummy < 0)
				{
					/* Bad */
					a = TERM_RED;

					/* Label boost */
					if (dummy > -10) c = '0' - dummy;
				}
			}
		}

		if (have_flag(flgs, stat + TR_SUST_STR))
		{
			/* Dark green "s" */
			a = TERM_GREEN;
			c = 's';
		}

		Term_putch(col, row + stat + 1, a, c);
	}
}


/*!
 * @brief プレイヤーのステータス表示メイン処理
 * Display the character on the screen (various modes)
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param mode 表示モードID
 * @return なし
 * @details
 * <pre>
 * The top one and bottom two lines are left blank.
 * Mode 0 = standard display with skills
 * Mode 1 = standard display with history
 * Mode 2 = summary of various things
 * Mode 3 = summary of various things (part 2)
 * Mode 4 = mutations
 * </pre>
 */
void display_player(player_type *creature_ptr, int mode)
{
	if ((creature_ptr->muta1 || creature_ptr->muta2 || creature_ptr->muta3) && display_mutations)
		mode = (mode % 5);
	else
		mode = (mode % 4);

	clear_from(0);

	if (mode == 2)
	{
		/* See "http://www.cs.berkeley.edu/~davidb/angband.html" */
		display_player_misc_info(creature_ptr);
		display_player_stat_info(creature_ptr);
		display_player_flag_info(creature_ptr);
		return;
	}

	if (mode == 3)
	{
		display_player_other_flag_info(creature_ptr);
		return;
	}

	if (mode == 4)
	{
		do_cmd_knowledge_mutations(creature_ptr);
		return;
	}

	char tmp[64];
	if ((mode != 0) && (mode != 1)) return;

	/* Name, Sex, Race, Class */
#ifdef JP
	sprintf(tmp, "%s%s%s", ap_ptr->title, ap_ptr->no == 1 ? "の" : "", creature_ptr->name);
#else
	sprintf(tmp, "%s %s", ap_ptr->title, creature_ptr->name);
#endif

	display_player_one_line(ENTRY_NAME, tmp, TERM_L_BLUE);
	display_player_one_line(ENTRY_SEX, sp_ptr->title, TERM_L_BLUE);
	display_player_one_line(ENTRY_RACE, (creature_ptr->mimic_form ? mimic_info[creature_ptr->mimic_form].title : rp_ptr->title), TERM_L_BLUE);
	display_player_one_line(ENTRY_CLASS, cp_ptr->title, TERM_L_BLUE);

	if (creature_ptr->realm1)
	{
		if (creature_ptr->realm2)
			sprintf(tmp, "%s, %s", realm_names[creature_ptr->realm1], realm_names[creature_ptr->realm2]);
		else
			strcpy(tmp, realm_names[creature_ptr->realm1]);
		display_player_one_line(ENTRY_REALM, tmp, TERM_L_BLUE);
	}

	if ((creature_ptr->pclass == CLASS_CHAOS_WARRIOR) || (creature_ptr->muta2 & MUT2_CHAOS_GIFT))
		display_player_one_line(ENTRY_PATRON, chaos_patrons[creature_ptr->chaos_patron], TERM_L_BLUE);

	/* Age, Height, Weight, Social */
	/* 身長はセンチメートルに、体重はキログラムに変更してあります */
#ifdef JP
	display_player_one_line(ENTRY_AGE, format("%d才", (int)creature_ptr->age), TERM_L_BLUE);
	display_player_one_line(ENTRY_HEIGHT, format("%dcm", (int)((creature_ptr->ht * 254) / 100)), TERM_L_BLUE);
	display_player_one_line(ENTRY_WEIGHT, format("%dkg", (int)((creature_ptr->wt * 4536) / 10000)), TERM_L_BLUE);
	display_player_one_line(ENTRY_SOCIAL, format("%d  ", (int)creature_ptr->sc), TERM_L_BLUE);
#else
	display_player_one_line(ENTRY_AGE, format("%d", (int)creature_ptr->age), TERM_L_BLUE);
	display_player_one_line(ENTRY_HEIGHT, format("%d", (int)creature_ptr->ht), TERM_L_BLUE);
	display_player_one_line(ENTRY_WEIGHT, format("%d", (int)creature_ptr->wt), TERM_L_BLUE);
	display_player_one_line(ENTRY_SOCIAL, format("%d", (int)creature_ptr->sc), TERM_L_BLUE);
#endif
	display_player_one_line(ENTRY_ALIGN, format("%s", your_alignment(creature_ptr)), TERM_L_BLUE);


	/* Display the stats */
	int i;
	char buf[80];
	for (i = 0; i < A_MAX; i++)
	{
		/* Special treatment of "injured" stats */
		if (creature_ptr->stat_cur[i] < creature_ptr->stat_max[i])
		{
			int value;

			/* Use lowercase stat name */
			put_str(stat_names_reduced[i], 3 + i, 53);

			/* Get the current stat */
			value = creature_ptr->stat_use[i];

			/* Obtain the current stat (modified) */
			cnv_stat(value, buf);

			/* Display the current stat (modified) */
			c_put_str(TERM_YELLOW, buf, 3 + i, 60);

			/* Acquire the max stat */
			value = creature_ptr->stat_top[i];

			/* Obtain the maximum stat (modified) */
			cnv_stat(value, buf);

			/* Display the maximum stat (modified) */
			c_put_str(TERM_L_GREEN, buf, 3 + i, 67);
		}

		/* Normal treatment of "normal" stats */
		else
		{
			/* Assume uppercase stat name */
			put_str(stat_names[i], 3 + i, 53);

			/* Obtain the current stat (modified) */
			cnv_stat(creature_ptr->stat_use[i], buf);

			/* Display the current stat (modified) */
			c_put_str(TERM_L_GREEN, buf, 3 + i, 60);
		}

		if (creature_ptr->stat_max[i] == creature_ptr->stat_max_max[i])
		{
			c_put_str(TERM_WHITE, "!", 3 + i, _(58, 58 - 2));
		}
	}

	/* Display "history" info */
	floor_type *floor_ptr = creature_ptr->current_floor_ptr;
	if (mode == 0)
	{
		display_player_middle(creature_ptr);
		display_player_various(creature_ptr);
		return;
	}

	char statmsg[1000];
	put_str(_("(キャラクターの生い立ち)", "(Character Background)"), 11, 25);

	for (i = 0; i < 4; i++)
	{
		put_str(creature_ptr->history[i], i + 12, 10);
	}

	*statmsg = '\0';

	if (creature_ptr->is_dead)
	{
		if (current_world_ptr->total_winner)
		{
#ifdef JP
			sprintf(statmsg, "…あなたは勝利の後%sした。", streq(creature_ptr->died_from, "Seppuku") ? "切腹" : "引退");
#else
			sprintf(statmsg, "...You %s after the winning.", streq(creature_ptr->died_from, "Seppuku") ? "did Seppuku" : "retired from the adventure");
#endif
		}
		else if (!floor_ptr->dun_level)
		{
#ifdef JP
			sprintf(statmsg, "…あなたは%sで%sに殺された。", map_name(creature_ptr), creature_ptr->died_from);
#else
			sprintf(statmsg, "...You were killed by %s in %s.", creature_ptr->died_from, map_name(creature_ptr));
#endif
		}
		else if (floor_ptr->inside_quest && is_fixed_quest_idx(floor_ptr->inside_quest))
		{
			/* Get the quest text */
			/* Bewere that INIT_ASSIGN resets the cur_num. */
			init_flags = INIT_NAME_ONLY;

			process_dungeon_file(creature_ptr, "q_info.txt", 0, 0, 0, 0);

#ifdef JP
			sprintf(statmsg, "…あなたは、クエスト「%s」で%sに殺された。", quest[floor_ptr->inside_quest].name, creature_ptr->died_from);
#else
			sprintf(statmsg, "...You were killed by %s in the quest '%s'.", creature_ptr->died_from, quest[floor_ptr->inside_quest].name);
#endif
		}
		else
		{
#ifdef JP
			sprintf(statmsg, "…あなたは、%sの%d階で%sに殺された。", map_name(creature_ptr), (int)floor_ptr->dun_level, creature_ptr->died_from);
#else
			sprintf(statmsg, "...You were killed by %s on level %d of %s.", creature_ptr->died_from, floor_ptr->dun_level, map_name(creature_ptr));
#endif
		}
	}
	else if (current_world_ptr->character_dungeon)
	{
		if (!floor_ptr->dun_level)
		{
			sprintf(statmsg, _("…あなたは現在、 %s にいる。", "...Now, you are in %s."), map_name(creature_ptr));
		}
		else if (floor_ptr->inside_quest && is_fixed_quest_idx(floor_ptr->inside_quest))
		{
			/* Clear the text */
			/* Must be done before doing INIT_SHOW_TEXT */
			for (i = 0; i < 10; i++)
			{
				quest_text[i][0] = '\0';
			}
			quest_text_line = 0;

			/* Get the quest text */
			init_flags = INIT_NAME_ONLY;

			process_dungeon_file(creature_ptr, "q_info.txt", 0, 0, 0, 0);

			sprintf(statmsg, _("…あなたは現在、 クエスト「%s」を遂行中だ。", "...Now, you are in the quest '%s'."), quest[floor_ptr->inside_quest].name);
		}
		else
		{
#ifdef JP
			sprintf(statmsg, "…あなたは現在、 %s の %d 階で探索している。", map_name(creature_ptr), (int)floor_ptr->dun_level);
#else
			sprintf(statmsg, "...Now, you are exploring level %d of %s.", floor_ptr->dun_level, map_name(creature_ptr));
#endif
		}
	}

	if (!*statmsg) return;

	char temp[64 * 2];
	roff_to_buf(statmsg, 60, temp, sizeof(temp));
	char  *t;
	t = temp;
	for (i = 0; i < 2; i++)
	{
		if (t[0] == 0) return;

		put_str(t, i + 5 + 12, 10);
		t += strlen(t) + 1;
	}
}


/*!
 * @brief プレイヤーのステータス表示をファイルにダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_display_player(player_type *creature_ptr, FILE *fff)
{
	TERM_LEN x, y;
	TERM_COLOR a;
	char c;
	char buf[1024];

	display_player(creature_ptr, 0);

	/* Dump part of the screen */
	for (y = 1; y < 22; y++)
	{
		/* Dump each row */
		for (x = 0; x < 79; x++)
		{
			/* Get the attr/char */
			(void)(Term_what(x, y, &a, &c));

			/* Dump it */
			buf[x] = c;
		}

		/* End the string */
		buf[x] = '\0';

		/* Kill trailing spaces */
		while ((x > 0) && (buf[x - 1] == ' ')) buf[--x] = '\0';

		/* End the row */
		fprintf(fff, _("%s\n", "%s\n"), buf);
	}

	/* Display history */
	display_player(creature_ptr, 1);

	/* Dump part of the screen */
	for (y = 10; y < 19; y++)
	{
		/* Dump each row */
		for (x = 0; x < 79; x++)
		{
			/* Get the attr/char */
			(void)(Term_what(x, y, &a, &c));

			/* Dump it */
			buf[x] = c;
		}

		/* End the string */
		buf[x] = '\0';

		/* Kill trailing spaces */
		while ((x > 0) && (buf[x - 1] == ' ')) buf[--x] = '\0';

		/* End the row */
		fprintf(fff, "%s\n", buf);
	}

	fprintf(fff, "\n");

	/* Display flags (part 1) */
	display_player(creature_ptr, 2);

	/* Dump part of the screen */
	for (y = 2; y < 22; y++)
	{
		/* Dump each row */
		for (x = 0; x < 79; x++)
		{
			/* Get the attr/char */
			(void)(Term_what(x, y, &a, &c));

			/* Dump it (Ignore equippy tile graphic) */
			if (a < 128)
				buf[x] = c;
			else
				buf[x] = ' ';
		}

		/* End the string */
		buf[x] = '\0';

		/* Kill trailing spaces */
		while ((x > 0) && (buf[x - 1] == ' ')) buf[--x] = '\0';

		/* End the row */
		fprintf(fff, "%s\n", buf);
	}

	fprintf(fff, "\n");

	/* Display flags (part 2) */
	display_player(creature_ptr, 3);

	/* Dump part of the screen */
	for (y = 1; y < 22; y++)
	{
		/* Dump each row */
		for (x = 0; x < 79; x++)
		{
			/* Get the attr/char */
			(void)(Term_what(x, y, &a, &c));

			/* Dump it (Ignore equippy tile graphic) */
			if (a < 128)
				buf[x] = c;
			else
				buf[x] = ' ';
		}

		/* End the string */
		buf[x] = '\0';

		/* Kill trailing spaces */
		while ((x > 0) && (buf[x - 1] == ' ')) buf[--x] = '\0';

		/* End the row */
		fprintf(fff, "%s\n", buf);
	}

	fprintf(fff, "\n");
}


/*!
 * @brief プレイヤーのペット情報をファイルにダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_pet(player_type *master_ptr, FILE *fff)
{
	bool pet = FALSE;
	bool pet_settings = FALSE;
	for (int i = master_ptr->current_floor_ptr->m_max - 1; i >= 1; i--)
	{
		monster_type *m_ptr = &master_ptr->current_floor_ptr->m_list[i];

		if (!monster_is_valid(m_ptr)) continue;
		if (!is_pet(m_ptr)) continue;
		pet_settings = TRUE;
		if (!m_ptr->nickname && (master_ptr->riding != i)) continue;
		if (!pet)
		{
			fprintf(fff, _("\n\n  [主なペット]\n\n", "\n\n  [Leading Pets]\n\n"));
			pet = TRUE;
		}

		GAME_TEXT pet_name[MAX_NLEN];
		monster_desc(pet_name, m_ptr, MD_ASSUME_VISIBLE | MD_INDEF_VISIBLE);
		fprintf(fff, "%s\n", pet_name);
	}

	if (!pet_settings) return;

	fprintf(fff, _("\n\n  [ペットへの命令]\n", "\n\n  [Command for Pets]\n"));

	fprintf(fff, _("\n ドアを開ける:                       %s", "\n Pets open doors:                    %s"),
		(master_ptr->pet_extra_flags & PF_OPEN_DOORS) ? "ON" : "OFF");

	fprintf(fff, _("\n アイテムを拾う:                     %s", "\n Pets pick up items:                 %s"),
		(master_ptr->pet_extra_flags & PF_PICKUP_ITEMS) ? "ON" : "OFF");

	fprintf(fff, _("\n テレポート系魔法を使う:             %s", "\n Allow teleport:                     %s"),
		(master_ptr->pet_extra_flags & PF_TELEPORT) ? "ON" : "OFF");

	fprintf(fff, _("\n 攻撃魔法を使う:                     %s", "\n Allow cast attack spell:            %s"),
		(master_ptr->pet_extra_flags & PF_ATTACK_SPELL) ? "ON" : "OFF");

	fprintf(fff, _("\n 召喚魔法を使う:                     %s", "\n Allow cast summon spell:            %s"),
		(master_ptr->pet_extra_flags & PF_SUMMON_SPELL) ? "ON" : "OFF");

	fprintf(fff, _("\n プレイヤーを巻き込む範囲魔法を使う: %s", "\n Allow involve player in area spell: %s"),
		(master_ptr->pet_extra_flags & PF_BALL_SPELL) ? "ON" : "OFF");

	fputc('\n', fff);
}


/*!
 * @brief プレイヤーの職業能力情報をファイルにダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_class_special(player_type *creature_ptr, FILE *fff)
{
	bool is_special_class = creature_ptr->pclass == CLASS_MAGIC_EATER;
	is_special_class |= creature_ptr->pclass == CLASS_SMITH;
	is_special_class |= creature_ptr->pclass != CLASS_BLUE_MAGE;
	if (!is_special_class) return;

	if (creature_ptr->pclass == CLASS_MAGIC_EATER)
	{
		char s[EATER_EXT][MAX_NLEN];
		OBJECT_TYPE_VALUE tval = 0;
		int ext;
		KIND_OBJECT_IDX k_idx;
		OBJECT_SUBTYPE_VALUE i;
		int magic_num;

		fprintf(fff, _("\n\n  [取り込んだ魔法道具]\n", "\n\n  [Magic devices eaten]\n"));

		for (ext = 0; ext < 3; ext++)
		{
			int eat_num = 0;

			/* Dump an extent name */
			switch (ext)
			{
			case 0:
				tval = TV_STAFF;
				fprintf(fff, _("\n[杖]\n", "\n[Staffs]\n"));
				break;
			case 1:
				tval = TV_WAND;
				fprintf(fff, _("\n[魔法棒]\n", "\n[Wands]\n"));
				break;
			case 2:
				tval = TV_ROD;
				fprintf(fff, _("\n[ロッド]\n", "\n[Rods]\n"));
				break;
			}

			/* Get magic device names that were eaten */
			for (i = 0; i < EATER_EXT; i++)
			{
				int idx = EATER_EXT * ext + i;

				magic_num = creature_ptr->magic_num2[idx];
				if (!magic_num) continue;

				k_idx = lookup_kind(tval, i);
				if (!k_idx) continue;
				sprintf(s[eat_num], "%23s (%2d)", (k_name + k_info[k_idx].name), magic_num);
				eat_num++;
			}

			/* Dump magic devices in this extent */
			if (eat_num > 0)
			{
				for (i = 0; i < eat_num; i++)
				{
					fputs(s[i], fff);
					if (i % 3 < 2) fputs("    ", fff);
					else fputs("\n", fff);
				}

				if (i % 3 > 0) fputs("\n", fff);
			}
			else /* Not found */
			{
				fputs(_("  (なし)\n", "  (none)\n"), fff);
			}
		}

		return;
	}

	if (creature_ptr->pclass == CLASS_SMITH)
	{
		int i, id[250], n = 0, row;

		fprintf(fff, _("\n\n  [手に入れたエッセンス]\n\n", "\n\n  [Get Essence]\n\n"));
		fprintf(fff, _("エッセンス   個数     エッセンス   個数     エッセンス   個数",
			"Essence      Num      Essence      Num      Essence      Num "));
		for (i = 0; essence_name[i]; i++)
		{
			if (!essence_name[i][0]) continue;
			id[n] = i;
			n++;
		}

		row = n / 3 + 1;

		for (i = 0; i < row; i++)
		{
			fprintf(fff, "\n");
			fprintf(fff, "%-11s %5d     ", essence_name[id[i]], (int)creature_ptr->magic_num1[id[i]]);
			if (i + row < n) fprintf(fff, "%-11s %5d     ", essence_name[id[i + row]], (int)creature_ptr->magic_num1[id[i + row]]);
			if (i + row * 2 < n) fprintf(fff, "%-11s %5d", essence_name[id[i + row * 2]], (int)creature_ptr->magic_num1[id[i + row * 2]]);
		}

		fputs("\n", fff);
		return;
	}

	// Blue mage
	int l1 = 0;
	int l2 = 0;
	int spellnum[MAX_MONSPELLS];
	BIT_FLAGS f4 = 0, f5 = 0, f6 = 0;
	char p[60][80];
	int col = 0;
	bool pcol = FALSE;

	for (int i = 0; i < 60; i++)
	{
		p[i][0] = '\0';
	}

	strcat(p[col], _("\n\n  [学習済みの青魔法]\n", "\n\n  [Learned Blue Magic]\n"));

	for (int j = 1; j < 6; j++)
	{
		col++;
		set_rf_masks(&f4, &f5, &f6, j);
		switch (j)
		{
		case MONSPELL_TYPE_BOLT:
			strcat(p[col], _("\n     [ボルト型]\n", "\n     [Bolt  Type]\n"));
			break;

		case MONSPELL_TYPE_BALL:
			strcat(p[col], _("\n     [ボール型]\n", "\n     [Ball  Type]\n"));
			break;

		case MONSPELL_TYPE_BREATH:
			strcat(p[col], _("\n     [ブレス型]\n", "\n     [  Breath  ]\n"));
			break;

		case MONSPELL_TYPE_SUMMON:
			strcat(p[col], _("\n     [召喚魔法]\n", "\n     [Summonning]\n"));
			break;

		case MONSPELL_TYPE_OTHER:
			strcat(p[col], _("\n     [ その他 ]\n", "\n     [Other Type]\n"));
			break;
		}

		int num = 0;
		for (int i = 0; i < 32; i++)
		{
			if ((0x00000001 << i) & f4) spellnum[num++] = i;
		}

		for (int i = 32; i < 64; i++)
		{
			if ((0x00000001 << (i - 32)) & f5) spellnum[num++] = i;
		}

		for (int i = 64; i < 96; i++)
		{
			if ((0x00000001 << (i - 64)) & f6) spellnum[num++] = i;
		}

		col++;
		pcol = FALSE;
		strcat(p[col], "       ");

		for (int i = 0; i < num; i++)
		{
			if (creature_ptr->magic_num2[spellnum[i]] == 0) continue;

			pcol = TRUE;
			/* Dump blue magic */
			l1 = strlen(p[col]);
			l2 = strlen(monster_powers_short[spellnum[i]]);
			if ((l1 + l2) >= 75)
			{
				strcat(p[col], "\n");
				col++;
				strcat(p[col], "       ");
			}

			strcat(p[col], monster_powers_short[spellnum[i]]);
			strcat(p[col], ", ");
		}

		if (!pcol)
		{
			strcat(p[col], _("なし", "None"));
			strcat(p[col], "\n");
			continue;
		}

		if (p[col][strlen(p[col]) - 2] == ',')
		{
			p[col][strlen(p[col]) - 2] = '\0';
		}
		else
		{
			p[col][strlen(p[col]) - 10] = '\0';
		}

		strcat(p[col], "\n");
	}

	for (int i = 0; i <= col; i++)
	{
		fputs(p[i], fff);
	}
}


/*!
 * @brief クエスト情報をファイルにダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_quest(player_type *creature_ptr, FILE *fff)
{
	fprintf(fff, _("\n\n  [クエスト情報]\n", "\n\n  [Quest Information]\n"));

	/* Allocate Memory */
	QUEST_IDX *quest_num;
	C_MAKE(quest_num, max_q_idx, QUEST_IDX);

	/* Sort by compete level */
	for (QUEST_IDX i = 1; i < max_q_idx; i++) quest_num[i] = i;
	int dummy;
	ang_sort(quest_num, &dummy, max_q_idx, ang_sort_comp_quest_num, ang_sort_swap_quest_num);

	/* Dump Quest Information */
	fputc('\n', fff);
	do_cmd_knowledge_quests_completed(creature_ptr, fff, quest_num);
	fputc('\n', fff);
	do_cmd_knowledge_quests_failed(creature_ptr, fff, quest_num);
	fputc('\n', fff);

	/* Free Memory */
	C_KILL(quest_num, max_q_idx, QUEST_IDX);
}


/*!
 * @brief 死の直前メッセージ並びに遺言をファイルにダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_last_message(player_type *creature_ptr, FILE *fff)
{
	if (!creature_ptr->is_dead) return;

	if (!current_world_ptr->total_winner)
	{
		fprintf(fff, _("\n  [死ぬ直前のメッセージ]\n\n", "\n  [Last Messages]\n\n"));
		for (int i = MIN(message_num(), 30); i >= 0; i--)
		{
			fprintf(fff, "> %s\n", message_str((s16b)i));
		}

		fputc('\n', fff);
		return;
	}

	/* Hack -- *Winning* message */
	if (creature_ptr->last_message)
	{
		fprintf(fff, _("\n  [*勝利*メッセージ]\n\n", "\n  [*Winning* Message]\n\n"));
		fprintf(fff, "  %s\n", creature_ptr->last_message);
		fputc('\n', fff);
	}
}


/*!
 * @brief 帰還場所情報をファイルにダンプする
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_recall(FILE *fff)
{
	fprintf(fff, _("\n  [帰還場所]\n\n", "\n  [Recall Depth]\n\n"));

	for (int y = 1; y < current_world_ptr->max_d_idx; y++)
	{
		bool seiha = FALSE;

		if (!d_info[y].maxdepth) continue;
		if (!max_dlv[y]) continue;
		if (d_info[y].final_guardian)
		{
			if (!r_info[d_info[y].final_guardian].max_num) seiha = TRUE;
		}
		else if (max_dlv[y] == d_info[y].maxdepth) seiha = TRUE;

		fprintf(fff, _("   %c%-12s: %3d 階\n", "   %c%-16s: level %3d\n"),
			seiha ? '!' : ' ', d_name + d_info[y].name, (int)max_dlv[y]);
	}
}


/*!
 * @brief オプション情報をファイルにダンプする
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_options(FILE *fff)
{
	fprintf(fff, _("\n  [オプション設定]\n", "\n  [Option Settings]\n"));

	if (preserve_mode)
		fprintf(fff, _("\n 保存モード:         ON", "\n Preserve Mode:      ON"));

	else
		fprintf(fff, _("\n 保存モード:         OFF", "\n Preserve Mode:      OFF"));

	if (ironman_small_levels)
		fprintf(fff, _("\n 小さいダンジョン:   ALWAYS", "\n Small Levels:       ALWAYS"));
	else if (always_small_levels)
		fprintf(fff, _("\n 小さいダンジョン:   ON", "\n Small Levels:       ON"));
	else if (small_levels)
		fprintf(fff, _("\n 小さいダンジョン:   ENABLED", "\n Small Levels:       ENABLED"));
	else
		fprintf(fff, _("\n 小さいダンジョン:   OFF", "\n Small Levels:       OFF"));

	if (vanilla_town)
		fprintf(fff, _("\n 元祖の町のみ:       ON", "\n Vanilla Town:       ON"));
	else if (lite_town)
		fprintf(fff, _("\n 小規模な町:         ON", "\n Lite Town:          ON"));

	if (ironman_shops)
		fprintf(fff, _("\n 店なし:             ON", "\n No Shops:           ON"));

	if (ironman_downward)
		fprintf(fff, _("\n 階段を上がれない:   ON", "\n Diving Only:        ON"));

	if (ironman_rooms)
		fprintf(fff, _("\n 普通でない部屋:     ON", "\n Unusual Rooms:      ON"));

	if (ironman_nightmare)
		fprintf(fff, _("\n 悪夢モード:         ON", "\n Nightmare Mode:     ON"));

	if (ironman_empty_levels)
		fprintf(fff, _("\n アリーナ:           ALWAYS", "\n Arena Levels:       ALWAYS"));
	else if (empty_levels)
		fprintf(fff, _("\n アリーナ:           ENABLED", "\n Arena Levels:       ENABLED"));
	else
		fprintf(fff, _("\n アリーナ:           OFF", "\n Arena Levels:       OFF"));

	fputc('\n', fff);

	if (current_world_ptr->noscore)
		fprintf(fff, _("\n 何か不正なことをしてしまっています。\n", "\n You have done something illegal.\n"));

	fputc('\n', fff);
}


/*!
 * @brief 闘技場の情報をファイルにダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_arena(player_type *creature_ptr, FILE *fff)
{
	if (lite_town || vanilla_town) return;

	if (creature_ptr->arena_number < 0)
	{
		if (creature_ptr->arena_number <= ARENA_DEFEATED_OLD_VER)
		{
			fprintf(fff, _("\n 闘技場: 敗北\n", "\n Arena: Defeated\n"));
		}
		else
		{
#ifdef JP
			fprintf(fff, "\n 闘技場: %d回戦で%sの前に敗北\n", -creature_ptr->arena_number,
				r_name + r_info[arena_info[-1 - creature_ptr->arena_number].r_idx].name);
#else
			fprintf(fff, "\n Arena: Defeated by %s in the %d%s fight\n",
				r_name + r_info[arena_info[-1 - creature_ptr->arena_number].r_idx].name,
				-creature_ptr->arena_number, get_ordinal_number_suffix(-creature_ptr->arena_number));
#endif
		}

		fprintf(fff, "\n");
		return;
	}

	if (creature_ptr->arena_number > MAX_ARENA_MONS + 2)
	{
		fprintf(fff, _("\n 闘技場: 真のチャンピオン\n", "\n Arena: True Champion\n"));
		fprintf(fff, "\n");
		return;
	}
	
	if (creature_ptr->arena_number > MAX_ARENA_MONS - 1)
	{
		fprintf(fff, _("\n 闘技場: チャンピオン\n", "\n Arena: Champion\n"));
		fprintf(fff, "\n");
		return;
	}

#ifdef JP
	fprintf(fff, "\n 闘技場: %2d勝\n", (creature_ptr->arena_number > MAX_ARENA_MONS ? MAX_ARENA_MONS : creature_ptr->arena_number));
#else
	fprintf(fff, "\n Arena: %2d Victor%s\n", (creature_ptr->arena_number > MAX_ARENA_MONS ? MAX_ARENA_MONS : creature_ptr->arena_number), (creature_ptr->arena_number > 1) ? "ies" : "y");
#endif
	fprintf(fff, "\n");
}


/*!
 * @brief 撃破モンスターの情報をファイルにダンプする
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_monsters(FILE *fff)
{
	fprintf(fff, _("\n  [倒したモンスター]\n\n", "\n  [Defeated Monsters]\n\n"));

	/* Allocate the "who" array */
	MONRACE_IDX *who;
	u16b why = 2;
	C_MAKE(who, max_r_idx, MONRACE_IDX);

	/* Count monster kills */
	long uniq_total = 0;
	long norm_total = 0;
	for (IDX k = 1; k < max_r_idx; k++)
	{
		/* Ignore unused index */
		monster_race *r_ptr = &r_info[k];
		if (!r_ptr->name) continue;

		if (r_ptr->flags1 & RF1_UNIQUE)
		{
			bool dead = (r_ptr->max_num == 0);
			if (dead)
			{
				norm_total++;

				/* Add a unique monster to the list */
				who[uniq_total++] = k;
			}

			continue;
		}

		if (r_ptr->r_pkills > 0)
		{
			norm_total += r_ptr->r_pkills;
		}
	}

	/* No monsters is defeated */
	if (norm_total < 1)
	{
		fprintf(fff, _("まだ敵を倒していません。\n", "You have defeated no enemies yet.\n"));
		C_KILL(who, max_r_idx, s16b);
		return;
	}

	/* Defeated more than one normal monsters */
	if (uniq_total == 0)
	{
#ifdef JP
		fprintf(fff, "%ld体の敵を倒しています。\n", norm_total);
#else
		fprintf(fff, "You have defeated %ld %s.\n", norm_total, norm_total == 1 ? "enemy" : "enemies");
#endif
		C_KILL(who, max_r_idx, s16b);
		return;
	}

	/* Defeated more than one unique monsters */
#ifdef JP
	fprintf(fff, "%ld体のユニーク・モンスターを含む、合計%ld体の敵を倒しています。\n", uniq_total, norm_total);
#else
	fprintf(fff, "You have defeated %ld %s including %ld unique monster%s in total.\n", norm_total, norm_total == 1 ? "enemy" : "enemies", uniq_total, (uniq_total == 1 ? "" : "s"));
#endif

	/* Sort the array by dungeon depth of monsters */
	ang_sort(who, &why, uniq_total, ang_sort_comp_hook, ang_sort_swap_hook);
	fprintf(fff, _("\n《上位%ld体のユニーク・モンスター》\n", "\n< Unique monsters top %ld >\n"), MIN(uniq_total, 10));

	for (IDX k = uniq_total - 1; k >= 0 && k >= uniq_total - 10; k--)
	{
		monster_race *r_ptr = &r_info[who[k]];
		fprintf(fff, _("  %-40s (レベル%3d)\n", "  %-40s (level %3d)\n"), (r_name + r_ptr->name), (int)r_ptr->level);
	}

	C_KILL(who, max_r_idx, s16b);
}


/*!
 * @brief 元種族情報をファイルにダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_race_history(player_type *creature_ptr, FILE *fff)
{
	if (!creature_ptr->old_race1 && !creature_ptr->old_race2) return;

	fprintf(fff, _("\n\n あなたは%sとして生まれた。", "\n\n You were born as %s."), race_info[creature_ptr->start_race].title);
	for (int i = 0; i < MAX_RACES; i++)
	{
		if (creature_ptr->start_race == i) continue;
		if (i < 32)
		{
			if (!(creature_ptr->old_race1 & 1L << i)) continue;
		}
		else
		{
			if (!(creature_ptr->old_race2 & 1L << (i - 32))) continue;
		}

		fprintf(fff, _("\n あなたはかつて%sだった。", "\n You were a %s before."), race_info[i].title);
	}

	fputc('\n', fff);
}


/*!
 * @brief 元魔法領域情報をファイルにダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_realm_history(player_type *creature_ptr, FILE *fff)
{
	if (creature_ptr->old_realm) return;

	fputc('\n', fff);
	for (int i = 0; i < MAX_MAGIC; i++)
	{
		if (!(creature_ptr->old_realm & 1L << i)) continue;
		fprintf(fff, _("\n あなたはかつて%s魔法を使えた。", "\n You were able to use %s magic before."), realm_names[i + 1]);
	}

	fputc('\n', fff);
}


/*!
 * @brief 徳の情報をファイルにダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_virtues(player_type *creature_ptr, FILE *fff)
{
	fprintf(fff, _("\n\n  [自分に関する情報]\n\n", "\n\n  [HP-rate & Max stat & Virtues]\n\n"));

	int percent = (int)(((long)creature_ptr->player_hp[PY_MAX_LEVEL - 1] * 200L) /
		(2 * creature_ptr->hitdie +
		((PY_MAX_LEVEL - 1 + 3) * (creature_ptr->hitdie + 1))));

#ifdef JP
	if (creature_ptr->knowledge & KNOW_HPRATE) fprintf(fff, "現在の体力ランク : %d/100\n\n", percent);
	else fprintf(fff, "現在の体力ランク : ???\n\n");
	fprintf(fff, "能力の最大値\n");
#else
	if (creature_ptr->knowledge & KNOW_HPRATE) fprintf(fff, "Your current Life Rating is %d/100.\n\n", percent);
	else fprintf(fff, "Your current Life Rating is ???.\n\n");
	fprintf(fff, "Limits of maximum stats\n");
#endif
	for (int v_nr = 0; v_nr < A_MAX; v_nr++)
	{
		if ((creature_ptr->knowledge & KNOW_STAT) || creature_ptr->stat_max[v_nr] == creature_ptr->stat_max_max[v_nr]) fprintf(fff, "%s 18/%d\n", stat_names[v_nr], creature_ptr->stat_max_max[v_nr] - 18);
		else fprintf(fff, "%s ???\n", stat_names[v_nr]);
	}

	fprintf(fff, _("\n属性 : %s\n", "\nYour alighnment : %s\n"), your_alignment(creature_ptr));
	fprintf(fff, "\n");
	dump_virtues(creature_ptr, fff);
}


/*!
 * @brief 突然変異の情報をファイルにダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_mutations(player_type *creature_ptr, FILE *fff)
{
	if (creature_ptr->muta1 || creature_ptr->muta2 || creature_ptr->muta3)
	{
		fprintf(fff, _("\n\n  [突然変異]\n\n", "\n\n  [Mutations]\n\n"));
		dump_mutations(creature_ptr, fff);
	}
}


/*!
 * @brief 所持品の情報をファイルにダンプする
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_equipment_inventory(player_type *creature_ptr, FILE *fff)
{
	GAME_TEXT o_name[MAX_NLEN];
	if (creature_ptr->equip_cnt)
	{
		fprintf(fff, _("  [キャラクタの装備]\n\n", "  [Character Equipment]\n\n"));
		for (int i = INVEN_RARM; i < INVEN_TOTAL; i++)
		{
			object_desc(o_name, &creature_ptr->inventory_list[i], 0);
			if ((((i == INVEN_RARM) && creature_ptr->hidarite) || ((i == INVEN_LARM) && creature_ptr->migite)) && creature_ptr->ryoute)
				strcpy(o_name, _("(武器を両手持ち)", "(wielding with two-hands)"));

			fprintf(fff, "%c) %s\n",
				index_to_label(i), o_name);
		}

		fprintf(fff, "\n\n");
	}

	fprintf(fff, _("  [キャラクタの持ち物]\n\n", "  [Character Inventory]\n\n"));

	for (int i = 0; i < INVEN_PACK; i++)
	{
		if (!creature_ptr->inventory_list[i].k_idx) break;
		object_desc(o_name, &creature_ptr->inventory_list[i], 0);
		fprintf(fff, "%c) %s\n", index_to_label(i), o_name);
	}

	fprintf(fff, "\n\n");
}


/*!
 * @brief 我が家と博物館のオブジェクト情報をファイルにダンプする
 * @param fff ファイルポインタ
 * @return なし
 */
static void dump_aux_home_museum(FILE *fff)
{
	store_type  *st_ptr;
	st_ptr = &town_info[1].store[STORE_HOME];

	GAME_TEXT o_name[MAX_NLEN];
	if (st_ptr->stock_num)
	{
		fprintf(fff, _("  [我が家のアイテム]\n", "  [Home Inventory]\n"));

		TERM_LEN x = 1;
		for (int i = 0; i < st_ptr->stock_num; i++)
		{
			if ((i % 12) == 0)
				fprintf(fff, _("\n ( %d ページ )\n", "\n ( page %d )\n"), x++);
			object_desc(o_name, &st_ptr->stock[i], 0);
			fprintf(fff, "%c) %s\n", I2A(i % 12), o_name);
		}

		fprintf(fff, "\n\n");
	}

	st_ptr = &town_info[1].store[STORE_MUSEUM];

	if (st_ptr->stock_num == 0) return;

	fprintf(fff, _("  [博物館のアイテム]\n", "  [Museum]\n"));

	TERM_LEN x = 1;
	for (int i = 0; i < st_ptr->stock_num; i++)
	{
#ifdef JP
		if ((i % 12) == 0) fprintf(fff, "\n ( %d ページ )\n", x++);
		object_desc(o_name, &st_ptr->stock[i], 0);
		fprintf(fff, "%c) %s\n", I2A(i % 12), o_name);
#else
		if ((i % 12) == 0) fprintf(fff, "\n ( page %d )\n", x++);
		object_desc(o_name, &st_ptr->stock[i], 0);
		fprintf(fff, "%c) %s\n", I2A(i % 12), o_name);
#endif
	}

	fprintf(fff, "\n\n");
}


/*!
 * @brief ダンプ出力のメインルーチン
 * Output the character dump to a file
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return エラーコード
 */
errr make_character_dump(player_type *creature_ptr, FILE *fff)
{
#ifdef JP
	fprintf(fff, "  [変愚蛮怒 %d.%d.%d キャラクタ情報]\n\n",
		FAKE_VER_MAJOR - 10, FAKE_VER_MINOR, FAKE_VER_PATCH);
#else
	fprintf(fff, "  [Hengband %d.%d.%d Character Dump]\n\n",
		FAKE_VER_MAJOR - 10, FAKE_VER_MINOR, FAKE_VER_PATCH);
#endif

	update_playtime();

	dump_aux_display_player(creature_ptr, fff);
	dump_aux_last_message(creature_ptr, fff);
	dump_aux_options(fff);
	dump_aux_recall(fff);
	dump_aux_quest(creature_ptr, fff);
	dump_aux_arena(creature_ptr, fff);
	dump_aux_monsters(fff);
	dump_aux_virtues(creature_ptr, fff);
	dump_aux_race_history(creature_ptr, fff);
	dump_aux_realm_history(creature_ptr, fff);
	dump_aux_class_special(creature_ptr, fff);
	dump_aux_mutations(creature_ptr, fff);
	dump_aux_pet(creature_ptr, fff);
	fputs("\n\n", fff);
	dump_aux_equipment_inventory(creature_ptr, fff);
	dump_aux_home_museum(fff);

	fprintf(fff, _("  [チェックサム: \"%s\"]\n\n", "  [Check Sum: \"%s\"]\n\n"), get_check_sum());
	return 0;
}

/*!
 * @brief プレイヤーステータスをファイルダンプ出力する
 * Hack -- Dump a character description file
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param name 出力ファイル名
 * @return エラーコード
 * @details
 * Allow the "full" flag to dump additional info,
 * and trigger its usage from various places in the code.
 */
errr file_character(player_type *creature_ptr, concptr name)
{
	char buf[1024];
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, name);

	FILE_TYPE(FILE_TYPE_TEXT);

	int	fd = fd_open(buf, O_RDONLY);

	/* Existing file */
	if (fd >= 0)
	{
		char out_val[160];
		(void)fd_close(fd);

		/* Build query */
		(void)sprintf(out_val, _("現存するファイル %s に上書きしますか? ", "Replace existing file %s? "), buf);

		/* Ask */
		if (get_check_strict(out_val, CHECK_NO_HISTORY)) fd = -1;
	}

	/* Open the non-existing file */
	FILE *fff = NULL;
	if (fd < 0) fff = my_fopen(buf, "w");

	if (!fff)
	{
		prt(_("キャラクタ情報のファイルへの書き出しに失敗しました！", "Character dump failed!"), 0, 0);

		(void)inkey();

		/* Error */
		return (-1);
	}

	(void)make_character_dump(creature_ptr, fff);
	my_fclose(fff);

	msg_print(_("キャラクタ情報のファイルへの書き出しに成功しました。", "Character dump successful."));
	msg_print(NULL);

	return 0;
}


/*!
 * @brief ファイル内容の一行をコンソールに出力する
 * Display single line of on-line help file
 * @param str 出力する文字列
 * @param cy コンソールの行
 * @param shower 確認中
 * @return なし
 * @details
 * <pre>
 * You can insert some special color tag to change text color.
 * Such as...
 * WHITETEXT [[[[y|SOME TEXT WHICH IS DISPLAYED IN YELLOW| WHITETEXT
 * A colored segment is between "[[[[y|" and the last "|".
 * You can use any single character in place of the "|".
 * </pre>
 */
static void show_file_aux_line(concptr str, int cy, concptr shower)
{
	char lcstr[1024];
	if (shower)
	{
		/* Make a lower case version of str for searching */
		strcpy(lcstr, str);
		str_tolower(lcstr);
	}

	/* Initial cursor position */
	int cx = 0;
	Term_gotoxy(cx, cy);

	static const char tag_str[] = "[[[[";
	byte color = TERM_WHITE;
	char in_tag = '\0';
	for (int i = 0; str[i];)
	{
		int len = strlen(&str[i]);
		int showercol = len + 1;
		int bracketcol = len + 1;
		int endcol = len;
		concptr ptr;

		/* Search for a shower string in the line */
		if (shower)
		{
			ptr = my_strstr(&lcstr[i], shower);
			if (ptr) showercol = ptr - &lcstr[i];
		}

		/* Search for a color segment tag */
		ptr = in_tag ? my_strchr(&str[i], in_tag) : my_strstr(&str[i], tag_str);
		if (ptr) bracketcol = ptr - &str[i];

		/* A color tag is found */
		if (bracketcol < endcol) endcol = bracketcol;

		/* The shower string is found before the color tag */
		if (showercol < endcol) endcol = showercol;

		/* Print a segment of the line */
		Term_addstr(endcol, color, &str[i]);
		cx += endcol;
		i += endcol;

		if (endcol == showercol)
		{
			int showerlen = strlen(shower);

			/* Print the shower string in yellow */
			Term_addstr(showerlen, TERM_YELLOW, &str[i]);
			cx += showerlen;
			i += showerlen;
			continue;
		}

		if (endcol != bracketcol) continue;

		if (in_tag)
		{
			/* Found the end of colored segment */
			i++;

			/* Now looking for an another tag_str */
			in_tag = '\0';

			/* Set back to the default color */
			color = TERM_WHITE;
			continue;
		}

		/* Found a tag_str, and get a tag color */
		i += sizeof(tag_str) - 1;

		/* Get tag color */
		color = color_char_to_attr(str[i]);

		/* Illegal color tag */
		if (color == 255 || str[i + 1] == '\0')
		{
			/* Illegal color tag */
			color = TERM_WHITE;

			/* Print the broken tag as a string */
			Term_addstr(-1, TERM_WHITE, tag_str);
			cx += sizeof(tag_str) - 1;
			continue;
		}

		/* Skip the color tag */
		i++;

		/* Now looking for a close tag */
		in_tag = str[i];

		/* Skip the close-tag-indicator */
		i++;
	}

	Term_erase(cx, cy, 255);
}


/*!
 * @brief ファイル内容をコンソールに出力する
 * Recursive file perusal.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param show_version TRUEならばコンソール上にゲームのバージョンを表示する
 * @param name ファイル名の文字列
 * @param what 内容キャプションの文字列
 * @param line 表示の現在行
 * @param mode オプション
 * @return なし
 * @details
 * <pre>
 * Process various special text in the input file, including
 * the "menu" structures used by the "help file" system.
 * Return FALSE on 'q' to exit from a deep, otherwise TRUE.
 * </pre>
 */
bool show_file(player_type *creature_ptr, bool show_version, concptr name, concptr what, int line, BIT_FLAGS mode)
{
	int i, n, skey;

	/* Number of "real" lines passed by */
	int next = 0;

	/* Number of "real" lines in the file */
	int size = 0;

	/* Backup value for "line" */
	int back = 0;

	/* This screen has sub-screens */
	bool menu = FALSE;

	/* Current help file */
	FILE *fff = NULL;

	/* Find this string (if any) */
	concptr find = NULL;

	/* Jump to this tag */
	concptr tag = NULL;

	/* Hold strings to find/show */
	char finder_str[81];
	char shower_str[81];
	char back_str[81];

	/* String to show */
	concptr shower = NULL;

	/* Filename */
	char filename[1024];

	/* Describe this thing */
	char caption[128];

	/* Path buffer */
	char path[1024];

	/* General buffer */
	char buf[1024];

	/* Sub-menu information */
	char hook[68][32];

	bool reverse = (line < 0);

	int wid, hgt, rows;

	Term_get_size(&wid, &hgt);
	rows = hgt - 4;

	strcpy(finder_str, "");
	strcpy(shower_str, "");
	strcpy(caption, "");
	for (i = 0; i < 68; i++)
	{
		hook[i][0] = '\0';
	}

	strcpy(filename, name);

	n = strlen(filename);

	/* Extract the tag from the filename */
	for (i = 0; i < n; i++)
	{
		if (filename[i] == '#')
		{
			filename[i] = '\0';
			tag = filename + i + 1;
			break;
		}
	}

	/* Redirect the name */
	name = filename;

	if (what)
	{
		strcpy(caption, what);
		strcpy(path, name);
		fff = my_fopen(path, "r");
	}

	/* Look in "help" */
	if (!fff)
	{
		sprintf(caption, _("ヘルプ・ファイル'%s'", "Help file '%s'"), name);
		path_build(path, sizeof(path), ANGBAND_DIR_HELP, name);
		fff = my_fopen(path, "r");
	}

	/* Look in "info" */
	if (!fff)
	{
		sprintf(caption, _("スポイラー・ファイル'%s'", "Info file '%s'"), name);
		path_build(path, sizeof(path), ANGBAND_DIR_INFO, name);
		fff = my_fopen(path, "r");
	}

	/* Look in "info" */
	if (!fff)
	{
		path_build(path, sizeof(path), ANGBAND_DIR, name);

		for (i = 0; path[i]; i++)
			if ('\\' == path[i])
				path[i] = PATH_SEP[0];

		sprintf(caption, _("スポイラー・ファイル'%s'", "Info file '%s'"), name);
		fff = my_fopen(path, "r");
	}

	if (!fff)
	{
		msg_format(_("'%s'をオープンできません。", "Cannot open '%s'."), name);
		msg_print(NULL);

		return TRUE;
	}


	/* Pre-Parse the file */
	while (TRUE)
	{
		char *str = buf;

		/* Read a line or stop */
		if (my_fgets(fff, buf, sizeof(buf))) break;

		/* XXX Parse "menu" items */
		if (!prefix(str, "***** "))
		{
			next++;
			continue;
		}

		/* Notice "menu" requests */
		if ((str[6] == '[') && isalpha(str[7]))
		{
			/* Extract the menu item */
			int k = str[7] - 'A';

			/* This is a menu file */
			menu = TRUE;

			if ((str[8] == ']') && (str[9] == ' '))
			{
				/* Extract the menu item */
				strncpy(hook[k], str + 10, 31);

				/* Make sure it's null-terminated */
				hook[k][31] = '\0';
			}

			continue;
		}
		
		/* Notice "tag" requests */
		if (str[6] != '<') continue;

		size_t len = strlen(str);

		if (str[len - 1] == '>')
		{
			str[len - 1] = '\0';
			if (tag && streq(str + 7, tag)) line = next;
		}
	}

	/* Save the number of "real" lines */
	size = next;

	/* start from bottom when reverse mode */
	if (line == -1) line = ((size - 1) / rows)*rows;
	Term_clear();

	/* Display the file */
	while (TRUE)
	{
		/* Restart when necessary */
		if (line >= size - rows) line = size - rows;
		if (line < 0) line = 0;

		/* Re-open the file if needed */
		if (next > line)
		{
			my_fclose(fff);

			/* Hack -- Re-Open the file */
			fff = my_fopen(path, "r");

			if (!fff) return FALSE;

			/* File has been restarted */
			next = 0;
		}

		/* Goto the selected line */
		while (next < line)
		{
			/* Get a line */
			if (my_fgets(fff, buf, sizeof(buf))) break;

			/* Skip tags/links */
			if (prefix(buf, "***** ")) continue;

			/* Count the lines */
			next++;
		}

		/* Dump the next 20, or rows, lines of the file */
		for (i = 0; i < rows; )
		{
			concptr str = buf;

			/* Hack -- track the "first" line */
			if (!i) line = next;

			/* Get a line of the file or stop */
			if (my_fgets(fff, buf, sizeof(buf))) break;

			/* Hack -- skip "special" lines */
			if (prefix(buf, "***** ")) continue;

			/* Count the "real" lines */
			next++;

			/* Hack -- keep searching */
			if (find && !i)
			{
				char lc_buf[1024];

				/* Make a lower case version of str for searching */
				strcpy(lc_buf, str);
				str_tolower(lc_buf);

				if (!my_strstr(lc_buf, find)) continue;
			}

			/* Hack -- stop searching */
			find = NULL;

			/* Dump the line */
			show_file_aux_line(str, i + 2, shower);

			/* Count the printed lines */
			i++;
		}

		while (i < rows)
		{
			/* Clear rest of line */
			Term_erase(0, i + 2, 255);

			i++;
		}

		/* Hack -- failed search */
		if (find)
		{
			bell();
			line = back;
			find = NULL;
			continue;
		}

		/* Show a general "title" */
		if (show_version)
		{
			prt(format(_("[変愚蛮怒 %d.%d.%d, %s, %d/%d]", "[Hengband %d.%d.%d, %s, Line %d/%d]"),
				FAKE_VER_MAJOR - 10, FAKE_VER_MINOR, FAKE_VER_PATCH,
				caption, line, size), 0, 0);
		}
		else
		{
			prt(format(_("[%s, %d/%d]", "[%s, Line %d/%d]"),
				caption, line, size), 0, 0);
		}

		/* Prompt -- small files */
		if (size <= rows)
		{
			/* Wait for it */
			prt(_("[キー:(?)ヘルプ (ESC)終了]", "[Press ESC to exit.]"), hgt - 1, 0);
		}

		/* Prompt -- large files */
		else
		{
#ifdef JP
			if (reverse)
				prt("[キー:(RET/スペース)↑ (-)↓ (?)ヘルプ (ESC)終了]", hgt - 1, 0);
			else
				prt("[キー:(RET/スペース)↓ (-)↑ (?)ヘルプ (ESC)終了]", hgt - 1, 0);
#else
			prt("[Press Return, Space, -, =, /, |, or ESC to exit.]", hgt - 1, 0);
#endif
		}

		/* Get a special key code */
		skey = inkey_special(TRUE);

		switch (skey)
		{
			/* Show the help for the help */
		case '?':
			/* Hack - prevent silly recursion */
			if (strcmp(name, _("jhelpinfo.txt", "helpinfo.txt")) != 0)
				show_file(creature_ptr, TRUE, _("jhelpinfo.txt", "helpinfo.txt"), NULL, 0, mode);
			break;

			/* Hack -- try showing */
		case '=':
			/* Get "shower" */
			prt(_("強調: ", "Show: "), hgt - 1, 0);

			strcpy(back_str, shower_str);
			if (askfor(shower_str, 80))
			{
				if (shower_str[0])
				{
					/* Make it lowercase */
					str_tolower(shower_str);

					/* Show it */
					shower = shower_str;
				}
				else shower = NULL; /* Stop showing */
			}
			else strcpy(shower_str, back_str);
			break;

			/* Hack -- try finding */
		case '/':
		case KTRL('s'):
			/* Get "finder" */
			prt(_("検索: ", "Find: "), hgt - 1, 0);

			strcpy(back_str, finder_str);
			if (askfor(finder_str, 80))
			{
				if (finder_str[0])
				{
					/* Find it */
					find = finder_str;
					back = line;
					line = line + 1;

					/* Make finder lowercase */
					str_tolower(finder_str);

					/* Show it */
					shower = finder_str;
				}
				else shower = NULL; /* Stop showing */
			}
			else strcpy(finder_str, back_str);
			break;

			/* Hack -- go to a specific line */
		case '#':
		{
			char tmp[81];
			prt(_("行: ", "Goto Line: "), hgt - 1, 0);
			strcpy(tmp, "0");

			if (askfor(tmp, 80)) line = atoi(tmp);
		}

		break;

		/* Hack -- go to the top line */
		case SKEY_TOP:
			line = 0;
			break;

			/* Hack -- go to the bottom line */
		case SKEY_BOTTOM:
			line = ((size - 1) / rows) * rows;
			break;

			/* Hack -- go to a specific file */
		case '%':
		{
			char tmp[81];
			prt(_("ファイル・ネーム: ", "Goto File: "), hgt - 1, 0);
			strcpy(tmp, _("jhelp.hlp", "help.hlp"));

			if (askfor(tmp, 80))
			{
				if (!show_file(creature_ptr, TRUE, tmp, NULL, 0, mode)) skey = 'q';
			}
		}

		break;

		/* Allow backing up */
		case '-':
			line = line + (reverse ? rows : -rows);
			if (line < 0) line = 0;
			break;

			/* One page up */
		case SKEY_PGUP:
			line = line - rows;
			if (line < 0) line = 0;
			break;

			/* Advance a single line */
		case '\n':
		case '\r':
			line = line + (reverse ? -1 : 1);
			if (line < 0) line = 0;
			break;

			/* Move up / down */
		case '8':
		case SKEY_UP:
			line--;
			if (line < 0) line = 0;
			break;

		case '2':
		case SKEY_DOWN:
			line++;
			break;

			/* Advance one page */
		case ' ':
			line = line + (reverse ? -rows : rows);
			if (line < 0) line = 0;
			break;

			/* One page down */
		case SKEY_PGDOWN:
			line = line + rows;
			break;
		}

		/* Recurse on numbers */
		if (menu)
		{
			int key = -1;

			if (!(skey & SKEY_MASK) && isalpha(skey))
				key = skey - 'A';

			if ((key > -1) && hook[key][0])
			{
				/* Recurse on that file */
				if (!show_file(creature_ptr, TRUE, hook[key], NULL, 0, mode))
					skey = 'q';
			}
		}

		/* Hack, dump to file */
		if (skey == '|')
		{
			FILE *ffp;
			char buff[1024];
			char xtmp[82];

			strcpy(xtmp, "");

			if (!get_string(_("ファイル名: ", "File name: "), xtmp, 80)) continue;
			my_fclose(fff);
			path_build(buff, sizeof(buff), ANGBAND_DIR_USER, xtmp);

			/* Hack -- Re-Open the file */
			fff = my_fopen(path, "r");

			ffp = my_fopen(buff, "w");

			if (!(fff && ffp))
			{
				msg_print(_("ファイルを開けません。", "Failed to open file."));
				skey = ESCAPE;
				break;
			}

			sprintf(xtmp, "%s: %s", creature_ptr->name, what ? what : caption);
			my_fputs(ffp, xtmp, 80);
			my_fputs(ffp, "\n", 80);

			while (!my_fgets(fff, buff, sizeof(buff)))
				my_fputs(ffp, buff, 80);
			my_fclose(fff);
			my_fclose(ffp);

			/* Hack -- Re-Open the file */
			fff = my_fopen(path, "r");
		}

		/* Return to last screen */
		if ((skey == ESCAPE) || (skey == '<')) break;

		/* Exit on the ^q */
		if (skey == KTRL('q')) skey = 'q';

		/* Exit on the q key */
		if (skey == 'q') break;
	}

	my_fclose(fff);

	if (skey == 'q') return FALSE;

	return TRUE;
}


/*!
 * @brief ヘルプを表示するコマンドのメインルーチン
 * Peruse the On-Line-Help
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 */
void do_cmd_help(player_type *creature_ptr)
{
	screen_save();

	/* Peruse the main help file */
	(void)show_file(creature_ptr, TRUE, _("jhelp.hlp", "help.hlp"), NULL, 0, 0);
	screen_load();
}


/*!
 * @brief プレイヤーの名前をチェックして修正する
 * Process the player name.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param sf セーブファイル名に合わせた修正を行うならばTRUE
 * @return なし
 * @details
 * Extract a clean "base name".
 * Build the savefile name if needed.
 */
void process_player_name(player_type *creature_ptr, bool sf)
{
	char old_player_base[32] = "";

	if (current_world_ptr->character_generated) strcpy(old_player_base, creature_ptr->base_name);

	/* Cannot be too long */
#if defined(MACINTOSH) || defined(ACORN)
	if (strlen(creature_ptr->name) > 15)
	{
		/* Name too long */
		quit_fmt(_("'%s'という名前は長すぎます！", "The name '%s' is too long!"), creature_ptr->name);
	}
#endif

	/* Cannot contain "icky" characters */
	for (int i = 0; creature_ptr->name[i]; i++)
	{
		/* No control characters */
#ifdef JP
		if (iskanji(creature_ptr->name[i])) { i++; continue; }
		if (iscntrl((unsigned char)creature_ptr->name[i]))
#else
		if (iscntrl(creature_ptr->name[i]))
#endif
		{
			/* Illegal characters */
			quit_fmt(_("'%s' という名前は不正なコントロールコードを含んでいます。", "The name '%s' contains control chars!"), creature_ptr->name);
		}
	}


#ifdef MACINTOSH
	/* Extract "useful" letters */
	for (int i = 0; creature_ptr->name[i]; i++)
	{
#ifdef JP
		unsigned char c = creature_ptr->name[i];
#else
		char c = creature_ptr->name[i];
#endif
		/* Convert "dot" to "underscore" */
		if (c == '.') c = '_';

		/* Accept all the letters */
		creature_ptr->base_name[k++] = c;
	}
#else

	/* Extract "useful" letters */
	int k = 0;
	for (int i = 0; creature_ptr->name[i]; i++)
	{
#ifdef JP
		unsigned char c = creature_ptr->name[i];
#else
		char c = creature_ptr->name[i];
#endif

		/* Accept some letters */
#ifdef JP
		if (iskanji(c)) {
			if (k + 2 >= sizeof(creature_ptr->base_name) || !creature_ptr->name[i + 1]) break;
			creature_ptr->base_name[k++] = c;
			i++;
			creature_ptr->base_name[k++] = creature_ptr->name[i];
		}
#ifdef SJIS
		else if (iskana(c)) creature_ptr->base_name[k++] = c;
#endif
		else
#endif
			/* Convert path separator to underscore */
			if (!strncmp(PATH_SEP, creature_ptr->name + i, strlen(PATH_SEP))) {
				creature_ptr->base_name[k++] = '_';
				i += strlen(PATH_SEP);
			}
		/* Convert some characters to underscore */
#if defined(WINDOWS)
			else if (my_strchr("\"*,/:;<>?\\|", c)) creature_ptr->base_name[k++] = '_';
#endif
			else if (isprint(c)) creature_ptr->base_name[k++] = c;
	}
#endif
	creature_ptr->base_name[k] = '\0';

	/* Require a "base" name */
	if (!creature_ptr->base_name[0]) strcpy(creature_ptr->base_name, "PLAYER");

#ifdef SAVEFILE_MUTABLE
	sf = TRUE;
#endif
	if (!savefile_base[0] && savefile[0])
	{
		concptr s;
		s = savefile;
		while (TRUE)
		{
			concptr t;
			t = my_strstr(s, PATH_SEP);
			if (!t)
				break;
			s = t + 1;
		}

		strcpy(savefile_base, s);
	}

	if (!savefile_base[0] || !savefile[0])
		sf = TRUE;

	/* Change the savefile name */
	if (sf)
	{
		char temp[128];

		strcpy(savefile_base, creature_ptr->base_name);

#ifdef SAVEFILE_USE_UID
		/* Rename the savefile, using the creature_ptr->player_uid and creature_ptr->base_name */
		(void)sprintf(temp, "%d.%s", creature_ptr->player_uid, creature_ptr->base_name);
#else
		/* Rename the savefile, using the creature_ptr->base_name */
		(void)sprintf(temp, "%s", creature_ptr->base_name);
#endif
		path_build(savefile, sizeof(savefile), ANGBAND_DIR_SAVE, temp);
	}

	/* Load an autopick preference file */
	if (current_world_ptr->character_generated)
	{
		if (!streq(old_player_base, creature_ptr->base_name)) autopick_load_pref(creature_ptr, FALSE);
	}
}


/*!
 * @brief プレイヤーの名前を変更するコマンドのメインルーチン
 * Gets a name for the character, reacting to name changes.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * <pre>
 * Assumes that "display_player()" has just been called
 * Perhaps we should NOT ask for a name (at "birth()") on
 * Unix machines?  XXX XXX
 * What a horrible name for a global function.
 * </pre>
 */
void get_name(player_type *creature_ptr)
{
	char tmp[64];
	strcpy(tmp, creature_ptr->name);

	/* Prompt for a new name */
	if (get_string(_("キャラクターの名前を入力して下さい: ", "Enter a name for your character: "), tmp, 15))
	{
		strcpy(creature_ptr->name, tmp);
	}

	if (strlen(creature_ptr->name) == 0)
	{
		strcpy(creature_ptr->name, "PLAYER");
	}

	strcpy(tmp, ap_ptr->title);
#ifdef JP
	if (ap_ptr->no == 1)
		strcat(tmp, "の");
#else
	strcat(tmp, " ");
#endif
	strcat(tmp, creature_ptr->name);

	/* Re-Draw the name (in light blue) */
	Term_erase(34, 1, 255);
	c_put_str(TERM_L_BLUE, tmp, 1, 34);

	/* Erase the prompt, etc */
	clear_from(22);
}


/*!
 * @brief セーブするコマンドのメインルーチン
 * Save the game
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param is_autosave オートセーブ中の処理ならばTRUE
 * @return なし
 * @details
 */
void do_cmd_save_game(player_type *creature_ptr, int is_autosave)
{
	/* Autosaves do not disturb */
	if (is_autosave)
	{
		msg_print(_("自動セーブ中", "Autosaving the game..."));
	}
	else
	{
		disturb(creature_ptr, TRUE, TRUE);
	}

	/* Clear messages */
	msg_print(NULL);
	handle_stuff(creature_ptr);

	prt(_("ゲームをセーブしています...", "Saving game..."), 0, 0);

	Term_fresh();

	/* The player is not dead */
	(void)strcpy(creature_ptr->died_from, _("(セーブ)", "(saved)"));

	/* Forbid suspend */
	signals_ignore_tstp();

	/* Save the player */
	if (save_player(creature_ptr))
	{
		prt(_("ゲームをセーブしています... 終了", "Saving game... done."), 0, 0);
	}

	/* Save failed (oops) */
	else
	{
		prt(_("ゲームをセーブしています... 失敗！", "Saving game... failed!"), 0, 0);
	}

	/* Allow suspend again */
	signals_handle_tstp();

	Term_fresh();

	/* Note that the player is not dead */
	(void)strcpy(creature_ptr->died_from, _("(元気に生きている)", "(alive and well)"));

	/* HACK -- don't get sanity blast on updating view */
	current_world_ptr->is_loading_now = FALSE;

	update_creature(creature_ptr);

	/* Initialize monster process */
	mproc_init();

	/* HACK -- reset the hackish flag */
	current_world_ptr->is_loading_now = TRUE;
}


/*!
 * @brief セーブ後にゲーム中断フラグを立てる/
 * Save the game and exit
 * @return なし
 * @details
 */
void do_cmd_save_and_exit(player_type *creature_ptr)
{
	creature_ptr->playing = FALSE;
	creature_ptr->leaving = TRUE;
	exe_write_diary(creature_ptr, NIKKI_GAMESTART, 0, _("----ゲーム中断----", "---- Save and Exit Game ----"));
}


/*!
 * @brief 墓石の真ん中に文字列を書き込む /
 * Centers a string within a GRAVE_LINE_WIDTH character string		-JWT-
 * @return なし
 * @details
 */
static void center_string(char *buf, concptr str)
{
	/* Total length */
	int i = strlen(str);

	/* Necessary border */
	int j = GRAVE_LINE_WIDTH / 2 - i / 2;

	/* Mega-Hack */
	(void)sprintf(buf, "%*s%s%*s", j, "", str, GRAVE_LINE_WIDTH - i - j, "");
}


#if 0
/*!
 * @brief 骨ファイル出力 /
 * Save a "bones" file for a dead character
 * @details
 * <pre>
 * Note that we will not use these files until Angband 2.8.0, and
 * then we will only use the name and level on which death occured.
 * Should probably attempt some form of locking...
 * </pre>
 */
static void make_bones(void)
{
	FILE                *fp;

	char                str[1024];


	/* Ignore wizards and borgs */
	if (!(current_world_ptr->noscore & 0x00FF))
	{
		/* Ignore people who die in town */
		if (p_ptr->current_floor_ptr->dun_level)
		{
			char tmp[128];

			/* "Bones" name */
			sprintf(tmp, "bone.%03d", p_ptr->current_floor_ptr->dun_level);
			path_build(str, sizeof(str), ANGBAND_DIR_BONE, tmp);

			/* Attempt to open the bones file */
			fp = my_fopen(str, "r");

			/* Close it right away */
			if (fp) my_fclose(fp);

			/* Do not over-write a previous ghost */
			if (fp) return;

			/* File type is "TEXT" */
			FILE_TYPE(FILE_TYPE_TEXT);

			/* Grab permissions */
			safe_setuid_grab();

			/* Try to write a new "Bones File" */
			fp = my_fopen(str, "w");

			/* Drop permissions */
			safe_setuid_drop();

			/* Not allowed to write it?  Weird. */
			if (!fp) return;

			/* Save the info */
			fprintf(fp, "%s\n", p_ptr->name);
			fprintf(fp, "%d\n", p_ptr->mhp);
			fprintf(fp, "%d\n", p_ptr->prace);
			fprintf(fp, "%d\n", p_ptr->pclass);

			/* Close and save the Bones file */
			my_fclose(fp);
		}
	}
}
#endif


/*
 * Redefinable "print_tombstone" action
 */
bool(*tombstone_aux)(void) = NULL;


/*!
 * @brief 墓石のアスキーアート表示 /
 * Display a "tomb-stone"
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void print_tomb(player_type *dead_ptr)
{
	/* Do we use a special tombstone ? */
	bool done = FALSE;
	if (tombstone_aux)
	{
		/* Use tombstone hook */
		done = (*tombstone_aux)();
	}

	if (done) return;

#ifdef JP
	int extra_line = 0;
#endif
	Term_clear();
	char buf[1024];
	path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, _("dead_j.txt", "dead.txt"));

	FILE *fp;
	fp = my_fopen(buf, "r");

	if (fp)
	{
		int i = 0;

		/* Dump the file to the screen */
		while (my_fgets(fp, buf, sizeof(buf)) == 0)
		{
			put_str(buf, i++, 0);
		}

		my_fclose(fp);
	}

	concptr p;
	if (current_world_ptr->total_winner || (dead_ptr->lev > PY_MAX_LEVEL))
	{
#ifdef JP
		p = "偉大なる者";
#else
		p = "Magnificent";
#endif
	}
	else
	{
		p = player_title[dead_ptr->pclass][(dead_ptr->lev - 1) / 5];
	}

	center_string(buf, dead_ptr->name);
	put_str(buf, 6, 11);

#ifndef JP
	center_string(buf, "the");
	put_str(buf, 7, 11);
#endif

	center_string(buf, p);
	put_str(buf, 8, 11);

	center_string(buf, cp_ptr->title);
	put_str(buf, 10, 11);

	char tmp[160];
	(void)sprintf(tmp, _("レベル: %d", "Level: %d"), (int)dead_ptr->lev);
	center_string(buf, tmp);
	put_str(buf, 11, 11);

	(void)sprintf(tmp, _("経験値: %ld", "Exp: %ld"), (long)dead_ptr->exp);
	center_string(buf, tmp);
	put_str(buf, 12, 11);

	(void)sprintf(tmp, _("所持金: %ld", "AU: %ld"), (long)dead_ptr->au);
	center_string(buf, tmp);
	put_str(buf, 13, 11);

#ifdef JP
	/* 墓に刻む言葉をオリジナルより細かく表示 */
	if (streq(dead_ptr->died_from, "途中終了"))
	{
		strcpy(tmp, "<自殺>");
	}
	else if (streq(dead_ptr->died_from, "ripe"))
	{
		strcpy(tmp, "引退後に天寿を全う");
	}
	else if (streq(dead_ptr->died_from, "Seppuku"))
	{
		strcpy(tmp, "勝利の後、切腹");
	}
	else
	{
		roff_to_buf(dead_ptr->died_from, GRAVE_LINE_WIDTH + 1, tmp, sizeof tmp);
		char *t;
		t = tmp + strlen(tmp) + 1;
		if (*t)
		{
			char dummy[80];
			strcpy(dummy, t); /* 2nd line */
			if (*(t + strlen(t) + 1)) /* Does 3rd line exist? */
			{
				for (t = dummy + strlen(dummy) - 2; iskanji(*(t - 1)); t--) /* Loop */;
				strcpy(t, "…");
			}
			else if (my_strstr(tmp, "『") && suffix(dummy, "』"))
			{
				char dummy2[80];
				char *name_head = my_strstr(tmp, "『");
				sprintf(dummy2, "%s%s", name_head, dummy);
				if (strlen(dummy2) <= GRAVE_LINE_WIDTH)
				{
					strcpy(dummy, dummy2);
					*name_head = '\0';
				}
			}
			else if (my_strstr(tmp, "「") && suffix(dummy, "」"))
			{
				char dummy2[80];
				char *name_head = my_strstr(tmp, "「");
				sprintf(dummy2, "%s%s", name_head, dummy);
				if (strlen(dummy2) <= GRAVE_LINE_WIDTH)
				{
					strcpy(dummy, dummy2);
					*name_head = '\0';
				}
			}

			center_string(buf, dummy);
			put_str(buf, 15, 11);
			extra_line = 1;
		}
	}

	center_string(buf, tmp);
	put_str(buf, 14, 11);

	if (!streq(dead_ptr->died_from, "ripe") && !streq(dead_ptr->died_from, "Seppuku"))
	{
		if (dead_ptr->current_floor_ptr->dun_level == 0)
		{
			concptr field_name = dead_ptr->town_num ? "街" : "荒野";
			if (streq(dead_ptr->died_from, "途中終了"))
			{
				sprintf(tmp, "%sで死んだ", field_name);
			}
			else
			{
				sprintf(tmp, "に%sで殺された", field_name);
			}
		}
		else
		{
			if (streq(dead_ptr->died_from, "途中終了"))
			{
				sprintf(tmp, "地下 %d 階で死んだ", (int)dead_ptr->current_floor_ptr->dun_level);
			}
			else
			{
				sprintf(tmp, "に地下 %d 階で殺された", (int)dead_ptr->current_floor_ptr->dun_level);
			}
		}

		center_string(buf, tmp);
		put_str(buf, 15 + extra_line, 11);
	}
#else
	(void)sprintf(tmp, "Killed on Level %d", dead_ptr->current_floor_ptr->dun_level);
	center_string(buf, tmp);
	put_str(buf, 14, 11);

	roff_to_buf(format("by %s.", dead_ptr->died_from), GRAVE_LINE_WIDTH + 1, tmp, sizeof tmp);
	center_string(buf, tmp);
	put_str(buf, 15, 11);
	t = tmp + strlen(tmp) + 1;
	if (*t)
	{
		strcpy(dummy, t); /* 2nd line */
		if (*(t + strlen(t) + 1)) /* Does 3rd line exist? */
		{
			int dummy_len = strlen(dummy);
			strcpy(dummy + MIN(dummy_len, GRAVE_LINE_WIDTH - 3), "...");
		}
		center_string(buf, dummy);
		put_str(buf, 16, 11);
	}
#endif
	time_t ct = time((time_t)0);
	(void)sprintf(tmp, "%-.24s", ctime(&ct));
	center_string(buf, tmp);
	put_str(buf, 17, 11);
	msg_format(_("さようなら、%s!", "Goodbye, %s!"), dead_ptr->name);
}


/*!
 * @brief 死亡、引退時の簡易ステータス表示 /
 * Display some character info
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void show_info(player_type *creature_ptr)
{
	/* Hack -- Know everything in the inven/equip */
	object_type *o_ptr;
	for (int i = 0; i < INVEN_TOTAL; i++)
	{
		o_ptr = &creature_ptr->inventory_list[i];
		if (!o_ptr->k_idx) continue;

		/* Aware and Known */
		object_aware(creature_ptr, o_ptr);
		object_known(o_ptr);
	}

	store_type *st_ptr;
	for (int i = 1; i < max_towns; i++)
	{
		st_ptr = &town_info[i].store[STORE_HOME];

		/* Hack -- Know everything in the home */
		for (int j = 0; j < st_ptr->stock_num; j++)
		{
			o_ptr = &st_ptr->stock[j];
			if (!o_ptr->k_idx) continue;

			/* Aware and Known */
			object_aware(creature_ptr, o_ptr);
			object_known(o_ptr);
		}
	}

	/* Hack -- Recalculate bonuses */
	creature_ptr->update |= (PU_BONUS);
	handle_stuff(creature_ptr);

	flush();
	msg_erase();

	prt(_("キャラクターの記録をファイルに書き出すことができます。", "You may now dump a character record to one or more files."), 21, 0);
	prt(_("リターンキーでキャラクターを見ます。ESCで中断します。", "Then, hit RETURN to see the character, or ESC to abort."), 22, 0);

	/* Dump character records as requested */
	while (TRUE)
	{
		char out_val[160];

		/* Prompt */
		put_str(_("ファイルネーム: ", "Filename: "), 23, 0);

		/* Default */
		strcpy(out_val, "");

		/* Ask for filename (or abort) */
		if (!askfor(out_val, 60)) return;

		/* Return means "show on screen" */
		if (!out_val[0]) break;
		screen_save();

		/* Dump a character file */
		(void)file_character(creature_ptr, out_val);
		screen_load();
	}

	update_playtime();
	display_player(creature_ptr, 0);

	prt(_("何かキーを押すとさらに情報が続きます (ESCで中断): ", "Hit any key to see more information (ESC to abort): "), 23, 0);

	/* Allow abort at this point */
	if (inkey() == ESCAPE) return;

	/* Equipment -- if any */
	if (creature_ptr->equip_cnt)
	{
		Term_clear();
		(void)show_equip(creature_ptr, 0, USE_FULL, 0);
		prt(_("装備していたアイテム: -続く-", "You are using: -more-"), 0, 0);

		if (inkey() == ESCAPE) return;
	}

	/* Inventory -- if any */
	if (creature_ptr->inven_cnt)
	{
		Term_clear();
		(void)show_inven(creature_ptr, 0, USE_FULL, 0);
		prt(_("持っていたアイテム: -続く-", "You are carrying: -more-"), 0, 0);

		if (inkey() == ESCAPE) return;
	}

	/* Homes in the different towns */
	for (int l = 1; l < max_towns; l++)
	{
		st_ptr = &town_info[l].store[STORE_HOME];

		/* Home -- if anything there */
		if (st_ptr->stock_num == 0) continue;

		/* Display contents of the home */
		for (int i = 0, k = 0; i < st_ptr->stock_num; k++)
		{
			Term_clear();

			/* Show 12 items */
			for (int j = 0; (j < 12) && (i < st_ptr->stock_num); j++, i++)
			{
				GAME_TEXT o_name[MAX_NLEN];
				char tmp_val[80];

				/* Acquire item */
				o_ptr = &st_ptr->stock[i];

				/* Print header, clear line */
				sprintf(tmp_val, "%c) ", I2A(j));
				prt(tmp_val, j + 2, 4);

				/* Display object description */
				object_desc(o_name, o_ptr, 0);
				c_put_str(tval_to_attr[o_ptr->tval], o_name, j + 2, 7);
			}

			prt(format(_("我が家に置いてあったアイテム ( %d ページ): -続く-", "Your home contains (page %d): -more-"), k + 1), 0, 0);

			if (inkey() == ESCAPE) return;
		}
	}
}


/*!
 * @brief 異常発生時のゲーム緊急終了処理 /
 * Handle abrupt death of the visual system
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * <pre>
 * This routine is called only in very rare situations, and only
 * by certain visual systems, when they experience fatal errors.
 * XXX XXX Hack -- clear the death flag when creating a HANGUP
 * save file so that player can see tombstone when restart.
 * </pre>
 */
void exit_game_panic(player_type *creature_ptr)
{
	/* If nothing important has happened, just quit */
	if (!current_world_ptr->character_generated || current_world_ptr->character_saved) quit(_("緊急事態", "panic"));

	/* Mega-Hack -- see "msg_print()" */
	msg_flag = FALSE;

	/* Clear the top line */
	prt("", 0, 0);

	/* Hack -- turn off some things */
	disturb(creature_ptr, TRUE, TRUE);

	/* Mega-Hack -- Delay death */
	if (creature_ptr->chp < 0) creature_ptr->is_dead = FALSE;

	/* Hardcode panic save */
	creature_ptr->panic_save = 1;

	/* Forbid suspend */
	signals_ignore_tstp();

	/* Indicate panic save */
	(void)strcpy(creature_ptr->died_from, _("(緊急セーブ)", "(panic save)"));

	/* Panic save, or get worried */
	if (!save_player(creature_ptr)) quit(_("緊急セーブ失敗！", "panic save failed!"));

	/* Successful panic save */
	quit(_("緊急セーブ成功！", "panic save succeeded!"));
}


/*!
 * @brief ファイルからランダムに行を一つ取得する /
 * Get a random line from a file
 * @param file_name ファイル名
 * @param entry 特定条件時のN:タグヘッダID
 * @param output 出力先の文字列参照ポインタ
 * @return エラーコード
 * @details
 * <pre>
 * Based on the monster speech patch by Matt Graham,
 * </pre>
 */
errr get_rnd_line(concptr file_name, int entry, char *output)
{
	char buf[1024];
	path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, file_name);
	FILE *fp;
	fp = my_fopen(buf, "r");

	/* Failed */
	if (!fp) return -1;

	/* Find the entry of the monster */
	int test;
	int line_num = 0;
	while (TRUE)
	{
		/* Get a line from the file */
		if (my_fgets(fp, buf, sizeof(buf)) != 0)
		{
			my_fclose(fp);
			return -1;
		}

		/* Count the lines */
		line_num++;

		/* Look for lines starting with 'N:' */
		if ((buf[0] != 'N') || (buf[1] != ':')) continue;

		if (buf[2] == '*')
		{
			break;
		}
		else if (buf[2] == 'M')
		{
			if (r_info[entry].flags1 & RF1_MALE) break;
		}
		else if (buf[2] == 'F')
		{
			if (r_info[entry].flags1 & RF1_FEMALE) break;
		}
		else if (sscanf(&(buf[2]), "%d", &test) != EOF)
		{
			if (test == entry) break;
		}

		msg_format("Error in line %d of %s!", line_num, file_name);
		my_fclose(fp);
		return -1;
	}

	/* Get the random line */
	int counter;
	for (counter = 0; ; counter++)
	{
		while (TRUE)
		{
			test = my_fgets(fp, buf, sizeof(buf));

			/* Count the lines */
			/* line_num++; No more needed */

			if (!test)
			{
				/* Ignore lines starting with 'N:' */
				if ((buf[0] == 'N') && (buf[1] == ':')) continue;

				if (buf[0] != '#') break;
			}
			else break;
		}

		if (!buf[0]) break;

		if (one_in_(counter + 1)) strcpy(output, buf);
	}

	my_fclose(fp);
	return counter ? 0 : -1;
}


#ifdef JP
/*!
 * @brief ファイルからランダムに行を一つ取得する(日本語文字列のみ) /
 * @param file_name ファイル名
 * @param entry 特定条件時のN:タグヘッダID
 * @param output 出力先の文字列参照ポインタ
 * @param count 試行回数
 * @return エラーコード
 * @details
 */
errr get_rnd_line_jonly(concptr file_name, int entry, char *output, int count)
{
	errr result = 1;
	for (int i = 0; i < count; i++)
	{
		result = get_rnd_line(file_name, entry, output);
		if (result) break;
		bool kanji = FALSE;
		for (int j = 0; output[j]; j++) kanji |= iskanji(output[j]);
		if (kanji) break;
	}

	return result;
}
#endif


/*!
 * @brief 自動拾いファイルを読み込む /
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param name ファイル名
 * @details
 */
errr process_autopick_file(player_type *creature_ptr, concptr name)
{
	char buf[1024];
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, name);
	errr err = process_pref_file_aux(creature_ptr, buf, PREF_TYPE_AUTOPICK);
	return err;
}


/*!
 * @brief プレイヤーの生い立ちファイルを読み込む /
 * Process file for player's history editor.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param name ファイル名
 * @return エラーコード
 * @details
 */
errr process_histpref_file(player_type *creature_ptr, concptr name)
{
	bool old_character_xtra = current_world_ptr->character_xtra;
	char buf[1024];
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, name);

	/* Hack -- prevent modification birth options in this file */
	current_world_ptr->character_xtra = TRUE;
	errr err = process_pref_file_aux(creature_ptr, buf, PREF_TYPE_HISTPREF);
	current_world_ptr->character_xtra = old_character_xtra;
	return err;
}

/*!
 * @brief ファイル位置をシーク /
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param fd ファイルディスクリプタ
 * @param where ファイルバイト位置
 * @param flag FALSEならば現ファイルを超えた位置へシーク時エラー、TRUEなら足りない間を0で埋め尽くす
 * @return エラーコード
 * @details
 */
static errr counts_seek(player_type *creature_ptr, int fd, u32b where, bool flag)
{
	char temp1[128], temp2[128];

#ifdef SAVEFILE_USE_UID
	(void)sprintf(temp1, "%d.%s.%d%d%d", creature_ptr->player_uid, savefile_base, creature_ptr->pclass, creature_ptr->pseikaku, creature_ptr->age);
#else
	(void)sprintf(temp1, "%s.%d%d%d", savefile_base, creature_ptr->pclass, creature_ptr->pseikaku, creature_ptr->age);
#endif
	for (int i = 0; temp1[i]; i++)
		temp1[i] ^= (i + 1) * 63;

	int seekpoint = 0;
	u32b zero_header[3] = { 0L, 0L, 0L };
	while (TRUE)
	{
		if (fd_seek(fd, seekpoint + 3 * sizeof(u32b)))
			return 1;
		if (fd_read(fd, (char*)(temp2), sizeof(temp2)))
		{
			if (!flag)
				return 1;
			/* add new name */
			fd_seek(fd, seekpoint);
			fd_write(fd, (char*)zero_header, 3 * sizeof(u32b));
			fd_write(fd, (char*)(temp1), sizeof(temp1));
			break;
		}

		if (strcmp(temp1, temp2) == 0)
			break;

		seekpoint += 128 + 3 * sizeof(u32b);
	}

	return fd_seek(fd, seekpoint + where * sizeof(u32b));
}

/*!
 * @brief ファイル位置を読み込む
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param where ファイルバイト位置
 * @return エラーコード
 * @details
 */
u32b counts_read(player_type *creature_ptr, int where)
{
	char buf[1024];
	path_build(buf, sizeof(buf), ANGBAND_DIR_DATA, _("z_info_j.raw", "z_info.raw"));
	int fd = fd_open(buf, O_RDONLY);

	u32b count = 0;
	if (counts_seek(creature_ptr, fd, where, FALSE) ||
		fd_read(fd, (char*)(&count), sizeof(u32b)))
		count = 0;

	(void)fd_close(fd);

	return count;
}

/*!
 * @brief ファイル位置に書き込む /
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param where ファイルバイト位置
 * @param count 書き込む値
 * @return エラーコード
 * @details
 */
errr counts_write(player_type *creature_ptr, int where, u32b count)
{
	char buf[1024];
	path_build(buf, sizeof(buf), ANGBAND_DIR_DATA, _("z_info_j.raw", "z_info.raw"));

	/* Grab permissions */
	safe_setuid_grab();

	int fd = fd_open(buf, O_RDWR);

	/* Drop permissions */
	safe_setuid_drop();

	if (fd < 0)
	{
		/* File type is "DATA" */
		FILE_TYPE(FILE_TYPE_DATA);

		/* Grab permissions */
		safe_setuid_grab();

		/* Create a new high score file */
		fd = fd_make(buf, 0644);

		/* Drop permissions */
		safe_setuid_drop();
	}

	/* Grab permissions */
	safe_setuid_grab();

	errr err = fd_lock(fd, F_WRLCK);

	/* Drop permissions */
	safe_setuid_drop();

	if (err) return 1;

	counts_seek(creature_ptr, fd, where, TRUE);
	fd_write(fd, (char*)(&count), sizeof(u32b));

	/* Grab permissions */
	safe_setuid_grab();

	err = fd_lock(fd, F_UNLCK);

	/* Drop permissions */
	safe_setuid_drop();

	if (err) return 1;

	(void)fd_close(fd);

	return 0;
}


#ifdef HANDLE_SIGNALS

#include <signal.h>

/*!
 * @brief OSからのシグナルを受けてサスペンド状態に入る /
 * Handle signals -- suspend
 * @param sig 受け取ったシグナル
 * @details
 * Actually suspend the game, and then resume cleanly
 */
static void handle_signal_suspend(int sig)
{
	/* Disable handler */
	(void)signal(sig, SIG_IGN);

#ifdef SIGSTOP

	/* Flush output */
	Term_fresh();

	/* Suspend the "Term" */
	Term_xtra(TERM_XTRA_ALIVE, 0);

	/* Suspend ourself */
	(void)kill(0, SIGSTOP);

	/* Resume the "Term" */
	Term_xtra(TERM_XTRA_ALIVE, 1);

	/* Redraw the term */
	Term_redraw();

	/* Flush the term */
	Term_fresh();

#endif

	/* Restore handler */
	(void)signal(sig, handle_signal_suspend);
}


/*!
 * todo ここにp_ptrを追加すると関数ポインタ周りの収拾がつかなくなるので保留
 * @brief OSからのシグナルを受けて中断、終了する /
 * Handle signals -- simple (interrupt and quit)
 * @param sig 受け取ったシグナル
 * @details
 * <pre>
 * This function was causing a *huge* number of problems, so it has
 * been simplified greatly.  We keep a global variable which counts
 * the number of times the user attempts to kill the process, and
 * we commit suicide if the user does this a certain number of times.
 * We attempt to give "feedback" to the user as he approaches the
 * suicide thresh-hold, but without penalizing accidental keypresses.
 * To prevent messy accidents, we should reset this global variable
 * whenever the user enters a keypress, or something like that.
 * </pre>
 */
static void handle_signal_simple(int sig)
{
	/* Disable handler */
	(void)signal(sig, SIG_IGN);


	/* Nothing to save, just quit */
	if (!current_world_ptr->character_generated || current_world_ptr->character_saved) quit(NULL);


	/* Count the signals */
	signal_count++;


	/* Terminate dead characters */
	if (p_ptr->is_dead)
	{
		/* Mark the savefile */
		(void)strcpy(p_ptr->died_from, _("強制終了", "Abortion"));

		forget_lite(p_ptr->current_floor_ptr);
		forget_view(p_ptr->current_floor_ptr);
		clear_mon_lite(p_ptr->current_floor_ptr);

		/* Close stuff */
		close_game(p_ptr);

		/* Quit */
		quit(_("強制終了", "interrupt"));
	}

	/* Allow suicide (after 5) */
	else if (signal_count >= 5)
	{
		/* Cause of "death" */
		(void)strcpy(p_ptr->died_from, _("強制終了中", "Interrupting"));

		forget_lite(p_ptr->current_floor_ptr);
		forget_view(p_ptr->current_floor_ptr);
		clear_mon_lite(p_ptr->current_floor_ptr);

		/* Stop playing */
		p_ptr->playing = FALSE;

		/* Suicide */
		p_ptr->is_dead = TRUE;
		p_ptr->leaving = TRUE;

		/* Close stuff */
		close_game(p_ptr);

		/* Quit */
		quit(_("強制終了", "interrupt"));
	}

	/* Give warning (after 4) */
	else if (signal_count >= 4)
	{
		/* Make a noise */
		Term_xtra(TERM_XTRA_NOISE, 0);

		/* Clear the top line */
		Term_erase(0, 0, 255);

		/* Display the cause */
		Term_putstr(0, 0, -1, TERM_WHITE, _("熟慮の上の自殺！", "Contemplating suicide!"));

		Term_fresh();
	}

	/* Give warning (after 2) */
	else if (signal_count >= 2)
	{
		/* Make a noise */
		Term_xtra(TERM_XTRA_NOISE, 0);
	}

	(void)signal(sig, handle_signal_simple);
}


/*!
 * todo ここにp_ptrを追加すると関数ポインタ周りの収拾がつかなくなるので保留
 * @brief OSからのシグナルを受けて強制終了する /
 * Handle signal -- abort, kill, etc
 * @param sig 受け取ったシグナル
 * @return なし
 * @details
 * <pre>
 * This function was causing a *huge* number of problems, so it has
 * been simplified greatly.  We keep a global variable which counts
 * the number of times the user attempts to kill the process, and
 * we commit suicide if the user does this a certain number of times.
 * We attempt to give "feedback" to the user as he approaches the
 * suicide thresh-hold, but without penalizing accidental keypresses.
 * To prevent messy accidents, we should reset this global variable
 * whenever the user enters a keypress, or something like that.
 * </pre>
 */
static void handle_signal_abort(int sig)
{
	int wid, hgt;
	Term_get_size(&wid, &hgt);

	(void)signal(sig, SIG_IGN);
	if (!current_world_ptr->character_generated || current_world_ptr->character_saved) quit(NULL);

	forget_lite(p_ptr->current_floor_ptr);
	forget_view(p_ptr->current_floor_ptr);
	clear_mon_lite(p_ptr->current_floor_ptr);

	Term_erase(0, hgt - 1, 255);
	Term_putstr(0, hgt - 1, -1, TERM_RED,
		_("恐ろしいソフトのバグが飛びかかってきた！", "A gruesome software bug LEAPS out at you!"));

	Term_putstr(45, hgt - 1, -1, TERM_RED, _("緊急セーブ...", "Panic save..."));

	exe_write_diary(p_ptr, NIKKI_GAMESTART, 0, _("----ゲーム異常終了----", "---- Panic Save and Abort Game ----"));
	Term_fresh();

	p_ptr->panic_save = 1;
	(void)strcpy(p_ptr->died_from, _("(緊急セーブ)", "(panic save)"));

	signals_ignore_tstp();

	if (save_player(p_ptr))
	{
		Term_putstr(45, hgt - 1, -1, TERM_RED, _("緊急セーブ成功！", "Panic save succeeded!"));
	}
	else
	{
		Term_putstr(45, hgt - 1, -1, TERM_RED, _("緊急セーブ失敗！", "Panic save failed!"));
	}

	Term_fresh();
	quit(_("ソフトのバグ", "software bug"));
}


/*!
 * @brief OSからのSIGTSTPシグナルを無視する関数 /
 * Ignore SIGTSTP signals (keyboard suspend)
 * @return なし
 * @details
 */
void signals_ignore_tstp(void)
{

#ifdef SIGTSTP
	(void)signal(SIGTSTP, SIG_IGN);
#endif

}


/*!
 * @brief OSからのSIGTSTPシグナルハンドラ /
 * Handle SIGTSTP signals (keyboard suspend)
 * @return なし
 * @details
 */
void signals_handle_tstp(void)
{

#ifdef SIGTSTP
	(void)signal(SIGTSTP, handle_signal_suspend);
#endif

}


/*!
 * @brief OSからのシグナルハンドルを初期化する /
 * Prepare to handle the relevant signals
 * @return なし
 * @details
 */
void signals_init(void)
{
#ifdef SIGHUP
	(void)signal(SIGHUP, SIG_IGN);
#endif

#ifdef SIGTSTP
	(void)signal(SIGTSTP, handle_signal_suspend);
#endif

#ifdef SIGINT
	(void)signal(SIGINT, handle_signal_simple);
#endif

#ifdef SIGQUIT
	(void)signal(SIGQUIT, handle_signal_simple);
#endif

#ifdef SIGFPE
	(void)signal(SIGFPE, handle_signal_abort);
#endif

#ifdef SIGILL
	(void)signal(SIGILL, handle_signal_abort);
#endif

#ifdef SIGTRAP
	(void)signal(SIGTRAP, handle_signal_abort);
#endif

#ifdef SIGIOT
	(void)signal(SIGIOT, handle_signal_abort);
#endif

#ifdef SIGKILL
	(void)signal(SIGKILL, handle_signal_abort);
#endif

#ifdef SIGBUS
	(void)signal(SIGBUS, handle_signal_abort);
#endif

#ifdef SIGSEGV
	(void)signal(SIGSEGV, handle_signal_abort);
#endif

#ifdef SIGTERM
	(void)signal(SIGTERM, handle_signal_abort);
#endif

#ifdef SIGPIPE
	(void)signal(SIGPIPE, handle_signal_abort);
#endif

#ifdef SIGEMT
	(void)signal(SIGEMT, handle_signal_abort);
#endif

#ifdef SIGDANGER
	(void)signal(SIGDANGER, handle_signal_abort);
#endif

#ifdef SIGSYS
	(void)signal(SIGSYS, handle_signal_abort);
#endif

#ifdef SIGXCPU
	(void)signal(SIGXCPU, handle_signal_abort);
#endif

#ifdef SIGPWR
	(void)signal(SIGPWR, handle_signal_abort);
#endif
}

#else


/*!
 * @brief ダミー /
 * Do nothing
 */
void signals_ignore_tstp(void)
{
}


/*!
 * @brief ダミー /
 * Do nothing
 */
void signals_handle_tstp(void)
{
}


/*!
 * @brief ダミー /
 * Do nothing
 */
void signals_init(void)
{
}
#endif
