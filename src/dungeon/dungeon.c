#include "dungeon/dungeon.h"
#include "game-option/birth-options.h"
#include "io/input-key-acceptor.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "world/world.h"

/*
 * The dungeon arrays
 */
dungeon_type *d_info;
char *d_name;
char *d_text;

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
	DUNGEON_IDX i;
	int num = 0;
	DUNGEON_IDX *dun;

	/* Hack -- No need to choose dungeon in some case */
	if (lite_town || vanilla_town || ironman_downward)
	{
		if (max_dlv[DUNGEON_ANGBAND]) return DUNGEON_ANGBAND;
		else
		{
			msg_format(_("まだ%sに入ったことはない。", "You haven't entered %s yet."), d_name + d_info[DUNGEON_ANGBAND].name);
			msg_print(NULL);
			return 0;
		}
	}

	/* Allocate the "dun" array */
	C_MAKE(dun, current_world_ptr->max_d_idx, DUNGEON_IDX);

	screen_save();
	for (i = 1; i < current_world_ptr->max_d_idx; i++)
	{
		char buf[80];
		bool seiha = FALSE;

		if (!d_info[i].maxdepth) continue;
		if (!max_dlv[i]) continue;
		if (d_info[i].final_guardian)
		{
			if (!r_info[d_info[i].final_guardian].max_num) seiha = TRUE;
		}
		else if (max_dlv[i] == d_info[i].maxdepth) seiha = TRUE;

		sprintf(buf, _("      %c) %c%-12s : 最大 %d 階", "      %c) %c%-16s : Max level %d"),
			'a' + num, seiha ? '!' : ' ', d_name + d_info[i].name, (int)max_dlv[i]);
		prt(buf, y + num, x);
		dun[num++] = i;
	}

	if (!num)
	{
		prt(_("      選べるダンジョンがない。", "      No dungeon is available."), y, x);
	}

	prt(format(_("どのダンジョン%sしますか:", "Which dungeon do you %s?: "), note), 0, 0);
	while (TRUE)
	{
		i = inkey();
		if ((i == ESCAPE) || !num)
		{
			/* Free the "dun" array */
			C_KILL(dun, current_world_ptr->max_d_idx, DUNGEON_IDX);

			screen_load();
			return 0;
		}
		if (i >= 'a' && i < ('a' + num))
		{
			select_dungeon = dun[i - 'a'];
			break;
		}
		else bell();
	}
	screen_load();

	/* Free the "dun" array */
	C_KILL(dun, current_world_ptr->max_d_idx, DUNGEON_IDX);

	return select_dungeon;
}

