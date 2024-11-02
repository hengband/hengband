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
#include "floor/floor-list.h"
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
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-util.h"
#include "pet/pet-fall-off.h"
#include "player/player-status.h"
#include "spell-class/spells-mirror-master.h"
#include "spell/range-calc.h"
#include "system/angband-system.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "timed-effect/timed-effects.h"
#include "tracking/lore-tracker.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 汎用的なビーム/ボルト/ボール系処理のルーチン Generic
 * "beam"/"bolt"/"ball" projection routine.
 * @param src_idx 魔法を発動したモンスター(0ならばプレイヤー) / Index of "source"
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
ProjectResult project(PlayerType *player_ptr, const MONSTER_IDX src_idx, POSITION rad, const POSITION target_y, const POSITION target_x, const int dam,
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
    POSITION gx[1024]{};
    POSITION gy[1024]{};
    POSITION gm[32]{};
    auto &floor = FloorList::get_instance().get_floor(0);
    monster_target_y = player_ptr->y;
    monster_target_x = player_ptr->x;

    ProjectResult res;

    if (any_bits(flag, PROJECT_JUMP)) {
        x1 = target_x;
        y1 = target_y;
    } else if (!is_monster(src_idx)) {
        x1 = player_ptr->x;
        y1 = player_ptr->y;
    } else if (is_monster(src_idx)) {
        x1 = floor.m_list[src_idx].fx;
        y1 = floor.m_list[src_idx].fy;
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
    const auto &system = AngbandSystem::get_instance();
    ProjectionPath path_g(player_ptr, (project_length ? project_length : system.get_max_range()), { y1, x1 }, { y2, x2 }, flag);
    handle_stuff(player_ptr);

    int k = 0;
    auto oy = y1;
    auto ox = x1;
    auto visual = false;
    bool see_s_msg = true;
    const auto is_blind = player_ptr->effects()->blindness().is_blind();
    for (const auto &[ny, nx] : path_g) {
        const Pos2D pos(ny, nx);
        if (flag & PROJECT_DISI) {
            if (cave_stop_disintegration(&floor, ny, nx) && (rad > 0)) {
                break;
            }
        } else if (flag & PROJECT_LOS) {
            if (!cave_los_bold(&floor, ny, nx) && (rad > 0)) {
                break;
            }
        } else {
            if (!cave_has_flag_bold(&floor, ny, nx, TerrainCharacteristics::PROJECT) && (rad > 0)) {
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
                if (panel_contains(ny, nx) && floor.has_los(pos)) {
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
        if (!old_hide) {
            flag &= ~(PROJECT_HIDE);
        }
    }

    gm[0] = 0;
    gm[1] = grids;
    project_length = 0;

    POSITION gm_rad = rad;
    /* If we found a "target", explode there */
    if (path_n <= system.get_max_range()) {
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
                        if (!in_bounds2(&floor, y, x)) {
                            continue;
                        }
                        if (distance(by, bx, y, x) != dist) {
                            continue;
                        }

                        switch (typ) {
                        case AttributeType::LITE:
                        case AttributeType::LITE_WEAK:
                            if (!los(by, bx, y, x)) {
                                continue;
                            }
                            break;
                        case AttributeType::DISINTEGRATE:
                            if (!in_disintegration_range(&floor, by, bx, y, x)) {
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
                const Pos2D pos(gy[i], gx[i]);
                if (panel_contains(pos.y, pos.x) && floor.has_los(pos)) {
                    drawn = true;
                    print_bolt_pict(player_ptr, pos.y, pos.x, pos.y, pos.x, typ);
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
                const Pos2D pos(gy[i], gx[i]);
                if (panel_contains(pos.y, pos.x) && floor.has_los(pos)) {
                    lite_spot(player_ptr, pos.y, pos.x);
                }
            }

            move_cursor_relative(by, bx);
            term_fresh();
        }
    }

    update_creature(player_ptr);

    if (flag & PROJECT_KILL) {
        see_s_msg = is_monster(src_idx) ? is_seen(player_ptr, &floor.m_list[src_idx])
                                        : (is_player(src_idx) ? true : (player_can_see_bold(player_ptr, y1, x1) && projectable(player_ptr, player_ptr->y, player_ptr->x, y1, x1)));
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
                if (affect_feature(player_ptr, src_idx, d, y, x, dam, typ)) {
                    res.notice = true;
                }
            } else {
                if (affect_feature(player_ptr, src_idx, dist, y, x, dam, typ)) {
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
                if (affect_item(player_ptr, src_idx, d, y, x, dam, typ)) {
                    res.notice = true;
                }
            } else {
                if (affect_item(player_ptr, src_idx, dist, y, x, dam, typ)) {
                    res.notice = true;
                }
            }
        }
    }

    FallOffHorseEffect fall_off_horse_effect(player_ptr);
    if (flag & (PROJECT_KILL)) {
        project_m_n = 0;
        project_m_x = 0;
        project_m_y = 0;
        auto dist = 0;
        auto &tracker = LoreTracker::get_instance();
        for (int i = 0; i < grids; i++) {
            int effective_dist;
            if (gm[dist + 1] == i) {
                dist++;
            }

            const Pos2D pos(gy[i], gx[i]);
            const auto &grid = floor.get_grid(pos);
            if (grids <= 1) {
                auto *m_ptr = &floor.m_list[grid.m_idx];
                MonsterRaceInfo *ref_ptr = &m_ptr->get_monrace();
                if ((flag & PROJECT_REFLECTABLE) && grid.m_idx && ref_ptr->misc_flags.has(MonsterMiscType::REFLECTING) && (!m_ptr->is_riding() || !(flag & PROJECT_PLAYER)) && (!src_idx || path_n > 1) && !one_in_(10)) {

                    POSITION t_y, t_x;
                    int max_attempts = 10;
                    do {
                        t_y = y1 - 1 + randint1(3);
                        t_x = x1 - 1 + randint1(3);
                        max_attempts--;
                    } while (max_attempts && in_bounds2u(&floor, t_y, t_x) && !projectable(player_ptr, pos.y, pos.x, t_y, t_x));

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
                    } else if (!is_monster(src_idx)) {
                        sound(SOUND_REFLECT);
                    }

                    if (is_original_ap_and_seen(player_ptr, m_ptr)) {
                        ref_ptr->r_misc_flags.set(MonsterMiscType::REFLECTING);
                    }

                    if (player_ptr->is_located_at(pos) || one_in_(2)) {
                        flag &= ~(PROJECT_PLAYER);
                    } else {
                        flag |= PROJECT_PLAYER;
                    }

                    project(player_ptr, grid.m_idx, 0, t_y, t_x, dam, typ, flag);
                    continue;
                }
            }

            /* Find the closest point in the blast */
            if (breath) {
                effective_dist = dist_to_line(pos.y, pos.x, y1, x1, by, bx);
            } else {
                effective_dist = dist;
            }

            if (player_ptr->riding && player_ptr->is_located_at(pos)) {
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
                else if (((pos.y == y2) && (pos.x == x2)) || (flag & PROJECT_AIMED)) {
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

            if (affect_monster(player_ptr, src_idx, effective_dist, pos.y, pos.x, dam, typ, flag, see_s_msg, cap_mon_ptr, &fall_off_horse_effect)) {
                res.notice = true;
            }
        }
        /* Player affected one monster (without "jumping") */
        if (!src_idx && (project_m_n == 1) && none_bits(flag, PROJECT_JUMP)) {
            const Pos2D pos_project(project_m_y, project_m_x);
            const auto &grid = floor.get_grid(pos_project);
            if (grid.has_monster()) {
                auto &monster = floor.m_list[grid.m_idx];
                if (monster.ml) {
                    if (!player_ptr->effects()->hallucination().is_hallucinated()) {
                        tracker.set_trackee(monster.ap_r_idx);
                    }

                    health_track(player_ptr, grid.m_idx);
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

            const Pos2D pos(gy[i], gx[i]);
            if (!player_ptr->is_located_at(pos)) {
                continue;
            }

            /* Find the closest point in the blast */
            if (breath) {
                effective_dist = dist_to_line(pos.y, pos.x, y1, x1, by, bx);
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
            if (is_monster(src_idx)) {
                who_name = monster_desc(player_ptr, &floor.m_list[src_idx], MD_WRONGDOER_NAME);
            }

            if (affect_player(src_idx, player_ptr, who_name.data(), effective_dist, pos.y, pos.x, dam, typ, flag, fall_off_horse_effect, project)) {
                res.notice = true;
                res.affected_player = true;
            }
        }
    }

    fall_off_horse_effect.apply();

    return res;
}
