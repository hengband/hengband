/* File: mspells3.c */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

/* Purpose: Imitation code */

#include "angband.h"

#define pseudo_plev() (((p_ptr->lev + 40) * (p_ptr->lev + 40) - 1550) / 130)

static void learned_info(char *p, int power)
{
	int plev = pseudo_plev();
	int hp = p_ptr->chp;

#ifdef JP
	cptr s_dam = "損傷:";
	cptr s_dur = "期間:";
	cptr s_range = "範囲:";
	cptr s_heal = "回復:";
#else
	cptr s_dam = "dam ";
	cptr s_dur = "dur ";
	cptr s_range = "range ";
	cptr s_heal = "heal ";
#endif

	strcpy(p, "");

	switch (power)
	{
		case MS_SHRIEK:
		case MS_XXX1:
		case MS_XXX2:
		case MS_XXX3:
		case MS_XXX4:
		case MS_SCARE:
		case MS_BLIND:
		case MS_CONF:
		case MS_SLOW:
		case MS_SLEEP:
		case MS_HAND_DOOM:
		case MS_WORLD:
		case MS_SPECIAL:
		case MS_TELE_TO:
		case MS_TELE_AWAY:
		case MS_TELE_LEVEL:
		case MS_DARKNESS:
		case MS_MAKE_TRAP:
		case MS_FORGET:
		case MS_S_KIN:
		case MS_S_CYBER:
		case MS_S_MONSTER:
		case MS_S_MONSTERS:
		case MS_S_ANT:
		case MS_S_SPIDER:
		case MS_S_HOUND:
		case MS_S_HYDRA:
		case MS_S_ANGEL:
		case MS_S_DEMON:
		case MS_S_UNDEAD:
		case MS_S_DRAGON:
		case MS_S_HI_UNDEAD:
		case MS_S_HI_DRAGON:
		case MS_S_AMBERITE:
		case MS_S_UNIQUE:
			break;
		case MS_BALL_MANA:
		case MS_BALL_DARK:
		case MS_STARBURST:
			sprintf(p, " %s%d+10d10", s_dam, plev * 8 + 50);
			break;
		case MS_DISPEL:
			break;
		case MS_ROCKET:
			sprintf(p, " %s%d", s_dam, hp/4);
			break;
		case MS_SHOOT:
		{
			object_type *o_ptr = NULL;
			if (buki_motteruka(INVEN_RARM)) o_ptr = &inventory[INVEN_RARM];
			else if (buki_motteruka(INVEN_LARM)) o_ptr = &inventory[INVEN_LARM];
			else
				sprintf(p, " %s1", s_dam);
			if (o_ptr)
				sprintf(p, " %s%dd%d+%d", s_dam, o_ptr->dd, o_ptr->ds, o_ptr->to_d);
			break;
		}
		case MS_BR_ACID:
		case MS_BR_ELEC:
		case MS_BR_FIRE:
		case MS_BR_COLD:
		case MS_BR_POIS:
		case MS_BR_NUKE:
			sprintf(p, " %s%d", s_dam, hp/3);
			break;
		case MS_BR_NEXUS:
			sprintf(p, " %s%d", s_dam, MIN(hp/3, 250));
			break;
		case MS_BR_TIME:
			sprintf(p, " %s%d", s_dam, MIN(hp/3, 150));
			break;
		case MS_BR_GRAVITY:
			sprintf(p, " %s%d", s_dam, MIN(hp/3, 200));
			break;
		case MS_BR_MANA:
			sprintf(p, " %s%d", s_dam, MIN(hp/3, 250));
			break;
		case MS_BR_NETHER:
		case MS_BR_LITE:
		case MS_BR_DARK:
		case MS_BR_CONF:
		case MS_BR_SOUND:
		case MS_BR_CHAOS:
		case MS_BR_DISEN:
		case MS_BR_SHARDS:
		case MS_BR_PLASMA:
			sprintf(p, " %s%d", s_dam, hp/6);
			break;
		case MS_BR_INERTIA:
		case MS_BR_FORCE:
			sprintf(p, " %s%d", s_dam, MIN(hp/6, 200));
			break;
		case MS_BR_DISI:
			sprintf(p, " %s%d", s_dam, MIN(hp/6, 150));
			break;
		case MS_BALL_NUKE:
			sprintf(p, " %s%d+10d6", s_dam, plev * 2);
			break;
		case MS_BALL_CHAOS:
			sprintf(p, " %s%d+10d10", s_dam, plev * 4);
			break;
		case MS_BALL_ACID:
			sprintf(p, " %s15+d%d", s_dam, plev * 6);
			break;
		case MS_BALL_ELEC:
			sprintf(p, " %s8+d%d", s_dam, plev * 3);
			break;
		case MS_BALL_FIRE:
			sprintf(p, " %s10+d%d", s_dam, plev * 7);
			break;
		case MS_BALL_COLD:
			sprintf(p, " %s10+d%d", s_dam, plev * 3);
			break;
		case MS_BALL_POIS:
			sprintf(p, " %s12d2", s_dam);
			break;
		case MS_BALL_NETHER:
			sprintf(p, " %s%d+10d10", s_dam, plev * 2 + 50);
			break;
		case MS_BALL_WATER:
			sprintf(p, " %s50+d%d", s_dam, plev * 4);
			break;
		case MS_DRAIN_MANA:
			sprintf(p, " %sd%d+%d", s_heal, plev, plev);
			break;
		case MS_MIND_BLAST:
			sprintf(p, " %s8d8", s_dam);
			break;
		case MS_BRAIN_SMASH:
			sprintf(p, " %s12d15", s_dam);
			break;
		case MS_CAUSE_1:
			sprintf(p, " %s3d8", s_dam);
			break;
		case MS_CAUSE_2:
			sprintf(p, " %s8d8", s_dam);
			break;
		case MS_CAUSE_3:
			sprintf(p, " %s10d15", s_dam);
			break;
		case MS_CAUSE_4:
			sprintf(p, " %s15d15", s_dam);
			break;
		case MS_BOLT_ACID:
			sprintf(p, " %s%d+7d8", s_dam, plev * 2 / 3);
			break;
		case MS_BOLT_ELEC:
			sprintf(p, " %s%d+4d8", s_dam, plev * 2 / 3);
			break;
		case MS_BOLT_FIRE:
			sprintf(p, " %s%d+9d8", s_dam, plev * 2 / 3);
			break;
		case MS_BOLT_COLD:
			sprintf(p, " %s%d+6d8", s_dam, plev * 2 / 3);
			break;
		case MS_BOLT_NETHER:
			sprintf(p, " %s%d+5d5", s_dam, 30 + plev * 8 / 3);
			break;
		case MS_BOLT_WATER:
			sprintf(p, " %s%d+10d10", s_dam, plev * 2);
			break;
		case MS_BOLT_MANA:
			sprintf(p, " %s50+d%d", s_dam, plev * 7);
			break;
		case MS_BOLT_PLASMA:
			sprintf(p, " %s%d+8d7", s_dam, plev * 2 + 10);
			break;
		case MS_BOLT_ICE:
			sprintf(p, " %s%d+6d6", s_dam, plev * 2);
			break;
		case MS_MAGIC_MISSILE:
			sprintf(p, " %s%d+2d6", s_dam, plev * 2 / 3);
			break;
		case MS_SPEED:
			sprintf(p, " %sd%d+%d", s_dur, 20+plev, plev);
			break;
		case MS_HEAL:
			sprintf(p, " %s%d", s_heal, plev*4);
			break;
		case MS_INVULNER:
			sprintf(p, " %sd7+7", s_dur);
			break;
		case MS_BLINK:
			sprintf(p, " %s10", s_range);
			break;
		case MS_TELEPORT:
			sprintf(p, " %s%d", s_range, plev * 5);
			break;
		case MS_PSY_SPEAR:
			sprintf(p, " %s100+d%d", s_dam, plev * 3);
			break;
		case MS_RAISE_DEAD:
			sprintf(p, " %s5", s_range);
			break;
		default:
			break;
	}
}


/*
 * Allow user to choose a imitation.
 *
 * If a valid spell is chosen, saves it in '*sn' and returns TRUE
 * If the user hits escape, returns FALSE, and set '*sn' to -1
 * If there are no legal choices, returns FALSE, and sets '*sn' to -2
 *
 * The "prompt" should be "cast", "recite", or "study"
 * The "known" should be TRUE for cast/pray, FALSE for study
 *
 * nb: This function has a (trivial) display bug which will be obvious
 * when you run it. It's probably easy to fix but I haven't tried,
 * sorry.
 */
static int get_learned_power(int *sn)
{
	int             i = 0;
	int             num = 0;
	int             y = 1;
	int             x = 18;
	int             minfail = 0;
	int             plev = p_ptr->lev;
	int             chance = 0;
	int             ask = TRUE, mode = 0;
	int             spellnum[MAX_MONSPELLS];
	char            ch;
	char            choice;
	char            out_val[160];
	char            comment[80];
	s32b            f4 = 0, f5 = 0, f6 = 0;
#ifdef JP
cptr            p = "魔法";
#else
	cptr            p = "magic";
#endif

	monster_power   spell;
	bool            flag, redraw;
	int menu_line = (use_menu ? 1 : 0);

	/* Assume cancelled */
	*sn = (-1);

	/* Nothing chosen yet */
	flag = FALSE;

	/* No redraw yet */
	redraw = FALSE;

#ifdef ALLOW_REPEAT /* TNB */

	/* Get the spell, if available */
	if (repeat_pull(sn))
	{
		/* Success */
		return (TRUE);
	}

#endif /* ALLOW_REPEAT -- TNB */

	if (use_menu)
	{
		screen_save();

		while(!mode)
		{
#ifdef JP
			prt(format(" %s ボルト", (menu_line == 1) ? "》" : "  "), 2, 14);
			prt(format(" %s ボール", (menu_line == 2) ? "》" : "  "), 3, 14);
			prt(format(" %s ブレス", (menu_line == 3) ? "》" : "  "), 4, 14);
			prt(format(" %s 召喚", (menu_line == 4) ? "》" : "  "), 5, 14);
			prt(format(" %s その他", (menu_line == 5) ? "》" : "  "), 6, 14);
			prt("どの種類の魔法を使いますか？", 0, 0);
#else
			prt(format(" %s bolt", (menu_line == 1) ? "> " : "  "), 2, 14);
			prt(format(" %s ball", (menu_line == 2) ? "> " : "  "), 3, 14);
			prt(format(" %s breath", (menu_line == 3) ? "> " : "  "), 4, 14);
			prt(format(" %s sommoning", (menu_line == 4) ? "> " : "  "), 5, 14);
			prt(format(" %s others", (menu_line == 5) ? "> " : "  "), 6, 14);
			prt("use which type of magic? ", 0, 0);
#endif
			choice = inkey();
			switch(choice)
			{
			case ESCAPE:
			case 'z':
			case 'Z':
				screen_load();
				return FALSE;
			case '2':
			case 'j':
			case 'J':
				menu_line++;
				break;
			case '8':
			case 'k':
			case 'K':
				menu_line+= 4;
				break;
			case '\r':
			case 'x':
			case 'X':
				mode = menu_line;
				break;
			}
			if (menu_line > 5) menu_line -= 5;
		}
		screen_load();
	}
	else
	{
#ifdef JP
	sprintf(comment, "[A]ボルト, [B]ボール, [C]ブレス, [D]召喚, [E]その他:");
#else
	sprintf(comment, "[A] bolt, [B] ball, [C] breath, [D] summoning, [E] others:");
#endif
	while (TRUE)
	{
		if (!get_com(comment, &ch, TRUE))
		{
			return FALSE;
		}
		if (ch == 'A' || ch == 'a')
		{
			mode = 1;
			break;
		}
		if (ch == 'B' || ch == 'b')
		{
			mode = 2;
			break;
		}
		if (ch == 'C' || ch == 'c')
		{
			mode = 3;
			break;
		}
		if (ch == 'D' || ch == 'd')
		{
			mode = 4;
			break;
		}
		if (ch == 'E' || ch == 'e')
		{
			mode = 5;
			break;
		}
	}
	}

	set_rf_masks(&f4, &f5, &f6, mode);

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
	for (i = 0; i < num; i++)
	{
		if (p_ptr->magic_num2[spellnum[i]])
		{
			if (use_menu) menu_line = i+1;
			break;
		}
	}
	if (i == num)
	{
#ifdef JP
		msg_print("その種類の魔法は覚えていない！");
#else
		msg_print("You don't know any spell of this type.");
#endif
		return (FALSE);
	}

	/* Build a prompt (accept all spells) */
	(void)strnfmt(out_val, 78, 
#ifdef JP
		      "(%c-%c, '*'で一覧, ESC) どの%sを唱えますか？",
#else
		      "(%c-%c, *=List, ESC=exit) Use which %s? ",
#endif
		      I2A(0), I2A(num - 1), p);

	if (use_menu) screen_save();

	/* Get a spell from the user */

	choice= (always_show_list || use_menu) ? ESCAPE:1 ;
	while (!flag)
	{
		if(choice==ESCAPE) choice = ' '; 
		else if( !get_com(out_val, &choice, TRUE) )break; 

		if (use_menu && choice != ' ')
		{
			switch(choice)
			{
				case '0':
				{
					screen_load();
					return (FALSE);
				}

				case '8':
				case 'k':
				case 'K':
				{
					do
					{
						menu_line += (num-1);
						if (menu_line > num) menu_line -= num;
					} while(!p_ptr->magic_num2[spellnum[menu_line-1]]);
					break;
				}

				case '2':
				case 'j':
				case 'J':
				{
					do
					{
						menu_line++;
						if (menu_line > num) menu_line -= num;
					} while(!p_ptr->magic_num2[spellnum[menu_line-1]]);
					break;
				}

				case '6':
				case 'l':
				case 'L':
				{
					menu_line=num;
					while(!p_ptr->magic_num2[spellnum[menu_line-1]]) menu_line--;
					break;
				}

				case '4':
				case 'h':
				case 'H':
				{
					menu_line=1;
					while(!p_ptr->magic_num2[spellnum[menu_line-1]]) menu_line++;
					break;
				}

				case 'x':
				case 'X':
				case '\r':
				{
					i = menu_line - 1;
					ask = FALSE;
					break;
				}
			}
		}
		/* Request redraw */
		if ((choice == ' ') || (choice == '*') || (choice == '?') || (use_menu && ask))
		{
			/* Show the list */
			if (!redraw || use_menu)
			{
				char psi_desc[80];

				/* Show list */
				redraw = TRUE;

				/* Save the screen */
				if (!use_menu) screen_save();

				/* Display a list of spells */
				prt("", y, x);
#ifdef JP
put_str("名前", y, x + 5);
#else
				put_str("Name", y, x + 5);
#endif

#ifdef JP
put_str("MP 失率 効果", y, x + 33);
#else
				put_str("SP Fail Info", y, x + 32);
#endif


				/* Dump the spells */
				for (i = 0; i < num; i++)
				{
					int need_mana;

					prt("", y + i + 1, x);
					if (!p_ptr->magic_num2[spellnum[i]]) continue;

					/* Access the spell */
					spell = monster_powers[spellnum[i]];

					chance = spell.fail;

					/* Reduce failure rate by "effective" level adjustment */
					if (plev > spell.level) chance -= 3 * (plev - spell.level);
					else chance += (spell.level - plev);

					/* Reduce failure rate by INT/WIS adjustment */
					chance -= 3 * (adj_mag_stat[p_ptr->stat_ind[A_INT]] - 1);

					chance = mod_spell_chance_1(chance);

					need_mana = mod_need_mana(monster_powers[spellnum[i]].smana, 0, REALM_NONE);

					/* Not enough mana to cast */
					if (need_mana > p_ptr->csp)
					{
						chance += 5 * (need_mana - p_ptr->csp);
					}

					/* Extract the minimum failure rate */
					minfail = adj_mag_fail[p_ptr->stat_ind[A_INT]];

					/* Minimum failure rate */
					if (chance < minfail) chance = minfail;

					/* Stunning makes spells harder */
					if (p_ptr->stun > 50) chance += 25;
					else if (p_ptr->stun) chance += 15;

					/* Always a 5 percent chance of working */
					if (chance > 95) chance = 95;

					chance = mod_spell_chance_2(chance);

					/* Get info */
					learned_info(comment, spellnum[i]);

					if (use_menu)
					{
#ifdef JP
						if (i == (menu_line-1)) strcpy(psi_desc, "  》");
#else
						if (i == (menu_line-1)) strcpy(psi_desc, "  > ");
#endif
						else strcpy(psi_desc, "    ");
					}
					else sprintf(psi_desc, "  %c)", I2A(i));

					/* Dump the spell --(-- */
					strcat(psi_desc, format(" %-26s %3d %3d%%%s",
						spell.name, need_mana,
						chance, comment));
					prt(psi_desc, y + i + 1, x);
				}

				/* Clear the bottom line */
				if (y < 22) prt("", y + i + 1, x);
			}

			/* Hide the list */
			else
			{
				/* Hide list */
				redraw = FALSE;

				/* Restore the screen */
				screen_load();
			}

			/* Redo asking */
			continue;
		}

		if (!use_menu)
		{
			/* Note verify */
			ask = isupper(choice);

			/* Lowercase */
			if (ask) choice = tolower(choice);

			/* Extract request */
			i = (islower(choice) ? A2I(choice) : -1);
		}

		/* Totally Illegal */
		if ((i < 0) || (i >= num) || !p_ptr->magic_num2[spellnum[i]])
		{
			bell();
			continue;
		}

		/* Save the spell index */
		spell = monster_powers[spellnum[i]];

		/* Verify it */
		if (ask)
		{
			char tmp_val[160];

			/* Prompt */
#ifdef JP
			(void) strnfmt(tmp_val, 78, "%sの魔法を唱えますか？", monster_powers[spellnum[i]].name);
#else
			(void)strnfmt(tmp_val, 78, "Use %s? ", monster_powers[spellnum[i]].name);
#endif


			/* Belay that order */
			if (!get_check(tmp_val)) continue;
		}

		/* Stop the loop */
		flag = TRUE;
	}

	/* Restore the screen */
	if (redraw) screen_load();

	/* Show choices */
	p_ptr->window |= (PW_SPELL);

	/* Window stuff */
	window_stuff();

	/* Abort if needed */
	if (!flag) return (FALSE);

	/* Save the choice */
	(*sn) = spellnum[i];

#ifdef ALLOW_REPEAT /* TNB */

	repeat_push(*sn);

#endif /* ALLOW_REPEAT -- TNB */

	/* Success */
	return (TRUE);
}


/*
 * do_cmd_cast calls this function if the player's class
 * is 'imitator'.
 */
static bool cast_learned_spell(int spell, bool success)
{
	int             dir;
	int             plev = pseudo_plev();
	int     summon_lev = p_ptr->lev * 2 / 3 + randint1(p_ptr->lev/2);
	int             hp = p_ptr->chp;
	int             damage = 0;
	bool   pet = success;
	bool   no_trump = FALSE;
	u32b p_mode, u_mode = 0L, g_mode;

	if (pet)
	{
		p_mode = PM_FORCE_PET;
		g_mode = 0;
	}
	else
	{
		p_mode = PM_NO_PET;
		g_mode = PM_ALLOW_GROUP;
	}

	if (!success || (randint1(50+plev) < plev/10)) u_mode = PM_ALLOW_UNIQUE;

	/* spell code */
	switch (spell)
	{
	case MS_SHRIEK:
#ifdef JP
msg_print("かん高い金切り声をあげた。");
#else
		msg_print("You make a high pitched shriek.");
#endif

		aggravate_monsters(0);
		break;
	case MS_XXX1:
		break;
	case MS_DISPEL:
	{
		int m_idx;

		if (!target_set(TARGET_KILL)) return FALSE;
		m_idx = cave[target_row][target_col].m_idx;
		if (!m_idx) break;
		if (!player_has_los_bold(target_row, target_col)) break;
		if (!projectable(py, px, target_row, target_col)) break;
		dispel_monster_status(m_idx);
		break;
	}
	case MS_ROCKET:
		if (!get_aim_dir(&dir)) return FALSE;
		else
#ifdef JP
msg_print("ロケットを発射した。");
#else
			msg_print("You fire a rocket.");
#endif
		damage = hp / 4;
			fire_rocket(GF_ROCKET, dir, damage, 2);
		break;
	case MS_SHOOT:
	{
		object_type *o_ptr = NULL;

		if (!get_aim_dir(&dir)) return FALSE;
		else
		{
#ifdef JP
msg_print("矢を放った。");
#else
			msg_print("You fire an arrow.");
#endif
			if (buki_motteruka(INVEN_RARM)) o_ptr = &inventory[INVEN_RARM];
			else if (buki_motteruka(INVEN_LARM)) o_ptr = &inventory[INVEN_LARM];
			else
			damage = 1;
			if (o_ptr)
			{
				damage = damroll(o_ptr->dd, o_ptr->ds)+ o_ptr->to_d;
				if (damage < 1) damage = 1;
			}
			fire_bolt(GF_ARROW, dir, damage);
		}
		break;
	}
	case MS_XXX2:
		break;
	case MS_XXX3:
		break;
	case MS_XXX4:
		break;
	case MS_BR_ACID:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("酸のブレスを吐いた。");
#else
			else msg_print("You breathe acid.");
#endif
		damage = hp / 3;
		fire_ball(GF_ACID, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_ELEC:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("稲妻のブレスを吐いた。");
#else
			else msg_print("You breathe lightning.");
#endif
		damage = hp / 3;
		fire_ball(GF_ELEC, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_FIRE:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("火炎のブレスを吐いた。");
#else
			else msg_print("You breathe fire.");
#endif
		damage = hp / 3;
		fire_ball(GF_FIRE, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_COLD:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("冷気のブレスを吐いた。");
#else
			else msg_print("You breathe frost.");
#endif
		damage = hp / 3;
		fire_ball(GF_COLD, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_POIS:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("ガスのブレスを吐いた。");
#else
			else msg_print("You breathe gas.");
#endif
		damage = hp / 3;
		fire_ball(GF_POIS, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_NETHER:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("地獄のブレスを吐いた。");
#else
			else msg_print("You breathe nether.");
#endif
		damage = hp / 6;
		fire_ball(GF_NETHER, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_LITE:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("閃光のブレスを吐いた。");
#else
			else msg_print("You breathe light.");
#endif
		damage = hp / 6;
		fire_ball(GF_LITE, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_DARK:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("暗黒のブレスを吐いた。");
#else
			else msg_print("You breathe darkness.");
#endif
		damage = hp / 6;
		fire_ball(GF_DARK, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_CONF:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("混乱のブレスを吐いた。");
#else
			else msg_print("You breathe confusion.");
#endif
		damage = hp / 6;
		fire_ball(GF_CONFUSION, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_SOUND:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("轟音のブレスを吐いた。");
#else
			else msg_print("You breathe sound.");
#endif
		damage = hp / 6;
		fire_ball(GF_SOUND, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_CHAOS:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("カオスのブレスを吐いた。");
#else
			else msg_print("You breathe chaos.");
#endif
		damage = hp / 6;
		fire_ball(GF_CHAOS, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_DISEN:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("劣化のブレスを吐いた。");
#else
			else msg_print("You breathe disenchantment.");
#endif
		damage = hp / 6;
		fire_ball(GF_DISENCHANT, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_NEXUS:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("因果混乱のブレスを吐いた。");
#else
			else msg_print("You breathe nexus.");
#endif
		damage = MIN(hp / 3, 250);
		fire_ball(GF_NEXUS, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_TIME:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("時間逆転のブレスを吐いた。");
#else
			else msg_print("You breathe time.");
#endif
		damage = MIN(hp / 3, 150);
		fire_ball(GF_TIME, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_INERTIA:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("遅鈍のブレスを吐いた。");
#else
			else msg_print("You breathe inertia.");
#endif
		damage = MIN(hp / 6, 200);
		fire_ball(GF_INERTIA, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_GRAVITY:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("重力のブレスを吐いた。");
#else
			else msg_print("You breathe gravity.");
#endif
		damage = MIN(hp / 3, 200);
		fire_ball(GF_GRAVITY, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_SHARDS:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("破片のブレスを吐いた。");
#else
			else msg_print("You breathe shards.");
#endif
		damage = hp / 6;
		fire_ball(GF_SHARDS, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_PLASMA:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("プラズマのブレスを吐いた。");
#else
			else msg_print("You breathe plasma.");
#endif
		damage = MIN(hp / 6, 150);
		fire_ball(GF_PLASMA, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_FORCE:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("フォースのブレスを吐いた。");
#else
			else msg_print("You breathe force.");
#endif
		damage = MIN(hp / 6, 200);
		fire_ball(GF_FORCE, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_MANA:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("魔力のブレスを吐いた。");
#else
			else msg_print("You breathe mana.");
#endif
		
		damage = MIN(hp / 3, 250);
		fire_ball(GF_MANA, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BALL_NUKE:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("放射能球を放った。");
#else
			else msg_print("You cast a ball of radiation.");
#endif
		damage = plev * 2 + damroll(10, 6);
		fire_ball(GF_NUKE, dir, damage, 2);
		break;
	case MS_BR_NUKE:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("放射性廃棄物のブレスを吐いた。");
#else
			else msg_print("You breathe toxic waste.");
#endif
		damage = hp / 3;
		fire_ball(GF_NUKE, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BALL_CHAOS:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("純ログルスを放った。");
#else
			else msg_print("You invoke a raw Logrus.");
#endif
		damage = plev * 4 + damroll(10, 10);
		fire_ball(GF_CHAOS, dir, damage, 4);
		break;
	case MS_BR_DISI:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("分解のブレスを吐いた。");
#else
			else msg_print("You breathe disintegration.");
#endif
		damage = MIN(hp / 6, 150);
		fire_ball(GF_DISINTEGRATE, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BALL_ACID:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("アシッド・ボールの呪文を唱えた。");
#else
			else msg_print("You cast an acid ball.");
#endif
		damage = randint1(plev * 6) + 15;
		fire_ball(GF_ACID, dir, damage, 2);
		break;
	case MS_BALL_ELEC:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("サンダー・ボールの呪文を唱えた。");
#else
			else msg_print("You cast a lightning ball.");
#endif
		damage = randint1(plev * 3) + 8;
		fire_ball(GF_ELEC, dir, damage, 2);
		break;
	case MS_BALL_FIRE:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("ファイア・ボールの呪文を唱えた。");
#else
			else msg_print("You cast a fire ball.");
#endif
		damage = randint1(plev * 7) + 10;
		fire_ball(GF_FIRE, dir, damage, 2);
		break;
	case MS_BALL_COLD:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("アイス・ボールの呪文を唱えた。");
#else
			else msg_print("You cast a frost ball.");
#endif
		damage = randint1(plev * 3) + 10;
		fire_ball(GF_COLD, dir, damage, 2);
		break;
	case MS_BALL_POIS:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("悪臭雲の呪文を唱えた。");
#else
			else msg_print("You cast a stinking cloud.");
#endif
		damage = damroll(12,2);
		fire_ball(GF_POIS, dir, damage, 2);
		break;
	case MS_BALL_NETHER:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("地獄球の呪文を唱えた。");
#else
			else msg_print("You cast a nether ball.");
#endif
		damage = plev * 2 + 50 + damroll(10, 10);
		fire_ball(GF_NETHER, dir, damage, 2);
		break;
	case MS_BALL_WATER:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("流れるような身振りをした。");
#else
			else msg_print("You gesture fluidly.");
#endif
		damage = randint1(plev * 4) + 50;
		fire_ball(GF_WATER, dir, damage, 4);
		break;
	case MS_BALL_MANA:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("魔力の嵐の呪文を念じた。");
#else
			else msg_print("You invoke a mana storm.");
#endif
		damage = plev * 8 + 50 + damroll(10, 10);
		fire_ball(GF_MANA, dir, damage, 4);
		break;
	case MS_BALL_DARK:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("暗黒の嵐の呪文を念じた。");
#else
			else msg_print("You invoke a darkness storm.");
#endif
		damage = plev * 8 + 50 + damroll(10, 10);
		fire_ball(GF_DARK, dir, damage, 4);
		break;
	case MS_DRAIN_MANA:
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball_hide(GF_DRAIN_MANA, dir, randint1(plev)+plev, 0);
		break;
	case MS_MIND_BLAST:
		if (!get_aim_dir(&dir)) return FALSE;
		damage = damroll(7, 7);
		fire_ball_hide(GF_MIND_BLAST, dir, damage, 0);
		break;
	case MS_BRAIN_SMASH:
		if (!get_aim_dir(&dir)) return FALSE;
		damage = damroll(12, 12);
		fire_ball_hide(GF_BRAIN_SMASH, dir, damage, 0);
		break;
	case MS_CAUSE_1:
		if (!get_aim_dir(&dir)) return FALSE;
		damage = damroll(3, 8);
		fire_ball_hide(GF_CAUSE_1, dir, damage, 0);
		break;
	case MS_CAUSE_2:
		if (!get_aim_dir(&dir)) return FALSE;
		damage = damroll(8, 8);
		fire_ball_hide(GF_CAUSE_2, dir, damage, 0);
		break;
	case MS_CAUSE_3:
		if (!get_aim_dir(&dir)) return FALSE;
		damage = damroll(10, 15);
		fire_ball_hide(GF_CAUSE_3, dir, damage, 0);
		break;
	case MS_CAUSE_4:
		if (!get_aim_dir(&dir)) return FALSE;
		damage = damroll(15, 15);
		fire_ball_hide(GF_CAUSE_4, dir, damage, 0);
		break;
	case MS_BOLT_ACID:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("アシッド・ボルトの呪文を唱えた。");
#else
			else msg_print("You cast an acid bolt.");
#endif
		damage = damroll(7, 8) + plev * 2 / 3;
		fire_bolt(GF_ACID, dir, damage);
		break;
	case MS_BOLT_ELEC:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("サンダー・ボルトの呪文を唱えた。");
#else
			else msg_print("You cast a lightning bolt.");
#endif
		damage = damroll(4, 8) + plev * 2 / 3;
		fire_bolt(GF_ELEC, dir, damage);
		break;
	case MS_BOLT_FIRE:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("ファイア・ボルトの呪文を唱えた。");
#else
			else msg_print("You cast a fire bolt.");
#endif
		damage = damroll(9, 8) + plev * 2 / 3;
		fire_bolt(GF_FIRE, dir, damage);
		break;
	case MS_BOLT_COLD:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("アイス・ボルトの呪文を唱えた。");
#else
			else msg_print("You cast a frost bolt.");
#endif
		damage = damroll(6, 8) + plev * 2 / 3;
		fire_bolt(GF_COLD, dir, damage);
		break;
	case MS_STARBURST:
		if (!get_aim_dir(&dir)) return FALSE;
		else
#ifdef JP
msg_print("スターバーストの呪文を念じた。");
#else
			msg_print("You invoke a starburst.");
#endif
		damage = plev * 8 + 50 + damroll(10, 10);
		fire_ball(GF_LITE, dir, damage, 4);
		break;
	case MS_BOLT_NETHER:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("地獄の矢の呪文を唱えた。");
#else
			else msg_print("You cast a nether bolt.");
#endif
		damage = 30 + damroll(5, 5) + plev * 8 / 3;
		fire_bolt(GF_NETHER, dir, damage);
		break;
	case MS_BOLT_WATER:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("ウォーター・ボルトの呪文を唱えた。");
#else
			else msg_print("You cast a water bolt.");
#endif
		damage = damroll(10, 10) + plev * 2;
		fire_bolt(GF_WATER, dir, damage);
		break;
	case MS_BOLT_MANA:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("魔力の矢の呪文を唱えた。");
#else
			else msg_print("You cast a mana bolt.");
#endif
		damage = randint1(plev * 7) + 50;
		fire_bolt(GF_MANA, dir, damage);
		break;
	case MS_BOLT_PLASMA:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("プラズマ・ボルトの呪文を唱えた。");
#else
			else msg_print("You cast a plasma bolt.");
#endif
		damage = 10 + damroll(8, 7) + plev * 2;
		fire_bolt(GF_PLASMA, dir, damage);
		break;
	case MS_BOLT_ICE:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("極寒の矢の呪文を唱えた。");
#else
			else msg_print("You cast a ice bolt.");
#endif
		damage = damroll(6, 6) + plev * 2;
		fire_bolt(GF_ICE, dir, damage);
		break;
	case MS_MAGIC_MISSILE:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("マジック・ミサイルの呪文を唱えた。");
#else
			else msg_print("You cast a magic missile.");
#endif
		damage = damroll(2, 6) + plev * 2 / 3;
		fire_bolt(GF_MISSILE, dir, damage);
		break;
	case MS_SCARE:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("恐ろしげな幻覚を作り出した。");
#else
			else msg_print("You cast a fearful illusion.");
#endif
		fear_monster(dir, plev+10);
		break;
	case MS_BLIND:
		if (!get_aim_dir(&dir)) return FALSE;
		confuse_monster(dir, plev * 2);
		break;
	case MS_CONF:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("誘惑的な幻覚をつくり出した。");
#else
			else msg_print("You cast a mesmerizing illusion.");
#endif
		confuse_monster(dir, plev * 2);
		break;
	case MS_SLOW:
		if (!get_aim_dir(&dir)) return FALSE;
		slow_monster(dir);
		break;
	case MS_SLEEP:
		if (!get_aim_dir(&dir)) return FALSE;
		sleep_monster(dir);
		break;
	case MS_SPEED:
		(void)set_fast(randint1(20 + plev) + plev, FALSE);
		break;
	case MS_HAND_DOOM:
	{
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("<破滅の手>を放った！");
#else
		else msg_print("You invoke the Hand of Doom!");
#endif

		fire_ball_hide(GF_HAND_DOOM, dir, plev * 3, 0);
		break;
	}
	case MS_HEAL:
#ifdef JP
msg_print("自分の傷に念を集中した。");
#else
			msg_print("You concentrate on your wounds!");
#endif
		(void)hp_player(plev*4);
		(void)set_stun(0);
		(void)set_cut(0);
		break;
	case MS_INVULNER:
#ifdef JP
msg_print("無傷の球の呪文を唱えた。");
#else
			msg_print("You cast a Globe of Invulnerability.");
#endif
		(void)set_invuln(randint1(4) + 4, FALSE);
		break;
	case MS_BLINK:
		teleport_player(10, 0L);
		break;
	case MS_TELEPORT:
		teleport_player(plev * 5, 0L);
		break;
	case MS_WORLD:
		world_player = TRUE;
#ifdef JP
		msg_print("「時よ！」");
#else
		msg_print("'Time!'");
#endif
		msg_print(NULL);

		/* Hack */
		p_ptr->energy_need -= 1000 + (100 + randint1(200)+200)*TURNS_PER_TICK/10;

		/* Redraw map */
		p_ptr->redraw |= (PR_MAP);

		/* Update monsters */
		p_ptr->update |= (PU_MONSTERS);

		/* Window stuff */
		p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

		handle_stuff();
		break;
	case MS_SPECIAL:
		break;
	case MS_TELE_TO:
	{
		monster_type *m_ptr;
		monster_race *r_ptr;
		char m_name[80];

		if (!target_set(TARGET_KILL)) return FALSE;
		if (!cave[target_row][target_col].m_idx) break;
		if (!player_has_los_bold(target_row, target_col)) break;
		if (!projectable(py, px, target_row, target_col)) break;
		m_ptr = &m_list[cave[target_row][target_col].m_idx];
		r_ptr = &r_info[m_ptr->r_idx];
		monster_desc(m_name, m_ptr, 0);
		if (r_ptr->flagsr & RFR_RES_TELE)
		{
			if ((r_ptr->flags1 & (RF1_UNIQUE)) || (r_ptr->flagsr & RFR_RES_ALL))
			{
				if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= RFR_RES_TELE;
#ifdef JP
				msg_format("%sには効果がなかった！", m_name);
#else
				msg_format("%s is unaffected!", m_name);
#endif

				break;
			}
			else if (r_ptr->level > randint1(100))
			{
				if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= RFR_RES_TELE;
#ifdef JP
				msg_format("%sには耐性がある！", m_name);
#else
				msg_format("%s resists!", m_name);
#endif

				break;
			}
		}
#ifdef JP
msg_format("%sを引き戻した。", m_name);
#else
		msg_format("You command %s to return.", m_name);
#endif

		teleport_monster_to(cave[target_row][target_col].m_idx, py, px, 100, TELEPORT_PASSIVE);
		break;
	}
	case MS_TELE_AWAY:
		if (!get_aim_dir(&dir)) return FALSE;

		(void)fire_beam(GF_AWAY_ALL, dir, 100);
		break;
	case MS_TELE_LEVEL:
	{
		int target_m_idx;
		monster_type *m_ptr;
		monster_race *r_ptr;
		char m_name[80];

		if (!target_set(TARGET_KILL)) return FALSE;
		target_m_idx = cave[target_row][target_col].m_idx;
		if (!target_m_idx) break;
		if (!player_has_los_bold(target_row, target_col)) break;
		if (!projectable(py, px, target_row, target_col)) break;
		m_ptr = &m_list[target_m_idx];
		r_ptr = &r_info[m_ptr->r_idx];
		monster_desc(m_name, m_ptr, 0);
#ifdef JP
		msg_format("%^sの足を指さした。", m_name);
#else
		msg_format("You gesture at %^s's feet.", m_name);
#endif

		if ((r_ptr->flagsr & (RFR_EFF_RES_NEXU_MASK | RFR_RES_TELE)) ||
			(r_ptr->flags1 & RF1_QUESTOR) || (r_ptr->level + randint1(50) > plev + randint1(60)))
		{
#ifdef JP
			msg_print("しかし効果がなかった！");
#else
			msg_format("%^s is unaffected!", m_name);
#endif
		}
		else teleport_level(target_m_idx);
		break;
	}
	case MS_PSY_SPEAR:
		if (!get_aim_dir(&dir)) return FALSE;

#ifdef JP
else msg_print("光の剣を放った。");
#else
			else msg_print("You throw a psycho-spear.");
#endif
		damage = randint1(plev * 3) + 100;
		(void)fire_beam(GF_PSY_SPEAR, dir, damage);
		break;
	case MS_DARKNESS:
#ifdef JP
msg_print("暗闇の中で手を振った。");
#else
			msg_print("You gesture in shadow.");
#endif
		(void)unlite_area(10, 3);
		break;
	case MS_MAKE_TRAP:
		if (!target_set(TARGET_KILL)) return FALSE;
#ifdef JP
msg_print("呪文を唱えて邪悪に微笑んだ。");
#else
			msg_print("You cast a spell and cackle evilly.");
#endif
		trap_creation(target_row, target_col);
		break;
	case MS_FORGET:
#ifdef JP
msg_print("しかし何も起きなかった。");
#else
			msg_print("Nothing happen.");
#endif
		break;
	case MS_RAISE_DEAD:
#ifdef JP
msg_print("死者復活の呪文を唱えた。");
#else
		msg_print("You cast a animate dead.");
#endif
		(void)animate_dead(0, py, px);
		break;
	case MS_S_KIN:
	{
		int k;
#ifdef JP
msg_print("援軍を召喚した。");
#else
			msg_print("You summon minions.");
#endif
		for (k = 0;k < 1; k++)
		{
			if (summon_kin_player(summon_lev, py, px, (pet ? PM_FORCE_PET : 0L)))
			{
				if (!pet)
#ifdef JP
msg_print("召喚された仲間は怒っている！");
#else
msg_print("Summoned fellows are angry!");
#endif
			}
			else
			{
				no_trump = TRUE;
			}
		}
		break;
	}
	case MS_S_CYBER:
	{
		int k;
#ifdef JP
msg_print("サイバーデーモンを召喚した！");
#else
			msg_print("You summon a Cyberdemon!");
#endif
		for (k = 0 ;k < 1 ; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_CYBER, p_mode))
			{
				if (!pet)
#ifdef JP
msg_print("召喚されたサイバーデーモンは怒っている！");
#else
msg_print("The summoned Cyberdemon are angry!");
#endif
			}
			else
			{
				no_trump = TRUE;
			}
		break;
	}
	case MS_S_MONSTER:
	{
		int k;
#ifdef JP
msg_print("仲間を召喚した。");
#else
			msg_print("You summon help.");
#endif
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, 0, p_mode))
			{
				if (!pet)
#ifdef JP
msg_print("召喚されたモンスターは怒っている！");
#else
msg_print("The summoned monster is angry!");
#endif
			}
			else
			{
				no_trump = TRUE;
			}
		break;
	}
	case MS_S_MONSTERS:
	{
		int k;
#ifdef JP
msg_print("モンスターを召喚した！");
#else
			msg_print("You summon monsters!");
#endif
		for (k = 0;k < plev / 15 + 2; k++)
			if(summon_specific((pet ? -1 : 0), py, px, summon_lev, 0, (p_mode | u_mode)))
			{
				if (!pet)
#ifdef JP
msg_print("召喚されたモンスターは怒っている！");
#else
msg_print("Summoned monsters are angry!");
#endif
			}
			else
			{
				no_trump = TRUE;
			}
		break;
	}
	case MS_S_ANT:
	{
		int k;
#ifdef JP
msg_print("アリを召喚した。");
#else
			msg_print("You summon ants.");
#endif
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_ANT, (PM_ALLOW_GROUP | p_mode)))
			{
				if (!pet)
#ifdef JP
msg_print("召喚されたアリは怒っている！");
#else
msg_print("Summoned ants are angry!");
#endif
			}
			else
			{
				no_trump = TRUE;
			}
		break;
	}
	case MS_S_SPIDER:
	{
		int k;
#ifdef JP
msg_print("蜘蛛を召喚した。");
#else
			msg_print("You summon spiders.");
#endif
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_SPIDER, (PM_ALLOW_GROUP | p_mode)))
			{
				if (!pet)
#ifdef JP
msg_print("召喚された蜘蛛は怒っている！");
#else
msg_print("Summoned spiders are angry!");
#endif
			}
			else
			{
				no_trump = TRUE;
			}
		break;
	}
	case MS_S_HOUND:
	{
		int k;
#ifdef JP
msg_print("ハウンドを召喚した。");
#else
			msg_print("You summon hounds.");
#endif
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_HOUND, (PM_ALLOW_GROUP | p_mode)))
			{
				if (!pet)
#ifdef JP
msg_print("召喚されたハウンドは怒っている！");
#else
msg_print("Summoned hounds are angry!");
#endif
			}
			else
			{
				no_trump = TRUE;
			}
		break;
	}
	case MS_S_HYDRA:
	{
		int k;
#ifdef JP
msg_print("ヒドラを召喚した。");
#else
			msg_print("You summon a hydras.");
#endif
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_HYDRA, (g_mode | p_mode)))
			{
				if (!pet)
#ifdef JP
msg_print("召喚されたヒドラは怒っている！");
#else
msg_print("Summoned hydras are angry!");
#endif
			}
			else
			{
				no_trump = TRUE;
			}
		break;
	}
	case MS_S_ANGEL:
	{
		int k;
#ifdef JP
msg_print("天使を召喚した！");
#else
			msg_print("You summon an angel!");
#endif
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_ANGEL, (g_mode | p_mode)))
			{
				if (!pet)
#ifdef JP
msg_print("召喚された天使は怒っている！");
#else
msg_print("Summoned angels are angry!");
#endif
			}
			else
			{
				no_trump = TRUE;
			}
		break;
	}
	case MS_S_DEMON:
	{
		int k;
#ifdef JP
msg_print("混沌の宮廷から悪魔を召喚した！");
#else
			msg_print("You summon a demon from the Courts of Chaos!");
#endif
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_DEMON, (g_mode | p_mode)))
			{
				if (!pet)
#ifdef JP
msg_print("召喚されたデーモンは怒っている！");
#else
msg_print("Summoned demons are angry!");
#endif
			}
			else
			{
				no_trump = TRUE;
			}
		break;
	}
	case MS_S_UNDEAD:
	{
		int k;
#ifdef JP
msg_print("アンデッドの強敵を召喚した！");
#else
			msg_print("You summon an undead adversary!");
#endif
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_UNDEAD, (g_mode | p_mode)))
			{
				if (!pet)
#ifdef JP
msg_print("召喚されたアンデッドは怒っている！");
#else
msg_print("Summoned undeads are angry!");
#endif
			}
			else
			{
				no_trump = TRUE;
			}
		break;
	}
	case MS_S_DRAGON:
	{
		int k;
#ifdef JP
msg_print("ドラゴンを召喚した！");
#else
			msg_print("You summon a dragon!");
#endif
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_DRAGON, (g_mode | p_mode)))
			{
				if (!pet)
#ifdef JP
msg_print("召喚されたドラゴンは怒っている！");
#else
msg_print("Summoned dragons are angry!");
#endif
			}
			else
			{
				no_trump = TRUE;
			}
		break;
	}
	case MS_S_HI_UNDEAD:
	{
		int k;
#ifdef JP
msg_print("強力なアンデッドを召喚した！");
#else
			msg_print("You summon a greater undead!");
#endif
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_HI_UNDEAD, (g_mode | p_mode | u_mode)))
			{
				if (!pet)
#ifdef JP
msg_print("召喚された上級アンデッドは怒っている！");
#else
msg_print("Summoned greater undeads are angry!");
#endif
			}
			else
			{
				no_trump = TRUE;
			}
		break;
	}
	case MS_S_HI_DRAGON:
	{
		int k;
#ifdef JP
msg_print("古代ドラゴンを召喚した！");
#else
			msg_print("You summon an ancient dragon!");
#endif
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_HI_DRAGON, (g_mode | p_mode | u_mode)))
			{
				if (!pet)
#ifdef JP
msg_print("召喚された古代ドラゴンは怒っている！");
#else
msg_print("Summoned ancient dragons are angry!");
#endif
			}
			else
			{
				no_trump = TRUE;
			}
		break;
	}
	case MS_S_AMBERITE:
	{
		int k;
#ifdef JP
msg_print("アンバーの王族を召喚した！");
#else
			msg_print("You summon a Lord of Amber!");
#endif
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_AMBERITES, (g_mode | p_mode | u_mode)))
			{
				if (!pet)
#ifdef JP
msg_print("召喚されたアンバーの王族は怒っている！");
#else
msg_print("Summoned Lords of Amber are angry!");
#endif
			}
			else
			{
				no_trump = TRUE;
			}
		break;
	}
	case MS_S_UNIQUE:
	{
		int k, count = 0;
#ifdef JP
msg_print("特別な強敵を召喚した！");
#else
			msg_print("You summon a special opponent!");
#endif
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_UNIQUE, (g_mode | p_mode | PM_ALLOW_UNIQUE)))
			{
				count++;
				if (!pet)
#ifdef JP
msg_print("召喚されたユニーク・モンスターは怒っている！");
#else
msg_print("Summoned special opponents are angry!");
#endif
			}
		for (k = count;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_HI_UNDEAD, (g_mode | p_mode | PM_ALLOW_UNIQUE)))
			{
				count++;
				if (!pet)
#ifdef JP
msg_print("召喚された上級アンデッドは怒っている！");
#else
msg_print("Summoned greater undeads are angry!");
#endif
			}
		if (!count)
		{
			no_trump = TRUE;
		}
		break;
	}
	default:
		msg_print("hoge?");
	}
	if (no_trump)
	{
#ifdef JP
msg_print("何も現れなかった。");
#else
msg_print("No one have appeared.");
#endif
	}

	return TRUE;
}


/*
 * do_cmd_cast calls this function if the player's class
 * is 'Blue-Mage'.
 */
bool do_cmd_cast_learned(void)
{
	int             n = 0;
	int             chance;
	int             minfail = 0;
	int             plev = p_ptr->lev;
	monster_power   spell;
	bool            cast;
	int             need_mana;


	/* not if confused */
	if (p_ptr->confused)
	{
#ifdef JP
msg_print("混乱していて唱えられない！");
#else
		msg_print("You are too confused!");
#endif

		return TRUE;
	}

	/* get power */
	if (!get_learned_power(&n)) return FALSE;

	spell = monster_powers[n];

	need_mana = mod_need_mana(spell.smana, 0, REALM_NONE);

	/* Verify "dangerous" spells */
	if (need_mana > p_ptr->csp)
	{
		/* Warning */
#ifdef JP
msg_print("ＭＰが足りません。");
#else
		msg_print("You do not have enough mana to use this power.");
#endif


		if (!over_exert) return FALSE;

		/* Verify */
#ifdef JP
if (!get_check("それでも挑戦しますか? ")) return FALSE;
#else
		if (!get_check("Attempt it anyway? ")) return FALSE;
#endif

	}

	/* Spell failure chance */
	chance = spell.fail;

	/* Reduce failure rate by "effective" level adjustment */
	if (plev > spell.level) chance -= 3 * (plev - spell.level);
	else chance += (spell.level - plev);

	/* Reduce failure rate by INT/WIS adjustment */
	chance -= 3 * (adj_mag_stat[p_ptr->stat_ind[A_INT]] - 1);

	chance = mod_spell_chance_1(chance);

	/* Not enough mana to cast */
	if (need_mana > p_ptr->csp)
	{
		chance += 5 * (need_mana - p_ptr->csp);
	}

	/* Extract the minimum failure rate */
	minfail = adj_mag_fail[p_ptr->stat_ind[A_INT]];

	/* Minimum failure rate */
	if (chance < minfail) chance = minfail;

	/* Stunning makes spells harder */
	if (p_ptr->stun > 50) chance += 25;
	else if (p_ptr->stun) chance += 15;

	/* Always a 5 percent chance of working */
	if (chance > 95) chance = 95;

	chance = mod_spell_chance_2(chance);

	/* Failed spell */
	if (randint0(100) < chance)
	{
		if (flush_failure) flush();
#ifdef JP
msg_print("魔法をうまく唱えられなかった。");
#else
		msg_print("You failed to concentrate hard enough!");
#endif

		sound(SOUND_FAIL);

		if (n >= MS_S_KIN)
			/* Cast the spell */
			cast = cast_learned_spell(n, FALSE);
	}
	else
	{
		sound(SOUND_ZAP);

		/* Cast the spell */
		cast = cast_learned_spell(n, TRUE);

		if (!cast) return FALSE;
	}

	/* Sufficient mana */
	if (need_mana <= p_ptr->csp)
	{
		/* Use some mana */
		p_ptr->csp -= need_mana;
	}
	else
	{
		int oops = need_mana;

		/* No mana left */
		p_ptr->csp = 0;
		p_ptr->csp_frac = 0;

		/* Message */
#ifdef JP
msg_print("精神を集中しすぎて気を失ってしまった！");
#else
		msg_print("You faint from the effort!");
#endif


		/* Hack -- Bypass free action */
		(void)set_paralyzed(p_ptr->paralyzed + randint1(5 * oops + 1));

		chg_virtue(V_KNOWLEDGE, -10);

		/* Damage CON (possibly permanently) */
		if (randint0(100) < 50)
		{
			bool perm = (randint0(100) < 25);

			/* Message */
#ifdef JP
msg_print("体を悪くしてしまった！");
#else
			msg_print("You have damaged your health!");
#endif


			/* Reduce constitution */
			(void)dec_stat(A_CON, 15 + randint1(10), perm);
		}
	}

	/* Take a turn */
	energy_use = 100;

	/* Window stuff */
	p_ptr->redraw |= (PR_MANA);
	p_ptr->window |= (PW_PLAYER);
	p_ptr->window |= (PW_SPELL);

	return TRUE;
}

void learn_spell(int monspell)
{
	if (p_ptr->action != ACTION_LEARN) return;
	if (monspell < 0) return; /* Paranoia */
	if (p_ptr->magic_num2[monspell]) return;
	if (p_ptr->confused || p_ptr->blind || p_ptr->image || p_ptr->stun || p_ptr->paralyzed) return;
	if (randint1(p_ptr->lev + 70) > monster_powers[monspell].level + 40)
	{
		p_ptr->magic_num2[monspell] = 1;
#ifdef JP
		msg_format("%sを学習した！", monster_powers[monspell].name);
#else
		msg_format("You have learned %s!", monster_powers[monspell].name);
#endif
		gain_exp(monster_powers[monspell].level * monster_powers[monspell].smana);

		/* Sound */
		sound(SOUND_STUDY);

		new_mane = TRUE;
		p_ptr->redraw |= (PR_STATE);
	}
}


/*
 * Extract monster spells mask for the given mode
 */
void set_rf_masks(s32b *f4, s32b *f5, s32b *f6, int mode)
{
	switch (mode)
	{
		case MONSPELL_TYPE_BOLT:
			*f4 = ((RF4_BOLT_MASK | RF4_BEAM_MASK) & ~(RF4_ROCKET));
			*f5 = RF5_BOLT_MASK | RF5_BEAM_MASK;
			*f6 = RF6_BOLT_MASK | RF6_BEAM_MASK;
			break;

		case MONSPELL_TYPE_BALL:
			*f4 = (RF4_BALL_MASK & ~(RF4_BREATH_MASK));
			*f5 = (RF5_BALL_MASK & ~(RF5_BREATH_MASK));
			*f6 = (RF6_BALL_MASK & ~(RF6_BREATH_MASK));
			break;

		case MONSPELL_TYPE_BREATH:
			*f4 = RF4_BREATH_MASK;
			*f5 = RF5_BREATH_MASK;
			*f6 = RF6_BREATH_MASK;
			break;

		case MONSPELL_TYPE_SUMMON:
			*f4 = RF4_SUMMON_MASK;
			*f5 = RF5_SUMMON_MASK;
			*f6 = RF6_SUMMON_MASK;
			break;

		case MONSPELL_TYPE_OTHER:
			*f4 = RF4_ATTACK_MASK & ~(RF4_BOLT_MASK | RF4_BEAM_MASK | RF4_BALL_MASK | RF4_INDIRECT_MASK);
			*f5 = RF5_ATTACK_MASK & ~(RF5_BOLT_MASK | RF5_BEAM_MASK | RF5_BALL_MASK | RF5_INDIRECT_MASK);
			*f6 = RF6_ATTACK_MASK & ~(RF6_BOLT_MASK | RF6_BEAM_MASK | RF6_BALL_MASK | RF6_INDIRECT_MASK);
			break;
	}

	return;
}
