#include "autopick/autopick-util.h"
#include "autopick/autopick-menu-data-table.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "game-option/input-options.h"
#include "main/sound-of-music.h"
#include "monster-race/race-indice-types.h"
#include "object-enchant/item-feeling.h"
#include "util/quarks.h"

/*
 * 自動拾い/破壊設定のリストに関する変数 / List for auto-picker/destroyer entries
 */
int max_autopick = 0; /*!< 現在登録している自動拾い/破壊設定の数 */
int max_max_autopick = 0; /*!< 自動拾い/破壊設定の限界数 */
autopick_type *autopick_list = NULL; /*!< 自動拾い/破壊設定構造体のポインタ配列 */

/*
 * Automatically destroy an item if it is to be destroyed
 *
 * When always_pickup is 'yes', we disable auto-destroyer function of
 * auto-picker/destroyer, and do only easy-auto-destroyer.
 */
object_type autopick_last_destroyed_object;

/*
 * A function to delete entry
 */
void autopick_free_entry(autopick_type *entry)
{
	string_free(entry->name);
	string_free(entry->insc);
	entry->name = NULL;
	entry->insc = NULL;
}


/*
 * Free memory of lines_list.
 */
void free_text_lines(concptr *lines_list)
{
	for (int lines = 0; lines_list[lines]; lines++)
	{
		string_free(lines_list[lines]);
	}

	/* free list of pointers */
	C_FREE((vptr)lines_list, MAX_LINES, concptr);
}


/*
 * Find a command by 'key'.
 */
int get_com_id(char key)
{
	for (int i = 0; menu_data[i].name; i++)
	{
		if (menu_data[i].key == key)
		{
			return menu_data[i].com_id;
		}
	}

	return 0;
}


/*
 * Auto inscription
 */
void auto_inscribe_item(player_type *player_ptr, object_type *o_ptr, int idx)
{
	if (idx < 0 || !autopick_list[idx].insc) return;

	if (!o_ptr->inscription)
		o_ptr->inscription = quark_add(autopick_list[idx].insc);

	player_ptr->window |= (PW_EQUIP | PW_INVEN);
	player_ptr->update |= (PU_BONUS);
}


/*
 * Add one line to autopick_list[]
 */
void add_autopick_list(autopick_type *entry)
{
	if (max_autopick >= max_max_autopick)
	{
		int old_max_max_autopick = max_max_autopick;
		autopick_type *old_autopick_list = autopick_list;
		max_max_autopick += MAX_AUTOPICK_DEFAULT;
		C_MAKE(autopick_list, max_max_autopick, autopick_type);
		(void)C_COPY(autopick_list, old_autopick_list, old_max_max_autopick, autopick_type);
		C_KILL(old_autopick_list, old_max_max_autopick, autopick_type);
	}

	autopick_list[max_autopick] = *entry;
	max_autopick++;
}
