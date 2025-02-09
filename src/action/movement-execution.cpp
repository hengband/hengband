/*!
 * @file movement-execution.cpp
 * @brief プレイヤーの歩行勝利実行定義
 */

#include "action/movement-execution.h"
#include "action/open-close-execution.h"
#include "artifact/fixed-art-types.h"
#include "cmd-action/cmd-attack.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "floor/geometry.h"
#include "floor/pattern-walk.h"
#include "game-option/input-options.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "locale/english.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-resistance-mask.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "mutation/mutation-flag-types.h"
#include "object/warning.h"
#include "player-base/player-class.h"
#include "player-status/player-energy.h"
#include "player/player-move.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor/floor-info.h"
#include "system/floor/wilderness-grid.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "timed-effect/timed-effects.h"
#include "tracking/lore-tracker.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * Determine if a "boundary" grid is "floor mimic"
 * @param grid グリッドへの参照
 * @param terrain 地形特性への参照
 * @param terrain_mimic ミミック地形特性への参照
 * @return 移動不能であればTRUE
 * @todo 負論理なので反転させたい
 */
static bool boundary_floor(const Grid &grid, const TerrainType &terrain, const TerrainType &terrain_mimic)
{
    auto is_boundary_floor = grid.mimic > 0;
    is_boundary_floor &= terrain.is_permanent_wall();
    is_boundary_floor &= terrain_mimic.flags.has_any_of({ TerrainCharacteristics::MOVE, TerrainCharacteristics::CAN_FLY });
    is_boundary_floor &= terrain_mimic.flags.has(TerrainCharacteristics::PROJECT);
    is_boundary_floor &= terrain_mimic.flags.has_not(TerrainCharacteristics::OPEN);
    return is_boundary_floor;
}

/*!
 * @brief 該当地形のトラップがプレイヤーにとって無効かどうかを判定して返す /
 * Move player in the given direction, with the given "pickup" flag.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 移動方向ID
 * @param do_pickup 罠解除を試みながらの移動ならばTRUE
 * @param break_trap トラップ粉砕処理を行うならばTRUE
 * @return 実際に移動が行われたならばTRUEを返す。
 * @note
 * This routine should (probably) always induce energy expenditure.\n
 * @details
 * Note that moving will *always* take a turn, and will *always* hit\n
 * any monster which might be in the destination grid.  Previously,\n
 * moving into walls was "free" and did NOT hit invisible monsters.\n
 */
void exe_movement(PlayerType *player_ptr, DIRECTION dir, bool do_pickup, bool break_trap)
{
    const auto pos = player_ptr->get_neighbor(dir);
    auto &floor = *player_ptr->current_floor_ptr;
    auto &grid = floor.get_grid(pos);
    bool p_can_enter = player_can_enter(player_ptr, grid.feat, CEM_P_CAN_ENTER_PATTERN);
    const auto &world = AngbandWorld::get_instance();
    if (!floor.is_underground() && !world.is_wild_mode() && ((pos.x == 0) || (pos.x == MAX_WID - 1) || (pos.y == 0) || (pos.y == MAX_HGT - 1))) {
        if (grid.mimic && player_can_enter(player_ptr, grid.mimic, 0)) {
            auto &wilderness = WildernessGrids::get_instance();
            if ((pos.y == 0) && (pos.x == 0)) {
                wilderness.move_player_to(Direction(7));
                player_ptr->oldpy = floor.height - 2;
                player_ptr->oldpx = floor.width - 2;
                player_ptr->ambush_flag = false;
            } else if ((pos.y == 0) && (pos.x == MAX_WID - 1)) {
                wilderness.move_player_to(Direction(9));
                player_ptr->oldpy = floor.height - 2;
                player_ptr->oldpx = 1;
                player_ptr->ambush_flag = false;
            } else if ((pos.y == MAX_HGT - 1) && (pos.x == 0)) {
                wilderness.move_player_to(Direction(1));
                player_ptr->oldpy = 1;
                player_ptr->oldpx = floor.width - 2;
                player_ptr->ambush_flag = false;
            } else if ((pos.y == MAX_HGT - 1) && (pos.x == MAX_WID - 1)) {
                wilderness.move_player_to(Direction(3));
                player_ptr->oldpy = 1;
                player_ptr->oldpx = 1;
                player_ptr->ambush_flag = false;
            } else if (pos.y == 0) {
                wilderness.move_player_to(Direction(8));
                player_ptr->oldpy = floor.height - 2;
                player_ptr->oldpx = pos.x;
                player_ptr->ambush_flag = false;
            } else if (pos.y == MAX_HGT - 1) {
                wilderness.move_player_to(Direction(2));
                player_ptr->oldpy = 1;
                player_ptr->oldpx = pos.x;
                player_ptr->ambush_flag = false;
            } else if (pos.x == 0) {
                wilderness.move_player_to(Direction(4));
                player_ptr->oldpx = floor.width - 2;
                player_ptr->oldpy = pos.y;
                player_ptr->ambush_flag = false;
            } else if (pos.x == MAX_WID - 1) {
                wilderness.move_player_to(Direction(6));
                player_ptr->oldpx = 1;
                player_ptr->oldpy = pos.y;
                player_ptr->ambush_flag = false;
            }

            player_ptr->leaving = true;
            PlayerEnergy(player_ptr).set_player_turn_energy(100);
            return;
        }

        p_can_enter = false;
    }

    const auto &monster = floor.m_list[grid.m_idx];

    // @todo 「特定の武器を装備している」旨のメソッドを別途作る
    constexpr auto stormbringer = FixedArtifactId::STORMBRINGER;
    auto is_stormbringer = false;
    if (player_ptr->inventory_list[INVEN_MAIN_HAND].is_specific_artifact(stormbringer)) {
        is_stormbringer = true;
    }

    if (player_ptr->inventory_list[INVEN_SUB_HAND].is_specific_artifact(stormbringer)) {
        is_stormbringer = true;
    }

    auto &terrain = grid.get_terrain();
    auto p_can_kill_walls = has_kill_wall(player_ptr);
    p_can_kill_walls &= terrain.flags.has(TerrainCharacteristics::HURT_DISI);
    p_can_kill_walls &= !p_can_enter || terrain.flags.has_not(TerrainCharacteristics::LOS);
    p_can_kill_walls &= terrain.flags.has_not(TerrainCharacteristics::PERMANENT);
    std::string m_name;
    bool can_move = true;
    bool do_past = false;
    if (grid.has_monster() && (monster.ml || p_can_enter || p_can_kill_walls)) {
        const auto &monrace = monster.get_monrace();
        const auto effects = player_ptr->effects();
        const auto is_stunned = effects->stun().is_stunned();
        auto can_cast = !effects->confusion().is_confused();
        const auto is_hallucinated = effects->hallucination().is_hallucinated();
        can_cast &= !is_hallucinated;
        can_cast &= monster.ml;
        can_cast &= !is_stunned;
        can_cast &= player_ptr->muta.has_not(PlayerMutationType::BERS_RAGE) || !is_shero(player_ptr);
        if (!monster.is_hostile() && can_cast && pattern_seq(player_ptr, pos) && (p_can_enter || p_can_kill_walls)) {
            (void)set_monster_csleep(player_ptr, grid.m_idx, 0);
            m_name = monster_desc(player_ptr, monster, 0);
            if (monster.ml) {
                if (!is_hallucinated) {
                    LoreTracker::get_instance().set_trackee(monster.ap_r_idx);
                }

                health_track(player_ptr, grid.m_idx);
            }

            if ((is_stormbringer && (randint1(1000) > 666)) || PlayerClass(player_ptr).equals(PlayerClassType::BERSERKER)) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_NONE);
                can_move = false;
            } else if (monster_can_cross_terrain(player_ptr, floor.get_grid(player_ptr->get_position()).feat, monrace, 0)) {
                do_past = true;
            } else {
                msg_format(_("%s^が邪魔だ！", "%s^ is in your way!"), m_name.data());
                PlayerEnergy(player_ptr).reset_player_turn();
                can_move = false;
            }
        } else {
            do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_NONE);
            can_move = false;
        }
    }

    const auto &riding_monster = floor.m_list[player_ptr->riding];
    const auto &riding_monrace = riding_monster.get_monrace();
    PlayerEnergy energy(player_ptr);
    if (can_move && player_ptr->riding) {
        if (riding_monrace.behavior_flags.has(MonsterBehaviorType::NEVER_MOVE)) {
            msg_print(_("動けない！", "Can't move!"));
            energy.reset_player_turn();
            can_move = false;
            disturb(player_ptr, false, true);
        } else if (riding_monster.is_fearful()) {
            const auto steed_name = monster_desc(player_ptr, riding_monster, 0);
            msg_format(_("%sが恐怖していて制御できない。", "%s^ is too scared to control."), steed_name.data());
            can_move = false;
            disturb(player_ptr, false, true);
        } else if (player_ptr->riding_ryoute) {
            can_move = false;
            disturb(player_ptr, false, true);
        } else if (terrain.flags.has(TerrainCharacteristics::CAN_FLY) && (riding_monrace.feature_flags.has(MonsterFeatureType::CAN_FLY))) {
            /* Allow moving */
        } else if (terrain.flags.has(TerrainCharacteristics::CAN_SWIM) && (riding_monrace.feature_flags.has(MonsterFeatureType::CAN_SWIM))) {
            /* Allow moving */
        } else if (terrain.flags.has(TerrainCharacteristics::WATER) && riding_monrace.feature_flags.has_not(MonsterFeatureType::AQUATIC) && (terrain.flags.has(TerrainCharacteristics::DEEP) || riding_monrace.aura_flags.has(MonsterAuraType::FIRE))) {
            msg_print(_(format("%sの上に行けない。", grid.get_terrain(TerrainKind::MIMIC).name.data()), "Can't swim."));
            energy.reset_player_turn();
            can_move = false;
            disturb(player_ptr, false, true);
        } else if (terrain.flags.has_not(TerrainCharacteristics::WATER) && riding_monrace.feature_flags.has(MonsterFeatureType::AQUATIC)) {
            constexpr auto fmt = _("%sから上がれない。", "Can't land from %s.");
            const auto p_pos = player_ptr->get_position();
            msg_format(fmt, floor.get_grid(p_pos).get_terrain(TerrainKind::MIMIC).name.data());
            energy.reset_player_turn();
            can_move = false;
            disturb(player_ptr, false, true);
        } else if (terrain.flags.has(TerrainCharacteristics::LAVA) && riding_monrace.resistance_flags.has_none_of(RFR_EFF_IM_FIRE_MASK)) {
            msg_print(_(format("%sの上に行けない。", grid.get_terrain(TerrainKind::MIMIC).name.data()), "Too hot to go through."));
            energy.reset_player_turn();
            can_move = false;
            disturb(player_ptr, false, true);
        }

        if (can_move && riding_monster.is_stunned() && one_in_(2)) {
            const auto steed_name = monster_desc(player_ptr, riding_monster, 0);
            msg_format(_("%sが朦朧としていてうまく動けない！", "You cannot control stunned %s!"), steed_name.data());
            can_move = false;
            disturb(player_ptr, false, true);
        }
    }

    if (!can_move) {
    } else if (terrain.flags.has_not(TerrainCharacteristics::MOVE) && terrain.flags.has(TerrainCharacteristics::CAN_FLY) && !player_ptr->levitation) {
        msg_format(_("空を飛ばないと%sの上には行けない。", "You need to fly to go through the %s."), grid.get_terrain(TerrainKind::MIMIC).name.data());
        energy.reset_player_turn();
        player_ptr->running = 0;
        can_move = false;
    } else if (terrain.flags.has(TerrainCharacteristics::TREE) && !p_can_kill_walls) {
        const auto riding_wild_wood = player_ptr->riding && riding_monrace.wilderness_flags.has(MonsterWildernessType::WILD_WOOD);
        if (!PlayerClass(player_ptr).equals(PlayerClassType::RANGER) && !player_ptr->levitation && !riding_wild_wood) {
            energy.mul_player_turn_energy(2);
        }
    } else if ((do_pickup != easy_disarm) && terrain.flags.has(TerrainCharacteristics::DISARM) && !grid.mimic) {
        if (!trap_can_be_ignored(player_ptr, grid.feat)) {
            (void)exe_disarm(player_ptr, pos.y, pos.x, dir);
            return;
        }
    } else if (!p_can_enter && !p_can_kill_walls) {
        const auto &terrain_mimic = grid.get_terrain(TerrainKind::MIMIC);
        const auto &name = terrain_mimic.name;
        can_move = false;
        if (!grid.is_mark() && !player_can_see_bold(player_ptr, pos.y, pos.x)) {
            if (boundary_floor(grid, terrain, terrain_mimic)) {
                msg_print(_("それ以上先には進めないようだ。", "You feel you cannot go any more."));
            } else {
#ifdef JP
                msg_format("%sが行く手をはばんでいるようだ。", name.data());
#else
                msg_format("You feel %s %s blocking your way.", is_a_vowel(name[0]) ? "an" : "a", name.data());
#endif
                grid.info |= (CAVE_MARK);
                lite_spot(player_ptr, pos.y, pos.x);
            }
        } else {
            const auto effects = player_ptr->effects();
            const auto is_confused = effects->confusion().is_confused();
            const auto is_stunned = effects->stun().is_stunned();
            const auto is_hallucinated = effects->hallucination().is_hallucinated();
            if (boundary_floor(grid, terrain, terrain_mimic)) {
                msg_print(_("それ以上先には進めない。", "You cannot go any more."));
                if (!(is_confused || is_stunned || is_hallucinated)) {
                    energy.reset_player_turn();
                }
            } else {
                if (easy_open && floor.has_closed_door_at(pos, true) && easy_open_door(player_ptr, pos.y, pos.x)) {
                    return;
                }

#ifdef JP
                msg_format("%sが行く手をはばんでいる。", name.data());
#else
                msg_format("There is %s %s blocking your way.", is_a_vowel(name[0]) ? "an" : "a", name.data());
#endif
                if (!(is_confused || is_stunned || is_hallucinated)) {
                    energy.reset_player_turn();
                }
            }
        }

        disturb(player_ptr, false, true);
        if (!boundary_floor(grid, terrain, terrain_mimic)) {
            sound(SOUND_HITWALL);
        }
    }

    if (can_move && !pattern_seq(player_ptr, pos)) {
        const auto effects = player_ptr->effects();
        const auto is_confused = effects->confusion().is_confused();
        const auto is_stunned = effects->stun().is_stunned();
        const auto is_hallucinated = effects->hallucination().is_hallucinated();
        if (!(is_confused || is_stunned || is_hallucinated)) {
            energy.reset_player_turn();
        }

        disturb(player_ptr, false, true);
        can_move = false;
    }

    if (!can_move) {
        return;
    }

    if (player_ptr->warning && (!process_warning(player_ptr, pos.x, pos.y))) {
        energy.set_player_turn_energy(25);
        return;
    }

    if (do_past) {
        msg_format(_("%sを押し退けた。", "You push past %s."), m_name.data());
    }

    if (world.is_wild_mode()) {
        const auto vec = Direction(dir).vec();
        if (vec.y > 0) {
            player_ptr->oldpy = 1;
        }
        if (vec.y < 0) {
            player_ptr->oldpy = MAX_HGT - 2;
        }
        if (vec.y == 0) {
            player_ptr->oldpy = MAX_HGT / 2;
        }
        if (vec.x > 0) {
            player_ptr->oldpx = 1;
        }
        if (vec.x < 0) {
            player_ptr->oldpx = MAX_WID - 2;
        }
        if (vec.x == 0) {
            player_ptr->oldpx = MAX_WID / 2;
        }
    }

    if (p_can_kill_walls) {
        cave_alter_feat(player_ptr, pos.y, pos.x, TerrainCharacteristics::HURT_DISI);
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::FLOW);
    }

    uint32_t mpe_mode = MPE_ENERGY_USE;
    if (do_pickup != always_pickup) {
        mpe_mode |= MPE_DO_PICKUP;
    }

    if (break_trap) {
        mpe_mode |= MPE_BREAK_TRAP;
    }

    (void)move_player_effect(player_ptr, pos.y, pos.x, mpe_mode);
}
