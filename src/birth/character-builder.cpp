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
#include "mind/mind-elementalist.h"
#include "monster-floor/monster-remover.h"
#include "player/patron.h"
#include "player/player-class.h"
#include "player/player-race-types.h"
#include "player/player-sex.h"
#include "player/race-info-table.h"
#include "store/store-owners.h"
#include "store/store.h"
#include "system/player-type-definition.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief プレーヤーキャラの作成結果を日記に書く
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
static void write_birth_diary(player_type *creature_ptr)
{
    concptr indent = "                            ";                  

    message_add(" ");
    message_add("  ");
    message_add("====================");
    message_add(" ");
    message_add("  ");

    exe_write_diary(creature_ptr, DIARY_GAMESTART, 1, _("-------- 新規ゲーム開始 --------", "------- Started New Game -------"));
    exe_write_diary(creature_ptr, DIARY_DIALY, 0, nullptr);
    char buf[80];
    sprintf(buf, _("%s性別に%sを選択した。", "%schose %s gender."), indent, sex_info[creature_ptr->psex].title);
    exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);
    sprintf(buf, _("%s種族に%sを選択した。", "%schose %s race."), indent, race_info[enum2i(creature_ptr->prace)].title);
    exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);
    sprintf(buf, _("%s職業に%sを選択した。", "%schose %s class."), indent, class_info[creature_ptr->pclass].title);
    exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);
    if (creature_ptr->realm1) {
        sprintf(buf, _("%s魔法の領域に%s%sを選択した。", "%schose %s%s."), indent,
            realm_names[creature_ptr->realm1], creature_ptr->realm2 ? format(_("と%s", " and %s realms"), realm_names[creature_ptr->realm2]) : _("", " realm"));
        exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);
    }
    if (creature_ptr->element) {
        sprintf(buf, _("%s元素系統に%sを選択した。", "%schose %s system."), indent, get_element_title(creature_ptr->element));
        exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);
    }
    sprintf(buf, _("%s性格に%sを選択した。", "%schose %s personality."), indent, personality_info[creature_ptr->pseikaku].title);
    exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);
    if (creature_ptr->pclass == CLASS_CHAOS_WARRIOR) {
        sprintf(buf, _("%s守護神%sと契約を交わした。", "%smade a contract with patron %s."), indent, chaos_patrons[creature_ptr->chaos_patron]);
        exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 1, buf);
    }
}

/*!
 * @brief プレイヤー作成処理のメインルーチン/ Create a new character.
 * @details
 * Note that we may be called with "junk" leftover in the various
 * fields, so we must be sure to clear them first.
 */
void player_birth(player_type *creature_ptr)
{
    current_world_ptr->play_time = 0;
    wipe_monsters_list(creature_ptr);
    player_wipe_without_name(creature_ptr);
    if (!ask_quick_start(creature_ptr)) {
        play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_NEW_GAME);
        while (true) {
            if (player_birth_wizard(creature_ptr))
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
    if (creature_ptr->prace == player_race_type::BEASTMAN)
        creature_ptr->hack_mutation = true;
    else
        creature_ptr->hack_mutation = false;

    if (!window_flag[1])
        window_flag[1] |= PW_MESSAGE;

    if (!window_flag[2])
        window_flag[2] |= PW_INVEN;
}
