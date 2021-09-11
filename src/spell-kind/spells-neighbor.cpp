#include "spell-kind/spells-neighbor.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
#include "floor/floor-util.h"
#include "grid/feature-flag-types.h"
#include "grid/grid.h"
#include "spell-kind/earthquake.h"
#include "spell/spell-types.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief ドア生成処理(プレイヤー中心に周囲1マス) / Hooks -- affect adjacent grids (radius 1 ball attack)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 作用が実際にあった場合TRUEを返す
 */
bool door_creation(player_type *player_ptr, POSITION y, POSITION x)
{
    BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
    return project(player_ptr, 0, 1, y, x, 0, GF_MAKE_DOOR, flg).notice;
}

/*!
 * @brief トラップ生成処理(起点から周囲1マス)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 起点Y座標
 * @param x 起点X座標
 * @return 作用が実際にあった場合TRUEを返す
 */
bool trap_creation(player_type *player_ptr, POSITION y, POSITION x)
{
    BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
    return project(player_ptr, 0, 1, y, x, 0, GF_MAKE_TRAP, flg).notice;
}

/*!
 * @brief 森林生成処理(プレイヤー中心に周囲1マス)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 作用が実際にあった場合TRUEを返す
 */
bool tree_creation(player_type *player_ptr, POSITION y, POSITION x)
{
    BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
    return project(player_ptr, 0, 1, y, x, 0, GF_MAKE_TREE, flg).notice;
}

/*!
 * @brief 魔法のルーン生成処理(プレイヤー中心に周囲1マス)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 作用が実際にあった場合TRUEを返す
 */
bool create_rune_protection_area(player_type *player_ptr, POSITION y, POSITION x)
{
    BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM;
    return project(player_ptr, 0, 1, y, x, 0, GF_MAKE_RUNE_PROTECTION, flg).notice;
}

/*!
 * @brief 壁生成処理(プレイヤー中心に周囲1マス)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 作用が実際にあった場合TRUEを返す
 */
bool wall_stone(player_type *player_ptr)
{
    BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
    bool dummy = project(player_ptr, 0, 1, player_ptr->y, player_ptr->x, 0, GF_STONE_WALL, flg).notice;
    player_ptr->update |= (PU_FLOW);
    player_ptr->redraw |= (PR_MAP);
    return dummy;
}

/*!
 * @brief ドア破壊処理(プレイヤー中心に周囲1マス)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 作用が実際にあった場合TRUEを返す
 */
bool destroy_doors_touch(player_type *player_ptr)
{
    BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
    return project(player_ptr, 0, 1, player_ptr->y, player_ptr->x, 0, GF_KILL_DOOR, flg).notice;
}

/*!
 * @brief トラップ解除処理(プレイヤー中心に周囲1マス)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 作用が実際にあった場合TRUEを返す
 */
bool disarm_traps_touch(player_type *player_ptr)
{
    BIT_FLAGS flg = PROJECT_GRID | PROJECT_ITEM | PROJECT_HIDE;
    return project(player_ptr, 0, 1, player_ptr->y, player_ptr->x, 0, GF_KILL_TRAP, flg).notice;
}

/*!
 * @brief スリープモンスター処理(プレイヤー中心に周囲1マス)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 作用が実際にあった場合TRUEを返す
 */
bool sleep_monsters_touch(player_type *player_ptr)
{
    BIT_FLAGS flg = PROJECT_KILL | PROJECT_HIDE;
    return project(player_ptr, 0, 1, player_ptr->y, player_ptr->x, player_ptr->lev, GF_OLD_SLEEP, flg).notice;
}

/*!
 * @brief 死者復活処理(起点より周囲5マス)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param who 術者モンスターID(0ならばプレイヤー)
 * @param y 起点Y座標
 * @param x 起点X座標
 * @return 作用が実際にあった場合TRUEを返す
 */
bool animate_dead(player_type *player_ptr, MONSTER_IDX who, POSITION y, POSITION x)
{
    BIT_FLAGS flg = PROJECT_ITEM | PROJECT_HIDE;
    return project(player_ptr, who, 5, y, x, 0, GF_ANIM_DEAD, flg).notice;
}

/*!
 * @brief 周辺破壊効果(プレイヤー中心)
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void wall_breaker(player_type *player_ptr)
{
    POSITION y = 0, x = 0;
    int attempts = 1000;
    if (randint1(80 + player_ptr->lev) < 70) {
        while (attempts--) {
            scatter(player_ptr, &y, &x, player_ptr->y, player_ptr->x, 4, PROJECT_NONE);

            if (!cave_has_flag_bold(player_ptr->current_floor_ptr, y, x, FF::PROJECT))
                continue;

            if (!player_bold(player_ptr, y, x))
                break;
        }

        project(player_ptr, 0, 0, y, x, 20 + randint1(30), GF_KILL_WALL, (PROJECT_BEAM | PROJECT_THRU | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL));
        return;
    }

    if (randint1(100) > 30) {
        earthquake(player_ptr, player_ptr->y, player_ptr->x, 1, 0);
        return;
    }

    int num = damroll(5, 3);
    for (int i = 0; i < num; i++) {
        while (true) {
            scatter(player_ptr, &y, &x, player_ptr->y, player_ptr->x, 10, PROJECT_NONE);

            if (!player_bold(player_ptr, y, x))
                break;
        }

        project(player_ptr, 0, 0, y, x, 20 + randint1(30), GF_KILL_WALL, (PROJECT_BEAM | PROJECT_THRU | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL));
    }
}
