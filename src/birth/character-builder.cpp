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
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/class-info.h"
#include "player-info/race-types.h"
#include "player/patron.h"
#include "player/player-personality.h"
#include "player/player-sex.h"
#include "player/race-info-table.h"
#include "realm/realm-names-table.h"
#include "store/store-owners.h"
#include "store/store.h"
#include "system/player-type-definition.h"
#include "term/z-form.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief プレイヤーキャラの作成結果を日記に書く
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void write_birth_diary(PlayerType *player_ptr)
{
    concptr indent = "                            ";

    message_add(" ");
    message_add("  ");
    message_add("====================");
    message_add(" ");
    message_add("  ");

    exe_write_diary(player_ptr, DIARY_GAMESTART, 1, _("-------- 新規ゲーム開始 --------", "------- Started New Game -------"));
    exe_write_diary(player_ptr, DIARY_DIALY, 0, nullptr);
    char buf[80];
    strnfmt(buf, sizeof(buf), _("%s性別に%sを選択した。", "%schose %s gender."), indent, sex_info[player_ptr->psex].title);
    exe_write_diary(player_ptr, DIARY_DESCRIPTION, 1, buf);
    strnfmt(buf, sizeof(buf), _("%s種族に%sを選択した。", "%schose %s race."), indent, race_info[enum2i(player_ptr->prace)].title);
    exe_write_diary(player_ptr, DIARY_DESCRIPTION, 1, buf);
    strnfmt(buf, sizeof(buf), _("%s職業に%sを選択した。", "%schose %s class."), indent, class_info[enum2i(player_ptr->pclass)].title);
    exe_write_diary(player_ptr, DIARY_DESCRIPTION, 1, buf);
    if (player_ptr->realm1) {
        strnfmt(buf, sizeof(buf), _("%s魔法の領域に%s%sを選択した。", "%schose %s%s."), indent, realm_names[player_ptr->realm1],
            player_ptr->realm2 ? format(_("と%s", " and %s realms"), realm_names[player_ptr->realm2]).data() : _("", " realm"));
        exe_write_diary(player_ptr, DIARY_DESCRIPTION, 1, buf);
    }
    if (player_ptr->element) {
        strnfmt(buf, sizeof(buf), _("%s元素系統に%sを選択した。", "%schose %s system."), indent, get_element_title(player_ptr->element));
        exe_write_diary(player_ptr, DIARY_DESCRIPTION, 1, buf);
    }
    strnfmt(buf, sizeof(buf), _("%s性格に%sを選択した。", "%schose %s personality."), indent, personality_info[player_ptr->ppersonality].title);
    exe_write_diary(player_ptr, DIARY_DESCRIPTION, 1, buf);
    if (PlayerClass(player_ptr).equals(PlayerClassType::CHAOS_WARRIOR)) {
        strnfmt(buf, sizeof(buf), _("%s守護神%sと契約を交わした。", "%smade a contract with patron %s."), indent, patron_list[player_ptr->chaos_patron].name.data());
        exe_write_diary(player_ptr, DIARY_DESCRIPTION, 1, buf);
    }
}

/*!
 * @brief プレイヤー作成処理のメインルーチン/ Create a new character.
 * @details
 * Note that we may be called with "junk" leftover in the various
 * fields, so we must be sure to clear them first.
 */
void player_birth(PlayerType *player_ptr)
{
    constexpr auto display_width = 80;
    constexpr auto display_height = 24;
    TermCenteredOffsetSetter tcos(display_width, display_height);

    w_ptr->play_time = 0;
    wipe_monsters_list(player_ptr);
    player_wipe_without_name(player_ptr);
    if (!ask_quick_start(player_ptr)) {
        play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_NEW_GAME);
        while (true) {
            if (player_birth_wizard(player_ptr)) {
                break;
            }

            player_wipe_without_name(player_ptr);
        }
    }

    write_birth_diary(player_ptr);
    for (int i = 1; i < max_towns; i++) {
        for (auto sst : STORE_SALE_TYPE_LIST) {
            store_init(i, sst);
        }
    }

    seed_wilderness();
    if (PlayerRace(player_ptr).equals(PlayerRaceType::BEASTMAN)) {
        player_ptr->hack_mutation = true;
    } else {
        player_ptr->hack_mutation = false;
    }

    if (!window_flag[1]) {
        window_flag[1] |= PW_MESSAGE;
    }

    if (!window_flag[2]) {
        window_flag[2] |= PW_INVEN;
    }
}
