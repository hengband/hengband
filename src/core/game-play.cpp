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
#include "floor/cave.h"
#include "floor/floor-changer.h"
#include "floor/floor-events.h"
#include "floor/floor-leaver.h"
#include "floor/floor-mode-changer.h"
#include "floor/floor-save.h"
#include "floor/floor-util.h"
#include "floor/wild.h"
#include "game-option/cheat-options.h"
#include "game-option/input-options.h"
#include "game-option/play-record-options.h"
#include "game-option/runtime-arguments.h"
#include "grid/feature.h"
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
#include "market/arena-info-table.h"
#include "market/bounty.h"
#include "market/building-initializer.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-lite.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-indice-types.h"
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
#include "realm/realm-names-table.h"
#include "save/save.h"
#include "spell/spells-status.h"
#include "spell/technic-info-table.h"
#include "status/buff-setter.h"
#include "store/home.h"
#include "store/store-util.h"
#include "store/store.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/angband-version.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
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
    w_ptr->character_icky_depth = 1;
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
    if (!player_ptr->wait_report_score) {
        return;
    }

    if (!input_check_strict(player_ptr, _("待機していたスコア登録を今行ないますか？", "Do you register score now? "), UserCheck::NO_HISTORY)) {
        quit(0);
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
    w_ptr->start_time = (uint32_t)time(nullptr);
    signals_ignore_tstp();
    w_ptr->character_icky_depth = 1;
    const auto &path = path_build(ANGBAND_DIR_APEX, "scores.raw");
    highscore_fd = fd_open(path, O_RDWR);

    /* 町名消失バグ対策(#38205)のためここで世界マップ情報を読み出す */
    parse_fixed_map(player_ptr, WILDERNESS_DEFINITION, 0, 0, w_ptr->max_wild_y, w_ptr->max_wild_x);
    bool success = send_world_score(player_ptr, true);
    if (!success && !input_check_strict(player_ptr, _("スコア登録を諦めますか？", "Do you give up score registration? "), UserCheck::NO_HISTORY)) {
        prt(_("引き続き待機します。", "standing by for future registration..."), 0, 0);
        (void)inkey();
    } else {
        player_ptr->wait_report_score = false;
        top_twenty(player_ptr);
        if (!save_player(player_ptr, SaveType::CLOSE_GAME)) {
            msg_print(_("セーブ失敗！", "death save failed!"));
        }
    }

    (void)fd_close(highscore_fd);
    highscore_fd = -1;
    signals_handle_tstp();
    quit(0);
}

static void init_random_seed(PlayerType *player_ptr, bool new_game)
{
    bool init_random_seed = false;
    if (!w_ptr->character_loaded) {
        new_game = true;
        w_ptr->character_dungeon = false;
        init_random_seed = true;
        init_saved_floors(player_ptr, false);
    } else if (new_game) {
        init_saved_floors(player_ptr, true);
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
    w_ptr->character_dungeon = false;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    floor_ptr->reset_dungeon_index();
    floor_ptr->dun_level = 0;
    floor_ptr->quest_number = QuestId::NONE;
    floor_ptr->inside_arena = false;
    player_ptr->phase_out = false;
    write_level = true;
    w_ptr->seed_flavor = randint0(0x10000000);
    w_ptr->seed_town = randint0(0x10000000);
    player_birth(player_ptr);
    counts_write(player_ptr, 2, 0);
    player_ptr->count = 0;
    load = false;
    determine_bounty_uniques(player_ptr);
    determine_daily_bounty(player_ptr, false);
    wipe_o_list(floor_ptr);
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
    exe_write_diary(player_ptr, DiaryKind::GAMESTART, 1, mes);

    if (player_ptr->riding == -1) {
        player_ptr->riding = 0;
        auto *floor_ptr = player_ptr->current_floor_ptr;
        for (MONSTER_IDX i = floor_ptr->m_max; i > 0; i--) {
            if (player_bold(player_ptr, floor_ptr->m_list[i].fy, floor_ptr->m_list[i].fx)) {
                player_ptr->riding = i;
                break;
            }
        }
    }
}

static void reset_world_info(PlayerType *player_ptr)
{
    w_ptr->creating_savefile = false;
    player_ptr->teleport_town = false;
    player_ptr->sutemi = false;
    w_ptr->timewalk_m_idx = 0;
    player_ptr->now_damaged = false;
    now_message = 0;
    w_ptr->start_time = time(nullptr) - 1;
    record_o_name[0] = '\0';
}

static void generate_wilderness(PlayerType *player_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if ((floor_ptr->dun_level == 0) && inside_quest(floor_ptr->quest_number)) {
        return;
    }

    parse_fixed_map(player_ptr, WILDERNESS_DEFINITION, 0, 0, w_ptr->max_wild_y, w_ptr->max_wild_x);
    init_flags = INIT_ONLY_BUILDINGS;
    parse_fixed_map(player_ptr, TOWN_DEFINITION_LIST, 0, 0, MAX_HGT, MAX_WID);
    select_floor_music(player_ptr);
}

static void change_floor_if_error(PlayerType *player_ptr)
{
    if (!w_ptr->character_dungeon) {
        change_floor(player_ptr);
        return;
    }

    if (player_ptr->panic_save == 0) {
        return;
    }

    if (!player_ptr->y || !player_ptr->x) {
        msg_print(_("プレイヤーの位置がおかしい。フロアを再生成します。", "What a strange player location, regenerate the dungeon floor."));
        change_floor(player_ptr);
    }

    if (!player_ptr->y || !player_ptr->x) {
        player_ptr->y = player_ptr->x = 10;
    }

    player_ptr->panic_save = 0;
}

static void generate_world(PlayerType *player_ptr, bool new_game)
{
    reset_world_info(player_ptr);
    auto *floor_ptr = player_ptr->current_floor_ptr;
    panel_row_min = floor_ptr->height;
    panel_col_min = floor_ptr->width;

    set_floor_and_wall(floor_ptr->dungeon_idx);
    initialize_items_flavor();
    prt(_("お待ち下さい...", "Please wait..."), 0, 0);
    term_fresh();
    generate_wilderness(player_ptr);
    change_floor_if_error(player_ptr);
    w_ptr->character_generated = true;
    w_ptr->character_icky_depth = 0;
    if (!new_game) {
        return;
    }

    const auto mes = format(_("%sに降り立った。", "arrived in %s."), map_name(player_ptr).data());
    exe_write_diary(player_ptr, DiaryKind::DESCRIPTION, 0, mes);
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

    MonsterRaceId pet_r_idx = pc.equals(PlayerClassType::CAVALRY) ? MonsterRaceId::HORSE : MonsterRaceId::YASE_HORSE;
    auto *r_ptr = &monraces_info[pet_r_idx];
    place_specific_monster(player_ptr, 0, player_ptr->y, player_ptr->x - 1, pet_r_idx, (PM_FORCE_PET | PM_NO_KAGE));
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[hack_m_idx_ii];
    m_ptr->mspeed = r_ptr->speed;
    m_ptr->maxhp = r_ptr->hdice * (r_ptr->hside + 1) / 2;
    m_ptr->max_maxhp = m_ptr->maxhp;
    m_ptr->hp = r_ptr->hdice * (r_ptr->hside + 1) / 2;
    m_ptr->dealt_damage = 0;
    m_ptr->energy_need = ENERGY_NEED() + ENERGY_NEED();
}

static void decide_arena_death(PlayerType *player_ptr)
{
    if (!player_ptr->playing || !player_ptr->is_dead) {
        return;
    }

    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (!floor_ptr->inside_arena) {
        if ((w_ptr->wizard || cheat_live) && !input_check(_("死にますか? ", "Die? "))) {
            cheat_death(player_ptr);
        }

        return;
    }

    floor_ptr->inside_arena = false;
    if (player_ptr->arena_number > MAX_ARENA_MONS) {
        player_ptr->arena_number++;
    } else {
        player_ptr->arena_number = -1 - player_ptr->arena_number;
    }

    player_ptr->is_dead = false;
    player_ptr->chp = 0;
    player_ptr->chp_frac = 0;
    player_ptr->exit_bldg = true;
    reset_tim_flags(player_ptr);
    prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS | CFM_RAND_CONNECT);
    leave_floor(player_ptr);
}

static void process_game_turn(PlayerType *player_ptr)
{
    bool load_game = true;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    while (true) {
        process_dungeon(player_ptr, load_game);
        w_ptr->character_xtra = true;
        handle_stuff(player_ptr);
        w_ptr->character_xtra = false;
        target_who = 0;
        health_track(player_ptr, 0);
        forget_lite(floor_ptr);
        forget_view(floor_ptr);
        clear_mon_lite(floor_ptr);
        if (!player_ptr->playing && !player_ptr->is_dead) {
            break;
        }

        wipe_o_list(floor_ptr);
        if (!player_ptr->is_dead) {
            wipe_monsters_list(player_ptr);
        }

        msg_print(nullptr);
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
    w_ptr->creating_savefile = new_game;
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
    quit(nullptr);
}
