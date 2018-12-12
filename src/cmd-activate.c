/*!
* @file cmd-activate.c
* @brief プレイヤーの発動コマンド実装
* @date 2018/09/07
* @details
* cmd6.cより分離。
*/

#include "angband.h"
#include "cmd-activate.h"
#include "object-hook.h"
#include "spells-summon.h"

/*!
* @brief ペット入りモンスターボールをソートするための比較関数
* @param u 所持品配列の参照ポインタ
* @param v 未使用
* @param a 所持品ID1
* @param b 所持品ID2
* @return 1の方が大であればTRUE
*/
static bool ang_sort_comp_pet(vptr u, vptr v, int a, int b)
{
	u16b *who = (u16b*)(u);

	int w1 = who[a];
	int w2 = who[b];

	monster_type *m_ptr1 = &m_list[w1];
	monster_type *m_ptr2 = &m_list[w2];
	monster_race *r_ptr1 = &r_info[m_ptr1->r_idx];
	monster_race *r_ptr2 = &r_info[m_ptr2->r_idx];

	/* Unused */
	(void)v;

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


/*!
 * @brief 装備を発動するコマンドのサブルーチン /
 * Activate a wielded object.  Wielded objects never stack.
 * And even if they did, activatable objects never stack.
 * @param item 発動するオブジェクトの所持品ID
 * @return なし
 * @details
 * <pre>
 * Currently, only (some) artifacts, and Dragon Scale Mail, can be activated.
 * But one could, for example, easily make an activatable "Ring of Plasma".
 * Note that it always takes a turn to activate an artifact, even if
 * the user hits "escape" at the "direction" prompt.
 * </pre>
 */
void do_cmd_activate_aux(INVENTORY_IDX item)
{
	DIRECTION dir;
	DEPTH lev;
	int chance, fail;
	object_type *o_ptr;
	bool success;


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

	p_ptr->energy_use = 100;

	/* Extract the item level */
	lev = k_info[o_ptr->k_idx].level;

	/* Hack -- use artifact level instead */
	if (object_is_fixed_artifact(o_ptr)) lev = a_info[o_ptr->name1].level;
	else if (object_is_random_artifact(o_ptr))
	{
		const activation_type* const act_ptr = find_activation_info(o_ptr);
		if (act_ptr) {
			lev = act_ptr->level;
		}
	}
	else if (((o_ptr->tval == TV_RING) || (o_ptr->tval == TV_AMULET)) && o_ptr->name2) lev = e_info[o_ptr->name2].level;

	/* Base chance of success */
	chance = p_ptr->skill_dev;

	/* Confusion hurts skill */
	if (p_ptr->confused) chance = chance / 2;

	fail = lev+5;
	if (chance > fail) fail -= (chance - fail)*2;
	else chance -= (fail - chance)*2;
	if (fail < USE_DEVICE) fail = USE_DEVICE;
	if (chance < USE_DEVICE) chance = USE_DEVICE;

	if (world_player)
	{
		if (flush_failure) flush();
		msg_print(_("止まった時の中ではうまく働かないようだ。", "It shows no reaction."));
		sound(SOUND_FAIL);
		return;
	}

	if (p_ptr->pclass == CLASS_BERSERKER) success = FALSE;
	else if (chance > fail)
	{
		if (randint0(chance*2) < fail) success = FALSE;
		else success = TRUE;
	}
	else
	{
		if (randint0(fail*2) < chance) success = TRUE;
		else success = FALSE;
	}

	/* Roll for usage */
	if (!success)
	{
		if (flush_failure) flush();
		msg_print(_("うまく始動させることができなかった。", "You failed to activate it properly."));
		sound(SOUND_FAIL);
		return;
	}

	/* Check the recharge */
	if (o_ptr->timeout)
	{
		msg_print(_("それは微かに音を立て、輝き、消えた...", "It whines, glows and fades..."));
		return;
	}

	/* Some lights need enough fuel for activation */
	if (!o_ptr->xtra4 && (o_ptr->tval == TV_FLASK) &&
		((o_ptr->sval == SV_LITE_TORCH) || (o_ptr->sval == SV_LITE_LANTERN)))
	{
		msg_print(_("燃料がない。", "It has no fuel."));
		p_ptr->energy_use = 0;
		return;
	}

	/* Activate the artifact */
	msg_print(_("始動させた...", "You activate it..."));

	sound(SOUND_ZAP);

	/* Activate object */
	if (activation_index(o_ptr))
	{
		(void)activate_artifact(o_ptr);

		p_ptr->window |= (PW_INVEN | PW_EQUIP);

		/* Success */
		return;
	}

	/* Special items */
	else if (o_ptr->tval == TV_WHISTLE)
	{
		if (music_singing_any()) stop_singing();
		if (hex_spelling_any()) stop_hex_spell_all();

		{
			IDX pet_ctr, i;
			IDX *who;
			int max_pet = 0;
			u16b dummy_why;

			/* Allocate the "who" array */
			C_MAKE(who, max_m_idx, IDX);

			/* Process the monsters (backwards) */
			for (pet_ctr = m_max - 1; pet_ctr >= 1; pet_ctr--)
			{
				if (is_pet(&m_list[pet_ctr]) && (p_ptr->riding != pet_ctr))
				  who[max_pet++] = pet_ctr;
			}

			/* Select the sort method */
			ang_sort_comp = ang_sort_comp_pet;
			ang_sort_swap = ang_sort_swap_hook;

			ang_sort(who, &dummy_why, max_pet);

			/* Process the monsters (backwards) */
			for (i = 0; i < max_pet; i++)
			{
				pet_ctr = who[i];
				teleport_monster_to(pet_ctr, p_ptr->y, p_ptr->x, 100, TELEPORT_PASSIVE);
			}

			/* Free the "who" array */
			C_KILL(who, max_m_idx, IDX);
		}
		o_ptr->timeout = 100 + randint1(100);
		return;
	}
	else if (o_ptr->tval == TV_CAPTURE)
	{
		if(!o_ptr->pval)
		{
			bool old_target_pet = target_pet;
			target_pet = TRUE;
			if (!get_aim_dir(&dir))
			{
				target_pet = old_target_pet;
				return;
			}
			target_pet = old_target_pet;

			if(fire_ball(GF_CAPTURE, dir, 0, 0))
			{
				o_ptr->pval = (PARAMETER_VALUE)cap_mon;
				o_ptr->xtra3 = (XTRA8)cap_mspeed;
				o_ptr->xtra4 = (XTRA16)cap_hp;
				o_ptr->xtra5 = (XTRA16)cap_maxhp;
				if (cap_nickname)
				{
					cptr t;
					char *s;
					char buf[80] = "";

					if (o_ptr->inscription)
						strcpy(buf, quark_str(o_ptr->inscription));
					s = buf;
					for (s = buf;*s && (*s != '#'); s++)
					{
#ifdef JP
						if (iskanji(*s)) s++;
#endif
					}
					*s = '#';
					s++;
#ifdef JP
 /*nothing*/
#else
					*s++ = '\'';
#endif
					t = quark_str(cap_nickname);
					while (*t)
					{
						*s = *t;
						s++;
						t++;
					}
#ifdef JP
 /*nothing*/
#else
					*s++ = '\'';
#endif
					*s = '\0';
					o_ptr->inscription = quark_add(buf);
				}
			}
		}
		else
		{
			success = FALSE;
			if (!get_direction(&dir, FALSE, FALSE)) return;
			if (monster_can_enter(p_ptr->y + ddy[dir], p_ptr->x + ddx[dir], &r_info[o_ptr->pval], 0))
			{
				if (place_monster_aux(0, p_ptr->y + ddy[dir], p_ptr->x + ddx[dir], o_ptr->pval, (PM_FORCE_PET | PM_NO_KAGE)))
				{
					if (o_ptr->xtra3) m_list[hack_m_idx_ii].mspeed = o_ptr->xtra3;
					if (o_ptr->xtra5) m_list[hack_m_idx_ii].max_maxhp = o_ptr->xtra5;
					if (o_ptr->xtra4) m_list[hack_m_idx_ii].hp = o_ptr->xtra4;
					m_list[hack_m_idx_ii].maxhp = m_list[hack_m_idx_ii].max_maxhp;
					if (o_ptr->inscription)
					{
						char buf[80];
						cptr t;
#ifndef JP
						bool quote = FALSE;
#endif

						t = quark_str(o_ptr->inscription);
						for (t = quark_str(o_ptr->inscription);*t && (*t != '#'); t++)
						{
#ifdef JP
							if (iskanji(*t)) t++;
#endif
						}
						if (*t)
						{
							char *s = buf;
							t++;
#ifdef JP
							/* nothing */
#else
							if (*t =='\'')
							{
								t++;
								quote = TRUE;
							}
#endif
							while(*t)
							{
								*s = *t;
								t++;
								s++;
							}
#ifdef JP
							/* nothing */
#else
							if (quote && *(s-1) =='\'')
								s--;
#endif
							*s = '\0';
							m_list[hack_m_idx_ii].nickname = quark_add(buf);
							t = quark_str(o_ptr->inscription);
							s = buf;
							while(*t && (*t != '#'))
							{
								*s = *t;
								t++;
								s++;
							}
							*s = '\0';
							o_ptr->inscription = quark_add(buf);
						}
					}
					o_ptr->pval = 0;
					o_ptr->xtra3 = 0;
					o_ptr->xtra4 = 0;
					o_ptr->xtra5 = 0;
					success = TRUE;
				}
			}
			if (!success)
				msg_print(_("おっと、解放に失敗した。", "Oops.  You failed to release your pet."));
		}
		calc_android_exp();
		return;
	}

	/* Mistake */
	msg_print(_("おっと、このアイテムは始動できない。", "Oops.  That object cannot be activated."));
}

/*!
 * @brief 装備を発動するコマンドのメインルーチン /
 * @return なし
 */
void do_cmd_activate(void)
{
	OBJECT_IDX item;
	cptr q, s;

	if (p_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN))
	{
		set_action(ACTION_NONE);
	}

	item_tester_no_ryoute = TRUE;
	item_tester_hook = item_tester_hook_activate;

	q = _("どのアイテムを始動させますか? ", "Activate which item? ");
	s = _("始動できるアイテムを装備していない。", "You have nothing to activate.");

	if (!get_item(&item, q, s, (USE_EQUIP))) return;

	/* Activate the item */
	do_cmd_activate_aux(item);
}

/*!
* @brief 発動によるブレスの属性をアイテムの耐性から選択し、実行を処理する。/ Dragon breath activation
* @details 対象となる耐性は dragonbreath_info テーブルを参照のこと。
* @param o_ptr 対象のオブジェクト構造体ポインタ
* @return 発動実行の是非を返す。
*/
static bool activate_dragon_breath(object_type *o_ptr)
{
	BIT_FLAGS flgs[TR_FLAG_SIZE]; /* for resistance flags */
	int type[20];
	cptr name[20];
	int i, t, n = 0;
	DIRECTION dir;

	if (!get_aim_dir(&dir)) return FALSE;

	object_flags(o_ptr, flgs);

	for (i = 0; dragonbreath_info[i].flag != 0; i++)
	{
		if (have_flag(flgs, dragonbreath_info[i].flag))
		{
			type[n] = dragonbreath_info[i].type;
			name[n] = dragonbreath_info[i].name;
			n++;
		}
	}

	/* Paranoia */
	if (n == 0) return FALSE;

	/* Stop speaking */
	if (music_singing_any()) stop_singing();
	if (hex_spelling_any()) stop_hex_spell_all();

	t = randint0(n);
	msg_format(_("あなたは%sのブレスを吐いた。", "You breathe %s."), name[t]);
	fire_breath(type[t], dir, 250, 4);

	return TRUE;
}

/*!
 * @brief アイテムの発動効果を処理する。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 発動実行の是非を返す。
 */
bool activate_artifact(object_type *o_ptr)
{
	PLAYER_LEVEL plev = p_ptr->lev;
	int k, dummy = 0;
	DIRECTION dir;
	cptr name = k_name + k_info[o_ptr->k_idx].name;
	const activation_type* const act_ptr = find_activation_info(o_ptr);

	/* Paranoia */
	if (!act_ptr) {
		/* Maybe forgot adding information to activation_info table ? */
		msg_print("Activation information is not found.");
		return FALSE;
	}

	/* Activate for attack */
	switch (act_ptr->index)
	{
	case ACT_SUNLIGHT:
	{
		if (!get_aim_dir(&dir)) return FALSE;
		msg_print(_("太陽光線が放たれた。", "A line of sunlight appears."));
		(void)lite_line(dir, damroll(6, 8));
		break;
	}

	case ACT_BO_MISS_1:
	{
		msg_print(_("それは眩しいくらいに明るく輝いている...", "It glows extremely brightly..."));
		if (!get_aim_dir(&dir)) return FALSE;
		fire_bolt(GF_MISSILE, dir, damroll(2, 6));
		break;
	}

	case ACT_BA_POIS_1:
	{
		msg_print(_("それは濃緑色に脈動している...", "It throbs deep green..."));
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball(GF_POIS, dir, 12, 3);
		break;
	}

	case ACT_BO_ELEC_1:
	{
		msg_print(_("それは火花に覆われた...", "It is covered in sparks..."));
		if (!get_aim_dir(&dir)) return FALSE;
		fire_bolt(GF_ELEC, dir, damroll(4, 8));
		break;
	}

	case ACT_BO_ACID_1:
	{
		msg_print(_("それは酸に覆われた...", "It is covered in acid..."));
		if (!get_aim_dir(&dir)) return FALSE;
		fire_bolt(GF_ACID, dir, damroll(5, 8));
		break;
	}

	case ACT_BO_COLD_1:
	{
		msg_print(_("それは霜に覆われた...", "It is covered in frost..."));
		if (!get_aim_dir(&dir)) return FALSE;
		fire_bolt(GF_COLD, dir, damroll(6, 8));
		break;
	}

	case ACT_BO_FIRE_1:
	{
		msg_print(_("それは炎に覆われた...", "It is covered in fire..."));
		if (!get_aim_dir(&dir)) return FALSE;
		fire_bolt(GF_FIRE, dir, damroll(9, 8));
		break;
	}

	case ACT_BA_COLD_1:
	{
		msg_print(_("それは霜に覆われた...", "It is covered in frost..."));
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball(GF_COLD, dir, 48, 2);
		break;
	}

	case ACT_BA_COLD_2:
	{
		msg_print(_("それは青く激しく輝いた...", "It glows an intense blue..."));
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball(GF_COLD, dir, 100, 2);
		break;
	}

	case ACT_BA_COLD_3:
	{
		msg_print(_("明るく白色に輝いている...", "It glows bright white..."));
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball(GF_COLD, dir, 400, 3);
		break;
	}

	case ACT_BA_FIRE_1:
	{
		msg_print(_("それは赤く激しく輝いた...", "It glows an intense red..."));
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball(GF_FIRE, dir, 72, 2);
		break;
	}

	case ACT_BA_FIRE_2:
	{
		msg_format(_("%sから炎が吹き出した...", "The %s rages in fire..."), name);
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball(GF_FIRE, dir, 120, 3);
		break;
	}

	case ACT_BA_FIRE_3:
	{
		msg_print(_("深赤色に輝いている...", "It glows deep red..."));
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball(GF_FIRE, dir, 300, 3);
		break;
	}

	case ACT_BA_FIRE_4:
	{
		msg_print(_("それは赤く激しく輝いた...", "It glows an intense red..."));
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball(GF_FIRE, dir, 100, 2);
		break;
	}

	case ACT_BA_ELEC_2:
	{
		msg_print(_("電気がパチパチ音を立てた...", "It crackles with electricity..."));
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball(GF_ELEC, dir, 100, 3);
		break;
	}

	case ACT_BA_ELEC_3:
	{
		msg_print(_("深青色に輝いている...", "It glows deep blue..."));
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball(GF_ELEC, dir, 500, 3);
		break;
	}

	case ACT_BA_ACID_1:
	{
		msg_print(_("それは黒く激しく輝いた...", "It glows an intense black..."));
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball(GF_ACID, dir, 100, 2);
		break;
	}

	case ACT_BA_NUKE_1:
	{
		msg_print(_("それは緑に激しく輝いた...", "It glows an intense green..."));
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball(GF_NUKE, dir, 100, 2);
		break;
	}

	case ACT_HYPODYNAMIA_1:
	{
		msg_format(_("あなたは%sに敵を締め殺すよう命じた。", "You order the %s to strangle your opponent."), name);
		if (!get_aim_dir(&dir)) return FALSE;
		if (hypodynamic_bolt(dir, 100))
			break;
	}

	case ACT_HYPODYNAMIA_2:
	{
		msg_print(_("黒く輝いている...", "It glows black..."));
		if (!get_aim_dir(&dir)) return FALSE;
		hypodynamic_bolt(dir, 120);
		break;
	}

	case ACT_DRAIN_1:
	{
		if (!get_aim_dir(&dir)) return FALSE;
		for (dummy = 0; dummy < 3; dummy++)
		{
			if (hypodynamic_bolt(dir, 50))
				hp_player(50);
		}
		break;
	}

	case ACT_BO_MISS_2:
	{
		msg_print(_("魔法のトゲが現れた...", "It grows magical spikes..."));
		if (!get_aim_dir(&dir)) return FALSE;
		fire_bolt(GF_ARROW, dir, 150);
		break;
	}

	case ACT_WHIRLWIND:
	{
		massacre();
		break;
	}

	case ACT_DRAIN_2:
	{
		if (!get_aim_dir(&dir)) return FALSE;
		for (dummy = 0; dummy < 3; dummy++)
		{
			if (hypodynamic_bolt(dir, 100))
				hp_player(100);
		}
		break;
	}


	case ACT_CALL_CHAOS:
	{
		msg_print(_("様々な色の火花を発している...", "It glows in scintillating colours..."));
		call_chaos();
		break;
	}

	case ACT_ROCKET:
	{
		if (!get_aim_dir(&dir)) return FALSE;
		msg_print(_("ロケットを発射した！", "You launch a rocket!"));
		fire_ball(GF_ROCKET, dir, 250 + plev * 3, 2);
		break;
	}

	case ACT_DISP_EVIL:
	{
		msg_print(_("神聖な雰囲気が充満した...", "It floods the area with goodness..."));
		dispel_evil(p_ptr->lev * 5);
		break;
	}

	case ACT_BA_MISS_3:
	{
		if (!get_aim_dir(&dir)) return FALSE;
		msg_print(_("あなたはエレメントのブレスを吐いた。", "You breathe the elements."));
		fire_breath(GF_MISSILE, dir, 300, 4);
		break;
	}

	case ACT_DISP_GOOD:
	{
		msg_print(_("邪悪な雰囲気が充満した...", "It floods the area with evil..."));
		dispel_good(p_ptr->lev * 5);
		break;
	}

	case ACT_BO_MANA:
	{
		msg_format(_("%sに魔法のトゲが現れた...", "The %s grows magical spikes..."), name);
		if (!get_aim_dir(&dir)) return FALSE;
		fire_bolt(GF_ARROW, dir, 150);
		break;
	}

	case ACT_BA_WATER:
	{
		msg_format(_("%sが深い青色に鼓動している...", "The %s throbs deep blue..."), name);
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball(GF_WATER, dir, 200, 3);
		break;
	}

	case ACT_BA_DARK:
	{
		msg_format(_("%sが深い闇に覆われた...", "The %s is coverd in pitch-darkness..."), name);
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball(GF_DARK, dir, 250, 4);
		break;
	}

	case ACT_BA_MANA:
	{
		msg_format(_("%sが青白く光った．．．", "The %s glows pale..."), name);
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball(GF_MANA, dir, 250, 4);
		break;
	}

	case ACT_PESTICIDE:
	{
		msg_print(_("あなたは害虫を一掃した。", "You exterminate small life."));
		(void)dispel_monsters(4);
		break;
	}

	case ACT_BLINDING_LIGHT:
	{
		msg_format(_("%sが眩しい光で輝いた...", "The %s gleams with blinding light..."), name);
		fire_ball(GF_LITE, 0, 300, 6);
		confuse_monsters(3 * p_ptr->lev / 2);
		break;
	}

	case ACT_BIZARRE:
	{
		msg_format(_("%sは漆黒に輝いた...", "The %s glows intensely black..."), name);
		if (!get_aim_dir(&dir)) return FALSE;
		ring_of_power(dir);
		break;
	}

	case ACT_CAST_BA_STAR:
	{
		HIT_POINT num = damroll(5, 3);
		POSITION y = 0, x = 0;
		int attempts;
		msg_format(_("%sが稲妻で覆われた...", "The %s is surrounded by lightning..."), name);
		for (k = 0; k < num; k++)
		{
			attempts = 1000;

			while (attempts--)
			{
				scatter(&y, &x, p_ptr->y, p_ptr->x, 4, 0);
				if (!cave_have_flag_bold(y, x, FF_PROJECT)) continue;
				if (!player_bold(y, x)) break;
			}

			project(0, 3, y, x, 150, GF_ELEC,
				(PROJECT_THRU | PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL), -1);
		}

		break;
	}

	case ACT_BLADETURNER:
	{
		if (!get_aim_dir(&dir)) return FALSE;
		msg_print(_("あなたはエレメントのブレスを吐いた。", "You breathe the elements."));
		fire_breath(GF_MISSILE, dir, 300, 4);
		msg_print(_("鎧が様々な色に輝いた...", "Your armor glows many colours..."));
		(void)set_afraid(0);
		(void)set_hero(randint1(50) + 50, FALSE);
		(void)hp_player(10);
		(void)set_blessed(randint1(50) + 50, FALSE);
		(void)set_oppose_acid(randint1(50) + 50, FALSE);
		(void)set_oppose_elec(randint1(50) + 50, FALSE);
		(void)set_oppose_fire(randint1(50) + 50, FALSE);
		(void)set_oppose_cold(randint1(50) + 50, FALSE);
		(void)set_oppose_pois(randint1(50) + 50, FALSE);
		break;
	}

	case ACT_BR_FIRE:
	{
		if (!get_aim_dir(&dir)) return FALSE;
		fire_breath(GF_FIRE, dir, 200, 2);
		if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_FLAMES))
		{
			(void)set_oppose_fire(randint1(20) + 20, FALSE);
		}
		break;
	}

	case ACT_BR_COLD:
	{
		if (!get_aim_dir(&dir)) return FALSE;
		fire_breath(GF_COLD, dir, 200, 2);
		if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ICE))
		{
			(void)set_oppose_cold(randint1(20) + 20, FALSE);
		}
		break;
	}

	case ACT_BR_DRAGON:
	{
		if (!activate_dragon_breath(o_ptr)) return FALSE;
		break;
	}

	/* Activate for other offensive action */
	case ACT_CONFUSE:
	{
		msg_print(_("様々な色の火花を発している...", "It glows in scintillating colours..."));
		if (!get_aim_dir(&dir)) return FALSE;
		confuse_monster(dir, 20);
		break;
	}

	case ACT_SLEEP:
	{
		msg_print(_("深青色に輝いている...", "It glows deep blue..."));
		sleep_monsters_touch();
		break;
	}

	case ACT_QUAKE:
	{
		earthquake(p_ptr->y, p_ptr->x, 5);
		break;
	}

	case ACT_TERROR:
	{
		turn_monsters(40 + p_ptr->lev);
		break;
	}

	case ACT_TELE_AWAY:
	{
		if (!get_aim_dir(&dir)) return FALSE;
		(void)fire_beam(GF_AWAY_ALL, dir, plev);
		break;
	}

	case ACT_BANISH_EVIL:
	{
		if (banish_evil(100))
		{
			msg_print(_("アーティファクトの力が邪悪を打ち払った！", "The power of the artifact banishes evil!"));
		}
		break;
	}

	case ACT_GENOCIDE:
	{
		msg_print(_("深青色に輝いている...", "It glows deep blue..."));
		(void)symbol_genocide(200, TRUE);
		break;
	}

	case ACT_MASS_GENO:
	{
		msg_print(_("ひどく鋭い音が流れ出た...", "It lets out a long, shrill note..."));
		(void)mass_genocide(200, TRUE);
		break;
	}

	case ACT_SCARE_AREA:
	{
		if (music_singing_any()) stop_singing();
		if (hex_spelling_any()) stop_hex_spell_all();
		msg_print(_("あなたは力強い突風を吹き鳴らした。周囲の敵が震え上っている!",
			"You wind a mighty blast; your enemies tremble!"));
		(void)turn_monsters((3 * p_ptr->lev / 2) + 10);
		break;
	}

	case ACT_AGGRAVATE:
	{
		if (o_ptr->name1 == ART_HYOUSIGI)
		{
			msg_print(_("拍子木を打った。", "You beat Your wooden clappers."));
		}
		else
		{
			msg_format(_("%sは不快な物音を立てた。", "The %s sounds an unpleasant noise."), name);
		}
		aggravate_monsters(0);
		break;
	}

	/* Activate for summoning / charming */

	case ACT_CHARM_ANIMAL:
	{
		if (!get_aim_dir(&dir)) return FALSE;
		(void)charm_animal(dir, plev);
		break;
	}

	case ACT_CHARM_UNDEAD:
	{
		if (!get_aim_dir(&dir)) return FALSE;
		(void)control_one_undead(dir, plev);
		break;
	}

	case ACT_CHARM_OTHER:
	{
		if (!get_aim_dir(&dir)) return FALSE;
		(void)charm_monster(dir, plev * 2);
		break;
	}

	case ACT_CHARM_ANIMALS:
	{
		(void)charm_animals(plev * 2);
		break;
	}

	case ACT_CHARM_OTHERS:
	{
		charm_monsters(plev * 2);
		break;
	}

	case ACT_SUMMON_ANIMAL:
	{
		(void)summon_specific(-1, p_ptr->y, p_ptr->x, plev, SUMMON_ANIMAL_RANGER, (PM_ALLOW_GROUP | PM_FORCE_PET));
		break;
	}

	case ACT_SUMMON_PHANTOM:
	{
		msg_print(_("幻霊を召喚した。", "You summon a phantasmal servant."));
		(void)summon_specific(-1, p_ptr->y, p_ptr->x, dun_level, SUMMON_PHANTOM, (PM_ALLOW_GROUP | PM_FORCE_PET));
		break;
	}

	case ACT_SUMMON_ELEMENTAL:
	{
		bool pet = one_in_(3);
		BIT_FLAGS mode = 0L;

		if (!(pet && (plev < 50))) mode |= PM_ALLOW_GROUP;
		if (pet) mode |= PM_FORCE_PET;
		else mode |= PM_NO_PET;

		if (summon_specific((pet ? -1 : 0), p_ptr->y, p_ptr->x, ((plev * 3) / 2), SUMMON_ELEMENTAL, mode))
		{
			msg_print(_("エレメンタルが現れた...", "An elemental materializes..."));
			if (pet)
				msg_print(_("あなたに服従しているようだ。", "It seems obedient to you."));
			else
				msg_print(_("それをコントロールできなかった！", "You fail to control it!"));
		}

		break;
	}

	case ACT_SUMMON_DEMON:
	{
		cast_summon_demon((plev * 3) / 2);
		break;
	}

	case ACT_SUMMON_UNDEAD:
	{
		bool pet = one_in_(3);
		int type;
		BIT_FLAGS mode = 0L;

		type = (plev > 47 ? SUMMON_HI_UNDEAD : SUMMON_UNDEAD);

		if (!pet || ((plev > 24) && one_in_(3))) mode |= PM_ALLOW_GROUP;
		if (pet) mode |= PM_FORCE_PET;
		else mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);

		if (summon_specific((pet ? -1 : 0), p_ptr->y, p_ptr->x, ((plev * 3) / 2), type, mode))
		{
			msg_print(_("冷たい風があなたの周りに吹き始めた。それは腐敗臭を運んでいる...",
				"Cold winds begin to blow around you, carrying with them the stench of decay..."));
			if (pet)
				msg_print(_("古えの死せる者共があなたに仕えるため土から甦った！",
					"Ancient, long-dead forms arise from the ground to serve you!"));
			else
				msg_print(_("死者が甦った。眠りを妨げるあなたを罰するために！",
					"'The dead arise... to punish you for disturbing them!'"));
		}

		break;
	}

	case ACT_SUMMON_HOUND:
	{
		BIT_FLAGS mode = PM_ALLOW_GROUP;
		bool pet = !one_in_(5);
		if (pet) mode |= PM_FORCE_PET;
		else mode |= PM_NO_PET;

		if (summon_specific((pet ? -1 : 0), p_ptr->y, p_ptr->x, ((p_ptr->lev * 3) / 2), SUMMON_HOUND, mode))
		{

			if (pet)
				msg_print(_("ハウンドがあなたの下僕として出現した。",
					"A group of hounds appear as your servant."));
			else
				msg_print(_("ハウンドはあなたに牙を向けている！",
					"A group of hounds appear as your enemy!"));
		}

		break;
	}

	case ACT_SUMMON_DAWN:
	{
		msg_print(_("暁の師団を召喚した。", "You summon the Legion of the Dawn."));
		(void)summon_specific(-1, p_ptr->y, p_ptr->x, dun_level, SUMMON_DAWN, (PM_ALLOW_GROUP | PM_FORCE_PET));
		break;
	}

	case ACT_SUMMON_OCTOPUS:
	{
		BIT_FLAGS mode = PM_ALLOW_GROUP;
		bool pet = !one_in_(5);
		if (pet) mode |= PM_FORCE_PET;

		if (summon_named_creature(0, p_ptr->y, p_ptr->x, MON_JIZOTAKO, mode))
		{
			if (pet)
				msg_print(_("蛸があなたの下僕として出現した。", "A group of octopuses appear as your servant."));
			else
				msg_print(_("蛸はあなたを睨んでいる！", "A group of octopuses appear as your enemy!"));
		}

		break;
	}

	/* Activate for healing */

	case ACT_CHOIR_SINGS:
	{
		msg_print(_("天国の歌が聞こえる...", "A heavenly choir sings..."));
		(void)cure_critical_wounds(777);
		(void)set_hero(randint1(25) + 25, FALSE);
		break;
	}

	case ACT_CURE_LW:
	{
		(void)set_afraid(0);
		(void)hp_player(30);
		break;
	}

	case ACT_CURE_MW:
	{
		msg_print(_("深紫色の光を発している...", "It radiates deep purple..."));
		(void)cure_serious_wounds(4, 8);
		break;
	}

	case ACT_CURE_POISON:
	{
		msg_print(_("深青色に輝いている...", "It glows deep blue..."));
		(void)set_afraid(0);
		(void)set_poisoned(0);
		break;
	}

	case ACT_REST_EXP:
	{
		msg_print(_("深紅に輝いている...", "It glows a deep red..."));
		restore_level();
		break;
	}

	case ACT_REST_ALL:
	{
		msg_print(_("濃緑色に輝いている...", "It glows a deep green..."));
		(void)restore_all_status();
		(void)restore_level();
		break;
	}

	case ACT_CURE_700:
	{
		msg_print(_("深青色に輝いている...", "It glows deep blue..."));
		msg_print(_("体内に暖かい鼓動が感じられる...", "You feel a warm tingling inside..."));
		(void)cure_critical_wounds(700);
		break;
	}

	case ACT_CURE_1000:
	{
		msg_print(_("白く明るく輝いている...", "It glows a bright white..."));
		msg_print(_("ひじょうに気分がよい...", "You feel much better..."));
		(void)cure_critical_wounds(1000);
		break;
	}

	case ACT_CURING:
	{
		msg_format(_("%sの優しさに癒される...", "the %s cures you affectionately ..."), name);
		true_healing(0);
		break;
	}

	case ACT_CURE_MANA_FULL:
	{
		msg_format(_("%sが青白く光った．．．", "The %s glows pale..."), name);
		restore_mana(TRUE);
		break;
	}

	/* Activate for timed effect */

	case ACT_ESP:
	{
		(void)set_tim_esp(randint1(30) + 25, FALSE);
		break;
	}

	case ACT_BERSERK:
	{
		(void)berserk(randint1(25) + 25);
		break;
	}

	case ACT_PROT_EVIL:
	{
		msg_format(_("%sから鋭い音が流れ出た...", "The %s lets out a shrill wail..."), name);
		k = 3 * p_ptr->lev;
		(void)set_protevil(randint1(25) + k, FALSE);
		break;
	}

	case ACT_RESIST_ALL:
	{
		msg_print(_("様々な色に輝いている...", "It glows many colours..."));
		(void)set_oppose_acid(randint1(40) + 40, FALSE);
		(void)set_oppose_elec(randint1(40) + 40, FALSE);
		(void)set_oppose_fire(randint1(40) + 40, FALSE);
		(void)set_oppose_cold(randint1(40) + 40, FALSE);
		(void)set_oppose_pois(randint1(40) + 40, FALSE);
		break;
	}

	case ACT_SPEED:
	{
		msg_print(_("明るく緑色に輝いている...", "It glows bright green..."));
		(void)set_fast(randint1(20) + 20, FALSE);
		break;
	}

	case ACT_XTRA_SPEED:
	{
		msg_print(_("明るく輝いている...", "It glows brightly..."));
		(void)set_fast(randint1(75) + 75, FALSE);
		break;
	}

	case ACT_WRAITH:
	{
		set_wraith_form(randint1(plev / 2) + (plev / 2), FALSE);
		break;
	}

	case ACT_INVULN:
	{
		(void)set_invuln(randint1(8) + 8, FALSE);
		break;
	}

	case ACT_HERO:
	{
		(void)heroism(25);
		break;
	}

	case ACT_HERO_SPEED:
	{
		(void)set_fast(randint1(50) + 50, FALSE);
		(void)heroism(50);
		break;
	}

	case ACT_RESIST_ACID:
	{
		msg_format(_("%sが黒く輝いた...", "The %s grows black."), name);
		if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ACID))
		{
			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_ACID, dir, 100, 2);
		}
		(void)set_oppose_acid(randint1(20) + 20, FALSE);
		break;
	}

	case ACT_RESIST_FIRE:
	{
		msg_format(_("%sが赤く輝いた...", "The %s grows red."), name);
		if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_FLAMES))
		{
			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_FIRE, dir, 100, 2);
		}
		(void)set_oppose_fire(randint1(20) + 20, FALSE);
		break;
	}

	case ACT_RESIST_COLD:
	{
		msg_format(_("%sが白く輝いた...", "The %s grows white."), name);
		if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ICE))
		{
			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_COLD, dir, 100, 2);
		}
		(void)set_oppose_cold(randint1(20) + 20, FALSE);
		break;
	}

	case ACT_RESIST_ELEC:
	{
		msg_format(_("%sが青く輝いた...", "The %s grows blue."), name);
		if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ELEC))
		{
			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_ELEC, dir, 100, 2);
		}
		(void)set_oppose_elec(randint1(20) + 20, FALSE);
		break;
	}

	case ACT_RESIST_POIS:
	{
		msg_format(_("%sが緑に輝いた...", "The %s grows green."), name);
		(void)set_oppose_pois(randint1(20) + 20, FALSE);
		break;
	}

	/* Activate for general purpose effect (detection etc.) */

	case ACT_LIGHT:
	{
		msg_format(_("%sから澄んだ光があふれ出た...", "The %s wells with clear light..."), name);
		lite_area(damroll(2, 15), 3);
		break;
	}

	case ACT_MAP_LIGHT:
	{
		msg_print(_("眩しく輝いた...", "It shines brightly..."));
		map_area(DETECT_RAD_MAP);
		lite_area(damroll(2, 15), 3);
		break;
	}

	case ACT_DETECT_ALL:
	{
		msg_print(_("白く明るく輝いている...", "It glows bright white..."));
		msg_print(_("心にイメージが浮かんできた...", "An image forms in your mind..."));
		detect_all(DETECT_RAD_DEFAULT);
		break;
	}

	case ACT_DETECT_XTRA:
	{
		msg_print(_("明るく輝いている...", "It glows brightly..."));
		detect_all(DETECT_RAD_DEFAULT);
		probing();
		identify_fully(FALSE);
		break;
	}

	case ACT_ID_FULL:
	{
		msg_print(_("黄色く輝いている...", "It glows yellow..."));
		identify_fully(FALSE);
		break;
	}

	case ACT_ID_PLAIN:
	{
		if (!ident_spell(FALSE)) return FALSE;
		break;
	}

	case ACT_RUNE_EXPLO:
	{
		msg_print(_("明るい赤色に輝いている...", "It glows bright red..."));
		explosive_rune();
		break;
	}

	case ACT_RUNE_PROT:
	{
		msg_print(_("ブルーに明るく輝いている...", "It glows light blue..."));
		warding_glyph();
		break;
	}

	case ACT_SATIATE:
	{
		(void)set_food(PY_FOOD_MAX - 1);
		break;
	}

	case ACT_DEST_DOOR:
	{
		msg_print(_("明るい赤色に輝いている...", "It glows bright red..."));
		destroy_doors_touch();
		break;
	}

	case ACT_STONE_MUD:
	{
		msg_print(_("鼓動している...", "It pulsates..."));
		if (!get_aim_dir(&dir)) return FALSE;
		wall_to_mud(dir, 20 + randint1(30));
		break;
	}

	case ACT_RECHARGE:
	{
		recharge(130);
		break;
	}

	case ACT_ALCHEMY:
	{
		msg_print(_("明るい黄色に輝いている...", "It glows bright yellow..."));
		(void)alchemy();
		break;
	}

	case ACT_DIM_DOOR:
	{
		msg_print(_("次元の扉が開いた。目的地を選んで下さい。", "You open a dimensional gate. Choose a destination."));
		if (!dimension_door()) return FALSE;
		break;
	}


	case ACT_TELEPORT:
	{
		msg_print(_("周りの空間が歪んでいる...", "It twists space around you..."));
		teleport_player(100, 0L);
		break;
	}

	case ACT_RECALL:
	{
		msg_print(_("やわらかな白色に輝いている...", "It glows soft white..."));
		if (!word_of_recall()) return FALSE;
		break;
	}

	case ACT_JUDGE:
	{
		msg_format(_("%sは赤く明るく光った！", "The %s flashes bright red!"), name);
		chg_virtue(V_KNOWLEDGE, 1);
		chg_virtue(V_ENLIGHTEN, 1);
		wiz_lite(FALSE);

		msg_format(_("%sはあなたの体力を奪った...", "The %s drains your vitality..."), name);
		take_hit(DAMAGE_LOSELIFE, damroll(3, 8), _("審判の宝石", "the Jewel of Judgement"), -1);

		(void)detect_traps(DETECT_RAD_DEFAULT, TRUE);
		(void)detect_doors(DETECT_RAD_DEFAULT);
		(void)detect_stairs(DETECT_RAD_DEFAULT);

		if (get_check(_("帰還の力を使いますか？", "Activate recall? ")))
		{
			(void)word_of_recall();
		}

		break;
	}

	case ACT_TELEKINESIS:
	{
		if (!get_aim_dir(&dir)) return FALSE;
		msg_format(_("%sを伸ばした。", "You stretched your %s."), name);
		fetch(dir, 500, TRUE);
		break;
	}

	case ACT_DETECT_UNIQUE:
	{
		int i;
		monster_type *m_ptr;
		monster_race *r_ptr;
		msg_print(_("奇妙な場所が頭の中に浮かんだ．．．", "Some strange places show up in your mind. And you see ..."));
		/* Process the monsters (backwards) */
		for (i = m_max - 1; i >= 1; i--)
		{
			/* Access the monster */
			m_ptr = &m_list[i];

			/* Ignore "dead" monsters */
			if (!m_ptr->r_idx) continue;

			r_ptr = &r_info[m_ptr->r_idx];

			if (r_ptr->flags1 & RF1_UNIQUE)
			{
				msg_format(_("%s． ", "%s. "), r_name + r_ptr->name);
			}
		}
		break;
	}

	case ACT_ESCAPE:
	{
		switch (randint1(13))
		{
		case 1: case 2: case 3: case 4: case 5:
			teleport_player(10, 0L);
			break;
		case 6: case 7: case 8: case 9: case 10:
			teleport_player(222, 0L);
			break;
		case 11: case 12:
			(void)stair_creation();
			break;
		default:
			if (get_check(_("この階を去りますか？", "Leave this level? ")))
			{
				if (autosave_l) do_cmd_save_game(TRUE);

				/* Leaving */
				p_ptr->leaving = TRUE;
			}
		}
		break;
	}

	case ACT_DISP_CURSE_XTRA:
	{
		msg_format(_("%sが真実を照らし出す...", "The %s exhibits the truth..."), name);
		(void)remove_all_curse();
		(void)probing();
		break;
	}

	case ACT_BRAND_FIRE_BOLTS:
	{
		msg_format(_("%sが深紅に輝いた...", "Your %s glows deep red..."), name);
		(void)brand_bolts();
		break;
	}

	case ACT_RECHARGE_XTRA:
	{
		msg_format(_("%sが白く輝いた．．．", "The %s gleams with blinding light..."), name);
		if (!recharge(1000)) return FALSE;
		break;
	}

	case ACT_LORE:
	{
		msg_print(_("石が隠された秘密を写し出した．．．", "The stone reveals hidden mysteries..."));
		if (!ident_spell(FALSE)) return FALSE;

		if (mp_ptr->spell_book)
		{
			/* Sufficient mana */
			if (20 <= p_ptr->csp)
			{
				/* Use some mana */
				p_ptr->csp -= 20;
			}

			/* Over-exert the player */
			else
			{
				int oops = 20 - p_ptr->csp;

				/* No mana left */
				p_ptr->csp = 0;
				p_ptr->csp_frac = 0;

				msg_print(_("石を制御できない！", "You are too weak to control the stone!"));
				/* Hack -- Bypass free action */
				(void)set_paralyzed(p_ptr->paralyzed + randint1(5 * oops + 1));

				/* Confusing. */
				(void)set_confused(p_ptr->confused + randint1(5 * oops + 1));
			}
			p_ptr->redraw |= (PR_MANA);
		}
		take_hit(DAMAGE_LOSELIFE, damroll(1, 12), _("危険な秘密", "perilous secrets"), -1);
		/* Confusing. */
		if (one_in_(5)) (void)set_confused(p_ptr->confused + randint1(10));

		/* Exercise a little care... */
		if (one_in_(20)) take_hit(DAMAGE_LOSELIFE, damroll(4, 10), _("危険な秘密", "perilous secrets"), -1);
		break;
	}

	case ACT_SHIKOFUMI:
	{
		msg_print(_("力強く四股を踏んだ。", "You stamp. (as if you are in a ring.)"));
		(void)set_afraid(0);
		(void)set_hero(randint1(20) + 20, FALSE);
		dispel_evil(p_ptr->lev * 3);
		break;
	}

	case ACT_PHASE_DOOR:
	{
		teleport_player(10, 0L);
		break;
	}

	case ACT_DETECT_ALL_MONS:
	{
		(void)detect_monsters_invis(255);
		(void)detect_monsters_normal(255);
		break;
	}

	case ACT_ULTIMATE_RESIST:
	{
		TIME_EFFECT v = randint1(25) + 25;
		(void)set_afraid(0);
		(void)set_hero(v, FALSE);
		(void)hp_player(10);
		(void)set_blessed(v, FALSE);
		(void)set_oppose_acid(v, FALSE);
		(void)set_oppose_elec(v, FALSE);
		(void)set_oppose_fire(v, FALSE);
		(void)set_oppose_cold(v, FALSE);
		(void)set_oppose_pois(v, FALSE);
		(void)set_ultimate_res(v, FALSE);
		break;
	}


	/* Unique activation */
	case ACT_CAST_OFF:
	{
		INVENTORY_IDX inv;
		int t;
		OBJECT_IDX o_idx;
		char o_name[MAX_NLEN];
		object_type forge;

		/* Cast off activated item */
		for (inv = INVEN_RARM; inv <= INVEN_FEET; inv++)
		{
			if (o_ptr == &inventory[inv]) break;
		}

		/* Paranoia */
		if (inv > INVEN_FEET) return FALSE;

		object_copy(&forge, o_ptr);
		inven_item_increase(inv, (0 - o_ptr->number));
		inven_item_optimize(inv);
		o_idx = drop_near(&forge, 0, p_ptr->y, p_ptr->x);
		o_ptr = &o_list[o_idx];

		object_desc(o_name, o_ptr, OD_NAME_ONLY);
		msg_format(_("%sを脱ぎ捨てた。", "You cast off %s."), o_name);

		/* Get effects */
		msg_print(_("「燃え上がれ俺の小宇宙！」", "You say, 'Burn up my cosmo!"));
		t = 20 + randint1(20);
		(void)set_blind(p_ptr->blind + t);
		(void)set_afraid(0);
		(void)set_tim_esp(p_ptr->tim_esp + t, FALSE);
		(void)set_tim_regen(p_ptr->tim_regen + t, FALSE);
		(void)set_hero(p_ptr->hero + t, FALSE);
		(void)set_blessed(p_ptr->blessed + t, FALSE);
		(void)set_fast(p_ptr->fast + t, FALSE);
		(void)set_shero(p_ptr->shero + t, FALSE);
		if (p_ptr->pclass == CLASS_FORCETRAINER)
		{
			P_PTR_KI = plev * 5 + 190;
			msg_print(_("気が爆発寸前になった。", "Your force are immediatly before explosion."));
		}

		break;
	}

	case ACT_FALLING_STAR:
	{
		msg_print(_("あなたは妖刀に魅入られた…", "You are enchanted by cursed blade..."));
		msg_print(_("「狂ほしく 血のごとき 月はのぼれり 秘めおきし 魔剣 いずこぞや」", "'Behold the blade arts.'"));
		massacre();
		break;
	}

	case ACT_GRAND_CROSS:
	{
		msg_print(_("「闇に還れ！」", "You say, 'Return to darkness!'"));
		project(0, 8, p_ptr->y, p_ptr->x, (randint1(100) + 200) * 2, GF_HOLY_FIRE, PROJECT_KILL | PROJECT_ITEM | PROJECT_GRID, -1);
		break;
	}

	case ACT_TELEPORT_LEVEL:
	{
		if (!get_check(_("本当に他の階にテレポートしますか？", "Are you sure? (Teleport Level)"))) return FALSE;
		teleport_level(0);
		break;
	}

	case ACT_STRAIN_HASTE:
	{
		int t;
		msg_format(_("%sはあなたの体力を奪った...", "The %s drains your vitality..."), name);
		take_hit(DAMAGE_LOSELIFE, damroll(3, 8), _("加速した疲労", "the strain of haste"), -1);
		t = 25 + randint1(25);
		(void)set_fast(p_ptr->fast + t, FALSE);
		break;
	}

	case ACT_FISHING:
	{
		POSITION x, y;

		if (!get_direction(&dir, FALSE, FALSE)) return FALSE;
		y = p_ptr->y + ddy[dir];
		x = p_ptr->x + ddx[dir];
		p_ptr->fishing_dir = dir;
		if (!cave_have_flag_bold(y, x, FF_WATER))
		{
			msg_print(_("そこは水辺ではない。", "There is no fishing place."));
			return FALSE;
		}
		else if (cave[y][x].m_idx)
		{
			char m_name[80];
			monster_desc(m_name, &m_list[cave[y][x].m_idx], 0);
			msg_format(_("%sが邪魔だ！", "%^s is stand in your way."), m_name);
			p_ptr->energy_use = 0;
			return FALSE;
		}
		set_action(ACTION_FISH);
		p_ptr->redraw |= (PR_STATE);
		break;
	}

	case ACT_INROU:
	{
		int count = 0, i;
		monster_type *m_ptr;
		cptr kakusan = "";

		if (summon_named_creature(0, p_ptr->y, p_ptr->x, MON_SUKE, PM_FORCE_PET))
		{
			msg_print(_("『助さん』が現れた。", "Suke-san apperars."));
			kakusan = "Suke-san";
			count++;
		}
		if (summon_named_creature(0, p_ptr->y, p_ptr->x, MON_KAKU, PM_FORCE_PET))
		{
			msg_print(_("『格さん』が現れた。", "Kaku-san appears."));
			kakusan = "Kaku-san";
			count++;
		}
		if (!count)
		{
			for (i = m_max - 1; i > 0; i--)
			{
				m_ptr = &m_list[i];
				if (!m_ptr->r_idx) continue;
				if (!((m_ptr->r_idx == MON_SUKE) || (m_ptr->r_idx == MON_KAKU))) continue;
				if (!los(m_ptr->fy, m_ptr->fx, p_ptr->y, p_ptr->x)) continue;
				if (!projectable(m_ptr->fy, m_ptr->fx, p_ptr->y, p_ptr->x)) continue;
				count++;
				break;
			}
		}

		if (count)
		{
			msg_format(_("「者ども、ひかえおろう！！！このお方をどなたとこころえる。」",
				"%^s says 'WHO do you think this person is! Bow your head, down your knees!'"), kakusan);
			sukekaku = TRUE;
			stun_monsters(120);
			confuse_monsters(120);
			turn_monsters(120);
			stasis_monsters(120);
			sukekaku = FALSE;
		}
		else
		{
			msg_print(_("しかし、何も起きなかった。", "Nothing happen."));
		}
		break;
	}

	case ACT_MURAMASA:
	{
		/* Only for Muramasa */
		if (o_ptr->name1 != ART_MURAMASA) return FALSE;
		if (get_check(_("本当に使いますか？", "Are you sure?!")))
		{
			msg_print(_("村正が震えた．．．", "The Muramasa pulsates..."));
			do_inc_stat(A_STR);
			if (one_in_(2))
			{
				msg_print(_("村正は壊れた！", "The Muramasa is destroyed!"));
				curse_weapon_object(TRUE, o_ptr);
			}
		}
		break;
	}

	case ACT_BLOODY_MOON:
	{
		/* Only for Bloody Moon */
		if (o_ptr->name1 != ART_BLOOD) return FALSE;
		msg_print(_("鎌が明るく輝いた...", "Your scythe glows brightly!"));
		get_bloody_moon_flags(o_ptr);
		if (p_ptr->prace == RACE_ANDROID) calc_android_exp();
		p_ptr->update |= (PU_BONUS | PU_HP);
		break;
	}

	case ACT_CRIMSON:
	{
		int num = 1;
		int i;
		BIT_FLAGS flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
		int tx, ty;

		/* Only for Crimson */
		if (o_ptr->name1 != ART_CRIMSON) return FALSE;

		msg_print(_("せっかくだから『クリムゾン』をぶっぱなすぜ！", "I'll fire CRIMSON! SEKKAKUDAKARA!"));

		if (!get_aim_dir(&dir)) return FALSE;

		/* Use the given direction */
		tx = p_ptr->x + 99 * ddx[dir];
		ty = p_ptr->y + 99 * ddy[dir];

		/* Hack -- Use an actual "target" */
		if ((dir == 5) && target_okay())
		{
			tx = target_col;
			ty = target_row;
		}

		if (p_ptr->pclass == CLASS_ARCHER)
		{
			/* Extra shot at level 10 */
			if (p_ptr->lev >= 10) num++;

			/* Extra shot at level 30 */
			if (p_ptr->lev >= 30) num++;

			/* Extra shot at level 45 */
			if (p_ptr->lev >= 45) num++;
		}

		for (i = 0; i < num; i++)
			project(0, p_ptr->lev / 20 + 1, ty, tx, p_ptr->lev*p_ptr->lev * 6 / 50, GF_ROCKET, flg, -1);
		break;
	}

	default:
	{
		msg_format(_("Unknown activation effect: %d.", "Unknown activation effect: %d."), act_ptr->index);
		return FALSE;
	}
	}

	/* Set activation timeout */
	if (act_ptr->timeout.constant >= 0) {
		o_ptr->timeout = (s16b)act_ptr->timeout.constant;
		if (act_ptr->timeout.dice > 0) {
			o_ptr->timeout += randint1(act_ptr->timeout.dice);
		}
	}
	else {
		/* Activations that have special timeout */
		switch (act_ptr->index) {
		case ACT_BR_FIRE:
			o_ptr->timeout = ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_FLAMES)) ? 200 : 250;
			break;
		case ACT_BR_COLD:
			o_ptr->timeout = ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ICE)) ? 200 : 250;
			break;
		case ACT_TERROR:
			o_ptr->timeout = 3 * (p_ptr->lev + 10);
			break;
		case ACT_MURAMASA:
			/* Nothing to do */
			break;
		default:
			msg_format("Special timeout is not implemented: %d.", act_ptr->index);
			return FALSE;
		}
	}

	return TRUE;
}

/*!
 * @brief 固定アーティファクト『ブラッディムーン』の特性を変更する。
 * @details スレイ2d2種、及びone_resistance()による耐性1d2種、pval2種を得る。
 * @param o_ptr 対象のオブジェクト構造体（ブラッディムーン）のポインタ
 * @return なし
 */
void get_bloody_moon_flags(object_type *o_ptr)
{
	int dummy, i;

	for (i = 0; i < TR_FLAG_SIZE; i++)
		o_ptr->art_flags[i] = a_info[ART_BLOOD].flags[i];

	dummy = randint1(2) + randint1(2);
	for (i = 0; i < dummy; i++)
	{
		int flag = randint0(26);
		if (flag >= 20) add_flag(o_ptr->art_flags, TR_KILL_UNDEAD + flag - 20);
		else if (flag == 19) add_flag(o_ptr->art_flags, TR_KILL_ANIMAL);
		else if (flag == 18) add_flag(o_ptr->art_flags, TR_SLAY_HUMAN);
		else add_flag(o_ptr->art_flags, TR_CHAOTIC + flag);
	}

	dummy = randint1(2);
	for (i = 0; i < dummy; i++) one_resistance(o_ptr);

	for (i = 0; i < 2; i++)
	{
		int tmp = randint0(11);
		if (tmp < 6) add_flag(o_ptr->art_flags, TR_STR + tmp);
		else add_flag(o_ptr->art_flags, TR_STEALTH + tmp - 6);
	}
}


