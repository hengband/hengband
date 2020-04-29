/*!
 * todo 単体で2400行近い。要分割
 * @brief 魔法によるモンスターへの効果まとめ
 * @date 2020/04/29
 * @author Hourier
 */

#include "angband.h"
#include "effect/effect-monster.h"
#include "spells-effect-util.h"
#include "player-damage.h"
#include "world.h"
#include "avatar.h"
#include "monster-spell.h"
#include "quest.h"
#include "io/write-diary.h"
#include "main/sound-definitions-table.h"
#include "player-move.h"
#include "monster-status.h"
#include "player-effects.h"
#include "spells-diceroll.h"
#include "monsterrace-hook.h"
#include "cmd/cmd-pet.h" // 暫定、後で消すかも.
#include "combat/melee.h"
#include "core.h" // 暫定、後で消す.
#include "effect/effect-monster-util.h"

typedef enum
{
	GF_SWITCH_FALSE = 0,
	GF_SWITCH_TRUE = 1,
	GF_SWITCH_CONTINUE = 2,
} gf_switch_result;

/*!
 * @brief ビーム/ボルト/ボール系魔法によるモンスターへの効果があるかないかを判定する
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return 効果が何もないならFALSE、何かあるならTRUE
 */
static bool is_never_effect(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	if (!em_ptr->g_ptr->m_idx) return FALSE;
	if (em_ptr->who && (em_ptr->g_ptr->m_idx == em_ptr->who)) return FALSE;
	if ((em_ptr->g_ptr->m_idx == caster_ptr->riding) &&
		!em_ptr->who &&
		!(em_ptr->effect_type == GF_OLD_HEAL) &&
		!(em_ptr->effect_type == GF_OLD_SPEED) &&
		!(em_ptr->effect_type == GF_STAR_HEAL))
		return FALSE;
	if (sukekaku && ((em_ptr->m_ptr->r_idx == MON_SUKE) || (em_ptr->m_ptr->r_idx == MON_KAKU))) return FALSE;
	if (em_ptr->m_ptr->hp < 0) return FALSE;

	return TRUE;
}


/*!
 * todo いずれ分離するが、時期尚早
 * @brief 魔法の効果によって様々なメッセーを出力したり与えるダメージの増減を行ったりする
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return ここのスイッチングで終るならTRUEかFALSE、後続処理を実行するならCONTINUE
 */
static gf_switch_result switch_effects_monster(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	floor_type *floor_ptr = caster_ptr->current_floor_ptr;
	switch (em_ptr->effect_type)
	{
	case GF_MISSILE:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		break;
	}
	case GF_ACID:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flagsr & RFR_IM_ACID)
		{
			em_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
			em_ptr->dam /= 9;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= (RFR_IM_ACID);
		}
		break;
	}
	case GF_ELEC:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flagsr & RFR_IM_ELEC)
		{
			em_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
			em_ptr->dam /= 9;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= (RFR_IM_ELEC);
		}
		break;
	}
	case GF_FIRE:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flagsr & RFR_IM_FIRE)
		{
			em_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
			em_ptr->dam /= 9;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= (RFR_IM_FIRE);
		}
		else if (em_ptr->r_ptr->flags3 & (RF3_HURT_FIRE))
		{
			em_ptr->note = _("はひどい痛手をうけた。", " is hit hard.");
			em_ptr->dam *= 2;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_HURT_FIRE);
		}
		break;
	}
	case GF_COLD:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flagsr & RFR_IM_COLD)
		{
			em_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
			em_ptr->dam /= 9;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= (RFR_IM_COLD);
		}
		else if (em_ptr->r_ptr->flags3 & (RF3_HURT_COLD))
		{
			em_ptr->note = _("はひどい痛手をうけた。", " is hit hard.");
			em_ptr->dam *= 2;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_HURT_COLD);
		}
		break;
	}
	case GF_POIS:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flagsr & RFR_IM_POIS)
		{
			em_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
			em_ptr->dam /= 9;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= (RFR_IM_POIS);
		}
		break;
	}
	case GF_NUKE:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flagsr & RFR_IM_POIS)
		{
			em_ptr->note = _("には耐性がある。", " resists.");
			em_ptr->dam *= 3; em_ptr->dam /= randint1(6) + 6;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= (RFR_IM_POIS);
		}
		else if (one_in_(3)) em_ptr->do_polymorph = TRUE;
		break;
	}
	case GF_HELL_FIRE:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flags3 & RF3_GOOD)
		{
			em_ptr->note = _("はひどい痛手をうけた。", " is hit hard.");
			em_ptr->dam *= 2;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_GOOD);
		}
		break;
	}
	case GF_HOLY_FIRE:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flags3 & RF3_EVIL)
		{
			em_ptr->dam *= 2;
			em_ptr->note = _("はひどい痛手をうけた。", " is hit hard.");
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= RF3_EVIL;
		}
		else
		{
			em_ptr->note = _("には耐性がある。", " resists.");
			em_ptr->dam *= 3; em_ptr->dam /= randint1(6) + 6;
		}
		break;
	}
	case GF_ARROW:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		break;
	}
	case GF_PLASMA:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flagsr & RFR_RES_PLAS)
		{
			em_ptr->note = _("には耐性がある。", " resists.");
			em_ptr->dam *= 3; em_ptr->dam /= randint1(6) + 6;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= (RFR_RES_PLAS);
		}

		break;
	}
	case GF_NETHER:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flagsr & RFR_RES_NETH)
		{
			if (em_ptr->r_ptr->flags3 & RF3_UNDEAD)
			{
				em_ptr->note = _("には完全な耐性がある！", " is immune.");
				em_ptr->dam = 0;
				if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);
			}
			else
			{
				em_ptr->note = _("には耐性がある。", " resists.");
				em_ptr->dam *= 3; em_ptr->dam /= randint1(6) + 6;
			}
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= (RFR_RES_NETH);
		}
		else if (em_ptr->r_ptr->flags3 & RF3_EVIL)
		{
			em_ptr->note = _("はいくらか耐性を示した。", " resists somewhat.");
			em_ptr->dam /= 2;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_EVIL);
		}

		break;
	}
	case GF_WATER:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flagsr & RFR_RES_WATE)
		{
			if ((em_ptr->m_ptr->r_idx == MON_WATER_ELEM) || (em_ptr->m_ptr->r_idx == MON_UNMAKER))
			{
				em_ptr->note = _("には完全な耐性がある！", " is immune.");
				em_ptr->dam = 0;
			}
			else
			{
				em_ptr->note = _("には耐性がある。", " resists.");
				em_ptr->dam *= 3; em_ptr->dam /= randint1(6) + 6;
			}
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= (RFR_RES_WATE);
		}

		break;
	}
	case GF_CHAOS:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flagsr & RFR_RES_CHAO)
		{
			em_ptr->note = _("には耐性がある。", " resists.");
			em_ptr->dam *= 3; em_ptr->dam /= randint1(6) + 6;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= (RFR_RES_CHAO);
		}
		else if ((em_ptr->r_ptr->flags3 & RF3_DEMON) && one_in_(3))
		{
			em_ptr->note = _("はいくらか耐性を示した。", " resists somewhat.");
			em_ptr->dam *= 3; em_ptr->dam /= randint1(6) + 6;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_DEMON);
		}
		else
		{
			em_ptr->do_polymorph = TRUE;
			em_ptr->do_conf = (5 + randint1(11) + em_ptr->r) / (em_ptr->r + 1);
		}

		break;
	}
	case GF_SHARDS:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flagsr & RFR_RES_SHAR)
		{
			em_ptr->note = _("には耐性がある。", " resists.");
			em_ptr->dam *= 3; em_ptr->dam /= randint1(6) + 6;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= (RFR_RES_SHAR);
		}

		break;
	}
	case GF_ROCKET:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flagsr & RFR_RES_SHAR)
		{
			em_ptr->note = _("はいくらか耐性を示した。", " resists somewhat.");
			em_ptr->dam /= 2;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= (RFR_RES_SHAR);
		}

		break;
	}
	case GF_SOUND:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flagsr & RFR_RES_SOUN)
		{
			em_ptr->note = _("には耐性がある。", " resists.");
			em_ptr->dam *= 2; em_ptr->dam /= randint1(6) + 6;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= (RFR_RES_SOUN);
		}
		else
			em_ptr->do_stun = (10 + randint1(15) + em_ptr->r) / (em_ptr->r + 1);

		break;
	}
	case GF_CONFUSION:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flags3 & RF3_NO_CONF)
		{
			em_ptr->note = _("には耐性がある。", " resists.");
			em_ptr->dam *= 3; em_ptr->dam /= randint1(6) + 6;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_NO_CONF);
		}
		else
			em_ptr->do_conf = (10 + randint1(15) + em_ptr->r) / (em_ptr->r + 1);

		break;
	}
	case GF_DISENCHANT:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flagsr & RFR_RES_DISE)
		{
			em_ptr->note = _("には耐性がある。", " resists.");
			em_ptr->dam *= 3; em_ptr->dam /= randint1(6) + 6;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= (RFR_RES_DISE);
		}

		break;
	}
	case GF_NEXUS:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flagsr & RFR_RES_NEXU)
		{
			em_ptr->note = _("には耐性がある。", " resists.");
			em_ptr->dam *= 3; em_ptr->dam /= randint1(6) + 6;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= (RFR_RES_NEXU);
		}

		break;
	}
	case GF_FORCE:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flagsr & RFR_RES_WALL)
		{
			em_ptr->note = _("には耐性がある。", " resists.");
			em_ptr->dam *= 3; em_ptr->dam /= randint1(6) + 6;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= (RFR_RES_WALL);
		}
		else
			em_ptr->do_stun = (randint1(15) + em_ptr->r) / (em_ptr->r + 1);

		break;
	}
	case GF_INERTIAL:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flagsr & RFR_RES_INER)
		{
			em_ptr->note = _("には耐性がある。", " resists.");
			em_ptr->dam *= 3; em_ptr->dam /= randint1(6) + 6;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= (RFR_RES_INER);
		}
		else
		{
			/* Powerful monsters can resist */
			if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) ||
				(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
			{
				em_ptr->obvious = FALSE;
			}
			/* Normal monsters slow down */
			else
			{
				if (set_monster_slow(caster_ptr, em_ptr->g_ptr->m_idx, MON_SLOW(em_ptr->m_ptr) + 50))
				{
					em_ptr->note = _("の動きが遅くなった。", " starts moving slower.");
				}
			}
		}

		break;
	}
	case GF_TIME:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flagsr & RFR_RES_TIME)
		{
			em_ptr->note = _("には耐性がある。", " resists.");
			em_ptr->dam *= 3; em_ptr->dam /= randint1(6) + 6;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= (RFR_RES_TIME);
		}
		else
			em_ptr->do_time = (em_ptr->dam + 1) / 2;

		break;
	}
	case GF_GRAVITY:
	{
		bool resist_tele = FALSE;

		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flagsr & RFR_RES_TELE)
		{
			if (em_ptr->r_ptr->flags1 & (RF1_UNIQUE))
			{
				if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
				em_ptr->note = _("には効果がなかった。", " is unaffected!");
				resist_tele = TRUE;
			}
			else if (em_ptr->r_ptr->level > randint1(100))
			{
				if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
				em_ptr->note = _("には耐性がある！", " resists!");
				resist_tele = TRUE;
			}
		}

		if (!resist_tele) em_ptr->do_dist = 10;
		else em_ptr->do_dist = 0;

		if (caster_ptr->riding && (em_ptr->g_ptr->m_idx == caster_ptr->riding)) em_ptr->do_dist = 0;

		if (em_ptr->r_ptr->flagsr & RFR_RES_GRAV)
		{
			em_ptr->note = _("には耐性がある！", " resists!");
			em_ptr->dam *= 3; em_ptr->dam /= randint1(6) + 6;
			em_ptr->do_dist = 0;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= (RFR_RES_GRAV);
		}
		else
		{
			/* 1. slowness */
			/* Powerful monsters can resist */
			if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) ||
				(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
			{
				em_ptr->obvious = FALSE;
			}
			/* Normal monsters slow down */
			else
			{
				if (set_monster_slow(caster_ptr, em_ptr->g_ptr->m_idx, MON_SLOW(em_ptr->m_ptr) + 50))
				{
					em_ptr->note = _("の動きが遅くなった。", " starts moving slower.");
				}
			}

			/* 2. stun */
			em_ptr->do_stun = damroll((em_ptr->caster_lev / 20) + 3, (em_ptr->dam)) + 1;

			/* Attempt a saving throw */
			if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) ||
				(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
			{
				/* Resist */
				em_ptr->do_stun = 0;
				/* No em_ptr->obvious effect */
				em_ptr->note = _("には効果がなかった。", " is unaffected!");
				em_ptr->obvious = FALSE;
			}
		}

		break;
	}
	case GF_MANA:
	case GF_SEEKER:
	case GF_SUPER_RAY:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		break;
	}
	case GF_DISINTEGRATE:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flags3 & RF3_HURT_ROCK)
		{
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_HURT_ROCK);
			em_ptr->note = _("の皮膚がただれた！", " loses some skin!");
			em_ptr->note_dies = _("は蒸発した！", " evaporates!");
			em_ptr->dam *= 2;
		}

		break;
	}
	case GF_PSI:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (!(los(caster_ptr, em_ptr->m_ptr->fy, em_ptr->m_ptr->fx, caster_ptr->y, caster_ptr->x)))
		{
			if (em_ptr->seen_msg)
				msg_format(_("%sはあなたが見えないので影響されない！", "%^s can't see you, and isn't affected!"), em_ptr->m_name);
			em_ptr->skipped = TRUE;
			break;
		}

		if (em_ptr->r_ptr->flags2 & RF2_EMPTY_MIND)
		{
			em_ptr->dam = 0;
			em_ptr->note = _("には完全な耐性がある！", " is immune.");
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags2 |= (RF2_EMPTY_MIND);

		}
		else if ((em_ptr->r_ptr->flags2 & (RF2_STUPID | RF2_WEIRD_MIND)) ||
			(em_ptr->r_ptr->flags3 & RF3_ANIMAL) ||
			(em_ptr->r_ptr->level > randint1(3 * em_ptr->dam)))
		{
			em_ptr->note = _("には耐性がある！", " resists!");
			em_ptr->dam /= 3;

			/*
			 * Powerful demons & undead can turn a mindcrafter's
			 * attacks back on them
			 */
			if ((em_ptr->r_ptr->flags3 & (RF3_UNDEAD | RF3_DEMON)) &&
				(em_ptr->r_ptr->level > caster_ptr->lev / 2) &&
				one_in_(2))
			{
				em_ptr->note = NULL;
				msg_format(_("%^sの堕落した精神は攻撃を跳ね返した！",
					(em_ptr->seen ? "%^s's corrupted mind backlashes your attack!" :
						"%^ss corrupted mind backlashes your attack!")), em_ptr->m_name);

				if ((randint0(100 + em_ptr->r_ptr->level / 2) < caster_ptr->skill_sav) && !CHECK_MULTISHADOW(caster_ptr))
				{
					msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
				}
				else
				{
					/* Injure +/- confusion */
					monster_desc(caster_ptr, em_ptr->killer, em_ptr->m_ptr, MD_WRONGDOER_NAME);
					take_hit(caster_ptr, DAMAGE_ATTACK, em_ptr->dam, em_ptr->killer, -1);  /* has already been /3 */
					if (one_in_(4) && !CHECK_MULTISHADOW(caster_ptr))
					{
						switch (randint1(4))
						{
						case 1:
							set_confused(caster_ptr, caster_ptr->confused + 3 + randint1(em_ptr->dam));
							break;
						case 2:
							set_stun(caster_ptr, caster_ptr->stun + randint1(em_ptr->dam));
							break;
						case 3:
						{
							if (em_ptr->r_ptr->flags3 & RF3_NO_FEAR)
								em_ptr->note = _("には効果がなかった。", " is unaffected.");
							else
								set_afraid(caster_ptr, caster_ptr->afraid + 3 + randint1(em_ptr->dam));
							break;
						}
						default:
							if (!caster_ptr->free_act)
								(void)set_paralyzed(caster_ptr, caster_ptr->paralyzed + randint1(em_ptr->dam));
							break;
						}
					}
				}

				em_ptr->dam = 0;
			}
		}

		if ((em_ptr->dam > 0) && one_in_(4))
		{
			switch (randint1(4))
			{
			case 1:
				em_ptr->do_conf = 3 + randint1(em_ptr->dam);
				break;
			case 2:
				em_ptr->do_stun = 3 + randint1(em_ptr->dam);
				break;
			case 3:
				em_ptr->do_fear = 3 + randint1(em_ptr->dam);
				break;
			default:
				em_ptr->note = _("は眠り込んでしまった！", " falls asleep!");
				em_ptr->do_sleep = 3 + randint1(em_ptr->dam);
				break;
			}
		}

		em_ptr->note_dies = _("の精神は崩壊し、肉体は抜け殻となった。", " collapses, a mindless husk.");
		break;
	}
	case GF_PSI_DRAIN:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flags2 & RF2_EMPTY_MIND)
		{
			em_ptr->dam = 0;
			em_ptr->note = _("には完全な耐性がある！", " is immune.");
		}
		else if ((em_ptr->r_ptr->flags2 & (RF2_STUPID | RF2_WEIRD_MIND)) ||
			(em_ptr->r_ptr->flags3 & RF3_ANIMAL) ||
			(em_ptr->r_ptr->level > randint1(3 * em_ptr->dam)))
		{
			em_ptr->note = _("には耐性がある！", " resists!");
			em_ptr->dam /= 3;

			/*
			 * Powerful demons & undead can turn a mindcrafter's
			 * attacks back on them
			 */
			if ((em_ptr->r_ptr->flags3 & (RF3_UNDEAD | RF3_DEMON)) &&
				(em_ptr->r_ptr->level > caster_ptr->lev / 2) &&
				(one_in_(2)))
			{
				em_ptr->note = NULL;
				msg_format(_("%^sの堕落した精神は攻撃を跳ね返した！",
					(em_ptr->seen ? "%^s's corrupted mind backlashes your attack!" :
						"%^ss corrupted mind backlashes your attack!")), em_ptr->m_name);
				if ((randint0(100 + em_ptr->r_ptr->level / 2) < caster_ptr->skill_sav) && !CHECK_MULTISHADOW(caster_ptr))
				{
					msg_print(_("あなたは効力を跳ね返した！", "You resist the effects!"));
				}
				else
				{
					monster_desc(caster_ptr, em_ptr->killer, em_ptr->m_ptr, MD_WRONGDOER_NAME);
					if (!CHECK_MULTISHADOW(caster_ptr))
					{
						msg_print(_("超能力パワーを吸いとられた！", "Your psychic energy is drained!"));
						caster_ptr->csp -= damroll(5, em_ptr->dam) / 2;
						if (caster_ptr->csp < 0) caster_ptr->csp = 0;
						caster_ptr->redraw |= PR_MANA;
						caster_ptr->window |= (PW_SPELL);
					}
					take_hit(caster_ptr, DAMAGE_ATTACK, em_ptr->dam, em_ptr->killer, -1);  /* has already been /3 */
				}

				em_ptr->dam = 0;
			}
		}
		else if (em_ptr->dam > 0)
		{
			int b = damroll(5, em_ptr->dam) / 4;
			concptr str = (caster_ptr->pclass == CLASS_MINDCRAFTER) ? _("超能力パワー", "psychic energy") : _("魔力", "mana");
			concptr msg = _("あなたは%sの苦痛を%sに変換した！",
				(em_ptr->seen ? "You convert %s's pain into %s!" :
					"You convert %ss pain into %s!"));
			msg_format(msg, em_ptr->m_name, str);

			b = MIN(caster_ptr->msp, caster_ptr->csp + b);
			caster_ptr->csp = b;
			caster_ptr->redraw |= PR_MANA;
			caster_ptr->window |= (PW_SPELL);
		}

		em_ptr->note_dies = _("の精神は崩壊し、肉体は抜け殻となった。", " collapses, a mindless husk.");
		break;
	}
	case GF_TELEKINESIS:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (one_in_(4))
		{
			if (caster_ptr->riding && (em_ptr->g_ptr->m_idx == caster_ptr->riding)) em_ptr->do_dist = 0;
			else em_ptr->do_dist = 7;
		}

		em_ptr->do_stun = damroll((em_ptr->caster_lev / 20) + 3, em_ptr->dam) + 1;
		if ((em_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
			(em_ptr->r_ptr->level > 5 + randint1(em_ptr->dam)))
		{
			em_ptr->do_stun = 0;
			em_ptr->obvious = FALSE;
		}

		break;
	}
	case GF_PSY_SPEAR:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		break;
	}
	case GF_METEOR:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		break;
	}
	case GF_DOMINATION:
	{
		if (!is_hostile(em_ptr->m_ptr)) break;
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) ||
			(em_ptr->r_ptr->flags3 & RF3_NO_CONF) ||
			(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
		{
			if (em_ptr->r_ptr->flags3 & RF3_NO_CONF)
			{
				if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_NO_CONF);
			}

			em_ptr->do_conf = 0;

			/*
			 * Powerful demons & undead can turn a mindcrafter's
			 * attacks back on them
			 */
			if ((em_ptr->r_ptr->flags3 & (RF3_UNDEAD | RF3_DEMON)) &&
				(em_ptr->r_ptr->level > caster_ptr->lev / 2) &&
				(one_in_(2)))
			{
				em_ptr->note = NULL;
				msg_format(_("%^sの堕落した精神は攻撃を跳ね返した！",
					(em_ptr->seen ? "%^s's corrupted mind backlashes your attack!" :
						"%^ss corrupted mind backlashes your attack!")), em_ptr->m_name);

				/* Saving throw */
				if (randint0(100 + em_ptr->r_ptr->level / 2) < caster_ptr->skill_sav)
				{
					msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
				}
				else
				{
					/* Confuse, stun, terrify */
					switch (randint1(4))
					{
					case 1:
						set_stun(caster_ptr, caster_ptr->stun + em_ptr->dam / 2);
						break;
					case 2:
						set_confused(caster_ptr, caster_ptr->confused + em_ptr->dam / 2);
						break;
					default:
					{
						if (em_ptr->r_ptr->flags3 & RF3_NO_FEAR)
							em_ptr->note = _("には効果がなかった。", " is unaffected.");
						else
							set_afraid(caster_ptr, caster_ptr->afraid + em_ptr->dam);
					}
					}
				}
			}
			else
			{
				em_ptr->note = _("には効果がなかった。", " is unaffected.");
				em_ptr->obvious = FALSE;
			}
		}
		else
		{
			if (!common_saving_throw_charm(caster_ptr, em_ptr->dam, em_ptr->m_ptr))
			{
				em_ptr->note = _("があなたに隷属した。", " is in your thrall!");
				set_pet(caster_ptr, em_ptr->m_ptr);
			}
			else
			{
				switch (randint1(4))
				{
				case 1:
					em_ptr->do_stun = em_ptr->dam / 2;
					break;
				case 2:
					em_ptr->do_conf = em_ptr->dam / 2;
					break;
				default:
					em_ptr->do_fear = em_ptr->dam;
				}
			}
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_ICE:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		em_ptr->do_stun = (randint1(15) + 1) / (em_ptr->r + 1);
		if (em_ptr->r_ptr->flagsr & RFR_IM_COLD)
		{
			em_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
			em_ptr->dam /= 9;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= (RFR_IM_COLD);
		}
		else if (em_ptr->r_ptr->flags3 & (RF3_HURT_COLD))
		{
			em_ptr->note = _("はひどい痛手をうけた。", " is hit hard.");
			em_ptr->dam *= 2;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_HURT_COLD);
		}

		break;
	}
	case GF_HYPODYNAMIA:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (!monster_living(em_ptr->m_ptr->r_idx))
		{
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr))
			{
				if (em_ptr->r_ptr->flags3 & RF3_DEMON) em_ptr->r_ptr->r_flags3 |= (RF3_DEMON);
				if (em_ptr->r_ptr->flags3 & RF3_UNDEAD) em_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);
				if (em_ptr->r_ptr->flags3 & RF3_NONLIVING) em_ptr->r_ptr->r_flags3 |= (RF3_NONLIVING);
			}
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;
			em_ptr->dam = 0;
		}
		else
			em_ptr->do_time = (em_ptr->dam + 7) / 8;

		break;
	}
	case GF_DEATH_RAY:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (!monster_living(em_ptr->m_ptr->r_idx))
		{
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr))
			{
				if (em_ptr->r_ptr->flags3 & RF3_DEMON) em_ptr->r_ptr->r_flags3 |= (RF3_DEMON);
				if (em_ptr->r_ptr->flags3 & RF3_UNDEAD) em_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);
				if (em_ptr->r_ptr->flags3 & RF3_NONLIVING) em_ptr->r_ptr->r_flags3 |= (RF3_NONLIVING);
			}
			em_ptr->note = _("には完全な耐性がある！", " is immune.");
			em_ptr->obvious = FALSE;
			em_ptr->dam = 0;
		}
		else if (((em_ptr->r_ptr->flags1 & RF1_UNIQUE) &&
			(randint1(888) != 666)) ||
			(((em_ptr->r_ptr->level + randint1(20)) > randint1((em_ptr->caster_lev / 2) + randint1(10))) &&
				randint1(100) != 66))
		{
			em_ptr->note = _("には耐性がある！", " resists!");
			em_ptr->obvious = FALSE;
			em_ptr->dam = 0;
		}

		break;
	}
	case GF_OLD_POLY:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		em_ptr->do_polymorph = TRUE;

		/* Powerful monsters can resist */
		if ((em_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
			(em_ptr->r_ptr->flags1 & RF1_QUESTOR) ||
			(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->do_polymorph = FALSE;
			em_ptr->obvious = FALSE;
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_OLD_CLONE:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		if ((floor_ptr->inside_arena) || is_pet(em_ptr->m_ptr) || (em_ptr->r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) || (em_ptr->r_ptr->flags7 & (RF7_NAZGUL | RF7_UNIQUE2)))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
		}
		else
		{
			em_ptr->m_ptr->hp = em_ptr->m_ptr->maxhp;
			if (multiply_monster(caster_ptr, em_ptr->g_ptr->m_idx, TRUE, 0L))
			{
				em_ptr->note = _("が分裂した！", " spawns!");
			}
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_STAR_HEAL:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		(void)set_monster_csleep(caster_ptr, em_ptr->g_ptr->m_idx, 0);

		if (em_ptr->m_ptr->maxhp < em_ptr->m_ptr->max_maxhp)
		{
			if (em_ptr->seen_msg) msg_format(_("%^sの強さが戻った。", "%^s recovers %s vitality."), em_ptr->m_name, em_ptr->m_poss);
			em_ptr->m_ptr->maxhp = em_ptr->m_ptr->max_maxhp;
		}

		if (!em_ptr->dam)
		{
			if (caster_ptr->health_who == em_ptr->g_ptr->m_idx) caster_ptr->redraw |= (PR_HEALTH);
			if (caster_ptr->riding == em_ptr->g_ptr->m_idx) caster_ptr->redraw |= (PR_UHEALTH);
			break;
		}
	}
	/* Fall through */
	case GF_OLD_HEAL:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		/* Wake up */
		(void)set_monster_csleep(caster_ptr, em_ptr->g_ptr->m_idx, 0);
		if (MON_STUNNED(em_ptr->m_ptr))
		{
			if (em_ptr->seen_msg) msg_format(_("%^sは朦朧状態から立ち直った。", "%^s is no longer stunned."), em_ptr->m_name);
			(void)set_monster_stunned(caster_ptr, em_ptr->g_ptr->m_idx, 0);
		}
		if (MON_CONFUSED(em_ptr->m_ptr))
		{
			if (em_ptr->seen_msg) msg_format(_("%^sは混乱から立ち直った。", "%^s is no longer confused."), em_ptr->m_name);
			(void)set_monster_confused(caster_ptr, em_ptr->g_ptr->m_idx, 0);
		}
		if (MON_MONFEAR(em_ptr->m_ptr))
		{
			if (em_ptr->seen_msg) msg_format(_("%^sは勇気を取り戻した。", "%^s recovers %s courage."), em_ptr->m_name, em_ptr->m_poss);
			(void)set_monster_monfear(caster_ptr, em_ptr->g_ptr->m_idx, 0);
		}

		if (em_ptr->m_ptr->hp < 30000) em_ptr->m_ptr->hp += em_ptr->dam;
		if (em_ptr->m_ptr->hp > em_ptr->m_ptr->maxhp) em_ptr->m_ptr->hp = em_ptr->m_ptr->maxhp;

		if (!em_ptr->who)
		{
			chg_virtue(caster_ptr, V_VITALITY, 1);

			if (em_ptr->r_ptr->flags1 & RF1_UNIQUE)
				chg_virtue(caster_ptr, V_INDIVIDUALISM, 1);

			if (is_friendly(em_ptr->m_ptr))
				chg_virtue(caster_ptr, V_HONOUR, 1);
			else if (!(em_ptr->r_ptr->flags3 & RF3_EVIL))
			{
				if (em_ptr->r_ptr->flags3 & RF3_GOOD)
					chg_virtue(caster_ptr, V_COMPASSION, 2);
				else
					chg_virtue(caster_ptr, V_COMPASSION, 1);
			}

			if (em_ptr->r_ptr->flags3 & RF3_ANIMAL)
				chg_virtue(caster_ptr, V_NATURE, 1);
		}

		if (em_ptr->m_ptr->r_idx == MON_LEPER)
		{
			em_ptr->heal_leper = TRUE;
			if (!em_ptr->who) chg_virtue(caster_ptr, V_COMPASSION, 5);
		}

		if (caster_ptr->health_who == em_ptr->g_ptr->m_idx) caster_ptr->redraw |= (PR_HEALTH);
		if (caster_ptr->riding == em_ptr->g_ptr->m_idx) caster_ptr->redraw |= (PR_UHEALTH);

		em_ptr->note = _("は体力を回復したようだ。", " looks healthier.");

		em_ptr->dam = 0;
		break;
	}
	case GF_OLD_SPEED:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		if (set_monster_fast(caster_ptr, em_ptr->g_ptr->m_idx, MON_FAST(em_ptr->m_ptr) + 100))
		{
			em_ptr->note = _("の動きが速くなった。", " starts moving faster.");
		}

		if (!em_ptr->who)
		{
			if (em_ptr->r_ptr->flags1 & RF1_UNIQUE)
				chg_virtue(caster_ptr, V_INDIVIDUALISM, 1);
			if (is_friendly(em_ptr->m_ptr))
				chg_virtue(caster_ptr, V_HONOUR, 1);
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_OLD_SLOW:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		/* Powerful monsters can resist */
		if ((em_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
			(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;
		}
		else
		{
			if (set_monster_slow(caster_ptr, em_ptr->g_ptr->m_idx, MON_SLOW(em_ptr->m_ptr) + 50))
			{
				em_ptr->note = _("の動きが遅くなった。", " starts moving slower.");
			}
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_OLD_SLEEP:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		if ((em_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
			(em_ptr->r_ptr->flags3 & RF3_NO_SLEEP) ||
			(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
		{
			if (em_ptr->r_ptr->flags3 & RF3_NO_SLEEP)
			{
				if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_NO_SLEEP);
			}

			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;
		}
		else
		{
			em_ptr->note = _("は眠り込んでしまった！", " falls asleep!");
			em_ptr->do_sleep = 500;
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_STASIS_EVIL:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		if ((em_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
			!(em_ptr->r_ptr->flags3 & RF3_EVIL) ||
			(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;
		}
		else
		{
			em_ptr->note = _("は動けなくなった！", " is suspended!");
			em_ptr->do_sleep = 500;
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_STASIS:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		if ((em_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
			(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;
		}
		else
		{
			em_ptr->note = _("は動けなくなった！", " is suspended!");
			em_ptr->do_sleep = 500;
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_CHARM:
	{
		int vir;
		vir = virtue_number(caster_ptr, V_HARMONY);
		if (vir)
		{
			em_ptr->dam += caster_ptr->virtues[vir - 1] / 10;
		}

		vir = virtue_number(caster_ptr, V_INDIVIDUALISM);
		if (vir)
		{
			em_ptr->dam -= caster_ptr->virtues[vir - 1] / 20;
		}

		if (em_ptr->seen) em_ptr->obvious = TRUE;

		if (common_saving_throw_charm(caster_ptr, em_ptr->dam, em_ptr->m_ptr))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;

			if (one_in_(4)) em_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
		}
		else if (caster_ptr->cursed & TRC_AGGRAVATE)
		{
			em_ptr->note = _("はあなたに敵意を抱いている！", " hates you too much!");
			if (one_in_(4)) em_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
		}
		else
		{
			em_ptr->note = _("は突然友好的になったようだ！", " suddenly seems friendly!");
			set_pet(caster_ptr, em_ptr->m_ptr);

			chg_virtue(caster_ptr, V_INDIVIDUALISM, -1);
			if (em_ptr->r_ptr->flags3 & RF3_ANIMAL)
				chg_virtue(caster_ptr, V_NATURE, 1);
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_CONTROL_UNDEAD:
	{
		int vir;
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		vir = virtue_number(caster_ptr, V_UNLIFE);
		if (vir)
		{
			em_ptr->dam += caster_ptr->virtues[vir - 1] / 10;
		}

		vir = virtue_number(caster_ptr, V_INDIVIDUALISM);
		if (vir)
		{
			em_ptr->dam -= caster_ptr->virtues[vir - 1] / 20;
		}

		if (common_saving_throw_control(caster_ptr, em_ptr->dam, em_ptr->m_ptr) ||
			!(em_ptr->r_ptr->flags3 & RF3_UNDEAD))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;
			if (one_in_(4)) em_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
		}
		else if (caster_ptr->cursed & TRC_AGGRAVATE)
		{
			em_ptr->note = _("はあなたに敵意を抱いている！", " hates you too much!");
			if (one_in_(4)) em_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
		}
		else
		{
			em_ptr->note = _("は既にあなたの奴隷だ！", " is in your thrall!");
			set_pet(caster_ptr, em_ptr->m_ptr);
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_CONTROL_DEMON:
	{
		int vir;
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		vir = virtue_number(caster_ptr, V_UNLIFE);
		if (vir)
		{
			em_ptr->dam += caster_ptr->virtues[vir - 1] / 10;
		}

		vir = virtue_number(caster_ptr, V_INDIVIDUALISM);
		if (vir)
		{
			em_ptr->dam -= caster_ptr->virtues[vir - 1] / 20;
		}

		if (common_saving_throw_control(caster_ptr, em_ptr->dam, em_ptr->m_ptr) ||
			!(em_ptr->r_ptr->flags3 & RF3_DEMON))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;
			if (one_in_(4)) em_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
		}
		else if (caster_ptr->cursed & TRC_AGGRAVATE)
		{
			em_ptr->note = _("はあなたに敵意を抱いている！", " hates you too much!");
			if (one_in_(4)) em_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
		}
		else
		{
			em_ptr->note = _("は既にあなたの奴隷だ！", " is in your thrall!");
			set_pet(caster_ptr, em_ptr->m_ptr);
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_CONTROL_ANIMAL:
	{
		int vir;
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		vir = virtue_number(caster_ptr, V_NATURE);
		if (vir)
		{
			em_ptr->dam += caster_ptr->virtues[vir - 1] / 10;
		}

		vir = virtue_number(caster_ptr, V_INDIVIDUALISM);
		if (vir)
		{
			em_ptr->dam -= caster_ptr->virtues[vir - 1] / 20;
		}

		if (common_saving_throw_control(caster_ptr, em_ptr->dam, em_ptr->m_ptr) ||
			!(em_ptr->r_ptr->flags3 & RF3_ANIMAL))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;
			if (one_in_(4)) em_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
		}
		else if (caster_ptr->cursed & TRC_AGGRAVATE)
		{
			em_ptr->note = _("はあなたに敵意を抱いている！", " hates you too much!");
			if (one_in_(4)) em_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
		}
		else
		{
			em_ptr->note = _("はなついた。", " is tamed!");
			set_pet(caster_ptr, em_ptr->m_ptr);
			if (em_ptr->r_ptr->flags3 & RF3_ANIMAL)
				chg_virtue(caster_ptr, V_NATURE, 1);
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_CHARM_LIVING:
	{
		int vir;

		vir = virtue_number(caster_ptr, V_UNLIFE);
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		vir = virtue_number(caster_ptr, V_UNLIFE);
		if (vir)
		{
			em_ptr->dam -= caster_ptr->virtues[vir - 1] / 10;
		}

		vir = virtue_number(caster_ptr, V_INDIVIDUALISM);
		if (vir)
		{
			em_ptr->dam -= caster_ptr->virtues[vir - 1] / 20;
		}

		msg_format(_("%sを見つめた。", "You stare into %s."), em_ptr->m_name);

		if (common_saving_throw_charm(caster_ptr, em_ptr->dam, em_ptr->m_ptr) ||
			!monster_living(em_ptr->m_ptr->r_idx))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;
			if (one_in_(4)) em_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
		}
		else if (caster_ptr->cursed & TRC_AGGRAVATE)
		{
			em_ptr->note = _("はあなたに敵意を抱いている！", " hates you too much!");
			if (one_in_(4)) em_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
		}
		else
		{
			em_ptr->note = _("を支配した。", " is tamed!");
			set_pet(caster_ptr, em_ptr->m_ptr);
			if (em_ptr->r_ptr->flags3 & RF3_ANIMAL)
				chg_virtue(caster_ptr, V_NATURE, 1);
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_OLD_CONF:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		em_ptr->do_conf = damroll(3, (em_ptr->dam / 2)) + 1;
		if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) ||
			(em_ptr->r_ptr->flags3 & (RF3_NO_CONF)) ||
			(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
		{
			if (em_ptr->r_ptr->flags3 & (RF3_NO_CONF))
			{
				if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_NO_CONF);
			}

			em_ptr->do_conf = 0;
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_STUN:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		em_ptr->do_stun = damroll((em_ptr->caster_lev / 20) + 3, (em_ptr->dam)) + 1;
		if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) ||
			(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
		{
			em_ptr->do_stun = 0;
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_LITE_WEAK:
	{
		if (!em_ptr->dam)
		{
			em_ptr->skipped = TRUE;
			break;
		}

		if (em_ptr->r_ptr->flags3 & (RF3_HURT_LITE))
		{
			if (em_ptr->seen) em_ptr->obvious = TRUE;

			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_HURT_LITE);

			em_ptr->note = _("は光に身をすくめた！", " cringes from the light!");
			em_ptr->note_dies = _("は光を受けてしぼんでしまった！", " shrivels away in the light!");
		}
		else
		{
			em_ptr->dam = 0;
		}

		break;
	}
	case GF_LITE:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		if (em_ptr->r_ptr->flagsr & RFR_RES_LITE)
		{
			em_ptr->note = _("には耐性がある！", " resists!");
			em_ptr->dam *= 2; em_ptr->dam /= (randint1(6) + 6);
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= (RFR_RES_LITE);
		}
		else if (em_ptr->r_ptr->flags3 & (RF3_HURT_LITE))
		{
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_HURT_LITE);
			em_ptr->note = _("は光に身をすくめた！", " cringes from the light!");
			em_ptr->note_dies = _("は光を受けてしぼんでしまった！", " shrivels away in the light!");
			em_ptr->dam *= 2;
		}
		break;
	}
	case GF_DARK:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		if (em_ptr->r_ptr->flagsr & RFR_RES_DARK)
		{
			em_ptr->note = _("には耐性がある！", " resists!");
			em_ptr->dam *= 2; em_ptr->dam /= (randint1(6) + 6);
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= (RFR_RES_DARK);
		}

		break;
	}
	case GF_KILL_WALL:
	{
		if (em_ptr->r_ptr->flags3 & (RF3_HURT_ROCK))
		{
			if (em_ptr->seen) em_ptr->obvious = TRUE;

			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_HURT_ROCK);

			em_ptr->note = _("の皮膚がただれた！", " loses some skin!");
			em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
		}
		else
		{
			em_ptr->dam = 0;
		}

		break;
	}
	case GF_AWAY_UNDEAD:
	{
		if (em_ptr->r_ptr->flags3 & (RF3_UNDEAD))
		{
			bool resists_tele = FALSE;

			if (em_ptr->r_ptr->flagsr & RFR_RES_TELE)
			{
				if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) || (em_ptr->r_ptr->flagsr & RFR_RES_ALL))
				{
					if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
					em_ptr->note = _("には効果がなかった。", " is unaffected.");
					resists_tele = TRUE;
				}
				else if (em_ptr->r_ptr->level > randint1(100))
				{
					if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
					em_ptr->note = _("には耐性がある！", " resists!");
					resists_tele = TRUE;
				}
			}

			if (!resists_tele)
			{
				if (em_ptr->seen) em_ptr->obvious = TRUE;
				if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);
				em_ptr->do_dist = em_ptr->dam;
			}
		}
		else
		{
			em_ptr->skipped = TRUE;
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_AWAY_EVIL:
	{
		if (em_ptr->r_ptr->flags3 & (RF3_EVIL))
		{
			bool resists_tele = FALSE;

			if (em_ptr->r_ptr->flagsr & RFR_RES_TELE)
			{
				if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) || (em_ptr->r_ptr->flagsr & RFR_RES_ALL))
				{
					if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
					em_ptr->note = _("には効果がなかった。", " is unaffected.");
					resists_tele = TRUE;
				}
				else if (em_ptr->r_ptr->level > randint1(100))
				{
					if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
					em_ptr->note = _("には耐性がある！", " resists!");
					resists_tele = TRUE;
				}
			}

			if (!resists_tele)
			{
				if (em_ptr->seen) em_ptr->obvious = TRUE;
				if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_EVIL);
				em_ptr->do_dist = em_ptr->dam;
			}
		}
		else
		{
			em_ptr->skipped = TRUE;
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_AWAY_ALL:
	{
		bool resists_tele = FALSE;
		if (em_ptr->r_ptr->flagsr & RFR_RES_TELE)
		{
			if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) || (em_ptr->r_ptr->flagsr & RFR_RES_ALL))
			{
				if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
				em_ptr->note = _("には効果がなかった。", " is unaffected.");
				resists_tele = TRUE;
			}
			else if (em_ptr->r_ptr->level > randint1(100))
			{
				if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
				em_ptr->note = _("には耐性がある！", " resists!");
				resists_tele = TRUE;
			}
		}

		if (!resists_tele)
		{
			if (em_ptr->seen) em_ptr->obvious = TRUE;

			em_ptr->do_dist = em_ptr->dam;
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_TURN_UNDEAD:
	{
		if (em_ptr->r_ptr->flags3 & (RF3_UNDEAD))
		{
			if (em_ptr->seen) em_ptr->obvious = TRUE;

			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);

			em_ptr->do_fear = damroll(3, (em_ptr->dam / 2)) + 1;
			if (em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10)
			{
				em_ptr->note = _("には効果がなかった。", " is unaffected.");
				em_ptr->obvious = FALSE;
				em_ptr->do_fear = 0;
			}
		}
		else
		{
			em_ptr->skipped = TRUE;
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_TURN_EVIL:
	{
		if (em_ptr->r_ptr->flags3 & (RF3_EVIL))
		{
			if (em_ptr->seen) em_ptr->obvious = TRUE;

			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_EVIL);

			em_ptr->do_fear = damroll(3, (em_ptr->dam / 2)) + 1;
			if (em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10)
			{
				em_ptr->note = _("には効果がなかった。", " is unaffected.");
				em_ptr->obvious = FALSE;
				em_ptr->do_fear = 0;
			}
		}
		else
		{
			em_ptr->skipped = TRUE;
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_TURN_ALL:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		em_ptr->do_fear = damroll(3, (em_ptr->dam / 2)) + 1;
		if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) ||
			(em_ptr->r_ptr->flags3 & (RF3_NO_FEAR)) ||
			(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->obvious = FALSE;
			em_ptr->do_fear = 0;
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_DISP_UNDEAD:
	{
		if (em_ptr->r_ptr->flags3 & (RF3_UNDEAD))
		{
			if (em_ptr->seen) em_ptr->obvious = TRUE;

			/* Learn about em_ptr->effect_type */
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);

			em_ptr->note = _("は身震いした。", " shudders.");
			em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
		}
		else
		{
			em_ptr->skipped = TRUE;
			em_ptr->dam = 0;
		}

		break;
	}
	case GF_DISP_EVIL:
	{
		if (em_ptr->r_ptr->flags3 & (RF3_EVIL))
		{
			if (em_ptr->seen) em_ptr->obvious = TRUE;

			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_EVIL);

			em_ptr->note = _("は身震いした。", " shudders.");
			em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
		}
		else
		{
			em_ptr->skipped = TRUE;
			em_ptr->dam = 0;
		}

		break;
	}
	case GF_DISP_GOOD:
	{
		if (em_ptr->r_ptr->flags3 & (RF3_GOOD))
		{
			if (em_ptr->seen) em_ptr->obvious = TRUE;

			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_GOOD);

			em_ptr->note = _("は身震いした。", " shudders.");
			em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
		}
		else
		{
			em_ptr->skipped = TRUE;
			em_ptr->dam = 0;
		}

		break;
	}
	case GF_DISP_LIVING:
	{
		if (monster_living(em_ptr->m_ptr->r_idx))
		{
			if (em_ptr->seen) em_ptr->obvious = TRUE;

			em_ptr->note = _("は身震いした。", " shudders.");
			em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
		}
		else
		{
			em_ptr->skipped = TRUE;
			em_ptr->dam = 0;
		}

		break;
	}
	case GF_DISP_DEMON:
	{
		if (em_ptr->r_ptr->flags3 & (RF3_DEMON))
		{
			if (em_ptr->seen) em_ptr->obvious = TRUE;

			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_DEMON);

			em_ptr->note = _("は身震いした。", " shudders.");
			em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
		}
		else
		{
			em_ptr->skipped = TRUE;
			em_ptr->dam = 0;
		}

		break;
	}
	case GF_DISP_ALL:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		em_ptr->note = _("は身震いした。", " shudders.");
		em_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
		break;
	}
	case GF_DRAIN_MANA:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if ((em_ptr->r_ptr->flags4 & ~(RF4_NOMAGIC_MASK)) || (em_ptr->r_ptr->a_ability_flags1 & ~(RF5_NOMAGIC_MASK)) || (em_ptr->r_ptr->a_ability_flags2 & ~(RF6_NOMAGIC_MASK)))
		{
			if (em_ptr->who > 0)
			{
				if (em_ptr->m_caster_ptr->hp < em_ptr->m_caster_ptr->maxhp)
				{
					em_ptr->m_caster_ptr->hp += em_ptr->dam;
					if (em_ptr->m_caster_ptr->hp > em_ptr->m_caster_ptr->maxhp) em_ptr->m_caster_ptr->hp = em_ptr->m_caster_ptr->maxhp;
					if (caster_ptr->health_who == em_ptr->who) caster_ptr->redraw |= (PR_HEALTH);
					if (caster_ptr->riding == em_ptr->who) caster_ptr->redraw |= (PR_UHEALTH);

					if (em_ptr->see_s_msg)
					{
						monster_desc(caster_ptr, em_ptr->killer, em_ptr->m_caster_ptr, 0);
						msg_format(_("%^sは気分が良さそうだ。", "%^s appears healthier."), em_ptr->killer);
					}
				}
			}
			else
			{
				msg_format(_("%sから精神エネルギーを吸いとった。", "You draw psychic energy from %s."), em_ptr->m_name);
				(void)hp_player(caster_ptr, em_ptr->dam);
			}
		}
		else
		{
			if (em_ptr->see_s_msg) msg_format(_("%sには効果がなかった。", "%s is unaffected."), em_ptr->m_name);
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_MIND_BLAST:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (!em_ptr->who) msg_format(_("%sをじっと睨んだ。", "You gaze intently at %s."), em_ptr->m_name);

		if ((em_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
			(em_ptr->r_ptr->flags3 & RF3_NO_CONF) ||
			(em_ptr->r_ptr->level > randint1((em_ptr->caster_lev - 10) < 1 ? 1 : (em_ptr->caster_lev - 10)) + 10))
		{
			if (em_ptr->r_ptr->flags3 & (RF3_NO_CONF))
			{
				if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_NO_CONF);
			}

			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->dam = 0;
		}
		else if (em_ptr->r_ptr->flags2 & RF2_EMPTY_MIND)
		{
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags2 |= (RF2_EMPTY_MIND);
			em_ptr->note = _("には完全な耐性がある！", " is immune.");
			em_ptr->dam = 0;
		}
		else if (em_ptr->r_ptr->flags2 & RF2_WEIRD_MIND)
		{
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags2 |= (RF2_WEIRD_MIND);
			em_ptr->note = _("には耐性がある。", " resists.");
			em_ptr->dam /= 3;
		}
		else
		{
			em_ptr->note = _("は精神攻撃を食らった。", " is blasted by psionic energy.");
			em_ptr->note_dies = _("の精神は崩壊し、肉体は抜け殻となった。", " collapses, a mindless husk.");

			if (em_ptr->who > 0) em_ptr->do_conf = randint0(4) + 4;
			else em_ptr->do_conf = randint0(8) + 8;
		}

		break;
	}
	case GF_BRAIN_SMASH:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (!em_ptr->who) msg_format(_("%sをじっと睨んだ。", "You gaze intently at %s."), em_ptr->m_name);

		if ((em_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
			(em_ptr->r_ptr->flags3 & RF3_NO_CONF) ||
			(em_ptr->r_ptr->level > randint1((em_ptr->caster_lev - 10) < 1 ? 1 : (em_ptr->caster_lev - 10)) + 10))
		{
			if (em_ptr->r_ptr->flags3 & (RF3_NO_CONF))
			{
				if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_NO_CONF);
			}

			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->dam = 0;
		}
		else if (em_ptr->r_ptr->flags2 & RF2_EMPTY_MIND)
		{
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags2 |= (RF2_EMPTY_MIND);
			em_ptr->note = _("には完全な耐性がある！", " is immune.");
			em_ptr->dam = 0;
		}
		else if (em_ptr->r_ptr->flags2 & RF2_WEIRD_MIND)
		{
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags2 |= (RF2_WEIRD_MIND);
			em_ptr->note = _("には耐性がある！", " resists!");
			em_ptr->dam /= 3;
		}
		else
		{
			em_ptr->note = _("は精神攻撃を食らった。", " is blasted by psionic energy.");
			em_ptr->note_dies = _("の精神は崩壊し、肉体は抜け殻となった。", " collapses, a mindless husk.");
			if (em_ptr->who > 0)
			{
				em_ptr->do_conf = randint0(4) + 4;
				em_ptr->do_stun = randint0(4) + 4;
			}
			else
			{
				em_ptr->do_conf = randint0(8) + 8;
				em_ptr->do_stun = randint0(8) + 8;
			}
			(void)set_monster_slow(caster_ptr, em_ptr->g_ptr->m_idx, MON_SLOW(em_ptr->m_ptr) + 10);
		}

		break;
	}
	case GF_CAUSE_1:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (!em_ptr->who) msg_format(_("%sを指差して呪いをかけた。", "You point at %s and curse."), em_ptr->m_name);
		if (randint0(100 + (em_ptr->caster_lev / 2)) < (em_ptr->r_ptr->level + 35))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->dam = 0;
		}

		break;
	}
	case GF_CAUSE_2:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (!em_ptr->who) msg_format(_("%sを指差して恐ろしげに呪いをかけた。", "You point at %s and curse horribly."), em_ptr->m_name);

		if (randint0(100 + (em_ptr->caster_lev / 2)) < (em_ptr->r_ptr->level + 35))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->dam = 0;
		}

		break;
	}
	case GF_CAUSE_3:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (!em_ptr->who) msg_format(_("%sを指差し、恐ろしげに呪文を唱えた！", "You point at %s, incanting terribly!"), em_ptr->m_name);

		if (randint0(100 + (em_ptr->caster_lev / 2)) < (em_ptr->r_ptr->level + 35))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->dam = 0;
		}

		break;
	}
	case GF_CAUSE_4:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (!em_ptr->who)
			msg_format(_("%sの秘孔を突いて、「お前は既に死んでいる」と叫んだ。",
				"You point at %s, screaming the word, 'DIE!'."), em_ptr->m_name);

		if ((randint0(100 + (em_ptr->caster_lev / 2)) < (em_ptr->r_ptr->level + 35)) && ((em_ptr->who <= 0) || (em_ptr->m_caster_ptr->r_idx != MON_KENSHIROU)))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->dam = 0;
		}
		break;
	}
	case GF_HAND_DOOM:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flags1 & RF1_UNIQUE)
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->dam = 0;
		}
		else
		{
			if ((em_ptr->who > 0) ? ((em_ptr->caster_lev + randint1(em_ptr->dam)) > (em_ptr->r_ptr->level + 10 + randint1(20))) :
				(((em_ptr->caster_lev / 2) + randint1(em_ptr->dam)) > (em_ptr->r_ptr->level + randint1(200))))
			{
				em_ptr->dam = ((40 + randint1(20)) * em_ptr->m_ptr->hp) / 100;

				if (em_ptr->m_ptr->hp < em_ptr->dam) em_ptr->dam = em_ptr->m_ptr->hp - 1;
			}
			else
			{
				/* todo 乱数で破滅のを弾いた結果が「耐性を持っている」ことになるのはおかしい */
				em_ptr->note = _("は耐性を持っている！", "resists!");
				em_ptr->dam = 0;
			}
		}

		break;
	}
	case GF_CAPTURE:
	{
		int nokori_hp;
		if ((floor_ptr->inside_quest && (quest[floor_ptr->inside_quest].type == QUEST_TYPE_KILL_ALL) && !is_pet(em_ptr->m_ptr)) ||
			(em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) || (em_ptr->r_ptr->flags7 & (RF7_NAZGUL)) || (em_ptr->r_ptr->flags7 & (RF7_UNIQUE2)) || (em_ptr->r_ptr->flags1 & RF1_QUESTOR) || em_ptr->m_ptr->parent_m_idx)
		{
			msg_format(_("%sには効果がなかった。", "%s is unaffected."), em_ptr->m_name);
			em_ptr->skipped = TRUE;
			break;
		}

		if (is_pet(em_ptr->m_ptr)) nokori_hp = em_ptr->m_ptr->maxhp * 4L;
		else if ((caster_ptr->pclass == CLASS_BEASTMASTER) && monster_living(em_ptr->m_ptr->r_idx))
			nokori_hp = em_ptr->m_ptr->maxhp * 3 / 10;
		else
			nokori_hp = em_ptr->m_ptr->maxhp * 3 / 20;

		if (em_ptr->m_ptr->hp >= nokori_hp)
		{
			msg_format(_("もっと弱らせないと。", "You need to weaken %s more."), em_ptr->m_name);
			em_ptr->skipped = TRUE;
		}
		else if (em_ptr->m_ptr->hp < randint0(nokori_hp))
		{
			if (em_ptr->m_ptr->mflag2 & MFLAG2_CHAMELEON) choose_new_monster(caster_ptr, em_ptr->g_ptr->m_idx, FALSE, MON_CHAMELEON);
			msg_format(_("%sを捕えた！", "You capture %^s!"), em_ptr->m_name);
			cap_mon = em_ptr->m_ptr->r_idx;
			cap_mspeed = em_ptr->m_ptr->mspeed;
			cap_hp = em_ptr->m_ptr->hp;
			cap_maxhp = em_ptr->m_ptr->max_maxhp;
			cap_nickname = em_ptr->m_ptr->nickname;
			if (em_ptr->g_ptr->m_idx == caster_ptr->riding)
			{
				if (rakuba(caster_ptr, -1, FALSE))
				{
					msg_format(_("地面に落とされた。", "You have fallen from %s."), em_ptr->m_name);
				}
			}

			delete_monster_idx(caster_ptr, em_ptr->g_ptr->m_idx);

			return GF_SWITCH_TRUE;
		}
		else
		{
			msg_format(_("うまく捕まえられなかった。", "You failed to capture %s."), em_ptr->m_name);
			em_ptr->skipped = TRUE;
		}

		break;
	}
	case GF_ATTACK:
	{
		return (gf_switch_result)py_attack(caster_ptr, em_ptr->y, em_ptr->x, em_ptr->dam);
	}
	case GF_ENGETSU:
	{
		int effect = 0;
		bool done = TRUE;

		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (em_ptr->r_ptr->flags2 & RF2_EMPTY_MIND)
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->dam = 0;
			em_ptr->skipped = TRUE;
			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags2 |= (RF2_EMPTY_MIND);
			break;
		}
		if (MON_CSLEEP(em_ptr->m_ptr))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->dam = 0;
			em_ptr->skipped = TRUE;
			break;
		}

		if (one_in_(5)) effect = 1;
		else if (one_in_(4)) effect = 2;
		else if (one_in_(3)) effect = 3;
		else done = FALSE;

		if (effect == 1)
		{
			if ((em_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
				(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
			{
				em_ptr->note = _("には効果がなかった。", " is unaffected.");
				em_ptr->obvious = FALSE;
			}
			else
			{
				if (set_monster_slow(caster_ptr, em_ptr->g_ptr->m_idx, MON_SLOW(em_ptr->m_ptr) + 50))
				{
					em_ptr->note = _("の動きが遅くなった。", " starts moving slower.");
				}
			}
		}
		else if (effect == 2)
		{
			em_ptr->do_stun = damroll((caster_ptr->lev / 10) + 3, (em_ptr->dam)) + 1;
			if ((em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) ||
				(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
			{
				em_ptr->do_stun = 0;
				em_ptr->note = _("には効果がなかった。", " is unaffected.");
				em_ptr->obvious = FALSE;
			}
		}
		else if (effect == 3)
		{
			if ((em_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
				(em_ptr->r_ptr->flags3 & RF3_NO_SLEEP) ||
				(em_ptr->r_ptr->level > randint1((em_ptr->dam - 10) < 1 ? 1 : (em_ptr->dam - 10)) + 10))
			{
				if (em_ptr->r_ptr->flags3 & RF3_NO_SLEEP)
				{
					if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_NO_SLEEP);
				}

				em_ptr->note = _("には効果がなかった。", " is unaffected.");
				em_ptr->obvious = FALSE;
			}
			else
			{
				/* Go to sleep (much) later */
				em_ptr->note = _("は眠り込んでしまった！", " falls asleep!");
				em_ptr->do_sleep = 500;
			}
		}

		if (!done)
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_GENOCIDE:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		if (genocide_aux(caster_ptr, em_ptr->g_ptr->m_idx, em_ptr->dam, !em_ptr->who, (em_ptr->r_ptr->level + 1) / 2, _("モンスター消滅", "Genocide One")))
		{
			if (em_ptr->seen_msg) msg_format(_("%sは消滅した！", "%^s disappeared!"), em_ptr->m_name);
			chg_virtue(caster_ptr, V_VITALITY, -1);
			return GF_SWITCH_TRUE;
		}

		em_ptr->skipped = TRUE;
		break;
	}
	case GF_PHOTO:
	{
		if (!em_ptr->who)
			msg_format(_("%sを写真に撮った。", "You take a photograph of %s."), em_ptr->m_name);

		if (em_ptr->r_ptr->flags3 & (RF3_HURT_LITE))
		{
			if (em_ptr->seen) em_ptr->obvious = TRUE;

			if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_HURT_LITE);

			em_ptr->note = _("は光に身をすくめた！", " cringes from the light!");
			em_ptr->note_dies = _("は光を受けてしぼんでしまった！", " shrivels away in the light!");
		}
		else
		{
			em_ptr->dam = 0;
		}

		em_ptr->photo = em_ptr->m_ptr->r_idx;
		break;
	}
	case GF_BLOOD_CURSE:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;
		break;
	}
	case GF_CRUSADE:
	{
		bool success = FALSE;
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		if ((em_ptr->r_ptr->flags3 & (RF3_GOOD)) && !floor_ptr->inside_arena)
		{
			if (em_ptr->r_ptr->flags3 & (RF3_NO_CONF)) em_ptr->dam -= 50;
			if (em_ptr->dam < 1) em_ptr->dam = 1;

			if (is_pet(em_ptr->m_ptr))
			{
				em_ptr->note = _("の動きが速くなった。", " starts moving faster.");
				(void)set_monster_fast(caster_ptr, em_ptr->g_ptr->m_idx, MON_FAST(em_ptr->m_ptr) + 100);
				success = TRUE;
			}
			else if ((em_ptr->r_ptr->flags1 & (RF1_QUESTOR)) ||
				(em_ptr->r_ptr->flags1 & (RF1_UNIQUE)) ||
				(em_ptr->m_ptr->mflag2 & MFLAG2_NOPET) ||
				(caster_ptr->cursed & TRC_AGGRAVATE) ||
				((em_ptr->r_ptr->level + 10) > randint1(em_ptr->dam)))
			{
				if (one_in_(4)) em_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
			}
			else
			{
				em_ptr->note = _("を支配した。", " is tamed!");
				set_pet(caster_ptr, em_ptr->m_ptr);
				(void)set_monster_fast(caster_ptr, em_ptr->g_ptr->m_idx, MON_FAST(em_ptr->m_ptr) + 100);

				if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr)) em_ptr->r_ptr->r_flags3 |= (RF3_GOOD);
				success = TRUE;
			}
		}

		if (!success)
		{
			if (!(em_ptr->r_ptr->flags3 & RF3_NO_FEAR))
			{
				em_ptr->do_fear = randint1(90) + 10;
			}
			else if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr))
				em_ptr->r_ptr->r_flags3 |= (RF3_NO_FEAR);
		}

		em_ptr->dam = 0;
		break;
	}
	case GF_WOUNDS:
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		if (randint0(100 + em_ptr->dam) < (em_ptr->r_ptr->level + 50))
		{
			em_ptr->note = _("には効果がなかった。", " is unaffected.");
			em_ptr->dam = 0;
		}
		break;
	}
	default:
	{
		em_ptr->skipped = TRUE;
		em_ptr->dam = 0;
		break;
	}
	}

	return GF_SWITCH_CONTINUE;
}


/*!
 * @brief 完全な耐性を持っていたら、モンスターへの効果処理をスキップする
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return 完全耐性ならCONTINUE、そうでないなら効果処理の結果
 */
static gf_switch_result process_monster_perfect_resistance(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	if (((em_ptr->r_ptr->flagsr & RFR_RES_ALL) == 0) ||
		em_ptr->effect_type == GF_OLD_CLONE ||
		em_ptr->effect_type == GF_STAR_HEAL ||
		em_ptr->effect_type == GF_OLD_HEAL ||
		em_ptr->effect_type == GF_OLD_SPEED ||
		em_ptr->effect_type == GF_CAPTURE ||
		em_ptr->effect_type == GF_PHOTO)
		return switch_effects_monster(caster_ptr, em_ptr);

	em_ptr->note = _("には完全な耐性がある！", " is immune.");
	em_ptr->dam = 0;
	if (is_original_ap_and_seen(caster_ptr, em_ptr->m_ptr))
		em_ptr->r_ptr->r_flagsr |= (RFR_RES_ALL);

	if (em_ptr->effect_type == GF_LITE_WEAK || em_ptr->effect_type == GF_KILL_WALL)
		em_ptr->skipped = TRUE;

	return GF_SWITCH_CONTINUE;
}


/*!
 * @brief ペットの死亡を処理する
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return なし
 */
static void process_pet_death(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	bool sad = is_pet(em_ptr->m_ptr) && !(em_ptr->m_ptr->ml);
	if (em_ptr->known && em_ptr->note)
	{
		monster_desc(caster_ptr, em_ptr->m_name, em_ptr->m_ptr, MD_TRUE_NAME);
		if (em_ptr->see_s_msg) msg_format("%^s%s", em_ptr->m_name, em_ptr->note);
		else caster_ptr->current_floor_ptr->monster_noise = TRUE;
	}

	if (em_ptr->who > 0) monster_gain_exp(caster_ptr, em_ptr->who, em_ptr->m_ptr->r_idx);

	monster_death(caster_ptr, em_ptr->g_ptr->m_idx, FALSE);
	delete_monster_idx(caster_ptr, em_ptr->g_ptr->m_idx);
	if (sad) msg_print(_("少し悲しい気分がした。", "You feel sad for a moment."));
}


/*!
 * @brief モンスターの睡眠を処理する
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return なし
 */
static void process_monster_sleep(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	if (em_ptr->note && em_ptr->seen_msg)
		msg_format("%^s%s", em_ptr->m_name, em_ptr->note);
	else if (em_ptr->see_s_msg)
		message_pain(caster_ptr, em_ptr->g_ptr->m_idx, em_ptr->dam);
	else
		caster_ptr->current_floor_ptr->monster_noise = TRUE;

	if (em_ptr->do_sleep) (void)set_monster_csleep(caster_ptr, em_ptr->g_ptr->m_idx, em_ptr->do_sleep);
}


/*!
 * @brief モンスターの被ダメージを処理する / If another monster did the damage, hurt the monster by hand
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return モンスターIDがプレーヤー自身だった場合FALSE、モンスターだった場合TRUE
 */
static bool process_monster_damage(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	if (em_ptr->who <= 0) return FALSE;

	if (caster_ptr->health_who == em_ptr->g_ptr->m_idx) caster_ptr->redraw |= (PR_HEALTH);
	if (caster_ptr->riding == em_ptr->g_ptr->m_idx) caster_ptr->redraw |= (PR_UHEALTH);

	(void)set_monster_csleep(caster_ptr, em_ptr->g_ptr->m_idx, 0);
	em_ptr->m_ptr->hp -= em_ptr->dam;
	if (em_ptr->m_ptr->hp < 0) process_pet_death(caster_ptr, em_ptr);
	else process_monster_sleep(caster_ptr, em_ptr);

	return TRUE;
}


/*!
 * @brief 不潔な病人の治療処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return 大賞モンスターが不潔な病人だった場合はTRUE、それ以外はFALSE
 */
static bool heal_leaper(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	if (!em_ptr->heal_leper) return FALSE;

	if (em_ptr->seen_msg)
		msg_print(_("不潔な病人は病気が治った！", "The Mangy looking leper is healed!"));

	if (record_named_pet && is_pet(em_ptr->m_ptr) && em_ptr->m_ptr->nickname)
	{
		char m2_name[MAX_NLEN];
		monster_desc(caster_ptr, m2_name, em_ptr->m_ptr, MD_INDEF_VISIBLE);
		exe_write_diary(caster_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_HEAL_LEPER, m2_name);
	}

	delete_monster_idx(caster_ptr, em_ptr->g_ptr->m_idx);
	return TRUE;
}


/*!
 * @brief モンスターの恐慌処理 / If the player did it, give him experience, check fear
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return モンスターが死んだらTRUE、生きていたらFALSE
 */
static bool process_monster_fear(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	bool fear = FALSE;
	if (mon_take_hit(caster_ptr, em_ptr->g_ptr->m_idx, em_ptr->dam, &fear, em_ptr->note_dies))
		return TRUE;

	if (em_ptr->do_sleep)
		anger_monster(caster_ptr, em_ptr->m_ptr);

	if (em_ptr->note && em_ptr->seen_msg)
		msg_format(_("%s%s", "%^s%s"), em_ptr->m_name, em_ptr->note);
	else if (em_ptr->known && (em_ptr->dam || !em_ptr->do_fear))
		message_pain(caster_ptr, em_ptr->g_ptr->m_idx, em_ptr->dam);

	if (((em_ptr->dam > 0) || em_ptr->get_angry) && !em_ptr->do_sleep)
		anger_monster(caster_ptr, em_ptr->m_ptr);

	if ((fear || em_ptr->do_fear) && em_ptr->seen)
	{
		sound(SOUND_FLEE);
		msg_format(_("%^sは恐怖して逃げ出した！", "%^s flees in terror!"), em_ptr->m_name);
	}

	return FALSE;
}


/*!
 * todo 睡眠処理があるので、死に際とは言えない。適切な関数名に要修正
 * @brief モンスターの死に際処理 (魔力吸収を除く)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return なし
 */
static void process_monster_last_moment(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	if (em_ptr->effect_type == GF_DRAIN_MANA) return;

	if (process_monster_damage(caster_ptr, em_ptr)) return;
	if (heal_leaper(caster_ptr, em_ptr)) return;
	if (process_monster_fear(caster_ptr, em_ptr)) return;

	if (em_ptr->do_sleep) (void)set_monster_csleep(caster_ptr, em_ptr->g_ptr->m_idx, em_ptr->do_sleep);
}


/*!
 * @brief モンスターの朦朧値を蓄積させる
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @param stun_damage 朦朧値
 * @return なし
 */
static void pile_monster_stun(player_type *caster_ptr, effect_monster_type *em_ptr, int *stun_damage)
{
	if ((em_ptr->do_stun == 0) ||
		(em_ptr->r_ptr->flagsr & (RFR_RES_SOUN | RFR_RES_WALL)) ||
		(em_ptr->r_ptr->flags3 & RF3_NO_STUN))
		return *stun_damage;

	if (em_ptr->seen) em_ptr->obvious = TRUE;

	if (MON_STUNNED(em_ptr->m_ptr))
	{
		em_ptr->note = _("はひどくもうろうとした。", " is more dazed.");
		*stun_damage = MON_STUNNED(em_ptr->m_ptr) + (em_ptr->do_stun / 2);
	}
	else
	{
		em_ptr->note = _("はもうろうとした。", " is dazed.");
		*stun_damage = em_ptr->do_stun;
	}

	(void)set_monster_stunned(caster_ptr, em_ptr->g_ptr->m_idx, *stun_damage);
	em_ptr->get_angry = TRUE;
}


/*!
 * @brief モンスターの混乱値を蓄積させる
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @param stun_damage 混乱値
 * @return なし
 */
static void pile_monster_conf(player_type *caster_ptr, effect_monster_type *em_ptr, int *conf_damage)
{
	if ((em_ptr->do_conf == 0) ||
		(em_ptr->r_ptr->flags3 & RF3_NO_CONF) ||
		(em_ptr->r_ptr->flagsr & RFR_EFF_RES_CHAO_MASK))
		return;

	if (em_ptr->seen) em_ptr->obvious = TRUE;

	if (MON_CONFUSED(em_ptr->m_ptr))
	{
		em_ptr->note = _("はさらに混乱したようだ。", " looks more confused.");
		*conf_damage = MON_CONFUSED(em_ptr->m_ptr) + (em_ptr->do_conf / 2);
	}
	else
	{
		em_ptr->note = _("は混乱したようだ。", " looks confused.");
		*conf_damage = em_ptr->do_conf;
	}

	(void)set_monster_confused(caster_ptr, em_ptr->g_ptr->m_idx, *conf_damage);
	em_ptr->get_angry = TRUE;
}


/*!
 * @brief モンスターを衰弱させる
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return なし
 */
static void process_monster_weakening(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	if (em_ptr->do_time == 0) return;

	if (em_ptr->seen) em_ptr->obvious = TRUE;

	if (em_ptr->do_time >= em_ptr->m_ptr->maxhp) em_ptr->do_time = em_ptr->m_ptr->maxhp - 1;

	if (em_ptr->do_time)
	{
		em_ptr->note = _("は弱くなったようだ。", " seems weakened.");
		em_ptr->m_ptr->maxhp -= em_ptr->do_time;
		if ((em_ptr->m_ptr->hp - em_ptr->dam) > em_ptr->m_ptr->maxhp) em_ptr->dam = em_ptr->m_ptr->hp - em_ptr->m_ptr->maxhp;
	}

	em_ptr->get_angry = TRUE;
}


/*!
 * @brief モンスターを変身させる
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param em_ptr モンスター効果構造体への参照ポインタ
 * @return なし
 */
static void process_monster_polymorph(player_type *caster_ptr, effect_monster_type *em_ptr)
{
	if (!em_ptr->do_polymorph || (randint1(90) <= em_ptr->r_ptr->level))
		return;

	if (polymorph_monster(caster_ptr, em_ptr->y, em_ptr->x))
	{
		if (em_ptr->seen) em_ptr->obvious = TRUE;

		em_ptr->note = _("が変身した！", " changes!");
		em_ptr->dam = 0;
	}

	em_ptr->m_ptr = &caster_ptr->current_floor_ptr->m_list[em_ptr->g_ptr->m_idx];
	em_ptr->r_ptr = &r_info[em_ptr->m_ptr->r_idx];
}


/*!
 * @brief 汎用的なビーム/ボルト/ボール系によるモンスターへの効果処理 / Handle a beam/bolt/ball causing damage to a monster.
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param who 魔法を発動したモンスター(0ならばプレイヤー) / Index of "source" monster (zero for "player")
 * @param r 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion (0 = beam/bolt, 1 to 9 = ball)
 * @param y 目標y座標 / Target y location (or location to travel "towards")
 * @param x 目標x座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or player)
 * @param effect_type 効果属性 / Type of damage to apply to monsters (and objects)
 * @param flag 効果フラグ
 * @param see_s_msg TRUEならばメッセージを表示する
 * @return 何か一つでも効力があればTRUEを返す / TRUE if any "effects" of the projection were observed, else FALSE
 */
bool affect_monster(player_type *caster_ptr, MONSTER_IDX who, POSITION r, POSITION y, POSITION x, HIT_POINT dam, EFFECT_ID effect_type, BIT_FLAGS flag, bool see_s_msg)
{
	effect_monster_type tmp_effect;
	effect_monster_type *em_ptr = &tmp_effect;
	initialize_effect_monster(caster_ptr, em_ptr, who, r, y, x, dam, effect_type, flag, see_s_msg);
	if (!is_never_effect(caster_ptr, em_ptr)) return FALSE;

	em_ptr->dam = (em_ptr->dam + em_ptr->r) / (em_ptr->r + 1);
	monster_desc(caster_ptr, em_ptr->m_name, em_ptr->m_ptr, 0);
	monster_desc(caster_ptr, em_ptr->m_poss, em_ptr->m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE);
	if (caster_ptr->riding && (em_ptr->g_ptr->m_idx == caster_ptr->riding))
		disturb(caster_ptr, TRUE, TRUE);

	gf_switch_result result = process_monster_perfect_resistance(caster_ptr, em_ptr);
	if (result != GF_SWITCH_CONTINUE) return (bool)result;

	if (em_ptr->skipped) return FALSE;

	if ((em_ptr->r_ptr->flags1 & RF1_UNIQUE) || 
		(em_ptr->r_ptr->flags1 & RF1_QUESTOR) || 
		(caster_ptr->riding && (em_ptr->g_ptr->m_idx == caster_ptr->riding)))
		em_ptr->do_polymorph = FALSE;

	if (((em_ptr->r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) || (em_ptr->r_ptr->flags7 & RF7_NAZGUL)) &&
		!caster_ptr->phase_out &&
		em_ptr->who &&
		(em_ptr->dam > em_ptr->m_ptr->hp))
			em_ptr->dam = em_ptr->m_ptr->hp;

	if (!em_ptr->who && em_ptr->slept)
	{
		if (!(em_ptr->r_ptr->flags3 & RF3_EVIL) || one_in_(5)) chg_virtue(caster_ptr, V_COMPASSION, -1);
		if (!(em_ptr->r_ptr->flags3 & RF3_EVIL) || one_in_(5)) chg_virtue(caster_ptr, V_HONOUR, -1);
	}

	int tmp_damage = em_ptr->dam;
	em_ptr->dam = mon_damage_mod(caster_ptr, em_ptr->m_ptr, em_ptr->dam, (bool)(em_ptr->effect_type == GF_PSY_SPEAR));
	if ((tmp_damage > 0) && (em_ptr->dam == 0)) em_ptr->note = _("はダメージを受けていない。", " is unharmed.");

	if (em_ptr->dam > em_ptr->m_ptr->hp)
	{
		em_ptr->note = em_ptr->note_dies;
	}
	else
	{
		pile_monster_stun(caster_ptr, em_ptr, &tmp_damage);
		pile_monster_conf(caster_ptr, em_ptr, &tmp_damage);
		process_monster_weakening(caster_ptr, em_ptr, &tmp_damage);
		process_monster_polymorph(caster_ptr, em_ptr);

		if (em_ptr->do_dist)
		{
			if (em_ptr->seen) em_ptr->obvious = TRUE;

			em_ptr->note = _("が消え去った！", " disappears!");

			if (!em_ptr->who) chg_virtue(caster_ptr, V_VALOUR, -1);

			teleport_away(caster_ptr, em_ptr->g_ptr->m_idx, em_ptr->do_dist,
				(!em_ptr->who ? TELEPORT_DEC_VALOUR : 0L) | TELEPORT_PASSIVE);

			em_ptr->y = em_ptr->m_ptr->fy;
			em_ptr->x = em_ptr->m_ptr->fx;
			em_ptr->g_ptr = &caster_ptr->current_floor_ptr->grid_array[em_ptr->y][em_ptr->x];
		}

		if (em_ptr->do_fear)
		{
			(void)set_monster_monfear(caster_ptr, em_ptr->g_ptr->m_idx, MON_MONFEAR(em_ptr->m_ptr) + em_ptr->do_fear);
			em_ptr->get_angry = TRUE;
		}
	}

	process_monster_last_moment(caster_ptr, em_ptr);
	if ((em_ptr->effect_type == GF_BLOOD_CURSE) && one_in_(4))
	{
		blood_curse_to_enemy(caster_ptr, em_ptr->who);
	}

	if (caster_ptr->phase_out)
	{
		caster_ptr->health_who = em_ptr->g_ptr->m_idx;
		caster_ptr->redraw |= (PR_HEALTH);
		handle_stuff(caster_ptr);
	}

	if (em_ptr->m_ptr->r_idx) update_monster(caster_ptr, em_ptr->g_ptr->m_idx, FALSE);

	lite_spot(caster_ptr, em_ptr->y, em_ptr->x);
	if ((caster_ptr->monster_race_idx == em_ptr->m_ptr->r_idx) && (em_ptr->seen || !em_ptr->m_ptr->r_idx))
	{
		caster_ptr->window |= (PW_MONSTER);
	}

	if ((em_ptr->dam > 0) && !is_pet(em_ptr->m_ptr) && !is_friendly(em_ptr->m_ptr))
	{
		if (!em_ptr->who)
		{
			if (!(em_ptr->flag & PROJECT_NO_HANGEKI))
			{
				set_target(em_ptr->m_ptr, monster_target_y, monster_target_x);
			}
		}
		else if ((em_ptr->who > 0) && is_pet(em_ptr->m_caster_ptr) && !player_bold(caster_ptr, em_ptr->m_ptr->target_y, em_ptr->m_ptr->target_x))
		{
			set_target(em_ptr->m_ptr, em_ptr->m_caster_ptr->fy, em_ptr->m_caster_ptr->fx);
		}
	}

	if (caster_ptr->riding && (caster_ptr->riding == em_ptr->g_ptr->m_idx) && (em_ptr->dam > 0))
	{
		if (em_ptr->m_ptr->hp > em_ptr->m_ptr->maxhp / 3) em_ptr->dam = (em_ptr->dam + 1) / 2;
		rakubadam_m = (em_ptr->dam > 200) ? 200 : em_ptr->dam;
	}

	if (em_ptr->photo)
	{
		object_type *q_ptr;
		object_type forge;
		q_ptr = &forge;
		object_prep(q_ptr, lookup_kind(TV_STATUE, SV_PHOTO));
		q_ptr->pval = em_ptr->photo;
		q_ptr->ident |= (IDENT_FULL_KNOWN);
		(void)drop_near(caster_ptr, q_ptr, -1, caster_ptr->y, caster_ptr->x);
	}

	project_m_n++;
	project_m_x = em_ptr->x;
	project_m_y = em_ptr->y;
	return (em_ptr->obvious);
}
