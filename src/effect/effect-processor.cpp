#include "effect/effect-processor.h"
#include "core/stuff-handler.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-feature.h"
#include "effect/effect-item.h"
#include "effect/effect-monster.h"
#include "effect/effect-player.h"
#include "effect/spells-effect-util.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "floor/line-of-sight.h"
#include "game-option/map-screen-options.h"
#include "game-option/special-options.h"
#include "grid/feature-flag-types.h"
#include "grid/grid.h"
#include "io/cursor.h"
#include "io/screen-util.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "pet/pet-fall-off.h"
#include "player/player-status.h"
#include "spell-class/spells-mirror-master.h"
#include "spell/range-calc.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "timed-effect/player-blindness.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 汎用的なビーム/ボルト/ボール系処理のルーチン Generic
 * "beam"/"bolt"/"ball" projection routine.
 * @param who 魔法を発動したモンスター(0ならばプレイヤー) / Index of "source"
 * monster (zero for "player")
 * @param rad 効果半径(ビーム/ボルト = 0 / ボール = 1以上) / Radius of explosion
 * (0 = beam/bolt, 1 to 9 = ball)
 * @param target_y 目標Y座標 / Target y location (or location to travel "towards")
 * @param target_x 目標X座標 / Target x location (or location to travel "towards")
 * @param dam 基本威力 / Base damage roll to apply to affected monsters (or
 * player)
 * @param typ 効果属性 / Type of damage to apply to monsters (and objects)
 * @param flag 効果フラグ / Extra bit flags (see PROJECT_xxxx)
 * @param monspell 効果元のモンスター魔法ID
 * @todo 似たような処理が山ほど並んでいる、何とかならないものか
 * @todo 引数にそのまま再代入していてカオスすぎる。直すのは簡単ではない
 */
ProjectResult project(PlayerType *player_ptr, const MONSTER_IDX who, POSITION rad, const POSITION target_y, const POSITION target_x, const int dam,
    const AttributeType typ, BIT_FLAGS flag, std::optional<CapturedMonsterType *> cap_mon_ptr)
{
    POSITION y1;
    POSITION x1;
    POSITION y2;
    POSITION x2;
    bool breath = false;
    bool old_hide = false;
    int path_n = 0;
    int grids = 0;
    POSITION gx[1024];
    POSITION gy[1024];
    POSITION gm[32];
    rakubadam_p = 0;
    rakubadam_m = 0;
    monster_target_y = player_ptr->y;
    monster_target_x = player_ptr->x;

    ProjectResult res;

    if (any_bits(flag, PROJECT_JUMP)) {
        x1 = target_x;
        y1 = target_y;
    } else if (who <= 0) {
        x1 = player_ptr->x;
        y1 = player_ptr->y;
    } else if (who > 0) {
        x1 = player_ptr->current_floor_ptr->m_list[who].fx;
        y1 = player_ptr->current_floor_ptr->m_list[who].fy;
    } else {
        x1 = target_x;
        y1 = target_y;
    }
    y2 = target_y;
    x2 = target_x;

    if (flag & (PROJECT_THRU)) {
        if ((x1 == x2) && (y1 == y2)) {
            flag &= ~(PROJECT_THRU);
        }
    }

    if (flag & (PROJECT_BREATH)) {
        breath = true;
        if (flag & PROJECT_HIDE) {
            old_hide = true;
        }
        flag |= PROJECT_HIDE;
    }

    for (auto dist = 0; dist < 32; dist++) {
        gm[dist] = 0;
    }

    if (flag & (PROJECT_BEAM)) {
        gy[grids] = y1;
        gx[grids] = x1;
        grids++;
    }

    switch (typ) {
    case AttributeType::LITE:
    case AttributeType::LITE_WEAK:
        if (breath || (flag & PROJECT_BEAM)) {
            flag |= (PROJECT_LOS);
        }
        break;
    case AttributeType::DISINTEGRATE:
        flag |= (PROJECT_GRID);
        if (breath || (flag & PROJECT_BEAM)) {
            flag |= (PROJECT_DISI);
        }
        break;
    default:
        break;
    }

    /* Calculate the projection path */
    projection_path path_g(player_ptr, (project_length ? project_length : get_max_range(player_ptr)), y1, x1, y2, x2, flag);
    handle_stuff(player_ptr);

    int k = 0;
    auto oy = y1;
    auto ox = x1;
    auto visual = false;
    POSITION gm_rad = rad;
    bool see_s_msg = true;
    const auto is_blind = player_ptr->effects()->blindness()->is_blind();
    for (const auto &[ny, nx] : path_g) {
        if (flag & PROJECT_DISI) {
            if (cave_stop_disintegration(player_ptr->current_floor_ptr, ny, nx) && (rad > 0)) {
                break;
            }
        } else if (flag & PROJECT_LOS) {
            if (!cave_los_bold(player_ptr->current_floor_ptr, ny, nx) && (rad > 0)) {
                break;
            }
        } else {
            if (!cave_has_flag_bold(player_ptr->current_floor_ptr, ny, nx, TerrainCharacteristics::PROJECT) && (rad > 0)) {
                break;
            }
        }
        if (flag & (PROJECT_BEAM)) {
            gy[grids] = ny;
            gx[grids] = nx;
            grids++;
        }

        if (delay_factor > 0) {
            if (!is_blind && !(flag & (PROJECT_HIDE | PROJECT_FAST))) {
                if (panel_contains(ny, nx) && player_has_los_bold(player_ptr, ny, nx)) {
                    print_bolt_pict(player_ptr, oy, ox, ny, nx, typ);
                    move_cursor_relative(ny, nx);
                    term_fresh();
                    term_xtra(TERM_XTRA_DELAY, delay_factor);
                    lite_spot(player_ptr, ny, nx);
                    term_fresh();
                    if (flag & (PROJECT_BEAM)) {
                        print_bolt_pict(player_ptr, ny, nx, ny, nx, typ);
                    }

                    visual = true;
                } else if (visual) {
                    term_xtra(TERM_XTRA_DELAY, delay_factor);
                }
            }
        }

        oy = ny;
        ox = nx;
        k++;
    }

    path_n = k;
    POSITION by = oy;
    POSITION bx = ox;
    if (breath && !path_n) {
        breath = false;
        gm_rad = rad;
        if (!old_hide) {
            flag &= ~(PROJECT_HIDE);
        }
    }

    gm[0] = 0;
    gm[1] = grids;
    project_length = 0;

    /* If we found a "target", explode there */
    if (path_n <= get_max_range(player_ptr)) {
        if ((flag & (PROJECT_BEAM)) && (grids > 0)) {
            grids--;
        }

        /*
         * Create a conical breath attack
         *
         *       ***
         *   ********
         * D********@**
         *   ********
         *       ***
         */
        if (breath) {
            flag &= ~(PROJECT_HIDE);
            breath_shape(player_ptr, path_g, path_n, &grids, gx, gy, gm, &gm_rad, rad, y1, x1, by, bx, typ);
        } else {
            for (auto dist = 0; dist <= rad; dist++) {
                for (auto y = by - dist; y <= by + dist; y++) {
                    for (auto x = bx - dist; x <= bx + dist; x++) {
                        if (!in_bounds2(player_ptr->current_floor_ptr, y, x)) {
                            continue;
                        }
                        if (distance(by, bx, y, x) != dist) {
                            continue;
                        }

                        switch (typ) {
                        case AttributeType::LITE:
                        case AttributeType::LITE_WEAK:
                            if (!los(player_ptr, by, bx, y, x)) {
                                continue;
                            }
                            break;
                        case AttributeType::DISINTEGRATE:
                            if (!in_disintegration_range(player_ptr->current_floor_ptr, by, bx, y, x)) {
                                continue;
                            }
                            break;
                        default:
                            if (!projectable(player_ptr, by, bx, y, x)) {
                                continue;
                            }
                            break;
                        }

                        gy[grids] = y;
                        gx[grids] = x;
                        grids++;
                    }
                }

                gm[dist + 1] = grids;
            }
        }
    }

    if (!grids) {
        return res;
    }

    if (!is_blind && !(flag & (PROJECT_HIDE)) && (delay_factor > 0)) {
        auto drawn = false;
        for (int t = 0; t <= gm_rad; t++) {
            for (int i = gm[t]; i < gm[t + 1]; i++) {
                auto y = gy[i];
                auto x = gx[i];
                if (panel_contains(y, x) && player_has_los_bold(player_ptr, y, x)) {
                    drawn = true;
                    print_bolt_pict(player_ptr, y, x, y, x, typ);
                }
            }

            move_cursor_relative(by, bx);
            term_fresh();
            if (visual || drawn) {
                term_xtra(TERM_XTRA_DELAY, delay_factor);
            }
        }

        if (drawn) {
            for (int i = 0; i < grids; i++) {
                auto y = gy[i];
                auto x = gx[i];
                if (panel_contains(y, x) && player_has_los_bold(player_ptr, y, x)) {
                    lite_spot(player_ptr, y, x);
                }
            }

            move_cursor_relative(by, bx);
            term_fresh();
        }
    }

    update_creature(player_ptr);

    if (flag & PROJECT_KILL) {
        see_s_msg = (who > 0) ? is_seen(player_ptr, &player_ptr->current_floor_ptr->m_list[who])
                              : (!who ? true : (player_can_see_bold(player_ptr, y1, x1) && projectable(player_ptr, player_ptr->y, player_ptr->x, y1, x1)));
    }

    if (flag & (PROJECT_GRID)) {
        auto dist = 0;
        for (int i = 0; i < grids; i++) {
            if (gm[dist + 1] == i) {
                dist++;
            }
            auto y = gy[i];
            auto x = gx[i];
            if (breath) {
                int d = dist_to_line(y, x, y1, x1, by, bx);
                if (affect_feature(player_ptr, who, d, y, x, dam, typ)) {
                    res.notice = true;
                }
            } else {
                if (affect_feature(player_ptr, who, dist, y, x, dam, typ)) {
                    res.notice = true;
                }
            }
        }
    }

    update_creature(player_ptr);
    if (flag & (PROJECT_ITEM)) {
        auto dist = 0;
        for (int i = 0; i < grids; i++) {
            if (gm[dist + 1] == i) {
                dist++;
            }

            auto y = gy[i];
            auto x = gx[i];
            if (breath) {
                int d = dist_to_line(y, x, y1, x1, by, bx);
                if (affect_item(player_ptr, who, d, y, x, dam, typ)) {
                    res.notice = true;
                }
            } else {
                if (affect_item(player_ptr, who, dist, y, x, dam, typ)) {
                    res.notice = true;
                }
            }
        }
    }

    if (flag & (PROJECT_KILL)) {
        project_m_n = 0;
        project_m_x = 0;
        project_m_y = 0;
        auto dist = 0;
        for (int i = 0; i < grids; i++) {
            int effective_dist;
            if (gm[dist + 1] == i) {
                dist++;
            }

            auto y = gy[i];
            auto x = gx[i];
            if (grids <= 1) {
                auto *m_ptr = &player_ptr->current_floor_ptr->m_list[player_ptr->current_floor_ptr->grid_array[y][x].m_idx];
                MonsterRaceInfo *ref_ptr = &monraces_info[m_ptr->r_idx];
                if ((flag & PROJECT_REFLECTABLE) && player_ptr->current_floor_ptr->grid_array[y][x].m_idx && (ref_ptr->flags2 & RF2_REFLECTING) && ((player_ptr->current_floor_ptr->grid_array[y][x].m_idx != player_ptr->riding) || !(flag & PROJECT_PLAYER)) && (!who || path_n > 1) && !one_in_(10)) {
                    POSITION t_y, t_x;
                    int max_attempts = 10;
                    do {
                        t_y = y1 - 1 + randint1(3);
                        t_x = x1 - 1 + randint1(3);
                        max_attempts--;
                    } while (max_attempts && in_bounds2u(player_ptr->current_floor_ptr, t_y, t_x) && !projectable(player_ptr, y, x, t_y, t_x));

                    if (max_attempts < 1) {
                        t_y = y1;
                        t_x = x1;
                    }

                    if (is_seen(player_ptr, m_ptr)) {
                        sound(SOUND_REFLECT);
                        if ((m_ptr->r_idx == MonsterRaceId::KENSHIROU) || (m_ptr->r_idx == MonsterRaceId::RAOU)) {
                            msg_print(_("「北斗神拳奥義・二指真空把！」", "The attack bounces!"));
                        } else if (m_ptr->r_idx == MonsterRaceId::DIO) {
                            msg_print(_("ディオ・ブランドーは指一本で攻撃を弾き返した！", "The attack bounces!"));
                        } else {
                            msg_print(_("攻撃は跳ね返った！", "The attack bounces!"));
                        }
                    } else if (who <= 0) {
                        sound(SOUND_REFLECT);
                    }

                    if (is_original_ap_and_seen(player_ptr, m_ptr)) {
                        ref_ptr->r_flags2 |= RF2_REFLECTING;
                    }

                    if (player_bold(player_ptr, y, x) || one_in_(2)) {
                        flag &= ~(PROJECT_PLAYER);
                    } else {
                        flag |= PROJECT_PLAYER;
                    }

                    project(player_ptr, player_ptr->current_floor_ptr->grid_array[y][x].m_idx, 0, t_y, t_x, dam, typ, flag);
                    continue;
                }
            }

            /* Find the closest point in the blast */
            if (breath) {
                effective_dist = dist_to_line(y, x, y1, x1, by, bx);
            } else {
                effective_dist = dist;
            }

            if (player_ptr->riding && player_bold(player_ptr, y, x)) {
                if (flag & PROJECT_PLAYER) {
                    if (flag & (PROJECT_BEAM | PROJECT_REFLECTABLE | PROJECT_AIMED)) {
                        /*
                         * A beam or bolt is well aimed
                         * at the PLAYER!
                         * So don't affects the mount.
                         */
                        continue;
                    } else {
                        /*
                         * The spell is not well aimed,
                         * So partly affect the mount too.
                         */
                        effective_dist++;
                    }
                }

                /*
                 * This grid is the original target.
                 * Or aimed on your horse.
                 */
                else if (((y == y2) && (x == x2)) || (flag & PROJECT_AIMED)) {
                    /* Hit the mount with full damage */
                }

                /*
                 * Otherwise this grid is not the
                 * original target, it means that line
                 * of fire is obstructed by this
                 * monster.
                 */
                /*
                 * A beam or bolt will hit either
                 * player or mount.  Choose randomly.
                 */
                else if (flag & (PROJECT_BEAM | PROJECT_REFLECTABLE)) {
                    if (one_in_(2)) {
                        /* Hit the mount with full damage */
                    } else {
                        flag |= PROJECT_PLAYER;
                        continue;
                    }
                }

                /*
                 * The spell is not well aimed, so
                 * partly affect both player and
                 * mount.
                 */
                else {
                    effective_dist++;
                }
            }

            if (affect_monster(player_ptr, who, effective_dist, y, x, dam, typ, flag, see_s_msg, cap_mon_ptr)) {
                res.notice = true;
            }
        }
        /* Player affected one monster (without "jumping") */
        if (!who && (project_m_n == 1) && none_bits(flag, PROJECT_JUMP)) {
            auto x = project_m_x;
            auto y = project_m_y;
            if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx > 0) {
                auto *m_ptr = &player_ptr->current_floor_ptr->m_list[player_ptr->current_floor_ptr->grid_array[y][x].m_idx];
                if (m_ptr->ml) {
                    if (!player_ptr->effects()->hallucination()->is_hallucinated()) {
                        monster_race_track(player_ptr, m_ptr->ap_r_idx);
                    }

                    health_track(player_ptr, player_ptr->current_floor_ptr->grid_array[y][x].m_idx);
                }
            }
        }
    }

    if (flag & (PROJECT_KILL)) {
        auto dist = 0;
        for (int i = 0; i < grids; i++) {
            int effective_dist;
            if (gm[dist + 1] == i) {
                dist++;
            }

            auto y = gy[i];
            auto x = gx[i];
            if (!player_bold(player_ptr, y, x)) {
                continue;
            }

            /* Find the closest point in the blast */
            if (breath) {
                effective_dist = dist_to_line(y, x, y1, x1, by, bx);
            } else {
                effective_dist = dist;
            }

            if (player_ptr->riding) {
                if (flag & PROJECT_PLAYER) {
                    /* Hit the player with full damage */
                }

                /*
                 * Hack -- When this grid was not the
                 * original target, a beam or bolt
                 * would hit either player or mount,
                 * and should be choosen randomly.
                 *
                 * But already choosen to hit the
                 * mount at this point.
                 *
                 * Or aimed on your horse.
                 */
                else if (flag & (PROJECT_BEAM | PROJECT_REFLECTABLE | PROJECT_AIMED)) {
                    /*
                     * A beam or bolt is well aimed
                     * at the mount!
                     * So don't affects the player.
                     */
                    continue;
                } else {
                    /*
                     * The spell is not well aimed,
                     * So partly affect the player too.
                     */
                    effective_dist++;
                }
            }

            std::string who_name;
            if (who > 0) {
                who_name = monster_desc(player_ptr, &player_ptr->current_floor_ptr->m_list[who], MD_WRONGDOER_NAME);
            }

            if (affect_player(who, player_ptr, who_name.data(), effective_dist, y, x, dam, typ, flag, project)) {
                res.notice = true;
                res.affected_player = true;
            }
        }
    }

    if (player_ptr->riding) {
        const auto m_name = monster_desc(player_ptr, &player_ptr->current_floor_ptr->m_list[player_ptr->riding], 0);
        if (rakubadam_m > 0) {
            if (process_fall_off_horse(player_ptr, rakubadam_m, false)) {
                msg_format(_("%^sに振り落とされた！", "%^s has thrown you off!"), m_name.data());
            }
        }

        if (player_ptr->riding && rakubadam_p > 0) {
            if (process_fall_off_horse(player_ptr, rakubadam_p, false)) {
                msg_format(_("%^sから落ちてしまった！", "You have fallen from %s."), m_name.data());
            }
        }
    }

    return res;
}
