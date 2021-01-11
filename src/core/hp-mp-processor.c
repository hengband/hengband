#include "core/hp-mp-processor.h"
#include "cmd-action/cmd-pet.h"
#include "core/hp-mp-regenerator.h"
#include "core/player-redraw-types.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/pattern-walk.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/trc-types.h"
#include "player/attack-defense-types.h"
#include "player-info/avatar.h"
#include "player/player-damage.h"
#include "player/player-race-types.h"
#include "player/player-race.h"
#include "player/special-defense-types.h"
#include "player/player-status-flags.h"
#include "status/bad-status-setter.h"
#include "status/element-resistance.h"
#include "system/floor-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"
#include "player/player-status-resist.h"
#include "util/bit-flags-calculator.h"
#include "object/object-flags.h"
#include "object-enchant/tr-types.h"


/*!
 * @brief 10ゲームターンが進行するごとにプレイヤーのHPとMPの増減処理を行う。
 *  / Handle timed damage and regeneration every 10 game turns
 * @return なし
 */
void process_player_hp_mp(player_type *creature_ptr)
{
    feature_type *f_ptr = &f_info[creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].feat];
    bool cave_no_regen = FALSE;
    int upkeep_factor = 0;
    int regen_amount = PY_REGEN_NORMAL;
    if (creature_ptr->poisoned && !is_invuln(creature_ptr)) {
        take_hit(creature_ptr, DAMAGE_NOESCAPE, 1, _("毒", "poison"), -1);
    }

    if (creature_ptr->cut && !is_invuln(creature_ptr)) {
        HIT_POINT dam;
        if (creature_ptr->cut > 1000) {
            dam = 200;
        } else if (creature_ptr->cut > 200) {
            dam = 80;
        } else if (creature_ptr->cut > 100) {
            dam = 32;
        } else if (creature_ptr->cut > 50) {
            dam = 16;
        } else if (creature_ptr->cut > 25) {
            dam = 7;
        } else if (creature_ptr->cut > 10) {
            dam = 3;
        } else {
            dam = 1;
        }

        take_hit(creature_ptr, DAMAGE_NOESCAPE, dam, _("致命傷", "a fatal wound"), -1);
    }

    if (is_specific_player_race(creature_ptr, RACE_VAMPIRE) || (creature_ptr->mimic_form == MIMIC_VAMPIRE)) {
        if (!creature_ptr->current_floor_ptr->dun_level && !has_resist_lite(creature_ptr) && !is_invuln(creature_ptr) && is_daytime()) {
            if ((creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].info & (CAVE_GLOW | CAVE_MNDK)) == CAVE_GLOW) {
                msg_print(_("日光があなたのアンデッドの肉体を焼き焦がした！", "The sun's rays scorch your undead flesh!"));
                take_hit(creature_ptr, DAMAGE_NOESCAPE, 1, _("日光", "sunlight"), -1);
                cave_no_regen = TRUE;
            }
        }

        object_type *o_ptr;
        o_ptr = &creature_ptr->inventory_list[INVEN_LITE];
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        object_flags(creature_ptr, o_ptr, flgs);

        if (creature_ptr->inventory_list[INVEN_LITE].tval && !has_flag(flgs, TR_DARK_SOURCE) && !has_resist_lite(creature_ptr)) {
            GAME_TEXT o_name[MAX_NLEN];
            char ouch[MAX_NLEN + 40];
            describe_flavor(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
            msg_format(_("%sがあなたのアンデッドの肉体を焼き焦がした！", "The %s scorches your undead flesh!"), o_name);

            cave_no_regen = TRUE;
            describe_flavor(creature_ptr, o_name, o_ptr, OD_NAME_ONLY);
            sprintf(ouch, _("%sを装備したダメージ", "wielding %s"), o_name);

            if (!is_invuln(creature_ptr))
                take_hit(creature_ptr, DAMAGE_NOESCAPE, 1, ouch, -1);
        }
    }

    if (has_flag(f_ptr->flags, FF_LAVA) && !is_invuln(creature_ptr) && !has_immune_fire(creature_ptr)) {
        int damage = 0;

        if (has_flag(f_ptr->flags, FF_DEEP)) {
            damage = 6000 + randint0(4000);
        } else if (!creature_ptr->levitation) {
            damage = 3000 + randint0(2000);
        }

        if (damage) {
            if (is_specific_player_race(creature_ptr, RACE_ENT))
                damage += damage / 3;
            if (has_resist_fire(creature_ptr))
                damage = damage / 3;
            if (is_oppose_fire(creature_ptr))
                damage = damage / 3;


            if (creature_ptr->levitation)
                damage = damage / 5;

            damage = damage / 100 + (randint0(100) < (damage % 100));

            if (creature_ptr->levitation) {
                msg_print(_("熱で火傷した！", "The heat burns you!"));
                take_hit(creature_ptr, DAMAGE_NOESCAPE, damage,
                    format(_("%sの上に浮遊したダメージ", "flying over %s"),
                        f_name + f_info[get_feat_mimic(&creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x])].name),
                    -1);
            } else {
                concptr name = f_name + f_info[get_feat_mimic(&creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x])].name;
                msg_format(_("%sで火傷した！", "The %s burns you!"), name);
                take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, name, -1);
            }

            cave_no_regen = TRUE;
        }
    }

    if (has_flag(f_ptr->flags, FF_COLD_PUDDLE) && !is_invuln(creature_ptr) && !has_immune_cold(creature_ptr)) {
        int damage = 0;

        if (has_flag(f_ptr->flags, FF_DEEP)) {
            damage = 6000 + randint0(4000);
        } else if (!creature_ptr->levitation) {
            damage = 3000 + randint0(2000);
        }

        if (damage) {
            if (has_resist_cold(creature_ptr))
                damage = damage / 3;
            if (is_oppose_cold(creature_ptr))
                damage = damage / 3;
            if (creature_ptr->levitation)
                damage = damage / 5;

            damage = damage / 100 + (randint0(100) < (damage % 100));

            if (creature_ptr->levitation) {
                msg_print(_("冷気に覆われた！", "The cold engulfs you!"));
                take_hit(creature_ptr, DAMAGE_NOESCAPE, damage,
                    format(_("%sの上に浮遊したダメージ", "flying over %s"),
                        f_name + f_info[get_feat_mimic(&creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x])].name),
                    -1);
            } else {
                concptr name = f_name + f_info[get_feat_mimic(&creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x])].name;
                msg_format(_("%sに凍えた！", "The %s frostbites you!"), name);
                take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, name, -1);
            }

            cave_no_regen = TRUE;
        }
    }

    if (has_flag(f_ptr->flags, FF_ELEC_PUDDLE) && !is_invuln(creature_ptr) && !has_immune_elec(creature_ptr)) {
        int damage = 0;

        if (has_flag(f_ptr->flags, FF_DEEP)) {
            damage = 6000 + randint0(4000);
        } else if (!creature_ptr->levitation) {
            damage = 3000 + randint0(2000);
        }

        if (damage) {
            if (has_resist_elec(creature_ptr))
                damage = damage / 3;
            if (is_oppose_elec(creature_ptr))
                damage = damage / 3;
            if (creature_ptr->levitation)
                damage = damage / 5;

            damage = damage / 100 + (randint0(100) < (damage % 100));

            if (creature_ptr->levitation) {
                msg_print(_("電撃を受けた！", "The electricity shocks you!"));
                take_hit(creature_ptr, DAMAGE_NOESCAPE, damage,
                    format(_("%sの上に浮遊したダメージ", "flying over %s"),
                        f_name + f_info[get_feat_mimic(&creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x])].name),
                    -1);
            } else {
                concptr name = f_name + f_info[get_feat_mimic(&creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x])].name;
                msg_format(_("%sに感電した！", "The %s shocks you!"), name);
                take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, name, -1);
            }

            cave_no_regen = TRUE;
        }
    }

    if (has_flag(f_ptr->flags, FF_ACID_PUDDLE) && !is_invuln(creature_ptr) && !has_immune_acid(creature_ptr)) {
        int damage = 0;

        if (has_flag(f_ptr->flags, FF_DEEP)) {
            damage = 6000 + randint0(4000);
        } else if (!creature_ptr->levitation) {
            damage = 3000 + randint0(2000);
        }

        if (damage) {
            if (has_resist_acid(creature_ptr))
                damage = damage / 3;
            if (is_oppose_acid(creature_ptr))
                damage = damage / 3;
            if (creature_ptr->levitation)
                damage = damage / 5;

            damage = damage / 100 + (randint0(100) < (damage % 100));

            if (creature_ptr->levitation) {
                msg_print(_("酸が飛び散った！", "The acid melts you!"));
                take_hit(creature_ptr, DAMAGE_NOESCAPE, damage,
                    format(_("%sの上に浮遊したダメージ", "flying over %s"),
                        f_name + f_info[get_feat_mimic(&creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x])].name),
                    -1);
            } else {
                concptr name = f_name + f_info[get_feat_mimic(&creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x])].name;
                msg_format(_("%sに溶かされた！", "The %s melts you!"), name);
                take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, name, -1);
            }

            cave_no_regen = TRUE;
        }
    }

    if (has_flag(f_ptr->flags, FF_POISON_PUDDLE) && !is_invuln(creature_ptr)) {
        int damage = 0;

        if (has_flag(f_ptr->flags, FF_DEEP)) {
            damage = 6000 + randint0(4000);
        } else if (!creature_ptr->levitation) {
            damage = 3000 + randint0(2000);
        }

        if (damage) {
            damage = calc_pois_damage_rate(creature_ptr) / 100;
            if (creature_ptr->levitation)
                damage = damage / 5;

            damage = damage / 100 + (randint0(100) < (damage % 100));

            if (creature_ptr->levitation) {
                msg_print(_("毒気を吸い込んだ！", "The gas poisons you!"));
                take_hit(creature_ptr, DAMAGE_NOESCAPE, damage,
                    format(_("%sの上に浮遊したダメージ", "flying over %s"),
                        f_name + f_info[get_feat_mimic(&creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x])].name),
                    -1);
                if (has_resist_pois(creature_ptr))
                    (void)set_poisoned(creature_ptr, creature_ptr->poisoned + 1);
            } else {
                concptr name = f_name + f_info[get_feat_mimic(&creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x])].name;
                msg_format(_("%sに毒された！", "The %s poisons you!"), name);
                take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, name, -1);
                if (has_resist_pois(creature_ptr))
                    (void)set_poisoned(creature_ptr, creature_ptr->poisoned + 3);
            }

            cave_no_regen = TRUE;
        }
    }

    if (has_flag(f_ptr->flags, FF_WATER) && has_flag(f_ptr->flags, FF_DEEP) && !creature_ptr->levitation && !creature_ptr->can_swim
        && !has_resist_water(creature_ptr)) {
        if (calc_inventory_weight(creature_ptr) > calc_weight_limit(creature_ptr)) {
            msg_print(_("溺れている！", "You are drowning!"));
            take_hit(creature_ptr, DAMAGE_NOESCAPE, randint1(creature_ptr->lev), _("溺れ", "drowning"), -1);
            cave_no_regen = TRUE;
        }
    }

    if (creature_ptr->riding) {
        HIT_POINT damage;
        if ((r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].flags2 & RF2_AURA_FIRE) && !has_immune_fire(creature_ptr)) {
            damage = r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].level / 2;
            if (is_specific_player_race(creature_ptr, RACE_ENT))
                damage += damage / 3;
            if (has_resist_fire(creature_ptr))
                damage = damage / 3;
            if (is_oppose_fire(creature_ptr))
                damage = damage / 3;
            msg_print(_("熱い！", "It's hot!"));
            take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, _("炎のオーラ", "Fire aura"), -1);
        }
        if ((r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].flags2 & RF2_AURA_ELEC) && !has_immune_elec(creature_ptr)) {
            damage = r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].level / 2;
            if (is_specific_player_race(creature_ptr, RACE_ANDROID))
                damage += damage / 3;
            if (has_resist_elec(creature_ptr))
                damage = damage / 3;
            if (is_oppose_elec(creature_ptr))
                damage = damage / 3;
            msg_print(_("痛い！", "It hurts!"));
            take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, _("電気のオーラ", "Elec aura"), -1);
        }
        if ((r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].flags3 & RF3_AURA_COLD) && !has_immune_cold(creature_ptr)) {
            damage = r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].level / 2;
            if (has_resist_cold(creature_ptr))
                damage = damage / 3;
            if (is_oppose_cold(creature_ptr))
                damage = damage / 3;
            msg_print(_("冷たい！", "It's cold!"));
            take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, _("冷気のオーラ", "Cold aura"), -1);
        }
    }

    /* Spectres -- take damage when moving through walls */
    /*
     * Added: ANYBODY takes damage if inside through walls
     * without wraith form -- NOTE: Spectres will never be
     * reduced below 0 hp by being inside a stone wall; others
     * WILL BE!
     */
    if (!has_flag(f_ptr->flags, FF_MOVE) && !has_flag(f_ptr->flags, FF_CAN_FLY)) {
        if (!is_invuln(creature_ptr) && !creature_ptr->wraith_form && !creature_ptr->tim_pass_wall
            && ((creature_ptr->chp > (creature_ptr->lev / 5)) || !has_pass_wall(creature_ptr))) {
            concptr dam_desc;
            cave_no_regen = TRUE;

            if (has_pass_wall(creature_ptr)) {
                msg_print(_("体の分子が分解した気がする！", "Your molecules feel disrupted!"));
                dam_desc = _("密度", "density");
            } else {
                msg_print(_("崩れた岩に押し潰された！", "You are being crushed!"));
                dam_desc = _("硬い岩", "solid rock");
            }

            take_hit(creature_ptr, DAMAGE_NOESCAPE, 1 + (creature_ptr->lev / 5), dam_desc, -1);
        }
    }

    if (creature_ptr->food < PY_FOOD_WEAK) {
        if (creature_ptr->food < PY_FOOD_STARVE) {
            regen_amount = 0;
        } else if (creature_ptr->food < PY_FOOD_FAINT) {
            regen_amount = PY_REGEN_FAINT;
        } else {
            regen_amount = PY_REGEN_WEAK;
        }
    }

    if (pattern_effect(creature_ptr)) {
        cave_no_regen = TRUE;
    } else {
        if (creature_ptr->regenerate) {
            regen_amount = regen_amount * 2;
        }
        if (creature_ptr->special_defense & (KAMAE_MASK | KATA_MASK)) {
            regen_amount /= 2;
        }
        if (creature_ptr->cursed & TRC_SLOW_REGEN) {
            regen_amount /= 5;
        }
    }

    if ((creature_ptr->action == ACTION_SEARCH) || (creature_ptr->action == ACTION_REST)) {
        regen_amount = regen_amount * 2;
    }

    upkeep_factor = calculate_upkeep(creature_ptr);
    if ((creature_ptr->action == ACTION_LEARN) || (creature_ptr->action == ACTION_HAYAGAKE) || (creature_ptr->special_defense & KATA_KOUKIJIN)) {
        upkeep_factor += 100;
    }

    regenmana(creature_ptr, upkeep_factor, regen_amount);
    if (creature_ptr->pclass == CLASS_MAGIC_EATER) {
        regenmagic(creature_ptr, regen_amount);
    }

    if ((creature_ptr->csp == 0) && (creature_ptr->csp_frac == 0)) {
        while (upkeep_factor > 100) {
            msg_print(_("こんなに多くのペットを制御できない！", "Too many pets to control at once!"));
            msg_print(NULL);
            do_cmd_pet_dismiss(creature_ptr);

            upkeep_factor = calculate_upkeep(creature_ptr);

            msg_format(_("維持ＭＰは %d%%", "Upkeep: %d%% mana."), upkeep_factor);
            msg_print(NULL);
        }
    }

    if (creature_ptr->poisoned)
        regen_amount = 0;
    if (creature_ptr->cut)
        regen_amount = 0;
    if (cave_no_regen)
        regen_amount = 0;

    regen_amount = (regen_amount * creature_ptr->mutant_regenerate_mod) / 100;
    if ((creature_ptr->chp < creature_ptr->mhp) && !cave_no_regen) {
        regenhp(creature_ptr, regen_amount);
    }
}

/*
 * Increase players hit points, notice effects
 */
bool hp_player(player_type *creature_ptr, int num)
{
    int vir;
    vir = virtue_number(creature_ptr, V_VITALITY);

    if (num <= 0)
        return FALSE;

    if (vir) {
        num = num * (creature_ptr->virtues[vir - 1] + 1250) / 1250;
    }

    if (creature_ptr->chp < creature_ptr->mhp) {
        if ((num > 0) && (creature_ptr->chp < (creature_ptr->mhp / 3)))
            chg_virtue(creature_ptr, V_TEMPERANCE, 1);

        creature_ptr->chp += num;
        if (creature_ptr->chp >= creature_ptr->mhp) {
            creature_ptr->chp = creature_ptr->mhp;
            creature_ptr->chp_frac = 0;
        }

        creature_ptr->redraw |= (PR_HP);
        creature_ptr->window |= (PW_PLAYER);
        if (num < 5) {
            msg_print(_("少し気分が良くなった。", "You feel a little better."));
        } else if (num < 15) {
            msg_print(_("気分が良くなった。", "You feel better."));
        } else if (num < 35) {
            msg_print(_("とても気分が良くなった。", "You feel much better."));
        } else {
            msg_print(_("ひじょうに気分が良くなった。", "You feel very good."));
        }

        return TRUE;
    }

    return FALSE;
}
