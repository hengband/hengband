#include "core/player-processor.h"
#include "action/run-execution.h"
#include "action/travel-execution.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/special-internal-keys.h"
#include "core/speed-table.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon.h"
#include "floor/floor-save-util.h"
#include "floor/floor-util.h"
#include "floor/geometry.h"
#include "floor/wild.h"
#include "game-option/disturbance-options.h"
#include "game-option/map-screen-options.h"
#include "game-option/cheat-options.h"
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
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/eldritch-horror.h"
#include "player/player-skill.h"
#include "player/special-defense-types.h"
#include "spell-kind/spells-random.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "status/action-setter.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "window/display-sub-windows.h"
#include "world/world-turn-processor.h"

bool load = true;
bool can_save = false;

static void process_fishing(player_type *creature_ptr)
{
    term_xtra(TERM_XTRA_DELAY, 10);
    if (one_in_(1000)) {
        MONRACE_IDX r_idx;
        bool success = false;
        get_mon_num_prep(creature_ptr, monster_is_fishing_target, nullptr);
        r_idx = get_mon_num(creature_ptr, 0,
            is_in_dungeon(creature_ptr) ? creature_ptr->current_floor_ptr->dun_level
                                                       : wilderness[creature_ptr->wilderness_y][creature_ptr->wilderness_x].level,
            0);
        msg_print(nullptr);
        if (r_idx && one_in_(2)) {
            POSITION y, x;
            y = creature_ptr->y + ddy[creature_ptr->fishing_dir];
            x = creature_ptr->x + ddx[creature_ptr->fishing_dir];
            if (place_monster_aux(creature_ptr, 0, y, x, r_idx, PM_NO_KAGE)) {
                GAME_TEXT m_name[MAX_NLEN];
                monster_desc(creature_ptr, m_name, &creature_ptr->current_floor_ptr->m_list[creature_ptr->current_floor_ptr->grid_array[y][x].m_idx], 0);
                msg_format(_("%sが釣れた！", "You have a good catch!"), m_name);
                success = true;
            }
        }

        if (!success) {
            msg_print(_("餌だけ食われてしまった！くっそ～！", "Damn!  The fish stole your bait!"));
        }

        disturb(creature_ptr, false, true);
    }
}

bool continuous_action_running(player_type *creature_ptr)
{
    return creature_ptr->running || travel.run || command_rep || (creature_ptr->action == ACTION_REST) || (creature_ptr->action == ACTION_FISH);
}

/*!
 * @brief プレイヤーの行動処理 / Process the player
 * @note
 * Notice the annoying code to handle "pack overflow", which\n
 * must come first just in case somebody manages to corrupt\n
 * the savefiles by clever use of menu commands or something.\n
 */
void process_player(player_type *creature_ptr)
{
    if (creature_ptr->hack_mutation) {
        msg_print(_("何か変わった気がする！", "You feel different!"));
        (void)gain_mutation(creature_ptr, 0);
        creature_ptr->hack_mutation = false;
    }

    if (creature_ptr->invoking_midnight_curse) {
        int count = 0;
        activate_ty_curse(creature_ptr, false, &count);
        creature_ptr->invoking_midnight_curse = false;
    }

    if (creature_ptr->phase_out) {
        for (MONSTER_IDX m_idx = 1; m_idx < creature_ptr->current_floor_ptr->m_max; m_idx++) {
            monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[m_idx];
            if (!monster_is_valid(m_ptr))
                continue;

            m_ptr->mflag2.set({MFLAG2::MARK, MFLAG2::SHOW});
            update_monster(creature_ptr, m_idx, false);
        }

        WorldTurnProcessor(creature_ptr).print_time();        
    } else if (!(load && creature_ptr->energy_need <= 0)) {
        creature_ptr->energy_need -= SPEED_TO_ENERGY(creature_ptr->pspeed);
    }

    if (creature_ptr->energy_need > 0)
        return;
    if (!command_rep) {
        WorldTurnProcessor(creature_ptr).print_time();
    }

    if (fresh_once && (continuous_action_running(creature_ptr) || !command_rep)) {
        stop_term_fresh();
    }

    if (creature_ptr->resting < 0) {
        if (creature_ptr->resting == COMMAND_ARG_REST_FULL_HEALING) {
            if ((creature_ptr->chp == creature_ptr->mhp) && (creature_ptr->csp >= creature_ptr->msp)) {
                set_action(creature_ptr, ACTION_NONE);
            }
        } else if (creature_ptr->resting == COMMAND_ARG_REST_UNTIL_DONE) {
            if ((creature_ptr->chp == creature_ptr->mhp) && (creature_ptr->csp >= creature_ptr->msp) && !creature_ptr->blind && !creature_ptr->confused
                && !creature_ptr->poisoned && !creature_ptr->afraid && !creature_ptr->stun && !creature_ptr->cut && !creature_ptr->slow
                && !creature_ptr->paralyzed && !creature_ptr->image && !creature_ptr->word_recall && !creature_ptr->alter_reality) {
                set_action(creature_ptr, ACTION_NONE);
            }
        }
    }

    if (creature_ptr->action == ACTION_FISH)
        process_fishing(creature_ptr);

    if (check_abort) {
        if (continuous_action_running(creature_ptr)) {
            inkey_scan = true;
            if (inkey()) {
                flush();
                disturb(creature_ptr, false, true);
                msg_print(_("中断しました。", "Canceled."));
            }
        }
    }

    if (creature_ptr->riding && !creature_ptr->confused && !creature_ptr->blind) {
        monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];
        monster_race *r_ptr = &r_info[m_ptr->r_idx];
        if (monster_csleep_remaining(m_ptr)) {
            GAME_TEXT m_name[MAX_NLEN];
            (void)set_monster_csleep(creature_ptr, creature_ptr->riding, 0);
            monster_desc(creature_ptr, m_name, m_ptr, 0);
            msg_format(_("%^sを起こした。", "You have woken %s up."), m_name);
        }

        if (monster_stunned_remaining(m_ptr)) {
            if (set_monster_stunned(creature_ptr, creature_ptr->riding,
                    (randint0(r_ptr->level) < creature_ptr->skill_exp[SKILL_RIDING]) ? 0 : (monster_stunned_remaining(m_ptr) - 1))) {
                GAME_TEXT m_name[MAX_NLEN];
                monster_desc(creature_ptr, m_name, m_ptr, 0);
                msg_format(_("%^sを朦朧状態から立ち直らせた。", "%^s is no longer stunned."), m_name);
            }
        }

        if (monster_confused_remaining(m_ptr)) {
            if (set_monster_confused(creature_ptr, creature_ptr->riding,
                    (randint0(r_ptr->level) < creature_ptr->skill_exp[SKILL_RIDING]) ? 0 : (monster_confused_remaining(m_ptr) - 1))) {
                GAME_TEXT m_name[MAX_NLEN];
                monster_desc(creature_ptr, m_name, m_ptr, 0);
                msg_format(_("%^sを混乱状態から立ち直らせた。", "%^s is no longer confused."), m_name);
            }
        }

        if (monster_fear_remaining(m_ptr)) {
            if (set_monster_monfear(creature_ptr, creature_ptr->riding,
                    (randint0(r_ptr->level) < creature_ptr->skill_exp[SKILL_RIDING]) ? 0 : (monster_fear_remaining(m_ptr) - 1))) {
                GAME_TEXT m_name[MAX_NLEN];
                monster_desc(creature_ptr, m_name, m_ptr, 0);
                msg_format(_("%^sを恐怖から立ち直らせた。", "%^s is no longer afraid."), m_name);
            }
        }

        handle_stuff(creature_ptr);
    }

    load = false;
    if (creature_ptr->lightspeed)
        set_lightspeed(creature_ptr, creature_ptr->lightspeed - 1, true);

    if ((creature_ptr->pclass == CLASS_FORCETRAINER) && get_current_ki(creature_ptr)) {
        if (get_current_ki(creature_ptr) < 40)
            set_current_ki(creature_ptr, true, 0);
        else
            set_current_ki(creature_ptr, false, -40);
        creature_ptr->update |= (PU_BONUS);
    }

    if (creature_ptr->action == ACTION_LEARN) {
        int32_t cost = 0L;
        uint32_t cost_frac = (creature_ptr->msp + 30L) * 256L;
        s64b_lshift(&cost, &cost_frac, 16);
        if (s64b_cmp(creature_ptr->csp, creature_ptr->csp_frac, cost, cost_frac) < 0) {
            creature_ptr->csp = 0;
            creature_ptr->csp_frac = 0;
            set_action(creature_ptr, ACTION_NONE);
        } else {
            s64b_sub(&(creature_ptr->csp), &(creature_ptr->csp_frac), cost, cost_frac);
        }

        creature_ptr->redraw |= PR_MANA;
    }

    if (creature_ptr->special_defense & KATA_MASK) {
        if (creature_ptr->special_defense & KATA_MUSOU) {
            if (creature_ptr->csp < 3) {
                set_action(creature_ptr, ACTION_NONE);
            } else {
                creature_ptr->csp -= 2;
                creature_ptr->redraw |= (PR_MANA);
            }
        }
    }

    /*** Handle actual user input ***/
    while (creature_ptr->energy_need <= 0) {
        creature_ptr->window_flags |= PW_PLAYER;
        creature_ptr->sutemi = false;
        creature_ptr->counter = false;
        creature_ptr->now_damaged = false;

        update_monsters(creature_ptr, false);
        handle_stuff(creature_ptr);
        move_cursor_relative(creature_ptr->y, creature_ptr->x);
        if (fresh_before)
            term_fresh_force();

        pack_overflow(creature_ptr);
        if (!command_new)
            command_see = false;

        PlayerEnergy energy(creature_ptr);
        energy.reset_player_turn();
        if (creature_ptr->phase_out) {
            move_cursor_relative(creature_ptr->y, creature_ptr->x);
            command_cmd = SPECIAL_KEY_BUILDING;
            process_command(creature_ptr);
        } else if ((creature_ptr->paralyzed || creature_ptr->stun >= 100) && !cheat_immortal) {
            energy.set_player_turn_energy(100);
        } else if (creature_ptr->action == ACTION_REST) {
            if (creature_ptr->resting > 0) {
                creature_ptr->resting--;
                if (!creature_ptr->resting)
                    set_action(creature_ptr, ACTION_NONE);
                creature_ptr->redraw |= (PR_STATE);
            }

            energy.set_player_turn_energy(100);
        } else if (creature_ptr->action == ACTION_FISH) {
            energy.set_player_turn_energy(100);
        } else if (creature_ptr->running) {
            run_step(creature_ptr, 0);
        } else if (travel.run) {
            travel_step(creature_ptr);
        } else if (command_rep) {
            command_rep--;
            creature_ptr->redraw |= (PR_STATE);
            handle_stuff(creature_ptr);
            msg_flag = false;
            prt("", 0, 0);
            process_command(creature_ptr);
        } else {
            move_cursor_relative(creature_ptr->y, creature_ptr->x);

            creature_ptr->window_flags |= PW_MONSTER_LIST;
            window_stuff(creature_ptr);

            can_save = true;
            request_command(creature_ptr, false);
            can_save = false;
            process_command(creature_ptr);
        }

        pack_overflow(creature_ptr);
        if (creature_ptr->energy_use) {
            if (creature_ptr->timewalk || creature_ptr->energy_use > 400) {
                creature_ptr->energy_need += creature_ptr->energy_use * TURNS_PER_TICK / 10;
            } else {
                creature_ptr->energy_need += (int16_t)((int32_t)creature_ptr->energy_use * ENERGY_NEED() / 100L);
            }

            if (creature_ptr->image)
                creature_ptr->redraw |= (PR_MAP);

            for (MONSTER_IDX m_idx = 1; m_idx < creature_ptr->current_floor_ptr->m_max; m_idx++) {
                monster_type *m_ptr;
                monster_race *r_ptr;
                m_ptr = &creature_ptr->current_floor_ptr->m_list[m_idx];
                if (!monster_is_valid(m_ptr))
                    continue;

                r_ptr = &r_info[m_ptr->ap_r_idx];

                // モンスターのシンボル/カラーの更新
                if (m_ptr->ml && any_bits(r_ptr->flags1, (RF1_ATTR_MULTI | RF1_SHAPECHANGER))) {
                    lite_spot(creature_ptr, m_ptr->fy, m_ptr->fx);
                }

                // 出現して即魔法を使わないようにするフラグを落とす処理
                if (m_ptr->mflag.has(MFLAG::PREVENT_MAGIC)) {
                    m_ptr->mflag.reset(MFLAG::PREVENT_MAGIC);
                }

                if (m_ptr->mflag.has(MFLAG::SANITY_BLAST)) {
                    m_ptr->mflag.reset(MFLAG::SANITY_BLAST);
                    sanity_blast(creature_ptr, m_ptr, false);
                }

                // 感知中のモンスターのフラグを落とす処理
                // 感知したターンはMFLAG2_SHOWを落とし、次のターンに感知中フラグのMFLAG2_MARKを落とす
                if (m_ptr->mflag2.has(MFLAG2::MARK)) {
                    if (m_ptr->mflag2.has(MFLAG2::SHOW)) {
                        m_ptr->mflag2.reset(MFLAG2::SHOW);
                    } else {
                        m_ptr->mflag2.reset(MFLAG2::MARK);
                        m_ptr->ml = false;
                        update_monster(creature_ptr, m_idx, false);
                        if (creature_ptr->health_who == m_idx)
                            creature_ptr->redraw |= (PR_HEALTH);
                        if (creature_ptr->riding == m_idx)
                            creature_ptr->redraw |= (PR_UHEALTH);

                        lite_spot(creature_ptr, m_ptr->fy, m_ptr->fx);
                    }
                }
            }

            if (creature_ptr->pclass == CLASS_IMITATOR) {
                if (creature_ptr->mane_num > (creature_ptr->lev > 44 ? 3 : creature_ptr->lev > 29 ? 2 : 1)) {
                    creature_ptr->mane_num--;
                    for (int j = 0; j < creature_ptr->mane_num; j++) {
                        creature_ptr->mane_spell[j] = creature_ptr->mane_spell[j + 1];
                        creature_ptr->mane_dam[j] = creature_ptr->mane_dam[j + 1];
                    }
                }

                creature_ptr->new_mane = false;
                creature_ptr->redraw |= (PR_IMITATION);
            }

            if (creature_ptr->action == ACTION_LEARN) {
                creature_ptr->new_mane = false;
                creature_ptr->redraw |= (PR_STATE);
            }

            if (creature_ptr->timewalk && (creature_ptr->energy_need > -1000)) {
                creature_ptr->redraw |= (PR_MAP);
                creature_ptr->update |= (PU_MONSTERS);
                creature_ptr->window_flags |= (PW_OVERHEAD | PW_DUNGEON);

                msg_print(_("「時は動きだす…」", "You feel time flowing around you once more."));
                msg_print(nullptr);
                creature_ptr->timewalk = false;
                creature_ptr->energy_need = ENERGY_NEED();

                handle_stuff(creature_ptr);
            }
        }

        if (!creature_ptr->playing || creature_ptr->is_dead) {
            creature_ptr->timewalk = false;
            break;
        }

        if (creature_ptr->energy_use && creature_ptr->reset_concent)
            reset_concentration(creature_ptr, true);

        if (creature_ptr->leaving)
            break;
    }

    update_smell(creature_ptr->current_floor_ptr, creature_ptr);
}

/*!
 * @brief プレイヤーの行動エネルギーが充填される（＝プレイヤーのターンが回る）毎に行われる処理  / process the effects per 100 energy at player speed.
 */
void process_upkeep_with_speed(player_type *creature_ptr)
{
    if (!load && creature_ptr->enchant_energy_need > 0 && !creature_ptr->leaving) {
        creature_ptr->enchant_energy_need -= SPEED_TO_ENERGY(creature_ptr->pspeed);
    }

    if (creature_ptr->enchant_energy_need > 0)
        return;

    while (creature_ptr->enchant_energy_need <= 0) {
        if (!load)
            check_music(creature_ptr);
        if (!load)
            check_hex(creature_ptr);
        if (!load)
            revenge_spell(creature_ptr);

        creature_ptr->enchant_energy_need += ENERGY_NEED();
    }
}
