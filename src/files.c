/* File: files.c */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

/* Purpose: code dealing with files (and death) */

#include "angband.h"


/*
 * You may or may not want to use the following "#undef".
 */
/* #undef _POSIX_SAVED_IDS */


/*
 * Hack -- drop permissions
 */
void safe_setuid_drop(void)
{

#ifdef SET_UID

# ifdef SAFE_SETUID

#  ifdef SAFE_SETUID_POSIX

	if (setuid(getuid()) != 0)
	{
#ifdef JP
quit("setuid(): 正しく許可が取れません！");
#else
		quit("setuid(): cannot set permissions correctly!");
#endif

	}
	if (setgid(getgid()) != 0)
	{
#ifdef JP
quit("setgid(): 正しく許可が取れません！");
#else
		quit("setgid(): cannot set permissions correctly!");
#endif

	}

#  else

	if (setreuid(geteuid(), getuid()) != 0)
	{
#ifdef JP
quit("setreuid(): 正しく許可が取れません！");
#else
		quit("setreuid(): cannot set permissions correctly!");
#endif

	}
	if (setregid(getegid(), getgid()) != 0)
	{
#ifdef JP
quit("setregid(): 正しく許可が取れません！");
#else
		quit("setregid(): cannot set permissions correctly!");
#endif

	}

#  endif

# endif

#endif

}


/*
 * Hack -- grab permissions
 */
void safe_setuid_grab(void)
{

#ifdef SET_UID

# ifdef SAFE_SETUID

#  ifdef SAFE_SETUID_POSIX

	if (setuid(player_euid) != 0)
	{
#ifdef JP
quit("setuid(): 正しく許可が取れません！");
#else
		quit("setuid(): cannot set permissions correctly!");
#endif

	}
	if (setgid(player_egid) != 0)
	{
#ifdef JP
quit("setgid(): 正しく許可が取れません！");
#else
		quit("setgid(): cannot set permissions correctly!");
#endif

	}

#  else

	if (setreuid(geteuid(), getuid()) != 0)
	{
#ifdef JP
quit("setreuid(): 正しく許可が取れません！");
#else
		quit("setreuid(): cannot set permissions correctly!");
#endif

	}
	if (setregid(getegid(), getgid()) != 0)
	{
#ifdef JP
quit("setregid(): 正しく許可が取れません！");
#else
		quit("setregid(): cannot set permissions correctly!");
#endif

	}

#  endif /* SAFE_SETUID_POSIX */

# endif /* SAFE_SETUID */

#endif /* SET_UID */

}


/*
 * Extract the first few "tokens" from a buffer
 *
 * This function uses "colon" and "slash" as the delimeter characters.
 *
 * We never extract more than "num" tokens.  The "last" token may include
 * "delimeter" characters, allowing the buffer to include a "string" token.
 *
 * We save pointers to the tokens in "tokens", and return the number found.
 *
 * Hack -- Attempt to handle the 'c' character formalism
 *
 * Hack -- An empty buffer, or a final delimeter, yields an "empty" token.
 *
 * Hack -- We will always extract at least one token
 */
s16b tokenize(char *buf, s16b num, char **tokens, int mode)
{
	int i = 0;

	char *s = buf;


	/* Process */
	while (i < num - 1)
	{
		char *t;

		/* Scan the string */
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

	/* Number found */
	return (i);
}


/* A number with a name */
typedef struct named_num named_num;

struct named_num
{
	cptr name;		/* The name of this thing */
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
	{"GF_PSY_SPEAR",			GF_PSY_SPEAR			},
	{"GF_MISSILE",				GF_MISSILE			},
	{"GF_ARROW",				GF_ARROW				},
	{"GF_PLASMA",				GF_PLASMA			},
	{"GF_WATER",				GF_WATER				},
	{"GF_LITE",					GF_LITE				},
	{"GF_DARK",					GF_DARK				},
	{"GF_LITE_WEAK",			GF_LITE_WEAK		},
	{"GF_DARK_WEAK",			GF_DARK_WEAK		},
	{"GF_SHARDS",				GF_SHARDS			},
	{"GF_SOUND",				GF_SOUND				},
	{"GF_CONFUSION",			GF_CONFUSION		},
	{"GF_FORCE",				GF_FORCE				},
	{"GF_INERTIA",				GF_INERTIA			},
	{"GF_MANA",					GF_MANA				},
	{"GF_METEOR",				GF_METEOR			},
	{"GF_ICE",					GF_ICE				},
	{"GF_CHAOS",				GF_CHAOS				},
	{"GF_NETHER",				GF_NETHER			},
	{"GF_DISENCHANT",			GF_DISENCHANT		},
	{"GF_NEXUS",				GF_NEXUS				},
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
	{"GF_OLD_POLY",			GF_OLD_POLY			},
	{"GF_OLD_HEAL",			GF_OLD_HEAL			},
	{"GF_OLD_SPEED",			GF_OLD_SPEED		},
	{"GF_OLD_SLOW",			GF_OLD_SLOW			},
	{"GF_OLD_CONF",			GF_OLD_CONF			},
	{"GF_OLD_SLEEP",			GF_OLD_SLEEP		},
	{"GF_OLD_DRAIN",			GF_OLD_DRAIN		},
	{"GF_AWAY_UNDEAD",		GF_AWAY_UNDEAD		},
	{"GF_AWAY_EVIL",			GF_AWAY_EVIL		},
	{"GF_AWAY_ALL",			GF_AWAY_ALL			},
	{"GF_TURN_UNDEAD",		GF_TURN_UNDEAD		},
	{"GF_TURN_EVIL",			GF_TURN_EVIL		},
	{"GF_TURN_ALL",			GF_TURN_ALL			},
	{"GF_DISP_UNDEAD",		GF_DISP_UNDEAD		},
	{"GF_DISP_EVIL",			GF_DISP_EVIL		},
	{"GF_DISP_ALL",			GF_DISP_ALL			},
	{"GF_DISP_DEMON",			GF_DISP_DEMON		},
	{"GF_DISP_LIVING",		GF_DISP_LIVING		},
	{"GF_ROCKET",				GF_ROCKET			},
	{"GF_NUKE",					GF_NUKE				},
	{"GF_MAKE_GLYPH",			GF_MAKE_GLYPH		},
	{"GF_STASIS",				GF_STASIS			},
	{"GF_STONE_WALL",			GF_STONE_WALL		},
	{"GF_DEATH_RAY",			GF_DEATH_RAY		},
	{"GF_STUN",					GF_STUN				},
	{"GF_HOLY_FIRE",			GF_HOLY_FIRE		},
	{"GF_HELL_FIRE",			GF_HELL_FIRE		},
	{"GF_DISINTEGRATE",		GF_DISINTEGRATE	},
	{"GF_CHARM",				GF_CHARM				},
	{"GF_CONTROL_UNDEAD",	GF_CONTROL_UNDEAD	},
	{"GF_CONTROL_ANIMAL",	GF_CONTROL_ANIMAL	},
	{"GF_PSI",					GF_PSI				},
	{"GF_PSI_DRAIN",			GF_PSI_DRAIN		},
	{"GF_TELEKINESIS",		GF_TELEKINESIS		},
	{"GF_JAM_DOOR",			GF_JAM_DOOR			},
	{"GF_DOMINATION",			GF_DOMINATION		},
	{"GF_DISP_GOOD",			GF_DISP_GOOD		},
	{"GF_DRAIN_MANA",			GF_DRAIN_MANA		},
	{"GF_MIND_BLAST",			GF_MIND_BLAST		},
	{"GF_BRAIN_SMASH",			GF_BRAIN_SMASH		},
	{"GF_CAUSE_1",			GF_CAUSE_1		},
	{"GF_CAUSE_2",			GF_CAUSE_2		},
	{"GF_CAUSE_3",			GF_CAUSE_3		},
	{"GF_CAUSE_4",			GF_CAUSE_4		},
	{"GF_HAND_DOOM",			GF_HAND_DOOM		},
	{"GF_CAPTURE",			GF_CAPTURE		},
	{"GF_ANIM_DEAD",			GF_ANIM_DEAD		},
	{"GF_CONTROL_LIVING",		GF_CONTROL_LIVING	},
	{"GF_IDENTIFY",			GF_IDENTIFY	},
	{"GF_ATTACK",			GF_ATTACK	},
	{"GF_ENGETSU",			GF_ENGETSU	},
	{"GF_GENOCIDE",			GF_GENOCIDE	},
	{"GF_PHOTO",			GF_PHOTO	},
	{"GF_CONTROL_DEMON",	GF_CONTROL_DEMON	},
	{"GF_LAVA_FLOW",	GF_LAVA_FLOW	},
	{"GF_BLOOD_CURSE",	GF_BLOOD_CURSE	},
	{"GF_SEEKER",			GF_SEEKER			},
	{"GF_SUPER_RAY",		GF_SUPER_RAY			},
	{"GF_STAR_HEAL",		GF_STAR_HEAL			},
	{"GF_WATER_FLOW",		GF_WATER_FLOW			},
	{"GF_CRUSADE",		GF_CRUSADE			},
	{"GF_STASIS_EVIL",			GF_STASIS_EVIL		},
	{"GF_WOUNDS",			GF_WOUNDS		},
	{NULL, 						0						}
};


/*
 * Parse a sub-file of the "extra info" (format shown below)
 *
 * Each "action" line has an "action symbol" in the first column,
 * followed by a colon, followed by some command specific info,
 * usually in the form of "tokens" separated by colons or slashes.
 *
 * Blank lines, lines starting with white space, and lines starting
 * with pound signs ("#") are ignored (as comments).
 *
 * Note the use of "tokenize()" to allow the use of both colons and
 * slashes as delimeters, while still allowing final tokens which
 * may contain any characters including "delimiters".
 *
 * Note the use of "strtol()" to allow all "integers" to be encoded
 * in decimal, hexidecimal, or octal form.
 *
 * Note that "monster zero" is used for the "player" attr/char, "object
 * zero" will be used for the "stack" attr/char, and "feature zero" is
 * used for the "nothing" attr/char.
 *
 * Parse another file recursively, see below for details
 *   %:<filename>
 *
 * Specify the attr/char values for "monsters" by race index
 *   R:<num>:<a>:<c>
 *
 * Specify the attr/char values for "objects" by kind index
 *   K:<num>:<a>:<c>
 *
 * Specify the attr/char values for "features" by feature index
 *   F:<num>:<a>:<c>
 *
 * Specify the attr/char values for unaware "objects" by kind tval
 *   U:<tv>:<a>:<c>
 *
 * Specify the attr/char values for inventory "objects" by kind tval
 *   E:<tv>:<a>:<c>
 *
 * Define a macro action, given an encoded macro action
 *   A:<str>
 *
 * Create a normal macro, given an encoded macro trigger
 *   P:<str>
 *
 * Create a command macro, given an encoded macro trigger
 *   C:<str>
 *
 * Create a keyset mapping
 *   S:<key>:<key>:<dir>
 *
 * Turn an option off, given its name
 *   X:<str>
 *
 * Turn an option on, given its name
 *   Y:<str>
 *
 * Specify visual information, given an index, and some data
 *   V:<num>:<kv>:<rv>:<gv>:<bv>
 *
 * Specify the set of colors to use when drawing a zapped spell
 *   Z:<type>:<str>
 *
 * Specify a macro trigger template and macro trigger names.
 *   T:<template>:<modifier chr>:<modifier name1>:<modifier name2>:...
 *   T:<trigger>:<keycode>:<shift-keycode>
 *
 */

errr process_pref_file_command(char *buf)
{
	int i, j, n1, n2;

	char *zz[16];


	/* Require "?:*" format */
	if (buf[1] != ':') return 1;


	switch (buf[0])
	{
	/* Mega-Hack -- read external player's history file */
	/* Process "H:<history>" */
	case 'H':
		add_history_from_pref_line(buf + 2);
		return 0;

	/* Process "R:<num>:<a>/<c>" -- attr/char for monster races */
	case 'R':
		if (tokenize(buf+2, 3, zz, TOKENIZE_CHECKQUOTE) == 3)
		{
			monster_race *r_ptr;
			i = (huge)strtol(zz[0], NULL, 0);
			n1 = strtol(zz[1], NULL, 0);
			n2 = strtol(zz[2], NULL, 0);
			if (i >= max_r_idx) return 1;
			r_ptr = &r_info[i];
			if (n1 || (!(n2 & 0x80) && n2)) r_ptr->x_attr = n1; /* Allow TERM_DARK text */
			if (n2) r_ptr->x_char = n2;
			return 0;
		}
		break;

	/* Process "K:<num>:<a>/<c>"  -- attr/char for object kinds */
	case 'K':
		if (tokenize(buf+2, 3, zz, TOKENIZE_CHECKQUOTE) == 3)
		{
			object_kind *k_ptr;
			i = (huge)strtol(zz[0], NULL, 0);
			n1 = strtol(zz[1], NULL, 0);
			n2 = strtol(zz[2], NULL, 0);
			if (i >= max_k_idx) return 1;
			k_ptr = &k_info[i];
			if (n1 || (!(n2 & 0x80) && n2)) k_ptr->x_attr = n1; /* Allow TERM_DARK text */
			if (n2) k_ptr->x_char = n2;
			return 0;
		}
		break;

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

			i = (huge)strtol(zz[0], NULL, 0);
			if (i >= max_f_idx) return 1;
			f_ptr = &f_info[i];

			n1 = strtol(zz[1], NULL, 0);
			n2 = strtol(zz[2], NULL, 0);
			if (n1 || (!(n2 & 0x80) && n2)) f_ptr->x_attr[F_LIT_STANDARD] = n1; /* Allow TERM_DARK text */
			if (n2) f_ptr->x_char[F_LIT_STANDARD] = n2;

			/* Mega-hack -- feat supports lighting */
			switch (num)
			{
			/* No lighting support */
			case 3:
				n1 = f_ptr->x_attr[F_LIT_STANDARD];
				n2 = f_ptr->x_char[F_LIT_STANDARD];
				for (j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++)
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
				for (j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++)
				{
					n1 = strtol(zz[j * 2 + 1], NULL, 0);
					n2 = strtol(zz[j * 2 + 2], NULL, 0);
					if (n1 || (!(n2 & 0x80) && n2)) f_ptr->x_attr[j] = n1; /* Allow TERM_DARK text */
					if (n2) f_ptr->x_char[j] = n2;
				}
				break;
			}
		}
		return 0;

	/* Process "S:<num>:<a>/<c>" -- attr/char for special things */
	case 'S':
		if (tokenize(buf+2, 3, zz, TOKENIZE_CHECKQUOTE) == 3)
		{
			j = (byte)strtol(zz[0], NULL, 0);
			n1 = strtol(zz[1], NULL, 0);
			n2 = strtol(zz[2], NULL, 0);
			misc_to_attr[j] = n1;
			misc_to_char[j] = n2;
			return 0;
		}
		break;

	/* Process "U:<tv>:<a>/<c>" -- attr/char for unaware items */
	case 'U':
		if (tokenize(buf+2, 3, zz, TOKENIZE_CHECKQUOTE) == 3)
		{
			j = (huge)strtol(zz[0], NULL, 0);
			n1 = strtol(zz[1], NULL, 0);
			n2 = strtol(zz[2], NULL, 0);
			for (i = 1; i < max_k_idx; i++)
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
		break;

	/* Process "E:<tv>:<a>" -- attribute for inventory objects */
	case 'E':
		if (tokenize(buf+2, 2, zz, TOKENIZE_CHECKQUOTE) == 2)
		{
			j = (byte)strtol(zz[0], NULL, 0) % 128;
			n1 = strtol(zz[1], NULL, 0);
			if (n1) tval_to_attr[j] = n1;
			return 0;
		}
		break;

	/* Process "A:<str>" -- save an "action" for later */
	case 'A':
		text_to_ascii(macro__buf, buf+2);
		return 0;

	/* Process "P:<str>" -- normal macro */
	case 'P':
	{
		char tmp[1024];

		text_to_ascii(tmp, buf+2);
		macro_add(tmp, macro__buf);
		return 0;
	}

	/* Process "C:<str>" -- create keymap */
	case 'C':
	{
		int mode;
		char tmp[1024];

		if (tokenize(buf+2, 2, zz, TOKENIZE_CHECKQUOTE) != 2) return 1;

		mode = strtol(zz[0], NULL, 0);
		if ((mode < 0) || (mode >= KEYMAP_MODES)) return 1;

		text_to_ascii(tmp, zz[1]);
		if (!tmp[0] || tmp[1]) return 1;
		i = (byte)(tmp[0]);

		string_free(keymap_act[mode][i]);

		keymap_act[mode][i] = string_make(macro__buf);

		return 0;
	}

	/* Process "V:<num>:<kv>:<rv>:<gv>:<bv>" -- visual info */
	case 'V':
		if (tokenize(buf+2, 5, zz, TOKENIZE_CHECKQUOTE) == 5)
		{
			i = (byte)strtol(zz[0], NULL, 0);
			angband_color_table[i][0] = (byte)strtol(zz[1], NULL, 0);
			angband_color_table[i][1] = (byte)strtol(zz[2], NULL, 0);
			angband_color_table[i][2] = (byte)strtol(zz[3], NULL, 0);
			angband_color_table[i][3] = (byte)strtol(zz[4], NULL, 0);
			return 0;
		}
		break;

	/* Process "X:<str>" -- turn option off */
	/* Process "Y:<str>" -- turn option on */
	case 'X':
	case 'Y':
		for (i = 0; option_info[i].o_desc; i++)
		{
			if (option_info[i].o_var &&
			    option_info[i].o_text &&
			    streq(option_info[i].o_text, buf + 2))
			{
				int os = option_info[i].o_set;
				int ob = option_info[i].o_bit;

				if ((p_ptr->playing || character_xtra) &&
					(OPT_PAGE_BIRTH == option_info[i].o_page) && !p_ptr->wizard)
				{
#ifdef JP
					msg_format("初期オプションは変更できません! '%s'", buf);
#else
					msg_format("Birth options can not changed! '%s'", buf);
#endif
					msg_print(NULL);
					return 0;
				}

				if (buf[0] == 'X')
				{
					/* Clear */
					option_flag[os] &= ~(1L << ob);
					(*option_info[i].o_var) = FALSE;
				}
				else
				{
					/* Set */
					option_flag[os] |= (1L << ob);
					(*option_info[i].o_var) = TRUE;
				}
				return 0;
			}
		}

		/* don't know that option. ignore it.*/
#ifdef JP
		msg_format("オプションの名前が正しくありません： %s", buf);
#else
		msg_format("Ignored invalid option: %s", buf);
#endif
		msg_print(NULL);
		return 0;

	/* Process "Z:<type>:<str>" -- set spell color */
	case 'Z':
	{
		/* Find the colon */
		char *t = my_strchr(buf + 2, ':');

		/* Oops */
		if (!t) return 1;

		/* Nuke the colon */
		*(t++) = '\0';

		for (i = 0; gf_desc[i].name; i++)
		{
			/* Match this type */
			if (streq(gf_desc[i].name, buf + 2))
			{
				/* Remember this color set */
				gf_color[gf_desc[i].num] = quark_add(t);

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
		int tok = tokenize(buf+2, 2+MAX_MACRO_MOD, zz, 0);

		/* Process "T:<template>:<modifier chr>:<modifier name>:..." */
		if (tok >= 4)
		{
			int i;
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
				for (i = 0; i < num; i++)
				{
					string_free(macro_modifier_name[i]);
				}

				/* Kill trigger name strings */
				for (i = 0; i < max_macrotrigger; i++)
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
			for (i = 0; i < num; i++)
			{
				macro_modifier_name[i] = string_make(zz[2+i]);
			}
		}

		/* Process "T:<trigger>:<keycode>:<shift-keycode>" */
		else if (tok >= 2)
		{
			char buf[1024];
			int m;
			char *t, *s;
			if (max_macrotrigger >= MAX_MACRO_TRIG)
			{
#ifdef JP
				msg_print("マクロトリガーの設定が多すぎます!");
#else
				msg_print("Too many macro triggers!");
#endif
				return 1;
			}
			m = max_macrotrigger;
			max_macrotrigger++;

			/* Take into account the escape character  */
			t = buf;
			s = zz[0];
			while (*s)
			{
				if ('\\' == *s) s++;
				*t++ = *s++;
			}
			*t = '\0';

			/* Get a trigger name */
			macro_trigger_name[m] = string_make(buf);

			/* Get the corresponding key code */
			macro_trigger_keycode[0][m] = string_make(zz[1]);

			if (tok == 3)
			{
				/* Key code of a combination of it with the shift key */
				macro_trigger_keycode[1][m] = string_make(zz[2]);
			}
			else
			{
				macro_trigger_keycode[1][m] = string_make(zz[1]);
			}
		}

		/* No error */
		return 0;
	}
	}

	/* Failure */
	return 1;
}


/*
 * Helper function for "process_pref_file()"
 *
 * Input:
 *   v: output buffer array
 *   f: final character
 *
 * Output:
 *   result
 */
cptr process_pref_file_expr(char **sp, char *fp)
{
	cptr v;

	char *b;
	char *s;

	char b1 = '[';
	char b2 = ']';

	char f = ' ';
	static char tmp[10];

	/* Initial */
	s = (*sp);

	/* Skip spaces */
	while (isspace(*s)) s++;

	/* Save start */
	b = s;

	/* Default */
	v = "?o?o?";

	/* Analyze */
	if (*s == b1)
	{
		const char *p;
		const char *t;

		/* Skip b1 */
		s++;

		/* First */
		t = process_pref_file_expr(&s, &f);

		/* Oops */
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
				t = process_pref_file_expr(&s, &f);
				if (*t && !streq(t, "0")) v = "1";
			}
		}

		/* Function: AND */
		else if (streq(t, "AND"))
		{
			v = "1";
			while (*s && (f != b2))
			{
				t = process_pref_file_expr(&s, &f);
				if (*t && streq(t, "0")) v = "0";
			}
		}

		/* Function: NOT */
		else if (streq(t, "NOT"))
		{
			v = "1";
			while (*s && (f != b2))
			{
				t = process_pref_file_expr(&s, &f);
				if (*t && streq(t, "1")) v = "0";
			}
		}

		/* Function: EQU */
		else if (streq(t, "EQU"))
		{
			v = "0";
			if (*s && (f != b2))
			{
				t = process_pref_file_expr(&s, &f);
			}
			while (*s && (f != b2))
			{
				p = process_pref_file_expr(&s, &f);
				if (streq(t, p)) v = "1";
			}
		}

		/* Function: LEQ */
		else if (streq(t, "LEQ"))
		{
			v = "1";
			if (*s && (f != b2))
			{
				t = process_pref_file_expr(&s, &f);
			}
			while (*s && (f != b2))
			{
				p = t;
				t = process_pref_file_expr(&s, &f);
				if (*t && atoi(p) > atoi(t)) v = "0";
			}
		}

		/* Function: GEQ */
		else if (streq(t, "GEQ"))
		{
			v = "1";
			if (*s && (f != b2))
			{
				t = process_pref_file_expr(&s, &f);
			}
			while (*s && (f != b2))
			{
				p = t;
				t = process_pref_file_expr(&s, &f);

				/* Compare two numbers instead of string */
				if (*t && atoi(p) < atoi(t)) v = "0";
			}
		}

		/* Oops */
		else
		{
			while (*s && (f != b2))
			{
				t = process_pref_file_expr(&s, &f);
			}
		}

		/* Verify ending */
		if (f != b2) v = "?x?x?";

		/* Extract final and Terminate */
		if ((f = *s) != '\0') *s++ = '\0';
	}

	/* Other */
	else
	{
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

		/* Variable */
		if (*b == '$')
		{
			/* System */
			if (streq(b+1, "SYS"))
			{
				v = ANGBAND_SYS;
			}

			else if (streq(b+1, "KEYBOARD"))
			{
				v = ANGBAND_KEYBOARD;
			}

			/* Graphics */
			else if (streq(b+1, "GRAF"))
			{
				v = ANGBAND_GRAF;
			}

			/* Monochrome mode */
			else if (streq(b+1, "MONOCHROME"))
			{
				if (arg_monochrome)
					v = "ON";
				else
					v = "OFF";
			}

			/* Race */
			else if (streq(b+1, "RACE"))
			{
#ifdef JP
				v = rp_ptr->E_title;
#else
				v = rp_ptr->title;
#endif
			}

			/* Class */
			else if (streq(b+1, "CLASS"))
			{
#ifdef JP
				v = cp_ptr->E_title;
#else
				v = cp_ptr->title;
#endif
			}

			/* Player */
			else if (streq(b+1, "PLAYER"))
			{
				static char tmp_player_name[32];
				char *pn, *tpn;
				for (pn = player_name, tpn = tmp_player_name; *pn; pn++, tpn++)
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
			else if (streq(b+1, "REALM1"))
			{
#ifdef JP
				v = E_realm_names[p_ptr->realm1];
#else
				v = realm_names[p_ptr->realm1];
#endif
			}

			/* Second realm */
			else if (streq(b+1, "REALM2"))
			{
#ifdef JP
				v = E_realm_names[p_ptr->realm2];
#else
				v = realm_names[p_ptr->realm2];
#endif
			}

			/* Level */
			else if (streq(b+1, "LEVEL"))
			{
				sprintf(tmp, "%02d", p_ptr->lev);
				v = tmp;
			}

			/* Autopick auto-register is in-use or not? */
			else if (streq(b+1, "AUTOREGISTER"))
			{
				if (p_ptr->autopick_autoregister)
					v = "1";
				else
					v = "0";
			}

			/* Money */
			else if (streq(b+1, "MONEY"))
			{
				sprintf(tmp, "%09d", p_ptr->au);
				v = tmp;
			}
		}

		/* Constant */
		else
		{
			v = b;
		}
	}

	/* Save */
	(*fp) = f;

	/* Save */
	(*sp) = s;

	/* Result */
	return (v);
}


#define PREF_TYPE_NORMAL   0
#define PREF_TYPE_AUTOPICK 1
#define PREF_TYPE_HISTPREF 2

/*
 * Open the "user pref file" and parse it.
 */
static errr process_pref_file_aux(cptr name, int preftype)
{
	FILE *fp;

	char buf[1024];

	char old[1024];

	int line = -1;

	errr err = 0;

	bool bypass = FALSE;


	/* Open the file */
	fp = my_fopen(name, "r");

	/* No such file */
	if (!fp) return (-1);

	/* Process the file */
	while (0 == my_fgets(fp, buf, sizeof(buf)))
	{
		/* Count lines */
		line++;

		/* Skip "empty" lines */
		if (!buf[0]) continue;

		/* Skip "blank" lines */
#ifdef JP
		if (!iskanji(buf[0]))
#endif
		if (isspace(buf[0])) continue;

		/* Skip comments */
		if (buf[0] == '#') continue;


		/* Save a copy */
		strcpy(old, buf);


		/* Process "?:<expr>" */
		if ((buf[0] == '?') && (buf[1] == ':'))
		{
			char f;
			cptr v;
			char *s;

			/* Start */
			s = buf + 2;

			/* Parse the expr */
			v = process_pref_file_expr(&s, &f);

			/* Set flag */
			bypass = (streq(v, "0") ? TRUE : FALSE);

			/* Continue */
			continue;
		}

		/* Apply conditionals */
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
				(void)process_autopick_file(buf + 2);
				break;
			case PREF_TYPE_HISTPREF:
				(void)process_histpref_file(buf + 2);
				break;
			default:
				(void)process_pref_file(buf + 2);
				break;
			}

			/* Set back depth level */
			depth_count--;

			/* Continue */
			continue;
		}


		/* Process the line */
		err = process_pref_file_command(buf);

		/* This is not original pref line... */
		if (err)
		{
			if (preftype != PREF_TYPE_AUTOPICK)
  				break;
			err = process_autopick_file_command(buf);
		}
	}


	/* Error */
	if (err)
	{
		/* Print error message */
		/* ToDo: Add better error messages */
#ifdef JP
	      msg_format("ファイル'%s'の%d行でエラー番号%dのエラー。", name, line, err);
	      msg_format("('%s'を解析中)", old);
#else
		msg_format("Error %d in line %d of file '%s'.", err, line, name);
		msg_format("Parsing '%s'", old);
#endif
		msg_print(NULL);
	}

	/* Close the file */
	my_fclose(fp);

	/* Result */
	return (err);
}



/*
 * Process the "user pref file" with the given name
 *
 * See the functions above for a list of legal "commands".
 *
 * We also accept the special "?" and "%" directives, which
 * allow conditional evaluation and filename inclusion.
 */
errr process_pref_file(cptr name)
{
	char buf[1024];

	errr err1, err2;

	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_PREF, name);

	/* Process the system pref file */
	err1 = process_pref_file_aux(buf, PREF_TYPE_NORMAL);

	/* Stop at parser errors, but not at non-existing file */
	if (err1 > 0) return err1;


	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, name);
	
	/* Process the user pref file */
	err2 = process_pref_file_aux(buf, PREF_TYPE_NORMAL);


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


/*
 * Handle CHECK_TIME
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



/*
 * Initialize CHECK_TIME
 */
errr check_time_init(void)
{

#ifdef CHECK_TIME

	FILE        *fp;

	char	buf[1024];


	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, "time.txt");

	/* Open the file */
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

	/* Close it */
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


/*
 * Handle CHECK_LOAD
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
		long val2 = (long)(check_load_value) * FSCALE;

		/* Check for violation */
		if (val1 >= val2) return (1);
	}

#endif

	/* Success */
	return (0);
}


/*
 * Initialize CHECK_LOAD
 */
errr check_load_init(void)
{

#ifdef CHECK_LOAD

	FILE        *fp;

	char	buf[1024];

	char	temphost[MAXHOSTNAMELEN+1];
	char	thishost[MAXHOSTNAMELEN+1];


	/* Build the filename */
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

		/* Done */
		break;
	}

	/* Close the file */
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
	{53, 10, -1, "打撃攻撃  :"},
	{53, 11, -1, "射撃攻撃  :"},
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
};
#endif

static void display_player_one_line(int entry, cptr val, byte attr)
{
	char buf[40];

	int row = disp_player_line[entry].row;
	int col = disp_player_line[entry].col;
	int len = disp_player_line[entry].len;
	cptr head = disp_player_line[entry].header;

	int head_len = strlen(head);

	Term_putstr(col, row, -1, TERM_WHITE, head);

	if (!val)
		return;

	if (len > 0)
	{
		int val_len = len - head_len;
		sprintf(buf, "%*.*s", val_len, val_len, val);
		Term_putstr(col + head_len, row, -1, attr, buf);
	}
	else
	{
		Term_putstr(col + head_len, row, -1, attr, val);
	}

	return;
}


static void display_player_melee_bonus(int hand, int hand_entry)
{
	char buf[160];
	int show_tohit = p_ptr->dis_to_h[hand];
	int show_todam = p_ptr->dis_to_d[hand];
	object_type *o_ptr = &inventory[INVEN_RARM + hand];

	/* Hack -- add in weapon info if known */
	if (object_is_known(o_ptr)) show_tohit += o_ptr->to_h;
	if (object_is_known(o_ptr)) show_todam += o_ptr->to_d;

	/* Melee attacks */
	sprintf(buf, "(%+d,%+d)", show_tohit, show_todam);

	/* Dump the bonuses to hit/dam */
	if (!buki_motteruka(INVEN_RARM) && !buki_motteruka(INVEN_LARM))
		display_player_one_line(ENTRY_BARE_HAND, buf, TERM_L_BLUE);
	else if (p_ptr->ryoute)
		display_player_one_line(ENTRY_TWO_HANDS, buf, TERM_L_BLUE);
	else
		display_player_one_line(hand_entry, buf, TERM_L_BLUE);
}


/*
 * Prints the following information on the screen.
 */
static void display_player_middle(void)
{
	char buf[160];

	/* Base skill */
	int show_tohit = p_ptr->dis_to_h_b;
	int show_todam = 0;

	/* Range weapon */
	object_type *o_ptr = &inventory[INVEN_BOW];

	int tmul = 0;
	int e;

	if (p_ptr->migite)
	{
		display_player_melee_bonus(0, left_hander ? ENTRY_LEFT_HAND1 : ENTRY_RIGHT_HAND1);
	}

	if (p_ptr->hidarite)
	{
		display_player_melee_bonus(1, left_hander ? ENTRY_RIGHT_HAND2: ENTRY_LEFT_HAND2);
	}
	else if ((p_ptr->pclass == CLASS_MONK) && (empty_hands(TRUE) & EMPTY_HAND_RARM))
	{
		int i;
		if (p_ptr->special_defense & KAMAE_MASK)
		{
			for (i = 0; i < MAX_KAMAE; i++)
			{
				if ((p_ptr->special_defense >> i) & KAMAE_GENBU) break;
			}
			if (i < MAX_KAMAE)
#ifdef JP
				display_player_one_line(ENTRY_POSTURE, format("%sの構え", kamae_shurui[i].desc), TERM_YELLOW);
#else
				display_player_one_line(ENTRY_POSTURE, format("%s form", kamae_shurui[i].desc), TERM_YELLOW);
#endif
		}
		else
#ifdef JP
				display_player_one_line(ENTRY_POSTURE, "構えなし", TERM_YELLOW);
#else
				display_player_one_line(ENTRY_POSTURE, "none", TERM_YELLOW);
#endif
	}

	/* Apply weapon bonuses */
	if (object_is_known(o_ptr)) show_tohit += o_ptr->to_h;
	if (object_is_known(o_ptr)) show_todam += o_ptr->to_d;

	if ((o_ptr->sval == SV_LIGHT_XBOW) || (o_ptr->sval == SV_HEAVY_XBOW))
		show_tohit += p_ptr->weapon_exp[0][o_ptr->sval] / 400;
	else
		show_tohit += (p_ptr->weapon_exp[0][o_ptr->sval] - (WEAPON_EXP_MASTER / 2)) / 200;

	/* Range attacks */
	display_player_one_line(ENTRY_SHOOT_HIT_DAM, format("(%+d,%+d)", show_tohit, show_todam), TERM_L_BLUE);

	if (inventory[INVEN_BOW].k_idx)
	{
		tmul = bow_tmul(inventory[INVEN_BOW].sval);

		/* Get extra "power" from "extra might" */
		if (p_ptr->xtra_might) tmul++;

		tmul = tmul * (100 + (int)(adj_str_td[p_ptr->stat_ind[A_STR]]) - 128);
	}

	/* shoot power */
	display_player_one_line(ENTRY_SHOOT_POWER, format("x%d.%02d", tmul/100, tmul%100), TERM_L_BLUE);

	/* Dump the armor class */
	display_player_one_line(ENTRY_BASE_AC, format("[%d,%+d]", p_ptr->dis_ac, p_ptr->dis_to_a), TERM_L_BLUE);

	/* Dump speed */
	{
		int tmp_speed = 0;
		byte attr;
		int i;

		i = p_ptr->pspeed-110;

		/* Hack -- Visually "undo" the Search Mode Slowdown */
		if (p_ptr->action == ACTION_SEARCH) i += 10;

		if (i > 0)
		{
			if (!p_ptr->riding)
				attr = TERM_L_GREEN;
			else
				attr = TERM_GREEN;
		}
		else if (i == 0)
		{
			if (!p_ptr->riding)
				attr = TERM_L_BLUE;
			else
				attr = TERM_GREEN;
		}
		else
		{
			if (!p_ptr->riding)
				attr = TERM_L_UMBER;
			else
				attr = TERM_RED;
		}

		if (!p_ptr->riding)
		{
			if (IS_FAST()) tmp_speed += 10;
			if (p_ptr->slow) tmp_speed -= 10;
			if (p_ptr->lightspeed) tmp_speed = 99;
		}
		else
		{
			if (MON_FAST(&m_list[p_ptr->riding])) tmp_speed += 10;
			if (MON_SLOW(&m_list[p_ptr->riding])) tmp_speed -= 10;
		}

		if (tmp_speed)
		{
			if (!p_ptr->riding)
				sprintf(buf, "(%+d%+d)", i-tmp_speed, tmp_speed);
			else
#ifdef JP
				sprintf(buf, "乗馬中 (%+d%+d)", i-tmp_speed, tmp_speed);
#else
				sprintf(buf, "Riding (%+d%+d)", i-tmp_speed, tmp_speed);
#endif

			if (tmp_speed > 0)
				attr = TERM_YELLOW;
			else
				attr = TERM_VIOLET;
		}
		else
		{
			if (!p_ptr->riding)
				sprintf(buf, "(%+d)", i);
			else
#ifdef JP
				sprintf(buf, "乗馬中 (%+d)", i);
#else
				sprintf(buf, "Riding (%+d)", i);
#endif
		}
	
		display_player_one_line(ENTRY_SPEED, buf, attr);
	}

	/* Dump character level */
	display_player_one_line(ENTRY_LEVEL, format("%d", p_ptr->lev), TERM_L_GREEN);

	/* Dump experience */
	if (p_ptr->prace == RACE_ANDROID) e = ENTRY_EXP_ANDR;
	else e = ENTRY_CUR_EXP;

	if (p_ptr->exp >= p_ptr->max_exp)
		display_player_one_line(e, format("%ld", p_ptr->exp), TERM_L_GREEN);
	else
		display_player_one_line(e, format("%ld", p_ptr->exp), TERM_YELLOW);

	/* Dump max experience */
	if (p_ptr->prace == RACE_ANDROID)
		/* Nothing */;
	else
		display_player_one_line(ENTRY_MAX_EXP, format("%ld", p_ptr->max_exp), TERM_L_GREEN);

	/* Dump exp to advance */
	if (p_ptr->prace == RACE_ANDROID) e = ENTRY_EXP_TO_ADV_ANDR;
	else e = ENTRY_EXP_TO_ADV;

	if (p_ptr->lev >= PY_MAX_LEVEL)
		display_player_one_line(e, "*****", TERM_L_GREEN);
	else if (p_ptr->prace == RACE_ANDROID)
		display_player_one_line(e, format("%ld", (s32b)(player_exp_a[p_ptr->lev - 1] * p_ptr->expfact / 100L)), TERM_L_GREEN);
	else
		display_player_one_line(e, format("%ld", (s32b)(player_exp[p_ptr->lev - 1] * p_ptr->expfact / 100L)), TERM_L_GREEN);

	/* Dump gold */
	display_player_one_line(ENTRY_GOLD, format("%ld", p_ptr->au), TERM_L_GREEN);

	/* Dump Day */
	{
		int day, hour, min;
		extract_day_hour_min(&day, &hour, &min);

#ifdef JP
		if (day < MAX_DAYS) sprintf(buf, "%d日目 %2d:%02d", day, hour, min);
		else sprintf(buf, "*****日目 %2d:%02d", hour, min);
#else
		if (day < MAX_DAYS) sprintf(buf, "Day %d %2d:%02d", day, hour, min);
		else sprintf(buf, "Day ***** %2d:%02d", hour, min);
#endif
	}
	display_player_one_line(ENTRY_DAY, buf, TERM_L_GREEN);

	/* Dump hit point */
	if (p_ptr->chp >= p_ptr->mhp) 
		display_player_one_line(ENTRY_HP, format("%4d/%4d", p_ptr->chp , p_ptr->mhp), TERM_L_GREEN);
	else if (p_ptr->chp > (p_ptr->mhp * hitpoint_warn) / 10) 
		display_player_one_line(ENTRY_HP, format("%4d/%4d", p_ptr->chp , p_ptr->mhp), TERM_YELLOW);
	else
		display_player_one_line(ENTRY_HP, format("%4d/%4d", p_ptr->chp , p_ptr->mhp), TERM_RED);

	/* Dump mana power */
	if (p_ptr->csp >= p_ptr->msp) 
		display_player_one_line(ENTRY_SP, format("%4d/%4d", p_ptr->csp , p_ptr->msp), TERM_L_GREEN);
	else if (p_ptr->csp > (p_ptr->msp * mana_warn) / 10) 
		display_player_one_line(ENTRY_SP, format("%4d/%4d", p_ptr->csp , p_ptr->msp), TERM_YELLOW);
	else
		display_player_one_line(ENTRY_SP, format("%4d/%4d", p_ptr->csp , p_ptr->msp), TERM_RED);

	/* Dump play time */
	display_player_one_line(ENTRY_PLAY_TIME, format("%.2lu:%.2lu:%.2lu", playtime/(60*60), (playtime/60)%60, playtime%60), TERM_L_GREEN);
}


/*
 * Hack -- pass color info around this file
 */
static byte likert_color = TERM_WHITE;


/*
 * Returns a "rating" of x depending on y
 */
static cptr likert(int x, int y)
{
	static char dummy[20] = "";

	/* Paranoia */
	if (y <= 0) y = 1;

	/* Negative value */
	if (x < 0)
	{
		likert_color = TERM_L_DARK;
#ifdef JP
		return "最低";
#else
		return "Very Bad";
#endif
	}

	/* Analyze the value */
	switch ((x / y))
	{
	case 0:
	case 1:
		likert_color = TERM_RED;
#ifdef JP
		return "悪い";
#else
		return "Bad";
#endif

	case 2:
		likert_color = TERM_L_RED;
#ifdef JP
		return "劣る";
#else
		return "Poor";
#endif

	case 3:
	case 4:
		likert_color = TERM_ORANGE;
#ifdef JP
		return "普通";
#else
		return "Fair";
#endif

	case 5:
		likert_color = TERM_YELLOW;
#ifdef JP
		return "良い";
#else
		return "Good";
#endif

	case 6:
		likert_color = TERM_YELLOW;
#ifdef JP
		return "大変良い";
#else
		return "Very Good";
#endif

	case 7:
	case 8:
		likert_color = TERM_L_GREEN;
#ifdef JP
		return "卓越";
#else
		return "Excellent";
#endif

	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
		likert_color = TERM_GREEN;
#ifdef JP
		return "超越";
#else
		return "Superb";
#endif

	case 14:
	case 15:
	case 16:
	case 17:
		likert_color = TERM_BLUE;
#ifdef JP
		return "英雄的";
#else
		return "Heroic";
#endif

	default:
		likert_color = TERM_VIOLET;
#ifdef JP
		sprintf(dummy, "伝説的[%d]", (int)((((x / y) - 17) * 5) / 2));
#else
		sprintf(dummy, "Legendary[%d]", (int)((((x / y) - 17) * 5) / 2));
#endif
		return dummy;
	}
}


/*
 * Prints ratings on certain abilities
 *
 * This code is "imitated" elsewhere to "dump" a character sheet.
 */
static void display_player_various(void)
{
	int         tmp, damage[2], blows1, blows2, i, basedam;
	int			xthn, xthb, xfos, xsrh;
	int			xdis, xdev, xsav, xstl;
	cptr		desc;
	int         muta_att = 0;
	u32b flgs[TR_FLAG_SIZE];
	int		shots, shot_frac;

	object_type		*o_ptr;

	if (p_ptr->muta2 & MUT2_HORNS)     muta_att++;
	if (p_ptr->muta2 & MUT2_SCOR_TAIL) muta_att++;
	if (p_ptr->muta2 & MUT2_BEAK)      muta_att++;
	if (p_ptr->muta2 & MUT2_TRUNK)     muta_att++;
	if (p_ptr->muta2 & MUT2_TENTACLES) muta_att++;

	xthn = p_ptr->skill_thn + (p_ptr->to_h_m * BTH_PLUS_ADJ);

	/* Shooting Skill (with current bow and normal missile) */
	o_ptr = &inventory[INVEN_BOW];
	tmp = p_ptr->to_h_b + o_ptr->to_h;
	xthb = p_ptr->skill_thb + (tmp * BTH_PLUS_ADJ);

	/* If the player is wielding one? */
	if (o_ptr->k_idx)
	{
		s16b energy_fire = bow_energy(o_ptr->sval);

		/* Calculate shots per round */
		shots = p_ptr->num_fire * 100;
		shot_frac = (shots * 100 / energy_fire) % 100;
		shots = shots / energy_fire;
		if (o_ptr->name1 == ART_CRIMSON)
		{
			shots = 1;
			shot_frac = 0;
			if (p_ptr->pclass == CLASS_ARCHER)
			{
				/* Extra shot at level 10 */
				if (p_ptr->lev >= 10) shots++;

				/* Extra shot at level 30 */
				if (p_ptr->lev >= 30) shots++;

				/* Extra shot at level 45 */
				if (p_ptr->lev >= 45) shots++;
			}
		}
	}
	else
	{
		shots = 0;
		shot_frac = 0;
	}

	for(i = 0; i < 2; i++)
	{
		damage[i] = p_ptr->dis_to_d[i] * 100;
		if (((p_ptr->pclass == CLASS_MONK) || (p_ptr->pclass == CLASS_FORCETRAINER)) && (empty_hands(TRUE) & EMPTY_HAND_RARM))
		{
			int level = p_ptr->lev;
			if (i)
			{
				damage[i] = 0;
				break;
			}
			if (p_ptr->pclass == CLASS_FORCETRAINER) level = MAX(1, level - 3);
			if (p_ptr->special_defense & KAMAE_BYAKKO)
				basedam = monk_ave_damage[level][1];
			else if (p_ptr->special_defense & (KAMAE_GENBU | KAMAE_SUZAKU))
				basedam = monk_ave_damage[level][2];
			else
				basedam = monk_ave_damage[level][0];
		}
		else
		{
			o_ptr = &inventory[INVEN_RARM + i];

			/* Average damage per round */
			if (o_ptr->k_idx)
			{
				if (object_is_known(o_ptr)) damage[i] += o_ptr->to_d * 100;
				basedam = ((o_ptr->dd + p_ptr->to_dd[i]) * (o_ptr->ds + p_ptr->to_ds[i] + 1)) * 50;
				object_flags_known(o_ptr, flgs);
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
				if ((p_ptr->pclass != CLASS_SAMURAI) && have_flag(flgs, TR_FORCE_WEAPON) && (p_ptr->csp > (o_ptr->dd * o_ptr->ds / 5)))
					basedam = basedam * 7 / 2;
			}
			else basedam = 0;
		}
		damage[i] += basedam;
		if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_DOKUBARI)) damage[i] = 1;
		if (damage[i] < 0) damage[i] = 0;
	}
	blows1 = p_ptr->migite ? p_ptr->num_blow[0]: 0;
	blows2 = p_ptr->hidarite ? p_ptr->num_blow[1] : 0;

	/* Basic abilities */

	xdis = p_ptr->skill_dis;
	xdev = p_ptr->skill_dev;
	xsav = p_ptr->skill_sav;
	xstl = p_ptr->skill_stl;
	xsrh = p_ptr->skill_srh;
	xfos = p_ptr->skill_fos;


	desc = likert(xthn, 12);
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

	if (!muta_att)
		display_player_one_line(ENTRY_BLOWS, format("%d+%d", blows1, blows2), TERM_L_BLUE);
	else
		display_player_one_line(ENTRY_BLOWS, format("%d+%d+%d", blows1, blows2, muta_att), TERM_L_BLUE);

	display_player_one_line(ENTRY_SHOTS, format("%d.%02d", shots, shot_frac), TERM_L_BLUE);


	if ((damage[0]+damage[1]) == 0)
		desc = "nil!";
	else
		desc = format("%d+%d", blows1 * damage[0] / 100, blows2 * damage[1] / 100);

	display_player_one_line(ENTRY_AVG_DMG, desc, TERM_L_BLUE);

	display_player_one_line(ENTRY_INFRA, format("%d feet", p_ptr->see_infra * 10), TERM_WHITE);
}



/*
 * Obtain the "flags" for the player as if he was an item
 */
static void player_flags(u32b flgs[TR_FLAG_SIZE])
{
	int i;

	/* Clear */
	for (i = 0; i < TR_FLAG_SIZE; i++)
		flgs[i] = 0L;

	/* Classes */
	switch (p_ptr->pclass)
	{
	case CLASS_WARRIOR:
		if (p_ptr->lev > 44)
			add_flag(flgs, TR_REGEN);
	case CLASS_SAMURAI:
		if (p_ptr->lev > 29)
			add_flag(flgs, TR_RES_FEAR);
		break;
	case CLASS_PALADIN:
		if (p_ptr->lev > 39)
			add_flag(flgs, TR_RES_FEAR);
		break;
	case CLASS_CHAOS_WARRIOR:
		if (p_ptr->lev > 29)
			add_flag(flgs, TR_RES_CHAOS);
		if (p_ptr->lev > 39)
			add_flag(flgs, TR_RES_FEAR);
		break;
	case CLASS_MONK:
	case CLASS_FORCETRAINER:
		if ((p_ptr->lev > 9) && !heavy_armor())
			add_flag(flgs, TR_SPEED);
		if ((p_ptr->lev>24) && !heavy_armor())
			add_flag(flgs, TR_FREE_ACT);
		break;
	case CLASS_NINJA:
		if (heavy_armor())
			add_flag(flgs, TR_SPEED);
		else
		{
			if ((!inventory[INVEN_RARM].k_idx || p_ptr->migite) &&
			    (!inventory[INVEN_LARM].k_idx || p_ptr->hidarite))
				add_flag(flgs, TR_SPEED);
			if (p_ptr->lev>24)
				add_flag(flgs, TR_FREE_ACT);
		}
		add_flag(flgs, TR_SLOW_DIGEST);
		add_flag(flgs, TR_RES_FEAR);
		if (p_ptr->lev > 19) add_flag(flgs, TR_RES_POIS);
		if (p_ptr->lev > 24) add_flag(flgs, TR_SUST_DEX);
		if (p_ptr->lev > 29) add_flag(flgs, TR_SEE_INVIS);
		break;
	case CLASS_MINDCRAFTER:
		if (p_ptr->lev > 9)
			add_flag(flgs, TR_RES_FEAR);
		if (p_ptr->lev > 19)
			add_flag(flgs, TR_SUST_WIS);
		if (p_ptr->lev > 29)
			add_flag(flgs, TR_RES_CONF);
		if (p_ptr->lev > 39)
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
		if (p_ptr->lev > 39) add_flag(flgs, TR_REFLECT);
		break;
	case CLASS_MIRROR_MASTER:
		if(p_ptr->lev > 39)add_flag(flgs, TR_REFLECT);
		break;
	default:
		break; /* Do nothing */
	}

	/* Races */
	if (p_ptr->mimic_form)
	{
		switch(p_ptr->mimic_form)
		{
		case MIMIC_DEMON:
			add_flag(flgs, TR_HOLD_LIFE);
			add_flag(flgs, TR_RES_CHAOS);
			add_flag(flgs, TR_RES_NETHER);
			add_flag(flgs, TR_RES_FIRE);
			add_flag(flgs, TR_SEE_INVIS);
			add_flag(flgs, TR_SPEED);
			break;
		case MIMIC_DEMON_LORD:
			add_flag(flgs, TR_HOLD_LIFE);
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
			add_flag(flgs, TR_HOLD_LIFE);
			add_flag(flgs, TR_RES_DARK);
			add_flag(flgs, TR_RES_NETHER);
			if (p_ptr->pclass != CLASS_NINJA) add_flag(flgs, TR_LITE);
			add_flag(flgs, TR_RES_POIS);
			add_flag(flgs, TR_RES_COLD);
			add_flag(flgs, TR_SEE_INVIS);
			add_flag(flgs, TR_SPEED);
			break;
		}
	}
	else
	{
	switch (p_ptr->prace)
	{
	case RACE_ELF:
		add_flag(flgs, TR_RES_LITE);
		break;
	case RACE_HOBBIT:
		add_flag(flgs, TR_HOLD_LIFE);
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
		if (p_ptr->lev > 14)
		{
			add_flag(flgs, TR_REGEN);
			if ((p_ptr->pclass == CLASS_WARRIOR) || (p_ptr->pclass == CLASS_BERSERKER))
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
		if (p_ptr->lev > 19)
			add_flag(flgs, TR_IM_ACID);
		break;
	case RACE_KLACKON:
		add_flag(flgs, TR_RES_CONF);
		add_flag(flgs, TR_RES_ACID);
		if (p_ptr->lev > 9)
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
		if (p_ptr->lev > 19)
			add_flag(flgs, TR_SEE_INVIS);
		break;
	case RACE_DRACONIAN:
		add_flag(flgs, TR_LEVITATION);
		if (p_ptr->lev > 4)
			add_flag(flgs, TR_RES_FIRE);
		if (p_ptr->lev > 9)
			add_flag(flgs, TR_RES_COLD);
		if (p_ptr->lev > 14)
			add_flag(flgs, TR_RES_ACID);
		if (p_ptr->lev > 19)
			add_flag(flgs, TR_RES_ELEC);
		if (p_ptr->lev > 34)
			add_flag(flgs, TR_RES_POIS);
		break;
	case RACE_MIND_FLAYER:
		add_flag(flgs, TR_SUST_INT);
		add_flag(flgs, TR_SUST_WIS);
		if (p_ptr->lev > 14)
			add_flag(flgs, TR_SEE_INVIS);
		if (p_ptr->lev > 29)
			add_flag(flgs, TR_TELEPATHY);
		break;
	case RACE_IMP:
		add_flag(flgs, TR_RES_FIRE);
		if (p_ptr->lev > 9)
			add_flag(flgs, TR_SEE_INVIS);
		break;
	case RACE_GOLEM:
		add_flag(flgs, TR_SEE_INVIS);
		add_flag(flgs, TR_FREE_ACT);
		add_flag(flgs, TR_RES_POIS);
		add_flag(flgs, TR_SLOW_DIGEST);
		if (p_ptr->lev > 34)
			add_flag(flgs, TR_HOLD_LIFE);
		break;
	case RACE_SKELETON:
		add_flag(flgs, TR_SEE_INVIS);
		add_flag(flgs, TR_RES_SHARDS);
		add_flag(flgs, TR_HOLD_LIFE);
		add_flag(flgs, TR_RES_POIS);
		if (p_ptr->lev > 9)
			add_flag(flgs, TR_RES_COLD);
		break;
	case RACE_ZOMBIE:
		add_flag(flgs, TR_SEE_INVIS);
		add_flag(flgs, TR_HOLD_LIFE);
		add_flag(flgs, TR_RES_NETHER);
		add_flag(flgs, TR_RES_POIS);
		add_flag(flgs, TR_SLOW_DIGEST);
		if (p_ptr->lev > 4)
			add_flag(flgs, TR_RES_COLD);
		break;
	case RACE_VAMPIRE:
		add_flag(flgs, TR_HOLD_LIFE);
		add_flag(flgs, TR_RES_DARK);
		add_flag(flgs, TR_RES_NETHER);
		if (p_ptr->pclass != CLASS_NINJA) add_flag(flgs, TR_LITE);
		add_flag(flgs, TR_RES_POIS);
		add_flag(flgs, TR_RES_COLD);
		break;
	case RACE_SPECTRE:
		add_flag(flgs, TR_LEVITATION);
		add_flag(flgs, TR_FREE_ACT);
		add_flag(flgs, TR_RES_COLD);
		add_flag(flgs, TR_SEE_INVIS);
		add_flag(flgs, TR_HOLD_LIFE);
		add_flag(flgs, TR_RES_NETHER);
		add_flag(flgs, TR_RES_POIS);
		add_flag(flgs, TR_SLOW_DIGEST);
		/* XXX pass_wall */
		if (p_ptr->lev > 34)
			add_flag(flgs, TR_TELEPATHY);
		break;
	case RACE_SPRITE:
		add_flag(flgs, TR_RES_LITE);
		add_flag(flgs, TR_LEVITATION);
		if (p_ptr->lev > 9)
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
		add_flag(flgs, TR_HOLD_LIFE);
		if (p_ptr->lev > 9)
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
		add_flag(flgs, TR_HOLD_LIFE);
		break;
	default:
		; /* Do nothing */
	}
	}

	/* Mutations */
	if (p_ptr->muta3)
	{
		if (p_ptr->muta3 & MUT3_FLESH_ROT)
		{
			remove_flag(flgs, TR_REGEN);
		}

		if ((p_ptr->muta3 & MUT3_XTRA_FAT) ||
			(p_ptr->muta3 & MUT3_XTRA_LEGS) ||
			(p_ptr->muta3 & MUT3_SHORT_LEG))
		{
			add_flag(flgs, TR_SPEED);
		}

		if (p_ptr->muta3  & MUT3_ELEC_TOUC)
		{
			add_flag(flgs, TR_SH_ELEC);
		}

		if (p_ptr->muta3 & MUT3_FIRE_BODY)
		{
			add_flag(flgs, TR_SH_FIRE);
			add_flag(flgs, TR_LITE);
		}

		if (p_ptr->muta3 & MUT3_WINGS)
		{
			add_flag(flgs, TR_LEVITATION);
		}

		if (p_ptr->muta3 & MUT3_FEARLESS)
		{
			add_flag(flgs, TR_RES_FEAR);
		}

		if (p_ptr->muta3 & MUT3_REGEN)
		{
			add_flag(flgs, TR_REGEN);
		}

		if (p_ptr->muta3 & MUT3_ESP)
		{
			add_flag(flgs, TR_TELEPATHY);
		}

		if (p_ptr->muta3 & MUT3_MOTION)
		{
			add_flag(flgs, TR_FREE_ACT);
		}
	}

	if (p_ptr->pseikaku == SEIKAKU_SEXY)
		add_flag(flgs, TR_AGGRAVATE);
	if (p_ptr->pseikaku == SEIKAKU_MUNCHKIN)
	{
		add_flag(flgs, TR_RES_BLIND);
		add_flag(flgs, TR_RES_CONF);
		add_flag(flgs, TR_HOLD_LIFE);
		if (p_ptr->pclass != CLASS_NINJA) add_flag(flgs, TR_LITE);
		if (p_ptr->lev > 9)
			add_flag(flgs, TR_SPEED);
	}
	if (p_ptr->special_defense & KATA_FUUJIN)
		add_flag(flgs, TR_REFLECT);
	if (p_ptr->special_defense & KAMAE_GENBU)
		add_flag(flgs, TR_REFLECT);
	if (p_ptr->special_defense & KAMAE_SUZAKU)
		add_flag(flgs, TR_LEVITATION);
	if (p_ptr->special_defense & KAMAE_SEIRYU)
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
	if (p_ptr->special_defense & KATA_MUSOU)
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
		add_flag(flgs, TR_HOLD_LIFE);
		add_flag(flgs, TR_FREE_ACT);
		add_flag(flgs, TR_SH_FIRE);
		add_flag(flgs, TR_SH_ELEC);
		add_flag(flgs, TR_SH_COLD);
		add_flag(flgs, TR_LEVITATION);
		add_flag(flgs, TR_LITE);
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


static void tim_player_flags(u32b flgs[TR_FLAG_SIZE])
{
	int i;

	/* Clear */
	for (i = 0; i < TR_FLAG_SIZE; i++)
		flgs[i] = 0L;

	if (IS_HERO() || p_ptr->shero)
		add_flag(flgs, TR_RES_FEAR);
	if (p_ptr->tim_invis)
		add_flag(flgs, TR_SEE_INVIS);
	if (p_ptr->tim_regen)
		add_flag(flgs, TR_REGEN);
	if (IS_TIM_ESP())
		add_flag(flgs, TR_TELEPATHY);
	if (IS_FAST() || p_ptr->slow)
		add_flag(flgs, TR_SPEED);

	if (IS_OPPOSE_ACID() && !(p_ptr->special_defense & DEFENSE_ACID) && !(prace_is_(RACE_YEEK) && (p_ptr->lev > 19)))
		add_flag(flgs, TR_RES_ACID);
	if (IS_OPPOSE_ELEC() && !(p_ptr->special_defense & DEFENSE_ELEC))
		add_flag(flgs, TR_RES_ELEC);
	if (IS_OPPOSE_FIRE() && !(p_ptr->special_defense & DEFENSE_FIRE))
		add_flag(flgs, TR_RES_FIRE);
	if (IS_OPPOSE_COLD() && !(p_ptr->special_defense & DEFENSE_COLD))
		add_flag(flgs, TR_RES_COLD);
	if (IS_OPPOSE_POIS())
		add_flag(flgs, TR_RES_POIS);

	if (p_ptr->special_attack & ATTACK_ACID)
		add_flag(flgs, TR_BRAND_ACID);
	if (p_ptr->special_attack & ATTACK_ELEC)
		add_flag(flgs, TR_BRAND_ELEC);
	if (p_ptr->special_attack & ATTACK_FIRE)
		add_flag(flgs, TR_BRAND_FIRE);
	if (p_ptr->special_attack & ATTACK_COLD)
		add_flag(flgs, TR_BRAND_COLD);
	if (p_ptr->special_attack & ATTACK_POIS)
		add_flag(flgs, TR_BRAND_POIS);
	if (p_ptr->special_defense & DEFENSE_ACID)
		add_flag(flgs, TR_IM_ACID);
	if (p_ptr->special_defense & DEFENSE_ELEC)
		add_flag(flgs, TR_IM_ELEC);
	if (p_ptr->special_defense & DEFENSE_FIRE)
		add_flag(flgs, TR_IM_FIRE);
	if (p_ptr->special_defense & DEFENSE_COLD)
		add_flag(flgs, TR_IM_COLD);
	if (p_ptr->wraith_form)
		add_flag(flgs, TR_REFLECT);
	/* by henkma */
	if (p_ptr->tim_reflect)
		add_flag(flgs, TR_REFLECT);

	if (p_ptr->magicdef)
	{
		add_flag(flgs, TR_RES_BLIND);
		add_flag(flgs, TR_RES_CONF);
		add_flag(flgs, TR_REFLECT);
		add_flag(flgs, TR_FREE_ACT);
		add_flag(flgs, TR_LEVITATION);
	}
	if (p_ptr->tim_res_nether)
	{
		add_flag(flgs, TR_RES_NETHER);
	}
	if (p_ptr->tim_sh_fire)
	{
		add_flag(flgs, TR_SH_FIRE);
	}
	if (p_ptr->ult_res)
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
		add_flag(flgs, TR_HOLD_LIFE);
		add_flag(flgs, TR_FREE_ACT);
		add_flag(flgs, TR_SH_FIRE);
		add_flag(flgs, TR_SH_ELEC);
		add_flag(flgs, TR_SH_COLD);
		add_flag(flgs, TR_LEVITATION);
		add_flag(flgs, TR_LITE);
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

	/* Hex bonuses */
	if (p_ptr->realm1 == REALM_HEX)
	{
		if (hex_spelling(HEX_DEMON_AURA))
		{
			add_flag(flgs, TR_SH_FIRE);
			add_flag(flgs, TR_REGEN);
		}
		if (hex_spelling(HEX_ICE_ARMOR)) add_flag(flgs, TR_SH_COLD);
		if (hex_spelling(HEX_SHOCK_CLOAK)) add_flag(flgs, TR_SH_ELEC);
	}
}


/* Mode flags for displaying player flags */
#define DP_CURSE   0x01
#define DP_IMM     0x02
#define DP_WP      0x08


/*
 * Equippy chars
 */
static void display_player_equippy(int y, int x, u16b mode)
{
	int i, max_i;

	byte a;
	char c;

	object_type *o_ptr;

	/* Weapon flags need only two column */
	if (mode & DP_WP) max_i = INVEN_LARM + 1;
	else max_i = INVEN_TOTAL;

	/* Dump equippy chars */
	for (i = INVEN_RARM; i < max_i; i++)
	{
		/* Object */
		o_ptr = &inventory[i];

		a = object_attr(o_ptr);
		c = object_char(o_ptr);

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


void print_equippy(void)
{
	display_player_equippy(ROW_EQUIPPY, COL_EQUIPPY, 0);
}

/*
 *
 */

static void known_obj_immunity(u32b flgs[TR_FLAG_SIZE])
{
	int i;

	/* Clear */
	for (i = 0; i < TR_FLAG_SIZE; i++)
		flgs[i] = 0L;

	/* Check equipment */
	for (i = INVEN_RARM; i < INVEN_TOTAL; i++)
	{
		u32b o_flgs[TR_FLAG_SIZE];

		object_type *o_ptr;

		/* Object */
		o_ptr = &inventory[i];

		if (!o_ptr->k_idx) continue;

		/* Known flags */
		object_flags_known(o_ptr, o_flgs);

		if (have_flag(o_flgs, TR_IM_ACID)) add_flag(flgs, TR_RES_ACID);
		if (have_flag(o_flgs, TR_IM_ELEC)) add_flag(flgs, TR_RES_ELEC);
		if (have_flag(o_flgs, TR_IM_FIRE)) add_flag(flgs, TR_RES_FIRE);
		if (have_flag(o_flgs, TR_IM_COLD)) add_flag(flgs, TR_RES_COLD);
	}
}

static void player_immunity(u32b flgs[TR_FLAG_SIZE])
{
	int i;

	/* Clear */
	for (i = 0; i < TR_FLAG_SIZE; i++)
		flgs[i] = 0L;

	if (prace_is_(RACE_SPECTRE))
		add_flag(flgs, TR_RES_NETHER);
	if (p_ptr->mimic_form == MIMIC_VAMPIRE || prace_is_(RACE_VAMPIRE))
		add_flag(flgs, TR_RES_DARK);
	if (p_ptr->mimic_form == MIMIC_DEMON_LORD)
		add_flag(flgs, TR_RES_FIRE);
	else if (prace_is_(RACE_YEEK) && p_ptr->lev > 19)
		add_flag(flgs, TR_RES_ACID);
}

static void tim_player_immunity(u32b flgs[TR_FLAG_SIZE])
{
	int i;

	/* Clear */
	for (i = 0; i < TR_FLAG_SIZE; i++)
		flgs[i] = 0L;

	if (p_ptr->special_defense & DEFENSE_ACID)
		add_flag(flgs, TR_RES_ACID);
	if (p_ptr->special_defense & DEFENSE_ELEC)
		add_flag(flgs, TR_RES_ELEC);
	if (p_ptr->special_defense & DEFENSE_FIRE)
		add_flag(flgs, TR_RES_FIRE);
	if (p_ptr->special_defense & DEFENSE_COLD)
		add_flag(flgs, TR_RES_COLD);
	if (p_ptr->wraith_form)
		add_flag(flgs, TR_RES_DARK);
}

static void player_vuln_flags(u32b flgs[TR_FLAG_SIZE])
{
	int i;

	/* Clear */
	for (i = 0; i < TR_FLAG_SIZE; i++)
		flgs[i] = 0L;

	if ((p_ptr->muta3 & MUT3_VULN_ELEM) || (p_ptr->special_defense & KATA_KOUKIJIN))
	{
		add_flag(flgs, TR_RES_ACID);
		add_flag(flgs, TR_RES_ELEC);
		add_flag(flgs, TR_RES_FIRE);
		add_flag(flgs, TR_RES_COLD);
	}
	if (prace_is_(RACE_ANDROID))
		add_flag(flgs, TR_RES_ELEC);
	if (prace_is_(RACE_ENT))
		add_flag(flgs, TR_RES_FIRE);
	if (prace_is_(RACE_VAMPIRE) || prace_is_(RACE_S_FAIRY) ||
	    (p_ptr->mimic_form == MIMIC_VAMPIRE))
		add_flag(flgs, TR_RES_LITE);
}


/*
 * A struct for storing misc. flags
 */
typedef struct {
	u32b player_flags[TR_FLAG_SIZE];
	u32b tim_player_flags[TR_FLAG_SIZE];
	u32b player_imm[TR_FLAG_SIZE];
	u32b tim_player_imm[TR_FLAG_SIZE];
	u32b player_vuln[TR_FLAG_SIZE];
	u32b known_obj_imm[TR_FLAG_SIZE];
} all_player_flags;


/*
 * Helper function, see below
 */
static void display_flag_aux(int row, int col, cptr header,
				    int flag1, all_player_flags *f, u16b mode)
{
	int     i;
	bool    vuln = FALSE;
	int max_i;

	if (have_flag(f->player_vuln, flag1) &&
	    !(have_flag(f->known_obj_imm, flag1) ||
	      have_flag(f->player_imm, flag1) ||
	      have_flag(f->tim_player_imm, flag1)))
		vuln = TRUE;

	/* Header */
	if (!(mode & DP_IMM)) c_put_str(TERM_WHITE, header, row, col);

	/* Advance */
	col += strlen(header) + 1;

	/* Weapon flags need only two column */
	if (mode & DP_WP) max_i = INVEN_LARM + 1;
	else max_i = INVEN_TOTAL;

	/* Check equipment */
	for (i = INVEN_RARM; i < max_i; i++)
	{
		u32b flgs[TR_FLAG_SIZE];
		object_type *o_ptr;

		/* Object */
		o_ptr = &inventory[i];

		/* Known flags */
		object_flags_known(o_ptr, flgs);

		/* Default */
		if (!(mode & DP_IMM))
			c_put_str((byte)(vuln ? TERM_RED : TERM_SLATE), ".", row, col);

		/* Check flags */
		if (mode & DP_CURSE)
		{
			if ((mode & DP_CURSE) && (o_ptr->curse_flags & (TRC_CURSED | TRC_HEAVY_CURSE)))
				c_put_str(TERM_WHITE, "+", row, col);
			if ((mode & DP_CURSE) && (o_ptr->curse_flags & TRC_PERMA_CURSE))
				c_put_str(TERM_WHITE, "*", row, col);
		}
		else
		{
			if (have_flag(flgs, flag1))
				c_put_str((byte)(vuln ? TERM_L_RED : TERM_WHITE),
					  (mode & DP_IMM) ? "*" : "+", row, col);
		}

		/* Advance */
		col++;
	}

	/* Assume that player flag is already written */
	if (mode & DP_IMM) return;

	/* Default */
	c_put_str((byte)(vuln ? TERM_RED : TERM_SLATE), ".", row, col);

	/* Player flags */
	if (have_flag(f->player_flags, flag1)) c_put_str((byte)(vuln ? TERM_L_RED : TERM_WHITE), "+", row, col);

	/* Timed player flags */
	if (have_flag(f->tim_player_flags, flag1)) c_put_str((byte)(vuln ? TERM_ORANGE : TERM_YELLOW), "#", row, col);

	/* Immunity */
	if (have_flag(f->tim_player_imm, flag1)) c_put_str(TERM_YELLOW, "*", row, col);
	if (have_flag(f->player_imm, flag1)) c_put_str(TERM_WHITE, "*", row, col);

	/* Vulnerability */
	if (vuln) c_put_str(TERM_RED, "v", row, col + 1);
}


/*
 * Special display, part 1
 */
static void display_player_flag_info(void)
{
	int row;
	int col;

	all_player_flags f;

	/* Extract flags and store */
	player_flags(f.player_flags);
	tim_player_flags(f.tim_player_flags);
	player_immunity(f.player_imm);
	tim_player_immunity(f.tim_player_imm);
	known_obj_immunity(f.known_obj_imm);
	player_vuln_flags(f.player_vuln);

	/*** Set 1 ***/

	row = 12;
	col = 1;

	display_player_equippy(row-2, col+8, 0);
	c_put_str(TERM_WHITE, "abcdefghijkl@", row-1, col+8);

#ifdef JP
display_flag_aux(row+0, col, "耐酸  :", TR_RES_ACID, &f, 0);
display_flag_aux(row+0, col, "耐酸  :", TR_IM_ACID, &f, DP_IMM);
display_flag_aux(row+1, col, "耐電撃:", TR_RES_ELEC, &f, 0);
display_flag_aux(row+1, col, "耐電撃:", TR_IM_ELEC, &f, DP_IMM);
display_flag_aux(row+2, col, "耐火炎:", TR_RES_FIRE, &f, 0);
display_flag_aux(row+2, col, "耐火炎:", TR_IM_FIRE, &f, DP_IMM);
display_flag_aux(row+3, col, "耐冷気:", TR_RES_COLD, &f, 0);
display_flag_aux(row+3, col, "耐冷気:", TR_IM_COLD, &f, DP_IMM);
display_flag_aux(row+4, col, "耐毒  :", TR_RES_POIS, &f, 0);
display_flag_aux(row+5, col, "耐閃光:", TR_RES_LITE, &f, 0);
display_flag_aux(row+6, col, "耐暗黒:", TR_RES_DARK, &f, 0);
display_flag_aux(row+7, col, "耐破片:", TR_RES_SHARDS, &f, 0);
display_flag_aux(row+8, col, "耐盲目:", TR_RES_BLIND, &f, 0);
display_flag_aux(row+9, col, "耐混乱:", TR_RES_CONF, &f, 0);
#else
	display_flag_aux(row+0, col, "Acid  :", TR_RES_ACID, &f, 0);
	display_flag_aux(row+0, col, "Acid  :", TR_IM_ACID, &f, DP_IMM);
	display_flag_aux(row+1, col, "Elec  :", TR_RES_ELEC, &f, 0);
	display_flag_aux(row+1, col, "Elec  :", TR_IM_ELEC, &f, DP_IMM);
	display_flag_aux(row+2, col, "Fire  :", TR_RES_FIRE, &f, 0);
	display_flag_aux(row+2, col, "Fire  :", TR_IM_FIRE, &f, DP_IMM);
	display_flag_aux(row+3, col, "Cold  :", TR_RES_COLD, &f, 0);
	display_flag_aux(row+3, col, "Cold  :", TR_IM_COLD, &f, DP_IMM);
	display_flag_aux(row+4, col, "Poison:", TR_RES_POIS, &f, 0);
	display_flag_aux(row+5, col, "Light :", TR_RES_LITE, &f, 0);
	display_flag_aux(row+6, col, "Dark  :", TR_RES_DARK, &f, 0);
	display_flag_aux(row+7, col, "Shard :", TR_RES_SHARDS, &f, 0);
	display_flag_aux(row+8, col, "Blind :", TR_RES_BLIND, &f, 0);
	display_flag_aux(row+9, col, "Conf  :", TR_RES_CONF, &f, 0);
#endif


	/*** Set 2 ***/

	row = 12;
	col = 26;

	display_player_equippy(row-2, col+8, 0);

	c_put_str(TERM_WHITE, "abcdefghijkl@", row-1, col+8);

#ifdef JP
display_flag_aux(row+0, col, "耐轟音:", TR_RES_SOUND, &f, 0);
display_flag_aux(row+1, col, "耐地獄:", TR_RES_NETHER, &f, 0);
display_flag_aux(row+2, col, "耐因混:", TR_RES_NEXUS, &f, 0);
display_flag_aux(row+3, col, "耐カオ:", TR_RES_CHAOS, &f, 0);
display_flag_aux(row+4, col, "耐劣化:", TR_RES_DISEN, &f, 0);
display_flag_aux(row+5, col, "耐恐怖:", TR_RES_FEAR, &f, 0);
display_flag_aux(row+6, col, "反射  :", TR_REFLECT, &f, 0);
display_flag_aux(row+7, col, "火炎オ:", TR_SH_FIRE, &f, 0);
display_flag_aux(row+8, col, "電気オ:", TR_SH_ELEC, &f, 0);
display_flag_aux(row+9, col, "冷気オ:", TR_SH_COLD, &f, 0);
#else
	display_flag_aux(row+0, col, "Sound :", TR_RES_SOUND, &f, 0);
	display_flag_aux(row+1, col, "Nether:", TR_RES_NETHER, &f, 0);
	display_flag_aux(row+2, col, "Nexus :", TR_RES_NEXUS, &f, 0);
	display_flag_aux(row+3, col, "Chaos :", TR_RES_CHAOS, &f, 0);
	display_flag_aux(row+4, col, "Disnch:", TR_RES_DISEN, &f, 0);
	display_flag_aux(row+5, col, "Fear  :", TR_RES_FEAR, &f, 0);
	display_flag_aux(row+6, col, "Reflct:", TR_REFLECT, &f, 0);
	display_flag_aux(row+7, col, "AuFire:", TR_SH_FIRE, &f, 0);
	display_flag_aux(row+8, col, "AuElec:", TR_SH_ELEC, &f, 0);
	display_flag_aux(row+9, col, "AuCold:", TR_SH_COLD, &f, 0);
#endif


	/*** Set 3 ***/

	row = 12;
	col = 51;

	display_player_equippy(row-2, col+12, 0);

	c_put_str(TERM_WHITE, "abcdefghijkl@", row-1, col+12);

#ifdef JP
display_flag_aux(row+0, col, "加速      :", TR_SPEED, &f, 0);
display_flag_aux(row+1, col, "耐麻痺    :", TR_FREE_ACT, &f, 0);
display_flag_aux(row+2, col, "透明体視認:", TR_SEE_INVIS, &f, 0);
display_flag_aux(row+3, col, "経験値保持:", TR_HOLD_LIFE, &f, 0);
display_flag_aux(row+4, col, "警告      :", TR_WARNING, &f, 0);
display_flag_aux(row+5, col, "遅消化    :", TR_SLOW_DIGEST, &f, 0);
display_flag_aux(row+6, col, "急回復    :", TR_REGEN, &f, 0);
display_flag_aux(row+7, col, "浮遊      :", TR_LEVITATION, &f, 0);
display_flag_aux(row+8, col, "永遠光源  :", TR_LITE, &f, 0);
display_flag_aux(row+9, col, "呪い      :", 0, &f, DP_CURSE);
#else
	display_flag_aux(row+0, col, "Speed     :", TR_SPEED, &f, 0);
	display_flag_aux(row+1, col, "FreeAction:", TR_FREE_ACT, &f, 0);
	display_flag_aux(row+2, col, "SeeInvisi.:", TR_SEE_INVIS, &f, 0);
	display_flag_aux(row+3, col, "Hold Life :", TR_HOLD_LIFE, &f, 0);
	display_flag_aux(row+4, col, "Warning   :", TR_WARNING, &f, 0);
	display_flag_aux(row+5, col, "SlowDigest:", TR_SLOW_DIGEST, &f, 0);
	display_flag_aux(row+6, col, "Regene.   :", TR_REGEN, &f, 0);
	display_flag_aux(row+7, col, "Levitation:", TR_LEVITATION, &f, 0);
	display_flag_aux(row+8, col, "Perm Lite :", TR_LITE, &f, 0);
	display_flag_aux(row+9, col, "Cursed    :", 0, &f, DP_CURSE);
#endif

}


/*
 * Special display, part 2
 */
static void display_player_other_flag_info(void)
{
	int row;
	int col;

	all_player_flags f;

	/* Extract flags and store */
	player_flags(f.player_flags);
	tim_player_flags(f.tim_player_flags);
	player_immunity(f.player_imm);
	tim_player_immunity(f.tim_player_imm);
	known_obj_immunity(f.known_obj_imm);
	player_vuln_flags(f.player_vuln);

	/*** Set 1 ***/

	row = 3;
	col = 1;

	display_player_equippy(row-2, col+12, DP_WP);

	c_put_str(TERM_WHITE, "ab@", row-1, col+12);

#ifdef JP
	display_flag_aux(row+ 0, col, "邪悪 倍打 :", TR_SLAY_EVIL, &f, DP_WP);
	display_flag_aux(row+ 0, col, "邪悪 倍打 :", TR_KILL_EVIL, &f, (DP_WP|DP_IMM));
	display_flag_aux(row+ 1, col, "不死 倍打 :", TR_SLAY_UNDEAD, &f, DP_WP);
	display_flag_aux(row+ 1, col, "不死 倍打 :", TR_KILL_UNDEAD, &f, (DP_WP|DP_IMM));
	display_flag_aux(row+ 2, col, "悪魔 倍打 :", TR_SLAY_DEMON, &f, DP_WP);
	display_flag_aux(row+ 2, col, "悪魔 倍打 :", TR_KILL_DEMON, &f, (DP_WP|DP_IMM));
	display_flag_aux(row+ 3, col, "龍 倍打   :", TR_SLAY_DRAGON, &f, DP_WP);
	display_flag_aux(row+ 3, col, "龍 倍打   :", TR_KILL_DRAGON, &f, (DP_WP|DP_IMM));
	display_flag_aux(row+ 4, col, "人間 倍打 :", TR_SLAY_HUMAN, &f, DP_WP);
	display_flag_aux(row+ 4, col, "人間 倍打 :", TR_KILL_HUMAN, &f, (DP_WP|DP_IMM));
	display_flag_aux(row+ 5, col, "動物 倍打 :", TR_SLAY_ANIMAL, &f, DP_WP);
	display_flag_aux(row+ 5, col, "動物 倍打 :", TR_KILL_ANIMAL, &f, (DP_WP|DP_IMM));
	display_flag_aux(row+ 6, col, "オーク倍打:", TR_SLAY_ORC, &f, DP_WP);
	display_flag_aux(row+ 6, col, "オーク倍打:", TR_KILL_ORC, &f, (DP_WP|DP_IMM));
	display_flag_aux(row+ 7, col, "トロル倍打:", TR_SLAY_TROLL, &f, DP_WP);
	display_flag_aux(row+ 7, col, "トロル倍打:", TR_KILL_TROLL, &f, (DP_WP|DP_IMM));
	display_flag_aux(row+ 8, col, "巨人 倍打 :", TR_SLAY_GIANT, &f, DP_WP);
	display_flag_aux(row+ 8, col, "巨人 倍打 :", TR_KILL_GIANT, &f, (DP_WP|DP_IMM));
	display_flag_aux(row+ 9, col, "溶解      :", TR_BRAND_ACID, &f, DP_WP);
	display_flag_aux(row+10, col, "電撃      :", TR_BRAND_ELEC, &f, DP_WP);
	display_flag_aux(row+11, col, "焼棄      :", TR_BRAND_FIRE, &f, DP_WP);
	display_flag_aux(row+12, col, "凍結      :", TR_BRAND_COLD, &f, DP_WP);
	display_flag_aux(row+13, col, "毒殺      :", TR_BRAND_POIS, &f, DP_WP);
	display_flag_aux(row+14, col, "切れ味    :", TR_VORPAL, &f, DP_WP);
	display_flag_aux(row+15, col, "地震      :", TR_IMPACT, &f, DP_WP);
	display_flag_aux(row+16, col, "吸血      :", TR_VAMPIRIC, &f, DP_WP);
	display_flag_aux(row+17, col, "カオス効果:", TR_CHAOTIC, &f, DP_WP);
	display_flag_aux(row+18, col, "理力      :", TR_FORCE_WEAPON, &f, DP_WP);
#else
	display_flag_aux(row+ 0, col, "Slay Evil :", TR_SLAY_EVIL, &f, DP_WP);
	display_flag_aux(row+ 0, col, "Slay Evil :", TR_KILL_EVIL, &f, (DP_WP|DP_IMM));
	display_flag_aux(row+ 1, col, "Slay Und. :", TR_SLAY_UNDEAD, &f, DP_WP);
	display_flag_aux(row+ 1, col, "Slay Und. :", TR_KILL_UNDEAD, &f, (DP_WP|DP_IMM));
	display_flag_aux(row+ 2, col, "Slay Demon:", TR_SLAY_DEMON, &f, DP_WP);
	display_flag_aux(row+ 2, col, "Slay Demon:", TR_KILL_DEMON, &f, (DP_WP|DP_IMM));
	display_flag_aux(row+ 3, col, "Slay Drag.:", TR_SLAY_DRAGON, &f, DP_WP);
	display_flag_aux(row+ 3, col, "Slay Drag.:", TR_KILL_DRAGON, &f, (DP_WP|DP_IMM));
	display_flag_aux(row+ 4, col, "Slay Human:", TR_SLAY_HUMAN, &f, DP_WP);
	display_flag_aux(row+ 4, col, "Slay Human:", TR_KILL_HUMAN, &f, (DP_WP|DP_IMM));
	display_flag_aux(row+ 5, col, "Slay Anim.:", TR_SLAY_ANIMAL, &f, DP_WP);
	display_flag_aux(row+ 5, col, "Slay Anim.:", TR_KILL_ANIMAL, &f, (DP_WP|DP_IMM));
	display_flag_aux(row+ 6, col, "Slay Orc  :", TR_SLAY_ORC, &f, DP_WP);
	display_flag_aux(row+ 6, col, "Slay Orc  :", TR_KILL_ORC, &f, (DP_WP|DP_IMM));
	display_flag_aux(row+ 7, col, "Slay Troll:", TR_SLAY_TROLL, &f, DP_WP);
	display_flag_aux(row+ 7, col, "Slay Troll:", TR_KILL_TROLL, &f, (DP_WP|DP_IMM));
	display_flag_aux(row+ 8, col, "Slay Giant:", TR_SLAY_GIANT, &f, DP_WP);
	display_flag_aux(row+ 8, col, "Slay Giant:", TR_KILL_GIANT, &f, (DP_WP|DP_IMM));
	display_flag_aux(row+ 9, col, "Acid Brand:", TR_BRAND_ACID, &f, DP_WP);
	display_flag_aux(row+10, col, "Elec Brand:", TR_BRAND_ELEC, &f, DP_WP);
	display_flag_aux(row+11, col, "Fire Brand:", TR_BRAND_FIRE, &f, DP_WP);
	display_flag_aux(row+12, col, "Cold Brand:", TR_BRAND_COLD, &f, DP_WP);
	display_flag_aux(row+13, col, "Poison Brd:", TR_BRAND_POIS, &f, DP_WP);
	display_flag_aux(row+14, col, "Sharpness :", TR_VORPAL, &f, DP_WP);
	display_flag_aux(row+15, col, "Quake     :", TR_IMPACT, &f, DP_WP);
	display_flag_aux(row+16, col, "Vampiric  :", TR_VAMPIRIC, &f, DP_WP);
	display_flag_aux(row+17, col, "Chaotic   :", TR_CHAOTIC, &f, DP_WP);
	display_flag_aux(row+18, col, "Force Wep.:", TR_FORCE_WEAPON, &f, DP_WP);
#endif


	/*** Set 2 ***/

	row = 3;
	col = col + 12 + 7;

	display_player_equippy(row-2, col+12, 0);
	c_put_str(TERM_WHITE, "abcdefghijkl@", row-1, col+12);

#ifdef JP
	display_flag_aux(row+ 0, col, "テレパシー:", TR_TELEPATHY, &f, 0);
	display_flag_aux(row+ 1, col, "邪悪ESP   :", TR_ESP_EVIL, &f, 0);
	display_flag_aux(row+ 2, col, "無生物ESP :", TR_ESP_NONLIVING, &f, 0);
	display_flag_aux(row+ 3, col, "善良ESP   :", TR_ESP_GOOD, &f, 0);
	display_flag_aux(row+ 4, col, "不死ESP   :", TR_ESP_UNDEAD, &f, 0);
	display_flag_aux(row+ 5, col, "悪魔ESP   :", TR_ESP_DEMON, &f, 0);
	display_flag_aux(row+ 6, col, "龍ESP     :", TR_ESP_DRAGON, &f, 0);
	display_flag_aux(row+ 7, col, "人間ESP   :", TR_ESP_HUMAN, &f, 0);
	display_flag_aux(row+ 8, col, "動物ESP   :", TR_ESP_ANIMAL, &f, 0);
	display_flag_aux(row+ 9, col, "オークESP :", TR_ESP_ORC, &f, 0);
	display_flag_aux(row+10, col, "トロルESP :", TR_ESP_TROLL, &f, 0);
	display_flag_aux(row+11, col, "巨人ESP   :", TR_ESP_GIANT, &f, 0);

	display_flag_aux(row+13, col, "腕力維持  :", TR_SUST_STR, &f, 0);
	display_flag_aux(row+14, col, "知力維持  :", TR_SUST_INT, &f, 0);
	display_flag_aux(row+15, col, "賢さ維持  :", TR_SUST_WIS, &f, 0);
	display_flag_aux(row+16, col, "器用維持  :", TR_SUST_DEX, &f, 0);
	display_flag_aux(row+17, col, "耐久維持  :", TR_SUST_CON, &f, 0);
	display_flag_aux(row+18, col, "魅力維持  :", TR_SUST_CHR, &f, 0);
#else
	display_flag_aux(row+ 0, col, "Telepathy :", TR_TELEPATHY, &f, 0);
	display_flag_aux(row+ 1, col, "ESP Evil  :", TR_ESP_EVIL, &f, 0);
	display_flag_aux(row+ 2, col, "ESP Noliv.:", TR_ESP_NONLIVING, &f, 0);
	display_flag_aux(row+ 3, col, "ESP Good  :", TR_ESP_GOOD, &f, 0);
	display_flag_aux(row+ 4, col, "ESP Undead:", TR_ESP_UNDEAD, &f, 0);
	display_flag_aux(row+ 5, col, "ESP Demon :", TR_ESP_DEMON, &f, 0);
	display_flag_aux(row+ 6, col, "ESP Dragon:", TR_ESP_DRAGON, &f, 0);
	display_flag_aux(row+ 7, col, "ESP Human :", TR_ESP_HUMAN, &f, 0);
	display_flag_aux(row+ 8, col, "ESP Animal:", TR_ESP_ANIMAL, &f, 0);
	display_flag_aux(row+ 9, col, "ESP Orc   :", TR_ESP_ORC, &f, 0);
	display_flag_aux(row+10, col, "ESP Troll :", TR_ESP_TROLL, &f, 0);
	display_flag_aux(row+11, col, "ESP Giant :", TR_ESP_GIANT, &f, 0);

	display_flag_aux(row+13, col, "Sust Str  :", TR_SUST_STR, &f, 0);
	display_flag_aux(row+14, col, "Sust Int  :", TR_SUST_INT, &f, 0);
	display_flag_aux(row+15, col, "Sust Wis  :", TR_SUST_WIS, &f, 0);
	display_flag_aux(row+16, col, "Sust Dex  :", TR_SUST_DEX, &f, 0);
	display_flag_aux(row+17, col, "Sust Con  :", TR_SUST_CON, &f, 0);
	display_flag_aux(row+18, col, "Sust Chr  :", TR_SUST_CHR, &f, 0);
#endif


	/*** Set 3 ***/

	row = 3;
	col = col + 12 + 17;

	display_player_equippy(row-2, col+14, 0);

	c_put_str(TERM_WHITE, "abcdefghijkl@", row-1, col+14);

#ifdef JP
	display_flag_aux(row+ 0, col, "追加攻撃    :", TR_BLOWS, &f, 0);
	display_flag_aux(row+ 1, col, "採掘        :", TR_TUNNEL, &f, 0);
	display_flag_aux(row+ 2, col, "赤外線視力  :", TR_INFRA, &f, 0);
	display_flag_aux(row+ 3, col, "魔法道具支配:", TR_MAGIC_MASTERY, &f, 0);
	display_flag_aux(row+ 4, col, "隠密        :", TR_STEALTH, &f, 0);
	display_flag_aux(row+ 5, col, "探索        :", TR_SEARCH, &f, 0);

	display_flag_aux(row+ 7, col, "乗馬        :", TR_RIDING, &f, 0);
	display_flag_aux(row+ 8, col, "投擲        :", TR_THROW, &f, 0);
	display_flag_aux(row+ 9, col, "祝福        :", TR_BLESSED, &f, 0);
	display_flag_aux(row+10, col, "反テレポート:", TR_NO_TELE, &f, 0);
	display_flag_aux(row+11, col, "反魔法      :", TR_NO_MAGIC, &f, 0);
	display_flag_aux(row+12, col, "消費魔力減少:", TR_DEC_MANA, &f, 0);

	display_flag_aux(row+14, col, "経験値減少  :", TR_DRAIN_EXP, &f, 0);
	display_flag_aux(row+15, col, "乱テレポート:", TR_TELEPORT, &f, 0);
	display_flag_aux(row+16, col, "反感        :", TR_AGGRAVATE, &f, 0);
	display_flag_aux(row+17, col, "太古の怨念  :", TR_TY_CURSE, &f, 0);
#else
	display_flag_aux(row+ 0, col, "Add Blows   :", TR_BLOWS, &f, 0);
	display_flag_aux(row+ 1, col, "Add Tunnel  :", TR_TUNNEL, &f, 0);
	display_flag_aux(row+ 2, col, "Add Infra   :", TR_INFRA, &f, 0);
	display_flag_aux(row+ 3, col, "Add Device  :", TR_MAGIC_MASTERY, &f, 0);
	display_flag_aux(row+ 4, col, "Add Stealth :", TR_STEALTH, &f, 0);
	display_flag_aux(row+ 5, col, "Add Search  :", TR_SEARCH, &f, 0);

	display_flag_aux(row+ 7, col, "Riding      :", TR_RIDING, &f, 0);
	display_flag_aux(row+ 8, col, "Throw       :", TR_THROW, &f, 0);
	display_flag_aux(row+ 9, col, "Blessed     :", TR_BLESSED, &f, 0);
	display_flag_aux(row+10, col, "No Teleport :", TR_NO_TELE, &f, 0);
	display_flag_aux(row+11, col, "Anti Magic  :", TR_NO_MAGIC, &f, 0);
	display_flag_aux(row+12, col, "Econom. Mana:", TR_DEC_MANA, &f, 0);

	display_flag_aux(row+14, col, "Drain Exp   :", TR_DRAIN_EXP, &f, 0);
	display_flag_aux(row+15, col, "Rnd.Teleport:", TR_TELEPORT, &f, 0);
	display_flag_aux(row+16, col, "Aggravate   :", TR_AGGRAVATE, &f, 0);
	display_flag_aux(row+17, col, "TY Curse    :", TR_TY_CURSE, &f, 0);
#endif

}


/*
 * Special display, part 2a
 */
static void display_player_misc_info(void)
{
	char	buf[80];
	char	tmp[80];

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

	strcpy(tmp,ap_ptr->title);
#ifdef JP
	if(ap_ptr->no == 1)
		strcat(tmp,"の");
#else
		strcat(tmp," ");
#endif
	strcat(tmp,player_name);

	c_put_str(TERM_L_BLUE, tmp, 1, 34);
	c_put_str(TERM_L_BLUE, sp_ptr->title, 3, 9);
	c_put_str(TERM_L_BLUE, (p_ptr->mimic_form ? mimic_info[p_ptr->mimic_form].title : rp_ptr->title), 4, 9);
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


	(void)sprintf(buf, "%d", (int)p_ptr->lev);
	c_put_str(TERM_L_BLUE, buf, 6, 9);
	(void)sprintf(buf, "%d/%d", (int)p_ptr->chp, (int)p_ptr->mhp);
	c_put_str(TERM_L_BLUE, buf, 7, 9);
	(void)sprintf(buf, "%d/%d", (int)p_ptr->csp, (int)p_ptr->msp);
	c_put_str(TERM_L_BLUE, buf, 8, 9);
}


/*
 * Special display, part 2b
 *
 * How to print out the modifications and sustains.
 * Positive mods with no sustain will be light green.
 * Positive mods with a sustain will be dark green.
 * Sustains (with no modification) will be a dark green 's'.
 * Negative mods (from a curse) will be red.
 * Huge mods (>9), like from MICoMorgoth, will be a '*'
 * No mod, no sustain, will be a slate '.'
 */
static void display_player_stat_info(void)
{
	int i, e_adj;
	int stat_col, stat;
	int row, col;

	object_type *o_ptr;
	u32b flgs[TR_FLAG_SIZE];

	byte a;
	char c;

	char buf[80];


	/* Column */
	stat_col = 22;

	/* Row */
	row = 3;

	/* Print out the labels for the columns */
#ifdef JP
c_put_str(TERM_WHITE, "能力", row, stat_col+1);
c_put_str(TERM_BLUE, "  基本", row, stat_col+7);
c_put_str(TERM_L_BLUE, " 種 職 性 装 ", row, stat_col+13);
c_put_str(TERM_L_GREEN, "合計", row, stat_col+28);
c_put_str(TERM_YELLOW, "現在", row, stat_col+35);
#else
	c_put_str(TERM_WHITE, "Stat", row, stat_col+1);
	c_put_str(TERM_BLUE, "  Base", row, stat_col+7);
	c_put_str(TERM_L_BLUE, "RacClaPerMod", row, stat_col+13);
	c_put_str(TERM_L_GREEN, "Actual", row, stat_col+26);
	c_put_str(TERM_YELLOW, "Current", row, stat_col+32);
#endif


	/* Display the stats */
	for (i = 0; i < 6; i++)
	{
		int r_adj;

		if (p_ptr->mimic_form) r_adj = mimic_info[p_ptr->mimic_form].r_adj[i];
		else r_adj = rp_ptr->r_adj[i];

		/* Calculate equipment adjustment */
		e_adj = 0;

		/* Icky formula to deal with the 18 barrier */
		if ((p_ptr->stat_max[i] > 18) && (p_ptr->stat_top[i] > 18))
			e_adj = (p_ptr->stat_top[i] - p_ptr->stat_max[i]) / 10;
		if ((p_ptr->stat_max[i] <= 18) && (p_ptr->stat_top[i] <= 18))
			e_adj = p_ptr->stat_top[i] - p_ptr->stat_max[i];
		if ((p_ptr->stat_max[i] <= 18) && (p_ptr->stat_top[i] > 18))
			e_adj = (p_ptr->stat_top[i] - 18) / 10 - p_ptr->stat_max[i] + 18;

		if ((p_ptr->stat_max[i] > 18) && (p_ptr->stat_top[i] <= 18))
			e_adj = p_ptr->stat_top[i] - (p_ptr->stat_max[i] - 19) / 10 - 19;

		if (prace_is_(RACE_ENT))
		{
			switch (i)
			{
				case A_STR:
				case A_CON:
					if (p_ptr->lev > 25) r_adj++;
					if (p_ptr->lev > 40) r_adj++;
					if (p_ptr->lev > 45) r_adj++;
					break;
				case A_DEX:
					if (p_ptr->lev > 25) r_adj--;
					if (p_ptr->lev > 40) r_adj--;
					if (p_ptr->lev > 45) r_adj--;
					break;
			}
		}

		e_adj -= r_adj;
		e_adj -= cp_ptr->c_adj[i];
		e_adj -= ap_ptr->a_adj[i];

		if (p_ptr->stat_cur[i] < p_ptr->stat_max[i])
			/* Reduced name of stat */
			c_put_str(TERM_WHITE, stat_names_reduced[i], row + i+1, stat_col+1);
		else
			c_put_str(TERM_WHITE, stat_names[i], row + i+1, stat_col+1);


		/* Internal "natural" max value.  Maxes at 18/100 */
		/* This is useful to see if you are maxed out */
		cnv_stat(p_ptr->stat_max[i], buf);
		if (p_ptr->stat_max[i] == p_ptr->stat_max_max[i])
		{
#ifdef JP
			c_put_str(TERM_WHITE, "!", row + i+1, stat_col + 6);
#else
			c_put_str(TERM_WHITE, "!", row + i+1, stat_col + 4);
#endif
		}
		c_put_str(TERM_BLUE, buf, row + i+1, stat_col + 13 - strlen(buf));

		/* Race, class, and equipment modifiers */
		(void)sprintf(buf, "%3d", r_adj);
		c_put_str(TERM_L_BLUE, buf, row + i+1, stat_col + 13);
		(void)sprintf(buf, "%3d", (int)cp_ptr->c_adj[i]);
		c_put_str(TERM_L_BLUE, buf, row + i+1, stat_col + 16);
		(void)sprintf(buf, "%3d", (int)ap_ptr->a_adj[i]);
		c_put_str(TERM_L_BLUE, buf, row + i+1, stat_col + 19);
		(void)sprintf(buf, "%3d", (int)e_adj);
		c_put_str(TERM_L_BLUE, buf, row + i+1, stat_col + 22);

		/* Actual maximal modified value */
		cnv_stat(p_ptr->stat_top[i], buf);
		c_put_str(TERM_L_GREEN, buf, row + i+1, stat_col + 26);

		/* Only display stat_use if not maximal */
		if (p_ptr->stat_use[i] < p_ptr->stat_top[i])
		{
			cnv_stat(p_ptr->stat_use[i], buf);
			c_put_str(TERM_YELLOW, buf, row + i+1, stat_col + 33);
		}
	}

	/* Column */
	col = stat_col + 41;

	/* Header and Footer */
	c_put_str(TERM_WHITE, "abcdefghijkl@", row, col);
#ifdef JP
c_put_str(TERM_L_GREEN, "能力修正", row - 1, col);
#else
	c_put_str(TERM_L_GREEN, "Modification", row - 1, col);
#endif


	/* Process equipment */
	for (i = INVEN_RARM; i < INVEN_TOTAL; i++)
	{
		/* Access object */
		o_ptr = &inventory[i];

		/* Acquire "known" flags */
		object_flags_known(o_ptr, flgs);

		/* Initialize color based of sign of pval. */
		for (stat = 0; stat < 6; stat++)
		{
			/* Default */
			a = TERM_SLATE;
			c = '.';

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
			Term_putch(col, row + stat+1, a, c);
		}

		/* Advance */
		col++;
	}

	/* Player flags */
	player_flags(flgs);

	/* Check stats */
	for (stat = 0; stat < 6; stat++)
	{
		/* Default */
		a = TERM_SLATE;
		c = '.';

		/* Mutations ... */
		if (p_ptr->muta3 || p_ptr->tsuyoshi)
		{
			int dummy = 0;

			if (stat == A_STR)
			{
				if (p_ptr->muta3 & MUT3_HYPER_STR) dummy += 4;
				if (p_ptr->muta3 & MUT3_PUNY) dummy -= 4;
				if (p_ptr->tsuyoshi) dummy += 4;
			}
			else if (stat == A_WIS || stat == A_INT)
			{
				if (p_ptr->muta3 & MUT3_HYPER_INT) dummy += 4;
				if (p_ptr->muta3 & MUT3_MORONIC) dummy -= 4;
			}
			else if (stat == A_DEX)
			{
				if (p_ptr->muta3 & MUT3_IRON_SKIN) dummy -= 1;
				if (p_ptr->muta3 & MUT3_LIMBER) dummy += 3;
				if (p_ptr->muta3 & MUT3_ARTHRITIS) dummy -= 3;
			}
			else if (stat == A_CON)
			{
				if (p_ptr->muta3 & MUT3_RESILIENT) dummy += 4;
				if (p_ptr->muta3 & MUT3_XTRA_FAT) dummy += 2;
				if (p_ptr->muta3 & MUT3_ALBINO) dummy -= 4;
				if (p_ptr->muta3 & MUT3_FLESH_ROT) dummy -= 2;
				if (p_ptr->tsuyoshi) dummy += 4;
			}
			else if (stat == A_CHR)
			{
				if (p_ptr->muta3 & MUT3_SILLY_VOI) dummy -= 4;
				if (p_ptr->muta3 & MUT3_BLANK_FAC) dummy -= 1;
				if (p_ptr->muta3 & MUT3_FLESH_ROT) dummy -= 1;
				if (p_ptr->muta3 & MUT3_SCALES) dummy -= 1;
				if (p_ptr->muta3 & MUT3_WART_SKIN) dummy -= 2;
				if (p_ptr->muta3 & MUT3_ILL_NORM) dummy = 0;
			}

			/* Boost */
			if (dummy)
			{
				/* Default */
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


		/* Sustain */
		if (have_flag(flgs, stat + TR_SUST_STR))
		{
			/* Dark green "s" */
			a = TERM_GREEN;
			c = 's';
		}


		/* Dump */
		Term_putch(col, row + stat+1, a, c);
	}
}


/*
 * Display the character on the screen (various modes)
 *
 * The top one and bottom two lines are left blank.
 *
 * Mode 0 = standard display with skills
 * Mode 1 = standard display with history
 * Mode 2 = summary of various things
 * Mode 3 = summary of various things (part 2)
 * Mode 4 = mutations
 */
void display_player(int mode)
{
	int i;

	char	buf[80];
	char	tmp[64];


	/* XXX XXX XXX */
	if ((p_ptr->muta1 || p_ptr->muta2 || p_ptr->muta3) && display_mutations)
		mode = (mode % 5);
	else
		mode = (mode % 4);

	/* Erase screen */
	clear_from(0);

	/* Standard */
	if ((mode == 0) || (mode == 1))
	{
		/* Name, Sex, Race, Class */
#ifdef JP
		sprintf(tmp, "%s%s%s", ap_ptr->title, ap_ptr->no == 1 ? "の":"", player_name);
#else
		sprintf(tmp, "%s %s", ap_ptr->title, player_name);
#endif

		display_player_one_line(ENTRY_NAME, tmp, TERM_L_BLUE);
		display_player_one_line(ENTRY_SEX, sp_ptr->title, TERM_L_BLUE);
		display_player_one_line(ENTRY_RACE, (p_ptr->mimic_form ? mimic_info[p_ptr->mimic_form].title : rp_ptr->title), TERM_L_BLUE);
		display_player_one_line(ENTRY_CLASS, cp_ptr->title, TERM_L_BLUE);

		if (p_ptr->realm1)
		{
			if (p_ptr->realm2)
				sprintf(tmp, "%s, %s", realm_names[p_ptr->realm1], realm_names[p_ptr->realm2]);
			else
				strcpy(tmp, realm_names[p_ptr->realm1]);
			display_player_one_line(ENTRY_REALM, tmp, TERM_L_BLUE);
		}

		if ((p_ptr->pclass == CLASS_CHAOS_WARRIOR) || (p_ptr->muta2 & MUT2_CHAOS_GIFT))
			display_player_one_line(ENTRY_PATRON, chaos_patrons[p_ptr->chaos_patron], TERM_L_BLUE);

		/* Age, Height, Weight, Social */
		/* 身長はセンチメートルに、体重はキログラムに変更してあります */
#ifdef JP
		display_player_one_line(ENTRY_AGE, format("%d才" ,(int)p_ptr->age), TERM_L_BLUE);
		display_player_one_line(ENTRY_HEIGHT, format("%dcm" ,(int)((p_ptr->ht*254)/100)), TERM_L_BLUE);
		display_player_one_line(ENTRY_WEIGHT, format("%dkg" ,(int)((p_ptr->wt*4536)/10000)), TERM_L_BLUE);
		display_player_one_line(ENTRY_SOCIAL, format("%d  " ,(int)p_ptr->sc), TERM_L_BLUE);
#else
		display_player_one_line(ENTRY_AGE, format("%d" ,(int)p_ptr->age), TERM_L_BLUE);
		display_player_one_line(ENTRY_HEIGHT, format("%d" ,(int)p_ptr->ht), TERM_L_BLUE);
		display_player_one_line(ENTRY_WEIGHT, format("%d" ,(int)p_ptr->wt), TERM_L_BLUE);
		display_player_one_line(ENTRY_SOCIAL, format("%d" ,(int)p_ptr->sc), TERM_L_BLUE);
#endif
		display_player_one_line(ENTRY_ALIGN, format("%s" ,your_alignment()), TERM_L_BLUE);


		/* Display the stats */
		for (i = 0; i < 6; i++)
		{
			/* Special treatment of "injured" stats */
			if (p_ptr->stat_cur[i] < p_ptr->stat_max[i])
			{
				int value;

				/* Use lowercase stat name */
				put_str(stat_names_reduced[i], 3 + i, 53);

				/* Get the current stat */
				value = p_ptr->stat_use[i];

				/* Obtain the current stat (modified) */
				cnv_stat(value, buf);

				/* Display the current stat (modified) */
				c_put_str(TERM_YELLOW, buf, 3 + i, 60);

				/* Acquire the max stat */
				value = p_ptr->stat_top[i];

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
				cnv_stat(p_ptr->stat_use[i], buf);

				/* Display the current stat (modified) */
				c_put_str(TERM_L_GREEN, buf, 3 + i, 60);
			}

			if (p_ptr->stat_max[i] == p_ptr->stat_max_max[i])
			{
#ifdef JP
				c_put_str(TERM_WHITE, "!", 3 + i, 58);
#else
				c_put_str(TERM_WHITE, "!", 3 + i, 58-2);
#endif
			}
		}

		/* Display "history" info */
		if (mode == 1)
		{
			char statmsg[1000];

#ifdef JP
			put_str("(キャラクターの生い立ち)", 11, 25);
#else
			put_str("(Character Background)", 11, 25);
#endif

			for (i = 0; i < 4; i++)
			{
				put_str(p_ptr->history[i], i + 12, 10);
			}

			*statmsg = '\0';

			if (p_ptr->is_dead)
			{
				if (p_ptr->total_winner)
				{
#ifdef JP
					sprintf(statmsg, "…あなたは勝利の後%sした。", streq(p_ptr->died_from, "Seppuku") ? "切腹" : "引退");
#else
					sprintf(statmsg, "...You %s after the winning.", streq(p_ptr->died_from, "Seppuku") ? "did Seppuku" : "retired from the adventure");
#endif
				}
				else if (!dun_level)
				{
#ifdef JP
					sprintf(statmsg, "…あなたは%sで%sに殺された。", map_name(), p_ptr->died_from);
#else
					sprintf(statmsg, "...You were killed by %s in %s.", p_ptr->died_from, map_name());
#endif
				}
				else if (p_ptr->inside_quest && is_fixed_quest_idx(p_ptr->inside_quest))
				{
					/* Get the quest text */
					/* Bewere that INIT_ASSIGN resets the cur_num. */
					init_flags = INIT_ASSIGN;

					process_dungeon_file("q_info.txt", 0, 0, 0, 0);

#ifdef JP
					sprintf(statmsg, "…あなたは、クエスト「%s」で%sに殺された。", quest[p_ptr->inside_quest].name, p_ptr->died_from);
#else
					sprintf(statmsg, "...You were killed by %s in the quest '%s'.", p_ptr->died_from, quest[p_ptr->inside_quest].name);
#endif
				}
				else
				{
#ifdef JP
					sprintf(statmsg, "…あなたは、%sの%d階で%sに殺された。", map_name(), dun_level, p_ptr->died_from);
#else
					sprintf(statmsg, "...You were killed by %s on level %d of %s.", p_ptr->died_from, dun_level, map_name());
#endif
				}
			}
			else if (character_dungeon)
			{
				if (!dun_level)
				{
#ifdef JP
					sprintf(statmsg, "…あなたは現在、 %s にいる。", map_name());
#else
					sprintf(statmsg, "...Now, you are in %s.", map_name());
#endif
				}
				else if (p_ptr->inside_quest && is_fixed_quest_idx(p_ptr->inside_quest))
				{
					/* Clear the text */
					/* Must be done before doing INIT_SHOW_TEXT */
					for (i = 0; i < 10; i++)
					{
						quest_text[i][0] = '\0';
					}
					quest_text_line = 0;

					/* Get the quest text */
					init_flags = INIT_SHOW_TEXT;

					process_dungeon_file("q_info.txt", 0, 0, 0, 0);

#ifdef JP
					sprintf(statmsg, "…あなたは現在、 クエスト「%s」を遂行中だ。", quest[p_ptr->inside_quest].name);
#else
					sprintf(statmsg, "...Now, you are in the quest '%s'.", quest[p_ptr->inside_quest].name);
#endif
				}
				else
				{
#ifdef JP
					sprintf(statmsg, "…あなたは現在、 %s の %d 階で探索している。", map_name(), dun_level);
#else
					sprintf(statmsg, "...Now, you are exploring level %d of %s.", dun_level, map_name());
#endif
				}
			}

			if (*statmsg)
			{
				char temp[64*2], *t;
				roff_to_buf(statmsg, 60, temp, sizeof(temp));
				t = temp;
				for(i=0 ; i<2 ; i++)
				{
					if(t[0]==0)
						break; 
					else
					{
						put_str(t, i + 5 + 12, 10);
						t += strlen(t)+1;
					}
				}
			}

		}

		/* Display "various" info */
		else
		{
			display_player_middle();
			display_player_various();
		}
	}

	/* Special */
	else if (mode == 2)
	{
		/* See "http://www.cs.berkeley.edu/~davidb/angband.html" */

		/* Dump the info */
		display_player_misc_info();
		display_player_stat_info();
		display_player_flag_info();
	}

	/* Special */
	else if (mode == 3)
	{
		display_player_other_flag_info();
	}

	else if (mode == 4)
	{
		do_cmd_knowledge_mutations();
	}
}


/*
 *
 */
static void dump_aux_display_player(FILE *fff)
{
	int x, y, i;
	byte a;
	char c;
	char		buf[1024];

	/* Display player */
	display_player(0);

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
		while ((x > 0) && (buf[x-1] == ' ')) buf[--x] = '\0';

		/* End the row */
#ifdef JP
		fprintf(fff, "%s\n", buf);
#else
		fprintf(fff, "%s\n", buf);
#endif

	}

	/* Display history */
	display_player(1);

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
		while ((x > 0) && (buf[x-1] == ' ')) buf[--x] = '\0';

		/* End the row */
		fprintf(fff, "%s\n", buf);
	}

	fprintf(fff, "\n");

	/* Display flags (part 1) */
	display_player(2);

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
		while ((x > 0) && (buf[x-1] == ' ')) buf[--x] = '\0';

		/* End the row */
		fprintf(fff, "%s\n", buf);
	}

	fprintf(fff, "\n");

	/* Display flags (part 2) */
	display_player(3);

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
		while ((x > 0) && (buf[x-1] == ' ')) buf[--x] = '\0';

		/* End the row */
		fprintf(fff, "%s\n", buf);
	}

	fprintf(fff, "\n");
}


/*
 *
 */
static void dump_aux_pet(FILE *fff)
{
	int i;
	bool pet = FALSE;
	bool pet_settings = FALSE;
	char pet_name[80];

	for (i = m_max - 1; i >= 1; i--)
	{
		monster_type *m_ptr = &m_list[i];

		if (!m_ptr->r_idx) continue;
		if (!is_pet(m_ptr)) continue;
		pet_settings = TRUE;
		if (!m_ptr->nickname && (p_ptr->riding != i)) continue;
		if (!pet)
		{
#ifdef JP
			fprintf(fff, "\n\n  [主なペット]\n\n");
#else
			fprintf(fff, "\n\n  [Leading Pets]\n\n");
#endif
			pet = TRUE;
		}
		monster_desc(pet_name, m_ptr, MD_ASSUME_VISIBLE | MD_INDEF_VISIBLE);
		fprintf(fff, "%s\n", pet_name);
	}

	if (pet_settings)
	{
#ifdef JP
		fprintf(fff, "\n\n  [ペットへの命令]\n");
#else
		fprintf(fff, "\n\n  [Command for Pets]\n");
#endif

#ifdef JP
		fprintf(fff, "\n ドアを開ける:                       %s", (p_ptr->pet_extra_flags & PF_OPEN_DOORS) ? "ON" : "OFF");
#else
		fprintf(fff, "\n Pets open doors:                    %s", (p_ptr->pet_extra_flags & PF_OPEN_DOORS) ? "ON" : "OFF");
#endif

#ifdef JP
		fprintf(fff, "\n アイテムを拾う:                     %s", (p_ptr->pet_extra_flags & PF_PICKUP_ITEMS) ? "ON" : "OFF");
#else
		fprintf(fff, "\n Pets pick up items:                 %s", (p_ptr->pet_extra_flags & PF_PICKUP_ITEMS) ? "ON" : "OFF");
#endif

#ifdef JP
		fprintf(fff, "\n テレポート系魔法を使う:             %s", (p_ptr->pet_extra_flags & PF_TELEPORT) ? "ON" : "OFF");
#else
		fprintf(fff, "\n Allow teleport:                     %s", (p_ptr->pet_extra_flags & PF_TELEPORT) ? "ON" : "OFF");
#endif

#ifdef JP
		fprintf(fff, "\n 攻撃魔法を使う:                     %s", (p_ptr->pet_extra_flags & PF_ATTACK_SPELL) ? "ON" : "OFF");
#else
		fprintf(fff, "\n Allow cast attack spell:            %s", (p_ptr->pet_extra_flags & PF_ATTACK_SPELL) ? "ON" : "OFF");
#endif

#ifdef JP
		fprintf(fff, "\n 召喚魔法を使う:                     %s", (p_ptr->pet_extra_flags & PF_SUMMON_SPELL) ? "ON" : "OFF");
#else
		fprintf(fff, "\n Allow cast summon spell:            %s", (p_ptr->pet_extra_flags & PF_SUMMON_SPELL) ? "ON" : "OFF");
#endif

#ifdef JP
		fprintf(fff, "\n プレイヤーを巻き込む範囲魔法を使う: %s", (p_ptr->pet_extra_flags & PF_BALL_SPELL) ? "ON" : "OFF");
#else
		fprintf(fff, "\n Allow involve player in area spell: %s", (p_ptr->pet_extra_flags & PF_BALL_SPELL) ? "ON" : "OFF");
#endif

		fputc('\n', fff);
	}
}


/*
 *
 */
static void dump_aux_class_special(FILE *fff)
{
	if (p_ptr->pclass == CLASS_BLUE_MAGE)
	{
		int i = 0;
		int j = 0;
		int l1 = 0;
		int l2 = 0;
		int num = 0;
		int spellnum[MAX_MONSPELLS];
		s32b f4 = 0, f5 = 0, f6 = 0;
		char p[60][80];
		int col = 0;
		bool pcol = FALSE;

		for (i=0;i<60;i++) { p[i][0] = '\0'; }

#ifdef JP
		strcat(p[col], "\n\n  [学習済みの青魔法]\n");
#else
		strcat(p[col], "\n\n  [Learned Blue Magic]\n");
#endif


		for (j=1;j<6;j++)
		{
			col++;
			set_rf_masks(&f4, &f5, &f6, j);
			switch(j)
			{
				case MONSPELL_TYPE_BOLT:
#ifdef JP
					strcat(p[col], "\n     [ボルト型]\n");
#else
					strcat(p[col], "\n     [Bolt  Type]\n");
#endif
					break;

				case MONSPELL_TYPE_BALL:
#ifdef JP
					strcat(p[col], "\n     [ボール型]\n");
#else
					strcat(p[col], "\n     [Ball  Type]\n");
#endif
					break;

				case MONSPELL_TYPE_BREATH:
#ifdef JP
					strcat(p[col], "\n     [ブレス型]\n");
#else
					strcat(p[col], "\n     [  Breath  ]\n");
#endif
					break;

				case MONSPELL_TYPE_SUMMON:
#ifdef JP
					strcat(p[col], "\n     [召喚魔法]\n");
#else
					strcat(p[col], "\n     [Summonning]\n");
#endif
					break;

				case MONSPELL_TYPE_OTHER:
#ifdef JP
					strcat(p[col], "\n     [ その他 ]\n");
#else
					strcat(p[col], "\n     [Other Type]\n");
#endif
					break;
			}

			for (i = 0, num = 0; i < 32; i++)
			{
				if ((0x00000001 << i) & f4) spellnum[num++] = i;
			}
			for (; i < 64; i++)
			{
				if ((0x00000001 << (i - 32)) & f5) spellnum[num++] = i;
			}
			for (; i < 96; i++)
			{
				if ((0x00000001 << (i - 64)) & f6) spellnum[num++] = i;
			}

			col++;
			pcol = FALSE;
			strcat(p[col], "       ");

			for (i = 0; i < num; i++)
			{
				if (p_ptr->magic_num2[spellnum[i]])
				{
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
			}
			
			if (!pcol)
			{
#ifdef JP
				strcat(p[col], "なし");
#else
				strcat(p[col], "None");
#endif
			}
			else
			{
				if (p[col][strlen(p[col])-2] == ',')
				{
					p[col][strlen(p[col])-2] = '\0';
				}
				else
				{
					p[col][strlen(p[col])-10] = '\0';
				}
			}
			
			strcat(p[col], "\n");
		}

		for (i=0;i<=col;i++)
		{
			fprintf(fff, p[i]);
		}
	}
	else if (p_ptr->pclass == CLASS_MAGIC_EATER)
	{
		char s[EATER_EXT][MAX_NLEN];
		int tval, ext, k_idx;
		int i, magic_num;

#ifdef JP
		fprintf(fff, "\n\n  [取り込んだ魔法道具]\n");
#else
		fprintf(fff, "\n\n  [Magic devices eaten]\n");
#endif

		for (ext = 0; ext < 3; ext++)
		{
			int eat_num = 0;

			/* Dump an extent name */
			switch (ext)
			{
			case 0:
				tval = TV_STAFF;
#ifdef JP
				fprintf(fff, "\n[杖]\n");
#else
				fprintf(fff, "\n[Staffs]\n");
#endif
				break;
			case 1:
				tval = TV_WAND;
#ifdef JP
				fprintf(fff, "\n[魔法棒]\n");
#else
				fprintf(fff, "\n[Wands]\n");
#endif
				break;
			case 2:
				tval = TV_ROD;
#ifdef JP
				fprintf(fff, "\n[ロッド]\n");
#else
				fprintf(fff, "\n[Rods]\n");
#endif
				break;
			}

			/* Get magic device names that were eaten */
			for (i = 0; i < EATER_EXT; i++)
			{
				int idx = EATER_EXT * ext + i;

				magic_num = p_ptr->magic_num2[idx];
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
#ifdef JP
				fputs("  (なし)\n", fff);
#else
				fputs("  (none)\n", fff);
#endif
			}
		}
	}
}


/*
 *
 */
static void dump_aux_quest(FILE *fff)
{
	int i;
	int *quest_num;
	int dummy;


#ifdef JP
	fprintf(fff, "\n\n  [クエスト情報]\n");
#else
	fprintf(fff, "\n\n  [Quest Information]\n");
#endif

	/* Allocate Memory */
	C_MAKE(quest_num, max_quests, int);

	/* Sort by compete level */
	for (i = 1; i < max_quests; i++) quest_num[i] = i;
	ang_sort_comp = ang_sort_comp_quest_num;
	ang_sort_swap = ang_sort_swap_quest_num;
	ang_sort(quest_num, &dummy, max_quests);

	/* Dump Quest Information */
	fputc('\n', fff);
	do_cmd_knowledge_quests_completed(fff, quest_num);
	fputc('\n', fff);
	do_cmd_knowledge_quests_failed(fff, quest_num);
	fputc('\n', fff);

	/* Free Memory */
	C_KILL(quest_num, max_quests, int);
}


/*
 *
 */
static void dump_aux_last_message(FILE *fff)
{
	if (p_ptr->is_dead)
	{
		if (!p_ptr->total_winner)
		{
			int i;

#ifdef JP
			fprintf(fff, "\n  [死ぬ直前のメッセージ]\n\n");
#else
			fprintf(fff, "\n  [Last Messages]\n\n");
#endif
			for (i = MIN(message_num(), 30); i >= 0; i--)
			{
				fprintf(fff,"> %s\n",message_str((s16b)i));
			}
			fputc('\n', fff);
		}

		/* Hack -- *Winning* message */
		else if (p_ptr->last_message)
		{
#ifdef JP
			fprintf(fff, "\n  [*勝利*メッセージ]\n\n");
#else
			fprintf(fff, "\n  [*Winning* Message]\n\n");
#endif
			fprintf(fff,"  %s\n", p_ptr->last_message);
			fputc('\n', fff);
		}
	}
}


/*
 *
 */
static void dump_aux_recall(FILE *fff)
{
	int y;

#ifdef JP
	fprintf(fff, "\n  [帰還場所]\n\n");
#else
	fprintf(fff, "\n  [Recall Depth]\n\n");
#endif

	for (y = 1; y < max_d_idx; y++)
	{
		bool seiha = FALSE;

		if (!d_info[y].maxdepth) continue;
		if (!max_dlv[y]) continue;
		if (d_info[y].final_guardian)
		{
			if (!r_info[d_info[y].final_guardian].max_num) seiha = TRUE;
		}
		else if (max_dlv[y] == d_info[y].maxdepth) seiha = TRUE;

#ifdef JP
		fprintf(fff, "   %c%-12s: %3d 階\n", seiha ? '!' : ' ', d_name+d_info[y].name, max_dlv[y]);
#else
		fprintf(fff, "   %c%-16s: level %3d\n", seiha ? '!' : ' ', d_name+d_info[y].name, max_dlv[y]);
#endif
	}
}


/*
 *
 */
static void dump_aux_options(FILE *fff)
{
#ifdef JP
	fprintf(fff, "\n  [オプション設定]\n");
#else
	fprintf(fff, "\n  [Option Settings]\n");
#endif


	if (preserve_mode)
#ifdef JP
		fprintf(fff, "\n 保存モード:         ON");
#else
		fprintf(fff, "\n Preserve Mode:      ON");
#endif

	else
#ifdef JP
		fprintf(fff, "\n 保存モード:         OFF");
#else
		fprintf(fff, "\n Preserve Mode:      OFF");
#endif


	if (ironman_small_levels)
#ifdef JP
		fprintf(fff, "\n 小さいダンジョン:   ALWAYS");
#else
		fprintf(fff, "\n Small Levels:       ALWAYS");
#endif

	else if (always_small_levels)
#ifdef JP
		fprintf(fff, "\n 小さいダンジョン:   ON");
#else
		fprintf(fff, "\n Small Levels:       ON");
#endif

	else if (small_levels)
#ifdef JP
		fprintf(fff, "\n 小さいダンジョン:   ENABLED");
#else
		fprintf(fff, "\n Small Levels:       ENABLED");
#endif

	else
#ifdef JP
		fprintf(fff, "\n 小さいダンジョン:   OFF");
#else
		fprintf(fff, "\n Small Levels:       OFF");
#endif


	if (vanilla_town)
#ifdef JP
		fprintf(fff, "\n 元祖の町のみ:       ON");
#else
		fprintf(fff, "\n Vanilla Town:       ON");
#endif

	else if (lite_town)
#ifdef JP
		fprintf(fff, "\n 小規模な町:         ON");
#else
		fprintf(fff, "\n Lite Town:          ON");
#endif


	if (ironman_shops)
#ifdef JP
		fprintf(fff, "\n 店なし:             ON");
#else
		fprintf(fff, "\n No Shops:           ON");
#endif


	if (ironman_downward)
#ifdef JP
		fprintf(fff, "\n 階段を上がれない:   ON");
#else
		fprintf(fff, "\n Diving Only:        ON");
#endif


	if (ironman_rooms)
#ifdef JP
		fprintf(fff, "\n 普通でない部屋:     ON");
#else
		fprintf(fff, "\n Unusual Rooms:      ON");
#endif


	if (ironman_nightmare)
#ifdef JP
		fprintf(fff, "\n 悪夢モード:         ON");
#else
		fprintf(fff, "\n Nightmare Mode:     ON");
#endif


	if (ironman_empty_levels)
#ifdef JP
		fprintf(fff, "\n アリーナ:           ALWAYS");
#else
		fprintf(fff, "\n Arena Levels:       ALWAYS");
#endif

	else if (empty_levels)
#ifdef JP
		fprintf(fff, "\n アリーナ:           ENABLED");
#else
		fprintf(fff, "\n Arena Levels:       ENABLED");
#endif

	else
#ifdef JP
		fprintf(fff, "\n アリーナ:           OFF");
#else
		fprintf(fff, "\n Arena Levels:       OFF");
#endif


	fputc('\n', fff);

	if (p_ptr->noscore)
#ifdef JP
		fprintf(fff, "\n 何か不正なことをしてしまっています。\n");
#else
		fprintf(fff, "\n You have done something illegal.\n");
#endif

	fputc('\n', fff);
}


/*
 *
 */
static void dump_aux_arena(FILE *fff)
{
	if (lite_town || vanilla_town) return;

	if (p_ptr->arena_number < 0)
	{
		if (p_ptr->arena_number <= ARENA_DEFEATED_OLD_VER)
		{
#ifdef JP
			fprintf(fff, "\n 闘技場: 敗北\n");
#else
			fprintf(fff, "\n Arena: Defeated\n");
#endif
		}
		else
		{
#ifdef JP
			fprintf(fff, "\n 闘技場: %d回戦で%sの前に敗北\n", -p_ptr->arena_number,
				r_name + r_info[arena_info[-1 - p_ptr->arena_number].r_idx].name);
#else
			fprintf(fff, "\n Arena: Defeated by %s in the %d%s fight\n",
				r_name + r_info[arena_info[-1 - p_ptr->arena_number].r_idx].name,
				-p_ptr->arena_number, get_ordinal_number_suffix(-p_ptr->arena_number));
#endif
		}
	}
	else if (p_ptr->arena_number > MAX_ARENA_MONS + 2)
	{
#ifdef JP
		fprintf(fff, "\n 闘技場: 真のチャンピオン\n");
#else
		fprintf(fff, "\n Arena: True Champion\n");
#endif
	}
	else if (p_ptr->arena_number > MAX_ARENA_MONS - 1)
	{
#ifdef JP
		fprintf(fff, "\n 闘技場: チャンピオン\n");
#else
		fprintf(fff, "\n Arena: Champion\n");
#endif
	}
	else
	{
#ifdef JP
		fprintf(fff, "\n 闘技場: %2d勝\n", (p_ptr->arena_number > MAX_ARENA_MONS ? MAX_ARENA_MONS : p_ptr->arena_number));
#else
		fprintf(fff, "\n Arena: %2d Victor%s\n", (p_ptr->arena_number > MAX_ARENA_MONS ? MAX_ARENA_MONS : p_ptr->arena_number), (p_ptr->arena_number > 1) ? "ies" : "y");
#endif
	}

	fprintf(fff, "\n");
}


/*
 *
 */
static void dump_aux_monsters(FILE *fff)
{
	/* Monsters slain */

	int k;
	long uniq_total = 0;
	long norm_total = 0;
	s16b *who;

	/* Sort by monster level */
	u16b why = 2;

#ifdef JP
	fprintf(fff, "\n  [倒したモンスター]\n\n");
#else
	fprintf(fff, "\n  [Defeated Monsters]\n\n");
#endif

	/* Allocate the "who" array */
	C_MAKE(who, max_r_idx, s16b);

	/* Count monster kills */
	for (k = 1; k < max_r_idx; k++)
	{
		monster_race *r_ptr = &r_info[k];

		/* Ignore unused index */
 		if (!r_ptr->name) continue;

		/* Unique monsters */
		if (r_ptr->flags1 & RF1_UNIQUE)
		{
			bool dead = (r_ptr->max_num == 0);
			if (dead)
			{
				norm_total++;

				/* Add a unique monster to the list */
				who[uniq_total++] = k;
			}
		}

		/* Normal monsters */
		else
		{
			if (r_ptr->r_pkills > 0)
			{
				norm_total += r_ptr->r_pkills;
			}
		}
	}


	/* No monsters is defeated */
	if (norm_total < 1)
	{
#ifdef JP
		fprintf(fff,"まだ敵を倒していません。\n");
#else
		fprintf(fff,"You have defeated no enemies yet.\n");
#endif
	}

	/* Defeated more than one normal monsters */
	else if (uniq_total == 0)
	{
#ifdef JP
		fprintf(fff,"%ld体の敵を倒しています。\n", norm_total);
#else
		fprintf(fff,"You have defeated %ld %s.\n", norm_total, norm_total == 1 ? "enemy" : "enemies");
#endif
	}

	/* Defeated more than one unique monsters */
	else /* if (uniq_total > 0) */
	{
#ifdef JP
		fprintf(fff, "%ld体のユニーク・モンスターを含む、合計%ld体の敵を倒しています。\n", uniq_total, norm_total); 
#else
		fprintf(fff, "You have defeated %ld %s including %ld unique monster%s in total.\n", norm_total, norm_total == 1 ? "enemy" : "enemies", uniq_total, (uniq_total == 1 ? "" : "s"));
#endif


		/* Select the sort method */
		ang_sort_comp = ang_sort_comp_hook;
		ang_sort_swap = ang_sort_swap_hook;

		/* Sort the array by dungeon depth of monsters */
		ang_sort(who, &why, uniq_total);

#ifdef JP
		fprintf(fff, "\n《上位%ld体のユニーク・モンスター》\n", MIN(uniq_total, 10));
#else
		fprintf(fff, "\n< Unique monsters top %ld >\n", MIN(uniq_total, 10));
#endif

		/* Print top 10 */
		for (k = uniq_total - 1; k >= 0 && k >= uniq_total - 10; k--)
		{
			monster_race *r_ptr = &r_info[who[k]];

#ifdef JP
			fprintf(fff, "  %-40s (レベル%3d)\n", (r_name + r_ptr->name), r_ptr->level); 
#else
			fprintf(fff, "  %-40s (level %3d)\n", (r_name + r_ptr->name), r_ptr->level); 
#endif
		}

	}

	/* Free the "who" array */
	C_KILL(who, max_r_idx, s16b);
}


/*
 *
 */
static void dump_aux_race_history(FILE *fff)
{
	if (p_ptr->old_race1 || p_ptr->old_race2)
	{
		int i;

#ifdef JP
		fprintf(fff, "\n\n あなたは%sとして生まれた。", race_info[p_ptr->start_race].title);
#else
		fprintf(fff, "\n\n You were born as %s.", race_info[p_ptr->start_race].title);
#endif
		for (i = 0; i < MAX_RACES; i++)
		{
			if (p_ptr->start_race == i) continue;
			if (i < 32)
			{
				if (!(p_ptr->old_race1 & 1L << i)) continue;
			}
			else
			{
				if (!(p_ptr->old_race2 & 1L << (i-32))) continue;
			}
#ifdef JP
			fprintf(fff, "\n あなたはかつて%sだった。", race_info[i].title);
#else
			fprintf(fff, "\n You were a %s before.", race_info[i].title);
#endif
		}

		fputc('\n', fff);
	}
}


/*
 *
 */
static void dump_aux_realm_history(FILE *fff)
{
	if (p_ptr->old_realm)
	{
		int i;

		fputc('\n', fff);
		for (i = 0; i < MAX_MAGIC; i++)
		{
			if (!(p_ptr->old_realm & 1L << i)) continue;
#ifdef JP
			fprintf(fff, "\n あなたはかつて%s魔法を使えた。", realm_names[i+1]);
#else
			fprintf(fff, "\n You were able to use %s magic before.", realm_names[i+1]);
#endif
		}
		fputc('\n', fff);
	}
}


/*
 *
 */
static void dump_aux_virtues(FILE *fff)
{
#ifdef JP
	fprintf(fff, "\n\n  [プレイヤーの徳]\n\n");
#else
	fprintf(fff, "\n\n  [Virtues]\n\n");
#endif

#ifdef JP
	fprintf(fff, "属性 : %s\n", your_alignment());
#else
	fprintf(fff, "Your alighnment : %s\n", your_alignment());
#endif

	fprintf(fff, "\n");
	dump_virtues(fff);
}


/*
 *
 */
static void dump_aux_mutations(FILE *fff)
{
	if (p_ptr->muta1 || p_ptr->muta2 || p_ptr->muta3)
	{
#ifdef JP
		fprintf(fff, "\n\n  [突然変異]\n\n");
#else
		fprintf(fff, "\n\n  [Mutations]\n\n");
#endif

		dump_mutations(fff);
	}
}


/*
 *
 */
static void dump_aux_equipment_inventory(FILE *fff)
{
	int i;
	char o_name[MAX_NLEN];

	/* Dump the equipment */
	if (equip_cnt)
	{
#ifdef JP
		fprintf(fff, "  [キャラクタの装備]\n\n");
#else
		fprintf(fff, "  [Character Equipment]\n\n");
#endif

		for (i = INVEN_RARM; i < INVEN_TOTAL; i++)
		{
			object_desc(o_name, &inventory[i], 0);
			if ((((i == INVEN_RARM) && p_ptr->hidarite) || ((i == INVEN_LARM) && p_ptr->migite)) && p_ptr->ryoute)
#ifdef JP
				strcpy(o_name, "(武器を両手持ち)");
#else
				strcpy(o_name, "(wielding with two-hands)");
#endif

			fprintf(fff, "%c) %s\n",
				index_to_label(i), o_name);
		}
		fprintf(fff, "\n\n");
	}

	/* Dump the inventory */
#ifdef JP
	fprintf(fff, "  [キャラクタの持ち物]\n\n");
#else
	fprintf(fff, "  [Character Inventory]\n\n");
#endif

	for (i = 0; i < INVEN_PACK; i++)
	{
		/* Don't dump the empty slots */
		if (!inventory[i].k_idx) break;

		/* Dump the inventory slots */
		object_desc(o_name, &inventory[i], 0);
		fprintf(fff, "%c) %s\n", index_to_label(i), o_name);
	}

	/* Add an empty line */
	fprintf(fff, "\n\n");
}


/*
 *
 */
static void dump_aux_home_museum(FILE *fff)
{
	char o_name[MAX_NLEN];
	store_type  *st_ptr;

	/* Do we need it?? */
	/* process_dungeon_file("w_info.txt", 0, 0, max_wild_y, max_wild_x); */

	/* Print the home */
	st_ptr = &town[1].store[STORE_HOME];

	/* Home -- if anything there */
	if (st_ptr->stock_num)
	{
		int i;
		int x = 1;

#ifdef JP
		fprintf(fff, "  [我が家のアイテム]\n");
#else
		fprintf(fff, "  [Home Inventory]\n");
#endif

		/* Dump all available items */
		for (i = 0; i < st_ptr->stock_num; i++)
		{
			if ((i % 12) == 0)
#ifdef JP
				fprintf(fff, "\n ( %d ページ )\n", x++);
#else
				fprintf(fff, "\n ( page %d )\n", x++);
#endif
			object_desc(o_name, &st_ptr->stock[i], 0);
			fprintf(fff, "%c) %s\n", I2A(i%12), o_name);
		}

		/* Add an empty line */
		fprintf(fff, "\n\n");
	}


	/* Print the home */
	st_ptr = &town[1].store[STORE_MUSEUM];

	/* Home -- if anything there */
	if (st_ptr->stock_num)
	{
		int i;
		int x = 1;

#ifdef JP
		fprintf(fff, "  [博物館のアイテム]\n");
#else
		fprintf(fff, "  [Museum]\n");
#endif

		/* Dump all available items */
		for (i = 0; i < st_ptr->stock_num; i++)
		{
#ifdef JP
		if ((i % 12) == 0) fprintf(fff, "\n ( %d ページ )\n", x++);
			object_desc(o_name, &st_ptr->stock[i], 0);
			fprintf(fff, "%c) %s\n", I2A(i%12), o_name);
#else
		if ((i % 12) == 0) fprintf(fff, "\n ( page %d )\n", x++);
			object_desc(o_name, &st_ptr->stock[i], 0);
			fprintf(fff, "%c) %s\n", I2A(i%12), o_name);
#endif

		}

		/* Add an empty line */
		fprintf(fff, "\n\n");
	}
}


/*
 * Output the character dump to a file
 */
errr make_character_dump(FILE *fff)
{
#ifdef JP
	fprintf(fff, "  [変愚蛮怒 %d.%d.%d キャラクタ情報]\n\n",
		FAKE_VER_MAJOR-10, FAKE_VER_MINOR, FAKE_VER_PATCH);
#else
	fprintf(fff, "  [Hengband %d.%d.%d Character Dump]\n\n",
		FAKE_VER_MAJOR-10, FAKE_VER_MINOR, FAKE_VER_PATCH);
#endif

	update_playtime();

	dump_aux_display_player(fff);
	dump_aux_last_message(fff);
	dump_aux_options(fff);
	dump_aux_recall(fff);
	dump_aux_quest(fff);
	dump_aux_arena(fff);
	dump_aux_monsters(fff);
	dump_aux_virtues(fff);
	dump_aux_race_history(fff);
	dump_aux_realm_history(fff);
	dump_aux_class_special(fff);
	dump_aux_mutations(fff);
	dump_aux_pet(fff);
	fputs("\n\n", fff);
	dump_aux_equipment_inventory(fff);
	dump_aux_home_museum(fff);

#ifdef JP
	fprintf(fff, "  [チェックサム: \"%s\"]\n\n", get_check_sum());
#else
	fprintf(fff, "  [Check Sum: \"%s\"]\n\n", get_check_sum());
#endif

	return 0;
}

/*
 * Hack -- Dump a character description file
 *
 * XXX XXX XXX Allow the "full" flag to dump additional info,
 * and trigger its usage from various places in the code.
 */
errr file_character(cptr name)
{
	int		fd = -1;
	FILE		*fff = NULL;
	char		buf[1024];

	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, name);

	/* File type is "TEXT" */
	FILE_TYPE(FILE_TYPE_TEXT);

	/* Check for existing file */
	fd = fd_open(buf, O_RDONLY);

	/* Existing file */
	if (fd >= 0)
	{
		char out_val[160];

		/* Close the file */
		(void)fd_close(fd);

		/* Build query */
#ifdef JP
		(void)sprintf(out_val, "現存するファイル %s に上書きしますか? ", buf);
#else
		(void)sprintf(out_val, "Replace existing file %s? ", buf);
#endif


		/* Ask */
		if (get_check_strict(out_val, CHECK_NO_HISTORY)) fd = -1;
	}

	/* Open the non-existing file */
	if (fd < 0) fff = my_fopen(buf, "w");

	/* Invalid file */
	if (!fff)
	{
		/* Message */
#ifdef JP
		prt("キャラクタ情報のファイルへの書き出しに失敗しました！", 0, 0);
#else
		prt("Character dump failed!", 0, 0);
#endif

		(void)inkey();

		/* Error */
		return (-1);
	}

	(void)make_character_dump(fff);

	/* Close it */
	my_fclose(fff);


	/* Message */
#ifdef JP
msg_print("キャラクタ情報のファイルへの書き出しに成功しました。");
#else
	msg_print("Character dump successful.");
#endif

	msg_print(NULL);

	/* Success */
	return (0);
}


/*
 * Display single line of on-line help file
 *
 * You can insert some special color tag to change text color.
 * Such as...
 * WHITETEXT [[[[y|SOME TEXT WHICH IS DISPLAYED IN YELLOW| WHITETEXT
 *
 * A colored segment is between "[[[[y|" and the last "|".
 * You can use any single character in place of the "|".
 */
static void show_file_aux_line(cptr str, int cy, cptr shower)
{
	static const char tag_str[] = "[[[[";
	byte color = TERM_WHITE;
	char in_tag = '\0';
	int cx = 0;
	int i;
	char lcstr[1024];

	if (shower)
	{
		/* Make a lower case version of str for searching */
		strcpy(lcstr, str);
		str_tolower(lcstr);
	}

	/* Initial cursor position */
	Term_gotoxy(cx, cy);

	for (i = 0; str[i];)
	{
		int len = strlen(&str[i]);
		int showercol = len + 1;
		int bracketcol = len + 1;
		int endcol = len;
		cptr ptr;

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

		/* Shower string? */
		if (endcol == showercol)
		{
			int showerlen = strlen(shower);

			/* Print the shower string in yellow */
			Term_addstr(showerlen, TERM_YELLOW, &str[i]);
			cx += showerlen;
			i += showerlen;
		}

		/* Colored segment? */
		else if (endcol == bracketcol)
		{
			if (in_tag)
			{
				/* Found the end of colored segment */
				i++;

				/* Now looking for an another tag_str */
				in_tag = '\0';

				/* Set back to the default color */
				color = TERM_WHITE;
			}
			else
			{
				/* Found a tag_str, and get a tag color */
				i += sizeof(tag_str)-1;

				/* Get tag color */
				color = color_char_to_attr(str[i]);

				/* Illegal color tag */
				if (color == 255 || str[i+1] == '\0')
				{
					/* Illegal color tag */
					color = TERM_WHITE;

					/* Print the broken tag as a string */
					Term_addstr(-1, TERM_WHITE, tag_str);
					cx += sizeof(tag_str)-1;
				}
				else
				{
					/* Skip the color tag */
					i++;

					/* Now looking for a close tag */
					in_tag = str[i];

					/* Skip the close-tag-indicator */
					i++;
				}
			}
		}

	} /* for (i = 0; str[i];) */

	/* Clear rest of line */
	Term_erase(cx, cy, 255);
}


/*
 * Recursive file perusal.
 *
 * Process various special text in the input file, including
 * the "menu" structures used by the "help file" system.
 *
 * Return FALSE on 'q' to exit from a deep, otherwise TRUE.
 */
bool show_file(bool show_version, cptr name, cptr what, int line, int mode)
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
	cptr find = NULL;

	/* Jump to this tag */
	cptr tag = NULL;

	/* Hold strings to find/show */
	char finder_str[81];
	char shower_str[81];
	char back_str[81];

	/* String to show */
	cptr shower = NULL;

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

	/* Wipe finder */
	strcpy(finder_str, "");

	/* Wipe shower */
	strcpy(shower_str, "");

	/* Wipe caption */
	strcpy(caption, "");

	/* Wipe the hooks */
	for (i = 0; i < 68; i++)
	{
		hook[i][0] = '\0';
	}

	/* Copy the filename */
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

	/* Hack XXX XXX XXX */
	if (what)
	{
		/* Caption */
		strcpy(caption, what);

		/* Access the "file" */
		strcpy(path, name);

		/* Open */
		fff = my_fopen(path, "r");
	}

	/* Look in "help" */
	if (!fff)
	{
		/* Caption */
#ifdef JP
sprintf(caption, "ヘルプ・ファイル'%s'", name);
#else
		sprintf(caption, "Help file '%s'", name);
#endif


		/* Build the filename */
		path_build(path, sizeof(path), ANGBAND_DIR_HELP, name);

		/* Open the file */
		fff = my_fopen(path, "r");
	}

	/* Look in "info" */
	if (!fff)
	{
		/* Caption */
#ifdef JP
sprintf(caption, "スポイラー・ファイル'%s'", name);
#else
		sprintf(caption, "Info file '%s'", name);
#endif


		/* Build the filename */
		path_build(path, sizeof(path), ANGBAND_DIR_INFO, name);

		/* Open the file */
		fff = my_fopen(path, "r");
	}

	/* Look in "info" */
	if (!fff)
	{
		/* Build the filename */
		path_build(path, sizeof(path), ANGBAND_DIR, name);

		for (i = 0; path[i]; i++)
			if ('\\' == path[i])
				path[i] = PATH_SEP[0];

		/* Caption */
#ifdef JP
sprintf(caption, "スポイラー・ファイル'%s'", name);
#else
		sprintf(caption, "Info file '%s'", name);
#endif

		/* Open the file */
		fff = my_fopen(path, "r");
	}

	/* Oops */
	if (!fff)
	{
		/* Message */
#ifdef JP
msg_format("'%s'をオープンできません。", name);
#else
		msg_format("Cannot open '%s'.", name);
#endif

		msg_print(NULL);

		/* Oops */
		return (TRUE);
	}


	/* Pre-Parse the file */
	while (TRUE)
	{
		char *str = buf;

		/* Read a line or stop */
		if (my_fgets(fff, buf, sizeof(buf))) break;

		/* XXX Parse "menu" items */
		if (prefix(str, "***** "))
		{
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
			}
			/* Notice "tag" requests */
			else if (str[6] == '<')
			{
				size_t len = strlen(str);

				if (str[len - 1] == '>')
				{
					str[len - 1] = '\0';
					if (tag && streq(str + 7, tag)) line = next;
				}
			}

			/* Skip this */
			continue;
		}

		/* Count the "real" lines */
		next++;
	}

	/* Save the number of "real" lines */
	size = next;

	/* start from bottom when reverse mode */
	if (line == -1) line = ((size-1)/rows)*rows;

	/* Clear screen */
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
			/* Close it */
			my_fclose(fff);

			/* Hack -- Re-Open the file */
			fff = my_fopen(path, "r");

			/* Oops */
			if (!fff) return (FALSE);

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
			cptr str = buf;

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
			prt(format(
#ifdef JP
				"[変愚蛮怒 %d.%d.%d, %s, %d/%d]",
#else
				"[Hengband %d.%d.%d, %s, Line %d/%d]",
#endif

			   FAKE_VER_MAJOR-10, FAKE_VER_MINOR, FAKE_VER_PATCH,
			   caption, line, size), 0, 0);
		}
		else
		{
			prt(format(
#ifdef JP
				"[%s, %d/%d]",
#else
				"[%s, Line %d/%d]",
#endif
				caption, line, size), 0, 0);
		}

		/* Prompt -- small files */
		if (size <= rows)
		{
			/* Wait for it */
#ifdef JP
prt("[キー:(?)ヘルプ (ESC)終了]", hgt - 1, 0);
#else
			prt("[Press ESC to exit.]", hgt - 1, 0);
#endif

		}

		/* Prompt -- large files */
		else
		{
#ifdef JP
			if(reverse)
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
#ifdef JP
			if (strcmp(name, "jhelpinfo.txt") != 0)
				show_file(TRUE, "jhelpinfo.txt", NULL, 0, mode);
#else
			if (strcmp(name, "helpinfo.txt") != 0)
				show_file(TRUE, "helpinfo.txt", NULL, 0, mode);
#endif
			break;

		/* Hack -- try showing */
		case '=':
			/* Get "shower" */
#ifdef JP
			prt("強調: ", hgt - 1, 0);
#else
			prt("Show: ", hgt - 1, 0);
#endif

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
#ifdef JP
			prt("検索: ", hgt - 1, 0);
#else
			prt("Find: ", hgt - 1, 0);
#endif

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
#ifdef JP
				prt("行: ", hgt - 1, 0);
#else
				prt("Goto Line: ", hgt - 1, 0);
#endif

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
#ifdef JP
				prt("ファイル・ネーム: ", hgt - 1, 0);
				strcpy(tmp, "jhelp.hlp");
#else
				prt("Goto File: ", hgt - 1, 0);
				strcpy(tmp, "help.hlp");
#endif

				if (askfor(tmp, 80))
				{
					if (!show_file(TRUE, tmp, NULL, 0, mode)) skey = 'q';
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
				if (!show_file(TRUE, hook[key], NULL, 0, mode))
					skey = 'q';
			}
		}

		/* Hack, dump to file */
		if (skey == '|')
		{
			FILE *ffp;
			char buff[1024];
			char xtmp[82];

			strcpy (xtmp, "");

#ifdef JP
			if (!get_string("ファイル名: ", xtmp, 80)) continue;
#else
			if (!get_string("File name: ", xtmp, 80)) continue;
#endif

			/* Close it */
			my_fclose(fff);

			/* Build the filename */
			path_build(buff, sizeof(buff), ANGBAND_DIR_USER, xtmp);

			/* Hack -- Re-Open the file */
			fff = my_fopen(path, "r");

			ffp = my_fopen(buff, "w");

			/* Oops */
			if (!(fff && ffp))
			{
#ifdef JP
				msg_print("ファイルを開けません。");
#else
				msg_print("Failed to open file.");
#endif
				skey = ESCAPE;
				break;
			}

			sprintf(xtmp, "%s: %s", player_name, what ? what : caption);
			my_fputs(ffp, xtmp, 80);
			my_fputs(ffp, "\n", 80);

			while (!my_fgets(fff, buff, sizeof(buff)))
				my_fputs(ffp, buff, 80);

			/* Close it */
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

	/* Close the file */
	my_fclose(fff);

	/* Escape */
	if (skey == 'q') return (FALSE);

	/* Normal return */
	return (TRUE);
}


/*
 * Peruse the On-Line-Help
 */
void do_cmd_help(void)
{
	/* Save screen */
	screen_save();

	/* Peruse the main help file */
#ifdef JP
(void)show_file(TRUE, "jhelp.hlp", NULL, 0, 0);
#else
	(void)show_file(TRUE, "help.hlp", NULL, 0, 0);
#endif


	/* Load screen */
	screen_load();
}


/*
 * Process the player name.
 * Extract a clean "base name".
 * Build the savefile name if needed.
 */
void process_player_name(bool sf)
{
	int i, k = 0;
	char old_player_base[32] = "";

	if (character_generated) strcpy(old_player_base, player_base);

	/* Cannot be too long */
#if defined(MACINTOSH) || defined(MSDOS) || defined(USE_EMX) || defined(AMIGA) || defined(ACORN) || defined(VM)
#ifdef MSDOS
	if (strlen(player_name) > 8)
#else
	if (strlen(player_name) > 15)
#endif
	{
		/* Name too long */
#ifdef JP
quit_fmt("'%s'という名前は長すぎます！", player_name);
#else
		quit_fmt("The name '%s' is too long!", player_name);
#endif

	}
#endif

	/* Cannot contain "icky" characters */
	for (i = 0; player_name[i]; i++)
	{
		/* No control characters */
#ifdef JP
		if (iskanji(player_name[i])){i++;continue;}
		if (iscntrl( (unsigned char)player_name[i]))
#else
		if (iscntrl(player_name[i]))
#endif

		{
			/* Illegal characters */
#ifdef JP
quit_fmt("'%s' という名前は不正なコントロールコードを含んでいます。", player_name);
#else
			quit_fmt("The name '%s' contains control chars!", player_name);
#endif

		}
	}


#ifdef MACINTOSH

	/* Extract "useful" letters */
	for (i = 0; player_name[i]; i++)
	{
#ifdef JP
		unsigned char c = player_name[i];
#else
		char c = player_name[i];
#endif


		/* Convert "dot" to "underscore" */
		if (c == '.') c = '_';

		/* Accept all the letters */
		player_base[k++] = c;
	}

#else

	/* Extract "useful" letters */
	for (i = 0; player_name[i]; i++)
	{
#ifdef JP
		unsigned char c = player_name[i];
#else
		char c = player_name[i];
#endif

		/* Accept some letters */
#ifdef JP
		if(iskanji(c)){
		  if(k + 2 >= sizeof(player_base) || !player_name[i+1]) break;
		  player_base[k++] = c;
		  i++;
		  player_base[k++] = player_name[i];
		}
#ifdef SJIS
		else if (iskana(c)) player_base[k++] = c;
#endif
		else
#endif
		/* Convert path separator to underscore */
		if (!strncmp(PATH_SEP, player_name+i, strlen(PATH_SEP))){
			player_base[k++] = '_';
			i += strlen(PATH_SEP);
		}
		/* Convert some characters to underscore */
#ifdef MSDOS
		else if (my_strchr(" \"*+,./:;<=>?[\\]|", c)) player_base[k++] = '_';
#elif defined(WINDOWS)
		else if (my_strchr("\"*,/:;<>?\\|", c)) player_base[k++] = '_';
#endif
		else if (isprint(c)) player_base[k++] = c;
	}

#endif


#if defined(MSDOS)

	/* Hack -- max length */
	if (k > 8) k = 8;

#endif

	/* Terminate */
	player_base[k] = '\0';

	/* Require a "base" name */
	if (!player_base[0]) strcpy(player_base, "PLAYER");


#ifdef SAVEFILE_MUTABLE

	/* Accept */
	sf = TRUE;

#endif
	if (!savefile_base[0] && savefile[0])
	{
		cptr s;
		s = savefile;
		while (1)
		{
			cptr t;
			t = my_strstr(s, PATH_SEP);
			if (!t)
				break;
			s = t+1;
		}
		strcpy(savefile_base, s);
	}

	if (!savefile_base[0] || !savefile[0])
		sf = TRUE;

	/* Change the savefile name */
	if (sf)
	{
		char temp[128];

		strcpy(savefile_base, player_base);

#ifdef SAVEFILE_USE_UID
		/* Rename the savefile, using the player_uid and player_base */
		(void)sprintf(temp, "%d.%s", player_uid, player_base);
#else
		/* Rename the savefile, using the player_base */
		(void)sprintf(temp, "%s", player_base);
#endif

#ifdef VM
		/* Hack -- support "flat directory" usage on VM/ESA */
		(void)sprintf(temp, "%s.sv", player_base);
#endif /* VM */

		/* Build the filename */
		path_build(savefile, sizeof(savefile), ANGBAND_DIR_SAVE, temp);
	}

	/* Load an autopick preference file */
	if (character_generated)
	{
		if (!streq(old_player_base, player_base)) autopick_load_pref(FALSE);
	}
}


/*
 * Gets a name for the character, reacting to name changes.
 *
 * Assumes that "display_player(0)" has just been called
 *
 * Perhaps we should NOT ask for a name (at "birth()") on
 * Unix machines?  XXX XXX
 *
 * What a horrible name for a global function.  XXX XXX XXX
 */
void get_name(void)
{
	char tmp[64];

	/* Save the player name */
	strcpy(tmp, player_name);

	/* Prompt for a new name */
#ifdef JP
	if (get_string("キャラクターの名前を入力して下さい: ", tmp, 15))
#else
	if (get_string("Enter a name for your character: ", tmp, 15))
#endif
	{
		/* Use the name */
		strcpy(player_name, tmp);
	}

	if (0 == strlen(player_name))
	{
		/* Use default name */
		strcpy(player_name, "PLAYER");
	}

	strcpy(tmp,ap_ptr->title);
#ifdef JP
	if(ap_ptr->no == 1)
		strcat(tmp,"の");
#else
	strcat(tmp, " ");
#endif
	strcat(tmp,player_name);

	/* Re-Draw the name (in light blue) */
	Term_erase(34, 1, 255);
	c_put_str(TERM_L_BLUE, tmp, 1, 34);

	/* Erase the prompt, etc */
	clear_from(22);
}



/*
 * Hack -- commit suicide
 */
void do_cmd_suicide(void)
{
	int i;

	/* Flush input */
	flush();

	/* Verify Retirement */
	if (p_ptr->total_winner)
	{
		/* Verify */
#ifdef JP
if (!get_check_strict("引退しますか? ", CHECK_NO_HISTORY)) return;
#else
		if (!get_check_strict("Do you want to retire? ", CHECK_NO_HISTORY)) return;
#endif

	}

	/* Verify Suicide */
	else
	{
		/* Verify */
#ifdef JP
if (!get_check("本当に自殺しますか？")) return;
#else
		if (!get_check("Do you really want to commit suicide? ")) return;
#endif
	}


	if (!p_ptr->noscore)
	{
		/* Special Verification for suicide */
#ifdef JP
prt("確認のため '@' を押して下さい。", 0, 0);
#else
		prt("Please verify SUICIDE by typing the '@' sign: ", 0, 0);
#endif

		flush();
		i = inkey();
		prt("", 0, 0);
		if (i != '@') return;
	}

	/* Initialize "last message" buffer */
	if (p_ptr->last_message) string_free(p_ptr->last_message);
	p_ptr->last_message = NULL;

	/* Hack -- Note *winning* message */
	if (p_ptr->total_winner && last_words)
	{
		char buf[1024] = "";

		do
		{
#ifdef JP
			while (!get_string("*勝利*メッセージ: ", buf, sizeof buf)) ;
#else
			while (!get_string("*Winning* message: ", buf, sizeof buf)) ;
#endif
		}
#ifdef JP
		while (!get_check_strict("よろしいですか？", CHECK_NO_HISTORY));
#else
		while (!get_check_strict("Are you sure? ", CHECK_NO_HISTORY));
#endif

		if (buf[0])
		{
			p_ptr->last_message = string_make(buf);
			msg_print(p_ptr->last_message);
		}
	}

	/* Stop playing */
	p_ptr->playing = FALSE;

	/* Kill the player */
	p_ptr->is_dead = TRUE;

	/* Leaving */
	p_ptr->leaving = TRUE;

	if (!p_ptr->total_winner)
	{
#ifdef JP
		do_cmd_write_nikki(NIKKI_BUNSHOU, 0, "ダンジョンの探索に絶望して自殺した。");
		do_cmd_write_nikki(NIKKI_GAMESTART, 1, "-------- ゲームオーバー --------");
#else
		do_cmd_write_nikki(NIKKI_BUNSHOU, 0, "give up all hope to commit suicide.");
		do_cmd_write_nikki(NIKKI_GAMESTART, 1, "--------   Game  Over   --------");
#endif
		do_cmd_write_nikki(NIKKI_BUNSHOU, 1, "\n\n\n\n");
	}

	/* Cause of death */
#ifdef JP
(void)strcpy(p_ptr->died_from, "途中終了");
#else
	(void)strcpy(p_ptr->died_from, "Quitting");
#endif

}



/*
 * Save the game
 */
void do_cmd_save_game(int is_autosave)
{
	/* Autosaves do not disturb */
	if (is_autosave)
	{
#ifdef JP
msg_print("自動セーブ中");
#else
		msg_print("Autosaving the game...");
#endif

	}
	else
	{
		/* Disturb the player */
		disturb(1, 0);
	}

	/* Clear messages */
	msg_print(NULL);

	/* Handle stuff */
	handle_stuff();

	/* Message */
#ifdef JP
prt("ゲームをセーブしています...", 0, 0);
#else
	prt("Saving game...", 0, 0);
#endif


	/* Refresh */
	Term_fresh();

	/* The player is not dead */
#ifdef JP
(void)strcpy(p_ptr->died_from, "(セーブ)");
#else
	(void)strcpy(p_ptr->died_from, "(saved)");
#endif


	/* Forbid suspend */
	signals_ignore_tstp();

	/* Save the player */
	if (save_player())
	{
#ifdef JP
prt("ゲームをセーブしています... 終了", 0, 0);
#else
		prt("Saving game... done.", 0, 0);
#endif

	}

	/* Save failed (oops) */
	else
	{
#ifdef JP
prt("ゲームをセーブしています... 失敗！", 0, 0);
#else
		prt("Saving game... failed!", 0, 0);
#endif

	}

	/* Allow suspend again */
	signals_handle_tstp();

	/* Refresh */
	Term_fresh();

	/* Note that the player is not dead */
#ifdef JP
(void)strcpy(p_ptr->died_from, "(元気に生きている)");
#else
	(void)strcpy(p_ptr->died_from, "(alive and well)");
#endif

	/* HACK -- don't get sanity blast on updating view */
	hack_mind = FALSE;

	/* Update stuff */
	update_stuff();

	/* Initialize monster process */
	mproc_init();

	/* HACK -- reset the hackish flag */
	hack_mind = TRUE;
}


/*
 * Save the game and exit
 */
void do_cmd_save_and_exit(void)
{
	p_ptr->playing = FALSE;

	/* Leaving */
	p_ptr->leaving = TRUE;
#ifdef JP
	do_cmd_write_nikki(NIKKI_GAMESTART, 0, "----ゲーム中断----");
#else
	do_cmd_write_nikki(NIKKI_GAMESTART, 0, "---- Save and Exit Game ----");
#endif
}


/*
 * Hack -- Calculates the total number of points earned		-JWT-
 */
long total_points(void)
{
	int i, mult = 100;
	s16b max_dl = 0;
	u32b point, point_h, point_l;
	int arena_win = MIN(p_ptr->arena_number, MAX_ARENA_MONS);

	if (!preserve_mode) mult += 10;
	if (!autoroller) mult += 10;
	if (!smart_learn) mult -= 20;
	if (smart_cheat) mult += 30;
	if (ironman_shops) mult += 50;
	if (ironman_small_levels) mult += 10;
	if (ironman_empty_levels) mult += 20;
	if (!powerup_home) mult += 50;
	if (ironman_rooms) mult += 100;
	if (ironman_nightmare) mult += 100;

	if (mult < 5) mult = 5;

	for (i = 0; i < max_d_idx; i++)
		if(max_dlv[i] > max_dl)
			max_dl = max_dlv[i];

	point_l = (p_ptr->max_max_exp + (100 * max_dl));
	point_h = point_l / 0x10000L;
	point_l = point_l % 0x10000L;
	point_h *= mult;
	point_l *= mult;
	point_h += point_l / 0x10000L;
	point_l %= 0x10000L;

	point_l += ((point_h % 100) << 16);
	point_h /= 100;
	point_l /= 100;

	point = (point_h << 16) + (point_l);
	if (p_ptr->arena_number >= 0)
		point += (arena_win * arena_win * (arena_win > 29 ? 1000 : 100));

	if (ironman_downward) point *= 2;
	if (p_ptr->pclass == CLASS_BERSERKER)
	{
		if ( p_ptr->prace == RACE_SPECTRE )
			point = point / 5;
	}

	if ((p_ptr->pseikaku == SEIKAKU_MUNCHKIN) && point)
	{
		point = 1;
		if (p_ptr->total_winner) point = 2;
	}
	if (easy_band) point = (0 - point);

	return point;
}


#define GRAVE_LINE_WIDTH 31

/*
 * Centers a string within a GRAVE_LINE_WIDTH character string		-JWT-
 */
static void center_string(char *buf, cptr str)
{
	int i, j;

	/* Total length */
	i = strlen(str);

	/* Necessary border */
	j = GRAVE_LINE_WIDTH / 2 - i / 2;

	/* Mega-Hack */
	(void)sprintf(buf, "%*s%s%*s", j, "", str, GRAVE_LINE_WIDTH - i - j, "");
}


#if 0
/*
 * Save a "bones" file for a dead character
 *
 * Note that we will not use these files until Angband 2.8.0, and
 * then we will only use the name and level on which death occured.
 *
 * Should probably attempt some form of locking...
 */
static void make_bones(void)
{
	FILE                *fp;

	char                str[1024];


	/* Ignore wizards and borgs */
	if (!(p_ptr->noscore & 0x00FF))
	{
		/* Ignore people who die in town */
		if (dun_level)
		{
			char tmp[128];

			/* XXX XXX XXX "Bones" name */
			sprintf(tmp, "bone.%03d", dun_level);

			/* Build the filename */
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
			fprintf(fp, "%s\n", player_name);
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
bool (*tombstone_aux)(void) = NULL;


/*
 * Display a "tomb-stone"
 */
static void print_tomb(void)
{
	bool done = FALSE;

	/* Do we use a special tombstone ? */
	if (tombstone_aux)
	{
		/* Use tombstone hook */
		done = (*tombstone_aux)();
	}

	/* Print the text-tombstone */
	if (!done)
	{
		cptr   p;
		char   tmp[160];
		char   buf[1024];
		char   dummy[80];
		char   *t;
		FILE   *fp;
		time_t ct = time((time_t)0);
#ifdef JP
		int    extra_line = 0;
#endif

		/* Clear screen */
		Term_clear();

		/* Build the filename */
#ifdef JP
		path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, "dead_j.txt");
#else
		path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, "dead.txt");
#endif

		/* Open the News file */
		fp = my_fopen(buf, "r");

		/* Dump */
		if (fp)
		{
			int i = 0;

			/* Dump the file to the screen */
			while (0 == my_fgets(fp, buf, sizeof(buf)))
			{
				/* Display and advance */
				put_str(buf, i++, 0);
			}

			/* Close */
			my_fclose(fp);
		}

		/* King or Queen */
		if (p_ptr->total_winner || (p_ptr->lev > PY_MAX_LEVEL))
		{
#ifdef JP
			/* 英日切り替え */
			p= "偉大なる者";
#else
			p = "Magnificent";
#endif
		}

		/* Normal */
		else
		{
			p =  player_title[p_ptr->pclass][(p_ptr->lev - 1) / 5];
		}

		center_string(buf, player_name);
		put_str(buf, 6, 11);

#ifndef JP
		center_string(buf, "the");
		put_str(buf, 7, 11);
#endif

		center_string(buf, p);
		put_str(buf, 8, 11);

		center_string(buf, cp_ptr->title);
		put_str(buf, 10, 11);

#ifdef JP
		(void)sprintf(tmp, "レベル: %d", (int)p_ptr->lev);
#else
		(void)sprintf(tmp, "Level: %d", (int)p_ptr->lev);
#endif
		center_string(buf, tmp);
		put_str(buf, 11, 11);

#ifdef JP
		(void)sprintf(tmp, "経験値: %ld", (long)p_ptr->exp);
#else
		(void)sprintf(tmp, "Exp: %ld", (long)p_ptr->exp);
#endif
		center_string(buf, tmp);
		put_str(buf, 12, 11);

#ifdef JP
		(void)sprintf(tmp, "所持金: %ld", (long)p_ptr->au);
#else
		(void)sprintf(tmp, "AU: %ld", (long)p_ptr->au);
#endif
		center_string(buf, tmp);
		put_str(buf, 13, 11);

#ifdef JP
		/* 墓に刻む言葉をオリジナルより細かく表示 */
		if (streq(p_ptr->died_from, "途中終了"))
		{
			strcpy(tmp, "<自殺>");
		}
		else if (streq(p_ptr->died_from, "ripe"))
		{
			strcpy(tmp, "引退後に天寿を全う");
		}
		else if (streq(p_ptr->died_from, "Seppuku"))
		{
			strcpy(tmp, "勝利の後、切腹");
		}
		else
		{
			roff_to_buf(p_ptr->died_from, GRAVE_LINE_WIDTH + 1, tmp, sizeof tmp);
			t = tmp + strlen(tmp) + 1;
			if (*t)
			{
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

		if (!streq(p_ptr->died_from, "ripe") && !streq(p_ptr->died_from, "Seppuku"))
		{
			if (dun_level == 0)
			{
				cptr town = p_ptr->town_num ? "街" : "荒野";
				if (streq(p_ptr->died_from, "途中終了"))
				{
					sprintf(tmp, "%sで死んだ", town);
				}
				else
				{
					sprintf(tmp, "に%sで殺された", town);
				}
			}
			else
			{
				if (streq(p_ptr->died_from, "途中終了"))
				{
					sprintf(tmp, "地下 %d 階で死んだ", dun_level);
				}
				else
				{
					sprintf(tmp, "に地下 %d 階で殺された", dun_level);
				}
			}
			center_string(buf, tmp);
			put_str(buf, 15 + extra_line, 11);
		}
#else
		(void)sprintf(tmp, "Killed on Level %d", dun_level);
		center_string(buf, tmp);
		put_str(buf, 14, 11);

		roff_to_buf(format("by %s.", p_ptr->died_from), GRAVE_LINE_WIDTH + 1, tmp, sizeof tmp);
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

		(void)sprintf(tmp, "%-.24s", ctime(&ct));
		center_string(buf, tmp);
		put_str(buf, 17, 11);

#ifdef JP
		msg_format("さようなら、%s!", player_name);
#else
		msg_format("Goodbye, %s!", player_name);
#endif
	}
}


/*
 * Display some character info
 */
static void show_info(void)
{
	int             i, j, k, l;
	object_type		*o_ptr;
	store_type		*st_ptr;

	/* Hack -- Know everything in the inven/equip */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		o_ptr = &inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Aware and Known */
		object_aware(o_ptr);
		object_known(o_ptr);
	}

	for (i = 1; i < max_towns; i++)
	{
		st_ptr = &town[i].store[STORE_HOME];

		/* Hack -- Know everything in the home */
		for (j = 0; j < st_ptr->stock_num; j++)
		{
			o_ptr = &st_ptr->stock[j];

			/* Skip non-objects */
			if (!o_ptr->k_idx) continue;

			/* Aware and Known */
			object_aware(o_ptr);
			object_known(o_ptr);
		}
	}

	/* Hack -- Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	/* Flush all input keys */
	flush();

	/* Flush messages */
	msg_print(NULL);


	/* Describe options */
#ifdef JP
prt("キャラクターの記録をファイルに書き出すことができます。", 21, 0);
prt("リターンキーでキャラクターを見ます。ESCで中断します。", 22, 0);
#else
	prt("You may now dump a character record to one or more files.", 21, 0);
	prt("Then, hit RETURN to see the character, or ESC to abort.", 22, 0);
#endif


	/* Dump character records as requested */
	while (TRUE)
	{
		char out_val[160];

		/* Prompt */
#ifdef JP
put_str("ファイルネーム: ", 23, 0);
#else
		put_str("Filename: ", 23, 0);
#endif


		/* Default */
		strcpy(out_val, "");

		/* Ask for filename (or abort) */
		if (!askfor(out_val, 60)) return;

		/* Return means "show on screen" */
		if (!out_val[0]) break;

		/* Save screen */
		screen_save();

		/* Dump a character file */
		(void)file_character(out_val);

		/* Load screen */
		screen_load();
	}

	update_playtime();

	/* Display player */
	display_player(0);

	/* Prompt for inventory */
#ifdef JP
prt("何かキーを押すとさらに情報が続きます (ESCで中断): ", 23, 0);
#else
	prt("Hit any key to see more information (ESC to abort): ", 23, 0);
#endif


	/* Allow abort at this point */
	if (inkey() == ESCAPE) return;


	/* Show equipment and inventory */

	/* Equipment -- if any */
	if (equip_cnt)
	{
		Term_clear();
		item_tester_full = TRUE;
		(void)show_equip(0);
#ifdef JP
prt("装備していたアイテム: -続く-", 0, 0);
#else
		prt("You are using: -more-", 0, 0);
#endif

		if (inkey() == ESCAPE) return;
	}

	/* Inventory -- if any */
	if (inven_cnt)
	{
		Term_clear();
		item_tester_full = TRUE;
		(void)show_inven(0);
#ifdef JP
prt("持っていたアイテム: -続く-", 0, 0);
#else
		prt("You are carrying: -more-", 0, 0);
#endif

		if (inkey() == ESCAPE) return;
	}

	/* Homes in the different towns */
	for (l = 1; l < max_towns; l++)
	{
		st_ptr = &town[l].store[STORE_HOME];

		/* Home -- if anything there */
		if (st_ptr->stock_num)
		{
			/* Display contents of the home */
			for (k = 0, i = 0; i < st_ptr->stock_num; k++)
			{
				/* Clear screen */
				Term_clear();

				/* Show 12 items */
				for (j = 0; (j < 12) && (i < st_ptr->stock_num); j++, i++)
				{
					char o_name[MAX_NLEN];
					char tmp_val[80];

					/* Acquire item */
					o_ptr = &st_ptr->stock[i];

					/* Print header, clear line */
					sprintf(tmp_val, "%c) ", I2A(j));
					prt(tmp_val, j+2, 4);

					/* Display object description */
					object_desc(o_name, o_ptr, 0);
					c_put_str(tval_to_attr[o_ptr->tval], o_name, j+2, 7);
				}

				/* Caption */
#ifdef JP
prt(format("我が家に置いてあったアイテム ( %d ページ): -続く-", k+1), 0, 0);
#else
				prt(format("Your home contains (page %d): -more-", k+1), 0, 0);
#endif


				/* Wait for it */
				if (inkey() == ESCAPE) return;
			}
		}
	}
}


static bool check_score(void)
{
	/* Clear screen */
	Term_clear();

	/* No score file */
	if (highscore_fd < 0)
	{
#ifdef JP
msg_print("スコア・ファイルが使用できません。");
#else
		msg_print("Score file unavailable.");
#endif

		msg_print(NULL);
		return FALSE;
	}

#ifndef SCORE_WIZARDS
	/* Wizard-mode pre-empts scoring */
	if (p_ptr->noscore & 0x000F)
	{
#ifdef JP
msg_print("ウィザード・モードではスコアが記録されません。");
#else
		msg_print("Score not registered for wizards.");
#endif

		msg_print(NULL);
		return FALSE;
	}
#endif

#ifndef SCORE_BORGS
	/* Borg-mode pre-empts scoring */
	if (p_ptr->noscore & 0x00F0)
	{
#ifdef JP
msg_print("ボーグ・モードではスコアが記録されません。");
#else
		msg_print("Score not registered for borgs.");
#endif

		msg_print(NULL);
		return FALSE;
	}
#endif

#ifndef SCORE_CHEATERS
	/* Cheaters are not scored */
	if (p_ptr->noscore & 0xFF00)
	{
#ifdef JP
msg_print("詐欺をやった人はスコアが記録されません。");
#else
		msg_print("Score not registered for cheaters.");
#endif

		msg_print(NULL);
		return FALSE;
	}
#endif

	/* Interupted */
#ifdef JP
if (!p_ptr->total_winner && streq(p_ptr->died_from, "強制終了"))
#else
	if (!p_ptr->total_winner && streq(p_ptr->died_from, "Interrupting"))
#endif

	{
#ifdef JP
msg_print("強制終了のためスコアが記録されません。");
#else
		msg_print("Score not registered due to interruption.");
#endif

		msg_print(NULL);
		return FALSE;
	}

	/* Quitter */
#ifdef JP
if (!p_ptr->total_winner && streq(p_ptr->died_from, "途中終了"))
#else
	if (!p_ptr->total_winner && streq(p_ptr->died_from, "Quitting"))
#endif

	{
#ifdef JP
msg_print("途中終了のためスコアが記録されません。");
#else
		msg_print("Score not registered due to quitting.");
#endif

		msg_print(NULL);
		return FALSE;
	}
	return TRUE;
}

/*
 * Close up the current game (player may or may not be dead)
 *
 * This function is called only from "main.c" and "signals.c".
 */
void close_game(void)
{
	char buf[1024];
	bool do_send = TRUE;

/*	cptr p = "[i:キャラクタの情報, f:ファイル書き出し, t:スコア, x:*鑑定*, ESC:ゲーム終了]"; */

	/* Handle stuff */
	handle_stuff();

	/* Flush the messages */
	msg_print(NULL);

	/* Flush the input */
	flush();


	/* No suspending now */
	signals_ignore_tstp();


	/* Hack -- Character is now "icky" */
	character_icky = TRUE;


	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_APEX, "scores.raw");

	/* Grab permissions */
	safe_setuid_grab();

	/* Open the high score file, for reading/writing */
	highscore_fd = fd_open(buf, O_RDWR);

	/* Drop permissions */
	safe_setuid_drop();

	/* Handle death */
	if (p_ptr->is_dead)
	{
		/* Handle retirement */
		if (p_ptr->total_winner) kingly();

		/* Save memories */
#ifdef JP
		if (!cheat_save || get_check("死んだデータをセーブしますか？ "))
#else
		if (!cheat_save || get_check("Save death? "))
#endif
		{

#ifdef JP
if (!save_player()) msg_print("セーブ失敗！");
#else
			if (!save_player()) msg_print("death save failed!");
#endif
		}
		else do_send = FALSE;

		/* You are dead */
		print_tomb();

		flush();

		/* Show more info */
		show_info();

		/* Clear screen */
		Term_clear();

		if (check_score())
		{
			if ((!send_world_score(do_send)))
			{
#ifdef JP
				if (get_check_strict("後でスコアを登録するために待機しますか？", (CHECK_NO_ESCAPE | CHECK_NO_HISTORY)))
#else
				if (get_check_strict("Stand by for later score registration? ", (CHECK_NO_ESCAPE | CHECK_NO_HISTORY)))
#endif
				{
					p_ptr->wait_report_score = TRUE;
					p_ptr->is_dead = FALSE;
#ifdef JP
					if (!save_player()) msg_print("セーブ失敗！");
#else
					if (!save_player()) msg_print("death save failed!");
#endif
				}
			}
			if (!p_ptr->wait_report_score)
				(void)top_twenty();
		}
		else if (highscore_fd >= 0)
		{
			display_scores_aux(0, 10, -1, NULL);
		}
#if 0
		/* Dump bones file */
		make_bones();
#endif
	}

	/* Still alive */
	else
	{
		/* Save the game */
		do_cmd_save_game(FALSE);

		/* Prompt for scores XXX XXX XXX */
#ifdef JP
prt("リターンキーか ESC キーを押して下さい。", 0, 40);
#else
		prt("Press Return (or Escape).", 0, 40);
#endif


		/* Predict score (or ESCAPE) */
		if (inkey() != ESCAPE) predict_score();
	}


	/* Shut the high score file */
	(void)fd_close(highscore_fd);

	/* Forget the high score fd */
	highscore_fd = -1;

	/* Kill all temporal files */
	clear_saved_floor_files();

	/* Allow suspending now */
	signals_handle_tstp();
}


/*
 * Handle abrupt death of the visual system
 *
 * This routine is called only in very rare situations, and only
 * by certain visual systems, when they experience fatal errors.
 *
 * XXX XXX Hack -- clear the death flag when creating a HANGUP
 * save file so that player can see tombstone when restart.
 */
void exit_game_panic(void)
{
	/* If nothing important has happened, just quit */
#ifdef JP
	if (!character_generated || character_saved) quit("緊急事態");
#else
	if (!character_generated || character_saved) quit("panic");
#endif

	/* Mega-Hack -- see "msg_print()" */
	msg_flag = FALSE;

	/* Clear the top line */
	prt("", 0, 0);

	/* Hack -- turn off some things */
	disturb(1, 0);

	/* Mega-Hack -- Delay death */
	if (p_ptr->chp < 0) p_ptr->is_dead = FALSE;

	/* Hardcode panic save */
	p_ptr->panic_save = 1;

	/* Forbid suspend */
	signals_ignore_tstp();

	/* Indicate panic save */
#ifdef JP
	(void)strcpy(p_ptr->died_from, "(緊急セーブ)");
#else
	(void)strcpy(p_ptr->died_from, "(panic save)");
#endif


	/* Panic save, or get worried */
#ifdef JP
	if (!save_player()) quit("緊急セーブ失敗！");
#else
	if (!save_player()) quit("panic save failed!");
#endif


	/* Successful panic save */
#ifdef JP
	quit("緊急セーブ成功！");
#else
	quit("panic save succeeded!");
#endif
}


/*
 * Get a random line from a file
 * Based on the monster speech patch by Matt Graham,
 */
errr get_rnd_line(cptr file_name, int entry, char *output)
{
	FILE    *fp;
	char    buf[1024];
	int     counter, test;
	int     line_num = 0;


	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, file_name);

	/* Open the file */
	fp = my_fopen(buf, "r");

	/* Failed */
	if (!fp) return -1;

	/* Find the entry of the monster */
	while (TRUE)
	{
		/* Get a line from the file */
		if (my_fgets(fp, buf, sizeof(buf)) == 0)
		{
			/* Count the lines */
			line_num++;

			/* Look for lines starting with 'N:' */
			if ((buf[0] == 'N') && (buf[1] == ':'))
			{
				/* Allow default lines */
				if (buf[2] == '*')
				{
					/* Default lines */
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
				/* Get the monster number */
				else if (sscanf(&(buf[2]), "%d", &test) != EOF)
				{
					/* Is it the right number? */
					if (test == entry) break;
				}
				else
				{
					/* Error while converting the number */
					msg_format("Error in line %d of %s!", line_num, file_name);
					my_fclose(fp);
					return -1;
				}
			}
		}
		else
		{
			/* Reached end of file */
			my_fclose(fp);
			return -1;
		}
	}

	/* Get the random line */
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

		/* Abort */
		if (!buf[0]) break;

		/* Copy the line */
		if (one_in_(counter + 1)) strcpy(output, buf);
	}

	/* Close the file */
	my_fclose(fp);

	/* Success */
	return counter ? 0 : -1;
}


#ifdef JP
errr get_rnd_line_jonly(cptr file_name, int entry, char *output, int count)
{
	int  i, j, kanji;
	errr result = 1;

	for (i = 0; i < count; i++)
	{
		result = get_rnd_line(file_name, entry, output);
		if (result) break;
		kanji = 0;
		for (j = 0; output[j]; j++) kanji |= iskanji(output[j]);
		if (kanji) break;
	}
	return result;
}
#endif

/*
 * Process file for auto picker/destroyer.
 */
errr process_autopick_file(cptr name)
{
	char buf[1024];

	errr err = 0;

	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, name);

	err = process_pref_file_aux(buf, PREF_TYPE_AUTOPICK);

	/* Result */
	return (err);
}


/*
 * Process file for player's history editor.
 */
errr process_histpref_file(cptr name)
{
	char buf[1024];
	errr err = 0;
	bool old_character_xtra = character_xtra;

	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, name);

	/* Hack -- prevent modification birth options in this file */
	character_xtra = TRUE;

	err = process_pref_file_aux(buf, PREF_TYPE_HISTPREF);

	character_xtra = old_character_xtra;

	/* Result */
	return (err);
}


static errr counts_seek(int fd, u32b where, bool flag)
{
	huge seekpoint;
	char temp1[128], temp2[128];
	u32b zero_header[3] = {0L, 0L, 0L};
	int i;

#ifdef SAVEFILE_USE_UID
	(void)sprintf(temp1, "%d.%s.%d%d%d", player_uid, savefile_base, p_ptr->pclass, p_ptr->pseikaku, p_ptr->age);
#else
	(void)sprintf(temp1, "%s.%d%d%d", savefile_base, p_ptr->pclass, p_ptr->pseikaku, p_ptr->age);
#endif
	for (i = 0; temp1[i]; i++)
		temp1[i] ^= (i+1) * 63;

	seekpoint = 0;
	while (1)
	{
		if (fd_seek(fd, seekpoint + 3 * sizeof(u32b)))
			return 1;
		if (fd_read(fd, (char*)(temp2), sizeof(temp2)))
		{
			if (!flag)
				return 1;
			/* add new name */
			fd_seek(fd, seekpoint);
			fd_write(fd, (char*)zero_header, 3*sizeof(u32b));
			fd_write(fd, (char*)(temp1), sizeof(temp1));
			break;
		}

		if (strcmp(temp1, temp2) == 0)
			break;

		seekpoint += 128 + 3 * sizeof(u32b);
	}

	return fd_seek(fd, seekpoint + where * sizeof(u32b));
}

u32b counts_read(int where)
{
	int fd;
	u32b count = 0;
	char buf[1024];

#ifdef JP
	path_build(buf, sizeof(buf), ANGBAND_DIR_DATA, "z_info_j.raw");
#else
	path_build(buf, sizeof(buf), ANGBAND_DIR_DATA, "z_info.raw");
#endif
	fd = fd_open(buf, O_RDONLY);

	if (counts_seek(fd, where, FALSE) ||
	    fd_read(fd, (char*)(&count), sizeof(u32b)))
		count = 0;

	(void)fd_close(fd);

	return count;
}

errr counts_write(int where, u32b count)
{
	int fd;
	char buf[1024];
	errr err;

#ifdef JP
	path_build(buf, sizeof(buf), ANGBAND_DIR_DATA, "z_info_j.raw");
#else
	path_build(buf, sizeof(buf), ANGBAND_DIR_DATA, "z_info.raw");
#endif

	/* Grab permissions */
	safe_setuid_grab();

	fd = fd_open(buf, O_RDWR);

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

	err = fd_lock(fd, F_WRLCK);

	/* Drop permissions */
	safe_setuid_drop();

	if (err) return 1;

	counts_seek(fd, where, TRUE);
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


/*
 * Handle signals -- suspend
 *
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


/*
 * Handle signals -- simple (interrupt and quit)
 *
 * This function was causing a *huge* number of problems, so it has
 * been simplified greatly.  We keep a global variable which counts
 * the number of times the user attempts to kill the process, and
 * we commit suicide if the user does this a certain number of times.
 *
 * We attempt to give "feedback" to the user as he approaches the
 * suicide thresh-hold, but without penalizing accidental keypresses.
 *
 * To prevent messy accidents, we should reset this global variable
 * whenever the user enters a keypress, or something like that.
 */
static void handle_signal_simple(int sig)
{
	/* Disable handler */
	(void)signal(sig, SIG_IGN);


	/* Nothing to save, just quit */
	if (!character_generated || character_saved) quit(NULL);


	/* Count the signals */
	signal_count++;


	/* Terminate dead characters */
	if (p_ptr->is_dead)
	{
		/* Mark the savefile */
#ifdef JP
(void)strcpy(p_ptr->died_from, "強制終了");
#else
		(void)strcpy(p_ptr->died_from, "Abortion");
#endif

		forget_lite();
		forget_view();
		clear_mon_lite();

		/* Close stuff */
		close_game();

		/* Quit */
#ifdef JP
quit("強制終了");
#else
		quit("interrupt");
#endif

	}

	/* Allow suicide (after 5) */
	else if (signal_count >= 5)
	{
		/* Cause of "death" */
#ifdef JP
(void)strcpy(p_ptr->died_from, "強制終了中");
#else
		(void)strcpy(p_ptr->died_from, "Interrupting");
#endif


		forget_lite();
		forget_view();
		clear_mon_lite();

		/* Stop playing */
		p_ptr->playing = FALSE;

		/* Suicide */
		p_ptr->is_dead = TRUE;

		/* Leaving */
		p_ptr->leaving = TRUE;

		/* Close stuff */
		close_game();

		/* Quit */
#ifdef JP
quit("強制終了");
#else
		quit("interrupt");
#endif

	}

	/* Give warning (after 4) */
	else if (signal_count >= 4)
	{
		/* Make a noise */
		Term_xtra(TERM_XTRA_NOISE, 0);

		/* Clear the top line */
		Term_erase(0, 0, 255);

		/* Display the cause */
#ifdef JP
Term_putstr(0, 0, -1, TERM_WHITE, "熟慮の上の自殺！");
#else
		Term_putstr(0, 0, -1, TERM_WHITE, "Contemplating suicide!");
#endif


		/* Flush */
		Term_fresh();
	}

	/* Give warning (after 2) */
	else if (signal_count >= 2)
	{
		/* Make a noise */
		Term_xtra(TERM_XTRA_NOISE, 0);
	}

	/* Restore handler */
	(void)signal(sig, handle_signal_simple);
}


/*
 * Handle signal -- abort, kill, etc
 */
static void handle_signal_abort(int sig)
{
	int wid, hgt;

	Term_get_size(&wid, &hgt);

	/* Disable handler */
	(void)signal(sig, SIG_IGN);


	/* Nothing to save, just quit */
	if (!character_generated || character_saved) quit(NULL);


	forget_lite();
	forget_view();
	clear_mon_lite();

	/* Clear the bottom line */
	Term_erase(0, hgt - 1, 255);

	/* Give a warning */
	Term_putstr(0, hgt - 1, -1, TERM_RED,
#ifdef JP
"恐ろしいソフトのバグが飛びかかってきた！");
#else
		    "A gruesome software bug LEAPS out at you!");
#endif


	/* Message */
#ifdef JP
Term_putstr(45, hgt - 1, -1, TERM_RED, "緊急セーブ...");
#else
	Term_putstr(45, hgt - 1, -1, TERM_RED, "Panic save...");
#endif


#ifdef JP
	do_cmd_write_nikki(NIKKI_GAMESTART, 0, "----ゲーム異常終了----");
#else
	do_cmd_write_nikki(NIKKI_GAMESTART, 0, "---- Panic Save and Abort Game ----");
#endif

	/* Flush output */
	Term_fresh();

	/* Panic Save */
	p_ptr->panic_save = 1;

	/* Panic save */
#ifdef JP
(void)strcpy(p_ptr->died_from, "(緊急セーブ)");
#else
	(void)strcpy(p_ptr->died_from, "(panic save)");
#endif


	/* Forbid suspend */
	signals_ignore_tstp();

	/* Attempt to save */
	if (save_player())
	{
#ifdef JP
Term_putstr(45, hgt - 1, -1, TERM_RED, "緊急セーブ成功！");
#else
		Term_putstr(45, hgt - 1, -1, TERM_RED, "Panic save succeeded!");
#endif

	}

	/* Save failed */
	else
	{
#ifdef JP
Term_putstr(45, hgt - 1, -1, TERM_RED, "緊急セーブ失敗！");
#else
		Term_putstr(45, hgt - 1, -1, TERM_RED, "Panic save failed!");
#endif

	}

	/* Flush output */
	Term_fresh();

	/* Quit */
#ifdef JP
quit("ソフトのバグ");
#else
	quit("software bug");
#endif

}




/*
 * Ignore SIGTSTP signals (keyboard suspend)
 */
void signals_ignore_tstp(void)
{

#ifdef SIGTSTP
	(void)signal(SIGTSTP, SIG_IGN);
#endif

}

/*
 * Handle SIGTSTP signals (keyboard suspend)
 */
void signals_handle_tstp(void)
{

#ifdef SIGTSTP
	(void)signal(SIGTSTP, handle_signal_suspend);
#endif

}


/*
 * Prepare to handle the relevant signals
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


#else	/* HANDLE_SIGNALS */


/*
 * Do nothing
 */
void signals_ignore_tstp(void)
{
}

/*
 * Do nothing
 */
void signals_handle_tstp(void)
{
}

/*
 * Do nothing
 */
void signals_init(void)
{
}
#endif	/* HANDLE_SIGNALS */
