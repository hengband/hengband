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
#include "core.h"
#include "util.h"

#include "artifact.h"
#include "floor.h"
#include "cmd-activate.h"
#include "objectkind.h"
#include "object-ego.h"
#include "object-flavor.h"
#include "object-hook.h"
#include "player-move.h"
#include "player-class.h"
#include "player-inventory.h"
#include "monster.h"
#include "files.h"
#include "term.h"
#include "cmd-smith.h"
#include "snipe.h"
#include "view-mainwindow.h"

#if defined(MACH_O_CARBON)
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
void reset_visuals(player_type *owner_ptr)
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
		process_pref_file(owner_ptr, "graf.prf");

		/* Access the "character" pref file */
		sprintf(buf, "graf-%s.prf", owner_ptr->base_name);

		/* Process "graf-<playername>.prf" */
		process_pref_file(owner_ptr, buf);
	}

	/* Normal symbols */
	else
	{
		char buf[1024];

		/* Process "font.prf" */
		process_pref_file(owner_ptr, "font.prf");

		/* Access the "character" pref file */
		sprintf(buf, "font-%s.prf", owner_ptr->base_name);

		/* Process "font-<playername>.prf" */
		process_pref_file(owner_ptr, buf);
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
		/* We can activate it every turn */
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
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr *鑑定*情報を取得する元のオブジェクト構造体参照ポインタ
 * @param mode 表示オプション
 * @return 特筆すべき情報が一つでもあった場合TRUE、一つもなく表示がキャンセルされた場合FALSEを返す。
 */
bool screen_object(player_type *player_ptr, object_type *o_ptr, BIT_FLAGS mode)
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

	if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_POISON_NEEDLE))
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
		info[i++] = _("それは巨人に対して特に恐るべき力を発揮する。", "It is especially deadly against giants.");
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
	if (i <= trivial_info) return FALSE;
	screen_save();

	Term_get_size(&wid, &hgt);

	/* Display Item name */
	if (!(mode & SCROBJ_FAKE_OBJECT))
		object_desc(player_ptr, o_name, o_ptr, 0);
	else
		object_desc(player_ptr, o_name, o_ptr, (OD_NAME_ONLY | OD_STORE));

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
	return TRUE;
}



/*!
 * @brief オブジェクト選択時の選択アルファベットラベルを返す /
 * Convert an inventory index into a one character label
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
 * @brief オブジェクトの該当装備部位IDを返す /
 * Determine which equipment slot (if any) an item likes
 * @param o_ptr 名称を取得する元のオブジェクト構造体参照ポインタ
 * @return 対応する装備部位ID
 */
s16b wield_slot(player_type *owner_ptr, object_type *o_ptr)
{
	/* Slot for equipment */
	switch (o_ptr->tval)
	{
		case TV_DIGGING:
		case TV_HAFTED:
		case TV_POLEARM:
		case TV_SWORD:
		{
			if (!owner_ptr->inventory_list[INVEN_RARM].k_idx) return (INVEN_RARM);
			if (owner_ptr->inventory_list[INVEN_LARM].k_idx) return (INVEN_RARM);
			return (INVEN_LARM);
		}

		case TV_CAPTURE:
		case TV_CARD:
		case TV_SHIELD:
		{
			if (!owner_ptr->inventory_list[INVEN_LARM].k_idx) return (INVEN_LARM);
			if (owner_ptr->inventory_list[INVEN_RARM].k_idx) return (INVEN_LARM);
			return (INVEN_RARM);
		}

		case TV_BOW:
		{
			return (INVEN_BOW);
		}

		case TV_RING:
		{
			/* Use the right hand first */
			if (!owner_ptr->inventory_list[INVEN_RIGHT].k_idx) return (INVEN_RIGHT);

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
	return -1;
}

/*!
 * @brief tval/sval指定のベースアイテムがプレイヤーの使用可能な魔法書かどうかを返す /
 * Hack: Check if a spellbook is one of the realms we can use. -- TY
 * @param book_tval ベースアイテムのtval
 * @param book_sval ベースアイテムのsval
 * @return 使用可能な魔法書ならばTRUEを返す。
 */
bool check_book_realm(player_type *owner_ptr, const OBJECT_TYPE_VALUE book_tval, const OBJECT_SUBTYPE_VALUE book_sval)
{
	if (book_tval < TV_LIFE_BOOK) return FALSE;
	if (owner_ptr->pclass == CLASS_SORCERER)
	{
		return is_magic(tval2realm(book_tval));
	}
	else if (owner_ptr->pclass == CLASS_RED_MAGE)
	{
		if (is_magic(tval2realm(book_tval)))
			return ((book_tval == TV_ARCANE_BOOK) || (book_sval < 2));
	}
	return (REALM1_BOOK == book_tval || REALM2_BOOK == book_tval);
}
