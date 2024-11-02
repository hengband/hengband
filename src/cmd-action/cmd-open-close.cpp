#include "cmd-action/cmd-open-close.h"
#include "action/open-close-execution.h"
#include "action/open-util.h"
#include "cmd-action/cmd-attack.h"
#include "core/disturbance.h"
#include "floor/geometry.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-requester.h"
#include "object/tval-types.h"
#include "player-base/player-class.h"
#include "player-info/samurai-data-type.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "specific-object/chest.h"
#include "status/action-setter.h"
#include "status/experience.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain-type-definition.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 箱を開ける実行処理 /
 * Attempt to open the given chest at the given location
 * @param y 箱の存在するマスのY座標
 * @param x 箱の存在するマスのX座標
 * @param o_idx 箱のオブジェクトID
 * @return 箱が開かなかった場合TRUE / Returns TRUE if repeated commands may continue
 * @details
 * Assume there is no monster blocking the destination
 */
static bool exe_open_chest(PlayerType *player_ptr, const Pos2D &pos, OBJECT_IDX o_idx)
{
    auto flag = true;
    auto more = false;
    auto *o_ptr = &player_ptr->current_floor_ptr->o_list[o_idx];
    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    if (o_ptr->pval > 0) {
        flag = false;
        int i = player_ptr->skill_dis;
        const auto effects = player_ptr->effects();
        if (effects->blindness().is_blind() || no_lite(player_ptr)) {
            i = i / 10;
        }

        if (effects->confusion().is_confused() || effects->hallucination().is_hallucinated()) {
            i = i / 10;
        }

        int j = i - o_ptr->pval;
        if (j < 2) {
            j = 2;
        }

        if (evaluate_percent(j)) {
            msg_print(_("鍵をはずした。", "You have picked the lock."));
            gain_exp(player_ptr, 1);
            flag = true;
        } else {
            more = true;
            if (flush_failure) {
                flush();
            }

            msg_print(_("鍵をはずせなかった。", "You failed to pick the lock."));
        }
    }

    if (flag) {
        Chest chest(player_ptr);
        chest.fire_trap(pos, o_idx);
        chest.open(false, pos, o_idx);
    }

    return more;
}

/*!
 * @brief 「開ける」コマンドのメインルーチン /
 * Open a closed/locked/jammed door or a closed/locked chest.
 * @details
 * Unlocking a locked door/chest is worth one experience point.
 */
void do_cmd_open(PlayerType *player_ptr)
{
    auto more = false;
    if (AngbandWorld::get_instance().is_wild_mode()) {
        return;
    }

    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU });
    if (easy_open) {
        const auto &[num_doors, pos_door] = count_dt(player_ptr, is_closed_door, false);
        const auto &[num_chests, pos_chest] = count_chests(player_ptr, false);
        if ((num_doors > 0) || (num_chests > 0)) {
            const auto pos = pos_chest == Pos2D(0, 0) ? pos_door : pos_chest;
            const auto too_many = (num_doors && num_chests) || (num_doors > 1) || (num_chests > 1);
            if (!too_many) {
                command_dir = coords_to_dir(player_ptr, pos.y, pos.x);
            }
        }
    }

    if (command_arg) {
        command_rep = command_arg - 1;
        RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::ACTION);
        command_arg = 0;
    }

    int dir;
    if (get_rep_dir(player_ptr, &dir, true)) {
        auto &floor = *player_ptr->current_floor_ptr;
        const auto pos = player_ptr->get_neighbor(dir);
        const auto &grid = floor.get_grid(pos);
        const auto o_idx = chest_check(&floor, pos, false);
        if (grid.get_terrain_mimic().flags.has_not(TerrainCharacteristics::OPEN) && !o_idx) {
            msg_print(_("そこには開けるものが見当たらない。", "You see nothing there to open."));
        } else if (grid.has_monster() && !floor.m_list[grid.m_idx].is_riding()) {
            PlayerEnergy(player_ptr).set_player_turn_energy(100);
            msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));
            do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_NONE);
        } else if (o_idx) {
            more = exe_open_chest(player_ptr, pos, o_idx);
        } else {
            more = exe_open(player_ptr, pos.y, pos.x);
        }
    }

    if (!more) {
        disturb(player_ptr, false, false);
    }
}

/*!
 * @brief 「閉じる」コマンドのメインルーチン /
 * Close an open door.
 * @details
 * Unlocking a locked door/chest is worth one experience point.
 */
void do_cmd_close(PlayerType *player_ptr)
{
    if (AngbandWorld::get_instance().is_wild_mode()) {
        return;
    }

    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU });
    if (easy_open) {
        const auto &[num_doors, pos] = count_dt(player_ptr, is_open, false);
        if (num_doors == 1) {
            command_dir = coords_to_dir(player_ptr, pos.y, pos.x);
        }
    }

    if (command_arg) {
        command_rep = command_arg - 1;
        RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::ACTION);
        command_arg = 0;
    }

    auto more = false;
    int dir;
    if (get_rep_dir(player_ptr, &dir)) {
        const auto pos = player_ptr->get_neighbor(dir);
        const auto &grid = player_ptr->current_floor_ptr->get_grid(pos);
        if (grid.get_terrain_mimic().flags.has_not(TerrainCharacteristics::CLOSE)) {
            msg_print(_("そこには閉じるものが見当たらない。", "You see nothing there to close."));
        } else if (grid.has_monster()) {
            PlayerEnergy(player_ptr).set_player_turn_energy(100);
            msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));
            do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_NONE);
        } else {
            more = exe_close(player_ptr, pos.y, pos.x);
        }
    }

    if (!more) {
        disturb(player_ptr, false, false);
    }
}

/*!
 * @brief 箱、床のトラップ解除処理双方の統合メインルーチン /
 * Disarms a trap, or chest
 */
void do_cmd_disarm(PlayerType *player_ptr)
{
    if (AngbandWorld::get_instance().is_wild_mode()) {
        return;
    }

    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU });
    if (easy_disarm) {
        const auto &[num_traps, pos_trap] = count_dt(player_ptr, is_trap, true);
        const auto &[num_chests, pos_chest] = count_chests(player_ptr, true);
        if ((num_traps > 0) || (num_chests > 0)) {
            const auto pos = pos_chest == Pos2D(0, 0) ? pos_trap : pos_chest;
            const auto too_many = (num_traps && num_chests) || (num_traps > 1) || (num_chests > 1);
            if (!too_many) {
                command_dir = coords_to_dir(player_ptr, pos.y, pos.x);
            }
        }
    }

    if (command_arg) {
        command_rep = command_arg - 1;
        RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::ACTION);
        command_arg = 0;
    }

    int dir;
    auto more = false;
    if (get_rep_dir(player_ptr, &dir, true)) {
        auto &floor = *player_ptr->current_floor_ptr;
        const auto pos = player_ptr->get_neighbor(dir);
        const auto &grid = floor.get_grid(pos);
        const auto feat = grid.get_feat_mimic();
        const auto o_idx = chest_check(&floor, pos, true);
        if (!is_trap(feat) && !o_idx) {
            msg_print(_("そこには解除するものが見当たらない。", "You see nothing there to disarm."));
        } else if (grid.has_monster() && !floor.m_list[grid.m_idx].is_riding()) {
            msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));
            do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_NONE);
        } else if (o_idx) {
            more = exe_disarm_chest(player_ptr, pos.y, pos.x, o_idx);
        } else {
            more = exe_disarm(player_ptr, pos.y, pos.x, dir);
        }
    }

    if (!more) {
        disturb(player_ptr, false, false);
    }
}

/*!
 * @brief 「打ち破る」動作コマンドのメインルーチン /
 * Bash open a door, success based on character strength
 * @details
 * <pre>
 * For a closed door, pval is positive if locked; negative if stuck.
 *
 * For an open door, pval is positive for a broken door.
 *
 * A closed door can be opened - harder if locked. Any door might be
 * bashed open (and thereby broken). Bashing a door is (potentially)
 * faster! You move into the door way. To open a stuck door, it must
 * be bashed. A closed door can be jammed (see do_cmd_spike()).
 *
 * Creatures can also open or bash doors, see elsewhere.
 * </pre>
 */
void do_cmd_bash(PlayerType *player_ptr)
{
    if (AngbandWorld::get_instance().is_wild_mode()) {
        return;
    }

    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU });
    if (command_arg) {
        command_rep = command_arg - 1;
        RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::ACTION);
        command_arg = 0;
    }

    int dir;
    auto more = false;
    if (get_rep_dir(player_ptr, &dir)) {
        const auto pos = player_ptr->get_neighbor(dir);
        const Grid &grid = player_ptr->current_floor_ptr->get_grid(pos);
        if (grid.get_terrain_mimic().flags.has_not(TerrainCharacteristics::BASH)) {
            msg_print(_("そこには体当たりするものが見当たらない。", "You see nothing there to bash."));
        } else if (grid.has_monster()) {
            PlayerEnergy(player_ptr).set_player_turn_energy(100);
            msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));
            do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_NONE);
        } else {
            more = exe_bash(player_ptr, pos.y, pos.x, dir);
        }
    }

    if (!more) {
        disturb(player_ptr, false, false);
    }
}

/*!
 * @brief 「くさびを打つ」ために必要なオブジェクトを所持しているかどうかの判定を返す /
 * Find the index of some "spikes", if possible.
 * @param ip くさびとして打てるオブジェクトのID
 * @return オブジェクトがある場合TRUEを返す
 * @details
 * <pre>
 * Let user choose a pile of spikes, perhaps?
 * </pre>
 */
static bool get_spike(PlayerType *player_ptr, INVENTORY_IDX *ip)
{
    for (INVENTORY_IDX i = 0; i < INVEN_PACK; i++) {
        auto *o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->is_valid()) {
            continue;
        }

        if (o_ptr->bi_key.tval() == ItemKindType::SPIKE) {
            *ip = i;
            return true;
        }
    }

    return false;
}

/*!
 * @brief 「くさびを打つ」動作コマンドのメインルーチン /
 * Jam a closed door with a spike
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * <pre>
 * This command may NOT be repeated
 * </pre>
 */
void do_cmd_spike(PlayerType *player_ptr)
{
    if (AngbandWorld::get_instance().is_wild_mode()) {
        return;
    }

    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU });
    DIRECTION dir;
    if (!get_rep_dir(player_ptr, &dir)) {
        return;
    }

    const auto pos = player_ptr->get_neighbor(dir);
    const auto &grid = player_ptr->current_floor_ptr->get_grid(pos);
    const auto &terrain_mimic = grid.get_terrain_mimic();
    INVENTORY_IDX i_idx;
    if (terrain_mimic.flags.has_not(TerrainCharacteristics::SPIKE)) {
        msg_print(_("そこにはくさびを打てるものが見当たらない。", "You see nothing there to spike."));
    } else if (!get_spike(player_ptr, &i_idx)) {
        msg_print(_("くさびを持っていない！", "You have no spikes!"));
    } else if (grid.has_monster()) {
        PlayerEnergy(player_ptr).set_player_turn_energy(100);
        msg_print(_("モンスターが立ちふさがっている！", "There is a monster in the way!"));
        do_cmd_attack(player_ptr, pos.y, pos.x, HISSATSU_NONE);
    } else {
        PlayerEnergy(player_ptr).set_player_turn_energy(100);
        msg_format(_("%sにくさびを打ち込んだ。", "You jam the %s with a spike."), terrain_mimic.name.data());
        cave_alter_feat(player_ptr, pos.y, pos.x, TerrainCharacteristics::SPIKE);
        vary_item(player_ptr, i_idx, -1);
    }
}
