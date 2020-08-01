#include "specific-object/death-crimson.h"
#include "effect/effect-characteristics.h"
#include "floor/geometry.h"
#include "spell/process-effect.h"
#include "spell/spell-types.h"
#include "target/target-checker.h"
#include "target/target-getter.h"

/*!
 * @brief クリムゾンを発射する / Fire Crimson, evoluting gun.
 @ @param shooter_ptr 射撃を行うクリーチャー参照
 * @return キャンセルした場合 false.
 * @details
 * Need to analyze size of the window.
 * Need more color coding.
 */
bool fire_crimson(player_type *shooter_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(shooter_ptr, &dir))
        return FALSE;

    POSITION tx = shooter_ptr->x + 99 * ddx[dir];
    POSITION ty = shooter_ptr->y + 99 * ddy[dir];
    if ((dir == 5) && target_okay(shooter_ptr)) {
        tx = target_col;
        ty = target_row;
    }

    int num = 1;
    if (shooter_ptr->pclass == CLASS_ARCHER) {
        if (shooter_ptr->lev >= 10)
            num++;
        if (shooter_ptr->lev >= 30)
            num++;
        if (shooter_ptr->lev >= 45)
            num++;
    }

    BIT_FLAGS flg = PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL;
    for (int i = 0; i < num; i++)
        project(shooter_ptr, 0, shooter_ptr->lev / 20 + 1, ty, tx, shooter_ptr->lev * shooter_ptr->lev * 6 / 50, GF_ROCKET, flg, -1);

    return TRUE;
}
