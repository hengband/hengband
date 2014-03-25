/*!
 * @file melee1.c
 * @brief モンスターの打撃処理 / Monster attacks
 * @date 2014/01/17
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen.\n
 * @details
 */

#include "angband.h"


/*!
 * @brief モンスター打撃のクリティカルランクを返す /
 * Critical blow. All hits that do 95% of total possible damage,
 * @param dice モンスター打撃のダイス数
 * @param sides モンスター打撃の最大ダイス目
 * @param dam プレイヤーに与えたダメージ
 * @details
 * and which also do at least 20 damage, or, sometimes, N damage.
 * This is used only to determine "cuts" and "stuns".
 */
static int monster_critical(int dice, int sides, int dam)
{
	int max = 0;
	int total = dice * sides;

	/* Must do at least 95% of perfect */
	if (dam < total * 19 / 20) return (0);

	/* Weak blows rarely work */
	if ((dam < 20) && (randint0(100) >= dam)) return (0);

	/* Perfect damage */
	if ((dam >= total) && (dam >= 40)) max++;

	/* Super-charge */
	if (dam >= 20)
	{
		while (randint0(100) < 2) max++;
	}

	/* Critical damage */
	if (dam > 45) return (6 + max);
	if (dam > 33) return (5 + max);
	if (dam > 25) return (4 + max);
	if (dam > 18) return (3 + max);
	if (dam > 11) return (2 + max);
	return (1 + max);
}

/*!
 * @brief モンスター打撃の命中を判定する /
 * Determine if a monster attack against the player succeeds.
 * @param power 打撃属性毎の基本命中値
 * @param level モンスターのレベル
 * @param stun モンスターの朦朧値
 * @return TRUEならば命中判定
 * @details
 * Always miss 5% of the time, Always hit 5% of the time.
 * Otherwise, match monster power against player armor.
 */
static int check_hit(int power, int level, int stun)
{
	int i, k, ac;

	/* Percentile dice */
	k = randint0(100);

	if (stun && one_in_(2)) return FALSE;

	/* Hack -- Always miss or hit */
	if (k < 10) return (k < 5);

	/* Calculate the "attack quality" */
	i = (power + (level * 3));

	/* Total armor */
	ac = p_ptr->ac + p_ptr->to_a;
	if (p_ptr->special_attack & ATTACK_SUIKEN) ac += (p_ptr->lev * 2);

	/* Power and Level compete against Armor */
	if ((i > 0) && (randint1(i) > ((ac * 3) / 4))) return (TRUE);

	/* Assume miss */
	return (FALSE);
}



/*! モンスターの侮辱行為メッセージテーブル / Hack -- possible "insult" messages */
static cptr desc_insult[] =
{
#ifdef JP
	"があなたを侮辱した！",
	"があなたの母を侮辱した！",
	"があなたを軽蔑した！",
	"があなたを辱めた！",
	"があなたを汚した！",
	"があなたの回りで踊った！",
	"が猥褻な身ぶりをした！",
	"があなたをぼんやりと見た！！！",
	"があなたをパラサイト呼ばわりした！",
	"があなたをサイボーグ扱いした！"
#else
	"insults you!",
	"insults your mother!",
	"gives you the finger!",
	"humiliates you!",
	"defiles you!",
	"dances around you!",
	"makes obscene gestures!",
	"moons you!!!"
	"calls you a parasite!",
	"calls you a cyborg!"
#endif

};


/*! マゴットのぼやきメッセージテーブル / Hack -- possible "insult" messages */
static cptr desc_moan[] =
{
#ifdef JP
	"は何かを悲しんでいるようだ。",
	"が彼の飼い犬を見なかったかと尋ねている。",
	"が縄張りから出て行けと言っている。",
	"はキノコがどうとか呟いている。"
#else
	"seems sad about something.",
	"asks if you have seen his dogs.",
	"tells you to get off his land.",
	"mumbles something about mushrooms."
#endif

};


/*!
 * @brief モンスターからプレイヤーへの打撃処理 / Attack the player via physical attacks.
 * @param m_idx 打撃を行うモンスターのID
 * @return 実際に攻撃処理を行った場合TRUEを返す
 */
bool make_attack_normal(int m_idx)
{
	monster_type *m_ptr = &m_list[m_idx];

	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	int ap_cnt;

	int i, k, tmp, ac, rlev;
	int do_cut, do_stun;

	s32b gold;

	object_type *o_ptr;

	char o_name[MAX_NLEN];

	char m_name[80];

	char ddesc[80];

	bool blinked;
	bool touched = FALSE, fear = FALSE, alive = TRUE;
	bool explode = FALSE;
	bool do_silly_attack = (one_in_(2) && p_ptr->image);
	int get_damage = 0;
	int abbreviate = 0;

	/* Not allowed to attack */
	if (r_ptr->flags1 & (RF1_NEVER_BLOW)) return (FALSE);

	if (d_info[dungeon_type].flags1 & DF1_NO_MELEE) return (FALSE);

	/* ...nor if friendly */
	if (!is_hostile(m_ptr)) return FALSE;

	/* Extract the effective monster level */
	rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);


	/* Get the monster name (or "it") */
	monster_desc(m_name, m_ptr, 0);

	/* Get the "died from" information (i.e. "a kobold") */
	monster_desc(ddesc, m_ptr, MD_IGNORE_HALLU | MD_ASSUME_VISIBLE | MD_INDEF_VISIBLE);

	if (p_ptr->special_defense & KATA_IAI)
	{
		msg_format(_("相手が襲いかかる前に素早く武器を振るった。", "You took sen, draw and cut in one motion before %s move."), m_name);
		if (py_attack(m_ptr->fy, m_ptr->fx, HISSATSU_IAI)) return TRUE;
	}

	if ((p_ptr->special_defense & NINJA_KAWARIMI) && (randint0(55) < (p_ptr->lev*3/5+20)))
	{
		if (kawarimi(TRUE)) return TRUE;
	}

	/* Assume no blink */
	blinked = FALSE;

	/* Scan through all four blows */
	for (ap_cnt = 0; ap_cnt < 4; ap_cnt++)
	{
		bool obvious = FALSE;

		int power = 0;
		int damage = 0;

		cptr act = NULL;

		/* Extract the attack infomation */
		int effect = r_ptr->blow[ap_cnt].effect;
		int method = r_ptr->blow[ap_cnt].method;
		int d_dice = r_ptr->blow[ap_cnt].d_dice;
		int d_side = r_ptr->blow[ap_cnt].d_side;


		if (!m_ptr->r_idx) break;

		/* Hack -- no more attacks */
		if (!method) break;

		if (is_pet(m_ptr) && (r_ptr->flags1 & RF1_UNIQUE) && (method == RBM_EXPLODE))
		{
			method = RBM_HIT;
			d_dice /= 10;
		}

		/* Stop if player is dead or gone */
		if (!p_ptr->playing || p_ptr->is_dead) break;
		if (distance(py, px, m_ptr->fy, m_ptr->fx) > 1) break;

		/* Handle "leaving" */
		if (p_ptr->leaving) break;

		if (method == RBM_SHOOT) continue;

		/* Extract the attack "power" */
		power = mbe_info[effect].power;

		/* Total armor */
		ac = p_ptr->ac + p_ptr->to_a;

		/* Monster hits player */
		if (!effect || check_hit(power, rlev, MON_STUNNED(m_ptr)))
		{
			/* Always disturbing */
			disturb(1, 1);


			/* Hack -- Apply "protection from evil" */
			if ((p_ptr->protevil > 0) &&
			    (r_ptr->flags3 & RF3_EVIL) &&
			    (p_ptr->lev >= rlev) &&
			    ((randint0(100) + p_ptr->lev) > 50))
			{
				/* Remember the Evil-ness */
				if (is_original_ap_and_seen(m_ptr)) r_ptr->r_flags3 |= RF3_EVIL;

				/* Message */
#ifdef JP
				if (abbreviate)
				    msg_format("撃退した。");
				else
				    msg_format("%^sは撃退された。", m_name);
				abbreviate = 1;/*２回目以降は省略 */
#else
				msg_format("%^s is repelled.", m_name);
#endif


				/* Hack -- Next attack */
				continue;
			}


			/* Assume no cut or stun */
			do_cut = do_stun = 0;

			/* Describe the attack method */
			switch (method)
			{
				case RBM_HIT:
				{
					act = _("殴られた。", "hits you.");
					do_cut = do_stun = 1;
					touched = TRUE;
					sound(SOUND_HIT);
					break;
				}

				case RBM_TOUCH:
				{
					act = _("触られた。", "touches you.");
					touched = TRUE;
					sound(SOUND_TOUCH);
					break;
				}

				case RBM_PUNCH:
				{
					act = _("パンチされた。", "punches you.");
					touched = TRUE;
					do_stun = 1;
					sound(SOUND_HIT);
					break;
				}

				case RBM_KICK:
				{
					act = _("蹴られた。", "kicks you.");
					touched = TRUE;
					do_stun = 1;
					sound(SOUND_HIT);
					break;
				}

				case RBM_CLAW:
				{
					act = _("ひっかかれた。", "claws you.");
					touched = TRUE;
					do_cut = 1;
					sound(SOUND_CLAW);
					break;
				}

				case RBM_BITE:
				{
					act = _("噛まれた。", "bites you.");
					do_cut = 1;
					touched = TRUE;
					sound(SOUND_BITE);
					break;
				}

				case RBM_STING:
				{
					act = _("刺された。", "stings you.");
					touched = TRUE;
					sound(SOUND_STING);
					break;
				}

				case RBM_SLASH:
				{
					act = _("斬られた。", "slashes you.");
					touched = TRUE;
					do_cut = 1;
					sound(SOUND_CLAW);
					break;
				}

				case RBM_BUTT:
				{
					act = _("角で突かれた。", "butts you.");
					do_stun = 1;
					touched = TRUE;
					sound(SOUND_HIT);
					break;
				}

				case RBM_CRUSH:
				{
					act = _("体当たりされた。", "crushes you.");
					do_stun = 1;
					touched = TRUE;
					sound(SOUND_CRUSH);
					break;
				}

				case RBM_ENGULF:
				{
					act = _("飲み込まれた。", "engulfs you.");
					touched = TRUE;
					sound(SOUND_CRUSH);
					break;
				}

				case RBM_CHARGE:
				{
					abbreviate = -1;
					act = _("は請求書をよこした。", "charges you.");
					touched = TRUE;
					sound(SOUND_BUY); /* Note! This is "charges", not "charges at". */
					break;
				}

				case RBM_CRAWL:
				{
					abbreviate = -1;
					act = _("が体の上を這い回った。", "crawls on you.");
					touched = TRUE;
					sound(SOUND_SLIME);
					break;
				}

				case RBM_DROOL:
				{
					act = _("よだれをたらされた。", "drools on you.");
					sound(SOUND_SLIME);
					break;
				}

				case RBM_SPIT:
				{
					act = _("唾を吐かれた。", "spits on you.");
					sound(SOUND_SLIME);
					break;
				}

				case RBM_EXPLODE:
				{
					abbreviate = -1;
					act = _("は爆発した。", "explodes.");
					explode = TRUE;
					break;
				}

				case RBM_GAZE:
				{
					act = _("にらまれた。", "gazes at you.");
					break;
				}

				case RBM_WAIL:
				{
					act = _("泣き叫ばれた。", "wails at you.");
					sound(SOUND_WAIL);
					break;
				}

				case RBM_SPORE:
				{
					act = _("胞子を飛ばされた。", "releases spores at you.");
					sound(SOUND_SLIME);
					break;
				}

				case RBM_XXX4:
				{
					abbreviate = -1;
					act = _("が XXX4 を発射した。", "projects XXX4's at you.");
					break;
				}

				case RBM_BEG:
				{
					act = _("金をせがまれた。", "begs you for money.");
					sound(SOUND_MOAN);
					break;
				}

				case RBM_INSULT:
				{
#ifdef JP
					abbreviate = -1;
#endif
					act = desc_insult[randint0(m_ptr->r_idx == MON_DEBBY ? 10 : 8)];
					sound(SOUND_MOAN);
					break;
				}

				case RBM_MOAN:
				{
#ifdef JP
					abbreviate = -1;
#endif
					act = desc_moan[randint0(4)];
					sound(SOUND_MOAN);
					break;
				}

				case RBM_SHOW:
				{
#ifdef JP
					abbreviate = -1;
#endif
					if (m_ptr->r_idx == MON_JAIAN)
					{
#ifdef JP
						switch(randint1(15))
						{
						  case 1:
						  case 6:
						  case 11:
							act = "「♪お～れはジャイアン～～ガ～キだいしょう～」";
							break;
						  case 2:
							act = "「♪て～んかむ～てきのお～とこだぜ～～」";
							break;
						  case 3:
							act = "「♪の～び太スネ夫はメじゃないよ～～」";
							break;
						  case 4:
							act = "「♪け～んかスポ～ツ～どんとこい～」";
							break;
						  case 5:
							act = "「♪うた～も～～う～まいぜ～まかしとけ～」";
							break;
						  case 7:
							act = "「♪ま～ちいちば～んのに～んきもの～～」";
							break;
						  case 8:
							act = "「♪べんきょうしゅくだいメじゃないよ～～」";
							break;
						  case 9:
							act = "「♪きはやさし～くて～ち～からもち～」";
							break;
						  case 10:
							act = "「♪かお～も～～スタイルも～バツグンさ～」";
							break;
						  case 12:
							act = "「♪がっこうい～ちの～あ～ばれんぼう～～」";
							break;
						  case 13:
							act = "「♪ド～ラもドラミもメじゃないよ～～」";
							break;
						  case 14:
							act = "「♪よじげんぽけっと～な～くたって～」";
							break;
						  case 15:
							act = "「♪あし～の～～ながさ～は～まけないぜ～」";
							break;
						}
#else
						act = "horribly sings 'I AM GIAAAAAN. THE BOOOSS OF THE KIIIIDS.'";
#endif
					}
					else
					{
						if (one_in_(3))
#ifdef JP
							act = "は♪僕らは楽しい家族♪と歌っている。";
						else
							act = "は♪アイ ラブ ユー、ユー ラブ ミー♪と歌っている。";
#else
							act = "sings 'We are a happy family.'";
						else
							act = "sings 'I love you, you love me.'";
#endif
					}

					sound(SOUND_SHOW);
					break;
				}
			}

			/* Message */
			if (act)
			{
				if (do_silly_attack)
				{
#ifdef JP
					abbreviate = -1;
#endif
					act = silly_attacks[randint0(MAX_SILLY_ATTACK)];
				}
#ifdef JP
				if (abbreviate == 0)
				    msg_format("%^sに%s", m_name, act);
				else if (abbreviate == 1)
				    msg_format("%s", act);
				else /* if (abbreviate == -1) */
				    msg_format("%^s%s", m_name, act);
				abbreviate = 1;/*２回目以降は省略 */
#else
				msg_format("%^s %s%s", m_name, act, do_silly_attack ? " you." : "");
#endif
			}

			/* Hack -- assume all attacks are obvious */
			obvious = TRUE;

			/* Roll out the damage */
			damage = damroll(d_dice, d_side);

			/*
			 * Skip the effect when exploding, since the explosion
			 * already causes the effect.
			 */
			if (explode)
				damage = 0;
			/* Apply appropriate damage */
			switch (effect)
			{
				case 0:
				{
					/* Hack -- Assume obvious */
					obvious = TRUE;

					/* Hack -- No damage */
					damage = 0;

					break;
				}

				case RBE_SUPERHURT:
				{
					if (((randint1(rlev*2+300) > (ac+200)) || one_in_(13)) && !CHECK_MULTISHADOW())
					{
						int tmp_damage = damage - (damage * ((ac < 150) ? ac : 150) / 250);
						msg_print(_("痛恨の一撃！", "It was a critical hit!"));
						tmp_damage = MAX(damage, tmp_damage*2);

						/* Take damage */
						get_damage += take_hit(DAMAGE_ATTACK, tmp_damage, ddesc, -1);
						break;
					}
				}
				case RBE_HURT:
				{
					/* Obvious */
					obvious = TRUE;

					/* Hack -- Player armor reduces total damage */
					damage -= (damage * ((ac < 150) ? ac : 150) / 250);

					/* Take damage */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					break;
				}

				case RBE_POISON:
				{
					if (explode) break;

					/* Take "poison" effect */
					if (!(p_ptr->resist_pois || IS_OPPOSE_POIS()) && !CHECK_MULTISHADOW())
					{
						if (set_poisoned(p_ptr->poisoned + randint1(rlev) + 5))
						{
							obvious = TRUE;
						}
					}

					/* Take some damage */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					/* Learn about the player */
					update_smart_learn(m_idx, DRS_POIS);

					break;
				}

				case RBE_UN_BONUS:
				{
					if (explode) break;

					/* Allow complete resist */
					if (!p_ptr->resist_disen && !CHECK_MULTISHADOW())
					{
						/* Apply disenchantment */
						if (apply_disenchant(0))
						{
							/* Hack -- Update AC */
							update_stuff();
							obvious = TRUE;
						}
					}

					/* Take some damage */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					/* Learn about the player */
					update_smart_learn(m_idx, DRS_DISEN);

					break;
				}

				case RBE_UN_POWER:
				{
					/* Take some damage */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					if (p_ptr->is_dead || CHECK_MULTISHADOW()) break;

					/* Find an item */
					for (k = 0; k < 10; k++)
					{
						/* Pick an item */
						i = randint0(INVEN_PACK);

						/* Obtain the item */
						o_ptr = &inventory[i];

						/* Skip non-objects */
						if (!o_ptr->k_idx) continue;

						/* Drain charged wands/staffs */
						if (((o_ptr->tval == TV_STAFF) ||
						     (o_ptr->tval == TV_WAND)) &&
						    (o_ptr->pval))
						{
							/* Calculate healed hitpoints */
							int heal=rlev * o_ptr->pval;
							if( o_ptr->tval == TV_STAFF)
							    heal *=  o_ptr->number;

							/* Don't heal more than max hp */
							heal = MIN(heal, m_ptr->maxhp - m_ptr->hp);

							/* Message */
							msg_print(_("ザックからエネルギーが吸い取られた！", "Energy drains from your pack!"));

							/* Obvious */
							obvious = TRUE;

							/* Heal the monster */
							m_ptr->hp += heal;

							/* Redraw (later) if needed */
							if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
							if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);

							/* Uncharge */
							o_ptr->pval = 0;

							/* Combine / Reorder the pack */
							p_ptr->notice |= (PN_COMBINE | PN_REORDER);

							/* Window stuff */
							p_ptr->window |= (PW_INVEN);

							/* Done */
							break;
						}
					}

					break;
				}

				case RBE_EAT_GOLD:
				{
					/* Take some damage */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					/* Confused monsters cannot steal successfully. -LM-*/
					if (MON_CONFUSED(m_ptr)) break;

					if (p_ptr->is_dead || CHECK_MULTISHADOW()) break;

					/* Obvious */
					obvious = TRUE;

					/* Saving throw (unless paralyzed) based on dex and level */
					if (!p_ptr->paralyzed &&
					    (randint0(100) < (adj_dex_safe[p_ptr->stat_ind[A_DEX]] +
							      p_ptr->lev)))
					{
						/* Saving throw message */
						msg_print(_("しかし素早く財布を守った！", "You quickly protect your money pouch!"));

						/* Occasional blink anyway */
						if (randint0(3)) blinked = TRUE;
					}

					/* Eat gold */
					else
					{
						gold = (p_ptr->au / 10) + randint1(25);
						if (gold < 2) gold = 2;
						if (gold > 5000) gold = (p_ptr->au / 20) + randint1(3000);
						if (gold > p_ptr->au) gold = p_ptr->au;
						p_ptr->au -= gold;
						if (gold <= 0)
						{
							msg_print(_("しかし何も盗まれなかった。", "Nothing was stolen."));
						}
						else if (p_ptr->au)
						{
							msg_print(_("財布が軽くなった気がする。", "Your purse feels lighter."));
							msg_format(_("$%ld のお金が盗まれた！", "%ld coins were stolen!"), (long)gold);
							chg_virtue(V_SACRIFICE, 1);
						}
						else
						{
							msg_print(_("財布が軽くなった気がする。", "Your purse feels lighter."));
							msg_print(_("お金が全部盗まれた！", "All of your coins were stolen!"));
							chg_virtue(V_SACRIFICE, 2);
						}

						/* Redraw gold */
						p_ptr->redraw |= (PR_GOLD);

						/* Window stuff */
						p_ptr->window |= (PW_PLAYER);

						/* Blink away */
						blinked = TRUE;
					}

					break;
				}

				case RBE_EAT_ITEM:
				{
					/* Take some damage */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					/* Confused monsters cannot steal successfully. -LM-*/
					if (MON_CONFUSED(m_ptr)) break;

					if (p_ptr->is_dead || CHECK_MULTISHADOW()) break;

					/* Saving throw (unless paralyzed) based on dex and level */
					if (!p_ptr->paralyzed &&
					    (randint0(100) < (adj_dex_safe[p_ptr->stat_ind[A_DEX]] +
							      p_ptr->lev)))
					{
						/* Saving throw message */
						msg_print(_("しかしあわててザックを取り返した！", "You grab hold of your backpack!"));

						/* Occasional "blink" anyway */
						blinked = TRUE;

						/* Obvious */
						obvious = TRUE;

						/* Done */
						break;
					}

					/* Find an item */
					for (k = 0; k < 10; k++)
					{
						s16b o_idx;

						/* Pick an item */
						i = randint0(INVEN_PACK);

						/* Obtain the item */
						o_ptr = &inventory[i];

						/* Skip non-objects */
						if (!o_ptr->k_idx) continue;

						/* Skip artifacts */
						if (object_is_artifact(o_ptr)) continue;

						/* Get a description */
						object_desc(o_name, o_ptr, OD_OMIT_PREFIX);

						/* Message */
#ifdef JP
						msg_format("%s(%c)を%s盗まれた！",
							   o_name, index_to_label(i),
							   ((o_ptr->number > 1) ? "一つ" : ""));
#else
						msg_format("%sour %s (%c) was stolen!",
							   ((o_ptr->number > 1) ? "One of y" : "Y"),
							   o_name, index_to_label(i));
#endif

						chg_virtue(V_SACRIFICE, 1);


						/* Make an object */
						o_idx = o_pop();

						/* Success */
						if (o_idx)
						{
							object_type *j_ptr;

							/* Get new object */
							j_ptr = &o_list[o_idx];

							/* Copy object */
							object_copy(j_ptr, o_ptr);

							/* Modify number */
							j_ptr->number = 1;

							/* Hack -- If a rod or wand, allocate total
							 * maximum timeouts or charges between those
							 * stolen and those missed. -LM-
							 */
							if ((o_ptr->tval == TV_ROD) || (o_ptr->tval == TV_WAND))
							{
								j_ptr->pval = o_ptr->pval / o_ptr->number;
								o_ptr->pval -= j_ptr->pval;
							}

							/* Forget mark */
							j_ptr->marked = OM_TOUCHED;

							/* Memorize monster */
							j_ptr->held_m_idx = m_idx;

							/* Build stack */
							j_ptr->next_o_idx = m_ptr->hold_o_idx;

							/* Build stack */
							m_ptr->hold_o_idx = o_idx;
						}

						/* Steal the items */
						inven_item_increase(i, -1);
						inven_item_optimize(i);

						/* Obvious */
						obvious = TRUE;

						/* Blink away */
						blinked = TRUE;

						/* Done */
						break;
					}

					break;
				}

				case RBE_EAT_FOOD:
				{
					/* Take some damage */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					if (p_ptr->is_dead || CHECK_MULTISHADOW()) break;

					/* Steal some food */
					for (k = 0; k < 10; k++)
					{
						/* Pick an item from the pack */
						i = randint0(INVEN_PACK);

						/* Get the item */
						o_ptr = &inventory[i];

						/* Skip non-objects */
						if (!o_ptr->k_idx) continue;

						/* Skip non-food objects */
						if ((o_ptr->tval != TV_FOOD) && !((o_ptr->tval == TV_CORPSE) && (o_ptr->sval))) continue;

						/* Get a description */
						object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

						/* Message */
#ifdef JP
						msg_format("%s(%c)を%s食べられてしまった！",
							  o_name, index_to_label(i),
							  ((o_ptr->number > 1) ? "一つ" : ""));
#else
						msg_format("%sour %s (%c) was eaten!",
							   ((o_ptr->number > 1) ? "One of y" : "Y"),
							   o_name, index_to_label(i));
#endif


						/* Steal the items */
						inven_item_increase(i, -1);
						inven_item_optimize(i);

						/* Obvious */
						obvious = TRUE;

						/* Done */
						break;
					}

					break;
				}

				case RBE_EAT_LITE:
				{
					/* Access the lite */
					o_ptr = &inventory[INVEN_LITE];

					/* Take some damage */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					if (p_ptr->is_dead || CHECK_MULTISHADOW()) break;

					/* Drain fuel */
					if ((o_ptr->xtra4 > 0) && (!object_is_fixed_artifact(o_ptr)))
					{
						/* Reduce fuel */
						o_ptr->xtra4 -= (s16b)(250 + randint1(250));
						if (o_ptr->xtra4 < 1) o_ptr->xtra4 = 1;

						/* Notice */
						if (!p_ptr->blind)
						{
							msg_print(_("明かりが暗くなってしまった。", "Your light dims."));
							obvious = TRUE;
						}

						/* Window stuff */
						p_ptr->window |= (PW_EQUIP);
					}

					break;
				}

				case RBE_ACID:
				{
					if (explode) break;
					/* Obvious */
					obvious = TRUE;

					/* Message */
					msg_print(_("酸を浴びせられた！", "You are covered in acid!"));

					/* Special damage */
					get_damage += acid_dam(damage, ddesc, -1, FALSE);

					/* Hack -- Update AC */
					update_stuff();

					/* Learn about the player */
					update_smart_learn(m_idx, DRS_ACID);

					break;
				}

				case RBE_ELEC:
				{
					if (explode) break;
					/* Obvious */
					obvious = TRUE;

					/* Message */
					msg_print(_("電撃を浴びせられた！", "You are struck by electricity!"));

					/* Special damage */
					get_damage += elec_dam(damage, ddesc, -1, FALSE);

					/* Learn about the player */
					update_smart_learn(m_idx, DRS_ELEC);

					break;
				}

				case RBE_FIRE:
				{
					if (explode) break;
					/* Obvious */
					obvious = TRUE;

					/* Message */
					msg_print(_("全身が炎に包まれた！", "You are enveloped in flames!"));

					/* Special damage */
					get_damage += fire_dam(damage, ddesc, -1, FALSE);

					/* Learn about the player */
					update_smart_learn(m_idx, DRS_FIRE);

					break;
				}

				case RBE_COLD:
				{
					if (explode) break;
					/* Obvious */
					obvious = TRUE;

					/* Message */
					msg_print(_("全身が冷気で覆われた！", "You are covered with frost!"));

					/* Special damage */
					get_damage += cold_dam(damage, ddesc, -1, FALSE);

					/* Learn about the player */
					update_smart_learn(m_idx, DRS_COLD);

					break;
				}

				case RBE_BLIND:
				{
					/* Take damage */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					if (p_ptr->is_dead) break;

					/* Increase "blind" */
					if (!p_ptr->resist_blind && !CHECK_MULTISHADOW())
					{
						if (set_blind(p_ptr->blind + 10 + randint1(rlev)))
						{
#ifdef JP
							if (m_ptr->r_idx == MON_DIO) msg_print("どうだッ！この血の目潰しはッ！");
#else
							/* nanka */
#endif
							obvious = TRUE;
						}
					}

					/* Learn about the player */
					update_smart_learn(m_idx, DRS_BLIND);

					break;
				}

				case RBE_CONFUSE:
				{
					if (explode) break;
					/* Take damage */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					if (p_ptr->is_dead) break;

					/* Increase "confused" */
					if (!p_ptr->resist_conf && !CHECK_MULTISHADOW())
					{
						if (set_confused(p_ptr->confused + 3 + randint1(rlev)))
						{
							obvious = TRUE;
						}
					}

					/* Learn about the player */
					update_smart_learn(m_idx, DRS_CONF);

					break;
				}

				case RBE_TERRIFY:
				{
					/* Take damage */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					if (p_ptr->is_dead) break;

					/* Increase "afraid" */
					if (CHECK_MULTISHADOW())
					{
						/* Do nothing */
					}
					else if (p_ptr->resist_fear)
					{
						msg_print(_("しかし恐怖に侵されなかった！", "You stand your ground!"));
						obvious = TRUE;
					}
					else if (randint0(100 + r_ptr->level/2) < p_ptr->skill_sav)
					{
						msg_print(_("しかし恐怖に侵されなかった！", "You stand your ground!"));
						obvious = TRUE;
					}
					else
					{
						if (set_afraid(p_ptr->afraid + 3 + randint1(rlev)))
						{
							obvious = TRUE;
						}
					}

					/* Learn about the player */
					update_smart_learn(m_idx, DRS_FEAR);

					break;
				}

				case RBE_PARALYZE:
				{
					/* Take damage */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					if (p_ptr->is_dead) break;

					/* Increase "paralyzed" */
					if (CHECK_MULTISHADOW())
					{
						/* Do nothing */
					}
					else if (p_ptr->free_act)
					{
						msg_print(_("しかし効果がなかった！", "You are unaffected!"));
						obvious = TRUE;
					}
					else if (randint0(100 + r_ptr->level/2) < p_ptr->skill_sav)
					{
						msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
						obvious = TRUE;
					}
					else
					{
						if (!p_ptr->paralyzed)
						{
							if (set_paralyzed(3 + randint1(rlev)))
							{
								obvious = TRUE;
							}
						}
					}

					/* Learn about the player */
					update_smart_learn(m_idx, DRS_FREE);

					break;
				}

				case RBE_LOSE_STR:
				{
					/* Damage (physical) */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					if (p_ptr->is_dead || CHECK_MULTISHADOW()) break;

					/* Damage (stat) */
					if (do_dec_stat(A_STR)) obvious = TRUE;

					break;
				}

				case RBE_LOSE_INT:
				{
					/* Damage (physical) */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					if (p_ptr->is_dead || CHECK_MULTISHADOW()) break;

					/* Damage (stat) */
					if (do_dec_stat(A_INT)) obvious = TRUE;

					break;
				}

				case RBE_LOSE_WIS:
				{
					/* Damage (physical) */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					if (p_ptr->is_dead || CHECK_MULTISHADOW()) break;

					/* Damage (stat) */
					if (do_dec_stat(A_WIS)) obvious = TRUE;

					break;
				}

				case RBE_LOSE_DEX:
				{
					/* Damage (physical) */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					if (p_ptr->is_dead || CHECK_MULTISHADOW()) break;

					/* Damage (stat) */
					if (do_dec_stat(A_DEX)) obvious = TRUE;

					break;
				}

				case RBE_LOSE_CON:
				{
					/* Damage (physical) */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					if (p_ptr->is_dead || CHECK_MULTISHADOW()) break;

					/* Damage (stat) */
					if (do_dec_stat(A_CON)) obvious = TRUE;

					break;
				}

				case RBE_LOSE_CHR:
				{
					/* Damage (physical) */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					if (p_ptr->is_dead || CHECK_MULTISHADOW()) break;

					/* Damage (stat) */
					if (do_dec_stat(A_CHR)) obvious = TRUE;

					break;
				}

				case RBE_LOSE_ALL:
				{
					/* Damage (physical) */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					if (p_ptr->is_dead || CHECK_MULTISHADOW()) break;

					/* Damage (stats) */
					if (do_dec_stat(A_STR)) obvious = TRUE;
					if (do_dec_stat(A_DEX)) obvious = TRUE;
					if (do_dec_stat(A_CON)) obvious = TRUE;
					if (do_dec_stat(A_INT)) obvious = TRUE;
					if (do_dec_stat(A_WIS)) obvious = TRUE;
					if (do_dec_stat(A_CHR)) obvious = TRUE;

					break;
				}

				case RBE_SHATTER:
				{
					/* Obvious */
					obvious = TRUE;

					/* Hack -- Reduce damage based on the player armor class */
					damage -= (damage * ((ac < 150) ? ac : 150) / 250);

					/* Take damage */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					/* Radius 8 earthquake centered at the monster */
					if (damage > 23 || explode)
					{
						earthquake_aux(m_ptr->fy, m_ptr->fx, 8, m_idx);
					}

					break;
				}

				case RBE_EXP_10:
				{
					s32b d = damroll(10, 6) + (p_ptr->exp / 100) * MON_DRAIN_LIFE;

					/* Obvious */
					obvious = TRUE;

					/* Take damage */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					if (p_ptr->is_dead || CHECK_MULTISHADOW()) break;

					(void)drain_exp(d, d / 10, 95);
					break;
				}

				case RBE_EXP_20:
				{
					s32b d = damroll(20, 6) + (p_ptr->exp / 100) * MON_DRAIN_LIFE;

					/* Obvious */
					obvious = TRUE;

					/* Take damage */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					if (p_ptr->is_dead || CHECK_MULTISHADOW()) break;

					(void)drain_exp(d, d / 10, 90);
					break;
				}

				case RBE_EXP_40:
				{
					s32b d = damroll(40, 6) + (p_ptr->exp / 100) * MON_DRAIN_LIFE;

					/* Obvious */
					obvious = TRUE;

					/* Take damage */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					if (p_ptr->is_dead || CHECK_MULTISHADOW()) break;

					(void)drain_exp(d, d / 10, 75);
					break;
				}

				case RBE_EXP_80:
				{
					s32b d = damroll(80, 6) + (p_ptr->exp / 100) * MON_DRAIN_LIFE;

					/* Obvious */
					obvious = TRUE;

					/* Take damage */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					if (p_ptr->is_dead || CHECK_MULTISHADOW()) break;

					(void)drain_exp(d, d / 10, 50);
					break;
				}

				case RBE_DISEASE:
				{
					/* Take some damage */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					if (p_ptr->is_dead || CHECK_MULTISHADOW()) break;

					/* Take "poison" effect */
					if (!(p_ptr->resist_pois || IS_OPPOSE_POIS()))
					{
						if (set_poisoned(p_ptr->poisoned + randint1(rlev) + 5))
						{
							obvious = TRUE;
						}
					}

					/* Damage CON (10% chance)*/
					if ((randint1(100) < 11) && (p_ptr->prace != RACE_ANDROID))
					{
						/* 1% chance for perm. damage */
						bool perm = one_in_(10);
						if (dec_stat(A_CON, randint1(10), perm))
						{
							msg_print(_("病があなたを蝕んでいる気がする。", "You feel strange sickness."));
							obvious = TRUE;
						}
					}

					break;
				}
				case RBE_TIME:
				{
					if (explode) break;
					if (!p_ptr->resist_time && !CHECK_MULTISHADOW())
					{
						switch (randint1(10))
						{
							case 1: case 2: case 3: case 4: case 5:
							{
								if (p_ptr->prace == RACE_ANDROID) break;
								msg_print(_("人生が逆戻りした気がする。", "You feel life has clocked back."));
								lose_exp(100 + (p_ptr->exp / 100) * MON_DRAIN_LIFE);
								break;
							}

							case 6: case 7: case 8: case 9:
							{
								int stat = randint0(6);

								switch (stat)
								{
#ifdef JP
									case A_STR: act = "強く"; break;
									case A_INT: act = "聡明で"; break;
									case A_WIS: act = "賢明で"; break;
									case A_DEX: act = "器用で"; break;
									case A_CON: act = "健康で"; break;
									case A_CHR: act = "美しく"; break;
#else
									case A_STR: act = "strong"; break;
									case A_INT: act = "bright"; break;
									case A_WIS: act = "wise"; break;
									case A_DEX: act = "agile"; break;
									case A_CON: act = "hale"; break;
									case A_CHR: act = "beautiful"; break;
#endif

								}

								msg_format(_("あなたは以前ほど%sなくなってしまった...。", "You're not as %s as you used to be..."), act);
								p_ptr->stat_cur[stat] = (p_ptr->stat_cur[stat] * 3) / 4;
								if (p_ptr->stat_cur[stat] < 3) p_ptr->stat_cur[stat] = 3;
								p_ptr->update |= (PU_BONUS);
								break;
							}

							case 10:
							{
								msg_print(_("あなたは以前ほど力強くなくなってしまった...。", "You're not as powerful as you used to be..."));

								for (k = 0; k < 6; k++)
								{
									p_ptr->stat_cur[k] = (p_ptr->stat_cur[k] * 7) / 8;
									if (p_ptr->stat_cur[k] < 3) p_ptr->stat_cur[k] = 3;
								}
								p_ptr->update |= (PU_BONUS);
								break;
							}
						}
					}
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					break;
				}
				case RBE_DR_LIFE:
				{
					s32b d = damroll(60, 6) + (p_ptr->exp / 100) * MON_DRAIN_LIFE;
					bool resist_drain;

					/* Obvious */
					obvious = TRUE;

					/* Take damage */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					if (p_ptr->is_dead || CHECK_MULTISHADOW()) break;

					resist_drain = !drain_exp(d, d / 10, 50);

					/* Heal the attacker? */
					if (p_ptr->mimic_form)
					{
						if (mimic_info[p_ptr->mimic_form].MIMIC_FLAGS & MIMIC_IS_NONLIVING)
							resist_drain = TRUE;
					}
					else
					{
						switch (p_ptr->prace)
						{
						case RACE_ZOMBIE:
						case RACE_VAMPIRE:
						case RACE_SPECTRE:
						case RACE_SKELETON:
						case RACE_DEMON:
						case RACE_GOLEM:
						case RACE_ANDROID:
							resist_drain = TRUE;
							break;
						}
					}

					if ((damage > 5) && !resist_drain)
					{
						bool did_heal = FALSE;

						if (m_ptr->hp < m_ptr->maxhp) did_heal = TRUE;

						/* Heal */
						m_ptr->hp += damroll(4, damage / 6);
						if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

						/* Redraw (later) if needed */
						if (p_ptr->health_who == m_idx) p_ptr->redraw |= (PR_HEALTH);
						if (p_ptr->riding == m_idx) p_ptr->redraw |= (PR_UHEALTH);

						/* Special message */
						if (m_ptr->ml && did_heal)
						{
							msg_format(_("%sは体力を回復したようだ。", "%^s appears healthier."), m_name);
						}
					}

					break;
				}
				case RBE_DR_MANA:
				{
					/* Obvious */
					obvious = TRUE;

					if (CHECK_MULTISHADOW())
					{
						msg_print(_("攻撃は幻影に命中し、あなたには届かなかった。", "The attack hits Shadow, you are unharmed!"));
					}
					else
					{
						do_cut = 0;

						/* Take damage */
						p_ptr->csp -= damage;
						if (p_ptr->csp < 0)
						{
							p_ptr->csp = 0;
							p_ptr->csp_frac = 0;
						}

						p_ptr->redraw |= (PR_MANA);
					}

					/* Learn about the player */
					update_smart_learn(m_idx, DRS_MANA);

					break;
				}
				case RBE_INERTIA:
				{
					/* Take damage */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					if (p_ptr->is_dead) break;

					/* Decrease speed */
					if (CHECK_MULTISHADOW())
					{
						/* Do nothing */
					}
					else
					{
						if (set_slow((p_ptr->slow + 4 + randint0(rlev / 10)), FALSE))
						{
							obvious = TRUE;
						}
					}

					break;
				}
				case RBE_STUN:
				{
					/* Take damage */
					get_damage += take_hit(DAMAGE_ATTACK, damage, ddesc, -1);

					if (p_ptr->is_dead) break;

					/* Decrease speed */
					if (p_ptr->resist_sound || CHECK_MULTISHADOW())
					{
						/* Do nothing */
					}
					else
					{
						if (set_stun(p_ptr->stun + 10 + randint1(r_ptr->level / 4)))
						{
							obvious = TRUE;
						}
					}

					break;
				}
			}

			/* Hack -- only one of cut or stun */
			if (do_cut && do_stun)
			{
				/* Cancel cut */
				if (randint0(100) < 50)
				{
					do_cut = 0;
				}

				/* Cancel stun */
				else
				{
					do_stun = 0;
				}
			}

			/* Handle cut */
			if (do_cut)
			{
				int k = 0;

				/* Critical hit (zero if non-critical) */
				tmp = monster_critical(d_dice, d_side, damage);

				/* Roll for damage */
				switch (tmp)
				{
					case 0: k = 0; break;
					case 1: k = randint1(5); break;
					case 2: k = randint1(5) + 5; break;
					case 3: k = randint1(20) + 20; break;
					case 4: k = randint1(50) + 50; break;
					case 5: k = randint1(100) + 100; break;
					case 6: k = 300; break;
					default: k = 500; break;
				}

				/* Apply the cut */
				if (k) (void)set_cut(p_ptr->cut + k);
			}

			/* Handle stun */
			if (do_stun)
			{
				int k = 0;

				/* Critical hit (zero if non-critical) */
				tmp = monster_critical(d_dice, d_side, damage);

				/* Roll for damage */
				switch (tmp)
				{
					case 0: k = 0; break;
					case 1: k = randint1(5); break;
					case 2: k = randint1(5) + 10; break;
					case 3: k = randint1(10) + 20; break;
					case 4: k = randint1(15) + 30; break;
					case 5: k = randint1(20) + 40; break;
					case 6: k = 80; break;
					default: k = 150; break;
				}

				/* Apply the stun */
				if (k) (void)set_stun(p_ptr->stun + k);
			}

			if (explode)
			{
				sound(SOUND_EXPLODE);

				if (mon_take_hit(m_idx, m_ptr->hp + 1, &fear, NULL))
				{
					blinked = FALSE;
					alive = FALSE;
				}
			}

			if (touched)
			{
				if (p_ptr->sh_fire && alive && !p_ptr->is_dead)
				{
					if (!(r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK))
					{
						int dam = damroll(2, 6);

						/* Modify the damage */
						dam = mon_damage_mod(m_ptr, dam, FALSE);

#ifdef JP
						msg_format("%^sは突然熱くなった！", m_name);
						if (mon_take_hit(m_idx, dam, &fear,
						    "は灰の山になった。"))
#else
						msg_format("%^s is suddenly very hot!", m_name);

						if (mon_take_hit(m_idx, dam, &fear,
						    " turns into a pile of ash."))
#endif

						{
							blinked = FALSE;
							alive = FALSE;
						}
					}
					else
					{
						if (is_original_ap_and_seen(m_ptr))
							r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK);
					}
				}

				if (p_ptr->sh_elec && alive && !p_ptr->is_dead)
				{
					if (!(r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK))
					{
						int dam = damroll(2, 6);

						/* Modify the damage */
						dam = mon_damage_mod(m_ptr, dam, FALSE);

#ifdef JP
						msg_format("%^sは電撃をくらった！", m_name);
						if (mon_take_hit(m_idx, dam, &fear,
						    "は燃え殻の山になった。"))
#else
						msg_format("%^s gets zapped!", m_name);

						if (mon_take_hit(m_idx, dam, &fear,
						    " turns into a pile of cinder."))
#endif

						{
							blinked = FALSE;
							alive = FALSE;
						}
					}
					else
					{
						if (is_original_ap_and_seen(m_ptr))
							r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK);
					}
				}

				if (p_ptr->sh_cold && alive && !p_ptr->is_dead)
				{
					if (!(r_ptr->flagsr & RFR_EFF_IM_COLD_MASK))
					{
						int dam = damroll(2, 6);

						/* Modify the damage */
						dam = mon_damage_mod(m_ptr, dam, FALSE);

#ifdef JP
						msg_format("%^sは冷気をくらった！", m_name);
						if (mon_take_hit(m_idx, dam, &fear,
						    "は凍りついた。"))
#else
						msg_format("%^s is very cold!", m_name);

						if (mon_take_hit(m_idx, dam, &fear,
						    " was frozen."))
#endif

						{
							blinked = FALSE;
							alive = FALSE;
						}
					}
					else
					{
						if (is_original_ap_and_seen(m_ptr))
							r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_COLD_MASK);
					}
				}

				/* by henkma */
				if (p_ptr->dustrobe && alive && !p_ptr->is_dead)
				{
					if (!(r_ptr->flagsr & RFR_EFF_RES_SHAR_MASK))
					{
						int dam = damroll(2, 6);

						/* Modify the damage */
						dam = mon_damage_mod(m_ptr, dam, FALSE);

#ifdef JP
						msg_format("%^sは鏡の破片をくらった！", m_name);
						if (mon_take_hit(m_idx, dam, &fear,
						    "はズタズタになった。"))
#else
						msg_format("%^s gets zapped!", m_name);

						if (mon_take_hit(m_idx, dam, &fear,
						    " had torn to pieces."))
#endif
						{
							blinked = FALSE;
							alive = FALSE;
						}
					}
					else
					{
						if (is_original_ap_and_seen(m_ptr))
							r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_RES_SHAR_MASK);
					}

					if (is_mirror_grid(&cave[py][px]))
					{
						teleport_player(10, 0L);
					}
				}

				if (p_ptr->tim_sh_holy && alive && !p_ptr->is_dead)
				{
					if (r_ptr->flags3 & RF3_EVIL)
					{
						if (!(r_ptr->flagsr & RFR_RES_ALL))
						{
							int dam = damroll(2, 6);

							/* Modify the damage */
							dam = mon_damage_mod(m_ptr, dam, FALSE);

#ifdef JP
							msg_format("%^sは聖なるオーラで傷ついた！", m_name);
							if (mon_take_hit(m_idx, dam, &fear,
							    "は倒れた。"))
#else
							msg_format("%^s is injured by holy power!", m_name);

							if (mon_take_hit(m_idx, dam, &fear,
							    " is destroyed."))
#endif
							{
								blinked = FALSE;
								alive = FALSE;
							}
							if (is_original_ap_and_seen(m_ptr))
								r_ptr->r_flags3 |= RF3_EVIL;
						}
						else
						{
							if (is_original_ap_and_seen(m_ptr))
								r_ptr->r_flagsr |= RFR_RES_ALL;
						}
					}
				}

				if (p_ptr->tim_sh_touki && alive && !p_ptr->is_dead)
				{
					if (!(r_ptr->flagsr & RFR_RES_ALL))
					{
						int dam = damroll(2, 6);

						/* Modify the damage */
						dam = mon_damage_mod(m_ptr, dam, FALSE);

#ifdef JP
						msg_format("%^sが鋭い闘気のオーラで傷ついた！", m_name);
						if (mon_take_hit(m_idx, dam, &fear,
						    "は倒れた。"))
#else
						msg_format("%^s is injured by the Force", m_name);

						if (mon_take_hit(m_idx, dam, &fear,
						    " is destroyed."))
#endif

						{
							blinked = FALSE;
							alive = FALSE;
						}
					}
					else
					{
						if (is_original_ap_and_seen(m_ptr))
							r_ptr->r_flagsr |= RFR_RES_ALL;
					}
				}

				if (hex_spelling(HEX_SHADOW_CLOAK) && alive && !p_ptr->is_dead)
				{
					int dam = 1;
					object_type *o_ptr = &inventory[INVEN_RARM];

					if (!(r_ptr->flagsr & RFR_RES_ALL || r_ptr->flagsr & RFR_RES_DARK))
					{
						if (o_ptr->k_idx)
						{
							int basedam = ((o_ptr->dd + p_ptr->to_dd[0]) * (o_ptr->ds + p_ptr->to_ds[0] + 1));
							dam = basedam / 2 + o_ptr->to_d + p_ptr->to_d[0];
						}

						/* Cursed armor makes damages doubled */
						o_ptr = &inventory[INVEN_BODY];
						if ((o_ptr->k_idx) && object_is_cursed(o_ptr)) dam *= 2;

						/* Modify the damage */
						dam = mon_damage_mod(m_ptr, dam, FALSE);

#ifdef JP
						msg_format("影のオーラが%^sに反撃した！", m_name);
						if (mon_take_hit(m_idx, dam, &fear, "は倒れた。"))
#else
						msg_format("Enveloped shadows attack %^s.", m_name);

						if (mon_take_hit(m_idx, dam, &fear, " is destroyed."))
#endif
						{
							blinked = FALSE;
							alive = FALSE;
						}
						else /* monster does not dead */
						{
							int j;
							int flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
							int typ[4][2] = {
								{ INVEN_HEAD, GF_OLD_CONF },
								{ INVEN_LARM,  GF_OLD_SLEEP },
								{ INVEN_HANDS, GF_TURN_ALL },
								{ INVEN_FEET, GF_OLD_SLOW }
							};

							/* Some cursed armours gives an extra effect */
							for (j = 0; j < 4; j++)
							{
								o_ptr = &inventory[typ[j][0]];
								if ((o_ptr->k_idx) && object_is_cursed(o_ptr) && object_is_armour(o_ptr))
									project(0, 0, m_ptr->fy, m_ptr->fx, (p_ptr->lev * 2), typ[j][1], flg, -1);
							}
						}
					}
					else
					{
						if (is_original_ap_and_seen(m_ptr))
							r_ptr->r_flagsr |= (RFR_RES_ALL | RFR_RES_DARK);
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

				/* Visible monsters */
				if (m_ptr->ml)
				{
					/* Disturbing */
					disturb(1, 1);

					/* Message */
#ifdef JP
					if (abbreviate)
					    msg_format("%sかわした。", (p_ptr->special_attack & ATTACK_SUIKEN) ? "奇妙な動きで" : "");
					else
					    msg_format("%s%^sの攻撃をかわした。", (p_ptr->special_attack & ATTACK_SUIKEN) ? "奇妙な動きで" : "", m_name);
					abbreviate = 1;/*２回目以降は省略 */
#else
					msg_format("%^s misses you.", m_name);
#endif

				}
				damage = 0;

				break;
			}
		}


		/* Analyze "visible" monsters only */
		if (is_original_ap_and_seen(m_ptr) && !do_silly_attack)
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

		if (p_ptr->riding && damage)
		{
			char m_name[80];
			monster_desc(m_name, &m_list[p_ptr->riding], 0);
			if (rakuba((damage > 200) ? 200 : damage, FALSE))
			{
				msg_format(_("%^sから落ちてしまった！", "You have fallen from %s."), m_name);
			}
		}

		if (p_ptr->special_defense & NINJA_KAWARIMI)
		{
			if (kawarimi(FALSE)) return TRUE;
		}
	}

	/* Hex - revenge damage stored */
	revenge_store(get_damage);

	if ((p_ptr->tim_eyeeye || hex_spelling(HEX_EYE_FOR_EYE))
		&& get_damage > 0 && !p_ptr->is_dead)
	{
#ifdef JP
		msg_format("攻撃が%s自身を傷つけた！", m_name);
#else
		char m_name_self[80];

		/* hisself */
		monster_desc(m_name_self, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE | MD_OBJECTIVE);

		msg_format("The attack of %s has wounded %s!", m_name, m_name_self);
#endif
		project(0, 0, m_ptr->fy, m_ptr->fx, get_damage, GF_MISSILE, PROJECT_KILL, -1);
		if (p_ptr->tim_eyeeye) set_tim_eyeeye(p_ptr->tim_eyeeye-5, TRUE);
	}

	if ((p_ptr->counter || (p_ptr->special_defense & KATA_MUSOU)) && alive && !p_ptr->is_dead && m_ptr->ml && (p_ptr->csp > 7))
	{
		char m_name[80];
		monster_desc(m_name, m_ptr, 0);

		p_ptr->csp -= 7;
		msg_format(_("%^sに反撃した！", "Your counterattack to %s!"), m_name);
		py_attack(m_ptr->fy, m_ptr->fx, HISSATSU_COUNTER);
		fear = FALSE;

		/* Redraw mana */
		p_ptr->redraw |= (PR_MANA);
	}

	/* Blink away */
	if (blinked && alive && !p_ptr->is_dead)
	{
		if (teleport_barrier(m_idx))
		{
			msg_print(_("泥棒は笑って逃げ...ようとしたがバリアに防がれた。", "The thief flees laughing...? But magic barrier obstructs it."));
		}
		else
		{
			msg_print(_("泥棒は笑って逃げた！", "The thief flees laughing!"));
			teleport_away(m_idx, MAX_SIGHT * 2 + 5, 0L);
		}
	}


	/* Always notice cause of death */
	if (p_ptr->is_dead && (r_ptr->r_deaths < MAX_SHORT) && !p_ptr->inside_arena)
	{
		r_ptr->r_deaths++;
	}

	if (m_ptr->ml && fear && alive && !p_ptr->is_dead)
	{
		sound(SOUND_FLEE);
		msg_format(_("%^sは恐怖で逃げ出した！", "%^s flees in terror!"), m_name);
	}

	if (p_ptr->special_defense & KATA_IAI)
	{
		set_action(ACTION_NONE);
	}

	/* Assume we attacked */
	return (TRUE);
}
