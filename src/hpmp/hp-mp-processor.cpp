#include "hpmp/hp-mp-processor.h"
#include "avatar/avatar.h"
#include "cmd-action/cmd-pet.h"
#include "core/player-redraw-types.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/pattern-walk.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "hpmp/hp-mp-regenerator.h"
#include "inventory/inventory-slot-types.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object/object-flags.h"
#include "pet/pet-util.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "player/player-status-resist.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "status/bad-status-setter.h"
#include "status/element-resistance.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <functional>

/*!
 * @brief 地形によるダメージを与える / Deal damage from feature.
 * @param creature_ptr プレイヤー情報への参照ポインタ
 * @param g_ptr 現在の床の情報への参照ポインタ
 * @param msg_levitation 浮遊時にダメージを受けた場合に表示するメッセージ
 * @param msg_normal 通常時にダメージを受けた場合に表示するメッセージの述部
 * @param 耐性等によるダメージレートを計算する関数
 * @param ダメージを受けた際の追加処理を行う関数
 * @return ダメージを与えたらTRUE、なければFALSE
 * @details
 * ダメージを受けた場合、自然回復できない。
 */
static bool deal_damege_by_feat(player_type *creature_ptr, grid_type *g_ptr, concptr msg_levitation, concptr msg_normal,
    std::function<PERCENTAGE(player_type *)> damage_rate, std::function<void(player_type *, int)> additional_effect)
{
    feature_type *f_ptr = &f_info[g_ptr->feat];
    int damage = 0;

    if (f_ptr->flags.has(FF::DEEP)) {
        damage = 6000 + randint0(4000);
    } else if (!creature_ptr->levitation) {
        damage = 3000 + randint0(2000);
    }

    damage *= damage_rate(creature_ptr);
    damage /= 100;
    if (creature_ptr->levitation)
        damage /= 5;

    damage = damage / 100 + (randint0(100) < (damage % 100));

    if (damage == 0)
        return false;

    if (creature_ptr->levitation) {
        msg_print(msg_levitation);

        take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, format(_("%sの上に浮遊したダメージ", "flying over %s"), f_info[g_ptr->get_feat_mimic()].name.c_str()));

        if (additional_effect != nullptr)
            additional_effect(creature_ptr, damage);
    } else {
        concptr name = f_info[creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].get_feat_mimic()].name.c_str();
        msg_format(_("%s%s！", "The %s %s!"), name, msg_normal);
        take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, name);

        if (additional_effect != nullptr)
            additional_effect(creature_ptr, damage);
    }

    return true;
}

/*!
 * @brief 10ゲームターンが進行するごとにプレイヤーのHPとMPの増減処理を行う。
 *  / Handle timed damage and regeneration every 10 game turns
 */
void process_player_hp_mp(player_type *creature_ptr)
{
    grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x];
    feature_type *f_ptr = &f_info[g_ptr->feat];
    bool cave_no_regen = false;
    int upkeep_factor = 0;
    int regen_amount = PY_REGEN_NORMAL;
    if (creature_ptr->poisoned && !is_invuln(creature_ptr)) {
        if (take_hit(creature_ptr, DAMAGE_NOESCAPE, 1, _("毒", "poison")) > 0) {
            sound(SOUND_DAMAGE_OVER_TIME);
        }
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

        if (take_hit(creature_ptr, DAMAGE_NOESCAPE, dam, _("致命傷", "a fatal wound")) > 0) {
            sound(SOUND_DAMAGE_OVER_TIME);
        }
    }

    if (player_race_life(creature_ptr) == PlayerRaceLife::UNDEAD && player_race_has_flag(creature_ptr, TR_VUL_LITE)) {
        if (!is_in_dungeon(creature_ptr) && !has_resist_lite(creature_ptr) && !is_invuln(creature_ptr) && is_daytime()) {
            if ((creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].info & (CAVE_GLOW | CAVE_MNDK)) == CAVE_GLOW) {
                msg_print(_("日光があなたのアンデッドの肉体を焼き焦がした！", "The sun's rays scorch your undead flesh!"));
                take_hit(creature_ptr, DAMAGE_NOESCAPE, 1, _("日光", "sunlight"));
                cave_no_regen = true;
            }
        }

        object_type *o_ptr;
        o_ptr = &creature_ptr->inventory_list[INVEN_LITE];
        auto flgs = object_flags(o_ptr);

        if (creature_ptr->inventory_list[INVEN_LITE].tval && flgs.has_not(TR_DARK_SOURCE) && !has_resist_lite(creature_ptr)) {
            GAME_TEXT o_name[MAX_NLEN];
            char ouch[MAX_NLEN + 40];
            describe_flavor(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
            msg_format(_("%sがあなたのアンデッドの肉体を焼き焦がした！", "The %s scorches your undead flesh!"), o_name);

            cave_no_regen = true;
            describe_flavor(creature_ptr, o_name, o_ptr, OD_NAME_ONLY);
            sprintf(ouch, _("%sを装備したダメージ", "wielding %s"), o_name);

            if (!is_invuln(creature_ptr))
                take_hit(creature_ptr, DAMAGE_NOESCAPE, 1, ouch);
        }
    }

    if (f_ptr->flags.has(FF::LAVA) && !is_invuln(creature_ptr) && !has_immune_fire(creature_ptr)) {
        if (deal_damege_by_feat(
                creature_ptr, g_ptr, _("熱で火傷した！", "The heat burns you!"), _("で火傷した！", "burns you!"), calc_fire_damage_rate, nullptr)) {
            cave_no_regen = true;
            sound(SOUND_TERRAIN_DAMAGE);
        }
    }

    if (f_ptr->flags.has(FF::COLD_PUDDLE) && !is_invuln(creature_ptr) && !has_immune_cold(creature_ptr)) {
        if (deal_damege_by_feat(
                creature_ptr, g_ptr, _("冷気に覆われた！", "The cold engulfs you!"), _("に凍えた！", "frostbites you!"), calc_cold_damage_rate, nullptr)) {
            cave_no_regen = true;
            sound(SOUND_TERRAIN_DAMAGE);
        }
    }

    if (f_ptr->flags.has(FF::ELEC_PUDDLE) && !is_invuln(creature_ptr) && !has_immune_elec(creature_ptr)) {
        if (deal_damege_by_feat(
                creature_ptr, g_ptr, _("電撃を受けた！", "The electricity shocks you!"), _("に感電した！", "shocks you!"), calc_elec_damage_rate, nullptr)) {
            cave_no_regen = true;
            sound(SOUND_TERRAIN_DAMAGE);
        }
    }

    if (f_ptr->flags.has(FF::ACID_PUDDLE) && !is_invuln(creature_ptr) && !has_immune_acid(creature_ptr)) {
        if (deal_damege_by_feat(
                creature_ptr, g_ptr, _("酸が飛び散った！", "The acid melts you!"), _("に溶かされた！", "melts you!"), calc_acid_damage_rate, nullptr)) {
            cave_no_regen = true;
            sound(SOUND_TERRAIN_DAMAGE);
        }
    }

    if (f_ptr->flags.has(FF::POISON_PUDDLE) && !is_invuln(creature_ptr)) {
        if (deal_damege_by_feat(creature_ptr, g_ptr, _("毒気を吸い込んだ！", "The gas poisons you!"), _("に毒された！", "poisons you!"), calc_acid_damage_rate,
                [](player_type *creature_ptr, int damage) {
                    if (!has_resist_pois(creature_ptr))
                        (void)set_poisoned(creature_ptr, creature_ptr->poisoned + damage);
                })) {
            cave_no_regen = true;
            sound(SOUND_TERRAIN_DAMAGE);
        }
    }

    if (f_ptr->flags.has_all_of({ FF::WATER, FF::DEEP }) && !creature_ptr->levitation && !creature_ptr->can_swim && !has_resist_water(creature_ptr)) {
        if (calc_inventory_weight(creature_ptr) > calc_weight_limit(creature_ptr)) {
            msg_print(_("溺れている！", "You are drowning!"));
            take_hit(creature_ptr, DAMAGE_NOESCAPE, randint1(creature_ptr->lev), _("溺れ", "drowning"));
            cave_no_regen = true;
            sound(SOUND_TERRAIN_DAMAGE);
        }
    }

    if (creature_ptr->riding) {
        HIT_POINT damage;
        if ((r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].flags2 & RF2_AURA_FIRE) && !has_immune_fire(creature_ptr)) {
            damage = r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].level / 2;
            if (player_race_has_flag(creature_ptr, TR_VUL_FIRE))
                damage += damage / 3;
            if (has_resist_fire(creature_ptr))
                damage = damage / 3;
            if (is_oppose_fire(creature_ptr))
                damage = damage / 3;
            msg_print(_("熱い！", "It's hot!"));
            take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, _("炎のオーラ", "Fire aura"));
        }
        if ((r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].flags2 & RF2_AURA_ELEC) && !has_immune_elec(creature_ptr)) {
            damage = r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].level / 2;
            if (player_race_has_flag(creature_ptr, TR_VUL_ELEC))
                damage += damage / 3;
            if (has_resist_elec(creature_ptr))
                damage = damage / 3;
            if (is_oppose_elec(creature_ptr))
                damage = damage / 3;
            msg_print(_("痛い！", "It hurts!"));
            take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, _("電気のオーラ", "Elec aura"));
        }
        if ((r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].flags3 & RF3_AURA_COLD) && !has_immune_cold(creature_ptr)) {
            damage = r_info[creature_ptr->current_floor_ptr->m_list[creature_ptr->riding].r_idx].level / 2;
            if (player_race_has_flag(creature_ptr, TR_VUL_COLD))
                damage += damage / 3;
            if (has_resist_cold(creature_ptr))
                damage = damage / 3;
            if (is_oppose_cold(creature_ptr))
                damage = damage / 3;
            msg_print(_("冷たい！", "It's cold!"));
            take_hit(creature_ptr, DAMAGE_NOESCAPE, damage, _("冷気のオーラ", "Cold aura"));
        }
    }

    /* Spectres -- take damage when moving through walls */
    /*
     * Added: ANYBODY takes damage if inside through walls
     * without wraith form -- NOTE: Spectres will never be
     * reduced below 0 hp by being inside a stone wall; others
     * WILL BE!
     */
    if (f_ptr->flags.has_none_of({ FF::MOVE, FF::CAN_FLY })) {
        if (!is_invuln(creature_ptr) && !creature_ptr->wraith_form && !creature_ptr->tim_pass_wall
            && ((creature_ptr->chp > (creature_ptr->lev / 5)) || !has_pass_wall(creature_ptr))) {
            concptr dam_desc;
            cave_no_regen = true;

            if (has_pass_wall(creature_ptr)) {
                msg_print(_("体の分子が分解した気がする！", "Your molecules feel disrupted!"));
                dam_desc = _("密度", "density");
            } else {
                msg_print(_("崩れた岩に押し潰された！", "You are being crushed!"));
                dam_desc = _("硬い岩", "solid rock");
            }

            take_hit(creature_ptr, DAMAGE_NOESCAPE, 1 + (creature_ptr->lev / 5), dam_desc);
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
        cave_no_regen = true;
    } else {
        if (creature_ptr->regenerate) {
            regen_amount = regen_amount * 2;
        }
        if (creature_ptr->special_defense & (KAMAE_MASK | KATA_MASK)) {
            regen_amount /= 2;
        }
        if (creature_ptr->cursed.has(TRC::SLOW_REGEN)) {
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
            msg_print(nullptr);
            do_cmd_pet_dismiss(creature_ptr);

            upkeep_factor = calculate_upkeep(creature_ptr);

            msg_format(_("維持ＭＰは %d%%", "Upkeep: %d%% mana."), upkeep_factor);
            msg_print(nullptr);
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
        return false;

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
        creature_ptr->window_flags |= (PW_PLAYER);
        if (num < 5) {
            msg_print(_("少し気分が良くなった。", "You feel a little better."));
        } else if (num < 15) {
            msg_print(_("気分が良くなった。", "You feel better."));
        } else if (num < 35) {
            msg_print(_("とても気分が良くなった。", "You feel much better."));
        } else {
            msg_print(_("ひじょうに気分が良くなった。", "You feel very good."));
        }

        return true;
    }

    return false;
}
