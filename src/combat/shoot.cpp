#include "combat/shoot.h"
#include "artifact/fixed-art-types.h"
#include "avatar/avatar.h"
#include "combat/attack-criticality.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "effect/spells-effect-util.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/geometry.h"
#include "game-option/cheat-types.h"
#include "game-option/special-options.h"
#include "grid/feature-flag-types.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/cursor.h"
#include "io/screen-util.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-sniper.h"
#include "mind/snipe-types.h"
#include "monster-floor/monster-death.h"
#include "monster-floor/monster-move.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-damage.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-enchant.h"
#include "object/object-broken.h"
#include "object/object-flags.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "object/object-mark-types.h"
#include "player-status/player-energy.h"
#include "player/player-class.h"
#include "player/player-personality-types.h"
#include "player/player-skill.h"
#include "player/player-status-table.h"
#include "spell/spell-types.h"
#include "sv-definition/sv-bow-types.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "wizard/wizard-messages.h"
#include "world/world-object.h"

/*!
 * @brief 矢弾を射撃した際のスレイ倍率をかけた結果を返す /
 * Determines the odds of an object breaking when thrown at a monster
 * @param bow_ptr 弓のオブジェクト構造体参照ポインタ
 * @param arrow_ptr 矢弾のオブジェクト構造体参照ポインタ
 * @param tdam 計算途中のダメージ量
 * @param monster_ptr 目標モンスターの構造体参照ポインタ
 * @return スレイ倍率をかけたダメージ量
 */
static MULTIPLY calc_shot_damage_with_slay(
    player_type *sniper_ptr, object_type *bow_ptr, object_type *arrow_ptr, HIT_POINT tdam, monster_type *monster_ptr, SPELL_IDX snipe_type)
{
    MULTIPLY mult = 10;

    monster_race *race_ptr = &r_info[monster_ptr->r_idx];

    TrFlags flags, bow_flags, arrow_flags;
    object_flags(sniper_ptr, arrow_ptr, arrow_flags);
    object_flags(sniper_ptr, bow_ptr, bow_flags);

    for (int i = 0; i < TR_FLAG_SIZE; i++)
        flags[i] = bow_flags[i] | arrow_flags[i];

    /* Some "weapons" and "ammo" do extra damage */
    switch (arrow_ptr->tval) {
    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT: {
        if ((has_flag(flags, TR_SLAY_ANIMAL)) && any_bits(race_ptr->flags3, RF3_ANIMAL)) {
            if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                set_bits(race_ptr->r_flags3, RF3_ANIMAL);
            }
            if (mult < 17)
                mult = 17;
        }

        if ((has_flag(flags, TR_KILL_ANIMAL)) && any_bits(race_ptr->flags3, RF3_ANIMAL)) {
            if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                set_bits(race_ptr->r_flags3, RF3_ANIMAL);
            }
            if (mult < 27)
                mult = 27;
        }

        if ((has_flag(flags, TR_SLAY_EVIL)) && any_bits(race_ptr->flags3, RF3_EVIL)) {
            if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                set_bits(race_ptr->r_flags3, RF3_EVIL);
            }
            if (mult < 15)
                mult = 15;
        }

        if ((has_flag(flags, TR_KILL_EVIL)) && any_bits(race_ptr->flags3, RF3_EVIL)) {
            if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                set_bits(race_ptr->r_flags3, RF3_EVIL);
            }
            if (mult < 25)
                mult = 25;
        }

        if ((has_flag(flags, TR_SLAY_GOOD)) && any_bits(race_ptr->flags3, RF3_GOOD)) {
            if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                set_bits(race_ptr->r_flags3, RF3_GOOD);
            }
            if (mult < 15)
                mult = 15;
        }

        if ((has_flag(flags, TR_KILL_GOOD)) && any_bits(race_ptr->flags3, RF3_GOOD)) {
            if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                set_bits(race_ptr->r_flags3, RF3_GOOD);
            }
            if (mult < 25)
                mult = 25;
        }

        if ((has_flag(flags, TR_SLAY_HUMAN)) && any_bits(race_ptr->flags2, RF2_HUMAN)) {
            if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                set_bits(race_ptr->r_flags2, RF2_HUMAN);
            }
            if (mult < 17)
                mult = 17;
        }

        if ((has_flag(flags, TR_KILL_HUMAN)) && any_bits(race_ptr->flags2, RF2_HUMAN)) {
            if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                set_bits(race_ptr->r_flags2, RF2_HUMAN);
            }
            if (mult < 27)
                mult = 27;
        }

        if ((has_flag(flags, TR_SLAY_UNDEAD)) && any_bits(race_ptr->flags3, RF3_UNDEAD)) {
            if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                set_bits(race_ptr->r_flags3, RF3_UNDEAD);
            }
            if (mult < 20)
                mult = 20;
        }

        if ((has_flag(flags, TR_KILL_UNDEAD)) && any_bits(race_ptr->flags3, RF3_UNDEAD)) {
            if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                set_bits(race_ptr->r_flags3, RF3_UNDEAD);
            }
            if (mult < 30)
                mult = 30;
        }

        if ((has_flag(flags, TR_SLAY_DEMON)) && any_bits(race_ptr->flags3, RF3_DEMON)) {
            if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                set_bits(race_ptr->r_flags3, RF3_DEMON);
            }
            if (mult < 20)
                mult = 20;
        }

        if ((has_flag(flags, TR_KILL_DEMON)) && any_bits(race_ptr->flags3, RF3_DEMON)) {
            if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                set_bits(race_ptr->r_flags3, RF3_DEMON);
            }
            if (mult < 30)
                mult = 30;
        }

        if ((has_flag(flags, TR_SLAY_ORC)) && any_bits(race_ptr->flags3, RF3_ORC)) {
            if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                set_bits(race_ptr->r_flags3, RF3_ORC);
            }
            if (mult < 20)
                mult = 20;
        }

        if ((has_flag(flags, TR_KILL_ORC)) && any_bits(race_ptr->flags3, RF3_ORC)) {
            if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                set_bits(race_ptr->r_flags3, RF3_ORC);
            }
            if (mult < 30)
                mult = 30;
        }

        if ((has_flag(flags, TR_SLAY_TROLL)) && any_bits(race_ptr->flags3, RF3_TROLL)) {
            if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                set_bits(race_ptr->r_flags3, RF3_TROLL);
            }

            if (mult < 20)
                mult = 20;
        }

        if ((has_flag(flags, TR_KILL_TROLL)) && any_bits(race_ptr->flags3, RF3_TROLL)) {
            if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                set_bits(race_ptr->r_flags3, RF3_TROLL);
            }
            if (mult < 30)
                mult = 30;
        }

        if ((has_flag(flags, TR_SLAY_GIANT)) && any_bits(race_ptr->flags3, RF3_GIANT)) {
            if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                set_bits(race_ptr->r_flags3, RF3_GIANT);
            }
            if (mult < 20)
                mult = 20;
        }

        if ((has_flag(flags, TR_KILL_GIANT)) && any_bits(race_ptr->flags3, RF3_GIANT)) {
            if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                set_bits(race_ptr->r_flags3, RF3_GIANT);
            }
            if (mult < 30)
                mult = 30;
        }

        if ((has_flag(flags, TR_SLAY_DRAGON)) && any_bits(race_ptr->flags3, RF3_DRAGON)) {
            if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                set_bits(race_ptr->r_flags3, RF3_DRAGON);
            }
            if (mult < 20)
                mult = 20;
        }

        if ((has_flag(flags, TR_KILL_DRAGON)) && any_bits(race_ptr->flags3, RF3_DRAGON)) {
            if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                set_bits(race_ptr->r_flags3, RF3_DRAGON);
            }
            if (mult < 30)
                mult = 30;
            if ((arrow_ptr->name1 == ART_BARD_ARROW) && (monster_ptr->r_idx == MON_SMAUG) && (sniper_ptr->inventory_list[INVEN_BOW].name1 == ART_BARD))
                mult *= 5;
        }

        if (has_flag(flags, TR_BRAND_ACID)) {
            /* Notice immunity */
            if (any_bits(race_ptr->flagsr, RFR_EFF_IM_ACID_MASK)) {
                if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                    set_bits(race_ptr->r_flagsr, (race_ptr->flagsr & RFR_EFF_IM_ACID_MASK));
                }
            } else {
                if (mult < 17)
                    mult = 17;
            }
        }

        if (has_flag(flags, TR_BRAND_ELEC)) {
            /* Notice immunity */
            if (any_bits(race_ptr->flagsr, RFR_EFF_IM_ELEC_MASK)) {
                if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                    set_bits(race_ptr->r_flagsr, (race_ptr->flagsr & RFR_EFF_IM_ELEC_MASK));
                }
            } else {
                if (mult < 17)
                    mult = 17;
            }
        }

        if (has_flag(flags, TR_BRAND_FIRE)) {
            /* Notice immunity */
            if (any_bits(race_ptr->flagsr, RFR_EFF_IM_FIRE_MASK)) {
                if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                    set_bits(race_ptr->r_flagsr, (race_ptr->flagsr & RFR_EFF_IM_FIRE_MASK));
                }
            }
            /* Otherwise, take the damage */
            else {
                if (any_bits(race_ptr->flags3, RF3_HURT_FIRE)) {
                    if (mult < 25)
                        mult = 25;
                    if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                        set_bits(race_ptr->r_flags3, RF3_HURT_FIRE);
                    }
                } else if (mult < 17)
                    mult = 17;
            }
        }

        if (has_flag(flags, TR_BRAND_COLD)) {
            /* Notice immunity */
            if (any_bits(race_ptr->flagsr, RFR_EFF_IM_COLD_MASK)) {
                if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                    set_bits(race_ptr->r_flagsr, (race_ptr->flagsr & RFR_EFF_IM_COLD_MASK));
                }
            }
            /* Otherwise, take the damage */
            else {
                if (any_bits(race_ptr->flags3, RF3_HURT_COLD)) {
                    if (mult < 25)
                        mult = 25;
                    if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                        set_bits(race_ptr->r_flags3, RF3_HURT_COLD);
                    }
                } else if (mult < 17)
                    mult = 17;
            }
        }

        if (has_flag(flags, TR_BRAND_POIS)) {
            /* Notice immunity */
            if (any_bits(race_ptr->flagsr, RFR_EFF_IM_POIS_MASK)) {
                if (is_original_ap_and_seen(sniper_ptr, monster_ptr)) {
                    set_bits(race_ptr->r_flagsr, race_ptr->flagsr & RFR_EFF_IM_POIS_MASK);
                }
            }
            /* Otherwise, take the damage */
            else {
                if (mult < 17)
                    mult = 17;
            }
        }

        if ((has_flag(flags, TR_FORCE_WEAPON)) && (sniper_ptr->csp > (sniper_ptr->msp / 30))) {
            sniper_ptr->csp -= (1 + (sniper_ptr->msp / 30));
            set_bits(sniper_ptr->redraw, PR_MANA);
            mult = mult * 5 / 2;
        }
        break;
    }

    default:
        break;
    }

    /* Sniper */
    if (snipe_type)
        mult = calc_snipe_damage_with_slay(sniper_ptr, mult, monster_ptr, snipe_type);

    /* Return the total damage */
    return (tdam * mult / 10);
}

/*!
 * @brief 射撃処理実行 /
 * Fire an object from the pack or floor.
 * @param item 射撃するオブジェクトの所持ID
 * @param bow_ptr 射撃武器のオブジェクト参照ポインタ
 * @details
 * <pre>
 * You may only fire items that "match" your missile launcher.
 * You must use slings + pebbles/shots, bows + arrows, xbows + bolts.
 * See "calc_bonuses()" for more calculations and such.
 * Note that "firing" a missile is MUCH better than "throwing" it.
 * Note: "unseen" monsters are very hard to hit.
 * Objects are more likely to break if they "attempt" to hit a monster.
 * Rangers (with Bows) and Anyone (with "Extra Shots") get extra shots.
 * The "extra shot" code works by decreasing the amount of energy
 * required to make each shot, spreading the shots out over time.
 * Note that when firing missiles, the launcher multiplier is applied
 * after all the bonuses are added in, making multipliers very useful.
 * Note that Bows of "Extra Might" get extra range and an extra bonus
 * for the damage multiplier.
 * Note that Bows of "Extra Shots" give an extra shot.
 * </pre>
 */
void exe_fire(player_type *shooter_ptr, INVENTORY_IDX item, object_type *j_ptr, SPELL_IDX snipe_type)
{
    DIRECTION dir;
    int i;
    POSITION y, x, ny, nx, ty, tx, prev_y, prev_x;
    int tdam_base, tdis, thits, tmul;
    int bonus, chance;
    int cur_dis, visible;
    PERCENTAGE j;

    object_type forge;
    object_type *q_ptr;

    object_type *o_ptr;

    bool hit_body = false;

    GAME_TEXT o_name[MAX_NLEN];

    uint16_t path_g[512]; /* For calcuration of path length */

    int msec = delay_factor * delay_factor * delay_factor;

    /* STICK TO */
    bool stick_to = false;

    /* Access the item (if in the pack) */
    if (item >= 0) {
        o_ptr = &shooter_ptr->inventory_list[item];
    } else {
        o_ptr = &shooter_ptr->current_floor_ptr->o_list[0 - item];
    }

    /* Sniper - Cannot shot a single arrow twice */
    if ((snipe_type == SP_DOUBLE) && (o_ptr->number < 2))
        snipe_type = SP_NONE;

    describe_flavor(shooter_ptr, o_name, o_ptr, OD_OMIT_PREFIX);

    /* Use the proper number of shots */
    thits = shooter_ptr->num_fire;

    /* Use a base distance */
    tdis = 10;

    /* Base damage from thrown object plus launcher bonus */
    tdam_base = damroll(o_ptr->dd, o_ptr->ds) + o_ptr->to_d + j_ptr->to_d;

    /* Actually "fire" the object */
    bonus = (shooter_ptr->to_h_b + o_ptr->to_h + j_ptr->to_h);
    if ((j_ptr->sval == SV_LIGHT_XBOW) || (j_ptr->sval == SV_HEAVY_XBOW))
        chance = (shooter_ptr->skill_thb + (shooter_ptr->weapon_exp[0][j_ptr->sval] / 400 + bonus) * BTH_PLUS_ADJ);
    else
        chance = (shooter_ptr->skill_thb + ((shooter_ptr->weapon_exp[0][j_ptr->sval] - (WEAPON_EXP_MASTER / 2)) / 200 + bonus) * BTH_PLUS_ADJ);

    PlayerEnergy(shooter_ptr).set_player_turn_energy(bow_energy(j_ptr->sval));
    tmul = bow_tmul(j_ptr->sval);

    /* Get extra "power" from "extra might" */
    if (shooter_ptr->xtra_might)
        tmul++;

    tmul = tmul * (100 + (int)(adj_str_td[shooter_ptr->stat_index[A_STR]]) - 128);

    /* Boost the damage */
    tdam_base *= tmul;
    tdam_base /= 100;

    /* Base range */
    tdis = 13 + tmul / 80;
    if ((j_ptr->sval == SV_LIGHT_XBOW) || (j_ptr->sval == SV_HEAVY_XBOW)) {
        if (shooter_ptr->concent)
            tdis -= (5 - (shooter_ptr->concent + 1) / 2);
        else
            tdis -= 5;
    }

    project_length = tdis + 1;

    /* Get a direction (or cancel) */
    if (!get_aim_dir(shooter_ptr, &dir)) {
        PlayerEnergy(shooter_ptr).reset_player_turn();

        if (snipe_type == SP_AWAY)
            snipe_type = SP_NONE;

        /* need not to reset project_length (already did)*/

        return;
    }

    if (snipe_type != SP_NONE)
        sound(SOUND_ZAP);

    /* Predict the "target" location */
    tx = shooter_ptr->x + 99 * ddx[dir];
    ty = shooter_ptr->y + 99 * ddy[dir];

    /* Check for "target request" */
    if ((dir == 5) && target_okay(shooter_ptr)) {
        tx = target_col;
        ty = target_row;
    }

    /* Get projection path length */
    tdis = projection_path(shooter_ptr, path_g, project_length, shooter_ptr->y, shooter_ptr->x, ty, tx, PROJECT_PATH | PROJECT_THRU) - 1;

    project_length = 0; /* reset to default */

    /* Don't shoot at my feet */
    if (tx == shooter_ptr->x && ty == shooter_ptr->y) {
        PlayerEnergy(shooter_ptr).reset_player_turn();

        /* project_length is already reset to 0 */

        return;
    }

    /* Take a (partial) turn */
    PlayerEnergy(shooter_ptr).div_player_turn_energy((ENERGY)thits);
    shooter_ptr->is_fired = true;

    /* Sniper - Difficult to shot twice at 1 turn */
    if (snipe_type == SP_DOUBLE)
        shooter_ptr->concent = (shooter_ptr->concent + 1) / 2;

    /* Sniper - Repeat shooting when double shots */
    for (i = 0; i < ((snipe_type == SP_DOUBLE) ? 2 : 1); i++) {
        /* Start at the player */
        y = shooter_ptr->y;
        x = shooter_ptr->x;
        q_ptr = &forge;
        q_ptr->copy_from(o_ptr);

        /* Single object */
        q_ptr->number = 1;

        vary_item(shooter_ptr, item, -1);

        sound(SOUND_SHOOT);
        handle_stuff(shooter_ptr);

        prev_y = y;
        prev_x = x;

        /* The shot does not hit yet */
        hit_body = false;

        /* Travel until stopped */
        for (cur_dis = 0; cur_dis <= tdis;) {
            grid_type *g_ptr;

            /* Hack -- Stop at the target */
            if ((y == ty) && (x == tx))
                break;

            /* Calculate the new location (see "project()") */
            ny = y;
            nx = x;
            mmove2(&ny, &nx, shooter_ptr->y, shooter_ptr->x, ty, tx);

            /* Shatter Arrow */
            if (snipe_type == SP_KILL_WALL) {
                g_ptr = &shooter_ptr->current_floor_ptr->grid_array[ny][nx];

                if (g_ptr->cave_has_flag(FF_HURT_ROCK) && !g_ptr->m_idx) {
                    if (any_bits(g_ptr->info, (CAVE_MARK)))
                        msg_print(_("岩が砕け散った。", "Wall rocks were shattered."));
                    /* Forget the wall */
                    reset_bits(g_ptr->info, (CAVE_MARK));
                    set_bits(shooter_ptr->update, PU_VIEW | PU_LITE | PU_FLOW | PU_MON_LITE);

                    /* Destroy the wall */
                    cave_alter_feat(shooter_ptr, ny, nx, FF_HURT_ROCK);

                    hit_body = true;
                    break;
                }
            }

            /* Stopped by walls/doors */
            if (!cave_has_flag_bold(shooter_ptr->current_floor_ptr, ny, nx, FF_PROJECT) && !shooter_ptr->current_floor_ptr->grid_array[ny][nx].m_idx)
                break;

            /* Advance the distance */
            cur_dis++;

            /* Sniper */
            if (snipe_type == SP_LITE) {
                set_bits(shooter_ptr->current_floor_ptr->grid_array[ny][nx].info, CAVE_GLOW);
                note_spot(shooter_ptr, ny, nx);
                lite_spot(shooter_ptr, ny, nx);
            }

            /* The player can see the (on screen) missile */
            if (panel_contains(ny, nx) && player_can_see_bold(shooter_ptr, ny, nx)) {
                SYMBOL_CODE c = object_char(q_ptr);
                byte a = object_attr(q_ptr);

                /* Draw, Hilite, Fresh, Pause, Erase */
                if (msec > 0) {
                    print_rel(shooter_ptr, c, a, ny, nx);
                    move_cursor_relative(ny, nx);
                    term_fresh();
                    term_xtra(TERM_XTRA_DELAY, msec);
                    lite_spot(shooter_ptr, ny, nx);
                    term_fresh();
                }
            }

            /* The player cannot see the missile */
            else {
                /* Pause anyway, for consistancy **/
                if (msec > 0) {
                    term_xtra(TERM_XTRA_DELAY, msec);
                }
            }

            /* Sniper */
            if (snipe_type == SP_KILL_TRAP) {
                project(shooter_ptr, 0, 0, ny, nx, 0, GF_KILL_TRAP, (PROJECT_JUMP | PROJECT_HIDE | PROJECT_GRID | PROJECT_ITEM));
            }

            /* Sniper */
            if (snipe_type == SP_EVILNESS) {
                reset_bits(shooter_ptr->current_floor_ptr->grid_array[ny][nx].info, (CAVE_GLOW | CAVE_MARK));
                note_spot(shooter_ptr, ny, nx);
                lite_spot(shooter_ptr, ny, nx);
            }

            prev_y = y;
            prev_x = x;

            /* Save the new location */
            x = nx;
            y = ny;

            /* Monster here, Try to hit it */
            if (shooter_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                sound(SOUND_SHOOT_HIT);
                grid_type *c_mon_ptr = &shooter_ptr->current_floor_ptr->grid_array[y][x];

                monster_type *m_ptr = &shooter_ptr->current_floor_ptr->m_list[c_mon_ptr->m_idx];
                monster_race *r_ptr = &r_info[m_ptr->r_idx];

                /* Check the visibility */
                visible = m_ptr->ml;

                /* Note the collision */
                hit_body = true;

                if (monster_csleep_remaining(m_ptr)) {
                    if (none_bits(r_ptr->flags3, RF3_EVIL) || one_in_(5))
                        chg_virtue(shooter_ptr, V_COMPASSION, -1);
                    if (none_bits(r_ptr->flags3, RF3_EVIL) || one_in_(5))
                        chg_virtue(shooter_ptr, V_HONOUR, -1);
                }

                if ((r_ptr->level + 10) > shooter_ptr->lev) {
                    int now_exp = shooter_ptr->weapon_exp[0][j_ptr->sval];
                    if (now_exp < s_info[shooter_ptr->pclass].w_max[0][j_ptr->sval]) {
                        SUB_EXP amount = 0;
                        if (now_exp < WEAPON_EXP_BEGINNER)
                            amount = 80;
                        else if (now_exp < WEAPON_EXP_SKILLED)
                            amount = 25;
                        else if ((now_exp < WEAPON_EXP_EXPERT) && (shooter_ptr->lev > 19))
                            amount = 10;
                        else if (shooter_ptr->lev > 34)
                            amount = 2;
                        shooter_ptr->weapon_exp[0][j_ptr->sval] += amount;
                        set_bits(shooter_ptr->update, PU_BONUS);
                    }
                }

                if (shooter_ptr->riding) {
                    if ((shooter_ptr->skill_exp[SKILL_RIDING] < s_info[shooter_ptr->pclass].s_max[SKILL_RIDING])
                        && ((shooter_ptr->skill_exp[SKILL_RIDING] - (RIDING_EXP_BEGINNER * 2)) / 200
                            < r_info[shooter_ptr->current_floor_ptr->m_list[shooter_ptr->riding].r_idx].level)
                        && one_in_(2)) {
                        shooter_ptr->skill_exp[SKILL_RIDING] += 1;
                        set_bits(shooter_ptr->update, PU_BONUS);
                    }
                }

                /* Did we hit it (penalize range) */
                if (test_hit_fire(shooter_ptr, chance - cur_dis, m_ptr, m_ptr->ml, o_name)) {
                    bool fear = false;
                    auto tdam = tdam_base; //!< @note 実際に与えるダメージ
                    auto base_dam = tdam; //!< @note 補正前の与えるダメージ(無傷、全ての耐性など)

                    /* Get extra damage from concentration */
                    if (shooter_ptr->concent)
                        tdam = boost_concentration_damage(shooter_ptr, tdam);

                    /* Handle unseen monster */
                    if (!visible) {
                        /* Invisible monster */
                        msg_format(_("%sが敵を捕捉した。", "The %s finds a mark."), o_name);
                    }

                    /* Handle visible monster */
                    else {
                        GAME_TEXT m_name[MAX_NLEN];

                        /* Get "the monster" or "it" */
                        monster_desc(shooter_ptr, m_name, m_ptr, 0);

                        msg_format(_("%sが%sに命中した。", "The %s hits %s."), o_name, m_name);

                        if (m_ptr->ml) {
                            if (!shooter_ptr->image)
                                monster_race_track(shooter_ptr, m_ptr->ap_r_idx);
                            health_track(shooter_ptr, c_mon_ptr->m_idx);
                        }
                    }

                    if (snipe_type == SP_NEEDLE) {
                        if ((randint1(randint1(r_ptr->level / (3 + shooter_ptr->concent)) + (8 - shooter_ptr->concent)) == 1)
                            && none_bits(r_ptr->flags1, RF1_UNIQUE) && none_bits(r_ptr->flags7, RF7_UNIQUE2)) {
                            GAME_TEXT m_name[MAX_NLEN];

                            /* Get "the monster" or "it" */
                            monster_desc(shooter_ptr, m_name, m_ptr, 0);

                            tdam = m_ptr->hp + 1;
                            base_dam = tdam;
                            msg_format(_("%sの急所に突き刺さった！", "Your shot hit a fatal spot of %s!"), m_name);
                        } else {
                            tdam = 1;
                            base_dam = tdam;
                        }
                    } else {
                        /* Apply special damage */
                        tdam = calc_shot_damage_with_slay(shooter_ptr, j_ptr, q_ptr, tdam, m_ptr, snipe_type);
                        tdam = critical_shot(shooter_ptr, q_ptr->weight, q_ptr->to_h, j_ptr->to_h, tdam);

                        /* No negative damage */
                        if (tdam < 0)
                            tdam = 0;

                        /* Modify the damage */
                        base_dam = tdam;
                        tdam = mon_damage_mod(shooter_ptr, m_ptr, tdam, false);
                    }

                    msg_format_wizard(shooter_ptr, CHEAT_MONSTER, _("%dのダメージを与えた。(残りHP %d/%d(%d))", "You do %d damage. (left HP %d/%d(%d))"), tdam,
                        m_ptr->hp - tdam, m_ptr->maxhp, m_ptr->max_maxhp);

                    /* Sniper */
                    if (snipe_type == SP_EXPLODE) {
                        uint16_t flg = (PROJECT_STOP | PROJECT_JUMP | PROJECT_KILL | PROJECT_GRID);

                        sound(SOUND_EXPLODE); /* No explode sound - use breath fire instead */
                        project(shooter_ptr, 0, ((shooter_ptr->concent + 1) / 2 + 1), ny, nx, base_dam, GF_MISSILE, flg);
                        break;
                    }

                    /* Sniper */
                    if (snipe_type == SP_HOLYNESS) {
                        set_bits(shooter_ptr->current_floor_ptr->grid_array[ny][nx].info, CAVE_GLOW);
                        note_spot(shooter_ptr, ny, nx);
                        lite_spot(shooter_ptr, ny, nx);
                    }

                    /* Hit the monster, check for death */
                    MonsterDamageProcessor mdp(shooter_ptr, c_mon_ptr->m_idx, tdam, &fear);
                    if (mdp.mon_take_hit(extract_note_dies(real_r_idx(m_ptr)))) {
                        /* Dead monster */
                    }

                    /* No death */
                    else {
                        /* STICK TO */
                        if (object_is_fixed_artifact(q_ptr) && (shooter_ptr->pclass != CLASS_SNIPER || shooter_ptr->concent == 0)) {
                            GAME_TEXT m_name[MAX_NLEN];

                            monster_desc(shooter_ptr, m_name, m_ptr, 0);

                            stick_to = true;
                            msg_format(_("%sは%sに突き刺さった！", "%^s is stuck in %s!"), o_name, m_name);
                        }

                        message_pain(shooter_ptr, c_mon_ptr->m_idx, tdam);

                        /* Anger the monster */
                        if (tdam > 0)
                            anger_monster(shooter_ptr, m_ptr);

                        if (fear && m_ptr->ml) {
                            GAME_TEXT m_name[MAX_NLEN];
                            sound(SOUND_FLEE);
                            monster_desc(shooter_ptr, m_name, m_ptr, 0);
                            msg_format(_("%^sは恐怖して逃げ出した！", "%^s flees in terror!"), m_name);
                        }

                        set_target(m_ptr, shooter_ptr->y, shooter_ptr->x);

                        /* Sniper */
                        if (snipe_type == SP_RUSH) {
                            int n = randint1(5) + 3;
                            MONSTER_IDX m_idx = c_mon_ptr->m_idx;

                            for (; cur_dis <= tdis;) {
                                POSITION ox = nx;
                                POSITION oy = ny;

                                if (!n)
                                    break;

                                /* Calculate the new location (see "project()") */
                                mmove2(&ny, &nx, shooter_ptr->y, shooter_ptr->x, ty, tx);

                                /* Stopped by wilderness boundary */
                                if (!in_bounds2(shooter_ptr->current_floor_ptr, ny, nx))
                                    break;

                                /* Stopped by walls/doors */
                                if (!player_can_enter(shooter_ptr, shooter_ptr->current_floor_ptr->grid_array[ny][nx].feat, 0))
                                    break;

                                /* Stopped by monsters */
                                if (!is_cave_empty_bold(shooter_ptr, ny, nx))
                                    break;

                                shooter_ptr->current_floor_ptr->grid_array[ny][nx].m_idx = m_idx;
                                shooter_ptr->current_floor_ptr->grid_array[oy][ox].m_idx = 0;

                                m_ptr->fx = nx;
                                m_ptr->fy = ny;

                                update_monster(shooter_ptr, m_idx, true);

                                if (msec > 0) {
                                    lite_spot(shooter_ptr, ny, nx);
                                    lite_spot(shooter_ptr, oy, ox);
                                    term_fresh();
                                    term_xtra(TERM_XTRA_DELAY, msec);
                                }

                                x = nx;
                                y = ny;
                                cur_dis++;
                                n--;
                            }
                        }
                    }
                }

                /* Sniper */
                if (snipe_type == SP_PIERCE) {
                    if (shooter_ptr->concent < 1)
                        break;
                    shooter_ptr->concent--;
                    continue;
                }

                /* Stop looking */
                break;
            }
        }

        /* Chance of breakage (during attacks) */
        j = (hit_body ? breakage_chance(shooter_ptr, q_ptr, shooter_ptr->pclass == CLASS_ARCHER, snipe_type) : 0);

        if (stick_to) {
            MONSTER_IDX m_idx = shooter_ptr->current_floor_ptr->grid_array[y][x].m_idx;
            monster_type *m_ptr = &shooter_ptr->current_floor_ptr->m_list[m_idx];
            OBJECT_IDX o_idx = o_pop(shooter_ptr->current_floor_ptr);

            if (!o_idx) {
                msg_format(_("%sはどこかへ行った。", "The %s went somewhere."), o_name);
                if (object_is_fixed_artifact(q_ptr)) {
                    a_info[j_ptr->name1].cur_num = 0;
                }
                return;
            }

            o_ptr = &shooter_ptr->current_floor_ptr->o_list[o_idx];
            o_ptr->copy_from(q_ptr);

            /* Forget mark */
            reset_bits(o_ptr->marked, OM_TOUCHED);

            /* Forget location */
            o_ptr->iy = o_ptr->ix = 0;

            /* Memorize monster */
            o_ptr->held_m_idx = m_idx;

            /* Carry object */
            m_ptr->hold_o_idx_list.add(shooter_ptr->current_floor_ptr, o_idx);
        } else if (cave_has_flag_bold(shooter_ptr->current_floor_ptr, y, x, FF_PROJECT)) {
            /* Drop (or break) near that location */
            (void)drop_near(shooter_ptr, q_ptr, j, y, x);
        } else {
            /* Drop (or break) near that location */
            (void)drop_near(shooter_ptr, q_ptr, j, prev_y, prev_x);
        }

        /* Sniper - Repeat shooting when double shots */
    }

    /* Sniper - Loose his/her concentration after any shot */
    if (shooter_ptr->concent)
        reset_concentration(shooter_ptr, false);
}

/*!
 * @brief プレイヤーからモンスターへの射撃命中判定 /
 * Determine if the player "hits" a monster (normal combat).
 * @param chance 基本命中値
 * @param monster_ptr モンスターの構造体参照ポインタ
 * @param vis 目標を視界に捕らえているならばTRUEを指定
 * @param o_name メッセージ表示時のモンスター名
 * @return 命中と判定された場合TRUEを返す
 * @note Always miss 5%, always hit 5%, otherwise random.
 */
bool test_hit_fire(player_type *shooter_ptr, int chance, monster_type *m_ptr, int vis, char *o_name)
{
    int k;
    ARMOUR_CLASS ac;
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    /* Percentile dice */
    k = randint1(100);

    /* Snipers with high-concentration reduce instant miss percentage.*/
    k += shooter_ptr->concent;

    /* Hack -- Instant miss or hit */
    if (k <= 5)
        return false;
    if (k > 95)
        return true;

    if (shooter_ptr->pseikaku == PERSONALITY_LAZY)
        if (one_in_(20))
            return false;

    /* Never hit */
    if (chance <= 0)
        return false;

    ac = r_ptr->ac;
    if (shooter_ptr->concent) {
        ac *= (8 - shooter_ptr->concent);
        ac /= 8;
    }

    if (m_ptr->r_idx == MON_GOEMON && !monster_csleep_remaining(m_ptr))
        ac *= 3;

    /* Invisible monsters are harder to hit */
    if (!vis)
        chance = (chance + 1) / 2;

    /* Power competes against armor */
    if (randint0(chance) < (ac * 3 / 4)) {
        if (m_ptr->r_idx == MON_GOEMON && !monster_csleep_remaining(m_ptr)) {
            GAME_TEXT m_name[MAX_NLEN];
            monster_desc(shooter_ptr, m_name, m_ptr, 0);
            msg_format(_("%sは%sを斬り捨てた！", "%s cuts down %s!"), m_name, o_name);
        }
        return false;
    }

    /* Assume hit */
    return true;
}

/*!
 * @brief プレイヤーからモンスターへの射撃クリティカル判定 /
 * Critical hits (from objects thrown by player) Factor in item weight, total plusses, and player level.
 * @param weight 矢弾の重量
 * @param plus_ammo 矢弾の命中修正
 * @param plus_bow 弓の命中修正
 * @param dam 現在算出中のダメージ値
 * @return クリティカル修正が入ったダメージ値
 */
HIT_POINT critical_shot(player_type *shooter_ptr, WEIGHT weight, int plus_ammo, int plus_bow, HIT_POINT dam)
{
    int i, k;
    object_type *j_ptr = &shooter_ptr->inventory_list[INVEN_BOW];

    /* Extract "shot" power */
    i = shooter_ptr->to_h_b + plus_ammo;

    if (shooter_ptr->tval_ammo == TV_BOLT)
        i = (shooter_ptr->skill_thb + (shooter_ptr->weapon_exp[0][j_ptr->sval] / 400 + i) * BTH_PLUS_ADJ);
    else
        i = (shooter_ptr->skill_thb + ((shooter_ptr->weapon_exp[0][j_ptr->sval] - (WEAPON_EXP_MASTER / 2)) / 200 + i) * BTH_PLUS_ADJ);

    /* Snipers can shot more critically with crossbows */
    if (shooter_ptr->concent)
        i += ((i * shooter_ptr->concent) / 5);
    if ((shooter_ptr->pclass == CLASS_SNIPER) && (shooter_ptr->tval_ammo == TV_BOLT))
        i *= 2;

    /* Good bow makes more critical */
    i += plus_bow * 8 * (shooter_ptr->concent ? shooter_ptr->concent + 5 : 5);

    /* Critical hit */
    if (randint1(10000) <= i) {
        k = weight * randint1(500);

        if (k < 900) {
            msg_print(_("手ごたえがあった！", "It was a good hit!"));
            dam += (dam / 2);
        } else if (k < 1350) {
            msg_print(_("かなりの手ごたえがあった！", "It was a great hit!"));
            dam *= 2;
        } else {
            msg_print(_("会心の一撃だ！", "It was a superb hit!"));
            dam *= 3;
        }
    }

    return (dam);
}

/*!
 * @brief 射撃武器の攻撃に必要な基本消費エネルギーを返す/Return bow energy
 * @param sval 射撃武器のアイテム副分類ID
 * @return 消費する基本エネルギー
 */
ENERGY bow_energy(OBJECT_SUBTYPE_VALUE sval)
{
    ENERGY energy = 10000;

    /* Analyze the launcher */
    switch (sval) {
        /* Sling and ammo */
    case SV_SLING: {
        energy = 8000;
        break;
    }

    /* Short Bow and Arrow */
    case SV_SHORT_BOW: {
        energy = 10000;
        break;
    }

    /* Long Bow and Arrow */
    case SV_LONG_BOW: {
        energy = 10000;
        break;
    }

    /* Bow of irresponsiblity and Arrow */
    case SV_NAMAKE_BOW: {
        energy = 7777;
        break;
    }

    /* Light Crossbow and Bolt */
    case SV_LIGHT_XBOW: {
        energy = 12000;
        break;
    }

    /* Heavy Crossbow and Bolt */
    case SV_HEAVY_XBOW: {
        energy = 13333;
        break;
    }
    }

    return (energy);
}

/*
 * Return bow tmul
 */
int bow_tmul(OBJECT_SUBTYPE_VALUE sval)
{
    int tmul = 0;

    /* Analyze the launcher */
    switch (sval) {
        /* Sling and ammo */
    case SV_SLING: {
        tmul = 2;
        break;
    }

    /* Short Bow and Arrow */
    case SV_SHORT_BOW: {
        tmul = 2;
        break;
    }

    /* Long Bow and Arrow */
    case SV_LONG_BOW: {
        tmul = 3;
        break;
    }

    /* Bow of irresponsiblity and Arrow */
    case SV_NAMAKE_BOW: {
        tmul = 3;
        break;
    }

    /* Light Crossbow and Bolt */
    case SV_LIGHT_XBOW: {
        tmul = 3;
        break;
    }

    /* Heavy Crossbow and Bolt */
    case SV_HEAVY_XBOW: {
        tmul = 4;
        break;
    }
    }

    return (tmul);
}

/*!
 * @brief 射撃時クリティカルによるダメージ期待値修正計算（スナイパーの集中処理と武器経験値） / critical happens at i / 10000
 * @param shooter_ptr 射撃を行うクリーチャー参照ポインタ
 * @param plus_ammo 矢弾のダメージ修正
 * @param plus_bow 弓のダメージ修正
 * @return ダメージ期待値
 * @note 基本ダメージ量と重量はこの部位では計算に加わらない。
 */
HIT_POINT calc_crit_ratio_shot(player_type *shooter_ptr, HIT_POINT plus_ammo, HIT_POINT plus_bow)
{
    HIT_POINT i;
    object_type *j_ptr = &shooter_ptr->inventory_list[INVEN_BOW];

    /* Extract "shot" power */
    i = shooter_ptr->to_h_b + plus_ammo;

    if (shooter_ptr->tval_ammo == TV_BOLT)
        i = (shooter_ptr->skill_thb + (shooter_ptr->weapon_exp[0][j_ptr->sval] / 400 + i) * BTH_PLUS_ADJ);
    else
        i = (shooter_ptr->skill_thb + ((shooter_ptr->weapon_exp[0][j_ptr->sval] - (WEAPON_EXP_MASTER / 2)) / 200 + i) * BTH_PLUS_ADJ);

    /* Snipers can shot more critically with crossbows */
    if (shooter_ptr->concent)
        i += ((i * shooter_ptr->concent) / 5);
    if ((shooter_ptr->pclass == CLASS_SNIPER) && (shooter_ptr->tval_ammo == TV_BOLT))
        i *= 2;

    /* Good bow makes more critical */
    i += plus_bow * 8 * (shooter_ptr->concent ? shooter_ptr->concent + 5 : 5);

    if (i < 0)
        i = 0;

    return i;
}

/*!
 * @brief 射撃時クリティカルによるダメージ期待値修正計算（重量依存部分） / critical happens at i / 10000
 * @param weight 武器の重量
 * @param plus_ammo 矢弾のダメージ修正
 * @param plus_bow 弓のダメージ修正
 * @param dam 基本ダメージ量
 * @return ダメージ期待値
 */
HIT_POINT calc_expect_crit_shot(player_type *shooter_ptr, WEIGHT weight, int plus_ammo, int plus_bow, HIT_POINT dam)
{
    uint32_t num;
    int i, k, crit;
    i = calc_crit_ratio_shot(shooter_ptr, plus_ammo, plus_bow);

    k = 0;
    num = 0;

    crit = MIN(500, 900 / weight);
    num += dam * 3 / 2 * crit;
    k = crit;

    crit = MIN(500, 1350 / weight);
    crit -= k;
    num += dam * 2 * crit;
    k += crit;

    if (k < 500) {
        crit = 500 - k;
        num += dam * 3 * crit;
    }

    num /= 500;

    num *= i;
    num += (10000 - i) * dam;
    num /= 10000;

    return num;
}

/*!
 * @brief 攻撃時クリティカルによるダメージ期待値修正計算（重量と毒針処理） / critical happens at i / 10000
 * @param shooter_ptr プレーヤーへの参照ポインタ
 * @param weight 武器の重量
 * @param plus 武器のダメージ修正
 * @param dam 基本ダメージ
 * @param meichuu 命中値
 * @param dokubari 毒針処理か否か
 * @param impact 強撃かどうか
 * @return ダメージ期待値
 */
HIT_POINT calc_expect_crit(player_type *shooter_ptr, WEIGHT weight, int plus, HIT_POINT dam, int16_t meichuu, bool dokubari, bool impact)
{
    if (dokubari)
        return dam;

    int i = (weight + (meichuu * 3 + plus * 5) + shooter_ptr->skill_thn);
    if (i < 0)
        i = 0;

    // 通常ダメージdam、武器重量weightでクリティカルが発生した時のクリティカルダメージ期待値
    auto calc_weight_expect_dam = [](HIT_POINT dam, WEIGHT weight) {
        HIT_POINT sum = 0;
        for (int d = 1; d <= 650; ++d) {
            int k = weight + d;
            sum += std::get<0>(apply_critical_norm_damage(k, dam));
        }
        return sum / 650;
    };

    uint32_t num = 0;

    if (impact) {
        for (int d = 1; d <= 650; ++d) {
            num += calc_weight_expect_dam(dam, weight + d);
        }
        num /= 650;
    } else {
        num += calc_weight_expect_dam(dam, weight);
    }

    int pow = (shooter_ptr->pclass == CLASS_NINJA) ? 4444 : 5000;
    if (impact)
        pow /= 2;

    num *= i;
    num += (pow - i) * dam;
    num /= pow;

    return num;
}
