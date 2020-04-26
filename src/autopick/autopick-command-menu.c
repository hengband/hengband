/*!
 * todo 1関数100行以上ある、後で関数を分割すること
 * @brief 自動拾いエディタのコマンドを受け付ける
 * @date 2020/04/26
 * @author Hourier
 */
#include "angband.h"
#include "autopick/autopick-command-menu.h"
#include "autopick/autopick-util.h"
#include "autopick/autopick-menu-data-table.h"
#include "gameterm.h"

/*
 * Display the menu, and get a command
 */
int do_command_menu(int level, int start)
{
	int max_len = 0;
	int menu_id_list[26];
	byte menu_key = 0;
	for (int i = start; menu_data[i].level >= level; i++)
	{
		/* Ignore lower level sub menus */
		if (menu_data[i].level > level) continue;

		int len = strlen(menu_data[i].name);
		if (len > max_len) max_len = len;

		menu_id_list[menu_key] = i;
		menu_key++;
	}

	while (menu_key < 26)
	{
		menu_id_list[menu_key] = -1;
		menu_key++;
	}

	int max_menu_wid = max_len + 3 + 3;

	char linestr[MAX_LINELEN];
	linestr[0] = '\0';
	strcat(linestr, "+");
	for (int i = 0; i < max_menu_wid + 2; i++)
	{
		strcat(linestr, "-");
	}

	strcat(linestr, "+");
	bool redraw = TRUE;
	while (TRUE)
	{
		if (redraw)
		{
			int col0 = 5 + level * 7;
			int row0 = 1 + level * 3;
			int row1 = row0 + 1;
			Term_putstr(col0, row0, -1, TERM_WHITE, linestr);

			menu_key = 0;
			for (int i = start; menu_data[i].level >= level; i++)
			{
				char com_key_str[3];
				concptr str;
				if (menu_data[i].level > level) continue;

				if (menu_data[i].com_id == -1)
				{
					strcpy(com_key_str, _("▼", ">"));
				}
				else if (menu_data[i].key != -1)
				{
					com_key_str[0] = '^';
					com_key_str[1] = menu_data[i].key + '@';
					com_key_str[2] = '\0';
				}
				else
				{
					com_key_str[0] = '\0';
				}

				str = format("| %c) %-*s %2s | ", menu_key + 'a', max_len, menu_data[i].name, com_key_str);

				Term_putstr(col0, row1++, -1, TERM_WHITE, str);

				menu_key++;
			}

			Term_putstr(col0, row1, -1, TERM_WHITE, linestr);
			redraw = FALSE;
		}

		prt(format(_("(a-%c) コマンド:", "(a-%c) Command:"), menu_key + 'a' - 1), 0, 0);
		char key = inkey();

		if (key == ESCAPE) return 0;

		int com_id;
		bool is_alphabet = key >= 'a' && key <= 'z';
		if (!is_alphabet)
		{
			com_id = get_com_id(key);
			if (com_id)
			{
				return com_id;
			}

			continue;
		}

		int menu_id = menu_id_list[key - 'a'];

		if (menu_id < 0) continue;

		com_id = menu_data[menu_id].com_id;

		if (com_id == -1)
		{
			com_id = do_command_menu(level + 1, menu_id + 1);

			if (com_id) return com_id;
			else redraw = TRUE;
		}
		else if (com_id)
		{
			return com_id;
		}
	}
}
