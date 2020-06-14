#include "core/hp-mp-processor.h"
#include "cmd-action/cmd-pet.h"
#include "core/hp-mp-regenerator.h"
#include "floor/floor.h"
#include "floor/pattern-walk.h"
#include "grid/feature.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/trc-types.h"
#include "object/object-flavor.h"
#include "player/player-damage.h"
#include "player/player-effects.h"
#include "player/player-race-types.h"
#include "realm/realm-song-numbers.h"
#include "view/display-messages.h"
#include "world/world.h"

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
    if (creature_ptr->poisoned && !IS_INVULN(creature_ptr)) {
        take_hit(creature_ptr, DAMAGE_NOESCAPE, 1, _("毒", "poison"), -1);
    }

    if (creature_ptr->cut && !IS_INVULN(creature_ptr)) {
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
        if (!creature_ptr->current_floor_ptr->dun_level && !creature_ptr->resist_lite && !IS_INVULN(creature_ptr) && is_daytime()) {
            if ((creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].info & (CAVE_GLOW | CAVE_MNDK)) == CAVE_GLOW) {
                msg_print(_("日光があなたのアンデッドの肉体を焼き焦がした！", "The sun's rays scorch your undead flesh!"));
                take_hit(creature_ptr, DAMAGE_NOESCAPE, 1, _("日光", "sunlight"), -1);
                cave_no_regen = TRUE;
            }
        }

        if (creature_ptr->inventory_list[INVEN_LITE].tval && (creature_ptr->inventory_list[INVEN_LITE].name2 != EGO_LITE_DARKNESS)
            && !creature_ptr->resist_lite) {
            object_type *o_ptr = &creature_ptr->inventory_list[INVEN_LITE];
            GAME_TEXT o_name[MAX_NLEN];
            char ouch[MAX_NLEN + 40];
            object_desc(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
            msg_format(_("%sがあなたのアンデッドの肉体を焼き焦がした！", "The %s scorches your undead flesh!"), o_name);

            cave_no_regen = TRUE;
            object_desc(creature_ptr, o_name, o_ptr, OD_NAME_ONLY);
            sprintf(ouch, _("%sを装備したダメージ", "wielding %s"), o_name);

            if (!IS_INVULN(creature_ptr))
                take_hit(creature_ptr, DAMAGE_NOESCAPE, 1, ouch, -1);
        }
    }

    if (have_flag(f_ptr->flags, FF_LAVA) && !IS_INVULN(creature_ptr) && !creature_ptr->immune_fire) {
        int damage = 0;

        if (have_flag(f_ptr->flags, FF_DEEP)) {
            damage = 6000 + randint0(4000);
        } else if (!creature_ptr->levitation) {
            damage = 3000 + randint0(2000);
        }

        if (damage) {
            if (is_specific_player_race(creature_ptr, RACE_ENT))
                damage += damage / 3;
            if (creature_ptr->resist_fire)
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

    if (have_flag(f_ptr->flags, FF_COLD_PUDDLE) && !IS_INVULN(creature_ptr) && !creature_ptr->immune_cold) {
        int damage = 0;

        if (have_flag(f_ptr->flags, FF_DEEP)) {
            damage = 6000 + randint0(4000);
        } else if (!creature_ptr->levitation) {
            damage = 3000 + randint0(2000);
        }

        if (damage) {
            if (creature_ptr->resist_cold)
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

    if (have_flag(f_ptr->flags, FF_ELEC_PUDDLE) && !IS_INVULN(creature_ptr) && !creature_ptr->immune_elec) {
        int damage = 0;

        if (have_flag(f_ptr->flags, FF_DEEP)) {
            damage = 6000 + randint0(4000);
        } else if (!creature_ptr->levitation) {
            damage = 3000 + randint0(2000);
        }

        if (damage) {
            if (creature_ptr->resist_elec)
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

    if (have_flag(f_ptr->flags, FF_ACID_PUDDLE) && !IS_INVULN(creature_ptr) && !creature_ptr->immune_acid) {
        int damage = 0;

        if (have_flag(f_ptr->flags, FF_DEEP)) {
            damage = 6000 + randint0(4000);
        } else if (!creature_ptr->levitation) {
            damage = 3000 + randint0(2000);
        }

        if (damage) {
            if (creature_ptr->resist_acid)
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

    if (have_flag(f_ptr->flags, FF_POISON_PUDDLE) && !IS_INVULN(creature_ptr)) {
        int damage = 0;

        if (have_flag(f_ptr->flags, FF_DEEP)) {
            damage = 6000 + randint0(4000);
        } else if (!creature_ptr->levitation) {
            damage = 3000 + randint0(2000);
        }

        if (damage) {
            if (creature_ptr->resist_pois)
                damage = damage / 3;
            if (is_oppose_pois(creature_ptr))
                damage = damage / 3;
            if (creature_ptr->levitation)
                damage = damage / 5;

            damage = damage / 100 + (randint0(100) < (damage % 100));

            if (creature_ptr->levitation) {
                msg_print(_("毒気を吸い込んだ！", "The gas poisons you!"));
                take_hit(creature_ptr, DAMAGE_NOESCAPE, damage,
                    format(_("%sの上に浮遊したダメージ", "flying over %s"),
                        f_name + f_info[get_feat_mimic(&creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x])].name),
                    -1);
                if (creature_ptr->resist_pois)
                    (void)set_poisoned(creature_ptr, creature_ptr->poisoned + 1);
            } else {
                concptr name = f_name + f_info[get_feat_mimic(&creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x])].name;
                msg_format(_("%sに毒された！", "The %s poisons you!"), name);
                take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, name, -1);
                if (creature_ptr->resist_pois)
                    (void)set_poisoned(creature_ptr, creature_ptr->poisoned + 3);
            }

            cave_no_regen = TRUE;
        }
    }

    if (have_flag(f_ptr->flags, FF_WATER) && have_flag(f_ptr->flags, FF_DEEP) && !creature_ptr->levitation && !creature_ptr->can_swim
        && !creature_ptr->resist_water) {
        if (creature_ptr->total_weight > weight_limit(creature_ptr)) {
            msg_print(_("溺れている！", "You are drowning!"));
            take_hit(creature_ptr, DAMAGE_NOESCAPE, randint1(creature_ptr->lev), _("溺れ", "drowning"), -1);
            cave_no_regen = TRUE;
        }
    }

    if (creature_ptr->riding) {
        HIT_POINT damage;
        if ((r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].flags2 & RF2_AURA_FIRE) && !creature_ptr->immune_fire) {
            damage = r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].level / 2;
            if (is_specific_player_race(creature_ptr, RACE_ENT))
                damage += damage / 3;
            if (creature_ptr->resist_fire)
                damage = damage / 3;
            if (is_oppose_fire(creature_ptr))
                damage = damage / 3;
            msg_print(_("熱い！", "It's hot!"));
            take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, _("炎のオーラ", "Fire aura"), -1);
        }
        if ((r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].flags2 & RF2_AURA_ELEC) && !creature_ptr->immune_elec) {
            damage = r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].level / 2;
            if (is_specific_player_race(creature_ptr, RACE_ANDROID))
                damage += damage / 3;
            if (creature_ptr->resist_elec)
                damage = damage / 3;
            if (is_oppose_elec(creature_ptr))
                damage = damage / 3;
            msg_print(_("痛い！", "It hurts!"));
            take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, _("電気のオーラ", "Elec aura"), -1);
        }
        if ((r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].flags3 & RF3_AURA_COLD) && !creature_ptr->immune_cold) {
            damage = r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].level / 2;
            if (creature_ptr->resist_cold)
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
    if (!have_flag(f_ptr->flags, FF_MOVE) && !have_flag(f_ptr->flags, FF_CAN_FLY)) {
        if (!IS_INVULN(creature_ptr) && !creature_ptr->wraith_form && !creature_ptr->kabenuke
            && ((creature_ptr->chp > (creature_ptr->lev / 5)) || !creature_ptr->pass_wall)) {
            concptr dam_desc;
            cave_no_regen = TRUE;

            if (creature_ptr->pass_wall) {
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
