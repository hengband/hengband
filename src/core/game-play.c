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

#include "angband.h"
#include "core/game-play.h"
#include "io/signal-handlers.h"
#include "util.h"
#include "core/angband-version.h"
#include "core/stuff-handler.h"
#include "inet.h"
#include "gameterm.h"
#include "chuukei.h"
#include "birth/birth.h"
#include "creature.h"
#include "feature.h"
#include "floor/floor-events.h"
#include "floor/floor-save.h"
#include "floor/floor.h"
#include "grid.h"
#include "io/write-diary.h"
#include "market/arena-info-table.h"
#include "market/building.h"
#include "market/store-util.h"
#include "market/store.h"
#include "object-flavor.h"
#include "player-class.h"
#include "player-effects.h"
#include "player-personality.h"
#include "player-race.h"
#include "player-skill.h"
#include "player/process-name.h"
#include "spell/technic-info-table.h"
#include "spells-status.h"
#include "view/display-player.h"
#include "wild.h"
#include "world/world.h"
#include "autopick/autopick-pref-processor.h"
#include "core/game-closer.h"
#include "core/output-updater.h"
#include "core/player-processor.h"
#include "dungeon/dungeon-file.h"
#include "dungeon/dungeon-processor.h"
#include "io/input-key-processor.h"
#include "io/read-pref-file.h"
#include "realm/realm.h"
#include "save.h"
#include "scores.h"
#include "targeting.h"
#include "view/display-main-window.h"
#include "core/system-variables.h"

/*!
 * @brief 1ゲームプレイの主要ルーチン / Actually play a game
 * @return なし
 * @note
 * If the "new_game" parameter is true, then, after loading the
 * savefile, we will commit suicide, if necessary, to allow the
 * player to start a new game.
 */
void play_game(player_type* player_ptr, bool new_game)
{
    bool load_game = TRUE;
    bool init_random_seed = FALSE;

#ifdef CHUUKEI
    if (chuukei_client) {
        reset_visuals(player_ptr, process_autopick_file_command);
        browse_chuukei();
        return;
    }

    else if (chuukei_server) {
        prepare_chuukei_hooks();
    }
#endif

    if (browsing_movie) {
        reset_visuals(player_ptr, process_autopick_file_command);
        browse_movie();
        return;
    }

    player_ptr->hack_mutation = FALSE;
    current_world_ptr->character_icky = TRUE;
    Term_activate(angband_term[0]);
    angband_term[0]->resize_hook = resize_map;
    for (MONSTER_IDX i = 1; i < 8; i++) {
        if (angband_term[i]) {
            angband_term[i]->resize_hook = redraw_window;
        }
    }

    (void)Term_set_cursor(0);
    if (!load_player(player_ptr)) {
        quit(_("セーブファイルが壊れています", "broken savefile"));
    }

    extract_option_vars();
    if (player_ptr->wait_report_score) {
        char buf[1024];
        bool success;

        if (!get_check_strict(_("待機していたスコア登録を今行ないますか？", "Do you register score now? "), CHECK_NO_HISTORY))
            quit(0);

        player_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
        update_creature(player_ptr);
        player_ptr->is_dead = TRUE;
        current_world_ptr->start_time = (u32b)time(NULL);
        signals_ignore_tstp();
        current_world_ptr->character_icky = TRUE;
        path_build(buf, sizeof(buf), ANGBAND_DIR_APEX, "scores.raw");
        highscore_fd = fd_open(buf, O_RDWR);

        /* 町名消失バグ対策(#38205)のためここで世界マップ情報を読み出す */
        process_dungeon_file(player_ptr, "w_info.txt", 0, 0, current_world_ptr->max_wild_y, current_world_ptr->max_wild_x);
        success = send_world_score(player_ptr, TRUE, update_playtime, display_player, map_name);

        if (!success && !get_check_strict(_("スコア登録を諦めますか？", "Do you give up score registration? "), CHECK_NO_HISTORY)) {
            prt(_("引き続き待機します。", "standing by for future registration..."), 0, 0);
            (void)inkey();
        } else {
            player_ptr->wait_report_score = FALSE;
            top_twenty(player_ptr);
            if (!save_player(player_ptr))
                msg_print(_("セーブ失敗！", "death save failed!"));
        }

        (void)fd_close(highscore_fd);
        highscore_fd = -1;
        signals_handle_tstp();

        quit(0);
    }

    current_world_ptr->creating_savefile = new_game;

    if (!current_world_ptr->character_loaded) {
        new_game = TRUE;
        current_world_ptr->character_dungeon = FALSE;
        init_random_seed = TRUE;
        init_saved_floors(player_ptr, FALSE);
    } else if (new_game) {
        init_saved_floors(player_ptr, TRUE);
    }

    if (!new_game) {
        process_player_name(player_ptr, FALSE);
    }

    if (init_random_seed) {
        Rand_state_init();
    }

    floor_type* floor_ptr = player_ptr->current_floor_ptr;
    if (new_game) {
        current_world_ptr->character_dungeon = FALSE;

        floor_ptr->dun_level = 0;
        floor_ptr->inside_quest = 0;
        floor_ptr->inside_arena = FALSE;
        player_ptr->phase_out = FALSE;
        write_level = TRUE;

        current_world_ptr->seed_flavor = randint0(0x10000000);
        current_world_ptr->seed_town = randint0(0x10000000);

        player_birth(player_ptr, process_autopick_file_command);
        counts_write(player_ptr, 2, 0);
        player_ptr->count = 0;
        load = FALSE;
        determine_bounty_uniques(player_ptr);
        determine_daily_bounty(player_ptr, FALSE);
        wipe_o_list(floor_ptr);
    } else {
        write_level = FALSE;
        exe_write_diary(player_ptr, DIARY_GAMESTART, 1,
            _("                            ----ゲーム再開----",
                "                            --- Restarted Game ---"));

        /*
		 * todo もう2.2.Xなので互換性は打ち切ってもいいのでは？
		 * 1.0.9 以前はセーブ前に player_ptr->riding = -1 としていたので、再設定が必要だった。
		 * もう不要だが、以前のセーブファイルとの互換のために残しておく。
		 */
        if (player_ptr->riding == -1) {
            player_ptr->riding = 0;
            for (MONSTER_IDX i = floor_ptr->m_max; i > 0; i--) {
                if (player_bold(player_ptr, floor_ptr->m_list[i].fy, floor_ptr->m_list[i].fx)) {
                    player_ptr->riding = i;
                    break;
                }
            }
        }
    }

    current_world_ptr->creating_savefile = FALSE;

    player_ptr->teleport_town = FALSE;
    player_ptr->sutemi = FALSE;
    current_world_ptr->timewalk_m_idx = 0;
    player_ptr->now_damaged = FALSE;
    now_message = 0;
    current_world_ptr->start_time = time(NULL) - 1;
    record_o_name[0] = '\0';

    panel_row_min = floor_ptr->height;
    panel_col_min = floor_ptr->width;
    if (player_ptr->pseikaku == SEIKAKU_SEXY)
        s_info[player_ptr->pclass].w_max[TV_HAFTED - TV_WEAPON_BEGIN][SV_WHIP] = WEAPON_EXP_MASTER;

    set_floor_and_wall(player_ptr->dungeon_idx);
    flavor_init();
    prt(_("お待ち下さい...", "Please wait..."), 0, 0);
    Term_fresh();

    if (arg_wizard) {
        if (enter_wizard_mode(player_ptr)) {
            current_world_ptr->wizard = TRUE;

            if (player_ptr->is_dead || !player_ptr->y || !player_ptr->x) {
                init_saved_floors(player_ptr, TRUE);
                floor_ptr->inside_quest = 0;
                player_ptr->y = player_ptr->x = 10;
            }
        } else if (player_ptr->is_dead) {
            quit("Already dead.");
        }
    }

    if (!floor_ptr->dun_level && !floor_ptr->inside_quest) {
        process_dungeon_file(player_ptr, "w_info.txt", 0, 0, current_world_ptr->max_wild_y, current_world_ptr->max_wild_x);
        init_flags = INIT_ONLY_BUILDINGS;
        process_dungeon_file(player_ptr, "t_info.txt", 0, 0, MAX_HGT, MAX_WID);
        select_floor_music(player_ptr);
    }

    if (!current_world_ptr->character_dungeon) {
        change_floor(player_ptr);
    } else {
        if (player_ptr->panic_save) {
            if (!player_ptr->y || !player_ptr->x) {
                msg_print(_("プレイヤーの位置がおかしい。フロアを再生成します。", "What a strange player location, regenerate the dungeon floor."));
                change_floor(player_ptr);
            }

            if (!player_ptr->y || !player_ptr->x)
                player_ptr->y = player_ptr->x = 10;

            player_ptr->panic_save = 0;
        }
    }

    current_world_ptr->character_generated = TRUE;
    current_world_ptr->character_icky = FALSE;

    if (new_game) {
        char buf[80];
        sprintf(buf, _("%sに降り立った。", "arrived in %s."), map_name(player_ptr));
        exe_write_diary(player_ptr, DIARY_DESCRIPTION, 0, buf);
    }

    player_ptr->playing = TRUE;
    reset_visuals(player_ptr, process_autopick_file_command);
    load_all_pref_files(player_ptr);
    if (new_game) {
        player_outfit(player_ptr);
    }

    Term_xtra(TERM_XTRA_REACT, 0);

    player_ptr->window |= (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER);
    player_ptr->window |= (PW_MESSAGE | PW_OVERHEAD | PW_DUNGEON | PW_MONSTER | PW_OBJECT);
    handle_stuff(player_ptr);

    if (arg_force_original)
        rogue_like_commands = FALSE;
    if (arg_force_roguelike)
        rogue_like_commands = TRUE;

    if (player_ptr->chp < 0)
        player_ptr->is_dead = TRUE;

    if (player_ptr->prace == RACE_ANDROID)
        calc_android_exp(player_ptr);

    if (new_game && ((player_ptr->pclass == CLASS_CAVALRY) || (player_ptr->pclass == CLASS_BEASTMASTER))) {
        monster_type* m_ptr;
        MONRACE_IDX pet_r_idx = ((player_ptr->pclass == CLASS_CAVALRY) ? MON_HORSE : MON_YASE_HORSE);
        monster_race* r_ptr = &r_info[pet_r_idx];
        place_monster_aux(player_ptr, 0, player_ptr->y, player_ptr->x - 1, pet_r_idx,
            (PM_FORCE_PET | PM_NO_KAGE));
        m_ptr = &floor_ptr->m_list[hack_m_idx_ii];
        m_ptr->mspeed = r_ptr->speed;
        m_ptr->maxhp = r_ptr->hdice * (r_ptr->hside + 1) / 2;
        m_ptr->max_maxhp = m_ptr->maxhp;
        m_ptr->hp = r_ptr->hdice * (r_ptr->hside + 1) / 2;
        m_ptr->dealt_damage = 0;
        m_ptr->energy_need = ENERGY_NEED() + ENERGY_NEED();
    }

    (void)combine_and_reorder_home(STORE_HOME);
    (void)combine_and_reorder_home(STORE_MUSEUM);
    select_floor_music(player_ptr);

    while (TRUE) {
        process_dungeon(player_ptr, load_game);
        current_world_ptr->character_xtra = TRUE;
        handle_stuff(player_ptr);

        current_world_ptr->character_xtra = FALSE;
        target_who = 0;
        health_track(player_ptr, 0);
        forget_lite(floor_ptr);
        forget_view(floor_ptr);
        clear_mon_lite(floor_ptr);
        if (!player_ptr->playing && !player_ptr->is_dead)
            break;

        wipe_o_list(floor_ptr);
        if (!player_ptr->is_dead)
            wipe_monsters_list(player_ptr);

        msg_print(NULL);
        load_game = FALSE;
        if (player_ptr->playing && player_ptr->is_dead) {
            if (floor_ptr->inside_arena) {
                floor_ptr->inside_arena = FALSE;
                if (player_ptr->arena_number > MAX_ARENA_MONS)
                    player_ptr->arena_number++;
                else
                    player_ptr->arena_number = -1 - player_ptr->arena_number;
                player_ptr->is_dead = FALSE;
                player_ptr->chp = 0;
                player_ptr->chp_frac = 0;
                player_ptr->exit_bldg = TRUE;
                reset_tim_flags(player_ptr);
                prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS | CFM_RAND_CONNECT);
                leave_floor(player_ptr);
            } else {
                if ((current_world_ptr->wizard || cheat_live) && !get_check(_("死にますか? ", "Die? "))) {
                    cheat_death(player_ptr);
                }
            }
        }

        if (player_ptr->is_dead)
            break;

        change_floor(player_ptr);
    }

    close_game(player_ptr);
    quit(NULL);
}
