

#include "angband.h"
#include "util.h"

#include "artifact.h"
#include "player-move.h"
#include "feature.h"
#include "floor.h"
#include "warning.h"
#include "player-status.h"
#include "monster.h"
#include "monster-spell.h"
#include "spells.h"
#include "object-flavor.h"
#include "dungeon.h"
#include "player-race.h"
#include "player-class.h"
#include "melee.h"
#include "realm-song.h"

/*!
 * @brief 警告を放つアイテムを選択する /
 * Choose one of items that have warning flag
 * Calculate spell damages
 * @return 警告を行う
 */
object_type *choose_warning_item(void)
{
	int i;
	int choices[INVEN_TOTAL - INVEN_RARM];
	int number = 0;

	/* Paranoia -- Player has no warning ability */
	if (!p_ptr->warning) return NULL;

	/* Search Inventory */
	for (i = INVEN_RARM; i < INVEN_TOTAL; i++)
	{
		BIT_FLAGS flgs[TR_FLAG_SIZE];
		object_type *o_ptr = &p_ptr->inventory_list[i];

		object_flags(o_ptr, flgs);
		if (have_flag(flgs, TR_WARNING))
		{
			choices[number] = i;
			number++;
		}
	}

	/* Choice one of them */
	return number ? &p_ptr->inventory_list[choices[randint0(number)]] : NULL;
}

/*!
 * @brief 警告基準を定めるために魔法の効果属性に基づいて最大魔法ダメージを計算する /
 * Calculate spell damages
 * @param m_ptr 魔法を行使するモンスターの構造体参照ポインタ
 * @param typ 効果属性のID
 * @param dam 基本ダメージ
 * @param max 算出した最大ダメージを返すポインタ
 * @return なし
 */
static void spell_damcalc(monster_type *m_ptr, EFFECT_ID typ, HIT_POINT dam, int *max)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	int          rlev = r_ptr->level;
	bool         ignore_wraith_form = FALSE;

	/* Vulnerability, resistance and immunity */
	switch (typ)
	{
	case GF_ELEC:
		if (p_ptr->immune_elec)
		{
			dam = 0;
			ignore_wraith_form = TRUE;
		}
		else
		{
			if (p_ptr->muta3 & MUT3_VULN_ELEM) dam *= 2;
			if (p_ptr->special_defense & KATA_KOUKIJIN) dam += dam / 3;
			if (PRACE_IS_(p_ptr, RACE_ANDROID)) dam += dam / 3;
			if (p_ptr->resist_elec) dam = (dam + 2) / 3;
			if (IS_OPPOSE_ELEC())
				dam = (dam + 2) / 3;
		}
		break;

	case GF_POIS:
		if (p_ptr->resist_pois) dam = (dam + 2) / 3;
		if (IS_OPPOSE_POIS()) dam = (dam + 2) / 3;
		break;

	case GF_ACID:
		if (p_ptr->immune_acid)
		{
			dam = 0;
			ignore_wraith_form = TRUE;
		}
		else
		{
			if (p_ptr->muta3 & MUT3_VULN_ELEM) dam *= 2;
			if (p_ptr->special_defense & KATA_KOUKIJIN) dam += dam / 3;
			if (p_ptr->resist_acid) dam = (dam + 2) / 3;
			if (IS_OPPOSE_ACID()) dam = (dam + 2) / 3;
		}
		break;

	case GF_COLD:
	case GF_ICE:
		if (p_ptr->immune_cold)
		{
			dam = 0;
			ignore_wraith_form = TRUE;
		}
		else
		{
			if (p_ptr->muta3 & MUT3_VULN_ELEM) dam *= 2;
			if (p_ptr->special_defense & KATA_KOUKIJIN) dam += dam / 3;
			if (p_ptr->resist_cold) dam = (dam + 2) / 3;
			if (IS_OPPOSE_COLD()) dam = (dam + 2) / 3;
		}
		break;

	case GF_FIRE:
		if (p_ptr->immune_fire)
		{
			dam = 0;
			ignore_wraith_form = TRUE;
		}
		else
		{
			if (p_ptr->muta3 & MUT3_VULN_ELEM) dam *= 2;
			if (PRACE_IS_(p_ptr, RACE_ENT)) dam += dam / 3;
			if (p_ptr->special_defense & KATA_KOUKIJIN) dam += dam / 3;
			if (p_ptr->resist_fire) dam = (dam + 2) / 3;
			if (IS_OPPOSE_FIRE()) dam = (dam + 2) / 3;
		}
		break;

	case GF_PSY_SPEAR:
		ignore_wraith_form = TRUE;
		break;

	case GF_ARROW:
		if (!p_ptr->blind &&
			((p_ptr->inventory_list[INVEN_RARM].k_idx && (p_ptr->inventory_list[INVEN_RARM].name1 == ART_ZANTETSU)) ||
			(p_ptr->inventory_list[INVEN_LARM].k_idx && (p_ptr->inventory_list[INVEN_LARM].name1 == ART_ZANTETSU))))
		{
			dam = 0;
			ignore_wraith_form = TRUE;
		}
		break;

	case GF_LITE:
		if (p_ptr->resist_lite) dam /= 2; /* Worst case of 4 / (d4 + 7) */
		if (PRACE_IS_(p_ptr, RACE_VAMPIRE) || (p_ptr->mimic_form == MIMIC_VAMPIRE)) dam *= 2;
		else if (PRACE_IS_(p_ptr, RACE_S_FAIRY)) dam = dam * 4 / 3;

		/*
		 * Cannot use "ignore_wraith_form" strictly (for "random one damage")
		 * "dam *= 2;" for later "dam /= 2"
		 */
		if (p_ptr->wraith_form) dam *= 2;
		break;

	case GF_DARK:
		if (PRACE_IS_(p_ptr, RACE_VAMPIRE) || (p_ptr->mimic_form == MIMIC_VAMPIRE) || p_ptr->wraith_form)
		{
			dam = 0;
			ignore_wraith_form = TRUE;
		}
		else if (p_ptr->resist_dark) dam /= 2; /* Worst case of 4 / (d4 + 7) */
		break;

	case GF_SHARDS:
		if (p_ptr->resist_shard) dam = dam * 3 / 4; /* Worst case of 6 / (d4 + 7) */
		break;

	case GF_SOUND:
		if (p_ptr->resist_sound) dam = dam * 5 / 8; /* Worst case of 5 / (d4 + 7) */
		break;

	case GF_CONFUSION:
		if (p_ptr->resist_conf) dam = dam * 5 / 8; /* Worst case of 5 / (d4 + 7) */
		break;

	case GF_CHAOS:
		if (p_ptr->resist_chaos) dam = dam * 3 / 4; /* Worst case of 6 / (d4 + 7) */
		break;

	case GF_NETHER:
		if (PRACE_IS_(p_ptr, RACE_SPECTRE))
		{
			dam = 0;
			ignore_wraith_form = TRUE;
		}
		else if (p_ptr->resist_neth) dam = dam * 3 / 4; /* Worst case of 6 / (d4 + 7) */
		break;

	case GF_DISENCHANT:
		if (p_ptr->resist_disen) dam = dam * 3 / 4; /* Worst case of 6 / (d4 + 7) */
		break;

	case GF_NEXUS:
		if (p_ptr->resist_nexus) dam = dam * 3 / 4; /* Worst case of 6 / (d4 + 7) */
		break;

	case GF_TIME:
		if (p_ptr->resist_time) dam /= 2; /* Worst case of 4 / (d4 + 7) */
		break;

	case GF_GRAVITY:
		if (p_ptr->levitation) dam = (dam * 2) / 3;
		break;

	case GF_ROCKET:
		if (p_ptr->resist_shard) dam /= 2;
		break;

	case GF_NUKE:
		if (p_ptr->resist_pois) dam = (2 * dam + 2) / 5;
		if (IS_OPPOSE_POIS()) dam = (2 * dam + 2) / 5;
		break;

	case GF_DEATH_RAY:
		if (p_ptr->mimic_form)
		{
			if (mimic_info[p_ptr->mimic_form].MIMIC_FLAGS & MIMIC_IS_NONLIVING)
			{
				dam = 0;
				ignore_wraith_form = TRUE;
			}
		}
		else
		{
			switch (p_ptr->prace)
			{
			case RACE_GOLEM:
			case RACE_SKELETON:
			case RACE_ZOMBIE:
			case RACE_VAMPIRE:
			case RACE_DEMON:
			case RACE_SPECTRE:
				dam = 0;
				ignore_wraith_form = TRUE;
				break;
			}
		}
		break;

	case GF_HOLY_FIRE:
		if (p_ptr->align > 10) dam /= 2;
		else if (p_ptr->align < -10) dam *= 2;
		break;

	case GF_HELL_FIRE:
		if (p_ptr->align > 10) dam *= 2;
		break;

	case GF_MIND_BLAST:
	case GF_BRAIN_SMASH:
		if (100 + rlev / 2 <= MAX(5, p_ptr->skill_sav))
		{
			dam = 0;
			ignore_wraith_form = TRUE;
		}
		break;

	case GF_CAUSE_1:
	case GF_CAUSE_2:
	case GF_CAUSE_3:
	case GF_HAND_DOOM:
		if (100 + rlev / 2 <= p_ptr->skill_sav)
		{
			dam = 0;
			ignore_wraith_form = TRUE;
		}
		break;

	case GF_CAUSE_4:
		if ((100 + rlev / 2 <= p_ptr->skill_sav) && (m_ptr->r_idx != MON_KENSHIROU))
		{
			dam = 0;
			ignore_wraith_form = TRUE;
		}
		break;
	}

	if (p_ptr->wraith_form && !ignore_wraith_form)
	{
		dam /= 2;
		if (!dam) dam = 1;
	}

	if (dam > *max) *max = dam;
}

/*!
* @brief 警告基準を定めるために魔法の効果属性に基づいて最大魔法ダメージを計算する。 /
* Calculate spell damages
* @param spell_num RF4ならRF4_SPELL_STARTのように32区切りのベースとなる数値
* @param typ 効果属性のID
* @param m_idx 魔法を行使するモンスターのID
* @param max 算出した最大ダメージを返すポインタ
* @return なし
*/
void spell_damcalc_by_spellnum(int spell_num, EFFECT_ID typ, MONSTER_IDX m_idx, int *max)
{
	monster_type *m_ptr = &current_floor_ptr->m_list[m_idx];
	HIT_POINT dam = monspell_damage((spell_num), m_idx, DAM_MAX);
	spell_damcalc(m_ptr, typ, dam, max);
}

/*!
 * @brief 警告基準を定めるためにモンスターの打撃最大ダメージを算出する /
 * Calculate blow damages
 * @param m_ptr 打撃を行使するモンスターの構造体参照ポインタ
 * @param blow_ptr モンスターの打撃能力の構造体参照ポインタ
 * @return 算出された最大ダメージを返す。
 */
static int blow_damcalc(monster_type *m_ptr, monster_blow *blow_ptr)
{
	int  dam = blow_ptr->d_dice * blow_ptr->d_side;
	int  dummy_max = 0;
	bool check_wraith_form = TRUE;

	if (blow_ptr->method != RBM_EXPLODE)
	{
		ARMOUR_CLASS ac = p_ptr->ac + p_ptr->to_a;

		switch (blow_ptr->effect)
		{
		case RBE_SUPERHURT:
		{
			int tmp_dam = dam - (dam * ((ac < 150) ? ac : 150) / 250);
			dam = MAX(dam, tmp_dam * 2);
			break;
		}

		case RBE_HURT:
		case RBE_SHATTER:
			dam -= (dam * ((ac < 150) ? ac : 150) / 250);
			break;

		case RBE_ACID:
			spell_damcalc(m_ptr, GF_ACID, dam, &dummy_max);
			dam = dummy_max;
			check_wraith_form = FALSE;
			break;

		case RBE_ELEC:
			spell_damcalc(m_ptr, GF_ELEC, dam, &dummy_max);
			dam = dummy_max;
			check_wraith_form = FALSE;
			break;

		case RBE_FIRE:
			spell_damcalc(m_ptr, GF_FIRE, dam, &dummy_max);
			dam = dummy_max;
			check_wraith_form = FALSE;
			break;

		case RBE_COLD:
			spell_damcalc(m_ptr, GF_COLD, dam, &dummy_max);
			dam = dummy_max;
			check_wraith_form = FALSE;
			break;

		case RBE_DR_MANA:
			dam = 0;
			check_wraith_form = FALSE;
			break;
		}

		if (check_wraith_form && p_ptr->wraith_form)
		{
			dam /= 2;
			if (!dam) dam = 1;
		}
	}
	else
	{
		dam = (dam + 1) / 2;
		spell_damcalc(m_ptr, mbe_info[blow_ptr->effect].explode_type, dam, &dummy_max);
		dam = dummy_max;
	}

	return dam;
}

/*!
 * @brief プレイヤーが特定地点へ移動した場合に警告を発する処理 /
 * Examine the grid (xx,yy) and warn the player if there are any danger
 * @param xx 危険性を調査するマスのX座標
 * @param yy 危険性を調査するマスのY座標
 * @return 警告を無視して進むことを選択するかか問題が無ければTRUE、警告に従ったならFALSEを返す。
 */
bool process_warning(POSITION xx, POSITION yy)
{
	POSITION mx, my;
	grid_type *g_ptr;
	GAME_TEXT o_name[MAX_NLEN];

#define WARNING_AWARE_RANGE 12
	int dam_max = 0;
	static int old_damage = 0;

	for (mx = xx - WARNING_AWARE_RANGE; mx < xx + WARNING_AWARE_RANGE + 1; mx++)
	{
		for (my = yy - WARNING_AWARE_RANGE; my < yy + WARNING_AWARE_RANGE + 1; my++)
		{
			int dam_max0 = 0;
			monster_type *m_ptr;
			monster_race *r_ptr;

			if (!in_bounds(my, mx) || (distance(my, mx, yy, xx) > WARNING_AWARE_RANGE)) continue;

			g_ptr = &current_floor_ptr->grid_array[my][mx];

			if (!g_ptr->m_idx) continue;

			m_ptr = &current_floor_ptr->m_list[g_ptr->m_idx];

			if (MON_CSLEEP(m_ptr)) continue;
			if (!is_hostile(m_ptr)) continue;

			r_ptr = &r_info[m_ptr->r_idx];

			/* Monster spells (only powerful ones)*/
			if (projectable(my, mx, yy, xx))
			{
				BIT_FLAGS f4 = r_ptr->flags4;
				BIT_FLAGS f5 = r_ptr->a_ability_flags1;
				BIT_FLAGS f6 = r_ptr->a_ability_flags2;

				if (!(d_info[p_ptr->dungeon_idx].flags1 & DF1_NO_MAGIC))
				{
					if (f4 & RF4_BA_CHAO) spell_damcalc_by_spellnum(MS_BALL_CHAOS, GF_CHAOS, g_ptr->m_idx, &dam_max0);
					if (f5 & RF5_BA_MANA) spell_damcalc_by_spellnum(MS_BALL_MANA, GF_MANA, g_ptr->m_idx, &dam_max0);
					if (f5 & RF5_BA_DARK) spell_damcalc_by_spellnum(MS_BALL_DARK, GF_DARK, g_ptr->m_idx, &dam_max0);
					if (f5 & RF5_BA_LITE) spell_damcalc_by_spellnum(MS_STARBURST, GF_LITE, g_ptr->m_idx, &dam_max0);
					if (f6 & RF6_HAND_DOOM) spell_damcalc_by_spellnum(MS_HAND_DOOM, GF_HAND_DOOM, g_ptr->m_idx, &dam_max0);
					if (f6 & RF6_PSY_SPEAR) spell_damcalc_by_spellnum(MS_PSY_SPEAR, GF_PSY_SPEAR, g_ptr->m_idx, &dam_max0);
				}
				if (f4 & RF4_ROCKET) spell_damcalc_by_spellnum(MS_ROCKET, GF_ROCKET, g_ptr->m_idx, &dam_max0);
				if (f4 & RF4_BR_ACID) spell_damcalc_by_spellnum(MS_BR_ACID, GF_ACID, g_ptr->m_idx, &dam_max0);
				if (f4 & RF4_BR_ELEC) spell_damcalc_by_spellnum(MS_BR_ELEC, GF_ELEC, g_ptr->m_idx, &dam_max0);
				if (f4 & RF4_BR_FIRE) spell_damcalc_by_spellnum(MS_BR_FIRE, GF_FIRE, g_ptr->m_idx, &dam_max0);
				if (f4 & RF4_BR_COLD) spell_damcalc_by_spellnum(MS_BR_COLD, GF_COLD, g_ptr->m_idx, &dam_max0);
				if (f4 & RF4_BR_POIS) spell_damcalc_by_spellnum(MS_BR_POIS, GF_POIS, g_ptr->m_idx, &dam_max0);
				if (f4 & RF4_BR_NETH) spell_damcalc_by_spellnum(MS_BR_NETHER, GF_NETHER, g_ptr->m_idx, &dam_max0);
				if (f4 & RF4_BR_LITE) spell_damcalc_by_spellnum(MS_BR_LITE, GF_LITE, g_ptr->m_idx, &dam_max0);
				if (f4 & RF4_BR_DARK) spell_damcalc_by_spellnum(MS_BR_DARK, GF_DARK, g_ptr->m_idx, &dam_max0);
				if (f4 & RF4_BR_CONF) spell_damcalc_by_spellnum(MS_BR_CONF, GF_CONFUSION, g_ptr->m_idx, &dam_max0);
				if (f4 & RF4_BR_SOUN) spell_damcalc_by_spellnum(MS_BR_SOUND, GF_SOUND, g_ptr->m_idx, &dam_max0);
				if (f4 & RF4_BR_CHAO) spell_damcalc_by_spellnum(MS_BR_CHAOS, GF_CHAOS, g_ptr->m_idx, &dam_max0);
				if (f4 & RF4_BR_DISE) spell_damcalc_by_spellnum(MS_BR_DISEN, GF_DISENCHANT, g_ptr->m_idx, &dam_max0);
				if (f4 & RF4_BR_NEXU) spell_damcalc_by_spellnum(MS_BR_NEXUS, GF_NEXUS, g_ptr->m_idx, &dam_max0);
				if (f4 & RF4_BR_TIME) spell_damcalc_by_spellnum(MS_BR_TIME, GF_TIME, g_ptr->m_idx, &dam_max0);
				if (f4 & RF4_BR_INER) spell_damcalc_by_spellnum(MS_BR_INERTIA, GF_INERTIAL, g_ptr->m_idx, &dam_max0);
				if (f4 & RF4_BR_GRAV) spell_damcalc_by_spellnum(MS_BR_GRAVITY, GF_GRAVITY, g_ptr->m_idx, &dam_max0);
				if (f4 & RF4_BR_SHAR) spell_damcalc_by_spellnum(MS_BR_SHARDS, GF_SHARDS, g_ptr->m_idx, &dam_max0);
				if (f4 & RF4_BR_PLAS) spell_damcalc_by_spellnum(MS_BR_PLASMA, GF_PLASMA, g_ptr->m_idx, &dam_max0);
				if (f4 & RF4_BR_WALL) spell_damcalc_by_spellnum(MS_BR_FORCE, GF_FORCE, g_ptr->m_idx, &dam_max0);
				if (f4 & RF4_BR_MANA) spell_damcalc_by_spellnum(MS_BR_MANA, GF_MANA, g_ptr->m_idx, &dam_max0);
				if (f4 & RF4_BR_NUKE) spell_damcalc_by_spellnum(MS_BR_NUKE, GF_NUKE, g_ptr->m_idx, &dam_max0);
				if (f4 & RF4_BR_DISI) spell_damcalc_by_spellnum(MS_BR_DISI, GF_DISINTEGRATE, g_ptr->m_idx, &dam_max0);
			}

			/* Monster melee attacks */
			if (!(r_ptr->flags1 & RF1_NEVER_BLOW) && !(d_info[p_ptr->dungeon_idx].flags1 & DF1_NO_MELEE))
			{
				if (mx <= xx + 1 && mx >= xx - 1 && my <= yy + 1 && my >= yy - 1)
				{
					int m;
					int dam_melee = 0;
					for (m = 0; m < 4; m++)
					{
						/* Skip non-attacks */
						if (!r_ptr->blow[m].method || (r_ptr->blow[m].method == RBM_SHOOT)) continue;

						/* Extract the attack info */
						dam_melee += blow_damcalc(m_ptr, &r_ptr->blow[m]);
						if (r_ptr->blow[m].method == RBM_EXPLODE) break;
					}
					if (dam_melee > dam_max0) dam_max0 = dam_melee;
				}
			}

			/* Contribution from this monster */
			dam_max += dam_max0;
		}
	}

	/* Prevent excessive warning */
	if (dam_max > old_damage)
	{
		old_damage = dam_max * 3 / 2;

		if (dam_max > p_ptr->chp / 2)
		{
			object_type *o_ptr = choose_warning_item();

			if (o_ptr)
				object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
			else
				strcpy(o_name, _("体", "body")); /* Warning ability without item */
			msg_format(_("%sが鋭く震えた！", "Your %s pulsates sharply!"), o_name);

			disturb(FALSE, TRUE);
			return get_check(_("本当にこのまま進むか？", "Really want to go ahead? "));
		}
	}
	else old_damage = old_damage / 2;

	g_ptr = &current_floor_ptr->grid_array[yy][xx];
	if (((!easy_disarm && is_trap(g_ptr->feat))
		|| (g_ptr->mimic && is_trap(g_ptr->feat))) && !one_in_(13))
	{
		object_type *o_ptr = choose_warning_item();

		if (o_ptr)
			object_desc(o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
		else
			strcpy(o_name, _("体", "body")); /* Warning ability without item */
		msg_format(_("%sが鋭く震えた！", "Your %s pulsates sharply!"), o_name);
		disturb(FALSE, TRUE);
		return get_check(_("本当にこのまま進むか？", "Really want to go ahead? "));
	}

	return TRUE;
}

