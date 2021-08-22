﻿#include "dungeon/dungeon.h"
#include "game-option/birth-options.h"
#include "grid/feature.h"
#include "io/input-key-acceptor.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "world/world.h"

/*
 * The dungeon arrays
 */
std::vector<dungeon_type> d_info;

/*
 * Maximum number of dungeon in d_info.txt
 */
DEPTH *max_dlv;


/*!
 * @brief これまでに入ったダンジョンの一覧を表示し、選択させる。
 * @param note ダンジョンに施す処理記述
 * @param y コンソールY座標
 * @param x コンソールX座標
 * @return 選択されたダンジョンID
 */
DUNGEON_IDX choose_dungeon(concptr note, POSITION y, POSITION x)
{
	DUNGEON_IDX select_dungeon;
	int i;
	int num = 0;
	DUNGEON_IDX *dun;

	/* Hack -- No need to choose dungeon in some case */
	if (lite_town || vanilla_town || ironman_downward)
	{
		if (max_dlv[static_cast<int>(DUNGEON_IDX::ANGBAND)]) return DUNGEON_IDX::ANGBAND;
		else
		{
            msg_format(_("まだ%sに入ったことはない。", "You haven't entered %s yet."), d_info[static_cast<int>(DUNGEON_IDX::ANGBAND)].name.c_str());
			msg_print(NULL);
			return DUNGEON_IDX::NONE;
		}
	}

	/* Allocate the "dun" array */
    C_MAKE(dun, static_cast<int>(current_world_ptr->max_d_idx), DUNGEON_IDX);

	screen_save();
	for (i = 1; i < static_cast<int>(current_world_ptr->max_d_idx); i++)
	{
		char buf[80];
		bool seiha = false;

		if (!d_info[static_cast<int>(i)].maxdepth) continue;
		if (!max_dlv[static_cast<int>(i)]) continue;
		if (d_info[static_cast<int>(i)].final_guardian)
		{
			if (!r_info[d_info[static_cast<int>(i)].final_guardian].max_num) seiha = true;
		}
		else if (max_dlv[static_cast<int>(i)] == d_info[static_cast<int>(i)].maxdepth) seiha = true;

		sprintf(buf, _("      %c) %c%-12s : 最大 %d 階", "      %c) %c%-16s : Max level %d"),
			'a' + num, seiha ? '!' : ' ', d_info[static_cast<int>(i)].name.c_str(), (int)max_dlv[static_cast<int>(i)]);
		prt(buf, y + num, x);
		dun[num++] = static_cast<DUNGEON_IDX>(i);
	}

	if (!num)
	{
		prt(_("      選べるダンジョンがない。", "      No dungeon is available."), y, x);
	}

	prt(format(_("どのダンジョン%sしますか:", "Which dungeon do you %s?: "), note), 0, 0);
	while (true)
	{
		i = inkey();
		if ((i == ESCAPE) || !num)
		{
			/* Free the "dun" array */
			C_KILL(dun, static_cast<int>(current_world_ptr->max_d_idx), DUNGEON_IDX);

			screen_load();
			return DUNGEON_IDX::NONE;
		}
		if (i >= 'a' && i < ('a' + num))
		{
			select_dungeon = static_cast<DUNGEON_IDX>(dun[i - 'a']);
			break;
		}
		else bell();
	}
	screen_load();

	/* Free the "dun" array */
	C_KILL(dun, static_cast<int>(current_world_ptr->max_d_idx), DUNGEON_IDX);

	return select_dungeon;
}

/*!
 * @brief プレイヤーが現在ダンジョンに潜っているかどうかを返す。
 * @memo 現在はdun_levelが0でないかそうでないかに限るが可読性を高めるため。
 */
bool is_in_dungeon(player_type *creature_ptr)
{
    return creature_ptr->current_floor_ptr->dun_level > 0;
}
