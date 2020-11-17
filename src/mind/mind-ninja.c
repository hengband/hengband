#include "mind/mind-ninja.h"
#include "cmd-action/cmd-attack.h"
#include "cmd-item/cmd-throw.h"
#include "combat/combat-options-type.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "effect/spells-effect-util.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/floor-util.h"
#include "game-option/disturbance-options.h"
#include "grid/feature.h"
#include "inventory/inventory-slot-types.h"
#include "mind/mind-mirror-master.h"
#include "mind/mind-numbers.h"
#include "mind/mind-warrior.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-describer.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "object-enchant/trc-types.h"
#include "object/object-generator.h"
#include "object/object-kind-hook.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-fetcher.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-grid.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spell-types.h"
#include "spell/spells-status.h"
#include "status/action-setter.h"
#include "status/body-improvement.h"
#include "status/element-resistance.h"
#include "status/temporary-resistance.h"
#include "system/floor-type-definition.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

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
    object_prep(caster_ptr, q_ptr, lookup_kind(TV_STATUE, SV_WOODEN_STATUE));

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
    int path_n = projection_path(attacker_ptr, path_g, project_length, attacker_ptr->y, attacker_ptr->x, ty, tx, PROJECT_STOP | PROJECT_KILL);
    project_length = 0;
    if (!path_n)
        return TRUE;

    ty = attacker_ptr->y;
    tx = attacker_ptr->x;
    bool tmp_mdeath = FALSE;
    bool moved = FALSE;
    for (int i = 0; i < path_n; i++) {
        monster_type *m_ptr;

        int ny = get_grid_y(path_g[i]);
        int nx = get_grid_x(path_g[i]);

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

/*!
 * @brief 盗賊と忍者における不意打ち
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return なし
 */
void process_surprise_attack(player_type *attacker_ptr, player_attack_type *pa_ptr)
{
    monster_race *r_ptr = &r_info[pa_ptr->m_ptr->r_idx];
    if (!has_melee_weapon(attacker_ptr, INVEN_RARM + pa_ptr->hand) || attacker_ptr->icky_wield[pa_ptr->hand])
        return;

    int tmp = attacker_ptr->lev * 6 + (attacker_ptr->skill_stl + 10) * 4;
    if (attacker_ptr->monlite && (pa_ptr->mode != HISSATSU_NYUSIN))
        tmp /= 3;
    if (attacker_ptr->cursed & TRC_AGGRAVATE)
        tmp /= 2;
    if (r_ptr->level > (attacker_ptr->lev * attacker_ptr->lev / 20 + 10))
        tmp /= 3;
    if (monster_csleep_remaining(pa_ptr->m_ptr) && pa_ptr->m_ptr->ml) {
        /* Can't backstab creatures that we can't see, right? */
        pa_ptr->backstab = TRUE;
    } else if ((attacker_ptr->special_defense & NINJA_S_STEALTH) && (randint0(tmp) > (r_ptr->level + 20)) && pa_ptr->m_ptr->ml
        && !(r_ptr->flagsr & RFR_RES_ALL)) {
        pa_ptr->surprise_attack = TRUE;
    } else if (monster_fear_remaining(pa_ptr->m_ptr) && pa_ptr->m_ptr->ml) {
        pa_ptr->stab_fleeing = TRUE;
    }
}

void print_surprise_attack(player_attack_type *pa_ptr)
{
    if (pa_ptr->backstab)
        msg_format(_("あなたは冷酷にも眠っている無力な%sを突き刺した！", "You cruelly stab the helpless, sleeping %s!"), pa_ptr->m_name);
    else if (pa_ptr->surprise_attack)
        msg_format(_("不意を突いて%sに強烈な一撃を喰らわせた！", "You make surprise attack, and hit %s with a powerful blow!"), pa_ptr->m_name);
    else if (pa_ptr->stab_fleeing)
        msg_format(_("逃げる%sを背中から突き刺した！", "You backstab the fleeing %s!"), pa_ptr->m_name);
    else if (!pa_ptr->monk_attack)
        msg_format(_("%sを攻撃した。", "You hit %s."), pa_ptr->m_name);
}

/*!
 * @brief 盗賊と忍者における不意打ちのダメージ計算
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return なし
 */
void calc_surprise_attack_damage(player_type *attacker_ptr, player_attack_type *pa_ptr)
{
    if (pa_ptr->backstab) {
        pa_ptr->attack_damage *= (3 + (attacker_ptr->lev / 20));
        return;
    }

    if (pa_ptr->surprise_attack) {
        pa_ptr->attack_damage = pa_ptr->attack_damage * (5 + (attacker_ptr->lev * 2 / 25)) / 2;
        return;
    }

    if (pa_ptr->stab_fleeing)
        pa_ptr->attack_damage = (3 * pa_ptr->attack_damage) / 2;
}

/*!
 * @brief 速駆け処理
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return 常にTRUE
 */
bool hayagake(player_type *creature_ptr)
{
    if (creature_ptr->action == ACTION_HAYAGAKE) {
        set_action(creature_ptr, ACTION_NONE);
        creature_ptr->energy_use = 0;
        return TRUE;
    }

    grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x];
    feature_type *f_ptr = &f_info[g_ptr->feat];

    if (!has_flag(f_ptr->flags, FF_PROJECT) || (!creature_ptr->levitation && has_flag(f_ptr->flags, FF_DEEP))) {
        msg_print(_("ここでは素早く動けない。", "You cannot run in here."));
    } else {
        set_action(creature_ptr, ACTION_HAYAGAKE);
    }

    creature_ptr->energy_use = 0;
    return TRUE;
}

/*!
 * @brief 超隠密状態をセットする
 * @param set TRUEならば超隠密状態になる。
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_superstealth(player_type *creature_ptr, bool set)
{
    bool notice = FALSE;

    if (creature_ptr->is_dead)
        return FALSE;

    if (set) {
        if (!(creature_ptr->special_defense & NINJA_S_STEALTH)) {
            if (creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].info & CAVE_MNLT) {
                msg_print(_("敵の目から薄い影の中に覆い隠された。", "You are mantled in weak shadow from ordinary eyes."));
                creature_ptr->monlite = creature_ptr->old_monlite = TRUE;
            } else {
                msg_print(_("敵の目から影の中に覆い隠された！", "You are mantled in shadow from ordinary eyes!"));
                creature_ptr->monlite = creature_ptr->old_monlite = FALSE;
            }

            notice = TRUE;
            creature_ptr->special_defense |= NINJA_S_STEALTH;
        }
    } else {
        if (creature_ptr->special_defense & NINJA_S_STEALTH) {
            msg_print(_("再び敵の目にさらされるようになった。", "You are exposed to common sight once more."));
            notice = TRUE;
            creature_ptr->special_defense &= ~(NINJA_S_STEALTH);
        }
    }

    if (!notice)
        return FALSE;
    creature_ptr->redraw |= (PR_STATUS);

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    return TRUE;
}

/*!
 * @brief 忍術の発動 /
 * do_cmd_cast calls this function if the player's class is 'ninja'.
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param spell 発動する特殊技能のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
bool cast_ninja_spell(player_type *caster_ptr, mind_ninja_type spell)
{
    POSITION x = 0, y = 0;
    DIRECTION dir;
    PLAYER_LEVEL plev = caster_ptr->lev;
    switch (spell) {
    case DARKNESS_CREATION:
        (void)unlite_area(caster_ptr, 0, 3);
        break;
    case DETECT_NEAR:
        if (plev > 44)
            wiz_lite(caster_ptr, TRUE);

        detect_monsters_normal(caster_ptr, DETECT_RAD_DEFAULT);
        if (plev > 4) {
            detect_traps(caster_ptr, DETECT_RAD_DEFAULT, TRUE);
            detect_doors(caster_ptr, DETECT_RAD_DEFAULT);
            detect_stairs(caster_ptr, DETECT_RAD_DEFAULT);
        }

        if (plev > 14)
            detect_objects_normal(caster_ptr, DETECT_RAD_DEFAULT);

        break;
    case HIDE_LEAVES:
        teleport_player(caster_ptr, 10, TELEPORT_SPONTANEOUS);
        break;
    case KAWARIMI:
        if (!(caster_ptr->special_defense & NINJA_KAWARIMI)) {
            msg_print(_("敵の攻撃に対して敏感になった。", "You are now prepared to evade any attacks."));
            caster_ptr->special_defense |= NINJA_KAWARIMI;
            caster_ptr->redraw |= (PR_STATUS);
        }

        break;
    case ABSCONDING:
        teleport_player(caster_ptr, caster_ptr->lev * 5, TELEPORT_SPONTANEOUS);
        break;
    case HIT_AND_AWAY:
        if (!hit_and_away(caster_ptr))
            return FALSE;

        break;
    case BIND_MONSTER:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        (void)stasis_monster(caster_ptr, dir);
        break;
    case ANCIENT_KNOWLEDGE:
        return ident_spell(caster_ptr, FALSE, 0);
    case FLOATING:
        set_tim_levitation(caster_ptr, randint1(20) + 20, FALSE);
        break;
    case HIDE_FLAMES:
        fire_ball(caster_ptr, GF_FIRE, 0, 50 + plev, plev / 10 + 2);
        teleport_player(caster_ptr, 30, TELEPORT_SPONTANEOUS);
        set_oppose_fire(caster_ptr, (TIME_EFFECT)plev, FALSE);
        break;
    case NYUSIN:
        return rush_attack(caster_ptr, NULL);
    case SYURIKEN_SPREADING: {
        for (int i = 0; i < 8; i++) {
            OBJECT_IDX slot;

            for (slot = 0; slot < INVEN_PACK; slot++) {
                if (caster_ptr->inventory_list[slot].tval == TV_SPIKE)
                    break;
            }

            if (slot == INVEN_PACK) {
                if (!i)
                    msg_print(_("くさびを持っていない。", "You have no Iron Spikes."));
                else
                    msg_print(_("くさびがなくなった。", "You have no more Iron Spikes."));

                return FALSE;
            }

            do_cmd_throw(caster_ptr, 1, FALSE, slot);
            take_turn(caster_ptr, 100);
        }

        break;
    }
    case CHAIN_HOOK:
        (void)fetch_monster(caster_ptr);
        break;
    case SMOKE_BALL:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        fire_ball(caster_ptr, GF_OLD_CONF, dir, plev * 3, 3);
        break;
    case SWAP_POSITION:
        project_length = -1;
        if (!get_aim_dir(caster_ptr, &dir)) {
            project_length = 0;
            return FALSE;
        }

        project_length = 0;
        (void)teleport_swap(caster_ptr, dir);
        break;
    case EXPLOSION_GLYPH:
        explosive_rune(caster_ptr, caster_ptr->y, caster_ptr->x);
        break;
    case HIDE_MUD:
        (void)set_pass_wall(caster_ptr, randint1(plev / 2) + plev / 2, FALSE);
        set_oppose_acid(caster_ptr, (TIME_EFFECT)plev, FALSE);
        break;
    case HIDE_MIST:
        fire_ball(caster_ptr, GF_POIS, 0, 75 + plev * 2 / 3, plev / 5 + 2);
        fire_ball(caster_ptr, GF_HYPODYNAMIA, 0, 75 + plev * 2 / 3, plev / 5 + 2);
        fire_ball(caster_ptr, GF_CONFUSION, 0, 75 + plev * 2 / 3, plev / 5 + 2);
        teleport_player(caster_ptr, 30, TELEPORT_SPONTANEOUS);
        break;
    case PURGATORY_FLAME: {
        int num = damroll(3, 9);
        for (int k = 0; k < num; k++) {
            EFFECT_ID typ = one_in_(2) ? GF_FIRE : one_in_(3) ? GF_NETHER : GF_PLASMA;
            int attempts = 1000;
            while (attempts--) {
                scatter(caster_ptr, &y, &x, caster_ptr->y, caster_ptr->x, 4, PROJECT_NONE);
                if (!player_bold(caster_ptr, y, x))
                    break;
            }

            project(caster_ptr, 0, 0, y, x, damroll(6 + plev / 8, 10), typ, (PROJECT_BEAM | PROJECT_THRU | PROJECT_GRID | PROJECT_KILL), -1);
        }

        break;
    }
    case ALTER_EGO:
        set_multishadow(caster_ptr, 6 + randint1(6), FALSE);
        break;
    default:
        msg_print(_("なに？", "Zap?"));
        break;
    }
    return TRUE;
}
