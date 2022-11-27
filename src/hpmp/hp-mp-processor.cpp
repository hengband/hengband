#include "hpmp/hp-mp-processor.h"
#include "avatar/avatar.h"
#include "cmd-action/cmd-pet.h"
#include "core/player-redraw-types.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/pattern-walk.h"
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
#include "object/tval-types.h"
#include "pet/pet-util.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/monk-data-type.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player-info/samurai-data-type.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "player/player-status-resist.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "status/bad-status-setter.h"
#include "status/element-resistance.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "timed-effect/player-cut.h"
#include "timed-effect/player-poison.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <functional>

/*!
 * @brief 地形によるダメージを与える / Deal damage from feature.
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param g_ptr 現在の床の情報への参照ポインタ
 * @param msg_levitation 浮遊時にダメージを受けた場合に表示するメッセージ
 * @param msg_normal 通常時にダメージを受けた場合に表示するメッセージの述部
 * @param 耐性等によるダメージレートを計算する関数
 * @param ダメージを受けた際の追加処理を行う関数
 * @return ダメージを与えたらTRUE、なければFALSE
 * @details
 * ダメージを受けた場合、自然回復できない。
 */
static bool deal_damege_by_feat(PlayerType *player_ptr, grid_type *g_ptr, concptr msg_levitation, concptr msg_normal,
    std::function<PERCENTAGE(PlayerType *)> damage_rate, std::function<void(PlayerType *, int)> additional_effect)
{
    auto *f_ptr = &terrains_info[g_ptr->feat];
    int damage = 0;

    if (f_ptr->flags.has(TerrainCharacteristics::DEEP)) {
        damage = 6000 + randint0(4000);
    } else if (!player_ptr->levitation) {
        damage = 3000 + randint0(2000);
    }

    damage *= damage_rate(player_ptr);
    damage /= 100;
    if (player_ptr->levitation) {
        damage /= 5;
    }

    damage = damage / 100 + (randint0(100) < (damage % 100));

    if (damage == 0) {
        return false;
    }

    if (player_ptr->levitation) {
        msg_print(msg_levitation);

        take_hit(player_ptr, DAMAGE_NOESCAPE, damage, format(_("%sの上に浮遊したダメージ", "flying over %s"), terrains_info[g_ptr->get_feat_mimic()].name.data()));

        if (additional_effect != nullptr) {
            additional_effect(player_ptr, damage);
        }
    } else {
        concptr name = terrains_info[player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x].get_feat_mimic()].name.data();
        msg_format(_("%s%s！", "The %s %s!"), name, msg_normal);
        take_hit(player_ptr, DAMAGE_NOESCAPE, damage, name);

        if (additional_effect != nullptr) {
            additional_effect(player_ptr, damage);
        }
    }

    return true;
}

/*!
 * @brief 10ゲームターンが進行するごとにプレイヤーのHPとMPの増減処理を行う。
 *  / Handle timed damage and regeneration every 10 game turns
 */
void process_player_hp_mp(PlayerType *player_ptr)
{
    auto &floor_ref = *player_ptr->current_floor_ptr;
    auto *g_ptr = &floor_ref.grid_array[player_ptr->y][player_ptr->x];
    auto *f_ptr = &terrains_info[g_ptr->feat];
    bool cave_no_regen = false;
    int upkeep_factor = 0;
    int regen_amount = PY_REGEN_NORMAL;
    const auto effects = player_ptr->effects();
    const auto player_poison = effects->poison();
    if (player_poison->is_poisoned() && !is_invuln(player_ptr)) {
        if (take_hit(player_ptr, DAMAGE_NOESCAPE, 1, _("毒", "poison")) > 0) {
            sound(SOUND_DAMAGE_OVER_TIME);
        }
    }

    const auto player_cut = effects->cut();
    if (player_cut->is_cut() && !is_invuln(player_ptr)) {
        auto dam = player_cut->get_damage();
        if (take_hit(player_ptr, DAMAGE_NOESCAPE, dam, _("致命傷", "a mortal wound")) > 0) {
            sound(SOUND_DAMAGE_OVER_TIME);
        }
    }

    const PlayerRace race(player_ptr);
    if (race.life() == PlayerRaceLifeType::UNDEAD && race.tr_flags().has(TR_VUL_LITE)) {
        if (!floor_ref.is_in_dungeon() && !has_resist_lite(player_ptr) && !is_invuln(player_ptr) && is_daytime()) {
            if ((floor_ref.grid_array[player_ptr->y][player_ptr->x].info & (CAVE_GLOW | CAVE_MNDK)) == CAVE_GLOW) {
                msg_print(_("日光があなたのアンデッドの肉体を焼き焦がした！", "The sun's rays scorch your undead flesh!"));
                take_hit(player_ptr, DAMAGE_NOESCAPE, 1, _("日光", "sunlight"));
                cave_no_regen = true;
            }
        }

        ItemEntity *o_ptr;
        o_ptr = &player_ptr->inventory_list[INVEN_LITE];
        auto flgs = object_flags(o_ptr);

        if ((player_ptr->inventory_list[INVEN_LITE].tval != ItemKindType::NONE) && flgs.has_not(TR_DARK_SOURCE) && !has_resist_lite(player_ptr)) {
            GAME_TEXT o_name[MAX_NLEN];
            char ouch[MAX_NLEN + 40];
            describe_flavor(player_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
            msg_format(_("%sがあなたのアンデッドの肉体を焼き焦がした！", "The %s scorches your undead flesh!"), o_name);

            cave_no_regen = true;
            describe_flavor(player_ptr, o_name, o_ptr, OD_NAME_ONLY);
            sprintf(ouch, _("%sを装備したダメージ", "wielding %s"), o_name);

            if (!is_invuln(player_ptr)) {
                take_hit(player_ptr, DAMAGE_NOESCAPE, 1, ouch);
            }
        }
    }

    if (f_ptr->flags.has(TerrainCharacteristics::LAVA) && !is_invuln(player_ptr) && !has_immune_fire(player_ptr)) {
        if (deal_damege_by_feat(
                player_ptr, g_ptr, _("熱で火傷した！", "The heat burns you!"), _("で火傷した！", "burns you!"), calc_fire_damage_rate, nullptr)) {
            cave_no_regen = true;
            sound(SOUND_TERRAIN_DAMAGE);
        }
    }

    if (f_ptr->flags.has(TerrainCharacteristics::COLD_PUDDLE) && !is_invuln(player_ptr) && !has_immune_cold(player_ptr)) {
        if (deal_damege_by_feat(
                player_ptr, g_ptr, _("冷気に覆われた！", "The cold engulfs you!"), _("に凍えた！", "frostbites you!"), calc_cold_damage_rate, nullptr)) {
            cave_no_regen = true;
            sound(SOUND_TERRAIN_DAMAGE);
        }
    }

    if (f_ptr->flags.has(TerrainCharacteristics::ELEC_PUDDLE) && !is_invuln(player_ptr) && !has_immune_elec(player_ptr)) {
        if (deal_damege_by_feat(
                player_ptr, g_ptr, _("電撃を受けた！", "The electricity shocks you!"), _("に感電した！", "shocks you!"), calc_elec_damage_rate, nullptr)) {
            cave_no_regen = true;
            sound(SOUND_TERRAIN_DAMAGE);
        }
    }

    if (f_ptr->flags.has(TerrainCharacteristics::ACID_PUDDLE) && !is_invuln(player_ptr) && !has_immune_acid(player_ptr)) {
        if (deal_damege_by_feat(
                player_ptr, g_ptr, _("酸が飛び散った！", "The acid melts you!"), _("に溶かされた！", "melts you!"), calc_acid_damage_rate, nullptr)) {
            cave_no_regen = true;
            sound(SOUND_TERRAIN_DAMAGE);
        }
    }

    if (f_ptr->flags.has(TerrainCharacteristics::POISON_PUDDLE) && !is_invuln(player_ptr)) {
        if (deal_damege_by_feat(player_ptr, g_ptr, _("毒気を吸い込んだ！", "The gas poisons you!"), _("に毒された！", "poisons you!"), calc_acid_damage_rate,
                [](PlayerType *player_ptr, int damage) {
                    if (!has_resist_pois(player_ptr)) {
                        (void)BadStatusSetter(player_ptr).mod_poison(static_cast<TIME_EFFECT>(damage));
                    }
                })) {
            cave_no_regen = true;
            sound(SOUND_TERRAIN_DAMAGE);
        }
    }

    if (f_ptr->flags.has_all_of({ TerrainCharacteristics::WATER, TerrainCharacteristics::DEEP }) && !player_ptr->levitation && !player_ptr->can_swim && !has_resist_water(player_ptr)) {
        if (calc_inventory_weight(player_ptr) > calc_weight_limit(player_ptr)) {
            msg_print(_("溺れている！", "You are drowning!"));
            take_hit(player_ptr, DAMAGE_NOESCAPE, randint1(player_ptr->lev), _("溺れ", "drowning"));
            cave_no_regen = true;
            sound(SOUND_TERRAIN_DAMAGE);
        }
    }

    if (get_player_flags(player_ptr, TR_SELF_FIRE) && !has_immune_fire(player_ptr)) {
        int damage;
        damage = player_ptr->lev;
        if (race.tr_flags().has(TR_VUL_FIRE)) {
            damage += damage / 3;
        }
        if (has_resist_fire(player_ptr)) {
            damage = damage / 3;
        }
        if (is_oppose_fire(player_ptr)) {
            damage = damage / 3;
        }

        damage = std::max(damage, 1);
        msg_print(_("熱い！", "It's hot!"));
        take_hit(player_ptr, DAMAGE_NOESCAPE, damage, _("炎のオーラ", "Fire aura"));
    }

    if (get_player_flags(player_ptr, TR_SELF_ELEC) && !has_immune_elec(player_ptr)) {
        int damage;
        damage = player_ptr->lev;
        if (race.tr_flags().has(TR_VUL_ELEC)) {
            damage += damage / 3;
        }
        if (has_resist_elec(player_ptr)) {
            damage = damage / 3;
        }
        if (is_oppose_elec(player_ptr)) {
            damage = damage / 3;
        }

        damage = std::max(damage, 1);
        msg_print(_("痛い！", "It hurts!"));
        take_hit(player_ptr, DAMAGE_NOESCAPE, damage, _("電気のオーラ", "Elec aura"));
    }

    if (get_player_flags(player_ptr, TR_SELF_COLD) && !has_immune_cold(player_ptr)) {
        int damage;
        damage = player_ptr->lev;
        if (race.tr_flags().has(TR_VUL_COLD)) {
            damage += damage / 3;
        }
        if (has_resist_cold(player_ptr)) {
            damage = damage / 3;
        }
        if (is_oppose_cold(player_ptr)) {
            damage = damage / 3;
        }

        damage = std::max(damage, 1);
        msg_print(_("冷たい！", "It's cold!"));
        take_hit(player_ptr, DAMAGE_NOESCAPE, damage, _("冷気のオーラ", "Cold aura"));
    }

    if (player_ptr->riding) {
        int damage;
        auto auras = monraces_info[floor_ref.m_list[player_ptr->riding].r_idx].aura_flags;
        if (auras.has(MonsterAuraType::FIRE) && !has_immune_fire(player_ptr)) {
            damage = monraces_info[floor_ref.m_list[player_ptr->riding].r_idx].level / 2;
            if (race.tr_flags().has(TR_VUL_FIRE)) {
                damage += damage / 3;
            }
            if (has_resist_fire(player_ptr)) {
                damage = damage / 3;
            }
            if (is_oppose_fire(player_ptr)) {
                damage = damage / 3;
            }

            damage = std::max(damage, 1);
            msg_print(_("熱い！", "It's hot!"));
            take_hit(player_ptr, DAMAGE_NOESCAPE, damage, _("炎のオーラ", "Fire aura"));
        }

        if (auras.has(MonsterAuraType::ELEC) && !has_immune_elec(player_ptr)) {
            damage = monraces_info[floor_ref.m_list[player_ptr->riding].r_idx].level / 2;
            if (race.tr_flags().has(TR_VUL_ELEC)) {
                damage += damage / 3;
            }
            if (has_resist_elec(player_ptr)) {
                damage = damage / 3;
            }
            if (is_oppose_elec(player_ptr)) {
                damage = damage / 3;
            }

            damage = std::max(damage, 1);
            msg_print(_("痛い！", "It hurts!"));
            take_hit(player_ptr, DAMAGE_NOESCAPE, damage, _("電気のオーラ", "Elec aura"));
        }

        if (auras.has(MonsterAuraType::COLD) && !has_immune_cold(player_ptr)) {
            damage = monraces_info[floor_ref.m_list[player_ptr->riding].r_idx].level / 2;
            if (race.tr_flags().has(TR_VUL_COLD)) {
                damage += damage / 3;
            }
            if (has_resist_cold(player_ptr)) {
                damage = damage / 3;
            }
            if (is_oppose_cold(player_ptr)) {
                damage = damage / 3;
            }

            damage = std::max(damage, 1);
            msg_print(_("冷たい！", "It's cold!"));
            take_hit(player_ptr, DAMAGE_NOESCAPE, damage, _("冷気のオーラ", "Cold aura"));
        }
    }

    /* Spectres -- take damage when moving through walls */
    /*
     * Added: ANYBODY takes damage if inside through walls
     * without wraith form -- NOTE: Spectres will never be
     * reduced below 0 hp by being inside a stone wall; others
     * WILL BE!
     */
    if (f_ptr->flags.has_none_of({ TerrainCharacteristics::MOVE, TerrainCharacteristics::CAN_FLY })) {
        if (!is_invuln(player_ptr) && !player_ptr->wraith_form && !player_ptr->tim_pass_wall && ((player_ptr->chp > (player_ptr->lev / 5)) || !has_pass_wall(player_ptr))) {
            concptr dam_desc;
            cave_no_regen = true;

            if (has_pass_wall(player_ptr)) {
                msg_print(_("体の分子が分解した気がする！", "Your molecules feel disrupted!"));
                dam_desc = _("密度", "density");
            } else {
                msg_print(_("崩れた岩に押し潰された！", "You are being crushed!"));
                dam_desc = _("硬い岩", "solid rock");
            }

            take_hit(player_ptr, DAMAGE_NOESCAPE, 1 + (player_ptr->lev / 5), dam_desc);
        }
    }

    if (player_ptr->food < PY_FOOD_WEAK) {
        if (player_ptr->food < PY_FOOD_STARVE) {
            regen_amount = 0;
        } else if (player_ptr->food < PY_FOOD_FAINT) {
            regen_amount = PY_REGEN_FAINT;
        } else {
            regen_amount = PY_REGEN_WEAK;
        }
    }

    PlayerClass pc(player_ptr);
    if (pattern_effect(player_ptr)) {
        cave_no_regen = true;
    } else {
        if (player_ptr->regenerate) {
            regen_amount = regen_amount * 2;
        }

        if (!pc.monk_stance_is(MonkStanceType::NONE) || !pc.samurai_stance_is(SamuraiStanceType::NONE)) {
            regen_amount /= 2;
        }
        if (player_ptr->cursed.has(CurseTraitType::SLOW_REGEN)) {
            regen_amount /= 5;
        }
    }

    if ((player_ptr->action == ACTION_SEARCH) || (player_ptr->action == ACTION_REST)) {
        regen_amount = regen_amount * 2;
    }

    upkeep_factor = calculate_upkeep(player_ptr);
    if ((player_ptr->action == ACTION_LEARN) || (player_ptr->action == ACTION_HAYAGAKE) || pc.samurai_stance_is(SamuraiStanceType::KOUKIJIN)) {
        upkeep_factor += 100;
    }

    regenmana(player_ptr, upkeep_factor, regen_amount);
    if (pc.equals(PlayerClassType::MAGIC_EATER)) {
        regenmagic(player_ptr, regen_amount);
    }

    if ((player_ptr->csp == 0) && (player_ptr->csp_frac == 0)) {
        while (upkeep_factor > 100) {
            msg_print(_("こんなに多くのペットを制御できない！", "Too many pets to control at once!"));
            msg_print(nullptr);
            do_cmd_pet_dismiss(player_ptr);

            upkeep_factor = calculate_upkeep(player_ptr);

            msg_format(_("維持ＭＰは %d%%", "Upkeep: %d%% mana."), upkeep_factor);
            msg_print(nullptr);
        }
    }

    if (player_poison->is_poisoned()) {
        regen_amount = 0;
    }
    if (player_cut->is_cut()) {
        regen_amount = 0;
    }
    if (cave_no_regen) {
        regen_amount = 0;
    }

    regen_amount = (regen_amount * player_ptr->mutant_regenerate_mod) / 100;
    if ((player_ptr->chp < player_ptr->mhp) && !cave_no_regen) {
        regenhp(player_ptr, regen_amount);
    }
}

/*
 * Increase players hit points, notice effects
 */
bool hp_player(PlayerType *player_ptr, int num)
{
    int vir;
    vir = virtue_number(player_ptr, V_VITALITY);

    if (num <= 0) {
        return false;
    }

    if (vir) {
        num = num * (player_ptr->virtues[vir - 1] + 1250) / 1250;
    }

    if (player_ptr->chp < player_ptr->mhp) {
        if ((num > 0) && (player_ptr->chp < (player_ptr->mhp / 3))) {
            chg_virtue(player_ptr, V_TEMPERANCE, 1);
        }

        player_ptr->chp += num;
        if (player_ptr->chp >= player_ptr->mhp) {
            player_ptr->chp = player_ptr->mhp;
            player_ptr->chp_frac = 0;
        }

        player_ptr->redraw |= (PR_HP);
        player_ptr->window_flags |= (PW_PLAYER);
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
