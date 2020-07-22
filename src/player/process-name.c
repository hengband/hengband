#include "player/process-name.h"
#include "autopick/autopick-reader-writer.h"
#include "core/asking-player.h"
#include "io/files-util.h"
#include "player/player-personality.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/string-processor.h"
#include "world/world.h"

/*!
 * @brief プレイヤーの名前をチェックして修正する
 * Process the player name.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param sf セーブファイル名に合わせた修正を行うならばTRUE
 * @return なし
 * @details
 * Extract a clean "base name".
 * Build the savefile name if needed.
 */
void process_player_name(player_type *creature_ptr, bool sf)
{
	char old_player_base[32] = "";
	if (current_world_ptr->character_generated)
		strcpy(old_player_base, creature_ptr->base_name);

	for (int i = 0; creature_ptr->name[i]; i++)
	{
#ifdef JP
		if (iskanji(creature_ptr->name[i]))
		{
			i++;
			continue;
		}

		if (iscntrl((unsigned char)creature_ptr->name[i]))
#else
		if (iscntrl(creature_ptr->name[i]))
#endif
		{
			quit_fmt(_("'%s' という名前は不正なコントロールコードを含んでいます。", "The name '%s' contains control chars!"), creature_ptr->name);
		}
	}

	int k = 0;
	for (int i = 0; creature_ptr->name[i]; i++)
	{
#ifdef JP
		unsigned char c = creature_ptr->name[i];
#else
		char c = creature_ptr->name[i];
#endif

#ifdef JP
		if (iskanji(c)) {
			if (k + 2 >= (int)sizeof(creature_ptr->base_name) || !creature_ptr->name[i + 1])
				break;

			creature_ptr->base_name[k++] = c;
			i++;
			creature_ptr->base_name[k++] = creature_ptr->name[i];
		}
#ifdef SJIS
		else if (iskana(c)) creature_ptr->base_name[k++] = c;
#endif
		else
#endif
			if (!strncmp(PATH_SEP, creature_ptr->name + i, strlen(PATH_SEP)))
			{
				creature_ptr->base_name[k++] = '_';
				i += strlen(PATH_SEP);
			}
#if defined(WINDOWS)
			else if (angband_strchr("\"*,/:;<>?\\|", c))
				creature_ptr->base_name[k++] = '_';
#endif
			else if (isprint(c))
				creature_ptr->base_name[k++] = c;
	}

	creature_ptr->base_name[k] = '\0';
	if (!creature_ptr->base_name[0])
		strcpy(creature_ptr->base_name, "PLAYER");

#ifdef SAVEFILE_MUTABLE
	sf = TRUE;
#endif
	if (!savefile_base[0] && savefile[0])
	{
		concptr s = savefile;
		while (TRUE)
		{
			concptr t;
			t = angband_strstr(s, PATH_SEP);
			if (!t)
				break;
			s = t + 1;
		}

		strcpy(savefile_base, s);
	}

	if (!savefile_base[0] || !savefile[0])
		sf = TRUE;

	if (sf)
	{
		char temp[128];
		strcpy(savefile_base, creature_ptr->base_name);

#ifdef SAVEFILE_USE_UID
		/* Rename the savefile, using the creature_ptr->player_uid and creature_ptr->base_name */
		(void)sprintf(temp, "%d.%s", creature_ptr->player_uid, creature_ptr->base_name);
#else
		/* Rename the savefile, using the creature_ptr->base_name */
		(void)sprintf(temp, "%s", creature_ptr->base_name);
#endif
		path_build(savefile, sizeof(savefile), ANGBAND_DIR_SAVE, temp);
	}

	if (current_world_ptr->character_generated && !streq(old_player_base, creature_ptr->base_name))
	{
		autopick_load_pref(creature_ptr, FALSE);
	}
}


/*!
 * @brief プレイヤーの名前を変更するコマンドのメインルーチン
 * Gets a name for the character, reacting to name changes.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * <pre>
 * Assumes that "display_player()" has just been called
 * Perhaps we should NOT ask for a name (at "birth()") on
 * Unix machines?  XXX XXX
 * What a horrible name for a global function.
 * </pre>
 */
void get_name(player_type *creature_ptr)
{
	char tmp[64];
	strcpy(tmp, creature_ptr->name);

	if (get_string(_("キャラクターの名前を入力して下さい: ", "Enter a name for your character: "), tmp, 15))
	{
		strcpy(creature_ptr->name, tmp);
	}

	if (strlen(creature_ptr->name) == 0)
	{
		strcpy(creature_ptr->name, "PLAYER");
	}

	strcpy(tmp, ap_ptr->title);
#ifdef JP
	if (ap_ptr->no == 1)
		strcat(tmp, "の");
#else
	strcat(tmp, " ");
#endif
	strcat(tmp, creature_ptr->name);

	term_erase(34, 1, 255);
	c_put_str(TERM_L_BLUE, tmp, 1, 34);
	clear_from(22);
}
