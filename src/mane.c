/* File: mind.c */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

/* Purpose: Imitation code */

#include "angband.h"


static int damage;

static void mane_info(char *p, int power, int dam)
{
	int plev = p_ptr->lev;
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

	if ((power > 2 && power < 41) || (power > 41 && power < 59) || (power == 75))
		sprintf(p, " %s%d", s_dam, dam);
	else
	{
		switch (power)
		{
			case 41:
				sprintf(p, " %sd%d+%d", s_heal, plev * 3, plev);
				break;
			case 64:
				sprintf(p, " %sd%d+%d", s_dur, 20+plev, plev);
				break;
			case 66:
				sprintf(p, " %s%d", s_heal, plev*6);
				break;
			case 67:
				sprintf(p, " %sd7+7", s_dur);
				break;
			case 68:
				sprintf(p, " %s10", s_range);
				break;
			case 69:
				sprintf(p, " %s%d", s_range, plev * 5);
				break;
			case 79:
				sprintf(p, " %s5", s_range);
				break;
			default:
				break;
		}
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
static int get_mane_power(int *sn, bool baigaesi)
{
	int             i = 0;
	int             num = 0;
	int             y = 1;
	int             x = 18;
	int             minfail = 0;
	int             plev = p_ptr->lev;
	int             chance = 0;
	int             ask;
	char            choice;
	char            out_val[160];
	char            comment[80];
#ifdef JP
cptr            p = "能力";
#else
	cptr            p = "power";
#endif

	monster_power   spell;
	bool            flag, redraw;

	/* Assume cancelled */
	*sn = (-1);

	/* Nothing chosen yet */
	flag = FALSE;

	/* No redraw yet */
	redraw = FALSE;

	num = p_ptr->mane_num;

	/* Build a prompt (accept all spells) */
	(void)strnfmt(out_val, 78, 
#ifdef JP
		      "(%c-%c, '*'で一覧, ESC) どの%sをまねますか？",
#else
		      "(%c-%c, *=List, ESC=exit) Use which %s? ",
#endif
		      I2A(0), I2A(num - 1), p);

	/* Get a spell from the user */

	choice= always_show_list ? ESCAPE:1 ;
	while (!flag)
	{
		if(choice==ESCAPE) choice = ' '; 
		else if( !get_com(out_val, &choice, TRUE) )break; 

		/* Request redraw */
		if ((choice == ' ') || (choice == '*') || (choice == '?'))
		{
			/* Show the list */
			if (!redraw)
			{
				char psi_desc[80];

				/* Show list */
				redraw = TRUE;

				/* Save the screen */
				screen_save();

				/* Display a list of spells */
				prt("", y, x);
#ifdef JP
put_str("名前", y, x + 5);
#else
				put_str("Name", y, x + 5);
#endif

#ifdef JP
put_str("失率 効果", y, x + 36);
#else
				put_str("Fail Info", y, x + 35);
#endif


				/* Dump the spells */
				for (i = 0; i < num; i++)
				{
					/* Access the spell */
					spell = monster_powers[p_ptr->mane_spell[i]];

					chance = spell.manefail;

					/* Reduce failure rate by "effective" level adjustment */
					if (plev > spell.level) chance -= 3 * (plev - spell.level);

					/* Reduce failure rate by INT/WIS adjustment */
					chance -= 3 * (adj_mag_stat[p_ptr->stat_ind[spell.use_stat]] + adj_mag_stat[p_ptr->stat_ind[A_DEX]] - 2) / 2;

					if (spell.manedam) chance = chance * p_ptr->mane_dam[i] / spell.manedam;

					chance += p_ptr->to_m_chance;

					/* Extract the minimum failure rate */
					minfail = adj_mag_fail[p_ptr->stat_ind[spell.use_stat]];

					/* Minimum failure rate */
					if (chance < minfail) chance = minfail;

					/* Stunning makes spells harder */
					if (p_ptr->stun > 50) chance += 25;
					else if (p_ptr->stun) chance += 15;

					/* Always a 5 percent chance of working */
					if (chance > 95) chance = 95;

					/* Get info */
					mane_info(comment, p_ptr->mane_spell[i], (baigaesi ? p_ptr->mane_dam[i]*2 : p_ptr->mane_dam[i]));

					/* Dump the spell --(-- */
					sprintf(psi_desc, "  %c) %-30s %3d%%%s",
						I2A(i), spell.name,
						chance, comment);
					prt(psi_desc, y + i + 1, x);
				}

				/* Clear the bottom line */
				prt("", y + i + 1, x);
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

		/* Note verify */
		ask = isupper(choice);

		/* Lowercase */
		if (ask) choice = tolower(choice);

		/* Extract request */
		i = (islower(choice) ? A2I(choice) : -1);

		/* Totally Illegal */
		if ((i < 0) || (i >= num))
		{
			bell();
			continue;
		}

		/* Save the spell index */
		spell = monster_powers[p_ptr->mane_spell[i]];

		/* Verify it */
		if (ask)
		{
			char tmp_val[160];

			/* Prompt */
#ifdef JP
			(void) strnfmt(tmp_val, 78, "%sをまねますか？", monster_powers[p_ptr->mane_spell[i]].name);
#else
			(void)strnfmt(tmp_val, 78, "Use %s? ", monster_powers[p_ptr->mane_spell[i]].name);
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
	(*sn) = i;

	damage = (baigaesi ? p_ptr->mane_dam[i]*2 : p_ptr->mane_dam[i]);

	/* Success */
	return (TRUE);
}


/*
 * do_cmd_cast calls this function if the player's class
 * is 'imitator'.
 */
static bool use_mane(int spell)
{
	int             dir;
	int             plev = p_ptr->lev;
	u32b mode = (PM_ALLOW_GROUP | PM_FORCE_PET);
	u32b u_mode = 0L;

	if (randint1(50+plev) < plev/10) u_mode = PM_ALLOW_UNIQUE;


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
#ifdef JP
else msg_print("ロケットを発射した。");
#else
			else msg_print("You fire a rocket.");
#endif
		
			fire_rocket(GF_ROCKET, dir, damage, 2);
		break;
	case MS_SHOOT:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("矢を放った。");
#else
			else msg_print("You fire an arrow.");
#endif
		
			fire_bolt(GF_ARROW, dir, damage);
		break;
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
		
			fire_ball(GF_ACID, dir, damage, (plev > 35 ? -3 : -2));
		break;
	case MS_BR_ELEC:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("稲妻のブレスを吐いた。");
#else
			else msg_print("You breathe lightning.");
#endif
		
			fire_ball(GF_ELEC, dir, damage, (plev > 35 ? -3 : -2));
		break;
	case MS_BR_FIRE:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("火炎のブレスを吐いた。");
#else
			else msg_print("You breathe fire.");
#endif
		
			fire_ball(GF_FIRE, dir, damage, (plev > 35 ? -3 : -2));
		break;
	case MS_BR_COLD:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("冷気のブレスを吐いた。");
#else
			else msg_print("You breathe frost.");
#endif
		
			fire_ball(GF_COLD, dir, damage, (plev > 35 ? -3 : -2));
		break;
	case MS_BR_POIS:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("ガスのブレスを吐いた。");
#else
			else msg_print("You breathe gas.");
#endif
		
			fire_ball(GF_POIS, dir, damage, (plev > 35 ? -3 : -2));
		break;
	case MS_BR_NETHER:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("地獄のブレスを吐いた。");
#else
			else msg_print("You breathe nether.");
#endif
		
			fire_ball(GF_NETHER, dir, damage, (plev > 35 ? -3 : -2));
		break;
	case MS_BR_LITE:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("閃光のブレスを吐いた。");
#else
			else msg_print("You breathe light.");
#endif
		
			fire_ball(GF_LITE, dir, damage, (plev > 35 ? -3 : -2));
		break;
	case MS_BR_DARK:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("暗黒のブレスを吐いた。");
#else
			else msg_print("You breathe darkness.");
#endif
		
			fire_ball(GF_DARK, dir, damage, (plev > 35 ? -3 : -2));
		break;
	case MS_BR_CONF:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("混乱のブレスを吐いた。");
#else
			else msg_print("You breathe confusion.");
#endif
		
			fire_ball(GF_CONFUSION, dir, damage, (plev > 35 ? -3 : -2));
		break;
	case MS_BR_SOUND:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("轟音のブレスを吐いた。");
#else
			else msg_print("You breathe sound.");
#endif
		
			fire_ball(GF_SOUND, dir, damage, (plev > 35 ? -3 : -2));
		break;
	case MS_BR_CHAOS:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("カオスのブレスを吐いた。");
#else
			else msg_print("You breathe chaos.");
#endif
		
			fire_ball(GF_CHAOS, dir, damage, (plev > 35 ? -3 : -2));
		break;
	case MS_BR_DISEN:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("劣化のブレスを吐いた。");
#else
			else msg_print("You breathe disenchantment.");
#endif
		
			fire_ball(GF_DISENCHANT, dir, damage, (plev > 35 ? -3 : -2));
		break;
	case MS_BR_NEXUS:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("因果混乱のブレスを吐いた。");
#else
			else msg_print("You breathe nexus.");
#endif
		
			fire_ball(GF_NEXUS, dir, damage, (plev > 35 ? -3 : -2));
		break;
	case MS_BR_TIME:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("時間逆転のブレスを吐いた。");
#else
			else msg_print("You breathe time.");
#endif
		
			fire_ball(GF_TIME, dir, damage, (plev > 35 ? -3 : -2));
		break;
	case MS_BR_INERTIA:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("遅鈍のブレスを吐いた。");
#else
			else msg_print("You breathe inertia.");
#endif
		
			fire_ball(GF_INERTIA, dir, damage, (plev > 35 ? -3 : -2));
		break;
	case MS_BR_GRAVITY:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("重力のブレスを吐いた。");
#else
			else msg_print("You breathe gravity.");
#endif
		
			fire_ball(GF_GRAVITY, dir, damage, (plev > 35 ? -3 : -2));
		break;
	case MS_BR_SHARDS:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("破片のブレスを吐いた。");
#else
			else msg_print("You breathe shards.");
#endif
		
			fire_ball(GF_SHARDS, dir, damage, (plev > 35 ? -3 : -2));
		break;
	case MS_BR_PLASMA:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("プラズマのブレスを吐いた。");
#else
			else msg_print("You breathe plasma.");
#endif
		
			fire_ball(GF_PLASMA, dir, damage, (plev > 35 ? -3 : -2));
		break;
	case MS_BR_FORCE:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("フォースのブレスを吐いた。");
#else
			else msg_print("You breathe force.");
#endif
		
			fire_ball(GF_FORCE, dir, damage, (plev > 35 ? -3 : -2));
		break;
	case MS_BR_MANA:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("魔力のブレスを吐いた。");
#else
			else msg_print("You breathe mana.");
#endif
		
			fire_ball(GF_MANA, dir, damage, (plev > 35 ? -3 : -2));
		break;
	case MS_BALL_NUKE:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("放射能球を放った。");
#else
			else msg_print("You cast a ball of radiation.");
#endif
		
			fire_ball(GF_NUKE, dir, damage, 2);
		break;
	case MS_BR_NUKE:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("放射性廃棄物のブレスを吐いた。");
#else
			else msg_print("You breathe toxic waste.");
#endif
		
			fire_ball(GF_NUKE, dir, damage, (plev > 35 ? -3 : -2));
		break;
	case MS_BALL_CHAOS:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("純ログルスを放った。");
#else
			else msg_print("You invoke a raw Logrus.");
#endif
		
			fire_ball(GF_CHAOS, dir, damage, 4);
		break;
	case MS_BR_DISI:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("分解のブレスを吐いた。");
#else
			else msg_print("You breathe disintegration.");
#endif
		
			fire_ball(GF_DISINTEGRATE, dir, damage, (plev > 35 ? -3 : -2));
		break;
	case MS_BALL_ACID:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("アシッド・ボールの呪文を唱えた。");
#else
			else msg_print("You cast an acid ball.");
#endif
		
			fire_ball(GF_ACID, dir, damage, 2);
		break;
	case MS_BALL_ELEC:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("サンダー・ボールの呪文を唱えた。");
#else
			else msg_print("You cast a lightning ball.");
#endif
		
			fire_ball(GF_ELEC, dir, damage, 2);
		break;
	case MS_BALL_FIRE:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("ファイア・ボールの呪文を唱えた。");
#else
			else msg_print("You cast a fire ball.");
#endif
		
			fire_ball(GF_FIRE, dir, damage, 2);
		break;
	case MS_BALL_COLD:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("アイス・ボールの呪文を唱えた。");
#else
			else msg_print("You cast a frost ball.");
#endif
		
			fire_ball(GF_COLD, dir, damage, 2);
		break;
	case MS_BALL_POIS:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("悪臭雲の呪文を唱えた。");
#else
			else msg_print("You cast a stinking cloud.");
#endif
		
			fire_ball(GF_POIS, dir, damage, 2);
		break;
	case MS_BALL_NETHER:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("地獄球の呪文を唱えた。");
#else
			else msg_print("You cast a nether ball.");
#endif
		
			fire_ball(GF_NETHER, dir, damage, 2);
		break;
	case MS_BALL_WATER:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("流れるような身振りをした。");
#else
			else msg_print("You gesture fluidly.");
#endif
		
			fire_ball(GF_WATER, dir, damage, 4);
		break;
	case MS_BALL_MANA:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("魔力の嵐の呪文を念じた。");
#else
			else msg_print("You invoke a mana storm.");
#endif
		
			fire_ball(GF_MANA, dir, damage, 4);
		break;
	case MS_BALL_DARK:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("暗黒の嵐の呪文を念じた。");
#else
			else msg_print("You invoke a darkness storm.");
#endif
		
			fire_ball(GF_DARK, dir, damage, 4);
		break;
	case MS_DRAIN_MANA:
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball_hide(GF_DRAIN_MANA, dir, randint1(plev*3)+plev, 0);
		break;
	case MS_MIND_BLAST:
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball_hide(GF_MIND_BLAST, dir, damage, 0);
		break;
	case MS_BRAIN_SMASH:
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball_hide(GF_BRAIN_SMASH, dir, damage, 0);
		break;
	case MS_CAUSE_1:
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball_hide(GF_CAUSE_1, dir, damage, 0);
		break;
	case MS_CAUSE_2:
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball_hide(GF_CAUSE_2, dir, damage, 0);
		break;
	case MS_CAUSE_3:
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball_hide(GF_CAUSE_3, dir, damage, 0);
		break;
	case MS_CAUSE_4:
		if (!get_aim_dir(&dir)) return FALSE;
		fire_ball_hide(GF_CAUSE_4, dir, damage, 0);
		break;
	case MS_BOLT_ACID:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("アシッド・ボルトの呪文を唱えた。");
#else
			else msg_print("You cast an acid bolt.");
#endif
		
			fire_bolt(GF_ACID, dir, damage);
		break;
	case MS_BOLT_ELEC:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("サンダー・ボルトの呪文を唱えた。");
#else
			else msg_print("You cast a lightning bolt.");
#endif
		
			fire_bolt(GF_ELEC, dir, damage);
		break;
	case MS_BOLT_FIRE:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("ファイア・ボルトの呪文を唱えた。");
#else
			else msg_print("You cast a fire bolt.");
#endif
		
			fire_bolt(GF_FIRE, dir, damage);
		break;
	case MS_BOLT_COLD:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("アイス・ボルトの呪文を唱えた。");
#else
			else msg_print("You cast a frost bolt.");
#endif
		
			fire_bolt(GF_COLD, dir, damage);
		break;
	case MS_STARBURST:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("スターバーストの呪文を念じた。");
#else
			else msg_print("You invoke a starburst.");
#endif
		
			fire_ball(GF_LITE, dir, damage, 4);
		break;
	case MS_BOLT_NETHER:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("地獄の矢の呪文を唱えた。");
#else
			else msg_print("You cast a nether bolt.");
#endif
		
			fire_bolt(GF_NETHER, dir, damage);
		break;
	case MS_BOLT_WATER:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("ウォーター・ボルトの呪文を唱えた。");
#else
			else msg_print("You cast a water bolt.");
#endif
		
			fire_bolt(GF_WATER, dir, damage);
		break;
	case MS_BOLT_MANA:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("魔力の矢の呪文を唱えた。");
#else
			else msg_print("You cast a mana bolt.");
#endif
		
			fire_bolt(GF_MANA, dir, damage);
		break;
	case MS_BOLT_PLASMA:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("プラズマ・ボルトの呪文を唱えた。");
#else
			else msg_print("You cast a plasma bolt.");
#endif
		
			fire_bolt(GF_PLASMA, dir, damage);
		break;
	case MS_BOLT_ICE:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("極寒の矢の呪文を唱えた。");
#else
			else msg_print("You cast a ice bolt.");
#endif
		
			fire_bolt(GF_ICE, dir, damage);
		break;
	case MS_MAGIC_MISSILE:
		if (!get_aim_dir(&dir)) return FALSE;
#ifdef JP
else msg_print("マジック・ミサイルの呪文を唱えた。");
#else
			else msg_print("You cast a magic missile.");
#endif
		
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

		fire_ball_hide(GF_HAND_DOOM, dir, 200, 0);
		break;
	}
	case MS_HEAL:
#ifdef JP
msg_print("自分の傷に念を集中した。");
#else
			msg_print("You concentrate on your wounds!");
#endif
		(void)hp_player(plev*6);
		(void)set_stun(0);
		(void)set_cut(0);
		break;
	case MS_INVULNER:
#ifdef JP
msg_print("無傷の球の呪文を唱えた。");
#else
			msg_print("You cast a Globe of Invulnerability.");
#endif
		(void)set_invuln(randint1(7) + 7, FALSE);
		break;
	case MS_BLINK:
		teleport_player(10, 0L);
		break;
	case MS_TELEPORT:
		teleport_player(plev * 5, 0L);
		break;
	case MS_WORLD:
		world_player = TRUE;
		if (damage == 1 || damage == 2)
#ifdef JP
			msg_print("「『ザ・ワールド』！時は止まった！」");
#else
			msg_print("You yell 'The World! Time has stopped!'");
#endif
		else if (damage == 3 || damage == 6)
#ifdef JP
			msg_print("「時よ！」");
#else
			msg_print("You yell 'Time!'");
#endif
		else
			msg_print("hek!");
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

		(void)fire_beam(GF_AWAY_ALL, dir, plev);
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
			msg_print("You cast a spell and cackles evilly.");
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
		if (!target_set(TARGET_KILL)) return FALSE;

#ifdef JP
msg_print("援軍を召喚した。");
#else
			msg_print("You summon minions.");
#endif
		for (k = 0;k < 4; k++)
		{
			(void)summon_kin_player(plev, target_row, target_col, (PM_FORCE_PET | PM_ALLOW_GROUP));
		}
		break;
	}
	case MS_S_CYBER:
	{
		int k;
		int max_cyber = (dun_level / 50) + randint1(3);
		if (!target_set(TARGET_KILL)) return FALSE;
#ifdef JP
msg_print("サイバーデーモンを召喚した！");
#else
			msg_print("You summon Cyberdemons!");
#endif
		if (max_cyber > 4) max_cyber = 4;
		for (k = 0;k < max_cyber; k++)
			summon_specific(-1, target_row, target_col, plev, SUMMON_CYBER, mode);
		break;
	}
	case MS_S_MONSTER:
	{
		int k;
		if (!target_set(TARGET_KILL)) return FALSE;
#ifdef JP
msg_print("仲間を召喚した。");
#else
			msg_print("You summon help.");
#endif
		for (k = 0;k < 1; k++)
			summon_specific(-1, target_row, target_col, plev, 0, (mode | u_mode));
		break;
	}
	case MS_S_MONSTERS:
	{
		int k;
		if (!target_set(TARGET_KILL)) return FALSE;
#ifdef JP
msg_print("モンスターを召喚した！");
#else
			msg_print("You summon monsters!");
#endif
		for (k = 0;k < 6; k++)
			summon_specific(-1, target_row, target_col, plev, 0, (mode | u_mode));
		break;
	}
	case MS_S_ANT:
	{
		int k;
		if (!target_set(TARGET_KILL)) return FALSE;
#ifdef JP
msg_print("アリを召喚した。");
#else
			msg_print("You summon ants.");
#endif
		for (k = 0;k < 6; k++)
			summon_specific(-1, target_row, target_col, plev, SUMMON_ANT, mode);
		break;
	}
	case MS_S_SPIDER:
	{
		int k;
		if (!target_set(TARGET_KILL)) return FALSE;
#ifdef JP
msg_print("蜘蛛を召喚した。");
#else
			msg_print("You summon spiders.");
#endif
		for (k = 0;k < 6; k++)
			summon_specific(-1, target_row, target_col, plev, SUMMON_SPIDER, mode);
		break;
	}
	case MS_S_HOUND:
	{
		int k;
		if (!target_set(TARGET_KILL)) return FALSE;
#ifdef JP
msg_print("ハウンドを召喚した。");
#else
			msg_print("You summon hounds.");
#endif
		for (k = 0;k < 4; k++)
			summon_specific(-1, target_row, target_col, plev, SUMMON_HOUND, mode);
		break;
	}
	case MS_S_HYDRA:
	{
		int k;
		if (!target_set(TARGET_KILL)) return FALSE;
#ifdef JP
msg_print("ヒドラを召喚した。");
#else
			msg_print("You summon hydras.");
#endif
		for (k = 0;k < 4; k++)
			summon_specific(-1, target_row, target_col, plev, SUMMON_HYDRA, mode);
		break;
	}
	case MS_S_ANGEL:
	{
		int k;
		if (!target_set(TARGET_KILL)) return FALSE;
#ifdef JP
msg_print("天使を召喚した！");
#else
			msg_print("You summon angel!");
#endif
		for (k = 0;k < 1; k++)
			summon_specific(-1, target_row, target_col, plev, SUMMON_ANGEL, mode);
		break;
	}
	case MS_S_DEMON:
	{
		int k;
		if (!target_set(TARGET_KILL)) return FALSE;
#ifdef JP
msg_print("混沌の宮廷から悪魔を召喚した！");
#else
			msg_print("You summon a demon from the Courts of Chaos!");
#endif
		for (k = 0;k < 1; k++)
			summon_specific(-1, target_row, target_col, plev, SUMMON_DEMON, (mode | u_mode));
		break;
	}
	case MS_S_UNDEAD:
	{
		int k;
		if (!target_set(TARGET_KILL)) return FALSE;
#ifdef JP
msg_print("アンデッドの強敵を召喚した！");
#else
			msg_print("You summon an undead adversary!");
#endif
		for (k = 0;k < 1; k++)
			summon_specific(-1, target_row, target_col, plev, SUMMON_UNDEAD, (mode | u_mode));
		break;
	}
	case MS_S_DRAGON:
	{
		int k;
		if (!target_set(TARGET_KILL)) return FALSE;
#ifdef JP
msg_print("ドラゴンを召喚した！");
#else
			msg_print("You summon dragon!");
#endif
		for (k = 0;k < 1; k++)
			summon_specific(-1, target_row, target_col, plev, SUMMON_DRAGON, (mode | u_mode));
		break;
	}
	case MS_S_HI_UNDEAD:
	{
		int k;
		if (!target_set(TARGET_KILL)) return FALSE;
#ifdef JP
msg_print("強力なアンデッドを召喚した！");
#else
			msg_print("You summon greater undead!");
#endif
		for (k = 0;k < 6; k++)
			summon_specific(-1, target_row, target_col, plev, SUMMON_HI_UNDEAD, (mode | u_mode));
		break;
	}
	case MS_S_HI_DRAGON:
	{
		int k;
		if (!target_set(TARGET_KILL)) return FALSE;
#ifdef JP
msg_print("古代ドラゴンを召喚した！");
#else
			msg_print("You summon ancient dragons!");
#endif
		for (k = 0;k < 4; k++)
			summon_specific(-1, target_row, target_col, plev, SUMMON_HI_DRAGON, (mode | u_mode));
		break;
	}
	case MS_S_AMBERITE:
	{
		int k;
		if (!target_set(TARGET_KILL)) return FALSE;
#ifdef JP
msg_print("アンバーの王族を召喚した！");
#else
			msg_print("You summon Lords of Amber!");
#endif
		for (k = 0;k < 4; k++)
			summon_specific(-1, target_row, target_col, plev, SUMMON_AMBERITES, (mode | PM_ALLOW_UNIQUE));
		break;
	}
	case MS_S_UNIQUE:
	{
		int k, count = 0;
		if (!target_set(TARGET_KILL)) return FALSE;
#ifdef JP
msg_print("特別な強敵を召喚した！");
#else
			msg_print("You summon special opponents!");
#endif
		for (k = 0;k < 4; k++)
			if (summon_specific(-1, target_row, target_col, plev, SUMMON_UNIQUE, (mode | PM_ALLOW_UNIQUE))) count++;
		for (k = count;k < 4; k++)
			summon_specific(-1, target_row, target_col, plev, SUMMON_HI_UNDEAD, (mode | u_mode));
		break;
	}
	default:
		msg_print("hoge?");
	}

	return TRUE;
}


/*
 * do_cmd_cast calls this function if the player's class
 * is 'imitator'.
 */
bool do_cmd_mane(bool baigaesi)
{
	int             n = 0, j;
	int             chance;
	int             minfail = 0;
	int             plev = p_ptr->lev;
	monster_power   spell;
	bool            cast;


	/* not if confused */
	if (p_ptr->confused)
	{
#ifdef JP
msg_print("混乱していて集中できない！");
#else
		msg_print("You are too confused!");
#endif

		return TRUE;
	}

	if (!p_ptr->mane_num)
	{
#ifdef JP
msg_print("まねられるものが何もない！");
#else
		msg_print("You don't remember any action!");
#endif

		return FALSE;
	}

	/* get power */
	if (!get_mane_power(&n, baigaesi)) return FALSE;

	spell = monster_powers[p_ptr->mane_spell[n]];

	/* Spell failure chance */
	chance = spell.manefail;

	/* Reduce failure rate by "effective" level adjustment */
	if (plev > spell.level) chance -= 3 * (plev - spell.level);

	/* Reduce failure rate by 1 stat and DEX adjustment */
	chance -= 3 * (adj_mag_stat[p_ptr->stat_ind[spell.use_stat]] + adj_mag_stat[p_ptr->stat_ind[A_DEX]] - 2) / 2;

	if (spell.manedam) chance = chance * damage / spell.manedam;

	chance += p_ptr->to_m_chance;

	/* Extract the minimum failure rate */
	minfail = adj_mag_fail[p_ptr->stat_ind[spell.use_stat]];

	/* Minimum failure rate */
	if (chance < minfail) chance = minfail;

	/* Stunning makes spells harder */
	if (p_ptr->stun > 50) chance += 25;
	else if (p_ptr->stun) chance += 15;

	/* Always a 5 percent chance of working */
	if (chance > 95) chance = 95;

	/* Failed spell */
	if (randint0(100) < chance)
	{
		if (flush_failure) flush();
#ifdef JP
msg_print("ものまねに失敗した！");
#else
		msg_print("You failed to concentrate hard enough!");
#endif

		sound(SOUND_FAIL);
	}
	else
	{
		sound(SOUND_ZAP);

		/* Cast the spell */
		cast = use_mane(p_ptr->mane_spell[n]);

		if (!cast) return FALSE;
	}

	p_ptr->mane_num--;
	for (j = n; j < p_ptr->mane_num;j++)
	{
		p_ptr->mane_spell[j] = p_ptr->mane_spell[j+1];
		p_ptr->mane_dam[j] = p_ptr->mane_dam[j+1];
	}

	/* Take a turn */
	energy_use = 100;

	/* Window stuff */
	p_ptr->redraw |= (PR_IMITATION);
	p_ptr->window |= (PW_PLAYER);
	p_ptr->window |= (PW_SPELL);

	return TRUE;
}
