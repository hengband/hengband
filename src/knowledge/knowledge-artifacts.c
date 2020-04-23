/*!
 * @brief 既知のアーティファクトを表示する
 * @date 2020/04/23
 * @author Hourier
 */

#include "angband.h"
#include "knowledge-artifacts.h"
#include "cmd/dump-util.h"
#include "artifact.h"
#include "sort.h"
#include "object-flavor.h"
#include "object-hook.h"
#include "object/object-kind.h"
#include "core/show-file.h"

/*
 * todo okay = 既知のアーティファクト？ と思われるが確証がない
 * 分かりやすい変数名へ変更求む＆万が一未知である旨の配列なら負論理なのでゴソッと差し替えるべき
 * Check the status of "artifacts"
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_knowledge_artifacts(player_type *player_ptr)
{
	FILE *fff = NULL;
	GAME_TEXT file_name[FILE_NAME_SIZE];
	if (!open_temporary_file(&fff, file_name)) return;

	ARTIFACT_IDX *who;
	C_MAKE(who, max_a_idx, ARTIFACT_IDX);
	bool *okay;
	C_MAKE(okay, max_a_idx, bool);

	for (ARTIFACT_IDX k = 0; k < max_a_idx; k++)
	{
		artifact_type *a_ptr = &a_info[k];
		okay[k] = FALSE;
		if (!a_ptr->name) continue;
		if (!a_ptr->cur_num) continue;

		okay[k] = TRUE;
	}

	for (POSITION y = 0; y < player_ptr->current_floor_ptr->height; y++)
	{
		for (POSITION x = 0; x < player_ptr->current_floor_ptr->width; x++)
		{
			grid_type *g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];
			OBJECT_IDX this_o_idx, next_o_idx = 0;
			for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx)
			{
				object_type *o_ptr;
				o_ptr = &player_ptr->current_floor_ptr->o_list[this_o_idx];
				next_o_idx = o_ptr->next_o_idx;
				if (!object_is_fixed_artifact(o_ptr)) continue;
				if (object_is_known(o_ptr)) continue;

				okay[o_ptr->name1] = FALSE;
			}
		}
	}

	for (ARTIFACT_IDX i = 0; i < INVEN_TOTAL; i++)
	{
		object_type *o_ptr = &player_ptr->inventory_list[i];
		if (!o_ptr->k_idx) continue;
		if (!object_is_fixed_artifact(o_ptr)) continue;
		if (object_is_known(o_ptr)) continue;

		okay[o_ptr->name1] = FALSE;
	}

	int n = 0;
	for (ARTIFACT_IDX k = 0; k < max_a_idx; k++)
	{
		if (okay[k]) who[n++] = k;
	}

	u16b why = 3;
	ang_sort(who, &why, n, ang_sort_art_comp, ang_sort_art_swap);
	for (ARTIFACT_IDX k = 0; k < n; k++)
	{
		artifact_type *a_ptr = &a_info[who[k]];
		GAME_TEXT base_name[MAX_NLEN];
		strcpy(base_name, _("未知の伝説のアイテム", "Unknown Artifact"));
		ARTIFACT_IDX z = lookup_kind(a_ptr->tval, a_ptr->sval);
		if (z)
		{
			object_type forge;
			object_type *q_ptr;
			q_ptr = &forge;
			object_prep(q_ptr, z);
			q_ptr->name1 = (byte)who[k];
			q_ptr->ident |= IDENT_STORE;
			object_desc(player_ptr, base_name, q_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
		}

		fprintf(fff, _("     %s\n", "     The %s\n"), base_name);
	}

	C_KILL(who, max_a_idx, ARTIFACT_IDX);
	C_KILL(okay, max_a_idx, bool);
	my_fclose(fff);
	(void)show_file(player_ptr, TRUE, file_name, _("既知の伝説のアイテム", "Artifacts Seen"), 0, 0);
	fd_kill(file_name);
}
