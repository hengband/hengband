
#include "angband.h"
#include "spells-object.h"
#include "object-hook.h"

/*!
 * @brief「弾/矢の製造」処理 / do_cmd_cast calls this function if the player's class is 'archer'.
 * Hook to determine if an object is contertible in an arrow/bolt
 * @return 製造を実際に行ったらTRUE、キャンセルしたらFALSEを返す
 */
bool do_cmd_archer(void)
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
		cave_type *c_ptr;

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
