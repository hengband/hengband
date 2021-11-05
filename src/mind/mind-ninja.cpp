﻿#include "mind/mind-ninja.h"
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
#include "floor/geometry.h"
#include "game-option/disturbance-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
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
#include "object/object-kind-hook.h"
#include "player-attack/player-attack-util.h"
#include "player-base/player-class.h"
#include "player-info/equipment-info.h"
#include "player-info/ninja-data-type.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/player-status-flags.h"
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
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 変わり身処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param success 判定成功上の処理ならばTRUE
 * @return 作用が実際にあった場合TRUEを返す
 */
bool kawarimi(PlayerType *player_ptr, bool success)
{
    auto ninja_data = PlayerClass(player_ptr).get_specific_data<ninja_data_type>();
    if (!ninja_data || !ninja_data->kawarimi) {
        return false;
    }

    object_type forge;
    object_type *q_ptr = &forge;
    if (player_ptr->is_dead) {
        return false;
    }

    auto effects = player_ptr->effects();
    if (player_ptr->confused || player_ptr->blind || player_ptr->paralyzed || player_ptr->hallucinated) {
        return false;
    }

    if (effects->stun()->current() > randint0(200)) {
        return false;
    }

    if (!success && one_in_(3)) {
        msg_print(_("変わり身失敗！逃げられなかった。", "Kawarimi failed! You couldn't run away."));
        ninja_data->kawarimi = false;
        player_ptr->redraw |= (PR_STATUS);
        return false;
    }

    POSITION y = player_ptr->y;
    POSITION x = player_ptr->x;

    teleport_player(player_ptr, 10 + randint1(90), TELEPORT_SPONTANEOUS);
    q_ptr->wipe();
    const int SV_WOODEN_STATUE = 0;
    q_ptr->prep(lookup_kind(ItemKindType::STATUE, SV_WOODEN_STATUE));

    q_ptr->pval = MON_NINJA;
    (void)drop_near(player_ptr, q_ptr, -1, y, x);

    if (success)
        msg_print(_("攻撃を受ける前に素早く身をひるがえした。", "You have turned around just before the attack hit you."));
    else
        msg_print(_("変わり身失敗！攻撃を受けてしまった。", "Kawarimi failed! You are hit by the attack."));

    ninja_data->kawarimi = false;
    player_ptr->redraw |= (PR_STATUS);
    return true;
}

/*!
 * @brief 入身処理 / "Rush Attack" routine for Samurai or Ninja
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param mdeath 目標モンスターが死亡したかを返す
 * @return 作用が実際にあった場合TRUEを返す /  Return value is for checking "done"
 */
bool rush_attack(PlayerType *player_ptr, bool *mdeath)
{
    if (mdeath)
        *mdeath = false;

    project_length = 5;
    DIRECTION dir;
    if (!get_aim_dir(player_ptr, &dir))
        return false;

    int tx = player_ptr->x + project_length * ddx[dir];
    int ty = player_ptr->y + project_length * ddy[dir];

    if ((dir == 5) && target_okay(player_ptr)) {
        tx = target_col;
        ty = target_row;
    }

    int tm_idx = 0;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (in_bounds(floor_ptr, ty, tx))
        tm_idx = floor_ptr->grid_array[ty][tx].m_idx;

    uint16_t path_g[32];
    int path_n = projection_path(player_ptr, path_g, project_length, player_ptr->y, player_ptr->x, ty, tx, PROJECT_STOP | PROJECT_KILL);
    project_length = 0;
    if (!path_n)
        return true;

    ty = player_ptr->y;
    tx = player_ptr->x;
    bool tmp_mdeath = false;
    bool moved = false;
    for (int i = 0; i < path_n; i++) {
        monster_type *m_ptr;

        int ny = get_grid_y(path_g[i]);
        int nx = get_grid_x(path_g[i]);

        if (is_cave_empty_bold(player_ptr, ny, nx) && player_can_enter(player_ptr, floor_ptr->grid_array[ny][nx].feat, 0)) {
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

        if (!player_bold(player_ptr, ty, tx))
            teleport_player_to(player_ptr, ty, tx, TELEPORT_NONMAGICAL);
        update_monster(player_ptr, floor_ptr->grid_array[ny][nx].m_idx, true);

        m_ptr = &floor_ptr->m_list[floor_ptr->grid_array[ny][nx].m_idx];
        if (tm_idx != floor_ptr->grid_array[ny][nx].m_idx) {
#ifdef JP
            msg_format("%s%sが立ちふさがっている！", tm_idx ? "別の" : "", m_ptr->ml ? "モンスター" : "何か");
#else
            msg_format("There is %s in the way!", m_ptr->ml ? (tm_idx ? "another monster" : "a monster") : "someone");
#endif
        } else if (!player_bold(player_ptr, ty, tx)) {
            GAME_TEXT m_name[MAX_NLEN];
            monster_desc(player_ptr, m_name, m_ptr, 0);
            msg_format(_("素早く%sの懐に入り込んだ！", "You quickly jump in and attack %s!"), m_name);
        }

        if (!player_bold(player_ptr, ty, tx))
            teleport_player_to(player_ptr, ty, tx, TELEPORT_NONMAGICAL);
        moved = true;
        tmp_mdeath = do_cmd_attack(player_ptr, ny, nx, HISSATSU_NYUSIN);

        break;
    }

    if (!moved && !player_bold(player_ptr, ty, tx))
        teleport_player_to(player_ptr, ty, tx, TELEPORT_NONMAGICAL);

    if (mdeath)
        *mdeath = tmp_mdeath;
    return true;
}

/*!
 * @brief 盗賊と忍者における不意打ち
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 */
void process_surprise_attack(PlayerType *player_ptr, player_attack_type *pa_ptr)
{
    monster_race *r_ptr = &r_info[pa_ptr->m_ptr->r_idx];
    if (!has_melee_weapon(player_ptr, INVEN_MAIN_HAND + pa_ptr->hand) || player_ptr->is_icky_wield[pa_ptr->hand])
        return;

    int tmp = player_ptr->lev * 6 + (player_ptr->skill_stl + 10) * 4;
    if (player_ptr->monlite && (pa_ptr->mode != HISSATSU_NYUSIN))
        tmp /= 3;
    if (has_aggravate(player_ptr))
        tmp /= 2;
    if (r_ptr->level > (player_ptr->lev * player_ptr->lev / 20 + 10))
        tmp /= 3;

    auto ninja_data = PlayerClass(player_ptr).get_specific_data<ninja_data_type>();
    if (monster_csleep_remaining(pa_ptr->m_ptr) && pa_ptr->m_ptr->ml) {
        /* Can't backstab creatures that we can't see, right? */
        pa_ptr->backstab = true;
    } else if ((ninja_data && ninja_data->s_stealth) && (randint0(tmp) > (r_ptr->level + 20)) &&
               pa_ptr->m_ptr->ml && !(r_ptr->flagsr & RFR_RES_ALL)) {
        pa_ptr->surprise_attack = true;
    } else if (monster_fear_remaining(pa_ptr->m_ptr) && pa_ptr->m_ptr->ml) {
        pa_ptr->stab_fleeing = true;
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
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 */
void calc_surprise_attack_damage(PlayerType *player_ptr, player_attack_type *pa_ptr)
{
    if (pa_ptr->backstab) {
        pa_ptr->attack_damage *= (3 + (player_ptr->lev / 20));
        return;
    }

    if (pa_ptr->surprise_attack) {
        pa_ptr->attack_damage = pa_ptr->attack_damage * (5 + (player_ptr->lev * 2 / 25)) / 2;
        return;
    }

    if (pa_ptr->stab_fleeing)
        pa_ptr->attack_damage = (3 * pa_ptr->attack_damage) / 2;
}

/*!
 * @brief 速駆け処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 常にTRUE
 */
bool hayagake(PlayerType *player_ptr)
{
    PlayerEnergy energy(player_ptr);
    if (player_ptr->action == ACTION_HAYAGAKE) {
        set_action(player_ptr, ACTION_NONE);
        energy.reset_player_turn();
        return true;
    }

    grid_type *g_ptr = &player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x];
    feature_type *f_ptr = &f_info[g_ptr->feat];

    if (f_ptr->flags.has_not(FF::PROJECT) || (!player_ptr->levitation && f_ptr->flags.has(FF::DEEP))) {
        msg_print(_("ここでは素早く動けない。", "You cannot run in here."));
    } else {
        set_action(player_ptr, ACTION_HAYAGAKE);
    }

    energy.reset_player_turn();
    return true;
}

/*!
 * @brief 超隠密状態をセットする
 * @param set TRUEならば超隠密状態になる。
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_superstealth(PlayerType *player_ptr, bool set)
{
    bool notice = false;

    auto ninja_data = PlayerClass(player_ptr).get_specific_data<ninja_data_type>();
    if (!ninja_data || player_ptr->is_dead)
        return false;

    if (set) {
        if (!ninja_data->s_stealth) {
            if (player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x].info & CAVE_MNLT) {
                msg_print(_("敵の目から薄い影の中に覆い隠された。", "You are mantled in weak shadow from ordinary eyes."));
                player_ptr->monlite = player_ptr->old_monlite = true;
            } else {
                msg_print(_("敵の目から影の中に覆い隠された！", "You are mantled in shadow from ordinary eyes!"));
                player_ptr->monlite = player_ptr->old_monlite = false;
            }

            notice = true;
            ninja_data->s_stealth = true;
        }
    } else {
        if (ninja_data->s_stealth) {
            msg_print(_("再び敵の目にさらされるようになった。", "You are exposed to common sight once more."));
            notice = true;
            ninja_data->s_stealth = false;
        }
    }

    if (!notice)
        return false;
    player_ptr->redraw |= (PR_STATUS);

    if (disturb_state)
        disturb(player_ptr, false, false);
    return true;
}

/*!
 * @brief 忍術の発動 /
 * do_cmd_cast calls this function if the player's class is 'ninja'.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param spell 発動する特殊技能のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
bool cast_ninja_spell(PlayerType *player_ptr, mind_ninja_type spell)
{
    POSITION x = 0, y = 0;
    DIRECTION dir;
    PLAYER_LEVEL plev = player_ptr->lev;
    auto ninja_data = PlayerClass(player_ptr).get_specific_data<ninja_data_type>();
    switch (spell) {
    case DARKNESS_CREATION:
        (void)unlite_area(player_ptr, 0, 3);
        break;
    case DETECT_NEAR:
        if (plev > 44)
            wiz_lite(player_ptr, true);

        detect_monsters_normal(player_ptr, DETECT_RAD_DEFAULT);
        if (plev > 4) {
            detect_traps(player_ptr, DETECT_RAD_DEFAULT, true);
            detect_doors(player_ptr, DETECT_RAD_DEFAULT);
            detect_stairs(player_ptr, DETECT_RAD_DEFAULT);
        }

        if (plev > 14)
            detect_objects_normal(player_ptr, DETECT_RAD_DEFAULT);

        break;
    case HIDE_LEAVES:
        teleport_player(player_ptr, 10, TELEPORT_SPONTANEOUS);
        break;
    case KAWARIMI:
        if (ninja_data && !ninja_data->kawarimi) {
            msg_print(_("敵の攻撃に対して敏感になった。", "You are now prepared to evade any attacks."));
            ninja_data->kawarimi = true;
            player_ptr->redraw |= (PR_STATUS);
        }

        break;
    case ABSCONDING:
        teleport_player(player_ptr, player_ptr->lev * 5, TELEPORT_SPONTANEOUS);
        break;
    case HIT_AND_AWAY:
        if (!hit_and_away(player_ptr))
            return false;

        break;
    case BIND_MONSTER:
        if (!get_aim_dir(player_ptr, &dir))
            return false;

        (void)stasis_monster(player_ptr, dir);
        break;
    case ANCIENT_KNOWLEDGE:
        return ident_spell(player_ptr, false);
    case FLOATING:
        set_tim_levitation(player_ptr, randint1(20) + 20, false);
        break;
    case HIDE_FLAMES:
        fire_ball(player_ptr, GF_FIRE, 0, 50 + plev, plev / 10 + 2);
        teleport_player(player_ptr, 30, TELEPORT_SPONTANEOUS);
        set_oppose_fire(player_ptr, (TIME_EFFECT)plev, false);
        break;
    case NYUSIN:
        return rush_attack(player_ptr, nullptr);
    case SYURIKEN_SPREADING: {
        for (int i = 0; i < 8; i++) {
            OBJECT_IDX slot;

            for (slot = 0; slot < INVEN_PACK; slot++) {
                if (player_ptr->inventory_list[slot].tval == ItemKindType::SPIKE)
                    break;
            }

            if (slot == INVEN_PACK) {
                if (!i)
                    msg_print(_("くさびを持っていない。", "You have no Iron Spikes."));
                else
                    msg_print(_("くさびがなくなった。", "You have no more Iron Spikes."));

                return false;
            }

            (void)ThrowCommand(player_ptr).do_cmd_throw(1, false, slot);
            PlayerEnergy(player_ptr).set_player_turn_energy(100);
        }

        break;
    }
    case CHAIN_HOOK:
        (void)fetch_monster(player_ptr);
        break;
    case SMOKE_BALL:
        if (!get_aim_dir(player_ptr, &dir))
            return false;

        fire_ball(player_ptr, GF_OLD_CONF, dir, plev * 3, 3);
        break;
    case SWAP_POSITION:
        project_length = -1;
        if (!get_aim_dir(player_ptr, &dir)) {
            project_length = 0;
            return false;
        }

        project_length = 0;
        (void)teleport_swap(player_ptr, dir);
        break;
    case EXPLOSIVE_RUNE:
        create_rune_explosion(player_ptr, player_ptr->y, player_ptr->x);
        break;
    case HIDE_MUD:
        (void)set_pass_wall(player_ptr, randint1(plev / 2) + plev / 2, false);
        set_oppose_acid(player_ptr, (TIME_EFFECT)plev, false);
        break;
    case HIDE_MIST:
        fire_ball(player_ptr, GF_POIS, 0, 75 + plev * 2 / 3, plev / 5 + 2);
        fire_ball(player_ptr, GF_HYPODYNAMIA, 0, 75 + plev * 2 / 3, plev / 5 + 2);
        fire_ball(player_ptr, GF_CONFUSION, 0, 75 + plev * 2 / 3, plev / 5 + 2);
        teleport_player(player_ptr, 30, TELEPORT_SPONTANEOUS);
        break;
    case PURGATORY_FLAME: {
        int num = damroll(3, 9);
        for (int k = 0; k < num; k++) {
            EFFECT_ID typ = one_in_(2) ? GF_FIRE : one_in_(3) ? GF_NETHER : GF_PLASMA;
            int attempts = 1000;
            while (attempts--) {
                scatter(player_ptr, &y, &x, player_ptr->y, player_ptr->x, 4, PROJECT_NONE);
                if (!player_bold(player_ptr, y, x))
                    break;
            }

            project(player_ptr, 0, 0, y, x, damroll(6 + plev / 8, 10), typ, (PROJECT_BEAM | PROJECT_THRU | PROJECT_GRID | PROJECT_KILL));
        }

        break;
    }
    case ALTER_EGO:
        set_multishadow(player_ptr, 6 + randint1(6), false);
        break;
    default:
        msg_print(_("なに？", "Zap?"));
        break;
    }
    return true;
}
