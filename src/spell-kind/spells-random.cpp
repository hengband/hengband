﻿/*!
 * @brief ダイス目により様々な効果を及ぼす魔法の処理
 * @date 2020/06/05
 * @author Hourier
 */

#include "spell-kind/spells-random.h"
#include "core/hp-mp-processor.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "mutation/mutation-investor-remover.h"
#include "player-info/avatar.h"
#include "player/player-damage.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-equipment.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-genocide.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-specific-bolt.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spell-types.h"
#include "spell/spells-diceroll.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/experience.h"
#include "system/floor-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

/*!
 * @brief 混沌招来処理
 * @return 作用が実際にあった場合TRUEを返す
 */
void call_chaos(player_type *caster_ptr)
{
    int hurt_types[31] = { GF_ELEC, GF_POIS, GF_ACID, GF_COLD, GF_FIRE, GF_MISSILE, GF_ARROW, GF_PLASMA, GF_HOLY_FIRE, GF_WATER, GF_LITE, GF_DARK, GF_FORCE,
        GF_INERTIAL, GF_MANA, GF_METEOR, GF_ICE, GF_CHAOS, GF_NETHER, GF_DISENCHANT, GF_SHARDS, GF_SOUND, GF_NEXUS, GF_CONFUSION, GF_TIME, GF_GRAVITY,
        GF_ROCKET, GF_NUKE, GF_HELL_FIRE, GF_DISINTEGRATE, GF_PSY_SPEAR };

    int chaos_type = hurt_types[randint0(31)];
    bool line_chaos = FALSE;
    if (one_in_(4))
        line_chaos = TRUE;

    int dir;
    if (one_in_(6)) {
        for (int dummy = 1; dummy < 10; dummy++) {
            if (dummy - 5) {
                if (line_chaos)
                    fire_beam(caster_ptr, chaos_type, dummy, 150);
                else
                    fire_ball(caster_ptr, chaos_type, dummy, 150, 2);
            }
        }

        return;
    }

    if (one_in_(3)) {
        fire_ball(caster_ptr, chaos_type, 0, 500, 8);
        return;
    }

    if (!get_aim_dir(caster_ptr, &dir))
        return;
    if (line_chaos)
        fire_beam(caster_ptr, chaos_type, dir, 250);
    else
        fire_ball(caster_ptr, chaos_type, dir, 250, 3 + (caster_ptr->lev / 35));
}

/*!
 * @brief TY_CURSE処理発動 / Activate the evil Topi Ylinen curse
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param stop_ty 再帰処理停止フラグ
 * @param count 発動回数
 * @return 作用が実際にあった場合TRUEを返す
 * @details
 * <pre>
 * rr9: Stop the nasty things when a Cyberdemon is summoned
 * or the player gets paralyzed.
 * </pre>
 */
bool activate_ty_curse(player_type *target_ptr, bool stop_ty, int *count)
{
    BIT_FLAGS flg = (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP);
    bool is_first_curse = TRUE;
    floor_type *floor_ptr = target_ptr->current_floor_ptr;
    while (is_first_curse || (one_in_(3) && !stop_ty)) {
        is_first_curse = FALSE;
        switch (randint1(34)) {
        case 28:
        case 29:
            if (!(*count)) {
                msg_print(_("地面が揺れた...", "The ground trembles..."));
                earthquake(target_ptr, target_ptr->y, target_ptr->x, 5 + randint0(10), 0);
                if (!one_in_(6))
                    break;
            }
            /* Fall through */
        case 30:
        case 31:
            if (!(*count)) {
                HIT_POINT dam = damroll(10, 10);
                msg_print(_("純粋な魔力の次元への扉が開いた！", "A portal opens to a plane of raw mana!"));
                project(target_ptr, 0, 8, target_ptr->y, target_ptr->x, dam, GF_MANA, flg, -1);
                take_hit(target_ptr, DAMAGE_NOESCAPE, dam, _("純粋な魔力の解放", "released pure mana"), -1);
                if (!one_in_(6))
                    break;
            }
            /* Fall through */
        case 32:
        case 33:
            if (!(*count)) {
                msg_print(_("周囲の空間が歪んだ！", "Space warps about you!"));
                teleport_player(target_ptr, damroll(10, 10), TELEPORT_PASSIVE);
                if (randint0(13))
                    (*count) += activate_hi_summon(target_ptr, target_ptr->y, target_ptr->x, FALSE);
                if (!one_in_(6))
                    break;
            }
            /* Fall through */
        case 34:
            msg_print(_("エネルギーのうねりを感じた！", "You feel a surge of energy!"));
            wall_breaker(target_ptr);
            if (!randint0(7)) {
                project(target_ptr, 0, 7, target_ptr->y, target_ptr->x, 50, GF_KILL_WALL, flg, -1);
                take_hit(target_ptr, DAMAGE_NOESCAPE, 50, _("エネルギーのうねり", "surge of energy"), -1);
            }

            if (!one_in_(6))
                break;
            /* Fall through */
        case 1:
        case 2:
        case 3:
        case 16:
        case 17:
            aggravate_monsters(target_ptr, 0);
            if (!one_in_(6))
                break;
            /* Fall through */
        case 4:
        case 5:
        case 6:
            (*count) += activate_hi_summon(target_ptr, target_ptr->y, target_ptr->x, FALSE);
            if (!one_in_(6))
                break;
            /* Fall through */
        case 7:
        case 8:
        case 9:
        case 18:
            (*count) += summon_specific(target_ptr, 0, target_ptr->y, target_ptr->x, floor_ptr->dun_level, SUMMON_NONE, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET));
            if (!one_in_(6))
                break;
            /* Fall through */
        case 10:
        case 11:
        case 12:
            msg_print(_("経験値が体から吸い取られた気がする！", "You feel your experience draining away..."));
            lose_exp(target_ptr, target_ptr->exp / 16);
            if (!one_in_(6))
                break;
            /* Fall through */
        case 13:
        case 14:
        case 15:
        case 19:
        case 20: {
            bool is_statue = stop_ty;
            is_statue |= target_ptr->free_act && (randint1(125) < target_ptr->skill_sav);
            is_statue |= target_ptr->pclass == CLASS_BERSERKER;
            if (!is_statue) {
                msg_print(_("彫像になった気分だ！", "You feel like a statue!"));
                if (target_ptr->free_act)
                    set_paralyzed(target_ptr, target_ptr->paralyzed + randint1(3));
                else
                    set_paralyzed(target_ptr, target_ptr->paralyzed + randint1(13));
                stop_ty = TRUE;
            }

            if (!one_in_(6))
                break;
        }
            /* Fall through */
        case 21:
        case 22:
        case 23:
            (void)do_dec_stat(target_ptr, randint0(6));
            if (!one_in_(6))
                break;
            /* Fall through */
        case 24:
            msg_print(_("ほえ？私は誰？ここで何してる？", "Huh? Who am I? What am I doing here?"));
            lose_all_info(target_ptr);
            if (!one_in_(6))
                break;
            /* Fall through */
        case 25:
            if ((floor_ptr->dun_level > 65) && !stop_ty) {
                (*count) += summon_cyber(target_ptr, -1, target_ptr->y, target_ptr->x);
                stop_ty = TRUE;
                break;
            }

            if (!one_in_(6))
                break;
            /* Fall through */
        default:
            for (int i = 0; i < A_MAX; i++) {
                bool is_first_dec_stat = TRUE;
                while (is_first_dec_stat || one_in_(2)) {
                    is_first_dec_stat = FALSE;
                    (void)do_dec_stat(target_ptr, i);
                }
            }
        }
    }

    return stop_ty;
}

/*!
 * @brief 運命の輪、並びにカオス的な効果の発動
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param spell ランダムな効果を選択するための基準ID
 * @return なし
 */
void wild_magic(player_type *caster_ptr, int spell)
{
    int type = SUMMON_MOLD + randint0(6);
    if (type < SUMMON_MOLD)
        type = SUMMON_MOLD;
    else if (type > SUMMON_MIMIC)
        type = SUMMON_MIMIC;

    floor_type *floor_ptr = caster_ptr->current_floor_ptr;
    switch (randint1(spell) + randint1(8) + 1) {
    case 1:
    case 2:
    case 3:
        teleport_player(caster_ptr, 10, TELEPORT_PASSIVE);
        break;
    case 4:
    case 5:
    case 6:
        teleport_player(caster_ptr, 100, TELEPORT_PASSIVE);
        break;
    case 7:
    case 8:
        teleport_player(caster_ptr, 200, TELEPORT_PASSIVE);
        break;
    case 9:
    case 10:
    case 11:
        unlite_area(caster_ptr, 10, 3);
        break;
    case 12:
    case 13:
    case 14:
        lite_area(caster_ptr, damroll(2, 3), 2);
        break;
    case 15:
        destroy_doors_touch(caster_ptr);
        break;
    case 16:
    case 17:
        wall_breaker(caster_ptr);
        break;
    case 18:
        sleep_monsters_touch(caster_ptr);
        break;
    case 19:
    case 20:
        trap_creation(caster_ptr, caster_ptr->y, caster_ptr->x);
        break;
    case 21:
    case 22:
        door_creation(caster_ptr, caster_ptr->y, caster_ptr->x);
        break;
    case 23:
    case 24:
    case 25:
        aggravate_monsters(caster_ptr, 0);
        break;
    case 26:
        earthquake(caster_ptr, caster_ptr->y, caster_ptr->x, 5, 0);
        break;
    case 27:
    case 28:
        (void)gain_mutation(caster_ptr, 0);
        break;
    case 29:
    case 30:
        apply_disenchant(caster_ptr, 1);
        break;
    case 31:
        lose_all_info(caster_ptr);
        break;
    case 32:
        fire_ball(caster_ptr, GF_CHAOS, 0, spell + 5, 1 + (spell / 10));
        break;
    case 33:
        wall_stone(caster_ptr);
        break;
    case 34:
    case 35:
        for (int counter = 0; counter < 8; counter++) {
            (void)summon_specific(caster_ptr, 0, caster_ptr->y, caster_ptr->x, (floor_ptr->dun_level * 3) / 2, static_cast<summon_type>(type), (PM_ALLOW_GROUP | PM_NO_PET));
        }

        break;
    case 36:
    case 37:
        activate_hi_summon(caster_ptr, caster_ptr->y, caster_ptr->x, FALSE);
        break;
    case 38:
        (void)summon_cyber(caster_ptr, -1, caster_ptr->y, caster_ptr->x);
        break;
    default: {
        int count = 0;
        (void)activate_ty_curse(caster_ptr, FALSE, &count);
        break;
    }
    }
}

/*!
 * @brief 「ワンダー」のランダムな効果を決定して処理する。
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 方向ID
 * @return なし
 * @details
 * This spell should become more useful (more controlled) as the\n
 * player gains experience levels.  Thus, add 1/5 of the player's\n
 * level to the die roll.  This eliminates the worst effects later on,\n
 * while keeping the results quite random.  It also allows some potent\n
 * effects only at high level.
 */
void cast_wonder(player_type *caster_ptr, DIRECTION dir)
{
    PLAYER_LEVEL plev = caster_ptr->lev;
    int die = randint1(100) + plev / 5;
    int vir = virtue_number(caster_ptr, V_CHANCE);
    if (vir) {
        if (caster_ptr->virtues[vir - 1] > 0) {
            while (randint1(400) < caster_ptr->virtues[vir - 1])
                die++;
        } else {
            while (randint1(400) < (0 - caster_ptr->virtues[vir - 1]))
                die--;
        }
    }

    if (die < 26) {
        chg_virtue(caster_ptr, V_CHANCE, 1);
    }

    if (die > 100) {
        msg_print(_("あなたは力がみなぎるのを感じた！", "You feel a surge of power!"));
    }

    if (die < 8) {
        clone_monster(caster_ptr, dir);
        return;
    }

    if (die < 14) {
        speed_monster(caster_ptr, dir, plev);
        return;
    }

    if (die < 26) {
        heal_monster(caster_ptr, dir, damroll(4, 6));
        return;
    }

    if (die < 31) {
        poly_monster(caster_ptr, dir, plev);
        return;
    }

    if (die < 36) {
        fire_bolt_or_beam(caster_ptr, beam_chance(caster_ptr) - 10, GF_MISSILE, dir, damroll(3 + ((plev - 1) / 5), 4));
        return;
    }

    if (die < 41) {
        confuse_monster(caster_ptr, dir, plev);
        return;
    }

    if (die < 46) {
        fire_ball(caster_ptr, GF_POIS, dir, 20 + (plev / 2), 3);
        return;
    }

    if (die < 51) {
        (void)lite_line(caster_ptr, dir, damroll(6, 8));
        return;
    }

    if (die < 56) {
        fire_bolt_or_beam(caster_ptr, beam_chance(caster_ptr) - 10, GF_ELEC, dir, damroll(3 + ((plev - 5) / 4), 8));
        return;
    }

    if (die < 61) {
        fire_bolt_or_beam(caster_ptr, beam_chance(caster_ptr) - 10, GF_COLD, dir, damroll(5 + ((plev - 5) / 4), 8));
        return;
    }

    if (die < 66) {
        fire_bolt_or_beam(caster_ptr, beam_chance(caster_ptr), GF_ACID, dir, damroll(6 + ((plev - 5) / 4), 8));
        return;
    }

    if (die < 71) {
        fire_bolt_or_beam(caster_ptr, beam_chance(caster_ptr), GF_FIRE, dir, damroll(8 + ((plev - 5) / 4), 8));
        return;
    }

    if (die < 76) {
        hypodynamic_bolt(caster_ptr, dir, 75);
        return;
    }

    if (die < 81) {
        fire_ball(caster_ptr, GF_ELEC, dir, 30 + plev / 2, 2);
        return;
    }

    if (die < 86) {
        fire_ball(caster_ptr, GF_ACID, dir, 40 + plev, 2);
        return;
    }

    if (die < 91) {
        fire_ball(caster_ptr, GF_ICE, dir, 70 + plev, 3);
        return;
    }

    if (die < 96) {
        fire_ball(caster_ptr, GF_FIRE, dir, 80 + plev, 3);
        return;
    }

    if (die < 101) {
        hypodynamic_bolt(caster_ptr, dir, 100 + plev);
        return;
    }

    if (die < 104) {
        earthquake(caster_ptr, caster_ptr->y, caster_ptr->x, 12, 0);
        return;
    }

    if (die < 106) {
        (void)destroy_area(caster_ptr, caster_ptr->y, caster_ptr->x, 13 + randint0(5), FALSE);
        return;
    }

    if (die < 108) {
        symbol_genocide(caster_ptr, plev + 50, TRUE);
        return;
    }

    if (die < 110) {
        dispel_monsters(caster_ptr, 120);
        return;
    }

    dispel_monsters(caster_ptr, 150);
    slow_monsters(caster_ptr, plev);
    sleep_monsters(caster_ptr, plev);
    hp_player(caster_ptr, 300);
}
