/* File: cmd5.c */

/* Purpose: Spell/Prayer commands */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "angband.h"

#include "spellstips.h"

cptr spell_categoly_name(int tval)
{
	switch (tval)
	{
#ifdef JP
	case TV_HISSATSU_BOOK:
		return "必殺技";
	case TV_LIFE_BOOK:
		return "祈り";
	case TV_MUSIC_BOOK:
		return "歌";
	default:
		return "呪文";
#else
	case TV_HISSATSU_BOOK:
		return "arts";
	case TV_LIFE_BOOK:
		return "prayer";
	case TV_MUSIC_BOOK:
		return "song";
	default:
		return "spell";
#endif
	}
}

/*
 * Allow user to choose a spell/prayer from the given book.
 *
 * If a valid spell is chosen, saves it in '*sn' and returns TRUE
 * If the user hits escape, returns FALSE, and set '*sn' to -1
 * If there are no legal choices, returns FALSE, and sets '*sn' to -2
 *
 * The "prompt" should be "cast", "recite", or "study"
 * The "known" should be TRUE for cast/pray, FALSE for study
 */

bool select_spellbook=FALSE;
bool select_the_force=FALSE;

static int get_spell(int *sn, cptr prompt, int sval, bool learned, int use_realm)
{
	int         i;
	int         spell = -1;
	int         num = 0;
	int         ask = TRUE;
	int         shouhimana;
	byte        spells[64];
	bool        flag, redraw, okay;
	char        choice;
	magic_type  *s_ptr;
	char        out_val[160];
	cptr        p;
#ifdef JP
        char jverb_buf[128];
#endif
	int menu_line = (use_menu ? 1 : 0);

#ifdef ALLOW_REPEAT /* TNB */

	/* Get the spell, if available */
	if (repeat_pull(sn))
	{
		/* Verify the spell */
		if (spell_okay(*sn, learned, FALSE, use_realm))
		{
			/* Success */
			return (TRUE);
		}
	}

#endif /* ALLOW_REPEAT -- TNB */

	p = spell_categoly_name(mp_ptr->spell_book);

	/* Extract spells */
	for (spell = 0; spell < 32; spell++)
	{
		/* Check for this spell */
		if ((fake_spell_flags[sval] & (1L << spell)))
		{
			/* Collect this spell */
			spells[num++] = spell;
		}
	}

	/* Assume no usable spells */
	okay = FALSE;

	/* Assume no spells available */
	(*sn) = -2;

	/* Check for "okay" spells */
	for (i = 0; i < num; i++)
	{
		/* Look for "okay" spells */
		if (spell_okay(spells[i], learned, FALSE, use_realm)) okay = TRUE;
	}

	/* No "okay" spells */
	if (!okay) return (FALSE);
	if (((use_realm) != p_ptr->realm1) && ((use_realm) != p_ptr->realm2) && (p_ptr->pclass != CLASS_SORCERER) && (p_ptr->pclass != CLASS_RED_MAGE)) return FALSE;
	if (((p_ptr->pclass == CLASS_SORCERER) || (p_ptr->pclass == CLASS_RED_MAGE)) && !is_magic(use_realm)) return FALSE;
	if ((p_ptr->pclass == CLASS_RED_MAGE) && ((use_realm) != REALM_ARCANE) && (sval > 1)) return FALSE;

	/* Assume cancelled */
	*sn = (-1);

	/* Nothing chosen yet */
	flag = FALSE;

	/* No redraw yet */
	redraw = FALSE;

	/* Show choices */
	if (show_choices)
	{
		/* Update */
		p_ptr->window |= (PW_SPELL);

		/* Window stuff */
		window_stuff();
	}

	/* Build a prompt (accept all spells) */
#ifdef JP
	jverb1( prompt, jverb_buf );
	(void) strnfmt(out_val, 78, "(%^s:%c-%c, '*'で一覧, ESCで中断) どの%sを%^sますか? ",
	        p, I2A(0), I2A(num - 1), p, jverb_buf );
#else
	(void)strnfmt(out_val, 78, "(%^ss %c-%c, *=List, ESC=exit) %^s which %s? ",
		p, I2A(0), I2A(num - 1), prompt, p);
#endif

	/* Get a spell from the user */

        choice = (always_show_list || use_menu) ? ESCAPE:1;
        while (!flag)
        {
		if( choice==ESCAPE ) choice = ' '; 
		else if( !get_com(out_val, &choice, TRUE) )break; 

		if (use_menu && choice != ' ')
		{
			switch(choice)
			{
				case '0':
				{
					screen_load();
					return (FALSE);
					break;
				}

				case '8':
				case 'k':
				case 'K':
				{
					menu_line += (num - 1);
					break;
				}

				case '2':
				case 'j':
				case 'J':
				{
					menu_line++;
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
			if (menu_line > num) menu_line -= num;
			/* Display a list of spells */
			print_spells(menu_line, spells, num, 1, 15, use_realm);
			if (ask) continue;
		}
		else
		{
			/* Request redraw */
			if ((choice == ' ') || (choice == '*') || (choice == '?'))
			{
				/* Show the list */
				if (!redraw)
				{
					/* Show list */
					redraw = TRUE;

					/* Save the screen */
					screen_save();

					/* Display a list of spells */
					print_spells(menu_line, spells, num, 1, 15, use_realm);
				}

				/* Hide the list */
				else
				{
					if (use_menu) continue;

					/* Hide list */
					redraw = FALSE;

					/* Restore the screen */
					screen_load();
				}

				/* Redo asking */
				continue;
			}


			/* Note verify */
			ask = (isupper(choice));

			/* Lowercase */
			if (ask) choice = tolower(choice);

			/* Extract request */
			i = (islower(choice) ? A2I(choice) : -1);
		}

		/* Totally Illegal */
		if ((i < 0) || (i >= num))
		{
			bell();
			continue;
		}

		/* Save the spell index */
		spell = spells[i];

		/* Require "okay" spells */
		if (!spell_okay(spell, learned, FALSE, use_realm))
		{
			bell();
#ifdef JP
                        msg_format("その%sを%sことはできません。", p, prompt);
#else
			msg_format("You may not %s that %s.", prompt, p);
#endif

			continue;
		}

		/* Verify it */
		if (ask)
		{
			char tmp_val[160];

			/* Access the spell */
			if (!is_magic(use_realm))
			{
				s_ptr = &technic_info[use_realm - MIN_TECHNIC][spell];
			}
			else
			{
				s_ptr = &mp_ptr->info[use_realm - 1][spell];
			}

			if (use_realm == REALM_HISSATSU)
			{
				shouhimana = s_ptr->smana;
			}
			else
			{
				/* Extract mana consumption rate */
				shouhimana = s_ptr->smana*(3800 - experience_of_spell(spell, use_realm)) + 2399;
				if(p_ptr->dec_mana)
					shouhimana *= 3;
				else shouhimana *= 4;
				shouhimana /= 9600;
				if(shouhimana < 1) shouhimana = 1;
			}

			/* Prompt */
#ifdef JP
			jverb1( prompt, jverb_buf );
                        /* 英日切り替え機能に対応 */
                        (void) strnfmt(tmp_val, 78, "%s(MP%d, 失敗率%d%%)を%sますか? ",
                                spell_names[technic2magic(use_realm)-1][spell], shouhimana,
				       spell_chance(spell, use_realm),jverb_buf);
#else
			(void)strnfmt(tmp_val, 78, "%^s %s (%d mana, %d%% fail)? ",
				prompt, spell_names[technic2magic(use_realm)-1][spell], shouhimana,
				spell_chance(spell, use_realm));
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
	if (show_choices)
	{
		/* Update */
		p_ptr->window |= (PW_SPELL);

		/* Window stuff */
		window_stuff();
	}


	/* Abort if needed */
	if (!flag) return (FALSE);

	/* Save the choice */
	(*sn) = spell;

#ifdef ALLOW_REPEAT /* TNB */

	repeat_push(*sn);

#endif /* ALLOW_REPEAT -- TNB */

	/* Success */
	return (TRUE);
}


static bool item_tester_learn_spell(object_type *o_ptr)
{
	s32b choices = realm_choices2[p_ptr->pclass];

	if (p_ptr->pclass == CLASS_PRIEST)
	{
		if (is_good_realm(p_ptr->realm1))
		{
			choices &= ~(CH_DEATH | CH_DAEMON);
		}
		else
		{
			choices &= ~(CH_LIFE | CH_CRUSADE);
		}
	}

	if ((o_ptr->tval < TV_LIFE_BOOK) || (o_ptr->tval > (TV_LIFE_BOOK + MAX_REALM - 1))) return (FALSE);
	if ((o_ptr->tval == TV_MUSIC_BOOK) && (p_ptr->pclass == CLASS_BARD)) return (TRUE);
	else if (!is_magic(tval2realm(o_ptr->tval))) return FALSE;
	if ((REALM1_BOOK == o_ptr->tval) || (REALM2_BOOK == o_ptr->tval)) return (TRUE);
	if (choices & (0x0001 << (tval2realm(o_ptr->tval) - 1))) return (TRUE);
	return (FALSE);
}


/*
 * Peruse the spells/prayers in a book
 *
 * Note that *all* spells in the book are listed
 *
 * Note that browsing is allowed while confused or blind,
 * and in the dark, primarily to allow browsing in stores.
 */
void do_cmd_browse(void)
{
	int		item, sval, use_realm = 0, j, line;
	int		spell = -1;
	int		num = 0;
	int             increment = 0;

	byte		spells[64];
	char            temp[62*4];

	object_type	*o_ptr;
	magic_type      *s_ptr;

	cptr q, s;

	/* Warriors are illiterate */
	if (!(p_ptr->realm1 || p_ptr->realm2) && (p_ptr->pclass != CLASS_SORCERER) && (p_ptr->pclass != CLASS_RED_MAGE))
	{
#ifdef JP
msg_print("本を読むことができない！");
#else
		msg_print("You cannot read books!");
#endif

		return;
	}

	if (p_ptr->special_defense & KATA_MUSOU)
	{
		set_action(ACTION_NONE);
	}

	/* Restrict choices to "useful" books */
	if (p_ptr->realm2 == REALM_NONE) item_tester_tval = mp_ptr->spell_book;
	else item_tester_hook = item_tester_learn_spell;

	/* Get an item */
#ifdef JP
q = "どの本を読みますか? ";
#else
	q = "Browse which book? ";
#endif

#ifdef JP
s = "読める本がない。";
#else
	s = "You have no books that you can read.";
#endif

        select_spellbook=TRUE;
	if (p_ptr->pclass == CLASS_FORCETRAINER)
		select_the_force = TRUE;
	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))){
            select_spellbook = FALSE;
	    select_the_force = FALSE;
            return;
        }
        select_spellbook = FALSE;
	select_the_force = FALSE;

	if (item == 1111) { /* the_force */
	    do_cmd_mind_browse();
	    return;
	} else
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

	/* Access the item's sval */
	sval = o_ptr->sval;

	use_realm = tval2realm(o_ptr->tval);
	if ((p_ptr->pclass != CLASS_SORCERER) && (p_ptr->pclass != CLASS_RED_MAGE) && is_magic(use_realm))
	{
		if (o_ptr->tval == REALM2_BOOK) increment = 32;
		else if (o_ptr->tval != REALM1_BOOK) increment = 64;
	}

	/* Track the object kind */
	object_kind_track(o_ptr->k_idx);

	/* Hack -- Handle stuff */
	handle_stuff();


	/* Extract spells */
	for (spell = 0; spell < 32; spell++)
	{
		/* Check for this spell */
		if ((fake_spell_flags[sval] & (1L << spell)))
		{
			/* Collect this spell */
			spells[num++] = spell;
		}
	}


	/* Save the screen */
	screen_save();

	/* Clear the top line */
	prt("", 0, 0);

	/* Keep browsing spells.  Exit browsing on cancel. */
	while(TRUE)
	{
		/* Ask for a spell, allow cancel */
#ifdef JP
		if (!get_spell(&spell, "読む", o_ptr->sval, TRUE, use_realm))
#else
		if (!get_spell(&spell, "browse", o_ptr->sval, TRUE, use_realm))
#endif
		{
			/* If cancelled, leave immediately. */
			if (spell == -1) break;

			/* Display a list of spells */
			print_spells(0, spells, num, 1, 15, use_realm);

			/* Notify that there's nothing to see, and wait. */
			if (use_realm == REALM_HISSATSU)
#ifdef JP
				prt("読める技がない。", 0, 0);
#else
				prt("No techniques to browse.", 0, 0);
#endif
			else
#ifdef JP
				prt("読める呪文がない。", 0, 0);
#else
				prt("No spells to browse.", 0, 0);
#endif
			(void)inkey();
			

			/* Restore the screen */
			screen_load();

			return;
		}				  

		/* Clear lines, position cursor  (really should use strlen here) */
		Term_erase(14, 14, 255);
		Term_erase(14, 13, 255);
		Term_erase(14, 12, 255);
		Term_erase(14, 11, 255);

		/* Access the spell */
		if (!is_magic(use_realm))
		{
			s_ptr = &technic_info[use_realm - MIN_TECHNIC][spell];
		}
		else
		{
			s_ptr = &mp_ptr->info[use_realm - 1][spell];
		}

		roff_to_buf( spell_tips[technic2magic(use_realm)-1][spell] ,62,temp);
		for(j=0, line = 11;temp[j];j+=(1+strlen(&temp[j])))
		{
			prt(&temp[j], line, 15);
			line++;
		}
	}

	/* Restore the screen */
	screen_load();
}


static void change_realm2(int next_realm)
{
	int i, j=0;
	char tmp[80];

	for (i = 0; i < 64; i++)
	{
		p_ptr->spell_order[j] = p_ptr->spell_order[i];
		if(p_ptr->spell_order[i] < 32) j++;
	}
	for (; j < 64; j++)
		p_ptr->spell_order[j] = 99;

	for (i = 32; i < 64; i++)
	{
		p_ptr->spell_exp[i] = 0;
	}
	p_ptr->spell_learned2 = 0L;
	p_ptr->spell_worked2 = 0L;
	p_ptr->spell_forgotten2 = 0L;	

#ifdef JP
	sprintf(tmp,"魔法の領域を%sから%sに変更した。", realm_names[p_ptr->realm2], realm_names[next_realm]);
#else
	sprintf(tmp,"change magic realm from %s to %s.", realm_names[p_ptr->realm2], realm_names[next_realm]);
#endif
	do_cmd_write_nikki(NIKKI_BUNSHOU, 0, tmp);
	p_ptr->old_realm |= 1 << (p_ptr->realm2-1);
	p_ptr->realm2 = next_realm;

	p_ptr->notice |= (PN_REORDER);
	p_ptr->update |= (PU_SPELLS);
	handle_stuff();
}


/*
 * Study a book to gain a new spell/prayer
 */
void do_cmd_study(void)
{
	int	i, item, sval;
	int	increment = 0;
	bool    learned = FALSE;

	/* Spells of realm2 will have an increment of +32 */
	int	spell = -1;

	cptr p = spell_categoly_name(mp_ptr->spell_book);

	object_type *o_ptr;

	cptr q, s;

	if (!p_ptr->realm1)
	{
#ifdef JP
msg_print("本を読むことができない！");
#else
		msg_print("You cannot read books!");
#endif

		return;
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
msg_format("新しい%sを覚えることはできない！", p);
#else
		msg_format("You cannot learn any new %ss!", p);
#endif

		return;
	}

	if (p_ptr->special_defense & KATA_MUSOU)
	{
		set_action(ACTION_NONE);
	}

	p = spell_categoly_name(mp_ptr->spell_book);

#ifdef JP
	if( p_ptr->new_spells < 10 ){
		msg_format("あと %d つの%sを学べる。", p_ptr->new_spells, p);
	}else{
		msg_format("あと %d 個の%sを学べる。", p_ptr->new_spells, p);
	}
#else
	msg_format("You can learn %d new %s%s.", p_ptr->new_spells, p,
		(p_ptr->new_spells == 1?"":"s"));
#endif

	msg_print(NULL);


	/* Restrict choices to "useful" books */
	if (p_ptr->realm2 == REALM_NONE) item_tester_tval = mp_ptr->spell_book;
	else item_tester_hook = item_tester_learn_spell;

	/* Get an item */
#ifdef JP
q = "どの本から学びますか? ";
#else
	q = "Study which book? ";
#endif

#ifdef JP
s = "読める本がない。";
#else
	s = "You have no books that you can read.";
#endif

        select_spellbook=TRUE;
	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return;
        select_spellbook=FALSE;

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

	/* Access the item's sval */
	sval = o_ptr->sval;

	if (o_ptr->tval == REALM2_BOOK) increment = 32;
	else if (o_ptr->tval != REALM1_BOOK)
	{
#ifdef JP
		if (!get_check("本当に魔法の領域を変更しますか？")) return;
#else
		if (!get_check("Really, change magic realm? ")) return;
#endif
		change_realm2(tval2realm(o_ptr->tval));
		increment = 32;
	}

	/* Track the object kind */
	object_kind_track(o_ptr->k_idx);

	/* Hack -- Handle stuff */
	handle_stuff();

	/* Mage -- Learn a selected spell */
	if (mp_ptr->spell_book != TV_LIFE_BOOK)
	{
		/* Ask for a spell, allow cancel */
#ifdef JP
                if (!get_spell(&spell, "学ぶ", sval, FALSE, o_ptr->tval - TV_LIFE_BOOK + 1)
			&& (spell == -1)) return;
#else
		if (!get_spell(&spell, "study", sval, FALSE, o_ptr->tval - TV_LIFE_BOOK + 1)
			&& (spell == -1)) return;
#endif

	}

	/* Priest -- Learn a random prayer */
	else
	{
		int k = 0;

		int gift = -1;

		/* Extract spells */
		for (spell = 0; spell < 32; spell++)
		{
			/* Check spells in the book */
			if ((fake_spell_flags[sval] & (1L << spell)))
			{
				/* Skip non "okay" prayers */
				if (!spell_okay(spell, FALSE, TRUE,
					(increment ? p_ptr->realm2 : p_ptr->realm1))) continue;

				/* Hack -- Prepare the randomizer */
				k++;

				/* Hack -- Apply the randomizer */
				if (one_in_(k)) gift = spell;
			}
		}

		/* Accept gift */
		spell = gift;
	}

	/* Nothing to study */
	if (spell < 0)
	{
		/* Message */
#ifdef JP
msg_format("その本には学ぶべき%sがない。", p);
#else
		msg_format("You cannot learn any %ss in that book.", p);
#endif


		/* Abort */
		return;
	}


	if (increment) spell += increment;

	/* Learn the spell */
	if (spell < 32)
	{
		if (p_ptr->spell_learned1 & (1L << spell)) learned = TRUE;
		else p_ptr->spell_learned1 |= (1L << spell);
	}
	else
	{
		if (p_ptr->spell_learned2 & (1L << (spell - 32))) learned = TRUE;
		else p_ptr->spell_learned2 |= (1L << (spell - 32));
	}

	if (learned)
	{
		int max_exp = (spell < 32) ? 1600 : 1400;
		int old_exp = p_ptr->spell_exp[spell];
		int new_rank = 0;
		cptr name = spell_names[technic2magic(increment ? p_ptr->realm2 : p_ptr->realm1)-1][spell%32];

		if (old_exp >= max_exp)
		{
#ifdef JP
			msg_format("その%sは完全に使いこなせるので学ぶ必要はない。", spell_categoly_name(mp_ptr->spell_book));
#else
			msg_format("You don't need to study this %s anymore.", spell_categoly_name(mp_ptr->spell_book));
#endif
			return;
		}
#ifdef JP
		if (!get_check(format("%sの%sをさらに学びます。よろしいですか？", name, spell_categoly_name(mp_ptr->spell_book))))
#else
		if (!get_check(format("You will study a %s of %s again. Are you sure? ", spell_categoly_name(mp_ptr->spell_book), name)))
#endif
		{
			return;
		}
		else if (old_exp >= 1400)
		{
			p_ptr->spell_exp[spell] = 1600;
			new_rank = 4;
		}
		else if (old_exp >= 1200)
		{
			if (spell >= 32) p_ptr->spell_exp[spell] = 1400;
			else p_ptr->spell_exp[spell] += 200;
			new_rank = 3;
		}
		else if (old_exp >= 900)
		{
			p_ptr->spell_exp[spell] = 1200+(old_exp-900)*2/3;
			new_rank = 2;
		}
		else
		{
			p_ptr->spell_exp[spell] = 900+(old_exp)/3;
			new_rank = 1;
		}
#ifdef JP
		msg_format("%sの熟練度が%sに上がった。", name, shougou_moji[new_rank]);
#else
		msg_format("Your proficiency of %s is now %s rank.", name, shougou_moji[new_rank]);
#endif
	}
	else
	{
		/* Find the next open entry in "p_ptr->spell_order[]" */
		for (i = 0; i < 64; i++)
		{
			/* Stop at the first empty space */
			if (p_ptr->spell_order[i] == 99) break;
		}

		/* Add the spell to the known list */
		p_ptr->spell_order[i++] = spell;

		/* Mention the result */
#ifdef JP
	        /* 英日切り替え機能に対応 */
		if (mp_ptr->spell_book == TV_MUSIC_BOOK)
		{
        	        msg_format("%sを学んだ。",
				    spell_names[technic2magic(increment ? p_ptr->realm2 : p_ptr->realm1)-1][spell % 32]);
		}
		else
		{
        	        msg_format("%sの%sを学んだ。",
				    spell_names[technic2magic(increment ? p_ptr->realm2 : p_ptr->realm1)-1][spell % 32] ,p);
		}
#else
		msg_format("You have learned the %s of %s.",
			p, spell_names[technic2magic(increment ? p_ptr->realm2 : p_ptr->realm1)-1][spell % 32]);
#endif
	}

	/* Take a turn */
	energy_use = 100;

	if (mp_ptr->spell_book == TV_LIFE_BOOK)
		chg_virtue(V_FAITH, 1);
	else if (mp_ptr->spell_book == TV_DEATH_BOOK)
		chg_virtue(V_UNLIFE, 1);
	else if (mp_ptr->spell_book == TV_NATURE_BOOK)
		chg_virtue(V_NATURE, 1);
	else
		chg_virtue(V_KNOWLEDGE, 1);

	/* Sound */
	sound(SOUND_STUDY);

	/* One less spell available */
	p_ptr->learned_spells++;
#if 0
	/* Message if needed */
	if (p_ptr->new_spells)
	{
		/* Message */
#ifdef JP
                        if( p_ptr->new_spells < 10 ){
                                msg_format("あと %d つの%sを学べる。", p_ptr->new_spells, p);
                        }else{
                                msg_format("あと %d 個の%sを学べる。", p_ptr->new_spells, p);
                        }
#else
		msg_format("You can learn %d more %s%s.",
			p_ptr->new_spells, p,
			(p_ptr->new_spells != 1) ? "s" : "");
#endif

	}
#endif

	/* Update Study */
	p_ptr->update |= (PU_SPELLS);
	update_stuff();

        /* Redraw object recall */
        p_ptr->window |= (PW_OBJECT);
}


static void wild_magic(int spell)
{
	int counter = 0;
	int type = SUMMON_BIZARRE1 + randint0(6);

	if (type < SUMMON_BIZARRE1) type = SUMMON_BIZARRE1;
	else if (type > SUMMON_BIZARRE6) type = SUMMON_BIZARRE6;

	switch (randint1(spell) + randint1(8) + 1)
	{
	case 1:
	case 2:
	case 3:
		teleport_player(10);
		break;
	case 4:
	case 5:
	case 6:
		teleport_player(100);
		break;
	case 7:
	case 8:
		teleport_player(200);
		break;
	case 9:
	case 10:
	case 11:
		unlite_area(10, 3);
		break;
	case 12:
	case 13:
	case 14:
		lite_area(damroll(2, 3), 2);
		break;
	case 15:
		destroy_doors_touch();
		break;
	case 16: case 17:
		wall_breaker();
	case 18:
		sleep_monsters_touch();
		break;
	case 19:
	case 20:
		trap_creation(py, px);
		break;
	case 21:
	case 22:
		door_creation();
		break;
	case 23:
	case 24:
	case 25:
		aggravate_monsters(0);
		break;
	case 26:
		earthquake(py, px, 5);
		break;
	case 27:
	case 28:
		(void)gain_random_mutation(0);
		break;
	case 29:
	case 30:
		apply_disenchant(1);
		break;
	case 31:
		lose_all_info();
		break;
	case 32:
		fire_ball(GF_CHAOS, 0, spell + 5, 1 + (spell / 10));
		break;
	case 33:
		wall_stone();
		break;
	case 34:
	case 35:
		while (counter++ < 8)
		{
			(void)summon_specific(0, py, px, (dun_level * 3) / 2, type, (PM_ALLOW_GROUP | PM_NO_PET));
		}
		break;
	case 36:
	case 37:
		activate_hi_summon(py, px, FALSE);
		break;
	case 38:
		(void)summon_cyber(-1, py, px);
		break;
	default:
		{
			int count = 0;
			(void)activate_ty_curse(FALSE, &count);
			break;
		}
	}

	return;
}


static bool cast_life_spell(int spell)
{
	int	dir;
	int	plev = p_ptr->lev;

	switch (spell)
	{
	case 0: /* Cure Light Wounds */
		(void)hp_player(damroll(2, 10));
		(void)set_cut(p_ptr->cut - 10);
		break;
	case 1: /* Bless */
		(void)set_blessed(randint1(12) + 12, FALSE);
		break;
	case 2: /* Make Light Wounds */
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball_hide(GF_WOUNDS, dir, damroll(3 + ((plev - 1) / 5), 4), 0);
		break;
	case 3: /* Call Light */
		(void)lite_area(damroll(2, (plev / 2)), (plev / 10) + 1);
		break;
	case 4: /* Detect Traps + Secret Doors */
		(void)detect_traps(DETECT_RAD_DEFAULT, TRUE);
		(void)detect_doors(DETECT_RAD_DEFAULT);
		(void)detect_stairs(DETECT_RAD_DEFAULT);
		break;
	case 5: /* Cure Medium Wounds */
		(void)hp_player(damroll(4, 10));
		(void)set_cut((p_ptr->cut / 2) - 20);
		break;
	case 6: /* Cure Poison */
		(void)set_poisoned(0);
		break;
	case 7: /* Satisfy Hunger */
		(void)set_food(PY_FOOD_MAX - 1);
		break;
	case 8: /* Remove Curse */
		if (remove_curse())
		{
#ifdef JP
			msg_print("誰かに見守られているような気がする。");
#else
			msg_print("You feel as if someone is watching over you.");
#endif
		}
		break;
	case 9: /* Make Medium Wounds */
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball_hide(GF_WOUNDS, dir, damroll(8 + ((plev - 5) / 4), 8), 0);
		break;
	case 10: /* Cure Critical Wounds */
		(void)hp_player(damroll(8, 10));
		(void)set_stun(0);
		(void)set_cut(0);
		break;
	case 11:
		(void)set_oppose_cold(randint1(20) + 20, FALSE);
		(void)set_oppose_fire(randint1(20) + 20, FALSE);
		break;
	case 12:
		map_area(DETECT_RAD_MAP);
		break;
	case 13:
		(void)turn_undead();
		break;
	case 14: /* Healing */
		(void)hp_player(300);
		(void)set_stun(0);
		(void)set_cut(0);
		break;
	case 15: /* Glyph of Warding */
		warding_glyph();
		break;
	case 16: /* Dispel Curse */
		if (remove_all_curse())
		{
#ifdef JP
			msg_print("誰かに見守られているような気がする。");
#else
			msg_print("You feel as if someone is watching over you.");
#endif
		}
		break;
	case 17: /* Perception */
		return ident_spell(FALSE);
	case 18: /* Dispel Undead */
		(void)dispel_undead(randint1(plev * 5));
		break;
	case 19: /* 'Day of the Dove' */
		charm_monsters(plev * 2);
		break;
	case 20: /* Make Critical Wounds */
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball_hide(GF_WOUNDS, dir, damroll(5+((plev - 5) / 3), 15), 0);
		break;
	case 21: /* Word of Recall */
		if (!word_of_recall()) return FALSE;
		break;
	case 22: /* Alter Reality */
		alter_reality();
		break;
	case 23: /* Warding True */
		warding_glyph();
		glyph_creation();
		break;
	case 24:
		num_repro += MAX_REPRO;
		break;
	case 25: /* Detection True */
		(void)detect_all(DETECT_RAD_DEFAULT);
		break;
	case 26: /* Genocide Undead */
		(void)mass_genocide_undead(plev+50,TRUE);
		break;
	case 27: /* Clairvoyance */
		wiz_lite(FALSE, FALSE);
		break;
	case 28: /* Restoration */
		(void)do_res_stat(A_STR);
		(void)do_res_stat(A_INT);
		(void)do_res_stat(A_WIS);
		(void)do_res_stat(A_DEX);
		(void)do_res_stat(A_CON);
		(void)do_res_stat(A_CHR);
		(void)restore_level();
		break;
	case 29: /* Healing True */
		(void)hp_player(2000);
		(void)set_stun(0);
		(void)set_cut(0);
		break;
	case 30: /* Holy Vision */
		return identify_fully(FALSE);
	case 31: /* Ultimate resistance */
	{
		int v = randint1(plev/2)+plev/2;
		(void)set_fast(v, FALSE);
		set_oppose_acid(v, FALSE);
		set_oppose_elec(v, FALSE);
		set_oppose_fire(v, FALSE);
		set_oppose_cold(v, FALSE);
		set_oppose_pois(v, FALSE);
		set_ultimate_res(v, FALSE);
		break;
	}
	default:
#ifdef JP
msg_format("あなたは不明なライフの呪文 %d を唱えた。", spell);
#else
		msg_format("You cast an unknown Life spell: %d.", spell);
#endif

		msg_print(NULL);
	}

	return TRUE;
}



static bool cast_sorcery_spell(int spell)
{
	int	dir;
	int	plev = p_ptr->lev;

	switch (spell)
	{
	case 0: /* Detect Monsters */
		(void)detect_monsters_normal(DETECT_RAD_DEFAULT);
		break;
	case 1: /* Phase Door */
		teleport_player(10);
		break;
	case 2: /* Detect Doors and Traps */
		(void)detect_traps(DETECT_RAD_DEFAULT, TRUE);
		(void)detect_doors(DETECT_RAD_DEFAULT);
		(void)detect_stairs(DETECT_RAD_DEFAULT);
		break;
	case 3: /* Light Area */
		(void)lite_area(damroll(2, (plev / 2)), (plev / 10) + 1);
		break;
	case 4: /* Confuse Monster */
		if (!get_aim_dir(&dir)) return FALSE;

		(void)confuse_monster(dir, (plev * 3) / 2);
		break;
	case 5: /* Teleport */
		teleport_player(plev * 5);
		break;
	case 6: /* Sleep Monster */
		if (!get_aim_dir(&dir)) return FALSE;

		(void)sleep_monster(dir);
		break;
	case 7: /* Recharging */
		return recharge(plev * 4);
	case 8: /* Magic Mapping */
		map_area(DETECT_RAD_MAP);
		break;
	case 9: /* Identify */
		return ident_spell(FALSE);
	case 10: /* Slow Monster */
		if (!get_aim_dir(&dir)) return FALSE;

		(void)slow_monster(dir);
		break;
	case 11: /* Mass Sleep */
		(void)sleep_monsters();
		break;
	case 12: /* Teleport Away */
		if (!get_aim_dir(&dir)) return FALSE;

		(void)fire_beam(GF_AWAY_ALL, dir, plev);
		break;
	case 13: /* Haste Self */
		(void)set_fast(randint1(20 + plev) + plev, FALSE);
		break;
	case 14: /* Detection True */
		(void)detect_all(DETECT_RAD_DEFAULT);
		break;
	case 15: /* Identify True */
		return identify_fully(FALSE);
	case 16: /* Detect Objects and Treasure*/
		(void)detect_objects_normal(DETECT_RAD_DEFAULT);
		(void)detect_treasure(DETECT_RAD_DEFAULT);
		(void)detect_objects_gold(DETECT_RAD_DEFAULT);
		break;
	case 17: /* Charm Monster */
		if (!get_aim_dir(&dir)) return FALSE;

		(void)charm_monster(dir, plev);
		break;
	case 18: /* Sense Minds */
		(void)set_tim_esp(randint1(30) + 25, FALSE);
		break;
	case 19: /* Teleport to town */
		return tele_town();
		break;
	case 20: /* Self knowledge */
		(void)self_knowledge();
		break;
	case 21: /* Teleport Level */
#ifdef JP
		if (!get_check("本当に他の階にテレポートしますか？")) return FALSE;
#else
		if (!get_check("Are you sure? (Teleport Level)")) return FALSE;
#endif
		(void)teleport_player_level();
		break;
	case 22: /* Word of Recall */
		if (!word_of_recall()) return FALSE;
		break;
	case 23: /* Dimension Door */
#ifdef JP
msg_print("次元の扉が開いた。目的地を選んで下さい。");
#else
		msg_print("You open a dimensional gate. Choose a destination.");
#endif

		return dimension_door();
	case 24: /* Probing */
		(void)probing();
		break;
	case 25: /* Explosive Rune */
		explosive_rune();
		break;
	case 26: /* Telekinesis */
		if (!get_aim_dir(&dir)) return FALSE;

		fetch(dir, plev * 15, FALSE);
		break;
	case 27: /* Clairvoyance */
		chg_virtue(V_KNOWLEDGE, 1);
		chg_virtue(V_ENLIGHTEN, 1);

		wiz_lite(FALSE, FALSE);
		if (!(p_ptr->telepathy))
		{
			(void)set_tim_esp(randint1(30) + 25, FALSE);
		}
		break;
	case 28: /* Charm Monsters */
		charm_monsters(plev * 2);
		break;
	case 29: /* Alchemy */
		return alchemy();
	case 30: /* Banish */
		banish_monsters(plev * 4);
		break;
	case 31: /* Globe of Invulnerability */
		(void)set_invuln(randint1(4) + 4, FALSE);
		break;
	default:
#ifdef JP
msg_format("あなたは不明なソーサリーの呪文 %d を唱えた。", spell);
#else
		msg_format("You cast an unknown Sorcery spell: %d.", spell);
#endif

		msg_print(NULL);
	}

	return TRUE;
}


static bool cast_nature_spell(int spell)
{
	int	    dir;
	int	    beam;
	int	    plev = p_ptr->lev;
	bool    no_trump = FALSE;

	if (p_ptr->pclass == CLASS_MAGE) beam = plev;
	else if (p_ptr->pclass == CLASS_HIGH_MAGE || p_ptr->pclass == CLASS_SORCERER) beam = plev + 10;
	else beam = plev / 2;

	switch (spell)
	{
	case 0: /* Detect Creatures */
		(void)detect_monsters_normal(DETECT_RAD_DEFAULT);
		break;
	case 1: /* Lightning Bolt */
		project_length = plev / 6 + 2;
		if (!get_aim_dir(&dir)) return FALSE;

		fire_beam(GF_ELEC, dir,
			damroll(3 + ((plev - 1) / 5), 4));
		break;
	case 2: /* Detect Doors & Traps */
		(void)detect_traps(DETECT_RAD_DEFAULT, TRUE);
		(void)detect_doors(DETECT_RAD_DEFAULT);
		(void)detect_stairs(DETECT_RAD_DEFAULT);
		break;
	case 3: /* Produce Food */
		(void)set_food(PY_FOOD_MAX - 1);
		break;
	case 4: /* Daylight */
		(void)lite_area(damroll(2, (plev / 2)), (plev / 10) + 1);
		if ((prace_is_(RACE_VAMPIRE) || (p_ptr->mimic_form == MIMIC_VAMPIRE)) && !p_ptr->resist_lite)
		{
#ifdef JP
msg_print("日の光があなたの肉体を焦がした！");
#else
			msg_print("The daylight scorches your flesh!");
#endif

#ifdef JP
take_hit(DAMAGE_NOESCAPE, damroll(2, 2), "日の光", -1);
#else
			take_hit(DAMAGE_NOESCAPE, damroll(2, 2), "daylight", -1);
#endif

		}
		break;
	case 5: /* Animal Taming */
		if (!get_aim_dir(&dir)) return FALSE;

		(void)charm_animal(dir, plev);
		break;
	case 6: /* Resist Environment */
		(void)set_oppose_cold(randint1(20) + 20, FALSE);
		(void)set_oppose_fire(randint1(20) + 20, FALSE);
		(void)set_oppose_elec(randint1(20) + 20, FALSE);
		break;
	case 7: /* Cure Wounds & Poison */
		(void)hp_player(damroll(2, 8));
		(void)set_cut(0);
		(void)set_poisoned(0);
		break;
	case 8: /* Stone to Mud */
		if (!get_aim_dir(&dir)) return FALSE;

		(void)wall_to_mud(dir);
		break;
	case 9: /* Frost Bolt */
		if (!get_aim_dir(&dir)) return FALSE;
		fire_bolt_or_beam(beam - 10, GF_COLD, dir,
			damroll(3 + ((plev - 5) / 4), 8));
		break;
	case 10: /* Nature Awareness -- downgraded */
		map_area(DETECT_RAD_MAP);
		(void)detect_traps(DETECT_RAD_DEFAULT, TRUE);
		(void)detect_doors(DETECT_RAD_DEFAULT);
		(void)detect_stairs(DETECT_RAD_DEFAULT);
		(void)detect_monsters_normal(DETECT_RAD_DEFAULT);
		break;
	case 11: /* Fire Bolt */
		if (!get_aim_dir(&dir)) return FALSE;
		fire_bolt_or_beam(beam - 10, GF_FIRE, dir,
			damroll(5 + ((plev - 5) / 4), 8));
		break;
	case 12: /* Ray of Sunlight */
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
msg_print("太陽光線が現れた。");
#else
		msg_print("A line of sunlight appears.");
#endif

		(void)lite_line(dir);
		break;
	case 13: /* Entangle */
		slow_monsters();
		break;
	case 14: /* Summon Animals */
		if (!(summon_specific(-1, py, px, plev, SUMMON_ANIMAL_RANGER, (PM_ALLOW_GROUP | PM_FORCE_PET))))
			no_trump = TRUE;
		break;
	case 15: /* Herbal Healing */
		(void)hp_player(500);
		(void)set_stun(0);
		(void)set_cut(0);
		(void)set_poisoned(0);
		break;
	case 16: /* Stair Building */
		(void)stair_creation();
		break;
	case 17: /* Stone Skin */
		(void)set_shield(randint1(20) + 30, FALSE);
		break;
	case 18: /* Resistance True */
		(void)set_oppose_acid(randint1(20) + 20, FALSE);
		(void)set_oppose_elec(randint1(20) + 20, FALSE);
		(void)set_oppose_fire(randint1(20) + 20, FALSE);
		(void)set_oppose_cold(randint1(20) + 20, FALSE);
		(void)set_oppose_pois(randint1(20) + 20, FALSE);
		break;
	case 19: /* Tree Creation */
		(void)tree_creation();
		break;
	case 20: /* Animal Friendship */
		(void)charm_animals(plev * 2);
		break;
	case 21: /* Stone Tell */
		return identify_fully(FALSE);
	case 22: /* Wall of Stone */
		(void)wall_stone();
		break;
	case 23: /* Protection from Corrosion */
		return rustproof();
	case 24: /* Earthquake */
		earthquake(py, px, 10);
		break;
	case 25: /* Whirlwind Attack */
		{
			int y = 0, x = 0;
			cave_type       *c_ptr;
			monster_type    *m_ptr;

			for (dir = 0; dir < 8; dir++)
			{
				y = py + ddy_ddd[dir];
				x = px + ddx_ddd[dir];
				c_ptr = &cave[y][x];

				/* Get the monster */
				m_ptr = &m_list[c_ptr->m_idx];

				/* Hack -- attack monsters */
				if (c_ptr->m_idx && (m_ptr->ml || cave_floor_bold(y, x)))
					py_attack(y, x, 0);
			}
		}
		break;
	case 26: /* Blizzard */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_ball(GF_COLD, dir, 70 + plev * 3 / 2, (plev / 12) + 1);
		break;
	case 27: /* Lightning Storm */
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball(GF_ELEC, dir, 90 + plev * 3 / 2, (plev / 12) + 1);
		break;
	case 28: /* Whirlpool */
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball(GF_WATER, dir, 100 + plev * 3 / 2, (plev / 12) + 1);
		break;
	case 29: /* Call Sunlight */
		fire_ball(GF_LITE, 0, 150, 8);
		chg_virtue(V_KNOWLEDGE, 1);
		chg_virtue(V_ENLIGHTEN, 1);
		wiz_lite(FALSE, FALSE);
		if ((prace_is_(RACE_VAMPIRE) || (p_ptr->mimic_form == MIMIC_VAMPIRE)) && !p_ptr->resist_lite)
		{
#ifdef JP
msg_print("日光があなたの肉体を焦がした！");
#else
			msg_print("The sunlight scorches your flesh!");
#endif

#ifdef JP
take_hit(DAMAGE_NOESCAPE, 50, "日光", -1);
#else
			take_hit(DAMAGE_NOESCAPE, 50, "sunlight", -1);
#endif

		}
		break;
	case 30: /* Elemental Branding */
		brand_weapon(randint0(2));
		break;
	case 31: /* Nature's Wrath */
		(void)dispel_monsters(plev * 4);
		earthquake(py, px, 20 + (plev / 2));
		project(0, 1 + plev / 12, py, px,
			(100 + plev) * 2, GF_DISINTEGRATE, PROJECT_KILL | PROJECT_ITEM, -1);
		break;
	default:
#ifdef JP
msg_format("あなたは不明なネイチャーの呪文 %d を唱えた。", spell);
#else
		msg_format("You cast an unknown Nature spell: %d.", spell);
#endif

		msg_print(NULL);
	}

	if (no_trump)
#ifdef JP
msg_print("動物は現れなかった。");
#else
		msg_print("No animals arrive.");
#endif


	return TRUE;
}


static bool cast_chaos_spell(int spell)
{
	int	dir, i, beam;
	int	plev = p_ptr->lev;

	if (p_ptr->pclass == CLASS_MAGE) beam = plev;
	else if (p_ptr->pclass == CLASS_HIGH_MAGE || p_ptr->pclass == CLASS_SORCERER) beam = plev + 10;
	else beam = plev / 2;

	switch (spell)
	{
	case 0: /* Magic Missile */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_bolt_or_beam(beam - 10, GF_MISSILE, dir,
			damroll(3 + ((plev - 1) / 5), 4));
		break;
	case 1: /* Trap / Door destruction */
		(void)destroy_doors_touch();
		break;
	case 2: /* Flash of Light == Light Area */
		(void)lite_area(damroll(2, (plev / 2)), (plev / 10) + 1);
		break;
	case 3: /* Touch of Confusion */
		if (!(p_ptr->special_attack & ATTACK_CONFUSE))
		{
#ifdef JP
msg_print("あなたの手は光り始めた。");
#else
			msg_print("Your hands start glowing.");
#endif

			p_ptr->special_attack |= ATTACK_CONFUSE;
			p_ptr->redraw |= (PR_STATUS);
		}
		break;
	case 4: /* Mana Burst */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_ball(GF_MISSILE, dir,
			(damroll(3, 5) + plev +
			(plev / (((p_ptr->pclass == CLASS_MAGE)
			|| (p_ptr->pclass == CLASS_HIGH_MAGE)
			|| (p_ptr->pclass == CLASS_SORCERER)) ? 2 : 4))),
			((plev < 30) ? 2 : 3));
			/* Shouldn't actually use GF_MANA, as it will destroy all
			 * items on the floor */
		break;
	case 5: /* Fire Bolt */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_bolt_or_beam(beam, GF_FIRE, dir,
			damroll(8 + ((plev - 5) / 4), 8));
		break;
	case 6: /* Fist of Force ("Fist of Fun") */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_ball(GF_DISINTEGRATE, dir,
			damroll(8 + ((plev - 5) / 4), 8), 0);
		break;
	case 7: /* Teleport Self */
		teleport_player(plev * 5);
		break;
	case 8: /* Wonder */
		{
		/* This spell should become more useful (more
		controlled) as the player gains experience levels.
		Thus, add 1/5 of the player's level to the die roll.
		This eliminates the worst effects later on, while
		keeping the results quite random.  It also allows
			some potent effects only at high level. */

			int die = randint1(100) + plev / 5;
			int vir = virtue_number(V_CHANCE);
			if (vir)
			{
				if (p_ptr->virtues[vir - 1] > 0)
				{
					while (randint1(400) < p_ptr->virtues[vir - 1]) die++;
				}
				else
				{
					while (randint1(400) < (0-p_ptr->virtues[vir - 1])) die--;
				}
			}

			if (die < 26)
				chg_virtue(V_CHANCE, 1);

			if (!get_aim_dir(&dir)) return FALSE;
			if (die > 100)
#ifdef JP
msg_print("あなたは力がみなぎるのを感じた！");
#else
				msg_print("You feel a surge of power!");
#endif

			if (die < 8) clone_monster(dir);
			else if (die < 14) speed_monster(dir);
			else if (die < 26) heal_monster(dir, damroll(4, 6));
			else if (die < 31) poly_monster(dir);
			else if (die < 36)
				fire_bolt_or_beam(beam - 10, GF_MISSILE, dir,
				    damroll(3 + ((plev - 1) / 5), 4));
			else if (die < 41) confuse_monster(dir, plev);
			else if (die < 46) fire_ball(GF_POIS, dir, 20 + (plev / 2), 3);
			else if (die < 51) (void)lite_line(dir);
			else if (die < 56)
				fire_bolt_or_beam(beam - 10, GF_ELEC, dir,
				    damroll(3 + ((plev - 5) / 4), 8));
			else if (die < 61)
				fire_bolt_or_beam(beam - 10, GF_COLD, dir,
				    damroll(5 + ((plev - 5) / 4), 8));
			else if (die < 66)
				fire_bolt_or_beam(beam, GF_ACID, dir,
				    damroll(6 + ((plev - 5) / 4), 8));
			else if (die < 71)
				fire_bolt_or_beam(beam, GF_FIRE, dir,
				    damroll(8 + ((plev - 5) / 4), 8));
			else if (die < 76) drain_life(dir, 75);
			else if (die < 81) fire_ball(GF_ELEC, dir, 30 + plev / 2, 2);
			else if (die < 86) fire_ball(GF_ACID, dir, 40 + plev, 2);
			else if (die < 91) fire_ball(GF_ICE, dir, 70 + plev, 3);
			else if (die < 96) fire_ball(GF_FIRE, dir, 80 + plev, 3);
			else if (die < 101) drain_life(dir, 100 + plev);
			else if (die < 104)
			{
				earthquake(py, px, 12);
			}
			else if (die < 106)
			{
				destroy_area(py, px, 13+randint0(5), TRUE);
			}
			else if (die < 108)
			{
				symbol_genocide(plev+50, TRUE);
			}
			else if (die < 110) dispel_monsters(120);
			else /* RARE */
			{
				dispel_monsters(150);
				slow_monsters();
				sleep_monsters();
				hp_player(300);
			}
			break;
		}
		break;
	case 9: /* Chaos Bolt */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_bolt_or_beam(beam, GF_CHAOS, dir,
			damroll(10 + ((plev - 5) / 4), 8));
		break;
	case 10: /* Sonic Boom */
#ifdef JP
msg_print("ドーン！部屋が揺れた！");
#else
		msg_print("BOOM! Shake the room!");
#endif

		project(0, plev / 10 + 2, py, px,
			(60 + plev), GF_SOUND, PROJECT_KILL | PROJECT_ITEM, -1);
		break;
	case 11: /* Doom Bolt -- always beam in 2.0.7 or later */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_beam(GF_MANA, dir, damroll(11 + ((plev - 5) / 4), 8));
		break;
	case 12: /* Fire Ball */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_ball(GF_FIRE, dir, plev + 55, 2);
		break;
	case 13: /* Teleport Other */
		if (!get_aim_dir(&dir)) return FALSE;

		(void)fire_beam(GF_AWAY_ALL, dir, plev);
		break;
	case 14: /* Word of Destruction */
		destroy_area(py, px, 13+randint0(5), TRUE);
		break;
	case 15: /* Invoke Logrus */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_ball(GF_CHAOS, dir, plev*2 + 99, plev / 5);
		break;
	case 16: /* Polymorph Other */
		if (!get_aim_dir(&dir)) return FALSE;

		(void)poly_monster(dir);
		break;
	case 17: /* Chain Lightning */
		for (dir = 0; dir <= 9; dir++)
			fire_beam(GF_ELEC, dir, damroll(5 + (plev / 10), 8));
		break;
	case 18: /* Arcane Binding == Charging */
		return recharge(90);
	case 19: /* Disintegration */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_ball(GF_DISINTEGRATE, dir, plev + 70, 3 + plev / 40);
		break;
	case 20: /* Alter Reality */
		alter_reality();
		break;
	case 21: /* Magic Rocket */
		if (!get_aim_dir(&dir)) return FALSE;

#ifdef JP
msg_print("ロケット発射！");
#else
		msg_print("You launch a rocket!");
#endif

		fire_rocket(GF_ROCKET, dir, 120 + plev*2, 2);
		break;
	case 22: /* Chaos Branding */
		brand_weapon(2);
		break;
	case 23: /* Summon monster, demon */
		{
			u32b mode = 0L;
			bool pet = !one_in_(3);

			if (pet) mode |= PM_FORCE_PET;
			else mode |= PM_NO_PET;
			if (!(pet && (plev < 50))) mode |= PM_ALLOW_GROUP;

			if (summon_specific((pet ? -1 : 0), py, px, (plev * 3) / 2, SUMMON_DEMON, mode))
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
msg_print("「卑しき者よ、我は汝の下僕にあらず！ お前の魂を頂くぞ！」");
#else
					msg_print("'NON SERVIAM! Wretch! I shall feast on thy mortal soul!'");
#endif

			}
			break;
		}
	case 24: /* Beam of Gravity */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_beam(GF_GRAVITY, dir, damroll(9 + ((plev - 5) / 4), 8));
		break;
	case 25: /* Meteor Swarm  */
		{
			int x, y, dx, dy;
			int b = 10 + randint1(10);
			for (i = 0; i < b; i++)
			{
				int count = 0, d = 0;

				while (1)
				{
					count++;
					if (count > 20) break;
					x = px - 8 + randint0(17);
					y = py - 8 + randint0(17);

					if (!in_bounds(y,x) || (!cave_floor_bold(y,x) && (cave[y][x].feat != FEAT_TREES)) || !player_has_los_bold(y, x)) continue;

					dx = (px > x) ? (px - x) : (x - px);
					dy = (py > y) ? (py - y) : (y - py);

					/* Approximate distance */
					d = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));
					if (d < 9) break;
				}

				if (count > 20) continue;

				project(0, 2, y, x, plev * 2, GF_METEOR, PROJECT_KILL | PROJECT_JUMP | PROJECT_ITEM, -1);
			}
		}
		break;
	case 26: /* Flame Strike */
		fire_ball(GF_FIRE, 0, 300 + (3 * plev), 8);
		break;
	case 27: /* Call Chaos */
		call_chaos();
		break;
	case 28: /* Polymorph Self */
#ifdef JP
		if (!get_check("変身します。よろしいですか？")) return FALSE;
#else
		if (!get_check("You will polymorph yourself. Are you sure? ")) return FALSE;
#endif
		do_poly_self();
		break;
	case 29: /* Mana Storm */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_ball(GF_MANA, dir, 300 + (plev * 4), 4);
		break;
	case 30: /* Breathe Logrus */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_ball(GF_CHAOS, dir, p_ptr->chp, 2);
		break;
	case 31: /* Call the Void */
		call_the_();
		break;
	default:
#ifdef JP
msg_format("あなたは不明なカオスの呪文 %d を唱えた。", spell);
#else
		msg_format("You cast an unknown Chaos spell: %d.", spell);
#endif

		msg_print(NULL);
	}

	return TRUE;
}


static bool cast_death_spell(int spell)
{
	int	dir;
	int	beam;
	int	plev = p_ptr->lev;
	int	dummy = 0;

	if (p_ptr->pclass == CLASS_MAGE) beam = plev;
	else if (p_ptr->pclass == CLASS_HIGH_MAGE || p_ptr->pclass == CLASS_SORCERER) beam = plev + 10;
	else beam = plev / 2;

	switch (spell)
	{
	case 0: /* Detect Undead & Demons -> Unlife */
		(void)detect_monsters_nonliving(DETECT_RAD_DEFAULT);
		break;
	case 1: /* Malediction */
		if (!get_aim_dir(&dir)) return FALSE;
		/* A radius-0 ball may (1) be aimed at objects etc.,
		 * and will affect them; (2) may be aimed at ANY
		 * visible monster, unlike a 'bolt' which must travel
		 * to the monster. */

		fire_ball(GF_HELL_FIRE, dir,
			damroll(3 + ((plev - 1) / 5), 4), 0);

		if (one_in_(5))
		{   /* Special effect first */
			dummy = randint1(1000);
			if (dummy == 666)
				fire_ball_hide(GF_DEATH_RAY, dir, plev * 200, 0);
			else if (dummy < 500)
				fire_ball_hide(GF_TURN_ALL, dir, plev, 0);
			else if (dummy < 800)
				fire_ball_hide(GF_OLD_CONF, dir, plev, 0);
			else
				fire_ball_hide(GF_STUN, dir, plev, 0);
		}
		break;
	case 2: /* Detect Evil */
		(void)detect_monsters_evil(DETECT_RAD_DEFAULT);
		break;
	case 3: /* Stinking Cloud */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_ball(GF_POIS, dir, 10 + (plev / 2), 2);
		break;
	case 4: /* Black Sleep */
		if (!get_aim_dir(&dir)) return FALSE;

		(void)sleep_monster(dir);
		break;
	case 5: /* Resist Poison */
		(void)set_oppose_pois(randint1(20) + 20, FALSE);
		break;
	case 6: /* Horrify */
		if (!get_aim_dir(&dir)) return FALSE;

		(void)fear_monster(dir, plev);
		(void)stun_monster(dir, plev);
		break;
	case 7: /* Enslave Undead */
		if (!get_aim_dir(&dir)) return FALSE;

		(void)control_one_undead(dir, plev);
		break;
	case 8: /* Orb of Entropy */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_ball(GF_OLD_DRAIN, dir,
			(damroll(3, 6) + plev +
			(plev / (((p_ptr->pclass == CLASS_MAGE) ||
			(p_ptr->pclass == CLASS_HIGH_MAGE) ||
			(p_ptr->pclass == CLASS_SORCERER)) ? 2 : 4))),
			((plev < 30) ? 2 : 3));
		break;
	case 9: /* Nether Bolt */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_bolt_or_beam(beam, GF_NETHER, dir,
		    damroll(8 + ((plev - 5) / 4), 8));
		break;
	case 10: /* Cloud kill */
		project(0, plev / 10 + 2, py, px,
			(30 + plev) * 2, GF_POIS, PROJECT_KILL | PROJECT_ITEM, -1);
		break;
	case 11: /* Genocide One */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_ball_hide(GF_GENOCIDE, dir, plev + 50, 0);
		break;
	case 12: /* Poison Branding */
		brand_weapon(3);
		break;
	case 13: /* Vampiric Drain */
		if (!get_aim_dir(&dir)) return FALSE;

		dummy = plev * 2 + randint1(plev * 2);   /* Dmg */
		if (drain_life(dir, dummy))
		{
			chg_virtue(V_SACRIFICE, -1);
			chg_virtue(V_VITALITY, -1);

			(void)hp_player(dummy);
			/* Gain nutritional sustenance: 150/hp drained */
			/* A Food ration gives 5000 food points (by contrast) */
			/* Don't ever get more than "Full" this way */
			/* But if we ARE Gorged,  it won't cure us */
			dummy = p_ptr->food + MIN(5000, 100 * dummy);
			if (p_ptr->food < PY_FOOD_MAX)   /* Not gorged already */
				(void)set_food(dummy >= PY_FOOD_MAX ? PY_FOOD_MAX - 1 : dummy);
		}
		break;
	case 14: /* Animate Dead */
		animate_dead(0, py, px);
		break;
	case 15: /* Genocide */
		(void)symbol_genocide(plev+50, TRUE);
		break;
	case 16: /* Berserk */
		(void)set_shero(randint1(25) + 25, FALSE);
		(void)hp_player(30);
		(void)set_afraid(0);
		break;
	case 17: /* Invoke Spirits */
		{
			int die = randint1(100) + plev / 5;
			int vir = virtue_number(V_CHANCE);
			if (vir)
			{
				if (p_ptr->virtues[vir - 1] > 0)
				{
					while (randint1(400) < p_ptr->virtues[vir - 1]) die++;
				}
				else
				{
					while (randint1(400) < (0-p_ptr->virtues[vir - 1])) die--;
				}
			}

			if (!get_aim_dir(&dir)) return FALSE;

#ifdef JP
msg_print("あなたは死者たちの力を招集した...");
#else
			msg_print("You call on the power of the dead...");
#endif
			if (die < 26)
				chg_virtue(V_CHANCE, 1);

			if (die > 100)
#ifdef JP
msg_print("あなたはおどろおどろしい力のうねりを感じた！");
#else
				msg_print("You feel a surge of eldritch force!");
#endif


			if (die < 8)
			{
#ifdef JP
msg_print("なんてこった！あなたの周りの地面から朽ちた人影が立ち上がってきた！");
#else
				msg_print("Oh no! Mouldering forms rise from the earth around you!");
#endif

				(void)summon_specific(0, py, px, dun_level, SUMMON_UNDEAD, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET));
				chg_virtue(V_UNLIFE, 1);
			}
			else if (die < 14)
			{
#ifdef JP
msg_print("名状し難い邪悪な存在があなたの心を通り過ぎて行った...");
#else
				msg_print("An unnamable evil brushes against your mind...");
#endif

				set_afraid(p_ptr->afraid + randint1(4) + 4);
			}
			else if (die < 26)
			{
#ifdef JP
msg_print("あなたの頭に大量の幽霊たちの騒々しい声が押し寄せてきた...");
#else
				msg_print("Your head is invaded by a horde of gibbering spectral voices...");
#endif

				set_confused(p_ptr->confused + randint1(4) + 4);
			}
			else if (die < 31)
			{
				poly_monster(dir);
			}
			else if (die < 36)
			{
				fire_bolt_or_beam(beam - 10, GF_MISSILE, dir,
					damroll(3 + ((plev - 1) / 5), 4));
			}
			else if (die < 41)
			{
				confuse_monster (dir, plev);
			}
			else if (die < 46)
			{
				fire_ball(GF_POIS, dir, 20 + (plev / 2), 3);
			}
			else if (die < 51)
			{
				(void)lite_line(dir);
			}
			else if (die < 56)
			{
				fire_bolt_or_beam(beam - 10, GF_ELEC, dir,
					damroll(3+((plev-5)/4),8));
			}
			else if (die < 61)
			{
				fire_bolt_or_beam(beam - 10, GF_COLD, dir,
					damroll(5+((plev-5)/4),8));
			}
			else if (die < 66)
			{
				fire_bolt_or_beam(beam, GF_ACID, dir,
					damroll(6+((plev-5)/4),8));
			}
			else if (die < 71)
			{
				fire_bolt_or_beam(beam, GF_FIRE, dir,
					damroll(8+((plev-5)/4),8));
			}
			else if (die < 76)
			{
				drain_life(dir, 75);
			}
			else if (die < 81)
			{
				fire_ball(GF_ELEC, dir, 30 + plev / 2, 2);
			}
			else if (die < 86)
			{
				fire_ball(GF_ACID, dir, 40 + plev, 2);
			}
			else if (die < 91)
			{
				fire_ball(GF_ICE, dir, 70 + plev, 3);
			}
			else if (die < 96)
			{
				fire_ball(GF_FIRE, dir, 80 + plev, 3);
			}
			else if (die < 101)
			{
				drain_life(dir, 100 + plev);
			}
			else if (die < 104)
			{
				earthquake(py, px, 12);
			}
			else if (die < 106)
			{
				destroy_area(py, px, 13+randint0(5), TRUE);
			}
			else if (die < 108)
			{
				symbol_genocide(plev+50, TRUE);
			}
			else if (die < 110)
			{
				dispel_monsters(120);
			}
			else
			{ /* RARE */
				dispel_monsters(150);
				slow_monsters();
				sleep_monsters();
				hp_player(300);
			}

			if (die < 31)
#ifdef JP
msg_print("陰欝な声がクスクス笑う。「もうすぐおまえは我々の仲間になるだろう。弱き者よ。」");
#else
				msg_print("Sepulchral voices chuckle. 'Soon you will join us, mortal.'");
#endif

			break;
		}
	case 18: /* Dark Bolt */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_bolt_or_beam(beam, GF_DARK, dir,
			damroll(4 + ((plev - 5) / 4), 8));
		break;
	case 19: /* Battle Frenzy */
		(void)set_shero(randint1(25) + 25, FALSE);
		(void)hp_player(30);
		(void)set_afraid(0);
		(void)set_fast(randint1(20 + (plev / 2)) + (plev / 2), FALSE);
		break;
	case 20: /* Vampiric Branding */
		brand_weapon(4);
		break;
	case 21: /* Vampirism True */
		if (!get_aim_dir(&dir)) return FALSE;

		chg_virtue(V_SACRIFICE, -1);
		chg_virtue(V_VITALITY, -1);

		for (dummy = 0; dummy < 3; dummy++)
		{
			if (drain_life(dir, 100))
				hp_player(100);
		}
		break;
	case 22: /* Word of Death */
		(void)dispel_living(randint1(plev * 3));
		break;
	case 23: /* Darkness Storm */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_ball(GF_DARK, dir, 100+plev*2, 4);
		break;
	case 24: /* Death Ray */
		if (!get_aim_dir(&dir)) return FALSE;

		(void)death_ray(dir, plev);
		break;
	case 25: /* Raise the Dead */
		{
			int type;
			bool pet = one_in_(3);
			u32b mode = 0L;

			type = (plev > 47 ? SUMMON_HI_UNDEAD : SUMMON_UNDEAD);

			if (!pet || (pet && (plev > 24) && one_in_(3)))
				mode |= PM_ALLOW_GROUP;

			if (pet) mode |= PM_FORCE_PET;
			else mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);

			if (summon_specific((pet ? -1 : 0), py, px, (plev * 3) / 2, type, mode))
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

			chg_virtue(V_UNLIFE, 1);
			}

			break;
		}
	case 26: /* Esoteria */
		if (randint1(50) > plev)
			return ident_spell(FALSE);
		else
			return identify_fully(FALSE);
		break;
	case 27: /* Mimic vampire */
		(void)set_mimic(10+plev/2 + randint1(10+plev/2), MIMIC_VAMPIRE, FALSE);
		break;
	case 28: /* Restore Life */
		(void)restore_level();
		break;
	case 29: /* Mass Genocide */
		(void)mass_genocide(plev+50, TRUE);
		break;
	case 30: /* Hellfire */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_ball(GF_HELL_FIRE, dir, 666, 3);
#ifdef JP
take_hit(DAMAGE_USELIFE, 20 + randint1(30), "地獄の劫火の呪文を唱えた疲労", -1);
#else
		take_hit(DAMAGE_USELIFE, 20 + randint1(30), "the strain of casting Hellfire", -1);
#endif

		break;
	case 31: /* Wraithform */
		set_wraith_form(randint1(plev / 2) + (plev / 2), FALSE);
		break;
	default:
		msg_format("You cast an unknown Death spell: %d.", spell);
		msg_print(NULL);
	}

	return TRUE;
}


static bool cast_trump_spell(int spell, bool success)
{
	int     dir;
	int     beam;
	int     plev = p_ptr->lev;
	int     summon_lev = plev * 2 / 3 + randint1(plev/2);
	int     dummy = 0;
	bool	no_trump = FALSE;
	bool    unique_okay = FALSE;


	if (p_ptr->pclass == CLASS_MAGE) beam = plev;
	else if (p_ptr->pclass == CLASS_HIGH_MAGE || p_ptr->pclass == CLASS_SORCERER) beam = plev + 10;
	else beam = plev / 2;

	if (summon_lev < 1) summon_lev = 1;
	if (!success || (randint1(50+plev) < plev/10)) unique_okay = TRUE;
	switch (spell)
	{
		case 0: /* Phase Door */
			if (success)
			{
				teleport_player(10);
			}
			break;
		case 1: /* Trump Spiders */
		{
			bool pet = success; /* (randint1(5) > 2) */
			u32b mode = PM_ALLOW_GROUP;

			if (pet) mode |= PM_FORCE_PET;
			else mode |= PM_NO_PET;

#ifdef JP
msg_print("あなたは蜘蛛のカードに集中する...");
#else
			msg_print("You concentrate on the trump of an spider...");
#endif

			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_SPIDER, mode))
			{
				if (!pet)
#ifdef JP
msg_print("召還された蜘蛛は怒っている！");
#else
					msg_print("The summoned spiders get angry!");
#endif
			}
			else
			{
				no_trump = TRUE;
			}

			break;
		}
		case 2: /* Shuffle */
			if (success)
			{
				/* A limited power 'wonder' spell */
				int die = randint1(120);
				int vir = virtue_number(V_CHANCE);

				if ((p_ptr->pclass == CLASS_ROGUE) ||
					(p_ptr->pclass == CLASS_HIGH_MAGE) ||
				        (p_ptr->pclass == CLASS_SORCERER))
					die = (randint1(110)) + plev / 5;
				/* Card sharks and high mages get a level bonus */

				if (vir)
				{
					if (p_ptr->virtues[vir - 1] > 0)
					{
						while (randint1(400) < p_ptr->virtues[vir - 1]) die++;
					}
					else
					{
						while (randint1(400) < (0-p_ptr->virtues[vir - 1])) die--;
					}
				}

#ifdef JP
msg_print("あなたはカードを切って一枚引いた...");
#else
				msg_print("You shuffle the deck and draw a card...");
#endif

				if (die < 30)
					chg_virtue(V_CHANCE, 1);

				if (die < 7)
				{
#ifdef JP
msg_print("なんてこった！《死》だ！");
#else
					msg_print("Oh no! It's Death!");
#endif

					for (dummy = 0; dummy < randint1(3); dummy++)
						(void)activate_hi_summon(py, px, FALSE);
				}
				else if (die < 14)
				{
#ifdef JP
msg_print("なんてこった！《悪魔》だ！");
#else
					msg_print("Oh no! It's the Devil!");
#endif

					(void)summon_specific(0, py, px, dun_level, SUMMON_DEMON, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET));
				}
				else if (die < 18)
				{
					int count = 0;
#ifdef JP
msg_print("なんてこった！《吊られた男》だ！");
#else
					msg_print("Oh no! It's the Hanged Man.");
#endif

					(void)activate_ty_curse(FALSE, &count);
				}
				else if (die < 22)
				{
#ifdef JP
msg_print("《不調和の剣》だ。");
#else
					msg_print("It's the swords of discord.");
#endif

					aggravate_monsters(0);
				}
				else if (die < 26)
				{
#ifdef JP
msg_print("《愚者》だ。");
#else
					msg_print("It's the Fool.");
#endif

					(void)do_dec_stat(A_INT);
					(void)do_dec_stat(A_WIS);
				}
				else if (die < 30)
				{
#ifdef JP
msg_print("奇妙なモンスターの絵だ。");
#else
					msg_print("It's the picture of a strange monster.");
#endif

					if (!(summon_specific(0, py, px, (dun_level * 3) / 2, 32 + randint1(6), (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET))))
						no_trump = TRUE;
				}
				else if (die < 33)
				{
#ifdef JP
msg_print("《月》だ。");
#else
					msg_print("It's the Moon.");
#endif

					unlite_area(10, 3);
				}
				else if (die < 38)
				{
#ifdef JP
msg_print("《運命の輪》だ。");
#else
					msg_print("It's the Wheel of Fortune.");
#endif

					wild_magic(randint0(32));
				}
				else if (die < 40)
				{
#ifdef JP
msg_print("テレポート・カードだ。");
#else
					msg_print("It's a teleport trump card.");
#endif

					teleport_player(10);
				}
				else if (die < 42)
				{
#ifdef JP
msg_print("《正義》だ。");
#else
					msg_print("It's Justice.");
#endif

					set_blessed(p_ptr->lev, FALSE);
				}
				else if (die < 47)
				{
#ifdef JP
msg_print("テレポート・カードだ。");
#else
					msg_print("It's a teleport trump card.");
#endif

					teleport_player(100);
				}
				else if (die < 52)
				{
#ifdef JP
msg_print("テレポート・カードだ。");
#else
					msg_print("It's a teleport trump card.");
#endif

					teleport_player(200);
				}
				else if (die < 60)
				{
#ifdef JP
msg_print("《塔》だ。");
#else
					msg_print("It's the Tower.");
#endif

					wall_breaker();
				}
				else if (die < 72)
				{
#ifdef JP
msg_print("《節制》だ。");
#else
					msg_print("It's Temperance.");
#endif

					sleep_monsters_touch();
				}
				else if (die < 80)
				{
#ifdef JP
msg_print("《塔》だ。");
#else
					msg_print("It's the Tower.");
#endif

					earthquake(py, px, 5);
				}
				else if (die < 82)
				{
#ifdef JP
msg_print("友好的なモンスターの絵だ。");
#else
					msg_print("It's the picture of a friendly monster.");
#endif

					if (!(summon_specific(-1, py, px, (dun_level * 3) / 2, SUMMON_BIZARRE1, PM_FORCE_PET)))
						no_trump = TRUE;
				}
				else if (die < 84)
				{
#ifdef JP
msg_print("友好的なモンスターの絵だ。");
#else
					msg_print("It's the picture of a friendly monster.");
#endif

					if (!(summon_specific(-1, py, px, (dun_level * 3) / 2, SUMMON_BIZARRE2, PM_FORCE_PET)))
						no_trump = TRUE;
				}
				else if (die < 86)
				{
#ifdef JP
msg_print("友好的なモンスターの絵だ。");
#else
					msg_print("It's the picture of a friendly monster.");
#endif

					if (!(summon_specific(-1, py, px, (dun_level * 3) / 2, SUMMON_BIZARRE4, PM_FORCE_PET)))
						no_trump = TRUE;
				}
				else if (die < 88)
				{
#ifdef JP
msg_print("友好的なモンスターの絵だ。");
#else
					msg_print("It's the picture of a friendly monster.");
#endif

					if (!(summon_specific(-1, py, px, (dun_level * 3) / 2, SUMMON_BIZARRE5, PM_FORCE_PET)))
						no_trump = TRUE;
				}
				else if (die < 96)
				{
#ifdef JP
msg_print("《恋人》だ。");
#else
					msg_print("It's the Lovers.");
#endif

					if (get_aim_dir(&dir))
						(void)charm_monster(dir, MIN(p_ptr->lev, 20));
				}
				else if (die < 101)
				{
#ifdef JP
msg_print("《隠者》だ。");
#else
					msg_print("It's the Hermit.");
#endif

					wall_stone();
				}
				else if (die < 111)
				{
#ifdef JP
msg_print("《審判》だ。");
#else
					msg_print("It's the Judgement.");
#endif

					do_cmd_rerate(FALSE);
					if (p_ptr->muta1 || p_ptr->muta2 || p_ptr->muta3)
					{
#ifdef JP
msg_print("全ての突然変異が治った。");
#else
						msg_print("You are cured of all mutations.");
#endif

						p_ptr->muta1 = p_ptr->muta2 = p_ptr->muta3 = 0;
						p_ptr->update |= PU_BONUS;
						handle_stuff();
					}
				}
				else if (die < 120)
				{
#ifdef JP
msg_print("《太陽》だ。");
#else
					msg_print("It's the Sun.");
#endif

					chg_virtue(V_KNOWLEDGE, 1);
					chg_virtue(V_ENLIGHTEN, 1);
					wiz_lite(FALSE, FALSE);
				}
				else
				{
#ifdef JP
msg_print("《世界》だ。");
#else
					msg_print("It's the World.");
#endif

					if (p_ptr->exp < PY_MAX_EXP)
					{
						s32b ee = (p_ptr->exp / 25) + 1;
						if (ee > 5000) ee = 5000;
#ifdef JP
msg_print("更に経験を積んだような気がする。");
#else
						msg_print("You feel more experienced.");
#endif

						gain_exp(ee);
					}
				}
			}
			break;
		case 3: /* Reset Recall */
			if (success)
			{
				if (!reset_recall()) return FALSE;
			}
			break;
		case 4: /* Teleport Self */
			if (success)
			{
				teleport_player(plev * 4);
			}
			break;
		case 5: /* Trump Spying */
			if (success)
			{
				(void)set_tim_esp(randint1(30) + 25, FALSE);
			}
			break;
		case 6: /* Teleport Away */
			if (success)
			{
				if (!get_aim_dir(&dir)) return FALSE;
				(void)fire_beam(GF_AWAY_ALL, dir, plev);
			}
			break;
		case 7: /* Trump Animals */
		{
			bool pet = success; /* was (randint1(5) > 2) */
			int type = (pet ? SUMMON_ANIMAL_RANGER : SUMMON_ANIMAL);
			u32b mode = 0L;

			if (pet) mode |= PM_FORCE_PET;
			else mode |= (PM_ALLOW_GROUP | PM_NO_PET);

#ifdef JP
msg_print("あなたは動物のカードに集中する...");
#else
			msg_print("You concentrate on the trump of an animal...");
#endif


			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, type, mode))
			{
				if (!pet)
#ifdef JP
msg_print("召喚された動物は怒っている！");
#else
					msg_print("The summoned animal gets angry!");
#endif

			}
			else
			{
				no_trump = TRUE;
			}

			break;
		}
		case 8: /* Trump Reach */
			if (success)
			{
				if (!get_aim_dir(&dir)) return FALSE;
				fetch(dir, plev * 15, TRUE);
			}
			break;
		case 9: /* Trump Kamikaze */
		{
			int x = px, y = py;
			if (success)
			{
				if (!target_set(TARGET_KILL)) return FALSE;
				x = target_col;
				y = target_row;
			}
			no_trump = TRUE;

#ifdef JP
msg_print("あなたはカミカゼのカードに集中する...");
#else
			msg_print("You concentrate on several trumps at once...");
#endif


			for (dummy = 2 + randint0(plev / 7); dummy > 0; dummy--)
			{
				bool pet = success; /* was (randint1(10) > 3) */
				u32b mode = 0L;
				int type;

				if (pet) mode |= PM_FORCE_PET;
				else mode |= (PM_ALLOW_GROUP | PM_NO_PET);

				if (p_ptr->pclass == CLASS_BEASTMASTER)
				{
					type = SUMMON_KAMIKAZE_LIVING;
				}
				else
				{
					type = SUMMON_KAMIKAZE;
				}

				if (summon_specific((pet ? -1 : 0), y, x, summon_lev, type, mode))
				{
					if (!pet)
#ifdef JP
msg_print("召還されたモンスターは怒っている！");
#else
						msg_print("The summoned creatures get angry!");
#endif

					no_trump = FALSE;
				}
			}
			break;
		}
		case 10: /* Phantasmal Servant */
			if (success)
			{
				if (summon_specific(-1, py, px, (summon_lev * 3) / 2, SUMMON_PHANTOM, PM_FORCE_PET))
				{
#ifdef JP
msg_print("御用でございますか、御主人様？");
#else
					msg_print("'Your wish, master?'");
#endif

				}
				else
				{
					no_trump = TRUE;
				}
			}
			break;
		case 11: /* Speed Monster */
			if (success)
			{
				bool old_target_pet = target_pet;
				target_pet = TRUE;
				if (!get_aim_dir(&dir))
				{
					target_pet = old_target_pet;
					return (FALSE);
				}
				target_pet = old_target_pet;
				(void)speed_monster(dir);
			}
			break;
		case 12: /* Teleport Level */
			if (success)
			{
#ifdef JP
				if (!get_check("本当に他の階にテレポートしますか？")) return FALSE;
#else
				if (!get_check("Are you sure? (Teleport Level)")) return FALSE;
#endif
				(void)teleport_player_level();
			}
			break;
		case 13: /* Dimension Door */
			if (success)
			{
#ifdef JP
msg_print("次元の扉が開いた。目的地を選んで下さい。");
#else
				msg_print("You open a dimensional gate. Choose a destination.");
#endif

				return dimension_door();
			}
			break;
		case 14: /* Word of Recall */
			if (success)
			{
				if (!word_of_recall()) return FALSE;
			}
			break;
		case 15: /* Banish */
			if (success)
			{
				banish_monsters(plev * 4);
			}
			break;
		case 16: /* Swap Position */
		{
			if (success)
			{
				project_length = -1;
				if (!get_aim_dir(&dir))
				{
					project_length = 0;
					return FALSE;
				}
				project_length = 0;

				(void)teleport_swap(dir);
			}
			break;
		}
		case 17: /* Trump Undead */
		{
			bool pet = success; /* (randint1(10) > 3) */
			u32b mode = 0L;

			if (pet) mode |= PM_FORCE_PET;
			else mode |= (PM_ALLOW_GROUP | PM_NO_PET);

#ifdef JP
msg_print("あなたはアンデッドのカードに集中する...");
#else
			msg_print("You concentrate on the trump of an undead creature...");
#endif


			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_UNDEAD, mode))
			{
				if (!pet)
#ifdef JP
msg_print("召還されたアンデッドは怒っている！");
#else
					msg_print("The summoned undead creature gets angry!");
#endif

			}
			else
			{
				no_trump = TRUE;
			}

			break;
		}
		case 18: /* Trump Reptiles */
		{
			bool pet = success; /* was (randint1(5) > 2) */
			u32b mode = 0L;

			if (pet) mode |= PM_FORCE_PET;
			else mode |= (PM_ALLOW_GROUP | PM_NO_PET);

#ifdef JP
msg_print("あなたは爬虫類のカードに集中する...");
#else
			msg_print("You concentrate on the trump of a reptile...");
#endif


			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_HYDRA, mode))
			{
				if (!pet)
#ifdef JP
msg_print("召還された爬虫類は怒っている！");
#else
					msg_print("The summoned reptile gets angry!");
#endif

			}
			else
			{
				no_trump = TRUE;
			}

			break;
		}
		case 19: /* Trump Monsters */
		{
			no_trump = TRUE;

#ifdef JP
msg_print("あなたはモンスターのカードに集中する...");
#else
			msg_print("You concentrate on several trumps at once...");
#endif


			for (dummy = 0; dummy < 1 + ((plev - 15)/ 10); dummy++)
			{
				bool pet = success; /* was (randint1(10) > 3) */
				int type;
				u32b mode = 0L;

				if (pet) mode |= PM_FORCE_PET;
				else mode |= (PM_ALLOW_GROUP | PM_NO_PET);

				if (unique_okay) mode |= PM_ALLOW_UNIQUE;

				if (p_ptr->pclass == CLASS_BEASTMASTER)
				{
					type = SUMMON_LIVING;
				}
				else
				{
					type = 0;
				}

				if (summon_specific((pet ? -1 : 0), py, px, summon_lev, type, mode))
				{
					if (!pet)
#ifdef JP
msg_print("召還されたモンスターは怒っている！");
#else
						msg_print("The summoned creatures get angry!");
#endif

					no_trump = FALSE;
				}
			}
			break;
		}
		case 20: /* Trump Hounds */
		{
			bool pet = success; /* was (randint1(5) > 2) */
			u32b mode = PM_ALLOW_GROUP;

			if (pet) mode |= PM_FORCE_PET;
			else mode |= PM_NO_PET;

#ifdef JP
msg_print("あなたはハウンドのカードに集中する...");
#else
			msg_print("You concentrate on the trump of a hound...");
#endif


			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_HOUND, mode))
			{
				if (!pet)
#ifdef JP
msg_print("召還されたハウンドは怒っている！");
#else
					msg_print("The summoned hounds get angry!");
#endif

			}
			else
			{
				no_trump = TRUE;
			}

			break;
		}
		case 21: /* Trump Branding */
			if (success)
			{
				brand_weapon(5);
			}
			break;
		case 22: /* Living Trump */
			if (success)
			{
				if (one_in_(7))
					/* Teleport control */
					dummy = 12;
				else
					/* Random teleportation (uncontrolled) */
					dummy = 77;
				/* Gain the mutation */
				if (gain_random_mutation(dummy))
#ifdef JP
msg_print("あなたは生きているカードに変わった。");
#else
					msg_print("You have turned into a Living Trump.");
#endif

			}
			break;
		case 23: /* Trump Cyberdemon */
		{
			bool pet = success; /* was (randint1(10) > 3) */
			u32b mode = 0L;

			if (pet) mode |= PM_FORCE_PET;
			else mode |= PM_NO_PET;

#ifdef JP
msg_print("あなたはサイバーデーモンのカードに集中する...");
#else
			msg_print("You concentrate on the trump of a Cyberdemon...");
#endif


			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_CYBER, mode))
			{
				if (!pet)
#ifdef JP
msg_print("召還されたサイバーデーモンは怒っている！");
#else
					msg_print("The summoned Cyberdemon gets angry!");
#endif

			}
			else
			{
					no_trump = TRUE;
			}

			break;
		}
		case 24: /* Trump Divination */
			if (success)
			{
				(void)detect_all(DETECT_RAD_DEFAULT);
			}
			break;
		case 25: /* Trump Lore */
			if (success)
			{
				return identify_fully(FALSE);
			}
			break;
		case 26: /* Heal Monster */
			if (success)
			{
				bool old_target_pet = target_pet;
				target_pet = TRUE;
				if (!get_aim_dir(&dir))
				{
					target_pet = old_target_pet;
					return (FALSE);
				}
				target_pet = old_target_pet;

				(void)heal_monster(dir, p_ptr->lev * 10 + 200);
			}
			break;
		case 27: /* Trump Dragon */
		{
			bool pet = success; /* was (randint1(10) > 3) */
			u32b mode = 0L;

			if (pet) mode |= PM_FORCE_PET;
			else mode |= (PM_ALLOW_GROUP | PM_NO_PET);

#ifdef JP
msg_print("あなたはドラゴンのカードに集中する...");
#else
			msg_print("You concentrate on the trump of a dragon...");
#endif


			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_DRAGON, mode))
			{
				if (!pet)
#ifdef JP
msg_print("召還されたドラゴンは怒っている！");
#else
					msg_print("The summoned dragon gets angry!");
#endif

			}
			else
			{
				no_trump = TRUE;
			}

			break;
		}
		case 28: /* Trump Meteor */
			if (success)
			{
				int x, y, dx, dy, i;
				int b = 10 + randint1(10);
				for (i = 0; i < b; i++)
				{
					int count = 0, d = 0;

					while (1)
					{
						count++;
						if (count > 20) break;
						x = px - 8 + randint0(17);
						y = py - 8 + randint0(17);

						if (!in_bounds(y,x) || (!cave_floor_bold(y,x) && (cave[y][x].feat != FEAT_TREES)) || !player_has_los_bold(y, x)) continue;

						dx = (px > x) ? (px - x) : (x - px);
						dy = (py > y) ? (py - y) : (y - py);

						/* Approximate distance */
						d = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));
						if (d < 9) break;
					}

					if (count > 20) continue;

					project(0, 2, y, x, plev * 2, GF_METEOR, PROJECT_KILL | PROJECT_JUMP | PROJECT_ITEM, -1);
				}
			}
			break;
		case 29: /* Trump Demon */
		{
			bool pet = success; /* was (randint1(10) > 3) */
			u32b mode = 0L;

			if (pet) mode |= PM_FORCE_PET;
			else mode |= (PM_ALLOW_GROUP | PM_NO_PET);

#ifdef JP
msg_print("あなたはデーモンのカードに集中する...");
#else
			msg_print("You concentrate on the trump of a demon...");
#endif


			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_DEMON, mode))
			{
				if (!pet)
#ifdef JP
msg_print("召還されたデーモンは怒っている！");
#else
					msg_print("The summoned demon gets angry!");
#endif

			}
			else
			{
				no_trump = TRUE;
			}

			break;
		}
		case 30: /* Trump Greater Undead */
		{
			bool pet = success; /* was (randint1(10) > 3) */
			u32b mode = 0L;

			if (pet) mode |= PM_FORCE_PET;
			else mode |= (PM_ALLOW_GROUP | PM_NO_PET);

			if (unique_okay) mode |= PM_ALLOW_UNIQUE;

#ifdef JP
msg_print("あなたは強力なアンデッドのカードに集中する...");
#else
			msg_print("You concentrate on the trump of a greater undead being...");
#endif


			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_HI_UNDEAD, mode))
			{
				if (!pet)
#ifdef JP
msg_print("召還された上級アンデッドは怒っている！");
#else
					msg_print("The summoned greater undead creature gets angry!");
#endif

			}
			else
			{
				no_trump = TRUE;
			}

			break;
		}
		case 31: /* Trump Ancient Dragon */
		{
			bool pet = success; /* was (randint1(10) > 3) */
			int type;
			u32b mode = 0L;

			if (pet) mode |= PM_FORCE_PET;
			else mode |= (PM_ALLOW_GROUP | PM_NO_PET);

			if (unique_okay) mode |= PM_ALLOW_UNIQUE;

			if (p_ptr->pclass == CLASS_BEASTMASTER)
			{
				type = SUMMON_HI_DRAGON_LIVING;
			}
			else
			{
				type = SUMMON_HI_DRAGON;
			}

#ifdef JP
msg_print("あなたは古代ドラゴンのカードに集中する...");
#else
			msg_print("You concentrate on the trump of an ancient dragon...");
#endif


			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, type, mode))
			{
				if (!pet)
#ifdef JP
msg_print("召還された古代ドラゴンは怒っている！");
#else
					msg_print("The summoned ancient dragon gets angry!");
#endif

			}
			else
			{
				no_trump = TRUE;
			}

			break;
		}
		default:
#ifdef JP
msg_format("未知のカードの呪文です: %d", spell);
#else
			msg_format("You cast an unknown Trump spell: %d.", spell);
#endif

			msg_print(NULL);
	}

	if (no_trump)
	{
#ifdef JP
msg_print("誰もあなたのカードの呼び声に答えない。");
#else
		msg_print("Nobody answers to your Trump call.");
#endif

	}

	return TRUE;
}


static bool cast_arcane_spell(int spell)
{
	int	dir;
	int	beam;
	int	plev = p_ptr->lev;
	int	dummy = 0;
	bool	no_trump = FALSE;

	if (p_ptr->pclass == CLASS_MAGE) beam = plev;
	else if (p_ptr->pclass == CLASS_HIGH_MAGE || p_ptr->pclass == CLASS_SORCERER) beam = plev + 10;
	else beam = plev / 2;

	switch (spell)
	{
	case 0: /* Zap */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_bolt_or_beam(beam - 10, GF_ELEC, dir,
			damroll(3 + ((plev - 1) / 5), 3));
		break;
	case 1: /* Wizard Lock */
		if (!get_aim_dir(&dir)) return FALSE;

		(void)wizard_lock(dir);
		break;
	case 2: /* Detect Invisibility */
		(void)detect_monsters_invis(DETECT_RAD_DEFAULT);
		break;
	case 3: /* Detect Monsters */
		(void)detect_monsters_normal(DETECT_RAD_DEFAULT);
		break;
	case 4: /* Blink */
		teleport_player(10);
		break;
	case 5: /* Light Area */
		(void)lite_area(damroll(2, (plev / 2)), (plev / 10) + 1);
		break;
	case 6: /* Trap & Door Destruction */
		if (!get_aim_dir(&dir)) return FALSE;

		(void)destroy_door(dir);
		break;
	case 7: /* Cure Light Wounds */
		(void)hp_player(damroll(2, 8));
		(void)set_cut(p_ptr->cut - 10);
		break;
	case 8: /* Detect Doors & Traps */
		(void)detect_traps(DETECT_RAD_DEFAULT, TRUE);
		(void)detect_doors(DETECT_RAD_DEFAULT);
		(void)detect_stairs(DETECT_RAD_DEFAULT);
		break;
	case 9: /* Phlogiston */
		phlogiston();
		break;
	case 10: /* Detect Treasure */
		(void)detect_treasure(DETECT_RAD_DEFAULT);
		(void)detect_objects_gold(DETECT_RAD_DEFAULT);
		break;
	case 11: /* Detect Enchantment */
		(void)detect_objects_magic(DETECT_RAD_DEFAULT);
		break;
	case 12: /* Detect Objects */
		(void)detect_objects_normal(DETECT_RAD_DEFAULT);
		break;
	case 13: /* Cure Poison */
		(void)set_poisoned(0);
		break;
	case 14: /* Resist Cold */
		(void)set_oppose_cold(randint1(20) + 20, FALSE);
		break;
	case 15: /* Resist Fire */
		(void)set_oppose_fire(randint1(20) + 20, FALSE);
		break;
	case 16: /* Resist Lightning */
		(void)set_oppose_elec(randint1(20) + 20, FALSE);
		break;
	case 17: /* Resist Acid */
		(void)set_oppose_acid(randint1(20) + 20, FALSE);
		break;
	case 18: /* Cure Medium Wounds */
		(void)hp_player(damroll(4, 8));
		(void)set_cut((p_ptr->cut / 2) - 50);
		break;
	case 19: /* Teleport */
		teleport_player(plev * 5);
		break;
	case 20: /* Identify */
		return ident_spell(FALSE);
	case 21: /* Stone to Mud */
		if (!get_aim_dir(&dir)) return FALSE;

		(void)wall_to_mud(dir);
		break;
	case 22: /* Ray of Light */
		if (!get_aim_dir(&dir)) return FALSE;

#ifdef JP
msg_print("光線が放たれた。");
#else
		msg_print("A line of light appears.");
#endif

		(void)lite_line(dir);
		break;
	case 23: /* Satisfy Hunger */
		(void)set_food(PY_FOOD_MAX - 1);
		break;
	case 24: /* See Invisible */
		(void)set_tim_invis(randint1(24) + 24, FALSE);
		break;
	case 25: /* Conjure Elemental */
		if (!summon_specific(-1, py, px, plev, SUMMON_ELEMENTAL, (PM_ALLOW_GROUP | PM_FORCE_PET)))
			no_trump = TRUE;
		break;
	case 26: /* Teleport Level */
#ifdef JP
		if (!get_check("本当に他の階にテレポートしますか？")) return FALSE;
#else
		if (!get_check("Are you sure? (Teleport Level)")) return FALSE;
#endif
		(void)teleport_player_level();
		break;
	case 27: /* Teleport Away */
		if (!get_aim_dir(&dir)) return FALSE;

		(void)fire_beam(GF_AWAY_ALL, dir, plev);
		break;
	case 28: /* Elemental Ball */
		if (!get_aim_dir(&dir)) return FALSE;

		switch (randint1(4))
		{
			case 1:  dummy = GF_FIRE;break;
			case 2:  dummy = GF_ELEC;break;
			case 3:  dummy = GF_COLD;break;
			default: dummy = GF_ACID;break;
		}
		fire_ball(dummy, dir, 75 + (plev), 2);
		break;
	case 29: /* Detection */
		(void)detect_all(DETECT_RAD_DEFAULT);
		break;
	case 30: /* Word of Recall */
		if (!word_of_recall()) return FALSE;
		break;
	case 31: /* Clairvoyance */
		chg_virtue(V_KNOWLEDGE, 1);
		chg_virtue(V_ENLIGHTEN, 1);
		wiz_lite(FALSE, FALSE);
		if (!p_ptr->telepathy)
		{
			(void)set_tim_esp(randint1(30) + 25, FALSE);
		}
		break;
	default:
		msg_format("You cast an unknown Arcane spell: %d.", spell);
		msg_print(NULL);
	}

	if (no_trump)
#ifdef JP
msg_print("エレメンタルは現れなかった。");
#else
		msg_print("No Elementals arrive.");
#endif

	return TRUE;
}


static bool cast_enchant_spell(int spell)
{
	int	plev = p_ptr->lev;
	int	dummy = 0;
	bool	no_trump = FALSE;

	switch (spell)
	{
	case 0: /* Infravision */
		set_tim_infra(100 + randint1(100), FALSE);
		break;
	case 1: /* Regeneration */
		set_tim_regen(80 + randint1(80), FALSE);
		break;
	case 2: /* Satisfy Hunger */
		(void)set_food(PY_FOOD_MAX - 1);
		break;
	case 3: /* Resist Cold */
		(void)set_oppose_cold(randint1(20) + 20, FALSE);
		break;
	case 4: /* Resist Fire */
		(void)set_oppose_fire(randint1(20) + 20, FALSE);
		break;
	case 5: /* Heroism */
		(void)set_hero(randint1(25) + 25, FALSE);
		(void)hp_player(10);
		(void)set_afraid(0);
		break;
	case 6: /* Resist Lightning */
		(void)set_oppose_elec(randint1(20) + 20, FALSE);
		break;
	case 7: /* Resist Acid */
		(void)set_oppose_acid(randint1(20) + 20, FALSE);
		break;
	case 8: /* See Invisibility */
		(void)set_tim_invis(randint1(24) + 24, FALSE);
		break;
	case 9: /* Remove Curse */
		if (remove_curse())
		{
#ifdef JP
			msg_print("誰かに見守られているような気がする。");
#else
			msg_print("You feel as if someone is watching over you.");
#endif
		}
		break;
	case 10: /* Resist Poison */
		(void)set_oppose_pois(randint1(20) + 20, FALSE);
		break;
	case 11: /* Berserk */
		(void)set_shero(randint1(25) + 25, FALSE);
		(void)hp_player(30);
		(void)set_afraid(0);
		break;
	case 12: /* Self Knowledge */
		(void)self_knowledge();
		break;
	case 13: /* Protection from Evil */
		(void)set_protevil(randint1(25) + 3 * p_ptr->lev, FALSE);
		break;
	case 14: /* Healing */
		set_poisoned(0);
		set_stun(0);
		set_cut(0);
		set_image(0);
		break;
	case 15: /* Mana Branding */
		return choose_ele_attack();
		break;
	case 16: /* Telepathy */
		(void)set_tim_esp(randint1(30) + 25, FALSE);
		break;
	case 17: /* Stone Skin */
		(void)set_shield(randint1(20) + 30, FALSE);
		break;
	case 18: /* Resistance */
		(void)set_oppose_acid(randint1(20) + 20, FALSE);
		(void)set_oppose_elec(randint1(20) + 20, FALSE);
		(void)set_oppose_fire(randint1(20) + 20, FALSE);
		(void)set_oppose_cold(randint1(20) + 20, FALSE);
		(void)set_oppose_pois(randint1(20) + 20, FALSE);
		break;
	case 19: /* Haste */
		(void)set_fast(randint1(20 + plev) + plev, FALSE);
		break;
	case 20: /* Walk through Wall */
		(void)set_kabenuke(randint1(plev/2) + plev/2, FALSE);
		break;
	case 21: /* Pulish Shield */
		(void)pulish_shield();
		break;
	case 22: /* Create Golem */
		if (summon_specific(-1, py, px, plev, SUMMON_GOLEM, PM_FORCE_PET))
		{
#ifdef JP
msg_print("ゴーレムを作った。");
#else
		msg_print("You make a golem.");
#endif
		}
		else
		{
			no_trump = TRUE;
		}
		break;
	case 23: /* Magic armor */
		(void)set_magicdef(randint1(20) + 20, FALSE);
		break;
	case 24: /* Remove Enchantment */
		if (!mundane_spell(TRUE)) return FALSE;
		break;
	case 25: /* Remove All Curse */
		if (remove_all_curse())
		{
#ifdef JP
			msg_print("誰かに見守られているような気がする。");
#else
			msg_print("You feel as if someone is watching over you.");
#endif
		}
		break;
	case 26: /* Total Knowledge */
		return identify_fully(FALSE);
		break;
	case 27: /* Enchant Weapon */
		return enchant_spell(randint0(4) + 1, randint0(4) + 1, 0);
		break;
	case 28: /* Enchant Armor */
		return enchant_spell(0, 0, randint0(3) + 2);
		break;
	case 29: /* Brand Weapon */
		brand_weapon(randint0(18));
		break;
	case 30: /* Living Trump */
		if (one_in_(7))
			/* Teleport control */
			dummy = 12;
		else
			/* Random teleportation (uncontrolled) */
			dummy = 77;
		/* Gain the mutation */
		if (gain_random_mutation(dummy))
#ifdef JP
msg_print("あなたは生きているカードに変わった。");
#else
			msg_print("You have turned into a Living Trump.");
#endif
		break;
	case 31: /* Immune */
		return (choose_ele_immune(13 + randint1(13)));
		break;
	default:
		msg_format("You cast an unknown Craft spell: %d.", spell);
		msg_print(NULL);
	}

	if (no_trump)
#ifdef JP
msg_print("うまくゴーレムを作れなかった。");
#else
		msg_print("No Golems arrive.");
#endif

	return TRUE;
}


/*
 * An "item_tester_hook" for offer
 */
static bool item_tester_offer(object_type *o_ptr)
{
	/* Flasks of oil are okay */
	if (o_ptr->tval != TV_CORPSE) return (FALSE);

	if (o_ptr->sval != SV_CORPSE) return (FALSE);

	if (strchr("pht", r_info[o_ptr->pval].d_char)) return (TRUE);

	/* Assume not okay */
	return (FALSE);
}


static bool cast_daemon_spell(int spell)
{
	int	dir, beam;
	int	plev = p_ptr->lev;

	if (p_ptr->pclass == CLASS_MAGE) beam = plev;
	else if (p_ptr->pclass == CLASS_HIGH_MAGE || p_ptr->pclass == CLASS_SORCERER) beam = plev + 10;
	else beam = plev / 2;

	switch (spell)
	{
	case 0: /* Magic Missile */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_bolt_or_beam(beam - 10, GF_MISSILE, dir,
			damroll(3 + ((plev - 1) / 5), 4));
		break;
	case 1: /* Detect Undead & Demons -> Unlife */
		(void)detect_monsters_nonliving(DETECT_RAD_DEFAULT);
		break;
	case 2: /* Bless */
		(void)set_blessed(randint1(12) + 12, FALSE);
		break;
	case 3: /* Resist Fire */
		(void)set_oppose_fire(randint1(20) + 20, FALSE);
		break;
	case 4: /* Horrify */
		if (!get_aim_dir(&dir)) return FALSE;

		(void)fear_monster(dir, plev);
		(void)stun_monster(dir, plev);
		break;
	case 5: /* Nether Bolt */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_bolt_or_beam(beam, GF_NETHER, dir,
		    damroll(6 + ((plev - 5) / 4), 8));
		break;
	case 6: /* Summon monster, demon */
		if (!summon_specific(-1, py, px, (plev * 3) / 2, SUMMON_MANES, (PM_ALLOW_GROUP | PM_FORCE_PET)))
		{
#ifdef JP
msg_print("古代の死霊は現れなかった。");
#else
			msg_print("No Manes arrive.");
#endif
		}
		break;
	case 7: /* Mini Hellfire */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_ball(GF_HELL_FIRE, dir,
			(damroll(3, 6) + plev +
			(plev / (((p_ptr->pclass == CLASS_MAGE) ||
			(p_ptr->pclass == CLASS_HIGH_MAGE) ||
			(p_ptr->pclass == CLASS_SORCERER)) ? 2 : 4))),
			((plev < 30) ? 2 : 3));
		break;
	case 8: /* Enslave Demon */
		if (!get_aim_dir(&dir)) return FALSE;

		(void)control_one_demon(dir, plev);
		break;
	case 9: /* Vision */
		map_area(DETECT_RAD_MAP);
		break;
	case 10: /* Resist Nether */
		(void)set_tim_res_nether(randint1(20) + 20, FALSE);
		break;
	case 11: /* Plasma Bolt */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_bolt_or_beam(beam, GF_PLASMA, dir,
		    damroll(11 + ((plev - 5) / 4), 8));
		break;
	case 12: /* Fire Ball */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_ball(GF_FIRE, dir, plev + 55, 2);
		break;
	case 13: /* Fire Branding */
		brand_weapon(1);
		break;
	case 14: /* Nether Ball */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_ball(GF_NETHER, dir, plev*3/2 + 100, plev / 20+2);
		break;
	case 15: /* Summon monster, demon */
	{
		bool pet = !one_in_(3);
		u32b mode = 0L;

		if (pet) mode |= PM_FORCE_PET;
		else mode |= PM_NO_PET;
		if (!(pet && (plev < 50))) mode |= PM_ALLOW_GROUP;

		if (summon_specific((pet ? -1 : 0), py, px, plev*2/3+randint1(plev/2), SUMMON_DEMON, mode))
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
msg_print("「卑しき者よ、我は汝の下僕にあらず！ お前の魂を頂くぞ！」");
#else
				msg_print("'NON SERVIAM! Wretch! I shall feast on thy mortal soul!'");
#endif

		}
		else
		{
#ifdef JP
msg_print("悪魔は現れなかった。");
#else
			msg_print("No Greater Demon arrive.");
#endif
		}
		break;
	}
	case 16: /* Telepathy */
		(void)set_tim_esp(randint1(30) + 25, FALSE);
		break;
	case 17: /* Demoncloak */
	{
		int dur=randint1(20) + 20;
			
		set_oppose_fire(dur, FALSE);
		set_oppose_cold(dur, FALSE);
		set_tim_sh_fire(dur, FALSE);
		set_afraid(0);
		break;
	}
	case 18: /* Rain of Lava */
		fire_ball(GF_FIRE, 0, (55 + plev)*2, 3);
		fire_ball_hide(GF_LAVA_FLOW, 0, 2+randint1(2), 3);
		break;
	case 19: /* Plasma ball */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_ball(GF_PLASMA, dir, plev*3/2 + 80, 2 + plev/40);
		break;
	case 20: /* Mimic demon */
		(void)set_mimic(10+plev/2 + randint1(10+plev/2), MIMIC_DEMON, FALSE);
		break;
	case 21: /* Nether Wave == Dispel Good */
		(void)dispel_monsters(randint1(plev * 2));
		(void)dispel_good(randint1(plev * 2));
		break;
	case 22: /*  */
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball(GF_NEXUS, dir, 100 + plev*2, 4);
		break;
	case 23: /* Hand Doom */
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("<破滅の手>を放った！");
#else
		else msg_print("You invokes the Hand of Doom!");
#endif

		fire_ball_hide(GF_HAND_DOOM, dir, plev * 2, 0);
		break;
	case 24: /* Heroism */
		(void)set_hero(randint1(25) + 25, FALSE);
		(void)hp_player(10);
		(void)set_afraid(0);
		break;
	case 25: /* Tim resist time */
		(void)set_tim_res_time(randint1(20)+20, FALSE);
		break;
	case 26: /* Circle of Madness */
		fire_ball(GF_CHAOS, 0, 50+plev, 3+plev/20);
		fire_ball(GF_CONFUSION, 0, 50+plev, 3+plev/20);
		fire_ball(GF_CHARM, 0, 20+plev, 3+plev/20);
		break;
	case 27: /* True Discharge Minion */
		discharge_minion();
		break;
	case 28: /* Summon Greater Demon */
	{
		int item;
		cptr q, s;
		int summon_lev;
		object_type *o_ptr;

		item_tester_hook = item_tester_offer;
#ifdef JP
		q = "どの死体を捧げますか? ";
		s = "捧げられる死体を持っていない。";
#else
		q = "Sacrifice which corpse? ";
		s = "You have nothing to scrifice.";
#endif
		if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))) return FALSE;

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

		summon_lev = p_ptr->lev * 2 / 3 + r_info[o_ptr->pval].level;
		if (summon_specific(-1, py, px, summon_lev, SUMMON_HI_DEMON, (PM_ALLOW_GROUP | PM_FORCE_PET)))
		{
#ifdef JP
msg_print("硫黄の悪臭が充満した。");
#else
			msg_print("The area fills with a stench of sulphur and brimstone.");
#endif


#ifdef JP
msg_print("「ご用でございますか、ご主人様」");
#else
			msg_print("'What is thy bidding... Master?'");
#endif

			/* Decrease the item (from the pack) */
			if (item >= 0)
			{
				inven_item_increase(item, -1);
				inven_item_describe(item);
				inven_item_optimize(item);
			}

			/* Decrease the item (from the floor) */
			else
			{
				floor_item_increase(0 - item, -1);
				floor_item_describe(0 - item);
				floor_item_optimize(0 - item);
			}
		}
		else
		{
#ifdef JP
msg_print("悪魔は現れなかった。");
#else
			msg_print("No Greater Demon arrive.");
#endif
		}
		break;
	}
	case 29: /* Nether Storm */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_ball(GF_NETHER, dir, plev*15, plev / 5);
		break;
	case 30: /* Blood curse */
	{
		if (!get_aim_dir(&dir)) return FALSE;

		fire_ball_hide(GF_BLOOD_CURSE, dir, 600, 0);
#ifdef JP
take_hit(DAMAGE_USELIFE, 20 + randint1(30), "血の呪い", -1);
#else
		take_hit(DAMAGE_USELIFE, 20 + randint1(30), "Blood curse", -1);
#endif
		break;
	}
	case 31: /* Mimic Demon lord */
		(void)set_mimic(15 + randint1(15), MIMIC_DEMON_LORD, FALSE);
		break;
	default:
		msg_format("You cast an unknown Daemon spell: %d.", spell);
		msg_print(NULL);
	}

	return TRUE;
}


static bool cast_crusade_spell(int spell)
{
	int	dir;
	int	beam;
	int	plev = p_ptr->lev;

	if (p_ptr->pclass == CLASS_MAGE) beam = plev;
	else if (p_ptr->pclass == CLASS_HIGH_MAGE || p_ptr->pclass == CLASS_SORCERER) beam = plev + 10;
	else beam = plev / 2;

	switch (spell)
	{
	case 0:
		if (!get_aim_dir(&dir)) return FALSE;

		fire_bolt_or_beam(beam - 10, GF_ELEC, dir,
			damroll(3 + ((plev - 1) / 5), 4));
		break;
	case 1:
		(void)detect_monsters_evil(DETECT_RAD_DEFAULT);
		break;
	case 2: /* Remove Fear */
		(void)set_afraid(0);
		break;
	case 3:
		if (!get_aim_dir(&dir)) return FALSE;

		(void)fear_monster(dir, plev);
		break;
	case 4:
		(void)sleep_monsters_touch();
		break;
	case 5:
		teleport_player(25+plev/2);
		break;
	case 6:
		if (!get_aim_dir(&dir)) return FALSE;
		fire_blast(GF_LITE, dir, 3+((plev-1)/9), 2, 10, 3);
		break;
	case 7:
		(void)set_cut(0);
		(void)set_poisoned(0);
		(void)set_stun(0);
		break;
	case 8:
		if (!get_aim_dir(&dir)) return FALSE;
		(void)fire_ball(GF_AWAY_EVIL, dir, MAX_SIGHT*5, 0);
		break;
	case 9: /* Holy Orb */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_ball(GF_HOLY_FIRE, dir,
		          (damroll(3, 6) + plev +
		          (plev / ((p_ptr->pclass == CLASS_PRIEST ||
		             p_ptr->pclass == CLASS_HIGH_MAGE ||
			     p_ptr->pclass == CLASS_SORCERER) ? 2 : 4))),
		          ((plev < 30) ? 2 : 3));

		break;
	case 10: /* Exorcism */
		(void)dispel_undead(randint1(plev));
		(void)dispel_demons(randint1(plev));
		(void)turn_evil(plev);
		break;
	case 11: /* Remove Curse */
		if (remove_curse())
		{
#ifdef JP
			msg_print("誰かに見守られているような気がする。");
#else
			msg_print("You feel as if someone is watching over you.");
#endif
		}
		break;
	case 12: /* Sense Unseen */
		(void)set_tim_invis(randint1(24) + 24, FALSE);
		break;
	case 13: /* Protection from Evil */
		(void)set_protevil(randint1(25) + 3 * p_ptr->lev, FALSE);
		break;
	case 14:
		if (!get_aim_dir(&dir)) return FALSE;
		(void)fire_bolt(GF_ELEC, dir, plev*5);
		break;
	case 15: /* Holy Word */
		(void)dispel_evil(randint1(plev * 6));
		(void)hp_player(100);
		(void)set_afraid(0);
		(void)set_poisoned(0);
		(void)set_stun(0);
		(void)set_cut(0);
		break;
	case 16:
		if (!get_aim_dir(&dir)) return FALSE;

		(void)destroy_door(dir);
		break;
	case 17:
		if (!get_aim_dir(&dir)) return FALSE;
		(void)stasis_evil(dir);
		break;
	case 18:
		set_tim_sh_holy(randint1(20)+20, FALSE);
		break;
	case 19: /* Dispel Undead + Demons */
		(void)dispel_undead(randint1(plev * 4));
		(void)dispel_demons(randint1(plev * 4));
		break;
	case 20: /* Dispel Evil */
		(void)dispel_evil(randint1(plev * 4));
		break;
	case 21:
		brand_weapon(13);
		break;
	case 22: /* Star Burst */
		if (!get_aim_dir(&dir)) return FALSE;

		fire_ball(GF_LITE, dir, 100+plev*2, 4);
		break;
	case 23: /* Summon monster, angel */
		{
			bool pet = !one_in_(3);
			u32b mode = 0L;

			if (pet) mode |= PM_FORCE_PET;
			else mode |= PM_NO_PET;
			if (!(pet && (plev < 50))) mode |= PM_ALLOW_GROUP;

			if (summon_specific((pet ? -1 : 0), py, px, (plev * 3) / 2, SUMMON_ANGEL, mode))
			{
				if (pet)
#ifdef JP
msg_print("「ご用でございますか、ご主人様」");
#else
					msg_print("'What is thy bidding... Master?'");
#endif

				else
#ifdef JP
msg_print("「我は汝の下僕にあらず！ 悪行者よ、悔い改めよ！」");
#else
					msg_print("Mortal! Repent of thy impiousness.");
#endif

			}
			break;
		}
	case 24: /* Heroism */
		(void)set_hero(randint1(25) + 25, FALSE);
		(void)hp_player(10);
		(void)set_afraid(0);
		break;
	case 25: /* Remove All Curse */
		if (remove_all_curse())
		{
#ifdef JP
			msg_print("誰かに見守られているような気がする。");
#else
			msg_print("You feel as if someone is watching over you.");
#endif
		}
		break;
	case 26: /* Banishment */
		if (banish_evil(100))
		{
#ifdef JP
msg_print("神聖な力が邪悪を打ち払った！");
#else
			msg_print("The holy power banishes evil!");
#endif

		}
		break;
	case 27: /* Word of Destruction */
		destroy_area(py, px, 13+randint0(5), TRUE);
		break;
	case 28: /* Eye for an Eye */
		set_tim_eyeeye(randint1(10)+10, FALSE);
		break;
	case 29:
		{
			int x, y, tx, ty;
			int nx, ny;
			int dir, i;
			int b = 10 + randint1(10);

			if (!get_aim_dir(&dir)) return FALSE;

			/* Use the given direction */
			tx = px + 99 * ddx[dir];
			ty = py + 99 * ddy[dir];

			/* Hack -- Use an actual "target" */
			if ((dir == 5) && target_okay())
			{
				tx = target_col;
				ty = target_row;
			}

			x = px;
			y = py;

			while(1)
			{
				/* Hack -- Stop at the target */
				if ((y == ty) && (x == tx)) break;

				ny = y;
				nx = x;
				mmove2(&ny, &nx, py, px, ty, tx);

				/* Stop at maximum range */
				if (MAX_SIGHT*2 < distance(py, px, ny, nx)) break;

				/* Stopped by walls/doors */
				if (!cave_floor_bold(ny, nx)) break;

				/* Stopped by monsters */
				if ((dir != 5) && cave[ny][nx].m_idx != 0) break;

				/* Save the new location */
				x = nx;
				y = ny;
			}
			tx = x;
			ty = y;

			for (i = 0; i < b; i++)
			{
				int count = 20, d = 0;

				while (count--)
				{
					int dx, dy;

					x = tx - 5 + randint0(11);
					y = ty - 5 + randint0(11);

					dx = (tx > x) ? (tx - x) : (x - tx);
					dy = (ty > y) ? (ty - y) : (y - ty);

					/* Approximate distance */
					d = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));
					/* Within the radius */
					if (d < 5) break;
				}

				if (count < 0) continue;

				/* Cannot penetrate perm walls */
				if (!in_bounds(y,x) ||
				    cave_stop_disintegration(y,x) ||
				    !in_disintegration_range(ty, tx, y, x))
					continue;

				project(0, 2, y, x, plev * 3+25, GF_DISINTEGRATE, PROJECT_JUMP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL, -1);
			}
		}
		break;
	case 30: /* Divine Intervention */
		project(0, 1, py, px, plev*11, GF_HOLY_FIRE, PROJECT_KILL, -1);
		dispel_monsters(plev * 4);
		slow_monsters();
		stun_monsters(plev * 4);
		confuse_monsters(plev * 4);
		turn_monsters(plev * 4);
		stasis_monsters(plev * 4);
		(void)hp_player(100);
		break;
	case 31:
	{
		int i;
		(void)crusade();
		for (i = 0; i < 12; i++)
		{
			int attempt = 10;
			int my, mx;

			while (attempt--)
			{
				scatter(&my, &mx, py, px, 4, 0);

				/* Require empty grids */
				if (cave_empty_bold2(my, mx)) break;
			}
			if (attempt < 0) continue;
			summon_specific(-1, my, mx, plev, SUMMON_KNIGHTS, (PM_ALLOW_GROUP | PM_FORCE_PET | PM_HASTE));
		}
		(void)set_hero(randint1(25) + 25, FALSE);
		(void)set_blessed(randint1(25) + 25, FALSE);
		(void)set_fast(randint1(20 + plev) + plev, FALSE);
		(void)set_protevil(randint1(25) + 25, FALSE);
		(void)set_afraid(0);
		break;
	}
	default:
#ifdef JP
msg_format("あなたは不明な破邪の呪文 %d を唱えた。", spell);
#else
		msg_format("You cast an unknown crusade spell: %d.", spell);
#endif

		msg_print(NULL);
	}

	return TRUE;
}



void stop_singing(void)
{
	if (p_ptr->pclass != CLASS_BARD) return;

	if (p_ptr->magic_num1[1])
	{
		p_ptr->magic_num1[1] = 0;
		return;
	}
	if (!p_ptr->magic_num1[0]) return;

	set_action(ACTION_NONE);

        switch(p_ptr->magic_num1[0])
        {
                case MUSIC_BLESS:
                        if (!p_ptr->blessed)
#ifdef JP
msg_print("高潔な気分が消え失せた。");
#else
				msg_print("The prayer has expired.");
#endif
                        break;
                case MUSIC_HERO:
                        if (!p_ptr->hero)
			{
#ifdef JP
msg_print("ヒーローの気分が消え失せた。");
#else
				msg_print("The heroism wears off.");
#endif
				/* Recalculate hitpoints */
				p_ptr->update |= (PU_HP);
			}
                        break;
                case MUSIC_MIND:
                        if (!p_ptr->tim_esp)
			{
#ifdef JP
msg_print("意識は元に戻った。");
#else
				msg_print("Your consciousness contracts again.");
#endif
				/* Update the monsters */
				p_ptr->update |= (PU_MONSTERS);
			}
                        break;
                case MUSIC_STEALTH:
                        if (!p_ptr->tim_stealth)
#ifdef JP
msg_print("姿がはっきりと見えるようになった。");
#else
				msg_print("You are no longer hided.");
#endif
                        break;
                case MUSIC_RESIST:
                        if (!p_ptr->oppose_acid)
#ifdef JP
msg_print("酸への耐性が薄れた気がする。");
#else
				msg_print("You feel less resistant to acid.");
#endif
                        if (!p_ptr->oppose_elec)
#ifdef JP
msg_print("電撃への耐性が薄れた気がする。");
#else
				msg_print("You feel less resistant to elec.");
#endif
                        if (!p_ptr->oppose_fire)
#ifdef JP
msg_print("火への耐性が薄れた気がする。");
#else
				msg_print("You feel less resistant to fire.");
#endif
                        if (!p_ptr->oppose_cold)
#ifdef JP
msg_print("冷気への耐性が薄れた気がする。");
#else
				msg_print("You feel less resistant to cold.");
#endif
                        if (!p_ptr->oppose_pois)
#ifdef JP
msg_print("毒への耐性が薄れた気がする。");
#else
				msg_print("You feel less resistant to pois.");
#endif
                        break;
                case MUSIC_SPEED:
                        if (!p_ptr->fast)
#ifdef JP
msg_print("動きの素早さがなくなったようだ。");
#else
				msg_print("You feel yourself slow down.");
#endif
                        break;
                case MUSIC_SHERO:
                        if (!p_ptr->hero)
			{
#ifdef JP
msg_print("ヒーローの気分が消え失せた。");
#else
				msg_print("The heroism wears off.");
#endif
				/* Recalculate hitpoints */
				p_ptr->update |= (PU_HP);
			}

                        if (!p_ptr->fast)
#ifdef JP
msg_print("動きの素早さがなくなったようだ。");
#else
				msg_print("You feel yourself slow down.");
#endif
                        break;
                case MUSIC_INVULN:
                        if (!p_ptr->invuln)
			{
#ifdef JP
msg_print("無敵ではなくなった。");
#else
				msg_print("The invulnerability wears off.");
#endif
				/* Redraw map */
				p_ptr->redraw |= (PR_MAP);

				/* Update monsters */
				p_ptr->update |= (PU_MONSTERS);

				/* Window stuff */
				p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
			}
                        break;
        }
	p_ptr->magic_num1[0] = MUSIC_NONE;
	p_ptr->magic_num2[0] = 0;

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS | PU_HP);

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);
}


static bool cast_music_spell(int spell)
{
	int	plev = p_ptr->lev;
	int dir;

        if(p_ptr->magic_num1[0])
        {
                stop_singing();
        }

        p_ptr->magic_num2[0] = spell;

	switch (spell)
	{
	case 0: /* Song of Holding 遅鈍の歌 */
#ifdef JP
		msg_print("ゆっくりとしたメロディを口ずさみ始めた．．．");
#else
		msg_print("You start humming a slow, steady melody...");
#endif
		p_ptr->magic_num1[0] = MUSIC_SLOW;
		break;
	case 1:  /* Song of Blessing 祝福の歌 */
#ifdef JP
		msg_print("厳かなメロディを奏で始めた．．．");
#else
		msg_print("The holy power of the Music of the Ainur enters you...");
#endif
		p_ptr->magic_num1[0] = MUSIC_BLESS;
		break;
		
	case 2:  /* Wrecking Note 崩壊の音色 */
		if (!get_aim_dir(&dir)) return FALSE;
		fire_bolt(GF_SOUND, dir,
			  damroll(4 + ((plev - 1) / 5), 4));
		break;
	case 3:  /* Stun Pattern 朦朧の旋律 */
#ifdef JP
		msg_print("眩惑させるメロディを奏で始めた．．．");
#else
		msg_print("You weave a pattern of sounds to bewilder and daze...");
#endif
		p_ptr->magic_num1[0] = MUSIC_STUN;
		break;
	case 4:  /* Flow of life 生命の流れ */
#ifdef JP
		msg_print("歌を通して体に活気が戻ってきた．．．");
#else
		msg_print("Life flows through you as you sing a song of healing...");
#endif
		p_ptr->magic_num1[0] = MUSIC_L_LIFE;
		break;
	case 5:  /* Song of the Sun 太陽の歌 */
#ifdef JP
		msg_print("光り輝く歌が辺りを照らした。");
#else
		msg_print("Your uplifting song brings brightness to dark places...");
#endif
		(void)lite_area(damroll(2, (plev / 2)), (plev / 10) + 1);
		break;
	case 6:  /* Song of fear 恐怖の歌 */
#ifdef JP
		msg_print("おどろおどろしいメロディを奏で始めた．．．");
#else
		msg_print("You start weaving a fearful pattern...");
#endif
		p_ptr->magic_num1[0] = MUSIC_FEAR;
		break;
	case 7:  /* Heroic Ballad 戦いの歌 */
#ifdef JP
		msg_print("激しい戦いの歌を歌った．．．");
#else
		msg_print("You start singing a song of intense fighting...");
#endif
		p_ptr->magic_num1[0] = MUSIC_HERO;
		break;
	case 8:  /* Clairaudience 霊的知覚 */
#ifdef JP
		msg_print("静かな音楽が感覚を研ぎ澄まさせた．．．");
#else
		msg_print("Your quiet music sharpens your sense of hearing...");
#endif
		p_ptr->magic_num1[0] = MUSIC_DETECT;
		break;
	case 9: /* 魂の歌 */
#ifdef JP
		msg_print("精神を捻じ曲げる歌を歌った．．．");
#else
		msg_print("You start singing a song of soul in pain...");
#endif
		p_ptr->magic_num1[0] = MUSIC_PSI;
		break;
	case 10:  /* Song of Lore 知識の歌 */
#ifdef JP
		msg_print("この世界の知識が流れ込んできた．．．");
#else
		msg_print("You recall the rich lore of the world...");
#endif
		p_ptr->magic_num1[0] = MUSIC_ID;
		break;
	case 11:  /* hidding song 隠遁の歌 */
#ifdef JP
		msg_print("あなたの姿が景色にとけこんでいった．．．");
#else
		msg_print("Your song carries you beyond the sight of mortal eyes...");
#endif
		p_ptr->magic_num1[0] = MUSIC_STEALTH;
		break;
	case 12:  /* Illusion Pattern 幻影の旋律 */
#ifdef JP
		msg_print("辺り一面に幻影が現れた．．．");
#else
		msg_print("You weave a pattern of sounds to beguile and confuse...");
#endif
		p_ptr->magic_num1[0] = MUSIC_CONF;
		break;
	case 13:  /* Doomcall (vibration song) 破滅の叫び */
#ifdef JP
		msg_print("轟音が響いた．．．");
#else
		msg_print("The fury of the Downfall of Numenor lashes out...");
#endif
		p_ptr->magic_num1[0] = MUSIC_SOUND;
		break;
	case 14:  /* Firiel's Song (song of the Undeads) フィリエルの歌 */
#ifdef JP
		msg_print("生命と復活のテーマを奏で始めた．．．");
#else
		msg_print("The themes of life and revival are woven into your song...");
#endif
		animate_dead(0, py, px);
		break;
	case 15:  /* Fellowship Chant (charming song) 旅の仲間 */
#ifdef JP
		msg_print("安らかなメロディを奏で始めた．．．");
#else
		msg_print("You weave a slow, soothing melody of imploration...");
#endif
		p_ptr->magic_num1[0] = MUSIC_CHARM;
		break;
	case 16:  /* (wall breaking song) 分解音波 */
#ifdef JP
		msg_print("粉砕するメロディを奏で始めた．．．");
#else
		msg_print("You weave a violent pattern of sounds to break wall.");
#endif
		p_ptr->magic_num1[0] = MUSIC_WALL;
		project(0, 0, py, px,
			0, GF_DISINTEGRATE, PROJECT_KILL | PROJECT_ITEM | PROJECT_HIDE, -1);
		break;
	case 17:  /* Finrod's Resistance (song of resistance) 元素耐性 */
#ifdef JP
		msg_print("元素の力に対する忍耐の歌を歌った。");
#else
		msg_print("You sing a song of perseverance against powers...");
#endif
		p_ptr->magic_num1[0] = MUSIC_RESIST;
		break;
	case 18:  /* Hobbit Melodies (song of time) ホビットのメロディ */
#ifdef JP
		msg_print("軽快な歌を口ずさみ始めた．．．");
#else
		msg_print("You start singing joyful pop song...");
#endif
		p_ptr->magic_num1[0] = MUSIC_SPEED;
		break;
	case 19:  /* World Contortion 歪んだ世界 */
#ifdef JP
		msg_print("歌が空間を歪めた．．．");
#else
		msg_print("Reality whirls wildly as you sing a dizzying melody...");
#endif
		project(0, plev/15 + 1, py, px, plev * 3 + 1, GF_AWAY_ALL , PROJECT_KILL, -1);
		break;
	case 20: /* 退散の歌 */
#ifdef JP
		msg_print("耐えられない不協和音が敵を責め立てた．．．");
#else
		msg_print("You cry out in an ear-wracking voice...");
#endif
		p_ptr->magic_num1[0] = MUSIC_DISPEL;
		break;
	case 21: /* The Voice of Saruman サルマンの甘言 */
#ifdef JP
		msg_print("優しく、魅力的な歌を口ずさみ始めた．．．");
#else
		msg_print("You start humming a gentle and attractive song...");
#endif
		p_ptr->magic_num1[0] = MUSIC_SARUMAN;
		break;
	case 22:  /* Song of Tempest (song of death) 嵐の音色 */
		if (!get_aim_dir(&dir)) return FALSE;
		fire_beam(GF_SOUND, dir,
			  damroll(15 + ((plev - 1) / 2), 10));
		break;
	case 23:  /* (song of disruption) もう一つの世界 */
#ifdef JP
		msg_print("周囲が変化し始めた．．．");
#else
		msg_print("You sing of the primeval shaping of Middle-earth...");
#endif
		alter_reality();
		break;
	case 24:  /* Wrecking Pattern (destruction shriek) 破壊の旋律 */
#ifdef JP
		msg_print("破壊的な歌が響きわたった．．．");
#else
		msg_print("You weave a pattern of sounds to contort and shatter...");
#endif
		p_ptr->magic_num1[0] = MUSIC_QUAKE;
		break;
	case 25: /* 停滞の歌  */
#ifdef JP
		msg_print("ゆっくりとしたメロディを奏で始めた．．．");
#else
		msg_print("You weave a very slow pattern which is almost likely to stop...");
#endif
		p_ptr->magic_num1[0] = MUSIC_STASIS;
		break;
	case 26: /* 守りの歌 */
#ifdef JP
		msg_print("歌が神聖な場を作り出した．．．");
#else
		msg_print("The holy power of the Music is creating sacred field...");
#endif
		warding_glyph();
		break;
	case 27: /* 英雄の詩 */
#ifdef JP
		msg_print("英雄の歌を口ずさんだ．．．");
#else
		msg_print("You chant a powerful, heroic call to arms...");
#endif
		p_ptr->magic_num1[0] = MUSIC_SHERO;
		(void)hp_player(10);
		(void)set_afraid(0);
		break;
	case 28: /* ヤヴァンナの助け */
#ifdef JP
		msg_print("歌を通して体に活気が戻ってきた．．．");
#else
		msg_print("Life flows through you as you sing the song...");
#endif
		p_ptr->magic_num1[0] = MUSIC_H_LIFE;
		break;
	case 29: /* 再生の歌 */
#ifdef JP
		msg_print("暗黒の中に光と美をふりまいた。体が元の活力を取り戻した。");
#else
		msg_print("You strewed light and beauty in the dark as you sing. You feel refreshed.");
#endif
		(void)do_res_stat(A_STR);
		(void)do_res_stat(A_INT);
		(void)do_res_stat(A_WIS);
		(void)do_res_stat(A_DEX);
		(void)do_res_stat(A_CON);
		(void)do_res_stat(A_CHR);
		(void)restore_level();
		break;
	case 30:  /* shriek of death サウロンの魔術 */
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball(GF_SOUND, dir, damroll(50 + plev, 10), 0);
		break;
	case 31:  /* song of liberty フィンゴルフィンの挑戦 */
#ifdef JP
		msg_print("フィンゴルフィンの冥王への挑戦を歌った．．．");
#else
		msg_print("You recall the valor of Fingolfin's challenge to the Dark Lord...");
#endif
		p_ptr->magic_num1[0] = MUSIC_INVULN;
		
		/* Redraw map */
		p_ptr->redraw |= (PR_MAP);
		
		/* Update monsters */
		p_ptr->update |= (PU_MONSTERS);
		
		/* Window stuff */
		p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
		break;
	default:
#ifdef JP
		msg_format("未知の歌(%d)を歌った。", spell);
#else
		msg_format("You sing an unknown song: %d.", spell);
#endif
		msg_print(NULL);
	}

	if (p_ptr->magic_num1[0]) set_action(ACTION_SING);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS | PU_HP);

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);
	return TRUE;
}


/*
 * Cast a spell
 */
void do_cmd_cast(void)
{
	int	item, sval, spell, realm;
	int	chance;
	int	increment = 0;
	int	use_realm;
	int	shouhimana;
	bool cast;

	cptr prayer;

	object_type	*o_ptr;

	magic_type	*s_ptr;

	cptr q, s;

	/* Require spell ability */
	if (!p_ptr->realm1 && (p_ptr->pclass != CLASS_SORCERER) && (p_ptr->pclass != CLASS_RED_MAGE))
	{
#ifdef JP
msg_print("呪文を唱えられない！");
#else
		msg_print("You cannot cast spells!");
#endif

		return;
	}

	/* Require lite */
	if (p_ptr->blind || no_lite())
	{
#ifdef JP
msg_print("目が見えない！");
#else
		msg_print("You cannot see!");
#endif
		if (p_ptr->pclass == CLASS_FORCETRAINER)
		    do_cmd_mind();
		else
			flush();
		return;
	}

	/* Not when confused */
	if (p_ptr->confused)
	{
#ifdef JP
msg_print("混乱していて唱えられない！");
#else
		msg_print("You are too confused!");
#endif
		flush();
		return;
	}

	prayer = spell_categoly_name(mp_ptr->spell_book);

	/* Restrict choices to spell books */
	item_tester_tval = mp_ptr->spell_book;

	/* Get an item */
#ifdef JP
q = "どの呪文書を使いますか? ";
#else
	q = "Use which book? ";
#endif

#ifdef JP
s = "呪文書がない！";
#else
	s = "You have no spell books!";
#endif

        select_spellbook=TRUE;
	if (p_ptr->pclass == CLASS_FORCETRAINER)
		select_the_force = TRUE;
	if (!get_item(&item, q, s, (USE_INVEN | USE_FLOOR))){
            select_spellbook = FALSE;
	    select_the_force = FALSE;
            return;
        }
        select_spellbook = FALSE;
	select_the_force = FALSE;

	if (item == 1111) { /* the_force */
	    do_cmd_mind();
	    return;
	} else
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

	/* Access the item's sval */
	sval = o_ptr->sval;

	if ((p_ptr->pclass != CLASS_SORCERER) && (p_ptr->pclass != CLASS_RED_MAGE) && (o_ptr->tval == REALM2_BOOK)) increment = 32;


	/* Track the object kind */
	object_kind_track(o_ptr->k_idx);

	/* Hack -- Handle stuff */
	handle_stuff();

	if ((p_ptr->pclass == CLASS_SORCERER) || (p_ptr->pclass == CLASS_RED_MAGE))
		realm = o_ptr->tval - TV_LIFE_BOOK + 1;
	else if (increment) realm = p_ptr->realm2;
	else realm = p_ptr->realm1;

	/* Ask for a spell */
#ifdef JP
        if (!get_spell(&spell,  
		                ((mp_ptr->spell_book == TV_LIFE_BOOK) ? "詠唱する" : (mp_ptr->spell_book == TV_MUSIC_BOOK) ? "歌う" : "唱える"), 
		       sval, TRUE, realm))
        {
                if (spell == -2) msg_format("その本には知っている%sがない。", prayer);
                return;
        }
#else
	if (!get_spell(&spell, ((mp_ptr->spell_book == TV_LIFE_BOOK) ? "recite" : "cast"),
		sval, TRUE, realm))
	{
		if (spell == -2)
			msg_format("You don't know any %ss in that book.", prayer);
		return;
	}
#endif


	use_realm = tval2realm(o_ptr->tval);

	if (!is_magic(use_realm))
	{
		s_ptr = &technic_info[use_realm - MIN_TECHNIC][spell];
	}
	else
	{
		s_ptr = &mp_ptr->info[realm - 1][spell];
	}

	/* Extract mana consumption rate */
	shouhimana = s_ptr->smana*(3800 - experience_of_spell(spell, realm)) + 2399;
	if(p_ptr->dec_mana)
		shouhimana *= 3;
	else shouhimana *= 4;
	shouhimana /= 9600;
	if(shouhimana < 1) shouhimana = 1;

	/* Verify "dangerous" spells */
	if (shouhimana > p_ptr->csp)
	{
		if (flush_failure) flush();

		/* Warning */
#ifdef JP
msg_format("その%sを%sのに十分なマジックポイントがない。",prayer,
((mp_ptr->spell_book == TV_LIFE_BOOK) ? "詠唱する" : (mp_ptr->spell_book == TV_LIFE_BOOK) ? "歌う" : "唱える"));
#else
		msg_format("You do not have enough mana to %s this %s.",
			((mp_ptr->spell_book == TV_LIFE_BOOK) ? "recite" : "cast"),
			prayer);
#endif


		if (!over_exert) return;

		/* Verify */
#ifdef JP
		if (!get_check_strict("それでも挑戦しますか? ", CHECK_OKAY_CANCEL)) return;
#else
		if (!get_check_strict("Attempt it anyway? ", CHECK_OKAY_CANCEL)) return;
#endif

	}


	/* Spell failure chance */
	chance = spell_chance(spell, use_realm);

	/* Failed spell */
	if (randint0(100) < chance)
	{
		if (flush_failure) flush();

#ifdef JP
msg_format("%sをうまく唱えられなかった！", prayer);
#else
		msg_format("You failed to get the %s off!", prayer);
#endif

		sound(SOUND_FAIL);

		if (realm == REALM_LIFE)
		{
			if (randint1(100) < chance)
				chg_virtue(V_VITALITY, -1);
		}
		else if (realm == REALM_DEATH)
		{
			if (randint1(100) < chance)
				chg_virtue(V_UNLIFE, -1);
		}
		else if (realm == REALM_NATURE)
		{
			if (randint1(100) < chance)
				chg_virtue(V_NATURE, -1);
		}
		else if (realm == REALM_DAEMON)
		{
			if (randint1(100) < chance)
				chg_virtue(V_JUSTICE, 1);
		}
		if (realm == REALM_CRUSADE)
		{
			if (randint1(100) < chance)
				chg_virtue(V_JUSTICE, -1);
		}
		else if (randint1(100) < chance)
		{
			chg_virtue(V_KNOWLEDGE, -1);
		}

		if (realm == REALM_TRUMP)
		{
			cast_trump_spell(spell, FALSE);
		}
		else if ((o_ptr->tval == TV_CHAOS_BOOK) && (randint1(100) < spell))
		{
#ifdef JP
msg_print("カオス的な効果を発生した！");
#else
			msg_print("You produce a chaotic effect!");
#endif

			wild_magic(spell);
		}
		else if ((o_ptr->tval == TV_DEATH_BOOK) && (randint1(100) < spell))
		{
			if ((sval == 3) && one_in_(2))
			{
				sanity_blast(0, TRUE);
			}
			else
			{
#ifdef JP
				msg_print("痛い！");
#else
				msg_print("It hurts!");
#endif

#ifdef JP
				take_hit(DAMAGE_LOSELIFE, damroll(o_ptr->sval + 1, 6), "暗黒魔法の逆流", -1);
#else
				take_hit(DAMAGE_LOSELIFE, damroll(o_ptr->sval + 1, 6), "a miscast Death spell", -1);
#endif

				if ((spell > 15) && one_in_(6) && !p_ptr->hold_life)
					lose_exp(spell * 250);
			}
		}
		else if ((o_ptr->tval == TV_MUSIC_BOOK) && (randint1(200) < spell))
		{
#ifdef JP
msg_print("いやな音が響いた");
#else
msg_print("An infernal sound echoed.");
#endif

			aggravate_monsters(0);
		}
		if (randint1(100) >= chance)
			chg_virtue(V_CHANCE,-1);
	}

	/* Process spell */
	else
	{
		/* Spells.  */
		switch (realm)
		{
		case REALM_LIFE: /* * LIFE * */
			cast = cast_life_spell(spell);
			break;
		case REALM_SORCERY: /* * SORCERY * */
			cast = cast_sorcery_spell(spell);
			break;
		case REALM_NATURE: /* * NATURE * */
			cast = cast_nature_spell(spell);
			break;
		case REALM_CHAOS: /* * CHAOS * */
			cast = cast_chaos_spell(spell);
			break;
		case REALM_DEATH: /* * DEATH * */
			cast = cast_death_spell(spell);
			break;
		case REALM_TRUMP: /* TRUMP */
			cast = cast_trump_spell(spell, TRUE);
			break;
		case REALM_ARCANE: /* ARCANE */
			cast = cast_arcane_spell(spell);
			break;
		case REALM_ENCHANT: /* ENCHANT */
			cast = cast_enchant_spell(spell);
			break;
		case REALM_DAEMON: /* DAEMON */
			cast = cast_daemon_spell(spell);
			break;
		case REALM_CRUSADE: /* CRUSADE */
			cast = cast_crusade_spell(spell);
			break;
		case REALM_MUSIC: /* MUSIC */
			cast = cast_music_spell(spell);
			break;
		default:
			cast = FALSE;
			msg_format("You cast a spell from an unknown realm: realm %d, spell %d.", realm, spell);
			msg_print(NULL);
		}

		/* Canceled spells cost neither a turn nor mana */
		if (!cast) return;

		if (randint1(100) < chance)
			chg_virtue(V_CHANCE,1);

		/* A spell was cast */
		if (!(increment ?
		    (p_ptr->spell_worked2 & (1L << spell)) :
		    (p_ptr->spell_worked1 & (1L << spell)))
		    && (p_ptr->pclass != CLASS_SORCERER)
		    && (p_ptr->pclass != CLASS_RED_MAGE))
		{
			int e = s_ptr->sexp;

			/* The spell worked */
			if (realm == p_ptr->realm1)
			{
				p_ptr->spell_worked1 |= (1L << spell);
			}
			else
			{
				p_ptr->spell_worked2 |= (1L << spell);
			}

			/* Gain experience */
			gain_exp(e * s_ptr->slevel);

                        /* Redraw object recall */
                        p_ptr->window |= (PW_OBJECT);

			if (realm == REALM_LIFE)
			{
				chg_virtue(V_TEMPERANCE, 1);
				chg_virtue(V_COMPASSION, 1);
				chg_virtue(V_VITALITY, 1);
				chg_virtue(V_DILIGENCE, 1);
			}
			else if (realm == REALM_DEATH)
			{
				chg_virtue(V_UNLIFE, 1);
				chg_virtue(V_JUSTICE, -1);
				chg_virtue(V_FAITH, -1);
				chg_virtue(V_VITALITY, -1);
			}
			else if (realm == REALM_DAEMON)
			{
				chg_virtue(V_JUSTICE, -1);
				chg_virtue(V_FAITH, -1);
				chg_virtue(V_HONOUR, -1);
				chg_virtue(V_TEMPERANCE, -1);
			}
			else if (realm == REALM_CRUSADE)
			{
				chg_virtue(V_FAITH, 1);
				chg_virtue(V_JUSTICE, 1);
				chg_virtue(V_SACRIFICE, 1);
				chg_virtue(V_HONOUR, 1);
			}
			else if (realm == REALM_NATURE)
			{
				chg_virtue(V_NATURE, 1);
				chg_virtue(V_HARMONY, 1);
			}
			else
				chg_virtue(V_KNOWLEDGE, 1);
		}
		if (realm == REALM_LIFE)
		{
			if (randint1(100 + p_ptr->lev) < shouhimana) chg_virtue(V_TEMPERANCE, 1);
			if (randint1(100 + p_ptr->lev) < shouhimana) chg_virtue(V_COMPASSION, 1);
			if (randint1(100 + p_ptr->lev) < shouhimana) chg_virtue(V_VITALITY, 1);
			if (randint1(100 + p_ptr->lev) < shouhimana) chg_virtue(V_DILIGENCE, 1);
		}
		else if (realm == REALM_DEATH)
		{
			if (randint1(100 + p_ptr->lev) < shouhimana) chg_virtue(V_UNLIFE, 1);
			if (randint1(100 + p_ptr->lev) < shouhimana) chg_virtue(V_JUSTICE, -1);
			if (randint1(100 + p_ptr->lev) < shouhimana) chg_virtue(V_FAITH, -1);
			if (randint1(100 + p_ptr->lev) < shouhimana) chg_virtue(V_VITALITY, -1);
		}
		else if (realm == REALM_DAEMON)
		{
			if (randint1(100 + p_ptr->lev) < shouhimana) chg_virtue(V_JUSTICE, -1);
			if (randint1(100 + p_ptr->lev) < shouhimana) chg_virtue(V_FAITH, -1);
			if (randint1(100 + p_ptr->lev) < shouhimana) chg_virtue(V_HONOUR, -1);
			if (randint1(100 + p_ptr->lev) < shouhimana) chg_virtue(V_TEMPERANCE, -1);
		}
		else if (realm == REALM_CRUSADE)
		{
			if (randint1(100 + p_ptr->lev) < shouhimana) chg_virtue(V_FAITH, 1);
			if (randint1(100 + p_ptr->lev) < shouhimana) chg_virtue(V_JUSTICE, 1);
			if (randint1(100 + p_ptr->lev) < shouhimana) chg_virtue(V_SACRIFICE, 1);
			if (randint1(100 + p_ptr->lev) < shouhimana) chg_virtue(V_HONOUR, 1);
		}
		else if (realm == REALM_NATURE)
		{
			if (randint1(100 + p_ptr->lev) < shouhimana) chg_virtue(V_NATURE, 1);
			if (randint1(100 + p_ptr->lev) < shouhimana) chg_virtue(V_HARMONY, 1);
		}
		if (mp_ptr->spell_xtra & MAGIC_GAIN_EXP)
		{
			s16b cur_exp = p_ptr->spell_exp[(increment ? 32 : 0)+spell];
			s16b exp_gain = 0;

			if (cur_exp < 900)
				exp_gain+=60;
			else if(cur_exp < 1200)
			{
				if ((dun_level > 4) && ((dun_level + 10) > p_ptr->lev))
					exp_gain = 8;
			}
			else if(cur_exp < 1400)
			{
				if (((dun_level + 5) > p_ptr->lev) && ((dun_level + 5) > s_ptr->slevel))
					exp_gain = 2;
			}
			else if((cur_exp < 1600) && !increment)
			{
				if (((dun_level + 5) > p_ptr->lev) && (dun_level > s_ptr->slevel))
					exp_gain = 1;
			}
			p_ptr->spell_exp[(increment ? 32 : 0)+spell] += exp_gain;
		}
	}

	/* Take a turn */
	energy_use = 100;

	/* Sufficient mana */
	if (shouhimana <= p_ptr->csp)
	{
		/* Use some mana */
		p_ptr->csp -= shouhimana;
	}

	/* Over-exert the player */
	else
	{
		int oops = shouhimana;

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

		if (realm == REALM_LIFE)
			chg_virtue(V_VITALITY, -10);
		else if (realm == REALM_DEATH)
			chg_virtue(V_UNLIFE, -10);
		else if (realm == REALM_DAEMON)
			chg_virtue(V_JUSTICE, 10);
		else if (realm == REALM_NATURE)
			chg_virtue(V_NATURE, -10);
		else if (realm == REALM_CRUSADE)
			chg_virtue(V_JUSTICE, -10);
		else
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

	/* Redraw mana */
	p_ptr->redraw |= (PR_MANA);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);
	p_ptr->window |= (PW_SPELL);
}


/*
 * Pray a prayer -- Unused in Hengband
 */
void do_cmd_pray(void)
{
	msg_print("Praying is not used in Hengband. Use magic spell casting instead.");
}

static bool ang_sort_comp_pet_dismiss(vptr u, vptr v, int a, int b)
{
	u16b *who = (u16b*)(u);

	int w1 = who[a];
	int w2 = who[b];

	monster_type *m_ptr1 = &m_list[w1];
	monster_type *m_ptr2 = &m_list[w2];
	monster_race *r_ptr1 = &r_info[m_ptr1->r_idx];
	monster_race *r_ptr2 = &r_info[m_ptr2->r_idx];

	if (w1 == p_ptr->riding) return TRUE;
	if (w2 == p_ptr->riding) return FALSE;

	if (m_ptr1->nickname && !m_ptr2->nickname) return TRUE;
	if (m_ptr2->nickname && !m_ptr1->nickname) return FALSE;

	if ((r_ptr1->flags1 & RF1_UNIQUE) && !(r_ptr2->flags1 & RF1_UNIQUE)) return TRUE;
	if ((r_ptr2->flags1 & RF1_UNIQUE) && !(r_ptr1->flags1 & RF1_UNIQUE)) return FALSE;

	if (r_ptr1->level > r_ptr2->level) return TRUE;
	if (r_ptr2->level > r_ptr1->level) return FALSE;

	if (m_ptr1->hp > m_ptr2->hp) return TRUE;
	if (m_ptr2->hp > m_ptr1->hp) return FALSE;
	
	return w1 <= w2;
}

int calculate_upkeep(void)
{
	s32b old_friend_align = friend_align;
	int m_idx;
	bool have_a_unique = FALSE;

	total_friends = 0;
	total_friend_levels = 0;
	friend_align = 0;

	for (m_idx = m_max - 1; m_idx >=1; m_idx--)
	{
		monster_type *m_ptr;
		monster_race *r_ptr;
		
		m_ptr = &m_list[m_idx];
		if (!m_ptr->r_idx) continue;
		r_ptr = &r_info[m_ptr->r_idx];

		if (is_pet(m_ptr))
		{
			total_friends++;
			if (r_ptr->flags1 & RF1_UNIQUE)
			{
				if (p_ptr->pclass == CLASS_CAVALRY)
				{
					if (p_ptr->riding == m_idx)
						total_friend_levels += (r_ptr->level+5)*2;
					else if (!have_a_unique && (r_info[m_ptr->r_idx].flags7 & RF7_RIDING))
						total_friend_levels += (r_ptr->level+5)*7/2;
					else
						total_friend_levels += (r_ptr->level+5)*10;
					have_a_unique = TRUE;
				}
				else
					total_friend_levels += (r_ptr->level+5)*10;
			}
			else
				total_friend_levels += r_ptr->level;
			
			/* Determine pet alignment */
			if (r_ptr->flags3 & RF3_GOOD)
			{
				friend_align += r_ptr->level;
			}
			else if (r_ptr->flags3 & RF3_EVIL)
			{
				friend_align -= r_ptr->level;
			}
		}
	}
	if (old_friend_align != friend_align) p_ptr->update |= (PU_BONUS);
	if (total_friends)
	{
		int upkeep_factor;
		upkeep_factor = (total_friend_levels - (p_ptr->lev * 80 / (cp_ptr->pet_upkeep_div)));
		if (upkeep_factor < 0) upkeep_factor = 0;
		if (upkeep_factor > 1000) upkeep_factor = 1000;
		return upkeep_factor;
	}
	else
		return 0;
}

void do_cmd_pet_dismiss(void)
{
	monster_type	*m_ptr;
	bool		all_pets = FALSE;
	int pet_ctr, i;
	int Dismissed = 0;

	u16b *who;
	u16b dummy_why;
	int max_pet = 0;
	int cu, cv;

	cu = Term->scr->cu;
	cv = Term->scr->cv;
	Term->scr->cu = 0;
	Term->scr->cv = 1;

	/* Allocate the "who" array */
	C_MAKE(who, max_m_idx, u16b);

	/* Process the monsters (backwards) */
	for (pet_ctr = m_max - 1; pet_ctr >= 1; pet_ctr--)
	{
		if (is_pet(&m_list[pet_ctr]))
			who[max_pet++] = pet_ctr;
	}

	/* Select the sort method */
	ang_sort_comp = ang_sort_comp_pet_dismiss;
	ang_sort_swap = ang_sort_swap_hook;

	ang_sort(who, &dummy_why, max_pet);

	/* Process the monsters (backwards) */
	for (i = 0; i < max_pet; i++)
	{
		bool delete_this;
		char friend_name[80];
		char buf[80];
		bool kakunin;

		/* Access the monster */
		pet_ctr = who[i];
		m_ptr = &m_list[pet_ctr];

		delete_this = FALSE;
		kakunin = ((pet_ctr == p_ptr->riding) || (m_ptr->nickname));
		monster_desc(friend_name, m_ptr, 0x80);
		
		if (!all_pets)
		{
			/* Hack -- health bar for this monster */
			health_track(pet_ctr);
			
			/* Hack -- handle stuff */
			handle_stuff();
			
#ifdef JP
			sprintf(buf, "%sを放しますか？ [Yes/No/Unnamed (%d匹)]", friend_name, max_pet-i);
#else
			sprintf(buf, "Dismiss %s? [Yes/No/Unnamed (%d remain)]", friend_name, max_pet-i);
#endif
			prt(buf, 0, 0);
			
			if (m_ptr->ml)
				move_cursor_relative(m_ptr->fy, m_ptr->fx);
			
			while (TRUE)
			{
				char ch = inkey();

				if (ch == 'Y' || ch == 'y')
				{
					delete_this = TRUE;
					
					if (kakunin)
					{
#ifdef JP
						sprintf(buf, "本当によろしいですか？ (%s) ", friend_name);
#else
						sprintf(buf, "Are you sure? (%s) ", friend_name);
#endif
						if (!get_check(buf))
							delete_this = FALSE;
					}
					break;
				}
				
				if (ch == 'U' || ch == 'u')
				{
					all_pets = TRUE;
					break;
				}
				
				if (ch == ESCAPE || ch == 'N' || ch == 'n')
					break;
				
				bell();
			}
		}
		
		if ((all_pets && !kakunin) || (!all_pets && delete_this))
		{
			if (record_named_pet && m_ptr->nickname)
			{
				char m_name[80];
				
				monster_desc(m_name, m_ptr, 0x08);
				do_cmd_write_nikki(NIKKI_NAMED_PET, 2, m_name);
			}
			
			if (pet_ctr == p_ptr->riding)
			{
#ifdef JP
				msg_format("%sから降りた。", friend_name);
#else
				msg_format("You have got off %s. ", friend_name);
#endif
				
				p_ptr->riding = 0;
				
				/* Update the monsters */
				p_ptr->update |= (PU_BONUS | PU_MONSTERS);
				p_ptr->redraw |= (PR_EXTRA);
			}

			/* HACK : Add the line to message buffer */
#ifdef JP
			sprintf(buf, "%s を離した。", friend_name);
#else
			sprintf(buf, "Dismissed %s.", friend_name);
#endif
			message_add(buf);
			p_ptr->window |= (PW_MESSAGE);
			window_stuff();

			delete_monster_idx(pet_ctr);
			Dismissed++;
		}
	}
	
	Term->scr->cu = cu;
	Term->scr->cv = cv;
	Term_fresh();

	C_KILL(who, max_m_idx, u16b);

#ifdef JP
	msg_format("%d 匹のペットを放しました。", Dismissed);
#else
	msg_format("You have dismissed %d pet%s.", Dismissed,
		   (Dismissed == 1 ? "" : "s"));
#endif
	if (Dismissed == 0 && all_pets)
#ifdef JP
		msg_print("'U'nnamed は、乗馬以外の名前のないペットだけを全て解放します。");
#else
		msg_print("'U'nnamed means all your pets except named pets and your mount.");
#endif

	p_ptr->update |= (PU_MON_LITE);
}

bool rakuba(int dam, bool force)
{
	int i, y, x, oy, ox;
	int sn = 0, sy = 0, sx = 0;
	char m_name[80];
	monster_type *m_ptr = &m_list[p_ptr->riding];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	if (!p_ptr->riding) return FALSE;
	if (p_ptr->wild_mode) return FALSE;

	if (dam >= 0 || force)
	{
		if (!force)
		{
			int level = r_ptr->level;
			if (p_ptr->riding_ryoute) level += 20;
			if ((dam/2 + r_ptr->level) > (p_ptr->skill_exp[GINOU_RIDING]/30+10))
			{
				if((p_ptr->skill_exp[GINOU_RIDING] < s_info[p_ptr->pclass].s_max[GINOU_RIDING]) && s_info[p_ptr->pclass].s_max[GINOU_RIDING] > 1000)
				{
					if (r_ptr->level*100 > (p_ptr->skill_exp[GINOU_RIDING] + 1500))
						p_ptr->skill_exp[GINOU_RIDING] += (1+(r_ptr->level - p_ptr->skill_exp[GINOU_RIDING]/100 - 15));
					else p_ptr->skill_exp[GINOU_RIDING]++;
				}
			}
			if (randint0(dam/2 + level*2) < (p_ptr->skill_exp[GINOU_RIDING]/30+10))
			{
				if ((((p_ptr->pclass == CLASS_BEASTMASTER) || (p_ptr->pclass == CLASS_CAVALRY)) && !p_ptr->riding_ryoute) || !one_in_(p_ptr->lev*(p_ptr->riding_ryoute ? 2 : 3)+30))
				{
					return FALSE;
				}
			}
		}
		/* Check around the player */
		for (i = 0; i < 8; i++)
		{
			cave_type *c_ptr;

			/* Access the location */
			y = py + ddy_ddd[i];
			x = px + ddx_ddd[i];

			c_ptr = &cave[y][x];

			/* Skip non-empty grids */
			if (cave_perma_grid(c_ptr)) continue;
			if (!cave_empty_grid2(c_ptr)) continue;

			if (c_ptr->m_idx) continue;

			/* Count "safe" grids */
			sn++;

			/* Randomize choice */
			if (randint0(sn) > 0) continue;

			/* Save the safe location */
			sy = y; sx = x;
		}
		if (!sn)
		{
			monster_desc(m_name, m_ptr, 0);
#ifdef JP
msg_format("%sから振り落とされそうになって、壁にぶつかった。",m_name);
			take_hit(DAMAGE_NOESCAPE, r_ptr->level+3, "壁への衝突", -1);
#else
			msg_format("You have nearly fallen from %s, but bumped into wall.",m_name);
			take_hit(DAMAGE_NOESCAPE, r_ptr->level+3, "bumping into wall", -1);
#endif
			return FALSE;
		}

		oy = py;
		ox = px;

		py = sy;
		px = sx;

		/* Redraw the old spot */
		lite_spot(oy, ox);

		/* Redraw the new spot */
		lite_spot(py, px);

		/* Check for new panel */
		verify_panel();
	}

	p_ptr->riding = 0;
	p_ptr->pet_extra_flags &= ~(PF_RYOUTE);
	p_ptr->riding_ryoute = p_ptr->old_riding_ryoute = FALSE;

	calc_bonuses();

	p_ptr->update |= (PU_BONUS);

	/* Update stuff */
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);

	/* Update the monsters */
	p_ptr->update |= (PU_DISTANCE);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

	p_ptr->redraw |= (PR_EXTRA);

	if (p_ptr->ffall && !force)
	{
		monster_desc(m_name, m_ptr, 0);
#ifdef JP
msg_format("%sから落ちたが、空中でうまく体勢を立て直して着地した。",m_name);
#else
		msg_format("You are thrown from %s, but make a good landing.",m_name);
#endif
		return FALSE;
	}
#ifdef JP
	take_hit(DAMAGE_NOESCAPE, r_ptr->level+3, "落馬", -1);
#else
	take_hit(DAMAGE_NOESCAPE, r_ptr->level+3, "Falling from riding", -1);
#endif
	p_ptr->redraw |= (PR_UHEALTH);

	return TRUE;
}

bool do_riding(bool force)
{
	int oy, ox, x, y, dir = 0;
	cave_type *c_ptr;
	monster_type *m_ptr;

	if (!get_rep_dir2(&dir)) return FALSE;
	y = py + ddy[dir];
	x = px + ddx[dir];
	c_ptr = &cave[y][x];

	if (p_ptr->riding)
	{
		/* Skip non-empty grids */
		if (!cave_empty_bold2(y, x) || c_ptr->m_idx)
		{
#ifdef JP
msg_print("そちらには降りられません。");
#else
			msg_print("You cannot go to that direction.");
#endif
			return FALSE;
		}
		p_ptr->riding = 0;
		p_ptr->pet_extra_flags &= ~(PF_RYOUTE);
		p_ptr->riding_ryoute = p_ptr->old_riding_ryoute = FALSE;
	}
	else
	{
		if (p_ptr->confused)
		{
#ifdef JP
msg_print("混乱していて乗れない！");
#else
			msg_print("You are too confused!");
#endif
			return FALSE;
		}
		if (!(c_ptr->m_idx))
		{
#ifdef JP
msg_print("その場所にはモンスターはいません。");
#else
			msg_print("Here is no pet.");
#endif

			return FALSE;
		}

		m_ptr = &m_list[c_ptr->m_idx];

		if (!is_pet(m_ptr) && !force)
		{
#ifdef JP
msg_print("そのモンスターはペットではありません。");
#else
			msg_print("That monster is no a pet.");
#endif

			return FALSE;
		}
		if (!(r_info[m_ptr->r_idx].flags7 & RF7_RIDING))
		{
#ifdef JP
msg_print("そのモンスターには乗れなさそうだ。");
#else
			msg_print("This monster doesn't seem suitable for riding.");
#endif

			return FALSE;
		}
		if (!(p_ptr->pass_wall) && (c_ptr->feat >= FEAT_RUBBLE) && (c_ptr->feat <= FEAT_PERM_SOLID))
		{
#ifdef JP
msg_print("そのモンスターは壁の中にいる。");
#else
			msg_print("This monster is in the wall.");
#endif

			return FALSE;
		}
		if ((cave[py][px].feat >= FEAT_PATTERN_START) && (cave[py][px].feat <= FEAT_PATTERN_XTRA2) && ((cave[y][x].feat < FEAT_PATTERN_START) || (cave[y][x].feat > FEAT_PATTERN_XTRA2)))
		{
#ifdef JP
msg_print("パターンの上からは乗れません。");
#else
			msg_print("You cannot ride from on Pattern.");
#endif

			return FALSE;
		}
		if (!m_ptr->ml)
		{
#ifdef JP
msg_print("その場所にはモンスターはいません。");
#else
			msg_print("Here is no monster.");
#endif

			return FALSE;
		}
		if (r_info[m_ptr->r_idx].level > randint1((p_ptr->skill_exp[GINOU_RIDING]/50 + p_ptr->lev/2 +20)))
		{
#ifdef JP
msg_print("うまく乗れなかった。");
#else
			msg_print("You failed to ride.");
#endif

			energy_use = 100;

			return FALSE;
		}
		if (m_ptr->csleep)
		{
			char m_name[80];
			monster_desc(m_name, m_ptr, 0);
			m_ptr->csleep = 0;
#ifdef JP
msg_format("%sを起こした。", m_name);
#else
			msg_format("You have waked %s up.", m_name);
#endif
		}

		p_ptr->riding = c_ptr->m_idx;
	}

	/* Save the old location */
	oy = py;
	ox = px;

	/* Move the player to the safe location */
	py = y;
	px = x;

	/* Redraw the old spot */
	lite_spot(oy, ox);

	/* Redraw the new spot */
	lite_spot(py, px);

	/* Check for new panel */
	verify_panel();

	energy_use = 100;

	/* Mega-Hack -- Forget the view and lite */
	p_ptr->update |= (PU_UN_VIEW | PU_UN_LITE);

	/* Update stuff */
	p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);

	/* Update the monsters */
	p_ptr->update |= (PU_DISTANCE);

	/* Update the monsters */
	p_ptr->update |= (PU_BONUS);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP | PR_EXTRA);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

	p_ptr->redraw |= (PR_UHEALTH);

	handle_stuff();
	return TRUE;
}

static void do_name_pet(void)
{
	monster_type *m_ptr;
	char out_val[20];
	char m_name[80];
	bool old_name = FALSE;
	bool old_target_pet = target_pet;

	target_pet = TRUE;
	if (!target_set(TARGET_KILL))
	{
		target_pet = old_target_pet;
		return;
	}
	target_pet = old_target_pet;

	if (cave[target_row][target_col].m_idx)
	{
		m_ptr = &m_list[cave[target_row][target_col].m_idx];

		if (!is_pet(m_ptr))
		{
			/* Message */
#ifdef JP
			msg_print("そのモンスターはペットではない。");
#else
			msg_format("This monster is not a pet.");
#endif
			return;
		}
		if (r_info[m_ptr->r_idx].flags1 & RF1_UNIQUE)
		{
#ifdef JP
			msg_print("そのモンスターの名前は変えられない！");
#else
			msg_format("You cannot change name of this monster!");
#endif
			return;
		}
		monster_desc(m_name, m_ptr, 0);

		/* Message */
#ifdef JP
		msg_format("%sに名前をつける。", m_name);
#else
		msg_format("Name %s.", m_name);
#endif

		msg_print(NULL);

		/* Start with nothing */
		strcpy(out_val, "");

		/* Use old inscription */
		if (m_ptr->nickname)
		{
			/* Start with the old inscription */
			strcpy(out_val, quark_str(m_ptr->nickname));
			old_name = TRUE;
		}

		/* Get a new inscription (possibly empty) */
#ifdef JP
		if (get_string("名前: ", out_val, 15))
#else
		if (get_string("Name: ", out_val, 15))
#endif

		{
			if (out_val[0])
			{
				/* Save the inscription */
				m_ptr->nickname = quark_add(out_val);
				if (record_named_pet)
				{
					char m_name[80];

					monster_desc(m_name, m_ptr, 0x08);
					do_cmd_write_nikki(NIKKI_NAMED_PET, 0, m_name);
				}
			}
			else
			{
				if (record_named_pet && old_name)
				{
					char m_name[80];

					monster_desc(m_name, m_ptr, 0x08);
					do_cmd_write_nikki(NIKKI_NAMED_PET, 1, m_name);
				}
				m_ptr->nickname = 0;
			}
		}
	}
}

/*
 * Issue a pet command
 */
void do_cmd_pet(void)
{
	int			i = 0;
	int			num;
	int			powers[36];
	cptr			power_desc[36];
	bool			flag, redraw;
	int			ask;
	char			choice;
	char			out_val[160];
	int			pets = 0, pet_ctr;
	monster_type	*m_ptr;

	int mode = 0;

	byte y = 1, x = 0;
	int ctr = 0;
	char buf[160];
	char target_buf[160];

	num = 0;

	/* Calculate pets */
	/* Process the monsters (backwards) */
	for (pet_ctr = m_max - 1; pet_ctr >= 1; pet_ctr--)
	{
		/* Access the monster */
		m_ptr = &m_list[pet_ctr];

		if (is_pet(m_ptr)) pets++;
	}

#ifdef JP
	power_desc[num] = "ペットを放す";
#else
	power_desc[num] = "dismiss pets";
#endif

	powers[num++] = PET_DISMISS;

#ifdef JP
	sprintf(target_buf,"ペットのターゲットを指定 (現在：%s)",
		(pet_t_m_idx ? r_name + r_info[m_list[pet_t_m_idx].r_idx].name : "指定なし"));
#else
	sprintf(target_buf,"specify a targert of pet (now:%s)",
		(pet_t_m_idx ? r_name + r_info[m_list[pet_t_m_idx].r_idx].name : "nothing"));
#endif
	power_desc[num] = target_buf;

	powers[num++] = PET_TARGET;

#ifdef JP
power_desc[num] = "近くにいろ";
#else
	power_desc[num] = "stay close";
#endif

	if (p_ptr->pet_follow_distance == PET_CLOSE_DIST) mode = num;
	powers[num++] = PET_STAY_CLOSE;

#ifdef JP
	power_desc[num] = "ついて来い";
#else
	power_desc[num] = "follow me";
#endif

	if (p_ptr->pet_follow_distance == PET_FOLLOW_DIST) mode = num;
	powers[num++] = PET_FOLLOW_ME;

#ifdef JP
power_desc[num] = "敵を見つけて倒せ";
#else
	power_desc[num] = "seek and destroy";
#endif

	if (p_ptr->pet_follow_distance == PET_DESTROY_DIST) mode = num;
	powers[num++] = PET_SEEK_AND_DESTROY;

#ifdef JP
power_desc[num] = "少し離れていろ";
#else
	power_desc[num] = "give me space";
#endif

	if (p_ptr->pet_follow_distance == PET_SPACE_DIST) mode = num;
	powers[num++] = PET_ALLOW_SPACE;

#ifdef JP
power_desc[num] = "離れていろ";
#else
	power_desc[num] = "stay away";
#endif

	if (p_ptr->pet_follow_distance == PET_AWAY_DIST) mode = num;
	powers[num++] = PET_STAY_AWAY;

	if (p_ptr->pet_extra_flags & PF_OPEN_DOORS)
	{
#ifdef JP
		power_desc[num] = "ドアを開ける (現在:ON)";
#else
		power_desc[num] = "pets open doors (now On)";
#endif

	}
	else
	{
#ifdef JP
		power_desc[num] = "ドアを開ける (現在:OFF)";
#else
		power_desc[num] = "pets open doors (now Off)";
#endif

	}
	powers[num++] = PET_OPEN_DOORS;

	if (p_ptr->pet_extra_flags & PF_PICKUP_ITEMS)
	{
#ifdef JP
		power_desc[num] = "アイテムを拾う (現在:ON)";
#else
		power_desc[num] = "pets pick up items (now On)";
#endif

	}
	else
	{
#ifdef JP
		power_desc[num] = "アイテムを拾う (現在:OFF)";
#else
		power_desc[num] = "pets pick up items (now Off)";
#endif

	}
	powers[num++] = PET_TAKE_ITEMS;

	if (p_ptr->pet_extra_flags & PF_TELEPORT)
	{
#ifdef JP
		power_desc[num] = "テレポート系魔法を使う (現在:ON)";
#else
		power_desc[num] = "allow teleport (now On)";
#endif

	}
	else
	{
#ifdef JP
		power_desc[num] = "テレポート系魔法を使う (現在:OFF)";
#else
		power_desc[num] = "allow teleport (now Off)";
#endif

	}
	powers[num++] = PET_TELEPORT;

	if (p_ptr->pet_extra_flags & PF_ATTACK_SPELL)
	{
#ifdef JP
		power_desc[num] = "攻撃魔法を使う (現在:ON)";
#else
		power_desc[num] = "allow cast attack spell (now On)";
#endif

	}
	else
	{
#ifdef JP
		power_desc[num] = "攻撃魔法を使う (現在:OFF)";
#else
		power_desc[num] = "allow cast attack spell (now Off)";
#endif

	}
	powers[num++] = PET_ATTACK_SPELL;

	if (p_ptr->pet_extra_flags & PF_SUMMON_SPELL)
	{
#ifdef JP
		power_desc[num] = "召喚魔法を使う (現在:ON)";
#else
		power_desc[num] = "allow cast summon spell (now On)";
#endif

	}
	else
	{
#ifdef JP
		power_desc[num] = "召喚魔法を使う (現在:OFF)";
#else
		power_desc[num] = "allow cast summon spell (now Off)";
#endif

	}
	powers[num++] = PET_SUMMON_SPELL;

	if (p_ptr->pet_extra_flags & PF_BALL_SPELL)
	{
#ifdef JP
		power_desc[num] = "プレイヤーを巻き込む範囲魔法を使う (現在:ON)";
#else
		power_desc[num] = "allow involve player in area spell (now On)";
#endif

	}
	else
	{
#ifdef JP
		power_desc[num] = "プレイヤーを巻き込む範囲魔法を使う (現在:OFF)";
#else
		power_desc[num] = "allow involve player in area spell (now Off)";
#endif

	}
	powers[num++] = PET_BALL_SPELL;

	if (p_ptr->riding)
	{
#ifdef JP
		power_desc[num] = "ペットから降りる";
#else
		power_desc[num] = "get off a pet";
#endif

	}
	else
	{
#ifdef JP
		power_desc[num] = "ペットに乗る";
#else
		power_desc[num] = "ride a pet";
#endif

	}
	powers[num++] = PET_RIDING;

#ifdef JP
	power_desc[num] = "ペットに名前をつける。";
#else
	power_desc[num] = "name pets";
#endif

	powers[num++] = PET_NAME;

	if (p_ptr->riding && buki_motteruka(INVEN_RARM) && (empty_hands(FALSE) & 0x00000001) && ((inventory[INVEN_RARM].weight > 99) || (inventory[INVEN_RARM].tval == TV_POLEARM)))
	{
		if (p_ptr->pet_extra_flags & PF_RYOUTE)
		{
#ifdef JP
			power_desc[num] = "武器を片手で持つ";
#else
			power_desc[num] = "use one hand to control a riding pet";
#endif

		}
		else
		{
#ifdef JP
			power_desc[num] = "武器を両手で持つ";
#else
			power_desc[num] = "use both hands for a weapon.";
#endif

		}

		powers[num++] = PET_RYOUTE;
	}

	/* Nothing chosen yet */
	flag = FALSE;

	/* Build a prompt (accept all spells) */
	if (num <= 26)
	{
		/* Build a prompt (accept all spells) */
#ifdef JP
strnfmt(out_val, 78, "(コマンド %c-%c、'*'=一覧、ESC=終了) コマンドを選んでください:",
#else
		strnfmt(out_val, 78, "(Command %c-%c, *=List, ESC=exit) Select a command: ",
#endif

			I2A(0), I2A(num - 1));
	}
	else
	{
#ifdef JP
strnfmt(out_val, 78, "(コマンド %c-%c、'*'=一覧、ESC=終了) コマンドを選んでください:",
#else
		strnfmt(out_val, 78, "(Command %c-%c, *=List, ESC=exit) Select a command: ",
#endif

			I2A(0), '0' + num - 27);
	}

	/* Show list */
	redraw = TRUE;

	/* Save the screen */
	Term_save();

	prt("", y++, x);

	while (ctr < num)
	{
		prt(format("%s%c) %s", (ctr == mode) ? "*" : " ", I2A(ctr), power_desc[ctr]), y + ctr, x);
		ctr++;
	}

	if (ctr < 17)
	{
		prt("", y + ctr, x);
	}
	else
	{
		prt("", y + 17, x);
	}

	/* Get a command from the user */
	while (!flag && get_com(out_val, &choice, TRUE))
	{
		/* Request redraw */
		if ((choice == ' ') || (choice == '*') || (choice == '?'))
		{
			/* Show the list */
			if (!redraw)
			{
				y = 1;
				x = 0;
				ctr = 0;

				/* Show list */
				redraw = TRUE;

				/* Save the screen */
				Term_save();

				prt("", y++, x);

				while (ctr < num)
				{
					sprintf(buf, "%s%c) %s", (ctr == mode) ? "*" : " ", I2A(ctr), power_desc[ctr]);
					prt(buf, y + ctr, x);
					ctr++;
				}

				if (ctr < 17)
				{
					prt("", y + ctr, x);
				}
				else
				{
					prt("", y + 17, x);
				}
			}

			/* Hide the list */
			else
			{
				/* Hide list */
				redraw = FALSE;

				/* Restore the screen */
				Term_load();
			}

			/* Redo asking */
			continue;
		}

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

		/* Totally Illegal */
		if ((i < 0) || (i >= num))
		{
			bell();
			continue;
		}

		/* Verify it */
		if (ask)
		{
			/* Prompt */
#ifdef JP
			strnfmt(buf, 78, "%sを使いますか？ ", power_desc[i]);
#else
			strnfmt(buf, 78, "Use %s? ", power_desc[i]);
#endif


			/* Belay that order */
			if (!get_check(buf)) continue;
		}

		/* Stop the loop */
		flag = TRUE;
	}

	/* Restore the screen */
	if (redraw) Term_load();

	/* Abort if needed */
	if (!flag)
	{
		energy_use = 0;
		return;
	}

	switch (powers[i])
	{
		case PET_DISMISS: /* Dismiss pets */
		{
			if (!pets)
			{
#ifdef JP
				msg_print("ペットがいない！");
#else
				msg_print("You have no pets!");
#endif
				break;
			}
			do_cmd_pet_dismiss();
			(void)calculate_upkeep();
			break;
		}
		case PET_TARGET:
		{
			project_length = -1;
			if (!target_set(TARGET_KILL)) pet_t_m_idx = 0;
			else
			{
				cave_type *c_ptr = &cave[target_row][target_col];
				if (c_ptr->m_idx && (m_list[c_ptr->m_idx].ml))
				{
					pet_t_m_idx = cave[target_row][target_col].m_idx;
					p_ptr->pet_follow_distance = PET_DESTROY_DIST;
				}
				else pet_t_m_idx = 0;
			}
			project_length = 0;

			break;
		}
		/* Call pets */
		case PET_STAY_CLOSE:
		{
			p_ptr->pet_follow_distance = PET_CLOSE_DIST;
			pet_t_m_idx = 0;
			break;
		}
		/* "Follow Me" */
		case PET_FOLLOW_ME:
		{
			p_ptr->pet_follow_distance = PET_FOLLOW_DIST;
			pet_t_m_idx = 0;
			break;
		}
		/* "Seek and destoy" */
		case PET_SEEK_AND_DESTROY:
		{
			p_ptr->pet_follow_distance = PET_DESTROY_DIST;
			break;
		}
		/* "Give me space" */
		case PET_ALLOW_SPACE:
		{
			p_ptr->pet_follow_distance = PET_SPACE_DIST;
			break;
		}
		/* "Stay away" */
		case PET_STAY_AWAY:
		{
			p_ptr->pet_follow_distance = PET_AWAY_DIST;
			break;
		}
		/* flag - allow pets to open doors */
		case PET_OPEN_DOORS:
		{
			if (p_ptr->pet_extra_flags & PF_OPEN_DOORS) p_ptr->pet_extra_flags &= ~(PF_OPEN_DOORS);
			else p_ptr->pet_extra_flags |= (PF_OPEN_DOORS);
			break;
		}
		/* flag - allow pets to pickup items */
		case PET_TAKE_ITEMS:
		{
			if (p_ptr->pet_extra_flags & PF_PICKUP_ITEMS)
			{
				p_ptr->pet_extra_flags &= ~(PF_PICKUP_ITEMS);
				for (pet_ctr = m_max - 1; pet_ctr >= 1; pet_ctr--)
				{
					/* Access the monster */
					m_ptr = &m_list[pet_ctr];

					if (is_pet(m_ptr))
					{
						monster_drop_carried_objects(m_ptr);
					}
				}
			}
			else p_ptr->pet_extra_flags |= (PF_PICKUP_ITEMS);

			break;
		}
		/* flag - allow pets to teleport */
		case PET_TELEPORT:
		{
			if (p_ptr->pet_extra_flags & PF_TELEPORT) p_ptr->pet_extra_flags &= ~(PF_TELEPORT);
			else p_ptr->pet_extra_flags |= (PF_TELEPORT);
			break;
		}
		/* flag - allow pets to cast attack spell */
		case PET_ATTACK_SPELL:
		{
			if (p_ptr->pet_extra_flags & PF_ATTACK_SPELL) p_ptr->pet_extra_flags &= ~(PF_ATTACK_SPELL);
			else p_ptr->pet_extra_flags |= (PF_ATTACK_SPELL);
			break;
		}
		/* flag - allow pets to cast attack spell */
		case PET_SUMMON_SPELL:
		{
			if (p_ptr->pet_extra_flags & PF_SUMMON_SPELL) p_ptr->pet_extra_flags &= ~(PF_SUMMON_SPELL);
			else p_ptr->pet_extra_flags |= (PF_SUMMON_SPELL);
			break;
		}
		/* flag - allow pets to cast attack spell */
		case PET_BALL_SPELL:
		{
			if (p_ptr->pet_extra_flags & PF_BALL_SPELL) p_ptr->pet_extra_flags &= ~(PF_BALL_SPELL);
			else p_ptr->pet_extra_flags |= (PF_BALL_SPELL);
			break;
		}

		case PET_RIDING:
		{
			do_riding(FALSE);
			break;
		}

		case PET_NAME:
		{
			do_name_pet();
			break;
		}

		case PET_RYOUTE:
		{
			if (p_ptr->pet_extra_flags & PF_RYOUTE) p_ptr->pet_extra_flags &= ~(PF_RYOUTE);
			else p_ptr->pet_extra_flags |= (PF_RYOUTE);
			p_ptr->update |= (PU_BONUS);
			handle_stuff();
			break;
		}
	}
}
