/*!
 * @file object1.c
 * @brief オブジェクトの実装 / Object code, part 1
 * @date 2014/01/10
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 *\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen.\n
 */

#include "angband.h"
#include "util.h"

#include "artifact.h"
#include "floor.h"
#include "cmd-activate.h"
#include "objectkind.h"
#include "object-ego.h"
#include "object-flavor.h"
#include "object-hook.h"
#include "player-move.h"
#include "monster.h"
#include "files.h"

#if defined(MACINTOSH) || defined(MACH_O_CARBON)
#ifdef verify
#undef verify
#endif
#endif

/*!
 * @brief オブジェクト、地形の表示シンボルなど初期化する / Reset the "visual" lists
 * @return なし
 * This involves resetting various things to their "default" state.\n
 *\n
 * If the "prefs" flag is TRUE, then we will also load the appropriate\n
 * "user pref file" based on the current setting of the "use_graphics"\n
 * flag.  This is useful for switching "graphics" on/off.\n
 *\n
 * The features, objects, and monsters, should all be encoded in the\n
 * relevant "font.pref" and/or "graf.prf" files.  \n
 *\n
 * The "prefs" parameter is no longer meaningful.  \n
 */
void reset_visuals(void)
{
	int i, j;

	/* Extract some info about terrain features */
	for (i = 0; i < max_f_idx; i++)
	{
		feature_type *f_ptr = &f_info[i];

		/* Assume we will use the underlying values */
		for (j = 0; j < F_LIT_MAX; j++)
		{
			f_ptr->x_attr[j] = f_ptr->d_attr[j];
			f_ptr->x_char[j] = f_ptr->d_char[j];
		}
	}

	/* Extract default attr/char code for objects */
	for (i = 0; i < max_k_idx; i++)
	{
		object_kind *k_ptr = &k_info[i];

		/* Default attr/char */
		k_ptr->x_attr = k_ptr->d_attr;
		k_ptr->x_char = k_ptr->d_char;
	}

	/* Extract default attr/char code for monsters */
	for (i = 0; i < max_r_idx; i++)
	{
		monster_race *r_ptr = &r_info[i];

		/* Default attr/char */
		r_ptr->x_attr = r_ptr->d_attr;
		r_ptr->x_char = r_ptr->d_char;
	}

	if (use_graphics)
	{
		char buf[1024];

		/* Process "graf.prf" */
		process_pref_file("graf.prf");

		/* Access the "character" pref file */
		sprintf(buf, "graf-%s.prf", player_base);

		/* Process "graf-<playername>.prf" */
		process_pref_file(buf);
	}

	/* Normal symbols */
	else
	{
		char buf[1024];

		/* Process "font.prf" */
		process_pref_file("font.prf");

		/* Access the "character" pref file */
		sprintf(buf, "font-%s.prf", player_base);

		/* Process "font-<playername>.prf" */
		process_pref_file(buf);
	}
}

/*!
 * @brief オブジェクトのフラグ類を配列に与える
 * Obtain the "flags" for an item
 * @param o_ptr フラグ取得元のオブジェクト構造体ポインタ
 * @param flgs フラグ情報を受け取る配列
 * @return なし
 */
void object_flags(object_type *o_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE])
{
	object_kind *k_ptr = &k_info[o_ptr->k_idx];
	int i;

	/* Base object */
	for (i = 0; i < TR_FLAG_SIZE; i++)
		flgs[i] = k_ptr->flags[i];

	/* Artifact */
	if (object_is_fixed_artifact(o_ptr))
	{
		artifact_type *a_ptr = &a_info[o_ptr->name1];

		for (i = 0; i < TR_FLAG_SIZE; i++)
			flgs[i] = a_ptr->flags[i];
	}

	/* Ego-item */
	if (object_is_ego(o_ptr))
	{
		ego_item_type *e_ptr = &e_info[o_ptr->name2];

		for (i = 0; i < TR_FLAG_SIZE; i++)
			flgs[i] |= e_ptr->flags[i];

		if ((o_ptr->name2 == EGO_LITE_AURA_FIRE) && !o_ptr->xtra4 && (o_ptr->sval <= SV_LITE_LANTERN))
		{
			remove_flag(flgs, TR_SH_FIRE);
		}
		else if ((o_ptr->name2 == EGO_LITE_INFRA) && !o_ptr->xtra4 && (o_ptr->sval <= SV_LITE_LANTERN))
		{
			remove_flag(flgs, TR_INFRA);
		}
		else if ((o_ptr->name2 == EGO_LITE_EYE) && !o_ptr->xtra4 && (o_ptr->sval <= SV_LITE_LANTERN))
		{
			remove_flag(flgs, TR_RES_BLIND);
			remove_flag(flgs, TR_SEE_INVIS);
		}
	}

	/* Random artifact ! */
	for (i = 0; i < TR_FLAG_SIZE; i++)
		flgs[i] |= o_ptr->art_flags[i];

	if (object_is_smith(o_ptr))
	{
		int add = o_ptr->xtra3 - 1;

		if (add < TR_FLAG_MAX)
		{
			add_flag(flgs, add);
		}
		else if (add == ESSENCE_TMP_RES_ACID)
		{
			add_flag(flgs, TR_RES_ACID);
			add_flag(flgs, TR_ACTIVATE);
		}
		else if (add == ESSENCE_TMP_RES_ELEC)
		{
			add_flag(flgs, TR_RES_ELEC);
			add_flag(flgs, TR_ACTIVATE);
		}
		else if (add == ESSENCE_TMP_RES_FIRE)
		{
			add_flag(flgs, TR_RES_FIRE);
			add_flag(flgs, TR_ACTIVATE);
		}
		else if (add == ESSENCE_TMP_RES_COLD)
		{
			add_flag(flgs, TR_RES_COLD);
			add_flag(flgs, TR_ACTIVATE);
		}
		else if (add == ESSENCE_SH_FIRE)
		{
			add_flag(flgs, TR_RES_FIRE);
			add_flag(flgs, TR_SH_FIRE);
		}
		else if (add == ESSENCE_SH_ELEC)
		{
			add_flag(flgs, TR_RES_ELEC);
			add_flag(flgs, TR_SH_ELEC);
		}
		else if (add == ESSENCE_SH_COLD)
		{
			add_flag(flgs, TR_RES_COLD);
			add_flag(flgs, TR_SH_COLD);
		}
		else if (add == ESSENCE_RESISTANCE)
		{
			add_flag(flgs, TR_RES_ACID);
			add_flag(flgs, TR_RES_ELEC);
			add_flag(flgs, TR_RES_FIRE);
			add_flag(flgs, TR_RES_COLD);
		}
		else if (add == TR_IMPACT)
		{
			add_flag(flgs, TR_ACTIVATE);
		}
	}
}

/*!
 * @brief オブジェクトの明示されているフラグ類を取得する
 * Obtain the "flags" for an item which are known to the player
 * @param o_ptr フラグ取得元のオブジェクト構造体ポインタ
 * @param flgs フラグ情報を受け取る配列
 * @return なし
 */
void object_flags_known(object_type *o_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE])
{
	bool spoil = FALSE;
	int i;

	object_kind *k_ptr = &k_info[o_ptr->k_idx];

	/* Clear */
	for (i = 0; i < TR_FLAG_SIZE; i++)
		flgs[i] = 0;

	if (!object_is_aware(o_ptr)) return;

	/* Base object */
	for (i = 0; i < TR_FLAG_SIZE; i++)
		flgs[i] = k_ptr->flags[i];

	/* Must be identified */
	if (!object_is_known(o_ptr)) return;

	/* Ego-item (known basic flags) */
	if (object_is_ego(o_ptr))
	{
		ego_item_type *e_ptr = &e_info[o_ptr->name2];

		for (i = 0; i < TR_FLAG_SIZE; i++)
			flgs[i] |= e_ptr->flags[i];

		if ((o_ptr->name2 == EGO_LITE_AURA_FIRE) && !o_ptr->xtra4 && (o_ptr->sval <= SV_LITE_LANTERN))
		{
			remove_flag(flgs, TR_SH_FIRE);
		}
		else if ((o_ptr->name2 == EGO_LITE_INFRA) && !o_ptr->xtra4 && (o_ptr->sval <= SV_LITE_LANTERN))
		{
			remove_flag(flgs, TR_INFRA);
		}
		else if ((o_ptr->name2 == EGO_LITE_EYE) && !o_ptr->xtra4 && (o_ptr->sval <= SV_LITE_LANTERN))
		{
			remove_flag(flgs, TR_RES_BLIND);
			remove_flag(flgs, TR_SEE_INVIS);
		}
	}


#ifdef SPOIL_ARTIFACTS
	/* Full knowledge for some artifacts */
	if (object_is_artifact(o_ptr)) spoil = TRUE;
#endif /* SPOIL_ARTIFACTS */

#ifdef SPOIL_EGO_ITEMS
	/* Full knowledge for some ego-items */
	if (object_is_ego(o_ptr)) spoil = TRUE;
#endif /* SPOIL_EGO_ITEMS */

	/* Need full knowledge or spoilers */
	if (spoil || (o_ptr->ident & IDENT_MENTAL))
	{
		/* Artifact */
		if (object_is_fixed_artifact(o_ptr))
		{
			artifact_type *a_ptr = &a_info[o_ptr->name1];

			for (i = 0; i < TR_FLAG_SIZE; i++)
				flgs[i] = a_ptr->flags[i];
		}

		/* Random artifact ! */
		for (i = 0; i < TR_FLAG_SIZE; i++)
			flgs[i] |= o_ptr->art_flags[i];
	}

	if (object_is_smith(o_ptr))
	{
		int add = o_ptr->xtra3 - 1;

		if (add < TR_FLAG_MAX)
		{
			add_flag(flgs, add);
		}
		else if (add == ESSENCE_TMP_RES_ACID)
		{
			add_flag(flgs, TR_RES_ACID);
		}
		else if (add == ESSENCE_TMP_RES_ELEC)
		{
			add_flag(flgs, TR_RES_ELEC);
		}
		else if (add == ESSENCE_TMP_RES_FIRE)
		{
			add_flag(flgs, TR_RES_FIRE);
		}
		else if (add == ESSENCE_TMP_RES_COLD)
		{
			add_flag(flgs, TR_RES_COLD);
		}
		else if (add == ESSENCE_SH_FIRE)
		{
			add_flag(flgs, TR_RES_FIRE);
			add_flag(flgs, TR_SH_FIRE);
		}
		else if (add == ESSENCE_SH_ELEC)
		{
			add_flag(flgs, TR_RES_ELEC);
			add_flag(flgs, TR_SH_ELEC);
		}
		else if (add == ESSENCE_SH_COLD)
		{
			add_flag(flgs, TR_RES_COLD);
			add_flag(flgs, TR_SH_COLD);
		}
		else if (add == ESSENCE_RESISTANCE)
		{
			add_flag(flgs, TR_RES_ACID);
			add_flag(flgs, TR_RES_ELEC);
			add_flag(flgs, TR_RES_FIRE);
			add_flag(flgs, TR_RES_COLD);
		}
	}
}

/*!
 * @brief オブジェクトの発動効果名称を返す（サブルーチン/ブレス）
 * @param o_ptr 名称を取得する元のオブジェクト構造体参照ポインタ
 * @return concptr 発動名称を返す文字列ポインタ
 */
static concptr item_activation_dragon_breath(object_type *o_ptr)
{
	static char desc[256];
	BIT_FLAGS flgs[TR_FLAG_SIZE]; /* for resistance flags */
	int i, n = 0;

	object_flags(o_ptr, flgs);
	strcpy(desc, _("", "breath "));

	for (i = 0; dragonbreath_info[i].flag != 0; i++)
	{
		if (have_flag(flgs, dragonbreath_info[i].flag))
		{
			if (n > 0) strcat(desc, _("、", ", "));
			strcat(desc, dragonbreath_info[i].name);
			n++;
		}
	}

	strcat(desc, _("のブレス(250)", ""));

	return (desc);
}

/*!
 * @brief オブジェクトの発動効果名称を返す（サブルーチン/汎用）
 * @param o_ptr 名称を取得する元のオブジェクト構造体参照ポインタ
 * @return concptr 発動名称を返す文字列ポインタ
 */
static concptr item_activation_aux(object_type *o_ptr)
{
	static char activation_detail[256];
	concptr desc;
	char timeout[32];
	int constant, dice;
	const activation_type* const act_ptr = find_activation_info(o_ptr);

	if (!act_ptr) return _("未定義", "something undefined");

	desc = act_ptr->desc;

	/* Overwrite description if it is special */
	switch (act_ptr->index) {
	case ACT_BR_FIRE:
		if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_FLAMES))
			desc = _("火炎のブレス (200) と火への耐性", "breath of fire (200) and resist fire");
		break;
	case ACT_BR_COLD:
		if ((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ICE))
			desc = _("冷気のブレス (200) と冷気への耐性", "breath of cold (200) and resist cold");
		break;
	case ACT_BR_DRAGON:
		desc = item_activation_dragon_breath(o_ptr);
		break;
	case ACT_AGGRAVATE:
		if (o_ptr->name1 == ART_HYOUSIGI)
			desc = _("拍子木を打ちならす", "beat wooden clappers");
		break;
	case ACT_RESIST_ACID:
		if (((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ACID)) || (o_ptr->name2 == EGO_BRAND_ACID))
			desc = _("アシッド・ボール (100) と酸への耐性", "ball of acid (100) and resist acid");
		break;
	case ACT_RESIST_FIRE:
		if (((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_FLAMES)) || (o_ptr->name2 == EGO_BRAND_FIRE))
			desc = _("ファイア・ボール (100) と火への耐性", "ball of fire (100) and resist fire");
		break;
	case ACT_RESIST_COLD:
		if (((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ICE)) || (o_ptr->name2 == EGO_BRAND_COLD))
			desc = _("アイス・ボール (100) と冷気への耐性", "ball of cold (100) and resist cold");
		break;
	case ACT_RESIST_ELEC:
		if (((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ELEC)) || (o_ptr->name2 == EGO_BRAND_ELEC))
			desc = _("サンダー・ボール (100) と電撃への耐性", "ball of elec (100) and resist elec");
		break;
	case ACT_RESIST_POIS:
		if (o_ptr->name2 == EGO_BRAND_POIS)
			desc = _("悪臭雲 (100) と毒への耐性", "ball of poison (100) and resist elec");
		break;
	}

	/* Timeout description */
	constant = act_ptr->timeout.constant;
	dice = act_ptr->timeout.dice;
	if (constant == 0 && dice == 0) {
		/* We can activate it every current_world_ptr->game_turn */
		strcpy(timeout, _("いつでも", "every current_world_ptr->game_turn"));
	} else if (constant < 0) {
		/* Activations that have special timeout */
		switch (act_ptr->index) {
		case ACT_BR_FIRE:
			sprintf(timeout, _("%d ターン毎", "every %d turns"),
				((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_FLAMES)) ? 200 : 250);
			break;
		case ACT_BR_COLD:
			sprintf(timeout, _("%d ターン毎", "every %d turns"),
				((o_ptr->tval == TV_RING) && (o_ptr->sval == SV_RING_ICE)) ? 200 : 250);
			break;
		case ACT_TERROR:
			strcpy(timeout, _("3*(レベル+10) ターン毎", "every 3 * (level+10) turns"));
			break;
		case ACT_MURAMASA:
			strcpy(timeout, _("確率50%で壊れる", "(destroyed 50%)"));
			break;
		default:
			strcpy(timeout, "undefined");
			break;
		}
	} else {
		/* Normal timeout activations */
		char constant_str[16], dice_str[16];
		sprintf(constant_str, "%d", constant);
		sprintf(dice_str, "d%d", dice);
		sprintf(timeout, _("%s%s%s ターン毎", "every %s%s%s turns"),
			(constant > 0) ? constant_str : "",
			(constant > 0 && dice > 0) ? "+" : "",
			(dice > 0) ? dice_str : "");
	}

	/* Build detail activate description */
	sprintf(activation_detail, _("%s : %s", "%s %s"), desc, timeout);

	return activation_detail;
}

/*!
 * @brief オブジェクトの発動効果名称を返す（メインルーチン） /
 * Determine the "Activation" (if any) for an artifact Return a string, or NULL for "no activation"
 * @param o_ptr 名称を取得する元のオブジェクト構造体参照ポインタ
 * @return concptr 発動名称を返す文字列ポインタ
 */
concptr item_activation(object_type *o_ptr)
{
	BIT_FLAGS flgs[TR_FLAG_SIZE];
	object_flags(o_ptr, flgs);

	/* Require activation ability */
	if (!(have_flag(flgs, TR_ACTIVATE))) return (_("なし", "nothing"));

	/* Get an explain of an activation */
	if (activation_index(o_ptr))
	{
		return item_activation_aux(o_ptr);
	}

	/* Special items */
	if (o_ptr->tval == TV_WHISTLE)
	{
		return _("ペット呼び寄せ : 100+d100ターン毎", "call pet every 100+d100 turns");
	}

	if (o_ptr->tval == TV_CAPTURE)
	{
		return _("モンスターを捕える、又は解放する。", "captures or releases a monster.");
	}

	return _("何も起きない", "Nothing");
}


/*!
 * @brief オブジェクトの*鑑定*内容を詳述して表示する /
 * Describe a "fully identified" item
 * @param o_ptr *鑑定*情報を取得する元のオブジェクト構造体参照ポインタ
 * @param mode 表示オプション
 * @return 特筆すべき情報が一つでもあった場合TRUE、一つもなく表示がキャンセルされた場合FALSEを返す。
 */
bool screen_object(object_type *o_ptr, BIT_FLAGS mode)
{
	int i = 0, j, k;

	BIT_FLAGS flgs[TR_FLAG_SIZE];

	char temp[70 * 20];
	concptr            info[128];
	GAME_TEXT o_name[MAX_NLEN];
	int wid, hgt;
	POSITION rad;
	char desc[256];

	int trivial_info = 0;
	object_flags(o_ptr, flgs);

	/* Extract the description */
	{
		roff_to_buf(o_ptr->name1 ? (a_text + a_info[o_ptr->name1].text) :
			    (k_text + k_info[o_ptr->k_idx].text),
			    77 - 15, temp, sizeof(temp));
		for (j = 0; temp[j]; j += 1 + strlen(&temp[j]))
		{ info[i] = &temp[j]; i++;}
	}

	if (object_is_equipment(o_ptr))
	{
		/* Descriptions of a basic equipment is just a flavor */
		trivial_info = i;
	}

	/* Mega-Hack -- describe activation */
	if (have_flag(flgs, TR_ACTIVATE))
	{
		info[i++] = _("始動したときの効果...", "It can be activated for...");
		info[i++] = item_activation(o_ptr);
		info[i++] = _("...ただし装備していなければならない。", "...if it is being worn.");
	}

	/* Figurines, a hack */
	if (o_ptr->tval == TV_FIGURINE)
	{
		info[i++] = _("それは投げた時ペットに変化する。", "It will transform into a pet when thrown.");
	}

	/* Figurines, a hack */
	if (o_ptr->name1 == ART_STONEMASK)
	{
		info[i++] = _("それを装備した者は吸血鬼になる。", "It makes you current_world_ptr->game_turn into a vampire permanently.");
	}

	if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_DOKUBARI))
	{
		info[i++] = _("それは相手を一撃で倒すことがある。", "It will attempt to kill a monster instantly.");
	}

	if ((o_ptr->tval == TV_POLEARM) && (o_ptr->sval == SV_DEATH_SCYTHE))
	{
		info[i++] = _("それは自分自身に攻撃が返ってくることがある。", "It causes you to strike yourself sometimes.");
		info[i++] = _("それは無敵のバリアを切り裂く。", "It always penetrates invulnerability barriers.");
	}

	if (o_ptr->name2 == EGO_2WEAPON)
	{
		info[i++] = _("それは二刀流での命中率を向上させる。", "It affects your ability to hit when you are wielding two weapons.");
	}

	if (have_flag(flgs, TR_EASY_SPELL))
	{
		info[i++] = _("それは魔法の難易度を下げる。", "It affects your ability to cast spells.");
	}

	if (o_ptr->name2 == EGO_AMU_FOOL)
	{
		info[i++] = _("それは魔法の難易度を上げる。", "It interferes with casting spells.");
	}

	if (o_ptr->name2 == EGO_RING_THROW)
	{
		info[i++] = _("それは物を強く投げることを可能にする。", "It provides great strength when you throw an item.");
	}

	if (o_ptr->name2 == EGO_AMU_NAIVETY)
	{
		info[i++] = _("それは魔法抵抗力を下げる。", "It decreases your magic resistance.");
	}

	if (o_ptr->tval == TV_STATUE)
	{
		monster_race *r_ptr = &r_info[o_ptr->pval];

		if (o_ptr->pval == MON_BULLGATES)
			info[i++] = _("それは部屋に飾ると恥ずかしい。", "It is shameful.");
		else if ( r_ptr->flags2 & (RF2_ELDRITCH_HORROR))
			info[i++] = _("それは部屋に飾ると恐い。", "It is fearful.");
		else
			info[i++] = _("それは部屋に飾ると楽しい。", "It is cheerful.");
	}
	
	/* Hack -- describe lite's */
	
	if (o_ptr->name2 == EGO_LITE_DARKNESS) info[i++] = _("それは全く光らない。", "It provides no light.");
	
	rad = 0;
	if (have_flag(flgs, TR_LITE_1) && o_ptr->name2 != EGO_LITE_DARKNESS)  rad += 1;
	if (have_flag(flgs, TR_LITE_2) && o_ptr->name2 != EGO_LITE_DARKNESS)  rad += 2;
	if (have_flag(flgs, TR_LITE_3) && o_ptr->name2 != EGO_LITE_DARKNESS)  rad += 3;
	if (have_flag(flgs, TR_LITE_M1)) rad -= 1;
	if (have_flag(flgs, TR_LITE_M2)) rad -= 2;
	if (have_flag(flgs, TR_LITE_M3)) rad -= 3;
	
	if(o_ptr->name2 == EGO_LITE_SHINE) rad++;
		
	if (have_flag(flgs, TR_LITE_FUEL) && o_ptr->name2 != EGO_LITE_DARKNESS)
	{
		if(rad > 0) sprintf(desc, _("それは燃料補給によって明かり(半径 %d)を授ける。", "It provides light (radius %d) when fueled."), (int)rad);	
	}
	else
	{
		if(rad > 0) sprintf(desc, _("それは永遠なる明かり(半径 %d)を授ける。", "It provides light (radius %d) forever."), (int)rad);
		if(rad < 0) sprintf(desc, _("それは明かりの半径を狭める(半径に-%d)。", "It decreases radius of light source by %d."), (int)-rad);
	}
	
	if(rad != 0) info[i++] = desc;

	
	if (o_ptr->name2 == EGO_LITE_LONG)
	{
		info[i++] = _("それは長いターン明かりを授ける。", "It provides light for much longer time.");
	}

	/* And then describe it fully */

	if (have_flag(flgs, TR_RIDING))
	{
		if ((o_ptr->tval == TV_POLEARM) && ((o_ptr->sval == SV_LANCE) || (o_ptr->sval == SV_HEAVY_LANCE)))
			info[i++] = _("それは乗馬中は非常に使いやすい。", "It is made for use while riding.");
		else
		{
			info[i++] = _("それは乗馬中でも使いやすい。", "It is suitable for use while riding.");
			/* This information is not important enough */
			trivial_info++;
		}
	}
	if (have_flag(flgs, TR_STR))
	{
		info[i++] = _("それは腕力に影響を及ぼす。", "It affects your strength.");
	}
	if (have_flag(flgs, TR_INT))
	{
		info[i++] = _("それは知能に影響を及ぼす。", "It affects your intelligence.");
	}
	if (have_flag(flgs, TR_WIS))
	{
		info[i++] = _("それは賢さに影響を及ぼす。", "It affects your wisdom.");
	}
	if (have_flag(flgs, TR_DEX))
	{
		info[i++] = _("それは器用さに影響を及ぼす。", "It affects your dexterity.");
	}
	if (have_flag(flgs, TR_CON))
	{
		info[i++] = _("それは耐久力に影響を及ぼす。", "It affects your constitution.");
	}
	if (have_flag(flgs, TR_CHR))
	{
		info[i++] = _("それは魅力に影響を及ぼす。", "It affects your charisma.");
	}

	if (have_flag(flgs, TR_MAGIC_MASTERY))
	{
		info[i++] = _("それは魔法道具使用能力に影響を及ぼす。", "It affects your ability to use magic devices.");

	}
	if (have_flag(flgs, TR_STEALTH))
	{
		info[i++] = _("それは隠密行動能力に影響を及ぼす。", "It affects your stealth.");
	}
	if (have_flag(flgs, TR_SEARCH))
	{
		info[i++] = _("それは探索能力に影響を及ぼす。", "It affects your searching.");
	}
	if (have_flag(flgs, TR_INFRA))
	{
		info[i++] = _("それは赤外線視力に影響を及ぼす。", "It affects your infravision.");
	}
	if (have_flag(flgs, TR_TUNNEL))
	{
		info[i++] = _("それは採掘能力に影響を及ぼす。", "It affects your ability to tunnel.");
	}
	if (have_flag(flgs, TR_SPEED))
	{
		info[i++] = _("それはスピードに影響を及ぼす。", "It affects your speed.");
	}
	if (have_flag(flgs, TR_BLOWS))
	{
		info[i++] = _("それは打撃回数に影響を及ぼす。", "It affects your attack speed.");
	}

	if (have_flag(flgs, TR_BRAND_ACID))
	{
		info[i++] = _("それは酸によって大きなダメージを与える。", "It does extra damage from acid.");
	}
	if (have_flag(flgs, TR_BRAND_ELEC))
	{
		info[i++] = _("それは電撃によって大きなダメージを与える。", "It does extra damage from electricity.");
	}
	if (have_flag(flgs, TR_BRAND_FIRE))
	{
		info[i++] = _("それは火炎によって大きなダメージを与える。", "It does extra damage from fire.");
	}
	if (have_flag(flgs, TR_BRAND_COLD))
	{
		info[i++] = _("それは冷気によって大きなダメージを与える。", "It does extra damage from frost.");
	}

	if (have_flag(flgs, TR_BRAND_POIS))
	{
		info[i++] = _("それは敵を毒する。", "It poisons your foes.");
	}

	if (have_flag(flgs, TR_CHAOTIC))
	{
		info[i++] = _("それはカオス的な効果を及ぼす。", "It produces chaotic effects.");
	}

	if (have_flag(flgs, TR_VAMPIRIC))
	{
		info[i++] = _("それは敵から生命力を吸収する。", "It drains life from your foes.");
	}

	if (have_flag(flgs, TR_IMPACT))
	{
		info[i++] = _("それは地震を起こすことができる。", "It can cause earthquakes.");
	}

	if (have_flag(flgs, TR_VORPAL))
	{
		info[i++] = _("それは非常に切れ味が鋭く敵を切断することができる。", "It is very sharp and can cut your foes.");
	}

	if (have_flag(flgs, TR_KILL_DRAGON))
	{
		info[i++] = _("それはドラゴンにとっての天敵である。", "It is a great bane of dragons.");
	}
	else if (have_flag(flgs, TR_SLAY_DRAGON))
	{
		info[i++] = _("それはドラゴンに対して特に恐るべき力を発揮する。", "It is especially deadly against dragons.");
	}

	if (have_flag(flgs, TR_KILL_ORC))
	{
		info[i++] = _("それはオークにとっての天敵である。", "It is a great bane of orcs.");
	}
	if (have_flag(flgs, TR_SLAY_ORC))
	{
		info[i++] = _("それはオークに対して特に恐るべき力を発揮する。", "It is especially deadly against orcs.");
	}

	if (have_flag(flgs, TR_KILL_TROLL))
	{
		info[i++] = _("それはトロルにとっての天敵である。", "It is a great bane of trolls.");
	}
	if (have_flag(flgs, TR_SLAY_TROLL))
	{
		info[i++] = _("それはトロルに対して特に恐るべき力を発揮する。", "It is especially deadly against trolls.");
	}

	if (have_flag(flgs, TR_KILL_GIANT))
	{
		info[i++] = _("それは巨人にとっての天敵である。", "It is a great bane of giants.");
	}
	else if (have_flag(flgs, TR_SLAY_GIANT))
	{
		info[i++] = _("それはジャイアントに対して特に恐るべき力を発揮する。", "It is especially deadly against giants.");
	}

	if (have_flag(flgs, TR_KILL_DEMON))
	{
		info[i++] = _("それはデーモンにとっての天敵である。", "It is a great bane of demons.");
	}
	if (have_flag(flgs, TR_SLAY_DEMON))
	{
		info[i++] = _("それはデーモンに対して聖なる力を発揮する。", "It strikes at demons with holy wrath.");
	}

	if (have_flag(flgs, TR_KILL_UNDEAD))
	{
		info[i++] = _("それはアンデッドにとっての天敵である。", "It is a great bane of undead.");
	}
	if (have_flag(flgs, TR_SLAY_UNDEAD))
	{
		info[i++] = _("それはアンデッドに対して聖なる力を発揮する。", "It strikes at undead with holy wrath.");
	}

	if (have_flag(flgs, TR_KILL_EVIL))
	{
		info[i++] = _("それは邪悪なる存在にとっての天敵である。", "It is a great bane of evil monsters.");
	}
	if (have_flag(flgs, TR_SLAY_EVIL))
	{
		info[i++] = _("それは邪悪なる存在に対して聖なる力で攻撃する。", "It fights against evil with holy fury.");
	}

	if (have_flag(flgs, TR_KILL_ANIMAL))
	{
		info[i++] = _("それは自然界の動物にとっての天敵である。", "It is a great bane of natural creatures.");
	}
	if (have_flag(flgs, TR_SLAY_ANIMAL))
	{
		info[i++] = _("それは自然界の動物に対して特に恐るべき力を発揮する。", "It is especially deadly against natural creatures.");
	}

	if (have_flag(flgs, TR_KILL_HUMAN))
	{
		info[i++] = _("それは人間にとっての天敵である。", "It is a great bane of humans.");
	}
	if (have_flag(flgs, TR_SLAY_HUMAN))
	{
		info[i++] = _("それは人間に対して特に恐るべき力を発揮する。", "It is especially deadly against humans.");
	}

	if (have_flag(flgs, TR_FORCE_WEAPON))
	{
		info[i++] = _("それは使用者の魔力を使って攻撃する。", "It powerfully strikes at a monster using your mana.");
	}
	if (have_flag(flgs, TR_DEC_MANA))
	{
		info[i++] = _("それは魔力の消費を押さえる。", "It decreases your mana consumption.");
	}
	if (have_flag(flgs, TR_SUST_STR))
	{
		info[i++] = _("それはあなたの腕力を維持する。", "It sustains your strength.");
	}
	if (have_flag(flgs, TR_SUST_INT))
	{
		info[i++] = _("それはあなたの知能を維持する。", "It sustains your intelligence.");
	}
	if (have_flag(flgs, TR_SUST_WIS))
	{
		info[i++] = _("それはあなたの賢さを維持する。", "It sustains your wisdom.");
	}
	if (have_flag(flgs, TR_SUST_DEX))
	{
		info[i++] = _("それはあなたの器用さを維持する。", "It sustains your dexterity.");
	}
	if (have_flag(flgs, TR_SUST_CON))
	{
		info[i++] = _("それはあなたの耐久力を維持する。", "It sustains your constitution.");
	}
	if (have_flag(flgs, TR_SUST_CHR))
	{
		info[i++] = _("それはあなたの魅力を維持する。", "It sustains your charisma.");
	}

	if (have_flag(flgs, TR_IM_ACID))
	{
		info[i++] = _("それは酸に対する完全な免疫を授ける。", "It provides immunity to acid.");
	}
	if (have_flag(flgs, TR_IM_ELEC))
	{
		info[i++] = _("それは電撃に対する完全な免疫を授ける。", "It provides immunity to electricity.");
	}
	if (have_flag(flgs, TR_IM_FIRE))
	{
		info[i++] = _("それは火に対する完全な免疫を授ける。", "It provides immunity to fire.");
	}
	if (have_flag(flgs, TR_IM_COLD))
	{
		info[i++] = _("それは寒さに対する完全な免疫を授ける。", "It provides immunity to cold.");
	}

	if (have_flag(flgs, TR_THROW))
	{
		info[i++] = _("それは敵に投げて大きなダメージを与えることができる。", "It is perfectly balanced for throwing.");
	}

	if (have_flag(flgs, TR_FREE_ACT))
	{
		info[i++] = _("それは麻痺に対する完全な免疫を授ける。", "It provides immunity to paralysis.");
	}
	if (have_flag(flgs, TR_HOLD_EXP))
	{
		info[i++] = _("それは経験値吸収に対する耐性を授ける。", "It provides resistance to experience draining.");
	}
	if (have_flag(flgs, TR_RES_FEAR))
	{
		info[i++] = _("それは恐怖への完全な耐性を授ける。", "It makes you completely fearless.");
	}
	if (have_flag(flgs, TR_RES_ACID))
	{
		info[i++] = _("それは酸への耐性を授ける。", "It provides resistance to acid.");
	}
	if (have_flag(flgs, TR_RES_ELEC))
	{
		info[i++] = _("それは電撃への耐性を授ける。", "It provides resistance to electricity.");
	}
	if (have_flag(flgs, TR_RES_FIRE))
	{
		info[i++] = _("それは火への耐性を授ける。", "It provides resistance to fire.");
	}
	if (have_flag(flgs, TR_RES_COLD))
	{
		info[i++] = _("それは寒さへの耐性を授ける。", "It provides resistance to cold.");
	}
	if (have_flag(flgs, TR_RES_POIS))
	{
		info[i++] = _("それは毒への耐性を授ける。", "It provides resistance to poison.");
	}

	if (have_flag(flgs, TR_RES_LITE))
	{
		info[i++] = _("それは閃光への耐性を授ける。", "It provides resistance to light.");
	}
	if (have_flag(flgs, TR_RES_DARK))
	{
		info[i++] = _("それは暗黒への耐性を授ける。", "It provides resistance to dark.");
	}

	if (have_flag(flgs, TR_RES_BLIND))
	{
		info[i++] = _("それは盲目への耐性を授ける。", "It provides resistance to blindness.");
	}
	if (have_flag(flgs, TR_RES_CONF))
	{
		info[i++] = _("それは混乱への耐性を授ける。", "It provides resistance to confusion.");
	}
	if (have_flag(flgs, TR_RES_SOUND))
	{
		info[i++] = _("それは轟音への耐性を授ける。", "It provides resistance to sound.");
	}
	if (have_flag(flgs, TR_RES_SHARDS))
	{
		info[i++] = _("それは破片への耐性を授ける。", "It provides resistance to shards.");
	}

	if (have_flag(flgs, TR_RES_NETHER))
	{
		info[i++] = _("それは地獄への耐性を授ける。", "It provides resistance to nether.");
	}
	if (have_flag(flgs, TR_RES_NEXUS))
	{
		info[i++] = _("それは因果混乱への耐性を授ける。", "It provides resistance to nexus.");
	}
	if (have_flag(flgs, TR_RES_CHAOS))
	{
		info[i++] = _("それはカオスへの耐性を授ける。", "It provides resistance to chaos.");
	}
	if (have_flag(flgs, TR_RES_DISEN))
	{
		info[i++] = _("それは劣化への耐性を授ける。", "It provides resistance to disenchantment.");
	}

	if (have_flag(flgs, TR_LEVITATION))
	{
		info[i++] = _("それは宙に浮くことを可能にする。", "It allows you to levitate.");
	}
		
	if (have_flag(flgs, TR_SEE_INVIS))
	{
		info[i++] = _("それは透明なモンスターを見ることを可能にする。", "It allows you to see invisible monsters.");
	}
	if (have_flag(flgs, TR_TELEPATHY))
	{
		info[i++] = _("それはテレパシー能力を授ける。", "It gives telepathic powers.");
	}
	if (have_flag(flgs, TR_ESP_ANIMAL))
	{
		info[i++] = _("それは自然界の生物を感知する。", "It senses natural creatures.");
	}
	if (have_flag(flgs, TR_ESP_UNDEAD))
	{
		info[i++] = _("それはアンデッドを感知する。", "It senses undead.");
	}
	if (have_flag(flgs, TR_ESP_DEMON))
	{
		info[i++] = _("それは悪魔を感知する。", "It senses demons.");
	}
	if (have_flag(flgs, TR_ESP_ORC))
	{
		info[i++] = _("それはオークを感知する。", "It senses orcs.");
	}
	if (have_flag(flgs, TR_ESP_TROLL))
	{
		info[i++] = _("それはトロルを感知する。", "It senses trolls.");
	}
	if (have_flag(flgs, TR_ESP_GIANT))
	{
		info[i++] = _("それは巨人を感知する。", "It senses giants.");
	}
	if (have_flag(flgs, TR_ESP_DRAGON))
	{
		info[i++] = _("それはドラゴンを感知する。", "It senses dragons.");
	}
	if (have_flag(flgs, TR_ESP_HUMAN))
	{
		info[i++] = _("それは人間を感知する。", "It senses humans.");
	}
	if (have_flag(flgs, TR_ESP_EVIL))
	{
		info[i++] = _("それは邪悪な存在を感知する。", "It senses evil creatures.");
	}
	if (have_flag(flgs, TR_ESP_GOOD))
	{
		info[i++] = _("それは善良な存在を感知する。", "It senses good creatures.");
	}
	if (have_flag(flgs, TR_ESP_NONLIVING))
	{
		info[i++] = _("それは活動する無生物体を感知する。", "It senses non-living creatures.");
	}
	if (have_flag(flgs, TR_ESP_UNIQUE))
	{
		info[i++] = _("それは特別な強敵を感知する。", "It senses unique monsters.");
	}
	if (have_flag(flgs, TR_SLOW_DIGEST))
	{
		info[i++] = _("それはあなたの新陳代謝を遅くする。", "It slows your metabolism.");
	}
	if (have_flag(flgs, TR_REGEN))
	{
		info[i++] = _("それは体力回復力を強化する。", "It speeds your regenerative powers.");
	}
	if (have_flag(flgs, TR_WARNING))
	{
		info[i++] = _("それは危険に対して警告を発する。", "It warns you of danger");
	}
	if (have_flag(flgs, TR_REFLECT))
	{
		info[i++] = _("それは矢の呪文を反射する。", "It reflects bolt spells.");
	}
	if (have_flag(flgs, TR_SH_FIRE))
	{
		info[i++] = _("それは炎のバリアを張る。", "It produces a fiery sheath.");
	}
	if (have_flag(flgs, TR_SH_ELEC))
	{
		info[i++] = _("それは電気のバリアを張る。", "It produces an electric sheath.");
	}
	if (have_flag(flgs, TR_SH_COLD))
	{
		info[i++] = _("それは冷気のバリアを張る。", "It produces a sheath of coldness.");
	}
	if (have_flag(flgs, TR_NO_MAGIC))
	{
		info[i++] = _("それは反魔法バリアを張る。", "It produces an anti-magic shell.");
	}
	if (have_flag(flgs, TR_NO_TELE))
	{
		info[i++] = _("それはテレポートを邪魔する。", "It prevents teleportation.");
	}
	if (have_flag(flgs, TR_XTRA_MIGHT))
	{
		info[i++] = _("それは矢／ボルト／弾をより強力に発射することができる。", "It fires missiles with extra might.");
	}
	if (have_flag(flgs, TR_XTRA_SHOTS))
	{
		info[i++] = _("それは矢／ボルト／弾を非常に早く発射することができる。", "It fires missiles excessively fast.");
	}

	if (have_flag(flgs, TR_BLESSED))
	{
		info[i++] = _("それは神に祝福されている。", "It has been blessed by the gods.");
	}

	if (object_is_cursed(o_ptr))
	{
		if (o_ptr->curse_flags & TRC_PERMA_CURSE)
		{
			info[i++] = _("それは永遠の呪いがかけられている。", "It is permanently cursed.");
		}
		else if (o_ptr->curse_flags & TRC_HEAVY_CURSE)
		{
			info[i++] = _("それは強力な呪いがかけられている。", "It is heavily cursed.");
		}
		else
		{
			info[i++] = _("それは呪われている。", "It is cursed.");

			/*
			 * It's a trivial infomation since there is
			 * fake inscription {cursed}
			 */
			trivial_info++;
		}
	}

	if ((have_flag(flgs, TR_TY_CURSE)) || (o_ptr->curse_flags & TRC_TY_CURSE))
	{
		info[i++] = _("それは太古の禍々しい怨念が宿っている。", "It carries an ancient foul curse.");
	}
	if ((have_flag(flgs, TR_AGGRAVATE)) || (o_ptr->curse_flags & TRC_AGGRAVATE))
	{
		info[i++] = _("それは付近のモンスターを怒らせる。", "It aggravates nearby creatures.");
	}
	if ((have_flag(flgs, TR_DRAIN_EXP)) || (o_ptr->curse_flags & TRC_DRAIN_EXP))
	{
		info[i++] = _("それは経験値を吸い取る。", "It drains experience.");
	}
	if (o_ptr->curse_flags & TRC_SLOW_REGEN)
	{
		info[i++] = _("それは回復力を弱める。", "It slows your regenerative powers.");
	}
	if ((o_ptr->curse_flags & TRC_ADD_L_CURSE) || have_flag(flgs, TR_ADD_L_CURSE))
	{
		info[i++] = _("それは弱い呪いを増やす。","It adds weak curses.");
	}
	if ((o_ptr->curse_flags & TRC_ADD_H_CURSE) || have_flag(flgs, TR_ADD_H_CURSE))
	{
		info[i++] = _("それは強力な呪いを増やす。","It adds heavy curses.");
	}
	if ((have_flag(flgs, TR_CALL_ANIMAL)) || (o_ptr->curse_flags & TRC_CALL_ANIMAL))
	{
		info[i++] = _("それは動物を呼び寄せる。", "It attracts animals.");
	}
	if ((have_flag(flgs, TR_CALL_DEMON)) || (o_ptr->curse_flags & TRC_CALL_DEMON))
	{
		info[i++] = _("それは悪魔を呼び寄せる。", "It attracts demons.");
	}
	if ((have_flag(flgs, TR_CALL_DRAGON)) || (o_ptr->curse_flags & TRC_CALL_DRAGON))
	{
		info[i++] = _("それはドラゴンを呼び寄せる。", "It attracts dragons.");
	}
	if ((have_flag(flgs, TR_CALL_UNDEAD)) || (o_ptr->curse_flags & TRC_CALL_UNDEAD))
	{
		info[i++] = _("それは死霊を呼び寄せる。", "It attracts undeads.");
	}
	if ((have_flag(flgs, TR_COWARDICE)) ||  (o_ptr->curse_flags & TRC_COWARDICE))
	{
		info[i++] = _("それは恐怖感を引き起こす。", "It makes you subject to cowardice.");
	}
	if ((have_flag(flgs, TR_TELEPORT)) || (o_ptr->curse_flags & TRC_TELEPORT))
	{
		info[i++] = _("それはランダムなテレポートを引き起こす。", "It induces random teleportation.");
	}
	if ((have_flag(flgs, TR_LOW_MELEE)) || o_ptr->curse_flags & TRC_LOW_MELEE)
	{
		info[i++] = _("それは攻撃を外しやすい。", "It causes you to miss blows.");
	}
	if ((have_flag(flgs, TR_LOW_AC)) || (o_ptr->curse_flags & TRC_LOW_AC))
	{
		info[i++] = _("それは攻撃を受けやすい。", "It helps your enemies' blows.");
	}
	if ((have_flag(flgs, TR_LOW_MAGIC)) || (o_ptr->curse_flags & TRC_LOW_MAGIC))
	{
		info[i++] = _("それは魔法を唱えにくくする。", "It encumbers you while spellcasting.");
	}
	if ((have_flag(flgs, TR_FAST_DIGEST)) || (o_ptr->curse_flags & TRC_FAST_DIGEST))
	{
		info[i++] = _("それはあなたの新陳代謝を速くする。", "It speeds your metabolism.");
	}
	if ((have_flag(flgs, TR_DRAIN_HP)) || (o_ptr->curse_flags & TRC_DRAIN_HP))
	{
		info[i++] = _("それはあなたの体力を吸い取る。", "It drains you.");
	}
	if ((have_flag(flgs, TR_DRAIN_MANA)) || (o_ptr->curse_flags & TRC_DRAIN_MANA))
	{
		info[i++] = _("それはあなたの魔力を吸い取る。", "It drains your mana.");
	}

	/* Describe about this kind of object instead of THIS fake object */
	if (mode & SCROBJ_FAKE_OBJECT)
	{
		switch (o_ptr->tval)
		{
		case TV_RING:
			switch (o_ptr->sval)
			{
			case SV_RING_LORDLY:
				info[i++] = _("それは幾つかのランダムな耐性を授ける。", "It provides some random resistances.");
				break;
			case SV_RING_WARNING:
				info[i++] = _("それはひとつの低級なESPを授ける事がある。", "It may provide a low rank ESP.");
				break;
			}
			break;

		case TV_AMULET:
			switch (o_ptr->sval)
			{
			case SV_AMULET_RESISTANCE:
				info[i++] = _("それは毒への耐性を授ける事がある。", "It may provides resistance to poison.");
				info[i++] = _("それはランダムな耐性を授ける事がある。", "It may provide a random resistances.");
				break;
			case SV_AMULET_THE_MAGI:
				info[i++] = _("それは最大で３つまでの低級なESPを授ける。", "It provides up to three low rank ESPs.");
				break;
			}
			break;
		}
	}

	if (have_flag(flgs, TR_IGNORE_ACID) &&
	    have_flag(flgs, TR_IGNORE_ELEC) &&
	    have_flag(flgs, TR_IGNORE_FIRE) &&
	    have_flag(flgs, TR_IGNORE_COLD))
	{
		info[i++] = _("それは酸・電撃・火炎・冷気では傷つかない。", "It cannot be harmed by the elements.");
	}
	else
	{
		if (have_flag(flgs, TR_IGNORE_ACID))
		{
			info[i++] = _("それは酸では傷つかない。", "It cannot be harmed by acid.");
		}
		if (have_flag(flgs, TR_IGNORE_ELEC))
		{
			info[i++] = _("それは電撃では傷つかない。", "It cannot be harmed by electricity.");
		}
		if (have_flag(flgs, TR_IGNORE_FIRE))
		{
			info[i++] = _("それは火炎では傷つかない。", "It cannot be harmed by fire.");
		}
		if (have_flag(flgs, TR_IGNORE_COLD))
		{
			info[i++] = _("それは冷気では傷つかない。", "It cannot be harmed by cold.");
		}
	}

	if (mode & SCROBJ_FORCE_DETAIL) trivial_info = 0;

	/* No relevant informations */
	if (i <= trivial_info) return (FALSE);
	screen_save();

	Term_get_size(&wid, &hgt);

	/* Display Item name */
	if (!(mode & SCROBJ_FAKE_OBJECT))
		object_desc(o_name, o_ptr, 0);
	else
		object_desc(o_name, o_ptr, (OD_NAME_ONLY | OD_STORE));

	prt(o_name, 0, 0);

	/* Erase the screen */
	for (k = 1; k < hgt; k++) prt("", k, 13);

	/* Label the information */
	if ((o_ptr->tval == TV_STATUE) && (o_ptr->sval == SV_PHOTO))
	{
		monster_race *r_ptr = &r_info[o_ptr->pval];
		int namelen = strlen(r_name + r_ptr->name);
		prt(format("%s: '", r_name + r_ptr->name), 1, 15);
		Term_queue_bigchar(18 + namelen, 1, r_ptr->x_attr, r_ptr->x_char, 0, 0);
		prt("'", 1, (use_bigtile ? 20 : 19) + namelen);
	}
	else
	{
		prt(_("     アイテムの能力:", "     Item Attributes:"), 1, 15);
	}

	/* We will print on top of the map (column 13) */
	for (k = 2, j = 0; j < i; j++)
	{
		/* Show the info */
		prt(info[j], k++, 15);

		/* Every 20 entries (lines 2 to 21), start over */
		if ((k == hgt - 2) && (j+1 < i))
		{
			prt(_("-- 続く --", "-- more --"), k, 15);
			inkey();
			for (; k > 2; k--) prt("", k, 15);
		}
	}

	/* Wait for it */
	prt(_("[何かキーを押すとゲームに戻ります]", "[Press any key to continue]"), k, 15);

	inkey();
	screen_load();

	/* Gave knowledge */
	return (TRUE);
}



/*!
 * @brief オブジェクト選択時の選択アルファベットラベルを返す /
 * Convert an p_ptr->inventory_list index into a one character label
 * @param i プレイヤーの所持/装備オブジェクトID
 * @return 対応するアルファベット
 * @details Note that the label does NOT distinguish inven/equip.
 */
char index_to_label(int i)
{
	/* Indexes for "inven" are easy */
	if (i < INVEN_RARM) return (I2A(i));

	/* Indexes for "equip" are offset */
	return (I2A(i - INVEN_RARM));
}

/*!
 * @brief 選択アルファベットラベルからプレイヤーの所持オブジェクトIDを返す /
 * Convert a label into the index of an item in the "inven"
 * @return 対応するID。該当スロットにオブジェクトが存在しなかった場合-1を返す / Return "-1" if the label does not indicate a real item
 * @details Note that the label does NOT distinguish inven/equip.
 */
INVENTORY_IDX label_to_inven(int c)
{
	INVENTORY_IDX i;

	/* Convert */
	i = (INVENTORY_IDX)(islower(c) ? A2I(c) : -1);

	/* Verify the index */
	if ((i < 0) || (i > INVEN_PACK)) return (-1);

	/* Empty slots can never be chosen */
	if (!p_ptr->inventory_list[i].k_idx) return (-1);

	/* Return the index */
	return (i);
}


/*! See cmd5.c */
extern bool select_ring_slot;


/*!
 * @brief プレイヤーの所持/装備オブジェクトIDが指輪枠かを返す /
 * @param i プレイヤーの所持/装備オブジェクトID
 * @return 指輪枠ならばTRUEを返す。
 */
static bool is_ring_slot(int i)
{
	return (i == INVEN_RIGHT) || (i == INVEN_LEFT);
}


/*!
 * @brief 選択アルファベットラベルからプレイヤーの装備オブジェクトIDを返す /
 * Convert a label into the index of a item in the "equip"
 * @return 対応するID。該当スロットにオブジェクトが存在しなかった場合-1を返す / Return "-1" if the label does not indicate a real item
 */
INVENTORY_IDX label_to_equip(int c)
{
	INVENTORY_IDX i;

	/* Convert */
	i = (INVENTORY_IDX)(islower(c) ? A2I(c) : -1) + INVEN_RARM;

	/* Verify the index */
	if ((i < INVEN_RARM) || (i >= INVEN_TOTAL)) return (-1);

	if (select_ring_slot) return is_ring_slot(i) ? i : -1;

	/* Empty slots can never be chosen */
	if (!p_ptr->inventory_list[i].k_idx) return (-1);

	/* Return the index */
	return (i);
}



/*!
 * @brief オブジェクトの該当装備部位IDを返す /
 * Determine which equipment slot (if any) an item likes
 * @param o_ptr 名称を取得する元のオブジェクト構造体参照ポインタ
 * @return 対応する装備部位ID
 */
s16b wield_slot(object_type *o_ptr)
{
	/* Slot for equipment */
	switch (o_ptr->tval)
	{
		case TV_DIGGING:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		{
			if (!p_ptr->inventory_list[INVEN_RARM].k_idx) return (INVEN_RARM);
			if (p_ptr->inventory_list[INVEN_LARM].k_idx) return (INVEN_RARM);
			return (INVEN_LARM);
		}

		case TV_CAPTURE:
		case TV_CARD:
		case TV_SHIELD:
		{
			if (!p_ptr->inventory_list[INVEN_LARM].k_idx) return (INVEN_LARM);
			if (p_ptr->inventory_list[INVEN_RARM].k_idx) return (INVEN_LARM);
			return (INVEN_RARM);
		}

		case TV_BOW:
		{
			return (INVEN_BOW);
		}

		case TV_RING:
		{
			/* Use the right hand first */
			if (!p_ptr->inventory_list[INVEN_RIGHT].k_idx) return (INVEN_RIGHT);

			/* Use the left hand for swapping (by default) */
			return (INVEN_LEFT);
		}

		case TV_AMULET:
		case TV_WHISTLE:
		{
			return (INVEN_NECK);
		}

		case TV_LITE:
		{
			return (INVEN_LITE);
		}

		case TV_DRAG_ARMOR:
		case TV_HARD_ARMOR:
		case TV_SOFT_ARMOR:
		{
			return (INVEN_BODY);
		}

		case TV_CLOAK:
		{
			return (INVEN_OUTER);
		}

		case TV_CROWN:
		case TV_HELM:
		{
			return (INVEN_HEAD);
		}

		case TV_GLOVES:
		{
			return (INVEN_HANDS);
		}

		case TV_BOOTS:
		{
			return (INVEN_FEET);
		}
	}

	/* No slot available */
	return (-1);
}

/*!
 * @brief 所持/装備オブジェクトIDの部位表現を返す /
 * Return a string mentioning how a given item is carried
 * @param i 部位表現を求めるプレイヤーの所持/装備オブジェクトID
 * @return 部位表現の文字列ポインタ
 */
concptr mention_use(int i)
{
	concptr p;

	/* Examine the location */
	switch (i)
	{
#ifdef JP
		case INVEN_RARM:  p = p_ptr->heavy_wield[0] ? "運搬中" : ((p_ptr->ryoute && p_ptr->migite) ? " 両手" : (left_hander ? " 左手" : " 右手")); break;
#else
		case INVEN_RARM:  p = p_ptr->heavy_wield[0] ? "Just lifting" : (p_ptr->migite ? "Wielding" : "On arm"); break;
#endif

#ifdef JP
		case INVEN_LARM:  p = p_ptr->heavy_wield[1] ? "運搬中" : ((p_ptr->ryoute && p_ptr->hidarite) ? " 両手" : (left_hander ? " 右手" : " 左手")); break;
#else
		case INVEN_LARM:  p = p_ptr->heavy_wield[1] ? "Just lifting" : (p_ptr->hidarite ? "Wielding" : "On arm"); break;
#endif

		case INVEN_BOW:   p = (adj_str_hold[p_ptr->stat_ind[A_STR]] < p_ptr->inventory_list[i].weight / 10) ? _("運搬中", "Just holding") : _("射撃用", "Shooting"); break;
		case INVEN_RIGHT: p = (left_hander ? _("左手指", "On left hand") : _("右手指", "On right hand")); break;
		case INVEN_LEFT:  p = (left_hander ? _("右手指", "On right hand") : _("左手指", "On left hand")); break;
		case INVEN_NECK:  p = _("  首", "Around neck"); break;
		case INVEN_LITE:  p = _(" 光源", "Light source"); break;
		case INVEN_BODY:  p = _("  体", "On body"); break;
		case INVEN_OUTER: p = _("体の上", "About body"); break;
		case INVEN_HEAD:  p = _("  頭", "On head"); break;
		case INVEN_HANDS: p = _("  手", "On hands"); break;
		case INVEN_FEET:  p = _("  足", "On feet"); break;
		default:          p = _("ザック", "In pack"); break;
	}

	/* Return the result */
	return p;
}


/*!
 * @brief 所持/装備オブジェクトIDの現在の扱い方の状態表現を返す /
 * Return a string describing how a given item is being worn.
 * @param i 状態表現を求めるプレイヤーの所持/装備オブジェクトID
 * @return 状態表現内容の文字列ポインタ
 * @details
 * Currently, only used for items in the equipment, not p_ptr->inventory_list.
 */
concptr describe_use(int i)
{
	concptr p;

	switch (i)
	{
#ifdef JP
		case INVEN_RARM:  p = p_ptr->heavy_wield[0] ? "運搬中の" : ((p_ptr->ryoute && p_ptr->migite) ? "両手に装備している" : (left_hander ? "左手に装備している" : "右手に装備している")); break;
#else
		case INVEN_RARM:  p = p_ptr->heavy_wield[0] ? "just lifting" : (p_ptr->migite ? "attacking monsters with" : "wearing on your arm"); break;
#endif

#ifdef JP
		case INVEN_LARM:  p = p_ptr->heavy_wield[1] ? "運搬中の" : ((p_ptr->ryoute && p_ptr->hidarite) ? "両手に装備している" : (left_hander ? "右手に装備している" : "左手に装備している")); break;
#else
		case INVEN_LARM:  p = p_ptr->heavy_wield[1] ? "just lifting" : (p_ptr->hidarite ? "attacking monsters with" : "wearing on your arm"); break;
#endif

		case INVEN_BOW:   p = (adj_str_hold[p_ptr->stat_ind[A_STR]] < p_ptr->inventory_list[i].weight / 10) ? _("持つだけで精一杯の", "just holding") : _("射撃用に装備している", "shooting missiles with"); break;
		case INVEN_RIGHT: p = (left_hander ? _("左手の指にはめている", "wearing on your left hand") : _("右手の指にはめている", "wearing on your right hand")); break;
		case INVEN_LEFT:  p = (left_hander ? _("右手の指にはめている", "wearing on your right hand") : _("左手の指にはめている", "wearing on your left hand")); break;
		case INVEN_NECK:  p = _("首にかけている", "wearing around your neck"); break;
		case INVEN_LITE:  p = _("光源にしている", "using to light the way"); break;
		case INVEN_BODY:  p = _("体に着ている", "wearing on your body"); break;
		case INVEN_OUTER: p = _("身にまとっている", "wearing on your back"); break;
		case INVEN_HEAD:  p = _("頭にかぶっている", "wearing on your head"); break;
		case INVEN_HANDS: p = _("手につけている", "wearing on your hands"); break;
		case INVEN_FEET:  p = _("足にはいている", "wearing on your feet"); break;
		default:          p = _("ザックに入っている", "carrying in your pack"); break;
	}

	/* Return the result */
	return p;
}


/*!
 * @brief tval/sval指定のベースアイテムがプレイヤーの使用可能な魔法書かどうかを返す /
 * Hack: Check if a spellbook is one of the realms we can use. -- TY
 * @param book_tval ベースアイテムのtval
 * @param book_sval ベースアイテムのsval
 * @return 使用可能な魔法書ならばTRUEを返す。
 */

bool check_book_realm(const OBJECT_TYPE_VALUE book_tval, const OBJECT_SUBTYPE_VALUE book_sval)
{
	if (book_tval < TV_LIFE_BOOK) return FALSE;
	if (p_ptr->pclass == CLASS_SORCERER)
	{
		return is_magic(tval2realm(book_tval));
	}
	else if (p_ptr->pclass == CLASS_RED_MAGE)
	{
		if (is_magic(tval2realm(book_tval)))
			return ((book_tval == TV_ARCANE_BOOK) || (book_sval < 2));
	}
	return (REALM1_BOOK == book_tval || REALM2_BOOK == book_tval);
}

/*!
 * @brief アイテムがitem_tester_hookグローバル関数ポインタの条件を満たしているかを返す汎用関数
 * Check an item against the item tester info
 * @param o_ptr 判定を行いたいオブジェクト構造体参照ポインタ
 * @return item_tester_hookの参照先、その他いくつかの例外に応じてTRUE/FALSEを返す。
 */
bool item_tester_okay(object_type *o_ptr)
{
	/* Hack -- allow listing empty slots */
	// if (item_tester_full) return (TRUE); // TODO:DELETE

	/* Require an item */
	if (!o_ptr->k_idx) return (FALSE);

	/* Hack -- ignore "gold" */
	if (o_ptr->tval == TV_GOLD)
	{
		/* See xtra2.c */
		extern bool show_gold_on_floor;

		if (!show_gold_on_floor) return (FALSE);
	}

	/* Check the tval */
	if (item_tester_tval)
	{
		/* Is it a spellbook? If so, we need a hack -- TY */
		if ((item_tester_tval <= TV_DEATH_BOOK) &&
			(item_tester_tval >= TV_LIFE_BOOK))
			return check_book_realm(o_ptr->tval, o_ptr->sval);
		else
			if (item_tester_tval != o_ptr->tval) return (FALSE);
	}

	/* Check the hook */
	if (item_tester_hook)
	{
		if (!(*item_tester_hook)(o_ptr)) return (FALSE);
	}

	/* Assume okay */
	return (TRUE);
}


/*!
 * @brief 所持アイテム一覧を表示する /
 * Choice window "shadow" of the "show_inven()" function
 * @return なし
 */
void display_inven(void)
{
	register int i, n, z = 0;
	object_type *o_ptr;
	TERM_COLOR attr = TERM_WHITE;
	char tmp_val[80];
	GAME_TEXT o_name[MAX_NLEN];
	TERM_LEN wid, hgt;

	Term_get_size(&wid, &hgt);

	for (i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &p_ptr->inventory_list[i];
		if (!o_ptr->k_idx) continue;
		z = i + 1;
	}

	for (i = 0; i < z; i++)
	{
		o_ptr = &p_ptr->inventory_list[i];
		tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';
		if (item_tester_okay(o_ptr))
		{
			tmp_val[0] = index_to_label(i);
			tmp_val[1] = ')';
		}

		Term_putstr(0, i, 3, TERM_WHITE, tmp_val);
		object_desc(o_name, o_ptr, 0);
		n = strlen(o_name);
		attr = tval_to_attr[o_ptr->tval % 128];
		if (o_ptr->timeout)
		{
			attr = TERM_L_DARK;
		}

		Term_putstr(3, i, n, attr, o_name);
		Term_erase(3+n, i, 255);

		if (show_weights)
		{
			int wgt = o_ptr->weight * o_ptr->number;
#ifdef JP
			sprintf(tmp_val, "%3d.%1d kg", lbtokg1(wgt), lbtokg2(wgt));
#else
			sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);
#endif
			prt(tmp_val, i, wid - 9);
		}
	}

	for (i = z; i < hgt; i++)
	{
		Term_erase(0, i, 255);
	}
}



/*!
 * @brief 装備アイテム一覧を表示する /
 * Choice window "shadow" of the "show_equip()" function
 * @return なし
 */
void display_equip(void)
{
	register int i, n;
	object_type *o_ptr;
	TERM_COLOR attr = TERM_WHITE;
	char tmp_val[80];
	GAME_TEXT o_name[MAX_NLEN];
	TERM_LEN wid, hgt;

	Term_get_size(&wid, &hgt);

	for (i = INVEN_RARM; i < INVEN_TOTAL; i++)
	{
		o_ptr = &p_ptr->inventory_list[i];
		tmp_val[0] = tmp_val[1] = tmp_val[2] = ' ';
		if (select_ring_slot ? is_ring_slot(i) : item_tester_okay(o_ptr))
		{
			tmp_val[0] = index_to_label(i);
			tmp_val[1] = ')';
		}

		Term_putstr(0, i - INVEN_RARM, 3, TERM_WHITE, tmp_val);
		if ((((i == INVEN_RARM) && p_ptr->hidarite) || ((i == INVEN_LARM) && p_ptr->migite)) && p_ptr->ryoute)
		{
			strcpy(o_name, _("(武器を両手持ち)", "(wielding with two-hands)"));
			attr = TERM_WHITE;
		}
		else
		{
			object_desc(o_name, o_ptr, 0);
			attr = tval_to_attr[o_ptr->tval % 128];
		}

		n = strlen(o_name);
		if (o_ptr->timeout)
		{
			attr = TERM_L_DARK;
		}
		Term_putstr(3, i - INVEN_RARM, n, attr, o_name);

		Term_erase(3 + n, i - INVEN_RARM, 255);

		if (show_weights)
		{
			int wgt = o_ptr->weight * o_ptr->number;
#ifdef JP
			sprintf(tmp_val, "%3d.%1d kg", lbtokg1(wgt) , lbtokg2(wgt));
#else
			sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);
#endif

			prt(tmp_val, i - INVEN_RARM, wid - (show_labels ? 28 : 9));
		}

		if (show_labels)
		{
			Term_putstr(wid - 20, i - INVEN_RARM, -1, TERM_WHITE, " <-- ");
			prt(mention_use(i), i - INVEN_RARM, wid - 15);
		}
	}

	for (i = INVEN_TOTAL - INVEN_RARM; i < hgt; i++)
	{
		Term_erase(0, i, 255);
	}
}


/*!
 * @brief 所持/装備オブジェクトに選択タグを与える/タグに該当するオブジェクトがあるかを返す /
 * Find the "first" p_ptr->inventory_list object with the given "tag".
 * @param cp 対応するタグIDを与える参照ポインタ
 * @param tag 該当するオブジェクトがあるかを調べたいタグ
 * @param mode 所持、装備の切り替え
 * @return タグに該当するオブジェクトがあるならTRUEを返す
 * @details
 * A "tag" is a numeral "n" appearing as "@n" anywhere in the\n
 * inscription of an object.  Alphabetical characters don't work as a\n
 * tag in this form.\n
 *\n
 * Also, the tag "@xn" will work as well, where "n" is a any tag-char,\n
 * and "x" is the "current" command_cmd code.\n
 */
static bool get_tag(COMMAND_CODE *cp, char tag, BIT_FLAGS mode)
{
	COMMAND_CODE i;
	COMMAND_CODE start, end;
	concptr s;

	/* Extract index from mode */
	switch (mode)
	{
	case USE_EQUIP:
		start = INVEN_RARM;
		end = INVEN_TOTAL - 1;
		break;

	case USE_INVEN:
		start = 0;
		end = INVEN_PACK - 1;
		break;

	default:
		return FALSE;
	}

	/**** Find a tag in the form of {@x#} (allow alphabet tag) ***/

	/* Check every p_ptr->inventory_list object */
	for (i = start; i <= end; i++)
	{
		object_type *o_ptr = &p_ptr->inventory_list[i];
		if (!o_ptr->k_idx) continue;

		/* Skip empty inscriptions */
		if (!o_ptr->inscription) continue;

		/* Skip non-choice */
		if (!item_tester_okay(o_ptr) && !(mode & USE_FULL)) continue;

		/* Find a '@' */
		s = my_strchr(quark_str(o_ptr->inscription), '@');

		/* Process all tags */
		while (s)
		{
			/* Check the special tags */
			if ((s[1] == command_cmd) && (s[2] == tag))
			{
				/* Save the actual p_ptr->inventory_list ID */
				*cp = i;

				/* Success */
				return (TRUE);
			}

			/* Find another '@' */
			s = my_strchr(s + 1, '@');
		}
	}


	/**** Find a tag in the form of {@#} (allows only numerals)  ***/

	/* Don't allow {@#} with '#' being alphabet */
	if (tag < '0' || '9' < tag)
	{
		/* No such tag */
		return FALSE;
	}

	/* Check every object */
	for (i = start; i <= end; i++)
	{
		object_type *o_ptr = &p_ptr->inventory_list[i];
		if (!o_ptr->k_idx) continue;

		/* Skip empty inscriptions */
		if (!o_ptr->inscription) continue;

		/* Skip non-choice */
		if (!item_tester_okay(o_ptr) && !(mode & USE_FULL)) continue;

		/* Find a '@' */
		s = my_strchr(quark_str(o_ptr->inscription), '@');

		/* Process all tags */
		while (s)
		{
			/* Check the normal tags */
			if (s[1] == tag)
			{
				/* Save the actual p_ptr->inventory_list ID */
				*cp = i;

				/* Success */
				return (TRUE);
			}

			/* Find another '@' */
			s = my_strchr(s + 1, '@');
		}
	}

	/* No such tag */
	return (FALSE);
}


/*!
 * @brief 床オブジェクトに選択タグを与える/タグに該当するオブジェクトがあるかを返す /
 * Find the "first" p_ptr->inventory_list object with the given "tag".
 * @param cp 対応するタグIDを与える参照ポインタ
 * @param tag 該当するオブジェクトがあるかを調べたいタグ
 * @param floor_list 床上アイテムの配列
 * @param floor_num  床上アイテムの配列ID
 * @return タグに該当するオブジェクトがあるならTRUEを返す
 * @details
 * A "tag" is a numeral "n" appearing as "@n" anywhere in the\n
 * inscription of an object.  Alphabetical characters don't work as a\n
 * tag in this form.\n
 *\n
 * Also, the tag "@xn" will work as well, where "n" is a any tag-char,\n
 * and "x" is the "current" command_cmd code.\n
 */
static bool get_tag_floor(COMMAND_CODE *cp, char tag, FLOOR_IDX floor_list[], ITEM_NUMBER floor_num)
{
	COMMAND_CODE i;
	concptr s;

	/**** Find a tag in the form of {@x#} (allow alphabet tag) ***/

	/* Check every object in the grid */
	for (i = 0; i < floor_num && i < 23; i++)
	{
		object_type *o_ptr = &current_floor_ptr->o_list[floor_list[i]];

		/* Skip empty inscriptions */
		if (!o_ptr->inscription) continue;

		/* Find a '@' */
		s = my_strchr(quark_str(o_ptr->inscription), '@');

		/* Process all tags */
		while (s)
		{
			/* Check the special tags */
			if ((s[1] == command_cmd) && (s[2] == tag))
			{
				/* Save the actual floor object ID */
				*cp = i;

				/* Success */
				return (TRUE);
			}

			/* Find another '@' */
			s = my_strchr(s + 1, '@');
		}
	}


	/**** Find a tag in the form of {@#} (allows only numerals)  ***/

	/* Don't allow {@#} with '#' being alphabet */
	if (tag < '0' || '9' < tag)
	{
		/* No such tag */
		return FALSE;
	}

	/* Check every object in the grid */
	for (i = 0; i < floor_num && i < 23; i++)
	{
		object_type *o_ptr = &current_floor_ptr->o_list[floor_list[i]];

		/* Skip empty inscriptions */
		if (!o_ptr->inscription) continue;

		/* Find a '@' */
		s = my_strchr(quark_str(o_ptr->inscription), '@');

		/* Process all tags */
		while (s)
		{
			/* Check the normal tags */
			if (s[1] == tag)
			{
				/* Save the floor object ID */
				*cp = i;

				/* Success */
				return (TRUE);
			}

			/* Find another '@' */
			s = my_strchr(s + 1, '@');
		}
	}

	/* No such tag */
	return (FALSE);
}


/*!
 * @brief タグIDにあわせてタグアルファベットのリストを返す /
 * Move around label characters with correspond tags
 * @param label ラベルリストを取得する文字列参照ポインタ
 * @param mode 所持品リストか装備品リストかの切り替え
 * @return なし
 */
static void prepare_label_string(char *label, BIT_FLAGS mode)
{
	concptr alphabet_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int  offset = (mode == USE_EQUIP) ? INVEN_RARM : 0;
	int  i;

	/* Prepare normal labels */
	strcpy(label, alphabet_chars);

	/* Move each label */
	for (i = 0; i < 52; i++)
	{
		COMMAND_CODE index;
		SYMBOL_CODE c = alphabet_chars[i];

		/* Find a tag with this label */
		if (get_tag(&index, c, mode))
		{
			/* Delete the overwritten label */
			if (label[i] == c) label[i] = ' ';

			/* Move the label to the place of corresponding tag */
			label[index - offset] = c;
		}
	}
}


/*!
 * @brief タグIDにあわせてタグアルファベットのリストを返す(床上アイテム用) /
 * Move around label characters with correspond tags (floor version)
 * @param label ラベルリストを取得する文字列参照ポインタ
 * @param floor_list 床上アイテムの配列
 * @param floor_num  床上アイテムの配列ID
 * @return なし
 */
/*
 */
static void prepare_label_string_floor(char *label, FLOOR_IDX floor_list[], ITEM_NUMBER floor_num)
{
	concptr alphabet_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int  i;

	/* Prepare normal labels */
	strcpy(label, alphabet_chars);

	/* Move each label */
	for (i = 0; i < 52; i++)
	{
		COMMAND_CODE index;
		SYMBOL_CODE c = alphabet_chars[i];

		/* Find a tag with this label */
		if (get_tag_floor(&index, c, floor_list, floor_num))
		{
			/* Delete the overwritten label */
			if (label[i] == c) label[i] = ' ';

			/* Move the label to the place of corresponding tag */
			label[index] = c;
		}
	}
}


/*!
 * @brief 所持アイテムの表示を行う /
 * Display the p_ptr->inventory_list.
 * @param target_item アイテムの選択処理を行うか否か。
 * @return 選択したアイテムのタグ
 * @details
 * Hack -- do not display "trailing" empty slots
 */
COMMAND_CODE show_inven(int target_item, BIT_FLAGS mode)
{
	COMMAND_CODE i;
	int j, k, l, z = 0;
	int             col, cur_col, len;
	object_type *o_ptr;
	GAME_TEXT o_name[MAX_NLEN];
	char            tmp_val[80];
	COMMAND_CODE    out_index[23];
	TERM_COLOR      out_color[23];
	char            out_desc[23][MAX_NLEN];
	COMMAND_CODE target_item_label = 0;
	TERM_LEN wid, hgt;
	char inven_label[52 + 1];

	/* Starting column */
	col = command_gap;

	Term_get_size(&wid, &hgt);

	/* Default "max-length" */
	len = wid - col - 1;


	/* Find the "final" slot */
	for (i = 0; i < INVEN_PACK; i++)
	{
		o_ptr = &p_ptr->inventory_list[i];
		if (!o_ptr->k_idx) continue;

		/* Track */
		z = i + 1;
	}

	prepare_label_string(inven_label, USE_INVEN);

	/* Display the p_ptr->inventory_list */
	for (k = 0, i = 0; i < z; i++)
	{
		o_ptr = &p_ptr->inventory_list[i];

		/* Is this item acceptable? */
		if (!item_tester_okay(o_ptr) && !(mode & USE_FULL)) continue;

		object_desc(o_name, o_ptr, 0);

		/* Save the object index, color, and description */
		out_index[k] = i;
		out_color[k] = tval_to_attr[o_ptr->tval % 128];

		/* Grey out charging items */
		if (o_ptr->timeout)
		{
			out_color[k] = TERM_L_DARK;
		}

		(void)strcpy(out_desc[k], o_name);

		/* Find the predicted "line length" */
		l = strlen(out_desc[k]) + 5;

		/* Be sure to account for the weight */
		if (show_weights) l += 9;

		/* Account for icon if displayed */
		if (show_item_graph)
		{
			l += 2;
			if (use_bigtile) l++;
		}

		/* Maintain the maximum length */
		if (l > len) len = l;

		/* Advance to next "line" */
		k++;
	}

	/* Find the column to start in */
	col = (len > wid - 4) ? 0 : (wid - len - 1);

	/* Output each entry */
	for (j = 0; j < k; j++)
	{
		i = out_index[j];
		o_ptr = &p_ptr->inventory_list[i];

		/* Clear the line */
		prt("", j + 1, col ? col - 2 : col);

		if (use_menu && target_item)
		{
			if (j == (target_item-1))
			{
				strcpy(tmp_val, _("》", "> "));
				target_item_label = i;
			}
			else strcpy(tmp_val, "  ");
		}
		else if (i <= INVEN_PACK)
		{
			/* Prepare an index --(-- */
			sprintf(tmp_val, "%c)", inven_label[i]);
		}
		else
		{
			/* Prepare an index --(-- */
			sprintf(tmp_val, "%c)", index_to_label(i));
		}

		/* Clear the line with the (possibly indented) index */
		put_str(tmp_val, j + 1, col);

		cur_col = col + 3;

		/* Display graphics for object, if desired */
		if (show_item_graph)
		{
			TERM_COLOR a = object_attr(o_ptr);
			SYMBOL_CODE c = object_char(o_ptr);
			Term_queue_bigchar(cur_col, j + 1, a, c, 0, 0);
			if (use_bigtile) cur_col++;

			cur_col += 2;
		}


		/* Display the entry itself */
		c_put_str(out_color[j], out_desc[j], j + 1, cur_col);

		/* Display the weight if needed */
		if (show_weights)
		{
			int wgt = o_ptr->weight * o_ptr->number;
#ifdef JP
			(void)sprintf(tmp_val, "%3d.%1d kg", lbtokg1(wgt) , lbtokg2(wgt) );
#else
			(void)sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);
#endif

			prt(tmp_val, j + 1, wid - 9);
		}
	}

	/* Make a "shadow" below the list (only if needed) */
	if (j && (j < 23)) prt("", j + 1, col ? col - 2 : col);

	/* Save the new column */
	command_gap = col;

	return target_item_label;
}


/*!
 * @brief 装備アイテムの表示を行う /
 * Display the equipment.
 * @param target_item アイテムの選択処理を行うか否か。
 * @return 選択したアイテムのタグ
 */
COMMAND_CODE show_equip(int target_item, BIT_FLAGS mode)
{
	COMMAND_CODE i;
	int j, k, l;
	int             col, cur_col, len;
	object_type *o_ptr;
	char            tmp_val[80];
	GAME_TEXT o_name[MAX_NLEN];
	COMMAND_CODE    out_index[23];
	TERM_COLOR      out_color[23];
	char            out_desc[23][MAX_NLEN];
	COMMAND_CODE target_item_label = 0;
	TERM_LEN wid, hgt;
	char            equip_label[52 + 1];

	/* Starting column */
	col = command_gap;

	Term_get_size(&wid, &hgt);

	/* Maximal length */
	len = wid - col - 1;


	/* Scan the equipment list */
	for (k = 0, i = INVEN_RARM; i < INVEN_TOTAL; i++)
	{
		o_ptr = &p_ptr->inventory_list[i];

		/* Is this item acceptable? */
		if (!(select_ring_slot ? is_ring_slot(i) : item_tester_okay(o_ptr) || (mode & USE_FULL)) &&
		    (!((((i == INVEN_RARM) && p_ptr->hidarite) || ((i == INVEN_LARM) && p_ptr->migite)) && p_ptr->ryoute) ||
				(mode & IGNORE_BOTHHAND_SLOT))) continue;

		object_desc(o_name, o_ptr, 0);

		if ((((i == INVEN_RARM) && p_ptr->hidarite) || ((i == INVEN_LARM) && p_ptr->migite)) && p_ptr->ryoute)
		{
			(void)strcpy(out_desc[k],_("(武器を両手持ち)", "(wielding with two-hands)"));
			out_color[k] = TERM_WHITE;
		}
		else
		{
			(void)strcpy(out_desc[k], o_name);
			out_color[k] = tval_to_attr[o_ptr->tval % 128];
		}

		out_index[k] = i;
		/* Grey out charging items */
		if (o_ptr->timeout)
		{
			out_color[k] = TERM_L_DARK;
		}

		/* Extract the maximal length (see below) */
#ifdef JP
		l = strlen(out_desc[k]) + (2 + 1);
#else
		l = strlen(out_desc[k]) + (2 + 3);
#endif


		/* Increase length for labels (if needed) */
#ifdef JP
		if (show_labels) l += (7 + 2);
#else
		if (show_labels) l += (14 + 2);
#endif


		/* Increase length for weight (if needed) */
		if (show_weights) l += 9;

		if (show_item_graph) l += 2;

		/* Maintain the max-length */
		if (l > len) len = l;

		/* Advance the entry */
		k++;
	}

	/* Hack -- Find a column to start in */
#ifdef JP
	col = (len > wid - 6) ? 0 : (wid - len - 1);
#else
	col = (len > wid - 4) ? 0 : (wid - len - 1);
#endif

	prepare_label_string(equip_label, USE_EQUIP);

	/* Output each entry */
	for (j = 0; j < k; j++)
	{
		i = out_index[j];
		o_ptr = &p_ptr->inventory_list[i];

		/* Clear the line */
		prt("", j + 1, col ? col - 2 : col);

		if (use_menu && target_item)
		{
			if (j == (target_item-1))
			{
				strcpy(tmp_val, _("》", "> "));
				target_item_label = i;
			}
			else strcpy(tmp_val, "  ");
		}
		else if (i >= INVEN_RARM)
		{
			/* Prepare an index --(-- */
			sprintf(tmp_val, "%c)", equip_label[i - INVEN_RARM]);
		}
		else
		{
			/* Prepare an index --(-- */
			sprintf(tmp_val, "%c)", index_to_label(i));
		}

		/* Clear the line with the (possibly indented) index */
		put_str(tmp_val, j+1, col);

		cur_col = col + 3;

		/* Display graphics for object, if desired */
		if (show_item_graph)
		{
			TERM_COLOR a = object_attr(o_ptr);
			SYMBOL_CODE c = object_char(o_ptr);
			Term_queue_bigchar(cur_col, j + 1, a, c, 0, 0);
			if (use_bigtile) cur_col++;

			cur_col += 2;
		}

		/* Use labels */
		if (show_labels)
		{
			/* Mention the use */
			(void)sprintf(tmp_val, _("%-7s: ", "%-14s: "), mention_use(i));

			put_str(tmp_val, j+1, cur_col);

			/* Display the entry itself */
			c_put_str(out_color[j], out_desc[j], j+1, _(cur_col + 9, cur_col + 16));
		}

		/* No labels */
		else
		{
			/* Display the entry itself */
			c_put_str(out_color[j], out_desc[j], j+1, cur_col);
		}

		/* Display the weight if needed */
		if (show_weights)
		{
			int wgt = o_ptr->weight * o_ptr->number;
#ifdef JP
			(void)sprintf(tmp_val, "%3d.%1d kg", lbtokg1(wgt) , lbtokg2(wgt) );
#else
			(void)sprintf(tmp_val, "%3d.%d lb", wgt / 10, wgt % 10);
#endif

			prt(tmp_val, j + 1, wid - 9);
		}
	}

	/* Make a "shadow" below the list (only if needed) */
	if (j && (j < 23)) prt("", j + 1, col ? col - 2 : col);

	/* Save the new column */
	command_gap = col;

	return target_item_label;
}

/*!
 * @brief サブウィンドウに所持品、装備品リストの表示を行う /
 * Flip "inven" and "equip" in any sub-windows
 * @return なし
 */
void toggle_inven_equip(void)
{
	int j;

	/* Scan windows */
	for (j = 0; j < 8; j++)
	{
		/* Unused */
		if (!angband_term[j]) continue;

		/* Flip inven to equip */
		if (window_flag[j] & (PW_INVEN))
		{
			/* Flip flags */
			window_flag[j] &= ~(PW_INVEN);
			window_flag[j] |= (PW_EQUIP);

			p_ptr->window |= (PW_EQUIP);
		}

		/* Flip inven to equip */
		else if (window_flag[j] & (PW_EQUIP))
		{
			/* Flip flags */
			window_flag[j] &= ~(PW_EQUIP);
			window_flag[j] |= (PW_INVEN);

			p_ptr->window |= (PW_INVEN);
		}
	}
}

/*!
 * @brief 選択したアイテムの確認処理の補助 /
 * Verify the choice of an item.
 * @param prompt メッセージ表示の一部
 * @param item 選択アイテムID
 * @return 確認がYesならTRUEを返す。
 * @details The item can be negative to mean "item on floor".
 */
static bool verify(concptr prompt, INVENTORY_IDX item)
{
	GAME_TEXT o_name[MAX_NLEN];
	char        out_val[MAX_NLEN+20];
	object_type *o_ptr;


	/* Inventory */
	if (item >= 0)
	{
		o_ptr = &p_ptr->inventory_list[item];
	}

	/* Floor */
	else
	{
		o_ptr = &current_floor_ptr->o_list[0 - item];
	}
	object_desc(o_name, o_ptr, 0);

	/* Prompt */
	(void)sprintf(out_val, _("%s%sですか? ", "%s %s? "), prompt, o_name);

	/* Query */
	return (get_check(out_val));
}


/*!
 * @brief 選択したアイテムの確認処理のメインルーチン /
 * @param item 選択アイテムID
 * @return 確認がYesならTRUEを返す。
 * @details The item can be negative to mean "item on floor".
 * Hack -- allow user to "prevent" certain choices
 */
static bool get_item_allow(INVENTORY_IDX item)
{
	concptr s;
	object_type *o_ptr;
	if (!command_cmd) return TRUE; /* command_cmd is no longer effective */

	/* Inventory */
	if (item >= 0)
	{
		o_ptr = &p_ptr->inventory_list[item];
	}

	/* Floor */
	else
	{
		o_ptr = &current_floor_ptr->o_list[0 - item];
	}

	/* No inscription */
	if (!o_ptr->inscription) return (TRUE);

	/* Find a '!' */
	s = my_strchr(quark_str(o_ptr->inscription), '!');

	/* Process preventions */
	while (s)
	{
		/* Check the "restriction" */
		if ((s[1] == command_cmd) || (s[1] == '*'))
		{
			/* Verify the choice */
			if (!verify(_("本当に", "Really try"), item)) return (FALSE);
		}

		/* Find another '!' */
		s = my_strchr(s + 1, '!');
	}

	/* Allow it */
	return (TRUE);
}


/*!
 * @brief プレイヤーの所持/装備オブジェクトが正規のものかを返す /
 * Auxiliary function for "get_item()" -- test an index
 * @param i 選択アイテムID
 * @return 正規のIDならばTRUEを返す。
 */
static bool get_item_okay(OBJECT_IDX i)
{
	/* Illegal items */
	if ((i < 0) || (i >= INVEN_TOTAL)) return (FALSE);

	if (select_ring_slot) return is_ring_slot(i);

	/* Verify the item */
	if (!item_tester_okay(&p_ptr->inventory_list[i])) return (FALSE);

	/* Assume okay */
	return (TRUE);
}

/*!
 * @brief プレイヤーがオブジェクトを拾うことができる状態かを返す /
 * Determine whether get_item() can get some item or not
 * @return アイテムを拾えるならばTRUEを返す。
 * @details assuming mode = (USE_EQUIP | USE_INVEN | USE_FLOOR).
 */
bool can_get_item(void)
{
	int j;
	OBJECT_IDX floor_list[23];
	ITEM_NUMBER floor_num = 0;

	for (j = 0; j < INVEN_TOTAL; j++)
		if (item_tester_okay(&p_ptr->inventory_list[j]))
			return TRUE;

	floor_num = scan_floor(floor_list, p_ptr->y, p_ptr->x, 0x03);
	if (floor_num)
		return TRUE;

	return FALSE;
}

/*!
 * @brief オブジェクト選択の汎用関数 /
 * Let the user select an item, save its "index"
 * @param cp 選択したオブジェクトのIDを返す。
 * @param pmt 選択目的のメッセージ
 * @param str 選択できるオブジェクトがない場合のキャンセルメッセージ
 * @param mode オプションフラグ
 * @return プレイヤーによりアイテムが選択されたならTRUEを返す。/
 * Return TRUE only if an acceptable item was chosen by the user.\n
 * @details
 * The selected item must satisfy the "item_tester_hook()" function,\n
 * if that hook is set, and the "item_tester_tval", if that value is set.\n
 *\n
 * All "item_tester" restrictions are cleared before this function returns.\n
 *\n
 * The user is allowed to choose acceptable items from the equipment,\n
 * p_ptr->inventory_list, or floor, respectively, if the proper flag was given,\n
 * and there are any acceptable items in that location.\n
 *\n
 * The equipment or p_ptr->inventory_list are displayed (even if no acceptable\n
 * items are in that location) if the proper flag was given.\n
 *\n
 * If there are no acceptable items available anywhere, and "str" is\n
 * not NULL, then it will be used as the text of a warning message\n
 * before the function returns.\n
 *\n
 * Note that the user must press "-" to specify the item on the floor,\n
 * and there is no way to "examine" the item on the floor, while the\n
 * use of "capital" letters will "examine" an p_ptr->inventory_list/equipment item,\n
 * and prompt for its use.\n
 *\n
 * If a legal item is selected from the p_ptr->inventory_list, we save it in "cp"\n
 * directly (0 to 35), and return TRUE.\n
 *\n
 * If a legal item is selected from the floor, we save it in "cp" as\n
 * a negative (-1 to -511), and return TRUE.\n
 *\n
 * If no item is available, we do nothing to "cp", and we display a\n
 * warning message, using "str" if available, and return FALSE.\n
 *\n
 * If no item is selected, we do nothing to "cp", and return FALSE.\n
 *\n
 * Global "p_ptr->command_new" is used when viewing the p_ptr->inventory_list or equipment\n
 * to allow the user to enter a command while viewing those screens, and\n
 * also to induce "auto-enter" of stores, and other such stuff.\n
 *\n
 * Global "p_ptr->command_see" may be set before calling this function to start\n
 * out in "browse" mode.  It is cleared before this function returns.\n
 *\n
 * Global "p_ptr->command_wrk" is used to choose between equip/inven listings.\n
 * If it is TRUE then we are viewing p_ptr->inventory_list, else equipment.\n
 *\n
 * We always erase the prompt when we are done, leaving a blank line,\n
 * or a warning message, if appropriate, if no items are available.\n
 */
bool get_item(OBJECT_IDX *cp, concptr pmt, concptr str, BIT_FLAGS mode)
{
	OBJECT_IDX this_o_idx, next_o_idx = 0;

	char which = ' ';

	int j;
	OBJECT_IDX k;
	OBJECT_IDX i1, i2;
	OBJECT_IDX e1, e2;

	bool done, item;

	bool oops = FALSE;

	bool equip = FALSE;
	bool inven = FALSE;
	bool floor = FALSE;

	bool allow_floor = FALSE;

	bool toggle = FALSE;

	char tmp_val[160];
	char out_val[160];

	int menu_line = (use_menu ? 1 : 0);
	int max_inven = 0;
	int max_equip = 0;

	static char prev_tag = '\0';
	char cur_tag = '\0';

	if (easy_floor || use_menu) return get_item_floor(cp, pmt, str, mode);

	/* Extract args */
	if (mode & USE_EQUIP) equip = TRUE;
	if (mode & USE_INVEN) inven = TRUE;
	if (mode & USE_FLOOR) floor = TRUE;

	/* Get the item index */
	if (repeat_pull(cp))
	{
		/* the_force */
		if (mode & USE_FORCE && (*cp == INVEN_FORCE))
		{
			item_tester_tval = 0;
			item_tester_hook = NULL;
			command_cmd = 0; /* Hack -- command_cmd is no longer effective */
			return (TRUE);
		}

		/* Floor item? */
		else if (floor && (*cp < 0))
		{
			object_type *o_ptr;

			/* Special index */
			k = 0 - (*cp);
			o_ptr = &current_floor_ptr->o_list[k];

			/* Validate the item */
			if (item_tester_okay(o_ptr) || (mode & USE_FULL))
			{
				/* Forget restrictions */
				item_tester_tval = 0;
				item_tester_hook = NULL;
				command_cmd = 0; /* Hack -- command_cmd is no longer effective */

				/* Success */
				return TRUE;
			}
		}

		else if ((inven && (*cp >= 0) && (*cp < INVEN_PACK)) ||
		         (equip && (*cp >= INVEN_RARM) && (*cp < INVEN_TOTAL)))
		{
			if (prev_tag && command_cmd)
			{
				/* Look up the tag and validate the item */
				if (!get_tag(&k, prev_tag, (*cp >= INVEN_RARM) ? USE_EQUIP : USE_INVEN)) /* Reject */;
				else if ((k < INVEN_RARM) ? !inven : !equip) /* Reject */;
				else if (!get_item_okay(k)) /* Reject */;
				else
				{
					/* Accept that choice */
					(*cp) = k;

					/* Forget restrictions */
					item_tester_tval = 0;
					item_tester_hook = NULL;
					command_cmd = 0; /* Hack -- command_cmd is no longer effective */

					/* Success */
					return TRUE;
				}

				prev_tag = '\0'; /* prev_tag is no longer effective */
			}

			/* Verify the item */
			else if (get_item_okay(*cp))
			{
				/* Forget restrictions */
				item_tester_tval = 0;
				item_tester_hook = NULL;
				command_cmd = 0; /* Hack -- command_cmd is no longer effective */

				/* Success */
				return TRUE;
			}
		}
	}
	msg_print(NULL);

	/* Not done */
	done = FALSE;

	/* No item selected */
	item = FALSE;


	/* Full p_ptr->inventory_list */
	i1 = 0;
	i2 = INVEN_PACK - 1;

	/* Forbid p_ptr->inventory_list */
	if (!inven) i2 = -1;
	else if (use_menu)
	{
		for (j = 0; j < INVEN_PACK; j++)
			if (item_tester_okay(&p_ptr->inventory_list[j]) || (mode & USE_FULL)) max_inven++;
	}

	/* Restrict p_ptr->inventory_list indexes */
	while ((i1 <= i2) && (!get_item_okay(i1))) i1++;
	while ((i1 <= i2) && (!get_item_okay(i2))) i2--;


	/* Full equipment */
	e1 = INVEN_RARM;
	e2 = INVEN_TOTAL - 1;

	/* Forbid equipment */
	if (!equip) e2 = -1;
	else if (use_menu)
	{
		for (j = INVEN_RARM; j < INVEN_TOTAL; j++)
			if (select_ring_slot ? is_ring_slot(j) : item_tester_okay(&p_ptr->inventory_list[j]) || (mode & USE_FULL)) max_equip++;
		if (p_ptr->ryoute && !(mode & IGNORE_BOTHHAND_SLOT)) max_equip++;
	}

	/* Restrict equipment indexes */
	while ((e1 <= e2) && (!get_item_okay(e1))) e1++;
	while ((e1 <= e2) && (!get_item_okay(e2))) e2--;

	if (equip && p_ptr->ryoute && !(mode & IGNORE_BOTHHAND_SLOT))
	{
		if (p_ptr->migite)
		{
			if (e2 < INVEN_LARM) e2 = INVEN_LARM;
		}
		else if (p_ptr->hidarite) e1 = INVEN_RARM;
	}


	/* Restrict floor usage */
	if (floor)
	{
		/* Scan all objects in the grid */
		for (this_o_idx = current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].o_idx; this_o_idx; this_o_idx = next_o_idx)
		{
			object_type *o_ptr;
			o_ptr = &current_floor_ptr->o_list[this_o_idx];
			next_o_idx = o_ptr->next_o_idx;

			/* Accept the item on the floor if legal */
			if ((item_tester_okay(o_ptr) || (mode & USE_FULL)) && (o_ptr->marked & OM_FOUND)) allow_floor = TRUE;
		}
	}

	/* Require at least one legal choice */
	if (!allow_floor && (i1 > i2) && (e1 > e2))
	{
		/* Cancel p_ptr->command_see */
		command_see = FALSE;
		oops = TRUE;
		done = TRUE;

		if (mode & USE_FORCE) {
		    *cp = INVEN_FORCE;
		    item = TRUE;
		}
	}

	/* Analyze choices */
	else
	{
		/* Hack -- Start on equipment if requested */
		if (command_see && command_wrk && equip)
		{
			command_wrk = TRUE;
		}

		/* Use p_ptr->inventory_list if allowed */
		else if (inven)
		{
			command_wrk = FALSE;
		}

		/* Use equipment if allowed */
		else if (equip)
		{
			command_wrk = TRUE;
		}

		/* Use p_ptr->inventory_list for floor */
		else
		{
			command_wrk = FALSE;
		}
	}


	/*
	 * 追加オプション(always_show_list)が設定されている場合は常に一覧を表示する
	 */
	if ((always_show_list == TRUE) || use_menu) command_see = TRUE;

	/* Hack -- start out in "display" mode */
	if (command_see)
	{
		screen_save();
	}


	/* Repeat until done */
	while (!done)
	{
		COMMAND_CODE get_item_label = 0;

		/* Show choices */
		int ni = 0;
		int ne = 0;

		/* Scan windows */
		for (j = 0; j < 8; j++)
		{
			/* Unused */
			if (!angband_term[j]) continue;

			/* Count windows displaying inven */
			if (window_flag[j] & (PW_INVEN)) ni++;

			/* Count windows displaying equip */
			if (window_flag[j] & (PW_EQUIP)) ne++;
		}

		/* Toggle if needed */
		if ((command_wrk && ni && !ne) || (!command_wrk && !ni && ne))
		{
			/* Toggle */
			toggle_inven_equip();

			/* Track toggles */
			toggle = !toggle;
		}

		p_ptr->window |= (PW_INVEN | PW_EQUIP);
		handle_stuff();

		/* Inventory screen */
		if (!command_wrk)
		{
			/* Redraw if needed */
			if (command_see) get_item_label = show_inven(menu_line, mode);
		}

		/* Equipment screen */
		else
		{
			/* Redraw if needed */
			if (command_see) get_item_label = show_equip(menu_line, mode);
		}

		/* Viewing p_ptr->inventory_list */
		if (!command_wrk)
		{
			/* Begin the prompt */
			sprintf(out_val, _("持ち物:", "Inven:"));

			/* Some legal items */
			if ((i1 <= i2) && !use_menu)
			{
				/* Build the prompt */
				sprintf(tmp_val, _("%c-%c,'(',')',", " %c-%c,'(',')',"),
					index_to_label(i1), index_to_label(i2));

				/* Append */
				strcat(out_val, tmp_val);
			}

			/* Indicate ability to "view" */
			if (!command_see && !use_menu) strcat(out_val, _(" '*'一覧,", " * to see,"));

			/* Append */
			if (equip) strcat(out_val, format(_(" %s 装備品,", " %s for Equip,"), use_menu ? _("'4'or'6'", "4 or 6") : _("'/'", "/")));
		}

		/* Viewing equipment */
		else
		{
			/* Begin the prompt */
			sprintf(out_val, _("装備品:", "Equip:"));

			/* Some legal items */
			if ((e1 <= e2) && !use_menu)
			{
				/* Build the prompt */
				sprintf(tmp_val, _("%c-%c,'(',')',", " %c-%c,'(',')',"),
					index_to_label(e1), index_to_label(e2));

				/* Append */
				strcat(out_val, tmp_val);
			}

			/* Indicate ability to "view" */
			if (!command_see && !use_menu) strcat(out_val, _(" '*'一覧,", " * to see,"));

			/* Append */
			if (inven) strcat(out_val, format(_(" %s 持ち物,", " %s for Inven,"), use_menu ? _("'4'or'6'", "4 or 6") : _("'/'", "'/'")));
		}

		/* Indicate legality of the "floor" item */
		if (allow_floor) strcat(out_val, _(" '-'床上,", " - for floor,"));
		if (mode & USE_FORCE) strcat(out_val, _(" 'w'練気術,", " w for the Force,"));

		/* Finish the prompt */
		strcat(out_val, " ESC");

		/* Build the prompt */
		sprintf(tmp_val, "(%s) %s", out_val, pmt);

		/* Show the prompt */
		prt(tmp_val, 0, 0);

		/* Get a key */
		which = inkey();

		if (use_menu)
		{
		int max_line = (command_wrk ? max_equip : max_inven);
		switch (which)
		{
			case ESCAPE:
			case 'z':
			case 'Z':
			case '0':
			{
				done = TRUE;
				break;
			}

			case '8':
			case 'k':
			case 'K':
			{
				menu_line += (max_line - 1);
				break;
			}

			case '2':
			case 'j':
			case 'J':
			{
				menu_line++;
				break;
			}

			case '4':
			case '6':
			case 'h':
			case 'H':
			case 'l':
			case 'L':
			{
				/* Verify legality */
				if (!inven || !equip)
				{
					bell();
					break;
				}

				/* Hack -- Fix screen */
				if (command_see)
				{
					screen_load();
					screen_save();
				}

				/* Switch inven/equip */
				command_wrk = !command_wrk;
				max_line = (command_wrk ? max_equip : max_inven);
				if (menu_line > max_line) menu_line = max_line;

				/* Need to redraw */
				break;
			}

			case 'x':
			case 'X':
			case '\r':
			case '\n':
			{
				if (command_wrk == USE_FLOOR)
				{
					/* Special index */
					(*cp) = -get_item_label;
				}
				else
				{
					/* Validate the item */
					if (!get_item_okay(get_item_label))
					{
						bell();
						break;
					}

					/* Allow player to "refuse" certain actions */
					if (!get_item_allow(get_item_label))
					{
						done = TRUE;
						break;
					}

					/* Accept that choice */
					(*cp) = get_item_label;
				}

				item = TRUE;
				done = TRUE;
				break;
			}
			case 'w':
			{
				if (mode & USE_FORCE) {
					*cp = INVEN_FORCE;
					item = TRUE;
					done = TRUE;
					break;
				}
			}
		}
		if (menu_line > max_line) menu_line -= max_line;
		}
		else
		{
		/* Parse it */
		switch (which)
		{
			case ESCAPE:
			{
				done = TRUE;
				break;
			}

			case '*':
			case '?':
			case ' ':
			{
				/* Hide the list */
				if (command_see)
				{
					/* Flip flag */
					command_see = FALSE;
					screen_load();
				}

				/* Show the list */
				else
				{
					screen_save();

					/* Flip flag */
					command_see = TRUE;
				}
				break;
			}

			case '/':
			{
				/* Verify legality */
				if (!inven || !equip)
				{
					bell();
					break;
				}

				/* Hack -- Fix screen */
				if (command_see)
				{
					screen_load();
					screen_save();
				}

				/* Switch inven/equip */
				command_wrk = !command_wrk;

				/* Need to redraw */
				break;
			}

			case '-':
			{
				/* Use floor item */
				if (allow_floor)
				{
					/* Scan all objects in the grid */
					for (this_o_idx = current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].o_idx; this_o_idx; this_o_idx = next_o_idx)
					{
						object_type *o_ptr;
						o_ptr = &current_floor_ptr->o_list[this_o_idx];
						next_o_idx = o_ptr->next_o_idx;

						/* Validate the item */
						if (!item_tester_okay(o_ptr) && !(mode & USE_FULL)) continue;

						/* Special index */
						k = 0 - this_o_idx;

						/* Verify the item (if required) */
						if (other_query_flag && !verify(_("本当に", "Try"), k)) continue;

						/* Allow player to "refuse" certain actions */
						if (!get_item_allow(k)) continue;

						/* Accept that choice */
						(*cp) = k;
						item = TRUE;
						done = TRUE;
						break;
					}

					/* Outer break */
					if (done) break;
				}

				bell();
				break;
			}

			case '0':
			case '1': case '2': case '3':
			case '4': case '5': case '6':
			case '7': case '8': case '9':
			{
				/* Look up the tag */
				if (!get_tag(&k, which, command_wrk ? USE_EQUIP : USE_INVEN))
				{
					bell();
					break;
				}

				/* Hack -- Validate the item */
				if ((k < INVEN_RARM) ? !inven : !equip)
				{
					bell();
					break;
				}

				/* Validate the item */
				if (!get_item_okay(k))
				{
					bell();
					break;
				}

				/* Allow player to "refuse" certain actions */
				if (!get_item_allow(k))
				{
					done = TRUE;
					break;
				}

				/* Accept that choice */
				(*cp) = k;
				item = TRUE;
				done = TRUE;
				cur_tag = which;
				break;
			}

#if 0
			case '\n':
			case '\r':
			{
				/* Choose "default" p_ptr->inventory_list item */
				if (!command_wrk)
				{
					k = ((i1 == i2) ? i1 : -1);
				}

				/* Choose "default" equipment item */
				else
				{
					k = ((e1 == e2) ? e1 : -1);
				}

				/* Validate the item */
				if (!get_item_okay(k))
				{
					bell();
					break;
				}

				/* Allow player to "refuse" certain actions */
				if (!get_item_allow(k))
				{
					done = TRUE;
					break;
				}

				/* Accept that choice */
				(*cp) = k;
				item = TRUE;
				done = TRUE;
				break;
			}
#endif

			case 'w':
			{
				if (mode & USE_FORCE) {
					*cp = INVEN_FORCE;
					item = TRUE;
					done = TRUE;
					break;
				}

				/* Fall through */
			}

			default:
			{
				int ver;
				bool not_found = FALSE;

				/* Look up the alphabetical tag */
				if (!get_tag(&k, which, command_wrk ? USE_EQUIP : USE_INVEN))
				{
					not_found = TRUE;
				}

				/* Hack -- Validate the item */
				else if ((k < INVEN_RARM) ? !inven : !equip)
				{
					not_found = TRUE;
				}

				/* Validate the item */
				else if (!get_item_okay(k))
				{
					not_found = TRUE;
				}

				if (!not_found)
				{
					/* Accept that choice */
					(*cp) = k;
					item = TRUE;
					done = TRUE;
					cur_tag = which;
					break;
				}

				/* Extract "query" setting */
				ver = isupper(which);
				which = (char)tolower(which);

				/* Convert letter to p_ptr->inventory_list index */
				if (!command_wrk)
				{
					if (which == '(') k = i1;
					else if (which == ')') k = i2;
					else k = label_to_inven(which);
				}

				/* Convert letter to equipment index */
				else
				{
					if (which == '(') k = e1;
					else if (which == ')') k = e2;
					else k = label_to_equip(which);
				}

				/* Validate the item */
				if (!get_item_okay(k))
				{
					bell();
					break;
				}

				/* Verify the item */
				if (ver && !verify(_("本当に", "Try"), k))
				{
					done = TRUE;
					break;
				}

				/* Allow player to "refuse" certain actions */
				if (!get_item_allow(k))
				{
					done = TRUE;
					break;
				}

				/* Accept that choice */
				(*cp) = k;
				item = TRUE;
				done = TRUE;
				break;
			}
		}
		}
	}


	/* Fix the screen if necessary */
	if (command_see)
	{
		screen_load();

		/* Hack -- Cancel "display" */
		command_see = FALSE;
	}


	/* Forget the item_tester_tval restriction */
	item_tester_tval = 0;

	/* Forget the item_tester_hook restriction */
	item_tester_hook = NULL;


	/* Clean up  'show choices' */
	/* Toggle again if needed */
	if (toggle) toggle_inven_equip();

	p_ptr->window |= (PW_INVEN | PW_EQUIP);
	handle_stuff();

	/* Clear the prompt line */
	prt("", 0, 0);

	/* Warning if needed */
	if (oops && str) msg_print(str);

	if (item)
	{
		repeat_push(*cp);
		if (command_cmd) prev_tag = cur_tag;
		command_cmd = 0; /* Hack -- command_cmd is no longer effective */
	}
	return (item);
}

/*
 * Choose an item and get auto-picker entry from it.
 */
object_type *choose_object(OBJECT_IDX *idx, concptr q, concptr s, BIT_FLAGS option)
{
	OBJECT_IDX item;
	if (!get_item(&item, q, s, option)) return NULL;
	if (idx) *idx = item;

	if (item == INVEN_FORCE) return NULL;

	/* Get the item (in the pack) */
	else if (item >= 0) return &p_ptr->inventory_list[item];

	/* Get the item (on the floor) */
	else return &current_floor_ptr->o_list[0 - item];
}


/*!
 * @brief 床下に落ちているオブジェクトの数を返す / scan_floor
 * @param items オブジェクトのIDリストを返すための配列参照ポインタ
 * @param y 走査するフロアのY座標
 * @param x 走査するフロアのX座標
 * @param mode オプションフラグ
 * @return 対象のマスに落ちているアイテム数
 * @details
 * Return a list of o_list[] indexes of items at the given current_floor_ptr->grid_array
 * location. Valid flags are:
 *
 *		mode & 0x01 -- Item tester
 *		mode & 0x02 -- Marked items only
 *		mode & 0x04 -- Stop after first
 */
ITEM_NUMBER scan_floor(OBJECT_IDX *items, POSITION y, POSITION x, BIT_FLAGS mode)
{
	OBJECT_IDX this_o_idx, next_o_idx;

	ITEM_NUMBER num = 0;

	/* Sanity */
	if (!in_bounds(y, x)) return 0;

	/* Scan all objects in the grid */
	for (this_o_idx = current_floor_ptr->grid_array[y][x].o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		object_type *o_ptr;
		o_ptr = &current_floor_ptr->o_list[this_o_idx];
		next_o_idx = o_ptr->next_o_idx;

		/* Item tester */
		if ((mode & 0x01) && !item_tester_okay(o_ptr)) continue;

		/* Marked */
		if ((mode & 0x02) && !(o_ptr->marked & OM_FOUND)) continue;

		/* Accept this item */
		/* XXX Hack -- Enforce limit */
		if (num < 23)
			items[num] = this_o_idx;

		num++;

		/* Only one */
		if (mode & 0x04) break;
	}
	return num;
}


/*!
 * @brief 床下に落ちているアイテムの一覧を返す / Display a list of the items on the floor at the given location.
 * @param target_item カーソルの初期値
 * @param y 走査するフロアのY座標
 * @param x 走査するフロアのX座標
 * @param min_width 表示の長さ
 * @return 選択したアイテムの添え字
 * @details
 */
COMMAND_CODE show_floor(int target_item, POSITION y, POSITION x, TERM_LEN *min_width)
{
	COMMAND_CODE i, m;
	int j, k, l;
	int col, len;

	object_type *o_ptr;

	GAME_TEXT o_name[MAX_NLEN];
	char tmp_val[80];

	COMMAND_CODE out_index[23];
	TERM_COLOR out_color[23];
	char out_desc[23][MAX_NLEN];
	COMMAND_CODE target_item_label = 0;

	OBJECT_IDX floor_list[23];
	ITEM_NUMBER floor_num;
	TERM_LEN wid, hgt;
	char floor_label[52 + 1];

	bool dont_need_to_show_weights = TRUE;

	Term_get_size(&wid, &hgt);

	/* Default length */
	len = MAX((*min_width), 20);

	/* Scan for objects in the grid, using item_tester_okay() */
	floor_num = scan_floor(floor_list, y, x, 0x03);

	/* Display the floor objects */
	for (k = 0, i = 0; i < floor_num && i < 23; i++)
	{
		o_ptr = &current_floor_ptr->o_list[floor_list[i]];

		object_desc(o_name, o_ptr, 0);

		/* Save the index */
		out_index[k] = i;

		/* Acquire p_ptr->inventory_list color */
		out_color[k] = tval_to_attr[o_ptr->tval & 0x7F];

		/* Save the object description */
		strcpy(out_desc[k], o_name);

		/* Find the predicted "line length" */
		l = strlen(out_desc[k]) + 5;

		/* Be sure to account for the weight */
		if (show_weights) l += 9;

		if (o_ptr->tval != TV_GOLD) dont_need_to_show_weights = FALSE;

		/* Maintain the maximum length */
		if (l > len) len = l;

		/* Advance to next "line" */
		k++;
	}

	if (show_weights && dont_need_to_show_weights) len -= 9;

	/* Save width */
	*min_width = len;

	/* Find the column to start in */
	col = (len > wid - 4) ? 0 : (wid - len - 1);

	prepare_label_string_floor(floor_label, floor_list, floor_num);

	/* Output each entry */
	for (j = 0; j < k; j++)
	{
		m = floor_list[out_index[j]];
		o_ptr = &current_floor_ptr->o_list[m];

		/* Clear the line */
		prt("", j + 1, col ? col - 2 : col);

		if (use_menu && target_item)
		{
			if (j == (target_item-1))
			{
				strcpy(tmp_val, _("》", "> "));
				target_item_label = m;
			}
			else strcpy(tmp_val, "   ");
		}
		else
		{
			/* Prepare an index --(-- */
			sprintf(tmp_val, "%c)", floor_label[j]);
		}

		/* Clear the line with the (possibly indented) index */
		put_str(tmp_val, j + 1, col);

		/* Display the entry itself */
		c_put_str(out_color[j], out_desc[j], j + 1, col + 3);

		/* Display the weight if needed */
		if (show_weights && (o_ptr->tval != TV_GOLD))
		{
			int wgt = o_ptr->weight * o_ptr->number;
#ifdef JP
			sprintf(tmp_val, "%3d.%1d kg", lbtokg1(wgt) , lbtokg2(wgt) );
#else
			sprintf(tmp_val, "%3d.%1d lb", wgt / 10, wgt % 10);
#endif

			prt(tmp_val, j + 1, wid - 9);
		}
	}

	/* Make a "shadow" below the list (only if needed) */
	if (j && (j < 23)) prt("", j + 1, col ? col - 2 : col);

	return target_item_label;
}

/*!
 * @brief オブジェクト選択の汎用関数(床上アイテム用) /
 * Let the user select an item, save its "index"
 * @param cp 選択したオブジェクトのIDを返す。
 * @param pmt 選択目的のメッセージ
 * @param str 選択できるオブジェクトがない場合のキャンセルメッセージ
 * @param mode オプションフラグ
 * @return プレイヤーによりアイテムが選択されたならTRUEを返す。/
 */
bool get_item_floor(COMMAND_CODE *cp, concptr pmt, concptr str, BIT_FLAGS mode)
{
	char n1 = ' ', n2 = ' ', which = ' ';

	int j;
	COMMAND_CODE i1, i2;
	COMMAND_CODE e1, e2;
	COMMAND_CODE k;

	bool done, item;

	bool oops = FALSE;

	/* Extract args */
	bool equip = (mode & USE_EQUIP) ? TRUE : FALSE;
	bool inven = (mode & USE_INVEN) ? TRUE : FALSE;
	bool floor = (mode & USE_FLOOR) ? TRUE : FALSE;
	bool force = (mode & USE_FORCE) ? TRUE : FALSE;

	bool allow_equip = FALSE;
	bool allow_inven = FALSE;
	bool allow_floor = FALSE;

	bool toggle = FALSE;

	char tmp_val[160];
	char out_val[160];

	ITEM_NUMBER floor_num;
	OBJECT_IDX floor_list[23];
	int floor_top = 0;
	TERM_LEN min_width = 0;

	int menu_line = (use_menu ? 1 : 0);
	int max_inven = 0;
	int max_equip = 0;

	static char prev_tag = '\0';
	char cur_tag = '\0';

	/* Get the item index */
	if (repeat_pull(cp))
	{
		/* the_force */
		if (force && (*cp == INVEN_FORCE))
		{
			item_tester_tval = 0;
			item_tester_hook = NULL;
			command_cmd = 0; /* Hack -- command_cmd is no longer effective */
			return (TRUE);
		}

		/* Floor item? */
		else if (floor && (*cp < 0))
		{
			if (prev_tag && command_cmd)
			{
				/* Scan all objects in the grid */
				floor_num = scan_floor(floor_list, p_ptr->y, p_ptr->x, 0x03);

				/* Look up the tag */
				if (get_tag_floor(&k, prev_tag, floor_list, floor_num))
				{
					/* Accept that choice */
					(*cp) = 0 - floor_list[k];

					/* Forget restrictions */
					item_tester_tval = 0;
					item_tester_hook = NULL;
					command_cmd = 0; /* Hack -- command_cmd is no longer effective */

					/* Success */
					return TRUE;
				}

				prev_tag = '\0'; /* prev_tag is no longer effective */
			}

			/* Validate the item */
			else if (item_tester_okay(&current_floor_ptr->o_list[0 - (*cp)]) || (mode & USE_FULL))
			{
				/* Forget restrictions */
				item_tester_tval = 0;
				item_tester_hook = NULL;
				command_cmd = 0; /* Hack -- command_cmd is no longer effective */

				/* Success */
				return TRUE;
			}
		}

		else if ((inven && (*cp >= 0) && (*cp < INVEN_PACK)) ||
		         (equip && (*cp >= INVEN_RARM) && (*cp < INVEN_TOTAL)))
		{
			if (prev_tag && command_cmd)
			{
				/* Look up the tag and validate the item */
				if (!get_tag(&k, prev_tag, (*cp >= INVEN_RARM) ? USE_EQUIP : USE_INVEN)) /* Reject */;
				else if ((k < INVEN_RARM) ? !inven : !equip) /* Reject */;
				else if (!get_item_okay(k)) /* Reject */;
				else
				{
					/* Accept that choice */
					(*cp) = k;

					/* Forget restrictions */
					item_tester_tval = 0;
					item_tester_hook = NULL;
					command_cmd = 0; /* Hack -- command_cmd is no longer effective */

					/* Success */
					return TRUE;
				}

				prev_tag = '\0'; /* prev_tag is no longer effective */
			}

			/* Verify the item */
			else if (get_item_okay(*cp))
			{
				/* Forget restrictions */
				item_tester_tval = 0;
				item_tester_hook = NULL;
				command_cmd = 0; /* Hack -- command_cmd is no longer effective */

				/* Success */
				return TRUE;
			}
		}
	}

	msg_print(NULL);


	/* Not done */
	done = FALSE;

	/* No item selected */
	item = FALSE;


	/* Full p_ptr->inventory_list */
	i1 = 0;
	i2 = INVEN_PACK - 1;

	/* Forbid p_ptr->inventory_list */
	if (!inven) i2 = -1;
	else if (use_menu)
	{
		for (j = 0; j < INVEN_PACK; j++)
			if (item_tester_okay(&p_ptr->inventory_list[j]) || (mode & USE_FULL)) max_inven++;
	}

	/* Restrict p_ptr->inventory_list indexes */
	while ((i1 <= i2) && (!get_item_okay(i1))) i1++;
	while ((i1 <= i2) && (!get_item_okay(i2))) i2--;


	/* Full equipment */
	e1 = INVEN_RARM;
	e2 = INVEN_TOTAL - 1;

	/* Forbid equipment */
	if (!equip) e2 = -1;
	else if (use_menu)
	{
		for (j = INVEN_RARM; j < INVEN_TOTAL; j++)
			if (select_ring_slot ? is_ring_slot(j) : item_tester_okay(&p_ptr->inventory_list[j]) || (mode & USE_FULL)) max_equip++;
		if (p_ptr->ryoute && !(mode & IGNORE_BOTHHAND_SLOT)) max_equip++;
	}

	/* Restrict equipment indexes */
	while ((e1 <= e2) && (!get_item_okay(e1))) e1++;
	while ((e1 <= e2) && (!get_item_okay(e2))) e2--;

	if (equip && p_ptr->ryoute && !(mode & IGNORE_BOTHHAND_SLOT))
	{
		if (p_ptr->migite)
		{
			if (e2 < INVEN_LARM) e2 = INVEN_LARM;
		}
		else if (p_ptr->hidarite) e1 = INVEN_RARM;
	}


	/* Count "okay" floor items */
	floor_num = 0;

	/* Restrict floor usage */
	if (floor)
	{
		/* Scan all objects in the grid */
		floor_num = scan_floor(floor_list, p_ptr->y, p_ptr->x, 0x03);
	}

	/* Accept p_ptr->inventory_list */
	if (i1 <= i2) allow_inven = TRUE;

	/* Accept equipment */
	if (e1 <= e2) allow_equip = TRUE;

	/* Accept floor */
	if (floor_num) allow_floor = TRUE;

	/* Require at least one legal choice */
	if (!allow_inven && !allow_equip && !allow_floor)
	{
		/* Cancel p_ptr->command_see */
		command_see = FALSE;
		oops = TRUE;
		done = TRUE;

		if (force) {
		    *cp = INVEN_FORCE;
		    item = TRUE;
		}
	}

	/* Analyze choices */
	else
	{
		/* Hack -- Start on equipment if requested */
		if (command_see && (command_wrk == (USE_EQUIP))
			&& allow_equip)
		{
			command_wrk = (USE_EQUIP);
		}

		/* Use p_ptr->inventory_list if allowed */
		else if (allow_inven)
		{
			command_wrk = (USE_INVEN);
		}

		/* Use equipment if allowed */
		else if (allow_equip)
		{
			command_wrk = (USE_EQUIP);
		}

		/* Use floor if allowed */
		else if (allow_floor)
		{
			command_wrk = (USE_FLOOR);
		}
	}

	/*
	 * 追加オプション(always_show_list)が設定されている場合は常に一覧を表示する
	 */
	if ((always_show_list == TRUE) || use_menu) command_see = TRUE;

	/* Hack -- start out in "display" mode */
	if (command_see)
	{
		screen_save();
	}

	/* Repeat until done */
	while (!done)
	{
		COMMAND_CODE get_item_label = 0;

		/* Show choices */
		int ni = 0;
		int ne = 0;

		/* Scan windows */
		for (j = 0; j < 8; j++)
		{
			/* Unused */
			if (!angband_term[j]) continue;

			/* Count windows displaying inven */
			if (window_flag[j] & (PW_INVEN)) ni++;

			/* Count windows displaying equip */
			if (window_flag[j] & (PW_EQUIP)) ne++;
		}

		/* Toggle if needed */
		if ((command_wrk == (USE_EQUIP) && ni && !ne) ||
		    (command_wrk == (USE_INVEN) && !ni && ne))
		{
			/* Toggle */
			toggle_inven_equip();

			/* Track toggles */
			toggle = !toggle;
		}

		p_ptr->window |= (PW_INVEN | PW_EQUIP);
		handle_stuff();

		/* Inventory screen */
		if (command_wrk == (USE_INVEN))
		{
			/* Extract the legal requests */
			n1 = I2A(i1);
			n2 = I2A(i2);

			/* Redraw if needed */
			if (command_see) get_item_label = show_inven(menu_line, mode);
		}

		/* Equipment screen */
		else if (command_wrk == (USE_EQUIP))
		{
			/* Extract the legal requests */
			n1 = I2A(e1 - INVEN_RARM);
			n2 = I2A(e2 - INVEN_RARM);

			/* Redraw if needed */
			if (command_see) get_item_label = show_equip(menu_line, mode);
		}

		/* Floor screen */
		else if (command_wrk == (USE_FLOOR))
		{
			j = floor_top;
			k = MIN(floor_top + 23, floor_num) - 1;

			/* Extract the legal requests */
			n1 = I2A(j - floor_top);
			n2 = I2A(k - floor_top);

			/* Redraw if needed */
			if (command_see) get_item_label = show_floor(menu_line, p_ptr->y, p_ptr->x, &min_width);
		}

		/* Viewing p_ptr->inventory_list */
		if (command_wrk == (USE_INVEN))
		{
			/* Begin the prompt */
			sprintf(out_val, _("持ち物:", "Inven:"));

			if (!use_menu)
			{
				/* Build the prompt */
				sprintf(tmp_val, _("%c-%c,'(',')',", " %c-%c,'(',')',"),
					index_to_label(i1), index_to_label(i2));

				/* Append */
				strcat(out_val, tmp_val);
			}

			/* Indicate ability to "view" */
			if (!command_see && !use_menu) strcat(out_val, _(" '*'一覧,", " * to see,"));

			/* Append */
			if (allow_equip)
			{
				if (!use_menu)
					strcat(out_val, _(" '/' 装備品,", " / for Equip,"));
				else if (allow_floor)
					strcat(out_val, _(" '6' 装備品,", " 6 for Equip,"));
				else
					strcat(out_val, _(" '4'or'6' 装備品,", " 4 or 6 for Equip,"));
			}

			/* Append */
			if (allow_floor)
			{
				if (!use_menu)
					strcat(out_val, _(" '-'床上,", " - for floor,"));
				else if (allow_equip)
					strcat(out_val, _(" '4' 床上,", " 4 for floor,"));
				else
					strcat(out_val, _(" '4'or'6' 床上,", " 4 or 6 for floor,"));
			}
		}

		/* Viewing equipment */
		else if (command_wrk == (USE_EQUIP))
		{
			/* Begin the prompt */
			sprintf(out_val, _("装備品:", "Equip:"));

			if (!use_menu)
			{
				/* Build the prompt */
				sprintf(tmp_val, _("%c-%c,'(',')',", " %c-%c,'(',')',"),
					index_to_label(e1), index_to_label(e2));

				/* Append */
				strcat(out_val, tmp_val);
			}

			/* Indicate ability to "view" */
			if (!command_see && !use_menu) strcat(out_val, _(" '*'一覧,", " * to see,"));

			/* Append */
			if (allow_inven)
			{
				if (!use_menu)
					strcat(out_val, _(" '/' 持ち物,", " / for Inven,"));
				else if (allow_floor)
					strcat(out_val, _(" '4' 持ち物,", " 4 for Inven,"));
				else
					strcat(out_val, _(" '4'or'6' 持ち物,", " 4 or 6 for Inven,"));
			}

			/* Append */
			if (allow_floor)
			{
				if (!use_menu)
					strcat(out_val, _(" '-'床上,", " - for floor,"));
				else if (allow_inven)
					strcat(out_val, _(" '6' 床上,", " 6 for floor,"));
				else
					strcat(out_val, _(" '4'or'6' 床上,", " 4 or 6 for floor,"));
			}
		}

		/* Viewing floor */
		else if (command_wrk == (USE_FLOOR))
		{
			/* Begin the prompt */
			sprintf(out_val, _("床上:", "Floor:"));

			if (!use_menu)
			{
				/* Build the prompt */
				sprintf(tmp_val, _("%c-%c,'(',')',", " %c-%c,'(',')',"), n1, n2);

				/* Append */
				strcat(out_val, tmp_val);
			}

			/* Indicate ability to "view" */
			if (!command_see && !use_menu) strcat(out_val, _(" '*'一覧,", " * to see,"));

			if (use_menu)
			{
				if (allow_inven && allow_equip)
				{
					strcat(out_val, _(" '4' 装備品, '6' 持ち物,", " 4 for Equip, 6 for Inven,"));
				}
				else if (allow_inven)
				{
					strcat(out_val, _(" '4'or'6' 持ち物,", " 4 or 6 for Inven,"));
				}
				else if (allow_equip)
				{
					strcat(out_val, _(" '4'or'6' 装備品,", " 4 or 6 for Equip,"));
				}
			}
			/* Append */
			else if (allow_inven)
			{
				strcat(out_val, _(" '/' 持ち物,", " / for Inven,"));
			}
			else if (allow_equip)
			{
				strcat(out_val, _(" '/'装備品,", " / for Equip,"));
			}

			/* Append */
			if (command_see && !use_menu)
			{
				strcat(out_val, _(" Enter 次,", " Enter for scroll down,"));
			}
		}

		/* Append */
		if (force) strcat(out_val, _(" 'w'練気術,", " w for the Force,"));

		/* Finish the prompt */
		strcat(out_val, " ESC");

		/* Build the prompt */
		sprintf(tmp_val, "(%s) %s", out_val, pmt);

		/* Show the prompt */
		prt(tmp_val, 0, 0);

		/* Get a key */
		which = inkey();

		if (use_menu)
		{
		int max_line = 1;
		if (command_wrk == USE_INVEN) max_line = max_inven;
		else if (command_wrk == USE_EQUIP) max_line = max_equip;
		else if (command_wrk == USE_FLOOR) max_line = MIN(23, floor_num);
		switch (which)
		{
			case ESCAPE:
			case 'z':
			case 'Z':
			case '0':
			{
				done = TRUE;
				break;
			}

			case '8':
			case 'k':
			case 'K':
			{
				menu_line += (max_line - 1);
				break;
			}

			case '2':
			case 'j':
			case 'J':
			{
				menu_line++;
				break;
			}

			case '4':
			case 'h':
			case 'H':
			{
				/* Verify legality */
				if (command_wrk == (USE_INVEN))
				{
					if (allow_floor) command_wrk = USE_FLOOR;
					else if (allow_equip) command_wrk = USE_EQUIP;
					else
					{
						bell();
						break;
					}
				}
				else if (command_wrk == (USE_EQUIP))
				{
					if (allow_inven) command_wrk = USE_INVEN;
					else if (allow_floor) command_wrk = USE_FLOOR;
					else
					{
						bell();
						break;
					}
				}
				else if (command_wrk == (USE_FLOOR))
				{
					if (allow_equip) command_wrk = USE_EQUIP;
					else if (allow_inven) command_wrk = USE_INVEN;
					else
					{
						bell();
						break;
					}
				}
				else
				{
					bell();
					break;
				}

				/* Hack -- Fix screen */
				if (command_see)
				{
					screen_load();
					screen_save();
				}

				/* Switch inven/equip */
				if (command_wrk == USE_INVEN) max_line = max_inven;
				else if (command_wrk == USE_EQUIP) max_line = max_equip;
				else if (command_wrk == USE_FLOOR) max_line = MIN(23, floor_num);
				if (menu_line > max_line) menu_line = max_line;

				/* Need to redraw */
				break;
			}

			case '6':
			case 'l':
			case 'L':
			{
				/* Verify legality */
				if (command_wrk == (USE_INVEN))
				{
					if (allow_equip) command_wrk = USE_EQUIP;
					else if (allow_floor) command_wrk = USE_FLOOR;
					else
					{
						bell();
						break;
					}
				}
				else if (command_wrk == (USE_EQUIP))
				{
					if (allow_floor) command_wrk = USE_FLOOR;
					else if (allow_inven) command_wrk = USE_INVEN;
					else
					{
						bell();
						break;
					}
				}
				else if (command_wrk == (USE_FLOOR))
				{
					if (allow_inven) command_wrk = USE_INVEN;
					else if (allow_equip) command_wrk = USE_EQUIP;
					else
					{
						bell();
						break;
					}
				}
				else
				{
					bell();
					break;
				}

				/* Hack -- Fix screen */
				if (command_see)
				{
					screen_load();
					screen_save();
				}

				/* Switch inven/equip */
				if (command_wrk == USE_INVEN) max_line = max_inven;
				else if (command_wrk == USE_EQUIP) max_line = max_equip;
				else if (command_wrk == USE_FLOOR) max_line = MIN(23, floor_num);
				if (menu_line > max_line) menu_line = max_line;

				/* Need to redraw */
				break;
			}

			case 'x':
			case 'X':
			case '\r':
			case '\n':
			{
				if (command_wrk == USE_FLOOR)
				{
					/* Special index */
					(*cp) = -get_item_label;
				}
				else
				{
					/* Validate the item */
					if (!get_item_okay(get_item_label))
					{
						bell();
						break;
					}

					/* Allow player to "refuse" certain actions */
					if (!get_item_allow(get_item_label))
					{
						done = TRUE;
						break;
					}

					/* Accept that choice */
					(*cp) = get_item_label;
				}

				item = TRUE;
				done = TRUE;
				break;
			}
			case 'w':
			{
				if (force) {
					*cp = INVEN_FORCE;
					item = TRUE;
					done = TRUE;
					break;
				}
			}
		}
		if (menu_line > max_line) menu_line -= max_line;
		}
		else
		{
		/* Parse it */
		switch (which)
		{
			case ESCAPE:
			{
				done = TRUE;
				break;
			}

			case '*':
			case '?':
			case ' ':
			{
				/* Hide the list */
				if (command_see)
				{
					/* Flip flag */
					command_see = FALSE;
					screen_load();
				}

				/* Show the list */
				else
				{
					screen_save();

					/* Flip flag */
					command_see = TRUE;
				}
				break;
			}

			case '\n':
			case '\r':
			case '+':
			{
				int i;
				OBJECT_IDX o_idx;
				grid_type *g_ptr = &current_floor_ptr->grid_array[p_ptr->y][p_ptr->x];

				if (command_wrk != (USE_FLOOR)) break;

				/* Get the object being moved. */
				o_idx = g_ptr->o_idx;

				/* Only rotate a pile of two or more objects. */
				if (!(o_idx && current_floor_ptr->o_list[o_idx].next_o_idx)) break;

				/* Remove the first object from the list. */
				excise_object_idx(o_idx);

				/* Find end of the list. */
				i = g_ptr->o_idx;
				while (current_floor_ptr->o_list[i].next_o_idx)
					i = current_floor_ptr->o_list[i].next_o_idx;

				/* Add after the last object. */
				current_floor_ptr->o_list[i].next_o_idx = o_idx;

				/* Re-scan floor list */ 
				floor_num = scan_floor(floor_list, p_ptr->y, p_ptr->x, 0x03);

				/* Hack -- Fix screen */
				if (command_see)
				{
					screen_load();
					screen_save();
				}

				break;
			}

			case '/':
			{
				if (command_wrk == (USE_INVEN))
				{
					if (!allow_equip)
					{
						bell();
						break;
					}
					command_wrk = (USE_EQUIP);
				}
				else if (command_wrk == (USE_EQUIP))
				{
					if (!allow_inven)
					{
						bell();
						break;
					}
					command_wrk = (USE_INVEN);
				}
				else if (command_wrk == (USE_FLOOR))
				{
					if (allow_inven)
					{
						command_wrk = (USE_INVEN);
					}
					else if (allow_equip)
					{
						command_wrk = (USE_EQUIP);
					}
					else
					{
						bell();
						break;
					}
				}

				/* Hack -- Fix screen */
				if (command_see)
				{
					screen_load();
					screen_save();
				}

				/* Need to redraw */
				break;
			}

			case '-':
			{
				if (!allow_floor)
				{
					bell();
					break;
				}

				/*
				 * If we are already examining the floor, and there
				 * is only one item, we will always select it.
				 * If we aren't examining the floor and there is only
				 * one item, we will select it if floor_query_flag
				 * is FALSE.
				 */
				if (floor_num == 1)
				{
					if ((command_wrk == (USE_FLOOR)) || (!carry_query_flag))
					{
						/* Special index */
						k = 0 - floor_list[0];

						/* Allow player to "refuse" certain actions */
						if (!get_item_allow(k))
						{
							done = TRUE;
							break;
						}

						/* Accept that choice */
						(*cp) = k;
						item = TRUE;
						done = TRUE;

						break;
					}
				}

				/* Hack -- Fix screen */
				if (command_see)
				{
					screen_load();
					screen_save();
				}

				command_wrk = (USE_FLOOR);

				break;
			}

			case '0':
			case '1': case '2': case '3':
			case '4': case '5': case '6':
			case '7': case '8': case '9':
			{
				if (command_wrk != USE_FLOOR)
				{
					/* Look up the tag */
					if (!get_tag(&k, which, command_wrk))
					{
						bell();
						break;
					}

					/* Hack -- Validate the item */
					if ((k < INVEN_RARM) ? !inven : !equip)
					{
						bell();
						break;
					}

					/* Validate the item */
					if (!get_item_okay(k))
					{
						bell();
						break;
					}
				}
				else
				{
					/* Look up the alphabetical tag */
					if (get_tag_floor(&k, which, floor_list, floor_num))
					{
						/* Special index */
						k = 0 - floor_list[k];
					}
					else
					{
						bell();
						break;
					}
				}

				/* Allow player to "refuse" certain actions */
				if (!get_item_allow(k))
				{
					done = TRUE;
					break;
				}

				/* Accept that choice */
				(*cp) = k;
				item = TRUE;
				done = TRUE;
				cur_tag = which;
				break;
			}

#if 0
			case '\n':
			case '\r':
			{
				/* Choose "default" p_ptr->inventory_list item */
				if (command_wrk == (USE_INVEN))
				{
					k = ((i1 == i2) ? i1 : -1);
				}

				/* Choose "default" equipment item */
				else if (command_wrk == (USE_EQUIP))
				{
					k = ((e1 == e2) ? e1 : -1);
				}

				/* Choose "default" floor item */
				else if (command_wrk == (USE_FLOOR))
				{
					if (floor_num == 1)
					{
						/* Special index */
						k = 0 - floor_list[0];

						/* Allow player to "refuse" certain actions */
						if (!get_item_allow(k))
						{
							done = TRUE;
							break;
						}

						/* Accept that choice */
						(*cp) = k;
						item = TRUE;
						done = TRUE;
					}
					break;
				}

				/* Validate the item */
				if (!get_item_okay(k))
				{
					bell();
					break;
				}

				/* Allow player to "refuse" certain actions */
				if (!get_item_allow(k))
				{
					done = TRUE;
					break;
				}

				/* Accept that choice */
				(*cp) = k;
				item = TRUE;
				done = TRUE;
				break;
			}
#endif

			case 'w':
			{
				if (force) {
					*cp = INVEN_FORCE;
					item = TRUE;
					done = TRUE;
					break;
				}

				/* Fall through */
			}

			default:
			{
				int ver;

				if (command_wrk != USE_FLOOR)
				{
					bool not_found = FALSE;

					/* Look up the alphabetical tag */
					if (!get_tag(&k, which, command_wrk))
					{
						not_found = TRUE;
					}

					/* Hack -- Validate the item */
					else if ((k < INVEN_RARM) ? !inven : !equip)
					{
						not_found = TRUE;
					}

					/* Validate the item */
					else if (!get_item_okay(k))
					{
						not_found = TRUE;
					}

					if (!not_found)
					{
						/* Accept that choice */
						(*cp) = k;
						item = TRUE;
						done = TRUE;
						cur_tag = which;
						break;
					}
				}
				else
				{
					/* Look up the alphabetical tag */
					if (get_tag_floor(&k, which, floor_list, floor_num))
					{
						/* Special index */
						k = 0 - floor_list[k];

						/* Accept that choice */
						(*cp) = k;
						item = TRUE;
						done = TRUE;
						cur_tag = which;
						break;
					}
				}

				/* Extract "query" setting */
				ver = isupper(which);
				which = (char)tolower(which);

				/* Convert letter to p_ptr->inventory_list index */
				if (command_wrk == (USE_INVEN))
				{
					if (which == '(') k = i1;
					else if (which == ')') k = i2;
					else k = label_to_inven(which);
				}

				/* Convert letter to equipment index */
				else if (command_wrk == (USE_EQUIP))
				{
					if (which == '(') k = e1;
					else if (which == ')') k = e2;
					else k = label_to_equip(which);
				}

				/* Convert letter to floor index */
				else if (command_wrk == USE_FLOOR)
				{
					if (which == '(') k = 0;
					else if (which == ')') k = floor_num - 1;
					else k = islower(which) ? A2I(which) : -1;
					if (k < 0 || k >= floor_num || k >= 23)
					{
						bell();
						break;
					}

					/* Special index */
					k = 0 - floor_list[k];
				}

				/* Validate the item */
				if ((command_wrk != USE_FLOOR) && !get_item_okay(k))
				{
					bell();
					break;
				}

				/* Verify the item */
				if (ver && !verify(_("本当に", "Try"), k))
				{
					done = TRUE;
					break;
				}

				/* Allow player to "refuse" certain actions */
				if (!get_item_allow(k))
				{
					done = TRUE;
					break;
				}

				/* Accept that choice */
				(*cp) = k;
				item = TRUE;
				done = TRUE;
				break;
			}
		}
		}
	}

	/* Fix the screen if necessary */
	if (command_see)
	{
		screen_load();

		/* Hack -- Cancel "display" */
		command_see = FALSE;
	}


	/* Forget the item_tester_tval restriction */
	item_tester_tval = 0;

	/* Forget the item_tester_hook restriction */
	item_tester_hook = NULL;


	/* Clean up  'show choices' */
	/* Toggle again if needed */
	if (toggle) toggle_inven_equip();

	p_ptr->window |= (PW_INVEN | PW_EQUIP);
	handle_stuff();

	/* Clear the prompt line */
	prt("", 0, 0);

	/* Warning if needed */
	if (oops && str) msg_print(str);

	if (item)
	{
		repeat_push(*cp);
		if (command_cmd) prev_tag = cur_tag;
		command_cmd = 0; /* Hack -- command_cmd is no longer effective */
	}
	return (item);
}

/*!
 * @brief 床上のアイテムを拾う選択用サブルーチン 
 * @return プレイヤーによりアイテムが選択されたならTRUEを返す。
 */
static bool py_pickup_floor_aux(void)
{
	OBJECT_IDX this_o_idx;
	concptr q, s;
	OBJECT_IDX item;

	/* Restrict the choices */
	item_tester_hook = inven_carry_okay;

	/* Get an object */
	q = _("どれを拾いますか？", "Get which item? ");
	s = _("もうザックには床にあるどのアイテムも入らない。", "You no longer have any room for the objects on the floor.");

	if (choose_object(&item, q, s, (USE_FLOOR)))
	{
		this_o_idx = 0 - item;
	}
	else
	{
		return (FALSE);
	}

	/* Pick up the object */
	py_pickup_aux(this_o_idx);

	return (TRUE);
}

/*!
 * @brief 床上のアイテムを拾うメイン処理
 * @param pickup FALSEなら金銭の自動拾いのみを行う/ FALSE then only gold will be picked up
 * @return なし
 * @details
 * This is called by py_pickup() when easy_floor is TRUE.
 */
void py_pickup_floor(bool pickup)
{
	OBJECT_IDX this_o_idx, next_o_idx = 0;

	GAME_TEXT o_name[MAX_NLEN];
	object_type *o_ptr;

	int floor_num = 0;
	OBJECT_IDX floor_o_idx = 0;

	int can_pickup = 0;

	/* Scan the pile of objects */
	for (this_o_idx = current_floor_ptr->grid_array[p_ptr->y][p_ptr->x].o_idx; this_o_idx; this_o_idx = next_o_idx)
	{
		/* Access the object */
		o_ptr = &current_floor_ptr->o_list[this_o_idx];

		object_desc(o_name, o_ptr, 0);

		/* Access the next object */
		next_o_idx = o_ptr->next_o_idx;

		disturb(FALSE, FALSE);

		/* Pick up gold */
		if (o_ptr->tval == TV_GOLD)
		{
#ifdef JP
			msg_format(" $%ld の価値がある%sを見つけた。",
				(long)o_ptr->pval, o_name);
#else
			msg_format("You have found %ld gold pieces worth of %s.",
				(long)o_ptr->pval, o_name);
#endif

			/* Collect the gold */
			p_ptr->au += o_ptr->pval;

			/* Redraw gold */
			p_ptr->redraw |= (PR_GOLD);

			p_ptr->window |= (PW_PLAYER);

			/* Delete the gold */
			delete_object_idx(this_o_idx);

			/* Check the next object */
			continue;
		}
		else if (o_ptr->marked & OM_NOMSG)
		{
			/* If 0 or 1 non-NOMSG items are in the pile, the NOMSG ones are
			 * ignored. Otherwise, they are included in the prompt. */
			o_ptr->marked &= ~(OM_NOMSG);
			continue;
		}

		/* Count non-gold objects that can be picked up. */
		if (inven_carry_okay(o_ptr))
		{
			can_pickup++;
		}

		/* Count non-gold objects */
		floor_num++;

		/* Remember this index */
		floor_o_idx = this_o_idx;
	}

	/* There are no non-gold objects */
	if (!floor_num)
		return;

	/* Mention the number of objects */
	if (!pickup)
	{
		/* One object */
		if (floor_num == 1)
		{
			/* Access the object */
			o_ptr = &current_floor_ptr->o_list[floor_o_idx];

#ifdef ALLOW_EASY_SENSE

			/* Option: Make object sensing easy */
			if (easy_sense)
			{
				/* Sense the object */
				(void) sense_object(o_ptr);
			}

#endif /* ALLOW_EASY_SENSE */

			object_desc(o_name, o_ptr, 0);

			msg_format(_("%sがある。", "You see %s."), o_name);
		}

		/* Multiple objects */
		else
		{
			msg_format(_("%d 個のアイテムの山がある。", "You see a pile of %d items."), floor_num);
		}

		return;
	}

	/* The player has no room for anything on the floor. */
	if (!can_pickup)
	{
		/* One object */
		if (floor_num == 1)
		{
			/* Access the object */
			o_ptr = &current_floor_ptr->o_list[floor_o_idx];

#ifdef ALLOW_EASY_SENSE

			/* Option: Make object sensing easy */
			if (easy_sense)
			{
				/* Sense the object */
				(void) sense_object(o_ptr);
			}

#endif /* ALLOW_EASY_SENSE */

			object_desc(o_name, o_ptr, 0);

			msg_format(_("ザックには%sを入れる隙間がない。", "You have no room for %s."), o_name);
		}

		/* Multiple objects */
		else
		{
			msg_print(_("ザックには床にあるどのアイテムも入らない。", "You have no room for any of the objects on the floor."));

		}

		return;
	}

	/* One object */
	if (floor_num == 1)
	{
		/* Hack -- query every object */
		if (carry_query_flag)
		{
			char out_val[MAX_NLEN+20];

			/* Access the object */
			o_ptr = &current_floor_ptr->o_list[floor_o_idx];

#ifdef ALLOW_EASY_SENSE

			/* Option: Make object sensing easy */
			if (easy_sense)
			{
				/* Sense the object */
				(void) sense_object(o_ptr);
			}

#endif /* ALLOW_EASY_SENSE */

			object_desc(o_name, o_ptr, 0);

			/* Build a prompt */
			(void) sprintf(out_val, _("%sを拾いますか? ", "Pick up %s? "), o_name);

			/* Ask the user to confirm */
			if (!get_check(out_val))
			{
				return;
			}
		}

		/* Access the object */
		o_ptr = &current_floor_ptr->o_list[floor_o_idx];

#ifdef ALLOW_EASY_SENSE

		/* Option: Make object sensing easy */
		if (easy_sense)
		{
			/* Sense the object */
			(void) sense_object(o_ptr);
		}

#endif /* ALLOW_EASY_SENSE */

		/* Pick up the object */
		py_pickup_aux(floor_o_idx);
	}

	/* Allow the user to choose an object */
	else
	{
		while (can_pickup--)
		{
			if (!py_pickup_floor_aux()) break;
		}
	}
}


/*!
 * @brief 矢弾を射撃した場合の破損確率を返す /
 * Determines the odds of an object breaking when thrown at a monster
 * @param o_ptr 矢弾のオブジェクト構造体参照ポインタ
 * @return 破損確率(%)
 * @details
 * Note that artifacts never break, see the "drop_near()" function.
 */
PERCENTAGE breakage_chance(object_type *o_ptr, SPELL_IDX snipe_type)
{
	PERCENTAGE archer_bonus = (p_ptr->pclass == CLASS_ARCHER ? (PERCENTAGE)(p_ptr->lev - 1) / 7 + 4 : 0);

	/* Examine the snipe type */
	if (snipe_type)
	{
		if (snipe_type == SP_KILL_WALL) return (100);
		if (snipe_type == SP_EXPLODE) return (100);
		if (snipe_type == SP_PIERCE) return (100);
		if (snipe_type == SP_FINAL) return (100);
		if (snipe_type == SP_NEEDLE) return (100);
		if (snipe_type == SP_EVILNESS) return (40);
		if (snipe_type == SP_HOLYNESS) return (40);
	}

	/* Examine the item type */
	switch (o_ptr->tval)
	{
		/* Always break */
	case TV_FLASK:
	case TV_POTION:
	case TV_BOTTLE:
	case TV_FOOD:
	case TV_JUNK:
		return (100);

		/* Often break */
	case TV_LITE:
	case TV_SCROLL:
	case TV_SKELETON:
		return (50);

		/* Sometimes break */
	case TV_WAND:
	case TV_SPIKE:
		return (25);
	case TV_ARROW:
		return (20 - archer_bonus * 2);

		/* Rarely break */
	case TV_SHOT:
	case TV_BOLT:
		return (10 - archer_bonus);
	default:
		return (10);
	}
}
