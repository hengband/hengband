#include "realm/realm-hissatsu.h"
#include "artifact/fixed-art-types.h"
#include "cmd-action/cmd-attack.h"
#include "cmd-item/cmd-throw.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "effect/spells-effect-util.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "mind/mind-ninja.h"
#include "monster-race/race-brightness-mask.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-update.h"
#include "player-info/equipment-info.h"
#include "player/player-damage.h"
#include "player/player-move.h"
#include "player/player-realm.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell/technic-info-table.h"
#include "status/bad-status-setter.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/grid-selector.h"
#include "target/projection-path-calculator.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 剣術の各処理を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param spell_id 剣術ID
 * @param mode 処理内容 (SpellProcessType::NAME / SPELL_DESC / SpellProcessType::CAST)
 * @return SpellProcessType::NAME / SPELL_DESC 時には文字列を返す。SpellProcessType::CAST時は tl::nullopt を返す。
 */
tl::optional<std::string> do_hissatsu_spell(PlayerType *player_ptr, SPELL_IDX spell_id, SpellProcessType mode)
{
    bool cast = mode == SpellProcessType::CAST;

    PLAYER_LEVEL plev = player_ptr->lev;

    switch (spell_id) {
    case 0:
        if (cast) {
            project_length = 2;
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            project_hook(player_ptr, AttributeType::ATTACK, dir, HISSATSU_2, PROJECT_STOP | PROJECT_KILL);
        }
        break;

    case 1:
        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            const auto attack_to = [player_ptr](const Direction &dir) {
                const auto pos = player_ptr->get_neighbor(dir);
                const auto &grid = player_ptr->current_floor_ptr->get_grid(pos);

                if (grid.has_monster()) {
                    do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_NONE);
                } else {
                    msg_print(_("攻撃は空を切った。", "You attack the empty air."));
                }
            };

            attack_to(dir);
            attack_to(dir.rotated_45degree(-1));
            attack_to(dir.rotated_45degree(1));
        }
        break;

    case 2:
        if (cast) {
            if (!ThrowCommand(player_ptr).do_cmd_throw(1, true, -1)) {
                return tl::nullopt;
            }
        }
        break;

    case 3:
        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir.has_direction()) {
                return tl::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(dir);
            if (player_ptr->current_floor_ptr->get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_FIRE);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return tl::nullopt;
            }
        }
        break;

    case 4:
        if (cast) {
            detect_monsters_mind(player_ptr, DETECT_RAD_DEFAULT);
        }
        break;

    case 5:
        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir.has_direction()) {
                return tl::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(dir);
            if (player_ptr->current_floor_ptr->get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_MINEUCHI);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return tl::nullopt;
            }
        }
        break;

    case 6:
        if (cast) {
            if (player_ptr->riding) {
                msg_print(_("乗馬中には無理だ。", "You cannot do it when riding."));
                return tl::nullopt;
            }
            msg_print(_("相手の攻撃に対して身構えた。", "You prepare to counterattack."));
            player_ptr->counter = true;
        }
        break;

    case 7:
        if (cast) {
            if (player_ptr->riding) {
                msg_print(_("乗馬中には無理だ。", "You cannot do it when riding."));
                return tl::nullopt;
            }

            const auto dir = get_direction(player_ptr);
            if (!dir.has_direction()) {
                return tl::nullopt;
            }

            const auto pos_target = player_ptr->get_neighbor(dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            const auto &grid_target = floor.get_grid(pos_target);
            if (!grid_target.has_monster()) {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return tl::nullopt;
            }

            do_cmd_attack(player_ptr, pos_target.y, pos_target.x, HISSATSU_NONE);
            if (!player_can_enter(player_ptr, grid_target.feat, 0) || floor.has_trap_at(pos_target)) {
                break;
            }

            const auto pos_opposite = pos_target + dir.vec();
            const auto &grid_opposite = floor.get_grid(pos_opposite);
            if (player_can_enter(player_ptr, grid_opposite.feat, 0) && !floor.has_trap_at(pos_opposite) && !grid_opposite.m_idx) {
                msg_erase();
                (void)move_player_effect(player_ptr, pos_opposite.y, pos_opposite.x, MPE_FORGET_FLOW | MPE_HANDLE_STUFF | MPE_DONT_PICKUP);
            }
        }
        break;

    case 8:
        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir.has_direction()) {
                return tl::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            if (floor.get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_POISON);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return tl::nullopt;
            }
        }
        break;

    case 9:
        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir.has_direction()) {
                return tl::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            if (floor.get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_ZANMA);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return tl::nullopt;
            }
        }
        break;

    case 10:
        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir.has_direction()) {
                return tl::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(dir);
            auto &floor = *player_ptr->current_floor_ptr;
            const auto &grid = floor.get_grid(pos);
            if (grid.has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_NONE);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return tl::nullopt;
            }
            if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::NO_MELEE)) {
                return "";
            }
            if (grid.has_monster()) {
                auto pos_target = pos;
                auto pos_origin = pos;
                const auto m_idx = grid.m_idx;
                auto &monster = floor.m_list[m_idx];
                const auto m_name = monster_desc(player_ptr, monster, 0);
                const auto p_pos = player_ptr->get_position();
                auto pos_neighbor = pos;
                for (auto i = 0; i < 5; i++) {
                    pos_neighbor += dir.vec();
                    if (floor.is_empty_at(pos_neighbor) && (pos != p_pos)) {
                        pos_target = pos_neighbor;
                    } else {
                        break;
                    }
                }
                if (pos_target != pos_origin) {
                    msg_format(_("%sを吹き飛ばした！", "You blow %s away!"), m_name.data());
                    floor.get_grid(pos_origin).m_idx = 0;
                    floor.get_grid(pos_target).m_idx = m_idx;
                    monster.fy = pos_target.y;
                    monster.fx = pos_target.x;

                    update_monster(player_ptr, m_idx, true);
                    lite_spot(player_ptr, pos_origin);
                    lite_spot(player_ptr, pos_target);

                    if (monster.get_monrace().brightness_flags.has_any_of(ld_mask)) {
                        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::MONSTER_LITE);
                    }
                }
            }
        }
        break;

    case 11:
        if (cast) {
            if (plev > 44) {
                if (!identify_fully(player_ptr, true)) {
                    return tl::nullopt;
                }
            } else {
                if (!ident_spell(player_ptr, true)) {
                    return tl::nullopt;
                }
            }
        }
        break;

    case 12:
        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir.has_direction()) {
                return tl::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            if (floor.get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_HAGAN);
            }

            if (!floor.has_terrain_characteristics(pos, TerrainCharacteristics::HURT_ROCK)) {
                break;
            }

            /* Destroy the feature */
            cave_alter_feat(player_ptr, pos.y, pos.x, TerrainCharacteristics::HURT_ROCK);
            RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::FLOW);
        }
        break;

    case 13:
        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir.has_direction()) {
                return tl::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            if (floor.get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_COLD);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return tl::nullopt;
            }
        }
        break;

    case 14:
        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir.has_direction()) {
                return tl::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            if (floor.get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_KYUSHO);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return tl::nullopt;
            }
        }
        break;

    case 15:
        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir.has_direction()) {
                return tl::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            if (floor.get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_MAJIN);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return tl::nullopt;
            }
        }
        break;

    case 16:
        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir.has_direction()) {
                return tl::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            if (floor.get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_SUTEMI);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return tl::nullopt;
            }
            player_ptr->sutemi = true;
        }
        break;

    case 17:
        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir.has_direction()) {
                return tl::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            if (floor.get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_ELEC);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return tl::nullopt;
            }
        }
        break;

    case 18:
        if (cast) {
            if (!rush_attack(player_ptr, nullptr)) {
                return tl::nullopt;
            }
        }
        break;

    case 19:
        if (cast) {
            const auto current_cut = player_ptr->effects()->cut().current();
            short new_cut = current_cut < 300 ? current_cut + 300 : current_cut * 2;
            (void)BadStatusSetter(player_ptr).set_cut(new_cut);
            const auto &floor = *player_ptr->current_floor_ptr;
            for (const auto &d : Direction::directions_8()) {
                const auto pos = player_ptr->get_position();
                const auto pos_ddd = pos + d.vec();
                const auto &grid = floor.get_grid(pos_ddd);
                const auto &monster = floor.m_list[grid.m_idx];
                if (!grid.has_monster() || (!monster.ml && !floor.has_terrain_characteristics(pos_ddd, TerrainCharacteristics::PROJECTION))) {
                    continue;
                }

                if (monster.has_living_flag()) {
                    do_cmd_attack(player_ptr, pos_ddd.y, pos_ddd.x, HISSATSU_SEKIRYUKA);
                    continue;
                }

                const auto m_name = monster_desc(player_ptr, monster, 0);
                msg_format(_("%sには効果がない！", "%s is unharmed!"), m_name.data());
            }
        }

        break;
    case 20:
        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir.has_direction()) {
                return tl::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            if (floor.get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_QUAKE);
            } else {
                earthquake(player_ptr, player_ptr->get_position(), 10);
            }
        }
        break;

    case 21:
        if (cast) {
            int total_damage = 0, basedam, i;
            ItemEntity *o_ptr;
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }
            msg_print(_("武器を大きく振り下ろした。", "You swing your weapon downward."));
            for (i = 0; i < 2; i++) {
                int damage;

                if (!has_melee_weapon(player_ptr, INVEN_MAIN_HAND + i)) {
                    break;
                }
                o_ptr = player_ptr->inventory[INVEN_MAIN_HAND + i].get();
                basedam = o_ptr->damage_dice.floored_expected_value_multiplied_by(100);
                damage = o_ptr->to_d * 100;

                // @todo ヴォーパルの多重定義.
                if (o_ptr->is_specific_artifact(FixedArtifactId::VORPAL_BLADE) || o_ptr->is_specific_artifact(FixedArtifactId::CHAINSWORD)) {
                    /* vorpal blade */
                    basedam *= 5;
                    basedam /= 3;
                } else if (o_ptr->get_flags().has(TR_VORPAL)) {
                    /* vorpal flag only */
                    basedam *= 11;
                    basedam /= 9;
                }
                damage += basedam;
                damage *= player_ptr->num_blow[i];
                total_damage += damage / 200;
                if (i) {
                    total_damage = total_damage * 7 / 10;
                }
            }
            fire_beam(player_ptr, AttributeType::FORCE, dir, total_damage);
        }
        break;

    case 22:
        if (cast) {
            msg_print(_("雄叫びをあげた！", "You roar!"));
            project_all_los(player_ptr, AttributeType::SOUND, randint1(plev * 3));
            aggravate_monsters(player_ptr, 0);
        }
        break;

    case 23:
        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir.has_direction()) {
                return tl::nullopt;
            }

            auto &floor = *player_ptr->current_floor_ptr;
            for (auto i = 0; i < 3; i++) {
                const Pos2D pos = player_ptr->get_neighbor(dir);
                auto &grid = floor.get_grid(pos);

                if (grid.has_monster()) {
                    do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_3DAN);
                } else {
                    msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                    return tl::nullopt;
                }

                if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::NO_MELEE)) {
                    return "";
                }

                /* Monster is dead? */
                if (!grid.has_monster()) {
                    break;
                }

                const auto pos_new = pos + dir.vec();
                const auto m_idx = grid.m_idx;
                auto &monster = floor.m_list[m_idx];

                /* Monster cannot move back? */
                if (!monster_can_enter(player_ptr, pos_new.y, pos_new.x, monster.get_monrace(), 0)) {
                    /* -more- */
                    if (i < 2) {
                        msg_erase();
                    }
                    continue;
                }

                grid.m_idx = 0;
                floor.get_grid(pos_new).m_idx = m_idx;
                monster.fy = pos_new.y;
                monster.fx = pos_new.x;

                update_monster(player_ptr, m_idx, true);

                /* Redraw the old spot */
                lite_spot(player_ptr, pos);

                /* Redraw the new spot */
                lite_spot(player_ptr, pos_new);

                /* Player can move forward? */
                if (player_can_enter(player_ptr, grid.feat, 0)) {
                    if (!move_player_effect(player_ptr, pos.y, pos.x, MPE_FORGET_FLOW | MPE_HANDLE_STUFF | MPE_DONT_PICKUP)) {
                        break;
                    }
                } else {
                    break;
                }

                /* -more- */
                if (i < 2) {
                    msg_erase();
                }
            }
        }
        break;

    case 24:
        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir.has_direction()) {
                return tl::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            if (floor.get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_DRAIN);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return tl::nullopt;
            }
        }
        break;

    case 25:
        if (cast) {
            msg_print(_("武器を不規則に揺らした．．．", "You irregularly wave your weapon..."));
            project_all_los(player_ptr, AttributeType::ENGETSU, plev * 4);
        }
        break;

    case 26:
        if (cast) {
            const int mana_cost_per_monster = 8;
            bool is_new = true;
            bool mdeath;
            const auto &spell = PlayerRealm::get_spell_info(RealmType::HISSATSU, spell_id);

            do {
                if (!rush_attack(player_ptr, &mdeath)) {
                    break;
                }
                if (is_new) {
                    /* Reserve needed mana point */
                    player_ptr->csp -= spell.smana;
                    is_new = false;
                } else {
                    player_ptr->csp -= mana_cost_per_monster;
                }

                if (!mdeath) {
                    break;
                }
                command_dir = Direction::none();

                RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::MP);
                handle_stuff(player_ptr);
            } while (player_ptr->csp > mana_cost_per_monster);

            if (is_new) {
                return tl::nullopt;
            }

            /* Restore reserved mana */
            player_ptr->csp += spell.smana;
        }
        break;

    case 27:
        if (cast) {
            const auto pos = point_target(player_ptr);
            if (!pos) {
                return tl::nullopt;
            }

            const auto p_pos = player_ptr->get_position();
            const auto is_teleportable = cave_player_teleportable_bold(player_ptr, pos->y, pos->x, TELEPORT_SPONTANEOUS);
            const auto dist = Grid::calc_distance(*pos, p_pos);
            if (!is_teleportable || (dist > MAX_PLAYER_SIGHT / 2) || !projectable(*player_ptr->current_floor_ptr, p_pos, *pos)) {
                msg_print(_("失敗！", "You cannot move to that place!"));
                break;
            }
            if (player_ptr->anti_tele) {
                msg_print(_("不思議な力がテレポートを防いだ！", "A mysterious force prevents you from teleporting!"));
                break;
            }
            project(player_ptr, 0, 0, pos->y, pos->x, HISSATSU_ISSEN, AttributeType::ATTACK, PROJECT_BEAM | PROJECT_KILL);
            teleport_player_to(player_ptr, pos->y, pos->x, TELEPORT_SPONTANEOUS);
        }
        break;

    case 28:
        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(dir);
            const auto &grid = player_ptr->current_floor_ptr->get_grid(pos);
            if (grid.has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_NONE);
                if (grid.has_monster()) {
                    handle_stuff(player_ptr);
                    do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_NONE);
                }
            } else {
                msg_print(_("その方向にはモンスターはいません。", "You don't see any monster in this direction"));
                return tl::nullopt;
            }
        }
        break;

    case 29:
        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir.has_direction()) {
                return tl::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::NO_MELEE)) {
                msg_print(_("なぜか攻撃することができない。", "Something prevents you from attacking."));
                return "";
            }

            msg_print(_("武器を大きく振り下ろした。", "You swing your weapon downward."));
            auto total_damage = 0;
            for (auto i = 0; i < 2; i++) {
                if (!has_melee_weapon(player_ptr, INVEN_MAIN_HAND + i)) {
                    break;
                }

                const auto &item = *player_ptr->inventory[INVEN_MAIN_HAND + i];
                auto basedam = item.damage_dice.floored_expected_value_multiplied_by(100);
                auto damage = item.to_d * 100;

                // @todo ヴォーパルの多重定義.
                if (item.is_specific_artifact(FixedArtifactId::VORPAL_BLADE) || item.is_specific_artifact(FixedArtifactId::CHAINSWORD)) {
                    /* vorpal blade */
                    basedam *= 5;
                    basedam /= 3;
                } else if (item.get_flags().has(TR_VORPAL)) {
                    /* vorpal flag only */
                    basedam *= 11;
                    basedam /= 9;
                }
                damage += basedam;
                damage += player_ptr->to_d[i] * 100;
                damage *= player_ptr->num_blow[i];
                total_damage += (damage / 100);
            }

            const auto is_bold = floor.has_terrain_characteristics(pos, TerrainCharacteristics::PROJECTION);
            constexpr auto flags = PROJECT_KILL | PROJECT_JUMP | PROJECT_ITEM;
            project(player_ptr, 0, (is_bold ? 5 : 0), pos.y, pos.x, total_damage * 3 / 2, AttributeType::METEOR, flags);
        }
        break;

    case 30:
        if (cast) {
            const auto dir = get_direction(player_ptr);
            if (!dir.has_direction()) {
                return tl::nullopt;
            }

            const auto pos = player_ptr->get_neighbor(dir);
            const auto &floor = *player_ptr->current_floor_ptr;
            if (floor.get_grid(pos).has_monster()) {
                do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_UNDEAD);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return tl::nullopt;
            }
            take_hit(player_ptr, DAMAGE_NOESCAPE, 100 + randint1(100), _("慶雲鬼忍剣を使った衝撃", "exhaustion on using Keiun-Kininken"));
        }
        break;

    case 31:
        if (cast) {
            int i;
            if (!input_check(_("本当に自殺しますか？", "Do you really want to commit suicide? "))) {
                return tl::nullopt;
            }
            /* Special Verification for suicide */
            prt(_("確認のため '@' を押して下さい。", "Please verify SUICIDE by typing the '@' sign: "), 0, 0);

            flush();
            i = inkey();
            prt("", 0, 0);
            if (i != '@') {
                return tl::nullopt;
            }

            auto &world = AngbandWorld::get_instance();
            if (world.total_winner) {
                take_hit(player_ptr, DAMAGE_FORCE, 9999, "Seppuku");
                world.total_winner = true;
            } else {
                msg_print(_("武士道とは、死ぬことと見つけたり。", "The meaning of bushido is found in death."));
                take_hit(player_ptr, DAMAGE_FORCE, 9999, "Seppuku");
            }
        }
        break;
    }

    return "";
}
