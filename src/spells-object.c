
#include "angband.h"
#include "spells-object.h"
#include "object-hook.h"
#include "player-status.h"


typedef struct
{
	OBJECT_TYPE_VALUE tval;
	OBJECT_SUBTYPE_VALUE sval;
	PERCENTAGE prob;
	byte flag;
} amuse_type;

/*
 * Scatter some "amusing" objects near the player
 */

#define AMS_NOTHING   0x00 /* No restriction */
#define AMS_NO_UNIQUE 0x01 /* Don't make the amusing object of uniques */
#define AMS_FIXED_ART 0x02 /* Make a fixed artifact based on the amusing object */
#define AMS_MULTIPLE  0x04 /* Drop 1-3 objects for one type */
#define AMS_PILE      0x08 /* Drop 1-99 pile objects for one type */

static amuse_type amuse_info[] =
{
	{ TV_BOTTLE, SV_ANY, 5, AMS_NOTHING },
	{ TV_JUNK, SV_ANY, 3, AMS_MULTIPLE },
	{ TV_SPIKE, SV_ANY, 10, AMS_PILE },
	{ TV_STATUE, SV_ANY, 15, AMS_NOTHING },
	{ TV_CORPSE, SV_ANY, 15, AMS_NO_UNIQUE },
	{ TV_SKELETON, SV_ANY, 10, AMS_NO_UNIQUE },
	{ TV_FIGURINE, SV_ANY, 10, AMS_NO_UNIQUE },
	{ TV_PARCHMENT, SV_ANY, 1, AMS_NOTHING },
	{ TV_POLEARM, SV_TSURIZAO, 3, AMS_NOTHING }, //Fishing Pole of Taikobo
	{ TV_SWORD, SV_BROKEN_DAGGER, 3, AMS_FIXED_ART }, //Broken Dagger of Magician
	{ TV_SWORD, SV_BROKEN_DAGGER, 10, AMS_NOTHING },
	{ TV_SWORD, SV_BROKEN_SWORD, 5, AMS_NOTHING },
	{ TV_SCROLL, SV_SCROLL_AMUSEMENT, 10, AMS_NOTHING },

	{ 0, 0, 0 }
};

/*!
 * @brief「弾/矢の製造」処理 / do_cmd_cast calls this function if the player's class is 'archer'.
 * Hook to determine if an object is contertible in an arrow/bolt
 * @return 製造を実際に行ったらTRUE、キャンセルしたらFALSEを返す
 */
bool create_ammo(void)
{
	int ext = 0;
	char ch;

	object_type	forge;
	object_type *q_ptr;

	char com[80];
	GAME_TEXT o_name[MAX_NLEN];

	q_ptr = &forge;

	if (p_ptr->lev >= 20)
		sprintf(com, _("[S]弾, [A]矢, [B]クロスボウの矢 :", "Create [S]hots, Create [A]rrow or Create [B]olt ?"));
	else if (p_ptr->lev >= 10)
		sprintf(com, _("[S]弾, [A]矢:", "Create [S]hots or Create [A]rrow ?"));
	else
		sprintf(com, _("[S]弾:", "Create [S]hots ?"));

	if (p_ptr->confused)
	{
		msg_print(_("混乱してる！", "You are too confused!"));
		return FALSE;
	}

	if (p_ptr->blind)
	{
		msg_print(_("目が見えない！", "You are blind!"));
		return FALSE;
	}

	while (TRUE)
	{
		if (!get_com(com, &ch, TRUE))
		{
			return FALSE;
		}
		if (ch == 'S' || ch == 's')
		{
			ext = 1;
			break;
		}
		if ((ch == 'A' || ch == 'a') && (p_ptr->lev >= 10))
		{
			ext = 2;
			break;
		}
		if ((ch == 'B' || ch == 'b') && (p_ptr->lev >= 20))
		{
			ext = 3;
			break;
		}
	}

	/**********Create shots*********/
	if (ext == 1)
	{
		POSITION x, y;
		DIRECTION dir;
		grid_type *c_ptr;

		if (!get_rep_dir(&dir, FALSE)) return FALSE;
		y = p_ptr->y + ddy[dir];
		x = p_ptr->x + ddx[dir];
		c_ptr = &cave[y][x];

		if (!have_flag(f_info[get_feat_mimic(c_ptr)].flags, FF_CAN_DIG))
		{
			msg_print(_("そこには岩石がない。", "You need pile of rubble."));
			return FALSE;
		}
		else if (!cave_have_flag_grid(c_ptr, FF_CAN_DIG) || !cave_have_flag_grid(c_ptr, FF_HURT_ROCK))
		{
			msg_print(_("硬すぎて崩せなかった。", "You failed to make ammo."));
		}
		else
		{
			s16b slot;
			q_ptr = &forge;

			/* Hack -- Give the player some small firestones */
			object_prep(q_ptr, lookup_kind(TV_SHOT, (OBJECT_SUBTYPE_VALUE)m_bonus(1, p_ptr->lev) + 1));
			q_ptr->number = (byte)rand_range(15, 30);
			object_aware(q_ptr);
			object_known(q_ptr);
			apply_magic(q_ptr, p_ptr->lev, AM_NO_FIXED_ART);
			q_ptr->discount = 99;

			slot = inven_carry(q_ptr);

			object_desc(o_name, q_ptr, 0);
			msg_format(_("%sを作った。", "You make some ammo."), o_name);

			/* Auto-inscription */
			if (slot >= 0) autopick_alter_item(slot, FALSE);

			/* Destroy the wall */
			cave_alter_feat(y, x, FF_HURT_ROCK);

			p_ptr->update |= (PU_FLOW);
		}
	}
	/**********Create arrows*********/
	else if (ext == 2)
	{
		OBJECT_IDX item;
		concptr q, s;
		s16b slot;

		item_tester_hook = item_tester_hook_convertible;

		q = _("どのアイテムから作りますか？ ", "Convert which item? ");
		s = _("材料を持っていない。", "You have no item to convert.");
		q_ptr = choose_object(&item, q, s, (USE_INVEN | USE_FLOOR));
		if (!q_ptr) return FALSE;

		q_ptr = &forge;

		/* Hack -- Give the player some small firestones */
		object_prep(q_ptr, lookup_kind(TV_ARROW, (OBJECT_SUBTYPE_VALUE)m_bonus(1, p_ptr->lev) + 1));
		q_ptr->number = (byte)rand_range(5, 10);
		object_aware(q_ptr);
		object_known(q_ptr);
		apply_magic(q_ptr, p_ptr->lev, AM_NO_FIXED_ART);

		q_ptr->discount = 99;

		object_desc(o_name, q_ptr, 0);
		msg_format(_("%sを作った。", "You make some ammo."), o_name);

		if (item >= 0)
		{
			inven_item_increase(item, -1);
			inven_item_describe(item);
			inven_item_optimize(item);
		}
		else
		{
			floor_item_increase(0 - item, -1);
			floor_item_describe(0 - item);
			floor_item_optimize(0 - item);
		}

		slot = inven_carry(q_ptr);

		/* Auto-inscription */
		if (slot >= 0) autopick_alter_item(slot, FALSE);
	}
	/**********Create bolts*********/
	else if (ext == 3)
	{
		OBJECT_IDX item;
		concptr q, s;
		s16b slot;

		item_tester_hook = item_tester_hook_convertible;

		q = _("どのアイテムから作りますか？ ", "Convert which item? ");
		s = _("材料を持っていない。", "You have no item to convert.");

		q_ptr = choose_object(&item, q, s, (USE_INVEN | USE_FLOOR));
		if (!q_ptr) return FALSE;

		q_ptr = &forge;

		/* Hack -- Give the player some small firestones */
		object_prep(q_ptr, lookup_kind(TV_BOLT, (OBJECT_SUBTYPE_VALUE)m_bonus(1, p_ptr->lev) + 1));
		q_ptr->number = (byte)rand_range(4, 8);
		object_aware(q_ptr);
		object_known(q_ptr);
		apply_magic(q_ptr, p_ptr->lev, AM_NO_FIXED_ART);

		q_ptr->discount = 99;

		object_desc(o_name, q_ptr, 0);
		msg_format(_("%sを作った。", "You make some ammo."), o_name);

		if (item >= 0)
		{
			inven_item_increase(item, -1);
			inven_item_describe(item);
			inven_item_optimize(item);
		}
		else
		{
			floor_item_increase(0 - item, -1);
			floor_item_describe(0 - item);
			floor_item_optimize(0 - item);
		}

		slot = inven_carry(q_ptr);

		/* Auto-inscription */
		if (slot >= 0) autopick_alter_item(slot, FALSE);
	}
	return TRUE;
}

/*!
 * @brief 魔道具術師の魔力取り込み処理
 * @return 取り込みを実行したらTRUE、キャンセルしたらFALSEを返す
 */
bool import_magic_device(void)
{
	OBJECT_IDX item;
	PARAMETER_VALUE pval;
	int ext = 0;
	concptr q, s;
	object_type *o_ptr;
	GAME_TEXT o_name[MAX_NLEN];

	/* Only accept legal items */
	item_tester_hook = item_tester_hook_recharge;

	q = _("どのアイテムの魔力を取り込みますか? ", "Gain power of which item? ");
	s = _("魔力を取り込めるアイテムがない。", "You have nothing to gain power.");

	o_ptr = choose_object(&item, q, s, (USE_INVEN | USE_FLOOR));
	if (!o_ptr) return (FALSE);

	if (o_ptr->tval == TV_STAFF && o_ptr->sval == SV_STAFF_NOTHING)
	{
		msg_print(_("この杖には発動の為の能力は何も備わっていないようだ。", "This staff doesn't have any magical ability."));
		return FALSE;
	}

	if (!object_is_known(o_ptr))
	{
		msg_print(_("鑑定されていないと取り込めない。", "You need to identify before absorbing."));
		return FALSE;
	}

	if (o_ptr->timeout)
	{
		msg_print(_("充填中のアイテムは取り込めない。", "This item is still charging."));
		return FALSE;
	}

	pval = o_ptr->pval;
	if (o_ptr->tval == TV_ROD)
		ext = 72;
	else if (o_ptr->tval == TV_WAND)
		ext = 36;

	if (o_ptr->tval == TV_ROD)
	{
		p_ptr->magic_num2[o_ptr->sval + ext] += (MAGIC_NUM2)o_ptr->number;
		if (p_ptr->magic_num2[o_ptr->sval + ext] > 99) p_ptr->magic_num2[o_ptr->sval + ext] = 99;
	}
	else
	{
		int num;
		for (num = o_ptr->number; num; num--)
		{
			int gain_num = pval;
			if (o_ptr->tval == TV_WAND) gain_num = (pval + num - 1) / num;
			if (p_ptr->magic_num2[o_ptr->sval + ext])
			{
				gain_num *= 256;
				gain_num = (gain_num / 3 + randint0(gain_num / 3)) / 256;
				if (gain_num < 1) gain_num = 1;
			}
			p_ptr->magic_num2[o_ptr->sval + ext] += (MAGIC_NUM2)gain_num;
			if (p_ptr->magic_num2[o_ptr->sval + ext] > 99) p_ptr->magic_num2[o_ptr->sval + ext] = 99;
			p_ptr->magic_num1[o_ptr->sval + ext] += pval * 0x10000;
			if (p_ptr->magic_num1[o_ptr->sval + ext] > 99 * 0x10000) p_ptr->magic_num1[o_ptr->sval + ext] = 99 * 0x10000;
			if (p_ptr->magic_num1[o_ptr->sval + ext] > p_ptr->magic_num2[o_ptr->sval + ext] * 0x10000) p_ptr->magic_num1[o_ptr->sval + ext] = p_ptr->magic_num2[o_ptr->sval + ext] * 0x10000;
			if (o_ptr->tval == TV_WAND) pval -= (pval + num - 1) / num;
		}
	}

	object_desc(o_name, o_ptr, 0);
	msg_format(_("%sの魔力を取り込んだ。", "You absorb magic of %s."), o_name);

	/* Eliminate the item (from the pack) */
	if (item >= 0)
	{
		inven_item_increase(item, -999);
		inven_item_describe(item);
		inven_item_optimize(item);
	}

	/* Eliminate the item (from the floor) */
	else
	{
		floor_item_increase(0 - item, -999);
		floor_item_describe(0 - item);
		floor_item_optimize(0 - item);
	}
	take_turn(p_ptr, 100);;
	return TRUE;
}

/*!
 * @brief 誰得ドロップを行う。
 * @param y1 配置したいフロアのY座標
 * @param x1 配置したいフロアのX座標
 * @param num 誰得の処理回数
 * @param known TRUEならばオブジェクトが必ず＊鑑定＊済になる
 * @return なし
 */
void amusement(POSITION y1, POSITION x1, int num, bool known)
{
	object_type *i_ptr;
	object_type object_type_body;
	int n, t = 0;

	for (n = 0; amuse_info[n].tval != 0; n++)
	{
		t += amuse_info[n].prob;
	}

	/* Acquirement */
	while (num)
	{
		int i;
		KIND_OBJECT_IDX k_idx;
		ARTIFACT_IDX a_idx = 0;
		int r = randint0(t);
		bool insta_art, fixed_art;

		for (i = 0; ; i++)
		{
			r -= amuse_info[i].prob;
			if (r <= 0) break;
		}
		i_ptr = &object_type_body;
		object_wipe(i_ptr);
		k_idx = lookup_kind(amuse_info[i].tval, amuse_info[i].sval);

		/* Paranoia - reroll if nothing */
		if (!k_idx) continue;

		/* Search an artifact index if need */
		insta_art = (k_info[k_idx].gen_flags & TRG_INSTA_ART);
		fixed_art = (amuse_info[i].flag & AMS_FIXED_ART);

		if (insta_art || fixed_art)
		{
			for (a_idx = 1; a_idx < max_a_idx; a_idx++)
			{
				if (insta_art && !(a_info[a_idx].gen_flags & TRG_INSTA_ART)) continue;
				if (a_info[a_idx].tval != k_info[k_idx].tval) continue;
				if (a_info[a_idx].sval != k_info[k_idx].sval) continue;
				if (a_info[a_idx].cur_num > 0) continue;
				break;
			}

			if (a_idx >= max_a_idx) continue;
		}

		/* Make an object (if possible) */
		object_prep(i_ptr, k_idx);
		if (a_idx) i_ptr->name1 = a_idx;
		apply_magic(i_ptr, 1, AM_NO_FIXED_ART);

		if (amuse_info[i].flag & AMS_NO_UNIQUE)
		{
			if (r_info[i_ptr->pval].flags1 & RF1_UNIQUE) continue;
		}

		if (amuse_info[i].flag & AMS_MULTIPLE) i_ptr->number = randint1(3);
		if (amuse_info[i].flag & AMS_PILE) i_ptr->number = randint1(99);

		if (known)
		{
			object_aware(i_ptr);
			object_known(i_ptr);
		}

		/* Paranoia - reroll if nothing */
		if (!(i_ptr->k_idx)) continue;

		/* Drop the object */
		(void)drop_near(i_ptr, -1, y1, x1);

		num--;
	}
}



/*!
 * @brief 獲得ドロップを行う。
 * Scatter some "great" objects near the player
 * @param y1 配置したいフロアのY座標
 * @param x1 配置したいフロアのX座標
 * @param num 獲得の処理回数
 * @param great TRUEならば必ず高級品以上を落とす
 * @param special TRUEならば必ず特別品を落とす
 * @param known TRUEならばオブジェクトが必ず＊鑑定＊済になる
 * @return なし
 */
void acquirement(POSITION y1, POSITION x1, int num, bool great, bool special, bool known)
{
	object_type *i_ptr;
	object_type object_type_body;
	BIT_FLAGS mode = AM_GOOD | (great || special ? AM_GREAT : 0L) | (special ? AM_SPECIAL : 0L);

	/* Acquirement */
	while (num--)
	{
		i_ptr = &object_type_body;
		object_wipe(i_ptr);

		/* Make a good (or great) object (if possible) */
		if (!make_object(i_ptr, mode)) continue;

		if (known)
		{
			object_aware(i_ptr);
			object_known(i_ptr);
		}

		/* Drop the object */
		(void)drop_near(i_ptr, -1, y1, x1);
	}
}
