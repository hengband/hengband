/*!
 * @file birth.c
 * @brief プレイヤーの作成を行う / Create a player character
 * @date 2013/12/28
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 *\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2013 Deskull Doxygen向けのコメント整理\n
 */

#include "birth/character-builder.h"
#include "birth/birth-explanations-table.h"
#include "birth/birth-wizard.h"
#include "birth/game-play-initializer.h"
#include "birth/quick-start.h"
#include "core/window-redrawer.h"
#include "floor/floor-town.h"
#include "floor/wild.h"
#include "game-option/option-flags.h"
#include "io/write-diary.h"
#include "main/music-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-remover.h"
#include "player/player-class.h"
#include "player/player-race-types.h"
#include "player/player-sex.h"
#include "player/race-info-table.h"
#include "store/store.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief プレーヤーキャラの作成結果を日記に書く
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void write_birth_diary(player_type *creature_ptr)
{
    message_add(" ");
    message_add("  ");
    message_add("====================");
    message_add(" ");
    message_add("  ");

    exe_write_diary(creature_ptr, DIARY_GAMESTART, 1, _("-------- 新規ゲーム開始 --------", "------- Started New Game -------"));
    exe_write_diary(creature_ptr, DIARY_DIALY, 0, NULL);
    char buf[80];
    sprintf(buf, _("                            性別に%sを選択した。", "                            chose %s gender."), sex_info[creature_ptr->psex].title);
    exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);
    sprintf(buf, _("                            種族に%sを選択した。", "                            chose %s race."), race_info[creature_ptr->prace].title);
    exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);
    sprintf(buf, _("                            職業に%sを選択した。", "                            chose %s class."), class_info[creature_ptr->pclass].title);
    exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);
    if (creature_ptr->realm1) {
        sprintf(buf, _("                            魔法の領域に%s%sを選択した。", "                            chose %s%s."),
            realm_names[creature_ptr->realm1], creature_ptr->realm2 ? format(_("と%s", " and %s realms"), realm_names[creature_ptr->realm2]) : _("", " realm"));
        exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);
    }

    sprintf(buf, _("                            性格に%sを選択した。", "                            chose %s personality."),
        personality_info[creature_ptr->pseikaku].title);
    exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);
}

/*!
 * @brief プレイヤー作成処理のメインルーチン/ Create a new character.
 * @details
 * Note that we may be called with "junk" leftover in the various
 * fields, so we must be sure to clear them first.
 * @return なし
 */
void player_birth(player_type *creature_ptr, void (*process_autopick_file_command)(char *))
{
    current_world_ptr->play_time = 0;
    wipe_monsters_list(creature_ptr);
    player_wipe_without_name(creature_ptr);
    if (!ask_quick_start(creature_ptr)) {
        play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_DEFAULT);
        while (TRUE) {
            if (player_birth_wizard(creature_ptr, process_autopick_file_command))
                break;

            player_wipe_without_name(creature_ptr);
        }
    }

    write_birth_diary(creature_ptr);
    for (int i = 1; i < max_towns; i++) {
        for (int j = 0; j < MAX_STORES; j++) {
            store_init(i, j);
        }
    }

    seed_wilderness();
    if (creature_ptr->prace == RACE_BEASTMAN)
        creature_ptr->hack_mutation = TRUE;
    else
        creature_ptr->hack_mutation = FALSE;

    if (!window_flag[1])
        window_flag[1] |= PW_MESSAGE;

    if (!window_flag[2])
        window_flag[2] |= PW_INVEN;
}
