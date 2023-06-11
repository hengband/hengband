#include "core/player-processor.h"
#include "action/run-execution.h"
#include "action/travel-execution.h"
#include "core/disturbance.h"
#include "core/special-internal-keys.h"
#include "core/speed-table.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "floor/floor-save-util.h"
#include "floor/floor-util.h"
#include "floor/geometry.h"
#include "floor/wild.h"
#include "game-option/cheat-options.h"
#include "game-option/disturbance-options.h"
#include "game-option/map-screen-options.h"
#include "grid/grid.h"
#include "inventory/pack-overflow.h"
#include "io/cursor.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-processor.h"
#include "io/input-key-requester.h"
#include "mind/mind-force-trainer.h"
#include "mind/mind-sniper.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-describer.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-list.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "monster/monster-util.h"
#include "mutation/mutation-investor-remover.h"
#include "player-base/player-class.h"
#include "player-info/bluemage-data-type.h"
#include "player-info/mane-data-type.h"
#include "player-info/samurai-data-type.h"
#include "player-info/sniper-data-type.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/eldritch-horror.h"
#include "player/player-skill.h"
#include "player/special-defense-types.h"
#include "spell-kind/spells-random.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "status/action-setter.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "timed-effect/player-blindness.h"
#include "timed-effect/player-confusion.h"
#include "timed-effect/player-cut.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/player-paralysis.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "window/display-sub-windows.h"
#include "world/world-turn-processor.h"

bool load = true;
bool can_save = false;

static void process_fishing(PlayerType *player_ptr)
{
    term_xtra(TERM_XTRA_DELAY, 10);
    if (one_in_(1000)) {
        bool success = false;
        get_mon_num_prep(player_ptr, monster_is_fishing_target, nullptr);
        auto *floor_ptr = player_ptr->current_floor_ptr;
        const auto wild_level = wilderness[player_ptr->wilderness_y][player_ptr->wilderness_x].level;
        const auto level = floor_ptr->is_in_dungeon() ? floor_ptr->dun_level : wild_level;
        const auto r_idx = get_mon_num(player_ptr, 0, level, 0);
        msg_print(nullptr);
        if (MonsterRace(r_idx).is_valid() && one_in_(2)) {
            POSITION y, x;
            y = player_ptr->y + ddy[player_ptr->fishing_dir];
            x = player_ptr->x + ddx[player_ptr->fishing_dir];
            if (place_specific_monster(player_ptr, 0, y, x, r_idx, PM_NO_KAGE)) {
                const auto m_name = monster_desc(player_ptr, &floor_ptr->m_list[floor_ptr->grid_array[y][x].m_idx], 0);
                msg_print(_(format("%sが釣れた！", m_name.data()), "You have a good catch!"));
                success = true;
            }
        }

        if (!success) {
            msg_print(_("餌だけ食われてしまった！くっそ～！", "Damn!  The fish stole your bait!"));
        }

        disturb(player_ptr, false, true);
    }
}

bool continuous_action_running(PlayerType *player_ptr)
{
    return player_ptr->running || travel.run || command_rep || (player_ptr->action == ACTION_REST) || (player_ptr->action == ACTION_FISH);
}

/*!
 * @brief プレイヤーの行動処理 / Process the player
 * @note
 * Notice the annoying code to handle "pack overflow", which\n
 * must come first just in case somebody manages to corrupt\n
 * the savefiles by clever use of menu commands or something.\n
 */
void process_player(PlayerType *player_ptr)
{
    if (player_ptr->hack_mutation) {
        msg_print(_("何か変わった気がする！", "You feel different!"));
        (void)gain_mutation(player_ptr, 0);
        player_ptr->hack_mutation = false;
    }

    if (player_ptr->invoking_midnight_curse) {
        int count = 0;
        activate_ty_curse(player_ptr, false, &count);
        player_ptr->invoking_midnight_curse = false;
    }

    if (player_ptr->phase_out) {
        for (MONSTER_IDX m_idx = 1; m_idx < player_ptr->current_floor_ptr->m_max; m_idx++) {
            auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
            if (!m_ptr->is_valid()) {
                continue;
            }

            m_ptr->mflag2.set({ MonsterConstantFlagType::MARK, MonsterConstantFlagType::SHOW });
            update_monster(player_ptr, m_idx, false);
        }

        WorldTurnProcessor(player_ptr).print_time();
    } else if (!(load && player_ptr->energy_need <= 0)) {
        player_ptr->energy_need -= speed_to_energy(player_ptr->pspeed);
    }

    if (player_ptr->energy_need > 0) {
        return;
    }
    if (!command_rep) {
        WorldTurnProcessor(player_ptr).print_time();
    }

    if (fresh_once && (continuous_action_running(player_ptr) || !command_rep)) {
        stop_term_fresh();
    }

    if (player_ptr->resting < 0) {
        if (player_ptr->resting == COMMAND_ARG_REST_FULL_HEALING) {
            if ((player_ptr->chp == player_ptr->mhp) && (player_ptr->csp >= player_ptr->msp)) {
                set_action(player_ptr, ACTION_NONE);
            }
        } else if (player_ptr->resting == COMMAND_ARG_REST_UNTIL_DONE) {
            if (player_ptr->is_fully_healthy()) {
                set_action(player_ptr, ACTION_NONE);
            }
        }
    }

    if (player_ptr->action == ACTION_FISH) {
        process_fishing(player_ptr);
    }

    if (check_abort) {
        if (continuous_action_running(player_ptr)) {
            inkey_scan = true;
            if (inkey()) {
                flush();
                disturb(player_ptr, false, true);
                msg_print(_("中断しました。", "Canceled."));
            }
        }
    }

    const auto effects = player_ptr->effects();
    if (player_ptr->riding && !effects->confusion()->is_confused() && !effects->blindness()->is_blind()) {
        auto *m_ptr = &player_ptr->current_floor_ptr->m_list[player_ptr->riding];
        auto *r_ptr = &monraces_info[m_ptr->r_idx];
        if (m_ptr->is_asleep()) {
            const auto m_name = monster_desc(player_ptr, m_ptr, 0);
            (void)set_monster_csleep(player_ptr, player_ptr->riding, 0);
            msg_format(_("%s^を起こした。", "You have woken %s up."), m_name.data());
        }

        if (m_ptr->is_stunned()) {
            if (set_monster_stunned(player_ptr, player_ptr->riding,
                    (randint0(r_ptr->level) < player_ptr->skill_exp[PlayerSkillKindType::RIDING]) ? 0 : (m_ptr->get_remaining_stun() - 1))) {
                const auto m_name = monster_desc(player_ptr, m_ptr, 0);
                msg_format(_("%s^を朦朧状態から立ち直らせた。", "%s^ is no longer stunned."), m_name.data());
            }
        }

        if (m_ptr->is_confused()) {
            if (set_monster_confused(player_ptr, player_ptr->riding,
                    (randint0(r_ptr->level) < player_ptr->skill_exp[PlayerSkillKindType::RIDING]) ? 0 : (m_ptr->get_remaining_confusion() - 1))) {
                const auto m_name = monster_desc(player_ptr, m_ptr, 0);
                msg_format(_("%s^を混乱状態から立ち直らせた。", "%s^ is no longer confused."), m_name.data());
            }
        }

        if (m_ptr->is_fearful()) {
            if (set_monster_monfear(player_ptr, player_ptr->riding,
                    (randint0(r_ptr->level) < player_ptr->skill_exp[PlayerSkillKindType::RIDING]) ? 0 : (m_ptr->get_remaining_fear() - 1))) {
                const auto m_name = monster_desc(player_ptr, m_ptr, 0);
                msg_format(_("%s^を恐怖から立ち直らせた。", "%s^ is no longer fearful."), m_name.data());
            }
        }

        handle_stuff(player_ptr);
    }

    load = false;
    if (player_ptr->lightspeed) {
        set_lightspeed(player_ptr, player_ptr->lightspeed - 1, true);
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (PlayerClass(player_ptr).equals(PlayerClassType::FORCETRAINER) && get_current_ki(player_ptr)) {
        if (get_current_ki(player_ptr) < 40) {
            set_current_ki(player_ptr, true, 0);
        } else {
            set_current_ki(player_ptr, false, -40);
        }
        rfu.set_flag(StatusRecalculatingFlag::BONUS);
    }

    if (player_ptr->action == ACTION_LEARN) {
        int32_t cost = 0L;
        uint32_t cost_frac = (player_ptr->msp + 30L) * 256L;
        s64b_lshift(&cost, &cost_frac, 16);
        if (s64b_cmp(player_ptr->csp, player_ptr->csp_frac, cost, cost_frac) < 0) {
            player_ptr->csp = 0;
            player_ptr->csp_frac = 0;
            set_action(player_ptr, ACTION_NONE);
        } else {
            s64b_sub(&(player_ptr->csp), &(player_ptr->csp_frac), cost, cost_frac);
        }

        rfu.set_flag(MainWindowRedrawingFlag::MP);
    }

    if (PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::MUSOU)) {
        if (player_ptr->csp < 3) {
            set_action(player_ptr, ACTION_NONE);
        } else {
            player_ptr->csp -= 2;
            rfu.set_flag(MainWindowRedrawingFlag::MP);
        }
    }

    /*** Handle actual user input ***/
    while (player_ptr->energy_need <= 0) {
        rfu.set_flag(SubWindowRedrawingFlag::PLAYER);
        player_ptr->sutemi = false;
        player_ptr->counter = false;
        player_ptr->now_damaged = false;

        update_monsters(player_ptr, false);
        handle_stuff(player_ptr);
        move_cursor_relative(player_ptr->y, player_ptr->x);
        if (fresh_before) {
            term_fresh_force();
        }

        pack_overflow(player_ptr);
        if (!command_new) {
            command_see = false;
        }

        PlayerEnergy energy(player_ptr);
        energy.reset_player_turn();
        auto is_knocked_out = effects->stun()->is_knocked_out();
        auto is_paralyzed = effects->paralysis()->is_paralyzed();
        if (player_ptr->phase_out) {
            move_cursor_relative(player_ptr->y, player_ptr->x);
            command_cmd = SPECIAL_KEY_BUILDING;
            process_command(player_ptr);
        } else if ((is_paralyzed || is_knocked_out) && !cheat_immortal) {
            energy.set_player_turn_energy(100);
        } else if (player_ptr->action == ACTION_REST) {
            if (player_ptr->resting > 0) {
                player_ptr->resting--;
                if (!player_ptr->resting) {
                    set_action(player_ptr, ACTION_NONE);
                }

                rfu.set_flag(MainWindowRedrawingFlag::ACTION);
            }

            energy.set_player_turn_energy(100);
        } else if (player_ptr->action == ACTION_FISH) {
            energy.set_player_turn_energy(100);
        } else if (player_ptr->running) {
            run_step(player_ptr, 0);
        } else if (travel.run) {
            travel_step(player_ptr);
        } else if (command_rep) {
            command_rep--;
            rfu.set_flag(MainWindowRedrawingFlag::ACTION);
            handle_stuff(player_ptr);
            msg_flag = false;
            prt("", 0, 0);
            process_command(player_ptr);
        } else {
            move_cursor_relative(player_ptr->y, player_ptr->x);

            static constexpr auto flags = {
                SubWindowRedrawingFlag::SIGHT_MONSTERS,
                SubWindowRedrawingFlag::PETS,
            };
            rfu.set_flags(flags);
            window_stuff(player_ptr);

            can_save = true;
            InputKeyRequestor(player_ptr, false).request_command();
            can_save = false;
            process_command(player_ptr);
        }

        pack_overflow(player_ptr);
        if (player_ptr->energy_use) {
            if (player_ptr->timewalk || player_ptr->energy_use > 400) {
                player_ptr->energy_need += player_ptr->energy_use * TURNS_PER_TICK / 10;
            } else {
                player_ptr->energy_need += (int16_t)((int32_t)player_ptr->energy_use * ENERGY_NEED() / 100L);
            }

            if (effects->hallucination()->is_hallucinated()) {
                rfu.set_flag(MainWindowRedrawingFlag::MAP);
            }

            for (MONSTER_IDX m_idx = 1; m_idx < player_ptr->current_floor_ptr->m_max; m_idx++) {
                MonsterEntity *m_ptr;
                MonsterRaceInfo *r_ptr;
                m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
                if (!m_ptr->is_valid()) {
                    continue;
                }

                r_ptr = &monraces_info[m_ptr->ap_r_idx];

                // モンスターのシンボル/カラーの更新
                if (m_ptr->ml && r_ptr->visual_flags.has_any_of({ MonsterVisualType::MULTI_COLOR, MonsterVisualType::SHAPECHANGER })) {
                    lite_spot(player_ptr, m_ptr->fy, m_ptr->fx);
                }

                // 出現して即魔法を使わないようにするフラグを落とす処理
                if (m_ptr->mflag.has(MonsterTemporaryFlagType::PREVENT_MAGIC)) {
                    m_ptr->mflag.reset(MonsterTemporaryFlagType::PREVENT_MAGIC);
                }

                if (m_ptr->mflag.has(MonsterTemporaryFlagType::SANITY_BLAST)) {
                    m_ptr->mflag.reset(MonsterTemporaryFlagType::SANITY_BLAST);
                    sanity_blast(player_ptr, m_ptr, false);
                }

                // 感知中のモンスターのフラグを落とす処理
                // 感知したターンはMFLAG2_SHOWを落とし、次のターンに感知中フラグのMFLAG2_MARKを落とす
                if (m_ptr->mflag2.has(MonsterConstantFlagType::MARK)) {
                    if (m_ptr->mflag2.has(MonsterConstantFlagType::SHOW)) {
                        m_ptr->mflag2.reset(MonsterConstantFlagType::SHOW);
                    } else {
                        m_ptr->mflag2.reset(MonsterConstantFlagType::MARK);
                        m_ptr->ml = false;
                        update_monster(player_ptr, m_idx, false);
                        if (player_ptr->health_who == m_idx) {
                            rfu.set_flag(MainWindowRedrawingFlag::HEALTH);
                        }

                        if (player_ptr->riding == m_idx) {
                            rfu.set_flag(MainWindowRedrawingFlag::UHEALTH);
                        }

                        lite_spot(player_ptr, m_ptr->fy, m_ptr->fx);
                    }
                }
            }

            if (PlayerClass(player_ptr).equals(PlayerClassType::IMITATOR)) {
                auto mane_data = PlayerClass(player_ptr).get_specific_data<mane_data_type>();
                if (static_cast<int>(mane_data->mane_list.size()) > (player_ptr->lev > 44 ? 3 : player_ptr->lev > 29 ? 2
                                                                                                                     : 1)) {
                    mane_data->mane_list.pop_front();
                }

                mane_data->new_mane = false;
                rfu.set_flag(MainWindowRedrawingFlag::IMITATION);
            }

            if (player_ptr->action == ACTION_LEARN) {
                auto mane_data = PlayerClass(player_ptr).get_specific_data<bluemage_data_type>();
                mane_data->new_magic_learned = false;
                rfu.set_flag(MainWindowRedrawingFlag::ACTION);
            }

            if (player_ptr->timewalk && (player_ptr->energy_need > -1000)) {
                rfu.set_flag(MainWindowRedrawingFlag::MAP);
                rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
                static constexpr auto flags_swrf = {
                    SubWindowRedrawingFlag::OVERHEAD,
                    SubWindowRedrawingFlag::DUNGEON,
                };
                rfu.set_flags(flags_swrf);
                msg_print(_("「時は動きだす…」", "You feel time flowing around you once more."));
                msg_print(nullptr);
                player_ptr->timewalk = false;
                player_ptr->energy_need = ENERGY_NEED();

                handle_stuff(player_ptr);
            }
        }

        if (!player_ptr->playing || player_ptr->is_dead) {
            player_ptr->timewalk = false;
            break;
        }

        auto sniper_data = PlayerClass(player_ptr).get_specific_data<SniperData>();
        if (player_ptr->energy_use && sniper_data && sniper_data->reset_concent) {
            reset_concentration(player_ptr, true);
        }

        if (player_ptr->leaving) {
            break;
        }
    }

    update_smell(player_ptr->current_floor_ptr, player_ptr);
}

/*!
 * @brief プレイヤーの行動エネルギーが充填される（＝プレイヤーのターンが回る）毎に行われる処理  / process the effects per 100 energy at player speed.
 */
void process_upkeep_with_speed(PlayerType *player_ptr)
{
    if (!load && player_ptr->enchant_energy_need > 0 && !player_ptr->leaving) {
        player_ptr->enchant_energy_need -= speed_to_energy(player_ptr->pspeed);
    }

    if (player_ptr->enchant_energy_need > 0) {
        return;
    }

    while (player_ptr->enchant_energy_need <= 0) {
        if (!load) {
            check_music(player_ptr);
        }

        SpellHex spell_hex(player_ptr);
        if (!load) {
            spell_hex.decrease_mana();
        }

        if (!load) {
            spell_hex.continue_revenge();
        }

        player_ptr->enchant_energy_need += ENERGY_NEED();
    }
}
