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

/*!
 * @brief ビーム/ボルト/ボール系魔法によるモンスターへの効果があるかないかを判定する
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param effect_monster_ptr モンスター効果構造体への参照ポインタ
 * @return 効果が何もないならFALSE、何かあるならTRUE
 */
static bool is_never_effect(player_type *caster_ptr, effect_monster *effect_monster_ptr)
{
	if (!effect_monster_ptr->g_ptr->m_idx) return FALSE;
	if (effect_monster_ptr->who && (effect_monster_ptr->g_ptr->m_idx == effect_monster_ptr->who)) return FALSE;
	if ((effect_monster_ptr->g_ptr->m_idx == caster_ptr->riding) &&
		!effect_monster_ptr->who &&
		!(effect_monster_ptr->effect_type == GF_OLD_HEAL) &&
		!(effect_monster_ptr->effect_type == GF_OLD_SPEED) &&
		!(effect_monster_ptr->effect_type == GF_STAR_HEAL))
		return FALSE;
	if (sukekaku && ((effect_monster_ptr->m_ptr->r_idx == MON_SUKE) || (effect_monster_ptr->m_ptr->r_idx == MON_KAKU))) return FALSE;
	if (effect_monster_ptr->m_ptr->hp < 0) return FALSE;

	return TRUE;
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
	floor_type *floor_ptr = caster_ptr->current_floor_ptr;
	effect_monster tmp_effect;
	effect_monster *effect_monster_ptr = &tmp_effect;
	initialize_effect_monster(caster_ptr, effect_monster_ptr, who, r, y, x, dam, effect_type, flag, see_s_msg);
	if (!is_never_effect(caster_ptr, effect_monster_ptr)) return FALSE;

	effect_monster_ptr->dam = (effect_monster_ptr->dam + effect_monster_ptr->r) / (effect_monster_ptr->r + 1);
	monster_desc(caster_ptr, effect_monster_ptr->m_name, effect_monster_ptr->m_ptr, 0);
	monster_desc(caster_ptr, effect_monster_ptr->m_poss, effect_monster_ptr->m_ptr, MD_PRON_VISIBLE | MD_POSSESSIVE);
	if (caster_ptr->riding && (effect_monster_ptr->g_ptr->m_idx == caster_ptr->riding))
		disturb(caster_ptr, TRUE, TRUE);

	if (effect_monster_ptr->r_ptr->flagsr & RFR_RES_ALL &&
		effect_monster_ptr->effect_type != GF_OLD_CLONE && effect_monster_ptr->effect_type != GF_STAR_HEAL && effect_monster_ptr->effect_type != GF_OLD_HEAL
		&& effect_monster_ptr->effect_type != GF_OLD_SPEED && effect_monster_ptr->effect_type != GF_CAPTURE && effect_monster_ptr->effect_type != GF_PHOTO)
	{
		effect_monster_ptr->note = _("には完全な耐性がある！", " is immune.");
		effect_monster_ptr->dam = 0;
		if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= (RFR_RES_ALL);
		if (effect_monster_ptr->effect_type == GF_LITE_WEAK || effect_monster_ptr->effect_type == GF_KILL_WALL) effect_monster_ptr->skipped = TRUE;
	}
	else
	{
		switch (effect_monster_ptr->effect_type)
		{
		case GF_MISSILE:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			break;
		}
		case GF_ACID:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flagsr & RFR_IM_ACID)
			{
				effect_monster_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
				effect_monster_ptr->dam /= 9;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= (RFR_IM_ACID);
			}
			break;
		}
		case GF_ELEC:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flagsr & RFR_IM_ELEC)
			{
				effect_monster_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
				effect_monster_ptr->dam /= 9;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= (RFR_IM_ELEC);
			}
			break;
		}
		case GF_FIRE:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flagsr & RFR_IM_FIRE)
			{
				effect_monster_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
				effect_monster_ptr->dam /= 9;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= (RFR_IM_FIRE);
			}
			else if (effect_monster_ptr->r_ptr->flags3 & (RF3_HURT_FIRE))
			{
				effect_monster_ptr->note = _("はひどい痛手をうけた。", " is hit hard.");
				effect_monster_ptr->dam *= 2;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_HURT_FIRE);
			}
			break;
		}
		case GF_COLD:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flagsr & RFR_IM_COLD)
			{
				effect_monster_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
				effect_monster_ptr->dam /= 9;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= (RFR_IM_COLD);
			}
			else if (effect_monster_ptr->r_ptr->flags3 & (RF3_HURT_COLD))
			{
				effect_monster_ptr->note = _("はひどい痛手をうけた。", " is hit hard.");
				effect_monster_ptr->dam *= 2;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_HURT_COLD);
			}
			break;
		}
		case GF_POIS:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flagsr & RFR_IM_POIS)
			{
				effect_monster_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
				effect_monster_ptr->dam /= 9;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= (RFR_IM_POIS);
			}
			break;
		}
		case GF_NUKE:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flagsr & RFR_IM_POIS)
			{
				effect_monster_ptr->note = _("には耐性がある。", " resists.");
				effect_monster_ptr->dam *= 3; effect_monster_ptr->dam /= randint1(6) + 6;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= (RFR_IM_POIS);
			}
			else if (one_in_(3)) effect_monster_ptr->do_poly = TRUE;
			break;
		}
		case GF_HELL_FIRE:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flags3 & RF3_GOOD)
			{
				effect_monster_ptr->note = _("はひどい痛手をうけた。", " is hit hard.");
				effect_monster_ptr->dam *= 2;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_GOOD);
			}
			break;
		}
		case GF_HOLY_FIRE:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flags3 & RF3_EVIL)
			{
				effect_monster_ptr->dam *= 2;
				effect_monster_ptr->note = _("はひどい痛手をうけた。", " is hit hard.");
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= RF3_EVIL;
			}
			else
			{
				effect_monster_ptr->note = _("には耐性がある。", " resists.");
				effect_monster_ptr->dam *= 3; effect_monster_ptr->dam /= randint1(6) + 6;
			}
			break;
		}
		case GF_ARROW:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			break;
		}
		case GF_PLASMA:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flagsr & RFR_RES_PLAS)
			{
				effect_monster_ptr->note = _("には耐性がある。", " resists.");
				effect_monster_ptr->dam *= 3; effect_monster_ptr->dam /= randint1(6) + 6;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= (RFR_RES_PLAS);
			}

			break;
		}
		case GF_NETHER:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flagsr & RFR_RES_NETH)
			{
				if (effect_monster_ptr->r_ptr->flags3 & RF3_UNDEAD)
				{
					effect_monster_ptr->note = _("には完全な耐性がある！", " is immune.");
					effect_monster_ptr->dam = 0;
					if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);
				}
				else
				{
					effect_monster_ptr->note = _("には耐性がある。", " resists.");
					effect_monster_ptr->dam *= 3; effect_monster_ptr->dam /= randint1(6) + 6;
				}
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= (RFR_RES_NETH);
			}
			else if (effect_monster_ptr->r_ptr->flags3 & RF3_EVIL)
			{
				effect_monster_ptr->note = _("はいくらか耐性を示した。", " resists somewhat.");
				effect_monster_ptr->dam /= 2;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_EVIL);
			}

			break;
		}
		case GF_WATER:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flagsr & RFR_RES_WATE)
			{
				if ((effect_monster_ptr->m_ptr->r_idx == MON_WATER_ELEM) || (effect_monster_ptr->m_ptr->r_idx == MON_UNMAKER))
				{
					effect_monster_ptr->note = _("には完全な耐性がある！", " is immune.");
					effect_monster_ptr->dam = 0;
				}
				else
				{
					effect_monster_ptr->note = _("には耐性がある。", " resists.");
					effect_monster_ptr->dam *= 3; effect_monster_ptr->dam /= randint1(6) + 6;
				}
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= (RFR_RES_WATE);
			}

			break;
		}
		case GF_CHAOS:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flagsr & RFR_RES_CHAO)
			{
				effect_monster_ptr->note = _("には耐性がある。", " resists.");
				effect_monster_ptr->dam *= 3; effect_monster_ptr->dam /= randint1(6) + 6;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= (RFR_RES_CHAO);
			}
			else if ((effect_monster_ptr->r_ptr->flags3 & RF3_DEMON) && one_in_(3))
			{
				effect_monster_ptr->note = _("はいくらか耐性を示した。", " resists somewhat.");
				effect_monster_ptr->dam *= 3; effect_monster_ptr->dam /= randint1(6) + 6;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_DEMON);
			}
			else
			{
				effect_monster_ptr->do_poly = TRUE;
				effect_monster_ptr->do_conf = (5 + randint1(11) + effect_monster_ptr->r) / (effect_monster_ptr->r + 1);
			}

			break;
		}
		case GF_SHARDS:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flagsr & RFR_RES_SHAR)
			{
				effect_monster_ptr->note = _("には耐性がある。", " resists.");
				effect_monster_ptr->dam *= 3; effect_monster_ptr->dam /= randint1(6) + 6;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= (RFR_RES_SHAR);
			}

			break;
		}
		case GF_ROCKET:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flagsr & RFR_RES_SHAR)
			{
				effect_monster_ptr->note = _("はいくらか耐性を示した。", " resists somewhat.");
				effect_monster_ptr->dam /= 2;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= (RFR_RES_SHAR);
			}

			break;
		}
		case GF_SOUND:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flagsr & RFR_RES_SOUN)
			{
				effect_monster_ptr->note = _("には耐性がある。", " resists.");
				effect_monster_ptr->dam *= 2; effect_monster_ptr->dam /= randint1(6) + 6;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= (RFR_RES_SOUN);
			}
			else
				effect_monster_ptr->do_stun = (10 + randint1(15) + effect_monster_ptr->r) / (effect_monster_ptr->r + 1);

			break;
		}
		case GF_CONFUSION:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flags3 & RF3_NO_CONF)
			{
				effect_monster_ptr->note = _("には耐性がある。", " resists.");
				effect_monster_ptr->dam *= 3; effect_monster_ptr->dam /= randint1(6) + 6;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_NO_CONF);
			}
			else
				effect_monster_ptr->do_conf = (10 + randint1(15) + effect_monster_ptr->r) / (effect_monster_ptr->r + 1);

			break;
		}
		case GF_DISENCHANT:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flagsr & RFR_RES_DISE)
			{
				effect_monster_ptr->note = _("には耐性がある。", " resists.");
				effect_monster_ptr->dam *= 3; effect_monster_ptr->dam /= randint1(6) + 6;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= (RFR_RES_DISE);
			}

			break;
		}
		case GF_NEXUS:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flagsr & RFR_RES_NEXU)
			{
				effect_monster_ptr->note = _("には耐性がある。", " resists.");
				effect_monster_ptr->dam *= 3; effect_monster_ptr->dam /= randint1(6) + 6;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= (RFR_RES_NEXU);
			}

			break;
		}
		case GF_FORCE:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flagsr & RFR_RES_WALL)
			{
				effect_monster_ptr->note = _("には耐性がある。", " resists.");
				effect_monster_ptr->dam *= 3; effect_monster_ptr->dam /= randint1(6) + 6;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= (RFR_RES_WALL);
			}
			else
				effect_monster_ptr->do_stun = (randint1(15) + effect_monster_ptr->r) / (effect_monster_ptr->r + 1);

			break;
		}
		case GF_INERTIAL:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flagsr & RFR_RES_INER)
			{
				effect_monster_ptr->note = _("には耐性がある。", " resists.");
				effect_monster_ptr->dam *= 3; effect_monster_ptr->dam /= randint1(6) + 6;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= (RFR_RES_INER);
			}
			else
			{
				/* Powerful monsters can resist */
				if ((effect_monster_ptr->r_ptr->flags1 & (RF1_UNIQUE)) ||
					(effect_monster_ptr->r_ptr->level > randint1((effect_monster_ptr->dam - 10) < 1 ? 1 : (effect_monster_ptr->dam - 10)) + 10))
				{
					effect_monster_ptr->obvious = FALSE;
				}
				/* Normal monsters slow down */
				else
				{
					if (set_monster_slow(caster_ptr, effect_monster_ptr->g_ptr->m_idx, MON_SLOW(effect_monster_ptr->m_ptr) + 50))
					{
						effect_monster_ptr->note = _("の動きが遅くなった。", " starts moving slower.");
					}
				}
			}

			break;
		}
		case GF_TIME:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flagsr & RFR_RES_TIME)
			{
				effect_monster_ptr->note = _("には耐性がある。", " resists.");
				effect_monster_ptr->dam *= 3; effect_monster_ptr->dam /= randint1(6) + 6;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= (RFR_RES_TIME);
			}
			else
				effect_monster_ptr->do_time = (effect_monster_ptr->dam + 1) / 2;

			break;
		}
		case GF_GRAVITY:
		{
			bool resist_tele = FALSE;

			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flagsr & RFR_RES_TELE)
			{
				if (effect_monster_ptr->r_ptr->flags1 & (RF1_UNIQUE))
				{
					if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
					effect_monster_ptr->note = _("には効果がなかった。", " is unaffected!");
					resist_tele = TRUE;
				}
				else if (effect_monster_ptr->r_ptr->level > randint1(100))
				{
					if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
					effect_monster_ptr->note = _("には耐性がある！", " resists!");
					resist_tele = TRUE;
				}
			}

			if (!resist_tele) effect_monster_ptr->do_dist = 10;
			else effect_monster_ptr->do_dist = 0;

			if (caster_ptr->riding && (effect_monster_ptr->g_ptr->m_idx == caster_ptr->riding)) effect_monster_ptr->do_dist = 0;

			if (effect_monster_ptr->r_ptr->flagsr & RFR_RES_GRAV)
			{
				effect_monster_ptr->note = _("には耐性がある！", " resists!");
				effect_monster_ptr->dam *= 3; effect_monster_ptr->dam /= randint1(6) + 6;
				effect_monster_ptr->do_dist = 0;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= (RFR_RES_GRAV);
			}
			else
			{
				/* 1. slowness */
				/* Powerful monsters can resist */
				if ((effect_monster_ptr->r_ptr->flags1 & (RF1_UNIQUE)) ||
					(effect_monster_ptr->r_ptr->level > randint1((effect_monster_ptr->dam - 10) < 1 ? 1 : (effect_monster_ptr->dam - 10)) + 10))
				{
					effect_monster_ptr->obvious = FALSE;
				}
				/* Normal monsters slow down */
				else
				{
					if (set_monster_slow(caster_ptr, effect_monster_ptr->g_ptr->m_idx, MON_SLOW(effect_monster_ptr->m_ptr) + 50))
					{
						effect_monster_ptr->note = _("の動きが遅くなった。", " starts moving slower.");
					}
				}

				/* 2. stun */
				effect_monster_ptr->do_stun = damroll((effect_monster_ptr->caster_lev / 20) + 3, (effect_monster_ptr->dam)) + 1;

				/* Attempt a saving throw */
				if ((effect_monster_ptr->r_ptr->flags1 & (RF1_UNIQUE)) ||
					(effect_monster_ptr->r_ptr->level > randint1((effect_monster_ptr->dam - 10) < 1 ? 1 : (effect_monster_ptr->dam - 10)) + 10))
				{
					/* Resist */
					effect_monster_ptr->do_stun = 0;
					/* No effect_monster_ptr->obvious effect */
					effect_monster_ptr->note = _("には効果がなかった。", " is unaffected!");
					effect_monster_ptr->obvious = FALSE;
				}
			}

			break;
		}
		case GF_MANA:
		case GF_SEEKER:
		case GF_SUPER_RAY:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			break;
		}
		case GF_DISINTEGRATE:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flags3 & RF3_HURT_ROCK)
			{
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_HURT_ROCK);
				effect_monster_ptr->note = _("の皮膚がただれた！", " loses some skin!");
				effect_monster_ptr->note_dies = _("は蒸発した！", " evaporates!");
				effect_monster_ptr->dam *= 2;
			}

			break;
		}
		case GF_PSI:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (!(los(caster_ptr, effect_monster_ptr->m_ptr->fy, effect_monster_ptr->m_ptr->fx, caster_ptr->y, caster_ptr->x)))
			{
				if (effect_monster_ptr->seen_msg)
					msg_format(_("%sはあなたが見えないので影響されない！", "%^s can't see you, and isn't affected!"), effect_monster_ptr->m_name);
				effect_monster_ptr->skipped = TRUE;
				break;
			}

			if (effect_monster_ptr->r_ptr->flags2 & RF2_EMPTY_MIND)
			{
				effect_monster_ptr->dam = 0;
				effect_monster_ptr->note = _("には完全な耐性がある！", " is immune.");
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags2 |= (RF2_EMPTY_MIND);

			}
			else if ((effect_monster_ptr->r_ptr->flags2 & (RF2_STUPID | RF2_WEIRD_MIND)) ||
				(effect_monster_ptr->r_ptr->flags3 & RF3_ANIMAL) ||
				(effect_monster_ptr->r_ptr->level > randint1(3 * effect_monster_ptr->dam)))
			{
				effect_monster_ptr->note = _("には耐性がある！", " resists!");
				effect_monster_ptr->dam /= 3;

				/*
				 * Powerful demons & undead can turn a mindcrafter's
				 * attacks back on them
				 */
				if ((effect_monster_ptr->r_ptr->flags3 & (RF3_UNDEAD | RF3_DEMON)) &&
					(effect_monster_ptr->r_ptr->level > caster_ptr->lev / 2) &&
					one_in_(2))
				{
					effect_monster_ptr->note = NULL;
					msg_format(_("%^sの堕落した精神は攻撃を跳ね返した！",
						(effect_monster_ptr->seen ? "%^s's corrupted mind backlashes your attack!" :
							"%^ss corrupted mind backlashes your attack!")), effect_monster_ptr->m_name);

					if ((randint0(100 + effect_monster_ptr->r_ptr->level / 2) < caster_ptr->skill_sav) && !CHECK_MULTISHADOW(caster_ptr))
					{
						msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
					}
					else
					{
						/* Injure +/- confusion */
						monster_desc(caster_ptr, effect_monster_ptr->killer, effect_monster_ptr->m_ptr, MD_WRONGDOER_NAME);
						take_hit(caster_ptr, DAMAGE_ATTACK, effect_monster_ptr->dam, effect_monster_ptr->killer, -1);  /* has already been /3 */
						if (one_in_(4) && !CHECK_MULTISHADOW(caster_ptr))
						{
							switch (randint1(4))
							{
							case 1:
								set_confused(caster_ptr, caster_ptr->confused + 3 + randint1(effect_monster_ptr->dam));
								break;
							case 2:
								set_stun(caster_ptr, caster_ptr->stun + randint1(effect_monster_ptr->dam));
								break;
							case 3:
							{
								if (effect_monster_ptr->r_ptr->flags3 & RF3_NO_FEAR)
									effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
								else
									set_afraid(caster_ptr, caster_ptr->afraid + 3 + randint1(effect_monster_ptr->dam));
								break;
							}
							default:
								if (!caster_ptr->free_act)
									(void)set_paralyzed(caster_ptr, caster_ptr->paralyzed + randint1(effect_monster_ptr->dam));
								break;
							}
						}
					}

					effect_monster_ptr->dam = 0;
				}
			}

			if ((effect_monster_ptr->dam > 0) && one_in_(4))
			{
				switch (randint1(4))
				{
				case 1:
					effect_monster_ptr->do_conf = 3 + randint1(effect_monster_ptr->dam);
					break;
				case 2:
					effect_monster_ptr->do_stun = 3 + randint1(effect_monster_ptr->dam);
					break;
				case 3:
					effect_monster_ptr->do_fear = 3 + randint1(effect_monster_ptr->dam);
					break;
				default:
					effect_monster_ptr->note = _("は眠り込んでしまった！", " falls asleep!");
					effect_monster_ptr->do_sleep = 3 + randint1(effect_monster_ptr->dam);
					break;
				}
			}

			effect_monster_ptr->note_dies = _("の精神は崩壊し、肉体は抜け殻となった。", " collapses, a mindless husk.");
			break;
		}
		case GF_PSI_DRAIN:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flags2 & RF2_EMPTY_MIND)
			{
				effect_monster_ptr->dam = 0;
				effect_monster_ptr->note = _("には完全な耐性がある！", " is immune.");
			}
			else if ((effect_monster_ptr->r_ptr->flags2 & (RF2_STUPID | RF2_WEIRD_MIND)) ||
				(effect_monster_ptr->r_ptr->flags3 & RF3_ANIMAL) ||
				(effect_monster_ptr->r_ptr->level > randint1(3 * effect_monster_ptr->dam)))
			{
				effect_monster_ptr->note = _("には耐性がある！", " resists!");
				effect_monster_ptr->dam /= 3;

				/*
				 * Powerful demons & undead can turn a mindcrafter's
				 * attacks back on them
				 */
				if ((effect_monster_ptr->r_ptr->flags3 & (RF3_UNDEAD | RF3_DEMON)) &&
					(effect_monster_ptr->r_ptr->level > caster_ptr->lev / 2) &&
					(one_in_(2)))
				{
					effect_monster_ptr->note = NULL;
					msg_format(_("%^sの堕落した精神は攻撃を跳ね返した！",
						(effect_monster_ptr->seen ? "%^s's corrupted mind backlashes your attack!" :
							"%^ss corrupted mind backlashes your attack!")), effect_monster_ptr->m_name);
					if ((randint0(100 + effect_monster_ptr->r_ptr->level / 2) < caster_ptr->skill_sav) && !CHECK_MULTISHADOW(caster_ptr))
					{
						msg_print(_("あなたは効力を跳ね返した！", "You resist the effects!"));
					}
					else
					{
						monster_desc(caster_ptr, effect_monster_ptr->killer, effect_monster_ptr->m_ptr, MD_WRONGDOER_NAME);
						if (!CHECK_MULTISHADOW(caster_ptr))
						{
							msg_print(_("超能力パワーを吸いとられた！", "Your psychic energy is drained!"));
							caster_ptr->csp -= damroll(5, effect_monster_ptr->dam) / 2;
							if (caster_ptr->csp < 0) caster_ptr->csp = 0;
							caster_ptr->redraw |= PR_MANA;
							caster_ptr->window |= (PW_SPELL);
						}
						take_hit(caster_ptr, DAMAGE_ATTACK, effect_monster_ptr->dam, effect_monster_ptr->killer, -1);  /* has already been /3 */
					}

					effect_monster_ptr->dam = 0;
				}
			}
			else if (effect_monster_ptr->dam > 0)
			{
				int b = damroll(5, effect_monster_ptr->dam) / 4;
				concptr str = (caster_ptr->pclass == CLASS_MINDCRAFTER) ? _("超能力パワー", "psychic energy") : _("魔力", "mana");
				concptr msg = _("あなたは%sの苦痛を%sに変換した！",
					(effect_monster_ptr->seen ? "You convert %s's pain into %s!" :
						"You convert %ss pain into %s!"));
				msg_format(msg, effect_monster_ptr->m_name, str);

				b = MIN(caster_ptr->msp, caster_ptr->csp + b);
				caster_ptr->csp = b;
				caster_ptr->redraw |= PR_MANA;
				caster_ptr->window |= (PW_SPELL);
			}

			effect_monster_ptr->note_dies = _("の精神は崩壊し、肉体は抜け殻となった。", " collapses, a mindless husk.");
			break;
		}
		case GF_TELEKINESIS:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (one_in_(4))
			{
				if (caster_ptr->riding && (effect_monster_ptr->g_ptr->m_idx == caster_ptr->riding)) effect_monster_ptr->do_dist = 0;
				else effect_monster_ptr->do_dist = 7;
			}

			effect_monster_ptr->do_stun = damroll((effect_monster_ptr->caster_lev / 20) + 3, effect_monster_ptr->dam) + 1;
			if ((effect_monster_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
				(effect_monster_ptr->r_ptr->level > 5 + randint1(effect_monster_ptr->dam)))
			{
				effect_monster_ptr->do_stun = 0;
				effect_monster_ptr->obvious = FALSE;
			}

			break;
		}
		case GF_PSY_SPEAR:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			break;
		}
		case GF_METEOR:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			break;
		}
		case GF_DOMINATION:
		{
			if (!is_hostile(effect_monster_ptr->m_ptr)) break;
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if ((effect_monster_ptr->r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) ||
				(effect_monster_ptr->r_ptr->flags3 & RF3_NO_CONF) ||
				(effect_monster_ptr->r_ptr->level > randint1((effect_monster_ptr->dam - 10) < 1 ? 1 : (effect_monster_ptr->dam - 10)) + 10))
			{
				if (effect_monster_ptr->r_ptr->flags3 & RF3_NO_CONF)
				{
					if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_NO_CONF);
				}

				effect_monster_ptr->do_conf = 0;

				/*
				 * Powerful demons & undead can turn a mindcrafter's
				 * attacks back on them
				 */
				if ((effect_monster_ptr->r_ptr->flags3 & (RF3_UNDEAD | RF3_DEMON)) &&
					(effect_monster_ptr->r_ptr->level > caster_ptr->lev / 2) &&
					(one_in_(2)))
				{
					effect_monster_ptr->note = NULL;
					msg_format(_("%^sの堕落した精神は攻撃を跳ね返した！",
						(effect_monster_ptr->seen ? "%^s's corrupted mind backlashes your attack!" :
							"%^ss corrupted mind backlashes your attack!")), effect_monster_ptr->m_name);

					/* Saving throw */
					if (randint0(100 + effect_monster_ptr->r_ptr->level / 2) < caster_ptr->skill_sav)
					{
						msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
					}
					else
					{
						/* Confuse, stun, terrify */
						switch (randint1(4))
						{
						case 1:
							set_stun(caster_ptr, caster_ptr->stun + effect_monster_ptr->dam / 2);
							break;
						case 2:
							set_confused(caster_ptr, caster_ptr->confused + effect_monster_ptr->dam / 2);
							break;
						default:
						{
							if (effect_monster_ptr->r_ptr->flags3 & RF3_NO_FEAR)
								effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
							else
								set_afraid(caster_ptr, caster_ptr->afraid + effect_monster_ptr->dam);
						}
						}
					}
				}
				else
				{
					effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
					effect_monster_ptr->obvious = FALSE;
				}
			}
			else
			{
				if (!common_saving_throw_charm(caster_ptr, effect_monster_ptr->dam, effect_monster_ptr->m_ptr))
				{
					effect_monster_ptr->note = _("があなたに隷属した。", " is in your thrall!");
					set_pet(caster_ptr, effect_monster_ptr->m_ptr);
				}
				else
				{
					switch (randint1(4))
					{
					case 1:
						effect_monster_ptr->do_stun = effect_monster_ptr->dam / 2;
						break;
					case 2:
						effect_monster_ptr->do_conf = effect_monster_ptr->dam / 2;
						break;
					default:
						effect_monster_ptr->do_fear = effect_monster_ptr->dam;
					}
				}
			}

			effect_monster_ptr->dam = 0;
			break;
		}
		case GF_ICE:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			effect_monster_ptr->do_stun = (randint1(15) + 1) / (effect_monster_ptr->r + 1);
			if (effect_monster_ptr->r_ptr->flagsr & RFR_IM_COLD)
			{
				effect_monster_ptr->note = _("にはかなり耐性がある！", " resists a lot.");
				effect_monster_ptr->dam /= 9;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= (RFR_IM_COLD);
			}
			else if (effect_monster_ptr->r_ptr->flags3 & (RF3_HURT_COLD))
			{
				effect_monster_ptr->note = _("はひどい痛手をうけた。", " is hit hard.");
				effect_monster_ptr->dam *= 2;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_HURT_COLD);
			}

			break;
		}
		case GF_HYPODYNAMIA:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (!monster_living(effect_monster_ptr->m_ptr->r_idx))
			{
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr))
				{
					if (effect_monster_ptr->r_ptr->flags3 & RF3_DEMON) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_DEMON);
					if (effect_monster_ptr->r_ptr->flags3 & RF3_UNDEAD) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);
					if (effect_monster_ptr->r_ptr->flags3 & RF3_NONLIVING) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_NONLIVING);
				}
				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
				effect_monster_ptr->obvious = FALSE;
				effect_monster_ptr->dam = 0;
			}
			else
				effect_monster_ptr->do_time = (effect_monster_ptr->dam + 7) / 8;

			break;
		}
		case GF_DEATH_RAY:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (!monster_living(effect_monster_ptr->m_ptr->r_idx))
			{
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr))
				{
					if (effect_monster_ptr->r_ptr->flags3 & RF3_DEMON) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_DEMON);
					if (effect_monster_ptr->r_ptr->flags3 & RF3_UNDEAD) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);
					if (effect_monster_ptr->r_ptr->flags3 & RF3_NONLIVING) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_NONLIVING);
				}
				effect_monster_ptr->note = _("には完全な耐性がある！", " is immune.");
				effect_monster_ptr->obvious = FALSE;
				effect_monster_ptr->dam = 0;
			}
			else if (((effect_monster_ptr->r_ptr->flags1 & RF1_UNIQUE) &&
				(randint1(888) != 666)) ||
				(((effect_monster_ptr->r_ptr->level + randint1(20)) > randint1((effect_monster_ptr->caster_lev / 2) + randint1(10))) &&
					randint1(100) != 66))
			{
				effect_monster_ptr->note = _("には耐性がある！", " resists!");
				effect_monster_ptr->obvious = FALSE;
				effect_monster_ptr->dam = 0;
			}

			break;
		}
		case GF_OLD_POLY:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			effect_monster_ptr->do_poly = TRUE;

			/* Powerful monsters can resist */
			if ((effect_monster_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
				(effect_monster_ptr->r_ptr->flags1 & RF1_QUESTOR) ||
				(effect_monster_ptr->r_ptr->level > randint1((effect_monster_ptr->dam - 10) < 1 ? 1 : (effect_monster_ptr->dam - 10)) + 10))
			{
				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
				effect_monster_ptr->do_poly = FALSE;
				effect_monster_ptr->obvious = FALSE;
			}

			effect_monster_ptr->dam = 0;
			break;
		}
		case GF_OLD_CLONE:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

			if ((floor_ptr->inside_arena) || is_pet(effect_monster_ptr->m_ptr) || (effect_monster_ptr->r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) || (effect_monster_ptr->r_ptr->flags7 & (RF7_NAZGUL | RF7_UNIQUE2)))
			{
				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
			}
			else
			{
				effect_monster_ptr->m_ptr->hp = effect_monster_ptr->m_ptr->maxhp;
				if (multiply_monster(caster_ptr, effect_monster_ptr->g_ptr->m_idx, TRUE, 0L))
				{
					effect_monster_ptr->note = _("が分裂した！", " spawns!");
				}
			}

			effect_monster_ptr->dam = 0;
			break;
		}
		case GF_STAR_HEAL:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

			(void)set_monster_csleep(caster_ptr, effect_monster_ptr->g_ptr->m_idx, 0);

			if (effect_monster_ptr->m_ptr->maxhp < effect_monster_ptr->m_ptr->max_maxhp)
			{
				if (effect_monster_ptr->seen_msg) msg_format(_("%^sの強さが戻った。", "%^s recovers %s vitality."), effect_monster_ptr->m_name, effect_monster_ptr->m_poss);
				effect_monster_ptr->m_ptr->maxhp = effect_monster_ptr->m_ptr->max_maxhp;
			}

			if (!effect_monster_ptr->dam)
			{
				if (caster_ptr->health_who == effect_monster_ptr->g_ptr->m_idx) caster_ptr->redraw |= (PR_HEALTH);
				if (caster_ptr->riding == effect_monster_ptr->g_ptr->m_idx) caster_ptr->redraw |= (PR_UHEALTH);
				break;
			}
		}
		/* Fall through */
		case GF_OLD_HEAL:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

			/* Wake up */
			(void)set_monster_csleep(caster_ptr, effect_monster_ptr->g_ptr->m_idx, 0);
			if (MON_STUNNED(effect_monster_ptr->m_ptr))
			{
				if (effect_monster_ptr->seen_msg) msg_format(_("%^sは朦朧状態から立ち直った。", "%^s is no longer stunned."), effect_monster_ptr->m_name);
				(void)set_monster_stunned(caster_ptr, effect_monster_ptr->g_ptr->m_idx, 0);
			}
			if (MON_CONFUSED(effect_monster_ptr->m_ptr))
			{
				if (effect_monster_ptr->seen_msg) msg_format(_("%^sは混乱から立ち直った。", "%^s is no longer confused."), effect_monster_ptr->m_name);
				(void)set_monster_confused(caster_ptr, effect_monster_ptr->g_ptr->m_idx, 0);
			}
			if (MON_MONFEAR(effect_monster_ptr->m_ptr))
			{
				if (effect_monster_ptr->seen_msg) msg_format(_("%^sは勇気を取り戻した。", "%^s recovers %s courage."), effect_monster_ptr->m_name, effect_monster_ptr->m_poss);
				(void)set_monster_monfear(caster_ptr, effect_monster_ptr->g_ptr->m_idx, 0);
			}

			if (effect_monster_ptr->m_ptr->hp < 30000) effect_monster_ptr->m_ptr->hp += effect_monster_ptr->dam;
			if (effect_monster_ptr->m_ptr->hp > effect_monster_ptr->m_ptr->maxhp) effect_monster_ptr->m_ptr->hp = effect_monster_ptr->m_ptr->maxhp;

			if (!effect_monster_ptr->who)
			{
				chg_virtue(caster_ptr, V_VITALITY, 1);

				if (effect_monster_ptr->r_ptr->flags1 & RF1_UNIQUE)
					chg_virtue(caster_ptr, V_INDIVIDUALISM, 1);

				if (is_friendly(effect_monster_ptr->m_ptr))
					chg_virtue(caster_ptr, V_HONOUR, 1);
				else if (!(effect_monster_ptr->r_ptr->flags3 & RF3_EVIL))
				{
					if (effect_monster_ptr->r_ptr->flags3 & RF3_GOOD)
						chg_virtue(caster_ptr, V_COMPASSION, 2);
					else
						chg_virtue(caster_ptr, V_COMPASSION, 1);
				}

				if (effect_monster_ptr->r_ptr->flags3 & RF3_ANIMAL)
					chg_virtue(caster_ptr, V_NATURE, 1);
			}

			if (effect_monster_ptr->m_ptr->r_idx == MON_LEPER)
			{
				effect_monster_ptr->heal_leper = TRUE;
				if (!effect_monster_ptr->who) chg_virtue(caster_ptr, V_COMPASSION, 5);
			}

			if (caster_ptr->health_who == effect_monster_ptr->g_ptr->m_idx) caster_ptr->redraw |= (PR_HEALTH);
			if (caster_ptr->riding == effect_monster_ptr->g_ptr->m_idx) caster_ptr->redraw |= (PR_UHEALTH);

			effect_monster_ptr->note = _("は体力を回復したようだ。", " looks healthier.");

			effect_monster_ptr->dam = 0;
			break;
		}
		case GF_OLD_SPEED:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

			if (set_monster_fast(caster_ptr, effect_monster_ptr->g_ptr->m_idx, MON_FAST(effect_monster_ptr->m_ptr) + 100))
			{
				effect_monster_ptr->note = _("の動きが速くなった。", " starts moving faster.");
			}

			if (!effect_monster_ptr->who)
			{
				if (effect_monster_ptr->r_ptr->flags1 & RF1_UNIQUE)
					chg_virtue(caster_ptr, V_INDIVIDUALISM, 1);
				if (is_friendly(effect_monster_ptr->m_ptr))
					chg_virtue(caster_ptr, V_HONOUR, 1);
			}

			effect_monster_ptr->dam = 0;
			break;
		}
		case GF_OLD_SLOW:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

			/* Powerful monsters can resist */
			if ((effect_monster_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
				(effect_monster_ptr->r_ptr->level > randint1((effect_monster_ptr->dam - 10) < 1 ? 1 : (effect_monster_ptr->dam - 10)) + 10))
			{
				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
				effect_monster_ptr->obvious = FALSE;
			}
			else
			{
				if (set_monster_slow(caster_ptr, effect_monster_ptr->g_ptr->m_idx, MON_SLOW(effect_monster_ptr->m_ptr) + 50))
				{
					effect_monster_ptr->note = _("の動きが遅くなった。", " starts moving slower.");
				}
			}

			effect_monster_ptr->dam = 0;
			break;
		}
		case GF_OLD_SLEEP:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

			if ((effect_monster_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
				(effect_monster_ptr->r_ptr->flags3 & RF3_NO_SLEEP) ||
				(effect_monster_ptr->r_ptr->level > randint1((effect_monster_ptr->dam - 10) < 1 ? 1 : (effect_monster_ptr->dam - 10)) + 10))
			{
				if (effect_monster_ptr->r_ptr->flags3 & RF3_NO_SLEEP)
				{
					if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_NO_SLEEP);
				}

				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
				effect_monster_ptr->obvious = FALSE;
			}
			else
			{
				effect_monster_ptr->note = _("は眠り込んでしまった！", " falls asleep!");
				effect_monster_ptr->do_sleep = 500;
			}

			effect_monster_ptr->dam = 0;
			break;
		}
		case GF_STASIS_EVIL:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

			if ((effect_monster_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
				!(effect_monster_ptr->r_ptr->flags3 & RF3_EVIL) ||
				(effect_monster_ptr->r_ptr->level > randint1((effect_monster_ptr->dam - 10) < 1 ? 1 : (effect_monster_ptr->dam - 10)) + 10))
			{
				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
				effect_monster_ptr->obvious = FALSE;
			}
			else
			{
				effect_monster_ptr->note = _("は動けなくなった！", " is suspended!");
				effect_monster_ptr->do_sleep = 500;
			}

			effect_monster_ptr->dam = 0;
			break;
		}
		case GF_STASIS:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

			if ((effect_monster_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
				(effect_monster_ptr->r_ptr->level > randint1((effect_monster_ptr->dam - 10) < 1 ? 1 : (effect_monster_ptr->dam - 10)) + 10))
			{
				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
				effect_monster_ptr->obvious = FALSE;
			}
			else
			{
				effect_monster_ptr->note = _("は動けなくなった！", " is suspended!");
				effect_monster_ptr->do_sleep = 500;
			}

			effect_monster_ptr->dam = 0;
			break;
		}
		case GF_CHARM:
		{
			int vir;
			vir = virtue_number(caster_ptr, V_HARMONY);
			if (vir)
			{
				effect_monster_ptr->dam += caster_ptr->virtues[vir - 1] / 10;
			}

			vir = virtue_number(caster_ptr, V_INDIVIDUALISM);
			if (vir)
			{
				effect_monster_ptr->dam -= caster_ptr->virtues[vir - 1] / 20;
			}

			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

			if (common_saving_throw_charm(caster_ptr, effect_monster_ptr->dam, effect_monster_ptr->m_ptr))
			{
				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
				effect_monster_ptr->obvious = FALSE;

				if (one_in_(4)) effect_monster_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
			}
			else if (caster_ptr->cursed & TRC_AGGRAVATE)
			{
				effect_monster_ptr->note = _("はあなたに敵意を抱いている！", " hates you too much!");
				if (one_in_(4)) effect_monster_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
			}
			else
			{
				effect_monster_ptr->note = _("は突然友好的になったようだ！", " suddenly seems friendly!");
				set_pet(caster_ptr, effect_monster_ptr->m_ptr);

				chg_virtue(caster_ptr, V_INDIVIDUALISM, -1);
				if (effect_monster_ptr->r_ptr->flags3 & RF3_ANIMAL)
					chg_virtue(caster_ptr, V_NATURE, 1);
			}

			effect_monster_ptr->dam = 0;
			break;
		}
		case GF_CONTROL_UNDEAD:
		{
			int vir;
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

			vir = virtue_number(caster_ptr, V_UNLIFE);
			if (vir)
			{
				effect_monster_ptr->dam += caster_ptr->virtues[vir - 1] / 10;
			}

			vir = virtue_number(caster_ptr, V_INDIVIDUALISM);
			if (vir)
			{
				effect_monster_ptr->dam -= caster_ptr->virtues[vir - 1] / 20;
			}

			if (common_saving_throw_control(caster_ptr, effect_monster_ptr->dam, effect_monster_ptr->m_ptr) ||
				!(effect_monster_ptr->r_ptr->flags3 & RF3_UNDEAD))
			{
				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
				effect_monster_ptr->obvious = FALSE;
				if (one_in_(4)) effect_monster_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
			}
			else if (caster_ptr->cursed & TRC_AGGRAVATE)
			{
				effect_monster_ptr->note = _("はあなたに敵意を抱いている！", " hates you too much!");
				if (one_in_(4)) effect_monster_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
			}
			else
			{
				effect_monster_ptr->note = _("は既にあなたの奴隷だ！", " is in your thrall!");
				set_pet(caster_ptr, effect_monster_ptr->m_ptr);
			}

			effect_monster_ptr->dam = 0;
			break;
		}
		case GF_CONTROL_DEMON:
		{
			int vir;
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

			vir = virtue_number(caster_ptr, V_UNLIFE);
			if (vir)
			{
				effect_monster_ptr->dam += caster_ptr->virtues[vir - 1] / 10;
			}

			vir = virtue_number(caster_ptr, V_INDIVIDUALISM);
			if (vir)
			{
				effect_monster_ptr->dam -= caster_ptr->virtues[vir - 1] / 20;
			}

			if (common_saving_throw_control(caster_ptr, effect_monster_ptr->dam, effect_monster_ptr->m_ptr) ||
				!(effect_monster_ptr->r_ptr->flags3 & RF3_DEMON))
			{
				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
				effect_monster_ptr->obvious = FALSE;
				if (one_in_(4)) effect_monster_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
			}
			else if (caster_ptr->cursed & TRC_AGGRAVATE)
			{
				effect_monster_ptr->note = _("はあなたに敵意を抱いている！", " hates you too much!");
				if (one_in_(4)) effect_monster_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
			}
			else
			{
				effect_monster_ptr->note = _("は既にあなたの奴隷だ！", " is in your thrall!");
				set_pet(caster_ptr, effect_monster_ptr->m_ptr);
			}

			effect_monster_ptr->dam = 0;
			break;
		}
		case GF_CONTROL_ANIMAL:
		{
			int vir;
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

			vir = virtue_number(caster_ptr, V_NATURE);
			if (vir)
			{
				effect_monster_ptr->dam += caster_ptr->virtues[vir - 1] / 10;
			}

			vir = virtue_number(caster_ptr, V_INDIVIDUALISM);
			if (vir)
			{
				effect_monster_ptr->dam -= caster_ptr->virtues[vir - 1] / 20;
			}

			if (common_saving_throw_control(caster_ptr, effect_monster_ptr->dam, effect_monster_ptr->m_ptr) ||
				!(effect_monster_ptr->r_ptr->flags3 & RF3_ANIMAL))
			{
				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
				effect_monster_ptr->obvious = FALSE;
				if (one_in_(4)) effect_monster_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
			}
			else if (caster_ptr->cursed & TRC_AGGRAVATE)
			{
				effect_monster_ptr->note = _("はあなたに敵意を抱いている！", " hates you too much!");
				if (one_in_(4)) effect_monster_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
			}
			else
			{
				effect_monster_ptr->note = _("はなついた。", " is tamed!");
				set_pet(caster_ptr, effect_monster_ptr->m_ptr);
				if (effect_monster_ptr->r_ptr->flags3 & RF3_ANIMAL)
					chg_virtue(caster_ptr, V_NATURE, 1);
			}

			effect_monster_ptr->dam = 0;
			break;
		}
		case GF_CHARM_LIVING:
		{
			int vir;

			vir = virtue_number(caster_ptr, V_UNLIFE);
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

			vir = virtue_number(caster_ptr, V_UNLIFE);
			if (vir)
			{
				effect_monster_ptr->dam -= caster_ptr->virtues[vir - 1] / 10;
			}

			vir = virtue_number(caster_ptr, V_INDIVIDUALISM);
			if (vir)
			{
				effect_monster_ptr->dam -= caster_ptr->virtues[vir - 1] / 20;
			}

			msg_format(_("%sを見つめた。", "You stare into %s."), effect_monster_ptr->m_name);

			if (common_saving_throw_charm(caster_ptr, effect_monster_ptr->dam, effect_monster_ptr->m_ptr) ||
				!monster_living(effect_monster_ptr->m_ptr->r_idx))
			{
				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
				effect_monster_ptr->obvious = FALSE;
				if (one_in_(4)) effect_monster_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
			}
			else if (caster_ptr->cursed & TRC_AGGRAVATE)
			{
				effect_monster_ptr->note = _("はあなたに敵意を抱いている！", " hates you too much!");
				if (one_in_(4)) effect_monster_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
			}
			else
			{
				effect_monster_ptr->note = _("を支配した。", " is tamed!");
				set_pet(caster_ptr, effect_monster_ptr->m_ptr);
				if (effect_monster_ptr->r_ptr->flags3 & RF3_ANIMAL)
					chg_virtue(caster_ptr, V_NATURE, 1);
			}

			effect_monster_ptr->dam = 0;
			break;
		}
		case GF_OLD_CONF:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

			effect_monster_ptr->do_conf = damroll(3, (effect_monster_ptr->dam / 2)) + 1;
			if ((effect_monster_ptr->r_ptr->flags1 & (RF1_UNIQUE)) ||
				(effect_monster_ptr->r_ptr->flags3 & (RF3_NO_CONF)) ||
				(effect_monster_ptr->r_ptr->level > randint1((effect_monster_ptr->dam - 10) < 1 ? 1 : (effect_monster_ptr->dam - 10)) + 10))
			{
				if (effect_monster_ptr->r_ptr->flags3 & (RF3_NO_CONF))
				{
					if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_NO_CONF);
				}

				effect_monster_ptr->do_conf = 0;
				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
				effect_monster_ptr->obvious = FALSE;
			}

			effect_monster_ptr->dam = 0;
			break;
		}
		case GF_STUN:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

			effect_monster_ptr->do_stun = damroll((effect_monster_ptr->caster_lev / 20) + 3, (effect_monster_ptr->dam)) + 1;
			if ((effect_monster_ptr->r_ptr->flags1 & (RF1_UNIQUE)) ||
				(effect_monster_ptr->r_ptr->level > randint1((effect_monster_ptr->dam - 10) < 1 ? 1 : (effect_monster_ptr->dam - 10)) + 10))
			{
				effect_monster_ptr->do_stun = 0;
				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
				effect_monster_ptr->obvious = FALSE;
			}

			effect_monster_ptr->dam = 0;
			break;
		}
		case GF_LITE_WEAK:
		{
			if (!effect_monster_ptr->dam)
			{
				effect_monster_ptr->skipped = TRUE;
				break;
			}

			if (effect_monster_ptr->r_ptr->flags3 & (RF3_HURT_LITE))
			{
				if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_HURT_LITE);

				effect_monster_ptr->note = _("は光に身をすくめた！", " cringes from the light!");
				effect_monster_ptr->note_dies = _("は光を受けてしぼんでしまった！", " shrivels away in the light!");
			}
			else
			{
				effect_monster_ptr->dam = 0;
			}

			break;
		}
		case GF_LITE:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

			if (effect_monster_ptr->r_ptr->flagsr & RFR_RES_LITE)
			{
				effect_monster_ptr->note = _("には耐性がある！", " resists!");
				effect_monster_ptr->dam *= 2; effect_monster_ptr->dam /= (randint1(6) + 6);
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= (RFR_RES_LITE);
			}
			else if (effect_monster_ptr->r_ptr->flags3 & (RF3_HURT_LITE))
			{
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_HURT_LITE);
				effect_monster_ptr->note = _("は光に身をすくめた！", " cringes from the light!");
				effect_monster_ptr->note_dies = _("は光を受けてしぼんでしまった！", " shrivels away in the light!");
				effect_monster_ptr->dam *= 2;
			}
			break;
		}
		case GF_DARK:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

			if (effect_monster_ptr->r_ptr->flagsr & RFR_RES_DARK)
			{
				effect_monster_ptr->note = _("には耐性がある！", " resists!");
				effect_monster_ptr->dam *= 2; effect_monster_ptr->dam /= (randint1(6) + 6);
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= (RFR_RES_DARK);
			}

			break;
		}
		case GF_KILL_WALL:
		{
			if (effect_monster_ptr->r_ptr->flags3 & (RF3_HURT_ROCK))
			{
				if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_HURT_ROCK);

				effect_monster_ptr->note = _("の皮膚がただれた！", " loses some skin!");
				effect_monster_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
			}
			else
			{
				effect_monster_ptr->dam = 0;
			}

			break;
		}
		case GF_AWAY_UNDEAD:
		{
			if (effect_monster_ptr->r_ptr->flags3 & (RF3_UNDEAD))
			{
				bool resists_tele = FALSE;

				if (effect_monster_ptr->r_ptr->flagsr & RFR_RES_TELE)
				{
					if ((effect_monster_ptr->r_ptr->flags1 & (RF1_UNIQUE)) || (effect_monster_ptr->r_ptr->flagsr & RFR_RES_ALL))
					{
						if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
						effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
						resists_tele = TRUE;
					}
					else if (effect_monster_ptr->r_ptr->level > randint1(100))
					{
						if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
						effect_monster_ptr->note = _("には耐性がある！", " resists!");
						resists_tele = TRUE;
					}
				}

				if (!resists_tele)
				{
					if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
					if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);
					effect_monster_ptr->do_dist = effect_monster_ptr->dam;
				}
			}
			else
			{
				effect_monster_ptr->skipped = TRUE;
			}

			effect_monster_ptr->dam = 0;
			break;
		}
		case GF_AWAY_EVIL:
		{
			if (effect_monster_ptr->r_ptr->flags3 & (RF3_EVIL))
			{
				bool resists_tele = FALSE;

				if (effect_monster_ptr->r_ptr->flagsr & RFR_RES_TELE)
				{
					if ((effect_monster_ptr->r_ptr->flags1 & (RF1_UNIQUE)) || (effect_monster_ptr->r_ptr->flagsr & RFR_RES_ALL))
					{
						if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
						effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
						resists_tele = TRUE;
					}
					else if (effect_monster_ptr->r_ptr->level > randint1(100))
					{
						if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
						effect_monster_ptr->note = _("には耐性がある！", " resists!");
						resists_tele = TRUE;
					}
				}

				if (!resists_tele)
				{
					if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
					if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_EVIL);
					effect_monster_ptr->do_dist = effect_monster_ptr->dam;
				}
			}
			else
			{
				effect_monster_ptr->skipped = TRUE;
			}

			effect_monster_ptr->dam = 0;
			break;
		}
		case GF_AWAY_ALL:
		{
			bool resists_tele = FALSE;
			if (effect_monster_ptr->r_ptr->flagsr & RFR_RES_TELE)
			{
				if ((effect_monster_ptr->r_ptr->flags1 & (RF1_UNIQUE)) || (effect_monster_ptr->r_ptr->flagsr & RFR_RES_ALL))
				{
					if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
					effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
					resists_tele = TRUE;
				}
				else if (effect_monster_ptr->r_ptr->level > randint1(100))
				{
					if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flagsr |= RFR_RES_TELE;
					effect_monster_ptr->note = _("には耐性がある！", " resists!");
					resists_tele = TRUE;
				}
			}

			if (!resists_tele)
			{
				if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

				effect_monster_ptr->do_dist = effect_monster_ptr->dam;
			}

			effect_monster_ptr->dam = 0;
			break;
		}
		case GF_TURN_UNDEAD:
		{
			if (effect_monster_ptr->r_ptr->flags3 & (RF3_UNDEAD))
			{
				if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);

				effect_monster_ptr->do_fear = damroll(3, (effect_monster_ptr->dam / 2)) + 1;
				if (effect_monster_ptr->r_ptr->level > randint1((effect_monster_ptr->dam - 10) < 1 ? 1 : (effect_monster_ptr->dam - 10)) + 10)
				{
					effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
					effect_monster_ptr->obvious = FALSE;
					effect_monster_ptr->do_fear = 0;
				}
			}
			else
			{
				effect_monster_ptr->skipped = TRUE;
			}

			effect_monster_ptr->dam = 0;
			break;
		}
		case GF_TURN_EVIL:
		{
			if (effect_monster_ptr->r_ptr->flags3 & (RF3_EVIL))
			{
				if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_EVIL);

				effect_monster_ptr->do_fear = damroll(3, (effect_monster_ptr->dam / 2)) + 1;
				if (effect_monster_ptr->r_ptr->level > randint1((effect_monster_ptr->dam - 10) < 1 ? 1 : (effect_monster_ptr->dam - 10)) + 10)
				{
					effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
					effect_monster_ptr->obvious = FALSE;
					effect_monster_ptr->do_fear = 0;
				}
			}
			else
			{
				effect_monster_ptr->skipped = TRUE;
			}

			effect_monster_ptr->dam = 0;
			break;
		}
		case GF_TURN_ALL:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

			effect_monster_ptr->do_fear = damroll(3, (effect_monster_ptr->dam / 2)) + 1;
			if ((effect_monster_ptr->r_ptr->flags1 & (RF1_UNIQUE)) ||
				(effect_monster_ptr->r_ptr->flags3 & (RF3_NO_FEAR)) ||
				(effect_monster_ptr->r_ptr->level > randint1((effect_monster_ptr->dam - 10) < 1 ? 1 : (effect_monster_ptr->dam - 10)) + 10))
			{
				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
				effect_monster_ptr->obvious = FALSE;
				effect_monster_ptr->do_fear = 0;
			}

			effect_monster_ptr->dam = 0;
			break;
		}
		case GF_DISP_UNDEAD:
		{
			if (effect_monster_ptr->r_ptr->flags3 & (RF3_UNDEAD))
			{
				if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

				/* Learn about effect_monster_ptr->effect_type */
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_UNDEAD);

				effect_monster_ptr->note = _("は身震いした。", " shudders.");
				effect_monster_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
			}
			else
			{
				effect_monster_ptr->skipped = TRUE;
				effect_monster_ptr->dam = 0;
			}

			break;
		}
		case GF_DISP_EVIL:
		{
			if (effect_monster_ptr->r_ptr->flags3 & (RF3_EVIL))
			{
				if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_EVIL);

				effect_monster_ptr->note = _("は身震いした。", " shudders.");
				effect_monster_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
			}
			else
			{
				effect_monster_ptr->skipped = TRUE;
				effect_monster_ptr->dam = 0;
			}

			break;
		}
		case GF_DISP_GOOD:
		{
			if (effect_monster_ptr->r_ptr->flags3 & (RF3_GOOD))
			{
				if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_GOOD);

				effect_monster_ptr->note = _("は身震いした。", " shudders.");
				effect_monster_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
			}
			else
			{
				effect_monster_ptr->skipped = TRUE;
				effect_monster_ptr->dam = 0;
			}

			break;
		}
		case GF_DISP_LIVING:
		{
			if (monster_living(effect_monster_ptr->m_ptr->r_idx))
			{
				if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

				effect_monster_ptr->note = _("は身震いした。", " shudders.");
				effect_monster_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
			}
			else
			{
				effect_monster_ptr->skipped = TRUE;
				effect_monster_ptr->dam = 0;
			}

			break;
		}
		case GF_DISP_DEMON:
		{
			if (effect_monster_ptr->r_ptr->flags3 & (RF3_DEMON))
			{
				if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_DEMON);

				effect_monster_ptr->note = _("は身震いした。", " shudders.");
				effect_monster_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
			}
			else
			{
				effect_monster_ptr->skipped = TRUE;
				effect_monster_ptr->dam = 0;
			}

			break;
		}
		case GF_DISP_ALL:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			effect_monster_ptr->note = _("は身震いした。", " shudders.");
			effect_monster_ptr->note_dies = _("はドロドロに溶けた！", " dissolves!");
			break;
		}
		case GF_DRAIN_MANA:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if ((effect_monster_ptr->r_ptr->flags4 & ~(RF4_NOMAGIC_MASK)) || (effect_monster_ptr->r_ptr->a_ability_flags1 & ~(RF5_NOMAGIC_MASK)) || (effect_monster_ptr->r_ptr->a_ability_flags2 & ~(RF6_NOMAGIC_MASK)))
			{
				if (effect_monster_ptr->who > 0)
				{
					if (effect_monster_ptr->m_caster_ptr->hp < effect_monster_ptr->m_caster_ptr->maxhp)
					{
						effect_monster_ptr->m_caster_ptr->hp += effect_monster_ptr->dam;
						if (effect_monster_ptr->m_caster_ptr->hp > effect_monster_ptr->m_caster_ptr->maxhp) effect_monster_ptr->m_caster_ptr->hp = effect_monster_ptr->m_caster_ptr->maxhp;
						if (caster_ptr->health_who == effect_monster_ptr->who) caster_ptr->redraw |= (PR_HEALTH);
						if (caster_ptr->riding == effect_monster_ptr->who) caster_ptr->redraw |= (PR_UHEALTH);

						if (effect_monster_ptr->see_s_msg)
						{
							monster_desc(caster_ptr, effect_monster_ptr->killer, effect_monster_ptr->m_caster_ptr, 0);
							msg_format(_("%^sは気分が良さそうだ。", "%^s appears healthier."), effect_monster_ptr->killer);
						}
					}
				}
				else
				{
					msg_format(_("%sから精神エネルギーを吸いとった。", "You draw psychic energy from %s."), effect_monster_ptr->m_name);
					(void)hp_player(caster_ptr, effect_monster_ptr->dam);
				}
			}
			else
			{
				if (effect_monster_ptr->see_s_msg) msg_format(_("%sには効果がなかった。", "%s is unaffected."), effect_monster_ptr->m_name);
			}

			effect_monster_ptr->dam = 0;
			break;
		}
		case GF_MIND_BLAST:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (!effect_monster_ptr->who) msg_format(_("%sをじっと睨んだ。", "You gaze intently at %s."), effect_monster_ptr->m_name);

			if ((effect_monster_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
				(effect_monster_ptr->r_ptr->flags3 & RF3_NO_CONF) ||
				(effect_monster_ptr->r_ptr->level > randint1((effect_monster_ptr->caster_lev - 10) < 1 ? 1 : (effect_monster_ptr->caster_lev - 10)) + 10))
			{
				if (effect_monster_ptr->r_ptr->flags3 & (RF3_NO_CONF))
				{
					if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_NO_CONF);
				}

				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
				effect_monster_ptr->dam = 0;
			}
			else if (effect_monster_ptr->r_ptr->flags2 & RF2_EMPTY_MIND)
			{
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags2 |= (RF2_EMPTY_MIND);
				effect_monster_ptr->note = _("には完全な耐性がある！", " is immune.");
				effect_monster_ptr->dam = 0;
			}
			else if (effect_monster_ptr->r_ptr->flags2 & RF2_WEIRD_MIND)
			{
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags2 |= (RF2_WEIRD_MIND);
				effect_monster_ptr->note = _("には耐性がある。", " resists.");
				effect_monster_ptr->dam /= 3;
			}
			else
			{
				effect_monster_ptr->note = _("は精神攻撃を食らった。", " is blasted by psionic energy.");
				effect_monster_ptr->note_dies = _("の精神は崩壊し、肉体は抜け殻となった。", " collapses, a mindless husk.");

				if (effect_monster_ptr->who > 0) effect_monster_ptr->do_conf = randint0(4) + 4;
				else effect_monster_ptr->do_conf = randint0(8) + 8;
			}

			break;
		}
		case GF_BRAIN_SMASH:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (!effect_monster_ptr->who) msg_format(_("%sをじっと睨んだ。", "You gaze intently at %s."), effect_monster_ptr->m_name);

			if ((effect_monster_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
				(effect_monster_ptr->r_ptr->flags3 & RF3_NO_CONF) ||
				(effect_monster_ptr->r_ptr->level > randint1((effect_monster_ptr->caster_lev - 10) < 1 ? 1 : (effect_monster_ptr->caster_lev - 10)) + 10))
			{
				if (effect_monster_ptr->r_ptr->flags3 & (RF3_NO_CONF))
				{
					if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_NO_CONF);
				}

				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
				effect_monster_ptr->dam = 0;
			}
			else if (effect_monster_ptr->r_ptr->flags2 & RF2_EMPTY_MIND)
			{
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags2 |= (RF2_EMPTY_MIND);
				effect_monster_ptr->note = _("には完全な耐性がある！", " is immune.");
				effect_monster_ptr->dam = 0;
			}
			else if (effect_monster_ptr->r_ptr->flags2 & RF2_WEIRD_MIND)
			{
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags2 |= (RF2_WEIRD_MIND);
				effect_monster_ptr->note = _("には耐性がある！", " resists!");
				effect_monster_ptr->dam /= 3;
			}
			else
			{
				effect_monster_ptr->note = _("は精神攻撃を食らった。", " is blasted by psionic energy.");
				effect_monster_ptr->note_dies = _("の精神は崩壊し、肉体は抜け殻となった。", " collapses, a mindless husk.");
				if (effect_monster_ptr->who > 0)
				{
					effect_monster_ptr->do_conf = randint0(4) + 4;
					effect_monster_ptr->do_stun = randint0(4) + 4;
				}
				else
				{
					effect_monster_ptr->do_conf = randint0(8) + 8;
					effect_monster_ptr->do_stun = randint0(8) + 8;
				}
				(void)set_monster_slow(caster_ptr, effect_monster_ptr->g_ptr->m_idx, MON_SLOW(effect_monster_ptr->m_ptr) + 10);
			}

			break;
		}
		case GF_CAUSE_1:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (!effect_monster_ptr->who) msg_format(_("%sを指差して呪いをかけた。", "You point at %s and curse."), effect_monster_ptr->m_name);
			if (randint0(100 + (effect_monster_ptr->caster_lev / 2)) < (effect_monster_ptr->r_ptr->level + 35))
			{
				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
				effect_monster_ptr->dam = 0;
			}

			break;
		}
		case GF_CAUSE_2:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (!effect_monster_ptr->who) msg_format(_("%sを指差して恐ろしげに呪いをかけた。", "You point at %s and curse horribly."), effect_monster_ptr->m_name);

			if (randint0(100 + (effect_monster_ptr->caster_lev / 2)) < (effect_monster_ptr->r_ptr->level + 35))
			{
				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
				effect_monster_ptr->dam = 0;
			}

			break;
		}
		case GF_CAUSE_3:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (!effect_monster_ptr->who) msg_format(_("%sを指差し、恐ろしげに呪文を唱えた！", "You point at %s, incanting terribly!"), effect_monster_ptr->m_name);

			if (randint0(100 + (effect_monster_ptr->caster_lev / 2)) < (effect_monster_ptr->r_ptr->level + 35))
			{
				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
				effect_monster_ptr->dam = 0;
			}

			break;
		}
		case GF_CAUSE_4:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (!effect_monster_ptr->who)
				msg_format(_("%sの秘孔を突いて、「お前は既に死んでいる」と叫んだ。",
					"You point at %s, screaming the word, 'DIE!'."), effect_monster_ptr->m_name);

			if ((randint0(100 + (effect_monster_ptr->caster_lev / 2)) < (effect_monster_ptr->r_ptr->level + 35)) && ((effect_monster_ptr->who <= 0) || (effect_monster_ptr->m_caster_ptr->r_idx != MON_KENSHIROU)))
			{
				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
				effect_monster_ptr->dam = 0;
			}
			break;
		}
		case GF_HAND_DOOM:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flags1 & RF1_UNIQUE)
			{
				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
				effect_monster_ptr->dam = 0;
			}
			else
			{
				if ((effect_monster_ptr->who > 0) ? ((effect_monster_ptr->caster_lev + randint1(effect_monster_ptr->dam)) > (effect_monster_ptr->r_ptr->level + 10 + randint1(20))) :
					(((effect_monster_ptr->caster_lev / 2) + randint1(effect_monster_ptr->dam)) > (effect_monster_ptr->r_ptr->level + randint1(200))))
				{
					effect_monster_ptr->dam = ((40 + randint1(20)) * effect_monster_ptr->m_ptr->hp) / 100;

					if (effect_monster_ptr->m_ptr->hp < effect_monster_ptr->dam) effect_monster_ptr->dam = effect_monster_ptr->m_ptr->hp - 1;
				}
				else
				{
					/* todo 乱数で破滅のを弾いた結果が「耐性を持っている」ことになるのはおかしい */
					effect_monster_ptr->note = _("は耐性を持っている！", "resists!");
					effect_monster_ptr->dam = 0;
				}
			}

			break;
		}
		case GF_CAPTURE:
		{
			int nokori_hp;
			if ((floor_ptr->inside_quest && (quest[floor_ptr->inside_quest].type == QUEST_TYPE_KILL_ALL) && !is_pet(effect_monster_ptr->m_ptr)) ||
				(effect_monster_ptr->r_ptr->flags1 & (RF1_UNIQUE)) || (effect_monster_ptr->r_ptr->flags7 & (RF7_NAZGUL)) || (effect_monster_ptr->r_ptr->flags7 & (RF7_UNIQUE2)) || (effect_monster_ptr->r_ptr->flags1 & RF1_QUESTOR) || effect_monster_ptr->m_ptr->parent_m_idx)
			{
				msg_format(_("%sには効果がなかった。", "%s is unaffected."), effect_monster_ptr->m_name);
				effect_monster_ptr->skipped = TRUE;
				break;
			}

			if (is_pet(effect_monster_ptr->m_ptr)) nokori_hp = effect_monster_ptr->m_ptr->maxhp * 4L;
			else if ((caster_ptr->pclass == CLASS_BEASTMASTER) && monster_living(effect_monster_ptr->m_ptr->r_idx))
				nokori_hp = effect_monster_ptr->m_ptr->maxhp * 3 / 10;
			else
				nokori_hp = effect_monster_ptr->m_ptr->maxhp * 3 / 20;

			if (effect_monster_ptr->m_ptr->hp >= nokori_hp)
			{
				msg_format(_("もっと弱らせないと。", "You need to weaken %s more."), effect_monster_ptr->m_name);
				effect_monster_ptr->skipped = TRUE;
			}
			else if (effect_monster_ptr->m_ptr->hp < randint0(nokori_hp))
			{
				if (effect_monster_ptr->m_ptr->mflag2 & MFLAG2_CHAMELEON) choose_new_monster(caster_ptr, effect_monster_ptr->g_ptr->m_idx, FALSE, MON_CHAMELEON);
				msg_format(_("%sを捕えた！", "You capture %^s!"), effect_monster_ptr->m_name);
				cap_mon = effect_monster_ptr->m_ptr->r_idx;
				cap_mspeed = effect_monster_ptr->m_ptr->mspeed;
				cap_hp = effect_monster_ptr->m_ptr->hp;
				cap_maxhp = effect_monster_ptr->m_ptr->max_maxhp;
				cap_nickname = effect_monster_ptr->m_ptr->nickname;
				if (effect_monster_ptr->g_ptr->m_idx == caster_ptr->riding)
				{
					if (rakuba(caster_ptr, -1, FALSE))
					{
						msg_format(_("地面に落とされた。", "You have fallen from %s."), effect_monster_ptr->m_name);
					}
				}

				delete_monster_idx(caster_ptr, effect_monster_ptr->g_ptr->m_idx);

				return TRUE;
			}
			else
			{
				msg_format(_("うまく捕まえられなかった。", "You failed to capture %s."), effect_monster_ptr->m_name);
				effect_monster_ptr->skipped = TRUE;
			}

			break;
		}
		case GF_ATTACK:
		{
			return py_attack(caster_ptr, effect_monster_ptr->y, effect_monster_ptr->x, effect_monster_ptr->dam);
		}
		case GF_ENGETSU:
		{
			int effect = 0;
			bool done = TRUE;

			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (effect_monster_ptr->r_ptr->flags2 & RF2_EMPTY_MIND)
			{
				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
				effect_monster_ptr->dam = 0;
				effect_monster_ptr->skipped = TRUE;
				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags2 |= (RF2_EMPTY_MIND);
				break;
			}
			if (MON_CSLEEP(effect_monster_ptr->m_ptr))
			{
				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
				effect_monster_ptr->dam = 0;
				effect_monster_ptr->skipped = TRUE;
				break;
			}

			if (one_in_(5)) effect = 1;
			else if (one_in_(4)) effect = 2;
			else if (one_in_(3)) effect = 3;
			else done = FALSE;

			if (effect == 1)
			{
				if ((effect_monster_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
					(effect_monster_ptr->r_ptr->level > randint1((effect_monster_ptr->dam - 10) < 1 ? 1 : (effect_monster_ptr->dam - 10)) + 10))
				{
					effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
					effect_monster_ptr->obvious = FALSE;
				}
				else
				{
					if (set_monster_slow(caster_ptr, effect_monster_ptr->g_ptr->m_idx, MON_SLOW(effect_monster_ptr->m_ptr) + 50))
					{
						effect_monster_ptr->note = _("の動きが遅くなった。", " starts moving slower.");
					}
				}
			}
			else if (effect == 2)
			{
				effect_monster_ptr->do_stun = damroll((caster_ptr->lev / 10) + 3, (effect_monster_ptr->dam)) + 1;
				if ((effect_monster_ptr->r_ptr->flags1 & (RF1_UNIQUE)) ||
					(effect_monster_ptr->r_ptr->level > randint1((effect_monster_ptr->dam - 10) < 1 ? 1 : (effect_monster_ptr->dam - 10)) + 10))
				{
					effect_monster_ptr->do_stun = 0;
					effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
					effect_monster_ptr->obvious = FALSE;
				}
			}
			else if (effect == 3)
			{
				if ((effect_monster_ptr->r_ptr->flags1 & RF1_UNIQUE) ||
					(effect_monster_ptr->r_ptr->flags3 & RF3_NO_SLEEP) ||
					(effect_monster_ptr->r_ptr->level > randint1((effect_monster_ptr->dam - 10) < 1 ? 1 : (effect_monster_ptr->dam - 10)) + 10))
				{
					if (effect_monster_ptr->r_ptr->flags3 & RF3_NO_SLEEP)
					{
						if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_NO_SLEEP);
					}

					effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
					effect_monster_ptr->obvious = FALSE;
				}
				else
				{
					/* Go to sleep (much) later */
					effect_monster_ptr->note = _("は眠り込んでしまった！", " falls asleep!");
					effect_monster_ptr->do_sleep = 500;
				}
			}

			if (!done)
			{
				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
			}

			effect_monster_ptr->dam = 0;
			break;
		}
		case GF_GENOCIDE:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			if (genocide_aux(caster_ptr, effect_monster_ptr->g_ptr->m_idx, effect_monster_ptr->dam, !effect_monster_ptr->who, (effect_monster_ptr->r_ptr->level + 1) / 2, _("モンスター消滅", "Genocide One")))
			{
				if (effect_monster_ptr->seen_msg) msg_format(_("%sは消滅した！", "%^s disappeared!"), effect_monster_ptr->m_name);
				chg_virtue(caster_ptr, V_VITALITY, -1);
				return TRUE;
			}

			effect_monster_ptr->skipped = TRUE;
			break;
		}
		case GF_PHOTO:
		{
			if (!effect_monster_ptr->who)
				msg_format(_("%sを写真に撮った。", "You take a photograph of %s."), effect_monster_ptr->m_name);

			if (effect_monster_ptr->r_ptr->flags3 & (RF3_HURT_LITE))
			{
				if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

				if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_HURT_LITE);

				effect_monster_ptr->note = _("は光に身をすくめた！", " cringes from the light!");
				effect_monster_ptr->note_dies = _("は光を受けてしぼんでしまった！", " shrivels away in the light!");
			}
			else
			{
				effect_monster_ptr->dam = 0;
			}

			effect_monster_ptr->photo = effect_monster_ptr->m_ptr->r_idx;
			break;
		}
		case GF_BLOOD_CURSE:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;
			break;
		}
		case GF_CRUSADE:
		{
			bool success = FALSE;
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

			if ((effect_monster_ptr->r_ptr->flags3 & (RF3_GOOD)) && !floor_ptr->inside_arena)
			{
				if (effect_monster_ptr->r_ptr->flags3 & (RF3_NO_CONF)) effect_monster_ptr->dam -= 50;
				if (effect_monster_ptr->dam < 1) effect_monster_ptr->dam = 1;

				if (is_pet(effect_monster_ptr->m_ptr))
				{
					effect_monster_ptr->note = _("の動きが速くなった。", " starts moving faster.");
					(void)set_monster_fast(caster_ptr, effect_monster_ptr->g_ptr->m_idx, MON_FAST(effect_monster_ptr->m_ptr) + 100);
					success = TRUE;
				}
				else if ((effect_monster_ptr->r_ptr->flags1 & (RF1_QUESTOR)) ||
					(effect_monster_ptr->r_ptr->flags1 & (RF1_UNIQUE)) ||
					(effect_monster_ptr->m_ptr->mflag2 & MFLAG2_NOPET) ||
					(caster_ptr->cursed & TRC_AGGRAVATE) ||
					((effect_monster_ptr->r_ptr->level + 10) > randint1(effect_monster_ptr->dam)))
				{
					if (one_in_(4)) effect_monster_ptr->m_ptr->mflag2 |= MFLAG2_NOPET;
				}
				else
				{
					effect_monster_ptr->note = _("を支配した。", " is tamed!");
					set_pet(caster_ptr, effect_monster_ptr->m_ptr);
					(void)set_monster_fast(caster_ptr, effect_monster_ptr->g_ptr->m_idx, MON_FAST(effect_monster_ptr->m_ptr) + 100);

					if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr)) effect_monster_ptr->r_ptr->r_flags3 |= (RF3_GOOD);
					success = TRUE;
				}
			}

			if (!success)
			{
				if (!(effect_monster_ptr->r_ptr->flags3 & RF3_NO_FEAR))
				{
					effect_monster_ptr->do_fear = randint1(90) + 10;
				}
				else if (is_original_ap_and_seen(caster_ptr, effect_monster_ptr->m_ptr))
					effect_monster_ptr->r_ptr->r_flags3 |= (RF3_NO_FEAR);
			}

			effect_monster_ptr->dam = 0;
			break;
		}
		case GF_WOUNDS:
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

			if (randint0(100 + effect_monster_ptr->dam) < (effect_monster_ptr->r_ptr->level + 50))
			{
				effect_monster_ptr->note = _("には効果がなかった。", " is unaffected.");
				effect_monster_ptr->dam = 0;
			}
			break;
		}
		default:
		{
			effect_monster_ptr->skipped = TRUE;
			effect_monster_ptr->dam = 0;
			break;
		}
		}
	}

	if (effect_monster_ptr->skipped) return FALSE;

	if (effect_monster_ptr->r_ptr->flags1 & (RF1_UNIQUE)) effect_monster_ptr->do_poly = FALSE;
	if (effect_monster_ptr->r_ptr->flags1 & RF1_QUESTOR) effect_monster_ptr->do_poly = FALSE;
	if (caster_ptr->riding && (effect_monster_ptr->g_ptr->m_idx == caster_ptr->riding))
		effect_monster_ptr->do_poly = FALSE;

	if (((effect_monster_ptr->r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR)) || (effect_monster_ptr->r_ptr->flags7 & RF7_NAZGUL)) && !caster_ptr->phase_out)
	{
		if (effect_monster_ptr->who && (effect_monster_ptr->dam > effect_monster_ptr->m_ptr->hp)) effect_monster_ptr->dam = effect_monster_ptr->m_ptr->hp;
	}

	if (!effect_monster_ptr->who && effect_monster_ptr->slept)
	{
		if (!(effect_monster_ptr->r_ptr->flags3 & RF3_EVIL) || one_in_(5)) chg_virtue(caster_ptr, V_COMPASSION, -1);
		if (!(effect_monster_ptr->r_ptr->flags3 & RF3_EVIL) || one_in_(5)) chg_virtue(caster_ptr, V_HONOUR, -1);
	}

	int tmp = effect_monster_ptr->dam;
	effect_monster_ptr->dam = mon_damage_mod(caster_ptr, effect_monster_ptr->m_ptr, effect_monster_ptr->dam, (bool)(effect_monster_ptr->effect_type == GF_PSY_SPEAR));
	if ((tmp > 0) && (effect_monster_ptr->dam == 0)) effect_monster_ptr->note = _("はダメージを受けていない。", " is unharmed.");

	if (effect_monster_ptr->dam > effect_monster_ptr->m_ptr->hp)
	{
		effect_monster_ptr->note = effect_monster_ptr->note_dies;
	}
	else
	{
		if (effect_monster_ptr->do_stun &&
			!(effect_monster_ptr->r_ptr->flagsr & (RFR_RES_SOUN | RFR_RES_WALL)) &&
			!(effect_monster_ptr->r_ptr->flags3 & RF3_NO_STUN))
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

			if (MON_STUNNED(effect_monster_ptr->m_ptr))
			{
				effect_monster_ptr->note = _("はひどくもうろうとした。", " is more dazed.");
				tmp = MON_STUNNED(effect_monster_ptr->m_ptr) + (effect_monster_ptr->do_stun / 2);
			}
			else
			{
				effect_monster_ptr->note = _("はもうろうとした。", " is dazed.");
				tmp = effect_monster_ptr->do_stun;
			}

			(void)set_monster_stunned(caster_ptr, effect_monster_ptr->g_ptr->m_idx, tmp);
			effect_monster_ptr->get_angry = TRUE;
		}

		if (effect_monster_ptr->do_conf &&
			!(effect_monster_ptr->r_ptr->flags3 & RF3_NO_CONF) &&
			!(effect_monster_ptr->r_ptr->flagsr & RFR_EFF_RES_CHAO_MASK))
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

			if (MON_CONFUSED(effect_monster_ptr->m_ptr))
			{
				effect_monster_ptr->note = _("はさらに混乱したようだ。", " looks more confused.");
				tmp = MON_CONFUSED(effect_monster_ptr->m_ptr) + (effect_monster_ptr->do_conf / 2);
			}
			else
			{
				effect_monster_ptr->note = _("は混乱したようだ。", " looks confused.");
				tmp = effect_monster_ptr->do_conf;
			}

			(void)set_monster_confused(caster_ptr, effect_monster_ptr->g_ptr->m_idx, tmp);
			effect_monster_ptr->get_angry = TRUE;
		}

		if (effect_monster_ptr->do_time)
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

			if (effect_monster_ptr->do_time >= effect_monster_ptr->m_ptr->maxhp) effect_monster_ptr->do_time = effect_monster_ptr->m_ptr->maxhp - 1;

			if (effect_monster_ptr->do_time)
			{
				effect_monster_ptr->note = _("は弱くなったようだ。", " seems weakened.");
				effect_monster_ptr->m_ptr->maxhp -= effect_monster_ptr->do_time;
				if ((effect_monster_ptr->m_ptr->hp - effect_monster_ptr->dam) > effect_monster_ptr->m_ptr->maxhp) effect_monster_ptr->dam = effect_monster_ptr->m_ptr->hp - effect_monster_ptr->m_ptr->maxhp;
			}

			effect_monster_ptr->get_angry = TRUE;
		}

		if (effect_monster_ptr->do_poly && (randint1(90) > effect_monster_ptr->r_ptr->level))
		{
			if (polymorph_monster(caster_ptr, effect_monster_ptr->y, effect_monster_ptr->x))
			{
				if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

				effect_monster_ptr->note = _("が変身した！", " changes!");
				effect_monster_ptr->dam = 0;
			}

			effect_monster_ptr->m_ptr = &floor_ptr->m_list[effect_monster_ptr->g_ptr->m_idx];
			effect_monster_ptr->r_ptr = &r_info[effect_monster_ptr->m_ptr->r_idx];
		}

		if (effect_monster_ptr->do_dist)
		{
			if (effect_monster_ptr->seen) effect_monster_ptr->obvious = TRUE;

			effect_monster_ptr->note = _("が消え去った！", " disappears!");

			if (!effect_monster_ptr->who) chg_virtue(caster_ptr, V_VALOUR, -1);

			teleport_away(caster_ptr, effect_monster_ptr->g_ptr->m_idx, effect_monster_ptr->do_dist,
				(!effect_monster_ptr->who ? TELEPORT_DEC_VALOUR : 0L) | TELEPORT_PASSIVE);

			effect_monster_ptr->y = effect_monster_ptr->m_ptr->fy;
			effect_monster_ptr->x = effect_monster_ptr->m_ptr->fx;
			effect_monster_ptr->g_ptr = &floor_ptr->grid_array[effect_monster_ptr->y][effect_monster_ptr->x];
		}

		if (effect_monster_ptr->do_fear)
		{
			(void)set_monster_monfear(caster_ptr, effect_monster_ptr->g_ptr->m_idx, MON_MONFEAR(effect_monster_ptr->m_ptr) + effect_monster_ptr->do_fear);
			effect_monster_ptr->get_angry = TRUE;
		}
	}

	if (effect_monster_ptr->effect_type == GF_DRAIN_MANA)
	{
		/* Drain mana does nothing */
	}

	/* If another monster did the damage, hurt the monster by hand */
	else if (effect_monster_ptr->who)
	{
		if (caster_ptr->health_who == effect_monster_ptr->g_ptr->m_idx) caster_ptr->redraw |= (PR_HEALTH);
		if (caster_ptr->riding == effect_monster_ptr->g_ptr->m_idx) caster_ptr->redraw |= (PR_UHEALTH);

		(void)set_monster_csleep(caster_ptr, effect_monster_ptr->g_ptr->m_idx, 0);
		effect_monster_ptr->m_ptr->hp -= effect_monster_ptr->dam;
		if (effect_monster_ptr->m_ptr->hp < 0)
		{
			bool sad = FALSE;

			if (is_pet(effect_monster_ptr->m_ptr) && !(effect_monster_ptr->m_ptr->ml))
				sad = TRUE;

			if (effect_monster_ptr->known && effect_monster_ptr->note)
			{
				monster_desc(caster_ptr, effect_monster_ptr->m_name, effect_monster_ptr->m_ptr, MD_TRUE_NAME);
				if (effect_monster_ptr->see_s_msg)
				{
					msg_format("%^s%s", effect_monster_ptr->m_name, effect_monster_ptr->note);
				}
				else
				{
					floor_ptr->monster_noise = TRUE;
				}
			}

			if (effect_monster_ptr->who > 0) monster_gain_exp(caster_ptr, effect_monster_ptr->who, effect_monster_ptr->m_ptr->r_idx);

			monster_death(caster_ptr, effect_monster_ptr->g_ptr->m_idx, FALSE);
			delete_monster_idx(caster_ptr, effect_monster_ptr->g_ptr->m_idx);
			if (sad)
			{
				msg_print(_("少し悲しい気分がした。", "You feel sad for a moment."));
			}
		}
		else
		{
			if (effect_monster_ptr->note && effect_monster_ptr->seen_msg)
				msg_format("%^s%s", effect_monster_ptr->m_name, effect_monster_ptr->note);
			else if (effect_monster_ptr->see_s_msg)
			{
				message_pain(caster_ptr, effect_monster_ptr->g_ptr->m_idx, effect_monster_ptr->dam);
			}
			else
			{
				floor_ptr->monster_noise = TRUE;
			}

			if (effect_monster_ptr->do_sleep) (void)set_monster_csleep(caster_ptr, effect_monster_ptr->g_ptr->m_idx, effect_monster_ptr->do_sleep);
		}
	}
	else if (effect_monster_ptr->heal_leper)
	{
		if (effect_monster_ptr->seen_msg)
			msg_print(_("不潔な病人は病気が治った！", "The Mangy looking leper is healed!"));

		if (record_named_pet && is_pet(effect_monster_ptr->m_ptr) && effect_monster_ptr->m_ptr->nickname)
		{
			char m2_name[MAX_NLEN];

			monster_desc(caster_ptr, m2_name, effect_monster_ptr->m_ptr, MD_INDEF_VISIBLE);
			exe_write_diary(caster_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_HEAL_LEPER, m2_name);
		}

		delete_monster_idx(caster_ptr, effect_monster_ptr->g_ptr->m_idx);
	}

	/* If the player did it, give him experience, check fear */
	else
	{
		bool fear = FALSE;
		if (mon_take_hit(caster_ptr, effect_monster_ptr->g_ptr->m_idx, effect_monster_ptr->dam, &fear, effect_monster_ptr->note_dies))
		{
			/* Dead monster */
		}
		else
		{
			if (effect_monster_ptr->do_sleep) anger_monster(caster_ptr, effect_monster_ptr->m_ptr);

			if (effect_monster_ptr->note && effect_monster_ptr->seen_msg)
				msg_format(_("%s%s", "%^s%s"), effect_monster_ptr->m_name, effect_monster_ptr->note);
			else if (effect_monster_ptr->known && (effect_monster_ptr->dam || !effect_monster_ptr->do_fear))
			{
				message_pain(caster_ptr, effect_monster_ptr->g_ptr->m_idx, effect_monster_ptr->dam);
			}

			if (((effect_monster_ptr->dam > 0) || effect_monster_ptr->get_angry) && !effect_monster_ptr->do_sleep)
				anger_monster(caster_ptr, effect_monster_ptr->m_ptr);

			if ((fear || effect_monster_ptr->do_fear) && effect_monster_ptr->seen)
			{
				sound(SOUND_FLEE);
				msg_format(_("%^sは恐怖して逃げ出した！", "%^s flees in terror!"), effect_monster_ptr->m_name);
			}

			if (effect_monster_ptr->do_sleep) (void)set_monster_csleep(caster_ptr, effect_monster_ptr->g_ptr->m_idx, effect_monster_ptr->do_sleep);
		}
	}

	if ((effect_monster_ptr->effect_type == GF_BLOOD_CURSE) && one_in_(4))
	{
		blood_curse_to_enemy(caster_ptr, effect_monster_ptr->who);
	}

	if (caster_ptr->phase_out)
	{
		caster_ptr->health_who = effect_monster_ptr->g_ptr->m_idx;
		caster_ptr->redraw |= (PR_HEALTH);
		handle_stuff(caster_ptr);
	}

	if (effect_monster_ptr->m_ptr->r_idx) update_monster(caster_ptr, effect_monster_ptr->g_ptr->m_idx, FALSE);

	lite_spot(caster_ptr, effect_monster_ptr->y, effect_monster_ptr->x);
	if ((caster_ptr->monster_race_idx == effect_monster_ptr->m_ptr->r_idx) && (effect_monster_ptr->seen || !effect_monster_ptr->m_ptr->r_idx))
	{
		caster_ptr->window |= (PW_MONSTER);
	}

	if ((effect_monster_ptr->dam > 0) && !is_pet(effect_monster_ptr->m_ptr) && !is_friendly(effect_monster_ptr->m_ptr))
	{
		if (!effect_monster_ptr->who)
		{
			if (!(effect_monster_ptr->flag & PROJECT_NO_HANGEKI))
			{
				set_target(effect_monster_ptr->m_ptr, monster_target_y, monster_target_x);
			}
		}
		else if ((effect_monster_ptr->who > 0) && is_pet(effect_monster_ptr->m_caster_ptr) && !player_bold(caster_ptr, effect_monster_ptr->m_ptr->target_y, effect_monster_ptr->m_ptr->target_x))
		{
			set_target(effect_monster_ptr->m_ptr, effect_monster_ptr->m_caster_ptr->fy, effect_monster_ptr->m_caster_ptr->fx);
		}
	}

	if (caster_ptr->riding && (caster_ptr->riding == effect_monster_ptr->g_ptr->m_idx) && (effect_monster_ptr->dam > 0))
	{
		if (effect_monster_ptr->m_ptr->hp > effect_monster_ptr->m_ptr->maxhp / 3) effect_monster_ptr->dam = (effect_monster_ptr->dam + 1) / 2;
		rakubadam_m = (effect_monster_ptr->dam > 200) ? 200 : effect_monster_ptr->dam;
	}

	if (effect_monster_ptr->photo)
	{
		object_type *q_ptr;
		object_type forge;
		q_ptr = &forge;
		object_prep(q_ptr, lookup_kind(TV_STATUE, SV_PHOTO));
		q_ptr->pval = effect_monster_ptr->photo;
		q_ptr->ident |= (IDENT_FULL_KNOWN);
		(void)drop_near(caster_ptr, q_ptr, -1, caster_ptr->y, caster_ptr->x);
	}

	project_m_n++;
	project_m_x = effect_monster_ptr->x;
	project_m_y = effect_monster_ptr->y;
	return (effect_monster_ptr->obvious);
}
