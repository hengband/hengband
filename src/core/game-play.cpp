/*!
 * @brief ゲームプレイのメインルーチン
 * @date 2020/05/10
 * @author Hourier
 * @details
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 * 2013 Deskull rearranged comment for Doxygen.
 */

#include "core/game-play.h"
#include "autopick/autopick-pref-processor.h"
#include "birth/character-builder.h"
#include "birth/inventory-initializer.h"
#include "cmd-io/cmd-gameoption.h"
#include "core/asking-player.h"
#include "core/game-closer.h"
#include "core/player-processor.h"
#include "core/score-util.h"
#include "core/scores.h"
#include "core/speed-table.h"
#include "core/stuff-handler.h"
#include "core/visuals-reseter.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-processor.h"
#include "dungeon/quest.h"
#include "floor/floor-changer.h"
#include "floor/floor-events.h"
#include "floor/floor-leaver.h"
#include "floor/floor-mode-changer.h"
#include "floor/floor-save.h"
#include "floor/floor-util.h"
#include "game-option/cheat-options.h"
#include "game-option/input-options.h"
#include "game-option/play-record-options.h"
#include "game-option/runtime-arguments.h"
#include "grid/grid.h"
#include "info-reader/fixed-map-parser.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-processor.h"
#include "io/read-pref-file.h"
#include "io/record-play-movie.h"
#include "io/screen-util.h"
#include "io/signal-handlers.h"
#include "io/write-diary.h"
#include "item-info/flavor-initializer.h"
#include "load/load.h"
#include "main/sound-of-music.h"
#include "market/arena-entry.h"
#include "market/bounty.h"
#include "market/building-initializer.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-lite.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-util.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/class-info.h"
#include "player-info/race-types.h"
#include "player/player-personality-types.h"
#include "player/player-skill.h"
#include "player/player-status.h"
#include "player/process-name.h"
#include "racial/racial-android.h"
#include "save/save.h"
#include "spell/spells-status.h"
#include "spell/technic-info-table.h"
#include "status/buff-setter.h"
#include "store/home.h"
#include "store/store-util.h"
#include "store/store.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/angband-system.h"
#include "system/angband-version.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/floor/wilderness-grid.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-checker.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include "view/display-player.h"
#include "window/main-window-util.h"
#include "wizard/wizard-special-process.h"
#include "world/world.h"

static void restore_windows(PlayerType *player_ptr)
{
    player_ptr->hack_mutation = false;
    AngbandWorld::get_instance().character_icky_depth = 1;
    term_activate(angband_terms[0]);
    angband_terms[0]->resize_hook = resize_map;
    for (auto i = 1U; i < angband_terms.size(); ++i) {
        if (angband_terms[i]) {
            angband_terms[i]->resize_hook = redraw_window;
        }
    }

    (void)term_set_cursor(0);
}

static void send_waiting_record(PlayerType *player_ptr)
{
    auto &system = AngbandSystem::get_instance();
    if (!system.is_awaiting_report_status()) {
        return;
    }

    if (!input_check_strict(player_ptr, _("待機していたスコア登録を今行ないますか？", "Do you register score now? "), UserCheck::NO_HISTORY)) {
        quit("");
    }

    static constexpr auto flags = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
        StatusRecalculatingFlag::MP,
        StatusRecalculatingFlag::SPELLS,
    };
    RedrawingFlagsUpdater::get_instance().set_flags(flags);
    update_creature(player_ptr);
    player_ptr->is_dead = true;
    auto &world = AngbandWorld::get_instance();
    world.play_time.pause();
    signals_ignore_tstp();
    world.character_icky_depth = 1;
    const auto path = path_build(ANGBAND_DIR_APEX, "scores.raw");
    highscore_fd = fd_open(path, O_RDWR);

    /* 町名消失バグ対策(#38205)のためここで世界マップ情報を読み出す */
    const auto &area = WildernessGrids::get_instance().get_area();
    parse_fixed_map(player_ptr, WILDERNESS_DEFINITION, 0, 0, area.height(), area.width());
    bool success = send_world_score(player_ptr, true);
    if (!success && !input_check_strict(player_ptr, _("スコア登録を諦めますか？", "Do you give up score registration? "), UserCheck::NO_HISTORY)) {
        prt(_("引き続き待機します。", "standing by for future registration..."), 0, 0);
        (void)inkey();
    } else {
        system.set_awaiting_report_score(false);
        top_twenty(player_ptr);
        if (!save_player(player_ptr, SaveType::CLOSE_GAME)) {
            msg_print(_("セーブ失敗！", "death save failed!"));
        }
    }

    (void)fd_close(highscore_fd);
    highscore_fd = -1;
    signals_handle_tstp();
    quit("");
}

static void init_random_seed(PlayerType *player_ptr, bool new_game)
{
    auto &world = AngbandWorld::get_instance();
    auto init_random_seed = false;
    if (!world.character_loaded) {
        new_game = true;
        world.character_dungeon = false;
        init_random_seed = true;
        init_saved_floors(false);
    } else if (new_game) {
        init_saved_floors(true);
    }

    if (!new_game) {
        process_player_name(player_ptr);
    }

    if (init_random_seed) {
        Rand_state_init();
    }
}

static void init_world_floor_info(PlayerType *player_ptr)
{
    AngbandWorld::get_instance().character_dungeon = false;
    auto &floor = *player_ptr->current_floor_ptr;
    floor.reset_dungeon_index();
    floor.dun_level = 0;
    floor.quest_number = QuestId::NONE;
    floor.inside_arena = false;
    AngbandSystem::get_instance().set_phase_out(false);
    write_level = true;
    auto &system = AngbandSystem::get_instance();
    system.set_seed_flavor(randint0(0x10000000));
    system.set_seed_town(randint0(0x10000000));
    player_birth(player_ptr);
    counts_write(player_ptr, 2, 0);
    player_ptr->count = 0;
    load = false;
    determine_bounty_uniques(player_ptr);
    determine_daily_bounty(player_ptr);
    wipe_o_list(floor);
}

/*!
 * @brief フロア情報をゲームロード時に復帰
 * @todo 3.0.Xで削除予定
 * 1.0.9 以前はセーブ前に player_ptr->riding = -1 としていたので、再設定が必要だった。
 * もう不要だが、以前のセーブファイルとの互換のために残しておく。
 */
static void restore_world_floor_info(PlayerType *player_ptr)
{
    write_level = false;
    constexpr auto mes = _("                            ----ゲーム再開----", "                            --- Restarted Game ---");
    const auto &floor = *player_ptr->current_floor_ptr;
    exe_write_diary(floor, DiaryKind::GAMESTART, 1, mes);

    if (player_ptr->riding != -1) {
        return;
    }

    player_ptr->ride_monster(0);
    for (short i = floor.m_max; i > 0; i--) {
        const auto &monster = floor.m_list[i];
        if (player_ptr->is_located_at({ monster.fy, monster.fx })) {
            player_ptr->ride_monster(i);
            break;
        }
    }
}

static void reset_world_info(PlayerType *player_ptr)
{
    auto &world = AngbandWorld::get_instance();
    world.creating_savefile = false;
    player_ptr->teleport_town = false;
    player_ptr->sutemi = false;
    world.timewalk_m_idx = 0;
    player_ptr->now_damaged = false;
    now_message = 0;
    record_item_name.clear();
}

static void generate_wilderness(PlayerType *player_ptr)
{
    const auto &area = WildernessGrids::get_instance().get_area();
    parse_fixed_map(player_ptr, WILDERNESS_DEFINITION, 0, 0, area.height(), area.width());
    init_flags = INIT_ONLY_BUILDINGS;
    parse_fixed_map(player_ptr, TOWN_DEFINITION_LIST, 0, 0, MAX_HGT, MAX_WID);
    select_floor_music(player_ptr);
}

static void change_floor_if_error(PlayerType *player_ptr)
{
    if (!AngbandWorld::get_instance().character_dungeon) {
        change_floor(player_ptr);
        return;
    }

    auto &system = AngbandSystem::get_instance();
    if (!system.is_panic_save_executed()) {
        return;
    }

    if (!player_ptr->y || !player_ptr->x) {
        msg_print(_("プレイヤーの位置がおかしい。フロアを再生成します。", "What a strange player location, regenerate the dungeon floor."));
        change_floor(player_ptr);
    }

    if (!player_ptr->y || !player_ptr->x) {
        player_ptr->y = player_ptr->x = 10;
    }

    system.set_panic_save(false);
}

static void generate_world(PlayerType *player_ptr, bool new_game)
{
    reset_world_info(player_ptr);
    const auto &floor = *player_ptr->current_floor_ptr;
    panel_row_min = floor.height;
    panel_col_min = floor.width;

    initialize_items_flavor();
    prt(_("お待ち下さい...", "Please wait..."), 0, 0);
    term_fresh();
    generate_wilderness(player_ptr);
    change_floor_if_error(player_ptr);
    auto &world = AngbandWorld::get_instance();
    world.character_generated = true;
    world.character_icky_depth = 0;
    if (!new_game) {
        return;
    }

    const auto mes = format(_("%sに降り立った。", "arrived in %s."), map_name(player_ptr).data());
    exe_write_diary(floor, DiaryKind::DESCRIPTION, 0, mes);
}

static void init_io(PlayerType *player_ptr)
{
    term_xtra(TERM_XTRA_REACT, 0);
    RedrawingFlagsUpdater::get_instance().fill_up_sub_flags();
    handle_stuff(player_ptr);
    if (arg_force_original) {
        rogue_like_commands = false;
    }

    if (arg_force_roguelike) {
        rogue_like_commands = true;
    }
}

static void init_riding_pet(PlayerType *player_ptr, bool new_game)
{
    PlayerClass pc(player_ptr);
    if (!new_game || !pc.is_tamer()) {
        return;
    }

    const auto pet_id = pc.equals(PlayerClassType::CAVALRY) ? MonraceId::HORSE : MonraceId::YASE_HORSE;
    const auto &monrace = MonraceList::get_instance().get_monrace(pet_id);
    const auto m_idx = place_specific_monster(player_ptr, player_ptr->y, player_ptr->x - 1, pet_id, (PM_FORCE_PET | PM_NO_KAGE));
    auto &monster = player_ptr->current_floor_ptr->m_list[*m_idx];
    monster.mspeed = monrace.speed;
    monster.maxhp = monrace.hit_dice.floored_expected_value();
    monster.max_maxhp = monster.maxhp;
    monster.hp = monrace.hit_dice.floored_expected_value();
    monster.dealt_damage = 0;
    monster.energy_need = ENERGY_NEED() + ENERGY_NEED();
}

static void decide_arena_death(PlayerType *player_ptr)
{
    if (!player_ptr->playing || !player_ptr->is_dead) {
        return;
    }

    auto &world = AngbandWorld::get_instance();
    auto &floor = *player_ptr->current_floor_ptr;
    if (!floor.inside_arena) {
        if ((world.wizard || cheat_live) && !input_check(_("死にますか? ", "Die? "))) {
            cheat_death(player_ptr);
        }

        return;
    }

    floor.inside_arena = false;
    auto &entries = ArenaEntryList::get_instance();
    if (entries.is_player_true_victor()) {
        entries.increment_entry();
    } else {
        entries.set_defeated_entry();
    }

    player_ptr->is_dead = false;
    player_ptr->chp = 0;
    player_ptr->chp_frac = 0;
    world.set_arena(true);
    reset_tim_flags(player_ptr);
    FloorChangeModesStore::get_instace()->set({ FloorChangeMode::SAVE_FLOORS, FloorChangeMode::RANDOM_CONNECT });
    leave_floor(player_ptr);
}

static void process_game_turn(PlayerType *player_ptr)
{
    auto load_game = true;
    auto &floor = *player_ptr->current_floor_ptr;
    auto &world = AngbandWorld::get_instance();
    world.play_time.unpause();
    while (true) {
        process_dungeon(player_ptr, load_game);
        world.character_xtra = true;
        handle_stuff(player_ptr);
        world.character_xtra = false;
        Target::clear_last_target();
        health_track(player_ptr, 0);
        floor.forget_lite();
        forget_view(floor);
        clear_mon_lite(floor);
        if (!player_ptr->playing && !player_ptr->is_dead) {
            break;
        }

        wipe_o_list(floor);
        if (!player_ptr->is_dead) {
            wipe_monsters_list(player_ptr);
        }

        msg_erase();
        load_game = false;
        decide_arena_death(player_ptr);
        if (player_ptr->is_dead) {
            break;
        }

        change_floor(player_ptr);
    }
}

/*!
 * @brief 1ゲームプレイの主要ルーチン / Actually play a game
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param new_game 新規にゲームを始めたかどうか
 * @param browsing_movie ムービーモードか
 * @note
 * If the "new_game" parameter is true, then, after loading the
 * savefile, we will commit suicide, if necessary, to allow the
 * player to start a new game.
 */
void play_game(PlayerType *player_ptr, bool new_game, bool browsing_movie)
{
    if (browsing_movie) {
        reset_visuals(player_ptr);
        browse_movie();
        return;
    }

    restore_windows(player_ptr);
    if (!load_savedata(player_ptr, &new_game)) {
        quit(_("セーブファイルが壊れています", "broken savefile"));
    }

    extract_option_vars();
    send_waiting_record(player_ptr);
    AngbandWorld::get_instance().creating_savefile = new_game;
    init_random_seed(player_ptr, new_game);
    if (new_game) {
        init_world_floor_info(player_ptr);
    } else {
        restore_world_floor_info(player_ptr);
    }

    generate_world(player_ptr, new_game);
    player_ptr->playing = true;
    reset_visuals(player_ptr);
    load_all_pref_files(player_ptr);
    if (new_game) {
        player_outfit(player_ptr);
    }

    init_io(player_ptr);
    if (player_ptr->chp < 0 && !cheat_immortal) {
        player_ptr->is_dead = true;
    }

    if (PlayerRace(player_ptr).equals(PlayerRaceType::ANDROID)) {
        calc_android_exp(player_ptr);
    }

    init_riding_pet(player_ptr, new_game);
    (void)combine_and_reorder_home(player_ptr, StoreSaleType::HOME);
    (void)combine_and_reorder_home(player_ptr, StoreSaleType::MUSEUM);
    select_floor_music(player_ptr);
    process_game_turn(player_ptr);
    close_game(player_ptr);
    quit("");
}
