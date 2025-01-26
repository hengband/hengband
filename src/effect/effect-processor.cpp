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
#include "grid/grid.h"
#include "io/cursor.h"
#include "io/screen-util.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-util.h"
#include "pet/pet-fall-off.h"
#include "player/player-status.h"
#include "spell-class/spells-mirror-master.h"
#include "spell/range-calc.h"
#include "system/angband-system.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "timed-effect/timed-effects.h"
#include "tracking/lore-tracker.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include <array>
#include <vector>

namespace {
Pos2D decide_source_position(PlayerType *player_ptr, MONSTER_IDX src_idx, const Pos2D &pos_target, BIT_FLAGS flag)
{
    if (any_bits(flag, PROJECT_JUMP)) {
        return pos_target;
    }
    if (is_monster(src_idx)) {
        return player_ptr->current_floor_ptr->m_list[src_idx].get_position();
    }
    return player_ptr->get_position();
}
}

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
    monster_target_y = player_ptr->y;
    monster_target_x = player_ptr->x;

    ProjectResult res;
    const Pos2D pos_target(target_y, target_x);
    const auto pos_source = decide_source_position(player_ptr, src_idx, pos_target, flag);

    if (flag & (PROJECT_THRU)) {
        if (pos_source == pos_target) {
            flag &= ~(PROJECT_THRU);
        }
    }

    auto breath = false;
    auto old_hide = false;
    if (flag & (PROJECT_BREATH)) {
        breath = true;
        if (flag & PROJECT_HIDE) {
            old_hide = true;
        }
        flag |= PROJECT_HIDE;
    }

    std::vector<Pos2D> project_positions;
    if (flag & (PROJECT_BEAM)) {
        project_positions.push_back(pos_source);
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
    ProjectionPath path_g(player_ptr, (project_length ? project_length : system.get_max_range()), pos_source, pos_target, flag);
    handle_stuff(player_ptr);

    auto k = 0;
    Pos2D pos_path = pos_source;
    auto visual = false;
    auto see_s_msg = true;
    const auto is_blind = player_ptr->effects()->blindness().is_blind();
    auto &floor = *player_ptr->current_floor_ptr;
    for (const auto &pos : path_g) {
        if (flag & PROJECT_DISI) {
            if (cave_stop_disintegration(player_ptr->current_floor_ptr, pos.y, pos.x) && (rad > 0)) {
                break;
            }
        } else if (flag & PROJECT_LOS) {
            if (!cave_los_bold(&floor, pos.y, pos.x) && (rad > 0)) {
                break;
            }
        } else {
            if (!floor.has_terrain_characteristics(pos, TerrainCharacteristics::PROJECT) && (rad > 0)) {
                break;
            }
        }
        if (flag & (PROJECT_BEAM)) {
            project_positions.push_back(pos);
        }

        if (delay_factor > 0) {
            if (!is_blind && !(flag & (PROJECT_HIDE | PROJECT_FAST))) {
                if (panel_contains(pos.y, pos.x) && floor.has_los(pos)) {
                    print_bolt_pict(player_ptr, pos_path.y, pos_path.x, pos.y, pos.x, typ);
                    move_cursor_relative(pos.y, pos.x);
                    term_fresh();
                    term_xtra(TERM_XTRA_DELAY, delay_factor);
                    lite_spot(player_ptr, pos.y, pos.x);
                    term_fresh();
                    if (flag & (PROJECT_BEAM)) {
                        print_bolt_pict(player_ptr, pos.y, pos.x, pos.y, pos.x, typ);
                    }

                    visual = true;
                } else if (visual) {
                    term_xtra(TERM_XTRA_DELAY, delay_factor);
                }
            }
        }

        pos_path = pos;
        k++;
    }

    auto path_n = k;
    const auto pos_impact = pos_path;
    if (breath && !path_n) {
        breath = false;
        if (!old_hide) {
            flag &= ~(PROJECT_HIDE);
        }
    }

    // 中心からの距離と位置のペア
    std::vector<std::pair<int, Pos2D>> positions;
    for (const auto &pos : project_positions) {
        // ビームの軌道上はすべての座標が効果の中心なので距離は0固定
        positions.emplace_back(0, pos);
    }
    project_length = 0;

    /* If we found a "target", explode there */
    if (path_n <= system.get_max_range()) {
        if ((flag & (PROJECT_BEAM)) && !positions.empty()) {
            positions.pop_back();
        }

        if (breath) {
            flag &= ~(PROJECT_HIDE);
            auto breath_positions = breath_shape(player_ptr, path_g, path_n, rad, pos_source, pos_impact, typ);
            positions.insert(positions.end(), std::make_move_iterator(breath_positions.begin()), std::make_move_iterator(breath_positions.end()));
        } else {
            auto ball_positions = ball_shape(player_ptr, pos_impact, rad, typ);
            positions.insert(positions.end(), std::make_move_iterator(ball_positions.begin()), std::make_move_iterator(ball_positions.end()));
        }
    }

    if (positions.empty()) {
        return res;
    }

    if (!is_blind && !(flag & (PROJECT_HIDE)) && (delay_factor > 0)) {
        auto drawn = false;
        auto pos_total = 0;
        for (auto dist_step = 0; std::cmp_less(pos_total, positions.size()); ++dist_step) {
            for (const auto &[dist, pos] : positions) {
                if (dist != dist_step) {
                    continue;
                }
                if (panel_contains(pos.y, pos.x) && floor.has_los(pos)) {
                    drawn = true;
                    print_bolt_pict(player_ptr, pos.y, pos.x, pos.y, pos.x, typ);
                }
                pos_total++;
            }

            move_cursor_relative(pos_impact.y, pos_impact.x);
            term_fresh();
            if (visual || drawn) {
                term_xtra(TERM_XTRA_DELAY, delay_factor);
            }
        }

        if (drawn) {
            for (const auto &[_, pos] : positions) {
                if (panel_contains(pos.y, pos.x) && floor.has_los(pos)) {
                    lite_spot(player_ptr, pos.y, pos.x);
                }
            }

            move_cursor_relative(pos_impact.y, pos_impact.x);
            term_fresh();
        }
    }

    update_creature(player_ptr);

    if (flag & PROJECT_KILL) {
        see_s_msg = is_monster(src_idx) ? is_seen(player_ptr, &player_ptr->current_floor_ptr->m_list[src_idx])
                                        : (is_player(src_idx) ? true : (player_can_see_bold(player_ptr, pos_source.y, pos_source.x) && projectable(player_ptr, player_ptr->get_position(), pos_source)));
    }

    if (flag & (PROJECT_GRID)) {
        for (const auto &[dist, pos] : positions) {
            if (breath) {
                int d = dist_to_line(pos.y, pos.x, pos_source.y, pos_source.x, pos_impact.y, pos_impact.x);
                if (affect_feature(player_ptr, src_idx, d, pos.y, pos.x, dam, typ)) {
                    res.notice = true;
                }
            } else {
                if (affect_feature(player_ptr, src_idx, dist, pos.y, pos.x, dam, typ)) {
                    res.notice = true;
                }
            }
        }
    }

    update_creature(player_ptr);
    if (flag & (PROJECT_ITEM)) {
        for (const auto &[dist, pos] : positions) {
            if (breath) {
                int d = dist_to_line(pos.y, pos.x, pos_source.y, pos_source.x, pos_impact.y, pos_impact.x);
                if (affect_item(player_ptr, src_idx, d, pos.y, pos.x, dam, typ)) {
                    res.notice = true;
                }
            } else {
                if (affect_item(player_ptr, src_idx, dist, pos.y, pos.x, dam, typ)) {
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
        auto &tracker = LoreTracker::get_instance();
        for (const auto &[dist, pos] : positions) {
            int effective_dist;
            const auto &grid = floor.get_grid(pos);
            if (positions.size() <= 1) {
                auto *m_ptr = &floor.m_list[grid.m_idx];
                MonraceDefinition *ref_ptr = &m_ptr->get_monrace();
                if ((flag & PROJECT_REFLECTABLE) && grid.m_idx && ref_ptr->misc_flags.has(MonsterMiscType::REFLECTING) && (!m_ptr->is_riding() || !(flag & PROJECT_PLAYER)) && (!src_idx || path_n > 1) && !one_in_(10)) {

                    Pos2D pos_refrect(0, 0);
                    int max_attempts = 10;
                    do {
                        pos_refrect.y = pos_source.y - 1 + randint1(3);
                        pos_refrect.x = pos_source.x - 1 + randint1(3);
                        max_attempts--;
                    } while (max_attempts && in_bounds2u(player_ptr->current_floor_ptr, pos_refrect.y, pos_refrect.x) && !projectable(player_ptr, pos, pos_refrect));

                    if (max_attempts < 1) {
                        pos_refrect = pos_source;
                    }

                    if (is_seen(player_ptr, m_ptr)) {
                        sound(SOUND_REFLECT);
                        if ((m_ptr->r_idx == MonraceId::KENSHIROU) || (m_ptr->r_idx == MonraceId::RAOU)) {
                            msg_print(_("「北斗神拳奥義・二指真空把！」", "The attack bounces!"));
                        } else if (m_ptr->r_idx == MonraceId::DIO) {
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

                    project(player_ptr, grid.m_idx, 0, pos_refrect.y, pos_refrect.x, dam, typ, flag);
                    continue;
                }
            }

            /* Find the closest point in the blast */
            if (breath) {
                effective_dist = dist_to_line(pos.y, pos.x, pos_source.y, pos_source.x, pos_impact.y, pos_impact.x);
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
                else if (pos == pos_target || (flag & PROJECT_AIMED)) {
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
        for (const auto &[dist, pos] : positions) {
            int effective_dist;
            if (!player_ptr->is_located_at(pos)) {
                continue;
            }

            /* Find the closest point in the blast */
            if (breath) {
                effective_dist = dist_to_line(pos.y, pos.x, pos_source.y, pos_source.x, pos_impact.y, pos_impact.x);
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
