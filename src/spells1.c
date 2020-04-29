/*!
 * @file spells1.c
 * @brief 魔法による遠隔処理の実装 / Spell projection
 * @date 2014/07/10
 * @author
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * </pre>
 */

#include "angband.h"
#include "core.h"
#include "util.h"
#include "main/sound-definitions-table.h"

#include "io/write-diary.h"
#include "cmd/cmd-pet.h"
#include "cmd/cmd-dump.h"
#include "player-damage.h"
#include "player-effects.h"
#include "player-class.h"

#include "monster.h"
#include "monster-status.h"
#include "monster-spell.h"
#include "spells.h"
#include "spells-diceroll.h"
#include "monsterrace-hook.h"

#include "melee.h"
#include "world.h"
#include "avatar.h"
#include "player-move.h"
#include "quest.h"
#include "gameterm.h"
#include "view/display-main-window.h"

#include "effect/effect-feature.h"
#include "effect/effect-item.h"
#include "effect/effect-player.h"
#include "effect/spells-effect-util.h"
#include "realm/realm-hex.h"

/*!
 * @brief 配置した鏡リストの次を取得する /
 * Get another mirror. for SEEKER
 * @param next_y 次の鏡のy座標を返す参照ポインタ
 * @param next_x 次の鏡のx座標を返す参照ポインタ
 * @param cury 現在の鏡のy座標
 * @param curx 現在の鏡のx座標
 */
static void next_mirror(player_type *creature_ptr, POSITION* next_y, POSITION* next_x, POSITION cury, POSITION curx)
{
	POSITION mirror_x[10], mirror_y[10]; /* 鏡はもっと少ない */
	int mirror_num = 0;			  /* 鏡の数 */
	POSITION x, y;
	int num;

	for (x = 0; x < creature_ptr->current_floor_ptr->width; x++)
	{
		for (y = 0; y < creature_ptr->current_floor_ptr->height; y++)
		{
			if (is_mirror_grid(&creature_ptr->current_floor_ptr->grid_array[y][x])) {
				mirror_y[mirror_num] = y;
				mirror_x[mirror_num] = x;
				mirror_num++;
			}
		}
	}
	if (mirror_num)
	{
		num = randint0(mirror_num);
		*next_y = mirror_y[num];
		*next_x = mirror_x[num];
		return;
	}
	*next_y = cury + randint0(5) - 2;
	*next_x = curx + randint0(5) - 2;
	return;
}


/*!
 * @brief 汎用的なビーム/ボルト/ボール系によるモンスターへの効果処理 / Handle a beam/bolt/ball causing damage to a monster.
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param who 魔法を発動したモンスター(0ならばプレイヤー) / Index of "source" monster (zero for "player")
 * @param r 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標Y座標 / Target y location (or location to travel "towards")
 * @param x 目標X座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param typ 効果属性 / Type of damage to apply to monsters (and objects)
 * @param flag 効果フラグ
 * @param see_s_msg TRUEならばメッセージを表示する
 * @return 何か一つでも効力があればTRUEを返す / TRUE if any "effects" of the projection were observed, else FALSE
 */
static bool project_m(player_type *caster_ptr, MONSTER_IDX who, POSITION r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID typ, BIT_FLAGS flag, bool see_s_msg)
{
	floor_type *floor_ptr = caster_ptr->current_floor_ptr;
	grid_type *g_ptr = &floor_ptr->grid_array[y][x];

	monster_type *m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
	monster_type *m_caster_ptr = (who > 0) ? &floor_ptr->m_list[who] : NULL;

	monster_race *r_ptr = &r_info[m_ptr->r_idx];

	char killer[80];

	bool seen = m_ptr->ml;
	bool seen_msg = is_seen(m_ptr);
	bool slept = (bool)MON_CSLEEP(m_ptr);
	bool obvious = FALSE;
	bool known = ((m_ptr->cdis <= MAX_SIGHT) || caster_ptr->phase_out);
	bool skipped = FALSE;
	bool get_angry = FALSE;
	bool do_poly = FALSE;
	int do_dist = 0;
	int do_conf = 0;
	int do_stun = 0;
	int do_sleep = 0;
	int do_fear = 0;
	int do_time = 0;
	bool heal_leper = FALSE;
	GAME_TEXT m_name[MAX_NLEN];
	char m_poss[10];
	PARAMETER_VALUE photo = 0;
	concptr note = NULL;
	concptr note_dies = extract_note_dies(real_r_idx(m_ptr));
	DEPTH caster_lev = (who > 0) ? r_info[m_caster_ptr->r_idx].level : (caster_ptr->lev * 2);

	if (!g_ptr->m_idx) return FALSE;

	/* Never affect projector */
	if (who && (g_ptr->m_idx == who)) return FALSE;
	if ((g_ptr->m_idx == caster_ptr->riding) && !who && !(typ == GF_OLD_HEAL) && !(typ == GF_OLD_SPEED) && !(typ == GF_STAR_HEAL)) return FALSE;
	if (sukekaku && ((m_ptr->r_idx == MON_SUKE) || (m_ptr->r_idx == MON_KAKU))) return FALSE;

	/* Don't affect already death monsters */
	/* Prevents problems with chain reactions of exploding monsters */
	if (m_ptr->hp < 0) return FALSE;

	dam = (dam + r) / (r + 1);

	/* Get the monster name (BEFORE polymorphing) */
	monster_desc(caster_ptr, m_name, m_ptr, 0);

	/* Get the monster possessive ("his"/"her"/"its") */
	monster_desc(caster_ptr, m_poss, m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE);

	if (caster_ptr->riding && (g_ptr->m_idx == caster_ptr->riding)) disturb(caster_ptr, TRUE, TRUE);

	if (r_ptr->flagsr & RFR_RES_ALL &&
		typ != GF_OLD_CLONE && typ != GF_STAR_HEAL && typ != GF_OLD_HEAL
		&& typ != GF_OLD_SPEED && typ != GF_CAPTURE && typ != GF_PHOTO)
	{
		note = _("には完全な耐性がある！", " is immune.");
		dam = 0;
		if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= (RFR_RES_ALL);
		if (typ == GF_LITE_WEAK || typ == GF_KILL_WALL) skipped = TRUE;
	}
	else
	{
		switch (typ)
		{
		case GF_MISSILE:
		{
			if (seen) obvious = TRUE;
			break;
		}
		case GF_ACID:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flagsr & RFR_IM_ACID)
			{
				note = _("にはかなり耐性がある！", " resists a lot.");
				dam /= 9;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= (RFR_IM_ACID);
			}
			break;
		}
		case GF_ELEC:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flagsr & RFR_IM_ELEC)
			{
				note = _("にはかなり耐性がある！", " resists a lot.");
				dam /= 9;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= (RFR_IM_ELEC);
			}
			break;
		}
		case GF_FIRE:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flagsr & RFR_IM_FIRE)
			{
				note = _("にはかなり耐性がある！", " resists a lot.");
				dam /= 9;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= (RFR_IM_FIRE);
			}
			else if (r_ptr->flags3 & (RF3_HURT_FIRE))
			{
				note = _("はひどい痛手をうけた。", " is hit hard.");
				dam *= 2;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_HURT_FIRE);
			}
			break;
		}
		case GF_COLD:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flagsr & RFR_IM_COLD)
			{
				note = _("にはかなり耐性がある！", " resists a lot.");
				dam /= 9;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= (RFR_IM_COLD);
			}
			else if (r_ptr->flags3 & (RF3_HURT_COLD))
			{
				note = _("はひどい痛手をうけた。", " is hit hard.");
				dam *= 2;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_HURT_COLD);
			}
			break;
		}
		case GF_POIS:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flagsr & RFR_IM_POIS)
			{
				note = _("にはかなり耐性がある！", " resists a lot.");
				dam /= 9;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= (RFR_IM_POIS);
			}
			break;
		}
		case GF_NUKE:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flagsr & RFR_IM_POIS)
			{
				note = _("には耐性がある。", " resists.");
				dam *= 3; dam /= randint1(6) + 6;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= (RFR_IM_POIS);
			}
			else if (one_in_(3)) do_poly = TRUE;
			break;
		}
		case GF_HELL_FIRE:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & RF3_GOOD)
			{
				note = _("はひどい痛手をうけた。", " is hit hard.");
				dam *= 2;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_GOOD);
			}
			break;
		}
		case GF_HOLY_FIRE:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & RF3_EVIL)
			{
				dam *= 2;
				note = _("はひどい痛手をうけた。", " is hit hard.");
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= RF3_EVIL;
			}
			else
			{
				note = _("には耐性がある。", " resists.");
				dam *= 3; dam /= randint1(6) + 6;
			}
			break;
		}
		case GF_ARROW:
		{
			if (seen) obvious = TRUE;
			break;
		}
		case GF_PLASMA:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flagsr & RFR_RES_PLAS)
			{
				note = _("には耐性がある。", " resists.");
				dam *= 3; dam /= randint1(6) + 6;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= (RFR_RES_PLAS);
			}

			break;
		}
		case GF_NETHER:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flagsr & RFR_RES_NETH)
			{
				if (r_ptr->flags3 & RF3_UNDEAD)
				{
					note = _("には完全な耐性がある！", " is immune.");
					dam = 0;
					if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_UNDEAD);
				}
				else
				{
					note = _("には耐性がある。", " resists.");
					dam *= 3; dam /= randint1(6) + 6;
				}
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= (RFR_RES_NETH);
			}
			else if (r_ptr->flags3 & RF3_EVIL)
			{
				note = _("はいくらか耐性を示した。", " resists somewhat.");
				dam /= 2;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_EVIL);
			}

			break;
		}
		case GF_WATER:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flagsr & RFR_RES_WATE)
			{
				if ((m_ptr->r_idx == MON_WATER_ELEM) || (m_ptr->r_idx == MON_UNMAKER))
				{
					note = _("には完全な耐性がある！", " is immune.");
					dam = 0;
				}
				else
				{
					note = _("には耐性がある。", " resists.");
					dam *= 3; dam /= randint1(6) + 6;
				}
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= (RFR_RES_WATE);
			}

			break;
		}
		case GF_CHAOS:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flagsr & RFR_RES_CHAO)
			{
				note = _("には耐性がある。", " resists.");
				dam *= 3; dam /= randint1(6) + 6;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= (RFR_RES_CHAO);
			}
			else if ((r_ptr->flags3 & RF3_DEMON) && one_in_(3))
			{
				note = _("はいくらか耐性を示した。", " resists somewhat.");
				dam *= 3; dam /= randint1(6) + 6;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_DEMON);
			}
			else
			{
				do_poly = TRUE;
				do_conf = (5 + randint1(11) + r) / (r + 1);
			}

			break;
		}
		case GF_SHARDS:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flagsr & RFR_RES_SHAR)
			{
				note = _("には耐性がある。", " resists.");
				dam *= 3; dam /= randint1(6) + 6;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= (RFR_RES_SHAR);
			}

			break;
		}
		case GF_ROCKET:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flagsr & RFR_RES_SHAR)
			{
				note = _("はいくらか耐性を示した。", " resists somewhat.");
				dam /= 2;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= (RFR_RES_SHAR);
			}

			break;
		}
		case GF_SOUND:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flagsr & RFR_RES_SOUN)
			{
				note = _("には耐性がある。", " resists.");
				dam *= 2; dam /= randint1(6) + 6;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= (RFR_RES_SOUN);
			}
			else
				do_stun = (10 + randint1(15) + r) / (r + 1);

			break;
		}
		case GF_CONFUSION:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & RF3_NO_CONF)
			{
				note = _("には耐性がある。", " resists.");
				dam *= 3; dam /= randint1(6) + 6;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_NO_CONF);
			}
			else
				do_conf = (10 + randint1(15) + r) / (r + 1);

			break;
		}
		case GF_DISENCHANT:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flagsr & RFR_RES_DISE)
			{
				note = _("には耐性がある。", " resists.");
				dam *= 3; dam /= randint1(6) + 6;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= (RFR_RES_DISE);
			}

			break;
		}
		case GF_NEXUS:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flagsr & RFR_RES_NEXU)
			{
				note = _("には耐性がある。", " resists.");
				dam *= 3; dam /= randint1(6) + 6;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= (RFR_RES_NEXU);
			}

			break;
		}
		case GF_FORCE:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flagsr & RFR_RES_WALL)
			{
				note = _("には耐性がある。", " resists.");
				dam *= 3; dam /= randint1(6) + 6;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= (RFR_RES_WALL);
			}
			else
				do_stun = (randint1(15) + r) / (r + 1);

			break;
		}
		case GF_INERTIAL:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flagsr & RFR_RES_INER)
			{
				note = _("には耐性がある。", " resists.");
				dam *= 3; dam /= randint1(6) + 6;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= (RFR_RES_INER);
			}
			else
			{
				/* Powerful monsters can resist */
				if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
					(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
				{
					obvious = FALSE;
				}
				/* Normal monsters slow down */
				else
				{
					if (set_monster_slow(caster_ptr, g_ptr->m_idx, MON_SLOW(m_ptr) + 50))
					{
						note = _("の動きが遅くなった。", " starts moving slower.");
					}
				}
			}

			break;
		}
		case GF_TIME:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flagsr & RFR_RES_TIME)
			{
				note = _("には耐性がある。", " resists.");
				dam *= 3; dam /= randint1(6) + 6;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= (RFR_RES_TIME);
			}
			else
				do_time = (dam + 1) / 2;

			break;
		}
		case GF_GRAVITY:
		{
			bool resist_tele = FALSE;

			if (seen) obvious = TRUE;
			if (r_ptr->flagsr & RFR_RES_TELE)
			{
				if (r_ptr->flags1 & (RF1_UNIQUE))
				{
					if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= RFR_RES_TELE;
					note = _("には効果がなかった。", " is unaffected!");
					resist_tele = TRUE;
				}
				else if (r_ptr->level > randint1(100))
				{
					if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= RFR_RES_TELE;
					note = _("には耐性がある！", " resists!");
					resist_tele = TRUE;
				}
			}

			if (!resist_tele) do_dist = 10;
			else do_dist = 0;

			if (caster_ptr->riding && (g_ptr->m_idx == caster_ptr->riding)) do_dist = 0;

			if (r_ptr->flagsr & RFR_RES_GRAV)
			{
				note = _("には耐性がある！", " resists!");
				dam *= 3; dam /= randint1(6) + 6;
				do_dist = 0;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= (RFR_RES_GRAV);
			}
			else
			{
				/* 1. slowness */
				/* Powerful monsters can resist */
				if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
					(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
				{
					obvious = FALSE;
				}
				/* Normal monsters slow down */
				else
				{
					if (set_monster_slow(caster_ptr, g_ptr->m_idx, MON_SLOW(m_ptr) + 50))
					{
						note = _("の動きが遅くなった。", " starts moving slower.");
					}
				}

				/* 2. stun */
				do_stun = damroll((caster_lev / 20) + 3, (dam)) + 1;

				/* Attempt a saving throw */
				if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
					(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
				{
					/* Resist */
					do_stun = 0;
					/* No obvious effect */
					note = _("には効果がなかった。", " is unaffected!");
					obvious = FALSE;
				}
			}

			break;
		}
		case GF_MANA:
		case GF_SEEKER:
		case GF_SUPER_RAY:
		{
			if (seen) obvious = TRUE;
			break;
		}
		case GF_DISINTEGRATE:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags3 & RF3_HURT_ROCK)
			{
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_HURT_ROCK);
				note = _("の皮膚がただれた！", " loses some skin!");
				note_dies = _("は蒸発した！", " evaporates!");
				dam *= 2;
			}

			break;
		}
		case GF_PSI:
		{
			if (seen) obvious = TRUE;
			if (!(los(caster_ptr, m_ptr->fy, m_ptr->fx, caster_ptr->y, caster_ptr->x)))
			{
				if (seen_msg)
					msg_format(_("%sはあなたが見えないので影響されない！", "%^s can't see you, and isn't affected!"), m_name);
				skipped = TRUE;
				break;
			}

			if (r_ptr->flags2 & RF2_EMPTY_MIND)
			{
				dam = 0;
				note = _("には完全な耐性がある！", " is immune.");
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags2 |= (RF2_EMPTY_MIND);

			}
			else if ((r_ptr->flags2 & (RF2_STUPID | RF2_WEIRD_MIND)) ||
				(r_ptr->flags3 & RF3_ANIMAL) ||
				(r_ptr->level > randint1(3 * dam)))
			{
				note = _("には耐性がある！", " resists!");
				dam /= 3;

				/*
				 * Powerful demons & undead can turn a mindcrafter's
				 * attacks back on them
				 */
				if ((r_ptr->flags3 & (RF3_UNDEAD | RF3_DEMON)) &&
					(r_ptr->level > caster_ptr->lev / 2) &&
					one_in_(2))
				{
					note = NULL;
					msg_format(_("%^sの堕落した精神は攻撃を跳ね返した！",
						(seen ? "%^s's corrupted mind backlashes your attack!" :
							"%^ss corrupted mind backlashes your attack!")), m_name);

					if ((randint0(100 + r_ptr->level / 2) < caster_ptr->skill_sav) && !CHECK_MULTISHADOW(caster_ptr))
					{
						msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
					}
					else
					{
						/* Injure +/- confusion */
						monster_desc(caster_ptr, killer, m_ptr, MD_WRONGDOER_NAME);
						take_hit(caster_ptr, DAMAGE_ATTACK, dam, killer, -1);  /* has already been /3 */
						if (one_in_(4) && !CHECK_MULTISHADOW(caster_ptr))
						{
							switch (randint1(4))
							{
							case 1:
								set_confused(caster_ptr, caster_ptr->confused + 3 + randint1(dam));
								break;
							case 2:
								set_stun(caster_ptr, caster_ptr->stun + randint1(dam));
								break;
							case 3:
							{
								if (r_ptr->flags3 & RF3_NO_FEAR)
									note = _("には効果がなかった。", " is unaffected.");
								else
									set_afraid(caster_ptr, caster_ptr->afraid + 3 + randint1(dam));
								break;
							}
							default:
								if (!caster_ptr->free_act)
									(void)set_paralyzed(caster_ptr, caster_ptr->paralyzed + randint1(dam));
								break;
							}
						}
					}

					dam = 0;
				}
			}

			if ((dam > 0) && one_in_(4))
			{
				switch (randint1(4))
				{
				case 1:
					do_conf = 3 + randint1(dam);
					break;
				case 2:
					do_stun = 3 + randint1(dam);
					break;
				case 3:
					do_fear = 3 + randint1(dam);
					break;
				default:
					note = _("は眠り込んでしまった！", " falls asleep!");
					do_sleep = 3 + randint1(dam);
					break;
				}
			}

			note_dies = _("の精神は崩壊し、肉体は抜け殻となった。", " collapses, a mindless husk.");
			break;
		}
		case GF_PSI_DRAIN:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags2 & RF2_EMPTY_MIND)
			{
				dam = 0;
				note = _("には完全な耐性がある！", " is immune.");
			}
			else if ((r_ptr->flags2 & (RF2_STUPID | RF2_WEIRD_MIND)) ||
				(r_ptr->flags3 & RF3_ANIMAL) ||
				(r_ptr->level > randint1(3 * dam)))
			{
				note = _("には耐性がある！", " resists!");
				dam /= 3;

				/*
				 * Powerful demons & undead can turn a mindcrafter's
				 * attacks back on them
				 */
				if ((r_ptr->flags3 & (RF3_UNDEAD | RF3_DEMON)) &&
					(r_ptr->level > caster_ptr->lev / 2) &&
					(one_in_(2)))
				{
					note = NULL;
					msg_format(_("%^sの堕落した精神は攻撃を跳ね返した！",
						(seen ? "%^s's corrupted mind backlashes your attack!" :
							"%^ss corrupted mind backlashes your attack!")), m_name);
					if ((randint0(100 + r_ptr->level / 2) < caster_ptr->skill_sav) && !CHECK_MULTISHADOW(caster_ptr))
					{
						msg_print(_("あなたは効力を跳ね返した！", "You resist the effects!"));
					}
					else
					{
						monster_desc(caster_ptr, killer, m_ptr, MD_WRONGDOER_NAME);
						if (!CHECK_MULTISHADOW(caster_ptr))
						{
							msg_print(_("超能力パワーを吸いとられた！", "Your psychic energy is drained!"));
							caster_ptr->csp -= damroll(5, dam) / 2;
							if (caster_ptr->csp < 0) caster_ptr->csp = 0;
							caster_ptr->redraw |= PR_MANA;
							caster_ptr->window |= (PW_SPELL);
						}
						take_hit(caster_ptr, DAMAGE_ATTACK, dam, killer, -1);  /* has already been /3 */
					}

					dam = 0;
				}
			}
			else if (dam > 0)
			{
				int b = damroll(5, dam) / 4;
				concptr str = (caster_ptr->pclass == CLASS_MINDCRAFTER) ? _("超能力パワー", "psychic energy") : _("魔力", "mana");
				concptr msg = _("あなたは%sの苦痛を%sに変換した！",
					(seen ? "You convert %s's pain into %s!" :
						"You convert %ss pain into %s!"));
				msg_format(msg, m_name, str);

				b = MIN(caster_ptr->msp, caster_ptr->csp + b);
				caster_ptr->csp = b;
				caster_ptr->redraw |= PR_MANA;
				caster_ptr->window |= (PW_SPELL);
			}

			note_dies = _("の精神は崩壊し、肉体は抜け殻となった。", " collapses, a mindless husk.");
			break;
		}
		case GF_TELEKINESIS:
		{
			if (seen) obvious = TRUE;
			if (one_in_(4))
			{
				if (caster_ptr->riding && (g_ptr->m_idx == caster_ptr->riding)) do_dist = 0;
				else do_dist = 7;
			}

			do_stun = damroll((caster_lev / 20) + 3, dam) + 1;
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
				(r_ptr->level > 5 + randint1(dam)))
			{
				do_stun = 0;
				obvious = FALSE;
			}

			break;
		}
		case GF_PSY_SPEAR:
		{
			if (seen) obvious = TRUE;
			break;
		}
		case GF_METEOR:
		{
			if (seen) obvious = TRUE;
			break;
		}
		case GF_DOMINATION:
		{
			if (!is_hostile(m_ptr)) break;
			if (seen) obvious = TRUE;
			if ((r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) ||
				(r_ptr->flags3 & RF3_NO_CONF) ||
				(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				if (r_ptr->flags3 & RF3_NO_CONF)
				{
					if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_NO_CONF);
				}

				do_conf = 0;

				/*
				 * Powerful demons & undead can turn a mindcrafter's
				 * attacks back on them
				 */
				if ((r_ptr->flags3 & (RF3_UNDEAD | RF3_DEMON)) &&
					(r_ptr->level > caster_ptr->lev / 2) &&
					(one_in_(2)))
				{
					note = NULL;
					msg_format(_("%^sの堕落した精神は攻撃を跳ね返した！",
						(seen ? "%^s's corrupted mind backlashes your attack!" :
							"%^ss corrupted mind backlashes your attack!")), m_name);

					/* Saving throw */
					if (randint0(100 + r_ptr->level / 2) < caster_ptr->skill_sav)
					{
						msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
					}
					else
					{
						/* Confuse, stun, terrify */
						switch (randint1(4))
						{
						case 1:
							set_stun(caster_ptr, caster_ptr->stun + dam / 2);
							break;
						case 2:
							set_confused(caster_ptr, caster_ptr->confused + dam / 2);
							break;
						default:
						{
							if (r_ptr->flags3 & RF3_NO_FEAR)
								note = _("には効果がなかった。", " is unaffected.");
							else
								set_afraid(caster_ptr, caster_ptr->afraid + dam);
						}
						}
					}
				}
				else
				{
					note = _("には効果がなかった。", " is unaffected.");
					obvious = FALSE;
				}
			}
			else
			{
				if (!common_saving_throw_charm(caster_ptr, dam, m_ptr))
				{
					note = _("があなたに隷属した。", " is in your thrall!");
					set_pet(caster_ptr, m_ptr);
				}
				else
				{
					switch (randint1(4))
					{
					case 1:
						do_stun = dam / 2;
						break;
					case 2:
						do_conf = dam / 2;
						break;
					default:
						do_fear = dam;
					}
				}
			}

			dam = 0;
			break;
		}
		case GF_ICE:
		{
			if (seen) obvious = TRUE;
			do_stun = (randint1(15) + 1) / (r + 1);
			if (r_ptr->flagsr & RFR_IM_COLD)
			{
				note = _("にはかなり耐性がある！", " resists a lot.");
				dam /= 9;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= (RFR_IM_COLD);
			}
			else if (r_ptr->flags3 & (RF3_HURT_COLD))
			{
				note = _("はひどい痛手をうけた。", " is hit hard.");
				dam *= 2;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_HURT_COLD);
			}

			break;
		}
		case GF_HYPODYNAMIA:
		{
			if (seen) obvious = TRUE;
			if (!monster_living(m_ptr->r_idx))
			{
				if (is_original_ap_and_seen(caster_ptr, m_ptr))
				{
					if (r_ptr->flags3 & RF3_DEMON) r_ptr->r_flags3 |= (RF3_DEMON);
					if (r_ptr->flags3 & RF3_UNDEAD) r_ptr->r_flags3 |= (RF3_UNDEAD);
					if (r_ptr->flags3 & RF3_NONLIVING) r_ptr->r_flags3 |= (RF3_NONLIVING);
				}
				note = _("には効果がなかった。", " is unaffected.");
				obvious = FALSE;
				dam = 0;
			}
			else
				do_time = (dam + 7) / 8;

			break;
		}
		case GF_DEATH_RAY:
		{
			if (seen) obvious = TRUE;
			if (!monster_living(m_ptr->r_idx))
			{
				if (is_original_ap_and_seen(caster_ptr, m_ptr))
				{
					if (r_ptr->flags3 & RF3_DEMON) r_ptr->r_flags3 |= (RF3_DEMON);
					if (r_ptr->flags3 & RF3_UNDEAD) r_ptr->r_flags3 |= (RF3_UNDEAD);
					if (r_ptr->flags3 & RF3_NONLIVING) r_ptr->r_flags3 |= (RF3_NONLIVING);
				}
				note = _("には完全な耐性がある！", " is immune.");
				obvious = FALSE;
				dam = 0;
			}
			else if (((r_ptr->flags1 & RF1_UNIQUE) &&
				(randint1(888) != 666)) ||
				(((r_ptr->level + randint1(20)) > randint1((caster_lev / 2) + randint1(10))) &&
					randint1(100) != 66))
			{
				note = _("には耐性がある！", " resists!");
				obvious = FALSE;
				dam = 0;
			}

			break;
		}
		case GF_OLD_POLY:
		{
			if (seen) obvious = TRUE;
			do_poly = TRUE;

			/* Powerful monsters can resist */
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
				(r_ptr->flags1 & RF1_QUESTOR) ||
				(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				note = _("には効果がなかった。", " is unaffected.");
				do_poly = FALSE;
				obvious = FALSE;
			}

			dam = 0;
			break;
		}
		case GF_OLD_CLONE:
		{
			if (seen) obvious = TRUE;

			if ((floor_ptr->inside_arena) || is_pet(m_ptr) || (r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) || (r_ptr->flags7 & (RF7_NAZGUL | RF7_UNIQUE2)))
			{
				note = _("には効果がなかった。", " is unaffected.");
			}
			else
			{
				m_ptr->hp = m_ptr->maxhp;
				if (multiply_monster(caster_ptr, g_ptr->m_idx, TRUE, 0L))
				{
					note = _("が分裂した！", " spawns!");
				}
			}

			dam = 0;
			break;
		}
		case GF_STAR_HEAL:
		{
			if (seen) obvious = TRUE;

			(void)set_monster_csleep(caster_ptr, g_ptr->m_idx, 0);

			if (m_ptr->maxhp < m_ptr->max_maxhp)
			{
				if (seen_msg) msg_format(_("%^sの強さが戻った。", "%^s recovers %s vitality."), m_name, m_poss);
				m_ptr->maxhp = m_ptr->max_maxhp;
			}

			if (!dam)
			{
				if (caster_ptr->health_who == g_ptr->m_idx) caster_ptr->redraw |= (PR_HEALTH);
				if (caster_ptr->riding == g_ptr->m_idx) caster_ptr->redraw |= (PR_UHEALTH);
				break;
			}
		}
			/* Fall through */
		case GF_OLD_HEAL:
		{
			if (seen) obvious = TRUE;

			/* Wake up */
			(void)set_monster_csleep(caster_ptr, g_ptr->m_idx, 0);
			if (MON_STUNNED(m_ptr))
			{
				if (seen_msg) msg_format(_("%^sは朦朧状態から立ち直った。", "%^s is no longer stunned."), m_name);
				(void)set_monster_stunned(caster_ptr, g_ptr->m_idx, 0);
			}
			if (MON_CONFUSED(m_ptr))
			{
				if (seen_msg) msg_format(_("%^sは混乱から立ち直った。", "%^s is no longer confused."), m_name);
				(void)set_monster_confused(caster_ptr, g_ptr->m_idx, 0);
			}
			if (MON_MONFEAR(m_ptr))
			{
				if (seen_msg) msg_format(_("%^sは勇気を取り戻した。", "%^s recovers %s courage."), m_name, m_poss);
				(void)set_monster_monfear(caster_ptr, g_ptr->m_idx, 0);
			}

			if (m_ptr->hp < 30000) m_ptr->hp += dam;
			if (m_ptr->hp > m_ptr->maxhp) m_ptr->hp = m_ptr->maxhp;

			if (!who)
			{
				chg_virtue(caster_ptr, V_VITALITY, 1);

				if (r_ptr->flags1 & RF1_UNIQUE)
					chg_virtue(caster_ptr, V_INDIVIDUALISM, 1);

				if (is_friendly(m_ptr))
					chg_virtue(caster_ptr, V_HONOUR, 1);
				else if (!(r_ptr->flags3 & RF3_EVIL))
				{
					if (r_ptr->flags3 & RF3_GOOD)
						chg_virtue(caster_ptr, V_COMPASSION, 2);
					else
						chg_virtue(caster_ptr, V_COMPASSION, 1);
				}

				if (r_ptr->flags3 & RF3_ANIMAL)
					chg_virtue(caster_ptr, V_NATURE, 1);
			}

			if (m_ptr->r_idx == MON_LEPER)
			{
				heal_leper = TRUE;
				if (!who) chg_virtue(caster_ptr, V_COMPASSION, 5);
			}

			if (caster_ptr->health_who == g_ptr->m_idx) caster_ptr->redraw |= (PR_HEALTH);
			if (caster_ptr->riding == g_ptr->m_idx) caster_ptr->redraw |= (PR_UHEALTH);

			note = _("は体力を回復したようだ。", " looks healthier.");

			dam = 0;
			break;
		}
		case GF_OLD_SPEED:
		{
			if (seen) obvious = TRUE;

			if (set_monster_fast(caster_ptr, g_ptr->m_idx, MON_FAST(m_ptr) + 100))
			{
				note = _("の動きが速くなった。", " starts moving faster.");
			}

			if (!who)
			{
				if (r_ptr->flags1 & RF1_UNIQUE)
					chg_virtue(caster_ptr, V_INDIVIDUALISM, 1);
				if (is_friendly(m_ptr))
					chg_virtue(caster_ptr, V_HONOUR, 1);
			}

			dam = 0;
			break;
		}
		case GF_OLD_SLOW:
		{
			if (seen) obvious = TRUE;

			/* Powerful monsters can resist */
			if ((r_ptr->flags1 & RF1_UNIQUE) ||
				(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				note = _("には効果がなかった。", " is unaffected.");
				obvious = FALSE;
			}
			else
			{
				if (set_monster_slow(caster_ptr, g_ptr->m_idx, MON_SLOW(m_ptr) + 50))
				{
					note = _("の動きが遅くなった。", " starts moving slower.");
				}
			}

			dam = 0;
			break;
		}
		case GF_OLD_SLEEP:
		{
			if (seen) obvious = TRUE;

			if ((r_ptr->flags1 & RF1_UNIQUE) ||
				(r_ptr->flags3 & RF3_NO_SLEEP) ||
				(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				if (r_ptr->flags3 & RF3_NO_SLEEP)
				{
					if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_NO_SLEEP);
				}

				note = _("には効果がなかった。", " is unaffected.");
				obvious = FALSE;
			}
			else
			{
				note = _("は眠り込んでしまった！", " falls asleep!");
				do_sleep = 500;
			}

			dam = 0;
			break;
		}
		case GF_STASIS_EVIL:
		{
			if (seen) obvious = TRUE;

			if ((r_ptr->flags1 & RF1_UNIQUE) ||
				!(r_ptr->flags3 & RF3_EVIL) ||
				(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				note = _("には効果がなかった。", " is unaffected.");
				obvious = FALSE;
			}
			else
			{
				note = _("は動けなくなった！", " is suspended!");
				do_sleep = 500;
			}

			dam = 0;
			break;
		}
		case GF_STASIS:
		{
			if (seen) obvious = TRUE;

			if ((r_ptr->flags1 & RF1_UNIQUE) ||
				(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				note = _("には効果がなかった。", " is unaffected.");
				obvious = FALSE;
			}
			else
			{
				note = _("は動けなくなった！", " is suspended!");
				do_sleep = 500;
			}

			dam = 0;
			break;
		}
		case GF_CHARM:
		{
			int vir;
			vir = virtue_number(caster_ptr, V_HARMONY);
			if (vir)
			{
				dam += caster_ptr->virtues[vir - 1] / 10;
			}

			vir = virtue_number(caster_ptr, V_INDIVIDUALISM);
			if (vir)
			{
				dam -= caster_ptr->virtues[vir - 1] / 20;
			}

			if (seen) obvious = TRUE;

			if (common_saving_throw_charm(caster_ptr, dam, m_ptr))
			{
				note = _("には効果がなかった。", " is unaffected.");
				obvious = FALSE;

				if (one_in_(4)) m_ptr->mflag2 |= MFLAG2_NOPET;
			}
			else if (caster_ptr->cursed & TRC_AGGRAVATE)
			{
				note = _("はあなたに敵意を抱いている！", " hates you too much!");
				if (one_in_(4)) m_ptr->mflag2 |= MFLAG2_NOPET;
			}
			else
			{
				note = _("は突然友好的になったようだ！", " suddenly seems friendly!");
				set_pet(caster_ptr, m_ptr);

				chg_virtue(caster_ptr, V_INDIVIDUALISM, -1);
				if (r_ptr->flags3 & RF3_ANIMAL)
					chg_virtue(caster_ptr, V_NATURE, 1);
			}

			dam = 0;
			break;
		}
		case GF_CONTROL_UNDEAD:
		{
			int vir;
			if (seen) obvious = TRUE;

			vir = virtue_number(caster_ptr, V_UNLIFE);
			if (vir)
			{
				dam += caster_ptr->virtues[vir - 1] / 10;
			}

			vir = virtue_number(caster_ptr, V_INDIVIDUALISM);
			if (vir)
			{
				dam -= caster_ptr->virtues[vir - 1] / 20;
			}

			if (common_saving_throw_control(caster_ptr, dam, m_ptr) ||
				!(r_ptr->flags3 & RF3_UNDEAD))
			{
				note = _("には効果がなかった。", " is unaffected.");
				obvious = FALSE;
				if (one_in_(4)) m_ptr->mflag2 |= MFLAG2_NOPET;
			}
			else if (caster_ptr->cursed & TRC_AGGRAVATE)
			{
				note = _("はあなたに敵意を抱いている！", " hates you too much!");
				if (one_in_(4)) m_ptr->mflag2 |= MFLAG2_NOPET;
			}
			else
			{
				note = _("は既にあなたの奴隷だ！", " is in your thrall!");
				set_pet(caster_ptr, m_ptr);
			}

			dam = 0;
			break;
		}
		case GF_CONTROL_DEMON:
		{
			int vir;
			if (seen) obvious = TRUE;

			vir = virtue_number(caster_ptr, V_UNLIFE);
			if (vir)
			{
				dam += caster_ptr->virtues[vir - 1] / 10;
			}

			vir = virtue_number(caster_ptr, V_INDIVIDUALISM);
			if (vir)
			{
				dam -= caster_ptr->virtues[vir - 1] / 20;
			}

			if (common_saving_throw_control(caster_ptr, dam, m_ptr) ||
				!(r_ptr->flags3 & RF3_DEMON))
			{
				note = _("には効果がなかった。", " is unaffected.");
				obvious = FALSE;
				if (one_in_(4)) m_ptr->mflag2 |= MFLAG2_NOPET;
			}
			else if (caster_ptr->cursed & TRC_AGGRAVATE)
			{
				note = _("はあなたに敵意を抱いている！", " hates you too much!");
				if (one_in_(4)) m_ptr->mflag2 |= MFLAG2_NOPET;
			}
			else
			{
				note = _("は既にあなたの奴隷だ！", " is in your thrall!");
				set_pet(caster_ptr, m_ptr);
			}

			dam = 0;
			break;
		}
		case GF_CONTROL_ANIMAL:
		{
			int vir;
			if (seen) obvious = TRUE;

			vir = virtue_number(caster_ptr, V_NATURE);
			if (vir)
			{
				dam += caster_ptr->virtues[vir - 1] / 10;
			}

			vir = virtue_number(caster_ptr, V_INDIVIDUALISM);
			if (vir)
			{
				dam -= caster_ptr->virtues[vir - 1] / 20;
			}

			if (common_saving_throw_control(caster_ptr, dam, m_ptr) ||
				!(r_ptr->flags3 & RF3_ANIMAL))
			{
				note = _("には効果がなかった。", " is unaffected.");
				obvious = FALSE;
				if (one_in_(4)) m_ptr->mflag2 |= MFLAG2_NOPET;
			}
			else if (caster_ptr->cursed & TRC_AGGRAVATE)
			{
				note = _("はあなたに敵意を抱いている！", " hates you too much!");
				if (one_in_(4)) m_ptr->mflag2 |= MFLAG2_NOPET;
			}
			else
			{
				note = _("はなついた。", " is tamed!");
				set_pet(caster_ptr, m_ptr);
				if (r_ptr->flags3 & RF3_ANIMAL)
					chg_virtue(caster_ptr, V_NATURE, 1);
			}

			dam = 0;
			break;
		}
		case GF_CHARM_LIVING:
		{
			int vir;

			vir = virtue_number(caster_ptr, V_UNLIFE);
			if (seen) obvious = TRUE;

			vir = virtue_number(caster_ptr, V_UNLIFE);
			if (vir)
			{
				dam -= caster_ptr->virtues[vir - 1] / 10;
			}

			vir = virtue_number(caster_ptr, V_INDIVIDUALISM);
			if (vir)
			{
				dam -= caster_ptr->virtues[vir - 1] / 20;
			}

			msg_format(_("%sを見つめた。", "You stare into %s."), m_name);

			if (common_saving_throw_charm(caster_ptr, dam, m_ptr) ||
				!monster_living(m_ptr->r_idx))
			{
				note = _("には効果がなかった。", " is unaffected.");
				obvious = FALSE;
				if (one_in_(4)) m_ptr->mflag2 |= MFLAG2_NOPET;
			}
			else if (caster_ptr->cursed & TRC_AGGRAVATE)
			{
				note = _("はあなたに敵意を抱いている！", " hates you too much!");
				if (one_in_(4)) m_ptr->mflag2 |= MFLAG2_NOPET;
			}
			else
			{
				note = _("を支配した。", " is tamed!");
				set_pet(caster_ptr, m_ptr);
				if (r_ptr->flags3 & RF3_ANIMAL)
					chg_virtue(caster_ptr, V_NATURE, 1);
			}

			dam = 0;
			break;
		}
		case GF_OLD_CONF:
		{
			if (seen) obvious = TRUE;

			do_conf = damroll(3, (dam / 2)) + 1;
			if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
				(r_ptr->flags3 & (RF3_NO_CONF)) ||
				(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				if (r_ptr->flags3 & (RF3_NO_CONF))
				{
					if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_NO_CONF);
				}

				do_conf = 0;
				note = _("には効果がなかった。", " is unaffected.");
				obvious = FALSE;
			}

			dam = 0;
			break;
		}
		case GF_STUN:
		{
			if (seen) obvious = TRUE;

			do_stun = damroll((caster_lev / 20) + 3, (dam)) + 1;
			if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
				(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				do_stun = 0;
				note = _("には効果がなかった。", " is unaffected.");
				obvious = FALSE;
			}

			dam = 0;
			break;
		}
		case GF_LITE_WEAK:
		{
			if (!dam)
			{
				skipped = TRUE;
				break;
			}

			if (r_ptr->flags3 & (RF3_HURT_LITE))
			{
				if (seen) obvious = TRUE;

				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_HURT_LITE);

				note = _("は光に身をすくめた！", " cringes from the light!");
				note_dies = _("は光を受けてしぼんでしまった！", " shrivels away in the light!");
			}
			else
			{
				dam = 0;
			}

			break;
		}
		case GF_LITE:
		{
			if (seen) obvious = TRUE;

			if (r_ptr->flagsr & RFR_RES_LITE)
			{
				note = _("には耐性がある！", " resists!");
				dam *= 2; dam /= (randint1(6) + 6);
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= (RFR_RES_LITE);
			}
			else if (r_ptr->flags3 & (RF3_HURT_LITE))
			{
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_HURT_LITE);
				note = _("は光に身をすくめた！", " cringes from the light!");
				note_dies = _("は光を受けてしぼんでしまった！", " shrivels away in the light!");
				dam *= 2;
			}
			break;
		}
		case GF_DARK:
		{
			if (seen) obvious = TRUE;

			if (r_ptr->flagsr & RFR_RES_DARK)
			{
				note = _("には耐性がある！", " resists!");
				dam *= 2; dam /= (randint1(6) + 6);
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= (RFR_RES_DARK);
			}

			break;
		}
		case GF_KILL_WALL:
		{
			if (r_ptr->flags3 & (RF3_HURT_ROCK))
			{
				if (seen) obvious = TRUE;

				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_HURT_ROCK);

				note = _("の皮膚がただれた！", " loses some skin!");
				note_dies = _("はドロドロに溶けた！", " dissolves!");
			}
			else
			{
				dam = 0;
			}

			break;
		}
		case GF_AWAY_UNDEAD:
		{
			if (r_ptr->flags3 & (RF3_UNDEAD))
			{
				bool resists_tele = FALSE;

				if (r_ptr->flagsr & RFR_RES_TELE)
				{
					if ((r_ptr->flags1 & (RF1_UNIQUE)) || (r_ptr->flagsr & RFR_RES_ALL))
					{
						if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= RFR_RES_TELE;
						note = _("には効果がなかった。", " is unaffected.");
						resists_tele = TRUE;
					}
					else if (r_ptr->level > randint1(100))
					{
						if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= RFR_RES_TELE;
						note = _("には耐性がある！", " resists!");
						resists_tele = TRUE;
					}
				}

				if (!resists_tele)
				{
					if (seen) obvious = TRUE;
					if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_UNDEAD);
					do_dist = dam;
				}
			}
			else
			{
				skipped = TRUE;
			}

			dam = 0;
			break;
		}
		case GF_AWAY_EVIL:
		{
			if (r_ptr->flags3 & (RF3_EVIL))
			{
				bool resists_tele = FALSE;

				if (r_ptr->flagsr & RFR_RES_TELE)
				{
					if ((r_ptr->flags1 & (RF1_UNIQUE)) || (r_ptr->flagsr & RFR_RES_ALL))
					{
						if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= RFR_RES_TELE;
						note = _("には効果がなかった。", " is unaffected.");
						resists_tele = TRUE;
					}
					else if (r_ptr->level > randint1(100))
					{
						if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= RFR_RES_TELE;
						note = _("には耐性がある！", " resists!");
						resists_tele = TRUE;
					}
				}

				if (!resists_tele)
				{
					if (seen) obvious = TRUE;
					if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_EVIL);
					do_dist = dam;
				}
			}
			else
			{
				skipped = TRUE;
			}

			dam = 0;
			break;
		}
		case GF_AWAY_ALL:
		{
			bool resists_tele = FALSE;
			if (r_ptr->flagsr & RFR_RES_TELE)
			{
				if ((r_ptr->flags1 & (RF1_UNIQUE)) || (r_ptr->flagsr & RFR_RES_ALL))
				{
					if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= RFR_RES_TELE;
					note = _("には効果がなかった。", " is unaffected.");
					resists_tele = TRUE;
				}
				else if (r_ptr->level > randint1(100))
				{
					if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flagsr |= RFR_RES_TELE;
					note = _("には耐性がある！", " resists!");
					resists_tele = TRUE;
				}
			}

			if (!resists_tele)
			{
				if (seen) obvious = TRUE;

				do_dist = dam;
			}

			dam = 0;
			break;
		}
		case GF_TURN_UNDEAD:
		{
			if (r_ptr->flags3 & (RF3_UNDEAD))
			{
				if (seen) obvious = TRUE;

				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_UNDEAD);

				do_fear = damroll(3, (dam / 2)) + 1;
				if (r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10)
				{
					note = _("には効果がなかった。", " is unaffected.");
					obvious = FALSE;
					do_fear = 0;
				}
			}
			else
			{
				skipped = TRUE;
			}

			dam = 0;
			break;
		}
		case GF_TURN_EVIL:
		{
			if (r_ptr->flags3 & (RF3_EVIL))
			{
				if (seen) obvious = TRUE;

				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_EVIL);

				do_fear = damroll(3, (dam / 2)) + 1;
				if (r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10)
				{
					note = _("には効果がなかった。", " is unaffected.");
					obvious = FALSE;
					do_fear = 0;
				}
			}
			else
			{
				skipped = TRUE;
			}

			dam = 0;
			break;
		}
		case GF_TURN_ALL:
		{
			if (seen) obvious = TRUE;

			do_fear = damroll(3, (dam / 2)) + 1;
			if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
				(r_ptr->flags3 & (RF3_NO_FEAR)) ||
				(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
			{
				note = _("には効果がなかった。", " is unaffected.");
				obvious = FALSE;
				do_fear = 0;
			}

			dam = 0;
			break;
		}
		case GF_DISP_UNDEAD:
		{
			if (r_ptr->flags3 & (RF3_UNDEAD))
			{
				if (seen) obvious = TRUE;

				/* Learn about type */
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_UNDEAD);

				note = _("は身震いした。", " shudders.");
				note_dies = _("はドロドロに溶けた！", " dissolves!");
			}
			else
			{
				skipped = TRUE;
				dam = 0;
			}

			break;
		}
		case GF_DISP_EVIL:
		{
			if (r_ptr->flags3 & (RF3_EVIL))
			{
				if (seen) obvious = TRUE;

				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_EVIL);

				note = _("は身震いした。", " shudders.");
				note_dies = _("はドロドロに溶けた！", " dissolves!");
			}
			else
			{
				skipped = TRUE;
				dam = 0;
			}

			break;
		}
		case GF_DISP_GOOD:
		{
			if (r_ptr->flags3 & (RF3_GOOD))
			{
				if (seen) obvious = TRUE;

				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_GOOD);

				note = _("は身震いした。", " shudders.");
				note_dies = _("はドロドロに溶けた！", " dissolves!");
			}
			else
			{
				skipped = TRUE;
				dam = 0;
			}

			break;
		}
		case GF_DISP_LIVING:
		{
			if (monster_living(m_ptr->r_idx))
			{
				if (seen) obvious = TRUE;

				note = _("は身震いした。", " shudders.");
				note_dies = _("はドロドロに溶けた！", " dissolves!");
			}
			else
			{
				skipped = TRUE;
				dam = 0;
			}

			break;
		}
		case GF_DISP_DEMON:
		{
			if (r_ptr->flags3 & (RF3_DEMON))
			{
				if (seen) obvious = TRUE;

				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_DEMON);

				note = _("は身震いした。", " shudders.");
				note_dies = _("はドロドロに溶けた！", " dissolves!");
			}
			else
			{
				skipped = TRUE;
				dam = 0;
			}

			break;
		}
		case GF_DISP_ALL:
		{
			if (seen) obvious = TRUE;
			note = _("は身震いした。", " shudders.");
			note_dies = _("はドロドロに溶けた！", " dissolves!");
			break;
		}
		case GF_DRAIN_MANA:
		{
			if (seen) obvious = TRUE;
			if ((r_ptr->flags4 & ~(RF4_NOMAGIC_MASK)) || (r_ptr->a_ability_flags1 & ~(RF5_NOMAGIC_MASK)) || (r_ptr->a_ability_flags2 & ~(RF6_NOMAGIC_MASK)))
			{
				if (who > 0)
				{
					if (m_caster_ptr->hp < m_caster_ptr->maxhp)
					{
						m_caster_ptr->hp += dam;
						if (m_caster_ptr->hp > m_caster_ptr->maxhp) m_caster_ptr->hp = m_caster_ptr->maxhp;
						if (caster_ptr->health_who == who) caster_ptr->redraw |= (PR_HEALTH);
						if (caster_ptr->riding == who) caster_ptr->redraw |= (PR_UHEALTH);

						if (see_s_msg)
						{
							monster_desc(caster_ptr, killer, m_caster_ptr, 0);
							msg_format(_("%^sは気分が良さそうだ。", "%^s appears healthier."), killer);
						}
					}
				}
				else
				{
					msg_format(_("%sから精神エネルギーを吸いとった。", "You draw psychic energy from %s."), m_name);
					(void)hp_player(caster_ptr, dam);
				}
			}
			else
			{
				if (see_s_msg) msg_format(_("%sには効果がなかった。", "%s is unaffected."), m_name);
			}

			dam = 0;
			break;
		}
		case GF_MIND_BLAST:
		{
			if (seen) obvious = TRUE;
			if (!who) msg_format(_("%sをじっと睨んだ。", "You gaze intently at %s."), m_name);

			if ((r_ptr->flags1 & RF1_UNIQUE) ||
				(r_ptr->flags3 & RF3_NO_CONF) ||
				(r_ptr->level > randint1((caster_lev - 10) < 1 ? 1 : (caster_lev - 10)) + 10))
			{
				if (r_ptr->flags3 & (RF3_NO_CONF))
				{
					if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_NO_CONF);
				}

				note = _("には効果がなかった。", " is unaffected.");
				dam = 0;
			}
			else if (r_ptr->flags2 & RF2_EMPTY_MIND)
			{
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags2 |= (RF2_EMPTY_MIND);
				note = _("には完全な耐性がある！", " is immune.");
				dam = 0;
			}
			else if (r_ptr->flags2 & RF2_WEIRD_MIND)
			{
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags2 |= (RF2_WEIRD_MIND);
				note = _("には耐性がある。", " resists.");
				dam /= 3;
			}
			else
			{
				note = _("は精神攻撃を食らった。", " is blasted by psionic energy.");
				note_dies = _("の精神は崩壊し、肉体は抜け殻となった。", " collapses, a mindless husk.");

				if (who > 0) do_conf = randint0(4) + 4;
				else do_conf = randint0(8) + 8;
			}

			break;
		}
		case GF_BRAIN_SMASH:
		{
			if (seen) obvious = TRUE;
			if (!who) msg_format(_("%sをじっと睨んだ。", "You gaze intently at %s."), m_name);

			if ((r_ptr->flags1 & RF1_UNIQUE) ||
				(r_ptr->flags3 & RF3_NO_CONF) ||
				(r_ptr->level > randint1((caster_lev - 10) < 1 ? 1 : (caster_lev - 10)) + 10))
			{
				if (r_ptr->flags3 & (RF3_NO_CONF))
				{
					if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_NO_CONF);
				}

				note = _("には効果がなかった。", " is unaffected.");
				dam = 0;
			}
			else if (r_ptr->flags2 & RF2_EMPTY_MIND)
			{
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags2 |= (RF2_EMPTY_MIND);
				note = _("には完全な耐性がある！", " is immune.");
				dam = 0;
			}
			else if (r_ptr->flags2 & RF2_WEIRD_MIND)
			{
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags2 |= (RF2_WEIRD_MIND);
				note = _("には耐性がある！", " resists!");
				dam /= 3;
			}
			else
			{
				note = _("は精神攻撃を食らった。", " is blasted by psionic energy.");
				note_dies = _("の精神は崩壊し、肉体は抜け殻となった。", " collapses, a mindless husk.");
				if (who > 0)
				{
					do_conf = randint0(4) + 4;
					do_stun = randint0(4) + 4;
				}
				else
				{
					do_conf = randint0(8) + 8;
					do_stun = randint0(8) + 8;
				}
				(void)set_monster_slow(caster_ptr, g_ptr->m_idx, MON_SLOW(m_ptr) + 10);
			}

			break;
		}
		case GF_CAUSE_1:
		{
			if (seen) obvious = TRUE;
			if (!who) msg_format(_("%sを指差して呪いをかけた。", "You point at %s and curse."), m_name);
			if (randint0(100 + (caster_lev / 2)) < (r_ptr->level + 35))
			{
				note = _("には効果がなかった。", " is unaffected.");
				dam = 0;
			}

			break;
		}
		case GF_CAUSE_2:
		{
			if (seen) obvious = TRUE;
			if (!who) msg_format(_("%sを指差して恐ろしげに呪いをかけた。", "You point at %s and curse horribly."), m_name);

			if (randint0(100 + (caster_lev / 2)) < (r_ptr->level + 35))
			{
				note = _("には効果がなかった。", " is unaffected.");
				dam = 0;
			}

			break;
		}
		case GF_CAUSE_3:
		{
			if (seen) obvious = TRUE;
			if (!who) msg_format(_("%sを指差し、恐ろしげに呪文を唱えた！", "You point at %s, incanting terribly!"), m_name);

			if (randint0(100 + (caster_lev / 2)) < (r_ptr->level + 35))
			{
				note = _("には効果がなかった。", " is unaffected.");
				dam = 0;
			}

			break;
		}
		case GF_CAUSE_4:
		{
			if (seen) obvious = TRUE;
			if (!who)
				msg_format(_("%sの秘孔を突いて、「お前は既に死んでいる」と叫んだ。",
					"You point at %s, screaming the word, 'DIE!'."), m_name);

			if ((randint0(100 + (caster_lev / 2)) < (r_ptr->level + 35)) && ((who <= 0) || (m_caster_ptr->r_idx != MON_KENSHIROU)))
			{
				note = _("には効果がなかった。", " is unaffected.");
				dam = 0;
			}
			break;
		}
		case GF_HAND_DOOM:
		{
			if (seen) obvious = TRUE;
			if (r_ptr->flags1 & RF1_UNIQUE)
			{
				note = _("には効果がなかった。", " is unaffected.");
				dam = 0;
			}
			else
			{
				if ((who > 0) ? ((caster_lev + randint1(dam)) > (r_ptr->level + 10 + randint1(20))) :
					(((caster_lev / 2) + randint1(dam)) > (r_ptr->level + randint1(200))))
				{
					dam = ((40 + randint1(20)) * m_ptr->hp) / 100;

					if (m_ptr->hp < dam) dam = m_ptr->hp - 1;
				}
				else
				{
					/* todo 乱数で破滅のを弾いた結果が「耐性を持っている」ことになるのはおかしい */
					note = _("は耐性を持っている！", "resists!");
					dam = 0;
				}
			}

			break;
		}
		case GF_CAPTURE:
		{
			int nokori_hp;
			if ((floor_ptr->inside_quest && (quest[floor_ptr->inside_quest].type == QUEST_TYPE_KILL_ALL) && !is_pet(m_ptr)) ||
				(r_ptr->flags1 & (RF1_UNIQUE)) || (r_ptr->flags7 & (RF7_NAZGUL)) || (r_ptr->flags7 & (RF7_UNIQUE2)) || (r_ptr->flags1 & RF1_QUESTOR) || m_ptr->parent_m_idx)
			{
				msg_format(_("%sには効果がなかった。", "%s is unaffected."), m_name);
				skipped = TRUE;
				break;
			}

			if (is_pet(m_ptr)) nokori_hp = m_ptr->maxhp * 4L;
			else if ((caster_ptr->pclass == CLASS_BEASTMASTER) && monster_living(m_ptr->r_idx))
				nokori_hp = m_ptr->maxhp * 3 / 10;
			else
				nokori_hp = m_ptr->maxhp * 3 / 20;

			if (m_ptr->hp >= nokori_hp)
			{
				msg_format(_("もっと弱らせないと。", "You need to weaken %s more."), m_name);
				skipped = TRUE;
			}
			else if (m_ptr->hp < randint0(nokori_hp))
			{
				if (m_ptr->mflag2 & MFLAG2_CHAMELEON) choose_new_monster(caster_ptr, g_ptr->m_idx, FALSE, MON_CHAMELEON);
				msg_format(_("%sを捕えた！", "You capture %^s!"), m_name);
				cap_mon = m_ptr->r_idx;
				cap_mspeed = m_ptr->mspeed;
				cap_hp = m_ptr->hp;
				cap_maxhp = m_ptr->max_maxhp;
				cap_nickname = m_ptr->nickname;
				if (g_ptr->m_idx == caster_ptr->riding)
				{
					if (rakuba(caster_ptr, -1, FALSE))
					{
						msg_format(_("地面に落とされた。", "You have fallen from %s."), m_name);
					}
				}

				delete_monster_idx(caster_ptr, g_ptr->m_idx);

				return TRUE;
			}
			else
			{
				msg_format(_("うまく捕まえられなかった。", "You failed to capture %s."), m_name);
				skipped = TRUE;
			}

			break;
		}
		case GF_ATTACK:
		{
			return py_attack(caster_ptr, y, x, dam);
		}
		case GF_ENGETSU:
		{
			int effect = 0;
			bool done = TRUE;

			if (seen) obvious = TRUE;
			if (r_ptr->flags2 & RF2_EMPTY_MIND)
			{
				note = _("には効果がなかった。", " is unaffected.");
				dam = 0;
				skipped = TRUE;
				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags2 |= (RF2_EMPTY_MIND);
				break;
			}
			if (MON_CSLEEP(m_ptr))
			{
				note = _("には効果がなかった。", " is unaffected.");
				dam = 0;
				skipped = TRUE;
				break;
			}

			if (one_in_(5)) effect = 1;
			else if (one_in_(4)) effect = 2;
			else if (one_in_(3)) effect = 3;
			else done = FALSE;

			if (effect == 1)
			{
				if ((r_ptr->flags1 & RF1_UNIQUE) ||
					(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
				{
					note = _("には効果がなかった。", " is unaffected.");
					obvious = FALSE;
				}
				else
				{
					if (set_monster_slow(caster_ptr, g_ptr->m_idx, MON_SLOW(m_ptr) + 50))
					{
						note = _("の動きが遅くなった。", " starts moving slower.");
					}
				}
			}
			else if (effect == 2)
			{
				do_stun = damroll((caster_ptr->lev / 10) + 3, (dam)) + 1;
				if ((r_ptr->flags1 & (RF1_UNIQUE)) ||
					(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
				{
					do_stun = 0;
					note = _("には効果がなかった。", " is unaffected.");
					obvious = FALSE;
				}
			}
			else if (effect == 3)
			{
				if ((r_ptr->flags1 & RF1_UNIQUE) ||
					(r_ptr->flags3 & RF3_NO_SLEEP) ||
					(r_ptr->level > randint1((dam - 10) < 1 ? 1 : (dam - 10)) + 10))
				{
					if (r_ptr->flags3 & RF3_NO_SLEEP)
					{
						if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_NO_SLEEP);
					}

					note = _("には効果がなかった。", " is unaffected.");
					obvious = FALSE;
				}
				else
				{
					/* Go to sleep (much) later */
					note = _("は眠り込んでしまった！", " falls asleep!");
					do_sleep = 500;
				}
			}

			if (!done)
			{
				note = _("には効果がなかった。", " is unaffected.");
			}

			dam = 0;
			break;
		}
		case GF_GENOCIDE:
		{
			if (seen) obvious = TRUE;
			if (genocide_aux(caster_ptr, g_ptr->m_idx, dam, !who, (r_ptr->level + 1) / 2, _("モンスター消滅", "Genocide One")))
			{
				if (seen_msg) msg_format(_("%sは消滅した！", "%^s disappeared!"), m_name);
				chg_virtue(caster_ptr, V_VITALITY, -1);
				return TRUE;
			}

			skipped = TRUE;
			break;
		}
		case GF_PHOTO:
		{
			if (!who)
				msg_format(_("%sを写真に撮った。", "You take a photograph of %s."), m_name);

			if (r_ptr->flags3 & (RF3_HURT_LITE))
			{
				if (seen) obvious = TRUE;

				if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_HURT_LITE);

				note = _("は光に身をすくめた！", " cringes from the light!");
				note_dies = _("は光を受けてしぼんでしまった！", " shrivels away in the light!");
			}
			else
			{
				dam = 0;
			}

			photo = m_ptr->r_idx;
			break;
		}
		case GF_BLOOD_CURSE:
		{
			if (seen) obvious = TRUE;
			break;
		}
		case GF_CRUSADE:
		{
			bool success = FALSE;
			if (seen) obvious = TRUE;

			if ((r_ptr->flags3 & (RF3_GOOD)) && !floor_ptr->inside_arena)
			{
				if (r_ptr->flags3 & (RF3_NO_CONF)) dam -= 50;
				if (dam < 1) dam = 1;

				if (is_pet(m_ptr))
				{
					note = _("の動きが速くなった。", " starts moving faster.");
					(void)set_monster_fast(caster_ptr, g_ptr->m_idx, MON_FAST(m_ptr) + 100);
					success = TRUE;
				}
				else if ((r_ptr->flags1 & (RF1_QUESTOR)) ||
					(r_ptr->flags1 & (RF1_UNIQUE)) ||
					(m_ptr->mflag2 & MFLAG2_NOPET) ||
					(caster_ptr->cursed & TRC_AGGRAVATE) ||
					((r_ptr->level + 10) > randint1(dam)))
				{
					if (one_in_(4)) m_ptr->mflag2 |= MFLAG2_NOPET;
				}
				else
				{
					note = _("を支配した。", " is tamed!");
					set_pet(caster_ptr, m_ptr);
					(void)set_monster_fast(caster_ptr, g_ptr->m_idx, MON_FAST(m_ptr) + 100);

					if (is_original_ap_and_seen(caster_ptr, m_ptr)) r_ptr->r_flags3 |= (RF3_GOOD);
					success = TRUE;
				}
			}

			if (!success)
			{
				if (!(r_ptr->flags3 & RF3_NO_FEAR))
				{
					do_fear = randint1(90) + 10;
				}
				else if (is_original_ap_and_seen(caster_ptr, m_ptr))
					r_ptr->r_flags3 |= (RF3_NO_FEAR);
			}

			dam = 0;
			break;
		}
		case GF_WOUNDS:
		{
			if (seen) obvious = TRUE;

			if (randint0(100 + dam) < (r_ptr->level + 50))
			{
				note = _("には効果がなかった。", " is unaffected.");
				dam = 0;
			}
			break;
		}
		default:
		{
			skipped = TRUE;
			dam = 0;
			break;
		}
		}
	}

	if (skipped) return FALSE;

	if (r_ptr->flags1 & (RF1_UNIQUE)) do_poly = FALSE;
	if (r_ptr->flags1 & RF1_QUESTOR) do_poly = FALSE;
	if (caster_ptr->riding && (g_ptr->m_idx == caster_ptr->riding))
		do_poly = FALSE;

	if (((r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) || (r_ptr->flags7 & RF7_NAZGUL)) && !caster_ptr->phase_out)
	{
		if (who && (dam > m_ptr->hp)) dam = m_ptr->hp;
	}

	if (!who && slept)
	{
		if (!(r_ptr->flags3 & RF3_EVIL) || one_in_(5)) chg_virtue(caster_ptr, V_COMPASSION, -1);
		if (!(r_ptr->flags3 & RF3_EVIL) || one_in_(5)) chg_virtue(caster_ptr, V_HONOUR, -1);
	}

	int tmp = dam;
	dam = mon_damage_mod(caster_ptr, m_ptr, dam, (bool)(typ == GF_PSY_SPEAR));
	if ((tmp > 0) && (dam == 0)) note = _("はダメージを受けていない。", " is unharmed.");

	if (dam > m_ptr->hp)
	{
		note = note_dies;
	}
	else
	{
		if (do_stun &&
			!(r_ptr->flagsr & (RFR_RES_SOUN | RFR_RES_WALL)) &&
			!(r_ptr->flags3 & RF3_NO_STUN))
		{
			if (seen) obvious = TRUE;

			if (MON_STUNNED(m_ptr))
			{
				note = _("はひどくもうろうとした。", " is more dazed.");
				tmp = MON_STUNNED(m_ptr) + (do_stun / 2);
			}
			else
			{
				note = _("はもうろうとした。", " is dazed.");
				tmp = do_stun;
			}

			(void)set_monster_stunned(caster_ptr, g_ptr->m_idx, tmp);
			get_angry = TRUE;
		}

		if (do_conf &&
			!(r_ptr->flags3 & RF3_NO_CONF) &&
			!(r_ptr->flagsr & RFR_EFF_RES_CHAO_MASK))
		{
			if (seen) obvious = TRUE;

			if (MON_CONFUSED(m_ptr))
			{
				note = _("はさらに混乱したようだ。", " looks more confused.");
				tmp = MON_CONFUSED(m_ptr) + (do_conf / 2);
			}
			else
			{
				note = _("は混乱したようだ。", " looks confused.");
				tmp = do_conf;
			}

			(void)set_monster_confused(caster_ptr, g_ptr->m_idx, tmp);
			get_angry = TRUE;
		}

		if (do_time)
		{
			if (seen) obvious = TRUE;

			if (do_time >= m_ptr->maxhp) do_time = m_ptr->maxhp - 1;

			if (do_time)
			{
				note = _("は弱くなったようだ。", " seems weakened.");
				m_ptr->maxhp -= do_time;
				if ((m_ptr->hp - dam) > m_ptr->maxhp) dam = m_ptr->hp - m_ptr->maxhp;
			}

			get_angry = TRUE;
		}

		if (do_poly && (randint1(90) > r_ptr->level))
		{
			if (polymorph_monster(caster_ptr, y, x))
			{
				if (seen) obvious = TRUE;

				note = _("が変身した！", " changes!");
				dam = 0;
			}

			m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
			r_ptr = &r_info[m_ptr->r_idx];
		}

		if (do_dist)
		{
			if (seen) obvious = TRUE;

			note = _("が消え去った！", " disappears!");

			if (!who) chg_virtue(caster_ptr, V_VALOUR, -1);

			teleport_away(caster_ptr, g_ptr->m_idx, do_dist,
				(!who ? TELEPORT_DEC_VALOUR : 0L) | TELEPORT_PASSIVE);

			y = m_ptr->fy;
			x = m_ptr->fx;
			g_ptr = &floor_ptr->grid_array[y][x];
		}

		if (do_fear)
		{
			(void)set_monster_monfear(caster_ptr, g_ptr->m_idx, MON_MONFEAR(m_ptr) + do_fear);
			get_angry = TRUE;
		}
	}

	if (typ == GF_DRAIN_MANA)
	{
		/* Drain mana does nothing */
	}

	/* If another monster did the damage, hurt the monster by hand */
	else if (who)
	{
		if (caster_ptr->health_who == g_ptr->m_idx) caster_ptr->redraw |= (PR_HEALTH);
		if (caster_ptr->riding == g_ptr->m_idx) caster_ptr->redraw |= (PR_UHEALTH);

		(void)set_monster_csleep(caster_ptr, g_ptr->m_idx, 0);
		m_ptr->hp -= dam;
		if (m_ptr->hp < 0)
		{
			bool sad = FALSE;

			if (is_pet(m_ptr) && !(m_ptr->ml))
				sad = TRUE;

			if (known && note)
			{
				monster_desc(caster_ptr, m_name, m_ptr, MD_TRUE_NAME);
				if (see_s_msg)
				{
					msg_format("%^s%s", m_name, note);
				}
				else
				{
					floor_ptr->monster_noise = TRUE;
				}
			}

			if (who > 0) monster_gain_exp(caster_ptr, who, m_ptr->r_idx);

			monster_death(caster_ptr, g_ptr->m_idx, FALSE);
			delete_monster_idx(caster_ptr, g_ptr->m_idx);
			if (sad)
			{
				msg_print(_("少し悲しい気分がした。", "You feel sad for a moment."));
			}
		}
		else
		{
			if (note && seen_msg)
				msg_format("%^s%s", m_name, note);
			else if (see_s_msg)
			{
				message_pain(caster_ptr, g_ptr->m_idx, dam);
			}
			else
			{
				floor_ptr->monster_noise = TRUE;
			}

			if (do_sleep) (void)set_monster_csleep(caster_ptr, g_ptr->m_idx, do_sleep);
		}
	}
	else if (heal_leper)
	{
		if (seen_msg)
			msg_print(_("不潔な病人は病気が治った！", "The Mangy looking leper is healed!"));

		if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname)
		{
			char m2_name[MAX_NLEN];

			monster_desc(caster_ptr, m2_name, m_ptr, MD_INDEF_VISIBLE);
			exe_write_diary(caster_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_HEAL_LEPER, m2_name);
		}

		delete_monster_idx(caster_ptr, g_ptr->m_idx);
	}

	/* If the player did it, give him experience, check fear */
	else
	{
		bool fear = FALSE;
		if (mon_take_hit(caster_ptr, g_ptr->m_idx, dam, &fear, note_dies))
		{
			/* Dead monster */
		}
		else
		{
			if (do_sleep) anger_monster(caster_ptr, m_ptr);

			if (note && seen_msg)
				msg_format(_("%s%s", "%^s%s"), m_name, note);
			else if (known && (dam || !do_fear))
			{
				message_pain(caster_ptr, g_ptr->m_idx, dam);
			}

			if (((dam > 0) || get_angry) && !do_sleep)
				anger_monster(caster_ptr, m_ptr);

			if ((fear || do_fear) && seen)
			{
				sound(SOUND_FLEE);
				msg_format(_("%^sは恐怖して逃げ出した！", "%^s flees in terror!"), m_name);
			}

			if (do_sleep) (void)set_monster_csleep(caster_ptr, g_ptr->m_idx, do_sleep);
		}
	}

	if ((typ == GF_BLOOD_CURSE) && one_in_(4))
	{
		blood_curse_to_enemy(caster_ptr, who);
	}

	if (caster_ptr->phase_out)
	{
		caster_ptr->health_who = g_ptr->m_idx;
		caster_ptr->redraw |= (PR_HEALTH);
		handle_stuff(caster_ptr);
	}

	if (m_ptr->r_idx) update_monster(caster_ptr, g_ptr->m_idx, FALSE);

	lite_spot(caster_ptr, y, x);
	if ((caster_ptr->monster_race_idx == m_ptr->r_idx) && (seen || !m_ptr->r_idx))
	{
		caster_ptr->window |= (PW_MONSTER);
	}

	if ((dam > 0) && !is_pet(m_ptr) && !is_friendly(m_ptr))
	{
		if (!who)
		{
			if (!(flag & PROJECT_NO_HANGEKI))
			{
				set_target(m_ptr, monster_target_y, monster_target_x);
			}
		}
		else if ((who > 0) && is_pet(m_caster_ptr) && !player_bold(caster_ptr, m_ptr->target_y, m_ptr->target_x))
		{
			set_target(m_ptr, m_caster_ptr->fy, m_caster_ptr->fx);
		}
	}

	if (caster_ptr->riding && (caster_ptr->riding == g_ptr->m_idx) && (dam > 0))
	{
		if (m_ptr->hp > m_ptr->maxhp / 3) dam = (dam + 1) / 2;
		rakubadam_m = (dam > 200) ? 200 : dam;
	}

	if (photo)
	{
		object_type *q_ptr;
		object_type forge;
		q_ptr = &forge;
		object_prep(q_ptr, lookup_kind(TV_STATUE, SV_PHOTO));
		q_ptr->pval = photo;
		q_ptr->ident |= (IDENT_FULL_KNOWN);
		(void)drop_near(caster_ptr, q_ptr, -1, caster_ptr->y, caster_ptr->x);
	}

	project_m_n++;
	project_m_x = x;
	project_m_y = y;
	return (obvious);
}


/*
 * Find the distance from (x, y) to a line.
 */
POSITION dist_to_line(POSITION y, POSITION x, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
	POSITION py = y1 - y;
	POSITION px = x1 - x;
	POSITION ny = x2 - x1;
	POSITION nx = y1 - y2;
	POSITION pd = distance(y1, x1, y, x);
	POSITION nd = distance(y1, x1, y2, x2);

	if (pd > nd) return distance(y, x, y2, x2);

	nd = ((nd) ? ((py * ny + px * nx) / nd) : 0);
	return((nd >= 0) ? nd : 0 - nd);
}


/*
 *
 * Modified version of los() for calculation of disintegration balls.
 * Disintegration effects are stopped by permanent walls.
 */
bool in_disintegration_range(floor_type *floor_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
	POSITION delta_y = y2 - y1;
	POSITION delta_x = x2 - x1;
	POSITION absolute_y = ABS(delta_y);
	POSITION absolute_x = ABS(delta_x);
	if ((absolute_x < 2) && (absolute_y < 2)) return TRUE;

	POSITION scanner_y;
	if (!delta_x)
	{
		/* South -- check for walls */
		if (delta_y > 0)
		{
			for (scanner_y = y1 + 1; scanner_y < y2; scanner_y++)
			{
				if (cave_stop_disintegration(floor_ptr, scanner_y, x1)) return FALSE;
			}
		}

		/* North -- check for walls */
		else
		{
			for (scanner_y = y1 - 1; scanner_y > y2; scanner_y--)
			{
				if (cave_stop_disintegration(floor_ptr, scanner_y, x1)) return FALSE;
			}
		}

		return TRUE;
	}

	/* Directly East/West */
	POSITION scanner_x;
	if (!delta_y)
	{
		/* East -- check for walls */
		if (delta_x > 0)
		{
			for (scanner_x = x1 + 1; scanner_x < x2; scanner_x++)
			{
				if (cave_stop_disintegration(floor_ptr, y1, scanner_x)) return FALSE;
			}
		}

		/* West -- check for walls */
		else
		{
			for (scanner_x = x1 - 1; scanner_x > x2; scanner_x--)
			{
				if (cave_stop_disintegration(floor_ptr, y1, scanner_x)) return FALSE;
			}
		}

		return TRUE;
	}

	POSITION sign_x = (delta_x < 0) ? -1 : 1;
	POSITION sign_y = (delta_y < 0) ? -1 : 1;
	if (absolute_x == 1)
	{
		if (absolute_y == 2)
		{
			if (!cave_stop_disintegration(floor_ptr, y1 + sign_y, x1)) return TRUE;
		}
	}
	else if (absolute_y == 1)
	{
		if (absolute_x == 2)
		{
			if (!cave_stop_disintegration(floor_ptr, y1, x1 + sign_x)) return TRUE;
		}
	}

	POSITION scale_factor_2 = (absolute_x * absolute_y);
	POSITION scale_factor_1 = scale_factor_2 << 1;
	POSITION fraction_y;
	POSITION m; /* Slope, or 1/Slope, of LOS */
	if (absolute_x >= absolute_y)
	{
		fraction_y = absolute_y * absolute_y;
		m = fraction_y << 1;
		scanner_x = x1 + sign_x;
		if (fraction_y == scale_factor_2)
		{
			scanner_y = y1 + sign_y;
			fraction_y -= scale_factor_1;
		}
		else
		{
			scanner_y = y1;
		}

		/* Note (below) the case (qy == f2), where */
		/* the LOS exactly meets the corner of a tile. */
		while (x2 - scanner_x)
		{
			if (cave_stop_disintegration(floor_ptr, scanner_y, scanner_x)) return FALSE;

			fraction_y += m;

			if (fraction_y < scale_factor_2)
			{
				scanner_x += sign_x;
			}
			else if (fraction_y > scale_factor_2)
			{
				scanner_y += sign_y;
				if (cave_stop_disintegration(floor_ptr, scanner_y, scanner_x)) return FALSE;
				fraction_y -= scale_factor_1;
				scanner_x += sign_x;
			}
			else
			{
				scanner_y += sign_y;
				fraction_y -= scale_factor_1;
				scanner_x += sign_x;
			}
		}

		return TRUE;
	}

	POSITION fraction_x = absolute_x * absolute_x;
	m = fraction_x << 1;
	scanner_y = y1 + sign_y;
	if (fraction_x == scale_factor_2)
	{
		scanner_x = x1 + sign_x;
		fraction_x -= scale_factor_1;
	}
	else
	{
		scanner_x = x1;
	}

	/* Note (below) the case (qx == f2), where */
	/* the LOS exactly meets the corner of a tile. */
	while (y2 - scanner_y)
	{
		if (cave_stop_disintegration(floor_ptr, scanner_y, scanner_x)) return FALSE;

		fraction_x += m;

		if (fraction_x < scale_factor_2)
		{
			scanner_y += sign_y;
		}
		else if (fraction_x > scale_factor_2)
		{
			scanner_x += sign_x;
			if (cave_stop_disintegration(floor_ptr, scanner_y, scanner_x)) return FALSE;
			fraction_x -= scale_factor_1;
			scanner_y += sign_y;
		}
		else
		{
			scanner_x += sign_x;
			fraction_x -= scale_factor_1;
			scanner_y += sign_y;
		}
	}

	return TRUE;
}


/*
 * breath shape
 */
void breath_shape(player_type *caster_ptr, u16b *path_g, int dist, int *pgrids, POSITION *gx, POSITION *gy, POSITION *gm, POSITION *pgm_rad, POSITION rad, POSITION y1, POSITION x1, POSITION y2, POSITION x2, EFFECT_ID typ)
{
	POSITION by = y1;
	POSITION bx = x1;
	int brad = 0;
	int brev = rad * rad / dist;
	int bdis = 0;
	int cdis;
	int path_n = 0;
	int mdis = distance(y1, x1, y2, x2) + rad;

	floor_type *floor_ptr = caster_ptr->current_floor_ptr;
	while (bdis <= mdis)
	{
		POSITION x, y;

		if ((0 < dist) && (path_n < dist))
		{
			POSITION ny = GRID_Y(path_g[path_n]);
			POSITION nx = GRID_X(path_g[path_n]);
			POSITION nd = distance(ny, nx, y1, x1);

			if (bdis >= nd)
			{
				by = ny;
				bx = nx;
				path_n++;
			}
		}

		/* Travel from center outward */
		for (cdis = 0; cdis <= brad; cdis++)
		{
			for (y = by - cdis; y <= by + cdis; y++)
			{
				for (x = bx - cdis; x <= bx + cdis; x++)
				{
					if (!in_bounds(floor_ptr, y, x)) continue;
					if (distance(y1, x1, y, x) != bdis) continue;
					if (distance(by, bx, y, x) != cdis) continue;

					switch (typ)
					{
					case GF_LITE:
					case GF_LITE_WEAK:
						/* Lights are stopped by opaque terrains */
						if (!los(caster_ptr, by, bx, y, x)) continue;
						break;
					case GF_DISINTEGRATE:
						/* Disintegration are stopped only by perma-walls */
						if (!in_disintegration_range(floor_ptr, by, bx, y, x)) continue;
						break;
					default:
						/* Ball explosions are stopped by walls */
						if (!projectable(caster_ptr, by, bx, y, x)) continue;
						break;
					}

					gy[*pgrids] = y;
					gx[*pgrids] = x;
					(*pgrids)++;
				}
			}
		}

		gm[bdis + 1] = *pgrids;
		brad = rad * (path_n + brev) / (dist + brev);
		bdis++;
	}

	*pgm_rad = bdis;
}


/*!
 * todo 似たような処理が山ほど並んでいる、何とかならないものか
 * @brief 汎用的なビーム/ボルト/ボール系処理のルーチン Generic "beam"/"bolt"/"ball" projection routine.
 * @param who 魔法を発動したモンスター(0ならばプレイヤー) / Index of "source" monster (zero for "player")
 * @param rad 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標Y座標 / Target y location (or location to travel "towards")
 * @param x 目標X座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param typ 効果属性 / Type of damage to apply to monsters (and objects)
 * @param flag 効果フラグ / Extra bit flags (see PROJECT_xxxx)
 * @param monspell 効果元のモンスター魔法ID
 * @return 何か一つでも効力があればTRUEを返す / TRUE if any "effects" of the projection were observed, else FALSE
 */
bool project(player_type *caster_ptr, MONSTER_IDX who, POSITION rad, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID typ, BIT_FLAGS flag, int monspell)
{
	int i, t, dist;
	POSITION y1, x1;
	POSITION y2, x2;
	POSITION by, bx;
	int dist_hack = 0;
	POSITION y_saver, x_saver; /* For reflecting monsters */
	int msec = delay_factor * delay_factor * delay_factor;
	bool notice = FALSE;
	bool visual = FALSE;
	bool drawn = FALSE;
	bool breath = FALSE;
	bool blind = caster_ptr->blind != 0;
	bool old_hide = FALSE;
	int path_n = 0;
	u16b path_g[512];
	int grids = 0;
	POSITION gx[1024], gy[1024];
	POSITION gm[32];
	POSITION gm_rad = rad;
	bool jump = FALSE;
	GAME_TEXT who_name[MAX_NLEN];
	bool see_s_msg = TRUE;
	who_name[0] = '\0';
	rakubadam_p = 0;
	rakubadam_m = 0;
	monster_target_y = caster_ptr->y;
	monster_target_x = caster_ptr->x;

	if (flag & (PROJECT_JUMP))
	{
		x1 = x;
		y1 = y;
		flag &= ~(PROJECT_JUMP);
		jump = TRUE;
	}
	else if (who <= 0)
	{
		x1 = caster_ptr->x;
		y1 = caster_ptr->y;
	}
	else if (who > 0)
	{
		x1 = caster_ptr->current_floor_ptr->m_list[who].fx;
		y1 = caster_ptr->current_floor_ptr->m_list[who].fy;
		monster_desc(caster_ptr, who_name, &caster_ptr->current_floor_ptr->m_list[who], MD_WRONGDOER_NAME);
	}
	else
	{
		x1 = x;
		y1 = y;
	}

	y_saver = y1;
	x_saver = x1;
	y2 = y;
	x2 = x;

	if (flag & (PROJECT_THRU))
	{
		if ((x1 == x2) && (y1 == y2))
		{
			flag &= ~(PROJECT_THRU);
		}
	}

	if (rad < 0)
	{
		rad = 0 - rad;
		breath = TRUE;
		if (flag & PROJECT_HIDE) old_hide = TRUE;
		flag |= PROJECT_HIDE;
	}

	for (dist = 0; dist < 32; dist++) gm[dist] = 0;

	y = y1;
	x = x1;
	dist = 0;
	if (flag & (PROJECT_BEAM))
	{
		gy[grids] = y;
		gx[grids] = x;
		grids++;
	}

	switch (typ)
	{
	case GF_LITE:
	case GF_LITE_WEAK:
		if (breath || (flag & PROJECT_BEAM)) flag |= (PROJECT_LOS);
		break;
	case GF_DISINTEGRATE:
		flag |= (PROJECT_GRID);
		if (breath || (flag & PROJECT_BEAM)) flag |= (PROJECT_DISI);
		break;
	}

	/* Calculate the projection path */
	path_n = project_path(caster_ptr, path_g, (project_length ? project_length : MAX_RANGE), y1, x1, y2, x2, flag);
	handle_stuff(caster_ptr);

	if (typ == GF_SEEKER)
	{
		int j;
		int last_i = 0;
		project_m_n = 0;
		project_m_x = 0;
		project_m_y = 0;
		for (i = 0; i < path_n; ++i)
		{
			POSITION oy = y;
			POSITION ox = x;
			POSITION ny = GRID_Y(path_g[i]);
			POSITION nx = GRID_X(path_g[i]);
			y = ny;
			x = nx;
			gy[grids] = y;
			gx[grids] = x;
			grids++;

			if (!blind && !(flag & (PROJECT_HIDE)))
			{
				if (panel_contains(y, x) && player_has_los_bold(caster_ptr, y, x))
				{
					u16b p = bolt_pict(oy, ox, y, x, typ);
					TERM_COLOR a = PICT_A(p);
					SYMBOL_CODE c = PICT_C(p);
					print_rel(caster_ptr, c, a, y, x);
					move_cursor_relative(y, x);
					Term_fresh();
					Term_xtra(TERM_XTRA_DELAY, msec);
					lite_spot(caster_ptr, y, x);
					Term_fresh();
					if (flag & (PROJECT_BEAM))
					{
						p = bolt_pict(y, x, y, x, typ);
						a = PICT_A(p);
						c = PICT_C(p);
						print_rel(caster_ptr, c, a, y, x);
					}

					visual = TRUE;
				}
				else if (visual)
				{
					Term_xtra(TERM_XTRA_DELAY, msec);
				}
			}

			if (affect_item(caster_ptr, 0, 0, y, x, dam, GF_SEEKER))notice = TRUE;
			if (!is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[y][x]))
				continue;

			monster_target_y = y;
			monster_target_x = x;
			remove_mirror(caster_ptr, y, x);
			next_mirror(caster_ptr, &oy, &ox, y, x);
			path_n = i + project_path(caster_ptr, &(path_g[i + 1]), (project_length ? project_length : MAX_RANGE), y, x, oy, ox, flag);
			for (j = last_i; j <= i; j++)
			{
				y = GRID_Y(path_g[j]);
				x = GRID_X(path_g[j]);
				if (project_m(caster_ptr, 0, 0, y, x, dam, GF_SEEKER, flag, TRUE)) notice = TRUE;
				if (!who && (project_m_n == 1) && !jump && (caster_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx > 0))
				{
					monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[caster_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx];
					if (m_ptr->ml)
					{
						if (!caster_ptr->image) monster_race_track(caster_ptr, m_ptr->ap_r_idx);
						health_track(caster_ptr, caster_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx);
					}
				}

				(void)affect_feature(caster_ptr, 0, 0, y, x, dam, GF_SEEKER);
			}

			last_i = i;
		}

		for (i = last_i; i < path_n; i++)
		{
			POSITION py, px;
			py = GRID_Y(path_g[i]);
			px = GRID_X(path_g[i]);
			if (project_m(caster_ptr, 0, 0, py, px, dam, GF_SEEKER, flag, TRUE))
				notice = TRUE;
			if (!who && (project_m_n == 1) && !jump) {
				if (caster_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx > 0)
				{
					monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[caster_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx];

					if (m_ptr->ml)
					{
						if (!caster_ptr->image) monster_race_track(caster_ptr, m_ptr->ap_r_idx);
						health_track(caster_ptr, caster_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx);
					}
				}
			}

			(void)affect_feature(caster_ptr, 0, 0, py, px, dam, GF_SEEKER);
		}

		return notice;
	}
	else if (typ == GF_SUPER_RAY)
	{
		int j;
		int second_step = 0;
		project_m_n = 0;
		project_m_x = 0;
		project_m_y = 0;
		for (i = 0; i < path_n; ++i)
		{
			POSITION oy = y;
			POSITION ox = x;
			POSITION ny = GRID_Y(path_g[i]);
			POSITION nx = GRID_X(path_g[i]);
			y = ny;
			x = nx;
			gy[grids] = y;
			gx[grids] = x;
			grids++;
			{
				if (panel_contains(y, x) && player_has_los_bold(caster_ptr, y, x))
				{
					u16b p;
					TERM_COLOR a;
					SYMBOL_CODE c;
					p = bolt_pict(oy, ox, y, x, typ);
					a = PICT_A(p);
					c = PICT_C(p);
					print_rel(caster_ptr, c, a, y, x);
					move_cursor_relative(y, x);
					Term_fresh();
					Term_xtra(TERM_XTRA_DELAY, msec);
					lite_spot(caster_ptr, y, x);
					Term_fresh();
					if (flag & (PROJECT_BEAM))
					{
						p = bolt_pict(y, x, y, x, typ);
						a = PICT_A(p);
						c = PICT_C(p);
						print_rel(caster_ptr, c, a, y, x);
					}

					visual = TRUE;
				}
				else if (visual)
				{
					Term_xtra(TERM_XTRA_DELAY, msec);
				}
			}

			if (affect_item(caster_ptr, 0, 0, y, x, dam, GF_SUPER_RAY))notice = TRUE;
			if (!cave_have_flag_bold(caster_ptr->current_floor_ptr, y, x, FF_PROJECT))
			{
				if (second_step)continue;
				break;
			}

			if (is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[y][x]) && !second_step)
			{
				monster_target_y = y;
				monster_target_x = x;
				remove_mirror(caster_ptr, y, x);
				for (j = 0; j <= i; j++)
				{
					y = GRID_Y(path_g[j]);
					x = GRID_X(path_g[j]);
					(void)affect_feature(caster_ptr, 0, 0, y, x, dam, GF_SUPER_RAY);
				}

				path_n = i;
				second_step = i + 1;
				path_n += project_path(caster_ptr, &(path_g[path_n + 1]), (project_length ? project_length : MAX_RANGE), y, x, y - 1, x - 1, flag);
				path_n += project_path(caster_ptr, &(path_g[path_n + 1]), (project_length ? project_length : MAX_RANGE), y, x, y - 1, x, flag);
				path_n += project_path(caster_ptr, &(path_g[path_n + 1]), (project_length ? project_length : MAX_RANGE), y, x, y - 1, x + 1, flag);
				path_n += project_path(caster_ptr, &(path_g[path_n + 1]), (project_length ? project_length : MAX_RANGE), y, x, y, x - 1, flag);
				path_n += project_path(caster_ptr, &(path_g[path_n + 1]), (project_length ? project_length : MAX_RANGE), y, x, y, x + 1, flag);
				path_n += project_path(caster_ptr, &(path_g[path_n + 1]), (project_length ? project_length : MAX_RANGE), y, x, y + 1, x - 1, flag);
				path_n += project_path(caster_ptr, &(path_g[path_n + 1]), (project_length ? project_length : MAX_RANGE), y, x, y + 1, x, flag);
				path_n += project_path(caster_ptr, &(path_g[path_n + 1]), (project_length ? project_length : MAX_RANGE), y, x, y + 1, x + 1, flag);
			}
		}

		for (i = 0; i < path_n; i++)
		{
			POSITION py = GRID_Y(path_g[i]);
			POSITION px = GRID_X(path_g[i]);
			(void)project_m(caster_ptr, 0, 0, py, px, dam, GF_SUPER_RAY, flag, TRUE);
			if (!who && (project_m_n == 1) && !jump) {
				if (caster_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx > 0) {
					monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[caster_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx];

					if (m_ptr->ml)
					{
						if (!caster_ptr->image) monster_race_track(caster_ptr, m_ptr->ap_r_idx);
						health_track(caster_ptr, caster_ptr->current_floor_ptr->grid_array[project_m_y][project_m_x].m_idx);
					}
				}
			}

			(void)affect_feature(caster_ptr, 0, 0, py, px, dam, GF_SUPER_RAY);
		}

		return notice;
	}

	for (i = 0; i < path_n; ++i)
	{
		POSITION oy = y;
		POSITION ox = x;
		POSITION ny = GRID_Y(path_g[i]);
		POSITION nx = GRID_X(path_g[i]);
		if (flag & PROJECT_DISI)
		{
			if (cave_stop_disintegration(caster_ptr->current_floor_ptr, ny, nx) && (rad > 0)) break;
		}
		else if (flag & PROJECT_LOS)
		{
			if (!cave_los_bold(caster_ptr->current_floor_ptr, ny, nx) && (rad > 0)) break;
		}
		else
		{
			if (!cave_have_flag_bold(caster_ptr->current_floor_ptr, ny, nx, FF_PROJECT) && (rad > 0)) break;
		}

		y = ny;
		x = nx;
		if (flag & (PROJECT_BEAM))
		{
			gy[grids] = y;
			gx[grids] = x;
			grids++;
		}

		if (!blind && !(flag & (PROJECT_HIDE | PROJECT_FAST)))
		{
			if (panel_contains(y, x) && player_has_los_bold(caster_ptr, y, x))
			{
				u16b p;
				TERM_COLOR a;
				SYMBOL_CODE c;
				p = bolt_pict(oy, ox, y, x, typ);
				a = PICT_A(p);
				c = PICT_C(p);
				print_rel(caster_ptr, c, a, y, x);
				move_cursor_relative(y, x);
				Term_fresh();
				Term_xtra(TERM_XTRA_DELAY, msec);
				lite_spot(caster_ptr, y, x);
				Term_fresh();
				if (flag & (PROJECT_BEAM))
				{
					p = bolt_pict(y, x, y, x, typ);
					a = PICT_A(p);
					c = PICT_C(p);
					print_rel(caster_ptr, c, a, y, x);
				}

				visual = TRUE;
			}
			else if (visual)
			{
				Term_xtra(TERM_XTRA_DELAY, msec);
			}
		}
	}

	path_n = i;
	by = y;
	bx = x;
	if (breath && !path_n)
	{
		breath = FALSE;
		gm_rad = rad;
		if (!old_hide)
		{
			flag &= ~(PROJECT_HIDE);
		}
	}

	gm[0] = 0;
	gm[1] = grids;
	dist = path_n;
	dist_hack = dist;
	project_length = 0;

	/* If we found a "target", explode there */
	if (dist <= MAX_RANGE)
	{
		if ((flag & (PROJECT_BEAM)) && (grids > 0)) grids--;

		/*
		 * Create a conical breath attack
		 *
		 *       ***
		 *   ********
		 * D********@**
		 *   ********
		 *       ***
		 */
		if (breath)
		{
			flag &= ~(PROJECT_HIDE);
			breath_shape(caster_ptr, path_g, dist, &grids, gx, gy, gm, &gm_rad, rad, y1, x1, by, bx, typ);
		}
		else
		{
			for (dist = 0; dist <= rad; dist++)
			{
				for (y = by - dist; y <= by + dist; y++)
				{
					for (x = bx - dist; x <= bx + dist; x++)
					{
						if (!in_bounds2(caster_ptr->current_floor_ptr, y, x)) continue;
						if (distance(by, bx, y, x) != dist) continue;

						switch (typ)
						{
						case GF_LITE:
						case GF_LITE_WEAK:
							if (!los(caster_ptr, by, bx, y, x)) continue;
							break;
						case GF_DISINTEGRATE:
							if (!in_disintegration_range(caster_ptr->current_floor_ptr, by, bx, y, x)) continue;
							break;
						default:
							if (!projectable(caster_ptr, by, bx, y, x)) continue;
							break;
						}

						gy[grids] = y;
						gx[grids] = x;
						grids++;
					}
				}

				gm[dist + 1] = grids;
			}
		}
	}

	if (!grids) return FALSE;

	if (!blind && !(flag & (PROJECT_HIDE)))
	{
		for (t = 0; t <= gm_rad; t++)
		{
			for (i = gm[t]; i < gm[t + 1]; i++)
			{
				y = gy[i];
				x = gx[i];
				if (panel_contains(y, x) && player_has_los_bold(caster_ptr, y, x))
				{
					u16b p;
					TERM_COLOR a;
					SYMBOL_CODE c;
					drawn = TRUE;
					p = bolt_pict(y, x, y, x, typ);
					a = PICT_A(p);
					c = PICT_C(p);
					print_rel(caster_ptr, c, a, y, x);
				}
			}

			move_cursor_relative(by, bx);
			Term_fresh();
			if (visual || drawn)
			{
				Term_xtra(TERM_XTRA_DELAY, msec);
			}
		}

		if (drawn)
		{
			for (i = 0; i < grids; i++)
			{
				y = gy[i];
				x = gx[i];
				if (panel_contains(y, x) && player_has_los_bold(caster_ptr, y, x))
				{
					lite_spot(caster_ptr, y, x);
				}
			}

			move_cursor_relative(by, bx);
			Term_fresh();
		}
	}

	update_creature(caster_ptr);

	if (flag & PROJECT_KILL)
	{
		see_s_msg = (who > 0) ? is_seen(&caster_ptr->current_floor_ptr->m_list[who]) :
			(!who ? TRUE : (player_can_see_bold(caster_ptr, y1, x1) && projectable(caster_ptr, caster_ptr->y, caster_ptr->x, y1, x1)));
	}

	if (flag & (PROJECT_GRID))
	{
		dist = 0;
		for (i = 0; i < grids; i++)
		{
			if (gm[dist + 1] == i) dist++;
			y = gy[i];
			x = gx[i];
			if (breath)
			{
				int d = dist_to_line(y, x, y1, x1, by, bx);
				if (affect_feature(caster_ptr, who, d, y, x, dam, typ)) notice = TRUE;
			}
			else
			{
				if (affect_feature(caster_ptr, who, dist, y, x, dam, typ)) notice = TRUE;
			}
		}
	}

	update_creature(caster_ptr);
	if (flag & (PROJECT_ITEM))
	{
		dist = 0;
		for (i = 0; i < grids; i++)
		{
			if (gm[dist + 1] == i) dist++;

			y = gy[i];
			x = gx[i];
			if (breath)
			{
				int d = dist_to_line(y, x, y1, x1, by, bx);
				if (affect_item(caster_ptr, who, d, y, x, dam, typ)) notice = TRUE;
			}
			else
			{
				if (affect_item(caster_ptr, who, dist, y, x, dam, typ)) notice = TRUE;
			}
		}
	}

	if (flag & (PROJECT_KILL))
	{
		project_m_n = 0;
		project_m_x = 0;
		project_m_y = 0;
		dist = 0;
		for (i = 0; i < grids; i++)
		{
			int effective_dist;
			if (gm[dist + 1] == i) dist++;

			y = gy[i];
			x = gx[i];
			if (grids <= 1)
			{
				monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[caster_ptr->current_floor_ptr->grid_array[y][x].m_idx];
				monster_race *ref_ptr = &r_info[m_ptr->r_idx];
				if ((flag & PROJECT_REFLECTABLE) && caster_ptr->current_floor_ptr->grid_array[y][x].m_idx && (ref_ptr->flags2 & RF2_REFLECTING) &&
					((caster_ptr->current_floor_ptr->grid_array[y][x].m_idx != caster_ptr->riding) || !(flag & PROJECT_PLAYER)) &&
					(!who || dist_hack > 1) && !one_in_(10))
				{
					POSITION t_y, t_x;
					int max_attempts = 10;
					do
					{
						t_y = y_saver - 1 + randint1(3);
						t_x = x_saver - 1 + randint1(3);
						max_attempts--;
					} while (max_attempts && in_bounds2u(caster_ptr->current_floor_ptr, t_y, t_x) && !projectable(caster_ptr, y, x, t_y, t_x));

					if (max_attempts < 1)
					{
						t_y = y_saver;
						t_x = x_saver;
					}

					sound(SOUND_REFLECT);
					if (is_seen(m_ptr))
					{
						if ((m_ptr->r_idx == MON_KENSHIROU) || (m_ptr->r_idx == MON_RAOU))
							msg_print(_("「北斗神拳奥義・二指真空把！」", "The attack bounces!"));
						else if (m_ptr->r_idx == MON_DIO)
							msg_print(_("ディオ・ブランドーは指一本で攻撃を弾き返した！", "The attack bounces!"));
						else
							msg_print(_("攻撃は跳ね返った！", "The attack bounces!"));
					}

					if (is_original_ap_and_seen(caster_ptr, m_ptr)) ref_ptr->r_flags2 |= RF2_REFLECTING;

					if (player_bold(caster_ptr, y, x) || one_in_(2)) flag &= ~(PROJECT_PLAYER);
					else flag |= PROJECT_PLAYER;

					project(caster_ptr, caster_ptr->current_floor_ptr->grid_array[y][x].m_idx, 0, t_y, t_x, dam, typ, flag, monspell);
					continue;
				}
			}

			/* Find the closest point in the blast */
			if (breath)
			{
				effective_dist = dist_to_line(y, x, y1, x1, by, bx);
			}
			else
			{
				effective_dist = dist;
			}

			if (caster_ptr->riding && player_bold(caster_ptr, y, x))
			{
				if (flag & PROJECT_PLAYER)
				{
					if (flag & (PROJECT_BEAM | PROJECT_REFLECTABLE | PROJECT_AIMED))
					{
						/*
						 * A beam or bolt is well aimed
						 * at the PLAYER!
						 * So don't affects the mount.
						 */
						continue;
					}
					else
					{
						/*
						 * The spell is not well aimed,
						 * So partly affect the mount too.
						 */
						effective_dist++;
					}
				}

				/*
				 * This grid is the original target.
				 * Or aimed on your horse.
				 */
				else if (((y == y2) && (x == x2)) || (flag & PROJECT_AIMED))
				{
					/* Hit the mount with full damage */
				}

				/*
				 * Otherwise this grid is not the
				 * original target, it means that line
				 * of fire is obstructed by this
				 * monster.
				 */
				 /*
				  * A beam or bolt will hit either
				  * player or mount.  Choose randomly.
				  */
				else if (flag & (PROJECT_BEAM | PROJECT_REFLECTABLE))
				{
					if (one_in_(2))
					{
						/* Hit the mount with full damage */
					}
					else
					{
						flag |= PROJECT_PLAYER;
						continue;
					}
				}

				/*
				 * The spell is not well aimed, so
				 * partly affect both player and
				 * mount.
				 */
				else
				{
					effective_dist++;
				}
			}

			if (project_m(caster_ptr, who, effective_dist, y, x, dam, typ, flag, see_s_msg)) notice = TRUE;
		}

		/* Player affected one monster (without "jumping") */
		if (!who && (project_m_n == 1) && !jump)
		{
			x = project_m_x;
			y = project_m_y;
			if (caster_ptr->current_floor_ptr->grid_array[y][x].m_idx > 0)
			{
				monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[caster_ptr->current_floor_ptr->grid_array[y][x].m_idx];

				if (m_ptr->ml)
				{
					if (!caster_ptr->image) monster_race_track(caster_ptr, m_ptr->ap_r_idx);
					health_track(caster_ptr, caster_ptr->current_floor_ptr->grid_array[y][x].m_idx);
				}
			}
		}
	}

	if (flag & (PROJECT_KILL))
	{
		dist = 0;
		for (i = 0; i < grids; i++)
		{
			int effective_dist;
			if (gm[dist + 1] == i) dist++;

			y = gy[i];
			x = gx[i];
			if (!player_bold(caster_ptr, y, x)) continue;

			/* Find the closest point in the blast */
			if (breath)
			{
				effective_dist = dist_to_line(y, x, y1, x1, by, bx);
			}
			else
			{
				effective_dist = dist;
			}

			if (caster_ptr->riding)
			{
				if (flag & PROJECT_PLAYER)
				{
					/* Hit the player with full damage */
				}

				/*
				 * Hack -- When this grid was not the
				 * original target, a beam or bolt
				 * would hit either player or mount,
				 * and should be choosen randomly.
				 *
				 * But already choosen to hit the
				 * mount at this point.
				 *
				 * Or aimed on your horse.
				 */
				else if (flag & (PROJECT_BEAM | PROJECT_REFLECTABLE | PROJECT_AIMED))
				{
					/*
					 * A beam or bolt is well aimed
					 * at the mount!
					 * So don't affects the player.
					 */
					continue;
				}
				else
				{
					/*
					 * The spell is not well aimed,
					 * So partly affect the player too.
					 */
					effective_dist++;
				}
			}

			if (affect_player(who, caster_ptr, who_name, effective_dist, y, x, dam, typ, flag, monspell)) notice = TRUE;
		}
	}

	if (caster_ptr->riding)
	{
		GAME_TEXT m_name[MAX_NLEN];
		monster_desc(caster_ptr, m_name, &caster_ptr->current_floor_ptr->m_list[caster_ptr->riding], 0);
		if (rakubadam_m > 0)
		{
			if (rakuba(caster_ptr, rakubadam_m, FALSE))
			{
				msg_format(_("%^sに振り落とされた！", "%^s has thrown you off!"), m_name);
			}
		}

		if (caster_ptr->riding && rakubadam_p > 0)
		{
			if (rakuba(caster_ptr, rakubadam_p, FALSE))
			{
				msg_format(_("%^sから落ちてしまった！", "You have fallen from %s."), m_name);
			}
		}
	}

	return (notice);
}


/*!
 * @brief 鏡魔法「封魔結界」の効果処理
 * @param dam ダメージ量
 * @return 効果があったらTRUEを返す
 */
bool binding_field(player_type *caster_ptr, HIT_POINT dam)
{
	POSITION mirror_x[10], mirror_y[10]; /* 鏡はもっと少ない */
	int mirror_num = 0;	/* 鏡の数 */
	POSITION x, y;
	int msec = delay_factor * delay_factor*delay_factor;

	/* 三角形の頂点 */
	POSITION point_x[3];
	POSITION point_y[3];

	/* Default target of monsterspell is player */
	monster_target_y = caster_ptr->y;
	monster_target_x = caster_ptr->x;

	for (x = 0; x < caster_ptr->current_floor_ptr->width; x++)
	{
		for (y = 0; y < caster_ptr->current_floor_ptr->height; y++)
		{
			if (is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[y][x]) &&
				distance(caster_ptr->y, caster_ptr->x, y, x) <= MAX_RANGE &&
				distance(caster_ptr->y, caster_ptr->x, y, x) != 0 &&
				player_has_los_bold(caster_ptr, y, x) &&
				projectable(caster_ptr, caster_ptr->y, caster_ptr->x, y, x))
			{
				mirror_y[mirror_num] = y;
				mirror_x[mirror_num] = x;
				mirror_num++;
			}
		}
	}

	if (mirror_num < 2)return FALSE;

	point_x[0] = randint0(mirror_num);
	do {
		point_x[1] = randint0(mirror_num);
	} while (point_x[0] == point_x[1]);

	point_y[0] = mirror_y[point_x[0]];
	point_x[0] = mirror_x[point_x[0]];
	point_y[1] = mirror_y[point_x[1]];
	point_x[1] = mirror_x[point_x[1]];
	point_y[2] = caster_ptr->y;
	point_x[2] = caster_ptr->x;

	x = point_x[0] + point_x[1] + point_x[2];
	y = point_y[0] + point_y[1] + point_y[2];

	POSITION centersign = (point_x[0] * 3 - x)*(point_y[1] * 3 - y)
		- (point_y[0] * 3 - y)*(point_x[1] * 3 - x);
	if (centersign == 0)return FALSE;

	POSITION x1 = point_x[0] < point_x[1] ? point_x[0] : point_x[1];
	x1 = x1 < point_x[2] ? x1 : point_x[2];
	POSITION y1 = point_y[0] < point_y[1] ? point_y[0] : point_y[1];
	y1 = y1 < point_y[2] ? y1 : point_y[2];

	POSITION x2 = point_x[0] > point_x[1] ? point_x[0] : point_x[1];
	x2 = x2 > point_x[2] ? x2 : point_x[2];
	POSITION y2 = point_y[0] > point_y[1] ? point_y[0] : point_y[1];
	y2 = y2 > point_y[2] ? y2 : point_y[2];

	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			if (centersign*((point_x[0] - x)*(point_y[1] - y)
				- (point_y[0] - y)*(point_x[1] - x)) >= 0 &&
				centersign*((point_x[1] - x)*(point_y[2] - y)
					- (point_y[1] - y)*(point_x[2] - x)) >= 0 &&
				centersign*((point_x[2] - x)*(point_y[0] - y)
					- (point_y[2] - y)*(point_x[0] - x)) >= 0)
			{
				if (player_has_los_bold(caster_ptr, y, x) && projectable(caster_ptr, caster_ptr->y, caster_ptr->x, y, x))
				{
					if (!(caster_ptr->blind)
						&& panel_contains(y, x))
					{
						u16b p = bolt_pict(y, x, y, x, GF_MANA);
						print_rel(caster_ptr, PICT_C(p), PICT_A(p), y, x);
						move_cursor_relative(y, x);
						Term_fresh();
						Term_xtra(TERM_XTRA_DELAY, msec);
					}
				}
			}
		}
	}

	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			if (centersign*((point_x[0] - x)*(point_y[1] - y)
				- (point_y[0] - y)*(point_x[1] - x)) >= 0 &&
				centersign*((point_x[1] - x)*(point_y[2] - y)
					- (point_y[1] - y)*(point_x[2] - x)) >= 0 &&
				centersign*((point_x[2] - x)*(point_y[0] - y)
					- (point_y[2] - y)*(point_x[0] - x)) >= 0)
			{
				if (player_has_los_bold(caster_ptr, y, x) && projectable(caster_ptr, caster_ptr->y, caster_ptr->x, y, x))
				{
					(void)affect_feature(caster_ptr, 0, 0, y, x, dam, GF_MANA);
				}
			}
		}
	}

	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			if (centersign*((point_x[0] - x)*(point_y[1] - y)
				- (point_y[0] - y)*(point_x[1] - x)) >= 0 &&
				centersign*((point_x[1] - x)*(point_y[2] - y)
					- (point_y[1] - y)*(point_x[2] - x)) >= 0 &&
				centersign*((point_x[2] - x)*(point_y[0] - y)
					- (point_y[2] - y)*(point_x[0] - x)) >= 0)
			{
				if (player_has_los_bold(caster_ptr, y, x) && projectable(caster_ptr, caster_ptr->y, caster_ptr->x, y, x))
				{
					(void)affect_item(caster_ptr, 0, 0, y, x, dam, GF_MANA);
				}
			}
		}
	}

	for (y = y1; y <= y2; y++)
	{
		for (x = x1; x <= x2; x++)
		{
			if (centersign*((point_x[0] - x)*(point_y[1] - y)
				- (point_y[0] - y)*(point_x[1] - x)) >= 0 &&
				centersign*((point_x[1] - x)*(point_y[2] - y)
					- (point_y[1] - y)*(point_x[2] - x)) >= 0 &&
				centersign*((point_x[2] - x)*(point_y[0] - y)
					- (point_y[2] - y)*(point_x[0] - x)) >= 0)
			{
				if (player_has_los_bold(caster_ptr, y, x) && projectable(caster_ptr, caster_ptr->y, caster_ptr->x, y, x))
				{
					(void)project_m(caster_ptr, 0, 0, y, x, dam, GF_MANA,
						(PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP), TRUE);
				}
			}
		}
	}

	if (one_in_(7))
	{
		msg_print(_("鏡が結界に耐えきれず、壊れてしまった。", "The field broke a mirror"));
		remove_mirror(caster_ptr, point_y[0], point_x[0]);
	}

	return TRUE;
}


/*!
 * @brief 鏡魔法「鏡の封印」の効果処理
 * @param dam ダメージ量
 * @return 効果があったらTRUEを返す
 */
void seal_of_mirror(player_type *caster_ptr, HIT_POINT dam)
{
	for (POSITION x = 0; x < caster_ptr->current_floor_ptr->width; x++)
	{
		for (POSITION y = 0; y < caster_ptr->current_floor_ptr->height; y++)
		{
			if (!is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[y][x]))
				continue;

			if (!project_m(caster_ptr, 0, 0, y, x, dam, GF_GENOCIDE,
				(PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP), TRUE))
				continue;

			if (!caster_ptr->current_floor_ptr->grid_array[y][x].m_idx)
			{
				remove_mirror(caster_ptr, y, x);
			}
		}
	}
}


/*!
 * @brief 領域魔法に応じて技能の名称を返す。
 * @param tval 魔法書のtval
 * @return 領域魔法の技能名称を保管した文字列ポインタ
 */
concptr spell_category_name(OBJECT_TYPE_VALUE tval)
{
	switch (tval)
	{
	case TV_HISSATSU_BOOK:
		return _("必殺技", "art");
	case TV_LIFE_BOOK:
		return _("祈り", "prayer");
	case TV_MUSIC_BOOK:
		return _("歌", "song");
	default:
		return _("呪文", "spell");
	}
}
