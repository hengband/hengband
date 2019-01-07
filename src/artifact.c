/*!
 * @file artifact.c
 * @brief アーティファクトの生成と管理 / Artifact code
 * @date 2013/12/11
 * @author
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research, and\n
 * not for profit purposes provided that this copyright and statement are\n
 * included in all such copies.\n
 * 2013 Deskull rearranged comment for Doxygen.
 */

#include "angband.h"
#include "cmd-activate.h"
#include "object-curse.h"

static bool suppression_evil_dam(object_type *o_ptr);
static bool weakening_artifact(object_type *o_ptr);


/*!
 * @brief 対象のオブジェクトにランダムな上位耐性を一つ付加する。/ Choose one random high resistance
 * @details 重複の抑止はない。候補は毒、閃光、暗黒、破片、盲目、混乱、地獄、因果混乱、カオス、劣化、恐怖のいずれか。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return なし
 */
void one_high_resistance(object_type *o_ptr)
{
	switch (randint0(12))
	{
		case  0: add_flag(o_ptr->art_flags, TR_RES_POIS);   break;
		case  1: add_flag(o_ptr->art_flags, TR_RES_LITE);   break;
		case  2: add_flag(o_ptr->art_flags, TR_RES_DARK);   break;
		case  3: add_flag(o_ptr->art_flags, TR_RES_SHARDS); break;
		case  4: add_flag(o_ptr->art_flags, TR_RES_BLIND);  break;
		case  5: add_flag(o_ptr->art_flags, TR_RES_CONF);   break;
		case  6: add_flag(o_ptr->art_flags, TR_RES_SOUND);  break;
		case  7: add_flag(o_ptr->art_flags, TR_RES_NETHER); break;
		case  8: add_flag(o_ptr->art_flags, TR_RES_NEXUS);  break;
		case  9: add_flag(o_ptr->art_flags, TR_RES_CHAOS);  break;
		case 10: add_flag(o_ptr->art_flags, TR_RES_DISEN);  break;
		case 11: add_flag(o_ptr->art_flags, TR_RES_FEAR);   break;
	}
}

/*!
 * @brief 対象のオブジェクトに王者の指輪向けの上位耐性を一つ付加する。/ Choose one random high resistance
 * @details 候補は閃光、暗黒、破片、盲目、混乱、地獄、因果混乱、カオス、恐怖であり
 * 王者の指輪にあらかじめついている耐性をone_high_resistance()から除外したものである。
 * ランダム付加そのものに重複の抑止はない。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return なし
 */
void one_lordly_high_resistance(object_type *o_ptr)
{
	switch (randint0(10))
	{
		case 0: add_flag(o_ptr->art_flags, TR_RES_LITE);   break;
		case 1: add_flag(o_ptr->art_flags, TR_RES_DARK);   break;
		case 2: add_flag(o_ptr->art_flags, TR_RES_SHARDS); break;
		case 3: add_flag(o_ptr->art_flags, TR_RES_BLIND);  break;
		case 4: add_flag(o_ptr->art_flags, TR_RES_CONF);   break;
		case 5: add_flag(o_ptr->art_flags, TR_RES_SOUND);  break;
		case 6: add_flag(o_ptr->art_flags, TR_RES_NETHER); break;
		case 7: add_flag(o_ptr->art_flags, TR_RES_NEXUS);  break;
		case 8: add_flag(o_ptr->art_flags, TR_RES_CHAOS);  break;
		case 9: add_flag(o_ptr->art_flags, TR_RES_FEAR);   break;
	}
}

/*!
 * @brief 対象のオブジェクトに元素耐性を一つ付加する。/ Choose one random element resistance
 * @details 候補は火炎、冷気、電撃、酸のいずれかであり、重複の抑止はない。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return なし
 */
void one_ele_resistance(object_type *o_ptr)
{
	switch (randint0(4))
	{
		case  0: add_flag(o_ptr->art_flags, TR_RES_ACID); break;
		case  1: add_flag(o_ptr->art_flags, TR_RES_ELEC); break;
		case  2: add_flag(o_ptr->art_flags, TR_RES_COLD); break;
		case  3: add_flag(o_ptr->art_flags, TR_RES_FIRE); break;
	}
}

/*!
 * @brief 対象のオブジェクトにドラゴン装備向け元素耐性を一つ付加する。/ Choose one random element or poison resistance
 * @details 候補は1/7の確率で毒、6/7の確率で火炎、冷気、電撃、酸のいずれか(one_ele_resistance()のコール)であり、重複の抑止はない。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return なし
 */
void one_dragon_ele_resistance(object_type *o_ptr)
{
	if (one_in_(7))
	{
		add_flag(o_ptr->art_flags, TR_RES_POIS);
	}
	else
	{
		one_ele_resistance(o_ptr);
	}
}

/*!
 * @brief 対象のオブジェクトに弱いESPを一つ付加する。/ Choose one lower rank esp
 * @details 候補は動物、アンデッド、悪魔、オーク、トロル、巨人、
 * ドラゴン、人間、善良、ユニークESPのいずれかであり、重複の抑止はない。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return なし
 */
void one_low_esp(object_type *o_ptr)
{
	switch (randint1(10))
	{
		case 1:  add_flag(o_ptr->art_flags, TR_ESP_ANIMAL);   break;
		case 2:  add_flag(o_ptr->art_flags, TR_ESP_UNDEAD);   break;
		case 3:  add_flag(o_ptr->art_flags, TR_ESP_DEMON);   break;
		case 4:  add_flag(o_ptr->art_flags, TR_ESP_ORC);   break;
		case 5:  add_flag(o_ptr->art_flags, TR_ESP_TROLL);   break;
		case 6:  add_flag(o_ptr->art_flags, TR_ESP_GIANT);   break;
		case 7:  add_flag(o_ptr->art_flags, TR_ESP_DRAGON);   break;
		case 8:  add_flag(o_ptr->art_flags, TR_ESP_HUMAN);   break;
		case 9:  add_flag(o_ptr->art_flags, TR_ESP_GOOD);   break;
		case 10: add_flag(o_ptr->art_flags, TR_ESP_UNIQUE);   break;
	}
}


/*!
 * @brief 対象のオブジェクトに耐性を一つ付加する。/ Choose one random resistance
 * @details 1/3で元素耐性(one_ele_resistance())、2/3で上位耐性(one_high_resistance)
 * をコールする。重複の抑止はない。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return なし
 */
void one_resistance(object_type *o_ptr)
{
	if (one_in_(3))
	{
		one_ele_resistance(o_ptr);
	}
	else
	{
		one_high_resistance(o_ptr);
	}
}


/*!
 * @brief 対象のオブジェクトに能力を一つ付加する。/ Choose one random ability
 * @details 候補は浮遊、永久光源+1、透明視、警告、遅消化、急回復、麻痺知らず、経験値維持のいずれか。
 * 重複の抑止はない。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return なし
 */
void one_ability(object_type *o_ptr)
{
	switch (randint0(10))
	{
	case 0: add_flag(o_ptr->art_flags, TR_LEVITATION);  break;
	case 1: add_flag(o_ptr->art_flags, TR_LITE_1);      break;
	case 2: add_flag(o_ptr->art_flags, TR_SEE_INVIS);   break;
	case 3: add_flag(o_ptr->art_flags, TR_WARNING);     break;
	case 4: add_flag(o_ptr->art_flags, TR_SLOW_DIGEST); break;
	case 5: add_flag(o_ptr->art_flags, TR_REGEN);       break;
	case 6: add_flag(o_ptr->art_flags, TR_FREE_ACT);    break;
	case 7: add_flag(o_ptr->art_flags, TR_HOLD_EXP);   break;
	case 8:
	case 9:
		one_low_esp(o_ptr);
		break;
	}
}

/*!
 * @brief 対象のオブジェクトに発動を一つ付加する。/ Choose one random activation
 * @details 候補多数。ランダムアーティファクトのバイアスには一切依存せず、
 * whileループによる構造で能力的に強力なものほど確率を落としている。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return なし
 */
void one_activation(object_type *o_ptr)
{
	int type = 0;
	int chance = 0;

	while (randint1(100) >= chance)
	{
		type = randint1(255);
		switch (type)
		{
			case ACT_SUNLIGHT:
			case ACT_BO_MISS_1:
			case ACT_BA_POIS_1:
			case ACT_BO_ELEC_1:
			case ACT_BO_ACID_1:
			case ACT_BO_COLD_1:
			case ACT_BO_FIRE_1:
			case ACT_CONFUSE:
			case ACT_SLEEP:
			case ACT_QUAKE:
			case ACT_CURE_LW:
			case ACT_CURE_MW:
			case ACT_CURE_POISON:
			case ACT_BERSERK:
			case ACT_LIGHT:
			case ACT_MAP_LIGHT:
			case ACT_DEST_DOOR:
			case ACT_STONE_MUD:
			case ACT_TELEPORT:
				chance = 101;
				break;
			case ACT_BA_COLD_1:
			case ACT_BA_FIRE_1:
			case ACT_HYPODYNAMIA_1:
			case ACT_TELE_AWAY:
			case ACT_ESP:
			case ACT_RESIST_ALL:
			case ACT_DETECT_ALL:
			case ACT_RECALL:
			case ACT_SATIATE:
			case ACT_RECHARGE:
				chance = 85;
				break;
			case ACT_TERROR:
			case ACT_PROT_EVIL:
			case ACT_ID_PLAIN:
				chance = 75;
				break;
			case ACT_HYPODYNAMIA_2:
			case ACT_DRAIN_1:
			case ACT_BO_MISS_2:
			case ACT_BA_FIRE_2:
			case ACT_REST_EXP:
				chance = 66;
				break;
			case ACT_BA_FIRE_3:
			case ACT_BA_COLD_3:
			case ACT_BA_ELEC_3:
			case ACT_WHIRLWIND:
			case ACT_DRAIN_2:
			case ACT_CHARM_ANIMAL:
				chance = 50;
				break;
			case ACT_SUMMON_ANIMAL:
				chance = 40;
				break;
			case ACT_DISP_EVIL:
			case ACT_BA_MISS_3:
			case ACT_DISP_GOOD:
			case ACT_BANISH_EVIL:
			case ACT_GENOCIDE:
			case ACT_MASS_GENO:
			case ACT_CHARM_UNDEAD:
			case ACT_CHARM_OTHER:
			case ACT_SUMMON_PHANTOM:
			case ACT_REST_ALL:
			case ACT_RUNE_EXPLO:
				chance = 33;
				break;
			case ACT_CALL_CHAOS:
			case ACT_ROCKET:
			case ACT_CHARM_ANIMALS:
			case ACT_CHARM_OTHERS:
			case ACT_SUMMON_ELEMENTAL:
			case ACT_CURE_700:
			case ACT_SPEED:
			case ACT_ID_FULL:
			case ACT_RUNE_PROT:
				chance = 25;
				break;
			case ACT_CURE_1000:
			case ACT_XTRA_SPEED:
			case ACT_DETECT_XTRA:
			case ACT_DIM_DOOR:
				chance = 10;
				break;
			case ACT_SUMMON_UNDEAD:
			case ACT_SUMMON_DEMON:
			case ACT_WRAITH:
			case ACT_INVULN:
			case ACT_ALCHEMY:
				chance = 5;
				break;
			default:
				chance = 0;
		}
	}

	/* A type was chosen... */
	o_ptr->xtra2 = (byte_hack)type;
	add_flag(o_ptr->art_flags, TR_ACTIVATE);
	o_ptr->timeout = 0;
}

/*!
 * @brief ランダムアーティファクト生成中、対象のオブジェクトを呪いのアーティファクトにする経過処理。/ generation process of cursed artifact.
 * @details pval、AC、命中、ダメージが正の場合、符号反転の上1d4だけ悪化させ、重い呪い、呪いフラグを必ず付加。
 * 祝福を無効。確率に応じて、永遠の呪い、太古の怨念、経験値吸収、弱い呪いの継続的付加、強い呪いの継続的付加、HP吸収の呪い、
 * MP吸収の呪い、乱テレポート、反テレポート、反魔法をつける。
 * @attention プレイヤーの職業依存処理あり。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return なし
 */
static void curse_artifact(object_type * o_ptr)
{
	if (o_ptr->pval > 0) o_ptr->pval = 0 - (o_ptr->pval + randint1(4));
	if (o_ptr->to_a > 0) o_ptr->to_a = 0 - (o_ptr->to_a + randint1(4));
	if (o_ptr->to_h > 0) o_ptr->to_h = 0 - (o_ptr->to_h + randint1(4));
	if (o_ptr->to_d > 0) o_ptr->to_d = 0 - (o_ptr->to_d + randint1(4));

	o_ptr->curse_flags |= (TRC_HEAVY_CURSE | TRC_CURSED);
	remove_flag(o_ptr->art_flags, TR_BLESSED);

	if (one_in_(4)) o_ptr->curse_flags |= TRC_PERMA_CURSE;
	if (one_in_(3)) add_flag(o_ptr->art_flags, TR_TY_CURSE);
	if (one_in_(2)) add_flag(o_ptr->art_flags, TR_AGGRAVATE);
	if (one_in_(3)) add_flag(o_ptr->art_flags, TR_DRAIN_EXP);
	if (one_in_(6)) add_flag(o_ptr->art_flags, TR_ADD_L_CURSE);
	if (one_in_(9)) add_flag(o_ptr->art_flags, TR_ADD_H_CURSE);
	if (one_in_(9)) add_flag(o_ptr->art_flags, TR_DRAIN_HP);
	if (one_in_(9)) add_flag(o_ptr->art_flags, TR_DRAIN_MANA);
	if (one_in_(2)) add_flag(o_ptr->art_flags, TR_TELEPORT);
	else if (one_in_(3)) add_flag(o_ptr->art_flags, TR_NO_TELE);

	if ((p_ptr->pclass != CLASS_WARRIOR) && (p_ptr->pclass != CLASS_ARCHER) && (p_ptr->pclass != CLASS_CAVALRY) && (p_ptr->pclass != CLASS_BERSERKER) && (p_ptr->pclass != CLASS_SMITH) && one_in_(3))
		add_flag(o_ptr->art_flags, TR_NO_MAGIC);
}

/*!
 * @brief ランダムアーティファクト生成中、対象のオブジェクトにpval能力を付加する。/ Add one pval on generation of randam artifact.
 * @details 優先的に付加されるpvalがランダムアーティファクトバイアスに依存して存在する。
 * 原則的候補は腕力、知力、賢さ、器用さ、耐久、魅力、探索、隠密、赤外線視力、加速。武器のみ採掘、追加攻撃も候補に入る。
 * @attention オブジェクトのtval、svalに依存したハードコーディング処理がある。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return なし
 */
static void random_plus(object_type * o_ptr)
{
	int this_type = (object_is_weapon_ammo(o_ptr) ? 23 : 19);

	switch (o_ptr->artifact_bias)
	{
	case BIAS_WARRIOR:
		if (!(have_flag(o_ptr->art_flags, TR_STR)))
		{
			add_flag(o_ptr->art_flags, TR_STR);
			if (one_in_(2)) return;
		}

		if (!(have_flag(o_ptr->art_flags, TR_CON)))
		{
			add_flag(o_ptr->art_flags, TR_CON);
			if (one_in_(2)) return;
		}

		if (!(have_flag(o_ptr->art_flags, TR_DEX)))
		{
			add_flag(o_ptr->art_flags, TR_DEX);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_MAGE:
		if (!(have_flag(o_ptr->art_flags, TR_INT)))
		{
			add_flag(o_ptr->art_flags, TR_INT);
			if (one_in_(2)) return;
		}
		if ((o_ptr->tval == TV_GLOVES) && !(have_flag(o_ptr->art_flags, TR_MAGIC_MASTERY)))
		{
			add_flag(o_ptr->art_flags, TR_MAGIC_MASTERY);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_PRIESTLY:
		if (!(have_flag(o_ptr->art_flags, TR_WIS)))
		{
			add_flag(o_ptr->art_flags, TR_WIS);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_RANGER:
		if (!(have_flag(o_ptr->art_flags, TR_DEX)))
		{
			add_flag(o_ptr->art_flags, TR_DEX);
			if (one_in_(2)) return;
		}

		if (!(have_flag(o_ptr->art_flags, TR_CON)))
		{
			add_flag(o_ptr->art_flags, TR_CON);
			if (one_in_(2)) return;
		}

		if (!(have_flag(o_ptr->art_flags, TR_STR)))
		{
			add_flag(o_ptr->art_flags, TR_STR);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_ROGUE:
		if (!(have_flag(o_ptr->art_flags, TR_STEALTH)))
		{
			add_flag(o_ptr->art_flags, TR_STEALTH);
			if (one_in_(2)) return;
		}
		if (!(have_flag(o_ptr->art_flags, TR_SEARCH)))
		{
			add_flag(o_ptr->art_flags, TR_SEARCH);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_STR:
		if (!(have_flag(o_ptr->art_flags, TR_STR)))
		{
			add_flag(o_ptr->art_flags, TR_STR);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_WIS:
		if (!(have_flag(o_ptr->art_flags, TR_WIS)))
		{
			add_flag(o_ptr->art_flags, TR_WIS);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_INT:
		if (!(have_flag(o_ptr->art_flags, TR_INT)))
		{
			add_flag(o_ptr->art_flags, TR_INT);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_DEX:
		if (!(have_flag(o_ptr->art_flags, TR_DEX)))
		{
			add_flag(o_ptr->art_flags, TR_DEX);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_CON:
		if (!(have_flag(o_ptr->art_flags, TR_CON)))
		{
			add_flag(o_ptr->art_flags, TR_CON);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_CHR:
		if (!(have_flag(o_ptr->art_flags, TR_CHR)))
		{
			add_flag(o_ptr->art_flags, TR_CHR);
			if (one_in_(2)) return;
		}
		break;
	}

	if ((o_ptr->artifact_bias == BIAS_MAGE || o_ptr->artifact_bias == BIAS_PRIESTLY) && (o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_ROBE))
	{
		if (!(have_flag(o_ptr->art_flags, TR_DEC_MANA)) && one_in_(3))
		{
			add_flag(o_ptr->art_flags, TR_DEC_MANA);
			if (one_in_(2)) return;
		}
	}

	switch (randint1(this_type))
	{
	case 1: case 2:
		add_flag(o_ptr->art_flags, TR_STR);
		if (!o_ptr->artifact_bias && !one_in_(13))
			o_ptr->artifact_bias = BIAS_STR;
		else if (!o_ptr->artifact_bias && one_in_(7))
			o_ptr->artifact_bias = BIAS_WARRIOR;
		break;
	case 3: case 4:
		add_flag(o_ptr->art_flags, TR_INT);
		if (!o_ptr->artifact_bias && !one_in_(13))
			o_ptr->artifact_bias = BIAS_INT;
		else if (!o_ptr->artifact_bias && one_in_(7))
			o_ptr->artifact_bias = BIAS_MAGE;
		break;
	case 5: case 6:
		add_flag(o_ptr->art_flags, TR_WIS);
		if (!o_ptr->artifact_bias && !one_in_(13))
			o_ptr->artifact_bias = BIAS_WIS;
		else if (!o_ptr->artifact_bias && one_in_(7))
			o_ptr->artifact_bias = BIAS_PRIESTLY;
		break;
	case 7: case 8:
		add_flag(o_ptr->art_flags, TR_DEX);
		if (!o_ptr->artifact_bias && !one_in_(13))
			o_ptr->artifact_bias = BIAS_DEX;
		else if (!o_ptr->artifact_bias && one_in_(7))
			o_ptr->artifact_bias = BIAS_ROGUE;
		break;
	case 9: case 10:
		add_flag(o_ptr->art_flags, TR_CON);
		if (!o_ptr->artifact_bias && !one_in_(13))
			o_ptr->artifact_bias = BIAS_CON;
		else if (!o_ptr->artifact_bias && one_in_(9))
			o_ptr->artifact_bias = BIAS_RANGER;
		break;
	case 11: case 12:
		add_flag(o_ptr->art_flags, TR_CHR);
		if (!o_ptr->artifact_bias && !one_in_(13))
			o_ptr->artifact_bias = BIAS_CHR;
		break;
	case 13: case 14:
		add_flag(o_ptr->art_flags, TR_STEALTH);
		if (!o_ptr->artifact_bias && one_in_(3))
			o_ptr->artifact_bias = BIAS_ROGUE;
		break;
	case 15: case 16:
		add_flag(o_ptr->art_flags, TR_SEARCH);
		if (!o_ptr->artifact_bias && one_in_(9))
			o_ptr->artifact_bias = BIAS_RANGER;
		break;
	case 17: case 18:
		add_flag(o_ptr->art_flags, TR_INFRA);
		break;
	case 19:
		add_flag(o_ptr->art_flags, TR_SPEED);
		if (!o_ptr->artifact_bias && one_in_(11))
			o_ptr->artifact_bias = BIAS_ROGUE;
		break;
	case 20: case 21:
		add_flag(o_ptr->art_flags, TR_TUNNEL);
		break;
	case 22: case 23:
		if (o_ptr->tval == TV_BOW) random_plus(o_ptr);
		else
		{
			add_flag(o_ptr->art_flags, TR_BLOWS);
			if (!o_ptr->artifact_bias && one_in_(11))
				o_ptr->artifact_bias = BIAS_WARRIOR;
		}
		break;
	}
}

/*!
 * @brief ランダムアーティファクト生成中、対象のオブジェクトに耐性を付加する。/ Add one resistance on generation of randam artifact.
 * @details 優先的に付加される耐性がランダムアーティファクトバイアスに依存して存在する。
 * 原則的候補は火炎、冷気、電撃、酸（以上免疫の可能性もあり）、
 * 毒、閃光、暗黒、破片、轟音、盲目、混乱、地獄、カオス、劣化、恐怖、火オーラ、冷気オーラ、電撃オーラ、反射。
 * 戦士系バイアスのみ反魔もつく。
 * @attention オブジェクトのtval、svalに依存したハードコーディング処理がある。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return なし
 */
static void random_resistance(object_type * o_ptr)
{
	switch (o_ptr->artifact_bias)
	{
	case BIAS_ACID:
		if (!(have_flag(o_ptr->art_flags, TR_RES_ACID)))
		{
			add_flag(o_ptr->art_flags, TR_RES_ACID);
			if (one_in_(2)) return;
		}
		if (one_in_(BIAS_LUCK) && !(have_flag(o_ptr->art_flags, TR_IM_ACID)))
		{
			add_flag(o_ptr->art_flags, TR_IM_ACID);
			if (!one_in_(IM_LUCK))
			{
				remove_flag(o_ptr->art_flags, TR_IM_ELEC);
				remove_flag(o_ptr->art_flags, TR_IM_COLD);
				remove_flag(o_ptr->art_flags, TR_IM_FIRE);
			}
			if (one_in_(2)) return;
		}
		break;

	case BIAS_ELEC:
		if (!(have_flag(o_ptr->art_flags, TR_RES_ELEC)))
		{
			add_flag(o_ptr->art_flags, TR_RES_ELEC);
			if (one_in_(2)) return;
		}
		if ((o_ptr->tval >= TV_CLOAK) && (o_ptr->tval <= TV_HARD_ARMOR) &&
		    !(have_flag(o_ptr->art_flags, TR_SH_ELEC)))
		{
			add_flag(o_ptr->art_flags, TR_SH_ELEC);
			if (one_in_(2)) return;
		}
		if (one_in_(BIAS_LUCK) && !(have_flag(o_ptr->art_flags, TR_IM_ELEC)))
		{
			add_flag(o_ptr->art_flags, TR_IM_ELEC);
			if (!one_in_(IM_LUCK))
			{
				remove_flag(o_ptr->art_flags, TR_IM_ACID);
				remove_flag(o_ptr->art_flags, TR_IM_COLD);
				remove_flag(o_ptr->art_flags, TR_IM_FIRE);
			}

			if (one_in_(2)) return;
		}
		break;

	case BIAS_FIRE:
		if (!(have_flag(o_ptr->art_flags, TR_RES_FIRE)))
		{
			add_flag(o_ptr->art_flags, TR_RES_FIRE);
			if (one_in_(2)) return;
		}
		if ((o_ptr->tval >= TV_CLOAK) &&
		    (o_ptr->tval <= TV_HARD_ARMOR) &&
		    !(have_flag(o_ptr->art_flags, TR_SH_FIRE)))
		{
			add_flag(o_ptr->art_flags, TR_SH_FIRE);
			if (one_in_(2)) return;
		}
		if (one_in_(BIAS_LUCK) &&
		    !(have_flag(o_ptr->art_flags, TR_IM_FIRE)))
		{
			add_flag(o_ptr->art_flags, TR_IM_FIRE);
			if (!one_in_(IM_LUCK))
			{
				remove_flag(o_ptr->art_flags, TR_IM_ELEC);
				remove_flag(o_ptr->art_flags, TR_IM_COLD);
				remove_flag(o_ptr->art_flags, TR_IM_ACID);
			}
			if (one_in_(2)) return;
		}
		break;

	case BIAS_COLD:
		if (!(have_flag(o_ptr->art_flags, TR_RES_COLD)))
		{
			add_flag(o_ptr->art_flags, TR_RES_COLD);
			if (one_in_(2)) return;
		}
		if ((o_ptr->tval >= TV_CLOAK) &&
		    (o_ptr->tval <= TV_HARD_ARMOR) &&
		    !(have_flag(o_ptr->art_flags, TR_SH_COLD)))
		{
			add_flag(o_ptr->art_flags, TR_SH_COLD);
			if (one_in_(2)) return;
		}
		if (one_in_(BIAS_LUCK) && !(have_flag(o_ptr->art_flags, TR_IM_COLD)))
		{
			add_flag(o_ptr->art_flags, TR_IM_COLD);
			if (!one_in_(IM_LUCK))
			{
				remove_flag(o_ptr->art_flags, TR_IM_ELEC);
				remove_flag(o_ptr->art_flags, TR_IM_ACID);
				remove_flag(o_ptr->art_flags, TR_IM_FIRE);
			}
			if (one_in_(2)) return;
		}
		break;

	case BIAS_POIS:
		if (!(have_flag(o_ptr->art_flags, TR_RES_POIS)))
		{
			add_flag(o_ptr->art_flags, TR_RES_POIS);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_WARRIOR:
		if (!one_in_(3) && (!(have_flag(o_ptr->art_flags, TR_RES_FEAR))))
		{
			add_flag(o_ptr->art_flags, TR_RES_FEAR);
			if (one_in_(2)) return;
		}
		if (one_in_(3) && (!(have_flag(o_ptr->art_flags, TR_NO_MAGIC))))
		{
			add_flag(o_ptr->art_flags, TR_NO_MAGIC);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_NECROMANTIC:
		if (!(have_flag(o_ptr->art_flags, TR_RES_NETHER)))
		{
			add_flag(o_ptr->art_flags, TR_RES_NETHER);
			if (one_in_(2)) return;
		}
		if (!(have_flag(o_ptr->art_flags, TR_RES_POIS)))
		{
			add_flag(o_ptr->art_flags, TR_RES_POIS);
			if (one_in_(2)) return;
		}
		if (!(have_flag(o_ptr->art_flags, TR_RES_DARK)))
		{
			add_flag(o_ptr->art_flags, TR_RES_DARK);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_CHAOS:
		if (!(have_flag(o_ptr->art_flags, TR_RES_CHAOS)))
		{
			add_flag(o_ptr->art_flags, TR_RES_CHAOS);
			if (one_in_(2)) return;
		}
		if (!(have_flag(o_ptr->art_flags, TR_RES_CONF)))
		{
			add_flag(o_ptr->art_flags, TR_RES_CONF);
			if (one_in_(2)) return;
		}
		if (!(have_flag(o_ptr->art_flags, TR_RES_DISEN)))
		{
			add_flag(o_ptr->art_flags, TR_RES_DISEN);
			if (one_in_(2)) return;
		}
		break;
	}

	switch (randint1(42))
	{
		case 1:
			if (!one_in_(WEIRD_LUCK))
				random_resistance(o_ptr);
			else
			{
				add_flag(o_ptr->art_flags, TR_IM_ACID);
				if (!o_ptr->artifact_bias)
					o_ptr->artifact_bias = BIAS_ACID;
			}
			break;
		case 2:
			if (!one_in_(WEIRD_LUCK))
				random_resistance(o_ptr);
			else
			{
				add_flag(o_ptr->art_flags, TR_IM_ELEC);
				if (!o_ptr->artifact_bias)
					o_ptr->artifact_bias = BIAS_ELEC;
			}
			break;
		case 3:
			if (!one_in_(WEIRD_LUCK))
				random_resistance(o_ptr);
			else
			{
				add_flag(o_ptr->art_flags, TR_IM_COLD);
				if (!o_ptr->artifact_bias)
					o_ptr->artifact_bias = BIAS_COLD;
			}
			break;
		case 4:
			if (!one_in_(WEIRD_LUCK))
				random_resistance(o_ptr);
			else
			{
				add_flag(o_ptr->art_flags, TR_IM_FIRE);
				if (!o_ptr->artifact_bias)
					o_ptr->artifact_bias = BIAS_FIRE;
			}
			break;
		case 5:
		case 6:
		case 13:
			add_flag(o_ptr->art_flags, TR_RES_ACID);
			if (!o_ptr->artifact_bias)
				o_ptr->artifact_bias = BIAS_ACID;
			break;
		case 7:
		case 8:
		case 14:
			add_flag(o_ptr->art_flags, TR_RES_ELEC);
			if (!o_ptr->artifact_bias)
				o_ptr->artifact_bias = BIAS_ELEC;
			break;
		case 9:
		case 10:
		case 15:
			add_flag(o_ptr->art_flags, TR_RES_FIRE);
			if (!o_ptr->artifact_bias)
				o_ptr->artifact_bias = BIAS_FIRE;
			break;
		case 11:
		case 12:
		case 16:
			add_flag(o_ptr->art_flags, TR_RES_COLD);
			if (!o_ptr->artifact_bias)
				o_ptr->artifact_bias = BIAS_COLD;
			break;
		case 17:
		case 18:
			add_flag(o_ptr->art_flags, TR_RES_POIS);
			if (!o_ptr->artifact_bias && !one_in_(4))
				o_ptr->artifact_bias = BIAS_POIS;
			else if (!o_ptr->artifact_bias && one_in_(2))
				o_ptr->artifact_bias = BIAS_NECROMANTIC;
			else if (!o_ptr->artifact_bias && one_in_(2))
				o_ptr->artifact_bias = BIAS_ROGUE;
			break;
		case 19:
		case 20:
			add_flag(o_ptr->art_flags, TR_RES_FEAR);
			if (!o_ptr->artifact_bias && one_in_(3))
				o_ptr->artifact_bias = BIAS_WARRIOR;
			break;
		case 21:
			add_flag(o_ptr->art_flags, TR_RES_LITE);
			break;
		case 22:
			add_flag(o_ptr->art_flags, TR_RES_DARK);
			break;
		case 23:
		case 24:
			add_flag(o_ptr->art_flags, TR_RES_BLIND);
			break;
		case 25:
		case 26:
			add_flag(o_ptr->art_flags, TR_RES_CONF);
			if (!o_ptr->artifact_bias && one_in_(6))
				o_ptr->artifact_bias = BIAS_CHAOS;
			break;
		case 27:
		case 28:
			add_flag(o_ptr->art_flags, TR_RES_SOUND);
			break;
		case 29:
		case 30:
			add_flag(o_ptr->art_flags, TR_RES_SHARDS);
			break;
		case 31:
		case 32:
			add_flag(o_ptr->art_flags, TR_RES_NETHER);
			if (!o_ptr->artifact_bias && one_in_(3))
				o_ptr->artifact_bias = BIAS_NECROMANTIC;
			break;
		case 33:
		case 34:
			add_flag(o_ptr->art_flags, TR_RES_NEXUS);
			break;
		case 35:
		case 36:
			add_flag(o_ptr->art_flags, TR_RES_CHAOS);
			if (!o_ptr->artifact_bias && one_in_(2))
				o_ptr->artifact_bias = BIAS_CHAOS;
			break;
		case 37:
		case 38:
			add_flag(o_ptr->art_flags, TR_RES_DISEN);
			break;
		case 39:
			if (o_ptr->tval >= TV_CLOAK && o_ptr->tval <= TV_HARD_ARMOR)
				add_flag(o_ptr->art_flags, TR_SH_ELEC);
			else
				random_resistance(o_ptr);
			if (!o_ptr->artifact_bias)
				o_ptr->artifact_bias = BIAS_ELEC;
			break;
		case 40:
			if (o_ptr->tval >= TV_CLOAK && o_ptr->tval <= TV_HARD_ARMOR)
				add_flag(o_ptr->art_flags, TR_SH_FIRE);
			else
				random_resistance(o_ptr);
			if (!o_ptr->artifact_bias)
				o_ptr->artifact_bias = BIAS_FIRE;
			break;
		case 41:
			if (o_ptr->tval == TV_SHIELD || o_ptr->tval == TV_CLOAK ||
			    o_ptr->tval == TV_HELM || o_ptr->tval == TV_HARD_ARMOR)
				add_flag(o_ptr->art_flags, TR_REFLECT);
			else
				random_resistance(o_ptr);
			break;
		case 42:
			if (o_ptr->tval >= TV_CLOAK && o_ptr->tval <= TV_HARD_ARMOR)
				add_flag(o_ptr->art_flags, TR_SH_COLD);
			else
				random_resistance(o_ptr);
			if (!o_ptr->artifact_bias)
				o_ptr->artifact_bias = BIAS_COLD;
			break;
	}
}


/*!
 * @brief ランダムアーティファクト生成中、対象のオブジェクトにその他特性を付加する。/ Add one misc flag on generation of randam artifact.
 * @details 優先的に付加される耐性がランダムアーティファクトバイアスに依存して存在する。
 * 原則的候補は各種能力維持、永久光源+1、麻痺知らず、経験値維持、浮遊、透明視、急回復、遅消化、
 * 乱テレポート、反魔法、反テレポート、警告、テレパシー、各種ESP、一部装備に殺戮修正。
 * @attention オブジェクトのtval、svalに依存したハードコーディング処理がある。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return なし
 */
static void random_misc(object_type * o_ptr)
{
	switch (o_ptr->artifact_bias)
	{
	case BIAS_RANGER:
		if (!(have_flag(o_ptr->art_flags, TR_SUST_CON)))
		{
			add_flag(o_ptr->art_flags, TR_SUST_CON);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_STR:
		if (!(have_flag(o_ptr->art_flags, TR_SUST_STR)))
		{
			add_flag(o_ptr->art_flags, TR_SUST_STR);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_WIS:
		if (!(have_flag(o_ptr->art_flags, TR_SUST_WIS)))
		{
			add_flag(o_ptr->art_flags, TR_SUST_WIS);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_INT:
		if (!(have_flag(o_ptr->art_flags, TR_SUST_INT)))
		{
			add_flag(o_ptr->art_flags, TR_SUST_INT);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_DEX:
		if (!(have_flag(o_ptr->art_flags, TR_SUST_DEX)))
		{
			add_flag(o_ptr->art_flags, TR_SUST_DEX);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_CON:
		if (!(have_flag(o_ptr->art_flags, TR_SUST_CON)))
		{
			add_flag(o_ptr->art_flags, TR_SUST_CON);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_CHR:
		if (!(have_flag(o_ptr->art_flags, TR_SUST_CHR)))
		{
			add_flag(o_ptr->art_flags, TR_SUST_CHR);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_CHAOS:
		if (!(have_flag(o_ptr->art_flags, TR_TELEPORT)))
		{
			add_flag(o_ptr->art_flags, TR_TELEPORT);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_FIRE:
		if (!(have_flag(o_ptr->art_flags, TR_LITE_1)))
		{
			add_flag(o_ptr->art_flags, TR_LITE_1); /* Freebie */
		}
		break;
	}

	switch (randint1(33))
	{
		case 1:
			add_flag(o_ptr->art_flags, TR_SUST_STR);
			if (!o_ptr->artifact_bias)
				o_ptr->artifact_bias = BIAS_STR;
			break;
		case 2:
			add_flag(o_ptr->art_flags, TR_SUST_INT);
			if (!o_ptr->artifact_bias)
				o_ptr->artifact_bias = BIAS_INT;
			break;
		case 3:
			add_flag(o_ptr->art_flags, TR_SUST_WIS);
			if (!o_ptr->artifact_bias)
				o_ptr->artifact_bias = BIAS_WIS;
			break;
		case 4:
			add_flag(o_ptr->art_flags, TR_SUST_DEX);
			if (!o_ptr->artifact_bias)
				o_ptr->artifact_bias = BIAS_DEX;
			break;
		case 5:
			add_flag(o_ptr->art_flags, TR_SUST_CON);
			if (!o_ptr->artifact_bias)
				o_ptr->artifact_bias = BIAS_CON;
			break;
		case 6:
			add_flag(o_ptr->art_flags, TR_SUST_CHR);
			if (!o_ptr->artifact_bias)
				o_ptr->artifact_bias = BIAS_CHR;
			break;
		case 7:
		case 8:
		case 14:
			add_flag(o_ptr->art_flags, TR_FREE_ACT);
			break;
		case 9:
			add_flag(o_ptr->art_flags, TR_HOLD_EXP);
			if (!o_ptr->artifact_bias && one_in_(5))
				o_ptr->artifact_bias = BIAS_PRIESTLY;
			else if (!o_ptr->artifact_bias && one_in_(6))
				o_ptr->artifact_bias = BIAS_NECROMANTIC;
			break;
		case 10:
		case 11:
			add_flag(o_ptr->art_flags, TR_LITE_1);
			break;
		case 12:
		case 13:
			add_flag(o_ptr->art_flags, TR_LEVITATION);
			break;
		case 15:
		case 16:
		case 17:
			add_flag(o_ptr->art_flags, TR_SEE_INVIS);
			break;
		case 19:
		case 20:
			add_flag(o_ptr->art_flags, TR_SLOW_DIGEST);
			break;
		case 21:
		case 22:
			add_flag(o_ptr->art_flags, TR_REGEN);
			break;
		case 23:
			add_flag(o_ptr->art_flags, TR_TELEPORT);
			break;
		case 24:
		case 25:
		case 26:
			if (object_is_armour(o_ptr))
				random_misc(o_ptr);
			else
			{
				o_ptr->to_a = 4 + randint1(11);
			}
			break;
		case 27:
		case 28:
		case 29:
		{
			HIT_PROB bonus_h;
			HIT_POINT bonus_d;
			add_flag(o_ptr->art_flags, TR_SHOW_MODS);
			bonus_h = 4 + (HIT_PROB)(randint1(11));
			bonus_d = 4 + (HIT_POINT)(randint1(11));
			if ((o_ptr->tval != TV_SWORD) && (o_ptr->tval != TV_POLEARM) && (o_ptr->tval != TV_HAFTED) && (o_ptr->tval != TV_DIGGING) && (o_ptr->tval != TV_GLOVES) && (o_ptr->tval != TV_RING))
			{
				bonus_h /= 2;
				bonus_d /= 2;
			}
			o_ptr->to_h += bonus_h;
			o_ptr->to_d += bonus_d;
			break;
		}
		case 30:
			add_flag(o_ptr->art_flags, TR_NO_MAGIC);
			break;
		case 31:
			add_flag(o_ptr->art_flags, TR_NO_TELE);
			break;
		case 32:
			add_flag(o_ptr->art_flags, TR_WARNING);
			break;

		case 18:
			switch (randint1(3))
			{
			case 1:
				add_flag(o_ptr->art_flags, TR_ESP_EVIL);
				if (!o_ptr->artifact_bias && one_in_(3))
					o_ptr->artifact_bias = BIAS_LAW;
				break;
			case 2:
				add_flag(o_ptr->art_flags, TR_ESP_NONLIVING);
				if (!o_ptr->artifact_bias && one_in_(3))
					o_ptr->artifact_bias = BIAS_MAGE;
				break;
			case 3:
				add_flag(o_ptr->art_flags, TR_TELEPATHY);
				if (!o_ptr->artifact_bias && one_in_(9))
					o_ptr->artifact_bias = BIAS_MAGE;
				break;
			}
			break;

		case 33:
		{
			int idx[3];
			int n = randint1(3);

			idx[0] = randint1(10);

			idx[1] = randint1(9);
			if (idx[1] >= idx[0]) idx[1]++;

			idx[2] = randint1(8);
			if (idx[2] >= idx[0]) idx[2]++;
			if (idx[2] >= idx[1]) idx[2]++;

			while (n--) switch (idx[n])
			{
			case 1:
				add_flag(o_ptr->art_flags, TR_ESP_ANIMAL);
				if (!o_ptr->artifact_bias && one_in_(4))
					o_ptr->artifact_bias = BIAS_RANGER;
				break;
			case 2:
				add_flag(o_ptr->art_flags, TR_ESP_UNDEAD);
				if (!o_ptr->artifact_bias && one_in_(3))
					o_ptr->artifact_bias = BIAS_PRIESTLY;
				else if (!o_ptr->artifact_bias && one_in_(6))
					o_ptr->artifact_bias = BIAS_NECROMANTIC;
				break;
			case 3:
				add_flag(o_ptr->art_flags, TR_ESP_DEMON);
				break;
			case 4:
				add_flag(o_ptr->art_flags, TR_ESP_ORC);
				break;
			case 5:
				add_flag(o_ptr->art_flags, TR_ESP_TROLL);
				break;
			case 6:
				add_flag(o_ptr->art_flags, TR_ESP_GIANT);
				break;
			case 7:
				add_flag(o_ptr->art_flags, TR_ESP_DRAGON);
				break;
			case 8:
				add_flag(o_ptr->art_flags, TR_ESP_HUMAN);
				if (!o_ptr->artifact_bias && one_in_(6))
					o_ptr->artifact_bias = BIAS_ROGUE;
				break;
			case 9:
				add_flag(o_ptr->art_flags, TR_ESP_GOOD);
				if (!o_ptr->artifact_bias && one_in_(3))
					o_ptr->artifact_bias = BIAS_LAW;
				break;
			case 10:
				add_flag(o_ptr->art_flags, TR_ESP_UNIQUE);
				if (!o_ptr->artifact_bias && one_in_(3))
					o_ptr->artifact_bias = BIAS_LAW;
				break;
			}
			break;
		}
	}
}

/*!
 * @brief ランダムアーティファクト生成中、対象のオブジェクトにスレイ効果を付加する。/ Add one slaying on generation of randam artifact.
 * @details 優先的に付加される耐性がランダムアーティファクトバイアスに依存して存在する。
 * 原則的候補は強力射、高速射、混沌効果、吸血効果、祝福、投擲しやすい、焼棄、凍結、電撃、溶解、毒殺、
 * 動物スレイ、邪悪スレイ、悪魔スレイ、不死スレイ、オークスレイ、トロルスレイ、巨人スレイ、ドラゴンスレイ、
 * *ドラゴンスレイ*、人間スレイ、切れ味、地震、理力。
 * @attention オブジェクトのtval、svalに依存したハードコーディング処理がある。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return なし
 */
static void random_slay(object_type *o_ptr)
{
	if (o_ptr->tval == TV_BOW)
	{
		switch (randint1(6))
		{
			case 1:
			case 2:
			case 3:
				add_flag(o_ptr->art_flags, TR_XTRA_MIGHT);
				if (!one_in_(7)) remove_flag(o_ptr->art_flags, TR_XTRA_SHOTS);
				if (!o_ptr->artifact_bias && one_in_(9))
					o_ptr->artifact_bias = BIAS_RANGER;
				break;
			default:
				add_flag(o_ptr->art_flags, TR_XTRA_SHOTS);
				if (!one_in_(7)) remove_flag(o_ptr->art_flags, TR_XTRA_MIGHT);
				if (!o_ptr->artifact_bias && one_in_(9))
					o_ptr->artifact_bias = BIAS_RANGER;
			break;
		}

		return;
	}

	switch (o_ptr->artifact_bias)
	{
	case BIAS_CHAOS:
		if (!(have_flag(o_ptr->art_flags, TR_CHAOTIC)))
		{
			add_flag(o_ptr->art_flags, TR_CHAOTIC);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_PRIESTLY:
		if((o_ptr->tval == TV_SWORD || o_ptr->tval == TV_POLEARM) &&
		   !(have_flag(o_ptr->art_flags, TR_BLESSED)))
		{
			/* A free power for "priestly" random artifacts */
			add_flag(o_ptr->art_flags, TR_BLESSED);
		}
		break;

	case BIAS_NECROMANTIC:
		if (!(have_flag(o_ptr->art_flags, TR_VAMPIRIC)))
		{
			add_flag(o_ptr->art_flags, TR_VAMPIRIC);
			if (one_in_(2)) return;
		}
		if (!(have_flag(o_ptr->art_flags, TR_BRAND_POIS)) && one_in_(2))
		{
			add_flag(o_ptr->art_flags, TR_BRAND_POIS);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_RANGER:
		if (!(have_flag(o_ptr->art_flags, TR_SLAY_ANIMAL)))
		{
			add_flag(o_ptr->art_flags, TR_SLAY_ANIMAL);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_ROGUE:
		if ((((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_DAGGER)) ||
		     ((o_ptr->tval == TV_POLEARM) && (o_ptr->sval == SV_SPEAR))) &&
			 !(have_flag(o_ptr->art_flags, TR_THROW)))
		{
			/* Free power for rogues... */
			add_flag(o_ptr->art_flags, TR_THROW);
		}
		if (!(have_flag(o_ptr->art_flags, TR_BRAND_POIS)))
		{
			add_flag(o_ptr->art_flags, TR_BRAND_POIS);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_POIS:
		if (!(have_flag(o_ptr->art_flags, TR_BRAND_POIS)))
		{
			add_flag(o_ptr->art_flags, TR_BRAND_POIS);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_FIRE:
		if (!(have_flag(o_ptr->art_flags, TR_BRAND_FIRE)))
		{
			add_flag(o_ptr->art_flags, TR_BRAND_FIRE);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_COLD:
		if (!(have_flag(o_ptr->art_flags, TR_BRAND_COLD)))
		{
			add_flag(o_ptr->art_flags, TR_BRAND_COLD);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_ELEC:
		if (!(have_flag(o_ptr->art_flags, TR_BRAND_ELEC)))
		{
			add_flag(o_ptr->art_flags, TR_BRAND_ELEC);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_ACID:
		if (!(have_flag(o_ptr->art_flags, TR_BRAND_ACID)))
		{
			add_flag(o_ptr->art_flags, TR_BRAND_ACID);
			if (one_in_(2)) return;
		}
		break;

	case BIAS_LAW:
		if (!(have_flag(o_ptr->art_flags, TR_SLAY_EVIL)))
		{
			add_flag(o_ptr->art_flags, TR_SLAY_EVIL);
			if (one_in_(2)) return;
		}
		if (!(have_flag(o_ptr->art_flags, TR_SLAY_UNDEAD)))
		{
			add_flag(o_ptr->art_flags, TR_SLAY_UNDEAD);
			if (one_in_(2)) return;
		}
		if (!(have_flag(o_ptr->art_flags, TR_SLAY_DEMON)))
		{
			add_flag(o_ptr->art_flags, TR_SLAY_DEMON);
			if (one_in_(2)) return;
		}
		break;
	}

	switch (randint1(36))
	{
		case 1:
		case 2:
			if (one_in_(4))
			{
				add_flag(o_ptr->art_flags, TR_KILL_ANIMAL);
			}
			else
			{
				add_flag(o_ptr->art_flags, TR_SLAY_ANIMAL);
			}
			break;
		case 3:
		case 4:
			if (one_in_(8))
			{
				add_flag(o_ptr->art_flags, TR_KILL_EVIL);
			}
			else
			{
				add_flag(o_ptr->art_flags, TR_SLAY_EVIL); 
			}
			if (!o_ptr->artifact_bias && one_in_(2))
				o_ptr->artifact_bias = BIAS_LAW;
			else if (!o_ptr->artifact_bias && one_in_(9))
				o_ptr->artifact_bias = BIAS_PRIESTLY;
			break;
		case 5:
		case 6:
			if (one_in_(4))
			{
				add_flag(o_ptr->art_flags, TR_KILL_UNDEAD);
			}
			else
			{
				add_flag(o_ptr->art_flags, TR_SLAY_UNDEAD);
			}
			if (!o_ptr->artifact_bias && one_in_(9))
				o_ptr->artifact_bias = BIAS_PRIESTLY;
			break;
		case 7:
		case 8:
			if (one_in_(4))
			{
				add_flag(o_ptr->art_flags, TR_KILL_DEMON);
			}
			else
			{
				add_flag(o_ptr->art_flags, TR_SLAY_DEMON);
			}
			if (!o_ptr->artifact_bias && one_in_(9))
				o_ptr->artifact_bias = BIAS_PRIESTLY;
			break;
		case 9:
		case 10:
			if (one_in_(4))
			{
				add_flag(o_ptr->art_flags, TR_KILL_ORC);
			}
			else
			{
				add_flag(o_ptr->art_flags, TR_SLAY_ORC);
			}
			break;
		case 11:
		case 12:
			if (one_in_(4))
			{
				add_flag(o_ptr->art_flags, TR_KILL_TROLL);
			}
			else
			{
				add_flag(o_ptr->art_flags, TR_SLAY_TROLL);
			}
			break;
		case 13:
		case 14:
			if (one_in_(4))
			{
				add_flag(o_ptr->art_flags, TR_KILL_GIANT);
			}
			else
			{
				add_flag(o_ptr->art_flags, TR_SLAY_GIANT);
			}
			break;
		case 15:
		case 16:
			add_flag(o_ptr->art_flags, TR_SLAY_DRAGON);
			break;
		case 17:
			add_flag(o_ptr->art_flags, TR_KILL_DRAGON);
			break;
		case 18:
		case 19:
			if (o_ptr->tval == TV_SWORD)
			{
				add_flag(o_ptr->art_flags, TR_VORPAL);
				if (!o_ptr->artifact_bias && one_in_(9))
					o_ptr->artifact_bias = BIAS_WARRIOR;
			}
			else
				random_slay(o_ptr);
			break;
		case 20:
			add_flag(o_ptr->art_flags, TR_IMPACT);
			break;
		case 21:
		case 22:
			add_flag(o_ptr->art_flags, TR_BRAND_FIRE);
			if (!o_ptr->artifact_bias)
				o_ptr->artifact_bias = BIAS_FIRE;
			break;
		case 23:
		case 24:
			add_flag(o_ptr->art_flags, TR_BRAND_COLD);
			if (!o_ptr->artifact_bias)
				o_ptr->artifact_bias = BIAS_COLD;
			break;
		case 25:
		case 26:
			add_flag(o_ptr->art_flags, TR_BRAND_ELEC);
			if (!o_ptr->artifact_bias)
				o_ptr->artifact_bias = BIAS_ELEC;
			break;
		case 27:
		case 28:
			add_flag(o_ptr->art_flags, TR_BRAND_ACID);
			if (!o_ptr->artifact_bias)
				o_ptr->artifact_bias = BIAS_ACID;
			break;
		case 29:
		case 30:
			add_flag(o_ptr->art_flags, TR_BRAND_POIS);
			if (!o_ptr->artifact_bias && !one_in_(3))
				o_ptr->artifact_bias = BIAS_POIS;
			else if (!o_ptr->artifact_bias && one_in_(6))
				o_ptr->artifact_bias = BIAS_NECROMANTIC;
			else if (!o_ptr->artifact_bias)
				o_ptr->artifact_bias = BIAS_ROGUE;
			break;
		case 31:
			add_flag(o_ptr->art_flags, TR_VAMPIRIC);
			if (!o_ptr->artifact_bias)
				o_ptr->artifact_bias = BIAS_NECROMANTIC;
			break;
		case 32:
			add_flag(o_ptr->art_flags, TR_FORCE_WEAPON);
			if (!o_ptr->artifact_bias)
				o_ptr->artifact_bias = (one_in_(2) ? BIAS_MAGE : BIAS_PRIESTLY);
			break;
		case 33:
		case 34:
			if (one_in_(4))
			{
				add_flag(o_ptr->art_flags, TR_KILL_HUMAN);
			}
			else
			{
				add_flag(o_ptr->art_flags, TR_SLAY_HUMAN);
			}
			break;
		default:
			add_flag(o_ptr->art_flags, TR_CHAOTIC);
			if (!o_ptr->artifact_bias)
				o_ptr->artifact_bias = BIAS_CHAOS;
			break;
	}
}

/*!
 * @brief ランダムアーティファクト生成中、対象のオブジェクトにバイアスに依存した発動を与える。/ Add one activaton of randam artifact depend on bias.
 * @details バイアスが無い場合、一部のバイアスの確率によっては one_ability() に処理が移行する。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return なし
 */
static void give_activation_power(object_type *o_ptr)
{
	int type = 0, chance = 0;

	switch (o_ptr->artifact_bias)
	{
		case BIAS_ELEC:
			if (!one_in_(3))
			{
				type = ACT_BO_ELEC_1;
			}
			else if (!one_in_(5))
			{
				type = ACT_BA_ELEC_2;
			}
			else
			{
				type = ACT_BA_ELEC_3;
			}
			chance = 101;
			break;

		case BIAS_POIS:
			type = ACT_BA_POIS_1;
			chance = 101;
			break;

		case BIAS_FIRE:
			if (!one_in_(3))
			{
				type = ACT_BO_FIRE_1;
			}
			else if (!one_in_(5))
			{
				type = ACT_BA_FIRE_1;
			}
			else
			{
				type = ACT_BA_FIRE_2;
			}
			chance = 101;
			break;

		case BIAS_COLD:
			chance = 101;
			if (!one_in_(3))
				type = ACT_BO_COLD_1;
			else if (!one_in_(3))
				type = ACT_BA_COLD_1;
			else if (!one_in_(3))
				type = ACT_BA_COLD_2;
			else
				type = ACT_BA_COLD_3;
			break;

		case BIAS_CHAOS:
			chance = 50;
			if (one_in_(6))
				type = ACT_SUMMON_DEMON;
			else
				type = ACT_CALL_CHAOS;
			break;

		case BIAS_PRIESTLY:
			chance = 101;

			if (one_in_(13))
				type = ACT_CHARM_UNDEAD;
			else if (one_in_(12))
				type = ACT_BANISH_EVIL;
			else if (one_in_(11))
				type = ACT_DISP_EVIL;
			else if (one_in_(10))
				type = ACT_PROT_EVIL;
			else if (one_in_(9))
				type = ACT_CURE_1000;
			else if (one_in_(8))
				type = ACT_CURE_700;
			else if (one_in_(7))
				type = ACT_REST_ALL;
			else if (one_in_(6))
				type = ACT_REST_EXP;
			else
				type = ACT_CURE_MW;
			break;

		case BIAS_NECROMANTIC:
			chance = 101;
			if (one_in_(66))
				type = ACT_WRAITH;
			else if (one_in_(13))
				type = ACT_DISP_GOOD;
			else if (one_in_(9))
				type = ACT_MASS_GENO;
			else if (one_in_(8))
				type = ACT_GENOCIDE;
			else if (one_in_(13))
				type = ACT_SUMMON_UNDEAD;
			else if (one_in_(9))
				type = ACT_DRAIN_2;
			else if (one_in_(6))
				type = ACT_CHARM_UNDEAD;
			else
				type = ACT_DRAIN_1;
			break;

		case BIAS_LAW:
			chance = 101;
			if (one_in_(8))
				type = ACT_BANISH_EVIL;
			else if (one_in_(4))
				type = ACT_DISP_EVIL;
			else
				type = ACT_PROT_EVIL;
			break;

		case BIAS_ROGUE:
			chance = 101;
			if (one_in_(50))
				type = ACT_SPEED;
			else if (one_in_(4))
				type = ACT_SLEEP;
			else if (one_in_(3))
				type = ACT_DETECT_ALL;
			else if (one_in_(8))
				type = ACT_ID_FULL;
			else
				type = ACT_ID_PLAIN;
			break;

		case BIAS_MAGE:
			chance = 66;
			if (one_in_(20))
				type = ACT_SUMMON_ELEMENTAL;
			else if (one_in_(10))
				type = ACT_SUMMON_PHANTOM;
			else if (one_in_(5))
				type = ACT_RUNE_EXPLO;
			else
				type = ACT_ESP;
			break;

		case BIAS_WARRIOR:
			chance = 80;
			if (one_in_(100))
				type = ACT_INVULN;
			else
				type = ACT_BERSERK;
			break;

		case BIAS_RANGER:
			chance = 101;
			if (one_in_(20))
				type = ACT_CHARM_ANIMALS;
			else if (one_in_(7))
				type = ACT_SUMMON_ANIMAL;
			else if (one_in_(6))
				type = ACT_CHARM_ANIMAL;
			else if (one_in_(4))
				type = ACT_RESIST_ALL;
			else if (one_in_(3))
				type = ACT_SATIATE;
			else
				type = ACT_CURE_POISON;
			break;
	}

	if (!type || (randint1(100) >= chance))
	{
		one_activation(o_ptr);
		return;
	}

	/* A type was chosen... */
	o_ptr->xtra2 = (byte_hack)type;
	add_flag(o_ptr->art_flags, TR_ACTIVATE);
	o_ptr->timeout = 0;
}

/*!
 * @brief ランダムアーティファクト生成中、対象のオブジェクトに名前を与える。/ Set name of randomartifact.
 * @details 確率によって、シンダリン銘、漢字銘、固定名のいずれか一つが与えられる。
 * @param o_ptr 処理中のアイテム参照ポインタ
 * @param return_name 名前を返すための文字列参照ポインタ
 * @param armour 対象のオブジェクトが防具が否か
 * @param power 銘の基準となるオブジェクトの価値レベル(0=呪い、1=低位、2=中位、3以上=高位)
 * @return なし
 */
static void get_random_name(object_type *o_ptr, char *return_name, bool armour, int power)
{
	PERCENTAGE prob = randint1(100);

	if (prob <= SINDARIN_NAME)
	{
		get_table_sindarin(return_name);
	}
	else if (prob <= TABLE_NAME)
	{
		get_table_name(return_name);
	}
	else
	{
		cptr filename;

		switch (armour)
		{
			case 1:
				switch (power)
				{
					case 0:
						filename = _("a_cursed_j.txt", "a_cursed.txt");
						break;
					case 1:
						filename = _("a_low_j.txt", "a_low.txt");
						break;
					case 2:
						filename = _("a_med_j.txt", "a_med.txt");
						break;
					default:
						filename = _("a_high_j.txt", "a_high.txt");
				}
				break;
			default:
				switch (power)
				{
					case 0:
						filename = _("w_cursed_j.txt", "w_cursed.txt");
						break;
					case 1:
						filename = _("w_low_j.txt", "w_low.txt");
						break;
					case 2:
						filename = _("w_med_j.txt", "w_med.txt");
						break;
					default:
						filename = _("w_high_j.txt", "w_high.txt");
				}
		}

		(void)get_rnd_line(filename, o_ptr->artifact_bias, return_name);
#ifdef JP
		 if (return_name[0] == 0) get_table_name(return_name);
#endif
	}
}

/*!
 * @brief ランダムアーティファクト生成のメインルーチン
 * @details 既に生成が済んでいるオブジェクトの構造体を、アーティファクトとして強化する。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @param a_scroll アーティファクト生成の巻物上の処理。呪いのアーティファクトが生成対象外となる。
 * @return 常にTRUE(1)を返す
 */
bool create_artifact(object_type *o_ptr, bool a_scroll)
{
	GAME_TEXT new_name[1024];
	PARAMETER_VALUE has_pval = 0;
	int     powers = randint1(5) + 1;
	int     max_powers;
	int     max_type = (object_is_weapon_ammo(o_ptr) ? 7 : 5);
	int     power_level;
	s32b    total_flags;
	bool    a_cursed = FALSE;
	int     warrior_artifact_bias = 0;
	int i;

	/* Reset artifact bias */
	o_ptr->artifact_bias = 0;

	/* Nuke enchantments */
	o_ptr->name1 = 0;
	o_ptr->name2 = 0;

	for (i = 0; i < TR_FLAG_SIZE; i++)
		o_ptr->art_flags[i] |= k_info[o_ptr->k_idx].flags[i];

	if (o_ptr->pval) has_pval = TRUE;

	if (a_scroll && one_in_(4))
	{
		switch (p_ptr->pclass)
		{
			case CLASS_WARRIOR:
			case CLASS_BERSERKER:
			case CLASS_ARCHER:
			case CLASS_SAMURAI:
			case CLASS_CAVALRY:
			case CLASS_SMITH:
				o_ptr->artifact_bias = BIAS_WARRIOR;
				break;
			case CLASS_MAGE:
			case CLASS_HIGH_MAGE:
			case CLASS_SORCERER:
			case CLASS_MAGIC_EATER:
			case CLASS_BLUE_MAGE:
				o_ptr->artifact_bias = BIAS_MAGE;
				break;
			case CLASS_PRIEST:
				o_ptr->artifact_bias = BIAS_PRIESTLY;
				break;
			case CLASS_ROGUE:
			case CLASS_NINJA:
				o_ptr->artifact_bias = BIAS_ROGUE;
				warrior_artifact_bias = 25;
				break;
			case CLASS_RANGER:
			case CLASS_SNIPER:
				o_ptr->artifact_bias = BIAS_RANGER;
				warrior_artifact_bias = 30;
				break;
			case CLASS_PALADIN:
				o_ptr->artifact_bias = BIAS_PRIESTLY;
				warrior_artifact_bias = 40;
				break;
			case CLASS_WARRIOR_MAGE:
			case CLASS_RED_MAGE:
				o_ptr->artifact_bias = BIAS_MAGE;
				warrior_artifact_bias = 40;
				break;
			case CLASS_CHAOS_WARRIOR:
				o_ptr->artifact_bias = BIAS_CHAOS;
				warrior_artifact_bias = 40;
				break;
			case CLASS_MONK:
			case CLASS_FORCETRAINER:
				o_ptr->artifact_bias = BIAS_PRIESTLY;
				break;
			case CLASS_MINDCRAFTER:
			case CLASS_BARD:
				if (randint1(5) > 2) o_ptr->artifact_bias = BIAS_PRIESTLY;
				break;
			case CLASS_TOURIST:
				if (randint1(5) > 2) o_ptr->artifact_bias = BIAS_WARRIOR;
				break;
			case CLASS_IMITATOR:
				if (randint1(2) > 1) o_ptr->artifact_bias = BIAS_RANGER;
				break;
			case CLASS_BEASTMASTER:
				o_ptr->artifact_bias = BIAS_CHR;
				warrior_artifact_bias = 50;
				break;
			case CLASS_MIRROR_MASTER:
				if (randint1(4) > 1) 
				{
				    o_ptr->artifact_bias = BIAS_MAGE;
				}
				else
				{
				    o_ptr->artifact_bias = BIAS_ROGUE;
				}
				break;
		}
	}

	if (a_scroll && (randint1(100) <= warrior_artifact_bias))
		o_ptr->artifact_bias = BIAS_WARRIOR;

	strcpy(new_name, "");

	if (!a_scroll && one_in_(A_CURSED))
		a_cursed = TRUE;
	if (((o_ptr->tval == TV_AMULET) || (o_ptr->tval == TV_RING)) && object_is_cursed(o_ptr))
		a_cursed = TRUE;

	while (one_in_(powers) || one_in_(7) || one_in_(10))
		powers++;

	if (!a_cursed && one_in_(WEIRD_LUCK))
		powers *= 2;

	if (a_cursed) powers /= 2;

	max_powers = powers;
	/* Main loop */
	while (powers--)
	{
		switch (randint1(max_type))
		{
			case 1: case 2:
				random_plus(o_ptr);
				has_pval = TRUE;
				break;
			case 3: case 4:
				if (one_in_(2) && object_is_weapon_ammo(o_ptr) && (o_ptr->tval != TV_BOW))
				{
					if (a_cursed && !one_in_(13)) break;
					if (one_in_(13))
					{
						if (one_in_(o_ptr->ds+4)) o_ptr->ds++;
					}
					else
					{
						if (one_in_(o_ptr->dd+1)) o_ptr->dd++;
					}
				}
				else
					random_resistance(o_ptr);
				break;
			case 5:
				random_misc(o_ptr);
				break;
			case 6: case 7:
				random_slay(o_ptr);
				break;
			default:
				if (p_ptr->wizard) msg_print("Switch error in create_artifact!");
				powers++;
		}
	};

	if (has_pval)
	{
		if (have_flag(o_ptr->art_flags, TR_BLOWS))
		{
			o_ptr->pval = randint1(2);
			if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_HAYABUSA))
				o_ptr->pval++;
		}
		else
		{
			do
			{
				o_ptr->pval++;
			}
			while (o_ptr->pval < randint1(5) || one_in_(o_ptr->pval));
		}

		if ((o_ptr->pval > 4) && !one_in_(WEIRD_LUCK))
			o_ptr->pval = 4;
	}


	/* give it some plusses... */
	if (object_is_armour(o_ptr))
		o_ptr->to_a += randint1(o_ptr->to_a > 19 ? 1 : 20 - o_ptr->to_a);
	else if (object_is_weapon_ammo(o_ptr))
	{
		o_ptr->to_h += randint1(o_ptr->to_h > 19 ? 1 : 20 - o_ptr->to_h);
		o_ptr->to_d += randint1(o_ptr->to_d > 19 ? 1 : 20 - o_ptr->to_d);
		if ((have_flag(o_ptr->art_flags, TR_WIS)) && (o_ptr->pval > 0)) add_flag(o_ptr->art_flags, TR_BLESSED);
	}

	/* Just to be sure */
	add_flag(o_ptr->art_flags, TR_IGNORE_ACID);
	add_flag(o_ptr->art_flags, TR_IGNORE_ELEC);
	add_flag(o_ptr->art_flags, TR_IGNORE_FIRE);
	add_flag(o_ptr->art_flags, TR_IGNORE_COLD);

	total_flags = flag_cost(o_ptr, o_ptr->pval);

	if (a_cursed) curse_artifact(o_ptr);

	if (!a_cursed &&
	    one_in_(object_is_armour(o_ptr) ? ACTIVATION_CHANCE * 2 : ACTIVATION_CHANCE))
	{
		o_ptr->xtra2 = 0;
		give_activation_power(o_ptr);
	}

	if (object_is_armour(o_ptr))
	{
		while ((o_ptr->to_d+o_ptr->to_h) > 20)
		{
			if (one_in_(o_ptr->to_d) && one_in_(o_ptr->to_h)) break;
			o_ptr->to_d -= (HIT_POINT)randint0(3);
			o_ptr->to_h -= (HIT_PROB)randint0(3);
		}
		while ((o_ptr->to_d+o_ptr->to_h) > 10)
		{
			if (one_in_(o_ptr->to_d) || one_in_(o_ptr->to_h)) break;
			o_ptr->to_d -= (HIT_POINT)randint0(3);
			o_ptr->to_h -= (HIT_PROB)randint0(3);
		}
	}

	if (((o_ptr->artifact_bias == BIAS_MAGE) || (o_ptr->artifact_bias == BIAS_INT)) && (o_ptr->tval == TV_GLOVES)) add_flag(o_ptr->art_flags, TR_FREE_ACT);

	if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_DOKUBARI))
	{
		o_ptr->to_h = 0;
		o_ptr->to_d = 0;
		remove_flag(o_ptr->art_flags, TR_BLOWS);
		remove_flag(o_ptr->art_flags, TR_FORCE_WEAPON);
		remove_flag(o_ptr->art_flags, TR_SLAY_ANIMAL);
		remove_flag(o_ptr->art_flags, TR_SLAY_EVIL);
		remove_flag(o_ptr->art_flags, TR_SLAY_UNDEAD);
		remove_flag(o_ptr->art_flags, TR_SLAY_DEMON);
		remove_flag(o_ptr->art_flags, TR_SLAY_ORC);
		remove_flag(o_ptr->art_flags, TR_SLAY_TROLL);
		remove_flag(o_ptr->art_flags, TR_SLAY_GIANT);
		remove_flag(o_ptr->art_flags, TR_SLAY_DRAGON);
		remove_flag(o_ptr->art_flags, TR_KILL_DRAGON);
		remove_flag(o_ptr->art_flags, TR_SLAY_HUMAN);
		remove_flag(o_ptr->art_flags, TR_VORPAL);
		remove_flag(o_ptr->art_flags, TR_BRAND_POIS);
		remove_flag(o_ptr->art_flags, TR_BRAND_ACID);
		remove_flag(o_ptr->art_flags, TR_BRAND_ELEC);
		remove_flag(o_ptr->art_flags, TR_BRAND_FIRE);
		remove_flag(o_ptr->art_flags, TR_BRAND_COLD);
	}

	if (!object_is_weapon_ammo(o_ptr))
	{
		/* For armors */
		if (a_cursed) power_level = 0;
		else if (total_flags < 15000) power_level = 1;
		else if (total_flags < 35000) power_level = 2;
		else power_level = 3;
	}

	else
	{
		/* For weapons */
		if (a_cursed) power_level = 0;
		else if (total_flags < 20000) power_level = 1;
		else if (total_flags < 45000) power_level = 2;
		else power_level = 3;
	}

	/* ダメージ抑制処理を行う */
	while (suppression_evil_dam(o_ptr) && !one_in_(SWORDFISH_LUCK));
	{
		weakening_artifact(o_ptr);
	}

	if (a_scroll)
	{
		GAME_TEXT dummy_name[MAX_NLEN] = "";
		cptr ask_msg = _("このアーティファクトを何と名付けますか？", "What do you want to call the artifact? ");

		/* Identify it fully */
		object_aware(o_ptr);
		object_known(o_ptr);

		/* Mark the item as fully known */
		o_ptr->ident |= (IDENT_MENTAL);

		/* For being treated as random artifact in screen_object() */
		o_ptr->art_name = quark_add("");

		(void)screen_object(o_ptr, 0L);

		if (!get_string(ask_msg, dummy_name, sizeof dummy_name)
		    || !dummy_name[0])
		{
			/* Cancelled */
			if (one_in_(2))
			{
				get_table_sindarin_aux(dummy_name);
			}
			else
			{
				get_table_name_aux(dummy_name);
			}
		}
		sprintf(new_name, _("《%s》", "'%s'"), dummy_name);
		chg_virtue(V_INDIVIDUALISM, 2);
		chg_virtue(V_ENCHANT, 5);
	}
	else
	{
		get_random_name(o_ptr, new_name, object_is_armour(o_ptr), power_level);
	}

	/* Save the inscription */
	o_ptr->art_name = quark_add(new_name);

	msg_format_wizard(CHEAT_OBJECT, _("パワー %d で 価値%ld のランダムアーティファクト生成 バイアスは「%s」",
		"Random artifact generated - Power:%d Value:%d Bias:%s."), max_powers, total_flags, artifact_bias_name[o_ptr->artifact_bias]);

	p_ptr->window |= (PW_INVEN | PW_EQUIP);

	return TRUE;
}

/*!
 * @brief オブジェクトから能力発動IDを取得する。
 * @details いくつかのケースで定義されている発動効果から、
 * 鍛冶師による付与＞固定アーティファクト＞エゴ＞ランダムアーティファクト＞ベースアイテムの優先順位で走査していく。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 発動効果のIDを返す
 */
int activation_index(object_type *o_ptr)
{
	/* Give priority to weaponsmith's essential activations */
	if (object_is_smith(o_ptr))
	{
		switch (o_ptr->xtra3 - 1)
		{
		case ESSENCE_TMP_RES_ACID: return ACT_RESIST_ACID;
		case ESSENCE_TMP_RES_ELEC: return ACT_RESIST_ELEC;
		case ESSENCE_TMP_RES_FIRE: return ACT_RESIST_FIRE;
		case ESSENCE_TMP_RES_COLD: return ACT_RESIST_COLD;
		case TR_IMPACT: return ACT_QUAKE;
		}
	}

	if (object_is_fixed_artifact(o_ptr))
	{
		if (have_flag(a_info[o_ptr->name1].flags, TR_ACTIVATE))
		{
			return a_info[o_ptr->name1].act_idx;
		}
	}
	if (object_is_ego(o_ptr))
	{
		if (have_flag(e_info[o_ptr->name2].flags, TR_ACTIVATE))
		{
			return e_info[o_ptr->name2].act_idx;
		}
	}
	if (!object_is_random_artifact(o_ptr))
	{
		if (have_flag(k_info[o_ptr->k_idx].flags, TR_ACTIVATE))
		{
			return k_info[o_ptr->k_idx].act_idx;
		}
	}

	return o_ptr->xtra2;
}

/*!
 * @brief オブジェクトから発動効果構造体のポインタを取得する。
 * @details activation_index() 関数の結果から参照する。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 発動効果構造体のポインタを返す
 */
const activation_type* find_activation_info(object_type *o_ptr)
{
	const int index = activation_index(o_ptr);
	const activation_type* p;

	for (p = activation_info; p->flag != NULL; ++ p) {
		if (p->index == index)
		{
			return p;
		}
	}

	return NULL;
}

/*!
 * @brief 固定アーティファクト生成時の特別なハードコーディング処理を行う。.
 * @details random_artifact_resistance()とあるが実際は固定アーティファクトである。
 * 対象は恐怖の仮面、村正、ロビントンのハープ、龍争虎鬪、ブラッディムーン、羽衣、天女の羽衣、ミリム、
 * その他追加耐性、特性追加処理。
 * @attention プレイヤーの各種ステータスに依存した処理がある。
 * @todo 折を見て関数名を変更すること。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @param a_ptr 生成する固定アーティファクト構造体ポインタ
 * @return なし
 */
void random_artifact_resistance(object_type * o_ptr, artifact_type *a_ptr)
{
	bool give_resistance = FALSE, give_power = FALSE;

	if (o_ptr->name1 == ART_TERROR) /* Terror Mask is for warriors... */
	{
		if (p_ptr->pclass == CLASS_WARRIOR || p_ptr->pclass == CLASS_ARCHER || p_ptr->pclass == CLASS_CAVALRY || p_ptr->pclass == CLASS_BERSERKER)
		{
			give_power = TRUE;
			give_resistance = TRUE;
		}
		else
		{
			add_flag(o_ptr->art_flags, TR_AGGRAVATE);
			add_flag(o_ptr->art_flags, TR_TY_CURSE);
			o_ptr->curse_flags |=
			    (TRC_CURSED | TRC_HEAVY_CURSE);
			o_ptr->curse_flags |= get_curse(2, o_ptr);
			return;
		}
	}

	if (o_ptr->name1 == ART_MURAMASA)
	{
		if (p_ptr->pclass != CLASS_SAMURAI)
		{
			add_flag(o_ptr->art_flags, TR_NO_MAGIC);
			o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
		}
	}

	if (o_ptr->name1 == ART_ROBINTON)
	{
		if (p_ptr->pclass == CLASS_BARD)
		{
			add_flag(o_ptr->art_flags, TR_DEC_MANA);
		}
	}

	if (o_ptr->name1 == ART_XIAOLONG)
	{
		if (p_ptr->pclass == CLASS_MONK)
			add_flag(o_ptr->art_flags, TR_BLOWS);
	}

	if (o_ptr->name1 == ART_BLOOD)
	{
		get_bloody_moon_flags(o_ptr);
	}

	if (o_ptr->name1 == ART_HEAVENLY_MAIDEN)
	{
		if (p_ptr->psex != SEX_FEMALE)
		{
			add_flag(o_ptr->art_flags, TR_AGGRAVATE);
		}
	}

	if (o_ptr->name1 == ART_MILIM)
	{
		if (p_ptr->pseikaku == SEIKAKU_SEXY)
		{
			o_ptr->pval = 3;
			add_flag(o_ptr->art_flags, TR_STR);
			add_flag(o_ptr->art_flags, TR_INT);
			add_flag(o_ptr->art_flags, TR_WIS);
			add_flag(o_ptr->art_flags, TR_DEX);
			add_flag(o_ptr->art_flags, TR_CON);
			add_flag(o_ptr->art_flags, TR_CHR);
		}
	}

	if (a_ptr->gen_flags & (TRG_XTRA_POWER)) give_power = TRUE;
	if (a_ptr->gen_flags & (TRG_XTRA_H_RES)) give_resistance = TRUE;
	if (a_ptr->gen_flags & (TRG_XTRA_RES_OR_POWER))
	{
		/* Give a resistance OR a power */
		if (one_in_(2)) give_resistance = TRUE;
		else give_power = TRUE;
	}

	if (give_power)
	{
		one_ability(o_ptr);
	}

	if (give_resistance)
	{
		one_high_resistance(o_ptr);
	}
}


/*!
 * @brief フロアの指定された位置に固定アーティファクトを生成する。 / Create the artifact of the specified number
 * @details 固定アーティファクト構造体から基本ステータスをコピーした後、所定の座標でdrop_item()で落とす。
 * @param a_idx 生成する固定アーティファクト構造体のID
 * @param y アイテムを落とす地点のy座標
 * @param x アイテムを落とす地点のx座標
 * @return 生成が成功したか否か、失敗はIDの不全、ベースアイテムの不全、drop_item()の失敗時に起こる。
 * @attention この処理はdrop_near()内で普通の固定アーティファクトが重ならない性質に依存する.
 * 仮に2個以上存在可能かつ装備品以外の固定アーティファクトが作成されれば
 * drop_near()関数の返り値は信用できなくなる.
 */
bool create_named_art(ARTIFACT_IDX a_idx, POSITION y, POSITION x)
{
	object_type forge;
	object_type *q_ptr;
	KIND_OBJECT_IDX i;

	artifact_type *a_ptr = &a_info[a_idx];
	q_ptr = &forge;

	/* Ignore "empty" artifacts */
	if (!a_ptr->name) return FALSE;

	/* Acquire the "kind" index */
	i = lookup_kind(a_ptr->tval, a_ptr->sval);

	if (!i) return FALSE;

	/* Create the artifact */
	object_prep(q_ptr, i);

	/* Save the name */
	q_ptr->name1 = a_idx;

	/* Extract the fields */
	q_ptr->pval = a_ptr->pval;
	q_ptr->ac = a_ptr->ac;
	q_ptr->dd = a_ptr->dd;
	q_ptr->ds = a_ptr->ds;
	q_ptr->to_a = a_ptr->to_a;
	q_ptr->to_h = a_ptr->to_h;
	q_ptr->to_d = a_ptr->to_d;
	q_ptr->weight = a_ptr->weight;

	/* Hack -- extract the "cursed" flag */
	if (a_ptr->gen_flags & TRG_CURSED) q_ptr->curse_flags |= (TRC_CURSED);
	if (a_ptr->gen_flags & TRG_HEAVY_CURSE) q_ptr->curse_flags |= (TRC_HEAVY_CURSE);
	if (a_ptr->gen_flags & TRG_PERMA_CURSE) q_ptr->curse_flags |= (TRC_PERMA_CURSE);
	if (a_ptr->gen_flags & (TRG_RANDOM_CURSE0)) q_ptr->curse_flags |= get_curse(0, q_ptr);
	if (a_ptr->gen_flags & (TRG_RANDOM_CURSE1)) q_ptr->curse_flags |= get_curse(1, q_ptr);
	if (a_ptr->gen_flags & (TRG_RANDOM_CURSE2)) q_ptr->curse_flags |= get_curse(2, q_ptr);

	random_artifact_resistance(q_ptr, a_ptr);

	/* Drop the artifact from heaven */
	return drop_near(q_ptr, -1, y, x) ? TRUE : FALSE;
}

/*対邪平均ダメージの計算処理*/
HIT_POINT calc_arm_avgdamage(object_type *o_ptr)
{
	BIT_FLAGS flgs[TR_FLAG_SIZE];
	object_flags(o_ptr, flgs);

	HIT_POINT dam, base, s_evil, forced, vorpal;
	dam = base = s_evil = forced = vorpal = 0;

	dam = base = (o_ptr->dd * o_ptr->ds + o_ptr->dd) / 2;

	if(have_flag(flgs, TR_KILL_EVIL))
	{
		dam = s_evil = dam * 7 / 2;
	}
	else if(!have_flag(flgs, TR_KILL_EVIL) && have_flag(flgs, TR_SLAY_EVIL))
	{	
		dam = s_evil = dam * 2;
	}
	else s_evil = dam;

	if (have_flag(flgs, TR_FORCE_WEAPON))
	{
		dam = forced = dam * 3 / 2 + (o_ptr->dd * o_ptr->ds + o_ptr->dd);
	}
	else forced = dam;

	if(have_flag(flgs, TR_VORPAL))
	{
		dam = vorpal = dam * 11 / 9;
	}
	else vorpal = dam;

	dam = dam + o_ptr->to_d;

	msg_format_wizard(CHEAT_OBJECT,"素:%d> 対邪:%d> 理力:%d> 切:%d> 最終:%d", base, s_evil, forced, vorpal, dam);

	return dam;
}

static bool suppression_evil_dam(object_type *o_ptr)
{
	BIT_FLAGS flgs[TR_FLAG_SIZE];
	object_flags(o_ptr, flgs);

	if (have_flag(flgs, TR_VAMPIRIC))
	{
		if(have_flag(flgs, TR_BLOWS) && (o_ptr->pval == 1) && (calc_arm_avgdamage(o_ptr) > 52))
		{
			return TRUE;
		}
		else if(have_flag(flgs, TR_BLOWS) && (o_ptr->pval == 2) && (calc_arm_avgdamage(o_ptr) > 43))
		{
			return TRUE;
		}
		else if( have_flag(flgs, TR_BLOWS) && (o_ptr->pval == 3) && (calc_arm_avgdamage(o_ptr) > 33))
		{
			return TRUE;
		}
		else if (calc_arm_avgdamage(o_ptr) > 63)
		{
			return TRUE;
		}
	}
	else
	{
		if (have_flag(flgs, TR_BLOWS) && (o_ptr->pval == 1) && (calc_arm_avgdamage(o_ptr) > 65))
		{
			return TRUE;
		}
		else if (have_flag(flgs, TR_BLOWS) && (o_ptr->pval == 2) && (calc_arm_avgdamage(o_ptr) > 52))
		{
			return TRUE;
		}
		else if (have_flag(flgs, TR_BLOWS) && (o_ptr->pval == 3) && (calc_arm_avgdamage(o_ptr) > 40))
		{
			return TRUE;
		}
		else if (calc_arm_avgdamage(o_ptr) > 75)
		{
			return TRUE;
		}
	}
	return FALSE;
}

static bool weakening_artifact(object_type *o_ptr)
{
	KIND_OBJECT_IDX k_idx = lookup_kind(o_ptr->sval, o_ptr->tval);
	object_kind *k_ptr = &k_info[k_idx];

	if ((k_ptr->dd < o_ptr->dd) || (k_ptr->ds < o_ptr->ds))
	{
		DICE_NUMBER pre_dd = o_ptr->dd;
		DICE_SID pre_ds = o_ptr->ds;

		if (o_ptr->dd > o_ptr->ds)
		{
			o_ptr->dd--;
		}
		else
		{
			o_ptr->ds--;
		}

		msg_format_wizard(CHEAT_OBJECT,
			_("ダイスが抑制されました。%dd%d -> %dd%d", "Dice Supress %dd%d -> %dd%d"),
			pre_dd, pre_ds, o_ptr->dd, o_ptr->ds);
		return 1;
	}

	if (o_ptr->to_d > 10)
	{
		HIT_POINT pre_damage = o_ptr->to_d;

		o_ptr->to_d = o_ptr->to_d - damroll(1, 6);
		if (o_ptr->to_d < 10)
		{
			o_ptr->to_d = 10;
		}

		msg_format_wizard(CHEAT_OBJECT,
			_("ダメージ修正が抑制されました。 %d -> %d", "Plus-Damage Supress %d -> %d"),
			pre_damage, o_ptr->to_d);

		return 1;
	}
	return 0;
}