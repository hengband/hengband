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
#include "player/player-realm.h"
#include "player/player-sex.h"
#include "player/race-info-table.h"
#include "store/store-owners.h"
#include "store/store.h"
#include "system/deferred-update-requests.h"
#include "system/floor/town-list.h"
#include "system/floor/wilderness-grid.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <fmt/format.h>

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
    const auto &floor = *player_ptr->current_floor_ptr;
    exe_write_diary(floor, DiaryKind::GAMESTART, 1, _("-------- 新規ゲーム開始 --------", "------- Started New Game -------"));
    exe_write_diary(floor, DiaryKind::DIALY, 0);
    const auto mes_sex = fmt::format(_("{}性別に{}を選択した。", "{}chose {} gender."), indent, sex_info[player_ptr->psex].title);
    exe_write_diary(floor, DiaryKind::DESCRIPTION, 1, mes_sex);
    const auto mes_race = fmt::format(_("{}種族に{}を選択した。", "{}chose {} race."), indent, race_info[enum2i(player_ptr->prace)].title);
    exe_write_diary(floor, DiaryKind::DESCRIPTION, 1, mes_race);
    const auto mes_class = fmt::format(_("{}職業に{}を選択した。", "{}chose {} class."), indent, class_info.at(player_ptr->pclass).title);
    exe_write_diary(floor, DiaryKind::DESCRIPTION, 1, mes_class);
    PlayerRealm pr(player_ptr);
    if (pr.realm1().is_available()) {
        const auto mes_realm2 = pr.realm2().is_available() ? fmt::format(_("と{}", " and {} realms"), pr.realm2().get_name()) : _("", " realm");
        const auto mes_realm = fmt::format(_("{}魔法の領域に{}{}を選択した。", "{}chose {}{}."), indent, pr.realm1().get_name(), mes_realm2);
        exe_write_diary(floor, DiaryKind::DESCRIPTION, 1, mes_realm);
    }

    if (player_ptr->element_realm != ElementRealmType::NONE) {
        const auto mes_element = fmt::format(_("{}元素系統に{}を選択した。", "{}chose {} system."), indent, get_element_title(player_ptr->element_realm));
        exe_write_diary(floor, DiaryKind::DESCRIPTION, 1, mes_element);
    }

    const auto mes_personality = fmt::format(_("{}性格に{}を選択した。", "{}chose {} personality."), indent, personality_info[player_ptr->ppersonality].title);
    exe_write_diary(floor, DiaryKind::DESCRIPTION, 1, mes_personality);
    if (PlayerClass(player_ptr).equals(PlayerClassType::CHAOS_WARRIOR)) {
        constexpr auto fmt_patron = _("{}守護神{}と契約を交わした。", "{}made a contract with patron {}.");
        const auto mes_patron = fmt::format(fmt_patron, indent, patron_list[player_ptr->chaos_patron].name);
        exe_write_diary(floor, DiaryKind::DESCRIPTION, 1, mes_patron);
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

    AngbandWorld::get_instance().play_time.reset();
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
    for (size_t i = 1; i < TownList::get_instance().size(); i++) {
        for (auto sst : STORE_SALE_TYPE_LIST) {
            store_init(i, sst);
        }
    }

    WildernessGrids::get_instance().initialize_seeds();
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
