/* File: monster1.c */

/* Purpose: describe monsters (using monster memory) */

/*
 * Copyright (c) 1989 James E. Wilson, Christopher J. Stuart
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "angband.h"


/*
 * Pronoun arrays, by gender.
 */
static cptr wd_he[3] =
#ifdef JP
{ "それ", "彼", "彼女" };
#else
{ "it", "he", "she" };
#endif

static cptr wd_his[3] =
#ifdef JP
{ "それの", "彼の", "彼女の" };
#else
{ "its", "his", "her" };
#endif



/*
 * Pluralizer.  Args(count, singular, plural)
 */
#define plural(c,s,p) \
    (((c) == 1) ? (s) : (p))






/*
 * Determine if the "armor" is known
 * The higher the level, the fewer kills needed.
 */
static bool know_armour(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	s32b level = r_ptr->level;

	s32b kills = r_ptr->r_tkills;

	if (cheat_know) return (TRUE);

	/* Normal monsters */
	if (kills > 304 / (4 + level)) return (TRUE);

	/* Skip non-uniques */
	if (!(r_ptr->flags1 & RF1_UNIQUE)) return (FALSE);

	/* Unique monsters */
	if (kills > 304 / (38 + (5 * level) / 4)) return (TRUE);

	/* Assume false */
	return (FALSE);
}


/*
 * Determine if the "damage" of the given attack is known
 * the higher the level of the monster, the fewer the attacks you need,
 * the more damage an attack does, the more attacks you need
 */
static bool know_damage(int r_idx, int i)
{
	monster_race *r_ptr = &r_info[r_idx];

	s32b level = r_ptr->level;

	s32b a = r_ptr->r_blows[i];

	s32b d1 = r_ptr->blow[i].d_dice;
	s32b d2 = r_ptr->blow[i].d_side;

	s32b d = d1 * d2;

	if (d >= ((4+level)*MAX_UCHAR)/80) d = ((4+level)*MAX_UCHAR-1)/80;

	/* Normal monsters */
	if ((4 + level) * a > 80 * d) return (TRUE);

	/* Skip non-uniques */
	if (!(r_ptr->flags1 & RF1_UNIQUE)) return (FALSE);

	/* Unique monsters */
	if ((4 + level) * (2 * a) > 80 * d) return (TRUE);

	/* Assume false */
	return (FALSE);
}


/*
 * Hack -- display monster information using "roff()"
 *
 * Note that there is now a compiler option to only read the monster
 * descriptions from the raw file when they are actually needed, which
 * saves about 60K of memory at the cost of disk access during monster
 * recall, which is optional to the user.
 *
 * This function should only be called with the cursor placed at the
 * left edge of the screen, on a cleared line, in which the recall is
 * to take place.  One extra blank line is left after the recall.
 */
static void roff_aux(int r_idx, int remem)
{
	monster_race    *r_ptr = &r_info[r_idx];

	bool            old = FALSE;
	bool            sin = FALSE;

	int             m, n, r;

	cptr            p, q;

#ifdef JP
        char            jverb_buf[64];
#endif
	int             msex = 0;

	int speed = (ironman_nightmare) ? r_ptr->speed + 5 : r_ptr->speed;

	bool            breath = FALSE;
	bool            magic = FALSE;

	u32b		flags1;
	u32b		flags2;
	u32b		flags3;
	u32b		flags4;
	u32b		flags5;
	u32b		flags6;
	u32b		flags7;

	int		vn = 0;
	byte		color[64];
	cptr		vp[64];

	monster_race    save_mem;


	/* Cheat -- Know everything */
	if (cheat_know)
	{
		/* XXX XXX XXX */

		/* Save the "old" memory */
		save_mem = *r_ptr;

		/* Hack -- Maximal kills */
/*		r_ptr->r_tkills = MAX_SHORT; */

		/* Hack -- Maximal info */
		r_ptr->r_wake = r_ptr->r_ignore = MAX_UCHAR;

		/* Observe "maximal" attacks */
		for (m = 0; m < 4; m++)
		{
			/* Examine "actual" blows */
			if (r_ptr->blow[m].effect || r_ptr->blow[m].method)
			{
				/* Hack -- maximal observations */
				r_ptr->r_blows[m] = MAX_UCHAR;
			}
		}

		/* Hack -- maximal drops */
		r_ptr->r_drop_gold = r_ptr->r_drop_item =
		(((r_ptr->flags1 & RF1_DROP_4D2) ? 8 : 0) +
		 ((r_ptr->flags1 & RF1_DROP_3D2) ? 6 : 0) +
		 ((r_ptr->flags1 & RF1_DROP_2D2) ? 4 : 0) +
		 ((r_ptr->flags1 & RF1_DROP_1D2) ? 2 : 0) +
		 ((r_ptr->flags1 & RF1_DROP_90)  ? 1 : 0) +
		 ((r_ptr->flags1 & RF1_DROP_60)  ? 1 : 0));

		/* Hack -- but only "valid" drops */
		if (r_ptr->flags1 & RF1_ONLY_GOLD) r_ptr->r_drop_item = 0;
		if (r_ptr->flags1 & RF1_ONLY_ITEM) r_ptr->r_drop_gold = 0;

		/* Hack -- observe many spells */
		r_ptr->r_cast_inate = MAX_UCHAR;
		r_ptr->r_cast_spell = MAX_UCHAR;

		/* Hack -- know all the flags */
		r_ptr->r_flags1 = r_ptr->flags1;
		r_ptr->r_flags2 = r_ptr->flags2;
		r_ptr->r_flags3 = r_ptr->flags3;
		r_ptr->r_flags4 = r_ptr->flags4;
		r_ptr->r_flags5 = r_ptr->flags5;
		r_ptr->r_flags6 = r_ptr->flags6;

		r_ptr->r_xtra1 |= MR1_SINKA;
	}


	/* Extract a gender (if applicable) */
	if (r_ptr->flags1 & RF1_FEMALE) msex = 2;
	else if (r_ptr->flags1 & RF1_MALE) msex = 1;


	/* Obtain a copy of the "known" flags */
	flags1 = (r_ptr->flags1 & r_ptr->r_flags1);
	flags2 = (r_ptr->flags2 & r_ptr->r_flags2);
	flags3 = (r_ptr->flags3 & r_ptr->r_flags3);
	flags4 = (r_ptr->flags4 & r_ptr->r_flags4);
	flags5 = (r_ptr->flags5 & r_ptr->r_flags5);
	flags6 = (r_ptr->flags6 & r_ptr->r_flags6);
	flags7 = (r_ptr->flags7 & r_ptr->flags7);


	/* Assume some "obvious" flags */
	if (r_ptr->flags1 & RF1_UNIQUE)  flags1 |= (RF1_UNIQUE);
	if (r_ptr->flags1 & RF1_QUESTOR) flags1 |= (RF1_QUESTOR);
	if (r_ptr->flags1 & RF1_MALE)    flags1 |= (RF1_MALE);
	if (r_ptr->flags1 & RF1_FEMALE)  flags1 |= (RF1_FEMALE);

	/* Assume some "creation" flags */
	if (r_ptr->flags1 & RF1_FRIENDS) flags1 |= (RF1_FRIENDS);
	if (r_ptr->flags1 & RF1_ESCORT)  flags1 |= (RF1_ESCORT);
	if (r_ptr->flags1 & RF1_ESCORTS) flags1 |= (RF1_ESCORTS);

	/* Killing a monster reveals some properties */
	if (r_ptr->r_tkills || cheat_know)
	{
		/* Know "race" flags */
		if (r_ptr->flags3 & RF3_ORC)      flags3 |= (RF3_ORC);
		if (r_ptr->flags3 & RF3_TROLL)    flags3 |= (RF3_TROLL);
		if (r_ptr->flags3 & RF3_GIANT)    flags3 |= (RF3_GIANT);
		if (r_ptr->flags3 & RF3_DRAGON)   flags3 |= (RF3_DRAGON);
		if (r_ptr->flags3 & RF3_DEMON)    flags3 |= (RF3_DEMON);
		if (r_ptr->flags3 & RF3_UNDEAD)   flags3 |= (RF3_UNDEAD);
		if (r_ptr->flags3 & RF3_EVIL)     flags3 |= (RF3_EVIL);
		if (r_ptr->flags3 & RF3_GOOD)     flags3 |= (RF3_GOOD);
		if (r_ptr->flags3 & RF3_ANIMAL)   flags3 |= (RF3_ANIMAL);
		if (r_ptr->flags3 & RF3_AMBERITE) flags3 |= (RF3_AMBERITE);

		/* Know 'quantum' flag */
		if (r_ptr->flags2 & RF2_QUANTUM)  flags2 |= (RF2_QUANTUM);

		/* Know "forced" flags */
		if (r_ptr->flags1 & RF1_FORCE_DEPTH) flags1 |= (RF1_FORCE_DEPTH);
		if (r_ptr->flags1 & RF1_FORCE_MAXHP) flags1 |= (RF1_FORCE_MAXHP);
	}


	/* Treat uniques differently */
	if (flags1 & RF1_UNIQUE)
	{
		/* Hack -- Determine if the unique is "dead" */
		bool dead = (r_ptr->max_num == 0) ? TRUE : FALSE;

		/* We've been killed... */
		if (r_ptr->r_deaths)
		{
			/* Killed ancestors */
#ifdef JP
			roff(format("%^sはあなたの先祖を %d 人葬っている",
#else
			roff(format("%^s has slain %d of your ancestors",
#endif

			            wd_he[msex], r_ptr->r_deaths));

			/* But we've also killed it */
			if (dead)
			{
#ifdef JP
				roff(format("が、すでに仇討ちは果たしている！"));
#else
				roff(format(", but you have avenged %s!  ",
				            plural(r_ptr->r_deaths, "him", "them")));
#endif

			}

			/* Unavenged (ever) */
			else
			{
#ifdef JP
				roff(format("のに、まだ仇討ちを果たしていない。"));
#else
				roff(format(", who %s unavenged.  ",
				            plural(r_ptr->r_deaths, "remains", "remain")));
#endif

			}
		}

		/* Dead unique who never hurt us */
		else if (dead)
		{
#ifdef JP
			roff("あなたはこの仇敵をすでに葬り去っている。");
#else
			roff("You have slain this foe.  ");
#endif

		}
	}

	/* Not unique, but killed us */
	else if (r_ptr->r_deaths)
	{
		/* Dead ancestors */
#ifdef JP
		roff(format("このモンスターはあなたの先祖を %d 人葬っている",
		            r_ptr->r_deaths ));
#else
		roff(format("%d of your ancestors %s been killed by this creature, ",
		            r_ptr->r_deaths, plural(r_ptr->r_deaths, "has", "have")));
#endif


		/* Some kills this life */
		if (r_ptr->r_pkills)
		{
#ifdef JP
			roff(format("が、あなたはこのモンスターを少なくとも %d 体は倒している。",
#else
			roff(format("and you have exterminated at least %d of the creatures.  ",
#endif

			            r_ptr->r_pkills));
		}

		/* Some kills past lives */
		else if (r_ptr->r_tkills)
		{
#ifdef JP
			roff(format("が、%sはこのモンスターを少なくとも %d 体は倒している。",
			            "あなたの先祖", r_ptr->r_tkills));
#else
			roff(format("and %s have exterminated at least %d of the creatures.  ",
			            "your ancestors", r_ptr->r_tkills));
#endif

		}

		/* No kills */
		else
		{
#ifdef JP
			roff(format("が、まだ%sを倒したことはない。",
#else
			roff(format("and %s is not ever known to have been defeated.  ",
#endif

			            wd_he[msex]));
		}
	}

	/* Normal monsters */
	else
	{
		/* Killed some this life */
		if (r_ptr->r_pkills)
		{
#ifdef JP
			roff(format("あなたはこのモンスターを少なくとも %d 体は殺している。",
#else
			roff(format("You have killed at least %d of these creatures.  ",
#endif

			            r_ptr->r_pkills));
		}

		/* Killed some last life */
		else if (r_ptr->r_tkills)
		{
#ifdef JP
			roff(format("あなたの先祖はこのモンスターを少なくとも %d 体は殺している。",
#else
			roff(format("Your ancestors have killed at least %d of these creatures.  ",
#endif

			            r_ptr->r_tkills));
		}

		/* Killed none */
		else
		{
#ifdef JP
			roff("このモンスターを倒したことはない。");
#else
			roff("No battles to the death are recalled.  ");
#endif

		}
	}


	/* Descriptions */
	if (1)
	{
		char buf[2048];

#ifdef DELAY_LOAD_R_TEXT

		int fd;

		/* Build the filename */
#ifdef JP
path_build(buf, 1024, ANGBAND_DIR_DATA, "r_info_j.raw");
#else
		path_build(buf, 1024, ANGBAND_DIR_DATA, "r_info.raw");
#endif


		/* Open the "raw" file */
		fd = fd_open(buf, O_RDONLY);

		/* Use file */
		if (fd >= 0)
		{
			huge pos;

			/* Starting position */
			pos = r_ptr->text;

			/* Additional offsets */
			pos += r_head->head_size;
			pos += r_head->info_size;
			pos += r_head->name_size;

			/* Seek */
			(void)fd_seek(fd, pos);

			/* Read a chunk of data */
			(void)fd_read(fd, buf, 2048);

			/* Close it */
			(void)fd_close(fd);
		}

#else

		/* Simple method */
		strcpy(buf, r_text + r_ptr->text);

#endif

		/* Dump it */
		roff(buf);
#ifndef JP
		roff("  ");
#endif
	}

	if (r_idx == MON_KAGE)
	{
		/* All done */
		roff("\n");

		/* Cheat -- know everything */
		if ((cheat_know) && (remem == 0))
		{
			/* Hack -- restore memory */
			COPY(r_ptr, &save_mem, monster_race);
		}
		return;
	}

	/* Nothing yet */
	old = FALSE;

	/* Describe location */
	if (r_ptr->level == 0)
	{
#ifdef JP
		roff(format("%^sは町に住み", wd_he[msex]));
#else
		roff(format("%^s lives in the town", wd_he[msex]));
#endif

		old = TRUE;
	}
	else if (r_ptr->r_tkills || cheat_know)
	{
		if (depth_in_feet)
		{
#ifdef JP
			roff(format("%^sは通常地下 %d フィートで出現し",
#else
			roff(format("%^s is normally found at depths of %d feet",
#endif

			            wd_he[msex], r_ptr->level * 50));
		}
		else
		{
#ifdef JP
			roff(format("%^sは通常地下 %d 階で出現し",
#else
			roff(format("%^s is normally found on dungeon level %d",
#endif

			            wd_he[msex], r_ptr->level));
		}
		old = TRUE;
	}


	/* Describe movement */
	if (r_idx == MON_CHAMELEON)
	{
#ifdef JP
		roff("、他のモンスターに化ける。");
#else
		roff("and can take the shape of other monster.");
#endif
		return;
	}
	else
	{
		/* Introduction */
		if (old)
		{
#ifdef JP
			roff("、");
#else
			roff(", and ");
#endif

		}
		else
		{
#ifdef JP
			roff(format("%^sは", wd_he[msex]));
#else
			roff(format("%^s ", wd_he[msex]));
#endif

			old = TRUE;
		}
#ifndef JP
		roff("moves");
#endif

		/* Random-ness */
		if ((flags1 & RF1_RAND_50) || (flags1 & RF1_RAND_25))
		{
			/* Adverb */
			if ((flags1 & RF1_RAND_50) && (flags1 & RF1_RAND_25))
			{
#ifdef JP
				roff("かなり");
#else
				roff(" extremely");
#endif

			}
			else if (flags1 & RF1_RAND_50)
			{
#ifdef JP
				roff("幾分");
#else
				roff(" somewhat");
#endif

			}
			else if (flags1 & RF1_RAND_25)
			{
#ifdef JP
				roff("少々");
#else
				roff(" a bit");
#endif

			}

			/* Adjective */
#ifdef JP
			roff("不規則に");
#else
			roff(" erratically");
#endif


			/* Hack -- Occasional conjunction */
#ifdef JP
			if (speed != 110) roff("、かつ");
#else
			if (speed != 110) roff(", and");
#endif

		}

		/* Speed */
		if (speed > 110)
		{
#ifdef JP
			if (speed > 139) c_roff(TERM_RED, "信じ難いほど");
			else if (speed > 134) c_roff(TERM_ORANGE, "猛烈に");
			else if (speed > 129) c_roff(TERM_ORANGE, "非常に");
			else if (speed > 124) c_roff(TERM_UMBER, "かなり");
			else if (speed < 120) c_roff(TERM_L_UMBER, "やや");
			c_roff(TERM_L_RED, "素早く");
#else
			if (speed > 130) roff(" incredibly");
			else if (speed > 120) roff(" very");
			roff(" quickly");
#endif

		}
		else if (speed < 110)
		{
#ifdef JP
			if (speed < 90) c_roff(TERM_L_GREEN, "信じ難いほど");
			else if (speed < 95) c_roff(TERM_BLUE, "非常に");
			else if (speed < 100) c_roff(TERM_BLUE, "かなり");
			else if (speed > 104) c_roff(TERM_GREEN, "やや");
			c_roff(TERM_L_BLUE, "ゆっくりと");
#else
			if (speed < 90) roff(" incredibly");
			else if (speed < 100) roff(" very");
			roff(" slowly");
#endif

		}
		else
		{
#ifdef JP
			roff("普通の速さで");
#else
			roff(" at normal speed");
#endif

		}
#ifdef JP
		roff("動いている");
#endif
	}

	/* The code above includes "attack speed" */
	if (flags1 & RF1_NEVER_MOVE)
	{
		/* Introduce */
		if (old)
		{
#ifdef JP
			roff("、しかし");
#else
			roff(", but ");
#endif

		}
		else
		{
#ifdef JP
			roff(format("%^sは", wd_he[msex]));
#else
			roff(format("%^s ", wd_he[msex]));
#endif

			old = TRUE;
		}

		/* Describe */
#ifdef JP
		roff("侵入者を追跡しない");
#else
		roff("does not deign to chase intruders");
#endif

	}

	/* End this sentence */
	if (old)
	{
#ifdef JP
		roff("。");
#else
		roff(".  ");
#endif

		old = FALSE;
	}


	/* Describe experience if known */
	if (r_ptr->r_tkills || cheat_know)
	{
		/* Introduction */
#ifdef JP
		roff("この");
#else
		if (flags1 & RF1_UNIQUE)
		{
			roff("Killing this");
		}
		else
		{
			roff("A kill of this");
		}
#endif


		/* Describe the "quality" */
#ifdef JP
if (flags2 & RF2_ELDRITCH_HORROR) c_roff(TERM_VIOLET, "狂気を誘う");/*nuke me*/
#else
		if (flags2 & RF2_ELDRITCH_HORROR) roff(" sanity-blasting");
#endif

#ifdef JP
if (flags3 & RF3_ANIMAL)          c_roff(TERM_L_GREEN, "自然界の");
#else
		if (flags3 & RF3_ANIMAL)          roff(" natural");
#endif

#ifdef JP
if (flags3 & RF3_EVIL)            c_roff(TERM_L_DARK, "邪悪なる");
#else
		if (flags3 & RF3_EVIL)            roff(" evil");
#endif

#ifdef JP
if (flags3 & RF3_GOOD)            c_roff(TERM_YELLOW, "善良な");
#else
		if (flags3 & RF3_GOOD)            roff(" good");
#endif

#ifdef JP
if (flags3 & RF3_UNDEAD)          c_roff(TERM_VIOLET, "アンデッドの");
#else
		if (flags3 & RF3_UNDEAD)          roff(" undead");
#endif


	if ((flags3 & (RF3_DRAGON | RF3_DEMON | RF3_GIANT | RF3_TROLL | RF3_ORC | RF3_AMBERITE)) || (flags2 & RF2_QUANTUM))
	{
	/* Describe the "race" */
#ifdef JP
     if (flags3 & RF3_DRAGON)   c_roff(TERM_ORANGE, "ドラゴン");
#else
		     if (flags3 & RF3_DRAGON)   roff(" dragon");
#endif

#ifdef JP
if (flags3 & RF3_DEMON)    c_roff(TERM_VIOLET, "デーモン");
#else
		if (flags3 & RF3_DEMON)    roff(" demon");
#endif

#ifdef JP
if (flags3 & RF3_GIANT)    c_roff(TERM_L_UMBER, "ジャイアント");
#else
		if (flags3 & RF3_GIANT)    roff(" giant");
#endif

#ifdef JP
if (flags3 & RF3_TROLL)    c_roff(TERM_BLUE, "トロル");
#else
		if (flags3 & RF3_TROLL)    roff(" troll");
#endif

#ifdef JP
if (flags3 & RF3_ORC)      c_roff(TERM_UMBER, "オーク");
#else
		if (flags3 & RF3_ORC)      roff(" orc");
#endif

#ifdef JP
if (flags3 & RF3_AMBERITE) c_roff(TERM_L_WHITE, "アンバーの王族");
#else
		if (flags3 & RF3_AMBERITE) roff(" Amberite");
#endif

#ifdef JP
if (flags2 & RF2_QUANTUM)  c_roff(TERM_VIOLET, "量子生物");
#else
		if (flags2 & RF2_QUANTUM)  roff(" quantum creature");
#endif

	}
#ifdef JP
else                            roff("モンスター");
#else
		else                            roff(" creature");
#endif


#ifdef JP
		roff("を倒すことは");
#endif
		/* Group some variables */
		if (TRUE)
		{
			long i, j;

#ifdef JP
			i = p_ptr->lev;
			roff(format(" %lu レベルのキャラクタにとって", (long)i));

			i = (long)r_ptr->mexp * r_ptr->level / (p_ptr->max_plv+2);
			j = ((((long)r_ptr->mexp * r_ptr->level % (p_ptr->max_plv+2)) *
			       (long)1000 / (p_ptr->max_plv+2) + 5) / 10);

			roff(format(" 約%ld.%02ld ポイントの経験となる。",
			        (long)i, (long)j ));
#else
			/* calculate the integer exp part */
			i = (long)r_ptr->mexp * r_ptr->level / (p_ptr->max_plv+2);

			/* calculate the fractional exp part scaled by 100, */
			/* must use long arithmetic to avoid overflow  */
			j = ((((long)r_ptr->mexp * r_ptr->level % (p_ptr->max_plv+2)) *
			       (long)1000 / (p_ptr->max_plv+2) + 5) / 10);

			/* Mention the experience */
			roff(format(" is worth about %ld.%02ld point%s",
			            (long)i, (long)j,
			            (((i == 1) && (j == 0)) ? "" : "s")));

			/* Take account of annoying English */
			p = "th";
			i = p_ptr->lev % 10;
			if ((p_ptr->lev / 10) == 1) /* nothing */;
			else if (i == 1) p = "st";
			else if (i == 2) p = "nd";
			else if (i == 3) p = "rd";

			/* Take account of "leading vowels" in numbers */
			q = "";
			i = p_ptr->lev;
			if ((i == 8) || (i == 11) || (i == 18)) q = "n";

			/* Mention the dependance on the player's level */
			roff(format(" for a%s %lu%s level character.  ",
			            q, (long)i, p));
#endif

		}
	}

	if ((flags2 & RF2_AURA_FIRE) && (flags2 & RF2_AURA_ELEC) && (flags3 & RF3_AURA_COLD))
	{
#ifdef JP
c_roff(TERM_VIOLET, format("%^sは炎と氷とスパークに包まれている。", wd_he[msex]));
#else
		roff(format("%^s is surrounded by flames and electricity.  ", wd_he[msex]));
#endif

	}
	else if ((flags2 & RF2_AURA_FIRE) && (flags2 & RF2_AURA_ELEC))
	{
#ifdef JP
c_roff(TERM_L_RED, format("%^sは炎とスパークに包まれている。", wd_he[msex]));
#else
		roff(format("%^s is surrounded by flames and electricity.  ", wd_he[msex]));
#endif

	}
	else if ((flags2 & RF2_AURA_FIRE) && (flags3 & RF3_AURA_COLD))
	{
#ifdef JP
c_roff(TERM_BLUE, format("%^sは炎と氷に包まれている。", wd_he[msex]));
#else
		roff(format("%^s is surrounded by flames and electricity.  ", wd_he[msex]));
#endif

	}
	else if ((flags3 & RF3_AURA_COLD) && (flags2 & RF2_AURA_ELEC))
	{
#ifdef JP
c_roff(TERM_L_GREEN, format("%^sは氷とスパークに包まれている。", wd_he[msex]));
#else
		roff(format("%^s is surrounded by ice and electricity.  ", wd_he[msex]));
#endif

	}
	else if (flags2 & RF2_AURA_FIRE)
	{
#ifdef JP
c_roff(TERM_RED, format("%^sは炎に包まれている。", wd_he[msex]));
#else
		roff(format("%^s is surrounded by flames.  ", wd_he[msex]));
#endif

	}
	else if (flags3 & RF3_AURA_COLD)
	{
#ifdef JP
c_roff(TERM_BLUE, format("%^sは氷に包まれている。", wd_he[msex]));
#else
		roff(format("%^s is surrounded by ice.  ", wd_he[msex]));
#endif

	}
	else if (flags2 & RF2_AURA_ELEC)
	{
#ifdef JP
c_roff(TERM_L_BLUE, format("%^sはスパークに包まれている。", wd_he[msex]));
#else
		roff(format("%^s is surrounded by electricity.  ", wd_he[msex]));
#endif

	}

	if (flags2 & RF2_REFLECTING)
	{
#ifdef JP
roff(format("%^sは矢の呪文を跳ね返す。", wd_he[msex]));
#else
		roff(format("%^s reflects bolt spells.  ", wd_he[msex]));
#endif

	}

	/* Describe escorts */
	if ((flags1 & RF1_ESCORT) || (flags1 & RF1_ESCORTS))
	{
#ifdef JP
		roff(format("%^sは通常護衛を伴って現れる。",
#else
		roff(format("%^s usually appears with escorts.  ",
#endif

		            wd_he[msex]));
	}

	/* Describe friends */
	else if (flags1 & RF1_FRIENDS)
	{
#ifdef JP
		roff(format("%^sは通常集団で現れる。",
#else
		roff(format("%^s usually appears in groups.  ",
#endif

		            wd_he[msex]));
	}


	/* Collect inate attacks */
	vn = 0;
#ifdef JP
	if (flags4 & RF4_SHRIEK)  {vp[vn] = "悲鳴で助けを求める";color[vn++] = TERM_L_WHITE;}
#else
	if (flags4 & RF4_SHRIEK)  {vp[vn] = "shriek for help";color[vn++] = TERM_L_WHITE;}
#endif

#ifdef JP
	if (flags4 & RF4_ROCKET)  {vp[vn] = "ロケットを発射する";color[vn++] = TERM_UMBER;}
#else
	if (flags4 & RF4_ROCKET)  {vp[vn] = "shoot a rocket";color[vn++] = TERM_UMBER;}
#endif

#ifdef JP
	if (flags4 & RF4_SHOOT) {vp[vn] = "射撃をする";color[vn++] = TERM_UMBER;}
#else
	if (flags4 & RF4_SHOOT) {vp[vn] = "fire an arrow";color[vn++] = TERM_UMBER;}
#endif

#ifdef JP
	if (flags6 & (RF6_SPECIAL)) {vp[vn] = "特別な行動をする";color[vn++] = TERM_VIOLET;}
#else
	if (flags6 & (RF6_SPECIAL)) {vp[vn] = "do something";color[vn++] = TERM_VIOLET;}
#endif

	/* Describe inate attacks */
	if (vn)
	{
		/* Intro */
#ifdef JP
		roff(format("%^sは", wd_he[msex]));
#else
		roff(format("%^s", wd_he[msex]));
#endif


		/* Scan */
		for (n = 0; n < vn; n++)
		{
#ifdef JP
			if(n!=vn-1){
			  jverb(vp[n],jverb_buf,JVERB_OR);
			  c_roff(color[n], jverb_buf);
			  c_roff(color[n], "り、");
			}
			else  c_roff(color[n], vp[n]);
#else
			/* Intro */
			if (n == 0) roff(" may ");
			else if (n < vn-1) roff(", ");
			else roff(" or ");

			/* Dump */
			roff(vp[n]);
#endif

		}

		/* End */
#ifdef JP
		roff("ことがある。");
#else
		roff(".  ");
#endif

	}


	/* Collect breaths */
	vn = 0;
#ifdef JP
	if (flags4 & (RF4_BR_ACID))		{vp[vn] = "酸";color[vn++] = TERM_GREEN;}
#else
	if (flags4 & (RF4_BR_ACID))		{vp[vn] = "acid";color[vn++] = TERM_GREEN;}
#endif

#ifdef JP
	if (flags4 & (RF4_BR_ELEC))		{vp[vn] = "稲妻";color[vn++] = TERM_BLUE;}
#else
	if (flags4 & (RF4_BR_ELEC))		{vp[vn] = "lightning";color[vn++] = TERM_BLUE;}
#endif

#ifdef JP
	if (flags4 & (RF4_BR_FIRE))		{vp[vn] = "火炎";color[vn++] = TERM_RED;}
#else
	if (flags4 & (RF4_BR_FIRE))		{vp[vn] = "fire";color[vn++] = TERM_RED;}
#endif

#ifdef JP
	if (flags4 & (RF4_BR_COLD))		{vp[vn] = "冷気";color[vn++] = TERM_L_WHITE;}
#else
	if (flags4 & (RF4_BR_COLD))		{vp[vn] = "frost";color[vn++] = TERM_L_WHITE;}
#endif

#ifdef JP
	if (flags4 & (RF4_BR_POIS))		{vp[vn] = "毒";color[vn++] = TERM_L_GREEN;}
#else
	if (flags4 & (RF4_BR_POIS))		{vp[vn] = "poison";color[vn++] = TERM_L_GREEN;}
#endif

#ifdef JP
	if (flags4 & (RF4_BR_NETH))		{vp[vn] = "地獄";color[vn++] = TERM_L_DARK;}
#else
	if (flags4 & (RF4_BR_NETH))		{vp[vn] = "nether";color[vn++] = TERM_L_DARK;}
#endif

#ifdef JP
	if (flags4 & (RF4_BR_LITE))		{vp[vn] = "閃光";color[vn++] = TERM_YELLOW;}
#else
	if (flags4 & (RF4_BR_LITE))		{vp[vn] = "light";color[vn++] = TERM_YELLOW;}
#endif

#ifdef JP
	if (flags4 & (RF4_BR_DARK))		{vp[vn] = "暗黒";color[vn++] = TERM_L_DARK;}
#else
	if (flags4 & (RF4_BR_DARK))		{vp[vn] = "darkness";color[vn++] = TERM_L_DARK;}
#endif

#ifdef JP
	if (flags4 & (RF4_BR_CONF))		{vp[vn] = "混乱";color[vn++] = TERM_L_UMBER;}
#else
	if (flags4 & (RF4_BR_CONF))		{vp[vn] = "confusion";color[vn++] = TERM_L_UMBER;}
#endif

#ifdef JP
	if (flags4 & (RF4_BR_SOUN))		{vp[vn] = "轟音";color[vn++] = TERM_ORANGE;}
#else
	if (flags4 & (RF4_BR_SOUN))		{vp[vn] = "sound";color[vn++] = TERM_ORANGE;}
#endif

#ifdef JP
	if (flags4 & (RF4_BR_CHAO))		{vp[vn] = "カオス";color[vn++] = TERM_VIOLET;}
#else
	if (flags4 & (RF4_BR_CHAO))		{vp[vn] = "chaos";color[vn++] = TERM_VIOLET;}
#endif

#ifdef JP
	if (flags4 & (RF4_BR_DISE))		{vp[vn] = "劣化";color[vn++] = TERM_VIOLET;}
#else
	if (flags4 & (RF4_BR_DISE))		{vp[vn] = "disenchantment";color[vn++] = TERM_VIOLET;}
#endif

#ifdef JP
	if (flags4 & (RF4_BR_NEXU))		{vp[vn] = "因果混乱";color[vn++] = TERM_VIOLET;}
#else
	if (flags4 & (RF4_BR_NEXU))		{vp[vn] = "nexus";color[vn++] = TERM_VIOLET;}
#endif

#ifdef JP
	if (flags4 & (RF4_BR_TIME))		{vp[vn] = "時間逆転";color[vn++] = TERM_L_BLUE;}
#else
	if (flags4 & (RF4_BR_TIME))		{vp[vn] = "time";color[vn++] = TERM_L_BLUE;}
#endif

#ifdef JP
	if (flags4 & (RF4_BR_INER))		{vp[vn] = "遅鈍";color[vn++] = TERM_SLATE;}
#else
	if (flags4 & (RF4_BR_INER))		{vp[vn] = "inertia";color[vn++] = TERM_SLATE;}
#endif

#ifdef JP
	if (flags4 & (RF4_BR_GRAV))		{vp[vn] = "重力";color[vn++] = TERM_SLATE;}
#else
	if (flags4 & (RF4_BR_GRAV))		{vp[vn] = "gravity";color[vn++] = TERM_SLATE;}
#endif

#ifdef JP
	if (flags4 & (RF4_BR_SHAR))		{vp[vn] = "破片";color[vn++] = TERM_L_UMBER;}
#else
	if (flags4 & (RF4_BR_SHAR))		{vp[vn] = "shards";color[vn++] = TERM_L_UMBER;}
#endif

#ifdef JP
	if (flags4 & (RF4_BR_PLAS))		{vp[vn] = "プラズマ";color[vn++] = TERM_L_RED;}
#else
	if (flags4 & (RF4_BR_PLAS))		{vp[vn] = "plasma";color[vn++] = TERM_L_RED;}
#endif

#ifdef JP
	if (flags4 & (RF4_BR_WALL))		{vp[vn] = "フォース";color[vn++] = TERM_UMBER;}
#else
	if (flags4 & (RF4_BR_WALL))		{vp[vn] = "force";color[vn++] = TERM_UMBER;}
#endif

#ifdef JP
	if (flags4 & (RF4_BR_MANA))		{vp[vn] = "魔力";color[vn++] = TERM_L_BLUE;}
#else
	if (flags4 & (RF4_BR_MANA))		{vp[vn] = "mana";color[vn++] = TERM_L_BLUE;}
#endif

#ifdef JP
	if (flags4 & (RF4_BR_NUKE))		{vp[vn] = "放射性廃棄物";color[vn++] = TERM_L_GREEN;}
#else
	if (flags4 & (RF4_BR_NUKE))		{vp[vn] = "toxic waste";color[vn++] = TERM_L_GREEN;}
#endif

#ifdef JP
	if (flags4 & (RF4_BR_DISI))		{vp[vn] = "分解";color[vn++] = TERM_SLATE;}
#else
	if (flags4 & (RF4_BR_DISI))		{vp[vn] = "disintegration";color[vn++] = TERM_SLATE;}
#endif


	/* Describe breaths */
	if (vn)
	{
		/* Note breath */
		breath = TRUE;

		/* Intro */
#ifdef JP
		roff(format("%^sは", wd_he[msex]));
#else
		roff(format("%^s", wd_he[msex]));
#endif


		/* Scan */
		for (n = 0; n < vn; n++)
		{
			/* Intro */
#ifdef JP
			if ( n != 0 ) roff("や");
#else
			if (n == 0) roff(" may breathe ");
			else if (n < vn-1) roff(", ");
			else roff(" or ");
#endif


			/* Dump */
			c_roff(color[n], vp[n]);
		}
#ifdef JP
		roff("のブレスを吐くことがある");
#endif
	}


	/* Collect spells */
	vn = 0;
#ifdef JP
if (flags5 & (RF5_BA_ACID))         {vp[vn] = "アシッド・ボール";color[vn++] = TERM_GREEN;}
#else
	if (flags5 & (RF5_BA_ACID))         {vp[vn] = "produce acid balls";color[vn++] = TERM_GREEN;}
#endif

#ifdef JP
if (flags5 & (RF5_BA_ELEC))         {vp[vn] = "サンダー・ボール";color[vn++] = TERM_BLUE;}
#else
	if (flags5 & (RF5_BA_ELEC))         {vp[vn] = "produce lightning balls";color[vn++] = TERM_BLUE;}
#endif

#ifdef JP
if (flags5 & (RF5_BA_FIRE))         {vp[vn] = "ファイア・ボール";color[vn++] = TERM_RED;}
#else
	if (flags5 & (RF5_BA_FIRE))         {vp[vn] = "produce fire balls";color[vn++] = TERM_RED;}
#endif

#ifdef JP
if (flags5 & (RF5_BA_COLD))         {vp[vn] = "アイス・ボール";color[vn++] = TERM_L_WHITE;}
#else
	if (flags5 & (RF5_BA_COLD))         {vp[vn] = "produce frost balls";color[vn++] = TERM_L_WHITE;}
#endif

#ifdef JP
if (flags5 & (RF5_BA_POIS))         {vp[vn] = "悪臭雲";color[vn++] = TERM_L_GREEN;}
#else
	if (flags5 & (RF5_BA_POIS))         {vp[vn] = "produce poison balls";color[vn++] = TERM_L_GREEN;}
#endif

#ifdef JP
if (flags5 & (RF5_BA_NETH))         {vp[vn] = "地獄球";color[vn++] = TERM_L_DARK;}
#else
	if (flags5 & (RF5_BA_NETH))         {vp[vn] = "produce nether balls";color[vn++] = TERM_L_DARK;}
#endif

#ifdef JP
if (flags5 & (RF5_BA_WATE))         {vp[vn] = "ウォーター・ボール";color[vn++] = TERM_BLUE;}
#else
	if (flags5 & (RF5_BA_WATE))         {vp[vn] = "produce water balls";color[vn++] = TERM_BLUE;}
#endif

#ifdef JP
if (flags4 & (RF4_BA_NUKE))         {vp[vn] = "放射能球";color[vn++] = TERM_L_GREEN;}
#else
	if (flags4 & (RF4_BA_NUKE))         {vp[vn] = "produce balls of radiation";color[vn++] = TERM_L_GREEN;}
#endif

#ifdef JP
if (flags5 & (RF5_BA_MANA))         {vp[vn] = "魔力の嵐";color[vn++] = TERM_L_BLUE;}
#else
	if (flags5 & (RF5_BA_MANA))         {vp[vn] = "invoke mana storms";color[vn++] = TERM_L_BLUE;}
#endif

#ifdef JP
if (flags5 & (RF5_BA_DARK))         {vp[vn] = "暗黒の嵐";color[vn++] = TERM_L_DARK;}
#else
	if (flags5 & (RF5_BA_DARK))         {vp[vn] = "invoke darkness storms";color[vn++] = TERM_L_DARK;}
#endif

#ifdef JP
if (flags5 & (RF5_BA_LITE))         {vp[vn] = "スターバースト";color[vn++] = TERM_YELLOW;}
#else
	if (flags5 & (RF5_BA_LITE))         {vp[vn] = "invoke starburst";color[vn++] = TERM_YELLOW;}
#endif

#ifdef JP
if (flags4 & (RF4_BA_CHAO))         {vp[vn] = "純ログルス";color[vn++] = TERM_VIOLET;}
#else
	if (flags4 & (RF4_BA_CHAO))         {vp[vn] = "invoke raw Logrus";color[vn++] = TERM_VIOLET;}
#endif

#ifdef JP
if (flags6 & (RF6_HAND_DOOM))       {vp[vn] = "破滅の手";color[vn++] = TERM_VIOLET;}
#else
	if (flags6 & (RF6_HAND_DOOM))       {vp[vn] = "invoke the Hand of Doom";color[vn++] = TERM_VIOLET;}
#endif

#ifdef JP
if (flags6 & (RF6_PSY_SPEAR))            {vp[vn] = "光の剣";color[vn++] = TERM_YELLOW;}
#else
	if (flags6 & (RF6_PSY_SPEAR))            {vp[vn] = "psycho-spear";color[vn++] = TERM_YELLOW;}
#endif

#ifdef JP
if (flags5 & (RF5_DRAIN_MANA))      {vp[vn] = "魔力吸収";color[vn++] = TERM_SLATE;}
#else
	if (flags5 & (RF5_DRAIN_MANA))      {vp[vn] = "drain mana";color[vn++] = TERM_SLATE;}
#endif

#ifdef JP
if (flags5 & (RF5_MIND_BLAST))      {vp[vn] = "精神攻撃";color[vn++] = TERM_L_RED;}
#else
	if (flags5 & (RF5_MIND_BLAST))      {vp[vn] = "cause mind blasting";color[vn++] = TERM_L_RED;}
#endif

#ifdef JP
if (flags5 & (RF5_BRAIN_SMASH))     {vp[vn] = "脳攻撃";color[vn++] = TERM_RED;}
#else
	if (flags5 & (RF5_BRAIN_SMASH))     {vp[vn] = "cause brain smashing";color[vn++] = TERM_RED;}
#endif

#ifdef JP
if (flags5 & (RF5_CAUSE_1))         {vp[vn] = "軽傷＋呪い";color[vn++] = TERM_L_WHITE;}
#else
	if (flags5 & (RF5_CAUSE_1))         {vp[vn] = "cause light wounds and cursing";color[vn++] = TERM_L_WHITE;}
#endif

#ifdef JP
if (flags5 & (RF5_CAUSE_2))         {vp[vn] = "重傷＋呪い";color[vn++] = TERM_L_WHITE;}
#else
	if (flags5 & (RF5_CAUSE_2))         {vp[vn] = "cause serious wounds and cursing";color[vn++] = TERM_L_WHITE;}
#endif

#ifdef JP
if (flags5 & (RF5_CAUSE_3))         {vp[vn] = "致命傷＋呪い";color[vn++] = TERM_L_WHITE;}
#else
	if (flags5 & (RF5_CAUSE_3))         {vp[vn] = "cause critical wounds and cursing";color[vn++] = TERM_L_WHITE;}
#endif

#ifdef JP
if (flags5 & (RF5_CAUSE_4))         {vp[vn] = "秘孔を突く";color[vn++] = TERM_L_WHITE;}
#else
	if (flags5 & (RF5_CAUSE_4))         {vp[vn] = "cause mortal wounds";color[vn++] = TERM_L_WHITE;}
#endif

#ifdef JP
if (flags5 & (RF5_BO_ACID))         {vp[vn] = "アシッド・ボルト";color[vn++] = TERM_GREEN;}
#else
	if (flags5 & (RF5_BO_ACID))         {vp[vn] = "produce acid bolts";color[vn++] = TERM_GREEN;}
#endif

#ifdef JP
if (flags5 & (RF5_BO_ELEC))         {vp[vn] = "サンダー・ボルト";color[vn++] = TERM_BLUE;}
#else
	if (flags5 & (RF5_BO_ELEC))         {vp[vn] = "produce lightning bolts";color[vn++] = TERM_BLUE;}
#endif

#ifdef JP
if (flags5 & (RF5_BO_FIRE))         {vp[vn] = "ファイア・ボルト";color[vn++] = TERM_RED;}
#else
	if (flags5 & (RF5_BO_FIRE))         {vp[vn] = "produce fire bolts";color[vn++] = TERM_RED;}
#endif

#ifdef JP
if (flags5 & (RF5_BO_COLD))         {vp[vn] = "アイス・ボルト";color[vn++] = TERM_L_WHITE;}
#else
	if (flags5 & (RF5_BO_COLD))         {vp[vn] = "produce frost bolts";color[vn++] = TERM_L_WHITE;}
#endif

#ifdef JP
if (flags5 & (RF5_BO_NETH))         {vp[vn] = "地獄の矢";color[vn++] = TERM_L_DARK;}
#else
	if (flags5 & (RF5_BO_NETH))         {vp[vn] = "produce nether bolts";color[vn++] = TERM_L_DARK;}
#endif

#ifdef JP
if (flags5 & (RF5_BO_WATE))         {vp[vn] = "ウォーター・ボルト";color[vn++] = TERM_BLUE;}
#else
	if (flags5 & (RF5_BO_WATE))         {vp[vn] = "produce water bolts";color[vn++] = TERM_BLUE;}
#endif

#ifdef JP
if (flags5 & (RF5_BO_MANA))         {vp[vn] = "魔力の矢";color[vn++] = TERM_L_BLUE;}
#else
	if (flags5 & (RF5_BO_MANA))         {vp[vn] = "produce mana bolts";color[vn++] = TERM_L_BLUE;}
#endif

#ifdef JP
if (flags5 & (RF5_BO_PLAS))         {vp[vn] = "プラズマ・ボルト";color[vn++] = TERM_L_RED;}
#else
	if (flags5 & (RF5_BO_PLAS))         {vp[vn] = "produce plasma bolts";color[vn++] = TERM_L_RED;}
#endif

#ifdef JP
if (flags5 & (RF5_BO_ICEE))         {vp[vn] = "極寒の矢";color[vn++] = TERM_WHITE;}
#else
	if (flags5 & (RF5_BO_ICEE))         {vp[vn] = "produce ice bolts";color[vn++] = TERM_WHITE;}
#endif

#ifdef JP
if (flags5 & (RF5_MISSILE))         {vp[vn] = "マジックミサイル";color[vn++] = TERM_SLATE;}
#else
	if (flags5 & (RF5_MISSILE))         {vp[vn] = "produce magic missiles";color[vn++] = TERM_SLATE;}
#endif

#ifdef JP
if (flags5 & (RF5_SCARE))           {vp[vn] = "恐怖";color[vn++] = TERM_SLATE;}
#else
	if (flags5 & (RF5_SCARE))           {vp[vn] = "terrify";color[vn++] = TERM_SLATE;}
#endif

#ifdef JP
if (flags5 & (RF5_BLIND))           {vp[vn] = "目くらまし";color[vn++] = TERM_L_DARK;}
#else
	if (flags5 & (RF5_BLIND))           {vp[vn] = "blind";color[vn++] = TERM_L_DARK;}
#endif

#ifdef JP
if (flags5 & (RF5_CONF))            {vp[vn] = "混乱";color[vn++] = TERM_L_UMBER;}
#else
	if (flags5 & (RF5_CONF))            {vp[vn] = "confuse";color[vn++] = TERM_L_UMBER;}
#endif

#ifdef JP
if (flags5 & (RF5_SLOW))            {vp[vn] = "減速";color[vn++] = TERM_UMBER;}
#else
	if (flags5 & (RF5_SLOW))            {vp[vn] = "slow";color[vn++] = TERM_UMBER;}
#endif

#ifdef JP
if (flags5 & (RF5_HOLD))            {vp[vn] = "麻痺";color[vn++] = TERM_RED;}
#else
	if (flags5 & (RF5_HOLD))            {vp[vn] = "paralyze";color[vn++] = TERM_RED;}
#endif

#ifdef JP
if (flags6 & (RF6_HASTE))           {vp[vn] = "加速";color[vn++] = TERM_L_GREEN;}
#else
	if (flags6 & (RF6_HASTE))           {vp[vn] = "haste-self";color[vn++] = TERM_L_GREEN;}
#endif

#ifdef JP
if (flags6 & (RF6_HEAL))            {vp[vn] = "治癒";color[vn++] = TERM_WHITE;}
#else
	if (flags6 & (RF6_HEAL))            {vp[vn] = "heal-self";color[vn++] = TERM_WHITE;}
#endif

#ifdef JP
        if (flags6 & (RF6_INVULNER))        {vp[vn] = "無敵化";color[vn++] = TERM_WHITE;}
#else
	if (flags6 & (RF6_INVULNER))        {vp[vn] = "make invulnerable";color[vn++] = TERM_WHITE;}
#endif

#ifdef JP
if (flags4 & RF4_DISPEL)    {vp[vn] = "魔力消去";color[vn++] = TERM_L_WHITE;}
#else
	if (flags4 & RF4_DISPEL)    {vp[vn] = "dispel-magic";color[vn++] = TERM_L_WHITE;}
#endif

#ifdef JP
if (flags6 & (RF6_BLINK))           {vp[vn] = "ショートテレポート";color[vn++] = TERM_UMBER;}
#else
	if (flags6 & (RF6_BLINK))           {vp[vn] = "blink-self";color[vn++] = TERM_UMBER;}
#endif

#ifdef JP
if (flags6 & (RF6_TPORT))           {vp[vn] = "テレポート";color[vn++] = TERM_ORANGE;}
#else
	if (flags6 & (RF6_TPORT))           {vp[vn] = "teleport-self";color[vn++] = TERM_ORANGE;}
#endif

#ifdef JP
if (flags6 & (RF6_WORLD))            {vp[vn] = "時を止める";color[vn++] = TERM_L_BLUE;}
#else
	if (flags6 & (RF6_WORLD))            {vp[vn] = "stop the time";color[vn++] = TERM_L_BLUE;}
#endif

#ifdef JP
if (flags6 & (RF6_TELE_TO))         {vp[vn] = "テレポートバック";color[vn++] = TERM_L_UMBER;}
#else
	if (flags6 & (RF6_TELE_TO))         {vp[vn] = "teleport to";color[vn++] = TERM_L_UMBER;}
#endif

#ifdef JP
if (flags6 & (RF6_TELE_AWAY))       {vp[vn] = "テレポートアウェイ";color[vn++] = TERM_UMBER;}
#else
	if (flags6 & (RF6_TELE_AWAY))       {vp[vn] = "teleport away";color[vn++] = TERM_UMBER;}
#endif

#ifdef JP
if (flags6 & (RF6_TELE_LEVEL))      {vp[vn] = "テレポート・レベル";color[vn++] = TERM_ORANGE;}
#else
	if (flags6 & (RF6_TELE_LEVEL))      {vp[vn] = "teleport level";color[vn++] = TERM_ORANGE;}
#endif

#ifdef JP
if (flags6 & (RF6_DARKNESS))        {if ((p_ptr->pclass != CLASS_NINJA) || (flags3 & (RF3_UNDEAD | RF3_HURT_LITE))) {vp[vn] =  "暗闇";color[vn++] = TERM_L_DARK;} else { vp[vn] = "閃光";color[vn++] = TERM_YELLOW;}}
#else
if (flags6 & (RF6_DARKNESS))        {vp[vn] = ((p_ptr->pclass != CLASS_NINJA) || (flags3 & (RF3_UNDEAD | RF3_HURT_LITE))) ? "create darkness" : "create light";color[vn++] = TERM_L_DARK;}
#endif

#ifdef JP
if (flags6 & (RF6_TRAPS))           {vp[vn] = "トラップ";color[vn++] = TERM_BLUE;}
#else
	if (flags6 & (RF6_TRAPS))           {vp[vn] = "create traps";color[vn++] = TERM_BLUE;}
#endif

#ifdef JP
if (flags6 & (RF6_FORGET))          {vp[vn] = "記憶消去";color[vn++] = TERM_BLUE;}
#else
	if (flags6 & (RF6_FORGET))          {vp[vn] = "cause amnesia";color[vn++] = TERM_BLUE;}
#endif

#ifdef JP
if (flags6 & (RF6_RAISE_DEAD))      {vp[vn] = "死者復活";color[vn++] = TERM_RED;}
#else
	if (flags6 & (RF6_RAISE_DEAD))      {vp[vn] = "raise dead";color[vn++] = TERM_RED;}
#endif

#ifdef JP
if (flags6 & (RF6_S_MONSTER))       {vp[vn] = "モンスター一体召喚";color[vn++] = TERM_SLATE;}
#else
	if (flags6 & (RF6_S_MONSTER))       {vp[vn] = "summon a monster";color[vn++] = TERM_SLATE;}
#endif

#ifdef JP
if (flags6 & (RF6_S_MONSTERS))      {vp[vn] = "モンスター複数召喚";color[vn++] = TERM_L_WHITE;}
#else
	if (flags6 & (RF6_S_MONSTERS))      {vp[vn] = "summon monsters";color[vn++] = TERM_L_WHITE;}
#endif

#ifdef JP
if (flags6 & (RF6_S_KIN))           {vp[vn] = "救援召喚";color[vn++] = TERM_ORANGE;}
#else
	if (flags6 & (RF6_S_KIN))           {vp[vn] = "summon aid";color[vn++] = TERM_ORANGE;}
#endif

#ifdef JP
if (flags6 & (RF6_S_ANT))           {vp[vn] = "アリ召喚";color[vn++] = TERM_RED;}
#else
	if (flags6 & (RF6_S_ANT))           {vp[vn] = "summon ants";color[vn++] = TERM_RED;}
#endif

#ifdef JP
if (flags6 & (RF6_S_SPIDER))        {vp[vn] = "クモ召喚";color[vn++] = TERM_L_DARK;}
#else
	if (flags6 & (RF6_S_SPIDER))        {vp[vn] = "summon spiders";color[vn++] = TERM_L_DARK;}
#endif

#ifdef JP
if (flags6 & (RF6_S_HOUND))         {vp[vn] = "ハウンド召喚";color[vn++] = TERM_L_UMBER;}
#else
	if (flags6 & (RF6_S_HOUND))         {vp[vn] = "summon hounds";color[vn++] = TERM_L_UMBER;}
#endif

#ifdef JP
if (flags6 & (RF6_S_HYDRA))         {vp[vn] = "ヒドラ召喚";color[vn++] = TERM_L_GREEN;}
#else
	if (flags6 & (RF6_S_HYDRA))         {vp[vn] = "summon hydras";color[vn++] = TERM_L_GREEN;}
#endif

#ifdef JP
if (flags6 & (RF6_S_ANGEL))         {vp[vn] = "天使一体召喚";color[vn++] = TERM_YELLOW;}
#else
	if (flags6 & (RF6_S_ANGEL))         {vp[vn] = "summon an angel";color[vn++] = TERM_YELLOW;}
#endif

#ifdef JP
if (flags6 & (RF6_S_DEMON))         {vp[vn] = "デーモン一体召喚";color[vn++] = TERM_L_RED;}
#else
	if (flags6 & (RF6_S_DEMON))         {vp[vn] = "summon a demon";color[vn++] = TERM_L_RED;}
#endif

#ifdef JP
if (flags6 & (RF6_S_UNDEAD))        {vp[vn] = "アンデッド一体召喚";color[vn++] = TERM_L_DARK;}
#else
	if (flags6 & (RF6_S_UNDEAD))        {vp[vn] = "summon an undead";color[vn++] = TERM_L_DARK;}
#endif

#ifdef JP
if (flags6 & (RF6_S_DRAGON))        {vp[vn] = "ドラゴン一体召喚";color[vn++] = TERM_ORANGE;}
#else
	if (flags6 & (RF6_S_DRAGON))        {vp[vn] = "summon a dragon";color[vn++] = TERM_ORANGE;}
#endif

#ifdef JP
if (flags6 & (RF6_S_HI_UNDEAD))     {vp[vn] = "強力なアンデッド召喚";color[vn++] = TERM_L_DARK;}
#else
	if (flags6 & (RF6_S_HI_UNDEAD))     {vp[vn] = "summon Greater Undead";color[vn++] = TERM_L_DARK;}
#endif

#ifdef JP
if (flags6 & (RF6_S_HI_DRAGON))     {vp[vn] = "古代ドラゴン召喚";color[vn++] = TERM_ORANGE;}
#else
	if (flags6 & (RF6_S_HI_DRAGON))     {vp[vn] = "summon Ancient Dragons";color[vn++] = TERM_ORANGE;}
#endif

#ifdef JP
if (flags6 & (RF6_S_CYBER))         {vp[vn] = "サイバーデーモン召喚";color[vn++] = TERM_UMBER;}
#else
	if (flags6 & (RF6_S_CYBER))         {vp[vn] = "summon Cyberdemons";color[vn++] = TERM_UMBER;}
#endif

#ifdef JP
if (flags6 & (RF6_S_AMBERITES))     {vp[vn] = "アンバーの王族召喚";color[vn++] = TERM_VIOLET;}
#else
	if (flags6 & (RF6_S_AMBERITES))     {vp[vn] = "summon Lords of Amber";color[vn++] = TERM_VIOLET;}
#endif

#ifdef JP
if (flags6 & (RF6_S_UNIQUE))        {vp[vn] = "ユニーク・モンスター召喚";color[vn++] = TERM_VIOLET;}
#else
	if (flags6 & (RF6_S_UNIQUE))        {vp[vn] = "summon Unique Monsters";color[vn++] = TERM_VIOLET;}
#endif


	/* Describe spells */
	if (vn)
	{
		/* Note magic */
		magic = TRUE;

		/* Intro */
		if (breath)
		{
#ifdef JP
			roff("、なおかつ");
#else
			roff(", and is also");
#endif

		}
		else
		{
#ifdef JP
			roff(format("%^sは", wd_he[msex]));
#else
			roff(format("%^s is", wd_he[msex]));
#endif

		}

#ifdef JP
		/* Adverb */
		if (flags2 & (RF2_SMART)) c_roff(TERM_YELLOW, "的確に");

		/* Verb Phrase */
		roff("魔法を使うことができ、");
#else
		/* Verb Phrase */
		roff(" magical, casting spells");

		/* Adverb */
		if (flags2 & RF2_SMART) roff(" intelligently");
#endif


		/* Scan */
		for (n = 0; n < vn; n++)
		{
			/* Intro */
#ifdef JP
			if ( n != 0 ) roff("、");
#else
			if (n == 0) roff(" which ");
			else if (n < vn-1) roff(", ");
			else roff(" or ");
#endif


			/* Dump */
			c_roff(color[n], vp[n]);
		}
#ifdef JP
                roff("の呪文を唱えることがある");
#endif
	}


	/* End the sentence about inate/other spells */
	if (breath || magic)
	{
		/* Total casting */
		m = r_ptr->r_cast_inate + r_ptr->r_cast_spell;

		/* Average frequency */
		n = (r_ptr->freq_inate + r_ptr->freq_spell) / 2;

		/* Describe the spell frequency */
		if (m > 100)
		{
#ifdef JP
			roff(format("(確率:1/%d)", 100 / n));
#else
			roff(format("; 1 time in %d", 100 / n));
#endif

		}

		/* Guess at the frequency */
		else if (m)
		{
			n = ((n + 9) / 10) * 10;
#ifdef JP
			roff(format("(確率:約1/%d)", 100 / n));
#else
			roff(format("; about 1 time in %d", 100 / n));
#endif

		}

		/* End this sentence */
#ifdef JP
		roff("。");
#else
		roff(".  ");
#endif

	}

	/* Describe monster "toughness" */
	if (know_armour(r_idx))
	{
		/* Armor */
#ifdef JP
		roff(format("%^sは AC%d の防御力と",
#else
		roff(format("%^s has an armor rating of %d",
#endif

		            wd_he[msex], r_ptr->ac));

		/* Maximized hitpoints */
		if (flags1 & RF1_FORCE_MAXHP)
		{
#ifdef JP
			roff(format(" %d の体力がある。",
#else
			roff(format(" and a life rating of %d.  ",
#endif

			            r_ptr->hdice * r_ptr->hside));
		}

		/* Variable hitpoints */
		else
		{
#ifdef JP
			roff(format(" %dd%d の体力がある。",
#else
			roff(format(" and a life rating of %dd%d.  ",
#endif

			            r_ptr->hdice, r_ptr->hside));
		}
	}



	/* Collect special abilities. */
	vn = 0;
#ifdef JP
if (flags7 & (RF7_HAS_LITE_1 | RF7_HAS_LITE_2)) vp[vn++] = "ダンジョンを照らす";
#else
	if (flags7 & (RF7_HAS_LITE_1 | RF7_HAS_LITE_2)) vp[vn++] = "illuminate the dungeon";
#endif

#ifdef JP
if (flags2 & RF2_OPEN_DOOR) vp[vn++] = "ドアを開ける";
#else
	if (flags2 & RF2_OPEN_DOOR) vp[vn++] = "open doors";
#endif

#ifdef JP
if (flags2 & RF2_BASH_DOOR) vp[vn++] = "ドアを打ち破る";
#else
	if (flags2 & RF2_BASH_DOOR) vp[vn++] = "bash down doors";
#endif

#ifdef JP
if (flags2 & RF2_PASS_WALL) vp[vn++] = "壁をすり抜ける";
#else
	if (flags2 & RF2_PASS_WALL) vp[vn++] = "pass through walls";
#endif

#ifdef JP
if (flags2 & RF2_KILL_WALL) vp[vn++] = "壁を掘り進む";
#else
	if (flags2 & RF2_KILL_WALL) vp[vn++] = "bore through walls";
#endif

#ifdef JP
if (flags2 & RF2_MOVE_BODY) vp[vn++] = "弱いモンスターを押しのける";
#else
	if (flags2 & RF2_MOVE_BODY) vp[vn++] = "push past weaker monsters";
#endif

#ifdef JP
if (flags2 & RF2_KILL_BODY) vp[vn++] = "弱いモンスターを倒す";
#else
	if (flags2 & RF2_KILL_BODY) vp[vn++] = "destroy weaker monsters";
#endif

#ifdef JP
if (flags2 & RF2_TAKE_ITEM) vp[vn++] = "アイテムを拾う";
#else
	if (flags2 & RF2_TAKE_ITEM) vp[vn++] = "pick up objects";
#endif

#ifdef JP
if (flags2 & RF2_KILL_ITEM) vp[vn++] = "アイテムを壊す";
#else
	if (flags2 & RF2_KILL_ITEM) vp[vn++] = "destroy objects";
#endif


	/* Describe special abilities. */
	if (vn)
	{
		/* Intro */
#ifdef JP
		roff(format("%^sは", wd_he[msex]));
#else
		roff(format("%^s", wd_he[msex]));
#endif


		/* Scan */
		for (n = 0; n < vn; n++)
		{
			/* Intro */
#ifdef JP
			if(n!=vn-1){
			  jverb(vp[n],jverb_buf,JVERB_AND);
			  roff(jverb_buf);
			  roff("、");
			}
			else  roff(vp[n]);
#else
			if (n == 0) roff(" can ");
			else if (n < vn-1) roff(", ");
			else roff(" and ");

			/* Dump */
			roff(vp[n]);
#endif

		}

		/* End */
#ifdef JP
		roff("ことができる。");
#else
		roff(".  ");
#endif

	}


	/* Describe special abilities. */
	if (flags7 & (RF7_SELF_LITE_1 | RF7_SELF_LITE_2))
	{
#ifdef JP
		roff(format("%^sは光っている。", wd_he[msex]));
#else
		roff(format("%^s illuminate the dungeon.  ", wd_he[msex]));
#endif

	}
	if (flags2 & RF2_INVISIBLE)
	{
#ifdef JP
		roff(format("%^sは透明で目に見えない。", wd_he[msex]));
#else
		roff(format("%^s is invisible.  ", wd_he[msex]));
#endif

	}
	if (flags2 & RF2_COLD_BLOOD)
	{
#ifdef JP
		roff(format("%^sは冷血動物である。", wd_he[msex]));
#else
		roff(format("%^s is cold blooded.  ", wd_he[msex]));
#endif

	}
	if (flags2 & RF2_EMPTY_MIND)
	{
#ifdef JP
		roff(format("%^sはテレパシーでは感知できない。", wd_he[msex]));
#else
		roff(format("%^s is not detected by telepathy.  ", wd_he[msex]));
#endif

	}
	else if (flags2 & RF2_WEIRD_MIND)
	{
#ifdef JP
		roff(format("%^sはまれにテレパシーで感知できる。", wd_he[msex]));
#else
		roff(format("%^s is rarely detected by telepathy.  ", wd_he[msex]));
#endif

	}
	if (flags2 & RF2_MULTIPLY)
	{
#ifdef JP
		c_roff(TERM_L_UMBER, format("%^sは爆発的に増殖する。", wd_he[msex]));
#else
		c_roff(TERM_L_UMBER, format("%^s breeds explosively.  ", wd_he[msex]));
#endif

	}
	if (flags2 & RF2_REGENERATE)
	{
#ifdef JP
		c_roff(TERM_L_WHITE, format("%^sは素早く体力を回復する。", wd_he[msex]));
#else
		c_roff(TERM_L_WHITE, format("%^s regenerates quickly.  ", wd_he[msex]));
#endif

	}
	if (flags7 & RF7_RIDING)
	{
#ifdef JP
		c_roff(TERM_SLATE, format("%^sに乗ることができる。", wd_he[msex]));
#else
		c_roff(TERM_SLATE, format("%^s is suitable for riding.  ", wd_he[msex]));
#endif

	}


	/* Collect susceptibilities */
	vn = 0;
#ifdef JP
	if (flags3 & RF3_HURT_ROCK) {vp[vn] = "岩を除去するもの";color[vn++] = TERM_UMBER;}
#else
	if (flags3 & RF3_HURT_ROCK) {vp[vn] = "rock remover";color[vn++] = TERM_UMBER;}
#endif

#ifdef JP
	if (flags3 & RF3_HURT_LITE) {vp[vn] = "明るい光";color[vn++] = TERM_YELLOW;}
#else
	if (flags3 & RF3_HURT_LITE) {vp[vn] = "bright light";color[vn++] = TERM_YELLOW;}
#endif

#ifdef JP
	if (flags3 & RF3_HURT_FIRE) {vp[vn] = "炎";color[vn++] = TERM_RED;}
#else
	if (flags3 & RF3_HURT_FIRE) {vp[vn] = "fire";color[vn++] = TERM_RED;}
#endif

#ifdef JP
	if (flags3 & RF3_HURT_COLD) {vp[vn] = "冷気";color[vn++] = TERM_L_WHITE;}
#else
	if (flags3 & RF3_HURT_COLD) {vp[vn] = "cold";color[vn++] = TERM_L_WHITE;}
#endif


	/* Describe susceptibilities */
	if (vn)
	{
		/* Intro */
#ifdef JP
		roff(format("%^sには", wd_he[msex]));
#else
		roff(format("%^s", wd_he[msex]));
#endif


		/* Scan */
		for (n = 0; n < vn; n++)
		{
			/* Intro */
#ifdef JP
			if ( n != 0 ) roff("や");
#else
			if (n == 0) roff(" is hurt by ");
			else if (n < vn-1) roff(", ");
			else roff(" and ");
#endif


			/* Dump */
			c_roff(color[n], vp[n]);
		}

		/* End */
#ifdef JP
		roff("でダメージを与えられる。");
#else
		roff(".  ");
#endif

	}


	/* Collect immunities */
	vn = 0;
#ifdef JP
	if (flags3 & RF3_IM_ACID) {vp[vn] = "酸";color[vn++] = TERM_GREEN;}
#else
	if (flags3 & RF3_IM_ACID) {vp[vn] = "acid";color[vn++] = TERM_GREEN;}
#endif

#ifdef JP
	if (flags3 & RF3_IM_ELEC) {vp[vn] = "稲妻";color[vn++] = TERM_BLUE;}
#else
	if (flags3 & RF3_IM_ELEC) {vp[vn] = "lightning";color[vn++] = TERM_BLUE;}
#endif

#ifdef JP
	if (flags3 & RF3_IM_FIRE) {vp[vn] = "炎";color[vn++] = TERM_RED;}
#else
	if (flags3 & RF3_IM_FIRE) {vp[vn] = "fire";color[vn++] = TERM_RED;}
#endif

#ifdef JP
	if (flags3 & RF3_IM_COLD) {vp[vn] = "冷気";color[vn++] = TERM_L_WHITE;}
#else
	if (flags3 & RF3_IM_COLD) {vp[vn] = "cold";color[vn++] = TERM_L_WHITE;}
#endif

#ifdef JP
	if (flags3 & RF3_IM_POIS) {vp[vn] = "毒";color[vn++] = TERM_L_GREEN;}
#else
	if (flags3 & RF3_IM_POIS) {vp[vn] = "poison";color[vn++] = TERM_L_GREEN;}
#endif


	/* Describe immunities */
	if (vn)
	{
		/* Intro */
#ifdef JP
		roff(format("%^sは", wd_he[msex]));
#else
		roff(format("%^s", wd_he[msex]));
#endif


		/* Scan */
		for (n = 0; n < vn; n++)
		{
			/* Intro */
#ifdef JP
			if ( n != 0 ) roff("と");
#else
			if (n == 0) roff(" resists ");
			else if (n < vn-1) roff(", ");
			else roff(" and ");
#endif


			/* Dump */
			c_roff(color[n], vp[n]);
		}

		/* End */
#ifdef JP
		roff("の耐性を持っている。");
#else
		roff(".  ");
#endif

	}


	/* Collect resistances */
	vn = 0;
#ifdef JP
if (flags4 & RF4_BR_LITE) {vp[vn] = "閃光";color[vn++] = TERM_YELLOW;}
#else
 if (flags4 & RF4_BR_LITE) {vp[vn] = "light";color[vn++] = TERM_YELLOW;}
#endif

#ifdef JP
if ((flags4 & RF4_BR_DARK) || (flags3 & RF3_ORC)) {vp[vn] = "暗黒";color[vn++] = TERM_L_DARK;}
#else
 if (flags4 & RF4_BR_DARK  || (flags3 & RF3_ORC)) {vp[vn] = "dark";color[vn++] = TERM_L_DARK;}
#endif

#ifdef JP
if (flags3 & RF3_RES_NETH) {vp[vn] = "地獄";color[vn++] = TERM_L_DARK;}
#else
 if (flags3 & RF3_RES_NETH) {vp[vn] = "nether";color[vn++] = TERM_L_DARK;}
#endif

#ifdef JP
if (flags3 & RF3_RES_WATE) {vp[vn] = "水";color[vn++] = TERM_BLUE;}
#else
if (flags3 & RF3_RES_WATE) {vp[vn] = "water";color[vn++] = TERM_BLUE;}
#endif

#ifdef JP
if (flags3 & RF3_RES_PLAS) {vp[vn] = "プラズマ";color[vn++] = TERM_L_RED;}
#else
 if (flags3 & RF3_RES_PLAS) {vp[vn] = "plasma";color[vn++] = TERM_L_RED;}
#endif

#ifdef JP
if (flags4 & RF4_BR_SHAR) {vp[vn] = "破片";color[vn++] = TERM_L_UMBER;}
#else
 if (flags4 & RF4_BR_SHAR) {vp[vn] = "shards";color[vn++] = TERM_L_UMBER;}
#endif

#ifdef JP
if (flags4 & RF4_BR_SOUN) {vp[vn] = "轟音";color[vn++] = TERM_ORANGE;}
#else
 if (flags4 & RF4_BR_SOUN) {vp[vn] = "sound";color[vn++] = TERM_ORANGE;}
#endif

#ifdef JP
if (flags4 & RF4_BR_CONF) {vp[vn] = "混乱";color[vn++] = TERM_L_UMBER;}
#else
 if (flags4 & RF4_BR_CONF) {vp[vn] = "conf";color[vn++] = TERM_L_UMBER;}
#endif

#ifdef JP
if ((flags4 & RF4_BR_CHAO) || (r_idx == MON_STORMBRINGER)) {vp[vn] = "カオス";color[vn++] = TERM_VIOLET;}
#else
if ((flags4 & RF4_BR_CHAO) || (r_idx == MON_STORMBRINGER)) {vp[vn] = "chaos";color[vn++] = TERM_VIOLET;}
#endif

#ifdef JP
if (flags3 & RF3_RES_NEXU) {vp[vn] = "因果混乱";color[vn++] = TERM_VIOLET;}
#else
 if (flags3 & RF3_RES_NEXU) {vp[vn] = "nexus";color[vn++] = TERM_VIOLET;}
#endif

#ifdef JP
if (flags3 & RF3_RES_DISE) {vp[vn] = "劣化";color[vn++] = TERM_VIOLET;}
#else
 if (flags3 & RF3_RES_DISE) {vp[vn] = "disenchantment";color[vn++] = TERM_VIOLET;}
#endif

#ifdef JP
if (flags4 & RF4_BR_WALL) {vp[vn] = "フォース";color[vn++] = TERM_UMBER;}
#else
 if (flags4 & RF4_BR_WALL) {vp[vn] = "sound";color[vn++] = TERM_UMBER;}
#endif

#ifdef JP
if (flags4 & RF4_BR_INER) {vp[vn] = "遅鈍";color[vn++] = TERM_SLATE;}
#else
 if (flags4 & RF4_BR_INER) {vp[vn] = "inertia";color[vn++] = TERM_SLATE;}
#endif

#ifdef JP
if (flags4 & RF4_BR_TIME) {vp[vn] = "時間逆転";color[vn++] = TERM_L_BLUE;}
#else
 if (flags4 & RF4_BR_TIME) {vp[vn] = "time";color[vn++] = TERM_L_BLUE;}
#endif

#ifdef JP
if (flags4 & RF4_BR_GRAV) {vp[vn] = "重力";color[vn++] = TERM_SLATE;}
#else
 if (flags4 & RF4_BR_GRAV) {vp[vn] = "gravity";color[vn++] = TERM_SLATE;}
#endif

#ifdef JP
if (flags3 & RF3_RES_ALL) {vp[vn] = "あらゆる攻撃";color[vn++] = TERM_YELLOW;}
#else
 if (flags3 & RF3_RES_ALL) {vp[vn] = "all";color[vn++] = TERM_YELLOW;}
#endif

#ifdef JP
if ((flags3 & RF3_RES_TELE) && !(r_ptr->flags1 & RF1_UNIQUE)) {vp[vn] = "テレポート";color[vn++] = TERM_ORANGE;}
#else
 if ((flags3 & RF3_RES_TELE) && !(r_ptr->flags1 & RF1_UNIQUE)) {vp[vn] = "teleportation";color[vn++] = TERM_ORANGE;}
#endif


	/* Describe resistances */
	if (vn)
	{
		/* Intro */
#ifdef JP
		roff(format("%^sは", wd_he[msex]));
#else
		roff(format("%^s", wd_he[msex]));
#endif


		/* Scan */
		for (n = 0; n < vn; n++)
		{
			/* Intro */
#ifdef JP
			if ( n != 0 ) roff("と");
#else
			if (n == 0) roff(" resists ");
			else if (n < vn-1) roff(", ");
			else roff(" and ");
#endif


			/* Dump */
			c_roff(color[n], vp[n]);
		}

		/* End */
#ifdef JP
		roff("の耐性を持っている。");
#else
		roff(".  ");
#endif

	}


	if ((r_ptr->r_xtra1 & MR1_SINKA) || cheat_know)
	{
		if (r_ptr->next_r_idx)
		{
#ifdef JP
			roff(format("%^sは経験を積むと、", wd_he[msex]));
#else
			roff(format("%^s will evolve into ", wd_he[msex]));
#endif
			c_roff(TERM_YELLOW, format("%s", r_name+r_info[r_ptr->next_r_idx].name));
#ifdef JP
			roff(format("に進化する。"));
#else
			roff(format(" when %s gets enugh experience.  ", wd_he[msex]));
#endif
		}
		else if (!(r_ptr->flags1 & RF1_UNIQUE))
		{
#ifdef JP
			roff(format("%sは進化しない。", wd_he[msex]));
#else
			roff(format("%s won't evolve.  ", wd_he[msex]));
#endif
		}
	}

	/* Collect non-effects */
	vn = 0;
#ifdef JP
if (flags3 & RF3_NO_STUN)  {vp[vn] = "朦朧としない";color[vn++] = TERM_ORANGE;}
#else
 if (flags3 & RF3_NO_STUN)  {vp[vn] = "stunned";color[vn++] = TERM_ORANGE;}
#endif

#ifdef JP
if (flags3 & RF3_NO_FEAR)  {vp[vn] = "恐怖を感じない";color[vn++] = TERM_SLATE;}
#else
 if (flags3 & RF3_NO_FEAR)  {vp[vn] = "frightened";color[vn++] = TERM_SLATE;}
#endif

#ifdef JP
if (flags3 & RF3_NO_CONF)  {vp[vn] = "混乱しない";color[vn++] = TERM_L_UMBER;}
#else
 if (flags3 & RF3_NO_CONF)  {vp[vn] = "confused";color[vn++] = TERM_L_UMBER;}
#endif

#ifdef JP
if (flags3 & RF3_NO_SLEEP) {vp[vn] = "眠らされない";color[vn++] = TERM_BLUE;}
#else
 if (flags3 & RF3_NO_SLEEP) {vp[vn] = "slept";color[vn++] = TERM_BLUE;}
#endif

#ifdef JP
if ((flags3 & RF3_RES_TELE) && (r_ptr->flags1 & RF1_UNIQUE)) {vp[vn] = "テレポートされない";color[vn++] = TERM_ORANGE;}
#else
 if ((flags3 & RF3_RES_TELE) && (r_ptr->flags1 & RF1_UNIQUE)) {vp[vn] = "teleported";color[vn++] = TERM_ORANGE;}
#endif

	/* Describe non-effects */
	if (vn)
	{
		/* Intro */
#ifdef JP
		roff(format("%^sは", wd_he[msex]));
#else
		roff(format("%^s", wd_he[msex]));
#endif


		/* Scan */
		for (n = 0; n < vn; n++)
		{
			/* Intro */
#ifdef JP
			if ( n != 0 ) roff("し、");
#else
			if (n == 0) roff(" cannot be ");
			else if (n < vn - 1) roff(", ");
			else roff(" or ");
#endif


			/* Dump */
			c_roff(color[n], vp[n]);
		}

		/* End */
#ifdef JP
		roff("。");
#else
		roff(".  ");
#endif

	}


	/* Do we know how aware it is? */
	if ((((int)r_ptr->r_wake * (int)r_ptr->r_wake) > r_ptr->sleep) ||
	          (r_ptr->r_ignore == MAX_UCHAR) ||
	         ((r_ptr->sleep == 0) && ((r_ptr->r_tkills >= 10) || cheat_know)))
	{
		cptr act;

		if (r_ptr->sleep > 200)
		{
#ifdef JP
			act = "を無視しがちであるが";
#else
			act = "prefers to ignore";
#endif

		}
		else if (r_ptr->sleep > 95)
		{
#ifdef JP
			act = "に対してほとんど注意を払わないが";
#else
			act = "pays very little attention to";
#endif

		}
		else if (r_ptr->sleep > 75)
		{
#ifdef JP
			act = "に対してあまり注意を払わないが";
#else
			act = "pays little attention to";
#endif

		}
		else if (r_ptr->sleep > 45)
		{
#ifdef JP
			act = "を見過ごしがちであるが";
#else
			act = "tends to overlook";
#endif

		}
		else if (r_ptr->sleep > 25)
		{
#ifdef JP
			act = "をほんの少しは見ており";
#else
			act = "takes quite a while to see";
#endif

		}
		else if (r_ptr->sleep > 10)
		{
#ifdef JP
			act = "をしばらくは見ており";
#else
			act = "takes a while to see";
#endif

		}
		else if (r_ptr->sleep > 5)
		{
#ifdef JP
			act = "を幾分注意深く見ており";
#else
			act = "is fairly observant of";
#endif

		}
		else if (r_ptr->sleep > 3)
		{
#ifdef JP
			act = "を注意深く見ており";
#else
			act = "is observant of";
#endif

		}
		else if (r_ptr->sleep > 1)
		{
#ifdef JP
			act = "をかなり注意深く見ており";
#else
			act = "is very observant of";
#endif

		}
		else if (r_ptr->sleep > 0)
		{
#ifdef JP
			act = "を警戒しており";
#else
			act = "is vigilant for";
#endif

		}
		else
		{
#ifdef JP
			act = "をかなり警戒しており";
#else
			act = "is ever vigilant for";
#endif

		}

#ifdef JP
		roff(format("%^sは侵入者%s、 %d フィート先から侵入者に気付くことがある。",
		     wd_he[msex], act, 10 * r_ptr->aaf));
#else
		roff(format("%^s %s intruders, which %s may notice from %d feet.  ",
		            wd_he[msex], act, wd_he[msex], 10 * r_ptr->aaf));
#endif

	}


	/* Drops gold and/or items */
	if (r_ptr->r_drop_gold || r_ptr->r_drop_item)
	{
		/* No "n" needed */
		sin = FALSE;

		/* Intro */
#ifdef JP
		roff(format("%^sは", wd_he[msex]));
#else
		roff(format("%^s may carry", wd_he[msex]));
#endif


		/* Count maximum drop */
		n = MAX(r_ptr->r_drop_gold, r_ptr->r_drop_item);

		/* One drop (may need an "n") */
		if (n == 1)
		{
#ifdef JP
			roff("一つの");
#else
			roff(" a");
#endif

			sin = TRUE;
		}

		/* Two drops */
		else if (n == 2)
		{
#ifdef JP
			roff("一つか二つの");
#else
			roff(" one or two");
#endif

		}

		/* Many drops */
		else
		{
#ifdef JP
			roff(format(" %d 個までの", n));
#else
			roff(format(" up to %d", n));
#endif

		}


		/* Great */
		if (flags1 & RF1_DROP_GREAT)
		{
#ifdef JP
			p = "特別な";
#else
			p = " exceptional";
#endif

		}

		/* Good (no "n" needed) */
		else if (flags1 & RF1_DROP_GOOD)
		{
#ifdef JP
			p = "上質な";
#else
			p = " good";
#endif

			sin = FALSE;
		}

		/* Okay */
		else
		{
			p = NULL;
		}


		/* Objects */
		if (r_ptr->r_drop_item)
		{
			/* Handle singular "an" */
#ifndef JP
			if (sin) roff("n");
#endif
			sin = FALSE;

			/* Dump "object(s)" */
			if (p) roff(p);
#ifdef JP
			roff("アイテム");
#else
			roff(" object");
			if (n != 1) roff("s");
#endif


			/* Conjunction replaces variety, if needed for "gold" below */
#ifdef JP
			p = "や";
#else
			p = " or";
#endif

		}

		/* Treasures */
		if (r_ptr->r_drop_gold)
		{
			/* Cancel prefix */
			if (!p) sin = FALSE;

			/* Handle singular "an" */
#ifndef JP
			if (sin) roff("n");
#endif
			sin = FALSE;

			/* Dump "treasure(s)" */
			if (p) roff(p);
#ifdef JP
			roff("財宝");
#else
			roff(" treasure");
			if (n != 1) roff("s");
#endif

		}

		/* End this sentence */
#ifdef JP
		roff("を持っていることがある。");
#else
		roff(".  ");
#endif

	}


	/* Count the number of "known" attacks */
	for (n = 0, m = 0; m < 4; m++)
	{
		/* Skip non-attacks */
		if (!r_ptr->blow[m].method) continue;
		if (r_ptr->blow[m].method == RBM_SHOOT) continue;

		/* Count known attacks */
		if (r_ptr->r_blows[m]) n++;
	}

	/* Examine (and count) the actual attacks */
	for (r = 0, m = 0; m < 4; m++)
	{
		int method, effect, d1, d2;

		/* Skip non-attacks */
		if (!r_ptr->blow[m].method) continue;
		if (r_ptr->blow[m].method == RBM_SHOOT) continue;

		/* Skip unknown attacks */
		if (!r_ptr->r_blows[m]) continue;

		/* Extract the attack info */
		method = r_ptr->blow[m].method;
		effect = r_ptr->blow[m].effect;
		d1 = r_ptr->blow[m].d_dice;
		d2 = r_ptr->blow[m].d_side;

		/* No method yet */
		p = NULL;

		/* Acquire the method */
		switch (method)
		{
#ifdef JP
case RBM_HIT:		p = "殴る"; break;
#else
			case RBM_HIT:		p = "hit"; break;
#endif

#ifdef JP
case RBM_TOUCH:		p = "触る"; break;
#else
			case RBM_TOUCH:		p = "touch"; break;
#endif

#ifdef JP
case RBM_PUNCH:		p = "パンチする"; break;
#else
			case RBM_PUNCH:		p = "punch"; break;
#endif

#ifdef JP
case RBM_KICK:		p = "蹴る"; break;
#else
			case RBM_KICK:		p = "kick"; break;
#endif

#ifdef JP
case RBM_CLAW:		p = "ひっかく"; break;
#else
			case RBM_CLAW:		p = "claw"; break;
#endif

#ifdef JP
case RBM_BITE:		p = "噛む"; break;
#else
			case RBM_BITE:		p = "bite"; break;
#endif

#ifdef JP
case RBM_STING:		p = "刺す"; break;
#else
			case RBM_STING:		p = "sting"; break;
#endif

#ifdef JP
case RBM_SLASH:		p = "斬る"; break;
#else
			case RBM_SLASH:		p = "slash"; break;
#endif

#ifdef JP
case RBM_BUTT:		p = "角で突く"; break;
#else
			case RBM_BUTT:		p = "butt"; break;
#endif

#ifdef JP
case RBM_CRUSH:		p = "体当たりする"; break;
#else
			case RBM_CRUSH:		p = "crush"; break;
#endif

#ifdef JP
case RBM_ENGULF:	p = "飲み込む"; break;
#else
			case RBM_ENGULF:	p = "engulf"; break;
#endif

#ifdef JP
case RBM_CHARGE: 	p = "請求書をよこす"; break;
#else
			case RBM_CHARGE: 	p = "charge";   break;
#endif

#ifdef JP
case RBM_CRAWL:		p = "体の上を這い回る"; break;
#else
			case RBM_CRAWL:		p = "crawl on you"; break;
#endif

#ifdef JP
case RBM_DROOL:		p = "よだれをたらす"; break;
#else
			case RBM_DROOL:		p = "drool on you"; break;
#endif

#ifdef JP
case RBM_SPIT:		p = "つばを吐く"; break;
#else
			case RBM_SPIT:		p = "spit"; break;
#endif

#ifdef JP
case RBM_EXPLODE:	p = "爆発する"; break;
#else
			case RBM_EXPLODE:	p = "explode"; break;
#endif

#ifdef JP
case RBM_GAZE:		p = "にらむ"; break;
#else
			case RBM_GAZE:		p = "gaze"; break;
#endif

#ifdef JP
case RBM_WAIL:		p = "泣き叫ぶ"; break;
#else
			case RBM_WAIL:		p = "wail"; break;
#endif

#ifdef JP
case RBM_SPORE:		p = "胞子を飛ばす"; break;
#else
			case RBM_SPORE:		p = "release spores"; break;
#endif

			case RBM_XXX4:		break;
#ifdef JP
case RBM_BEG:		p = "金をせがむ"; break;
#else
			case RBM_BEG:		p = "beg"; break;
#endif

#ifdef JP
case RBM_INSULT:	p = "侮辱する"; break;
#else
			case RBM_INSULT:	p = "insult"; break;
#endif

#ifdef JP
case RBM_MOAN:		p = "うめく"; break;
#else
			case RBM_MOAN:		p = "moan"; break;
#endif

#ifdef JP
case RBM_SHOW:  	p = "歌う"; break;
#else
			case RBM_SHOW:  	p = "sing"; break;
#endif

		}


		/* Default effect */
		q = NULL;

		/* Acquire the effect */
		switch (effect)
		{
#ifdef JP
case RBE_SUPERHURT:
case RBE_HURT:    	q = "攻撃する"; break;
#else
			case RBE_SUPERHURT:
			case RBE_HURT:    	q = "attack"; break;
#endif

#ifdef JP
case RBE_POISON:  	q = "毒をくらわす"; break;
#else
			case RBE_POISON:  	q = "poison"; break;
#endif

#ifdef JP
case RBE_UN_BONUS:	q = "劣化させる"; break;
#else
			case RBE_UN_BONUS:	q = "disenchant"; break;
#endif

#ifdef JP
case RBE_UN_POWER:	q = "魔力を吸い取る"; break;
#else
			case RBE_UN_POWER:	q = "drain charges"; break;
#endif

#ifdef JP
case RBE_EAT_GOLD:	q = "金を盗む"; break;
#else
			case RBE_EAT_GOLD:	q = "steal gold"; break;
#endif

#ifdef JP
case RBE_EAT_ITEM:	q = "アイテムを盗む"; break;
#else
			case RBE_EAT_ITEM:	q = "steal items"; break;
#endif

#ifdef JP
case RBE_EAT_FOOD:	q = "あなたの食料を食べる"; break;
#else
			case RBE_EAT_FOOD:	q = "eat your food"; break;
#endif

#ifdef JP
case RBE_EAT_LITE:	q = "明かりを吸収する"; break;
#else
			case RBE_EAT_LITE:	q = "absorb light"; break;
#endif

#ifdef JP
case RBE_ACID:    	q = "酸を飛ばす"; break;
#else
			case RBE_ACID:    	q = "shoot acid"; break;
#endif

#ifdef JP
case RBE_ELEC:    	q = "感電させる"; break;
#else
			case RBE_ELEC:    	q = "electrocute"; break;
#endif

#ifdef JP
case RBE_FIRE:    	q = "燃やす"; break;
#else
			case RBE_FIRE:    	q = "burn"; break;
#endif

#ifdef JP
case RBE_COLD:    	q = "凍らせる"; break;
#else
			case RBE_COLD:    	q = "freeze"; break;
#endif

#ifdef JP
case RBE_BLIND:   	q = "盲目にする"; break;
#else
			case RBE_BLIND:   	q = "blind"; break;
#endif

#ifdef JP
case RBE_CONFUSE: 	q = "混乱させる"; break;
#else
			case RBE_CONFUSE: 	q = "confuse"; break;
#endif

#ifdef JP
case RBE_TERRIFY: 	q = "恐怖させる"; break;
#else
			case RBE_TERRIFY: 	q = "terrify"; break;
#endif

#ifdef JP
case RBE_PARALYZE:	q = "麻痺させる"; break;
#else
			case RBE_PARALYZE:	q = "paralyze"; break;
#endif

#ifdef JP
case RBE_LOSE_STR:	q = "腕力を減少させる"; break;
#else
			case RBE_LOSE_STR:	q = "reduce strength"; break;
#endif

#ifdef JP
case RBE_LOSE_INT:	q = "知能を減少させる"; break;
#else
			case RBE_LOSE_INT:	q = "reduce intelligence"; break;
#endif

#ifdef JP
case RBE_LOSE_WIS:	q = "賢さを減少させる"; break;
#else
			case RBE_LOSE_WIS:	q = "reduce wisdom"; break;
#endif

#ifdef JP
case RBE_LOSE_DEX:	q = "器用さを減少させる"; break;
#else
			case RBE_LOSE_DEX:	q = "reduce dexterity"; break;
#endif

#ifdef JP
case RBE_LOSE_CON:	q = "耐久力を減少させる"; break;
#else
			case RBE_LOSE_CON:	q = "reduce constitution"; break;
#endif

#ifdef JP
case RBE_LOSE_CHR:	q = "魅力を減少させる"; break;
#else
			case RBE_LOSE_CHR:	q = "reduce charisma"; break;
#endif

#ifdef JP
case RBE_LOSE_ALL:	q = "全ステータスを減少させる"; break;
#else
			case RBE_LOSE_ALL:	q = "reduce all stats"; break;
#endif

#ifdef JP
case RBE_SHATTER:	q = "粉砕する"; break;
#else
			case RBE_SHATTER:	q = "shatter"; break;
#endif

#ifdef JP
case RBE_EXP_10:	q = "経験値を減少(10d6+)させる"; break;
#else
			case RBE_EXP_10:	q = "lower experience (by 10d6+)"; break;
#endif

#ifdef JP
case RBE_EXP_20:	q = "経験値を減少(20d6+)させる"; break;
#else
			case RBE_EXP_20:	q = "lower experience (by 20d6+)"; break;
#endif

#ifdef JP
case RBE_EXP_40:	q = "経験値を減少(40d6+)させる"; break;
#else
			case RBE_EXP_40:	q = "lower experience (by 40d6+)"; break;
#endif

#ifdef JP
case RBE_EXP_80:	q = "経験値を減少(80d6+)させる"; break;
#else
			case RBE_EXP_80:	q = "lower experience (by 80d6+)"; break;
#endif

#ifdef JP
case RBE_DISEASE:	q = "病気にする"; break;
#else
			case RBE_DISEASE:	q = "disease"; break;
#endif

#ifdef JP
case RBE_TIME:      q = "時間を逆戻りさせる"; break;
#else
			case RBE_TIME:      q = "time"; break;
#endif

#ifdef JP
case RBE_EXP_VAMP:  q = "生命力を吸収する"; break;
#else
			case RBE_EXP_VAMP:  q = "drain life force"; break;
#endif

#ifdef JP
case RBE_DR_MANA:  q = "魔力を奪う"; break;
#else
			case RBE_DR_MANA:  q = "drain mana force"; break;
#endif

		}


#ifdef JP
                if ( r == 0 ) roff( format("%^sは", wd_he[msex]) );

		/***若干表現を変更 ita ***/

                        /* Describe damage (if known) */
		if (d1 && d2 && know_damage(r_idx, m))
		  {
		    
		    /* Display the damage */
		    roff(format(" %dd%d ", d1, d2));
		    roff("のダメージで");
		  }
		/* Hack -- force a method */
		if (!p) p = "何か奇妙なことをする";

		/* Describe the method */
		/* XXしてYYし/XXしてYYする/XXし/XXする */
		if(q) jverb( p ,jverb_buf, JVERB_TO);
		else if(r!=n-1) jverb( p ,jverb_buf, JVERB_AND);
		else strcpy(jverb_buf, p);

		roff(jverb_buf);

		/* Describe the effect (if any) */
		if (q)
		{
		  if(r!=n-1) jverb( q,jverb_buf, JVERB_AND);
		  else strcpy(jverb_buf,q); 
		  roff(jverb_buf);
		}
		if(r!=n-1) roff("、");
#else
		/* Introduce the attack description */
		if (!r)
		{
			roff(format("%^s can ", wd_he[msex]));
		}
		else if (r < n-1)
		{
			roff(", ");
		}
		else
		{
			roff(", and ");
		}


		/* Hack -- force a method */
		if (!p) p = "do something weird";

		/* Describe the method */
		roff(p);


		/* Describe the effect (if any) */
		if (q)
		{
			/* Describe the attack type */
			roff(" to ");
			roff(q);

			/* Describe damage (if known) */
			if (d1 && d2 && know_damage(r_idx, m))
			{
				/* Display the damage */
				roff(" with damage");
				roff(format(" %dd%d", d1, d2));
			}
		}
#endif



		/* Count the attacks as printed */
		r++;
	}

	/* Finish sentence above */
	if (r)
	{
#ifdef JP
		roff("。");
#else
		roff(".  ");
#endif

	}

	/* Notice lack of attacks */
	else if (flags1 & RF1_NEVER_BLOW)
	{
#ifdef JP
		roff(format("%^sは物理的な攻撃方法を持たない。", wd_he[msex]));
#else
		roff(format("%^s has no physical attacks.  ", wd_he[msex]));
#endif

	}

	/* Or describe the lack of knowledge */
	else
	{
#ifdef JP
		roff(format("%s攻撃については何も知らない。", wd_his[msex]));
#else
		roff(format("Nothing is known about %s attack.  ", wd_his[msex]));
#endif

	}


	/*
	 * Notice "Quest" monsters, but only if you
	 * already encountered the monster.
	 */
	if ((flags1 & RF1_QUESTOR) && ((r_ptr->r_sights) && (r_ptr->max_num) && ((r_idx == MON_OBERON) || (r_idx == MON_SERPENT))))
	{
#ifdef JP
		c_roff(TERM_VIOLET, "あなたはこのモンスターを殺したいという強い欲望を感じている...");
#else
		c_roff(TERM_VIOLET, "You feel an intense desire to kill this monster...  ");
#endif

	}

	else if (flags7 & RF7_GUARDIAN)
	{
#ifdef JP
		c_roff(TERM_L_RED, "このモンスターはダンジョンの主である。");
#else
		c_roff(TERM_L_RED, "This monster is the master of a dungeon.");
#endif

	}


	/* All done */
	roff("\n");

	/* Cheat -- know everything */
	if ((cheat_know) && (remem == 0))
	{
		/* Hack -- restore memory */
		COPY(r_ptr, &save_mem, monster_race);
	}
}



/*
 * Hack -- Display the "name" and "attr/chars" of a monster race
 */
void roff_top(int r_idx)
{
	monster_race	*r_ptr = &r_info[r_idx];

	byte		a1, a2;
	char		c1, c2;


	/* Access the chars */
	c1 = r_ptr->d_char;
	c2 = r_ptr->x_char;

	/* Access the attrs */
	a1 = r_ptr->d_attr;
	a2 = r_ptr->x_attr;


	/* Clear the top line */
	Term_erase(0, 0, 255);

	/* Reset the cursor */
	Term_gotoxy(0, 0);

	/* A title (use "The" for non-uniques) */
#ifdef JP
        if (0)
#else
	if (!(r_ptr->flags1 & RF1_UNIQUE))
#endif

	{
		Term_addstr(-1, TERM_WHITE, "The ");
	}

	/* Dump the name */
	Term_addstr(-1, TERM_WHITE, (r_name + r_ptr->name));

	/* Append the "standard" attr/char info */
	Term_addstr(-1, TERM_WHITE, " ('");
	Term_addch(a1, c1);
	if (use_bigtile && (a1 & 0x80)) Term_addch(255, 255);
	Term_addstr(-1, TERM_WHITE, "')");

	/* Append the "optional" attr/char info */
	Term_addstr(-1, TERM_WHITE, "/('");
	Term_addch(a2, c2);
	if (use_bigtile && (a2 & 0x80)) Term_addch(255, 255);
	Term_addstr(-1, TERM_WHITE, "'):");

	/* Wizards get extra info */
	if (wizard)
	{
		char buf[6];

		sprintf(buf, "%d", r_idx);

		Term_addstr(-1, TERM_WHITE, " (");
		Term_addstr(-1, TERM_L_BLUE, buf);
		Term_addch(TERM_WHITE, ')');
	}
}



/*
 * Hack -- describe the given monster race at the top of the screen
 */
void screen_roff(int r_idx, int remember)
{
	/* Flush messages */
	msg_print(NULL);

	/* Begin recall */
	Term_erase(0, 1, 255);

	/* Recall monster */
	roff_aux(r_idx, remember);

	/* Describe monster */
	roff_top(r_idx);
}




/*
 * Hack -- describe the given monster race in the current "term" window
 */
void display_roff(int r_idx)
{
	int y;

	/* Erase the window */
	for (y = 0; y < Term->hgt; y++)
	{
		/* Erase the line */
		Term_erase(0, y, 255);
	}

	/* Begin recall */
	Term_gotoxy(0, 1);

	/* Recall monster */
	roff_aux(r_idx, 0);

	/* Describe monster */
	roff_top(r_idx);
}


bool monster_quest(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	/* Random quests are in the dungeon */
	if (r_ptr->flags8 & RF8_WILD_ONLY) return FALSE;

	/* No random quests for aquatic monsters */
	if (r_ptr->flags7 & RF7_AQUATIC) return FALSE;

	/* No random quests for multiplying monsters */
	if (r_ptr->flags2 & RF2_MULTIPLY) return FALSE;

	/* No quests to kill friendly monsters */
	if (r_ptr->flags7 & RF7_FRIENDLY) return FALSE;

	return TRUE;
}


bool monster_dungeon(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (!(r_ptr->flags8 & RF8_WILD_ONLY))
		return TRUE;
	else
		return FALSE;
}


bool monster_ocean(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & RF8_WILD_OCEAN)
		return TRUE;
	else
		return FALSE;
}


bool monster_shore(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & RF8_WILD_SHORE)
		return TRUE;
	else
		return FALSE;
}


bool monster_waste(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & (RF8_WILD_WASTE | RF8_WILD_ALL))
		return TRUE;
	else
		return FALSE;
}


bool monster_town(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & (RF8_WILD_TOWN | RF8_WILD_ALL))
		return TRUE;
	else
		return FALSE;
}


bool monster_wood(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & (RF8_WILD_WOOD | RF8_WILD_ALL))
		return TRUE;
	else
		return FALSE;
}


bool monster_volcano(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & RF8_WILD_VOLCANO)
		return TRUE;
	else
		return FALSE;
}


bool monster_mountain(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & RF8_WILD_MOUNTAIN)
		return TRUE;
	else
		return FALSE;
}


bool monster_grass(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (r_ptr->flags8 & (RF8_WILD_GRASS | RF8_WILD_ALL))
		return TRUE;
	else
		return FALSE;
}


bool monster_deep_water(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (!monster_dungeon(r_idx)) return FALSE;

	if (r_ptr->flags7 & RF7_AQUATIC)
		return TRUE;
	else
		return FALSE;
}


bool monster_shallow_water(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (!monster_dungeon(r_idx)) return FALSE;

	if (r_ptr->flags2 & RF2_AURA_FIRE)
		return FALSE;
	else
		return TRUE;
}


bool monster_lava(int r_idx)
{
	monster_race *r_ptr = &r_info[r_idx];

	if (!monster_dungeon(r_idx)) return FALSE;

	if (((r_ptr->flags3 & RF3_IM_FIRE) ||
	     (r_ptr->flags7 & RF7_CAN_FLY)) &&
	    !(r_ptr->flags3 & RF3_AURA_COLD))
		return TRUE;
	else
		return FALSE;
}


monster_hook_type get_monster_hook(void)
{
	if (!dun_level && !p_ptr->inside_quest)
	{
		switch (wilderness[p_ptr->wilderness_y][p_ptr->wilderness_x].terrain)
		{
		case TERRAIN_TOWN:
			return (monster_hook_type)monster_town;
		case TERRAIN_DEEP_WATER:
			return (monster_hook_type)monster_ocean;
		case TERRAIN_SHALLOW_WATER:
		case TERRAIN_SWAMP:
			return (monster_hook_type)monster_shore;
		case TERRAIN_DIRT:
		case TERRAIN_DESERT:
			return (monster_hook_type)monster_waste;
		case TERRAIN_GRASS:
			return (monster_hook_type)monster_grass;
		case TERRAIN_TREES:
			return (monster_hook_type)monster_wood;
		case TERRAIN_SHALLOW_LAVA:
		case TERRAIN_DEEP_LAVA:
			return (monster_hook_type)monster_volcano;
		case TERRAIN_MOUNTAIN:
			return (monster_hook_type)monster_mountain;
		default:
			return (monster_hook_type)monster_dungeon;
		}
	}
	else
	{
		return (monster_hook_type)monster_dungeon;
	}
}


monster_hook_type get_monster_hook2(int y, int x)
{
	/* Set the monster list */
	switch (cave[y][x].feat)
	{
	case FEAT_SHAL_WATER:
		return (monster_hook_type)monster_shallow_water;
	case FEAT_DEEP_WATER:
		return (monster_hook_type)monster_deep_water;
	case FEAT_DEEP_LAVA:
	case FEAT_SHAL_LAVA:
		return (monster_hook_type)monster_lava;
	default:
		return NULL;
	}
}


void set_friendly(monster_type *m_ptr)
{
	m_ptr->smart |= SM_FRIENDLY;
}

void set_pet(monster_type *m_ptr)
{
	/* Check for quest completion */
	check_quest_completion(m_ptr);

	m_ptr->smart |= SM_PET;
}

/*
 * Makes the monster hostile towards the player
 */
void set_hostile(monster_type *m_ptr)
{
	if (p_ptr->inside_battle) return;
	m_ptr->smart &= ~SM_PET;
	m_ptr->smart &= ~SM_FRIENDLY;
}


/*
 * Anger the monster
 */
void anger_monster(monster_type *m_ptr)
{
	if (p_ptr->inside_battle) return;
	if (is_friendly(m_ptr))
	{
		char m_name[80];

		monster_desc(m_name, m_ptr, 0);
#ifdef JP
msg_format("%^sは怒った！", m_name);
#else
		msg_format("%^s gets angry!", m_name);
#endif

		set_hostile(m_ptr);

		chg_virtue(V_INDIVIDUALISM, 1);
		chg_virtue(V_HONOUR, -1);
		chg_virtue(V_JUSTICE, -1);
		chg_virtue(V_COMPASSION, -1);
	}
}


/*
 * Check if monster can cross terrain
 */
bool monster_can_cross_terrain(byte feat, monster_race *r_ptr)
{
	/* Deep water */
	if (feat == FEAT_DEEP_WATER)
	{
		if ((r_ptr->flags7 & RF7_AQUATIC) ||
		    (r_ptr->flags7 & RF7_CAN_FLY) ||
		    (r_ptr->flags7 & RF7_CAN_SWIM))
			return TRUE;
		else
			return FALSE;
	}
	/* Shallow water */
	else if (feat == FEAT_SHAL_WATER)
	{
		if (!(r_ptr->flags2 & RF2_AURA_FIRE) ||
		    (r_ptr->flags7 & RF7_AQUATIC) ||
		    (r_ptr->flags7 & RF7_CAN_FLY) ||
		    (r_ptr->flags7 & RF7_CAN_SWIM))
			return TRUE;
		else
			return FALSE;
	}
	/* Aquatic monster */
	else if ((r_ptr->flags7 & RF7_AQUATIC) &&
		    !(r_ptr->flags7 & RF7_CAN_FLY))
	{
		return FALSE;
	}
	/* Lava */
	else if ((feat == FEAT_SHAL_LAVA) ||
	    (feat == FEAT_DEEP_LAVA))
	{
		if ((r_ptr->flags3 & RF3_IM_FIRE) ||
		    (r_ptr->flags7 & RF7_CAN_FLY))
			return TRUE;
		else
			return FALSE;
	}

	return TRUE;
}


/*
 * Check if two monsters are enemies
 */
bool are_enemies(monster_type *m_ptr, monster_type *n_ptr)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	monster_race *s_ptr = &r_info[n_ptr->r_idx];

	if (p_ptr->inside_battle)
	{
		if (is_pet(m_ptr) || is_pet(n_ptr)) return FALSE;
		return TRUE;
	}

	if ((r_ptr->flags8 & (RF8_WILD_TOWN | RF8_WILD_ALL))
	    && (s_ptr->flags8 & (RF8_WILD_TOWN | RF8_WILD_ALL)))
	{
		if (!is_pet(m_ptr) && !is_pet(n_ptr)) return FALSE;
	}

	/* Friendly vs. opposite aligned normal or pet */
	if (((r_ptr->flags3 & RF3_EVIL) &&
		  (s_ptr->flags3 & RF3_GOOD)) ||
		 ((r_ptr->flags3 & RF3_GOOD) &&
		  (s_ptr->flags3 & RF3_EVIL)))
	{
		if (!(m_ptr->mflag2 & MFLAG_CHAMELEON) || !(n_ptr->mflag2 & MFLAG_CHAMELEON)) return TRUE;
	}

	/* Hostile vs. non-hostile */
	if (is_hostile(m_ptr) != is_hostile(n_ptr))
	{
		return TRUE;
	}

	/* Default */
	return FALSE;
}


/*
 * Is the monster "alive"?
 *
 * Used to determine the message to print for a killed monster.
 * ("dies", "destroyed")
 */
bool monster_living(monster_race *r_ptr)
{
	/* Non-living, undead, or demon */
	if (r_ptr->flags3 & (RF3_DEMON | RF3_UNDEAD | RF3_NONLIVING))
		return FALSE;
	else
		return TRUE;
}



/* Dwarves */
static char *dwarf_syllable1[] =
{
	"B", "D", "F", "G", "Gl", "H", "K", "L", "M", "N", "R", "S", "T", "Th", "V",
};

static char *dwarf_syllable2[] =
{
	"a", "e", "i", "o", "oi", "u",
};

static char *dwarf_syllable3[] =
{
	"bur", "fur", "gan", "gnus", "gnar", "li", "lin", "lir", "mli", "nar",
	"nus", "rin", "ran", "sin", "sil", "sur",
};

/* Elves */
static char *elf_syllable1[] =
{
	"Al", "An", "Bal", "Bel", "Cal", "Cel", "El", "Elr", "Elv", "Eow", "F",
	"Fal", "Fel", "Fin", "G", "Gal", "Gel", "Gl", "Is", "Lan", "Leg", "N",
	"Nal", "Nel",  "S", "Sal", "Sel", "T", "Tal", "Tel", "Thr", "Tin",
};

static char *elf_syllable2[] =
{
	"a", "adrie", "ara", "e", "ebri", "ele", "ere", "i", "io", "ithra",
	"ilma", "il-Ga", "ili", "o", "orfi", "u", "y",
};

static char *elf_syllable3[] =
{
	"l", "las", "lad", "ldor", "ldur", "linde", "lith", "mir", "n", "nd",
	"ndel", "ndil", "ndir", "nduil", "ng", "mbor", "r", "rith", "ril",
	"riand", "rion", "s", "thien", "viel", "wen", "wyn",
};

/* Gnomes */
static char *gnome_syllable1[] =
{
	"Aar", "An", "Ar", "As", "C", "H", "Han", "Har", "Hel", "Iir", "J",
	"Jan", "Jar", "K", "L", "M", "Mar", "N", "Nik", "Os", "Ol", "P", "R",
	"S", "Sam", "San", "T", "Ter", "Tom", "Ul", "V", "W", "Y",
};

static char *gnome_syllable2[] =
{
	"a", "aa",  "ai", "e", "ei", "i", "o", "uo", "u", "uu",
};

static char *gnome_syllable3[] =
{
	"ron", "re", "la", "ki", "kseli", "ksi", "ku", "ja", "ta", "na",
	"namari", "neli", "nika", "nikki", "nu", "nukka", "ka", "ko", "li",
	"kki", "rik", "po", "to", "pekka", "rjaana", "rjatta", "rjukka", "la",
	"lla", "lli", "mo", "nni",
};

/* Hobbit */
static char *hobbit_syllable1[] =
{
	"B", "Ber", "Br", "D", "Der", "Dr", "F", "Fr", "G", "H", "L", "Ler",
	"M", "Mer", "N", "P", "Pr", "Per", "R", "S", "T", "W",
};

static char *hobbit_syllable2[] =
{
	"a", "e", "i", "ia", "o", "oi", "u",
};

static char *hobbit_syllable3[] =
{
	"bo", "ck", "decan", "degar", "do", "doc", "go", "grin", "lba", "lbo",
	"lda", "ldo", "lla", "ll", "lo", "m", "mwise", "nac", "noc", "nwise",
	"p", "ppin", "pper", "tho", "to",
};

/* Human */
static char *human_syllable1[] =
{
	"Ab", "Ac", "Ad", "Af", "Agr", "Ast", "As", "Al", "Adw", "Adr", "Ar",
	"B", "Br", "C", "Cr", "Ch", "Cad", "D", "Dr", "Dw", "Ed", "Eth", "Et",
	"Er", "El", "Eow", "F", "Fr", "G", "Gr", "Gw", "Gal", "Gl", "H", "Ha",
	"Ib", "Jer", "K", "Ka", "Ked", "L", "Loth", "Lar", "Leg", "M", "Mir",
	"N", "Nyd", "Ol", "Oc", "On", "P", "Pr", "R", "Rh", "S", "Sev", "T",
	"Tr", "Th", "V", "Y", "Z", "W", "Wic",
};

static char *human_syllable2[] =
{
	"a", "ae", "au", "ao", "are", "ale", "ali", "ay", "ardo", "e", "ei",
	"ea", "eri", "era", "ela", "eli", "enda", "erra", "i", "ia", "ie",
	"ire", "ira", "ila", "ili", "ira", "igo", "o", "oa", "oi", "oe",
	"ore", "u", "y",
};

static char *human_syllable3[] =
{
	"a", "and", "b", "bwyn", "baen", "bard", "c", "ctred", "cred", "ch",
	"can", "d", "dan", "don", "der", "dric", "dfrid", "dus", "f", "g",
	"gord", "gan", "l", "li", "lgrin", "lin", "lith", "lath", "loth", "ld",
	"ldric", "ldan", "m", "mas", "mos", "mar", "mond", "n", "nydd", "nidd",
	"nnon", "nwan", "nyth", "nad", "nn", "nnor", "nd", "p", "r", "ron",
	"rd", "s", "sh", "seth", "sean", "t", "th", "tha", "tlan", "trem",
	"tram", "v", "vudd", "w", "wan", "win", "wyn", "wyr", "wyr", "wyth",
};

/* Orc */
static char *orc_syllable1[] =
{
	"B", "Er", "G", "Gr", "H", "P", "Pr", "R", "V", "Vr", "T", "Tr", "M", "Dr",
};

static char *orc_syllable2[] =
{
	"a", "i", "o", "oo", "u", "ui",
};

static char *orc_syllable3[] =
{
	"dash", "dish", "dush", "gar", "gor", "gdush", "lo", "gdish", "k",
	"lg", "nak", "rag", "rbag", "rg", "rk", "ng", "nk", "rt", "ol", "urk",
	"shnak", "mog", "mak", "rak",
};


/*
 * Random Name Generator
 * based on a Javascript by Michael Hensley
 * "http://geocities.com/timessquare/castle/6274/"
 */
void create_name(int type, char *name)
{
	/* Paranoia */
	if (!name) return;

	/* Select the monster type */
	switch (type)
	{
		/* Create the monster name */
		case NAME_DWARF:
			strcpy(name, dwarf_syllable1[randint0(sizeof(dwarf_syllable1) / sizeof(char*))]);
			strcat(name, dwarf_syllable2[randint0(sizeof(dwarf_syllable2) / sizeof(char*))]);
			strcat(name, dwarf_syllable3[randint0(sizeof(dwarf_syllable3) / sizeof(char*))]);
			break;
		case NAME_ELF:
			strcpy(name, elf_syllable1[randint0(sizeof(elf_syllable1) / sizeof(char*))]);
			strcat(name, elf_syllable2[randint0(sizeof(elf_syllable2) / sizeof(char*))]);
			strcat(name, elf_syllable3[randint0(sizeof(elf_syllable3) / sizeof(char*))]);
			break;
		case NAME_GNOME:
			strcpy(name, gnome_syllable1[randint0(sizeof(gnome_syllable1) / sizeof(char*))]);
			strcat(name, gnome_syllable2[randint0(sizeof(gnome_syllable2) / sizeof(char*))]);
			strcat(name, gnome_syllable3[randint0(sizeof(gnome_syllable3) / sizeof(char*))]);
			break;
		case NAME_HOBBIT:
			strcpy(name, hobbit_syllable1[randint0(sizeof(hobbit_syllable1) / sizeof(char*))]);
			strcat(name, hobbit_syllable2[randint0(sizeof(hobbit_syllable2) / sizeof(char*))]);
			strcat(name, hobbit_syllable3[randint0(sizeof(hobbit_syllable3) / sizeof(char*))]);
			break;
		case NAME_HUMAN:
			strcpy(name, human_syllable1[randint0(sizeof(human_syllable1) / sizeof(char*))]);
			strcat(name, human_syllable2[randint0(sizeof(human_syllable2) / sizeof(char*))]);
			strcat(name, human_syllable3[randint0(sizeof(human_syllable3) / sizeof(char*))]);
			break;
		case NAME_ORC:
			strcpy(name, orc_syllable1[randint0(sizeof(orc_syllable1) / sizeof(char*))]);
			strcat(name, orc_syllable2[randint0(sizeof(orc_syllable2) / sizeof(char*))]);
			strcat(name, orc_syllable3[randint0(sizeof(orc_syllable3) / sizeof(char*))]);
			break;
		/* Create an empty name */
		default:
			name[0] = '\0';
			break;
	}
}
