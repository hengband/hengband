#include "dungeon/dungeon-processor.h"
#include "cmd-io/cmd-dump.h"
#include "core/disturbance.h"
#include "core/object-compressor.h"
#include "core/player-processor.h"
#include "core/stuff-handler.h"
#include "core/turn-compensator.h"
#include "dungeon/quest.h"
#include "floor/floor-leaver.h"
#include "game-option/map-screen-options.h"
#include "game-option/play-record-options.h"
#include "hpmp/hp-mp-regenerator.h"
#include "io/cursor.h"
#include "io/input-key-requester.h"
#include "io/write-diary.h"
#include "locale/language-switcher.h"
#include "mind/mind-ninja.h"
#include "monster/monster-compaction.h"
#include "monster/monster-processor.h"
#include "pet/pet-util.h"
#include "player-base/player-class.h"
#include "realm/realm-song-numbers.h"
#include "spell-realm/spells-song.h"
#include "system/building-type-definition.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-record.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/floor/floor-info.h"
#include "system/floor/wilderness-grid.h"
#include "system/monrace/monrace-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-checker.h"
#include "view/display-messages.h"
#include "world/world-turn-processor.h"
#include "world/world.h"

static void redraw_character_xtra(PlayerType *player_ptr)
{
    auto &world = AngbandWorld::get_instance();
    world.character_xtra = true;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::SPELL,
        SubWindowRedrawingFlag::PLAYER,
        SubWindowRedrawingFlag::MONSTER_LORE,
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    rfu.set_flags(flags_swrf);
    static constexpr auto flags_mwrf = {
        MainWindowRedrawingFlag::WIPE,
        MainWindowRedrawingFlag::BASIC,
        MainWindowRedrawingFlag::EXTRA,
        MainWindowRedrawingFlag::EQUIPPY,
        MainWindowRedrawingFlag::MAP,
    };
    rfu.set_flags(flags_mwrf);
    auto flags_srf = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
        StatusRecalculatingFlag::MP,
        StatusRecalculatingFlag::SPELLS,
        StatusRecalculatingFlag::VIEW,
        StatusRecalculatingFlag::LITE,
        StatusRecalculatingFlag::MONSTER_LITE,
        StatusRecalculatingFlag::TORCH,
        StatusRecalculatingFlag::MONSTER_STATUSES,
        StatusRecalculatingFlag::DISTANCE,
        StatusRecalculatingFlag::FLOW,
    };
    rfu.set_flags(flags_srf);
    handle_stuff(player_ptr);
    world.character_xtra = false;
}

/*!
 * process_player()、process_world() をcore.c から移設するのが先.
 * process_upkeep_with_speed() はこの関数と同じところでOK
 * @brief 現在プレイヤーがいるダンジョンの全体処理 / Interact with the current dungeon level.
 * @details
 * <p>
 * この関数から現在の階層を出る、プレイヤーがキャラが死ぬ、
 * ゲームを終了するかのいずれかまでループする。
 * </p>
 * <p>
 * This function will not exit until the level is completed,\n
 * the user dies, or the game is terminated.\n
 * </p>
 */
void process_dungeon(PlayerType *player_ptr, bool load_game)
{
    auto &floor = *player_ptr->current_floor_ptr;
    floor.base_level = floor.dun_level;
    auto &world = AngbandWorld::get_instance();
    world.is_loading_now = false;
    player_ptr->leaving = false;

    command_cmd = 0;
    command_rep = 0;
    command_arg = 0;
    command_dir = 0;

    target_who = 0;
    player_ptr->pet_t_m_idx = 0;
    player_ptr->riding_t_m_idx = 0;
    player_ptr->ambush_flag = false;
    health_track(player_ptr, 0);

    disturb(player_ptr, true, true);
    const auto quest_id = floor.get_quest_id();
    auto &quests = QuestList::get_instance();
    auto &monrace_questor = quests.get_quest(quest_id).get_bounty();
    if (inside_quest(quest_id)) {
        monrace_questor.misc_flags.set(MonsterMiscType::QUESTOR);
    }

    if (player_ptr->max_plv < player_ptr->lev) {
        player_ptr->max_plv = player_ptr->lev;
    }

    auto &dungeon_record = DungeonRecords::get_instance().get_record(floor.dungeon_id);
    if ((dungeon_record.get_max_level() < floor.dun_level) && !floor.is_in_quest()) {
        dungeon_record.set_max_level(floor.dun_level);
        if (record_maxdepth) {
            exe_write_diary(floor, DiaryKind::MAXDEAPTH, floor.dun_level);
        }
    }

    (void)calculate_upkeep(player_ptr);
    panel_bounds_center();
    verify_panel(player_ptr);
    msg_erase();

    redraw_character_xtra(player_ptr);
    auto flags_srf = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
        StatusRecalculatingFlag::MP,
        StatusRecalculatingFlag::SPELLS,
        StatusRecalculatingFlag::COMBINATION,
        StatusRecalculatingFlag::REORDER,
    };
    RedrawingFlagsUpdater::get_instance().set_flags(flags_srf);
    handle_stuff(player_ptr);
    term_fresh();

    auto no_feeling_quest = (quest_id == QuestId::OBERON);
    no_feeling_quest |= (quest_id == QuestId::SERPENT);
    no_feeling_quest |= none_bits(quests.get_quest(quest_id).flags, QUEST_FLAG_PRESET);
    if (inside_quest(quest_id) && QuestType::is_fixed(quest_id) && !no_feeling_quest) {
        do_cmd_feeling(player_ptr);
    }

    const auto is_watching = AngbandSystem::get_instance().is_phase_out();
    if (is_watching) {
        if (load_game) {
            player_ptr->energy_need = 0;
            auto &melee_arena = MeleeArena::get_instance();
            melee_arena.update_gladiators(player_ptr);
        } else {
            msg_print(_("試合開始！", "Ready..Fight!"));
            msg_print(nullptr);
        }
    }

    if (PlayerClass(player_ptr).equals(PlayerClassType::BARD) && (get_singing_song_effect(player_ptr) > MUSIC_DETECT)) {
        set_singing_song_effect(player_ptr, MUSIC_DETECT);
    }

    if (!player_ptr->playing || player_ptr->is_dead) {
        return;
    }

    if (!floor.is_in_quest() && (floor.dungeon_id == DungeonId::ANGBAND)) {
        const auto random_quest_id = floor.get_random_quest_id();
        quest_discovery(random_quest_id);
        floor.quest_number = random_quest_id;
    }

    const auto &dungeon = floor.get_dungeon_definition();
    if (dungeon.has_guardian() && (floor.dun_level == dungeon.maxdepth)) {
        const auto &monrace = dungeon.get_guardian();
        if (!monrace.is_dead_unique()) {
#ifdef JP
            msg_format("この階には%sの主である%sが棲んでいる。", dungeon.name.data(), monrace.name.data());
#else
            msg_format("%s^ lives in this level as the keeper of %s.", monrace.name.data(), dungeon.name.data());
#endif
        }
    }

    if (!load_game) {
        set_superstealth(player_ptr, false);
    }

    floor.monster_level = floor.base_level;
    floor.object_level = floor.base_level;
    world.is_loading_now = true;
    if (player_ptr->energy_need > 0 && !is_watching && (floor.is_underground() || floor.is_leaving_dungeon() || floor.inside_arena)) {
        player_ptr->energy_need = 0;
    }

    floor.leave_dungeon(false);
    floor.reset_mproc();

    while (true) {
        if ((floor.m_cnt + 32 > MAX_FLOOR_MONSTERS) && !is_watching) {
            compact_monsters(player_ptr, 64);
        }

        if ((floor.m_cnt + 32 < floor.m_max) && !is_watching) {
            compact_monsters(player_ptr, 0);
        }

        if (floor.o_cnt + 32 > MAX_FLOOR_ITEMS) {
            compact_objects(player_ptr, 64);
        }

        if (floor.o_cnt + 32 < floor.o_max) {
            compact_objects(player_ptr, 0);
        }

        process_player(player_ptr);
        process_upkeep_with_speed(player_ptr);
        handle_stuff(player_ptr);

        move_cursor_relative(player_ptr->y, player_ptr->x);
        if (fresh_after) {
            term_fresh_force();
        }

        if (!player_ptr->playing || player_ptr->is_dead) {
            break;
        }

        process_monsters(player_ptr);
        handle_stuff(player_ptr);

        move_cursor_relative(player_ptr->y, player_ptr->x);
        if (fresh_after) {
            term_fresh_force();
        }

        if (!player_ptr->playing || player_ptr->is_dead) {
            break;
        }

        WorldTurnProcessor(player_ptr).process_world();
        handle_stuff(player_ptr);

        move_cursor_relative(player_ptr->y, player_ptr->x);
        if (fresh_after) {
            term_fresh_force();
        }

        if (!player_ptr->playing || player_ptr->is_dead) {
            break;
        }

        world.game_turn++;
        const auto is_wild_mode = world.is_wild_mode();
        if (world.dungeon_turn < world.dungeon_turn_limit) {
            if (!is_wild_mode || wild_regen) {
                world.dungeon_turn++;
            } else if (is_wild_mode && !(world.game_turn % ((MAX_HGT + MAX_WID) / 2))) {
                world.dungeon_turn++;
            }
        }

        prevent_turn_overflow(player_ptr);

        if (player_ptr->leaving) {
            break;
        }

        if (wild_regen) {
            wild_regen--;
        }
    }

    if ((inside_quest(quest_id)) && monrace_questor.kind_flags.has_not(MonsterKindType::UNIQUE)) {
        monrace_questor.misc_flags.reset(MonsterMiscType::QUESTOR);
    }

    if (player_ptr->playing && !player_ptr->is_dead) {
        /*
         * Maintain Unique monsters and artifact, save current
         * floor, then prepare next floor
         */
        leave_floor(player_ptr);
        WildernessGrids::get_instance().set_reinitialization(false);
    }

    write_level = true;
}
