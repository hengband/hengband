/* File: mind.c */

/* Purpose: Mindcrafter code */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

#include "angband.h"

#define TECHNIC_HISSATSU (REALM_HISSATSU - MIN_TECHNIC)

/*
 * Allow user to choose a mindcrafter power.
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
static int get_hissatsu_power(int *sn)
{
	int             i, j = 0;
	int             num = 0;
	int             y = 1;
	int             x = 15;
	int             plev = p_ptr->lev;
	int             ask = TRUE;
	char            choice;
	char            out_val[160];
	char sentaku[32];
#ifdef JP
cptr            p = "必殺剣";
#else
	cptr            p = "special attack";
#endif

	magic_type spell;
	bool            flag, redraw;
	int menu_line = (use_menu ? 1 : 0);

	/* Assume cancelled */
	*sn = (-1);

#ifdef ALLOW_REPEAT /* TNB */

	/* Get the spell, if available */
	if (repeat_pull(sn))
	{
		/* Verify the spell */
		if (technic_info[TECHNIC_HISSATSU][*sn].slevel <= plev)
		{
			/* Success */
			return (TRUE);
		}
	}

#endif /* ALLOW_REPEAT -- TNB */

	/* Nothing chosen yet */
	flag = FALSE;

	/* No redraw yet */
	redraw = FALSE;

	for (i = 0; i < 32; i++)
	{
		if (technic_info[TECHNIC_HISSATSU][i].slevel <= PY_MAX_LEVEL)
		{
			sentaku[num] = i;
			num++;
		}
	}

	/* Build a prompt (accept all spells) */
	(void) strnfmt(out_val, 78, 
#ifdef JP
		       "(%^s %c-%c, '*'で一覧, ESC) どの%sを使いますか？",
#else
		       "(%^ss %c-%c, *=List, ESC=exit) Use which %s? ",
#endif
		       p, I2A(0), "abcdefghijklmnopqrstuvwxyz012345"[num-1], p);

	if (use_menu) screen_save();

	/* Get a spell from the user */

	choice= always_show_list ? ESCAPE:1 ;
	while (!flag)
	{
		if(choice==ESCAPE) choice = ' '; 
		else if( !get_com(out_val, &choice, FALSE) )break;

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
						menu_line += 31;
						if (menu_line > 32) menu_line -= 32;
					} while(!(p_ptr->spell_learned1 & (1L << (menu_line-1))));
					break;
				}

				case '2':
				case 'j':
				case 'J':
				{
					do
					{
						menu_line++;
						if (menu_line > 32) menu_line -= 32;
					} while(!(p_ptr->spell_learned1 & (1L << (menu_line-1))));
					break;
				}

				case '4':
				case 'h':
				case 'H':
				case '6':
				case 'l':
				case 'L':
				{
					bool reverse = FALSE;
					if ((choice == '4') || (choice == 'h') || (choice == 'H')) reverse = TRUE;
					if (menu_line > 16)
					{
						menu_line -= 16;
						reverse = TRUE;
					}
					else menu_line+=16;
					while(!(p_ptr->spell_learned1 & (1L << (menu_line-1))))
					{
						if (reverse)
						{
							menu_line--;
							if (menu_line < 2) reverse = FALSE;
						}
						else
						{
							menu_line++;
							if (menu_line > 31) reverse = TRUE;
						}
					}
					break;
				}

				case 'x':
				case 'X':
				case '\r':
				case '\n':
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
				int line;

				/* Show list */
				redraw = TRUE;

				/* Save the screen */
				if (!use_menu) screen_save();

				/* Display a list of spells */
				prt("", y, x);
#ifdef JP
put_str("名前              Lv  MP      名前              Lv  MP ", y, x + 5);
#else
put_str("name              Lv  SP      name              Lv  SP ", y, x + 5);
#endif
				prt("", y+1, x);
				/* Dump the spells */
				for (i = 0, line = 0; i < 32; i++)
				{
					spell = technic_info[TECHNIC_HISSATSU][i];

					if (spell.slevel > PY_MAX_LEVEL) continue;
					line++;
					if (!(p_ptr->spell_learned1 >> i)) break;

					/* Access the spell */
					if (spell.slevel > plev)   continue;
					if (!(p_ptr->spell_learned1 & (1L << i))) continue;
					if (use_menu)
					{
						if (i == (menu_line-1))
#ifdef JP
							strcpy(psi_desc, "  》");
#else
							strcpy(psi_desc, "  > ");
#endif
						else strcpy(psi_desc, "    ");
						
					}
					else
					{
						char letter;
						if (line <= 26)
							letter = I2A(line-1);
						else
							letter = '0' + line - 27;
						sprintf(psi_desc, "  %c)",letter);
					}

					/* Dump the spell --(-- */
					strcat(psi_desc, format(" %-18s%2d %3d",
						do_spell(REALM_HISSATSU, i, SPELL_NAME),
						spell.slevel, spell.smana));
					prt(psi_desc, y + (line%17) + (line >= 17), x+(line/17)*30);
					prt("", y + (line%17) + (line >= 17) + 1, x+(line/17)*30);
				}
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
			if (isalpha(choice))
			{
				/* Note verify */
				ask = (isupper(choice));

				/* Lowercase */
				if (ask) choice = tolower(choice);

				/* Extract request */
				i = (islower(choice) ? A2I(choice) : -1);
			}
			else
			{
				ask = FALSE; /* Can't uppercase digits */

				i = choice - '0' + 26;
			}
		}

		/* Totally Illegal */
		if ((i < 0) || (i >= 32) || !(p_ptr->spell_learned1 & (1 << sentaku[i])))
		{
			bell();
			continue;
		}

		j = sentaku[i];

		/* Verify it */
		if (ask)
		{
			char tmp_val[160];

			/* Prompt */
#ifdef JP
			(void) strnfmt(tmp_val, 78, "%sを使いますか？", do_spell(REALM_HISSATSU, j, SPELL_NAME));
#else
			(void)strnfmt(tmp_val, 78, "Use %s? ", do_spell(REALM_HISSATSU, j, SPELL_NAME));
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
	(*sn) = j;

#ifdef ALLOW_REPEAT /* TNB */

	repeat_push(*sn);

#endif /* ALLOW_REPEAT -- TNB */

	/* Success */
	return (TRUE);
}


/*
 * do_cmd_cast calls this function if the player's class
 * is 'mindcrafter'.
 */
void do_cmd_hissatsu(void)
{
	int             n = 0;
	magic_type      spell;


	/* not if confused */
	if (p_ptr->confused)
	{
#ifdef JP
msg_print("混乱していて集中できない！");
#else
		msg_print("You are too confused!");
#endif

		return;
	}
	if (!buki_motteruka(INVEN_RARM) && !buki_motteruka(INVEN_LARM))
	{
		if (flush_failure) flush();
#ifdef JP
msg_print("武器を持たないと必殺技は使えない！");
#else
		msg_print("You need to wield a weapon!");
#endif

		return;
	}
	if (!p_ptr->spell_learned1)
	{
#ifdef JP
msg_print("何も技を知らない。");
#else
		msg_print("You don't know any special attacks.");
#endif

		return;
	}

	if (p_ptr->special_defense & KATA_MASK)
	{
		set_action(ACTION_NONE);
	}

	/* get power */
	if (!get_hissatsu_power(&n)) return;

	spell = technic_info[TECHNIC_HISSATSU][n];

	/* Verify "dangerous" spells */
	if (spell.smana > p_ptr->csp)
	{
		if (flush_failure) flush();
		/* Warning */
#ifdef JP
msg_print("ＭＰが足りません。");
#else
		msg_print("You do not have enough mana to use this power.");
#endif
		msg_print(NULL);
		return;
	}

	sound(SOUND_ZAP);

	/* Cast the spell */
	if (!do_spell(REALM_HISSATSU, n, SPELL_CAST)) return;

	/* Take a turn */
	energy_use = 100;

	/* Use some mana */
	p_ptr->csp -= spell.smana;

	/* Limit */
	if (p_ptr->csp < 0) p_ptr->csp = 0;

	/* Redraw mana */
	p_ptr->redraw |= (PR_MANA);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);
	p_ptr->window |= (PW_SPELL);
}


void do_cmd_gain_hissatsu(void)
{
	int item, i, j;

	object_type *o_ptr;
	cptr q, s;

	bool gain = FALSE;

	if (p_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN))
	{
		set_action(ACTION_NONE);
	}

	if (p_ptr->blind || no_lite())
	{
#ifdef JP
msg_print("目が見えない！");
#else
		msg_print("You cannot see!");
#endif

		return;
	}

	if (p_ptr->confused)
	{
#ifdef JP
msg_print("混乱していて読めない！");
#else
		msg_print("You are too confused!");
#endif

		return;
	}

	if (!(p_ptr->new_spells))
	{
#ifdef JP
msg_print("新しい必殺技を覚えることはできない！");
#else
		msg_print("You cannot learn any new special attacks!");
#endif

		return;
	}

#ifdef JP
	if( p_ptr->new_spells < 10 ){
		msg_format("あと %d つの必殺技を学べる。", p_ptr->new_spells);
	}else{
		msg_format("あと %d 個の必殺技を学べる。", p_ptr->new_spells);
	}
#else
	msg_format("You can learn %d new special attack%s.", p_ptr->new_spells,
		(p_ptr->new_spells == 1?"":"s"));
#endif

	item_tester_tval = TV_HISSATSU_BOOK;

	/* Get an item */
#ifdef JP
q = "どの書から学びますか? ";
#else
	q = "Study which book? ";
#endif

#ifdef JP
s = "読める書がない。";
#else
	s = "You have no books that you can read.";
#endif

	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return;

	/* Get the item (in the pack) */
	if (item >= 0)
	{
		o_ptr = &inventory[item];
	}

	/* Get the item (on the floor) */
	else
	{
		o_ptr = &o_list[0 - item];
	}

	for (i = o_ptr->sval * 8; i < o_ptr->sval * 8 + 8; i++)
	{
		if (p_ptr->spell_learned1 & (1L << i)) continue;
		if (technic_info[TECHNIC_HISSATSU][i].slevel > p_ptr->lev) continue;

		p_ptr->spell_learned1 |= (1L << i);
		p_ptr->spell_worked1 |= (1L << i);
#ifdef JP
		msg_format("%sの技を覚えた。", do_spell(REALM_HISSATSU, i, SPELL_NAME));
#else
		msg_format("You have learned the special attack of %s.", do_spell(REALM_HISSATSU, i, SPELL_NAME));
#endif
		for (j = 0; j < 64; j++)
		{
			/* Stop at the first empty space */
			if (p_ptr->spell_order[j] == 99) break;
		}
		p_ptr->spell_order[j] = i;
		gain = TRUE;
	}

	/* No gain ... */
	if (!gain)
#ifdef JP
		msg_print("何も覚えられなかった。");
#else
		msg_print("You were not able to learn any special attacks.");
#endif

	/* Take a turn */
	else
		energy_use = 100;

	p_ptr->update |= (PU_SPELLS);
}
