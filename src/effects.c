/* File: effects.c */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

/* Purpose: effects of various "objects" */

#include "angband.h"

void set_action(int typ)
{
	int prev_typ = p_ptr->action;

	if (typ == prev_typ)
	{
		return;
	}
	else
	{
		switch (prev_typ)
		{
			case ACTION_SEARCH:
			{
#ifdef JP
				msg_print("探索をやめた。");
#else
				msg_print("You no longer walk carefully.");
#endif
				p_ptr->redraw |= (PR_SPEED);
				break;
			}
			case ACTION_REST:
			{
				resting = 0;
				break;
			}
			case ACTION_LEARN:
			{
#ifdef JP
				msg_print("学習をやめた。");
#else
				msg_print("You stop Learning");
#endif
				new_mane = FALSE;
				break;
			}
			case ACTION_KAMAE:
			{
#ifdef JP
				msg_print("構えをといた。");
#else
				msg_print("You stop assuming the posture.");
#endif
				p_ptr->special_defense &= ~(KAMAE_MASK);
				break;
			}
			case ACTION_KATA:
			{
#ifdef JP
				msg_print("型を崩した。");
#else
				msg_print("You stop assuming the posture.");
#endif
				p_ptr->special_defense &= ~(KATA_MASK);
				p_ptr->update |= (PU_MONSTERS);
				p_ptr->redraw |= (PR_STATUS);
				break;
			}
			case ACTION_SING:
			{
#ifdef JP
				msg_print("歌うのをやめた。");
#else
				msg_print("You stop singing.");
#endif
				break;
			}
			case ACTION_HAYAGAKE:
			{
#ifdef JP
				msg_print("足が重くなった。");
#else
				msg_print("You are no longer walking extremely fast.");
#endif
				energy_use = 100;
				break;
			}
			case ACTION_SPELL:
			{
#ifdef JP
				msg_print("呪文の詠唱を中断した。");
#else
				msg_print("You stopped spelling all spells.");
#endif
				break;
			}
		}
	}

	p_ptr->action = typ;

	/* If we are requested other action, stop singing */
	if (prev_typ == ACTION_SING) stop_singing();

	switch (p_ptr->action)
	{
		case ACTION_SEARCH:
		{
#ifdef JP
			msg_print("注意深く歩き始めた。");
#else
			msg_print("You begin to walk carefully.");
#endif
			p_ptr->redraw |= (PR_SPEED);
			break;
		}
		case ACTION_LEARN:
		{
#ifdef JP
			msg_print("学習を始めた。");
#else
			msg_print("You begin Learning");
#endif
			break;
		}
		case ACTION_FISH:
		{
#ifdef JP
			msg_print("水面に糸を垂らした．．．");
#else
			msg_print("You begin fishing...");
#endif
			break;
		}
		case ACTION_HAYAGAKE:
		{
#ifdef JP
			msg_print("足が羽のように軽くなった。");
#else
			msg_print("You begin to walk extremely fast.");
#endif
			break;
		}
		default:
		{
			break;
		}
	}

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Redraw the state */
	p_ptr->redraw |= (PR_STATE);
}

/* reset timed flags */
void reset_tim_flags(void)
{
	p_ptr->fast = 0;            /* Timed -- Fast */
	p_ptr->lightspeed = 0;
	p_ptr->slow = 0;            /* Timed -- Slow */
	p_ptr->blind = 0;           /* Timed -- Blindness */
	p_ptr->paralyzed = 0;       /* Timed -- Paralysis */
	p_ptr->confused = 0;        /* Timed -- Confusion */
	p_ptr->afraid = 0;          /* Timed -- Fear */
	p_ptr->image = 0;           /* Timed -- Hallucination */
	p_ptr->poisoned = 0;        /* Timed -- Poisoned */
	p_ptr->cut = 0;             /* Timed -- Cut */
	p_ptr->stun = 0;            /* Timed -- Stun */

	p_ptr->protevil = 0;        /* Timed -- Protection */
	p_ptr->invuln = 0;          /* Timed -- Invulnerable */
	p_ptr->ult_res = 0;
	p_ptr->hero = 0;            /* Timed -- Heroism */
	p_ptr->shero = 0;           /* Timed -- Super Heroism */
	p_ptr->shield = 0;          /* Timed -- Shield Spell */
	p_ptr->blessed = 0;         /* Timed -- Blessed */
	p_ptr->tim_invis = 0;       /* Timed -- Invisibility */
	p_ptr->tim_infra = 0;       /* Timed -- Infra Vision */
	p_ptr->tim_regen = 0;       /* Timed -- Regeneration */
	p_ptr->tim_stealth = 0;     /* Timed -- Stealth */
	p_ptr->tim_esp = 0;
	p_ptr->wraith_form = 0;     /* Timed -- Wraith Form */
	p_ptr->tim_levitation = 0;
	p_ptr->tim_sh_touki = 0;
	p_ptr->tim_sh_fire = 0;
	p_ptr->tim_sh_holy = 0;
	p_ptr->tim_eyeeye = 0;
	p_ptr->magicdef = 0;
	p_ptr->resist_magic = 0;
	p_ptr->tsuyoshi = 0;
	p_ptr->kabenuke = 0;
	p_ptr->tim_res_nether = 0;
	p_ptr->tim_res_time = 0;
	p_ptr->tim_mimic = 0;
	p_ptr->mimic_form = 0;
	p_ptr->tim_reflect = 0;
	p_ptr->multishadow = 0;
	p_ptr->dustrobe = 0;
	p_ptr->action = ACTION_NONE;


	p_ptr->oppose_acid = 0;     /* Timed -- oppose acid */
	p_ptr->oppose_elec = 0;     /* Timed -- oppose lightning */
	p_ptr->oppose_fire = 0;     /* Timed -- oppose heat */
	p_ptr->oppose_cold = 0;     /* Timed -- oppose cold */
	p_ptr->oppose_pois = 0;     /* Timed -- oppose poison */

	p_ptr->word_recall = 0;
	p_ptr->alter_reality = 0;
	p_ptr->sutemi = FALSE;
	p_ptr->counter = FALSE;
	p_ptr->ele_attack = 0;
	p_ptr->ele_immune = 0;
	p_ptr->special_attack = 0L;
	p_ptr->special_defense = 0L;

	while(p_ptr->energy_need < 0) p_ptr->energy_need += ENERGY_NEED();
	world_player = FALSE;

	if (prace_is_(RACE_DEMON) && (p_ptr->lev > 44)) p_ptr->oppose_fire = 1;
	if ((p_ptr->pclass == CLASS_NINJA) && (p_ptr->lev > 44)) p_ptr->oppose_pois = 1;
	if (p_ptr->pclass == CLASS_BERSERKER) p_ptr->shero = 1;

	if (p_ptr->riding)
	{
		(void)set_monster_fast(p_ptr->riding, 0);
		(void)set_monster_slow(p_ptr->riding, 0);
		(void)set_monster_invulner(p_ptr->riding, 0, FALSE);
	}

	if (p_ptr->pclass == CLASS_BARD)
	{
		p_ptr->magic_num1[0] = 0;
		p_ptr->magic_num2[0] = 0;
	}
}


void dispel_player(void)
{
	(void)set_fast(0, TRUE);
	(void)set_lightspeed(0, TRUE);
	(void)set_slow(0, TRUE);
	(void)set_shield(0, TRUE);
	(void)set_blessed(0, TRUE);
	(void)set_tsuyoshi(0, TRUE);
	(void)set_hero(0, TRUE);
	(void)set_shero(0, TRUE);
	(void)set_protevil(0, TRUE);
	(void)set_invuln(0, TRUE);
	(void)set_wraith_form(0, TRUE);
	(void)set_kabenuke(0, TRUE);
	(void)set_tim_res_nether(0, TRUE);
	(void)set_tim_res_time(0, TRUE);
	/* by henkma */
	(void)set_tim_reflect(0,TRUE);
	(void)set_multishadow(0,TRUE);
	(void)set_dustrobe(0,TRUE);

	(void)set_tim_invis(0, TRUE);
	(void)set_tim_infra(0, TRUE);
	(void)set_tim_esp(0, TRUE);
	(void)set_tim_regen(0, TRUE);
	(void)set_tim_stealth(0, TRUE);
	(void)set_tim_levitation(0, TRUE);
	(void)set_tim_sh_touki(0, TRUE);
	(void)set_tim_sh_fire(0, TRUE);
	(void)set_tim_sh_holy(0, TRUE);
	(void)set_tim_eyeeye(0, TRUE);
	(void)set_magicdef(0, TRUE);
	(void)set_resist_magic(0, TRUE);
	(void)set_oppose_acid(0, TRUE);
	(void)set_oppose_elec(0, TRUE);
	(void)set_oppose_fire(0, TRUE);
	(void)set_oppose_cold(0, TRUE);
	(void)set_oppose_pois(0, TRUE);
	(void)set_ultimate_res(0, TRUE);
	(void)set_mimic(0, 0, TRUE);
	(void)set_ele_attack(0, 0);
	(void)set_ele_immune(0, 0);

	/* Cancel glowing hands */
	if (p_ptr->special_attack & ATTACK_CONFUSE)
	{
		p_ptr->special_attack &= ~(ATTACK_CONFUSE);
#ifdef JP
		msg_print("手の輝きがなくなった。");
#else
		msg_print("Your hands stop glowing.");
#endif
	}

	if (music_singing_any() || hex_spelling_any())
	{
#ifdef JP
		cptr str = (music_singing_any()) ? "歌" : "呪文";
#else
		cptr str = (music_singing_any()) ? "singing" : "spelling";
#endif
		p_ptr->magic_num1[1] = p_ptr->magic_num1[0];
		p_ptr->magic_num1[0] = 0;
#ifdef JP
		msg_format("%sが途切れた。", str);
#else
		msg_format("Your %s is interrupted.", str);
#endif
		p_ptr->action = ACTION_NONE;

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS | PU_HP);

		/* Redraw map */
		p_ptr->redraw |= (PR_MAP | PR_STATUS | PR_STATE);

		/* Update monsters */
		p_ptr->update |= (PU_MONSTERS);

		/* Window stuff */
		p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

		p_ptr->energy_need += ENERGY_NEED();
	}
}


/*
 * Set "p_ptr->tim_mimic", and "p_ptr->mimic_form",
 * notice observable changes
 */
bool set_mimic(int v, int p, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->tim_mimic && (p_ptr->mimic_form == p) && !do_dec)
		{
			if (p_ptr->tim_mimic > v) return FALSE;
		}
		else if ((!p_ptr->tim_mimic) || (p_ptr->mimic_form != p))
		{
#ifdef JP
			msg_print("自分の体が変わってゆくのを感じた。");
#else
			msg_print("You feel that your body changes.");
#endif
			p_ptr->mimic_form=p;
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->tim_mimic)
		{
#ifdef JP
			msg_print("変身が解けた。");
#else
			msg_print("You are no longer transformed.");
#endif
			if (p_ptr->mimic_form == MIMIC_DEMON) set_oppose_fire(0, TRUE);
			p_ptr->mimic_form=0;
			notice = TRUE;
			p = 0;
		}
	}

	/* Use the value */
	p_ptr->tim_mimic = v;

	/* Nothing to notice */
	if (!notice)
		return (FALSE);

	/* Disturb */
	if (disturb_state)
		disturb(0, 0);

	/* Redraw title */
	p_ptr->redraw |= (PR_BASIC | PR_STATUS);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS | PU_HP);

	handle_stuff();

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->blind", notice observable changes
 *
 * Note the use of "PU_UN_LITE" and "PU_UN_VIEW", which is needed to
 * memorize any terrain features which suddenly become "visible".
 * Note that blindness is currently the only thing which can affect
 * "player_can_see_bold()".
 */
bool set_blind(int v)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (!p_ptr->blind)
		{
			if (p_ptr->prace == RACE_ANDROID)
			{
#ifdef JP
msg_print("センサーをやられた！");
#else
				msg_print("You are blind!");
#endif
			}
			else
			{
#ifdef JP
msg_print("目が見えなくなってしまった！");
#else
				msg_print("You are blind!");
#endif
			}

			notice = TRUE;
			chg_virtue(V_ENLIGHTEN, -1);
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->blind)
		{
			if (p_ptr->prace == RACE_ANDROID)
			{
#ifdef JP
msg_print("センサーが復旧した。");
#else
				msg_print("You can see again.");
#endif
			}
			else
			{
#ifdef JP
msg_print("やっと目が見えるようになった。");
#else
				msg_print("You can see again.");
#endif
			}

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->blind = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Fully update the visuals */
	p_ptr->update |= (PU_UN_VIEW | PU_UN_LITE | PU_VIEW | PU_LITE | PU_MONSTERS | PU_MON_LITE);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->confused", notice observable changes
 */
bool set_confused(int v)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (!p_ptr->confused)
		{
#ifdef JP
msg_print("あなたは混乱した！");
#else
			msg_print("You are confused!");
#endif

			if (p_ptr->action == ACTION_LEARN)
			{
#ifdef JP
				msg_print("学習が続けられない！");
#else
				msg_print("You cannot continue Learning!");
#endif
				new_mane = FALSE;

				p_ptr->redraw |= (PR_STATE);
				p_ptr->action = ACTION_NONE;
			}
			if (p_ptr->action == ACTION_KAMAE)
			{
#ifdef JP
				msg_print("構えがとけた。");
#else
				msg_print("Your posture gets loose.");
#endif
				p_ptr->special_defense &= ~(KAMAE_MASK);
				p_ptr->update |= (PU_BONUS);
				p_ptr->redraw |= (PR_STATE);
				p_ptr->action = ACTION_NONE;
			}
			else if (p_ptr->action == ACTION_KATA)
			{
#ifdef JP
				msg_print("型が崩れた。");
#else
				msg_print("Your posture gets loose.");
#endif
				p_ptr->special_defense &= ~(KATA_MASK);
				p_ptr->update |= (PU_BONUS);
				p_ptr->update |= (PU_MONSTERS);
				p_ptr->redraw |= (PR_STATE);
				p_ptr->redraw |= (PR_STATUS);
				p_ptr->action = ACTION_NONE;
			}

			/* Sniper */
			if (p_ptr->concent) reset_concentration(TRUE);

			/* Hex */
			if (hex_spelling_any()) stop_hex_spell_all();

			notice = TRUE;
			p_ptr->counter = FALSE;
			chg_virtue(V_HARMONY, -1);
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->confused)
		{
#ifdef JP
msg_print("やっと混乱がおさまった。");
#else
			msg_print("You feel less confused now.");
#endif

			p_ptr->special_attack &= ~(ATTACK_SUIKEN);
			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->confused = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->poisoned", notice observable changes
 */
bool set_poisoned(int v)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (!p_ptr->poisoned)
		{
#ifdef JP
msg_print("毒に侵されてしまった！");
#else
			msg_print("You are poisoned!");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->poisoned)
		{
#ifdef JP
msg_print("やっと毒の痛みがなくなった。");
#else
			msg_print("You are no longer poisoned.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->poisoned = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->afraid", notice observable changes
 */
bool set_afraid(int v)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (!p_ptr->afraid)
		{
#ifdef JP
msg_print("何もかも恐くなってきた！");
#else
			msg_print("You are terrified!");
#endif

			if (p_ptr->special_defense & KATA_MASK)
			{
#ifdef JP
				msg_print("型が崩れた。");
#else
				msg_print("Your posture gets loose.");
#endif
				p_ptr->special_defense &= ~(KATA_MASK);
				p_ptr->update |= (PU_BONUS);
				p_ptr->update |= (PU_MONSTERS);
				p_ptr->redraw |= (PR_STATE);
				p_ptr->redraw |= (PR_STATUS);
				p_ptr->action = ACTION_NONE;
			}

			notice = TRUE;
			p_ptr->counter = FALSE;
			chg_virtue(V_VALOUR, -1);
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->afraid)
		{
#ifdef JP
msg_print("やっと恐怖を振り払った。");
#else
			msg_print("You feel bolder now.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->afraid = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->paralyzed", notice observable changes
 */
bool set_paralyzed(int v)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (!p_ptr->paralyzed)
		{
#ifdef JP
msg_print("体が麻痺してしまった！");
#else
			msg_print("You are paralyzed!");
#endif

			/* Sniper */
			if (p_ptr->concent) reset_concentration(TRUE);

			/* Hex */
			if (hex_spelling_any()) stop_hex_spell_all();

			p_ptr->counter = FALSE;
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->paralyzed)
		{
#ifdef JP
msg_print("やっと動けるようになった。");
#else
			msg_print("You can move again.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->paralyzed = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Redraw the state */
	p_ptr->redraw |= (PR_STATE);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->image", notice observable changes
 *
 * Note that we must redraw the map when hallucination changes.
 */
bool set_image(int v)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;


	/* Open */
	if (v)
	{
		set_tsuyoshi(0, TRUE);
		if (!p_ptr->image)
		{
#ifdef JP
msg_print("ワーオ！何もかも虹色に見える！");
#else
			msg_print("Oh, wow! Everything looks so cosmic now!");
#endif

			/* Sniper */
			if (p_ptr->concent) reset_concentration(TRUE);

			p_ptr->counter = FALSE;
			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->image)
		{
#ifdef JP
msg_print("やっとはっきりと物が見えるようになった。");
#else
			msg_print("You can see clearly again.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->image = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Redraw map */
	p_ptr->redraw |= (PR_MAP);

	/* Update the health bar */
	p_ptr->redraw |= (PR_HEALTH | PR_UHEALTH);

	/* Update monsters */
	p_ptr->update |= (PU_MONSTERS);

	/* Window stuff */
	p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->fast", notice observable changes
 */
bool set_fast(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->fast && !do_dec)
		{
			if (p_ptr->fast > v) return FALSE;
		}
		else if (!IS_FAST() && !p_ptr->lightspeed)
		{
#ifdef JP
msg_print("素早く動けるようになった！");
#else
			msg_print("You feel yourself moving much faster!");
#endif

			notice = TRUE;
			chg_virtue(V_PATIENCE, -1);
			chg_virtue(V_DILIGENCE, 1);
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->fast && !p_ptr->lightspeed && !music_singing(MUSIC_SPEED) && !music_singing(MUSIC_SHERO))
		{
#ifdef JP
msg_print("動きの素早さがなくなったようだ。");
#else
			msg_print("You feel yourself slow down.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->fast = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->lightspeed", notice observable changes
 */
bool set_lightspeed(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	if (p_ptr->wild_mode) v = 0;

	/* Open */
	if (v)
	{
		if (p_ptr->lightspeed && !do_dec)
		{
			if (p_ptr->lightspeed > v) return FALSE;
		}
		else if (!p_ptr->lightspeed)
		{
#ifdef JP
msg_print("非常に素早く動けるようになった！");
#else
			msg_print("You feel yourself moving extremely faster!");
#endif

			notice = TRUE;
			chg_virtue(V_PATIENCE, -1);
			chg_virtue(V_DILIGENCE, 1);
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->lightspeed)
		{
#ifdef JP
msg_print("動きの素早さがなくなったようだ。");
#else
			msg_print("You feel yourself slow down.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->lightspeed = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->slow", notice observable changes
 */
bool set_slow(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->slow && !do_dec)
		{
			if (p_ptr->slow > v) return FALSE;
		}
		else if (!p_ptr->slow)
		{
#ifdef JP
msg_print("体の動きが遅くなってしまった！");
#else
			msg_print("You feel yourself moving slower!");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->slow)
		{
#ifdef JP
msg_print("動きの遅さがなくなったようだ。");
#else
			msg_print("You feel yourself speed up.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->slow = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->shield", notice observable changes
 */
bool set_shield(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->shield && !do_dec)
		{
			if (p_ptr->shield > v) return FALSE;
		}
		else if (!p_ptr->shield)
		{
#ifdef JP
msg_print("肌が石になった。");
#else
			msg_print("Your skin turns to stone.");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->shield)
		{
#ifdef JP
msg_print("肌が元に戻った。");
#else
			msg_print("Your skin returns to normal.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->shield = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}



/*
 * Set "p_ptr->tsubureru", notice observable changes
 */
bool set_tsubureru(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->tsubureru && !do_dec)
		{
			if (p_ptr->tsubureru > v) return FALSE;
		}
		else if (!p_ptr->tsubureru)
		{
#ifdef JP
msg_print("横に伸びた。");
#else
			msg_print("Your body expands horizontally.");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->tsubureru)
		{
#ifdef JP
msg_print("もう横に伸びていない。");
#else
			msg_print("Your body returns to normal.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tsubureru = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}



/*
 * Set "p_ptr->magicdef", notice observable changes
 */
bool set_magicdef(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->magicdef && !do_dec)
		{
			if (p_ptr->magicdef > v) return FALSE;
		}
		else if (!p_ptr->magicdef)
		{
#ifdef JP
			msg_print("魔法の防御力が増したような気がする。");
#else
			msg_print("You feel more resistant to magic.");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->magicdef)
		{
#ifdef JP
			msg_print("魔法の防御力が元に戻った。");
#else
			msg_print("You feel less resistant to magic.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->magicdef = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}



/*
 * Set "p_ptr->blessed", notice observable changes
 */
bool set_blessed(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->blessed && !do_dec)
		{
			if (p_ptr->blessed > v) return FALSE;
		}
		else if (!IS_BLESSED())
		{
#ifdef JP
msg_print("高潔な気分になった！");
#else
			msg_print("You feel righteous!");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->blessed && !music_singing(MUSIC_BLESS))
		{
#ifdef JP
msg_print("高潔な気分が消え失せた。");
#else
			msg_print("The prayer has expired.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->blessed = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->hero", notice observable changes
 */
bool set_hero(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->hero && !do_dec)
		{
			if (p_ptr->hero > v) return FALSE;
		}
		else if (!IS_HERO())
		{
#ifdef JP
msg_print("ヒーローになった気がする！");
#else
			msg_print("You feel like a hero!");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->hero && !music_singing(MUSIC_HERO) && !music_singing(MUSIC_SHERO))
		{
#ifdef JP
msg_print("ヒーローの気分が消え失せた。");
#else
			msg_print("The heroism wears off.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->hero = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Recalculate hitpoints */
	p_ptr->update |= (PU_HP);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->shero", notice observable changes
 */
bool set_shero(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	if (p_ptr->pclass == CLASS_BERSERKER) v = 1;
	/* Open */
	if (v)
	{
		if (p_ptr->shero && !do_dec)
		{
			if (p_ptr->shero > v) return FALSE;
		}
		else if (!p_ptr->shero)
		{
#ifdef JP
msg_print("殺戮マシーンになった気がする！");
#else
			msg_print("You feel like a killing machine!");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->shero)
		{
#ifdef JP
msg_print("野蛮な気持ちが消え失せた。");
#else
			msg_print("You feel less Berserk.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->shero = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Recalculate hitpoints */
	p_ptr->update |= (PU_HP);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->protevil", notice observable changes
 */
bool set_protevil(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->protevil && !do_dec)
		{
			if (p_ptr->protevil > v) return FALSE;
		}
		else if (!p_ptr->protevil)
		{
#ifdef JP
msg_print("邪悪なる存在から守られているような感じがする！");
#else
			msg_print("You feel safe from evil!");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->protevil)
		{
#ifdef JP
msg_print("邪悪なる存在から守られている感じがなくなった。");
#else
			msg_print("You no longer feel safe from evil.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->protevil = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}

/*
 * Set "p_ptr->wraith_form", notice observable changes
 */
bool set_wraith_form(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->wraith_form && !do_dec)
		{
			if (p_ptr->wraith_form > v) return FALSE;
		}
		else if (!p_ptr->wraith_form)
		{
#ifdef JP
msg_print("物質界を離れて幽鬼のような存在になった！");
#else
			msg_print("You leave the physical world and turn into a wraith-being!");
#endif

			notice = TRUE;

			chg_virtue(V_UNLIFE, 3);
			chg_virtue(V_HONOUR, -2);
			chg_virtue(V_SACRIFICE, -2);
			chg_virtue(V_VALOUR, -5);

			/* Redraw map */
			p_ptr->redraw |= (PR_MAP);

			/* Update monsters */
			p_ptr->update |= (PU_MONSTERS);

			/* Window stuff */
			p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->wraith_form)
		{
#ifdef JP
msg_print("不透明になった感じがする。");
#else
			msg_print("You feel opaque.");
#endif

			notice = TRUE;

			/* Redraw map */
			p_ptr->redraw |= (PR_MAP);

			/* Update monsters */
			p_ptr->update |= (PU_MONSTERS);

			/* Window stuff */
			p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
		}
	}

	/* Use the value */
	p_ptr->wraith_form = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);

}


/*
 * Set "p_ptr->invuln", notice observable changes
 */
bool set_invuln(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->invuln && !do_dec)
		{
			if (p_ptr->invuln > v) return FALSE;
		}
		else if (!IS_INVULN())
		{
#ifdef JP
msg_print("無敵だ！");
#else
			msg_print("Invulnerability!");
#endif

			notice = TRUE;

			chg_virtue(V_UNLIFE, -2);
			chg_virtue(V_HONOUR, -2);
			chg_virtue(V_SACRIFICE, -3);
			chg_virtue(V_VALOUR, -5);

			/* Redraw map */
			p_ptr->redraw |= (PR_MAP);

			/* Update monsters */
			p_ptr->update |= (PU_MONSTERS);

			/* Window stuff */
			p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->invuln && !music_singing(MUSIC_INVULN))
		{
#ifdef JP
msg_print("無敵ではなくなった。");
#else
			msg_print("The invulnerability wears off.");
#endif

			notice = TRUE;

			/* Redraw map */
			p_ptr->redraw |= (PR_MAP);

			/* Update monsters */
			p_ptr->update |= (PU_MONSTERS);

			/* Window stuff */
			p_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);

			p_ptr->energy_need += ENERGY_NEED();
		}
	}

	/* Use the value */
	p_ptr->invuln = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->tim_esp", notice observable changes
 */
bool set_tim_esp(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->tim_esp && !do_dec)
		{
			if (p_ptr->tim_esp > v) return FALSE;
		}
		else if (!IS_TIM_ESP())
		{
#ifdef JP
msg_print("意識が広がった気がする！");
#else
			msg_print("You feel your consciousness expand!");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->tim_esp && !music_singing(MUSIC_MIND))
		{
#ifdef JP
msg_print("意識は元に戻った。");
#else
			msg_print("Your consciousness contracts again.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_esp = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Update the monsters */
	p_ptr->update |= (PU_MONSTERS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->tim_invis", notice observable changes
 */
bool set_tim_invis(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->tim_invis && !do_dec)
		{
			if (p_ptr->tim_invis > v) return FALSE;
		}
		else if (!p_ptr->tim_invis)
		{
#ifdef JP
msg_print("目が非常に敏感になった気がする！");
#else
			msg_print("Your eyes feel very sensitive!");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->tim_invis)
		{
#ifdef JP
msg_print("目の敏感さがなくなったようだ。");
#else
			msg_print("Your eyes feel less sensitive.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_invis = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Update the monsters */
	p_ptr->update |= (PU_MONSTERS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->tim_infra", notice observable changes
 */
bool set_tim_infra(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->tim_infra && !do_dec)
		{
			if (p_ptr->tim_infra > v) return FALSE;
		}
		else if (!p_ptr->tim_infra)
		{
#ifdef JP
msg_print("目がランランと輝き始めた！");
#else
			msg_print("Your eyes begin to tingle!");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->tim_infra)
		{
#ifdef JP
msg_print("目の輝きがなくなった。");
#else
			msg_print("Your eyes stop tingling.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_infra = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Update the monsters */
	p_ptr->update |= (PU_MONSTERS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->tim_regen", notice observable changes
 */
bool set_tim_regen(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->tim_regen && !do_dec)
		{
			if (p_ptr->tim_regen > v) return FALSE;
		}
		else if (!p_ptr->tim_regen)
		{
#ifdef JP
msg_print("回復力が上がった！");
#else
			msg_print("You feel yourself regenerating quickly!");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->tim_regen)
		{
#ifdef JP
msg_print("素早く回復する感じがなくなった。");
#else
			msg_print("You feel yourself regenerating slowly.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_regen = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->tim_stealth", notice observable changes
 */
bool set_tim_stealth(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->tim_stealth && !do_dec)
		{
			if (p_ptr->tim_stealth > v) return FALSE;
		}
		else if (!IS_TIM_STEALTH())
		{
#ifdef JP
msg_print("足音が小さくなった！");
#else
			msg_print("You begin to walk silently!");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->tim_stealth && !music_singing(MUSIC_STEALTH))
		{
#ifdef JP
msg_print("足音が大きくなった。");
#else
			msg_print("You no longer walk silently.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_stealth = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


bool set_superstealth(bool set)
{
	bool notice = FALSE;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (set)
	{
		if (!(p_ptr->special_defense & NINJA_S_STEALTH))
		{
			if (cave[py][px].info & CAVE_MNLT)
			{
#ifdef JP
				msg_print("敵の目から薄い影の中に覆い隠された。");
#else
				msg_print("You are mantled in weak shadow from ordinary eyes.");
#endif
				p_ptr->monlite = p_ptr->old_monlite = TRUE;
			}
			else
			{
#ifdef JP
				msg_print("敵の目から影の中に覆い隠された！");
#else
				msg_print("You are mantled in shadow from ordinary eyes!");
#endif
				p_ptr->monlite = p_ptr->old_monlite = FALSE;
			}

			notice = TRUE;

			/* Use the value */
			p_ptr->special_defense |= NINJA_S_STEALTH;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->special_defense & NINJA_S_STEALTH)
		{
#ifdef JP
			msg_print("再び敵の目にさらされるようになった。");
#else
			msg_print("You are exposed to common sight once more.");
#endif

			notice = TRUE;

			/* Use the value */
			p_ptr->special_defense &= ~(NINJA_S_STEALTH);
		}
	}

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->tim_levitation", notice observable changes
 */
bool set_tim_levitation(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->tim_levitation && !do_dec)
		{
			if (p_ptr->tim_levitation > v) return FALSE;
		}
		else if (!p_ptr->tim_levitation)
		{
#ifdef JP
msg_print("体が宙に浮き始めた。");
#else
			msg_print("You begin to fly!");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->tim_levitation)
		{
#ifdef JP
msg_print("もう宙に浮かべなくなった。");
#else
			msg_print("You stop flying.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_levitation = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->tim_sh_touki", notice observable changes
 */
bool set_tim_sh_touki(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->tim_sh_touki && !do_dec)
		{
			if (p_ptr->tim_sh_touki > v) return FALSE;
		}
		else if (!p_ptr->tim_sh_touki)
		{
#ifdef JP
msg_print("体が闘気のオーラで覆われた。");
#else
			msg_print("You have enveloped by the aura of the Force!");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->tim_sh_touki)
		{
#ifdef JP
msg_print("闘気が消えた。");
#else
			msg_print("Aura of the Force disappeared.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_sh_touki = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->tim_sh_fire", notice observable changes
 */
bool set_tim_sh_fire(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->tim_sh_fire && !do_dec)
		{
			if (p_ptr->tim_sh_fire > v) return FALSE;
		}
		else if (!p_ptr->tim_sh_fire)
		{
#ifdef JP
msg_print("体が炎のオーラで覆われた。");
#else
			msg_print("You have enveloped by fiery aura!");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->tim_sh_fire)
		{
#ifdef JP
msg_print("炎のオーラが消えた。");
#else
			msg_print("Fiery aura disappeared.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_sh_fire = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->tim_sh_holy", notice observable changes
 */
bool set_tim_sh_holy(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->tim_sh_holy && !do_dec)
		{
			if (p_ptr->tim_sh_holy > v) return FALSE;
		}
		else if (!p_ptr->tim_sh_holy)
		{
#ifdef JP
msg_print("体が聖なるオーラで覆われた。");
#else
			msg_print("You have enveloped by holy aura!");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->tim_sh_holy)
		{
#ifdef JP
msg_print("聖なるオーラが消えた。");
#else
			msg_print("Holy aura disappeared.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_sh_holy = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}



/*
 * Set "p_ptr->tim_eyeeye", notice observable changes
 */
bool set_tim_eyeeye(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->tim_eyeeye && !do_dec)
		{
			if (p_ptr->tim_eyeeye > v) return FALSE;
		}
		else if (!p_ptr->tim_eyeeye)
		{
#ifdef JP
msg_print("法の守り手になった気がした！");
#else
			msg_print("You feel like a keeper of commandments!");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->tim_eyeeye)
		{
#ifdef JP
msg_print("懲罰を執行することができなくなった。");
#else
			msg_print("You no longer feel like a keeper.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_eyeeye = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}



/*
 * Set "p_ptr->resist_magic", notice observable changes
 */
bool set_resist_magic(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->resist_magic && !do_dec)
		{
			if (p_ptr->resist_magic > v) return FALSE;
		}
		else if (!p_ptr->resist_magic)
		{
#ifdef JP
msg_print("魔法への耐性がついた。");
#else
			msg_print("You have been protected from magic!");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->resist_magic)
		{
#ifdef JP
msg_print("魔法に弱くなった。");
#else
msg_print("You are no longer protected from magic.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->resist_magic = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->tim_reflect", notice observable changes
 */
bool set_tim_reflect(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->tim_reflect && !do_dec)
		{
			if (p_ptr->tim_reflect > v) return FALSE;
		}
		else if (!p_ptr->tim_reflect)
		{
#ifdef JP
msg_print("体の表面が滑かになった気がする。");
#else
			msg_print("Your body becames smooth.");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->tim_reflect)
		{
#ifdef JP
msg_print("体の表面が滑かでなくなった。");
#else
			msg_print("Your body is no longer smooth.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_reflect = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->multishadow", notice observable changes
 */
bool set_multishadow(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->multishadow && !do_dec)
		{
			if (p_ptr->multishadow > v) return FALSE;
		}
		else if (!p_ptr->multishadow)
		{
#ifdef JP
msg_print("あなたの周りに幻影が生まれた。");
#else
			msg_print("Your Shadow enveloped you.");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->multishadow)
		{
#ifdef JP
msg_print("幻影が消えた。");
#else
			msg_print("Your Shadow disappears.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->multishadow = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->dustrobe", notice observable changes
 */
bool set_dustrobe(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->dustrobe && !do_dec)
		{
			if (p_ptr->dustrobe > v) return FALSE;
		}
		else if (!p_ptr->dustrobe)
		{
#ifdef JP
msg_print("体が鏡のオーラで覆われた。");
#else
			msg_print("You were enveloped by mirror shards.");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->dustrobe)
		{
#ifdef JP
msg_print("鏡のオーラが消えた。");
#else
			msg_print("The mirror shards disappear.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->dustrobe = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->tim_regen", notice observable changes
 */
bool set_kabenuke(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->kabenuke && !do_dec)
		{
			if (p_ptr->kabenuke > v) return FALSE;
		}
		else if (!p_ptr->kabenuke)
		{
#ifdef JP
msg_print("体が半物質の状態になった。");
#else
			msg_print("You became ethereal form.");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->kabenuke)
		{
#ifdef JP
msg_print("体が物質化した。");
#else
			msg_print("You are no longer in an ethereal form.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->kabenuke = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


bool set_tsuyoshi(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->tsuyoshi && !do_dec)
		{
			if (p_ptr->tsuyoshi > v) return FALSE;
		}
		else if (!p_ptr->tsuyoshi)
		{
#ifdef JP
msg_print("「オクレ兄さん！」");
#else
			msg_print("Brother OKURE!");
#endif

			notice = TRUE;
			chg_virtue(V_VITALITY, 2);
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->tsuyoshi)
		{
#ifdef JP
msg_print("肉体が急速にしぼんでいった。");
#else
			msg_print("Your body had quickly shriveled.");
#endif

			(void)dec_stat(A_CON, 20, TRUE);
			(void)dec_stat(A_STR, 20, TRUE);

			notice = TRUE;
			chg_virtue(V_VITALITY, -3);
		}
	}

	/* Use the value */
	p_ptr->tsuyoshi = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Recalculate hitpoints */
	p_ptr->update |= (PU_HP);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set a temporary elemental brand.  Clear all other brands.  Print status 
 * messages. -LM-
 */
bool set_ele_attack(u32b attack_type, int v)
{
	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Clear all elemental attacks (only one is allowed at a time). */
	if ((p_ptr->special_attack & (ATTACK_ACID)) && (attack_type != ATTACK_ACID))
	{
		p_ptr->special_attack &= ~(ATTACK_ACID);
#ifdef JP
		msg_print("酸で攻撃できなくなった。");
#else
		msg_print("Your temporary acidic brand fades away.");
#endif
	}
	if ((p_ptr->special_attack & (ATTACK_ELEC)) && (attack_type != ATTACK_ELEC))
	{
		p_ptr->special_attack &= ~(ATTACK_ELEC);
#ifdef JP
		msg_print("電撃で攻撃できなくなった。");
#else
		msg_print("Your temporary electrical brand fades away.");
#endif
	}
	if ((p_ptr->special_attack & (ATTACK_FIRE)) && (attack_type != ATTACK_FIRE))
	{
		p_ptr->special_attack &= ~(ATTACK_FIRE);
#ifdef JP
		msg_print("火炎で攻撃できなくなった。");
#else
		msg_print("Your temporary fiery brand fades away.");
#endif
	}
	if ((p_ptr->special_attack & (ATTACK_COLD)) && (attack_type != ATTACK_COLD))
	{
		p_ptr->special_attack &= ~(ATTACK_COLD);
#ifdef JP
		msg_print("冷気で攻撃できなくなった。");
#else
		msg_print("Your temporary frost brand fades away.");
#endif
	}
	if ((p_ptr->special_attack & (ATTACK_POIS)) && (attack_type != ATTACK_POIS))
	{
		p_ptr->special_attack &= ~(ATTACK_POIS);
#ifdef JP
		msg_print("毒で攻撃できなくなった。");
#else
		msg_print("Your temporary poison brand fades away.");
#endif
	}

	if ((v) && (attack_type))
	{
		/* Set attack type. */
		p_ptr->special_attack |= (attack_type);

		/* Set duration. */
		p_ptr->ele_attack = v;

		/* Message. */
#ifdef JP
		msg_format("%sで攻撃できるようになった！",
			     ((attack_type == ATTACK_ACID) ? "酸" :
			      ((attack_type == ATTACK_ELEC) ? "電撃" :
			       ((attack_type == ATTACK_FIRE) ? "火炎" : 
				((attack_type == ATTACK_COLD) ? "冷気" : 
				 ((attack_type == ATTACK_POIS) ? "毒" : 
					"(なし)"))))));
#else
		msg_format("For a while, the blows you deal will %s",
			     ((attack_type == ATTACK_ACID) ? "melt with acid!" :
			      ((attack_type == ATTACK_ELEC) ? "shock your foes!" :
			       ((attack_type == ATTACK_FIRE) ? "burn with fire!" : 
				((attack_type == ATTACK_COLD) ? "chill to the bone!" : 
				 ((attack_type == ATTACK_POIS) ? "poison your enemies!" : 
					"do nothing special."))))));
#endif
	}

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	return (TRUE);
}


/*
 * Set a temporary elemental brand.  Clear all other brands.  Print status 
 * messages. -LM-
 */
bool set_ele_immune(u32b immune_type, int v)
{
	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	/* Clear all elemental attacks (only one is allowed at a time). */
	if ((p_ptr->special_defense & (DEFENSE_ACID)) && (immune_type != DEFENSE_ACID))
	{
		p_ptr->special_defense &= ~(DEFENSE_ACID);
#ifdef JP
		msg_print("酸の攻撃で傷つけられるようになった。。");
#else
		msg_print("You are no longer immune to acid.");
#endif
	}
	if ((p_ptr->special_defense & (DEFENSE_ELEC)) && (immune_type != DEFENSE_ELEC))
	{
		p_ptr->special_defense &= ~(DEFENSE_ELEC);
#ifdef JP
		msg_print("電撃の攻撃で傷つけられるようになった。。");
#else
		msg_print("You are no longer immune to electricity.");
#endif
	}
	if ((p_ptr->special_defense & (DEFENSE_FIRE)) && (immune_type != DEFENSE_FIRE))
	{
		p_ptr->special_defense &= ~(DEFENSE_FIRE);
#ifdef JP
		msg_print("火炎の攻撃で傷つけられるようになった。。");
#else
		msg_print("You are no longer immune to fire.");
#endif
	}
	if ((p_ptr->special_defense & (DEFENSE_COLD)) && (immune_type != DEFENSE_COLD))
	{
		p_ptr->special_defense &= ~(DEFENSE_COLD);
#ifdef JP
		msg_print("冷気の攻撃で傷つけられるようになった。。");
#else
		msg_print("You are no longer immune to cold.");
#endif
	}
	if ((p_ptr->special_defense & (DEFENSE_POIS)) && (immune_type != DEFENSE_POIS))
	{
		p_ptr->special_defense &= ~(DEFENSE_POIS);
#ifdef JP
		msg_print("毒の攻撃で傷つけられるようになった。。");
#else
		msg_print("You are no longer immune to poison.");
#endif
	}

	if ((v) && (immune_type))
	{
		/* Set attack type. */
		p_ptr->special_defense |= (immune_type);

		/* Set duration. */
		p_ptr->ele_immune = v;

		/* Message. */
#ifdef JP
		msg_format("%sの攻撃を受けつけなくなった！",
			     ((immune_type == DEFENSE_ACID) ? "酸" :
			      ((immune_type == DEFENSE_ELEC) ? "電撃" :
			       ((immune_type == DEFENSE_FIRE) ? "火炎" : 
				((immune_type == DEFENSE_COLD) ? "冷気" : 
				 ((immune_type == DEFENSE_POIS) ? "毒" : 
					"(なし)"))))));
#else
		msg_format("For a while, You are immune to %s",
			     ((immune_type == DEFENSE_ACID) ? "acid!" :
			      ((immune_type == DEFENSE_ELEC) ? "electricity!" :
			       ((immune_type == DEFENSE_FIRE) ? "fire!" : 
				((immune_type == DEFENSE_COLD) ? "cold!" : 
				 ((immune_type == DEFENSE_POIS) ? "poison!" : 
					"do nothing special."))))));
#endif
	}

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	return (TRUE);
}


/*
 * Set "p_ptr->oppose_acid", notice observable changes
 */
bool set_oppose_acid(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->oppose_acid && !do_dec)
		{
			if (p_ptr->oppose_acid > v) return FALSE;
		}
		else if (!IS_OPPOSE_ACID())
		{
#ifdef JP
msg_print("酸への耐性がついた気がする！");
#else
			msg_print("You feel resistant to acid!");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->oppose_acid && !music_singing(MUSIC_RESIST) && !(p_ptr->special_defense & KATA_MUSOU))
		{
#ifdef JP
msg_print("酸への耐性が薄れた気がする。");
#else
			msg_print("You feel less resistant to acid.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->oppose_acid = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->oppose_elec", notice observable changes
 */
bool set_oppose_elec(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->oppose_elec && !do_dec)
		{
			if (p_ptr->oppose_elec > v) return FALSE;
		}
		else if (!IS_OPPOSE_ELEC())
		{
#ifdef JP
msg_print("電撃への耐性がついた気がする！");
#else
			msg_print("You feel resistant to electricity!");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->oppose_elec && !music_singing(MUSIC_RESIST) && !(p_ptr->special_defense & KATA_MUSOU))
		{
#ifdef JP
msg_print("電撃への耐性が薄れた気がする。");
#else
			msg_print("You feel less resistant to electricity.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->oppose_elec = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->oppose_fire", notice observable changes
 */
bool set_oppose_fire(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	if ((prace_is_(RACE_DEMON) && (p_ptr->lev > 44)) || (p_ptr->mimic_form == MIMIC_DEMON)) v = 1;
	/* Open */
	if (v)
	{
		if (p_ptr->oppose_fire && !do_dec)
		{
			if (p_ptr->oppose_fire > v) return FALSE;
		}
		else if (!IS_OPPOSE_FIRE())
		{
#ifdef JP
msg_print("火への耐性がついた気がする！");
#else
			msg_print("You feel resistant to fire!");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->oppose_fire && !music_singing(MUSIC_RESIST) && !(p_ptr->special_defense & KATA_MUSOU))
		{
#ifdef JP
msg_print("火への耐性が薄れた気がする。");
#else
			msg_print("You feel less resistant to fire.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->oppose_fire = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->oppose_cold", notice observable changes
 */
bool set_oppose_cold(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->oppose_cold && !do_dec)
		{
			if (p_ptr->oppose_cold > v) return FALSE;
		}
		else if (!IS_OPPOSE_COLD())
		{
#ifdef JP
msg_print("冷気への耐性がついた気がする！");
#else
			msg_print("You feel resistant to cold!");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->oppose_cold && !music_singing(MUSIC_RESIST) && !(p_ptr->special_defense & KATA_MUSOU))
		{
#ifdef JP
msg_print("冷気への耐性が薄れた気がする。");
#else
			msg_print("You feel less resistant to cold.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->oppose_cold = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->oppose_pois", notice observable changes
 */
bool set_oppose_pois(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if ((p_ptr->pclass == CLASS_NINJA) && (p_ptr->lev > 44)) v = 1;
	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->oppose_pois && !do_dec)
		{
			if (p_ptr->oppose_pois > v) return FALSE;
		}
		else if (!IS_OPPOSE_POIS())
		{
#ifdef JP
msg_print("毒への耐性がついた気がする！");
#else
			msg_print("You feel resistant to poison!");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->oppose_pois && !music_singing(MUSIC_RESIST) && !(p_ptr->special_defense & KATA_MUSOU))
		{
#ifdef JP
msg_print("毒への耐性が薄れた気がする。");
#else
			msg_print("You feel less resistant to poison.");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->oppose_pois = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->stun", notice observable changes
 *
 * Note the special code to only notice "range" changes.
 */
bool set_stun(int v)
{
	int old_aux, new_aux;
	bool notice = FALSE;


	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	if (prace_is_(RACE_GOLEM) || ((p_ptr->pclass == CLASS_BERSERKER) && (p_ptr->lev > 34))) v = 0;

	/* Knocked out */
	if (p_ptr->stun > 100)
	{
		old_aux = 3;
	}

	/* Heavy stun */
	else if (p_ptr->stun > 50)
	{
		old_aux = 2;
	}

	/* Stun */
	else if (p_ptr->stun > 0)
	{
		old_aux = 1;
	}

	/* None */
	else
	{
		old_aux = 0;
	}

	/* Knocked out */
	if (v > 100)
	{
		new_aux = 3;
	}

	/* Heavy stun */
	else if (v > 50)
	{
		new_aux = 2;
	}

	/* Stun */
	else if (v > 0)
	{
		new_aux = 1;
	}

	/* None */
	else
	{
		new_aux = 0;
	}

	/* Increase cut */
	if (new_aux > old_aux)
	{
		/* Describe the state */
		switch (new_aux)
		{
			/* Stun */
			case 1:
#ifdef JP
msg_print("意識がもうろうとしてきた。");
#else
			msg_print("You have been stunned.");
#endif

			break;

			/* Heavy stun */
			case 2:
#ifdef JP
msg_print("意識がひどくもうろうとしてきた。");
#else
			msg_print("You have been heavily stunned.");
#endif

			break;

			/* Knocked out */
			case 3:
#ifdef JP
msg_print("頭がクラクラして意識が遠のいてきた。");
#else
			msg_print("You have been knocked out.");
#endif

			break;
		}

		if (randint1(1000) < v || one_in_(16))
		{
#ifdef JP
msg_print("割れるような頭痛がする。");
#else
			msg_print("A vicious blow hits your head.");
#endif

			if (one_in_(3))
			{
				if (!p_ptr->sustain_int) (void)do_dec_stat(A_INT);
				if (!p_ptr->sustain_wis) (void)do_dec_stat(A_WIS);
			}
			else if (one_in_(2))
			{
				if (!p_ptr->sustain_int) (void)do_dec_stat(A_INT);
			}
			else
			{
				if (!p_ptr->sustain_wis) (void)do_dec_stat(A_WIS);
			}
		}
		if (p_ptr->special_defense & KATA_MASK)
		{
#ifdef JP
			msg_print("型が崩れた。");
#else
			msg_print("Your posture gets loose.");
#endif
			p_ptr->special_defense &= ~(KATA_MASK);
			p_ptr->update |= (PU_BONUS);
			p_ptr->update |= (PU_MONSTERS);
			p_ptr->redraw |= (PR_STATE);
			p_ptr->redraw |= (PR_STATUS);
			p_ptr->action = ACTION_NONE;
		}

		/* Sniper */
		if (p_ptr->concent) reset_concentration(TRUE);

		/* Hex */
		if (hex_spelling_any()) stop_hex_spell_all();

		/* Notice */
		notice = TRUE;
	}

	/* Decrease cut */
	else if (new_aux < old_aux)
	{
		/* Describe the state */
		switch (new_aux)
		{
			/* None */
			case 0:
#ifdef JP
msg_print("やっと朦朧状態から回復した。");
#else
			msg_print("You are no longer stunned.");
#endif

			if (disturb_state) disturb(0, 0);
			break;
		}

		/* Notice */
		notice = TRUE;
	}

	/* Use the value */
	p_ptr->stun = v;

	/* No change */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Redraw the "stun" */
	p_ptr->redraw |= (PR_STUN);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->cut", notice observable changes
 *
 * Note the special code to only notice "range" changes.
 */
bool set_cut(int v)
{
	int old_aux, new_aux;
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	if ((p_ptr->prace == RACE_GOLEM ||
	    p_ptr->prace == RACE_SKELETON ||
	    p_ptr->prace == RACE_SPECTRE ||
		(p_ptr->prace == RACE_ZOMBIE && p_ptr->lev > 11)) &&
	    !p_ptr->mimic_form)
		v = 0;

	/* Mortal wound */
	if (p_ptr->cut > 1000)
	{
		old_aux = 7;
	}

	/* Deep gash */
	else if (p_ptr->cut > 200)
	{
		old_aux = 6;
	}

	/* Severe cut */
	else if (p_ptr->cut > 100)
	{
		old_aux = 5;
	}

	/* Nasty cut */
	else if (p_ptr->cut > 50)
	{
		old_aux = 4;
	}

	/* Bad cut */
	else if (p_ptr->cut > 25)
	{
		old_aux = 3;
	}

	/* Light cut */
	else if (p_ptr->cut > 10)
	{
		old_aux = 2;
	}

	/* Graze */
	else if (p_ptr->cut > 0)
	{
		old_aux = 1;
	}

	/* None */
	else
	{
		old_aux = 0;
	}

	/* Mortal wound */
	if (v > 1000)
	{
		new_aux = 7;
	}

	/* Deep gash */
	else if (v > 200)
	{
		new_aux = 6;
	}

	/* Severe cut */
	else if (v > 100)
	{
		new_aux = 5;
	}

	/* Nasty cut */
	else if (v > 50)
	{
		new_aux = 4;
	}

	/* Bad cut */
	else if (v > 25)
	{
		new_aux = 3;
	}

	/* Light cut */
	else if (v > 10)
	{
		new_aux = 2;
	}

	/* Graze */
	else if (v > 0)
	{
		new_aux = 1;
	}

	/* None */
	else
	{
		new_aux = 0;
	}

	/* Increase cut */
	if (new_aux > old_aux)
	{
		/* Describe the state */
		switch (new_aux)
		{
			/* Graze */
			case 1:
#ifdef JP
msg_print("かすり傷を負ってしまった。");
#else
			msg_print("You have been given a graze.");
#endif

			break;

			/* Light cut */
			case 2:
#ifdef JP
msg_print("軽い傷を負ってしまった。");
#else
			msg_print("You have been given a light cut.");
#endif

			break;

			/* Bad cut */
			case 3:
#ifdef JP
msg_print("ひどい傷を負ってしまった。");
#else
			msg_print("You have been given a bad cut.");
#endif

			break;

			/* Nasty cut */
			case 4:
#ifdef JP
msg_print("大変な傷を負ってしまった。");
#else
			msg_print("You have been given a nasty cut.");
#endif

			break;

			/* Severe cut */
			case 5:
#ifdef JP
msg_print("重大な傷を負ってしまった。");
#else
			msg_print("You have been given a severe cut.");
#endif

			break;

			/* Deep gash */
			case 6:
#ifdef JP
msg_print("ひどい深手を負ってしまった。");
#else
			msg_print("You have been given a deep gash.");
#endif

			break;

			/* Mortal wound */
			case 7:
#ifdef JP
msg_print("致命的な傷を負ってしまった。");
#else
			msg_print("You have been given a mortal wound.");
#endif

			break;
		}

		/* Notice */
		notice = TRUE;

		if (randint1(1000) < v || one_in_(16))
		{
			if (!p_ptr->sustain_chr)
			{
#ifdef JP
msg_print("ひどい傷跡が残ってしまった。");
#else
				msg_print("You have been horribly scarred.");
#endif


				do_dec_stat(A_CHR);
			}
		}
	}

	/* Decrease cut */
	else if (new_aux < old_aux)
	{
		/* Describe the state */
		switch (new_aux)
		{
			/* None */
			case 0:
#ifdef JP
msg_format("やっと%s。", p_ptr->prace == RACE_ANDROID ? "怪我が直った" : "出血が止まった");
#else
			msg_print("You are no longer bleeding.");
#endif

			if (disturb_state) disturb(0, 0);
			break;
		}

		/* Notice */
		notice = TRUE;
	}

	/* Use the value */
	p_ptr->cut = v;

	/* No change */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Redraw the "cut" */
	p_ptr->redraw |= (PR_CUT);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Set "p_ptr->food", notice observable changes
 *
 * The "p_ptr->food" variable can get as large as 20000, allowing the
 * addition of the most "filling" item, Elvish Waybread, which adds
 * 7500 food units, without overflowing the 32767 maximum limit.
 *
 * Perhaps we should disturb the player with various messages,
 * especially messages about hunger status changes.  XXX XXX XXX
 *
 * Digestion of food is handled in "dungeon.c", in which, normally,
 * the player digests about 20 food units per 100 game turns, more
 * when "fast", more when "regenerating", less with "slow digestion",
 * but when the player is "gorged", he digests 100 food units per 10
 * game turns, or a full 1000 food units per 100 game turns.
 *
 * Note that the player's speed is reduced by 10 units while gorged,
 * so if the player eats a single food ration (5000 food units) when
 * full (15000 food units), he will be gorged for (5000/100)*10 = 500
 * game turns, or 500/(100/5) = 25 player turns (if nothing else is
 * affecting the player speed).
 */
bool set_food(int v)
{
	int old_aux, new_aux;

	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 20000) ? 20000 : (v < 0) ? 0 : v;

	/* Fainting / Starving */
	if (p_ptr->food < PY_FOOD_FAINT)
	{
		old_aux = 0;
	}

	/* Weak */
	else if (p_ptr->food < PY_FOOD_WEAK)
	{
		old_aux = 1;
	}

	/* Hungry */
	else if (p_ptr->food < PY_FOOD_ALERT)
	{
		old_aux = 2;
	}

	/* Normal */
	else if (p_ptr->food < PY_FOOD_FULL)
	{
		old_aux = 3;
	}

	/* Full */
	else if (p_ptr->food < PY_FOOD_MAX)
	{
		old_aux = 4;
	}

	/* Gorged */
	else
	{
		old_aux = 5;
	}

	/* Fainting / Starving */
	if (v < PY_FOOD_FAINT)
	{
		new_aux = 0;
	}

	/* Weak */
	else if (v < PY_FOOD_WEAK)
	{
		new_aux = 1;
	}

	/* Hungry */
	else if (v < PY_FOOD_ALERT)
	{
		new_aux = 2;
	}

	/* Normal */
	else if (v < PY_FOOD_FULL)
	{
		new_aux = 3;
	}

	/* Full */
	else if (v < PY_FOOD_MAX)
	{
		new_aux = 4;
	}

	/* Gorged */
	else
	{
		new_aux = 5;
	}

	if (old_aux < 1 && new_aux > 0)
		chg_virtue(V_PATIENCE, 2);
	else if (old_aux < 3 && (old_aux != new_aux))
		chg_virtue(V_PATIENCE, 1);
	if (old_aux == 2)
		chg_virtue(V_TEMPERANCE, 1);
	if (old_aux == 0)
		chg_virtue(V_TEMPERANCE, -1);

	/* Food increase */
	if (new_aux > old_aux)
	{
		/* Describe the state */
		switch (new_aux)
		{
			/* Weak */
			case 1:
#ifdef JP
msg_print("まだ空腹で倒れそうだ。");
#else
			msg_print("You are still weak.");
#endif

			break;

			/* Hungry */
			case 2:
#ifdef JP
msg_print("まだ空腹だ。");
#else
			msg_print("You are still hungry.");
#endif

			break;

			/* Normal */
			case 3:
#ifdef JP
msg_print("空腹感がおさまった。");
#else
			msg_print("You are no longer hungry.");
#endif

			break;

			/* Full */
			case 4:
#ifdef JP
msg_print("満腹だ！");
#else
			msg_print("You are full!");
#endif

			break;

			/* Bloated */
			case 5:
#ifdef JP
msg_print("食べ過ぎだ！");
#else
			msg_print("You have gorged yourself!");
#endif
			chg_virtue(V_HARMONY, -1);
			chg_virtue(V_PATIENCE, -1);
			chg_virtue(V_TEMPERANCE, -2);

			break;
		}

		/* Change */
		notice = TRUE;
	}

	/* Food decrease */
	else if (new_aux < old_aux)
	{
		/* Describe the state */
		switch (new_aux)
		{
			/* Fainting / Starving */
			case 0:
#ifdef JP
msg_print("あまりにも空腹で気を失ってしまった！");
#else
			msg_print("You are getting faint from hunger!");
#endif

			break;

			/* Weak */
			case 1:
#ifdef JP
msg_print("お腹が空いて倒れそうだ。");
#else
			msg_print("You are getting weak from hunger!");
#endif

			break;

			/* Hungry */
			case 2:
#ifdef JP
msg_print("お腹が空いてきた。");
#else
			msg_print("You are getting hungry.");
#endif

			break;

			/* Normal */
			case 3:
#ifdef JP
msg_print("満腹感がなくなった。");
#else
			msg_print("You are no longer full.");
#endif

			break;

			/* Full */
			case 4:
#ifdef JP
msg_print("やっとお腹がきつくなくなった。");
#else
			msg_print("You are no longer gorged.");
#endif

			break;
		}

		if (p_ptr->wild_mode && (new_aux < 2))
		{
			change_wild_mode();
		}

		/* Change */
		notice = TRUE;
	}

	/* Use the value */
	p_ptr->food = v;

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Redraw hunger */
	p_ptr->redraw |= (PR_HUNGER);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}

/*
 * Increases a stat by one randomized level             -RAK-
 *
 * Note that this function (used by stat potions) now restores
 * the stat BEFORE increasing it.
 */
bool inc_stat(int stat)
{
	int value, gain;

	/* Then augment the current/max stat */
	value = p_ptr->stat_cur[stat];

	/* Cannot go above 18/100 */
	if (value < p_ptr->stat_max_max[stat])
	{
		/* Gain one (sometimes two) points */
		if (value < 18)
		{
			gain = ((randint0(100) < 75) ? 1 : 2);
			value += gain;
		}

		/* Gain 1/6 to 1/3 of distance to 18/100 */
		else if (value < (p_ptr->stat_max_max[stat]-2))
		{
			/* Approximate gain value */
			gain = (((p_ptr->stat_max_max[stat]) - value) / 2 + 3) / 2;

			/* Paranoia */
			if (gain < 1) gain = 1;

			/* Apply the bonus */
			value += randint1(gain) + gain / 2;

			/* Maximal value */
			if (value > (p_ptr->stat_max_max[stat]-1)) value = p_ptr->stat_max_max[stat]-1;
		}

		/* Gain one point at a time */
		else
		{
			value++;
		}

		/* Save the new value */
		p_ptr->stat_cur[stat] = value;

		/* Bring up the maximum too */
		if (value > p_ptr->stat_max[stat])
		{
			p_ptr->stat_max[stat] = value;
		}

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Success */
		return (TRUE);
	}

	/* Nothing to gain */
	return (FALSE);
}



/*
 * Decreases a stat by an amount indended to vary from 0 to 100 percent.
 *
 * Amount could be a little higher in extreme cases to mangle very high
 * stats from massive assaults.  -CWS
 *
 * Note that "permanent" means that the *given* amount is permanent,
 * not that the new value becomes permanent.  This may not work exactly
 * as expected, due to "weirdness" in the algorithm, but in general,
 * if your stat is already drained, the "max" value will not drop all
 * the way down to the "cur" value.
 */
bool dec_stat(int stat, int amount, int permanent)
{
	int cur, max, loss, same, res = FALSE;


	/* Acquire current value */
	cur = p_ptr->stat_cur[stat];
	max = p_ptr->stat_max[stat];

	/* Note when the values are identical */
	same = (cur == max);

	/* Damage "current" value */
	if (cur > 3)
	{
		/* Handle "low" values */
		if (cur <= 18)
		{
			if (amount > 90) cur--;
			if (amount > 50) cur--;
			if (amount > 20) cur--;
			cur--;
		}

		/* Handle "high" values */
		else
		{
			/* Hack -- Decrement by a random amount between one-quarter */
			/* and one-half of the stat bonus times the percentage, with a */
			/* minimum damage of half the percentage. -CWS */
			loss = (((cur-18) / 2 + 1) / 2 + 1);

			/* Paranoia */
			if (loss < 1) loss = 1;

			/* Randomize the loss */
			loss = ((randint1(loss) + loss) * amount) / 100;

			/* Maximal loss */
			if (loss < amount/2) loss = amount/2;

			/* Lose some points */
			cur = cur - loss;

			/* Hack -- Only reduce stat to 17 sometimes */
			if (cur < 18) cur = (amount <= 20) ? 18 : 17;
		}

		/* Prevent illegal values */
		if (cur < 3) cur = 3;

		/* Something happened */
		if (cur != p_ptr->stat_cur[stat]) res = TRUE;
	}

	/* Damage "max" value */
	if (permanent && (max > 3))
	{
		chg_virtue(V_SACRIFICE, 1);
		if (stat == A_WIS || stat == A_INT)
			chg_virtue(V_ENLIGHTEN, -2);

		/* Handle "low" values */
		if (max <= 18)
		{
			if (amount > 90) max--;
			if (amount > 50) max--;
			if (amount > 20) max--;
			max--;
		}

		/* Handle "high" values */
		else
		{
			/* Hack -- Decrement by a random amount between one-quarter */
			/* and one-half of the stat bonus times the percentage, with a */
			/* minimum damage of half the percentage. -CWS */
			loss = (((max-18) / 2 + 1) / 2 + 1);
			loss = ((randint1(loss) + loss) * amount) / 100;
			if (loss < amount/2) loss = amount/2;

			/* Lose some points */
			max = max - loss;

			/* Hack -- Only reduce stat to 17 sometimes */
			if (max < 18) max = (amount <= 20) ? 18 : 17;
		}

		/* Hack -- keep it clean */
		if (same || (max < cur)) max = cur;

		/* Something happened */
		if (max != p_ptr->stat_max[stat]) res = TRUE;
	}

	/* Apply changes */
	if (res)
	{
		/* Actually set the stat to its new value. */
		p_ptr->stat_cur[stat] = cur;
		p_ptr->stat_max[stat] = max;

		/* Redisplay the stats later */
		p_ptr->redraw |= (PR_STATS);

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);
	}

	/* Done */
	return (res);
}


/*
 * Restore a stat.  Return TRUE only if this actually makes a difference.
 */
bool res_stat(int stat)
{
	/* Restore if needed */
	if (p_ptr->stat_cur[stat] != p_ptr->stat_max[stat])
	{
		/* Restore */
		p_ptr->stat_cur[stat] = p_ptr->stat_max[stat];

		/* Recalculate bonuses */
		p_ptr->update |= (PU_BONUS);

		/* Redisplay the stats later */
		p_ptr->redraw |= (PR_STATS);

		/* Success */
		return (TRUE);
	}

	/* Nothing to restore */
	return (FALSE);
}


/*
 * Increase players hit points, notice effects
 */
bool hp_player(int num)
{
	int vir;
	vir = virtue_number(V_VITALITY);
	if (vir)
	{
		num = num * (p_ptr->virtues[vir - 1] + 1250) / 1250;
	}
	/* Healing needed */
	if (p_ptr->chp < p_ptr->mhp)
	{
		if ((num > 0) && (p_ptr->chp < (p_ptr->mhp/3)))
			chg_virtue(V_TEMPERANCE, 1);
		/* Gain hitpoints */
		p_ptr->chp += num;

		/* Enforce maximum */
		if (p_ptr->chp >= p_ptr->mhp)
		{
			p_ptr->chp = p_ptr->mhp;
			p_ptr->chp_frac = 0;
		}

		/* Redraw */
		p_ptr->redraw |= (PR_HP);

		/* Window stuff */
		p_ptr->window |= (PW_PLAYER);

		/* Heal 0-4 */
		if (num < 5)
		{
#ifdef JP
msg_print("少し気分が良くなった。");
#else
			msg_print("You feel a little better.");
#endif

		}

		/* Heal 5-14 */
		else if (num < 15)
		{
#ifdef JP
msg_print("気分が良くなった。");
#else
			msg_print("You feel better.");
#endif

		}

		/* Heal 15-34 */
		else if (num < 35)
		{
#ifdef JP
msg_print("とても気分が良くなった。");
#else
			msg_print("You feel much better.");
#endif

		}

		/* Heal 35+ */
		else
		{
#ifdef JP
msg_print("ひじょうに気分が良くなった。");
#else
			msg_print("You feel very good.");
#endif

		}

		/* Notice */
		return (TRUE);
	}

	/* Ignore */
	return (FALSE);
}


/*
 * Array of stat "descriptions"
 */
static cptr desc_stat_pos[] =
{
#ifdef JP
"強く",
#else
	"strong",
#endif

#ifdef JP
"知的に",
#else
	"smart",
#endif

#ifdef JP
"賢く",
#else
	"wise",
#endif

#ifdef JP
"器用に",
#else
	"dextrous",
#endif

#ifdef JP
"健康に",
#else
	"healthy",
#endif

#ifdef JP
"美しく"
#else
	"cute"
#endif

};


/*
 * Array of stat "descriptions"
 */
static cptr desc_stat_neg[] =
{
#ifdef JP
"弱く",
"無知に",
"愚かに",
"不器用に",
"不健康に",
"醜く"
#else
	"weak",
	"stupid",
	"naive",
	"clumsy",
	"sickly",
	"ugly"
#endif

};


/*
 * Lose a "point"
 */
bool do_dec_stat(int stat)
{
	bool sust = FALSE;

	/* Access the "sustain" */
	switch (stat)
	{
		case A_STR: if (p_ptr->sustain_str) sust = TRUE; break;
		case A_INT: if (p_ptr->sustain_int) sust = TRUE; break;
		case A_WIS: if (p_ptr->sustain_wis) sust = TRUE; break;
		case A_DEX: if (p_ptr->sustain_dex) sust = TRUE; break;
		case A_CON: if (p_ptr->sustain_con) sust = TRUE; break;
		case A_CHR: if (p_ptr->sustain_chr) sust = TRUE; break;
	}

	/* Sustain */
	if (sust && (!ironman_nightmare || randint0(13)))
	{
		/* Message */
#ifdef JP
msg_format("%sなった気がしたが、すぐに元に戻った。",
#else
		msg_format("You feel %s for a moment, but the feeling passes.",
#endif

				desc_stat_neg[stat]);

		/* Notice effect */
		return (TRUE);
	}

	/* Attempt to reduce the stat */
	if (dec_stat(stat, 10, (ironman_nightmare && !randint0(13))))
	{
		/* Message */
#ifdef JP
msg_format("ひどく%sなった気がする。", desc_stat_neg[stat]);
#else
		msg_format("You feel very %s.", desc_stat_neg[stat]);
#endif


		/* Notice effect */
		return (TRUE);
	}

	/* Nothing obvious */
	return (FALSE);
}


/*
 * Restore lost "points" in a stat
 */
bool do_res_stat(int stat)
{
	/* Attempt to increase */
	if (res_stat(stat))
	{
		/* Message */
#ifdef JP
msg_format("元通りに%sなった気がする。", desc_stat_pos[stat]);
#else
		msg_format("You feel less %s.", desc_stat_neg[stat]);
#endif


		/* Notice */
		return (TRUE);
	}

	/* Nothing obvious */
	return (FALSE);
}


/*
 * Gain a "point" in a stat
 */
bool do_inc_stat(int stat)
{
	bool res;

	/* Restore strength */
	res = res_stat(stat);

	/* Attempt to increase */
	if (inc_stat(stat))
	{
		if (stat == A_WIS)
		{
			chg_virtue(V_ENLIGHTEN, 1);
			chg_virtue(V_FAITH, 1);
		}
		else if (stat == A_INT)
		{
			chg_virtue(V_KNOWLEDGE, 1);
			chg_virtue(V_ENLIGHTEN, 1);
		}
		else if (stat == A_CON)
			chg_virtue(V_VITALITY, 1);

		/* Message */
#ifdef JP
msg_format("ワーオ！とても%sなった！", desc_stat_pos[stat]);
#else
		msg_format("Wow!  You feel very %s!", desc_stat_pos[stat]);
#endif


		/* Notice */
		return (TRUE);
	}

	/* Restoration worked */
	if (res)
	{
		/* Message */
#ifdef JP
msg_format("元通りに%sなった気がする。", desc_stat_pos[stat]);
#else
		msg_format("You feel less %s.", desc_stat_neg[stat]);
#endif


		/* Notice */
		return (TRUE);
	}

	/* Nothing obvious */
	return (FALSE);
}


/*
 * Restores any drained experience
 */
bool restore_level(void)
{
	/* Restore experience */
	if (p_ptr->exp < p_ptr->max_exp)
	{
		/* Message */
#ifdef JP
msg_print("生命力が戻ってきた気がする。");
#else
		msg_print("You feel your life energies returning.");
#endif


		/* Restore the experience */
		p_ptr->exp = p_ptr->max_exp;

		/* Check the experience */
		check_experience();

		/* Did something */
		return (TRUE);
	}

	/* No effect */
	return (FALSE);
}


/*
 * Forget everything
 */
bool lose_all_info(void)
{
	int i;

	chg_virtue(V_KNOWLEDGE, -5);
	chg_virtue(V_ENLIGHTEN, -5);

	/* Forget info about objects */
	for (i = 0; i < INVEN_TOTAL; i++)
	{
		object_type *o_ptr = &inventory[i];

		/* Skip non-objects */
		if (!o_ptr->k_idx) continue;

		/* Allow "protection" by the MENTAL flag */
		if (o_ptr->ident & (IDENT_MENTAL)) continue;

		/* Remove "default inscriptions" */
		o_ptr->feeling = FEEL_NONE;

		/* Hack -- Clear the "empty" flag */
		o_ptr->ident &= ~(IDENT_EMPTY);

		/* Hack -- Clear the "known" flag */
		o_ptr->ident &= ~(IDENT_KNOWN);

		/* Hack -- Clear the "felt" flag */
		o_ptr->ident &= ~(IDENT_SENSE);
	}

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Combine / Reorder the pack (later) */
	p_ptr->notice |= (PN_COMBINE | PN_REORDER);

	/* Window stuff */
	p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

	/* Mega-Hack -- Forget the map */
	wiz_dark();

	/* It worked */
	return (TRUE);
}


void do_poly_wounds(void)
{
	/* Changed to always provide at least _some_ healing */
	s16b wounds = p_ptr->cut;
	s16b hit_p = (p_ptr->mhp - p_ptr->chp);
	s16b change = damroll(p_ptr->lev, 5);
	bool Nasty_effect = one_in_(5);

	if (!(wounds || hit_p || Nasty_effect)) return;

#ifdef JP
msg_print("傷がより軽いものに変化した。");
#else
	msg_print("Your wounds are polymorphed into less serious ones.");
#endif

	hp_player(change);
	if (Nasty_effect)
	{
#ifdef JP
msg_print("新たな傷ができた！");
take_hit(DAMAGE_LOSELIFE, change / 2, "変化した傷", -1);
#else
		msg_print("A new wound was created!");
		take_hit(DAMAGE_LOSELIFE, change / 2, "a polymorphed wound", -1);
#endif

		set_cut(change);
	}
	else
	{
		set_cut(p_ptr->cut - (change / 2));
	}
}


/*
 * Change player race
 */
void change_race(int new_race, cptr effect_msg)
{
	cptr title = race_info[new_race].title;
	int  old_race = p_ptr->prace;

#ifdef JP
	msg_format("あなたは%s%sに変化した！", effect_msg, title);
#else
	msg_format("You turn into %s %s%s!", (!effect_msg[0] && is_a_vowel(title[0]) ? "an" : "a"), effect_msg, title);
#endif

	chg_virtue(V_CHANCE, 2);

	if (p_ptr->prace < 32)
	{
		p_ptr->old_race1 |= 1L << p_ptr->prace;
	}
	else
	{
		p_ptr->old_race2 |= 1L << (p_ptr->prace-32);
	}
	p_ptr->prace = new_race;
	rp_ptr = &race_info[p_ptr->prace];

	/* Experience factor */
	p_ptr->expfact = rp_ptr->r_exp + cp_ptr->c_exp;

	/*
	 * The speed bonus of Klackons and Sprites are disabled
	 * and the experience penalty is decreased.
	 */
	if (((p_ptr->pclass == CLASS_MONK) || (p_ptr->pclass == CLASS_FORCETRAINER) || (p_ptr->pclass == CLASS_NINJA)) && ((p_ptr->prace == RACE_KLACKON) || (p_ptr->prace == RACE_SPRITE)))
		p_ptr->expfact -= 15;

	/* Get character's height and weight */
	get_height_weight();

	/* Hitdice */
	if (p_ptr->pclass == CLASS_SORCERER)
		p_ptr->hitdie = rp_ptr->r_mhp/2 + cp_ptr->c_mhp + ap_ptr->a_mhp;
	else
		p_ptr->hitdie = rp_ptr->r_mhp + cp_ptr->c_mhp + ap_ptr->a_mhp;

	do_cmd_rerate(FALSE);

	/* The experience level may be modified */
	check_experience();

	p_ptr->redraw |= (PR_BASIC);

	p_ptr->update |= (PU_BONUS);

	handle_stuff();

	/* Load an autopick preference file */
	if (old_race != p_ptr->prace) autopick_load_pref(FALSE);

	/* Player's graphic tile may change */
	lite_spot(py, px);
}


void do_poly_self(void)
{
	int power = p_ptr->lev;

#ifdef JP
msg_print("あなたは変化の訪れを感じた...");
#else
	msg_print("You feel a change coming over you...");
#endif

	chg_virtue(V_CHANCE, 1);

	if ((power > randint0(20)) && one_in_(3) && (p_ptr->prace != RACE_ANDROID))
	{
		char effect_msg[80] = "";
		int new_race, expfact, goalexpfact;

		/* Some form of racial polymorph... */
		power -= 10;

		if ((power > randint0(5)) && one_in_(4))
		{
			/* sex change */
			power -= 2;

			if (p_ptr->psex == SEX_MALE)
			{
				p_ptr->psex = SEX_FEMALE;
				sp_ptr = &sex_info[p_ptr->psex];
#ifdef JP
sprintf(effect_msg, "女性の");
#else
				sprintf(effect_msg, "female ");
#endif

			}
			else
			{
				p_ptr->psex = SEX_MALE;
				sp_ptr = &sex_info[p_ptr->psex];
#ifdef JP
sprintf(effect_msg, "男性の");
#else
				sprintf(effect_msg, "male ");
#endif

			}
		}

		if ((power > randint0(30)) && one_in_(5))
		{
			int tmp = 0;

			/* Harmful deformity */
			power -= 15;

			while (tmp < 6)
			{
				if (one_in_(2))
				{
					(void)dec_stat(tmp, randint1(6) + 6, one_in_(3));
					power -= 1;
				}
				tmp++;
			}

			/* Deformities are discriminated against! */
			(void)dec_stat(A_CHR, randint1(6), TRUE);

			if (effect_msg[0])
			{
				char tmp_msg[10];
#ifdef JP
				sprintf(tmp_msg,"%s",effect_msg);
				sprintf(effect_msg,"奇形の%s",tmp_msg);
#else
				sprintf(tmp_msg,"%s ",effect_msg);
				sprintf(effect_msg,"deformed %s ",tmp_msg);
#endif

			}
			else
			{
#ifdef JP
				sprintf(effect_msg,"奇形の");
#else
				sprintf(effect_msg,"deformed ");
#endif

			}
		}

		while ((power > randint0(20)) && one_in_(10))
		{
			/* Polymorph into a less mutated form */
			power -= 10;

			if (!lose_mutation(0))
#ifdef JP
msg_print("奇妙なくらい普通になった気がする。");
#else
				msg_print("You feel oddly normal.");
#endif

		}

		/*
		 * Restrict the race choices by exp penalty so
		 * weak polymorph always means weak race
		 */
		if (power < 0)
			goalexpfact = 100;
		else
			goalexpfact = 100 + 3 * randint0(power);

		do
		{
			new_race = randint0(MAX_RACES);
			expfact = race_info[new_race].r_exp;
		}
		while (((new_race == p_ptr->prace) && (expfact > goalexpfact)) || (new_race == RACE_ANDROID));

		change_race(new_race, effect_msg);
	}

	if ((power > randint0(30)) && one_in_(6))
	{
		int tmp = 0;

		/* Abomination! */
		power -= 20;

#ifdef JP
msg_format("%sの構成が変化した！", p_ptr->prace == RACE_ANDROID ? "機械" : "内臓");
#else
		msg_print("Your internal organs are rearranged!");
#endif

		while (tmp < 6)
		{
			(void)dec_stat(tmp, randint1(6) + 6, one_in_(3));
			tmp++;
		}
		if (one_in_(6))
		{
#ifdef JP
			msg_print("現在の姿で生きていくのは困難なようだ！");
			take_hit(DAMAGE_LOSELIFE, damroll(randint1(10), p_ptr->lev), "致命的な突然変異", -1);
#else
			msg_print("You find living difficult in your present form!");
			take_hit(DAMAGE_LOSELIFE, damroll(randint1(10), p_ptr->lev), "a lethal mutation", -1);
#endif

			power -= 10;
		}
	}

	if ((power > randint0(20)) && one_in_(4))
	{
		power -= 10;

		get_max_stats();
		do_cmd_rerate(FALSE);
	}

	while ((power > randint0(15)) && one_in_(3))
	{
		power -= 7;
		(void)gain_random_mutation(0);
	}

	if (power > randint0(5))
	{
		power -= 5;
		do_poly_wounds();
	}

	/* Note: earlier deductions may have left power < 0 already. */
	while (power > 0)
	{
		mutate_player();
		power--;
	}
}


/*
 * Decreases players hit points and sets death flag if necessary
 *
 * XXX XXX XXX Invulnerability needs to be changed into a "shield"
 *
 * XXX XXX XXX Hack -- this function allows the user to save (or quit)
 * the game when he dies, since the "You die." message is shown before
 * setting the player to "dead".
 */

int take_hit(int damage_type, int damage, cptr hit_from, int monspell)
{
	int old_chp = p_ptr->chp;

	char death_message[1024];
	char tmp[80];

	int warning = (p_ptr->mhp * hitpoint_warn / 10);

	/* Paranoia */
	if (p_ptr->is_dead) return 0;

	if (p_ptr->sutemi) damage *= 2;
	if (p_ptr->special_defense & KATA_IAI) damage += (damage + 4) / 5;

	if (easy_band) damage = (damage+1)/2;

	if (damage_type != DAMAGE_USELIFE)
	{
		/* Disturb */
		disturb(1, 0);
		if (auto_more)
		{
			now_damaged = TRUE;
		}
	}

	if (monspell >= 0) learn_spell(monspell);

	/* Mega-Hack -- Apply "invulnerability" */
	if ((damage_type != DAMAGE_USELIFE) && (damage_type != DAMAGE_LOSELIFE))
	{
		if (IS_INVULN() && (damage < 9000))
		{
			if (damage_type == DAMAGE_FORCE)
			{
#ifdef JP
				msg_print("バリアが切り裂かれた！");
#else
				msg_print("The attack cuts your shield of invulnerability open!");
#endif
			}
			else if (one_in_(PENETRATE_INVULNERABILITY))
			{
#ifdef JP
				msg_print("無敵のバリアを破って攻撃された！");
#else
				msg_print("The attack penetrates your shield of invulnerability!");
#endif
			}
			else
			{
				return 0;
			}
		}

		if (CHECK_MULTISHADOW())
		{
			if (damage_type == DAMAGE_FORCE)
			{
#ifdef JP
				msg_print("幻影もろとも体が切り裂かれた！");
#else
				msg_print("The attack hits Shadow together with you!");
#endif
			}
			else if (damage_type == DAMAGE_ATTACK)
			{
#ifdef JP
				msg_print("攻撃は幻影に命中し、あなたには届かなかった。");
#else
				msg_print("The attack hits Shadow, you are unharmed!");
#endif
				return 0;
			}
		}

		if (p_ptr->wraith_form)
		{
			if (damage_type == DAMAGE_FORCE)
			{
#ifdef JP
				msg_print("半物質の体が切り裂かれた！");
#else
				msg_print("The attack cuts through your ethereal body!");
#endif
			}
			else
			{
				damage /= 2;
				if ((damage == 0) && one_in_(2)) damage = 1;
			}
		}

		if (p_ptr->special_defense & KATA_MUSOU)
		{
			damage /= 2;
			if ((damage == 0) && one_in_(2)) damage = 1;
		}
	} /* not if LOSELIFE USELIFE */

	/* Hurt the player */
	p_ptr->chp -= damage;
	if(damage_type == DAMAGE_GENO && p_ptr->chp < 0)
	{
		damage += p_ptr->chp;
		p_ptr->chp = 0;
	}

	/* Display the hitpoints */
	p_ptr->redraw |= (PR_HP);

	/* Window stuff */
	p_ptr->window |= (PW_PLAYER);

	handle_stuff();

	if (damage_type != DAMAGE_GENO && p_ptr->chp == 0)
	{
		chg_virtue(V_SACRIFICE, 1);
		chg_virtue(V_CHANCE, 2);
	}

	/* Dead player */
	if (p_ptr->chp < 0)
	{
		bool android = (p_ptr->prace == RACE_ANDROID ? TRUE : FALSE);

#ifdef JP       /* 死んだ時に強制終了して死を回避できなくしてみた by Habu */
		if (!cheat_save)
			if(!save_player()) msg_print("セーブ失敗！");
#endif

		/* Sound */
		sound(SOUND_DEATH);

		chg_virtue(V_SACRIFICE, 10);

		/* Leaving */
		p_ptr->leaving = TRUE;

		/* Note death */
		p_ptr->is_dead = TRUE;

		if (p_ptr->inside_arena)
		{
			cptr m_name = r_name+r_info[arena_info[p_ptr->arena_number].r_idx].name;
#ifdef JP
			msg_format("あなたは%sの前に敗れ去った。", m_name);
#else
			msg_format("You are beaten by %s.", m_name);
#endif
			msg_print(NULL);
			if (record_arena) do_cmd_write_nikki(NIKKI_ARENA, -1 - p_ptr->arena_number, m_name);
		}
		else
		{
			int q_idx = quest_number(dun_level);
			bool seppuku = streq(hit_from, "Seppuku");
			bool winning_seppuku = p_ptr->total_winner && seppuku;

#ifdef WORLD_SCORE
			/* Make screen dump */
			screen_dump = make_screen_dump();
#endif

			/* Note cause of death */
			if (seppuku)
			{
				strcpy(p_ptr->died_from, hit_from);
#ifdef JP
				if (!winning_seppuku) strcpy(p_ptr->died_from, "切腹");
#endif
			}
			else
			{
				char dummy[1024];
#ifdef JP
				sprintf(dummy, "%s%s%s", !p_ptr->paralyzed ? "" : p_ptr->free_act ? "彫像状態で" : "麻痺状態で", p_ptr->image ? "幻覚に歪んだ" : "", hit_from);
#else
				sprintf(dummy, "%s%s", hit_from, !p_ptr->paralyzed ? "" : " while helpless");
#endif
				my_strcpy(p_ptr->died_from, dummy, sizeof p_ptr->died_from);
			}

			/* No longer a winner */
			p_ptr->total_winner = FALSE;

			if (winning_seppuku)
			{
#ifdef JP
				do_cmd_write_nikki(NIKKI_BUNSHOU, 0, "勝利の後切腹した。");
#else
				do_cmd_write_nikki(NIKKI_BUNSHOU, 0, "did Seppuku after the winning.");
#endif
			}
			else
			{
				char buf[20];

				if (p_ptr->inside_arena)
#ifdef JP
					strcpy(buf,"アリーナ");
#else
					strcpy(buf,"in the Arena");
#endif
				else if (!dun_level)
#ifdef JP
					strcpy(buf,"地上");
#else
					strcpy(buf,"on the surface");
#endif
				else if (q_idx && (is_fixed_quest_idx(q_idx) &&
				         !((q_idx == QUEST_OBERON) || (q_idx == QUEST_SERPENT))))
#ifdef JP
					strcpy(buf,"クエスト");
#else
					strcpy(buf,"in a quest");
#endif
				else
#ifdef JP
					sprintf(buf,"%d階", dun_level);
#else
					sprintf(buf,"level %d", dun_level);
#endif

#ifdef JP
				sprintf(tmp, "%sで%sに殺された。", buf, p_ptr->died_from);
#else
				sprintf(tmp, "killed by %s %s.", p_ptr->died_from, buf);
#endif
				do_cmd_write_nikki(NIKKI_BUNSHOU, 0, tmp);
			}

#ifdef JP
			do_cmd_write_nikki(NIKKI_GAMESTART, 1, "-------- ゲームオーバー --------");
#else
			do_cmd_write_nikki(NIKKI_GAMESTART, 1, "--------   Game  Over   --------");
#endif
			do_cmd_write_nikki(NIKKI_BUNSHOU, 1, "\n\n\n\n");

			flush();

#ifdef JP
			if (get_check_strict("画面を保存しますか？", CHECK_NO_HISTORY))
#else
			if (get_check_strict("Dump the screen? ", CHECK_NO_HISTORY))
#endif
			{
				do_cmd_save_screen();
			}

			flush();

			/* Initialize "last message" buffer */
			if (p_ptr->last_message) string_free(p_ptr->last_message);
			p_ptr->last_message = NULL;

			/* Hack -- Note death */
			if (!last_words)
			{
#ifdef JP
				msg_format("あなたは%sました。", android ? "壊れ" : "死に");
#else
				msg_print(android ? "You are broken." : "You die.");
#endif

				msg_print(NULL);
			}
			else
			{
				if (winning_seppuku)
				{
#ifdef JP
					get_rnd_line("seppuku_j.txt", 0, death_message);
#else
					get_rnd_line("seppuku.txt", 0, death_message);
#endif
				}
				else
				{
#ifdef JP
					get_rnd_line("death_j.txt", 0, death_message);
#else
					get_rnd_line("death.txt", 0, death_message);
#endif
				}

				do
				{
#ifdef JP
					while (!get_string(winning_seppuku ? "辞世の句: " : "断末魔の叫び: ", death_message, 1024)) ;
#else
					while (!get_string("Last word: ", death_message, 1024)) ;
#endif
				}
#ifdef JP
				while (winning_seppuku && !get_check_strict("よろしいですか？", CHECK_NO_HISTORY));
#else
				while (winning_seppuku && !get_check_strict("Are you sure? ", CHECK_NO_HISTORY));
#endif

				if (death_message[0] == '\0')
				{
#ifdef JP
					strcpy(death_message, format("あなたは%sました。", android ? "壊れ" : "死に"));
#else
					strcpy(death_message, android ? "You are broken." : "You die.");
#endif
				}
				else p_ptr->last_message = string_make(death_message);

#ifdef JP
				if (winning_seppuku)
				{
					int i, len;
					int w = Term->wid;
					int h = Term->hgt;
					int msg_pos_x[9] = {  5,  7,  9, 12,  14,  17,  19,  21, 23};
					int msg_pos_y[9] = {  3,  4,  5,  4,   5,   4,   5,   6,  4};
					cptr str;
					char* str2;

					Term_clear();

					/* 桜散る */
					for (i = 0; i < 40; i++)
						Term_putstr(randint0(w / 2) * 2, randint0(h), 2, TERM_VIOLET, "υ");

					str = death_message;
					if (strncmp(str, "「", 2) == 0) str += 2;

					str2 = my_strstr(str, "」");
					if (str2 != NULL) *str2 = '\0';

					i = 0;
					while (i < 9)
					{
						str2 = my_strstr(str, " ");
						if (str2 == NULL) len = strlen(str);
						else len = str2 - str;

						if (len != 0)
						{
							Term_putstr_v(w * 3 / 4 - 2 - msg_pos_x[i] * 2, msg_pos_y[i], len,
							TERM_WHITE, str);
							if (str2 == NULL) break;
							i++;
						}
						str = str2 + 1;
						if (*str == 0) break;
					}

					/* Hide cursor */
					Term_putstr(w-1, h-1, 1, TERM_WHITE, " ");

					flush();
#ifdef WORLD_SCORE
					/* Make screen dump */
					screen_dump = make_screen_dump();
#endif

					/* Wait a key press */
					(void)inkey();
				}
				else
#endif
					msg_print(death_message);
			}
		}

		/* Dead */
		return damage;
	}

	/* Hitpoint warning */
	if (p_ptr->chp < warning)
	{
		/* Hack -- bell on first notice */
		if (old_chp > warning) bell();

		sound(SOUND_WARN);

		if (record_danger && (old_chp > warning))
		{
			if (p_ptr->image && damage_type == DAMAGE_ATTACK)
#ifdef JP
				hit_from = "何か";
#else
				hit_from = "something";
#endif

#ifdef JP
			sprintf(tmp,"%sによってピンチに陥った。",hit_from);
#else
			sprintf(tmp,"A critical situation because of %s.",hit_from);
#endif
			do_cmd_write_nikki(NIKKI_BUNSHOU, 0, tmp);
		}

		if (auto_more)
		{
			/* stop auto_more even if DAMAGE_USELIFE */
			now_damaged = TRUE;
		}

		/* Message */
#ifdef JP
msg_print("*** 警告:低ヒット・ポイント！ ***");
#else
		msg_print("*** LOW HITPOINT WARNING! ***");
#endif

		msg_print(NULL);
		flush();
	}
	if (p_ptr->wild_mode && !p_ptr->leaving && (p_ptr->chp < MAX(warning, p_ptr->mhp/5)))
	{
		change_wild_mode();
	}
	return damage;
}


/*
 * Gain experience
 */
void gain_exp_64(s32b amount, u32b amount_frac)
{
	if (p_ptr->is_dead) return;

	if (p_ptr->prace == RACE_ANDROID) return;

	/* Gain some experience */
	s64b_add(&(p_ptr->exp), &(p_ptr->exp_frac), amount, amount_frac);

	/* Slowly recover from experience drainage */
	if (p_ptr->exp < p_ptr->max_exp)
	{
		/* Gain max experience (20%) (was 10%) */
		p_ptr->max_exp += amount / 5;
	}

	/* Check Experience */
	check_experience();
}


/*
 * Gain experience
 */
void gain_exp(s32b amount)
{
	gain_exp_64(amount, 0L);
}


void calc_android_exp(void)
{
	int i;
	u32b total_exp = 0;
	if (p_ptr->is_dead) return;

	if (p_ptr->prace != RACE_ANDROID) return;

	for (i = INVEN_RARM; i < INVEN_TOTAL; i++)
	{
		object_type *o_ptr = &inventory[i];
		object_type forge;
		object_type *q_ptr = &forge;
		u32b value, exp;
		int level = MAX(k_info[o_ptr->k_idx].level - 8, 1);

		if ((i == INVEN_RIGHT) || (i == INVEN_LEFT) || (i == INVEN_NECK) || (i == INVEN_LITE)) continue;
		if (!o_ptr->k_idx) continue;

		/* Wipe the object */
		object_wipe(q_ptr);

		object_copy(q_ptr, o_ptr);
		q_ptr->discount = 0;
		q_ptr->curse_flags = 0L;

		if (object_is_fixed_artifact(o_ptr))
		{
			level = (level + MAX(a_info[o_ptr->name1].level - 8, 5)) / 2;
			level += MIN(20, a_info[o_ptr->name1].rarity/(a_info[o_ptr->name1].gen_flags & TRG_INSTA_ART ? 10 : 3));
		}
		else if (object_is_ego(o_ptr))
		{
			level += MAX(3, (e_info[o_ptr->name2].rating - 5)/2);
		}
		else if (o_ptr->art_name)
		{
			s32b total_flags = flag_cost(o_ptr, o_ptr->pval);
			int fake_level;

			if (!object_is_weapon_ammo(o_ptr))
			{
				/* For armors */
				if (total_flags < 15000) fake_level = 10;
				else if (total_flags < 35000) fake_level = 25;
				else fake_level = 40;
			}
			else
			{
				/* For weapons */
				if (total_flags < 20000) fake_level = 10;
				else if (total_flags < 45000) fake_level = 25;
				else fake_level = 40;
			}

			level = MAX(level, (level + MAX(fake_level - 8, 5)) / 2 + 3);
		}

		value = object_value_real(q_ptr);

		if (value <= 0) continue;
		if ((o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_ABUNAI_MIZUGI) && (p_ptr->pseikaku != SEIKAKU_SEXY)) value /= 32;
		if (value > 5000000L) value = 5000000L;
		if ((o_ptr->tval == TV_DRAG_ARMOR) || (o_ptr->tval == TV_CARD)) level /= 2;

		if (object_is_artifact(o_ptr) || object_is_ego(o_ptr) ||
		    (o_ptr->tval == TV_DRAG_ARMOR) ||
		    ((o_ptr->tval == TV_HELM) && (o_ptr->sval == SV_DRAGON_HELM)) ||
		    ((o_ptr->tval == TV_SHIELD) && (o_ptr->sval == SV_DRAGON_SHIELD)) ||
		    ((o_ptr->tval == TV_GLOVES) && (o_ptr->sval == SV_SET_OF_DRAGON_GLOVES)) ||
		    ((o_ptr->tval == TV_BOOTS) && (o_ptr->sval == SV_PAIR_OF_DRAGON_GREAVE)) ||
		    ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_DIAMOND_EDGE)))
		{
			if (level > 65) level = 35 + (level - 65) / 5;
			else if (level > 35) level = 25 + (level - 35) / 3;
			else if (level > 15) level = 15 + (level - 15) / 2;
			exp = MIN(100000L, value) * level * level / 2;
			if (value > 100000L)
				exp += (value - 100000L) * level * level / 8;
		}
		else
		{
			exp = MIN(100000L, value) * level;
			if (value > 100000L)
				exp += (value - 100000L) * level / 4;
		}
		if ((((i == INVEN_RARM) || (i == INVEN_LARM)) && (buki_motteruka(i))) || (i == INVEN_BOW)) total_exp += exp / 48;
		else total_exp += exp / 16;
		if (i == INVEN_BODY) total_exp += exp / 32;
	}
	p_ptr->exp = p_ptr->max_exp = total_exp;

	/* Check Experience */
	check_experience();
}


/*
 * Lose experience
 */
void lose_exp(s32b amount)
{
	if (p_ptr->prace == RACE_ANDROID) return;

	/* Never drop below zero experience */
	if (amount > p_ptr->exp) amount = p_ptr->exp;

	/* Lose some experience */
	p_ptr->exp -= amount;

	/* Check Experience */
	check_experience();
}


/*
 * Drain experience
 * If resisted to draining, return FALSE
 */
bool drain_exp(s32b drain, s32b slip, int hold_life_prob)
{
	/* Androids and their mimics are never drained */
	if (p_ptr->prace == RACE_ANDROID) return FALSE;

	if (p_ptr->hold_life && (randint0(100) < hold_life_prob))
	{
		/* Hold experience */
#ifdef JP
		msg_print("しかし自己の生命力を守りきった！");
#else
		msg_print("You keep hold of your life force!");
#endif
		return FALSE;
	}

	/* Hold experience failed */
	if (p_ptr->hold_life)
	{
#ifdef JP
		msg_print("生命力を少し吸い取られた気がする！");
#else
		msg_print("You feel your life slipping away!");
#endif
		lose_exp(slip);
	}
	else
	{
#ifdef JP
		msg_print("生命力が体から吸い取られた気がする！");
#else
		msg_print("You feel your life draining away!");
#endif
		lose_exp(drain);
	}

	return TRUE;
}


bool set_ultimate_res(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->ult_res && !do_dec)
		{
			if (p_ptr->ult_res > v) return FALSE;
		}
		else if (!p_ptr->ult_res)
		{
#ifdef JP
msg_print("あらゆることに対して耐性がついた気がする！");
#else
			msg_print("You feel resistant!");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->ult_res)
		{
#ifdef JP
msg_print("あらゆることに対する耐性が薄れた気がする。");
#else
			msg_print("You feel less resistant");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->ult_res = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}

bool set_tim_res_nether(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->tim_res_nether && !do_dec)
		{
			if (p_ptr->tim_res_nether > v) return FALSE;
		}
		else if (!p_ptr->tim_res_nether)
		{
#ifdef JP
msg_print("地獄の力に対して耐性がついた気がする！");
#else
			msg_print("You feel nether resistant!");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->tim_res_nether)
		{
#ifdef JP
msg_print("地獄の力に対する耐性が薄れた気がする。");
#else
			msg_print("You feel less nether resistant");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_res_nether = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}

bool set_tim_res_time(int v, bool do_dec)
{
	bool notice = FALSE;

	/* Hack -- Force good values */
	v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

	if (p_ptr->is_dead) return FALSE;

	/* Open */
	if (v)
	{
		if (p_ptr->tim_res_time && !do_dec)
		{
			if (p_ptr->tim_res_time > v) return FALSE;
		}
		else if (!p_ptr->tim_res_time)
		{
#ifdef JP
msg_print("時間逆転の力に対して耐性がついた気がする！");
#else
			msg_print("You feel time resistant!");
#endif

			notice = TRUE;
		}
	}

	/* Shut */
	else
	{
		if (p_ptr->tim_res_time)
		{
#ifdef JP
msg_print("時間逆転の力に対する耐性が薄れた気がする。");
#else
			msg_print("You feel less time resistant");
#endif

			notice = TRUE;
		}
	}

	/* Use the value */
	p_ptr->tim_res_time = v;

	/* Redraw status bar */
	p_ptr->redraw |= (PR_STATUS);

	/* Nothing to notice */
	if (!notice) return (FALSE);

	/* Disturb */
	if (disturb_state) disturb(0, 0);

	/* Recalculate bonuses */
	p_ptr->update |= (PU_BONUS);

	/* Handle stuff */
	handle_stuff();

	/* Result */
	return (TRUE);
}


/*
 * Choose a warrior-mage elemental attack. -LM-
 */
bool choose_ele_attack(void)
{
	int num;

	char choice;

	if (!buki_motteruka(INVEN_RARM) && !buki_motteruka(INVEN_LARM))
	{
#ifdef JP
		msg_format("武器を持たないと魔法剣は使えない。");
#else
		msg_format("You cannot use temporary branding with no weapon.");
#endif
		return FALSE;
	}

	/* Save screen */
	screen_save();

	num = (p_ptr->lev - 20) / 5;

#ifdef JP
		      c_prt(TERM_RED,    "        a) 焼棄", 2, 14);
#else
		      c_prt(TERM_RED,    "        a) Fire Brand", 2, 14);
#endif

#ifdef JP
	if (num >= 2) c_prt(TERM_L_WHITE,"        b) 凍結", 3, 14);
#else
	if (num >= 2) c_prt(TERM_L_WHITE,"        b) Cold Brand", 3, 14);
#endif
	else prt("", 3, 14);

#ifdef JP
	if (num >= 3) c_prt(TERM_GREEN,  "        c) 毒殺", 4, 14);
#else
	if (num >= 3) c_prt(TERM_GREEN,  "        c) Poison Brand", 4, 14);
#endif
	else prt("", 4, 14);

#ifdef JP
	if (num >= 4) c_prt(TERM_L_DARK, "        d) 溶解", 5, 14);
#else
	if (num >= 4) c_prt(TERM_L_DARK, "        d) Acid Brand", 5, 14);
#endif
	else prt("", 5, 14);

#ifdef JP
	if (num >= 5) c_prt(TERM_BLUE,   "        e) 電撃", 6, 14);
#else
	if (num >= 5) c_prt(TERM_BLUE,   "        e) Elec Brand", 6, 14);
#endif
	else prt("", 6, 14);

	prt("", 7, 14);
	prt("", 8, 14);
	prt("", 9, 14);

	prt("", 1, 0);
#ifdef JP
	prt("        どの元素攻撃をしますか？", 1, 14);
#else
	prt("        Choose a temporary elemental brand ", 1, 14);
#endif

	choice = inkey();

	if ((choice == 'a') || (choice == 'A')) 
		set_ele_attack(ATTACK_FIRE, p_ptr->lev/2 + randint1(p_ptr->lev/2));
	else if (((choice == 'b') || (choice == 'B')) && (num >= 2))
		set_ele_attack(ATTACK_COLD, p_ptr->lev/2 + randint1(p_ptr->lev/2));
	else if (((choice == 'c') || (choice == 'C')) && (num >= 3))
		set_ele_attack(ATTACK_POIS, p_ptr->lev/2 + randint1(p_ptr->lev/2));
	else if (((choice == 'd') || (choice == 'D')) && (num >= 4))
		set_ele_attack(ATTACK_ACID, p_ptr->lev/2 + randint1(p_ptr->lev/2));
	else if (((choice == 'e') || (choice == 'E')) && (num >= 5))
		set_ele_attack(ATTACK_ELEC, p_ptr->lev/2 + randint1(p_ptr->lev/2));
	else
	{
#ifdef JP
		msg_print("魔法剣を使うのをやめた。");
#else
		msg_print("You cancel the temporary branding.");
#endif
		screen_load();
		return FALSE;
	}
	/* Load screen */
	screen_load();
	return TRUE;
}


/*
 * Choose a elemental immune. -LM-
 */
bool choose_ele_immune(int turn)
{
	char choice;

	/* Save screen */
	screen_save();

#ifdef JP
	c_prt(TERM_RED,    "        a) 火炎", 2, 14);
#else
	c_prt(TERM_RED,    "        a) Immune Fire", 2, 14);
#endif

#ifdef JP
	c_prt(TERM_L_WHITE,"        b) 冷気", 3, 14);
#else
	c_prt(TERM_L_WHITE,"        b) Immune Cold", 3, 14);
#endif

#ifdef JP
	c_prt(TERM_L_DARK, "        c) 酸", 4, 14);
#else
	c_prt(TERM_L_DARK, "        c) Immune Acid", 4, 14);
#endif

#ifdef JP
	c_prt(TERM_BLUE,   "        d) 電撃", 5, 14);
#else
	c_prt(TERM_BLUE,   "        d) Immune Elec", 5, 14);
#endif


	prt("", 6, 14);
	prt("", 7, 14);
	prt("", 8, 14);
	prt("", 9, 14);

	prt("", 1, 0);
#ifdef JP
	prt("        どの元素の免疫をつけますか？", 1, 14);
#else
	prt("        Choose a temporary elemental immune ", 1, 14);
#endif

	choice = inkey();

	if ((choice == 'a') || (choice == 'A')) 
		set_ele_immune(DEFENSE_FIRE, turn);
	else if ((choice == 'b') || (choice == 'B'))
		set_ele_immune(DEFENSE_COLD, turn);
	else if ((choice == 'c') || (choice == 'C'))
		set_ele_immune(DEFENSE_ACID, turn);
	else if ((choice == 'd') || (choice == 'D'))
		set_ele_immune(DEFENSE_ELEC, turn);
	else
	{
#ifdef JP
		msg_print("免疫を付けるのをやめた。");
#else
		msg_print("You cancel the temporary immune.");
#endif
		screen_load();
		return FALSE;
	}
	/* Load screen */
	screen_load();
	return TRUE;
}
