/* File: mind.c */

/* Purpose: Mindcrafter code */

/*
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "angband.h"

#define TECHNIC_HISSATSU (REALM_HISSATSU - MIN_TECHNIC - 1)

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
#ifdef JP
(void) strnfmt(out_val, 78, "(%^s %c-%c, '*'で一覧, ESC) どの%sを使いますか？",
#else
	(void)strnfmt(out_val, 78, "(%^ss %c-%c, *=List, ESC=exit) Use which %s? ",
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
					break;
				}

				case '8':
				case 'k':
				case 'K':
				{
					do
					{
						menu_line += 31;
						if (menu_line > 32) menu_line -= 32;
					} while(!(spell_learned1 & (1L << (menu_line-1))));
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
					} while(!(spell_learned1 & (1L << (menu_line-1))));
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
					while(!(spell_learned1 & (1L << (menu_line-1))))
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
					if (!(spell_learned1 >> i)) break;

					/* Access the spell */
					if (spell.slevel > plev)   continue;
					if (!(spell_learned1 & (1L << i))) continue;
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
					        spell_names[technic2magic(REALM_HISSATSU)-1][i],
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
		if ((i < 0) || (i >= 32) || !(spell_learned1 & (1 << sentaku[i])))
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
(void) strnfmt(tmp_val, 78, "%sを使いますか？", spell_names[technic2magic(REALM_HISSATSU)-1][j]);
#else
			(void)strnfmt(tmp_val, 78, "Use %s? ", spell_names[technic2magic(REALM_HISSATSU)-1][j]);
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
static bool cast_hissatsu_spell(int spell)
{
	int             y, x;
	int             dir;


	/* spell code */
	switch (spell)
	{
	case 0:
		project_length = 2;
		if (!get_aim_dir(&dir)) return FALSE;
		project_hook(GF_ATTACK, dir, HISSATSU_2, PROJECT_STOP | PROJECT_KILL | PROJECT_NO_REF);

		break;
	case 1:
	{
		int cdir;
		if (!get_rep_dir2(&dir)) return FALSE;
		if (dir == 5) return FALSE;
		for (cdir = 0;cdir < 8; cdir++)
		{
			if (cdd[cdir] == dir) break;
		}
		if (cdir == 8) return FALSE;
		y = py + ddy_cdd[cdir];
		x = px + ddx_cdd[cdir];
		if (cave[y][x].m_idx)
			py_attack(y, x, 0);
		else
#ifdef JP
			msg_print("攻撃は空を切った。");
#else
			msg_print("You attack the empty air.");
#endif
		y = py + ddy_cdd[(cdir + 7) % 8];
		x = px + ddx_cdd[(cdir + 7) % 8];
		if (cave[y][x].m_idx)
			py_attack(y, x, 0);
		else
#ifdef JP
			msg_print("攻撃は空を切った。");
#else
			msg_print("You attack the empty air.");
#endif
		y = py + ddy_cdd[(cdir + 1) % 8];
		x = px + ddx_cdd[(cdir + 1) % 8];
		if (cave[y][x].m_idx)
			py_attack(y, x, 0);
		else
#ifdef JP
			msg_print("攻撃は空を切った。");
#else
			msg_print("You attack the empty air.");
#endif

		break;
	}
	case 2:
	{
		if (!do_cmd_throw_aux(1, TRUE, 0)) return FALSE;
		break;
	}
	case 3:
	{
		if (!get_rep_dir2(&dir)) return FALSE;
		if (dir == 5) return FALSE;
		y = py + ddy[dir];
		x = px + ddx[dir];
		if (cave[y][x].m_idx)
			py_attack(y, x, HISSATSU_FIRE);
		else
		{
#ifdef JP
			msg_print("その方向にはモンスターはいません。");
#else
			msg_print("There is no monster.");
#endif
			return FALSE;
		}
		break;
	}
	case 4:
	{
		detect_monsters_mind(DETECT_RAD_DEFAULT);
		break;
	}
	case 5:
	{
		if (!get_rep_dir2(&dir)) return FALSE;
		if (dir == 5) return FALSE;
		y = py + ddy[dir];
		x = px + ddx[dir];
		if (cave[y][x].m_idx)
			py_attack(y, x, HISSATSU_MINEUCHI);
		else
		{
#ifdef JP
			msg_print("その方向にはモンスターはいません。");
#else
			msg_print("There is no monster.");
#endif
			return FALSE;
		}
		break;
	}
	case 6:
	{
		if (p_ptr->riding)
		{
#ifdef JP
			msg_print("乗馬中には無理だ。");
#else
			msg_print("You cannot do it when riding.");
#endif
			return FALSE;
		}
#ifdef JP
		msg_print("相手の攻撃に対して身構えた。");
#else
		msg_print("You prepare to counter blow.");
#endif
		p_ptr->counter = TRUE;
		break;
	}
	case 7:
	{
		if (p_ptr->riding)
		{
#ifdef JP
			msg_print("乗馬中には無理だ。");
#else
			msg_print("You cannot do it when riding.");
#endif
			return FALSE;
		}

		if (!get_rep_dir2(&dir)) return FALSE;

		if (dir == 5) return FALSE;
		y = py + ddy[dir];
		x = px + ddx[dir];

		if (!cave[y][x].m_idx)
		{
#ifdef JP
			msg_print("その方向にはモンスターはいません。");
#else
			msg_print("There is no monster.");
#endif
			return FALSE;
		}

		py_attack(y, x, 0);

		if (!player_can_enter(cave[y][x].feat) || is_trap(cave[y][x].feat))
			break;

		y += ddy[dir];
		x += ddx[dir];

		if (player_can_enter(cave[y][x].feat) && !is_trap(cave[y][x].feat) && !cave[y][x].m_idx)
		{
			int oy, ox;

			msg_print(NULL);

			/* Save the old location */
			oy = py;
			ox = px;

			/* Move the player */
			py = y;
			px = x;

			forget_flow();

			/* Redraw the old spot */
			lite_spot(oy, ox);

			/* Redraw the new spot */
			lite_spot(py, px);

			/* Check for new panel (redraw map) */
			verify_panel();

			/* Update stuff */
			p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);

			/* Update the monsters */
			p_ptr->update |= (PU_DISTANCE);

			/* Window stuff */
			p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

			/* Handle stuff XXX XXX XXX */
			handle_stuff();
		}
		break;
	}
	case 8:
	{
		if (!get_rep_dir2(&dir)) return FALSE;
		if (dir == 5) return FALSE;
		y = py + ddy[dir];
		x = px + ddx[dir];
		if (cave[y][x].m_idx)
			py_attack(y, x, HISSATSU_POISON);
		else
		{
#ifdef JP
			msg_print("その方向にはモンスターはいません。");
#else
			msg_print("There is no monster.");
#endif
			return FALSE;
		}
		break;
	}
	case 9:
	{
		if (!get_rep_dir2(&dir)) return FALSE;
		if (dir == 5) return FALSE;
		y = py + ddy[dir];
		x = px + ddx[dir];
		if (cave[y][x].m_idx)
			py_attack(y, x, HISSATSU_ZANMA);
		else
		{
#ifdef JP
			msg_print("その方向にはモンスターはいません。");
#else
			msg_print("There is no monster.");
#endif
			return FALSE;
		}
		break;
	}
	case 10:
	{
		if (!get_rep_dir2(&dir)) return FALSE;
		if (dir == 5) return FALSE;
		y = py + ddy[dir];
		x = px + ddx[dir];
		if (cave[y][x].m_idx)
			py_attack(y, x, 0);
		else
		{
#ifdef JP
			msg_print("その方向にはモンスターはいません。");
#else
			msg_print("There is no monster.");
#endif
			return FALSE;
		}
		if (d_info[dungeon_type].flags1 & DF1_NO_MELEE)
		{
			return TRUE;
		}
		if (cave[y][x].m_idx)
		{
			int i;
			int ty = y, tx = x;
			int oy = y, ox = x;
			int m_idx = cave[y][x].m_idx;
			monster_type *m_ptr = &m_list[m_idx];
			char m_name[80];

			monster_desc(m_name, m_ptr, 0);

			for (i = 0; i < 5; i++)
			{
				y += ddy[dir];
				x += ddx[dir];
				if (cave_empty_bold(y, x))
				{
					ty = y;
					tx = x;
				}
				else break;
			}
			if ((ty != oy) || (tx != ox))
			{
#ifdef JP
				msg_format("%sを吹き飛ばした！", m_name);
#else
				msg_format("You blow %s away!", m_name);
#endif
				cave[oy][ox].m_idx = 0;
				cave[ty][tx].m_idx = m_idx;
				m_ptr->fy = ty;
				m_ptr->fx = tx;

				update_mon(m_idx, TRUE);
				lite_spot(oy, ox);
				lite_spot(ty, tx);
			}
		}
		break;
	}
	case 11:
	{
		if (p_ptr->lev > 44)
		{
			if (!identify_fully(TRUE)) return FALSE;
		}
		else
		{
			if (!ident_spell(TRUE)) return FALSE;
		}
		break;
	}
	case 12:
	{
		if (!get_rep_dir2(&dir)) return FALSE;
		if (dir == 5) return FALSE;
		y = py + ddy[dir];
		x = px + ddx[dir];
		if (cave[y][x].m_idx)
			py_attack(y, x, HISSATSU_HAGAN);

		/* Non-walls (etc) */
		if (cave_floor_bold(y, x)) break;

		/* Permanent walls */
		if (cave[y][x].feat >= FEAT_PERM_EXTRA) break;

		if (cave[y][x].feat < FEAT_DOOR_HEAD) break;

		/* Forget the wall */
		cave[y][x].info &= ~(CAVE_MARK);

		/* Destroy the feature */
		cave[y][x].feat = floor_type[randint0(100)];

		/* Update some things */
		p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW | PU_MONSTERS | PU_MON_LITE);

		lite_spot(y, x);

		break;
	}
	case 13:
	{
		if (!get_rep_dir2(&dir)) return FALSE;
		if (dir == 5) return FALSE;
		y = py + ddy[dir];
		x = px + ddx[dir];
		if (cave[y][x].m_idx)
			py_attack(y, x, HISSATSU_COLD);
		else
		{
#ifdef JP
			msg_print("その方向にはモンスターはいません。");
#else
			msg_print("There is no monster.");
#endif
			return FALSE;
		}
		break;
	}
	case 14:
	{
		if (!get_rep_dir2(&dir)) return FALSE;
		if (dir == 5) return FALSE;
		y = py + ddy[dir];
		x = px + ddx[dir];
		if (cave[y][x].m_idx)
			py_attack(y, x, HISSATSU_KYUSHO);
		else
		{
#ifdef JP
			msg_print("その方向にはモンスターはいません。");
#else
			msg_print("There is no monster.");
#endif
			return FALSE;
		}
		break;
	}
	case 15:
	{
		if (!get_rep_dir2(&dir)) return FALSE;
		if (dir == 5) return FALSE;
		y = py + ddy[dir];
		x = px + ddx[dir];
		if (cave[y][x].m_idx)
			py_attack(y, x, HISSATSU_MAJIN);
		else
		{
#ifdef JP
			msg_print("その方向にはモンスターはいません。");
#else
			msg_print("There is no monster.");
#endif
			return FALSE;
		}
		break;
	}
	case 16:
	{
		if (!get_rep_dir2(&dir)) return FALSE;
		if (dir == 5) return FALSE;
		y = py + ddy[dir];
		x = px + ddx[dir];
		if (cave[y][x].m_idx)
			py_attack(y, x, HISSATSU_SUTEMI);
		else
		{
#ifdef JP
			msg_print("その方向にはモンスターはいません。");
#else
			msg_print("There is no monster.");
#endif
			return FALSE;
		}
		p_ptr->sutemi = TRUE;
		break;
	}
	case 17:
	{
		if (!get_rep_dir2(&dir)) return FALSE;
		if (dir == 5) return FALSE;
		y = py + ddy[dir];
		x = px + ddx[dir];
		if (cave[y][x].m_idx)
			py_attack(y, x, HISSATSU_ELEC);
		else
		{
#ifdef JP
			msg_print("その方向にはモンスターはいません。");
#else
			msg_print("There is no monster.");
#endif
			return FALSE;
		}
		break;
	}
	case 18:
		project_length = 5;
		if (!get_aim_dir(&dir)) return FALSE;
		project_hook(GF_ATTACK, dir, HISSATSU_NYUSIN, PROJECT_STOP | PROJECT_KILL | PROJECT_NO_REF);

		break;
	case 19: /* Whirlwind Attack */
	{
		int y = 0, x = 0;
		cave_type       *c_ptr;
		monster_type    *m_ptr;

		if (p_ptr->cut < 300)
			set_cut(p_ptr->cut + 300);
		else
			set_cut(p_ptr->cut * 2);

		for (dir = 0; dir < 8; dir++)
		{
			y = py + ddy_ddd[dir];
			x = px + ddx_ddd[dir];
			c_ptr = &cave[y][x];

			/* Get the monster */
			m_ptr = &m_list[c_ptr->m_idx];

			/* Hack -- attack monsters */
			if (c_ptr->m_idx && (m_ptr->ml || cave_floor_bold(y, x)))
			{
				if (r_info[m_list[c_ptr->m_idx].r_idx].flags3 & (RF3_DEMON | RF3_UNDEAD | RF3_NONLIVING))
				{
					char m_name[80];

					monster_desc(m_name, &m_list[c_ptr->m_idx], 0);
#ifdef JP
					msg_format("%sには効果がない！", m_name);
#else
					msg_format("%s is unharmed!", m_name);
#endif
				}
				else py_attack(y, x, HISSATSU_SEKIRYUKA);
			}
		}
		break;
	}
	case 20:
	{
		if (!get_rep_dir2(&dir)) return FALSE;
		if (dir == 5) return FALSE;
		y = py + ddy[dir];
		x = px + ddx[dir];
		if (cave[y][x].m_idx)
			py_attack(y, x, HISSATSU_QUAKE);
		else
		{
			earthquake(py, px, 10);
		}
		break;
	}
	case 21:
	{
		int total_damage = 0, basedam, i;
		u32b f1, f2, f3;
		object_type *o_ptr;
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
		msg_print("武器を大きく振り下ろした。");
#else
		msg_print("You swing your weapon downward.");
#endif
		for (i = 0; i < 2; i++)
		{
			int damage;

			if (!buki_motteruka(INVEN_RARM+i)) break;
			o_ptr = &inventory[INVEN_RARM+i];
			basedam = (o_ptr->dd * (o_ptr->ds + 1)) * 50;
			damage = o_ptr->to_d * 100;
			object_flags(o_ptr, &f1, &f2, &f3);
			if ((o_ptr->name1 == ART_VORPAL_BLADE) || (o_ptr->name1 == ART_CHAINSWORD))
			{
				/* vorpal blade */
				basedam *= 5;
				basedam /= 3;
			}
			else if (object_known_p(o_ptr) && (f1 & TR1_VORPAL))
			{
				/* vorpal flag only */
				basedam *= 11;
				basedam /= 9;
			}
			damage += basedam;
			damage *= p_ptr->num_blow[i];
			total_damage += damage / 200;
			if (i) total_damage = total_damage*7/10;
		}
		fire_beam(GF_FORCE, dir, total_damage);
		break;
	}
	case 22:
	{
#ifdef JP
		msg_print("雄叫びをあげた！");
#else
		msg_print("You roar out!");
#endif
		project_hack(GF_SOUND, randint1(p_ptr->lev * 3));
		aggravate_monsters(0);
		break;
	}
	case 23:
	{
		int i;
		if (!get_rep_dir2(&dir)) return FALSE;
		if (dir == 5) return FALSE;
		y = py + ddy[dir];
		x = px + ddx[dir];
		for (i = 0; i < 3; i++)
		{
			if (cave[y][x].m_idx)
				py_attack(y, x, HISSATSU_3DAN);
			else
			{
#ifdef JP
				msg_print("その方向にはモンスターはいません。");
#else
				msg_print("There is no monster.");
#endif
				return FALSE;
			}
			if (d_info[dungeon_type].flags1 & DF1_NO_MELEE)
			{
				return TRUE;
			}
			if (cave[y][x].m_idx)
			{
				int oy, ox;
				int ny = y + ddy[dir];
				int nx = x + ddx[dir];
				int m_idx = cave[y][x].m_idx;
				monster_type *m_ptr = &m_list[m_idx];
				if (cave_empty_bold(ny, nx))
				{
					cave[y][x].m_idx = 0;
					cave[ny][nx].m_idx = m_idx;
					m_ptr->fy = ny;
					m_ptr->fx = nx;

					update_mon(m_idx, TRUE);

					/* Save the old location */
					oy = py;
					ox = px;

					/* Move the player */
					py = y;
					px = x;

					if (p_ptr->riding)
					{
						int tmp;
						tmp = cave[py][px].m_idx;
						cave[py][px].m_idx = cave[oy][ox].m_idx;
						cave[oy][ox].m_idx = tmp;
						m_list[p_ptr->riding].fy = py;
						m_list[p_ptr->riding].fx = px;
						update_mon(cave[py][px].m_idx, TRUE);
					}

					forget_flow();

					/* Redraw the old spot */
					lite_spot(oy, ox);

					/* Redraw the new spot */
					lite_spot(py, px);

					/* Redraw the new spot */
					lite_spot(ny, nx);

					/* Check for new panel (redraw map) */
					verify_panel();

					/* Update stuff */
					p_ptr->update |= (PU_VIEW | PU_LITE | PU_FLOW);

					/* Update the monsters */
					p_ptr->update |= (PU_DISTANCE);

					/* Window stuff */
					p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

					/* Handle stuff XXX XXX XXX */
					handle_stuff();

					if (i < 2) msg_print(NULL);
					y += ddy[dir];
					x += ddx[dir];
				}
			}
			else break;
		}
		break;
	}
	case 24:
	{
		if (!get_rep_dir2(&dir)) return FALSE;
		if (dir == 5) return FALSE;
		y = py + ddy[dir];
		x = px + ddx[dir];
		if (cave[y][x].m_idx)
			py_attack(y, x, HISSATSU_DRAIN);
		else
		{
#ifdef JP
				msg_print("その方向にはモンスターはいません。");
#else
				msg_print("There is no monster.");
#endif
			return FALSE;
		}
		break;
	}
	case 25:
	{
#ifdef JP
		msg_print("武器を不規則に揺らした．．．");
#else
		msg_print("You irregularly wave your weapon...");
#endif
		project_hack(GF_ENGETSU, p_ptr->lev * 4);
		project_hack(GF_ENGETSU, p_ptr->lev * 4);
		project_hack(GF_ENGETSU, p_ptr->lev * 4);
		break;
	}
	case 26:
	{
		bool new = TRUE;
		int count = 0;
		do
		{
			project_length = 5;
			if (!get_aim_dir(&dir)) break;
			if (new)
				/* Reserve needed mana point */
				p_ptr->csp -= technic_info[TECHNIC_HISSATSU][26].smana;
			else
				p_ptr->csp -= 8;
			new = FALSE;
			if (!project_hook(GF_ATTACK, dir, HISSATSU_NYUSIN, PROJECT_STOP | PROJECT_KILL | PROJECT_NO_REF)) break;
			count++;
			command_dir = 0;
			p_ptr->redraw |= PR_MANA;
			handle_stuff();
		} while (p_ptr->csp > 8);
		if (new) return FALSE;

		/* Restore reserved mana */
		p_ptr->csp += technic_info[TECHNIC_HISSATSU][26].smana;

		break;
	}
	case 27:
	{
		if (!tgt_pt(&x, &y)) return FALSE;
		if (!cave_empty_bold(y, x) || (cave[y][x].info & CAVE_ICKY) ||
			(distance(y, x, py, px) > MAX_SIGHT / 2) ||
		    !projectable(py, px, y, x))
		{
#ifdef JP
			msg_print("失敗！");
#else
			msg_print("You cannot move to that place!");
#endif
			break;
		}
		if (p_ptr->anti_tele)
		{
#ifdef JP
msg_print("不思議な力がテレポートを防いだ！");
#else
			msg_print("A mysterious force prevents you from teleporting!");
#endif

			break;
		}
		project(0, 0, y, x, HISSATSU_ISSEN, GF_ATTACK, PROJECT_BEAM | PROJECT_KILL | PROJECT_NO_REF, -1);
		teleport_player_to(y, x, TRUE);
		break;
	}
	case 28:
	{
		int x, y;

		if (!get_rep_dir(&dir, FALSE)) return FALSE;
		y = py + ddy[dir];
		x = px + ddx[dir];
		if (cave[y][x].m_idx)
		{
			py_attack(y, x, 0);
			if (cave[y][x].m_idx)
			{
				handle_stuff();
				py_attack(y, x, 0);
			}
		}
		else
		{
#ifdef JP
msg_print("その方向にはモンスターはいません。");
#else
			msg_print("You don't see any monster in this direction");
#endif
			return FALSE;
		}
		break;
	}
	case 29:
	{
		int total_damage = 0, basedam, i;
		int y, x;
		u32b f1, f2, f3;
		object_type *o_ptr;

		if (!get_rep_dir2(&dir)) return FALSE;
		if (dir == 5) return FALSE;
		y = py + ddy[dir];
		x = px + ddx[dir];
		if (d_info[dungeon_type].flags1 & DF1_NO_MELEE)
		{
#ifdef JP
			msg_print("なぜか攻撃することができない。");
#else
			msg_print("Something prevent you from attacking.");
#endif
			return TRUE;
		}
#ifdef JP
		msg_print("武器を大きく振り下ろした。");
#else
		msg_print("You swing your weapon downward.");
#endif
		for (i = 0; i < 2; i++)
		{
			int damage;
			if (!buki_motteruka(INVEN_RARM+i)) break;
			o_ptr = &inventory[INVEN_RARM+i];
			basedam = (o_ptr->dd * (o_ptr->ds + 1)) * 50;
			damage = o_ptr->to_d * 100;
			object_flags(o_ptr, &f1, &f2, &f3);
			if ((o_ptr->name1 == ART_VORPAL_BLADE) || (o_ptr->name1 == ART_CHAINSWORD))
			{
				/* vorpal blade */
				basedam *= 5;
				basedam /= 3;
			}
			else if (object_known_p(o_ptr) && (f1 & TR1_VORPAL))
			{
				/* vorpal flag only */
				basedam *= 11;
				basedam /= 9;
			}
			damage += basedam;
			damage += p_ptr->to_d[i] * 100;
			damage *= p_ptr->num_blow[i];
			total_damage += (damage / 100);
		}
		project(0, (cave_floor_bold(y, x) ? 5 : 0), y, x, total_damage * 3 / 2, GF_METEOR, PROJECT_KILL | PROJECT_JUMP | PROJECT_ITEM, -1);
		break;
	}
	case 30:
	{
		if (!get_rep_dir2(&dir)) return FALSE;
		if (dir == 5) return FALSE;
		y = py + ddy[dir];
		x = px + ddx[dir];
		if (cave[y][x].m_idx)
			py_attack(y, x, HISSATSU_UNDEAD);
		else
		{
#ifdef JP
			msg_print("その方向にはモンスターはいません。");
#else
			msg_print("There is no monster.");
#endif
			return FALSE;
		}
#ifdef JP
		take_hit(DAMAGE_NOESCAPE, 100 + randint1(100), "慶雲鬼忍剣を使った衝撃", -1);
#else
		take_hit(DAMAGE_NOESCAPE, 100 + randint1(100), "reaction of your attack", -1); /*nanka*/
#endif
		break;
	}
	case 31:
	{
		int i;
#ifdef JP
if (!get_check("本当に自殺しますか？")) return FALSE;
#else
		if (!get_check("Do you really want to commit suicide? ")) return FALSE;
#endif
			/* Special Verification for suicide */
#ifdef JP
prt("確認のため '@' を押して下さい。", 0, 0);
#else
		prt("Please verify SUICIDE by typing the '@' sign: ", 0, 0);
#endif

		flush();
		i = inkey();
		prt("", 0, 0);
		if (i != '@') return FALSE;
		if (total_winner)
		{
			take_hit(DAMAGE_FORCE, 9999, "Seppuku", -1);
			total_winner = TRUE;
		}
		else
		{
#ifdef JP
			msg_print("武士道とは、死ぬことと見つけたり。");
			take_hit(DAMAGE_FORCE, 9999, "切腹", -1);
#else
			msg_print("Meaning of Bushi-do is found in the death.");
			take_hit(DAMAGE_FORCE, 9999, "Seppuku", -1);
#endif
		}
		break;
	}
	default:
#ifdef JP
msg_print("なに？");
#else
		msg_print("Zap?");
#endif

	}

	return TRUE;
}


/*
 * do_cmd_cast calls this function if the player's class
 * is 'mindcrafter'.
 */
void do_cmd_hissatsu(void)
{
	int             n = 0;
	magic_type      spell;
	bool            cast;


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
	if (!buki_motteruka(INVEN_RARM))
	{
		if (flush_failure) flush();
#ifdef JP
msg_print("武器を持たないと必殺技は使えない！");
#else
		msg_print("You need to wield a weapon!");
#endif

		return;
	}
	if (!spell_learned1)
	{
#ifdef JP
msg_print("何も技を知らない。");
#else
		msg_print("You don't know any martial arts.");
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
	cast = cast_hissatsu_spell(n);

	if (!cast) return;

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
		if (spell_learned1 & (1L << i)) continue;
		if (technic_info[TECHNIC_HISSATSU][i].slevel > p_ptr->lev) continue;

		spell_learned1 |= (1L << i);
		spell_worked1 |= (1L << i);
#ifdef JP
		msg_format("%sの技を覚えた。", spell_names[technic2magic(REALM_HISSATSU)-1][i]);
#else
		msg_format("You have learned the special attack of %s.", spell_names[technic2magic(REALM_HISSATSU)-1][i]);
#endif
		for (j = 0; j < 64; j++)
		{
			/* Stop at the first empty space */
			if (spell_order[j] == 99) break;
		}
		spell_order[j] = i;
		gain = TRUE;
	}
	if (!gain)
#ifdef JP
		msg_print("何も覚えられなかった。");
#else
		msg_print("You were not able to learn any special attacks.");
#endif

	p_ptr->update |= (PU_SPELLS);
}
