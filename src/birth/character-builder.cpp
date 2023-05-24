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
#include "system/redrawing-flags-updater.h"
#include "term/gameterm.h"
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

    exe_write_diary(player_ptr, DiaryKind::GAMESTART, 1, _("-------- 新規ゲーム開始 --------", "------- Started New Game -------"));
    exe_write_diary(player_ptr, DiaryKind::DIALY, 0);
    const auto mes_sex = format(_("%s性別に%sを選択した。", "%schose %s gender."), indent, sex_info[player_ptr->psex].title);
    exe_write_diary(player_ptr, DiaryKind::DESCRIPTION, 1, mes_sex);
    const auto mes_race = format(_("%s種族に%sを選択した。", "%schose %s race."), indent, race_info[enum2i(player_ptr->prace)].title);
    exe_write_diary(player_ptr, DiaryKind::DESCRIPTION, 1, mes_race);
    const auto mes_class = format(_("%s職業に%sを選択した。", "%schose %s class."), indent, class_info[enum2i(player_ptr->pclass)].title);
    exe_write_diary(player_ptr, DiaryKind::DESCRIPTION, 1, mes_class);
    if (player_ptr->realm1) {
        const std::string mes_realm2 = player_ptr->realm2 ? format(_("と%s", " and %s realms"), realm_names[player_ptr->realm2]) : _("", " realm");
        const auto mes_realm = format(_("%s魔法の領域に%s%sを選択した。", "%schose %s%s."), indent, realm_names[player_ptr->realm1], mes_realm2.data());
        exe_write_diary(player_ptr, DiaryKind::DESCRIPTION, 1, mes_realm);
    }

    if (player_ptr->element) {
        const auto mes_element = format(_("%s元素系統に%sを選択した。", "%schose %s system."), indent, get_element_title(player_ptr->element));
        exe_write_diary(player_ptr, DiaryKind::DESCRIPTION, 1, mes_element);
    }

    const auto mes_personality = format(_("%s性格に%sを選択した。", "%schose %s personality."), indent, personality_info[player_ptr->ppersonality].title);
    exe_write_diary(player_ptr, DiaryKind::DESCRIPTION, 1, mes_personality);
    if (PlayerClass(player_ptr).equals(PlayerClassType::CHAOS_WARRIOR)) {
        const auto fmt_patron = _("%s守護神%sと契約を交わした。", "%smade a contract with patron %s.");
        const auto mes_patron = format(fmt_patron, indent, patron_list[player_ptr->chaos_patron].name.data());
        exe_write_diary(player_ptr, DiaryKind::DESCRIPTION, 1, mes_patron);
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
    TermCenteredOffsetSetter tcos(MAIN_TERM_MIN_COLS, MAIN_TERM_MIN_ROWS);

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
    for (size_t i = 1; i < towns_info.size(); i++) {
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

    if (g_window_flags[1].none()) {
        g_window_flags[1].set(SubWindowRedrawingFlag::MESSAGE);
    }

    if (g_window_flags[2].none()) {
        g_window_flags[2].set(SubWindowRedrawingFlag::INVENTORY);
    }
}
