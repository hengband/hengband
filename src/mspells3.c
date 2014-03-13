/*!
 * @file mspells3.c
 * @brief 青魔法の処理実装 / Blue magic
 * @date 2014/01/15
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen.\n
 */

#include "angband.h"

#define pseudo_plev() (((p_ptr->lev + 40) * (p_ptr->lev + 40) - 1550) / 130) /*!< モンスター魔法をプレイヤーが使用する場合の換算レベル */


/*!
* @brief 文字列に青魔導師の呪文の攻撃力を加える
* @param r_idx モンスターの種族ID
* @param SPELL_NUM 呪文番号
* @param msg 表示する文字列
* @param tmp 返すメッセージを格納する配列
* @return なし
*/
void set_bluemage_damage(int SPELL_NUM, int plev, cptr msg, char* tmp)
{
    int base_damage = monspell_bluemage_damage(SPELL_NUM, plev, BASE_DAM);
    int dice_num = monspell_bluemage_damage(SPELL_NUM, plev, DICE_NUM);
    int dice_side = monspell_bluemage_damage(SPELL_NUM, plev, DICE_SIDE);
    int dice_mult = monspell_bluemage_damage(SPELL_NUM, plev, DICE_MULT);
    int dice_div = monspell_bluemage_damage(SPELL_NUM, plev, DICE_DIV);
    char dmg_str[80];
    dice_to_string(base_damage, dice_num, dice_side, dice_mult, dice_div, dmg_str);
    sprintf(tmp, msg, dmg_str);
}

/*!
 * @brief 受け取ったモンスター魔法のIDに応じて青魔法の効果情報をまとめたフォーマットを返す
 * @param p 情報を返す文字列参照ポインタ
 * @param power モンスター魔法のID
 * @return なし
 */
static void learned_info(char *p, int power)
{
	int plev = pseudo_plev();
	int hp = p_ptr->chp;

#ifdef JP
	cptr s_dam = " 損傷:%s";
	cptr s_dur = "期間:";
	cptr s_range = "範囲:";
	cptr s_heal = " 回復:%s";
#else
	cptr s_dam = " dam %s";
	cptr s_dur = "dur ";
	cptr s_range = "range ";
	cptr s_heal = " heal %s";
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
            set_bluemage_damage((power), plev, s_dam, p); break;
		case MS_DISPEL:
			break;
        case MS_ROCKET:
        case MS_SHOOT:
        case MS_BR_ACID:
        case MS_BR_ELEC:
        case MS_BR_FIRE:
        case MS_BR_COLD:
        case MS_BR_POIS:
        case MS_BR_NUKE: 
        case MS_BR_NEXUS:
        case MS_BR_TIME:
        case MS_BR_GRAVITY:
        case MS_BR_MANA:
        case MS_BR_NETHER:
        case MS_BR_LITE:
        case MS_BR_DARK:
        case MS_BR_CONF:
        case MS_BR_SOUND:
        case MS_BR_CHAOS:
        case MS_BR_DISEN:
        case MS_BR_SHARDS:
        case MS_BR_PLASMA:
        case MS_BR_INERTIA:
        case MS_BR_FORCE:
        case MS_BR_DISI:
        case MS_BALL_NUKE:
        case MS_BALL_CHAOS:
        case MS_BALL_ACID:
        case MS_BALL_ELEC:
        case MS_BALL_FIRE:
        case MS_BALL_COLD:
        case MS_BALL_POIS:
        case MS_BALL_NETHER:
        case MS_BALL_WATER:
            set_bluemage_damage((power), plev, s_dam, p); break;
        case MS_DRAIN_MANA:
            set_bluemage_damage((power), plev, s_heal, p); break;
        case MS_MIND_BLAST:
        case MS_BRAIN_SMASH:
        case MS_CAUSE_1:
        case MS_CAUSE_2:
        case MS_CAUSE_3:
        case MS_CAUSE_4:
        case MS_BOLT_ACID:
        case MS_BOLT_ELEC:
        case MS_BOLT_FIRE:
        case MS_BOLT_COLD:
        case MS_BOLT_NETHER:
        case MS_BOLT_WATER:
        case MS_BOLT_MANA:
        case MS_BOLT_PLASMA:
        case MS_BOLT_ICE: 
        case MS_MAGIC_MISSILE: 
            set_bluemage_damage((power), plev, s_dam, p); break;
		case MS_SPEED:
			sprintf(p, " %sd%d+%d", s_dur, 20+plev, plev);
			break;
        case MS_HEAL:
            set_bluemage_damage((power), plev, s_heal, p); break;
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
            set_bluemage_damage((power), plev, s_dam, p); break;
			break;
		case MS_RAISE_DEAD:
			sprintf(p, " %s5", s_range);
			break;
		default:
			break;
	}
}


/*!
 * @brief 使用可能な青魔法を選択する /
 * Allow user to choose a imitation.
 * @param sn 選択したモンスター攻撃ID、キャンセルの場合-1、不正な選択の場合-2を返す
 * @return 発動可能な魔法を選択した場合TRUE、キャンセル処理か不正な選択が行われた場合FALSEを返す。
 * @details
 * If a valid spell is chosen, saves it in '*sn' and returns TRUE\n
 * If the user hits escape, returns FALSE, and set '*sn' to -1\n
 * If there are no legal choices, returns FALSE, and sets '*sn' to -2\n
 *\n
 * The "prompt" should be "cast", "recite", or "study"\n
 * The "known" should be TRUE for cast/pray, FALSE for study\n
 *\n
 * nb: This function has a (trivial) display bug which will be obvious\n
 * when you run it. It's probably easy to fix but I haven't tried,\n
 * sorry.\n
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
	cptr            p = _("魔法", "magic");

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
	sprintf(comment, _("[A]ボルト, [B]ボール, [C]ブレス, [D]召喚, [E]その他:", "[A] bolt, [B] ball, [C] breath, [D] summoning, [E] others:"));
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
		msg_print(_("その種類の魔法は覚えていない！", "You don't know any spell of this type."));
		return (FALSE);
	}

	/* Build a prompt (accept all spells) */
	(void)strnfmt(out_val, 78, 
		      _("(%c-%c, '*'で一覧, ESC) どの%sを唱えますか？", "(%c-%c, *=List, ESC=exit) Use which %s? "),
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
				put_str(_("名前", "Name"), y, x + 5);
				put_str(_("MP 失率 効果", "SP Fail Info"), y, x + 33);


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
						if (i == (menu_line-1)) strcpy(psi_desc, _("  》", "  > "));
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
			(void) strnfmt(tmp_val, 78, _("%sの魔法を唱えますか？", "Use %s? "), monster_powers[spellnum[i]].name);

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


/*!
 * @brief 青魔法の発動 /
 * do_cmd_cast calls this function if the player's class is 'blue-mage'.
 * @param spell 発動するモンスター攻撃のID
 * @param success TRUEは成功時、FALSEは失敗時の処理を行う
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
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
        msg_print(_("かん高い金切り声をあげた。", "You make a high pitched shriek."));
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
		
        msg_print(_("ロケットを発射した。", "You fire a rocket."));
        damage = monspell_bluemage_damage((MS_ROCKET), plev, DAM_ROLL);
		fire_rocket(GF_ROCKET, dir, damage, 2);
		break;
	case MS_SHOOT:
	{
		object_type *o_ptr = NULL;

		if (!get_aim_dir(&dir)) return FALSE;
		
        msg_print(_("矢を放った。", "You fire an arrow."));
        damage = monspell_bluemage_damage((MS_SHOOT), plev, DAM_ROLL);
		fire_bolt(GF_ARROW, dir, damage);
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

        msg_print(_("酸のブレスを吐いた。", "You breathe acid."));
        damage = monspell_bluemage_damage((MS_BR_ACID), plev, DAM_ROLL);
		fire_ball(GF_ACID, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_ELEC:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("稲妻のブレスを吐いた。", "You breathe lightning."));
        damage = monspell_bluemage_damage((MS_BR_ELEC), plev, DAM_ROLL);
		fire_ball(GF_ELEC, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_FIRE:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("火炎のブレスを吐いた。", "You breathe fire."));
        damage = monspell_bluemage_damage((MS_BR_FIRE), plev, DAM_ROLL);
		fire_ball(GF_FIRE, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_COLD:
        if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("冷気のブレスを吐いた。", "You breathe frost."));
        damage = monspell_bluemage_damage((MS_BR_COLD), plev, DAM_ROLL);
		fire_ball(GF_COLD, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_POIS:
        if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("ガスのブレスを吐いた。", "You breathe gas."));
        damage = monspell_bluemage_damage((MS_BR_POIS), plev, DAM_ROLL);
		fire_ball(GF_POIS, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_NETHER:
        if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("地獄のブレスを吐いた。", "You breathe nether."));
        damage = monspell_bluemage_damage((MS_BR_NETHER), plev, DAM_ROLL);
		fire_ball(GF_NETHER, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_LITE:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("閃光のブレスを吐いた。", "You breathe light."));
        damage = monspell_bluemage_damage((MS_BR_LITE), plev, DAM_ROLL);
		fire_ball(GF_LITE, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_DARK:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("暗黒のブレスを吐いた。", "You breathe darkness."));
        damage = monspell_bluemage_damage((MS_BR_DARK), plev, DAM_ROLL);
		fire_ball(GF_DARK, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_CONF:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("混乱のブレスを吐いた。", "You breathe confusion."));
        damage = monspell_bluemage_damage((MS_BR_CONF), plev, DAM_ROLL);
		fire_ball(GF_CONFUSION, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_SOUND:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("轟音のブレスを吐いた。", "You breathe sound."));
        damage = monspell_bluemage_damage((MS_BR_SOUND), plev, DAM_ROLL);
		fire_ball(GF_SOUND, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_CHAOS:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("カオスのブレスを吐いた。", "You breathe chaos."));
        damage = monspell_bluemage_damage((MS_BR_CHAOS), plev, DAM_ROLL);
		fire_ball(GF_CHAOS, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_DISEN:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("劣化のブレスを吐いた。", "You breathe disenchantment."));
        damage = monspell_bluemage_damage((MS_BR_DISEN), plev, DAM_ROLL);
		fire_ball(GF_DISENCHANT, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_NEXUS:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("因果混乱のブレスを吐いた。", "You breathe nexus."));
        damage = monspell_bluemage_damage((MS_BR_NEXUS), plev, DAM_ROLL);
		fire_ball(GF_NEXUS, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_TIME:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("時間逆転のブレスを吐いた。", "You breathe time."));
        damage = monspell_bluemage_damage((MS_BR_TIME), plev, DAM_ROLL);
		fire_ball(GF_TIME, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_INERTIA:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("遅鈍のブレスを吐いた。", "You breathe inertia."));
        damage = monspell_bluemage_damage((MS_BR_INERTIA), plev, DAM_ROLL);
		fire_ball(GF_INERTIA, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_GRAVITY:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("重力のブレスを吐いた。", "You breathe gravity."));
        damage = monspell_bluemage_damage((MS_BR_GRAVITY), plev, DAM_ROLL);
		fire_ball(GF_GRAVITY, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_SHARDS:
        if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("破片のブレスを吐いた。", "You breathe shards."));
        damage = monspell_bluemage_damage((MS_BR_SHARDS), plev, DAM_ROLL);
		fire_ball(GF_SHARDS, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_PLASMA:
        if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("プラズマのブレスを吐いた。", "You breathe plasma."));
        damage = monspell_bluemage_damage((MS_BR_PLASMA), plev, DAM_ROLL);
		fire_ball(GF_PLASMA, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_FORCE:
        if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("フォースのブレスを吐いた。", "You breathe force."));
        damage = monspell_bluemage_damage((MS_BR_FORCE), plev, DAM_ROLL);
		fire_ball(GF_FORCE, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BR_MANA:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("魔力のブレスを吐いた。", "You breathe mana."));
        damage = monspell_bluemage_damage((MS_BR_MANA), plev, DAM_ROLL);
		fire_ball(GF_MANA, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BALL_NUKE:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("放射能球を放った。", "You cast a ball of radiation."));
        damage = monspell_bluemage_damage((MS_BALL_NUKE), plev, DAM_ROLL);
		fire_ball(GF_NUKE, dir, damage, 2);
		break;
	case MS_BR_NUKE:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("放射性廃棄物のブレスを吐いた。", "You breathe toxic waste."));
        damage = monspell_bluemage_damage((MS_BR_NUKE), plev, DAM_ROLL);
		fire_ball(GF_NUKE, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BALL_CHAOS:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("純ログルスを放った。", "You invoke a raw Logrus."));
        damage = monspell_bluemage_damage((MS_BALL_CHAOS), plev, DAM_ROLL);
		fire_ball(GF_CHAOS, dir, damage, 4);
		break;
	case MS_BR_DISI:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("分解のブレスを吐いた。", "You breathe disintegration."));
        damage = monspell_bluemage_damage((MS_BR_DISI), plev, DAM_ROLL);
		fire_ball(GF_DISINTEGRATE, dir, damage, (plev > 40 ? -3 : -2));
		break;
	case MS_BALL_ACID:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("アシッド・ボールの呪文を唱えた。", "You cast an acid ball."));
        damage = monspell_bluemage_damage((MS_BALL_ACID), plev, DAM_ROLL);
		fire_ball(GF_ACID, dir, damage, 2);
		break;
	case MS_BALL_ELEC:
        if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("サンダー・ボールの呪文を唱えた。", "You cast a lightning ball."));
        damage = monspell_bluemage_damage((MS_BALL_ELEC), plev, DAM_ROLL);
		fire_ball(GF_ELEC, dir, damage, 2);
		break;
	case MS_BALL_FIRE:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("ファイア・ボールの呪文を唱えた。", "You cast a fire ball."));
        damage = monspell_bluemage_damage((MS_BALL_FIRE), plev, DAM_ROLL);
		fire_ball(GF_FIRE, dir, damage, 2);
		break;
	case MS_BALL_COLD:
        if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("アイス・ボールの呪文を唱えた。", "You cast a frost ball."));
        damage = monspell_bluemage_damage((MS_BALL_COLD), plev, DAM_ROLL);
		fire_ball(GF_COLD, dir, damage, 2);
		break;
	case MS_BALL_POIS:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("悪臭雲の呪文を唱えた。", "You cast a stinking cloud."));
        damage = monspell_bluemage_damage((MS_BALL_POIS), plev, DAM_ROLL);
		fire_ball(GF_POIS, dir, damage, 2);
		break;
	case MS_BALL_NETHER:
        if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("地獄球の呪文を唱えた。", "You cast a nether ball."));
        damage = monspell_bluemage_damage((MS_BALL_NETHER), plev, DAM_ROLL);
		fire_ball(GF_NETHER, dir, damage, 2);
		break;
	case MS_BALL_WATER:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("流れるような身振りをした。", "You gesture fluidly."));
        damage = monspell_bluemage_damage((MS_BALL_WATER), plev, DAM_ROLL);
		fire_ball(GF_WATER, dir, damage, 4);
		break;
	case MS_BALL_MANA:
        if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("魔力の嵐の呪文を念じた。", "You invoke a mana storm."));
        damage = monspell_bluemage_damage((MS_BALL_MANA), plev, DAM_ROLL);
		fire_ball(GF_MANA, dir, damage, 4);
		break;
	case MS_BALL_DARK:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("暗黒の嵐の呪文を念じた。", "You invoke a darkness storm."));
        damage = monspell_bluemage_damage((MS_BALL_DARK), plev, DAM_ROLL);
		fire_ball(GF_DARK, dir, damage, 4);
		break;
	case MS_DRAIN_MANA:
		if (!get_aim_dir(&dir)) return FALSE;

        damage = monspell_bluemage_damage((MS_DRAIN_MANA), plev, DAM_ROLL);
        fire_ball_hide(GF_DRAIN_MANA, dir, damage, 0);
		break;
	case MS_MIND_BLAST:
		if (!get_aim_dir(&dir)) return FALSE;

        damage = monspell_bluemage_damage((MS_MIND_BLAST), plev, DAM_ROLL);
		fire_ball_hide(GF_MIND_BLAST, dir, damage, 0);
		break;
	case MS_BRAIN_SMASH:
        if (!get_aim_dir(&dir)) return FALSE;

        damage = monspell_bluemage_damage((MS_BRAIN_SMASH), plev, DAM_ROLL);
		fire_ball_hide(GF_BRAIN_SMASH, dir, damage, 0);
		break;
	case MS_CAUSE_1:
		if (!get_aim_dir(&dir)) return FALSE;

        damage = monspell_bluemage_damage((MS_CAUSE_1), plev, DAM_ROLL);
		fire_ball_hide(GF_CAUSE_1, dir, damage, 0);
		break;
	case MS_CAUSE_2:
		if (!get_aim_dir(&dir)) return FALSE;

        damage = monspell_bluemage_damage((MS_CAUSE_2), plev, DAM_ROLL);
		fire_ball_hide(GF_CAUSE_2, dir, damage, 0);
		break;
	case MS_CAUSE_3:
		if (!get_aim_dir(&dir)) return FALSE;

        damage = monspell_bluemage_damage((MS_CAUSE_3), plev, DAM_ROLL);
		fire_ball_hide(GF_CAUSE_3, dir, damage, 0);
		break;
	case MS_CAUSE_4:
		if (!get_aim_dir(&dir)) return FALSE;

        damage = monspell_bluemage_damage((MS_CAUSE_4), plev, DAM_ROLL);
		fire_ball_hide(GF_CAUSE_4, dir, damage, 0);
		break;
	case MS_BOLT_ACID:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("アシッド・ボルトの呪文を唱えた。", "You cast an acid bolt."));
        damage = monspell_bluemage_damage((MS_BOLT_ACID), plev, DAM_ROLL);
        fire_bolt(GF_ACID, dir, damage);
		break;
	case MS_BOLT_ELEC:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("サンダー・ボルトの呪文を唱えた。", "You cast a lightning bolt."));
        damage = monspell_bluemage_damage((MS_BOLT_ELEC), plev, DAM_ROLL);
		fire_bolt(GF_ELEC, dir, damage);
		break;
	case MS_BOLT_FIRE:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("ファイア・ボルトの呪文を唱えた。", "You cast a fire bolt."));
        damage = monspell_bluemage_damage((MS_BOLT_FIRE), plev, DAM_ROLL);
		fire_bolt(GF_FIRE, dir, damage);
		break;
	case MS_BOLT_COLD:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("アイス・ボルトの呪文を唱えた。", "You cast a frost bolt."));
        damage = monspell_bluemage_damage((MS_BOLT_COLD), plev, DAM_ROLL);
		fire_bolt(GF_COLD, dir, damage);
		break;
	case MS_STARBURST:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("スターバーストの呪文を念じた。", "You invoke a starburst."));
        damage = monspell_bluemage_damage((MS_STARBURST), plev, DAM_ROLL);
		fire_ball(GF_LITE, dir, damage, 4);
		break;
	case MS_BOLT_NETHER:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("地獄の矢の呪文を唱えた。", "You cast a nether bolt."));
        damage = monspell_bluemage_damage((MS_BOLT_NETHER), plev, DAM_ROLL);
		fire_bolt(GF_NETHER, dir, damage);
		break;
	case MS_BOLT_WATER:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("ウォーター・ボルトの呪文を唱えた。", "You cast a water bolt."));
        damage = monspell_bluemage_damage((MS_BOLT_WATER), plev, DAM_ROLL);
		fire_bolt(GF_WATER, dir, damage);
		break;
	case MS_BOLT_MANA:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("魔力の矢の呪文を唱えた。", "You cast a mana bolt."));
        damage = monspell_bluemage_damage((MS_BOLT_MANA), plev, DAM_ROLL);
		fire_bolt(GF_MANA, dir, damage);
		break;
	case MS_BOLT_PLASMA:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("プラズマ・ボルトの呪文を唱えた。", "You cast a plasma bolt."));
        damage = monspell_bluemage_damage((MS_BOLT_PLASMA), plev, DAM_ROLL);
		fire_bolt(GF_PLASMA, dir, damage);
		break;
	case MS_BOLT_ICE:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("極寒の矢の呪文を唱えた。", "You cast a ice bolt."));
        damage = monspell_bluemage_damage((MS_BOLT_ICE), plev, DAM_ROLL);
		fire_bolt(GF_ICE, dir, damage);
		break;
	case MS_MAGIC_MISSILE:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("マジック・ミサイルの呪文を唱えた。", "You cast a magic missile."));
        damage = monspell_bluemage_damage((MS_MAGIC_MISSILE), plev, DAM_ROLL);
		fire_bolt(GF_MISSILE, dir, damage);
		break;
	case MS_SCARE:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("恐ろしげな幻覚を作り出した。", "You cast a fearful illusion."));
		fear_monster(dir, plev+10);
		break;
	case MS_BLIND:
		if (!get_aim_dir(&dir)) return FALSE;
		confuse_monster(dir, plev * 2);
		break;
	case MS_CONF:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("誘惑的な幻覚をつくり出した。", "You cast a mesmerizing illusion."));
		confuse_monster(dir, plev * 2);
		break;
	case MS_SLOW:
		if (!get_aim_dir(&dir)) return FALSE;
		slow_monster(dir, plev);
		break;
	case MS_SLEEP:
		if (!get_aim_dir(&dir)) return FALSE;
		sleep_monster(dir, plev);
		break;
	case MS_SPEED:
		(void)set_fast(randint1(20 + plev) + plev, FALSE);
		break;
	case MS_HAND_DOOM:
	{
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("<破滅の手>を放った！", "You invoke the Hand of Doom!"));
		fire_ball_hide(GF_HAND_DOOM, dir, plev * 3, 0);
		break;
	}
	case MS_HEAL:
        msg_print(_("自分の傷に念を集中した。", "You concentrate on your wounds!"));
		(void)hp_player(plev*4);
		(void)set_stun(0);
		(void)set_cut(0);
		break;
	case MS_INVULNER:
        msg_print(_("無傷の球の呪文を唱えた。", "You cast a Globe of Invulnerability."));
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
        msg_print(_("「時よ！」", "'Time!'"));
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
                msg_format(_("%sには効果がなかった！", "%s is unaffected!"), m_name);
				break;
			}
			else if (r_ptr->level > randint1(100))
			{
				if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flagsr |= RFR_RES_TELE;
                msg_format(_("%sには耐性がある！", "%s resists!"), m_name);
				break;
			}
		}
        msg_format(_("%sを引き戻した。", "You command %s to return."), m_name);
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
        msg_format(_("%^sの足を指さした。", "You gesture at %^s's feet."), m_name);

		if ((r_ptr->flagsr & (RFR_EFF_RES_NEXU_MASK | RFR_RES_TELE)) ||
			(r_ptr->flags1 & RF1_QUESTOR) || (r_ptr->level + randint1(50) > plev + randint1(60)))
		{
            msg_format(_("しかし効果がなかった！", "%^s is unaffected!"), m_name);
		}
		else teleport_level(target_m_idx);
		break;
	}
	case MS_PSY_SPEAR:
		if (!get_aim_dir(&dir)) return FALSE;

        msg_print(_("光の剣を放った。", "You throw a psycho-spear."));
        damage = monspell_bluemage_damage((MS_PSY_SPEAR), plev, DAM_ROLL);
		(void)fire_beam(GF_PSY_SPEAR, dir, damage);
		break;
	case MS_DARKNESS:

        msg_print(_("暗闇の中で手を振った。", "You gesture in shadow."));
		(void)unlite_area(10, 3);
		break;
	case MS_MAKE_TRAP:
		if (!target_set(TARGET_KILL)) return FALSE;

        msg_print(_("呪文を唱えて邪悪に微笑んだ。", "You cast a spell and cackle evilly."));
		trap_creation(target_row, target_col);
		break;
	case MS_FORGET:
        msg_print(_("しかし何も起きなかった。", "Nothing happen."));
		break;
    case MS_RAISE_DEAD:
        msg_print(_("死者復活の呪文を唱えた。", "You cast a animate dead."));
		(void)animate_dead(0, py, px);
		break;
	case MS_S_KIN:
	{
		int k;

        msg_print(_("援軍を召喚した。", "You summon minions."));
		for (k = 0;k < 1; k++)
		{
			if (summon_kin_player(summon_lev, py, px, (pet ? PM_FORCE_PET : 0L)))
			{
				if (!pet) msg_print(_("召喚された仲間は怒っている！", "Summoned fellows are angry!"));
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

        msg_print(_("サイバーデーモンを召喚した！", "You summon a Cyberdemon!"));
		for (k = 0 ;k < 1 ; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_CYBER, p_mode))
			{
                if (!pet)
                    msg_print(_("召喚されたサイバーデーモンは怒っている！", "The summoned Cyberdemon are angry!"));
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
        msg_print(_("仲間を召喚した。", "You summon help."));
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, 0, p_mode))
			{
                if (!pet)
                    msg_print(_("召喚されたモンスターは怒っている！", "The summoned monster is angry!"));
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
        msg_print(_("モンスターを召喚した！", "You summon monsters!"));
		for (k = 0;k < plev / 15 + 2; k++)
			if(summon_specific((pet ? -1 : 0), py, px, summon_lev, 0, (p_mode | u_mode)))
			{
                if (!pet)
                    msg_print(_("召喚されたモンスターは怒っている！", "The summoned monsters are angry!"));
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
        msg_print(_("アリを召喚した。", "You summon ants."));
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_ANT, (PM_ALLOW_GROUP | p_mode)))
			{
                if (!pet)
                    msg_print(_("召喚されたアリは怒っている！", "The summoned ants are angry!"));
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
        msg_print(_("蜘蛛を召喚した。", "You summon spiders."));
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_SPIDER, (PM_ALLOW_GROUP | p_mode)))
			{
                if (!pet)
                    msg_print(_("召喚された蜘蛛は怒っている！", "Summoned spiders are angry!"));
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
        msg_print(_("ハウンドを召喚した。", "You summon hounds."));
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_HOUND, (PM_ALLOW_GROUP | p_mode)))
			{
                if (!pet)
                    msg_print(_("召喚されたハウンドは怒っている！", "Summoned hounds are angry!"));
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
        msg_print(_("ヒドラを召喚した。", "You summon a hydras."));
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_HYDRA, (g_mode | p_mode)))
			{
                if (!pet)
                    msg_print(_("召喚されたヒドラは怒っている！", "Summoned hydras are angry!"));
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
        msg_print(_("天使を召喚した！", "You summon an angel!"));
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_ANGEL, (g_mode | p_mode)))
			{
                if (!pet)
                    msg_print(_("召喚された天使は怒っている！", "Summoned angels are angry!"));
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
        msg_print(_("混沌の宮廷から悪魔を召喚した！", "You summon a demon from the Courts of Chaos!"));
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_DEMON, (g_mode | p_mode)))
			{
                if (!pet)
                    msg_print(_("召喚されたデーモンは怒っている！", "Summoned demons are angry!"));
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
        msg_print(_("アンデッドの強敵を召喚した！", "You summon an undead adversary!"));
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_UNDEAD, (g_mode | p_mode)))
			{
                if (!pet)
                    msg_print(_("召喚されたアンデッドは怒っている！", "Summoned undeads are angry!"));
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
        msg_print(_("ドラゴンを召喚した！", "You summon a dragon!"));
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_DRAGON, (g_mode | p_mode)))
			{
                if (!pet)
                    msg_print(_("召喚されたドラゴンは怒っている！", "Summoned dragons are angry!"));
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
        msg_print(_("強力なアンデッドを召喚した！", "You summon a greater undead!"));
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_HI_UNDEAD, (g_mode | p_mode | u_mode)))
			{
                if (!pet)
                    msg_print(_("召喚された上級アンデッドは怒っている！", "Summoned greater undeads are angry!"));
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
        msg_print(_("古代ドラゴンを召喚した！", "You summon an ancient dragon!"));
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_HI_DRAGON, (g_mode | p_mode | u_mode)))
			{
                if (!pet)
                    msg_print(_("召喚された古代ドラゴンは怒っている！", "Summoned ancient dragons are angry!"));
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
        msg_print(_("アンバーの王族を召喚した！", "You summon a Lord of Amber!"));
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_AMBERITES, (g_mode | p_mode | u_mode)))
			{
                if (!pet)
                    msg_print(_("召喚されたアンバーの王族は怒っている！", "Summoned Lords of Amber are angry!"));
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
        msg_print(_("特別な強敵を召喚した！", "You summon a special opponent!"));
		for (k = 0;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_UNIQUE, (g_mode | p_mode | PM_ALLOW_UNIQUE)))
			{
				count++;
                if (!pet)
                    msg_print(_("召喚されたユニーク・モンスターは怒っている！", "Summoned special opponents are angry!"));
			}
		for (k = count;k < 1; k++)
			if (summon_specific((pet ? -1 : 0), py, px, summon_lev, SUMMON_HI_UNDEAD, (g_mode | p_mode | PM_ALLOW_UNIQUE)))
			{
				count++;
                if (!pet)
                    msg_print(_("召喚された上級アンデッドは怒っている！", "Summoned greater undeads are angry!"));
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
        msg_print(_("何も現れなかった。", "No one have appeared."));
	}

	return TRUE;
}

/*!
 * @brief 青魔法コマンドのメインルーチン /
 * do_cmd_cast calls this function if the player's class is 'Blue-Mage'.
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
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
		msg_print(_("混乱していて唱えられない！", "You are too confused!"));
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
		msg_print(_("ＭＰが足りません。", "You do not have enough mana to use this power."));

		if (!over_exert) return FALSE;

		/* Verify */
		if (!get_check(_("それでも挑戦しますか? ", "Attempt it anyway? "))) return FALSE;
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
		msg_print(_("魔法をうまく唱えられなかった。", "You failed to concentrate hard enough!"));

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
		msg_print(_("精神を集中しすぎて気を失ってしまった！", "You faint from the effort!"));

		/* Hack -- Bypass free action */
		(void)set_paralyzed(p_ptr->paralyzed + randint1(5 * oops + 1));

		chg_virtue(V_KNOWLEDGE, -10);

		/* Damage CON (possibly permanently) */
		if (randint0(100) < 50)
		{
			bool perm = (randint0(100) < 25);

			/* Message */
			msg_print(_("体を悪くしてしまった！", "You have damaged your health!"));

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

/*!
 * @brief 青魔法のラーニング判定と成功した場合のラーニング処理
 * @param monspell ラーニングを試みるモンスター攻撃のID
 * @return なし
 */
void learn_spell(int monspell)
{
	if (p_ptr->action != ACTION_LEARN) return;
	if (monspell < 0) return; /* Paranoia */
	if (p_ptr->magic_num2[monspell]) return;
	if (p_ptr->confused || p_ptr->blind || p_ptr->image || p_ptr->stun || p_ptr->paralyzed) return;
	if (randint1(p_ptr->lev + 70) > monster_powers[monspell].level + 40)
	{
		p_ptr->magic_num2[monspell] = 1;
		msg_format(_("%sを学習した！", "You have learned %s!"), monster_powers[monspell].name);
		gain_exp(monster_powers[monspell].level * monster_powers[monspell].smana);

		/* Sound */
		sound(SOUND_STUDY);

		new_mane = TRUE;
		p_ptr->redraw |= (PR_STATE);
	}
}


/*!
 * @brief モンスター特殊能力のフラグ配列から特定条件の魔法だけを抜き出す処理
 * Extract monster spells mask for the given mode
 * @param f4 モンスター特殊能力の4番目のフラグ配列
 * @param f5 モンスター特殊能力の5番目のフラグ配列
 * @param f6 モンスター特殊能力の6番目のフラグ配列
 * @param mode 抜き出したい条件
 * @return なし
 */
/*
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
