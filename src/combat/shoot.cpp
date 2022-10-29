#include "combat/shoot.h"
#include "artifact/fixed-art-types.h"
#include "avatar/avatar.h"
#include "combat/attack-criticality.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "effect/spells-effect-util.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/geometry.h"
#include "game-option/cheat-types.h"
#include "game-option/special-options.h"
#include "grid/feature-flag-types.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/cursor.h"
#include "io/screen-util.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-sniper.h"
#include "mind/snipe-types.h"
#include "monster-floor/monster-death.h"
#include "monster-floor/monster-move.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster-race/race-resistance-mask.h"
#include "monster/monster-damage.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "object/object-broken.h"
#include "object/object-flags.h"
#include "object/object-info.h"
#include "object/object-mark-types.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/sniper-data-type.h"
#include "player-status/player-energy.h"
#include "player/player-personality-types.h"
#include "player/player-skill.h"
#include "player/player-status-table.h"
#include "sv-definition/sv-bow-types.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem-info-definition.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "wizard/wizard-messages.h"
#include "world/world-object.h"

/*!
 * @brief 矢弾の属性を定義する
 * @param bow_ptr 弓のオブジェクト構造体参照ポインタ
 * @param arrow_ptr 矢弾のオブジェクト構造体参照ポインタ
 * @return スナイパーの射撃属性、弓矢の属性を考慮する。デフォルトはGF_PLAYER_SHOOT。
 */
AttributeFlags shot_attribute(PlayerType *player_ptr, ObjectType *bow_ptr, ObjectType *arrow_ptr, SPELL_IDX snipe_type)
{
    AttributeFlags attribute_flags{};
    attribute_flags.set(AttributeType::PLAYER_SHOOT);

    TrFlags flags{};
    auto arrow_flags = object_flags(arrow_ptr);
    auto bow_flags = object_flags(bow_ptr);

    flags = bow_flags | arrow_flags;

    static const struct snipe_convert_table_t {
        SPELL_IDX snipe_type;
        AttributeType attribute;
    } snipe_convert_table[] = {
        { SP_LITE, AttributeType::LITE },
        { SP_FIRE, AttributeType::FIRE },
        { SP_COLD, AttributeType::COLD },
        { SP_ELEC, AttributeType::ELEC },
        { SP_KILL_WALL, AttributeType::KILL_WALL },
        { SP_EVILNESS, AttributeType::HELL_FIRE },
        { SP_HOLYNESS, AttributeType::HOLY_FIRE },
        { SP_FINAL, AttributeType::MANA },
    };

    static const struct brand_convert_table_t {
        tr_type brand_type;
        AttributeType attribute;
    } brand_convert_table[] = {
        { TR_BRAND_ACID, AttributeType::ACID },
        { TR_BRAND_FIRE, AttributeType::FIRE },
        { TR_BRAND_ELEC, AttributeType::ELEC },
        { TR_BRAND_COLD, AttributeType::COLD },
        { TR_BRAND_POIS, AttributeType::POIS },
        { TR_SLAY_GOOD, AttributeType::HELL_FIRE },
        { TR_KILL_GOOD, AttributeType::HELL_FIRE },
        { TR_SLAY_EVIL, AttributeType::HOLY_FIRE },
        { TR_KILL_EVIL, AttributeType::HOLY_FIRE },
    };

    for (size_t i = 0; i < sizeof(snipe_convert_table) / sizeof(snipe_convert_table[0]); ++i) {
        const struct snipe_convert_table_t *p = &snipe_convert_table[i];

        if (snipe_type == p->snipe_type) {
            attribute_flags.set(p->attribute);
        }
    }

    for (size_t i = 0; i < sizeof(brand_convert_table) / sizeof(brand_convert_table[0]); ++i) {
        const struct brand_convert_table_t *p = &brand_convert_table[i];

        if (flags.has(p->brand_type)) {
            attribute_flags.set(p->attribute);
        }
    }

    if ((flags.has(TR_FORCE_WEAPON)) && (player_ptr->csp > (player_ptr->msp / 30))) {
        attribute_flags.set(AttributeType::MANA);
    }

    return attribute_flags;
}

/*!
 * @brief 矢弾を射撃した際のスレイ倍率をかけた結果を返す /
 * Determines the odds of an object breaking when thrown at a monster
 * @param bow_ptr 弓のオブジェクト構造体参照ポインタ
 * @param arrow_ptr 矢弾のオブジェクト構造体参照ポインタ
 * @param tdam 計算途中のダメージ量
 * @param monster_ptr 目標モンスターの構造体参照ポインタ
 * @return スレイ倍率をかけたダメージ量
 */
static MULTIPLY calc_shot_damage_with_slay(
    PlayerType *player_ptr, ObjectType *bow_ptr, ObjectType *arrow_ptr, int tdam, monster_type *monster_ptr, SPELL_IDX snipe_type)
{
    MULTIPLY mult = 10;

    monster_race *race_ptr = &monraces_info[monster_ptr->r_idx];

    TrFlags flags{};
    auto arrow_flags = object_flags(arrow_ptr);
    auto bow_flags = object_flags(bow_ptr);

    flags = bow_flags | arrow_flags;

    /* Some "weapons" and "ammo" do extra damage */
    switch (arrow_ptr->tval) {
    case ItemKindType::SHOT:
    case ItemKindType::ARROW:
    case ItemKindType::BOLT: {
        if ((flags.has(TR_SLAY_ANIMAL)) && race_ptr->kind_flags.has(MonsterKindType::ANIMAL)) {
            if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                race_ptr->r_kind_flags.set(MonsterKindType::ANIMAL);
            }
            if (mult < 17) {
                mult = 17;
            }
        }

        if ((flags.has(TR_KILL_ANIMAL)) && race_ptr->kind_flags.has(MonsterKindType::ANIMAL)) {
            if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                race_ptr->r_kind_flags.set(MonsterKindType::ANIMAL);
            }
            if (mult < 27) {
                mult = 27;
            }
        }

        if ((flags.has(TR_SLAY_EVIL)) && race_ptr->kind_flags.has(MonsterKindType::EVIL)) {
            if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                race_ptr->r_kind_flags.set(MonsterKindType::EVIL);
            }
            if (mult < 15) {
                mult = 15;
            }
        }

        if ((flags.has(TR_KILL_EVIL)) && race_ptr->kind_flags.has(MonsterKindType::EVIL)) {
            if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                race_ptr->r_kind_flags.set(MonsterKindType::EVIL);
            }
            if (mult < 25) {
                mult = 25;
            }
        }

        if ((flags.has(TR_SLAY_GOOD)) && race_ptr->kind_flags.has(MonsterKindType::GOOD)) {
            if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                race_ptr->r_kind_flags.set(MonsterKindType::GOOD);
            }
            if (mult < 15) {
                mult = 15;
            }
        }

        if ((flags.has(TR_KILL_GOOD)) && race_ptr->kind_flags.has(MonsterKindType::GOOD)) {
            if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                race_ptr->r_kind_flags.set(MonsterKindType::GOOD);
            }
            if (mult < 25) {
                mult = 25;
            }
        }

        if ((flags.has(TR_SLAY_HUMAN)) && race_ptr->kind_flags.has(MonsterKindType::HUMAN)) {
            if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                race_ptr->r_kind_flags.set(MonsterKindType::HUMAN);
            }
            if (mult < 17) {
                mult = 17;
            }
        }

        if ((flags.has(TR_KILL_HUMAN)) && race_ptr->kind_flags.has(MonsterKindType::HUMAN)) {
            if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                race_ptr->r_kind_flags.set(MonsterKindType::HUMAN);
            }
            if (mult < 27) {
                mult = 27;
            }
        }

        if ((flags.has(TR_SLAY_UNDEAD)) && race_ptr->kind_flags.has(MonsterKindType::UNDEAD)) {
            if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                race_ptr->r_kind_flags.set(MonsterKindType::UNDEAD);
            }
            if (mult < 20) {
                mult = 20;
            }
        }

        if ((flags.has(TR_KILL_UNDEAD)) && race_ptr->kind_flags.has(MonsterKindType::UNDEAD)) {
            if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                race_ptr->r_kind_flags.set(MonsterKindType::UNDEAD);
            }
            if (mult < 30) {
                mult = 30;
            }
        }

        if ((flags.has(TR_SLAY_DEMON)) && race_ptr->kind_flags.has(MonsterKindType::DEMON)) {
            if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                race_ptr->r_kind_flags.set(MonsterKindType::DEMON);
            }
            if (mult < 20) {
                mult = 20;
            }
        }

        if ((flags.has(TR_KILL_DEMON)) && race_ptr->kind_flags.has(MonsterKindType::DEMON)) {
            if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                race_ptr->r_kind_flags.set(MonsterKindType::DEMON);
            }
            if (mult < 30) {
                mult = 30;
            }
        }

        if ((flags.has(TR_SLAY_ORC)) && race_ptr->kind_flags.has(MonsterKindType::ORC)) {
            if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                race_ptr->r_kind_flags.set(MonsterKindType::ORC);
            }
            if (mult < 20) {
                mult = 20;
            }
        }

        if ((flags.has(TR_KILL_ORC)) && race_ptr->kind_flags.has(MonsterKindType::ORC)) {
            if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                race_ptr->r_kind_flags.set(MonsterKindType::ORC);
            }
            if (mult < 30) {
                mult = 30;
            }
        }

        if ((flags.has(TR_SLAY_TROLL)) && race_ptr->kind_flags.has(MonsterKindType::TROLL)) {
            if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                race_ptr->r_kind_flags.set(MonsterKindType::TROLL);
            }

            if (mult < 20) {
                mult = 20;
            }
        }

        if ((flags.has(TR_KILL_TROLL)) && race_ptr->kind_flags.has(MonsterKindType::TROLL)) {
            if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                race_ptr->r_kind_flags.set(MonsterKindType::TROLL);
            }
            if (mult < 30) {
                mult = 30;
            }
        }

        if ((flags.has(TR_SLAY_GIANT)) && race_ptr->kind_flags.has(MonsterKindType::GIANT)) {
            if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                race_ptr->r_kind_flags.set(MonsterKindType::GIANT);
            }
            if (mult < 20) {
                mult = 20;
            }
        }

        if ((flags.has(TR_KILL_GIANT)) && race_ptr->kind_flags.has(MonsterKindType::GIANT)) {
            if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                race_ptr->r_kind_flags.set(MonsterKindType::GIANT);
            }
            if (mult < 30) {
                mult = 30;
            }
        }

        if ((flags.has(TR_SLAY_DRAGON)) && race_ptr->kind_flags.has(MonsterKindType::DRAGON)) {
            if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                race_ptr->r_kind_flags.set(MonsterKindType::DRAGON);
            }
            if (mult < 20) {
                mult = 20;
            }
        }

        if ((flags.has(TR_KILL_DRAGON)) && race_ptr->kind_flags.has(MonsterKindType::DRAGON)) {
            if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                race_ptr->r_kind_flags.set(MonsterKindType::DRAGON);
            }
            if (mult < 30) {
                mult = 30;
            }

            auto can_eliminate_smaug = arrow_ptr->is_specific_artifact(FixedArtifactId::BARD_ARROW);
            can_eliminate_smaug &= player_ptr->inventory_list[INVEN_BOW].is_specific_artifact(FixedArtifactId::BARD);
            can_eliminate_smaug &= monster_ptr->r_idx == MonsterRaceId::SMAUG;
            if (can_eliminate_smaug) {
                mult *= 5;
            }
        }

        if (flags.has(TR_BRAND_ACID)) {
            /* Notice immunity */
            if (race_ptr->resistance_flags.has_any_of(RFR_EFF_IM_ACID_MASK)) {
                if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                    race_ptr->r_resistance_flags.set(race_ptr->resistance_flags & RFR_EFF_IM_ACID_MASK);
                }
            } else {
                if (mult < 17) {
                    mult = 17;
                }
            }
        }

        if (flags.has(TR_BRAND_ELEC)) {
            /* Notice immunity */
            if (race_ptr->resistance_flags.has_any_of(RFR_EFF_IM_ELEC_MASK)) {
                if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                    race_ptr->r_resistance_flags.set(race_ptr->resistance_flags & RFR_EFF_IM_ELEC_MASK);
                }
            } else {
                if (mult < 17) {
                    mult = 17;
                }
            }
        }

        if (flags.has(TR_BRAND_FIRE)) {
            /* Notice immunity */
            if (race_ptr->resistance_flags.has_any_of(RFR_EFF_IM_FIRE_MASK)) {
                if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                    race_ptr->r_resistance_flags.set(race_ptr->resistance_flags & RFR_EFF_IM_FIRE_MASK);
                }
            }
            /* Otherwise, take the damage */
            else {
                if (race_ptr->resistance_flags.has(MonsterResistanceType::HURT_FIRE)) {
                    if (mult < 25) {
                        mult = 25;
                    }
                    if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                        race_ptr->r_resistance_flags.set(MonsterResistanceType::HURT_FIRE);
                    }
                } else if (mult < 17) {
                    mult = 17;
                }
            }
        }

        if (flags.has(TR_BRAND_COLD)) {
            /* Notice immunity */
            if (race_ptr->resistance_flags.has_any_of(RFR_EFF_IM_COLD_MASK)) {
                if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                    race_ptr->r_resistance_flags.set(race_ptr->resistance_flags & RFR_EFF_IM_COLD_MASK);
                }
            }
            /* Otherwise, take the damage */
            else {
                if (race_ptr->resistance_flags.has(MonsterResistanceType::HURT_COLD)) {
                    if (mult < 25) {
                        mult = 25;
                    }
                    if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                        race_ptr->r_resistance_flags.set(MonsterResistanceType::HURT_COLD);
                    }
                } else if (mult < 17) {
                    mult = 17;
                }
            }
        }

        if (flags.has(TR_BRAND_POIS)) {
            /* Notice immunity */
            if (race_ptr->resistance_flags.has_any_of(RFR_EFF_IM_POISON_MASK)) {
                if (is_original_ap_and_seen(player_ptr, monster_ptr)) {
                    race_ptr->r_resistance_flags.set(race_ptr->resistance_flags & RFR_EFF_IM_POISON_MASK);
                }
            }
            /* Otherwise, take the damage */
            else {
                if (mult < 17) {
                    mult = 17;
                }
            }
        }

        if ((flags.has(TR_FORCE_WEAPON)) && (player_ptr->csp > (player_ptr->msp / 30))) {
            player_ptr->csp -= (1 + (player_ptr->msp / 30));
            set_bits(player_ptr->redraw, PR_MANA);
            mult = mult * 5 / 2;
        }
        break;
    }

    default:
        break;
    }

    /* Sniper */
    if (snipe_type) {
        mult = calc_snipe_damage_with_slay(player_ptr, mult, monster_ptr, snipe_type);
    }

    /* Return the total damage */
    return tdam * mult / 10;
}

/*!
 * @brief 射撃処理実行 /
 * Fire an object from the pack or floor.
 * @param item 射撃するオブジェクトの所持ID
 * @param bow_ptr 射撃武器のオブジェクト参照ポインタ
 * @details
 * <pre>
 * You may only fire items that "match" your missile launcher.
 * You must use slings + pebbles/shots, bows + arrows, xbows + bolts.
 * See "calc_bonuses()" for more calculations and such.
 * Note that "firing" a missile is MUCH better than "throwing" it.
 * Note: "unseen" monsters are very hard to hit.
 * Objects are more likely to break if they "attempt" to hit a monster.
 * Rangers (with Bows) and Anyone (with "Extra Shots") get extra shots.
 * The "extra shot" code works by decreasing the amount of energy
 * required to make each shot, spreading the shots out over time.
 * Note that when firing missiles, the launcher multiplier is applied
 * after all the bonuses are added in, making multipliers very useful.
 * Note that Bows of "Extra Might" get extra range and an extra bonus
 * for the damage multiplier.
 * Note that Bows of "Extra Shots" give an extra shot.
 * </pre>
 */
void exe_fire(PlayerType *player_ptr, INVENTORY_IDX item, ObjectType *j_ptr, SPELL_IDX snipe_type)
{
    DIRECTION dir;
    int i;
    POSITION y, x, ny, nx, ty, tx, prev_y, prev_x;
    int tdam_base, tdis, thits, tmul;
    int bonus, chance;
    int cur_dis, visible;
    PERCENTAGE j;

    ObjectType forge;
    ObjectType *q_ptr;
    ObjectType *o_ptr;

    AttributeFlags attribute_flags{};
    attribute_flags.set(AttributeType::PLAYER_SHOOT);

    bool hit_body = false;

    GAME_TEXT o_name[MAX_NLEN];

    /* STICK TO */
    bool stick_to = false;

    /* Access the item (if in the pack) */
    if (item >= 0) {
        o_ptr = &player_ptr->inventory_list[item];
    } else {
        o_ptr = &player_ptr->current_floor_ptr->o_list[0 - item];
    }

    /* Sniper - Cannot shot a single arrow twice */
    if ((snipe_type == SP_DOUBLE) && (o_ptr->number < 2)) {
        snipe_type = SP_NONE;
    }

    describe_flavor(player_ptr, o_name, o_ptr, OD_OMIT_PREFIX);

    /* Use the proper number of shots */
    thits = player_ptr->num_fire;

    /* Use a base distance */
    tdis = 10;

    /* Base damage from thrown object plus launcher bonus */
    tdam_base = damroll(o_ptr->dd, o_ptr->ds) + o_ptr->to_d + j_ptr->to_d;

    /* Actually "fire" the object */
    bonus = (player_ptr->to_h_b + o_ptr->to_h + j_ptr->to_h);
    if ((j_ptr->sval == SV_LIGHT_XBOW) || (j_ptr->sval == SV_HEAVY_XBOW)) {
        chance = (player_ptr->skill_thb + (player_ptr->weapon_exp[j_ptr->tval][j_ptr->sval] / 400 + bonus) * BTH_PLUS_ADJ);
    } else {
        chance = (player_ptr->skill_thb + ((player_ptr->weapon_exp[j_ptr->tval][j_ptr->sval] - (PlayerSkill::weapon_exp_at(PlayerSkillRank::MASTER) / 2)) / 200 + bonus) * BTH_PLUS_ADJ);
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(bow_energy(j_ptr->sval));
    tmul = bow_tmul(j_ptr->sval);

    /* Get extra "power" from "extra might" */
    if (player_ptr->xtra_might) {
        tmul++;
    }

    tmul = tmul * (100 + (int)(adj_str_td[player_ptr->stat_index[A_STR]]) - 128);

    /* Boost the damage */
    tdam_base *= tmul;
    tdam_base /= 100;

    auto sniper_data = PlayerClass(player_ptr).get_specific_data<sniper_data_type>();
    auto sniper_concent = sniper_data ? sniper_data->concent : 0;

    /* Base range */
    tdis = 13 + tmul / 80;
    if ((j_ptr->sval == SV_LIGHT_XBOW) || (j_ptr->sval == SV_HEAVY_XBOW)) {
        tdis -= (5 - (sniper_concent + 1) / 2);
    }

    project_length = tdis + 1;

    /* Get a direction (or cancel) */
    if (!get_aim_dir(player_ptr, &dir)) {
        PlayerEnergy(player_ptr).reset_player_turn();

        if (snipe_type == SP_AWAY) {
            snipe_type = SP_NONE;
        }

        /* need not to reset project_length (already did)*/

        return;
    }

    if (snipe_type != SP_NONE) {
        sound(SOUND_ZAP);
    }

    /* Predict the "target" location */
    tx = player_ptr->x + 99 * ddx[dir];
    ty = player_ptr->y + 99 * ddy[dir];

    /* Check for "target request" */
    if ((dir == 5) && target_okay(player_ptr)) {
        tx = target_col;
        ty = target_row;
    }

    /* Get projection path length */
    projection_path path_g(player_ptr, project_length, player_ptr->y, player_ptr->x, ty, tx, PROJECT_PATH | PROJECT_THRU);
    tdis = path_g.path_num() - 1;

    project_length = 0; /* reset to default */

    /* Don't shoot at my feet */
    if (tx == player_ptr->x && ty == player_ptr->y) {
        PlayerEnergy(player_ptr).reset_player_turn();

        /* project_length is already reset to 0 */

        return;
    }

    /* Take a (partial) turn */
    PlayerEnergy(player_ptr).div_player_turn_energy((ENERGY)thits);
    player_ptr->is_fired = true;

    /* Sniper - Difficult to shot twice at 1 turn */
    if (snipe_type == SP_DOUBLE) {
        sniper_concent = (sniper_concent + 1) / 2;
    }

    /* Sniper - Repeat shooting when double shots */
    for (i = 0; i < ((snipe_type == SP_DOUBLE) ? 2 : 1); i++) {
        /* Start at the player */
        y = player_ptr->y;
        x = player_ptr->x;
        q_ptr = &forge;
        q_ptr->copy_from(o_ptr);

        /* Single object */
        q_ptr->number = 1;

        vary_item(player_ptr, item, -1);

        sound(SOUND_SHOOT);
        handle_stuff(player_ptr);

        prev_y = y;
        prev_x = x;

        /* The shot does not hit yet */
        hit_body = false;

        /* Travel until stopped */
        for (cur_dis = 0; cur_dis <= tdis;) {
            grid_type *g_ptr;

            /* Hack -- Stop at the target */
            if ((y == ty) && (x == tx)) {
                break;
            }

            /* Calculate the new location (see "project()") */
            ny = y;
            nx = x;
            mmove2(&ny, &nx, player_ptr->y, player_ptr->x, ty, tx);

            /* Shatter Arrow */
            if (snipe_type == SP_KILL_WALL) {
                g_ptr = &player_ptr->current_floor_ptr->grid_array[ny][nx];

                if (g_ptr->cave_has_flag(TerrainCharacteristics::HURT_ROCK) && !g_ptr->m_idx) {
                    if (any_bits(g_ptr->info, (CAVE_MARK))) {
                        msg_print(_("岩が砕け散った。", "Wall rocks were shattered."));
                    }
                    /* Forget the wall */
                    reset_bits(g_ptr->info, (CAVE_MARK));
                    set_bits(player_ptr->update, PU_VIEW | PU_LITE | PU_FLOW | PU_MON_LITE);

                    /* Destroy the wall */
                    cave_alter_feat(player_ptr, ny, nx, TerrainCharacteristics::HURT_ROCK);

                    hit_body = true;
                    break;
                }
            }

            /* Stopped by walls/doors */
            if (!cave_has_flag_bold(player_ptr->current_floor_ptr, ny, nx, TerrainCharacteristics::PROJECT) && !player_ptr->current_floor_ptr->grid_array[ny][nx].m_idx) {
                break;
            }

            /* Advance the distance */
            cur_dis++;

            /* Sniper */
            if (snipe_type == SP_LITE) {
                set_bits(player_ptr->current_floor_ptr->grid_array[ny][nx].info, CAVE_GLOW);
                note_spot(player_ptr, ny, nx);
                lite_spot(player_ptr, ny, nx);
            }

            /* The player can see the (on screen) missile */
            if (panel_contains(ny, nx) && player_can_see_bold(player_ptr, ny, nx)) {
                const auto a = q_ptr->get_color();
                const auto c = q_ptr->get_symbol();

                /* Draw, Hilite, Fresh, Pause, Erase */
                if (delay_factor > 0) {
                    print_rel(player_ptr, c, a, ny, nx);
                    move_cursor_relative(ny, nx);
                    term_fresh();
                    term_xtra(TERM_XTRA_DELAY, delay_factor);
                    lite_spot(player_ptr, ny, nx);
                    term_fresh();
                }
            }

            /* The player cannot see the missile */
            else {
                /* Pause anyway, for consistancy **/
                if (delay_factor > 0) {
                    term_xtra(TERM_XTRA_DELAY, delay_factor);
                }
            }

            /* Sniper */
            if (snipe_type == SP_KILL_TRAP) {
                project(player_ptr, 0, 0, ny, nx, 0, AttributeType::KILL_TRAP, (PROJECT_JUMP | PROJECT_HIDE | PROJECT_GRID | PROJECT_ITEM));
            }

            /* Sniper */
            if (snipe_type == SP_EVILNESS) {
                reset_bits(player_ptr->current_floor_ptr->grid_array[ny][nx].info, (CAVE_GLOW | CAVE_MARK));
                note_spot(player_ptr, ny, nx);
                lite_spot(player_ptr, ny, nx);
            }

            prev_y = y;
            prev_x = x;

            /* Save the new location */
            x = nx;
            y = ny;

            /* Monster here, Try to hit it */
            if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                sound(SOUND_SHOOT_HIT);
                grid_type *c_mon_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];

                auto *m_ptr = &player_ptr->current_floor_ptr->m_list[c_mon_ptr->m_idx];
                auto *r_ptr = &monraces_info[m_ptr->r_idx];

                /* Check the visibility */
                visible = m_ptr->ml;

                /* Note the collision */
                hit_body = true;

                if (m_ptr->is_asleep()) {
                    if (r_ptr->kind_flags.has_not(MonsterKindType::EVIL) || one_in_(5)) {
                        chg_virtue(player_ptr, V_COMPASSION, -1);
                    }
                    if (r_ptr->kind_flags.has_not(MonsterKindType::EVIL) || one_in_(5)) {
                        chg_virtue(player_ptr, V_HONOUR, -1);
                    }
                }

                if ((r_ptr->level + 10) > player_ptr->lev) {
                    PlayerSkill(player_ptr).gain_range_weapon_exp(j_ptr);
                }

                if (player_ptr->riding) {
                    PlayerSkill(player_ptr).gain_riding_skill_exp_on_range_attack();
                }

                /* Did we hit it (penalize range) */
                if (test_hit_fire(player_ptr, chance - cur_dis, m_ptr, m_ptr->ml, o_name)) {
                    bool fear = false;
                    auto tdam = tdam_base; //!< @note 実際に与えるダメージ
                    auto base_dam = tdam; //!< @note 補正前の与えるダメージ(無傷、全ての耐性など)

                    /* Get extra damage from concentration */
                    tdam = boost_concentration_damage(player_ptr, tdam);

                    /* Handle unseen monster */
                    if (!visible) {
                        /* Invisible monster */
                        msg_format(_("%sが敵を捕捉した。", "The %s finds a mark."), o_name);
                    }

                    /* Handle visible monster */
                    else {
                        GAME_TEXT m_name[MAX_NLEN];

                        /* Get "the monster" or "it" */
                        monster_desc(player_ptr, m_name, m_ptr, 0);

                        msg_format(_("%sが%sに命中した。", "The %s hits %s."), o_name, m_name);

                        if (m_ptr->ml) {
                            if (!player_ptr->effects()->hallucination()->is_hallucinated()) {
                                monster_race_track(player_ptr, m_ptr->ap_r_idx);
                            }

                            health_track(player_ptr, c_mon_ptr->m_idx);
                        }
                    }

                    if (snipe_type == SP_NEEDLE) {
                        if ((randint1(randint1(r_ptr->level / (3 + sniper_concent)) + (8 - sniper_concent)) == 1) && r_ptr->kind_flags.has_not(MonsterKindType::UNIQUE) && none_bits(r_ptr->flags7, RF7_UNIQUE2)) {
                            GAME_TEXT m_name[MAX_NLEN];

                            /* Get "the monster" or "it" */
                            monster_desc(player_ptr, m_name, m_ptr, 0);

                            tdam = m_ptr->hp + 1;
                            base_dam = tdam;
                            msg_format(_("%sの急所に突き刺さった！", "Your shot hit a fatal spot of %s!"), m_name);
                        } else {
                            tdam = 1;
                            base_dam = tdam;
                        }
                    } else {

                        attribute_flags = shot_attribute(player_ptr, j_ptr, q_ptr, snipe_type);
                        /* Apply special damage */
                        tdam = calc_shot_damage_with_slay(player_ptr, j_ptr, q_ptr, tdam, m_ptr, snipe_type);
                        tdam = critical_shot(player_ptr, q_ptr->weight, q_ptr->to_h, j_ptr->to_h, tdam);

                        /* No negative damage */
                        if (tdam < 0) {
                            tdam = 0;
                        }

                        /* Modify the damage */
                        base_dam = tdam;
                        tdam = mon_damage_mod(player_ptr, m_ptr, tdam, false);
                    }

                    msg_format_wizard(player_ptr, CHEAT_MONSTER, _("%dのダメージを与えた。(残りHP %d/%d(%d))", "You do %d damage. (left HP %d/%d(%d))"), tdam,
                        m_ptr->hp - tdam, m_ptr->maxhp, m_ptr->max_maxhp);

                    /* Sniper */
                    if (snipe_type == SP_EXPLODE) {
                        uint16_t flg = (PROJECT_STOP | PROJECT_JUMP | PROJECT_KILL | PROJECT_GRID);

                        sound(SOUND_EXPLODE); /* No explode sound - use breath fire instead */
                        project(player_ptr, 0, ((sniper_concent + 1) / 2 + 1), ny, nx, base_dam, AttributeType::MISSILE, flg);
                        break;
                    }

                    /* Sniper */
                    if (snipe_type == SP_HOLYNESS) {
                        set_bits(player_ptr->current_floor_ptr->grid_array[ny][nx].info, CAVE_GLOW);
                        note_spot(player_ptr, ny, nx);
                        lite_spot(player_ptr, ny, nx);
                    }

                    /* Hit the monster, check for death */
                    MonsterDamageProcessor mdp(player_ptr, c_mon_ptr->m_idx, tdam, &fear, attribute_flags);
                    if (mdp.mon_take_hit(extract_note_dies(m_ptr->get_real_r_idx()))) {
                        /* Dead monster */
                    }

                    /* No death */
                    else {
                        /* STICK TO */
                        if (q_ptr->is_fixed_artifact() && (sniper_concent == 0)) {
                            GAME_TEXT m_name[MAX_NLEN];

                            monster_desc(player_ptr, m_name, m_ptr, 0);

                            stick_to = true;
                            msg_format(_("%sは%sに突き刺さった！", "%^s is stuck in %s!"), o_name, m_name);
                        }

                        message_pain(player_ptr, c_mon_ptr->m_idx, tdam);

                        /* Anger the monster */
                        if (tdam > 0) {
                            anger_monster(player_ptr, m_ptr);
                        }

                        if (fear && m_ptr->ml) {
                            GAME_TEXT m_name[MAX_NLEN];
                            sound(SOUND_FLEE);
                            monster_desc(player_ptr, m_name, m_ptr, 0);
                            msg_format(_("%^sは恐怖して逃げ出した！", "%^s flees in terror!"), m_name);
                        }

                        set_target(m_ptr, player_ptr->y, player_ptr->x);

                        /* Sniper */
                        if (snipe_type == SP_RUSH) {
                            int n = randint1(5) + 3;
                            MONSTER_IDX m_idx = c_mon_ptr->m_idx;

                            for (; cur_dis <= tdis;) {
                                POSITION ox = nx;
                                POSITION oy = ny;

                                if (!n) {
                                    break;
                                }

                                /* Calculate the new location (see "project()") */
                                mmove2(&ny, &nx, player_ptr->y, player_ptr->x, ty, tx);

                                /* Stopped by wilderness boundary */
                                if (!in_bounds2(player_ptr->current_floor_ptr, ny, nx)) {
                                    break;
                                }

                                /* Stopped by walls/doors */
                                if (!player_can_enter(player_ptr, player_ptr->current_floor_ptr->grid_array[ny][nx].feat, 0)) {
                                    break;
                                }

                                /* Stopped by monsters */
                                if (!is_cave_empty_bold(player_ptr, ny, nx)) {
                                    break;
                                }

                                player_ptr->current_floor_ptr->grid_array[ny][nx].m_idx = m_idx;
                                player_ptr->current_floor_ptr->grid_array[oy][ox].m_idx = 0;

                                m_ptr->fx = nx;
                                m_ptr->fy = ny;

                                update_monster(player_ptr, m_idx, true);

                                if (delay_factor > 0) {
                                    lite_spot(player_ptr, ny, nx);
                                    lite_spot(player_ptr, oy, ox);
                                    term_fresh();
                                    term_xtra(TERM_XTRA_DELAY, delay_factor);
                                }

                                x = nx;
                                y = ny;
                                cur_dis++;
                                n--;
                            }
                        }
                    }
                }

                /* Sniper */
                if (snipe_type == SP_PIERCE && sniper_concent > 0) {
                    sniper_concent--;
                    continue;
                }

                /* Stop looking */
                break;
            }
        }

        /* Chance of breakage (during attacks) */
        j = (hit_body ? breakage_chance(player_ptr, q_ptr, PlayerClass(player_ptr).equals(PlayerClassType::ARCHER), snipe_type) : 0);

        if (stick_to) {
            MONSTER_IDX m_idx = player_ptr->current_floor_ptr->grid_array[y][x].m_idx;
            auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
            OBJECT_IDX o_idx = o_pop(player_ptr->current_floor_ptr);

            if (!o_idx) {
                msg_format(_("%sはどこかへ行った。", "The %s went somewhere."), o_name);
                if (q_ptr->is_fixed_artifact()) {
                    artifacts_info.at(j_ptr->fixed_artifact_idx).is_generated = false;
                }
                return;
            }

            o_ptr = &player_ptr->current_floor_ptr->o_list[o_idx];
            o_ptr->copy_from(q_ptr);

            /* Forget mark */
            reset_bits(o_ptr->marked, OM_TOUCHED);

            /* Forget location */
            o_ptr->iy = o_ptr->ix = 0;

            /* Memorize monster */
            o_ptr->held_m_idx = m_idx;

            /* Carry object */
            m_ptr->hold_o_idx_list.add(player_ptr->current_floor_ptr, o_idx);
        } else if (cave_has_flag_bold(player_ptr->current_floor_ptr, y, x, TerrainCharacteristics::PROJECT)) {
            /* Drop (or break) near that location */
            (void)drop_near(player_ptr, q_ptr, j, y, x);
        } else {
            /* Drop (or break) near that location */
            (void)drop_near(player_ptr, q_ptr, j, prev_y, prev_x);
        }

        /* Sniper - Repeat shooting when double shots */
    }

    /* Sniper - Loose his/her concentration after any shot */
    reset_concentration(player_ptr, false);
}

/*!
 * @brief プレイヤーからモンスターへの射撃命中判定 /
 * Determine if the player "hits" a monster (normal combat).
 * @param chance 基本命中値
 * @param monster_ptr モンスターの構造体参照ポインタ
 * @param vis 目標を視界に捕らえているならばTRUEを指定
 * @param o_name メッセージ表示時のモンスター名
 * @return 命中と判定された場合TRUEを返す
 * @note Always miss 5%, always hit 5%, otherwise random.
 */
bool test_hit_fire(PlayerType *player_ptr, int chance, monster_type *m_ptr, int vis, char *o_name)
{
    int k;
    ARMOUR_CLASS ac;
    auto *r_ptr = &monraces_info[m_ptr->r_idx];

    /* Percentile dice */
    k = randint1(100);

    auto sniper_data = PlayerClass(player_ptr).get_specific_data<sniper_data_type>();
    auto sniper_concent = sniper_data ? sniper_data->concent : 0;

    /* Snipers with high-concentration reduce instant miss percentage.*/
    k += sniper_concent;

    /* Hack -- Instant miss or hit */
    if (k <= 5) {
        return false;
    }
    if (k > 95) {
        return true;
    }

    if (player_ptr->ppersonality == PERSONALITY_LAZY) {
        if (one_in_(20)) {
            return false;
        }
    }

    /* Never hit */
    if (chance <= 0) {
        return false;
    }

    ac = r_ptr->ac;
    ac = ac * (8 - sniper_concent) / 8;

    if (m_ptr->r_idx == MonsterRaceId::GOEMON && !m_ptr->is_asleep()) {
        ac *= 3;
    }

    /* Invisible monsters are harder to hit */
    if (!vis) {
        chance = (chance + 1) / 2;
    }

    /* Power competes against armor */
    if (randint0(chance) < (ac * 3 / 4)) {
        if (m_ptr->r_idx == MonsterRaceId::GOEMON && !m_ptr->is_asleep()) {
            GAME_TEXT m_name[MAX_NLEN];
            monster_desc(player_ptr, m_name, m_ptr, 0);
            msg_format(_("%sは%sを斬り捨てた！", "%s cuts down %s!"), m_name, o_name);
        }
        return false;
    }

    /* Assume hit */
    return true;
}

/*!
 * @brief プレイヤーからモンスターへの射撃クリティカル判定 /
 * Critical hits (from objects thrown by player) Factor in item weight, total plusses, and player level.
 * @param weight 矢弾の重量
 * @param plus_ammo 矢弾の命中修正
 * @param plus_bow 弓の命中修正
 * @param dam 現在算出中のダメージ値
 * @return クリティカル修正が入ったダメージ値
 */
int critical_shot(PlayerType *player_ptr, WEIGHT weight, int plus_ammo, int plus_bow, int dam)
{
    int i, k;
    auto *j_ptr = &player_ptr->inventory_list[INVEN_BOW];

    /* Extract "shot" power */
    i = player_ptr->to_h_b + plus_ammo;

    if (player_ptr->tval_ammo == ItemKindType::BOLT) {
        i = (player_ptr->skill_thb + (player_ptr->weapon_exp[j_ptr->tval][j_ptr->sval] / 400 + i) * BTH_PLUS_ADJ);
    } else {
        i = (player_ptr->skill_thb + ((player_ptr->weapon_exp[j_ptr->tval][j_ptr->sval] - (PlayerSkill::weapon_exp_at(PlayerSkillRank::MASTER) / 2)) / 200 + i) * BTH_PLUS_ADJ);
    }

    PlayerClass pc(player_ptr);
    auto sniper_data = pc.get_specific_data<sniper_data_type>();
    auto sniper_concent = sniper_data ? sniper_data->concent : 0;

    /* Snipers can shot more critically with crossbows */
    i += ((i * sniper_concent) / 5);
    if (pc.equals(PlayerClassType::SNIPER) && (player_ptr->tval_ammo == ItemKindType::BOLT)) {
        i *= 2;
    }

    /* Good bow makes more critical */
    i += plus_bow * 8 * (sniper_concent + 5);

    /* Critical hit */
    if (randint1(10000) <= i) {
        k = weight * randint1(500);

        if (k < 900) {
            msg_print(_("手ごたえがあった！", "It was a good hit!"));
            dam += (dam / 2);
        } else if (k < 1350) {
            msg_print(_("かなりの手ごたえがあった！", "It was a great hit!"));
            dam *= 2;
        } else {
            msg_print(_("会心の一撃だ！", "It was a superb hit!"));
            dam *= 3;
        }
    }

    return dam;
}

/*!
 * @brief 射撃武器の攻撃に必要な基本消費エネルギーを返す/Return bow energy
 * @param sval 射撃武器のアイテム副分類ID
 * @return 消費する基本エネルギー
 */
ENERGY bow_energy(OBJECT_SUBTYPE_VALUE sval)
{
    ENERGY energy = 10000;

    /* Analyze the launcher */
    switch (sval) {
        /* Sling and ammo */
    case SV_SLING: {
        energy = 8000;
        break;
    }

    /* Short Bow and Arrow */
    case SV_SHORT_BOW: {
        energy = 10000;
        break;
    }

    /* Long Bow and Arrow */
    case SV_LONG_BOW: {
        energy = 10000;
        break;
    }

    /* Bow of irresponsiblity and Arrow */
    case SV_NAMAKE_BOW: {
        energy = 7777;
        break;
    }

    /* Light Crossbow and Bolt */
    case SV_LIGHT_XBOW: {
        energy = 12000;
        break;
    }

    /* Heavy Crossbow and Bolt */
    case SV_HEAVY_XBOW: {
        energy = 13333;
        break;
    }
    }

    return energy;
}

/*
 * Return bow tmul
 */
int bow_tmul(OBJECT_SUBTYPE_VALUE sval)
{
    int tmul = 0;

    /* Analyze the launcher */
    switch (sval) {
        /* Sling and ammo */
    case SV_SLING: {
        tmul = 2;
        break;
    }

    /* Short Bow and Arrow */
    case SV_SHORT_BOW: {
        tmul = 2;
        break;
    }

    /* Long Bow and Arrow */
    case SV_LONG_BOW: {
        tmul = 3;
        break;
    }

    /* Bow of irresponsiblity and Arrow */
    case SV_NAMAKE_BOW: {
        tmul = 3;
        break;
    }

    /* Light Crossbow and Bolt */
    case SV_LIGHT_XBOW: {
        tmul = 3;
        break;
    }

    /* Heavy Crossbow and Bolt */
    case SV_HEAVY_XBOW: {
        tmul = 4;
        break;
    }
    }

    return tmul;
}

/*!
 * @brief 射撃時クリティカルによるダメージ期待値修正計算（スナイパーの集中処理と武器経験値） / critical happens at i / 10000
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param plus_ammo 矢弾のダメージ修正
 * @param plus_bow 弓のダメージ修正
 * @return ダメージ期待値
 * @note 基本ダメージ量と重量はこの部位では計算に加わらない。
 */
int calc_crit_ratio_shot(PlayerType *player_ptr, int plus_ammo, int plus_bow)
{
    int i;
    auto *j_ptr = &player_ptr->inventory_list[INVEN_BOW];

    /* Extract "shot" power */
    i = player_ptr->to_h_b + plus_ammo;

    if (player_ptr->tval_ammo == ItemKindType::BOLT) {
        i = (player_ptr->skill_thb + (player_ptr->weapon_exp[j_ptr->tval][j_ptr->sval] / 400 + i) * BTH_PLUS_ADJ);
    } else {
        i = (player_ptr->skill_thb + ((player_ptr->weapon_exp[j_ptr->tval][j_ptr->sval] - (PlayerSkill::weapon_exp_at(PlayerSkillRank::MASTER) / 2)) / 200 + i) * BTH_PLUS_ADJ);
    }

    PlayerClass pc(player_ptr);
    auto sniper_data = pc.get_specific_data<sniper_data_type>();
    auto sniper_concent = sniper_data ? sniper_data->concent : 0;

    /* Snipers can shot more critically with crossbows */
    i += ((i * sniper_concent) / 5);
    if (pc.equals(PlayerClassType::SNIPER) && (player_ptr->tval_ammo == ItemKindType::BOLT)) {
        i *= 2;
    }

    /* Good bow makes more critical */
    i += plus_bow * 8 * (sniper_concent + 5);

    if (i < 0) {
        i = 0;
    }

    return i;
}

/*!
 * @brief 射撃時クリティカルによるダメージ期待値修正計算（重量依存部分） / critical happens at i / 10000
 * @param weight 武器の重量
 * @param plus_ammo 矢弾のダメージ修正
 * @param plus_bow 弓のダメージ修正
 * @param dam 基本ダメージ量
 * @return ダメージ期待値
 */
int calc_expect_crit_shot(PlayerType *player_ptr, WEIGHT weight, int plus_ammo, int plus_bow, int dam)
{
    uint32_t num;
    int i, k, crit;
    i = calc_crit_ratio_shot(player_ptr, plus_ammo, plus_bow);

    k = 0;
    num = 0;

    crit = std::min(500, 900 / weight);
    num += dam * 3 / 2 * crit;
    k = crit;

    crit = std::min(500, 1350 / weight);
    crit -= k;
    num += dam * 2 * crit;
    k += crit;

    if (k < 500) {
        crit = 500 - k;
        num += dam * 3 * crit;
    }

    num /= 500;

    num *= i;
    num += (10000 - i) * dam;
    num /= 10000;

    return num;
}

/*!
 * @brief 攻撃時クリティカルによるダメージ期待値修正計算（重量と毒針処理） / critical happens at i / 10000
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param weight 武器の重量
 * @param plus 武器のダメージ修正
 * @param dam 基本ダメージ
 * @param meichuu 命中値
 * @param dokubari 毒針処理か否か
 * @param impact 強撃かどうか
 * @return ダメージ期待値
 */
int calc_expect_crit(PlayerType *player_ptr, WEIGHT weight, int plus, int dam, int16_t meichuu, bool dokubari, bool impact)
{
    if (dokubari) {
        return dam;
    }

    int i = (weight + (meichuu * 3 + plus * 5) + player_ptr->skill_thn);
    if (i < 0) {
        i = 0;
    }

    // 通常ダメージdam、武器重量weightでクリティカルが発生した時のクリティカルダメージ期待値
    auto calc_weight_expect_dam = [](int dam, WEIGHT weight) {
        int sum = 0;
        for (int d = 1; d <= 650; ++d) {
            int k = weight + d;
            sum += std::get<0>(apply_critical_norm_damage(k, dam));
        }
        return sum / 650;
    };

    uint32_t num = 0;

    if (impact) {
        for (int d = 1; d <= 650; ++d) {
            num += calc_weight_expect_dam(dam, weight + d);
        }
        num /= 650;
    } else {
        num += calc_weight_expect_dam(dam, weight);
    }

    int pow = PlayerClass(player_ptr).equals(PlayerClassType::NINJA) ? 4444 : 5000;
    if (impact) {
        pow /= 2;
    }

    num *= i;
    num += (pow - i) * dam;
    num /= pow;

    return num;
}
