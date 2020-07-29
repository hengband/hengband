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

#include "cmd-action/cmd-mind.h"
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
#include "mind/mind-berserker.h"
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
