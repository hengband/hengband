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


/* Chance of using syllables to form the name instead of the "template" files */
#define SINDARIN_NAME   10 /*!< ランダムアーティファクトにシンダリン銘をつける確率 */
#define TABLE_NAME      20 /*!< ランダムアーティファクトに漢字銘をつける確率(正確には TABLE_NAME - SINDARIN_NAME %)となる */
#define A_CURSED        13 /*!< 1/nの確率で生成の巻物以外のランダムアーティファクトが呪いつきになる。 */
#define WEIRD_LUCK      12 /*!< 1/nの確率でrandom_resistance()の処理中バイアス外の耐性がつき、create_artifactで4を超えるpvalが許可される。*/
#define BIAS_LUCK       20 /*!< 1/nの確率でrandom_resistance()で付加する元素耐性が免疫になる */
#define IM_LUCK         7 /*!< 1/nの確率でrandom_resistance()で複数免疫の除去処理が免除される */

/*! @note
 * Bias luck needs to be higher than weird luck,
 * since it is usually tested several times...
 */

#define ACTIVATION_CHANCE 3 /*!< 1/nの確率でランダムアーティファクトに発動が付加される。ただし防具はさらに1/2 */


/*!
 * アーティファクトのバイアスIDを保管する。 / Use for biased artifact creation
 */
static int artifact_bias;


/*!
 * @brief 対象のオブジェクトにランダムな能力維持を一つ付加する。/ Choose one random sustain
 * @details 重複の抑止はない。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return なし
 */
void one_sustain(object_type *o_ptr)
{
	switch (randint0(6))
	{
		case 0: add_flag(o_ptr->art_flags, TR_SUST_STR); break;
		case 1: add_flag(o_ptr->art_flags, TR_SUST_INT); break;
		case 2: add_flag(o_ptr->art_flags, TR_SUST_WIS); break;
		case 3: add_flag(o_ptr->art_flags, TR_SUST_DEX); break;
		case 4: add_flag(o_ptr->art_flags, TR_SUST_CON); break;
		case 5: add_flag(o_ptr->art_flags, TR_SUST_CHR); break;
	}
}


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
			case ACT_DRAIN_1:
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
			case ACT_DRAIN_2:
			case ACT_VAMPIRE_1:
			case ACT_BO_MISS_2:
			case ACT_BA_FIRE_2:
			case ACT_REST_EXP:
				chance = 66;
				break;
			case ACT_BA_FIRE_3:
			case ACT_BA_COLD_3:
			case ACT_BA_ELEC_3:
			case ACT_WHIRLWIND:
			case ACT_VAMPIRE_2:
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
	o_ptr->xtra2 = type;
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

	switch (artifact_bias)
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

	if ((artifact_bias == BIAS_MAGE || artifact_bias == BIAS_PRIESTLY) && (o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_ROBE))
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
		if (!artifact_bias && !one_in_(13))
			artifact_bias = BIAS_STR;
		else if (!artifact_bias && one_in_(7))
			artifact_bias = BIAS_WARRIOR;
		break;
	case 3: case 4:
		add_flag(o_ptr->art_flags, TR_INT);
		if (!artifact_bias && !one_in_(13))
			artifact_bias = BIAS_INT;
		else if (!artifact_bias && one_in_(7))
			artifact_bias = BIAS_MAGE;
		break;
	case 5: case 6:
		add_flag(o_ptr->art_flags, TR_WIS);
		if (!artifact_bias && !one_in_(13))
			artifact_bias = BIAS_WIS;
		else if (!artifact_bias && one_in_(7))
			artifact_bias = BIAS_PRIESTLY;
		break;
	case 7: case 8:
		add_flag(o_ptr->art_flags, TR_DEX);
		if (!artifact_bias && !one_in_(13))
			artifact_bias = BIAS_DEX;
		else if (!artifact_bias && one_in_(7))
			artifact_bias = BIAS_ROGUE;
		break;
	case 9: case 10:
		add_flag(o_ptr->art_flags, TR_CON);
		if (!artifact_bias && !one_in_(13))
			artifact_bias = BIAS_CON;
		else if (!artifact_bias && one_in_(9))
			artifact_bias = BIAS_RANGER;
		break;
	case 11: case 12:
		add_flag(o_ptr->art_flags, TR_CHR);
		if (!artifact_bias && !one_in_(13))
			artifact_bias = BIAS_CHR;
		break;
	case 13: case 14:
		add_flag(o_ptr->art_flags, TR_STEALTH);
		if (!artifact_bias && one_in_(3))
			artifact_bias = BIAS_ROGUE;
		break;
	case 15: case 16:
		add_flag(o_ptr->art_flags, TR_SEARCH);
		if (!artifact_bias && one_in_(9))
			artifact_bias = BIAS_RANGER;
		break;
	case 17: case 18:
		add_flag(o_ptr->art_flags, TR_INFRA);
		break;
	case 19:
		add_flag(o_ptr->art_flags, TR_SPEED);
		if (!artifact_bias && one_in_(11))
			artifact_bias = BIAS_ROGUE;
		break;
	case 20: case 21:
		add_flag(o_ptr->art_flags, TR_TUNNEL);
		break;
	case 22: case 23:
		if (o_ptr->tval == TV_BOW) random_plus(o_ptr);
		else
		{
			add_flag(o_ptr->art_flags, TR_BLOWS);
			if (!artifact_bias && one_in_(11))
				artifact_bias = BIAS_WARRIOR;
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
	switch (artifact_bias)
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
				if (!artifact_bias)
					artifact_bias = BIAS_ACID;
			}
			break;
		case 2:
			if (!one_in_(WEIRD_LUCK))
				random_resistance(o_ptr);
			else
			{
				add_flag(o_ptr->art_flags, TR_IM_ELEC);
				if (!artifact_bias)
					artifact_bias = BIAS_ELEC;
			}
			break;
		case 3:
			if (!one_in_(WEIRD_LUCK))
				random_resistance(o_ptr);
			else
			{
				add_flag(o_ptr->art_flags, TR_IM_COLD);
				if (!artifact_bias)
					artifact_bias = BIAS_COLD;
			}
			break;
		case 4:
			if (!one_in_(WEIRD_LUCK))
				random_resistance(o_ptr);
			else
			{
				add_flag(o_ptr->art_flags, TR_IM_FIRE);
				if (!artifact_bias)
					artifact_bias = BIAS_FIRE;
			}
			break;
		case 5:
		case 6:
		case 13:
			add_flag(o_ptr->art_flags, TR_RES_ACID);
			if (!artifact_bias)
				artifact_bias = BIAS_ACID;
			break;
		case 7:
		case 8:
		case 14:
			add_flag(o_ptr->art_flags, TR_RES_ELEC);
			if (!artifact_bias)
				artifact_bias = BIAS_ELEC;
			break;
		case 9:
		case 10:
		case 15:
			add_flag(o_ptr->art_flags, TR_RES_FIRE);
			if (!artifact_bias)
				artifact_bias = BIAS_FIRE;
			break;
		case 11:
		case 12:
		case 16:
			add_flag(o_ptr->art_flags, TR_RES_COLD);
			if (!artifact_bias)
				artifact_bias = BIAS_COLD;
			break;
		case 17:
		case 18:
			add_flag(o_ptr->art_flags, TR_RES_POIS);
			if (!artifact_bias && !one_in_(4))
				artifact_bias = BIAS_POIS;
			else if (!artifact_bias && one_in_(2))
				artifact_bias = BIAS_NECROMANTIC;
			else if (!artifact_bias && one_in_(2))
				artifact_bias = BIAS_ROGUE;
			break;
		case 19:
		case 20:
			add_flag(o_ptr->art_flags, TR_RES_FEAR);
			if (!artifact_bias && one_in_(3))
				artifact_bias = BIAS_WARRIOR;
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
			if (!artifact_bias && one_in_(6))
				artifact_bias = BIAS_CHAOS;
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
			if (!artifact_bias && one_in_(3))
				artifact_bias = BIAS_NECROMANTIC;
			break;
		case 33:
		case 34:
			add_flag(o_ptr->art_flags, TR_RES_NEXUS);
			break;
		case 35:
		case 36:
			add_flag(o_ptr->art_flags, TR_RES_CHAOS);
			if (!artifact_bias && one_in_(2))
				artifact_bias = BIAS_CHAOS;
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
			if (!artifact_bias)
				artifact_bias = BIAS_ELEC;
			break;
		case 40:
			if (o_ptr->tval >= TV_CLOAK && o_ptr->tval <= TV_HARD_ARMOR)
				add_flag(o_ptr->art_flags, TR_SH_FIRE);
			else
				random_resistance(o_ptr);
			if (!artifact_bias)
				artifact_bias = BIAS_FIRE;
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
			if (!artifact_bias)
				artifact_bias = BIAS_COLD;
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
	switch (artifact_bias)
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
			if (!artifact_bias)
				artifact_bias = BIAS_STR;
			break;
		case 2:
			add_flag(o_ptr->art_flags, TR_SUST_INT);
			if (!artifact_bias)
				artifact_bias = BIAS_INT;
			break;
		case 3:
			add_flag(o_ptr->art_flags, TR_SUST_WIS);
			if (!artifact_bias)
				artifact_bias = BIAS_WIS;
			break;
		case 4:
			add_flag(o_ptr->art_flags, TR_SUST_DEX);
			if (!artifact_bias)
				artifact_bias = BIAS_DEX;
			break;
		case 5:
			add_flag(o_ptr->art_flags, TR_SUST_CON);
			if (!artifact_bias)
				artifact_bias = BIAS_CON;
			break;
		case 6:
			add_flag(o_ptr->art_flags, TR_SUST_CHR);
			if (!artifact_bias)
				artifact_bias = BIAS_CHR;
			break;
		case 7:
		case 8:
		case 14:
			add_flag(o_ptr->art_flags, TR_FREE_ACT);
			break;
		case 9:
			add_flag(o_ptr->art_flags, TR_HOLD_EXP);
			if (!artifact_bias && one_in_(5))
				artifact_bias = BIAS_PRIESTLY;
			else if (!artifact_bias && one_in_(6))
				artifact_bias = BIAS_NECROMANTIC;
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
			int bonus_h, bonus_d;
			add_flag(o_ptr->art_flags, TR_SHOW_MODS);
			bonus_h = 4 + (randint1(11));
			bonus_d = 4 + (randint1(11));
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
				if (!artifact_bias && one_in_(3))
					artifact_bias = BIAS_LAW;
				break;
			case 2:
				add_flag(o_ptr->art_flags, TR_ESP_NONLIVING);
				if (!artifact_bias && one_in_(3))
					artifact_bias = BIAS_MAGE;
				break;
			case 3:
				add_flag(o_ptr->art_flags, TR_TELEPATHY);
				if (!artifact_bias && one_in_(9))
					artifact_bias = BIAS_MAGE;
				break;
			}
			break;

		case 33:
		{
			int idx[3];
			int n = randint1(3);

			idx[0] = randint1(8);

			idx[1] = randint1(7);
			if (idx[1] >= idx[0]) idx[1]++;

			idx[2] = randint1(6);
			if (idx[2] >= idx[0]) idx[2]++;
			if (idx[2] >= idx[1]) idx[2]++;

			while (n--) switch (idx[n])
			{
			case 1:
				add_flag(o_ptr->art_flags, TR_ESP_ANIMAL);
				if (!artifact_bias && one_in_(4))
					artifact_bias = BIAS_RANGER;
				break;
			case 2:
				add_flag(o_ptr->art_flags, TR_ESP_UNDEAD);
				if (!artifact_bias && one_in_(3))
					artifact_bias = BIAS_PRIESTLY;
				else if (!artifact_bias && one_in_(6))
					artifact_bias = BIAS_NECROMANTIC;
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
				add_flag(o_ptr->art_flags, TR_ESP_HUMAN);
				if (!artifact_bias && one_in_(6))
					artifact_bias = BIAS_ROGUE;
				break;
			case 8:
				add_flag(o_ptr->art_flags, TR_ESP_GOOD);
				if (!artifact_bias && one_in_(3))
					artifact_bias = BIAS_LAW;
				break;
			case 9:
				add_flag(o_ptr->art_flags, TR_ESP_UNIQUE);
				if (!artifact_bias && one_in_(3))
					artifact_bias = BIAS_LAW;
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
				if (!artifact_bias && one_in_(9))
					artifact_bias = BIAS_RANGER;
				break;
			default:
				add_flag(o_ptr->art_flags, TR_XTRA_SHOTS);
				if (!one_in_(7)) remove_flag(o_ptr->art_flags, TR_XTRA_MIGHT);
				if (!artifact_bias && one_in_(9))
					artifact_bias = BIAS_RANGER;
			break;
		}

		return;
	}

	switch (artifact_bias)
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
			add_flag(o_ptr->art_flags, TR_SLAY_ANIMAL);
			break;
		case 3:
		case 4:
			add_flag(o_ptr->art_flags, TR_SLAY_EVIL);
			if (!artifact_bias && one_in_(2))
				artifact_bias = BIAS_LAW;
			else if (!artifact_bias && one_in_(9))
				artifact_bias = BIAS_PRIESTLY;
			break;
		case 5:
		case 6:
			add_flag(o_ptr->art_flags, TR_SLAY_UNDEAD);
			if (!artifact_bias && one_in_(9))
				artifact_bias = BIAS_PRIESTLY;
			break;
		case 7:
		case 8:
			add_flag(o_ptr->art_flags, TR_SLAY_DEMON);
			if (!artifact_bias && one_in_(9))
				artifact_bias = BIAS_PRIESTLY;
			break;
		case 9:
		case 10:
			add_flag(o_ptr->art_flags, TR_SLAY_ORC);
			break;
		case 11:
		case 12:
			add_flag(o_ptr->art_flags, TR_SLAY_TROLL);
			break;
		case 13:
		case 14:
			add_flag(o_ptr->art_flags, TR_SLAY_GIANT);
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
				if (!artifact_bias && one_in_(9))
					artifact_bias = BIAS_WARRIOR;
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
			if (!artifact_bias)
				artifact_bias = BIAS_FIRE;
			break;
		case 23:
		case 24:
			add_flag(o_ptr->art_flags, TR_BRAND_COLD);
			if (!artifact_bias)
				artifact_bias = BIAS_COLD;
			break;
		case 25:
		case 26:
			add_flag(o_ptr->art_flags, TR_BRAND_ELEC);
			if (!artifact_bias)
				artifact_bias = BIAS_ELEC;
			break;
		case 27:
		case 28:
			add_flag(o_ptr->art_flags, TR_BRAND_ACID);
			if (!artifact_bias)
				artifact_bias = BIAS_ACID;
			break;
		case 29:
		case 30:
			add_flag(o_ptr->art_flags, TR_BRAND_POIS);
			if (!artifact_bias && !one_in_(3))
				artifact_bias = BIAS_POIS;
			else if (!artifact_bias && one_in_(6))
				artifact_bias = BIAS_NECROMANTIC;
			else if (!artifact_bias)
				artifact_bias = BIAS_ROGUE;
			break;
		case 31:
			add_flag(o_ptr->art_flags, TR_VAMPIRIC);
			if (!artifact_bias)
				artifact_bias = BIAS_NECROMANTIC;
			break;
		case 32:
			add_flag(o_ptr->art_flags, TR_FORCE_WEAPON);
			if (!artifact_bias)
				artifact_bias = (one_in_(2) ? BIAS_MAGE : BIAS_PRIESTLY);
			break;
		case 33:
		case 34:
			add_flag(o_ptr->art_flags, TR_SLAY_HUMAN);
			break;
		default:
			add_flag(o_ptr->art_flags, TR_CHAOTIC);
			if (!artifact_bias)
				artifact_bias = BIAS_CHAOS;
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

	switch (artifact_bias)
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
				type = ACT_VAMPIRE_2;
			else if (one_in_(6))
				type = ACT_CHARM_UNDEAD;
			else
				type = ACT_VAMPIRE_1;
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
	o_ptr->xtra2 = type;
	add_flag(o_ptr->art_flags, TR_ACTIVATE);
	o_ptr->timeout = 0;
}

/*!
 * @brief ランダムアーティファクト生成中、対象のオブジェクトに名前を与える。/ Set name of randomartifact.
 * @details 確率によって、シンダリン銘、漢字銘、固定名のいずれか一つが与えられる。
 * @param return_name 名前を返すための文字列参照ポインタ
 * @param armour 対象のオブジェクトが防具が否か
 * @param power 銘の基準となるオブジェクトの価値レベル(0=呪い、1=低位、2=中位、3以上=高位)
 * @return なし
 */
static void get_random_name(char *return_name, bool armour, int power)
{
	int prob = randint1(100);

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

		(void)get_rnd_line(filename, artifact_bias, return_name);
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
	char    new_name[1024];
	int     has_pval = 0;
	int     powers = randint1(5) + 1;
	int     max_powers;
	int     max_type = (object_is_weapon_ammo(o_ptr) ? 7 : 5);
	int     power_level;
	s32b    total_flags;
	bool    a_cursed = FALSE;
	int     warrior_artifact_bias = 0;
	int i;

	/* Reset artifact bias */
	artifact_bias = 0;

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
				artifact_bias = BIAS_WARRIOR;
				break;
			case CLASS_MAGE:
			case CLASS_HIGH_MAGE:
			case CLASS_SORCERER:
			case CLASS_MAGIC_EATER:
			case CLASS_BLUE_MAGE:
				artifact_bias = BIAS_MAGE;
				break;
			case CLASS_PRIEST:
				artifact_bias = BIAS_PRIESTLY;
				break;
			case CLASS_ROGUE:
			case CLASS_NINJA:
				artifact_bias = BIAS_ROGUE;
				warrior_artifact_bias = 25;
				break;
			case CLASS_RANGER:
			case CLASS_SNIPER:
				artifact_bias = BIAS_RANGER;
				warrior_artifact_bias = 30;
				break;
			case CLASS_PALADIN:
				artifact_bias = BIAS_PRIESTLY;
				warrior_artifact_bias = 40;
				break;
			case CLASS_WARRIOR_MAGE:
			case CLASS_RED_MAGE:
				artifact_bias = BIAS_MAGE;
				warrior_artifact_bias = 40;
				break;
			case CLASS_CHAOS_WARRIOR:
				artifact_bias = BIAS_CHAOS;
				warrior_artifact_bias = 40;
				break;
			case CLASS_MONK:
			case CLASS_FORCETRAINER:
				artifact_bias = BIAS_PRIESTLY;
				break;
			case CLASS_MINDCRAFTER:
			case CLASS_BARD:
				if (randint1(5) > 2) artifact_bias = BIAS_PRIESTLY;
				break;
			case CLASS_TOURIST:
				if (randint1(5) > 2) artifact_bias = BIAS_WARRIOR;
				break;
			case CLASS_IMITATOR:
				if (randint1(2) > 1) artifact_bias = BIAS_RANGER;
				break;
			case CLASS_BEASTMASTER:
				artifact_bias = BIAS_CHR;
				warrior_artifact_bias = 50;
				break;
			case CLASS_MIRROR_MASTER:
				if (randint1(4) > 1) 
				{
				    artifact_bias = BIAS_MAGE;
				}
				else
				{
				    artifact_bias = BIAS_ROGUE;
				}
				break;
		}
	}

	if (a_scroll && (randint1(100) <= warrior_artifact_bias))
		artifact_bias = BIAS_WARRIOR;

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
#if 0
		add_flag(o_ptr->art_flags, TR_SHOW_MODS);

		/* This one commented out by gw's request... */
		if (!a_scroll)
			add_flag(o_ptr->art_flags, TR_HIDE_TYPE);
#endif

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
			o_ptr->to_d -= (s16b)randint0(3);
			o_ptr->to_h -= (s16b)randint0(3);
		}
		while ((o_ptr->to_d+o_ptr->to_h) > 10)
		{
			if (one_in_(o_ptr->to_d) || one_in_(o_ptr->to_h)) break;
			o_ptr->to_d -= (s16b)randint0(3);
			o_ptr->to_h -= (s16b)randint0(3);
		}
	}

	if (((artifact_bias == BIAS_MAGE) || (artifact_bias == BIAS_INT)) && (o_ptr->tval == TV_GLOVES)) add_flag(o_ptr->art_flags, TR_FREE_ACT);

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

	if (a_scroll)
	{
		char dummy_name[80] = "";
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
		get_random_name(new_name, object_is_armour(o_ptr), power_level);
	}

	/* Save the inscription */
	o_ptr->art_name = quark_add(new_name);

	if (cheat_xtra)
	{
		char o_name[MAX_NLEN];

		object_aware(o_ptr);
		object_known(o_ptr);

		/* Mark the item as fully known */
		o_ptr->ident |= (IDENT_MENTAL);

		/* Description */
		object_desc(o_name, o_ptr, 0);

#ifdef JP
		msg_format("パワー %d で 価値%ld のランダムアーティファクト生成 バイアスは「%s」:", max_powers, total_flags, artifact_bias_name[artifact_bias]);
#else
		msg_format("Random artifact generated '%s'. (Power:%d, Value:%ld) :", artifact_bias_name[artifact_bias], max_powers, total_flags);
#endif
		msg_format("%s", o_name);
	}

	/* Window stuff */
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
		switch (o_ptr->xtra3-1)
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
 * @brief 発動によるブレスの属性をアイテムの耐性から選択し、実行を処理する。/ Dragon breath activation
 * @details 対象となる耐性は dragonbreath_info テーブルを参照のこと。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 発動実行の是非を返す。
 */
static bool activate_dragon_breath(object_type *o_ptr)
{
	u32b flgs[TR_FLAG_SIZE]; /* for resistance flags */
	int type[20];
	cptr name[20];
	int i, dir, t, n = 0;

	if (!get_aim_dir(&dir)) return FALSE;

	object_flags(o_ptr, flgs);

	for (i = 0; dragonbreath_info[i].flag != 0; i++)
	{
		if (have_flag(flgs, dragonbreath_info[i].flag))
		{
			type[n] = dragonbreath_info[i].type;
			name[n] = dragonbreath_info[i].name;
			n++;
		}
	}

	/* Paranoia */
	if (n == 0) return FALSE;

	/* Stop speaking */
	if (music_singing_any()) stop_singing();
	if (hex_spelling_any()) stop_hex_spell_all();

	t = randint0(n);
	msg_format(_("あなたは%sのブレスを吐いた。", "You breathe %s."), name[t]);
	fire_ball(type[t], dir, 250, -4);

	return TRUE;
}

/*!
 * @brief アイテムの発動効果を処理する。
 * @details activate_random_artifact()とされているが、実際は全発動が統合された。
 * @todo 折を見て関数名を修正すること。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return 発動実行の是非を返す。
 */
bool activate_random_artifact(object_type *o_ptr)
{
	int plev = p_ptr->lev;
	int k, dir, dummy = 0;
	cptr name = k_name + k_info[o_ptr->k_idx].name;
	const activation_type* const act_ptr = find_activation_info(o_ptr);

	/* Paranoia */
	if (!act_ptr) {
		/* Maybe forgot adding information to activation_info table ? */
		msg_print("Activation information is not found.");
		return FALSE;
	}

	/* Activate for attack */
	switch (act_ptr->index)
	{
		case ACT_SUNLIGHT:
		{
			if (!get_aim_dir(&dir)) return FALSE;
			msg_print(_("太陽光線が放たれた。", "A line of sunlight appears."));
			(void)lite_line(dir, damroll(6, 8));
			break;
		}

		case ACT_BO_MISS_1:
		{
			msg_print(_("それは眩しいくらいに明るく輝いている...", "It glows extremely brightly..."));
			if (!get_aim_dir(&dir)) return FALSE;
			fire_bolt(GF_MISSILE, dir, damroll(2, 6));
			break;
		}

		case ACT_BA_POIS_1:
		{
			msg_print(_("それは濃緑色に脈動している...","It throbs deep green..."));
			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_POIS, dir, 12, 3);
			break;
		}

		case ACT_BO_ELEC_1:
		{
			msg_print(_("それは火花に覆われた...", "It is covered in sparks..."));
			if (!get_aim_dir(&dir)) return FALSE;
			fire_bolt(GF_ELEC, dir, damroll(4, 8));
			break;
		}

		case ACT_BO_ACID_1:
		{
			msg_print(_("それは酸に覆われた...","It is covered in acid..."));
			if (!get_aim_dir(&dir)) return FALSE;
			fire_bolt(GF_ACID, dir, damroll(5, 8));
			break;
		}

		case ACT_BO_COLD_1:
		{
			msg_print(_("それは霜に覆われた...","It is covered in frost..."));
			if (!get_aim_dir(&dir)) return FALSE;
			fire_bolt(GF_COLD, dir, damroll(6, 8));
			break;
		}

		case ACT_BO_FIRE_1:
		{
			msg_print(_("それは炎に覆われた...","It is covered in fire..."));
			if (!get_aim_dir(&dir)) return FALSE;
			fire_bolt(GF_FIRE, dir, damroll(9, 8));
			break;
		}

		case ACT_BA_COLD_1:
		{
			msg_print(_("それは霜に覆われた...","It is covered in frost..."));
			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_COLD, dir, 48, 2);
			break;
		}
		
		case ACT_BA_COLD_2:
		{
			msg_print(_("それは青く激しく輝いた...", "It glows an intense blue..."));
			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_COLD, dir, 100, 2);
			break;
		}
		
		case ACT_BA_COLD_3:
		{
			msg_print(_("明るく白色に輝いている...", "It glows bright white..."));
			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_COLD, dir, 400, 3);
			break;
		}

		case ACT_BA_FIRE_1:
		{
			msg_print(_("それは赤く激しく輝いた...","It glows an intense red..."));
			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_FIRE, dir, 72, 2);
			break;
		}
		
		case ACT_BA_FIRE_2:
		{
			msg_format(_("%sから炎が吹き出した...", "The %s rages in fire..."), name);
			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_FIRE, dir, 120, 3);
			break;
		}
		
		case ACT_BA_FIRE_3:
		{
			msg_print(_("深赤色に輝いている...", "It glows deep red..."));
			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_FIRE, dir, 300, 3);
			break;
		}
		
		case ACT_BA_FIRE_4:
		{
			msg_print(_("それは赤く激しく輝いた...","It glows an intense red..."));
			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_FIRE, dir, 100, 2);
			break;
		}
		
		case ACT_BA_ELEC_2:
		{
			msg_print(_("電気がパチパチ音を立てた...","It crackles with electricity..."));
			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_ELEC, dir, 100, 3);
			break;
		}
		
		case ACT_BA_ELEC_3:
		{
			msg_print(_("深青色に輝いている...", "It glows deep blue..."));
			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_ELEC, dir, 500, 3);
			break;
		}
		
		case ACT_BA_ACID_1:
		{
			msg_print(_("それは黒く激しく輝いた...","It glows an intense black..."));
			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_ACID, dir, 100, 2);
			break;
		}
		
		case ACT_BA_NUKE_1:
		{
			msg_print(_("それは緑に激しく輝いた...","It glows an intense green..."));
			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_NUKE, dir, 100, 2);
			break;
		}
		
		case ACT_DRAIN_1:
		{
			msg_format(_("あなたは%sに敵を締め殺すよう命じた。", "You order the %s to strangle your opponent."), name);
			if (!get_aim_dir(&dir)) return FALSE;
			if (drain_life(dir, 100))
			break;
		}

		case ACT_DRAIN_2:
		{
			msg_print(_("黒く輝いている...", "It glows black..."));
			if (!get_aim_dir(&dir)) return FALSE;
			drain_life(dir, 120);
			break;
		}

		case ACT_VAMPIRE_1:
		{
			if (!get_aim_dir(&dir)) return FALSE;
			for (dummy = 0; dummy < 3; dummy++)
			{
				if (drain_life(dir, 50))
				hp_player(50);
			}
			break;
		}

		case ACT_BO_MISS_2:
		{
			msg_print(_("魔法のトゲが現れた...", "It grows magical spikes..."));
			if (!get_aim_dir(&dir)) return FALSE;
			fire_bolt(GF_ARROW, dir, 150);
			break;
		}

		case ACT_WHIRLWIND:
		{
			{
				int y = 0, x = 0;
				cave_type       *c_ptr;
				monster_type    *m_ptr;

				for (dir = 0; dir <= 9; dir++)
				{
					y = py + ddy[dir];
					x = px + ddx[dir];
					c_ptr = &cave[y][x];

					/* Get the monster */
					m_ptr = &m_list[c_ptr->m_idx];

					/* Hack -- attack monsters */
					if (c_ptr->m_idx && (m_ptr->ml || cave_have_flag_bold(y, x, FF_PROJECT)))
						py_attack(y, x, 0);
				}
			}
			break;
		}

		case ACT_VAMPIRE_2:
		{
			if (!get_aim_dir(&dir)) return FALSE;
			for (dummy = 0; dummy < 3; dummy++)
			{
				if (drain_life(dir, 100))
				hp_player(100);
			}
			break;
		}


		case ACT_CALL_CHAOS:
		{
			msg_print(_("様々な色の火花を発している...","It glows in scintillating colours..."));
			call_chaos();
			break;
		}

		case ACT_ROCKET:
		{
			if (!get_aim_dir(&dir)) return FALSE;
			msg_print(_("ロケットを発射した！", "You launch a rocket!"));
			fire_ball(GF_ROCKET, dir, 250 + plev*3, 2);
			break;
		}

		case ACT_DISP_EVIL:
		{
			msg_print(_("神聖な雰囲気が充満した...", "It floods the area with goodness..."));
			dispel_evil(p_ptr->lev * 5);
			break;
		}

		case ACT_BA_MISS_3:
		{
			if (!get_aim_dir(&dir)) return FALSE;
			msg_print(_("あなたはエレメントのブレスを吐いた。", "You breathe the elements."));
			fire_ball(GF_MISSILE, dir, 300, -4);
			break;
		}

		case ACT_DISP_GOOD:
		{
			msg_print(_("邪悪な雰囲気が充満した...", "It floods the area with evil..."));
			dispel_good(p_ptr->lev * 5);
			break;
		}

		case ACT_BO_MANA:
		{
			msg_format(_("%sに魔法のトゲが現れた...", "The %s grows magical spikes..."), name);
			if (!get_aim_dir(&dir)) return FALSE;
			fire_bolt(GF_ARROW, dir, 150);
			break;
		}

		case ACT_BA_WATER:
		{
			msg_format(_("%sが深い青色に鼓動している...", "The %s throbs deep blue..."), name);
			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_WATER, dir, 200, 3);
			break;
		}

		case ACT_BA_DARK:
		{
			msg_format(_("%sが深い闇に覆われた...","The %s is coverd in pitch-darkness..."), name);
			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_DARK, dir, 250, 4);
			break;
		}

		case ACT_BA_MANA:
		{
			msg_format(_("%sが青白く光った．．．", "The %s glows pale..."), name);
			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_MANA, dir, 250, 4);
			break;
		}

		case ACT_PESTICIDE:
		{
			msg_print(_("あなたは害虫を一掃した。","You exterminate small life."));
			(void)dispel_monsters(4);
			break;
		}

		case ACT_BLINDING_LIGHT:
		{
			msg_format(_("%sが眩しい光で輝いた...", "The %s gleams with blinding light..."), name);
			fire_ball(GF_LITE, 0, 300, 6);
			confuse_monsters(3 * p_ptr->lev / 2);
			break;
		}

		case ACT_BIZARRE:
		{
			msg_format(_("%sは漆黒に輝いた...", "The %s glows intensely black..."), name);
			if (!get_aim_dir(&dir)) return FALSE;
			ring_of_power(dir);
			break;
		}

		case ACT_CAST_BA_STAR:
		{
			int num = damroll(5, 3);
			int y, x;
			int attempts;
			msg_format(_("%sが稲妻で覆われた...","The %s is surrounded by lightning..."), name);
			for (k = 0; k < num; k++)
			{
				attempts = 1000;

				while (attempts--)
				{
					scatter(&y, &x, py, px, 4, 0);

					if (!cave_have_flag_bold(y, x, FF_PROJECT)) continue;

					if (!player_bold(y, x)) break;
				}

				project(0, 3, y, x, 150, GF_ELEC,
							(PROJECT_THRU | PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL), -1);
			}

			break;
		}

		case ACT_BLADETURNER:
		{
			if (!get_aim_dir(&dir)) return FALSE;
			msg_print(_("あなたはエレメントのブレスを吐いた。", "You breathe the elements."));
			fire_ball(GF_MISSILE, dir, 300, -4);
			msg_print(_("鎧が様々な色に輝いた...", "Your armor glows many colours..."));
			(void)set_afraid(0);
			(void)set_hero(randint1(50) + 50, FALSE);
			(void)hp_player(10);
			(void)set_blessed(randint1(50) + 50, FALSE);
			(void)set_oppose_acid(randint1(50) + 50, FALSE);
			(void)set_oppose_elec(randint1(50) + 50, FALSE);
			(void)set_oppose_fire(randint1(50) + 50, FALSE);
			(void)set_oppose_cold(randint1(50) + 50, FALSE);
			(void)set_oppose_pois(randint1(50) + 50, FALSE);
			break;
		}

		case ACT_BR_FIRE:
		{
			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_FIRE, dir, 200, -2);
			if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_FLAMES))
			{
				(void)set_oppose_fire(randint1(20) + 20, FALSE);
			}
			break;
		}
		case ACT_BR_COLD:
		{
			if (!get_aim_dir(&dir)) return FALSE;
			fire_ball(GF_COLD, dir, 200, -2);
			if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ICE))
			{
				(void)set_oppose_cold(randint1(20) + 20, FALSE);
			}
			break;
		}
		case ACT_BR_DRAGON:
		{
			if (!activate_dragon_breath(o_ptr)) return FALSE;
			break;
		}

		/* Activate for other offensive action */

		case ACT_CONFUSE:
		{
			msg_print(_("様々な色の火花を発している...", "It glows in scintillating colours..."));
			if (!get_aim_dir(&dir)) return FALSE;
			confuse_monster(dir, 20);
			break;
		}

		case ACT_SLEEP:
		{
			msg_print(_("深青色に輝いている...", "It glows deep blue..."));
			sleep_monsters_touch();
			break;
		}

		case ACT_QUAKE:
		{
			earthquake(py, px, 5);
			break;
		}

		case ACT_TERROR:
		{
			turn_monsters(40 + p_ptr->lev);
			break;
		}

		case ACT_TELE_AWAY:
		{
			if (!get_aim_dir(&dir)) return FALSE;
			(void)fire_beam(GF_AWAY_ALL, dir, plev);
			break;
		}

		case ACT_BANISH_EVIL:
		{
			if (banish_evil(100))
			{
				msg_print(_("アーティファクトの力が邪悪を打ち払った！", "The power of the artifact banishes evil!"));
			}
			break;
		}

		case ACT_GENOCIDE:
		{
			msg_print(_("深青色に輝いている...", "It glows deep blue..."));
			(void)symbol_genocide(200, TRUE);
			break;
		}

		case ACT_MASS_GENO:
		{
			msg_print(_("ひどく鋭い音が流れ出た...", "It lets out a long, shrill note..."));
			(void)mass_genocide(200, TRUE);
			break;
		}

		case ACT_SCARE_AREA:
		{
			if (music_singing_any()) stop_singing();
			if (hex_spelling_any()) stop_hex_spell_all();
			msg_print(_("あなたは力強い突風を吹き鳴らした。周囲の敵が震え上っている!",
					"You wind a mighty blast; your enemies tremble!"));
			(void)turn_monsters((3 * p_ptr->lev / 2) + 10);
			break;
		}

		case ACT_AGGRAVATE:
		{
			if (o_ptr->name1 == ART_HYOUSIGI)
			{
				msg_print(_("拍子木を打った。", "You beat Your wooden clappers."));
			}
			else
			{
				msg_format(_("%sは不快な物音を立てた。","The %s sounds an unpleasant noise."), name);
			}
			aggravate_monsters(0);
			break;
		}

		/* Activate for summoning / charming */

		case ACT_CHARM_ANIMAL:
		{
			if (!get_aim_dir(&dir)) return FALSE;
			(void)charm_animal(dir, plev * 2);
			break;
		}

		case ACT_CHARM_UNDEAD:
		{
			if (!get_aim_dir(&dir)) return FALSE;
			(void)control_one_undead(dir, plev * 2);
			break;
		}

		case ACT_CHARM_OTHER:
		{
			if (!get_aim_dir(&dir)) return FALSE;
			(void)charm_monster(dir, plev * 2);
			break;
		}

		case ACT_CHARM_ANIMALS:
		{
			(void)charm_animals(plev * 2);
			break;
		}

		case ACT_CHARM_OTHERS:
		{
			charm_monsters(plev * 2);
			break;
		}

		case ACT_SUMMON_ANIMAL:
		{
			(void)summon_specific(-1, py, px, plev, SUMMON_ANIMAL_RANGER, (PM_ALLOW_GROUP | PM_FORCE_PET));
			break;
		}

		case ACT_SUMMON_PHANTOM:
		{
			msg_print(_("幻霊を召喚した。", "You summon a phantasmal servant."));
			(void)summon_specific(-1, py, px, dun_level, SUMMON_PHANTOM, (PM_ALLOW_GROUP | PM_FORCE_PET));
			break;
		}

		case ACT_SUMMON_ELEMENTAL:
		{
			bool pet = one_in_(3);
			u32b mode = 0L;

			if (!(pet && (plev < 50))) mode |= PM_ALLOW_GROUP;
			if (pet) mode |= PM_FORCE_PET;
			else mode |= PM_NO_PET;

			if (summon_specific((pet ? -1 : 0), py, px, ((plev * 3) / 2), SUMMON_ELEMENTAL, mode))
			{
				msg_print(_("エレメンタルが現れた...", "An elemental materializes..."));
				if (pet)
					msg_print(_("あなたに服従しているようだ。", "It seems obedient to you."));
				else
					msg_print(_("それをコントロールできなかった！", "You fail to control it!"));
			}

			break;
		}

		case ACT_SUMMON_DEMON:
		{
			bool pet = one_in_(3);
			u32b mode = 0L;

			if (!(pet && (plev < 50))) mode |= PM_ALLOW_GROUP;
			if (pet) mode |= PM_FORCE_PET;
			else mode |= PM_NO_PET;

			if (summon_specific((pet ? -1 : 0), py, px, ((plev * 3) / 2), SUMMON_DEMON, mode))
			{
				msg_print(_("硫黄の悪臭が充満した。", "The area fills with a stench of sulphur and brimstone."));
				if (pet)
					msg_print(_("「ご用でございますか、ご主人様」", "'What is thy bidding... Master?'"));
				else
					msg_print(_("「NON SERVIAM! Wretch! お前の魂を頂くぞ！」", "'NON SERVIAM! Wretch! I shall feast on thy mortal soul!'"));
			}

			break;
		}

		case ACT_SUMMON_UNDEAD:
		{
			bool pet = one_in_(3);
			int type;
			u32b mode = 0L;

			type = (plev > 47 ? SUMMON_HI_UNDEAD : SUMMON_UNDEAD);

			if (!pet || ((plev > 24) && one_in_(3))) mode |= PM_ALLOW_GROUP;
			if (pet) mode |= PM_FORCE_PET;
			else mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);

			if (summon_specific((pet ? -1 : 0), py, px, ((plev * 3) / 2), type, mode))
			{
				msg_print(_("冷たい風があなたの周りに吹き始めた。それは腐敗臭を運んでいる...",
						"Cold winds begin to blow around you, carrying with them the stench of decay..."));
				if (pet)
				msg_print(_("古えの死せる者共があなたに仕えるため土から甦った！",
						"Ancient, long-dead forms arise from the ground to serve you!"));
				else
				msg_print(_("死者が甦った。眠りを妨げるあなたを罰するために！",
						"'The dead arise... to punish you for disturbing them!'"));
			}

			break;
		}

		case ACT_SUMMON_HOUND:
		{
			u32b mode = PM_ALLOW_GROUP;
			bool pet = !one_in_(5);
			if (pet) mode |= PM_FORCE_PET;
			else mode |= PM_NO_PET;

			if (summon_specific((pet ? -1 : 0), py, px, ((p_ptr->lev * 3) / 2), SUMMON_HOUND, mode))
			{

				if (pet)
					msg_print(_("ハウンドがあなたの下僕として出現した。",
						"A group of hounds appear as your servant."));
				else
					msg_print(_("ハウンドはあなたに牙を向けている！",
						"A group of hounds appear as your enemy!"));
			}

			break;
		}

		case ACT_SUMMON_DAWN:
		{
			msg_print(_("暁の師団を召喚した。","You summon the Legion of the Dawn."));
			(void)summon_specific(-1, py, px, dun_level, SUMMON_DAWN, (PM_ALLOW_GROUP | PM_FORCE_PET));
			break;
		}

		case ACT_SUMMON_OCTOPUS:
		{
			u32b mode = PM_ALLOW_GROUP;
			bool pet = !one_in_(5);
			if (pet) mode |= PM_FORCE_PET;

			if (summon_named_creature(0, py, px, MON_JIZOTAKO, mode))
			{
				if (pet)
					msg_print(_("蛸があなたの下僕として出現した。", "A group of octopuses appear as your servant."));
				else
					msg_print(_("蛸はあなたを睨んでいる！", "A group of octopuses appear as your enemy!"));
			}

			break;
		}

		/* Activate for healing */

		case ACT_CHOIR_SINGS:
		{
			msg_print(_("天国の歌が聞こえる...", "A heavenly choir sings..."));
			(void)set_poisoned(0);
			(void)set_cut(0);
			(void)set_stun(0);
			(void)set_confused(0);
			(void)set_blind(0);
			(void)set_afraid(0);
			(void)set_hero(randint1(25) + 25, FALSE);
			(void)hp_player(777);
			break;
		}

		case ACT_CURE_LW:
		{
			(void)set_afraid(0);
			(void)hp_player(30);
			break;
		}

		case ACT_CURE_MW:
		{
			msg_print(_("深紫色の光を発している...", "It radiates deep purple..."));
			hp_player(damroll(4, 8));
			(void)set_cut((p_ptr->cut / 2) - 50);
			break;
		}

		case ACT_CURE_POISON:
		{
			msg_print(_("深青色に輝いている...", "It glows deep blue..."));
			(void)set_afraid(0);
			(void)set_poisoned(0);
			break;
		}

		case ACT_REST_EXP:
		{
			msg_print(_("深紅に輝いている...", "It glows a deep red..."));
			restore_level();
			break;
		}

		case ACT_REST_ALL:
		{
			msg_print(_("濃緑色に輝いている...", "It glows a deep green..."));
			(void)do_res_stat(A_STR);
			(void)do_res_stat(A_INT);
			(void)do_res_stat(A_WIS);
			(void)do_res_stat(A_DEX);
			(void)do_res_stat(A_CON);
			(void)do_res_stat(A_CHR);
			(void)restore_level();
			break;
		}

		case ACT_CURE_700:
		{
			msg_print(_("深青色に輝いている...","It glows deep blue..."));
			msg_print(_("体内に暖かい鼓動が感じられる...","You feel a warm tingling inside..."));
			(void)hp_player(700);
			(void)set_cut(0);
			break;
		}

		case ACT_CURE_1000:
		{
			msg_print(_("白く明るく輝いている...","It glows a bright white..."));
			msg_print(_("ひじょうに気分がよい...","You feel much better..."));
			(void)hp_player(1000);
			(void)set_cut(0);
			break;
		}

		case ACT_CURING:
		{
			msg_format(_("%sの優しさに癒される...", "the %s cures you affectionately ..."), name);
			(void)set_poisoned(0);
			(void)set_confused(0);
			(void)set_blind(0);
			(void)set_stun(0);
			(void)set_cut(0);
			(void)set_image(0);

			break;
		}

		case ACT_CURE_MANA_FULL:
		{
			msg_format(_("%sが青白く光った．．．","The %s glows pale..."), name);
			if (p_ptr->pclass == CLASS_MAGIC_EATER)
			{
				int i;
				for (i = 0; i < EATER_EXT*2; i++)
				{
					p_ptr->magic_num1[i] += (p_ptr->magic_num2[i] < 10) ? EATER_CHARGE * 3 : p_ptr->magic_num2[i]*EATER_CHARGE/3;
					if (p_ptr->magic_num1[i] > p_ptr->magic_num2[i]*EATER_CHARGE) p_ptr->magic_num1[i] = p_ptr->magic_num2[i]*EATER_CHARGE;
				}
				for (; i < EATER_EXT*3; i++)
				{
					int k_idx = lookup_kind(TV_ROD, i-EATER_EXT*2);
					p_ptr->magic_num1[i] -= ((p_ptr->magic_num2[i] < 10) ? EATER_ROD_CHARGE*3 : p_ptr->magic_num2[i]*EATER_ROD_CHARGE/3)*k_info[k_idx].pval;
					if (p_ptr->magic_num1[i] < 0) p_ptr->magic_num1[i] = 0;
				}
				msg_print(_("頭がハッキリとした。", "You feel your head clear."));
				p_ptr->window |= (PW_PLAYER);
			}
			else if (p_ptr->csp < p_ptr->msp)
			{
				p_ptr->csp = p_ptr->msp;
				p_ptr->csp_frac = 0;
				msg_print(_("頭がハッキリとした。", "You feel your head clear."));
				p_ptr->redraw |= (PR_MANA);
				p_ptr->window |= (PW_PLAYER);
				p_ptr->window |= (PW_SPELL);
			}
			break;
		}

		/* Activate for timed effect */

		case ACT_ESP:
		{
			(void)set_tim_esp(randint1(30) + 25, FALSE);
			break;
		}

		case ACT_BERSERK:
		{
			(void)set_afraid(0);
			(void)set_shero(randint1(25) + 25, FALSE);
			/* (void)set_afraid(0);
			(void)set_hero(randint1(50) + 50, FALSE);
			(void)set_blessed(randint1(50) + 50, FALSE);
			o_ptr->timeout = 100 + randint1(100); */
			break;
		}

		case ACT_PROT_EVIL:
		{
			msg_format(_("%sから鋭い音が流れ出た...", "The %s lets out a shrill wail..."), name);
			k = 3 * p_ptr->lev;
			(void)set_protevil(randint1(25) + k, FALSE);
			break;
		}

		case ACT_RESIST_ALL:
		{
			msg_print(_("様々な色に輝いている...", "It glows many colours..."));
			(void)set_oppose_acid(randint1(40) + 40, FALSE);
			(void)set_oppose_elec(randint1(40) + 40, FALSE);
			(void)set_oppose_fire(randint1(40) + 40, FALSE);
			(void)set_oppose_cold(randint1(40) + 40, FALSE);
			(void)set_oppose_pois(randint1(40) + 40, FALSE);
			break;
		}

		case ACT_SPEED:
		{
			msg_print(_("明るく緑色に輝いている...", "It glows bright green..."));
			(void)set_fast(randint1(20) + 20, FALSE);
			break;
		}

		case ACT_XTRA_SPEED:
		{
			msg_print(_("明るく輝いている...", "It glows brightly..."));
			(void)set_fast(randint1(75) + 75, FALSE);
			break;
		}

		case ACT_WRAITH:
		{
			set_wraith_form(randint1(plev / 2) + (plev / 2), FALSE);
			break;
		}

		case ACT_INVULN:
		{
			(void)set_invuln(randint1(8) + 8, FALSE);
			break;
		}

		case ACT_HELO:
		{
			(void)set_afraid(0);
			set_hero(randint1(25)+25, FALSE);
			hp_player(10);
			break;
		}

		case ACT_HELO_SPEED:
		{
			(void)set_fast(randint1(50) + 50, FALSE);
			hp_player(10);
			set_afraid(0);
			set_hero(randint1(50) + 50, FALSE);
			break;
		}

		case ACT_RESIST_ACID:
		{
			msg_format(_("%sが黒く輝いた...", "The %s grows black."), name);
			if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ACID))
			{
				if (!get_aim_dir(&dir)) return FALSE;
				fire_ball(GF_ACID, dir, 100, 2);
			}
			(void)set_oppose_acid(randint1(20) + 20, FALSE);
			break;
		}

		case ACT_RESIST_FIRE:
		{
			msg_format(_("%sが赤く輝いた...","The %s grows red."), name);
			if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_FLAMES))
			{
				if (!get_aim_dir(&dir)) return FALSE;
				fire_ball(GF_FIRE, dir, 100, 2);
			}
			(void)set_oppose_fire(randint1(20) + 20, FALSE);
			break;
		}

		case ACT_RESIST_COLD:
		{
			msg_format(_("%sが白く輝いた...","The %s grows white.") , name);
			if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ICE))
			{
				if (!get_aim_dir(&dir)) return FALSE;
				fire_ball(GF_COLD, dir, 100, 2);
			}
			(void)set_oppose_cold(randint1(20) + 20, FALSE);
			break;
		}

		case ACT_RESIST_ELEC:
		{
			msg_format(_("%sが青く輝いた...", "The %s grows blue."), name);
			if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ELEC))
			{
				if (!get_aim_dir(&dir)) return FALSE;
				fire_ball(GF_ELEC, dir, 100, 2);
			}
			(void)set_oppose_elec(randint1(20) + 20, FALSE);
			break;
		}

		case ACT_RESIST_POIS:
		{
			msg_format(_("%sが緑に輝いた...", "The %s grows green."), name);
			(void)set_oppose_pois(randint1(20) + 20, FALSE);
			break;
		}

		/* Activate for general purpose effect (detection etc.) */

		case ACT_LIGHT:
		{
			msg_format(_("%sから澄んだ光があふれ出た...", "The %s wells with clear light..."), name);
			lite_area(damroll(2, 15), 3);
			break;
		}

		case ACT_MAP_LIGHT:
		{
			msg_print(_("眩しく輝いた...", "It shines brightly..."));
			map_area(DETECT_RAD_MAP);
			lite_area(damroll(2, 15), 3);
			break;
		}

		case ACT_DETECT_ALL:
		{
			msg_print(_("白く明るく輝いている...", "It glows bright white..."));
			msg_print(_("心にイメージが浮かんできた...", "An image forms in your mind..."));
			detect_all(DETECT_RAD_DEFAULT);
			break;
		}

		case ACT_DETECT_XTRA:
		{
			msg_print(_("明るく輝いている...", "It glows brightly..."));
			detect_all(DETECT_RAD_DEFAULT);
			probing();
			identify_fully(FALSE);
			break;
		}

		case ACT_ID_FULL:
		{
			msg_print(_("黄色く輝いている...", "It glows yellow..."));
			identify_fully(FALSE);
			break;
		}

		case ACT_ID_PLAIN:
		{
			if (!ident_spell(FALSE)) return FALSE;
			break;
		}

		case ACT_RUNE_EXPLO:
		{
			msg_print(_("明るい赤色に輝いている...", "It glows bright red..."));
			explosive_rune();
			break;
		}

		case ACT_RUNE_PROT:
		{
			msg_print(_("ブルーに明るく輝いている...", "It glows light blue..."));
			warding_glyph();
			break;
		}

		case ACT_SATIATE:
		{
			(void)set_food(PY_FOOD_MAX - 1);
			break;
		}

		case ACT_DEST_DOOR:
		{
			msg_print(_("明るい赤色に輝いている...", "It glows bright red..."));
			destroy_doors_touch();
			break;
		}

		case ACT_STONE_MUD:
		{
			msg_print(_("鼓動している...", "It pulsates..."));
			if (!get_aim_dir(&dir)) return FALSE;
			wall_to_mud(dir, 20 + randint1(30));
			break;
		}

		case ACT_RECHARGE:
		{
			recharge(130);
			break;
		}

		case ACT_ALCHEMY:
		{
			msg_print(_("明るい黄色に輝いている...", "It glows bright yellow..."));
			(void)alchemy();
			break;
		}

		case ACT_DIM_DOOR:
		{
			msg_print(_("次元の扉が開いた。目的地を選んで下さい。", "You open a dimensional gate. Choose a destination."));
			if (!dimension_door()) return FALSE;
			break;
		}


		case ACT_TELEPORT:
		{
			msg_print(_("周りの空間が歪んでいる...", "It twists space around you..."));
			teleport_player(100, 0L);
			break;
		}

		case ACT_RECALL:
		{
			msg_print(_("やわらかな白色に輝いている...", "It glows soft white..."));
			if (!word_of_recall()) return FALSE;
			break;
		}

		case ACT_JUDGE:
		{
			msg_format(_("%sは赤く明るく光った！", "The %s flashes bright red!"), name);
			chg_virtue(V_KNOWLEDGE, 1);
			chg_virtue(V_ENLIGHTEN, 1);
			wiz_lite(FALSE);
			
			msg_format(_("%sはあなたの体力を奪った...", "The %s drains your vitality..."), name);
			take_hit(DAMAGE_LOSELIFE, damroll(3,8), _("審判の宝石", "the Jewel of Judgement"), -1);
			
			(void)detect_traps(DETECT_RAD_DEFAULT, TRUE);
			(void)detect_doors(DETECT_RAD_DEFAULT);
			(void)detect_stairs(DETECT_RAD_DEFAULT);
			
			if (get_check(_("帰還の力を使いますか？", "Activate recall? ")))
			{
				(void)word_of_recall();
			}

			break;
		}

		case ACT_TELEKINESIS:
		{
			if (!get_aim_dir(&dir)) return FALSE;
			msg_format(_("%sを伸ばした。", "You stretched your %s."), name);
			fetch(dir, 500, TRUE);
			break;
		}

		case ACT_DETECT_UNIQUE:
		{
			int i;
			monster_type *m_ptr;
			monster_race *r_ptr;
			msg_print(_("奇妙な場所が頭の中に浮かんだ．．．", "Some strange places show up in your mind. And you see ..."));
			/* Process the monsters (backwards) */
			for (i = m_max - 1; i >= 1; i--)
			{
				/* Access the monster */
				m_ptr = &m_list[i];

				/* Ignore "dead" monsters */
				if (!m_ptr->r_idx) continue;

				r_ptr = &r_info[m_ptr->r_idx];

				if(r_ptr->flags1 & RF1_UNIQUE)
				{
					msg_format(_("%s． ", "%s. "),r_name + r_ptr->name);
				}
			}
			break;
		}

		case ACT_ESCAPE:
		{
			switch (randint1(13))
			{
			case 1: case 2: case 3: case 4: case 5:
				teleport_player(10, 0L);
				break;
			case 6: case 7: case 8: case 9: case 10:
				teleport_player(222, 0L);
				break;
			case 11: case 12:
				(void)stair_creation();
				break;
			default:
				if (get_check(_("この階を去りますか？", "Leave this level? ")))
				{
					if (autosave_l) do_cmd_save_game(TRUE);

					/* Leaving */
					p_ptr->leaving = TRUE;
				}
			}
			break;
		}

		case ACT_DISP_CURSE_XTRA:
		{
			msg_format(_("%sが真実を照らし出す...", "The %s exhibits the truth..."), name);
			if (remove_all_curse())
			{
				msg_print(_("誰かに見守られているような気がする。", "You feel as if someone is watching over you."));
			}
			(void)probing();
			break;
		}

		case ACT_BRAND_FIRE_BOLTS:
		{
			msg_format(_("%sが深紅に輝いた...", "Your %s glows deep red..."), name);
			(void)brand_bolts();
			break;
		}

		case ACT_RECHARGE_XTRA:
		{
			msg_format(_("%sが白く輝いた．．．", "The %s gleams with blinding light..."), name);
			if (!recharge(1000)) return FALSE;
			break;
		}

		case ACT_LORE:
		{
			msg_print(_("石が隠された秘密を写し出した．．．", "The stone reveals hidden mysteries..."));
			if (!ident_spell(FALSE)) return FALSE;

			if (mp_ptr->spell_book)
			{
				/* Sufficient mana */
				if (20 <= p_ptr->csp)
				{
					/* Use some mana */
					p_ptr->csp -= 20;
				}

				/* Over-exert the player */
				else
				{
					int oops = 20 - p_ptr->csp;

					/* No mana left */
					p_ptr->csp = 0;
					p_ptr->csp_frac = 0;

					/* Message */
					msg_print(_("石を制御できない！", "You are too weak to control the stone!"));
					/* Hack -- Bypass free action */
					(void)set_paralyzed(p_ptr->paralyzed +
						randint1(5 * oops + 1));

					/* Confusing. */
					(void)set_confused(p_ptr->confused +
						randint1(5 * oops + 1));
				}

				/* Redraw mana */
				p_ptr->redraw |= (PR_MANA);
			}
			take_hit(DAMAGE_LOSELIFE, damroll(1, 12), _("危険な秘密", "perilous secrets"), -1);
			/* Confusing. */
			if (one_in_(5)) (void)set_confused(p_ptr->confused +
				randint1(10));

			/* Exercise a little care... */
			if (one_in_(20))
				take_hit(DAMAGE_LOSELIFE, damroll(4, 10), _("危険な秘密", "perilous secrets"), -1);
			break;
		}

		case ACT_SHIKOFUMI:
		{
			msg_print(_("力強く四股を踏んだ。", "You stamp. (as if you are in a ring.)"));
			(void)set_afraid(0);
			(void)set_hero(randint1(20) + 20, FALSE);
			dispel_evil(p_ptr->lev * 3);
			break;
		}

		case ACT_PHASE_DOOR:
		{
			teleport_player(10, 0L);
			break;
		}

		case ACT_DETECT_ALL_MONS:
		{
			(void)detect_monsters_invis(255);
			(void)detect_monsters_normal(255);
			break;
		}

		case ACT_ULTIMATE_RESIST:
		{
			int v = randint1(25)+25;
			(void)set_afraid(0);
			(void)set_hero(v, FALSE);
			(void)hp_player(10);
			(void)set_blessed(v, FALSE);
			(void)set_oppose_acid(v, FALSE);
			(void)set_oppose_elec(v, FALSE);
			(void)set_oppose_fire(v, FALSE);
			(void)set_oppose_cold(v, FALSE);
			(void)set_oppose_pois(v, FALSE);
			(void)set_ultimate_res(v, FALSE);
			break;
		}


		/* Unique activation */
		case ACT_CAST_OFF:
		{
			int inv, o_idx, t;
			char o_name[MAX_NLEN];
			object_type forge;

			/* Cast off activated item */
			for (inv = INVEN_RARM; inv <= INVEN_FEET; inv++)
			{
				if (o_ptr == &inventory[inv]) break;
			}

			/* Paranoia */
			if (inv > INVEN_FEET) return FALSE;

			object_copy(&forge, o_ptr);
			inven_item_increase(inv, (0 - o_ptr->number));
			inven_item_optimize(inv);
			o_idx = drop_near(&forge, 0, py, px);
			o_ptr = &o_list[o_idx];

			object_desc(o_name, o_ptr, OD_NAME_ONLY);
			msg_format(_("%sを脱ぎ捨てた。", "You cast off %s."), o_name);

			/* Get effects */
			msg_print(_("「燃え上がれ俺の小宇宙！」", "You say, 'Burn up my cosmo!"));
			t = 20 + randint1(20);
			(void)set_blind(p_ptr->blind + t);
			(void)set_afraid(0);
			(void)set_tim_esp(p_ptr->tim_esp + t, FALSE);
			(void)set_tim_regen(p_ptr->tim_regen + t, FALSE);
			(void)set_hero(p_ptr->hero + t, FALSE);
			(void)set_blessed(p_ptr->blessed + t, FALSE);
			(void)set_fast(p_ptr->fast + t, FALSE);
			(void)set_shero(p_ptr->shero + t, FALSE);
			if (p_ptr->pclass == CLASS_FORCETRAINER)
			{
				p_ptr->magic_num1[0] = plev * 5 + 190;
				msg_print(_("気が爆発寸前になった。", "Your force are immediatly before explosion."));
			}

			break;
		}

		case ACT_FALLING_STAR:
		{
			msg_print(_("あなたは妖刀に魅入られた…", "You are enchanted by cursed blade..."));
			msg_print(_("「狂ほしく 血のごとき 月はのぼれり 秘めおきし 魔剣 いずこぞや」", "'Behold the blade arts.'"));
			massacre(py, px);
			break;
		}

		case ACT_GRAND_CROSS:
		{
			msg_print(_("「闇に還れ！」", "You say, 'Return to darkness!'"));
			project(0, 8, py, px, (randint1(100) + 200) * 2, GF_HOLY_FIRE, PROJECT_KILL | PROJECT_ITEM | PROJECT_GRID, -1);
			break;
		}

		case ACT_TELEPORT_LEVEL:
		{
			if (!get_check(_("本当に他の階にテレポートしますか？", "Are you sure? (Teleport Level)"))) return FALSE;
			teleport_level(0);
			break;
		}

		case ACT_STRAIN_HASTE:
		{
			int t;
			msg_format(_("%sはあなたの体力を奪った...", "The %s drains your vitality..."), name);
			take_hit(DAMAGE_LOSELIFE, damroll(3, 8), _("加速した疲労", "the strain of haste"), -1);
			t = 25 + randint1(25);
			(void)set_fast(p_ptr->fast + t, FALSE);
			break;
		}

		case ACT_FISHING:
		{
			int x, y;

			if (!get_rep_dir2(&dir)) return FALSE;
			y = py+ddy[dir];
			x = px+ddx[dir];
			tsuri_dir = dir;
			if (!cave_have_flag_bold(y, x, FF_WATER))
			{
				msg_print(_("そこは水辺ではない。", "There is no fishing place."));
				return FALSE;
			}
			else if (cave[y][x].m_idx)
			{
				char m_name[80];
				monster_desc(m_name, &m_list[cave[y][x].m_idx], 0);
				msg_format(_("%sが邪魔だ！", "%^s is stand in your way."), m_name);
				energy_use = 0;
				return FALSE;
			}
			set_action(ACTION_FISH);
			p_ptr->redraw |= (PR_STATE);
			break;
		}

		case ACT_INROU:
		{
			int count = 0, i;
			monster_type *m_ptr;
			cptr kakusan = "";
			
			if (summon_named_creature(0, py, px, MON_SUKE, PM_FORCE_PET))
			{
				msg_print(_("『助さん』が現れた。", "Suke-san apperars."));
				kakusan = "Suke-san";
				count++;
			}
			if (summon_named_creature(0, py, px, MON_KAKU, PM_FORCE_PET))
			{
				msg_print(_("『格さん』が現れた。", "Kaku-san appears."));
				kakusan = "Kaku-san";
				count++;
			}
			if (!count)
			{
				for (i = m_max - 1; i > 0; i--)
				{
					m_ptr = &m_list[i];
					if (!m_ptr->r_idx) continue;
					if (!((m_ptr->r_idx == MON_SUKE) || (m_ptr->r_idx == MON_KAKU))) continue;
					if (!los(m_ptr->fy, m_ptr->fx, py, px)) continue;
					if (!projectable(m_ptr->fy, m_ptr->fx, py, px)) continue;
					count++;
					break;
				}
			}

			if (count)
			{
				msg_format(_("「者ども、ひかえおろう！！！このお方をどなたとこころえる。」", 
							"%^s says 'WHO do you think this person is! Bow your head, down your knees!'"), kakusan);
				sukekaku = TRUE;
				stun_monsters(120);
				confuse_monsters(120);
				turn_monsters(120);
				stasis_monsters(120);
				sukekaku = FALSE;
			}
			else
			{
				msg_print(_("しかし、何も起きなかった。", "Nothing happen."));
			}
			break;
		}

		case ACT_MURAMASA:
		{
			/* Only for Muramasa */
			if (o_ptr->name1 != ART_MURAMASA) return FALSE;
			if (get_check(_("本当に使いますか？", "Are you sure?!")))
			{
				msg_print(_("村正が震えた．．．", "The Muramasa pulsates..."));
				do_inc_stat(A_STR);
				if (one_in_(2))
				{
					msg_print(_("村正は壊れた！", "The Muramasa is destroyed!"));
					curse_weapon_object(TRUE, o_ptr);
				}
			}
			break;
		}

		case ACT_BLOODY_MOON:
		{
			/* Only for Bloody Moon */
			if (o_ptr->name1 != ART_BLOOD) return FALSE;
			msg_print(_("鎌が明るく輝いた...", "Your scythe glows brightly!"));
			get_bloody_moon_flags(o_ptr);
			if (p_ptr->prace == RACE_ANDROID) calc_android_exp();
			p_ptr->update |= (PU_BONUS | PU_HP);
			break;
		}

		case ACT_CRIMSON:
		{
			int num = 1;
			int i;
			int flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
			int tx, ty;

			/* Only for Crimson */
			if (o_ptr->name1 != ART_CRIMSON) return FALSE;

			msg_print(_("せっかくだから『クリムゾン』をぶっぱなすぜ！", "I'll fire CRIMSON! SEKKAKUDAKARA!"));

			if (!get_aim_dir(&dir)) return FALSE;

			/* Use the given direction */
			tx = px + 99 * ddx[dir];
			ty = py + 99 * ddy[dir];

			/* Hack -- Use an actual "target" */
			if ((dir == 5) && target_okay())
			{
				tx = target_col;
				ty = target_row;
			}

			if (p_ptr->pclass == CLASS_ARCHER)
			{
				/* Extra shot at level 10 */
				if (p_ptr->lev >= 10) num++;

				/* Extra shot at level 30 */
				if (p_ptr->lev >= 30) num++;

				/* Extra shot at level 45 */
				if (p_ptr->lev >= 45) num++;
			}

			for (i = 0; i < num; i++)
				project(0, p_ptr->lev/20+1, ty, tx, p_ptr->lev*p_ptr->lev*6/50, GF_ROCKET, flg, -1);
			break;
		}

		default:
		{
			msg_format(_("Unknown activation effect: %d.", "Unknown activation effect: %d."), act_ptr->index);
			return FALSE;
		}
	}

	/* Set activation timeout */
	if (act_ptr->timeout.constant >= 0) {
		o_ptr->timeout = act_ptr->timeout.constant;
		if (act_ptr->timeout.dice > 0) {
			o_ptr->timeout += randint1(act_ptr->timeout.dice);
		}
	} else {
		/* Activations that have special timeout */
		switch (act_ptr->index) {
		case ACT_BR_FIRE:
			o_ptr->timeout = ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_FLAMES)) ? 200 : 250;
			break;
		case ACT_BR_COLD:
			o_ptr->timeout = ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ICE)) ? 200 : 250;
			break;
		case ACT_TERROR:
			o_ptr->timeout = 3 * (p_ptr->lev + 10);
			break;
		case ACT_MURAMASA:
			/* Nothing to do */
			break;
		default:
			msg_format("Special timeout is not implemented: %d.", act_ptr->index);
			return FALSE;
		}
	}

	return TRUE;
}

/*!
 * @brief 固定アーティファクト『ブラッディムーン』の特性を変更する。
 * @details スレイ2d2種、及びone_resistance()による耐性1d2種、pval2種を得る。
 * @param o_ptr 対象のオブジェクト構造体（ブラッディムーン）のポインタ
 * @return なし
 */
void get_bloody_moon_flags(object_type *o_ptr)
{
	int dummy, i;

	for (i = 0; i < TR_FLAG_SIZE; i++)
		o_ptr->art_flags[i] = a_info[ART_BLOOD].flags[i];

	dummy = randint1(2) + randint1(2);
	for (i = 0; i < dummy; i++)
	{
		int flag = randint0(26);
		if (flag >= 20) add_flag(o_ptr->art_flags, TR_KILL_UNDEAD + flag - 20);
		else if (flag == 19) add_flag(o_ptr->art_flags, TR_KILL_ANIMAL);
		else if (flag == 18) add_flag(o_ptr->art_flags, TR_SLAY_HUMAN);
		else add_flag(o_ptr->art_flags, TR_CHAOTIC + flag);
	}

	dummy = randint1(2);
	for (i = 0; i < dummy; i++) one_resistance(o_ptr);

	for (i = 0; i < 2; i++)
	{
		int tmp = randint0(11);
		if (tmp < 6) add_flag(o_ptr->art_flags, TR_STR + tmp);
		else add_flag(o_ptr->art_flags, TR_STEALTH + tmp - 6);
	}
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
bool create_named_art(int a_idx, int y, int x)
{
	object_type forge;
	object_type *q_ptr;
	int i;

	artifact_type *a_ptr = &a_info[a_idx];

	/* Get local object */
	q_ptr = &forge;

	/* Ignore "empty" artifacts */
	if (!a_ptr->name) return FALSE;

	/* Acquire the "kind" index */
	i = lookup_kind(a_ptr->tval, a_ptr->sval);

	/* Oops */
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
