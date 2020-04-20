#include "cmd/cmd-inventory.h"
#include "object-flavor.h"
#include "market/store-util.h"
#include "floor-town.h"
#include "object-hook.h"
#include "objectkind.h"
#include "core/show-file.h"

static concptr inven_res_label = _(
	"                               酸電火冷毒光闇破轟獄因沌劣 盲怖乱痺透命感消復浮",
	"                               AcElFiCoPoLiDkShSoNtNxCaDi BlFeCfFaSeHlEpSdRgLv");

#define IM_FLAG_STR  _("＊", "* ")
#define HAS_FLAG_STR _("＋", "+ ")
#define NO_FLAG_STR  _("・", ". ")

// todo: 普通の関数に直す
#define print_im_or_res_flag(IM, RES) \
{ \
	fputs(have_flag(flgs, (IM)) ? IM_FLAG_STR : \
	      (have_flag(flgs, (RES)) ? HAS_FLAG_STR : NO_FLAG_STR), fff); \
}


// todo: 普通の関数に直す
#define print_flag(TR) \
{ \
	fputs(have_flag(flgs, (TR)) ? HAS_FLAG_STR : NO_FLAG_STR, fff); \
}


/*!
 * @brief 
 * @param creature_ptr
 * @param fff
 * @param o_ptr
 * @param j
 * @param tval
 * @param where
 * @return なし
 */
void do_cmd_knowledge_inventory_aux(player_type *creature_ptr, FILE *fff, object_type *o_ptr, int *j, OBJECT_TYPE_VALUE tval, char *where)
{
	if (o_ptr->k_idx == 0) return;
	if (o_ptr->tval != tval) return;
	if (!object_is_known(o_ptr)) return;

	bool is_special_item_type = (object_is_wearable(o_ptr) && object_is_ego(o_ptr))
		|| ((tval == TV_AMULET) && (o_ptr->sval == SV_AMULET_RESISTANCE))
		|| ((tval == TV_RING) && (o_ptr->sval == SV_RING_LORDLY))
		|| ((tval == TV_SHIELD) && (o_ptr->sval == SV_DRAGON_SHIELD))
		|| ((tval == TV_HELM) && (o_ptr->sval == SV_DRAGON_HELM))
		|| ((tval == TV_GLOVES) && (o_ptr->sval == SV_SET_OF_DRAGON_GLOVES))
		|| ((tval == TV_BOOTS) && (o_ptr->sval == SV_PAIR_OF_DRAGON_GREAVE))
		|| object_is_artifact(o_ptr);
	if (!is_special_item_type)
	{
		return;
	}

	int i = 0;
	GAME_TEXT o_name[MAX_NLEN];
	object_desc(creature_ptr, o_name, o_ptr, OD_NAME_ONLY);
	while (o_name[i] && (i < 26))
	{
#ifdef JP
		if (iskanji(o_name[i])) i++;
#endif
		i++;
	}

	if (i < 28)
	{
		while (i < 28)
		{
			o_name[i] = ' '; i++;
		}
	}

	o_name[i] = '\0';

	fprintf(fff, "%s %s", where, o_name);

	if (!OBJECT_IS_FULL_KNOWN(o_ptr))
	{
		fputs(_("-------不明--------------- -------不明---------\n",
			"-------unknown------------ -------unknown------\n"), fff);
	}
	else
	{
		BIT_FLAGS flgs[TR_FLAG_SIZE];
		object_flags_known(o_ptr, flgs);

		print_im_or_res_flag(TR_IM_ACID, TR_RES_ACID);
		print_im_or_res_flag(TR_IM_ELEC, TR_RES_ELEC);
		print_im_or_res_flag(TR_IM_FIRE, TR_RES_FIRE);
		print_im_or_res_flag(TR_IM_COLD, TR_RES_COLD);
		print_flag(TR_RES_POIS);
		print_flag(TR_RES_LITE);
		print_flag(TR_RES_DARK);
		print_flag(TR_RES_SHARDS);
		print_flag(TR_RES_SOUND);
		print_flag(TR_RES_NETHER);
		print_flag(TR_RES_NEXUS);
		print_flag(TR_RES_CHAOS);
		print_flag(TR_RES_DISEN);

		fputs(" ", fff);

		print_flag(TR_RES_BLIND);
		print_flag(TR_RES_FEAR);
		print_flag(TR_RES_CONF);
		print_flag(TR_FREE_ACT);
		print_flag(TR_SEE_INVIS);
		print_flag(TR_HOLD_EXP);
		print_flag(TR_TELEPATHY);
		print_flag(TR_SLOW_DIGEST);
		print_flag(TR_REGEN);
		print_flag(TR_LEVITATION);

		fputc('\n', fff);
	}

	(*j)++;
	if (*j == 9)
	{
		*j = 0;
		fprintf(fff, "%s\n", inven_res_label);
	}
}


/*
 * @brief Display *ID* ed weapons/armors's resistances
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_knowledge_inventory(player_type *creature_ptr)
{
	FILE *fff;
	GAME_TEXT file_name[1024];

	char where[32];
	fff = my_fopen_temp(file_name, 1024);
	if (!fff)
	{
		msg_format(_("一時ファイル %s を作成できませんでした。", "Failed to create temporary file %s."), file_name);
		msg_print(NULL);
		return;
	}

	fprintf(fff, "%s\n", inven_res_label);
	int j = 0;
	for (OBJECT_TYPE_VALUE tval = TV_WEARABLE_BEGIN; tval <= TV_WEARABLE_END; tval++)
	{
		if (j != 0)
		{
			for (; j < 9; j++)
			{
				fputc('\n', fff);
			}

			j = 0;
			fprintf(fff, "%s\n", inven_res_label);
		}

		strcpy(where, _("装", "E "));
		for (int i = INVEN_RARM; i < INVEN_TOTAL; i++)
		{
			do_cmd_knowledge_inventory_aux(creature_ptr, fff, &creature_ptr->inventory_list[i], &j, tval, where);
		}

		strcpy(where, _("持", "I "));
		for (int i = 0; i < INVEN_PACK; i++)
		{
			do_cmd_knowledge_inventory_aux(creature_ptr, fff, &creature_ptr->inventory_list[i], &j, tval, where);
		}

		store_type *store_ptr;
		store_ptr = &town_info[1].store[STORE_HOME];
		strcpy(where, _("家", "H "));
		for (int i = 0; i < store_ptr->stock_num; i++)
		{
			do_cmd_knowledge_inventory_aux(creature_ptr, fff, &store_ptr->stock[i], &j, tval, where);
		}
	}

	my_fclose(fff);
	(void)show_file(creature_ptr, TRUE, file_name, _("*鑑定*済み武器/防具の耐性リスト", "Resistances of *identified* equipment"), 0, 0);
	fd_kill(file_name);
}
