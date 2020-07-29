/*!
 * @brief 各職業の特殊技能実装 / Special magics
 * @date 2014/01/15
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2005 henkma \n
 * 2014 Deskull rearranged comment for Doxygen.\n
 * @details
 * mind.cとあるが実際には超能力者、練気術師、狂戦士、鏡使い、忍者までの
 * 特殊技能を揃えて実装している。
 */

#include "mind/mind-switcher.h"
#include "action/action-limited.h"
#include "action/movement-execution.h"
#include "cmd-action/cmd-attack.h"
#include "cmd-action/cmd-spell.h"
#include "cmd-item/cmd-throw.h"
#include "core/asking-player.h"
#include "core/hp-mp-processor.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "effect/effect-characteristics.h"
#include "effect/spells-effect-util.h"
#include "floor/cave.h"
#include "floor/floor.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "game-option/text-display-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "io/command-repeater.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-explanations-table.h"
#include "mind/mind-force-trainer.h"
#include "mind/mind-info.h"
#include "mind/mind-magic-resistance.h"
#include "mind/mind-mindcrafter.h"
#include "mind/mind-mirror-master.h"
#include "mind/mind-ninja.h"
#include "mind/mind-power-getter.h"
#include "mind/mind-types.h"
#include "mind/mind-warrior.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-status.h"
#include "player-attack/player-attack.h"
#include "player/avatar.h"
#include "player/player-class.h"
#include "player/player-damage.h"
#include "player/player-move.h"
#include "player/special-defense-types.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-fetcher.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-grid.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell/process-effect.h"
#include "spell/spell-types.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "status/sight-setter.h"
#include "status/temporary-resistance.h"
#include "system/floor-type-definition.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "term/screen-processor.h"
#include "util/buffer-shaper.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

/*!
 * @brief 超能力の発動 /
 * do_cmd_cast calls this function if the player's class is 'mindcrafter'.
 * @param spell 発動する特殊技能のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
static bool cast_mindcrafter_spell(player_type *caster_ptr, int spell)
{
    int b = 0;
    DIRECTION dir;
    TIME_EFFECT t;
    PLAYER_LEVEL plev = caster_ptr->lev;
    // todo enum化する！
    switch (spell) {
    case 0: /* Precog */
        if (plev > 44) {
            chg_virtue(caster_ptr, V_KNOWLEDGE, 1);
            chg_virtue(caster_ptr, V_ENLIGHTEN, 1);
            wiz_lite(caster_ptr, FALSE);
        } else if (plev > 19)
            map_area(caster_ptr, DETECT_RAD_MAP);

        if (plev < 30) {
            b = detect_monsters_normal(caster_ptr, DETECT_RAD_DEFAULT);
            if (plev > 14)
                b |= detect_monsters_invis(caster_ptr, DETECT_RAD_DEFAULT);
            if (plev > 4) {
                b |= detect_traps(caster_ptr, DETECT_RAD_DEFAULT, TRUE);
                b |= detect_doors(caster_ptr, DETECT_RAD_DEFAULT);
            }
        } else {
            b = detect_all(caster_ptr, DETECT_RAD_DEFAULT);
        }

        if ((plev > 24) && (plev < 40))
            set_tim_esp(caster_ptr, (TIME_EFFECT)plev, FALSE);

        if (!b)
            msg_print(_("安全な気がする。", "You feel safe."));

        break;
    case 1:
        /* Mindblast */
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        if (randint1(100) < plev * 2)
            fire_beam(caster_ptr, GF_PSI, dir, damroll(3 + ((plev - 1) / 4), (3 + plev / 15)));
        else
            fire_ball(caster_ptr, GF_PSI, dir, damroll(3 + ((plev - 1) / 4), (3 + plev / 15)), 0);
        break;
    case 2:
        /* Minor displace */
        teleport_player(caster_ptr, 10, TELEPORT_SPONTANEOUS);
        break;
    case 3:
        /* Major displace */
        teleport_player(caster_ptr, plev * 5, TELEPORT_SPONTANEOUS);
        break;
    case 4:
        /* Domination */
        if (plev < 30) {
            if (!get_aim_dir(caster_ptr, &dir))
                return FALSE;

            fire_ball(caster_ptr, GF_DOMINATION, dir, plev, 0);
        } else {
            charm_monsters(caster_ptr, plev * 2);
        }
        break;
    case 5:
        /* Fist of Force  ---  not 'true' TK  */
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        fire_ball(caster_ptr, GF_TELEKINESIS, dir, damroll(8 + ((plev - 5) / 4), 8), (plev > 20 ? (plev - 20) / 8 + 1 : 0));
        break;
    case 6:
        /* Character Armour */
        set_shield(caster_ptr, (TIME_EFFECT)plev, FALSE);
        if (plev > 14)
            set_oppose_acid(caster_ptr, (TIME_EFFECT)plev, FALSE);
        if (plev > 19)
            set_oppose_fire(caster_ptr, (TIME_EFFECT)plev, FALSE);
        if (plev > 24)
            set_oppose_cold(caster_ptr, (TIME_EFFECT)plev, FALSE);
        if (plev > 29)
            set_oppose_elec(caster_ptr, (TIME_EFFECT)plev, FALSE);
        if (plev > 34)
            set_oppose_pois(caster_ptr, (TIME_EFFECT)plev, FALSE);
        break;
    case 7:
        /* Psychometry */
        if (plev < 25)
            return psychometry(caster_ptr);
        else
            return ident_spell(caster_ptr, FALSE, 0);
    case 8:
        /* Mindwave */
        msg_print(_("精神を捻じ曲げる波動を発生させた！", "Mind-warping forces emanate from your brain!"));

        if (plev < 25)
            project(caster_ptr, 0, 2 + plev / 10, caster_ptr->y, caster_ptr->x, (plev * 3), GF_PSI, PROJECT_KILL, -1);
        else
            (void)mindblast_monsters(caster_ptr, randint1(plev * ((plev - 5) / 10 + 1)));
        break;
    case 9:
        /* Adrenaline */
        set_afraid(caster_ptr, 0);
        set_stun(caster_ptr, 0);
        if (!is_fast(caster_ptr) || !is_hero(caster_ptr))
            hp_player(caster_ptr, plev);

        t = 10 + randint1((plev * 3) / 2);
        set_hero(caster_ptr, t, FALSE);
        (void)set_fast(caster_ptr, t, FALSE);
        break;
    case 10:
        /* Telekinesis */
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        fetch_item(caster_ptr, dir, plev * 15, FALSE);
        break;
    case 11:
        /* Psychic Drain */
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        b = damroll(plev / 2, 6);
        if (fire_ball(caster_ptr, GF_PSI_DRAIN, dir, b, 0))
            caster_ptr->energy_need += randint1(150);

        break;
    case 12:
        /* psycho-spear */
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        fire_beam(caster_ptr, GF_PSY_SPEAR, dir, randint1(plev * 3) + plev * 3);
        break;
    case 13: {
        time_walk(caster_ptr);
        break;
    }
    default:
        msg_print(_("なに？", "Zap?"));
    }

    return TRUE;
}

/*!
 * @brief 練気術の発動 /
 * do_cmd_cast calls this function if the player's class is 'ForceTrainer'.
 * @param spell 発動する特殊技能のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
static bool cast_force_spell(player_type *caster_ptr, int spell)
{
    DIRECTION dir;
    PLAYER_LEVEL plev = caster_ptr->lev;
    int boost = get_current_ki(caster_ptr);
    if (heavy_armor(caster_ptr))
        boost /= 2;

    switch (spell) {
    case 0:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        fire_ball(caster_ptr, GF_MISSILE, dir, damroll(3 + ((plev - 1) / 5) + boost / 12, 4), 0);
        break;
    case 1:
        (void)lite_area(caster_ptr, damroll(2, (plev / 2)), (plev / 10) + 1);
        break;
    case 2:
        set_tim_levitation(caster_ptr, randint1(30) + 30 + boost / 5, FALSE);
        break;
    case 3:
        project_length = plev / 8 + 3;
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        fire_beam(caster_ptr, GF_MISSILE, dir, damroll(5 + ((plev - 1) / 5) + boost / 10, 5));
        break;
    case 4:
        set_resist_magic(caster_ptr, randint1(20) + 20 + boost / 5, FALSE);
        break;
    case 5:
        msg_print(_("気を練った。", "You improved the Force."));
        set_current_ki(caster_ptr, FALSE, 70 + plev);
        caster_ptr->update |= (PU_BONUS);
        if (randint1(get_current_ki(caster_ptr)) > (plev * 4 + 120)) {
            msg_print(_("気が暴走した！", "The Force exploded!"));
            fire_ball(caster_ptr, GF_MANA, 0, get_current_ki(caster_ptr) / 2, 10);
            take_hit(caster_ptr, DAMAGE_LOSELIFE, caster_ptr->magic_num1[0] / 2, _("気の暴走", "Explosion of the Force"), -1);
        } else
            return TRUE;

        break;
    case 6:
        set_tim_sh_force(caster_ptr, randint1(plev / 2) + 15 + boost / 7, FALSE);
        break;
    case 7:
        return shock_power(caster_ptr);
        break;
    case 8:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        fire_ball(caster_ptr, GF_MISSILE, dir, damroll(10, 6) + plev * 3 / 2 + boost * 3 / 5, (plev < 30) ? 2 : 3);
        break;
    case 9: {
        if (!target_set(caster_ptr, TARGET_KILL))
            return FALSE;

        MONSTER_IDX m_idx = caster_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx;
        if ((m_idx == 0) || !player_has_los_bold(caster_ptr, target_row, target_col)
            || !projectable(caster_ptr, caster_ptr->y, caster_ptr->x, target_row, target_col))
            break;

        dispel_monster_status(caster_ptr, m_idx);
        break;
    }
    case 10: {
        bool success = FALSE;
        for (int i = 0; i < 1 + boost / 100; i++)
            if (summon_specific(caster_ptr, -1, caster_ptr->y, caster_ptr->x, plev, SUMMON_PHANTOM, PM_FORCE_PET))
                success = TRUE;

        if (success)
            msg_print(_("御用でございますが、御主人様？", "'Your wish, master?'"));
        else
            msg_print(_("何も現れなかった。", "Nothing happen."));

        break;
    }
    case 11:
        fire_ball(caster_ptr, GF_FIRE, 0, 200 + (2 * plev) + boost * 2, 10);
        break;
    case 12:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        fire_beam(caster_ptr, GF_MANA, dir, damroll(10 + (plev / 2) + boost * 3 / 10, 15));
        break;
    case 13:
        set_lightspeed(caster_ptr, randint1(16) + 16 + boost / 20, FALSE);
        break;
    default:
        msg_print(_("なに？", "Zap?"));
    }

    set_current_ki(caster_ptr, TRUE, 0);
    caster_ptr->update |= PU_BONUS;
    return TRUE;
}

/*!
 * @brief 現在フロアに存在している鏡の数を数える / calculate mirrors
 * @return 鏡の枚数
 */
static int number_of_mirrors(floor_type *floor_ptr)
{
    int val = 0;
    for (POSITION x = 0; x < floor_ptr->width; x++) {
        for (POSITION y = 0; y < floor_ptr->height; y++) {
            if (is_mirror_grid(&floor_ptr->grid_array[y][x]))
                val++;
        }
    }

    return val;
}

/*!
 * @brief 鏡魔法の発動 /
 * do_cmd_cast calls this function if the player's class is 'Mirror magic'.
 * @param spell 発動する特殊技能のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
static bool cast_mirror_spell(player_type *caster_ptr, int spell)
{
    DIRECTION dir;
    PLAYER_LEVEL plev = caster_ptr->lev;
    int tmp;
    TIME_EFFECT t;
    POSITION x, y;
    // todo enum化する！
    switch (spell) {
        /* mirror of seeing */
    case 0:
        tmp = is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x]) ? 4 : 0;
        if (plev + tmp > 4)
            detect_monsters_normal(caster_ptr, DETECT_RAD_DEFAULT);
        if (plev + tmp > 18)
            detect_monsters_invis(caster_ptr, DETECT_RAD_DEFAULT);
        if (plev + tmp > 28)
            set_tim_esp(caster_ptr, (TIME_EFFECT)plev, FALSE);
        if (plev + tmp > 38)
            map_area(caster_ptr, DETECT_RAD_MAP);
        if (tmp == 0 && plev < 5) {
            msg_print(_("鏡がなくて集中できなかった！", "You need a mirror to concentrate!"));
        }
        break;
        /* drip of light */
    case 1:
        if (number_of_mirrors(caster_ptr->current_floor_ptr) < 4 + plev / 10)
            place_mirror(caster_ptr);
        else
            msg_format(_("これ以上鏡は制御できない！", "There are too many mirrors to control!"));

        break;
    case 2:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        if (plev > 9 && is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x]))
            fire_beam(caster_ptr, GF_LITE, dir, damroll(3 + ((plev - 1) / 5), 4));
        else
            fire_bolt(caster_ptr, GF_LITE, dir, damroll(3 + ((plev - 1) / 5), 4));

        break;
        /* warped mirror */
    case 3:
        teleport_player(caster_ptr, 10, TELEPORT_SPONTANEOUS);
        break;
        /* mirror of light */
    case 4:
        (void)lite_area(caster_ptr, damroll(2, (plev / 2)), (plev / 10) + 1);
        break;
        /* mirror of wandering */
    case 5:
        teleport_player(caster_ptr, plev * 5, TELEPORT_SPONTANEOUS);
        break;
        /* robe of dust */
    case 6:
        set_dustrobe(caster_ptr, 20 + randint1(20), FALSE);
        break;
        /* banishing mirror */
    case 7:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        (void)fire_beam(caster_ptr, GF_AWAY_ALL, dir, plev);
        break;
        /* mirror clashing */
    case 8:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        fire_ball(caster_ptr, GF_SHARDS, dir, damroll(8 + ((plev - 5) / 4), 8), (plev > 20 ? (plev - 20) / 8 + 1 : 0));
        break;
        /* mirror sleeping */
    case 9:
        for (x = 0; x < caster_ptr->current_floor_ptr->width; x++)
            for (y = 0; y < caster_ptr->current_floor_ptr->height; y++)
                if (is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[y][x]))
                    project(caster_ptr, 0, 2, y, x, (HIT_POINT)plev, GF_OLD_SLEEP,
                        (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI), -1);

        break;
        /* seeker ray */
    case 10:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        fire_beam(caster_ptr, GF_SEEKER, dir, damroll(11 + (plev - 5) / 4, 8));
        break;
        /* seal of mirror */
    case 11:
        seal_of_mirror(caster_ptr, plev * 4 + 100);
        break;
        /* shield of water */
    case 12:
        t = 20 + randint1(20);
        set_shield(caster_ptr, t, FALSE);
        if (plev > 31)
            set_tim_reflect(caster_ptr, t, FALSE);

        if (plev > 39)
            set_resist_magic(caster_ptr, t, FALSE);

        break;
        /* super ray */
    case 13:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        fire_beam(caster_ptr, GF_SUPER_RAY, dir, 150 + randint1(2 * plev));
        break;
        /* illusion light */
    case 14:
        tmp = is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x]) ? 4 : 3;
        slow_monsters(caster_ptr, plev);
        stun_monsters(caster_ptr, plev * tmp);
        confuse_monsters(caster_ptr, plev * tmp);
        turn_monsters(caster_ptr, plev * tmp);
        stun_monsters(caster_ptr, plev * tmp);
        stasis_monsters(caster_ptr, plev * tmp);
        break;
        /* mirror shift */
    case 15:
        if (!is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x])) {
            msg_print(_("鏡の国の場所がわからない！", "You cannot find out where the mirror is!"));
            break;
        }

        reserve_alter_reality(caster_ptr);
        break;
        /* mirror tunnel */
    case 16:
        msg_print(_("鏡の世界を通り抜け…  ", "You try to enter the mirror..."));
        return mirror_tunnel(caster_ptr);

        /* mirror of recall */
    case 17:
        return recall_player(caster_ptr, randint0(21) + 15);
        /* multi-shadow */
    case 18:
        set_multishadow(caster_ptr, 6 + randint1(6), FALSE);
        break;
        /* binding field */
    case 19:
        if (!binding_field(caster_ptr, plev * 11 + 5))
            msg_print(_("適当な鏡を選べなかった！", "You were not able to choose suitable mirrors!"));

        break;
        /* mirror of Ruffnor */
    case 20:
        (void)set_invuln(caster_ptr, randint1(4) + 4, FALSE);
        break;
    default:
        msg_print(_("なに？", "Zap?"));
    }

    caster_ptr->magic_num1[0] = 0;
    return TRUE;
}

/*!
 * @brief 怒りの発動 /
 * do_cmd_cast calls this function if the player's class is 'berserker'.
 * @param spell 発動する特殊技能のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
static bool cast_berserk_spell(player_type *caster_ptr, int spell)
{
    POSITION y, x;
    DIRECTION dir;

    // todo enum化する！
    switch (spell) {
    case 0:
        detect_monsters_mind(caster_ptr, DETECT_RAD_DEFAULT);
        break;
    case 1: {
        if (caster_ptr->riding) {
            msg_print(_("乗馬中には無理だ。", "You cannot do it when riding."));
            return FALSE;
        }

        if (!get_direction(caster_ptr, &dir, FALSE, FALSE) || (dir == 5))
            return FALSE;

        y = caster_ptr->y + ddy[dir];
        x = caster_ptr->x + ddx[dir];
        if (!caster_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
            msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
            return FALSE;
        }

        do_cmd_attack(caster_ptr, y, x, 0);
        if (!player_can_enter(caster_ptr, caster_ptr->current_floor_ptr->grid_array[y][x].feat, 0)
            || is_trap(caster_ptr, caster_ptr->current_floor_ptr->grid_array[y][x].feat))
            break;

        y += ddy[dir];
        x += ddx[dir];
        if (player_can_enter(caster_ptr, caster_ptr->current_floor_ptr->grid_array[y][x].feat, 0)
            && !is_trap(caster_ptr, caster_ptr->current_floor_ptr->grid_array[y][x].feat) && !caster_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
            msg_print(NULL);
            (void)move_player_effect(caster_ptr, y, x, MPE_FORGET_FLOW | MPE_HANDLE_STUFF | MPE_DONT_PICKUP);
        }

        break;
    }
    case 2: {
        if (!get_direction(caster_ptr, &dir, FALSE, FALSE))
            return FALSE;

        y = caster_ptr->y + ddy[dir];
        x = caster_ptr->x + ddx[dir];
        exe_movement(caster_ptr, dir, easy_disarm, TRUE);
        break;
    }
    case 3:
        earthquake(caster_ptr, caster_ptr->y, caster_ptr->x, 8 + randint0(5), 0);
        break;
    case 4:
        massacre(caster_ptr);
        break;
    default:
        msg_print(_("なに？", "Zap?"));
        break;
    }

    return TRUE;
}

/*!
 * @brief 忍術の発動 /
 * do_cmd_cast calls this function if the player's class is 'ninja'.
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param spell 発動する特殊技能のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
static bool cast_ninja_spell(player_type *caster_ptr, int spell)
{
    POSITION x = 0, y = 0;
    DIRECTION dir;
    PLAYER_LEVEL plev = caster_ptr->lev;

    switch (spell) {
    case 0:
        (void)unlite_area(caster_ptr, 0, 3);
        break;
    case 1:
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
    case 2:
        teleport_player(caster_ptr, 10, TELEPORT_SPONTANEOUS);
        break;
    case 3:
        if (!(caster_ptr->special_defense & NINJA_KAWARIMI)) {
            msg_print(_("敵の攻撃に対して敏感になった。", "You are now prepared to evade any attacks."));
            caster_ptr->special_defense |= NINJA_KAWARIMI;
            caster_ptr->redraw |= (PR_STATUS);
        }

        break;
    case 4:
        teleport_player(caster_ptr, caster_ptr->lev * 5, TELEPORT_SPONTANEOUS);
        break;
    case 5:
        if (!hit_and_away(caster_ptr))
            return FALSE;

        break;
    case 6:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        (void)stasis_monster(caster_ptr, dir);
        break;
    case 7:
        return ident_spell(caster_ptr, FALSE, 0);
    case 8:
        set_tim_levitation(caster_ptr, randint1(20) + 20, FALSE);
        break;
    case 9:
        fire_ball(caster_ptr, GF_FIRE, 0, 50 + plev, plev / 10 + 2);
        teleport_player(caster_ptr, 30, TELEPORT_SPONTANEOUS);
        set_oppose_fire(caster_ptr, (TIME_EFFECT)plev, FALSE);
        break;
    case 10:
        return rush_attack(caster_ptr, NULL);
    case 11: {
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
    case 12:
        (void)fetch_monster(caster_ptr);
        break;
    case 13:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        fire_ball(caster_ptr, GF_OLD_CONF, dir, plev * 3, 3);
        break;
    case 14:
        project_length = -1;
        if (!get_aim_dir(caster_ptr, &dir)) {
            project_length = 0;
            return FALSE;
        }

        project_length = 0;
        (void)teleport_swap(caster_ptr, dir);
        break;
    case 15:
        explosive_rune(caster_ptr, caster_ptr->y, caster_ptr->x);
        break;
    case 16:
        (void)set_pass_wall(caster_ptr, randint1(plev / 2) + plev / 2, FALSE);
        set_oppose_acid(caster_ptr, (TIME_EFFECT)plev, FALSE);
        break;
    case 17:
        fire_ball(caster_ptr, GF_POIS, 0, 75 + plev * 2 / 3, plev / 5 + 2);
        fire_ball(caster_ptr, GF_HYPODYNAMIA, 0, 75 + plev * 2 / 3, plev / 5 + 2);
        fire_ball(caster_ptr, GF_CONFUSION, 0, 75 + plev * 2 / 3, plev / 5 + 2);
        teleport_player(caster_ptr, 30, TELEPORT_SPONTANEOUS);
        break;
    case 18: {
        int num = damroll(3, 9);
        for (int k = 0; k < num; k++) {
            EFFECT_ID typ = one_in_(2) ? GF_FIRE : one_in_(3) ? GF_NETHER : GF_PLASMA;
            int attempts = 1000;
            while (attempts--) {
                scatter(caster_ptr, &y, &x, caster_ptr->y, caster_ptr->x, 4, 0);
                if (!player_bold(caster_ptr, y, x))
                    break;
            }

            project(caster_ptr, 0, 0, y, x, damroll(6 + plev / 8, 10), typ, (PROJECT_BEAM | PROJECT_THRU | PROJECT_GRID | PROJECT_KILL), -1);
        }

        break;
    }
    case 19:
        set_multishadow(caster_ptr, 6 + randint1(6), FALSE);
        break;
    default:
        msg_print(_("なに？", "Zap?"));
        break;
    }
    return TRUE;
}

/*!
 * @brief 特殊技能コマンドのメインルーチン /
 * @return なし
 */
void do_cmd_mind(player_type *caster_ptr)
{
    SPELL_IDX n = 0;
    int b = 0;
    PERCENTAGE chance;
    PERCENTAGE minfail = 0;
    PLAYER_LEVEL plev = caster_ptr->lev;
    int old_csp = caster_ptr->csp;
    mind_type spell;
    bool cast;
    int use_mind, mana_cost;
    concptr p;
    bool on_mirror = FALSE;

    if (cmd_limit_confused(caster_ptr) || !get_mind_power(caster_ptr, &n, FALSE))
        return;

    switch (caster_ptr->pclass) {
    case CLASS_MINDCRAFTER:
        use_mind = MIND_MINDCRAFTER;
        p = _("精神", "skill");
        break;
    case CLASS_FORCETRAINER:
        use_mind = MIND_KI;
        p = _("気", "skill");
        break;
    case CLASS_BERSERKER:
        use_mind = MIND_BERSERKER;
        p = _("怒り", "skill");
        break;
    case CLASS_MIRROR_MASTER:
        use_mind = MIND_MIRROR_MASTER;
        p = _("鏡魔法", "skill");
        break;
    case CLASS_NINJA:
        use_mind = MIND_NINJUTSU;
        p = _("精神", "skill");
        break;
    default:
        use_mind = 0;
        p = _("超能力", "skill");
        break;
    }

    spell = mind_powers[use_mind].info[n];
    chance = spell.fail;
    mana_cost = spell.mana_cost;
    if (use_mind == MIND_KI) {
        if (heavy_armor(caster_ptr))
            chance += 20;

        if (caster_ptr->icky_wield[0])
            chance += 20;
        else if (has_melee_weapon(caster_ptr, INVEN_RARM))
            chance += 10;

        if (caster_ptr->icky_wield[1])
            chance += 20;
        else if (has_melee_weapon(caster_ptr, INVEN_LARM))
            chance += 10;

        if (n == 5) {
            for (int j = 0; j < get_current_ki(caster_ptr) / 50; j++)
                mana_cost += (j + 1) * 3 / 2;
        }
    }

    if ((use_mind == MIND_BERSERKER) || (use_mind == MIND_NINJUTSU)) {
        if (mana_cost > caster_ptr->chp) {
            msg_print(_("ＨＰが足りません。", "You do not have enough hp to use this power."));
            return;
        }
    } else if (mana_cost > caster_ptr->csp) {
        msg_print(_("ＭＰが足りません。", "You do not have enough mana to use this power."));
        if (!over_exert)
            return;

        if (!get_check(_("それでも挑戦しますか? ", "Attempt it anyway? ")))
            return;
    }

    if (chance) {
        chance -= 3 * (plev - spell.min_lev);
        chance += caster_ptr->to_m_chance;
        chance -= 3 * (adj_mag_stat[caster_ptr->stat_ind[mp_ptr->spell_stat]] - 1);
        if ((mana_cost > caster_ptr->csp) && (use_mind != MIND_BERSERKER) && (use_mind != MIND_NINJUTSU))
            chance += 5 * (mana_cost - caster_ptr->csp);

        minfail = adj_mag_fail[caster_ptr->stat_ind[mp_ptr->spell_stat]];
        if (chance < minfail)
            chance = minfail;

        if (caster_ptr->stun > 50)
            chance += 25;
        else if (caster_ptr->stun)
            chance += 15;

        if (use_mind == MIND_KI) {
            if (heavy_armor(caster_ptr))
                chance += 5;
            if (caster_ptr->icky_wield[0])
                chance += 5;
            if (caster_ptr->icky_wield[1])
                chance += 5;
        }
    }

    if (chance > 95)
        chance = 95;

    if (randint0(100) < chance) {
        if (flush_failure)
            flush();

        msg_format(_("%sの集中に失敗した！", "You failed to concentrate hard enough for %s!"), p);
        sound(SOUND_FAIL);
        if ((use_mind != MIND_BERSERKER) && (use_mind != MIND_NINJUTSU)) {
            if ((use_mind == MIND_KI) && (n != 5) && get_current_ki(caster_ptr)) {
                msg_print(_("気が散ってしまった．．．", "Your improved Force has gone away..."));
                set_current_ki(caster_ptr, TRUE, 0);
            }

            if (randint1(100) < (chance / 2)) {
                b = randint1(100);
                if (use_mind == MIND_MINDCRAFTER) {
                    if (b < 5) {
                        msg_print(_("なんてこった！頭の中が真っ白になった！", "Oh, no! Your mind has gone blank!"));
                        lose_all_info(caster_ptr);
                    } else if (b < 15) {
                        msg_print(_("奇妙な光景が目の前で踊っている...", "Weird visions seem to dance before your eyes..."));
                        set_image(caster_ptr, caster_ptr->image + 5 + randint1(10));
                    } else if (b < 45) {
                        msg_print(_("あなたの頭は混乱した！", "Your brain is addled!"));
                        set_confused(caster_ptr, caster_ptr->confused + randint1(8));
                    } else if (b < 90) {
                        set_stun(caster_ptr, caster_ptr->stun + randint1(8));
                    } else {
                        msg_format(_("%sの力が制御できない氾流となって解放された！", "Your mind unleashes its power in an uncontrollable storm!"), p);
                        project(caster_ptr, PROJECT_WHO_UNCTRL_POWER, 2 + plev / 10, caster_ptr->y, caster_ptr->x, plev * 2, GF_MANA,
                            PROJECT_JUMP | PROJECT_KILL | PROJECT_GRID | PROJECT_ITEM, -1);
                        caster_ptr->csp = MAX(0, caster_ptr->csp - plev * MAX(1, plev / 10));
                    }
                }

                if (use_mind == MIND_MIRROR_MASTER) {
                    if (b < 51) {
                    } else if (b < 81) {
                        msg_print(_("鏡の世界の干渉を受けた！", "Weird visions seem to dance before your eyes..."));
                        teleport_player(caster_ptr, 10, TELEPORT_PASSIVE);
                    } else if (b < 96) {
                        msg_print(_("まわりのものがキラキラ輝いている！", "Your brain is addled!"));
                        set_image(caster_ptr, caster_ptr->image + 5 + randint1(10));
                    } else {
                        msg_format(_("%sの力が制御できない氾流となって解放された！", "Your mind unleashes its power in an uncontrollable storm!"), p);
                        project(caster_ptr, PROJECT_WHO_UNCTRL_POWER, 2 + plev / 10, caster_ptr->y, caster_ptr->x, plev * 2, GF_MANA,
                            PROJECT_JUMP | PROJECT_KILL | PROJECT_GRID | PROJECT_ITEM, -1);
                        caster_ptr->csp = MAX(0, caster_ptr->csp - plev * MAX(1, plev / 10));
                    }
                }
            }
        }
    } else {
        sound(SOUND_ZAP);
        switch (use_mind) {
        case MIND_MINDCRAFTER:
            cast = cast_mindcrafter_spell(caster_ptr, n);
            break;
        case MIND_KI:
            cast = cast_force_spell(caster_ptr, n);
            break;
        case MIND_BERSERKER:
            cast = cast_berserk_spell(caster_ptr, n);
            break;
        case MIND_MIRROR_MASTER:
            if (is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x]))
                on_mirror = TRUE;

            cast = cast_mirror_spell(caster_ptr, n);
            break;
        case MIND_NINJUTSU:
            cast = cast_ninja_spell(caster_ptr, n);
            break;
        default:
            msg_format(_("謎の能力:%d, %d", "Mystery power:%d, %d"), use_mind, n);
            return;
        }

        if (!cast)
            return;
    }

    if (on_mirror && caster_ptr->pclass == CLASS_MIRROR_MASTER) {
        if (n == 3 || n == 5 || n == 7 || n == 16)
            take_turn(caster_ptr, 50);
    } else
        take_turn(caster_ptr, 100);

    if ((use_mind == MIND_BERSERKER) || (use_mind == MIND_NINJUTSU)) {
        take_hit(caster_ptr, DAMAGE_USELIFE, mana_cost, _("過度の集中", "concentrating too hard"), -1);
        caster_ptr->redraw |= (PR_HP);
    } else if (mana_cost <= old_csp) {
        caster_ptr->csp -= mana_cost;
        if (caster_ptr->csp < 0)
            caster_ptr->csp = 0;

        if ((use_mind == MIND_MINDCRAFTER) && (n == 13)) {
            caster_ptr->csp = 0;
            caster_ptr->csp_frac = 0;
        }
    } else {
        int oops = mana_cost - old_csp;
        if ((caster_ptr->csp - mana_cost) < 0)
            caster_ptr->csp_frac = 0;

        caster_ptr->csp = MAX(0, caster_ptr->csp - mana_cost);
        msg_format(_("%sを集中しすぎて気を失ってしまった！", "You faint from the effort!"), p);
        (void)set_paralyzed(caster_ptr, caster_ptr->paralyzed + randint1(5 * oops + 1));
        if (randint0(100) < 50) {
            bool perm = (randint0(100) < 25);
            msg_print(_("自分の精神を攻撃してしまった！", "You have damaged your mind!"));
            (void)dec_stat(caster_ptr, A_WIS, 15 + randint1(10), perm);
        }
    }

    caster_ptr->redraw |= (PR_MANA);
    caster_ptr->window |= (PW_PLAYER);
    caster_ptr->window |= (PW_SPELL);
}

/*!
 * @brief 現在プレイヤーが使用可能な特殊技能の一覧表示 /
 * @return なし
 */
void do_cmd_mind_browse(player_type *caster_ptr)
{
    SPELL_IDX n = 0;
    char temp[62 * 5];
    int use_mind = 0;
    if (caster_ptr->pclass == CLASS_MINDCRAFTER)
        use_mind = MIND_MINDCRAFTER;
    else if (caster_ptr->pclass == CLASS_FORCETRAINER)
        use_mind = MIND_KI;
    else if (caster_ptr->pclass == CLASS_BERSERKER)
        use_mind = MIND_BERSERKER;
    else if (caster_ptr->pclass == CLASS_NINJA)
        use_mind = MIND_NINJUTSU;
    else if (caster_ptr->pclass == CLASS_MIRROR_MASTER)
        use_mind = MIND_MIRROR_MASTER;

    screen_save();
    while (TRUE) {
        if (!get_mind_power(caster_ptr, &n, TRUE)) {
            screen_load();
            return;
        }

        term_erase(12, 21, 255);
        term_erase(12, 20, 255);
        term_erase(12, 19, 255);
        term_erase(12, 18, 255);
        term_erase(12, 17, 255);
        term_erase(12, 16, 255);
        shape_buffer(mind_tips[use_mind][n], 62, temp, sizeof(temp));
        for (int j = 0, line = 17; temp[j]; j += (1 + strlen(&temp[j]))) {
            prt(&temp[j], line, 15);
            line++;
        }

        switch (use_mind) {
        case MIND_MIRROR_MASTER:
        case MIND_NINJUTSU:
            prt(_("何かキーを押して下さい。", "Hit any key."), 0, 0);
            (void)inkey();
            break;
        default:
            break;
        }
    }
}
