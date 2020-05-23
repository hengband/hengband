/*!
 * @file melee1.c
 * @brief 打撃処理 / Melee process.
 * @date 2014/01/17
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen.\n
 * @details
 */

#include "system/angband.h"
#include "util/util.h"
#include "main/sound-definitions-table.h"
#include "cmd-pet.h"
#include "monster/monsterrace-hook.h"
#include "melee.h"
#include "monster/monster.h"
#include "monster/monster-status.h"
#include "mspell/monster-spell.h"
#include "realm/realm-hex.h"
#include "realm/realm-song.h"
#include "grid/grid.h"
#include "dungeon/dungeon.h"
#include "effect/effect-characteristics.h"
#include "spell/process-effect.h"
#include "spell/spells-type.h"
#include "player/player-move.h"
#include "player/player-status.h"
#include "player/player-class.h"
#include "player/player-personalities-table.h"
#include "spell/spells-floor.h"
#include "spell/spells3.h"
#include "player/player-races-table.h"
#include "combat/hallucination-attacks-table.h"
#include "combat/monster-attack-types.h"
#include "combat/monster-attack-effect.h"
#include "combat/attack-accuracy.h"
#include "mind/samurai-slaying.h"

#define BLOW_EFFECT_TYPE_NONE  0
#define BLOW_EFFECT_TYPE_FEAR  1
#define BLOW_EFFECT_TYPE_SLEEP 2
#define BLOW_EFFECT_TYPE_HEAL  3

/*!
 * @brief モンスターから敵モンスターへの打撃攻撃処理
 * @param m_idx 攻撃側モンスターの参照ID
 * @param t_idx 目標側モンスターの参照ID
 * @return 実際に打撃処理が行われた場合TRUEを返す
 */
bool monst_attack_monst(player_type *subject_ptr, MONSTER_IDX m_idx, MONSTER_IDX t_idx)
{
	monster_type *m_ptr = &subject_ptr->current_floor_ptr->m_list[m_idx];
	monster_type *t_ptr = &subject_ptr->current_floor_ptr->m_list[t_idx];

	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_race *tr_ptr = &r_info[t_ptr->r_idx];

	int pt;
	GAME_TEXT m_name[MAX_NLEN], t_name[MAX_NLEN];
	char temp[MAX_NLEN];
	bool explode = FALSE, touched = FALSE, fear = FALSE, dead = FALSE;
	POSITION y_saver = t_ptr->fy;
	POSITION x_saver = t_ptr->fx;
	int effect_type;

	bool see_m = is_seen(m_ptr);
	bool see_t = is_seen(t_ptr);
	bool see_either = see_m || see_t;

	/* Can the player be aware of this attack? */
	bool known = (m_ptr->cdis <= MAX_SIGHT) || (t_ptr->cdis <= MAX_SIGHT);
	bool do_silly_attack = (one_in_(2) && subject_ptr->image);

	if (m_idx == t_idx) return FALSE;
	if (r_ptr->flags1 & RF1_NEVER_BLOW) return FALSE;
	if (d_info[subject_ptr->dungeon_idx].flags1 & DF1_NO_MELEE) return FALSE;

	/* Total armor */
	ARMOUR_CLASS ac = tr_ptr->ac;

	/* Extract the effective monster level */
	DEPTH rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);

	monster_desc(subject_ptr, m_name, m_ptr, 0);
	monster_desc(subject_ptr, t_name, t_ptr, 0);

	/* Assume no blink */
	bool blinked = FALSE;

	if (!see_either && known)
	{
		subject_ptr->current_floor_ptr->monster_noise = TRUE;
	}

	if (subject_ptr->riding && (m_idx == subject_ptr->riding)) disturb(subject_ptr, TRUE, TRUE);

	/* Scan through all four blows */
	for (ARMOUR_CLASS ap_cnt = 0; ap_cnt < 4; ap_cnt++)
	{
		bool obvious = FALSE;

		HIT_POINT power = 0;
		HIT_POINT damage = 0;

		concptr act = NULL;

		/* Extract the attack infomation */
		int effect = r_ptr->blow[ap_cnt].effect;
		rbm_type method = r_ptr->blow[ap_cnt].method;
		int d_dice = r_ptr->blow[ap_cnt].d_dice;
		int d_side = r_ptr->blow[ap_cnt].d_side;

		if (!monster_is_valid(m_ptr)) break;

		/* Stop attacking if the target dies! */
		if (t_ptr->fx != x_saver || t_ptr->fy != y_saver)
			break;

		/* Hack -- no more attacks */
		if (!method) break;

		if (method == RBM_SHOOT) continue;

		/* Extract the attack "power" */
		power = mbe_info[effect].power;

		/* Monster hits */
		if (!effect || check_hit_from_monster_to_monster(power, rlev, ac, MON_STUNNED(m_ptr)))
		{
			(void)set_monster_csleep(subject_ptr, t_idx, 0);

			if (t_ptr->ml)
			{
				/* Redraw the health bar */
				if (subject_ptr->health_who == t_idx) subject_ptr->redraw |= (PR_HEALTH);
				if (subject_ptr->riding == t_idx) subject_ptr->redraw |= (PR_UHEALTH);
			}

			/* Describe the attack method */
			switch (method)
			{
			case RBM_HIT:
			{
				act = _("%sを殴った。", "hits %s.");
				touched = TRUE;
				break;
			}

			case RBM_TOUCH:
			{
				act = _("%sを触った。", "touches %s.");
				touched = TRUE;
				break;
			}

			case RBM_PUNCH:
			{
				act = _("%sをパンチした。", "punches %s.");
				touched = TRUE;
				break;
			}

			case RBM_KICK:
			{
				act = _("%sを蹴った。", "kicks %s.");
				touched = TRUE;
				break;
			}

			case RBM_CLAW:
			{
				act = _("%sをひっかいた。", "claws %s.");
				touched = TRUE;
				break;
			}

			case RBM_BITE:
			{
				act = _("%sを噛んだ。", "bites %s.");
				touched = TRUE;
				break;
			}

			case RBM_STING:
			{
				act = _("%sを刺した。", "stings %s.");
				touched = TRUE;
				break;
			}

			case RBM_SLASH:
			{
				act = _("%sを斬った。", "slashes %s.");
				break;
			}

			case RBM_BUTT:
			{
				act = _("%sを角で突いた。", "butts %s.");
				touched = TRUE;
				break;
			}

			case RBM_CRUSH:
			{
				act = _("%sに体当りした。", "crushes %s.");
				touched = TRUE;
				break;
			}

			case RBM_ENGULF:
			{
				act = _("%sを飲み込んだ。", "engulfs %s.");
				touched = TRUE;
				break;
			}

			case RBM_CHARGE:
			{
				act = _("%sに請求書をよこした。", "charges %s.");
				touched = TRUE;
				break;
			}

			case RBM_CRAWL:
			{
				act = _("%sの体の上を這い回った。", "crawls on %s.");
				touched = TRUE;
				break;
			}

			case RBM_DROOL:
			{
				act = _("%sによだれをたらした。", "drools on %s.");
				touched = FALSE;
				break;
			}

			case RBM_SPIT:
			{
				act = _("%sに唾を吐いた。", "spits on %s.");
				touched = FALSE;
				break;
			}

			case RBM_EXPLODE:
			{
				if (see_either) disturb(subject_ptr, TRUE, TRUE);
				act = _("爆発した。", "explodes.");
				explode = TRUE;
				touched = FALSE;
				break;
			}

			case RBM_GAZE:
			{
				act = _("%sをにらんだ。", "gazes at %s.");
				touched = FALSE;
				break;
			}

			case RBM_WAIL:
			{
				act = _("%sに泣きついた。", "wails at %s.");
				touched = FALSE;
				break;
			}

			case RBM_SPORE:
			{
				act = _("%sに胞子を飛ばした。", "releases spores at %s.");
				touched = FALSE;
				break;
			}

			case RBM_XXX4:
			{
				act = _("%sにXXX4を飛ばした。", "projects XXX4's at %s.");
				touched = FALSE;
				break;
			}

			case RBM_BEG:
			{
				act = _("%sに金をせがんだ。", "begs %s for money.");
				touched = FALSE;
				break;
			}

			case RBM_INSULT:
			{
				act = _("%sを侮辱した。", "insults %s.");
				touched = FALSE;
				break;
			}

			case RBM_MOAN:
			{
				act = _("%sにむかってうめいた。", "moans at %s.");
				touched = FALSE;
				break;
			}

			case RBM_SHOW:
			{
				act = _("%sにむかって歌った。", "sings to %s.");
				touched = FALSE;
				break;
			}
			}

			if (act && see_either)
			{
#ifdef JP
				if (do_silly_attack) act = silly_attacks2[randint0(MAX_SILLY_ATTACK)];
				strfmt(temp, act, t_name);
				msg_format("%^sは%s", m_name, temp);
#else
				if (do_silly_attack)
				{
					act = silly_attacks[randint0(MAX_SILLY_ATTACK)];
					strfmt(temp, "%s %s.", act, t_name);
				}
				else strfmt(temp, act, t_name);
				msg_format("%^s %s", m_name, temp);
#endif
			}

			/* Hack -- assume all attacks are obvious */
			obvious = TRUE;

			/* Roll out the damage */
			damage = damroll(d_dice, d_side);

			/* Assume no effect */
			effect_type = BLOW_EFFECT_TYPE_NONE;

			pt = GF_MISSILE;

			/* Apply appropriate damage */
			switch (effect)
			{
			case 0:
			case RBE_DR_MANA:
				damage = pt = 0;
				break;

			case RBE_SUPERHURT:
				if ((randint1(rlev * 2 + 250) > (ac + 200)) || one_in_(13))
				{
					int tmp_damage = damage - (damage * ((ac < 150) ? ac : 150) / 250);
					damage = MAX(damage, tmp_damage * 2);
					break;
				}

				/* Fall through */

			case RBE_HURT:
				damage -= (damage * ((ac < 150) ? ac : 150) / 250);
				break;

			case RBE_POISON:
			case RBE_DISEASE:
				pt = GF_POIS;
				break;

			case RBE_UN_BONUS:
			case RBE_UN_POWER:
				pt = GF_DISENCHANT;
				break;

			case RBE_EAT_ITEM:
			case RBE_EAT_GOLD:
				if ((subject_ptr->riding != m_idx) && one_in_(2)) blinked = TRUE;
				break;

			case RBE_EAT_FOOD:
			case RBE_EAT_LITE:
			case RBE_BLIND:
			case RBE_LOSE_STR:
			case RBE_LOSE_INT:
			case RBE_LOSE_WIS:
			case RBE_LOSE_DEX:
			case RBE_LOSE_CON:
			case RBE_LOSE_CHR:
			case RBE_LOSE_ALL:
				break;

			case RBE_ACID:
				pt = GF_ACID;
				break;

			case RBE_ELEC:
				pt = GF_ELEC;
				break;

			case RBE_FIRE:
				pt = GF_FIRE;
				break;

			case RBE_COLD:
				pt = GF_COLD;
				break;

			case RBE_CONFUSE:
				pt = GF_CONFUSION;
				break;

			case RBE_TERRIFY:
				effect_type = BLOW_EFFECT_TYPE_FEAR;
				break;

			case RBE_PARALYZE:
				effect_type = BLOW_EFFECT_TYPE_SLEEP;
				break;

			case RBE_SHATTER:
				damage -= (damage * ((ac < 150) ? ac : 150) / 250);
				if (damage > 23) earthquake(subject_ptr, m_ptr->fy, m_ptr->fx, 8, m_idx);
				break;

			case RBE_EXP_10:
			case RBE_EXP_20:
			case RBE_EXP_40:
			case RBE_EXP_80:
				pt = GF_NETHER;
				break;

			case RBE_TIME:
				pt = GF_TIME;
				break;

			case RBE_DR_LIFE:
				pt = GF_HYPODYNAMIA;
				effect_type = BLOW_EFFECT_TYPE_HEAL;
				break;

			case RBE_INERTIA:
				pt = GF_INERTIAL;
				break;

			case RBE_STUN:
				pt = GF_SOUND;
				break;

			default:
				pt = 0;
				break;
			}

			if (pt)
			{
				/* Do damage if not exploding */
				if (!explode)
				{
					project(subject_ptr, m_idx, 0, t_ptr->fy, t_ptr->fx,
						damage, pt, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
				}

				switch (effect_type)
				{
				case BLOW_EFFECT_TYPE_FEAR:
					project(subject_ptr, m_idx, 0, t_ptr->fy, t_ptr->fx,
						damage, GF_TURN_ALL, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
					break;

				case BLOW_EFFECT_TYPE_SLEEP:
					project(subject_ptr, m_idx, 0, t_ptr->fy, t_ptr->fx,
						r_ptr->level, GF_OLD_SLEEP, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
					break;

				case BLOW_EFFECT_TYPE_HEAL:
					if ((monster_living(m_idx)) && (damage > 2))
					{
						bool did_heal = FALSE;

						if (m_ptr->hp < m_ptr->maxhp) did_heal = TRUE;

						/* Heal */
						m_ptr->hp += damroll(4, damage / 6);
						if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

						/* Redraw (later) if needed */
						if (subject_ptr->health_who == m_idx) subject_ptr->redraw |= (PR_HEALTH);
						if (subject_ptr->riding == m_idx) subject_ptr->redraw |= (PR_UHEALTH);

						/* Special message */
						if (see_m && did_heal)
						{
							msg_format(_("%sは体力を回復したようだ。", "%^s appears healthier."), m_name);
						}
					}
					break;
				}

				if (touched)
				{
					/* Aura fire */
					if ((tr_ptr->flags2 & RF2_AURA_FIRE) && m_ptr->r_idx)
					{
						if (!(r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK))
						{
							if (see_either)
							{
								msg_format(_("%^sは突然熱くなった！", "%^s is suddenly very hot!"), m_name);
							}
							if (m_ptr->ml && is_original_ap_and_seen(subject_ptr, t_ptr)) tr_ptr->r_flags2 |= RF2_AURA_FIRE;
							project(subject_ptr, t_idx, 0, m_ptr->fy, m_ptr->fx,
								damroll(1 + ((tr_ptr->level) / 26),
									1 + ((tr_ptr->level) / 17)),
								GF_FIRE, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
						}
						else
						{
							if (is_original_ap_and_seen(subject_ptr, m_ptr)) r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK);
						}
					}

					/* Aura cold */
					if ((tr_ptr->flags3 & RF3_AURA_COLD) && m_ptr->r_idx)
					{
						if (!(r_ptr->flagsr & RFR_EFF_IM_COLD_MASK))
						{
							if (see_either)
							{
								msg_format(_("%^sは突然寒くなった！", "%^s is suddenly very cold!"), m_name);
							}
							if (m_ptr->ml && is_original_ap_and_seen(subject_ptr, t_ptr)) tr_ptr->r_flags3 |= RF3_AURA_COLD;
							project(subject_ptr, t_idx, 0, m_ptr->fy, m_ptr->fx,
								damroll(1 + ((tr_ptr->level) / 26),
									1 + ((tr_ptr->level) / 17)),
								GF_COLD, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
						}
						else
						{
							if (is_original_ap_and_seen(subject_ptr, m_ptr)) r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_COLD_MASK);
						}
					}

					/* Aura elec */
					if ((tr_ptr->flags2 & RF2_AURA_ELEC) && m_ptr->r_idx)
					{
						if (!(r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK))
						{
							if (see_either)
							{
								msg_format(_("%^sは電撃を食らった！", "%^s gets zapped!"), m_name);
							}
							if (m_ptr->ml && is_original_ap_and_seen(subject_ptr, t_ptr)) tr_ptr->r_flags2 |= RF2_AURA_ELEC;
							project(subject_ptr, t_idx, 0, m_ptr->fy, m_ptr->fx,
								damroll(1 + ((tr_ptr->level) / 26),
									1 + ((tr_ptr->level) / 17)),
								GF_ELEC, PROJECT_KILL | PROJECT_STOP | PROJECT_AIMED, -1);
						}
						else
						{
							if (is_original_ap_and_seen(subject_ptr, m_ptr)) r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK);
						}
					}
				}
			}
		}

		/* Monster missed player */
		else
		{
			/* Analyze failed attacks */
			switch (method)
			{
			case RBM_HIT:
			case RBM_TOUCH:
			case RBM_PUNCH:
			case RBM_KICK:
			case RBM_CLAW:
			case RBM_BITE:
			case RBM_STING:
			case RBM_SLASH:
			case RBM_BUTT:
			case RBM_CRUSH:
			case RBM_ENGULF:
			case RBM_CHARGE:
			{
				(void)set_monster_csleep(subject_ptr, t_idx, 0);

				/* Visible monsters */
				if (see_m)
				{
#ifdef JP
					msg_format("%sは%^sの攻撃をかわした。", t_name, m_name);
#else
					msg_format("%^s misses %s.", m_name, t_name);
#endif
				}

				break;
			}
			}
		}


		/* Analyze "visible" monsters only */
		if (is_original_ap_and_seen(subject_ptr, m_ptr) && !do_silly_attack)
		{
			/* Count "obvious" attacks (and ones that cause damage) */
			if (obvious || damage || (r_ptr->r_blows[ap_cnt] > 10))
			{
				/* Count attacks of this type */
				if (r_ptr->r_blows[ap_cnt] < MAX_UCHAR)
				{
					r_ptr->r_blows[ap_cnt]++;
				}
			}
		}
	}

	if (explode)
	{
		sound(SOUND_EXPLODE);

		/* Cancel Invulnerability */
		(void)set_monster_invulner(subject_ptr, m_idx, 0, FALSE);
		mon_take_hit_mon(subject_ptr, m_idx, m_ptr->hp + 1, &dead, &fear, _("は爆発して粉々になった。", " explodes into tiny shreds."), m_idx);
		blinked = FALSE;
	}

	if (!blinked || m_ptr->r_idx == 0) return TRUE;

	if (teleport_barrier(subject_ptr, m_idx))
	{
		if (see_m)
		{
			msg_print(_("泥棒は笑って逃げ...ようとしたがバリアに防がれた。", "The thief flees laughing...? But a magic barrier obstructs it."));
		}
		else if (known)
		{
			subject_ptr->current_floor_ptr->monster_noise = TRUE;
		}
	}
	else
	{
		if (see_m)
		{
			msg_print(_("泥棒は笑って逃げた！", "The thief flees laughing!"));
		}
		else if (known)
		{
			subject_ptr->current_floor_ptr->monster_noise = TRUE;
		}

		teleport_away(subject_ptr, m_idx, MAX_SIGHT * 2 + 5, TELEPORT_SPONTANEOUS);
	}

	return TRUE;
}



/*!
 * @brief モンスターが敵モンスターに行う打撃処理 /
 * Hack, based on mon_take_hit... perhaps all monster attacks on other monsters should use this?
 * @param m_idx 目標となるモンスターの参照ID
 * @param dam ダメージ量
 * @param dead 目標となったモンスターの死亡状態を返す参照ポインタ
 * @param fear 目標となったモンスターの恐慌状態を返す参照ポインタ
 * @param note 目標モンスターが死亡した場合の特別メッセージ(NULLならば標準表示を行う)
 * @param who 打撃を行ったモンスターの参照ID
 * @return なし
 */
void mon_take_hit_mon(player_type *player_ptr, MONSTER_IDX m_idx, HIT_POINT dam, bool *dead, bool *fear, concptr note, MONSTER_IDX who)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	monster_type *m_ptr = &floor_ptr->m_list[m_idx];
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	GAME_TEXT m_name[160];
	bool seen = is_seen(m_ptr);

	/* Can the player be aware of this attack? */
	bool known = (m_ptr->cdis <= MAX_SIGHT);

	monster_desc(player_ptr, m_name, m_ptr, 0);

	/* Redraw (later) if needed */
	if (m_ptr->ml)
	{
		if (player_ptr->health_who == m_idx) player_ptr->redraw |= (PR_HEALTH);
		if (player_ptr->riding == m_idx) player_ptr->redraw |= (PR_UHEALTH);
	}

	(void)set_monster_csleep(player_ptr, m_idx, 0);

	if (player_ptr->riding && (m_idx == player_ptr->riding)) disturb(player_ptr, TRUE, TRUE);

	if (MON_INVULNER(m_ptr) && randint0(PENETRATE_INVULNERABILITY))
	{
		if (seen)
		{
			msg_format(_("%^sはダメージを受けない。", "%^s is unharmed."), m_name);
		}
		return;
	}

	if (r_ptr->flagsr & RFR_RES_ALL)
	{
		if (dam > 0)
		{
			dam /= 100;
			if ((dam == 0) && one_in_(3)) dam = 1;
		}
		if (dam == 0)
		{
			if (seen)
			{
				msg_format(_("%^sはダメージを受けない。", "%^s is unharmed."), m_name);
			}
			return;
		}
	}

	/* Hurt it */
	m_ptr->hp -= dam;

	/* It is dead now... or is it? */
	if (m_ptr->hp < 0)
	{
		if (((r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) ||
			(r_ptr->flags7 & RF7_NAZGUL)) &&
			!player_ptr->phase_out)
		{
			m_ptr->hp = 1;
		}
		else
		{
			/* Make a sound */
			if (!monster_living(m_ptr->r_idx))
			{
				sound(SOUND_N_KILL);
			}
			else
			{
				sound(SOUND_KILL);
			}

			*dead = TRUE;

			if (known)
			{
				monster_desc(player_ptr, m_name, m_ptr, MD_TRUE_NAME);
				/* Unseen death by normal attack */
				if (!seen)
				{
					floor_ptr->monster_noise = TRUE;
				}
				/* Death by special attack */
				else if (note)
				{
					msg_format(_("%^s%s", "%^s%s"), m_name, note);
				}
				/* Death by normal attack -- nonliving monster */
				else if (!monster_living(m_ptr->r_idx))
				{
					msg_format(_("%^sは破壊された。", "%^s is destroyed."), m_name);
				}
				/* Death by normal attack -- living monster */
				else
				{
					msg_format(_("%^sは殺された。", "%^s is killed."), m_name);
				}
			}

			monster_gain_exp(player_ptr, who, m_ptr->r_idx);
			monster_death(player_ptr, m_idx, FALSE);
			delete_monster_idx(player_ptr, m_idx);

			/* Not afraid */
			(*fear) = FALSE;

			/* Monster is dead */
			return;
		}
	}

	*dead = FALSE;

	/* Mega-Hack -- Pain cancels fear */
	if (MON_MONFEAR(m_ptr) && (dam > 0))
	{
		/* Cure fear */
		if (set_monster_monfear(player_ptr, m_idx, MON_MONFEAR(m_ptr) - randint1(dam / 4)))
		{
			/* No more fear */
			(*fear) = FALSE;
		}
	}

	/* Sometimes a monster gets scared by damage */
	if (!MON_MONFEAR(m_ptr) && !(r_ptr->flags3 & RF3_NO_FEAR))
	{
		/* Percentage of fully healthy */
		int percentage = (100L * m_ptr->hp) / m_ptr->maxhp;

		/*
		* Run (sometimes) if at 10% or less of max hit points,
		* or (usually) when hit for half its current hit points
		 */
		if (((percentage <= 10) && (randint0(10) < percentage)) ||
			((dam >= m_ptr->hp) && (randint0(100) < 80)))
		{
			/* Hack -- note fear */
			(*fear) = TRUE;

			/* Hack -- Add some timed fear */
			(void)set_monster_monfear(player_ptr, m_idx, (randint1(10) +
				(((dam >= m_ptr->hp) && (percentage > 7)) ?
					20 : ((11 - percentage) * 5))));
		}
	}

	if ((dam > 0) && !is_pet(m_ptr) && !is_friendly(m_ptr) && (who != m_idx))
	{
		if (is_pet(&floor_ptr->m_list[who]) && !player_bold(player_ptr, m_ptr->target_y, m_ptr->target_x))
		{
			set_target(m_ptr, floor_ptr->m_list[who].fy, floor_ptr->m_list[who].fx);
		}
	}

	if (player_ptr->riding && (player_ptr->riding == m_idx) && (dam > 0))
	{
		monster_desc(player_ptr, m_name, m_ptr, 0);

		if (m_ptr->hp > m_ptr->maxhp / 3) dam = (dam + 1) / 2;
		if (rakuba(player_ptr, (dam > 200) ? 200 : dam, FALSE))
		{
			msg_format(_("%^sに振り落とされた！", "You have been thrown off from %s!"), m_name);
		}
	}
}
