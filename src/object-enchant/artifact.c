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

#include "object-enchant/artifact.h"
#include "art-definition/art-armor-types.h"
#include "art-definition/art-protector-types.h"
#include "art-definition/art-sword-types.h"
#include "art-definition/art-weapon-types.h"
#include "art-definition/random-art-effects.h"
#include "cmd-item/cmd-smith.h"
#include "core/asking-player.h"
#include "floor/floor-object.h"
#include "floor/floor.h"
#include "game-option/cheat-types.h"
#include "grid/grid.h"
#include "io/files-util.h"
#include "perception/identification.h"
#include "perception/object-perception.h"
#include "object-enchant/object-boost.h"
#include "object-enchant/object-curse.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/random-art-bias-types.h"
#include "object-enchant/trg-types.h"
#include "object/object-flags.h"
#include "object/object-flavor.h"
#include "object/object-generator.h"
#include "object/object-hook.h"
#include "object/object-kind-hook.h"
#include "object/object-kind.h"
#include "object/object-value-calc.h"
#include "object-enchant/special-object-flags.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/system-variables.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "player/avatar.h"
#include "player/player-class.h"
#include "player/player-personalities-types.h"
#include "spell/spells-object.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
#include "view/display-main-window.h"
#include "view/display-messages.h"
#include "world/world.h"

 /*
  * The artifact arrays
  */
artifact_type *a_info;
char *a_name;
char *a_text;

/*
 * Maximum number of artifacts in a_info.txt
 */
ARTIFACT_IDX max_a_idx;

static bool has_extreme_damage_rate(object_type *o_ptr);
static bool weakening_artifact(object_type *o_ptr);

#ifdef JP
/*!
 * @brief ランダムアーティファクトのバイアス名称テーブル
 */
const concptr artifact_bias_name[MAX_BIAS] =
{
	"なし",
	"電撃",
	"毒",
	"火炎",
	"冷気",
	"酸",
	"腕力",
	"知力",
	"賢さ",
	"器用さ",
	"耐久",
	"魅力",
	"混沌",
	"プリースト",
	"死霊",
	"法",
	"盗賊",
	"メイジ",
	"戦士",
	"レンジャー",
};
#else
const concptr artifact_bias_name[MAX_BIAS] =
{
	"None",
	"Elec",
	"Poison",
	"Fire",
	"Cold",
	"Acid",
	"STR",
	"INT",
	"WIS",
	"DEX",
	"CON",
	"CHA",
	"Chaos",
	"Pristly",
	"Necromantic",
	"Law",
	"Rogue",
	"Mage",
	"Warrior",
	"Ranger",
};
#endif


/*!
 * @brief ランダムアーティファクト生成中、対象のオブジェクトを呪いのアーティファクトにする経過処理。/ generation process of cursed artifact.
 * @details pval、AC、命中、ダメージが正の場合、符号反転の上1d4だけ悪化させ、重い呪い、呪いフラグを必ず付加。
 * 祝福を無効。確率に応じて、永遠の呪い、太古の怨念、経験値吸収、弱い呪いの継続的付加、強い呪いの継続的付加、HP吸収の呪い、
 * MP吸収の呪い、乱テレポート、反テレポート、反魔法をつける。
 * @attention プレイヤーの職業依存処理あり。
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return なし
 */
static void curse_artifact(player_type *player_ptr, object_type *o_ptr)
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

	if ((player_ptr->pclass != CLASS_WARRIOR) && (player_ptr->pclass != CLASS_ARCHER) && (player_ptr->pclass != CLASS_CAVALRY) && (player_ptr->pclass != CLASS_BERSERKER) && (player_ptr->pclass != CLASS_SMITH) && one_in_(3))
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
static void random_plus(object_type *o_ptr)
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
static void random_resistance(object_type *o_ptr)
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
static void random_misc(object_type *o_ptr)
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

		while (n--)
		{
			switch (idx[n])
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
		if ((o_ptr->tval == TV_SWORD || o_ptr->tval == TV_POLEARM) &&
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
	o_ptr->xtra2 = (byte)type;
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
		return;
	}

	if (prob <= TABLE_NAME)
	{
		get_table_name(return_name);
		return;
	}

	concptr filename;
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


/*!
 * @brief ランダムアーティファクト生成のメインルーチン
 * @details 既に生成が済んでいるオブジェクトの構造体を、アーティファクトとして強化する。
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @param a_scroll アーティファクト生成の巻物上の処理。呪いのアーティファクトが生成対象外となる。
 * @return 常にTRUE(1)を返す
 */
bool become_random_artifact(player_type *player_ptr, object_type *o_ptr, bool a_scroll)
{
	/* Reset artifact bias */
	o_ptr->artifact_bias = 0;

	/* Nuke enchantments */
	o_ptr->name1 = 0;
	o_ptr->name2 = 0;

	for (int i = 0; i < TR_FLAG_SIZE; i++)
		o_ptr->art_flags[i] |= k_info[o_ptr->k_idx].flags[i];

	bool has_pval = FALSE;
	if (o_ptr->pval) has_pval = TRUE;

	int warrior_artifact_bias = 0;
	if (a_scroll && one_in_(4))
	{
		switch (player_ptr->pclass)
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

	GAME_TEXT new_name[1024];
	strcpy(new_name, "");

	bool a_cursed = FALSE;
	if (!a_scroll && one_in_(A_CURSED))
		a_cursed = TRUE;
	if (((o_ptr->tval == TV_AMULET) || (o_ptr->tval == TV_RING)) && object_is_cursed(o_ptr))
		a_cursed = TRUE;

	int powers = randint1(5) + 1;
	while (one_in_(powers) || one_in_(7) || one_in_(10))
		powers++;

	if (!a_cursed && one_in_(WEIRD_LUCK))
		powers *= 2;

	if (a_cursed) powers /= 2;

	int max_powers = powers;
	int max_type = object_is_weapon_ammo(o_ptr) ? 7 : 5;
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
					if (one_in_(o_ptr->ds + 4)) o_ptr->ds++;
				}
				else
				{
					if (one_in_(o_ptr->dd + 1)) o_ptr->dd++;
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
			if (current_world_ptr->wizard) msg_print("Switch error in become_random_artifact!");
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
			} while (o_ptr->pval < randint1(5) || one_in_(o_ptr->pval));
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

	s32b total_flags = flag_cost(o_ptr, o_ptr->pval);

	if (a_cursed) curse_artifact(player_ptr, o_ptr);

	if (!a_cursed &&
		one_in_(object_is_armour(o_ptr) ? ACTIVATION_CHANCE * 2 : ACTIVATION_CHANCE))
	{
		o_ptr->xtra2 = 0;
		give_activation_power(o_ptr);
	}

	if (object_is_armour(o_ptr))
	{
		while ((o_ptr->to_d + o_ptr->to_h) > 20)
		{
			if (one_in_(o_ptr->to_d) && one_in_(o_ptr->to_h)) break;
			o_ptr->to_d -= (HIT_POINT)randint0(3);
			o_ptr->to_h -= (HIT_PROB)randint0(3);
		}
		while ((o_ptr->to_d + o_ptr->to_h) > 10)
		{
			if (one_in_(o_ptr->to_d) || one_in_(o_ptr->to_h)) break;
			o_ptr->to_d -= (HIT_POINT)randint0(3);
			o_ptr->to_h -= (HIT_PROB)randint0(3);
		}
	}

	if (((o_ptr->artifact_bias == BIAS_MAGE) || (o_ptr->artifact_bias == BIAS_INT)) && (o_ptr->tval == TV_GLOVES)) add_flag(o_ptr->art_flags, TR_FREE_ACT);

	if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_POISON_NEEDLE))
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

	int power_level;
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
	while (has_extreme_damage_rate(o_ptr) && !one_in_(SWORDFISH_LUCK))
	{
		weakening_artifact(o_ptr);
	}

	if (a_scroll)
	{
		GAME_TEXT dummy_name[MAX_NLEN] = "";
		concptr ask_msg = _("このアーティファクトを何と名付けますか？", "What do you want to call the artifact? ");

		object_aware(player_ptr, o_ptr);
		object_known(o_ptr);

		/* Mark the item as fully known */
		o_ptr->ident |= (IDENT_FULL_KNOWN);

		/* For being treated as random artifact in screen_object() */
		o_ptr->art_name = quark_add("");

		(void)screen_object(player_ptr, o_ptr, 0L);

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
		chg_virtue(player_ptr, V_INDIVIDUALISM, 2);
		chg_virtue(player_ptr, V_ENCHANT, 5);
	}
	else
	{
		get_random_name(o_ptr, new_name, object_is_armour(o_ptr), power_level);
	}

	/* Save the inscription */
	o_ptr->art_name = quark_add(new_name);

	msg_format_wizard(CHEAT_OBJECT, _("パワー %d で 価値%ld のランダムアーティファクト生成 バイアスは「%s」",
		"Random artifact generated - Power:%d Value:%d Bias:%s."), max_powers, total_flags, artifact_bias_name[o_ptr->artifact_bias]);

	player_ptr->window |= (PW_INVEN | PW_EQUIP);

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
	for (p = activation_info; p->flag != NULL; ++p)
	{
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
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @param a_ptr 生成する固定アーティファクト構造体ポインタ
 * @return なし
 */
void random_artifact_resistance(player_type *player_ptr, object_type *o_ptr, artifact_type *a_ptr)
{
	bool give_resistance = FALSE, give_power = FALSE;

	if (o_ptr->name1 == ART_TERROR) /* Terror Mask is for warriors... */
	{
		bool is_special_class = player_ptr->pclass == CLASS_WARRIOR;
		is_special_class |= player_ptr->pclass == CLASS_ARCHER;
		is_special_class |= player_ptr->pclass == CLASS_CAVALRY;
		is_special_class |= player_ptr->pclass == CLASS_BERSERKER;
		if (is_special_class)
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
		if (player_ptr->pclass != CLASS_SAMURAI)
		{
			add_flag(o_ptr->art_flags, TR_NO_MAGIC);
			o_ptr->curse_flags |= (TRC_HEAVY_CURSE);
		}
	}

	if (o_ptr->name1 == ART_ROBINTON)
	{
		if (player_ptr->pclass == CLASS_BARD)
		{
			add_flag(o_ptr->art_flags, TR_DEC_MANA);
		}
	}

	if (o_ptr->name1 == ART_XIAOLONG)
	{
		if (player_ptr->pclass == CLASS_MONK)
			add_flag(o_ptr->art_flags, TR_BLOWS);
	}

	if (o_ptr->name1 == ART_BLOOD)
	{
		get_bloody_moon_flags(o_ptr);
	}

	if (o_ptr->name1 == ART_HEAVENLY_MAIDEN)
	{
		if (player_ptr->psex != SEX_FEMALE)
		{
			add_flag(o_ptr->art_flags, TR_AGGRAVATE);
		}
	}

	if (o_ptr->name1 == ART_MILIM)
	{
		if (player_ptr->pseikaku == PERSONALITY_SEXY)
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
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param a_idx 生成する固定アーティファクト構造体のID
 * @param y アイテムを落とす地点のy座標
 * @param x アイテムを落とす地点のx座標
 * @return 生成が成功したか否か、失敗はIDの不全、ベースアイテムの不全、drop_item()の失敗時に起こる。
 * @attention この処理はdrop_near()内で普通の固定アーティファクトが重ならない性質に依存する.
 * 仮に2個以上存在可能かつ装備品以外の固定アーティファクトが作成されれば
 * drop_near()関数の返り値は信用できなくなる.
 */
bool create_named_art(player_type *player_ptr, ARTIFACT_IDX a_idx, POSITION y, POSITION x)
{
	artifact_type *a_ptr = &a_info[a_idx];

	/* Ignore "empty" artifacts */
	if (!a_ptr->name) return FALSE;

	/* Acquire the "kind" index */
	KIND_OBJECT_IDX i = lookup_kind(a_ptr->tval, a_ptr->sval);
	if (i == 0) return FALSE;

	object_type forge;
	object_type *q_ptr;
	q_ptr = &forge;
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

	random_artifact_resistance(player_ptr, q_ptr, a_ptr);

	/* Drop the artifact from heaven */
	return drop_near(player_ptr, q_ptr, -1, y, x) ? TRUE : FALSE;
}


/*対邪平均ダメージの計算処理*/
HIT_POINT calc_arm_avgdamage(object_type *o_ptr)
{
	BIT_FLAGS flgs[TR_FLAG_SIZE];
	object_flags(o_ptr, flgs);

	HIT_POINT base, forced, vorpal;
	HIT_POINT s_evil = forced = vorpal = 0;
	HIT_POINT dam = base = (o_ptr->dd * o_ptr->ds + o_ptr->dd) / 2;

	if (have_flag(flgs, TR_KILL_EVIL))
	{
		dam = s_evil = dam * 7 / 2;
	}
	else if (!have_flag(flgs, TR_KILL_EVIL) && have_flag(flgs, TR_SLAY_EVIL))
	{
		dam = s_evil = dam * 2;
	}
	else s_evil = dam;

	if (have_flag(flgs, TR_FORCE_WEAPON))
	{
		dam = forced = dam * 3 / 2 + (o_ptr->dd * o_ptr->ds + o_ptr->dd);
	}
	else forced = dam;

	if (have_flag(flgs, TR_VORPAL))
	{
		dam = vorpal = dam * 11 / 9;
	}
	else vorpal = dam;

	dam = dam + o_ptr->to_d;

	msg_format_wizard(CHEAT_OBJECT, "素:%d> 対邪:%d> 理力:%d> 切:%d> 最終:%d", base, s_evil, forced, vorpal, dam);

	return dam;
}


static bool has_extreme_damage_rate(object_type *o_ptr)
{
	BIT_FLAGS flgs[TR_FLAG_SIZE];
	object_flags(o_ptr, flgs);

	if (have_flag(flgs, TR_VAMPIRIC))
	{
		if (have_flag(flgs, TR_BLOWS) && (o_ptr->pval == 1) && (calc_arm_avgdamage(o_ptr) > 52))
		{
			return TRUE;
		}

		if (have_flag(flgs, TR_BLOWS) && (o_ptr->pval == 2) && (calc_arm_avgdamage(o_ptr) > 43))
		{
			return TRUE;
		}

		if (have_flag(flgs, TR_BLOWS) && (o_ptr->pval == 3) && (calc_arm_avgdamage(o_ptr) > 33))
		{
			return TRUE;
		}

		if (calc_arm_avgdamage(o_ptr) > 63)
		{
			return TRUE;
		}

		return FALSE;
	}

	if (have_flag(flgs, TR_BLOWS) && (o_ptr->pval == 1) && (calc_arm_avgdamage(o_ptr) > 65))
	{
		return TRUE;
	}

	if (have_flag(flgs, TR_BLOWS) && (o_ptr->pval == 2) && (calc_arm_avgdamage(o_ptr) > 52))
	{
		return TRUE;
	}

	if (have_flag(flgs, TR_BLOWS) && (o_ptr->pval == 3) && (calc_arm_avgdamage(o_ptr) > 40))
	{
		return TRUE;
	}

	if (calc_arm_avgdamage(o_ptr) > 75)
	{
		return TRUE;
	}

	return FALSE;
}


static bool weakening_artifact(object_type *o_ptr)
{
	KIND_OBJECT_IDX k_idx = lookup_kind(o_ptr->tval, o_ptr->sval);
	object_kind *k_ptr = &k_info[k_idx];
	BIT_FLAGS flgs[TR_FLAG_SIZE];
	object_flags(o_ptr, flgs);

	if (have_flag(flgs, TR_KILL_EVIL))
	{
		remove_flag(o_ptr->art_flags, TR_KILL_EVIL);
		add_flag(o_ptr->art_flags, TR_SLAY_EVIL);
		return TRUE;
	}

	if (k_ptr->dd < o_ptr->dd)
	{
		o_ptr->dd--;
		return TRUE;
	}

	if (k_ptr->ds < o_ptr->ds)
	{
		o_ptr->ds--;
		return TRUE;
	}

	if (o_ptr->to_d > 10)
	{
		o_ptr->to_d = o_ptr->to_d - damroll(1, 6);
		if (o_ptr->to_d < 10)
		{
			o_ptr->to_d = 10;
		}

		return TRUE;
	}

	return FALSE;
}


/*!
 * @brief 非INSTA_ART型の固定アーティファクトの生成を確率に応じて試行する。
 * Mega-Hack -- Attempt to create one of the "Special Objects"
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 生成に割り当てたいオブジェクトの構造体参照ポインタ
 * @return 生成に成功したらTRUEを返す。
 * @details
 * Attempt to change an object into an artifact\n
 * This routine should only be called by "apply_magic()"\n
 * Note -- see "make_artifact_special()" and "apply_magic()"\n
 */
bool make_artifact(player_type *player_ptr, object_type *o_ptr)
{
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	if (floor_ptr->dun_level == 0) return FALSE;

	/* Paranoia -- no "plural" artifacts */
	if (o_ptr->number != 1) return FALSE;

	/* Check the artifact list (skip the "specials") */
	for (ARTIFACT_IDX i = 0; i < max_a_idx; i++)
	{
		artifact_type *a_ptr = &a_info[i];

		/* Skip "empty" items */
		if (!a_ptr->name) continue;

		/* Cannot make an artifact twice */
		if (a_ptr->cur_num) continue;

		if (a_ptr->gen_flags & TRG_QUESTITEM) continue;

		if (a_ptr->gen_flags & TRG_INSTA_ART) continue;

		/* Must have the correct fields */
		if (a_ptr->tval != o_ptr->tval) continue;
		if (a_ptr->sval != o_ptr->sval) continue;

		/* XXX XXX Enforce minimum "depth" (loosely) */
		if (a_ptr->level > floor_ptr->dun_level)
		{
			/* Acquire the "out-of-depth factor" */
			int d = (a_ptr->level - floor_ptr->dun_level) * 2;

			/* Roll for out-of-depth creation */
			if (!one_in_(d)) continue;
		}

		/* We must make the "rarity roll" */
		if (!one_in_(a_ptr->rarity)) continue;

		/* Hack -- mark the item as an artifact */
		o_ptr->name1 = i;

		/* Hack: Some artifacts get random extra powers */
		random_artifact_resistance(player_ptr, o_ptr, a_ptr);

		return TRUE;
	}

	return FALSE;
}

/*!
 * @brief INSTA_ART型の固定アーティファクトの生成を確率に応じて試行する。
 * Mega-Hack -- Attempt to create one of the "Special Objects"
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 生成に割り当てたいオブジェクトの構造体参照ポインタ
 * @return 生成に成功したらTRUEを返す。
 * @details
 * We are only called from "make_object()", and we assume that\n
 * "apply_magic()" is called immediately after we return.\n
 *\n
 * Note -- see "make_artifact()" and "apply_magic()"\n
 */
bool make_artifact_special(player_type *player_ptr, object_type *o_ptr)
{
	KIND_OBJECT_IDX k_idx = 0;

	/*! @note 地上ではキャンセルする / No artifacts in the town */
	floor_type *floor_ptr = player_ptr->current_floor_ptr;
	if (floor_ptr->dun_level == 0) return FALSE;

	/*! @note get_obj_num_hookによる指定がある場合は生成をキャンセルする / Themed object */
	if (get_obj_num_hook) return FALSE;

	/*! @note 全固定アーティファクト中からIDの若い順に生成対象とその確率を走査する / Check the artifact list (just the "specials") */
	for (ARTIFACT_IDX i = 0; i < max_a_idx; i++)
	{
		artifact_type *a_ptr = &a_info[i];

		/*! @note アーティファクト名が空の不正なデータは除外する / Skip "empty" artifacts */
		if (!a_ptr->name) continue;

		/*! @note 既に生成回数がカウントされたアーティファクト、QUESTITEMと非INSTA_ARTは除外 / Cannot make an artifact twice */
		if (a_ptr->cur_num) continue;
		if (a_ptr->gen_flags & TRG_QUESTITEM) continue;
		if (!(a_ptr->gen_flags & TRG_INSTA_ART)) continue;

		/*! @note アーティファクト生成階が現在に対して足りない場合は高確率で1/(不足階層*2)を満たさないと生成リストに加えられない /
		 *  XXX XXX Enforce minimum "depth" (loosely) */
		if (a_ptr->level > floor_ptr->object_level)
		{
			/* @note  / Acquire the "out-of-depth factor". Roll for out-of-depth creation. */
			int d = (a_ptr->level - floor_ptr->object_level) * 2;
			if (!one_in_(d)) continue;
		}

		/*! @note 1/(レア度)の確率を満たさないと除外される / Artifact "rarity roll" */
		if (!one_in_(a_ptr->rarity)) continue;

		/*! @note INSTA_ART型固定アーティファクトのベースアイテムもチェック対象とする。ベースアイテムの生成階層が足りない場合1/(不足階層*5) を満たさないと除外される。 /
		 *  Find the base object. XXX XXX Enforce minimum "object" level (loosely). Acquire the "out-of-depth factor". Roll for out-of-depth creation. */
		k_idx = lookup_kind(a_ptr->tval, a_ptr->sval);
		if (k_info[k_idx].level > floor_ptr->object_level)
		{
			int d = (k_info[k_idx].level - floor_ptr->object_level) * 5;
			if (!one_in_(d)) continue;
		}

		/*! @note 前述の条件を満たしたら、後のIDのアーティファクトはチェックせずすぐ確定し生成処理に移す /
		 * Assign the template. Mega-Hack -- mark the item as an artifact. Hack: Some artifacts get random extra powers. Success. */
		object_prep(o_ptr, k_idx);

		o_ptr->name1 = i;
		random_artifact_resistance(player_ptr, o_ptr, a_ptr);
		return TRUE;
	}

	/*! @note 全INSTA_ART固定アーティファクトを試行しても決まらなかった場合 FALSEを返す / Failure */
	return FALSE;
}
