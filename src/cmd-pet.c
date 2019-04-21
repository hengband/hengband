#include "angband.h"
#include "util.h"

#include "floor.h"
#include "grid.h"
#include "melee.h"
#include "sort.h"
#include "player-move.h"
#include "player-status.h"
#include "player-effects.h"
#include "object-hook.h"
#include "monster.h"
#include "monster-status.h"
#include "cmd-pet.h"
#include "cmd-basic.h"
#include "view-mainwindow.h"

/*!
* @brief プレイヤーの騎乗/下馬処理判定
* @param g_ptr プレイヤーの移動先マスの構造体参照ポインタ
* @param now_riding TRUEなら下馬処理、FALSEならば騎乗処理
* @return 可能ならばTRUEを返す
*/
bool player_can_ride_aux(grid_type *g_ptr, bool now_riding)
{
	bool p_can_enter;
	bool old_character_xtra = character_xtra;
	MONSTER_IDX old_riding = p_ptr->riding;
	bool old_riding_ryoute = p_ptr->riding_ryoute;
	bool old_old_riding_ryoute = p_ptr->old_riding_ryoute;
	bool old_pf_ryoute = (p_ptr->pet_extra_flags & PF_RYOUTE) ? TRUE : FALSE;

	/* Hack -- prevent "icky" message */
	character_xtra = TRUE;

	if (now_riding) p_ptr->riding = g_ptr->m_idx;
	else
	{
		p_ptr->riding = 0;
		p_ptr->pet_extra_flags &= ~(PF_RYOUTE);
		p_ptr->riding_ryoute = p_ptr->old_riding_ryoute = FALSE;
	}

	p_ptr->update |= PU_BONUS;
	handle_stuff();

	p_can_enter = player_can_enter(g_ptr->feat, CEM_P_CAN_ENTER_PATTERN);

	p_ptr->riding = old_riding;
	if (old_pf_ryoute) p_ptr->pet_extra_flags |= (PF_RYOUTE);
	else p_ptr->pet_extra_flags &= ~(PF_RYOUTE);
	p_ptr->riding_ryoute = old_riding_ryoute;
	p_ptr->old_riding_ryoute = old_old_riding_ryoute;

	p_ptr->update |= PU_BONUS;
	handle_stuff();

	character_xtra = old_character_xtra;

	return p_can_enter;
}


/*!
* @brief ペットの維持コスト計算
* @return 維持コスト(%)
*/
PERCENTAGE calculate_upkeep(void)
{
	MONSTER_IDX m_idx;
	bool have_a_unique = FALSE;
	DEPTH total_friend_levels = 0;

	total_friends = 0;

	for (m_idx = m_max - 1; m_idx >= 1; m_idx--)
	{
		monster_type *m_ptr;
		monster_race *r_ptr;

		m_ptr = &current_floor_ptr->m_list[m_idx];
		if (!monster_is_valid(m_ptr)) continue;
		r_ptr = &r_info[m_ptr->r_idx];

		if (is_pet(m_ptr))
		{
			total_friends++;
			if (r_ptr->flags1 & RF1_UNIQUE)
			{
				if (p_ptr->pclass == CLASS_CAVALRY)
				{
					if (p_ptr->riding == m_idx)
						total_friend_levels += (r_ptr->level + 5) * 2;
					else if (!have_a_unique && (r_info[m_ptr->r_idx].flags7 & RF7_RIDING))
						total_friend_levels += (r_ptr->level + 5) * 7 / 2;
					else
						total_friend_levels += (r_ptr->level + 5) * 10;
					have_a_unique = TRUE;
				}
				else
					total_friend_levels += (r_ptr->level + 5) * 10;
			}
			else
				total_friend_levels += r_ptr->level;

		}
	}

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

/*!
* @brief ペットを開放するコマンドのメインルーチン
* @return なし
*/
void do_cmd_pet_dismiss(void)
{
	monster_type *m_ptr;
	bool all_pets = FALSE;
	MONSTER_IDX pet_ctr;
	int i;
	int Dismissed = 0;

	MONSTER_IDX *who;
	u16b dummy_why;
	int max_pet = 0;
	bool_hack cu, cv;

	cu = Term->scr->cu;
	cv = Term->scr->cv;
	Term->scr->cu = 0;
	Term->scr->cv = 1;

	/* Allocate the "who" array */
	C_MAKE(who, current_floor_ptr->max_m_idx, MONSTER_IDX);

	/* Process the monsters (backwards) */
	for (pet_ctr = m_max - 1; pet_ctr >= 1; pet_ctr--)
	{
		if (is_pet(&current_floor_ptr->m_list[pet_ctr]))
			who[max_pet++] = pet_ctr;
	}

	ang_sort(who, &dummy_why, max_pet, ang_sort_comp_pet_dismiss, ang_sort_swap_hook);

	/* Process the monsters (backwards) */
	for (i = 0; i < max_pet; i++)
	{
		bool delete_this;
		GAME_TEXT friend_name[MAX_NLEN];
		bool kakunin;

		/* Access the monster */
		pet_ctr = who[i];
		m_ptr = &current_floor_ptr->m_list[pet_ctr];

		delete_this = FALSE;
		kakunin = ((pet_ctr == p_ptr->riding) || (m_ptr->nickname));
		monster_desc(friend_name, m_ptr, MD_ASSUME_VISIBLE);

		if (!all_pets)
		{
			/* Hack -- health bar for this monster */
			health_track(pet_ctr);
			handle_stuff();

			msg_format(_("%sを放しますか？ [Yes/No/Unnamed (%d体)]", "Dismiss %s? [Yes/No/Unnamed (%d remain)]"), friend_name, max_pet - i);

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
						msg_format(_("本当によろしいですか？ (%s) ", "Are you sure? (%s) "), friend_name);
						ch = inkey();
						if (ch != 'Y' && ch != 'y')
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
				GAME_TEXT m_name[MAX_NLEN];

				monster_desc(m_name, m_ptr, MD_INDEF_VISIBLE);
				do_cmd_write_nikki(NIKKI_NAMED_PET, RECORD_NAMED_PET_DISMISS, m_name);
			}

			if (pet_ctr == p_ptr->riding)
			{
				msg_format(_("%sから降りた。", "You have got off %s. "), friend_name);

				p_ptr->riding = 0;

				p_ptr->update |= (PU_MONSTERS);
				p_ptr->redraw |= (PR_EXTRA | PR_UHEALTH);
			}

			/* HACK : Add the line to message buffer */
			msg_format(_("%s を放した。", "Dismissed %s."), friend_name);
			p_ptr->update |= (PU_BONUS);
			p_ptr->window |= (PW_MESSAGE);

			delete_monster_idx(pet_ctr);
			Dismissed++;
		}
	}

	Term->scr->cu = cu;
	Term->scr->cv = cv;
	Term_fresh();

	C_KILL(who, current_floor_ptr->max_m_idx, MONSTER_IDX);

#ifdef JP
	msg_format("%d 体のペットを放しました。", Dismissed);
#else
	msg_format("You have dismissed %d pet%s.", Dismissed,
		(Dismissed == 1 ? "" : "s"));
#endif
	if (Dismissed == 0 && all_pets)
		msg_print(_("'U'nnamed は、乗馬以外の名前のないペットだけを全て解放します。", "'U'nnamed means all your pets except named pets and your mount."));

	handle_stuff();
}



/*!
* @brief ペットから騎乗/下馬するコマンドのメインルーチン /
* @param force 強制的に騎乗/下馬するならばTRUE
* @return 騎乗/下馬できたらTRUE
*/
bool do_riding(bool force)
{
	POSITION x, y;
	DIRECTION dir = 0;
	grid_type *g_ptr;
	monster_type *m_ptr;

	if (!get_direction(&dir, FALSE, FALSE)) return FALSE;
	y = p_ptr->y + ddy[dir];
	x = p_ptr->x + ddx[dir];
	g_ptr = &current_floor_ptr->grid_array[y][x];

	if (p_ptr->special_defense & KATA_MUSOU) set_action(ACTION_NONE);

	if (p_ptr->riding)
	{
		/* Skip non-empty grids */
		if (!player_can_ride_aux(g_ptr, FALSE))
		{
			msg_print(_("そちらには降りられません。", "You cannot go to that direction."));
			return FALSE;
		}

		if (!pattern_seq(p_ptr->y, p_ptr->x, y, x)) return FALSE;

		if (g_ptr->m_idx)
		{
			take_turn(p_ptr, 100);

			msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));

			py_attack(y, x, 0);
			return FALSE;
		}

		p_ptr->riding = 0;
		p_ptr->pet_extra_flags &= ~(PF_RYOUTE);
		p_ptr->riding_ryoute = p_ptr->old_riding_ryoute = FALSE;
	}
	else
	{
		if (cmd_limit_confused(p_ptr)) return FALSE;

		m_ptr = &current_floor_ptr->m_list[g_ptr->m_idx];

		if (!g_ptr->m_idx || !m_ptr->ml)
		{
			msg_print(_("その場所にはモンスターはいません。", "Here is no monster."));
			return FALSE;
		}
		if (!is_pet(m_ptr) && !force)
		{
			msg_print(_("そのモンスターはペットではありません。", "That monster is not a pet."));
			return FALSE;
		}
		if (!(r_info[m_ptr->r_idx].flags7 & RF7_RIDING))
		{
			msg_print(_("そのモンスターには乗れなさそうだ。", "This monster doesn't seem suitable for riding."));
			return FALSE;
		}

		if (!pattern_seq(p_ptr->y, p_ptr->x, y, x)) return FALSE;

		if (!player_can_ride_aux(g_ptr, TRUE))
		{
			/* Feature code (applying "mimic" field) */
			feature_type *f_ptr = &f_info[get_feat_mimic(g_ptr)];
#ifdef JP
			msg_format("そのモンスターは%sの%sにいる。", f_name + f_ptr->name,
				((!have_flag(f_ptr->flags, FF_MOVE) && !have_flag(f_ptr->flags, FF_CAN_FLY)) ||
					(!have_flag(f_ptr->flags, FF_LOS) && !have_flag(f_ptr->flags, FF_TREE))) ?
				"中" : "上");
#else
			msg_format("This monster is %s the %s.",
				((!have_flag(f_ptr->flags, FF_MOVE) && !have_flag(f_ptr->flags, FF_CAN_FLY)) ||
					(!have_flag(f_ptr->flags, FF_LOS) && !have_flag(f_ptr->flags, FF_TREE))) ?
				"in" : "on", f_name + f_ptr->name);
#endif

			return FALSE;
		}
		if (r_info[m_ptr->r_idx].level > randint1((p_ptr->skill_exp[GINOU_RIDING] / 50 + p_ptr->lev / 2 + 20)))
		{
			msg_print(_("うまく乗れなかった。", "You failed to ride."));
			take_turn(p_ptr, 100);
			return FALSE;
		}

		if (MON_CSLEEP(m_ptr))
		{
			GAME_TEXT m_name[MAX_NLEN];
			monster_desc(m_name, m_ptr, 0);
			(void)set_monster_csleep(g_ptr->m_idx, 0);
			msg_format(_("%sを起こした。", "You have waked %s up."), m_name);
		}

		if (p_ptr->action == ACTION_KAMAE) set_action(ACTION_NONE);

		p_ptr->riding = g_ptr->m_idx;

		/* Hack -- remove tracked monster */
		if (p_ptr->riding == p_ptr->health_who) health_track(0);
	}

	take_turn(p_ptr, 100);

	/* Mega-Hack -- Forget the view and lite */
	p_ptr->update |= (PU_UN_VIEW | PU_UN_LITE);
	p_ptr->update |= (PU_BONUS);
	p_ptr->redraw |= (PR_MAP | PR_EXTRA);
	p_ptr->redraw |= (PR_UHEALTH);

	(void)move_player_effect(y, x, MPE_HANDLE_STUFF | MPE_ENERGY_USE | MPE_DONT_PICKUP | MPE_DONT_SWAP_MON);

	return TRUE;
}

/*!
* @brief ペットに名前をつけるコマンドのメインルーチン
* @return なし
*/
static void do_name_pet(void)
{
	monster_type *m_ptr;
	char out_val[20];
	GAME_TEXT m_name[MAX_NLEN];
	bool old_name = FALSE;
	bool old_target_pet = target_pet;

	target_pet = TRUE;
	if (!target_set(TARGET_KILL))
	{
		target_pet = old_target_pet;
		return;
	}
	target_pet = old_target_pet;

	if (current_floor_ptr->grid_array[target_row][target_col].m_idx)
	{
		m_ptr = &current_floor_ptr->m_list[current_floor_ptr->grid_array[target_row][target_col].m_idx];

		if (!is_pet(m_ptr))
		{
			msg_print(_("そのモンスターはペットではない。", "This monster is not a pet."));
			return;
		}
		if (r_info[m_ptr->r_idx].flags1 & RF1_UNIQUE)
		{
			msg_print(_("そのモンスターの名前は変えられない！", "You cannot change name of this monster!"));
			return;
		}
		monster_desc(m_name, m_ptr, 0);

		msg_format(_("%sに名前をつける。", "Name %s."), m_name);
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
		if (get_string(_("名前: ", "Name: "), out_val, 15))
		{
			if (out_val[0])
			{
				/* Save the inscription */
				m_ptr->nickname = quark_add(out_val);
				if (record_named_pet)
				{
					monster_desc(m_name, m_ptr, MD_INDEF_VISIBLE);
					do_cmd_write_nikki(NIKKI_NAMED_PET, RECORD_NAMED_PET_NAME, m_name);
				}
			}
			else
			{
				if (record_named_pet && old_name)
				{
					monster_desc(m_name, m_ptr, MD_INDEF_VISIBLE);
					do_cmd_write_nikki(NIKKI_NAMED_PET, RECORD_NAMED_PET_UNNAME, m_name);
				}
				m_ptr->nickname = 0;
			}
		}
	}
}


/*!
* @brief ペットに関するコマンドリストのメインルーチン /
* Issue a pet command
* @return なし
*/
void do_cmd_pet(void)
{
	COMMAND_CODE i = 0;
	int			num;
	int			powers[36];
	concptr			power_desc[36];
	bool			flag, redraw;
	char			choice;
	char			out_val[160];
	int			pet_ctr;
	monster_type	*m_ptr;

	PET_COMMAND_IDX mode = 0;

	char buf[160];
	char target_buf[160];

	int menu_line = use_menu ? 1 : 0;

	num = 0;

	if(p_ptr->wild_mode) return;

	power_desc[num] = _("ペットを放す", "dismiss pets");
	powers[num++] = PET_DISMISS;

#ifdef JP
	sprintf(target_buf, "ペットのターゲットを指定 (現在：%s)",
		(pet_t_m_idx ? (p_ptr->image ? "何か奇妙な物" : (r_name + r_info[current_floor_ptr->m_list[pet_t_m_idx].ap_r_idx].name)) : "指定なし"));
#else
	sprintf(target_buf, "specify a target of pet (now:%s)",
		(pet_t_m_idx ? (p_ptr->image ? "something strange" : (r_name + r_info[current_floor_ptr->m_list[pet_t_m_idx].ap_r_idx].name)) : "nothing"));
#endif
	power_desc[num] = target_buf;
	powers[num++] = PET_TARGET;
	power_desc[num] = _("近くにいろ", "stay close");

	if (p_ptr->pet_follow_distance == PET_CLOSE_DIST) mode = num;
	powers[num++] = PET_STAY_CLOSE;
	power_desc[num] = _("ついて来い", "follow me");

	if (p_ptr->pet_follow_distance == PET_FOLLOW_DIST) mode = num;
	powers[num++] = PET_FOLLOW_ME;
	power_desc[num] = _("敵を見つけて倒せ", "seek and destroy");

	if (p_ptr->pet_follow_distance == PET_DESTROY_DIST) mode = num;
	powers[num++] = PET_SEEK_AND_DESTROY;
	power_desc[num] = _("少し離れていろ", "give me space");

	if (p_ptr->pet_follow_distance == PET_SPACE_DIST) mode = num;
	powers[num++] = PET_ALLOW_SPACE;
	power_desc[num] = _("離れていろ", "stay away");

	if (p_ptr->pet_follow_distance == PET_AWAY_DIST) mode = num;
	powers[num++] = PET_STAY_AWAY;

	if (p_ptr->pet_extra_flags & PF_OPEN_DOORS)
	{
		power_desc[num] = _("ドアを開ける (現在:ON)", "pets open doors (now On)");
	}
	else
	{
		power_desc[num] = _("ドアを開ける (現在:OFF)", "pets open doors (now Off)");
	}
	powers[num++] = PET_OPEN_DOORS;

	if (p_ptr->pet_extra_flags & PF_PICKUP_ITEMS)
	{
		power_desc[num] = _("アイテムを拾う (現在:ON)", "pets pick up items (now On)");
	}
	else
	{
		power_desc[num] = _("アイテムを拾う (現在:OFF)", "pets pick up items (now Off)");
	}
	powers[num++] = PET_TAKE_ITEMS;

	if (p_ptr->pet_extra_flags & PF_TELEPORT)
	{
		power_desc[num] = _("テレポート系魔法を使う (現在:ON)", "allow teleport (now On)");
	}
	else
	{
		power_desc[num] = _("テレポート系魔法を使う (現在:OFF)", "allow teleport (now Off)");
	}
	powers[num++] = PET_TELEPORT;

	if (p_ptr->pet_extra_flags & PF_ATTACK_SPELL)
	{
		power_desc[num] = _("攻撃魔法を使う (現在:ON)", "allow cast attack spell (now On)");
	}
	else
	{
		power_desc[num] = _("攻撃魔法を使う (現在:OFF)", "allow cast attack spell (now Off)");
	}
	powers[num++] = PET_ATTACK_SPELL;

	if (p_ptr->pet_extra_flags & PF_SUMMON_SPELL)
	{
		power_desc[num] = _("召喚魔法を使う (現在:ON)", "allow cast summon spell (now On)");
	}
	else
	{
		power_desc[num] = _("召喚魔法を使う (現在:OFF)", "allow cast summon spell (now Off)");
	}
	powers[num++] = PET_SUMMON_SPELL;

	if (p_ptr->pet_extra_flags & PF_BALL_SPELL)
	{
		power_desc[num] = _("プレイヤーを巻き込む範囲魔法を使う (現在:ON)", "allow involve player in area spell (now On)");
	}
	else
	{
		power_desc[num] = _("プレイヤーを巻き込む範囲魔法を使う (現在:OFF)", "allow involve player in area spell (now Off)");
	}
	powers[num++] = PET_BALL_SPELL;

	if (p_ptr->riding)
	{
		power_desc[num] = _("ペットから降りる", "get off a pet");
	}
	else
	{
		power_desc[num] = _("ペットに乗る", "ride a pet");
	}
	powers[num++] = PET_RIDING;
	power_desc[num] = _("ペットに名前をつける", "name pets");
	powers[num++] = PET_NAME;

	if (p_ptr->riding)
	{
		if ((p_ptr->migite && (empty_hands(FALSE) == EMPTY_HAND_LARM) &&
			object_allow_two_hands_wielding(&inventory[INVEN_RARM])) ||
			(p_ptr->hidarite && (empty_hands(FALSE) == EMPTY_HAND_RARM) &&
				object_allow_two_hands_wielding(&inventory[INVEN_LARM])))
		{
			if (p_ptr->pet_extra_flags & PF_RYOUTE)
			{
				power_desc[num] = _("武器を片手で持つ", "use one hand to control a riding pet");
			}
			else
			{
				power_desc[num] = _("武器を両手で持つ", "use both hands for a weapon");
			}

			powers[num++] = PET_RYOUTE;
		}
		else
		{
			switch (p_ptr->pclass)
			{
			case CLASS_MONK:
			case CLASS_FORCETRAINER:
			case CLASS_BERSERKER:
				if (empty_hands(FALSE) == (EMPTY_HAND_RARM | EMPTY_HAND_LARM))
				{
					if (p_ptr->pet_extra_flags & PF_RYOUTE)
					{
						power_desc[num] = _("片手で格闘する", "use one hand to control a riding pet");
					}
					else
					{
						power_desc[num] = _("両手で格闘する", "use both hands for melee");
					}

					powers[num++] = PET_RYOUTE;
				}
				else if ((empty_hands(FALSE) != EMPTY_HAND_NONE) && !has_melee_weapon(INVEN_RARM) && !has_melee_weapon(INVEN_LARM))
				{
					if (p_ptr->pet_extra_flags & PF_RYOUTE)
					{
						power_desc[num] = _("格闘を行わない", "use one hand to control a riding pet");
					}
					else
					{
						power_desc[num] = _("格闘を行う", "use one hand for melee");
					}

					powers[num++] = PET_RYOUTE;
				}
				break;
			}
		}
	}

	if (!(repeat_pull(&i) && (i >= 0) && (i < num)))
	{
		/* Nothing chosen yet */
		flag = FALSE;

		/* No redraw yet */
		redraw = FALSE;

		if (use_menu)
		{
			screen_save();

			/* Build a prompt */
			strnfmt(out_val, 78, _("(コマンド、ESC=終了) コマンドを選んでください:", "(Command, ESC=exit) Choose command from menu."));
		}
		else
		{
			/* Build a prompt */
			strnfmt(out_val, 78,
				_("(コマンド %c-%c、'*'=一覧、ESC=終了) コマンドを選んでください:", "(Command %c-%c, *=List, ESC=exit) Select a command: "),
				I2A(0), I2A(num - 1));
		}

		choice = (always_show_list || use_menu) ? ESCAPE : 1;

		/* Get a command from the user */
		while (!flag)
		{
			int ask = TRUE;

			if (choice == ESCAPE) choice = ' ';
			else if (!get_com(out_val, &choice, TRUE)) break;

			if (use_menu && (choice != ' '))
			{
				switch (choice)
				{
				case '0':
					screen_load();
					return;

				case '8':
				case 'k':
				case 'K':
					menu_line += (num - 1);
					break;

				case '2':
				case 'j':
				case 'J':
					menu_line++;
					break;

				case '4':
				case 'h':
				case 'H':
					menu_line = 1;
					break;

				case '6':
				case 'l':
				case 'L':
					menu_line = num;
					break;

				case 'x':
				case 'X':
				case '\r':
				case '\n':
					i = menu_line - 1;
					ask = FALSE;
					break;
				}
				if (menu_line > num) menu_line -= num;
			}

			/* Request redraw */
			if ((choice == ' ') || (choice == '*') || (choice == '?') || (use_menu && ask))
			{
				/* Show the list */
				if (!redraw || use_menu)
				{
					byte y = 1, x = 0;
					PET_COMMAND_IDX ctr = 0;
					redraw = TRUE;
					if (!use_menu) screen_save();

					prt("", y++, x);

					/* Print list */
					for (ctr = 0; ctr < num; ctr++)
					{
						/* Letter/number for power selection */
						if (use_menu)
							sprintf(buf, "%c%s ", (ctr == mode) ? '*' : ' ', (ctr == (menu_line - 1)) ? _("》", "> ") : "  ");
						else
							sprintf(buf, "%c%c) ", (ctr == mode) ? '*' : ' ', I2A(ctr));

						strcat(buf, power_desc[ctr]);

						prt(buf, y + ctr, x);
					}

					prt("", y + MIN(ctr, 17), x);
				}

				/* Hide the list */
				else
				{
					/* Hide list */
					redraw = FALSE;
					screen_load();
				}

				/* Redo asking */
				continue;
			}

			if (!use_menu)
			{
				/* Note verify */
				ask = (isupper(choice));

				/* Lowercase */
				if (ask) choice = (char)tolower(choice);

				/* Extract request */
				i = (islower(choice) ? A2I(choice) : -1);
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
				strnfmt(buf, 78, _("%sを使いますか？ ", "Use %s? "), power_desc[i]);

				/* Belay that order */
				if (!get_check(buf)) continue;
			}

			/* Stop the loop */
			flag = TRUE;
		}
		if (redraw) screen_load();

		/* Abort if needed */
		if (!flag)
		{
			free_turn(p_ptr);
			return;
		}

		repeat_push(i);
	}
	switch (powers[i])
	{
	case PET_DISMISS: /* Dismiss pets */
	{
		/* Check pets (backwards) */
		for (pet_ctr = m_max - 1; pet_ctr >= 1; pet_ctr--)
		{
			/* Player has pet */
			if (is_pet(&current_floor_ptr->m_list[pet_ctr])) break;
		}

		if (!pet_ctr)
		{
			msg_print(_("ペットがいない！", "You have no pets!"));
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
			grid_type *g_ptr = &current_floor_ptr->grid_array[target_row][target_col];
			if (g_ptr->m_idx && (current_floor_ptr->m_list[g_ptr->m_idx].ml))
			{
				pet_t_m_idx = current_floor_ptr->grid_array[target_row][target_col].m_idx;
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
				m_ptr = &current_floor_ptr->m_list[pet_ctr];

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
		(void)do_riding(FALSE);
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


/*!
* @brief プレイヤーの落馬判定処理
* @param dam 落馬判定を発した際に受けたダメージ量
* @param force TRUEならば強制的に落馬する
* @return 実際に落馬したらTRUEを返す
*/
bool rakuba(HIT_POINT dam, bool force)
{
	int i, y, x, oy, ox;
	int sn = 0, sy = 0, sx = 0;
	GAME_TEXT m_name[MAX_NLEN];
	monster_type *m_ptr = &current_floor_ptr->m_list[p_ptr->riding];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	bool fall_dam = FALSE;

	if (!p_ptr->riding) return FALSE;
	if (p_ptr->wild_mode) return FALSE;

	if (dam >= 0 || force)
	{
		if (!force)
		{
			int cur = p_ptr->skill_exp[GINOU_RIDING];
			int max = s_info[p_ptr->pclass].s_max[GINOU_RIDING];
			int ridinglevel = r_ptr->level;

			/* 落馬のしやすさ */
			int rakubalevel = r_ptr->level;
			if (p_ptr->riding_ryoute) rakubalevel += 20;

			if ((cur < max) && (max > 1000) &&
				(dam / 2 + ridinglevel) > (cur / 30 + 10))
			{
				int inc = 0;

				if (ridinglevel > (cur / 100 + 15))
					inc += 1 + (ridinglevel - cur / 100 - 15);
				else
					inc += 1;

				p_ptr->skill_exp[GINOU_RIDING] = MIN(max, cur + inc);
			}

			/* レベルの低い乗馬からは落馬しにくい */
			if (randint0(dam / 2 + rakubalevel * 2) < cur / 30 + 10)
			{
				if ((((p_ptr->pclass == CLASS_BEASTMASTER) || (p_ptr->pclass == CLASS_CAVALRY)) && !p_ptr->riding_ryoute) || !one_in_(p_ptr->lev*(p_ptr->riding_ryoute ? 2 : 3) + 30))
				{
					return FALSE;
				}
			}
		}

		/* Check around the player */
		for (i = 0; i < 8; i++)
		{
			grid_type *g_ptr;

			y = p_ptr->y + ddy_ddd[i];
			x = p_ptr->x + ddx_ddd[i];

			g_ptr = &current_floor_ptr->grid_array[y][x];

			if (g_ptr->m_idx) continue;

			/* Skip non-empty grids */
			if (!cave_have_flag_grid(g_ptr, FF_MOVE) && !cave_have_flag_grid(g_ptr, FF_CAN_FLY))
			{
				if (!player_can_ride_aux(g_ptr, FALSE)) continue;
			}

			if (cave_have_flag_grid(g_ptr, FF_PATTERN)) continue;

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
			msg_format(_("%sから振り落とされそうになって、壁にぶつかった。", "You have nearly fallen from %s, but bumped into wall."), m_name);
			take_hit(DAMAGE_NOESCAPE, r_ptr->level + 3, _("壁への衝突", "bumping into wall"), -1);
			return FALSE;
		}

		oy = p_ptr->y;
		ox = p_ptr->x;

		p_ptr->y = sy;
		p_ptr->x = sx;

		/* Redraw the old spot */
		lite_spot(oy, ox);

		/* Redraw the new spot */
		lite_spot(p_ptr->y, p_ptr->x);

		/* Check for new panel */
		verify_panel();
	}

	p_ptr->riding = 0;
	p_ptr->pet_extra_flags &= ~(PF_RYOUTE);
	p_ptr->riding_ryoute = p_ptr->old_riding_ryoute = FALSE;

	p_ptr->update |= (PU_BONUS | PU_VIEW | PU_LITE | PU_FLOW | PU_MON_LITE | PU_MONSTERS);
	handle_stuff();


	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
	p_ptr->redraw |= (PR_EXTRA);

	/* Update health track of mount */
	p_ptr->redraw |= (PR_UHEALTH);

	if (p_ptr->levitation && !force)
	{
		monster_desc(m_name, m_ptr, 0);
		msg_format(_("%sから落ちたが、空中でうまく体勢を立て直して着地した。", "You are thrown from %s, but make a good landing."), m_name);
	}
	else
	{
		take_hit(DAMAGE_NOESCAPE, r_ptr->level + 3, _("落馬", "Falling from riding"), -1);
		fall_dam = TRUE;
	}

	if (sy && !p_ptr->is_dead)
		(void)move_player_effect(p_ptr->y, p_ptr->x, MPE_DONT_PICKUP | MPE_DONT_SWAP_MON);

	return fall_dam;
}

