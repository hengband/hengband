#include "mind/racial-ninja.h"
#include "cmd-action/cmd-attack.h"
#include "combat/combat-options-type.h"
#include "floor/floor-object.h"
#include "io/targeting.h"
#include "effect/effect-characteristics.h"
#include "effect/spells-effect-util.h"
#include "object/object-generator.h"
#include "object/object-kind-hook.h"
#include "spell/spells-teleport.h"

/*!
 * @brief 変わり身処理
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param success 判定成功上の処理ならばTRUE
 * @return 作用が実際にあった場合TRUEを返す
 */
bool kawarimi(player_type *caster_ptr, bool success)
{
    object_type forge;
    object_type *q_ptr = &forge;

    if (caster_ptr->is_dead)
        return FALSE;
    if (caster_ptr->confused || caster_ptr->blind || caster_ptr->paralyzed || caster_ptr->image)
        return FALSE;
    if (randint0(200) < caster_ptr->stun)
        return FALSE;

    if (!success && one_in_(3)) {
        msg_print(_("失敗！逃げられなかった。", "Failed! You couldn't run away."));
        caster_ptr->special_defense &= ~(NINJA_KAWARIMI);
        caster_ptr->redraw |= (PR_STATUS);
        return FALSE;
    }

    POSITION y = caster_ptr->y;
    POSITION x = caster_ptr->x;

    teleport_player(caster_ptr, 10 + randint1(90), TELEPORT_SPONTANEOUS);
    object_wipe(q_ptr);
    const int SV_WOODEN_STATUE = 0;
    object_prep(q_ptr, lookup_kind(TV_STATUE, SV_WOODEN_STATUE));

    q_ptr->pval = MON_NINJA;
    (void)drop_near(caster_ptr, q_ptr, -1, y, x);

    if (success)
        msg_print(_("攻撃を受ける前に素早く身をひるがえした。", "You have turned around just before the attack hit you."));
    else
        msg_print(_("失敗！攻撃を受けてしまった。", "Failed! You are hit by the attack."));

    caster_ptr->special_defense &= ~(NINJA_KAWARIMI);
    caster_ptr->redraw |= (PR_STATUS);
    return TRUE;
}

/*!
 * @brief 入身処理 / "Rush Attack" routine for Samurai or Ninja
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param mdeath 目標モンスターが死亡したかを返す
 * @return 作用が実際にあった場合TRUEを返す /  Return value is for checking "done"
 */
bool rush_attack(player_type *attacker_ptr, bool *mdeath)
{
    if (mdeath)
        *mdeath = FALSE;

    project_length = 5;
    DIRECTION dir;
    if (!get_aim_dir(attacker_ptr, &dir))
        return FALSE;

    int tx = attacker_ptr->x + project_length * ddx[dir];
    int ty = attacker_ptr->y + project_length * ddy[dir];

    if ((dir == 5) && target_okay(attacker_ptr)) {
        tx = target_col;
        ty = target_row;
    }

    int tm_idx = 0;
    floor_type *floor_ptr = attacker_ptr->current_floor_ptr;
    if (in_bounds(floor_ptr, ty, tx))
        tm_idx = floor_ptr->grid_array[ty][tx].m_idx;

    u16b path_g[32];
    int path_n = project_path(attacker_ptr, path_g, project_length, attacker_ptr->y, attacker_ptr->x, ty, tx, PROJECT_STOP | PROJECT_KILL);
    project_length = 0;
    if (!path_n)
        return TRUE;

    ty = attacker_ptr->y;
    tx = attacker_ptr->x;
    bool tmp_mdeath = FALSE;
    bool moved = FALSE;
    for (int i = 0; i < path_n; i++) {
        monster_type *m_ptr;

        int ny = GRID_Y(path_g[i]);
        int nx = GRID_X(path_g[i]);

        if (is_cave_empty_bold(attacker_ptr, ny, nx) && player_can_enter(attacker_ptr, floor_ptr->grid_array[ny][nx].feat, 0)) {
            ty = ny;
            tx = nx;
            continue;
        }

        if (!floor_ptr->grid_array[ny][nx].m_idx) {
            if (tm_idx) {
                msg_print(_("失敗！", "Failed!"));
            } else {
                msg_print(_("ここには入身では入れない。", "You can't move to that place."));
            }

            break;
        }

        if (!player_bold(attacker_ptr, ty, tx))
            teleport_player_to(attacker_ptr, ty, tx, TELEPORT_NONMAGICAL);
        update_monster(attacker_ptr, floor_ptr->grid_array[ny][nx].m_idx, TRUE);

        m_ptr = &floor_ptr->m_list[floor_ptr->grid_array[ny][nx].m_idx];
        if (tm_idx != floor_ptr->grid_array[ny][nx].m_idx) {
#ifdef JP
            msg_format("%s%sが立ちふさがっている！", tm_idx ? "別の" : "", m_ptr->ml ? "モンスター" : "何か");
#else
            msg_format("There is %s in the way!", m_ptr->ml ? (tm_idx ? "another monster" : "a monster") : "someone");
#endif
        } else if (!player_bold(attacker_ptr, ty, tx)) {
            GAME_TEXT m_name[MAX_NLEN];
            monster_desc(attacker_ptr, m_name, m_ptr, 0);
            msg_format(_("素早く%sの懐に入り込んだ！", "You quickly jump in and attack %s!"), m_name);
        }

        if (!player_bold(attacker_ptr, ty, tx))
            teleport_player_to(attacker_ptr, ty, tx, TELEPORT_NONMAGICAL);
        moved = TRUE;
        tmp_mdeath = do_cmd_attack(attacker_ptr, ny, nx, HISSATSU_NYUSIN);

        break;
    }

    if (!moved && !player_bold(attacker_ptr, ty, tx))
        teleport_player_to(attacker_ptr, ty, tx, TELEPORT_NONMAGICAL);

    if (mdeath)
        *mdeath = tmp_mdeath;
    return TRUE;
}
