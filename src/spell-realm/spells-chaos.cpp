#include "spell-realm/spells-chaos.h"
#include "core/window-redrawer.h"
#include "dungeon/quest.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "monster/monster-describer.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "player-info/class-info.h"
#include "player/player-damage.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-launcher.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 虚無招来処理 /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * Sorry, it becomes not (void)...
 */
void call_the_void(PlayerType *player_ptr)
{
    auto do_call = true;
    const auto &floor = *player_ptr->current_floor_ptr;
    for (int i = 0; i < 9; i++) {
        const Pos2D p_pos_neighbor(player_ptr->y + ddy_ddd[i], player_ptr->x + ddx_ddd[i]);
        const auto &grid = floor.get_grid(p_pos_neighbor);
        if (!grid.cave_has_flag(TerrainCharacteristics::PROJECT)) {
            if (!grid.mimic || terrains_info[grid.mimic].flags.has_not(TerrainCharacteristics::PROJECT) || !terrains_info[grid.feat].is_permanent_wall()) {
                do_call = false;
                break;
            }
        }
    }

    if (do_call) {
        for (int i = 1; i < 10; i++) {
            if (i - 5) {
                fire_ball(player_ptr, AttributeType::ROCKET, i, 175, 2);
            }
        }

        for (int i = 1; i < 10; i++) {
            if (i - 5) {
                fire_ball(player_ptr, AttributeType::MANA, i, 175, 3);
            }
        }

        for (int i = 1; i < 10; i++) {
            if (i - 5) {
                fire_ball(player_ptr, AttributeType::NUKE, i, 175, 4);
            }
        }

        return;
    }

    auto is_special_fllor = floor.is_in_quest() && QuestType::is_fixed(floor.quest_number);
    is_special_fllor |= floor.dun_level == 0;
    if (is_special_fllor) {
        msg_print(_("地面が揺れた。", "The ground trembles."));
        return;
    }

#ifdef JP
    msg_format("あなたは%sを壁に近すぎる場所で唱えてしまった！", ((mp_ptr->spell_book == ItemKindType::LIFE_BOOK) ? "祈り" : "呪文"));
#else
    msg_format("You %s the %s too close to a wall!", ((mp_ptr->spell_book == ItemKindType::LIFE_BOOK) ? "recite" : "cast"),
        ((mp_ptr->spell_book == ItemKindType::LIFE_BOOK) ? "prayer" : "spell"));
#endif
    msg_print(_("大きな爆発音があった！", "There is a loud explosion!"));

    if (one_in_(666)) {
        if (!vanish_dungeon(player_ptr)) {
            msg_print(_("ダンジョンは一瞬静まり返った。", "The dungeon becomes quiet for a moment."));
        }
        take_hit(player_ptr, DAMAGE_NOESCAPE, 100 + randint1(150), _("自殺的な虚無招来", "a suicidal Call the Void"));
        return;
    }

    if (destroy_area(player_ptr, player_ptr->y, player_ptr->x, 15 + player_ptr->lev + randint0(11), false)) {
        msg_print(_("ダンジョンが崩壊した...", "The dungeon collapses..."));
    } else {
        msg_print(_("ダンジョンは大きく揺れた。", "The dungeon trembles."));
    }
    take_hit(player_ptr, DAMAGE_NOESCAPE, 100 + randint1(150), _("自殺的な虚無招来", "a suicidal Call the Void"));
}

/*!
 * @brief 与えられたグリッドを壁から床にする
 * @param floor フロアへの参照
 * @param pos グリッドの座標
 * @todo FloorType/Grid のオブジェクトメソッドへ繰り込む
 */
static void erase_wall(FloorType &floor, const Pos2D &pos)
{
    auto &grid = floor.get_grid(pos);
    const auto &terrain = terrains_info[grid.mimic];
    grid.info &= ~(CAVE_ROOM | CAVE_ICKY);
    if ((grid.mimic == 0) || terrain.flags.has_not(TerrainCharacteristics::HURT_DISI)) {
        return;
    }

    grid.mimic = feat_state(&floor, grid.mimic, TerrainCharacteristics::HURT_DISI);
    const auto &terrain_changed = terrains_info[grid.mimic];
    if (terrain_changed.flags.has_not(TerrainCharacteristics::REMEMBER)) {
        grid.info &= ~(CAVE_MARK);
    }
}

/*!
 * @brief フロアの全てを床にする
 * @param floor フロアへの参照
 * @todo FloorType のオブジェクトメソッドへ繰り込む
 */
static void erase_all_walls(FloorType &floor)
{
    for (auto x = 0; x < floor.width; x++) {
        erase_wall(floor, { 0, x });
        erase_wall(floor, { floor.height - 1, x });
    }

    for (auto y = 1; y < (floor.height - 1); y++) {
        erase_wall(floor, { y, 0 });
        erase_wall(floor, { y, floor.width - 1 });
    }
}

/*!
 * @brief 虚無招来によるフロア中の全壁除去処理 /
 * Vanish all walls in this floor
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param player_ptr 術者の参照ポインタ
 * @return 実際に処理が反映された場合TRUE
 */
bool vanish_dungeon(PlayerType *player_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto is_special_floor = floor.is_in_quest() && QuestType::is_fixed(floor.quest_number);
    is_special_floor |= (floor.dun_level == 0);
    if (is_special_floor) {
        return false;
    }

    for (auto y = 1; y < floor.height - 1; y++) {
        for (auto x = 1; x < floor.width - 1; x++) {
            const Pos2D pos(y, x);
            auto &grid = floor.get_grid(pos);
            const auto &terrrain = terrains_info[grid.feat];
            grid.info &= ~(CAVE_ROOM | CAVE_ICKY);
            const auto &monster = floor.m_list[grid.m_idx];
            if (grid.m_idx && monster.is_asleep()) {
                (void)set_monster_csleep(player_ptr, grid.m_idx, 0);
                if (monster.ml) {
                    const auto m_name = monster_desc(player_ptr, &monster, 0);
                    msg_format(_("%s^が目を覚ました。", "%s^ wakes up."), m_name.data());
                }
            }

            if (terrrain.flags.has(TerrainCharacteristics::HURT_DISI)) {
                cave_alter_feat(player_ptr, y, x, TerrainCharacteristics::HURT_DISI);
            }
        }
    }

    erase_all_walls(floor);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::UN_VIEW,
        StatusRecalculatingFlag::UN_LITE,
        StatusRecalculatingFlag::VIEW,
        StatusRecalculatingFlag::LITE,
        StatusRecalculatingFlag::FLOW,
        StatusRecalculatingFlag::MONSTER_LITE,
        StatusRecalculatingFlag::MONSTER_STATUSES,
    };
    rfu.set_flags(flags_srf);
    rfu.set_flag(MainWindowRedrawingFlag::MAP);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    rfu.set_flags(flags_swrf);
    return true;
}

/*!
 * @brief カオス魔法「流星群」/トランプ魔法「隕石のカード」の処理としてプレイヤーを中心に隕石落下処理を10+1d10回繰り返す。
 * / Drop 10+1d10 meteor ball at random places near the player
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam ダメージ
 * @param rad 効力の半径
 * @details このファイルにいるのは、spells-trump.c と比べて行数が少なかったため。それ以上の意図はない
 */
void cast_meteor(PlayerType *player_ptr, int dam, POSITION rad)
{
    int b = 10 + randint1(10);
    for (int i = 0; i < b; i++) {
        POSITION y = 0, x = 0;
        int count;

        for (count = 0; count <= 20; count++) {
            int dy, dx, d;

            x = player_ptr->x - 8 + randint0(17);
            y = player_ptr->y - 8 + randint0(17);
            dx = (player_ptr->x > x) ? (player_ptr->x - x) : (x - player_ptr->x);
            dy = (player_ptr->y > y) ? (player_ptr->y - y) : (y - player_ptr->y);
            d = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));

            if (d >= 9) {
                continue;
            }

            auto *floor_ptr = player_ptr->current_floor_ptr;
            if (!in_bounds(floor_ptr, y, x)) {
                continue;
            }

            const auto is_projectable = projectable(player_ptr, player_ptr->y, player_ptr->x, y, x);
            if (!is_projectable || !cave_has_flag_bold(floor_ptr, y, x, TerrainCharacteristics::PROJECT)) {
                continue;
            }

            break;
        }

        if (count > 20) {
            continue;
        }

        project(player_ptr, 0, rad, y, x, dam, AttributeType::METEOR, PROJECT_KILL | PROJECT_JUMP | PROJECT_ITEM);
    }
}
