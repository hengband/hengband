/* File: obj_kind.c */

#include "angband.h"


bool object_is_potion(object_type *o_ptr)
{
	return (k_info[o_ptr->k_idx].tval == TV_POTION);
}


bool object_is_shoukinkubi(object_type *o_ptr)
{
	int i;
	if (vanilla_town) return FALSE;
	if (p_ptr->today_mon > 0 && o_ptr->pval == p_ptr->today_mon) return TRUE;
	if (o_ptr->pval == MON_TSUCHINOKO) return TRUE;
	for (i = 0; i < MAX_KUBI; i++)
		if (o_ptr->pval == kubi_r_idx[i]) break;
	if (i < MAX_KUBI) return TRUE;
	return FALSE;
}


