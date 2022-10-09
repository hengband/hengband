#include "racial/racial-android.h"
#include "effect/attribute-types.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/trg-types.h"
#include "object-hook/hook-weapon.h"
#include "object/object-value-calc.h"
#include "object/object-value.h"
#include "object/tval-types.h"
#include "player-base/player-race.h"
#include "player-info/equipment-info.h"
#include "player/player-status.h"
#include "spell-kind/spells-launcher.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem-info-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool android_inside_weapon(PlayerType *player_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    if (player_ptr->lev < 10) {
        msg_print(_("レイガンを発射した。", "You fire your ray gun."));
        fire_bolt(player_ptr, AttributeType::MISSILE, dir, (player_ptr->lev + 1) / 2);
        return true;
    }

    if (player_ptr->lev < 25) {
        msg_print(_("ブラスターを発射した。", "You fire your blaster."));
        fire_bolt(player_ptr, AttributeType::MISSILE, dir, player_ptr->lev);
        return true;
    }

    if (player_ptr->lev < 35) {
        msg_print(_("バズーカを発射した。", "You fire your bazooka."));
        fire_ball(player_ptr, AttributeType::MISSILE, dir, player_ptr->lev * 2, 2);
        return true;
    }

    if (player_ptr->lev < 45) {
        msg_print(_("ビームキャノンを発射した。", "You fire a beam cannon."));
        fire_beam(player_ptr, AttributeType::MISSILE, dir, player_ptr->lev * 2);
        return true;
    }

    msg_print(_("ロケットを発射した。", "You fire a rocket."));
    fire_rocket(player_ptr, AttributeType::ROCKET, dir, player_ptr->lev * 5, 2);
    return true;
}

void calc_android_exp(PlayerType *player_ptr)
{
    uint32_t total_exp = 0;
    if (player_ptr->is_dead || !PlayerRace(player_ptr).equals(PlayerRaceType::ANDROID)) {
        return;
    }

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        auto *o_ptr = &player_ptr->inventory_list[i];
        ObjectType forge;
        auto *q_ptr = &forge;
        uint32_t value, exp;
        DEPTH level = std::max(k_info[o_ptr->k_idx].level - 8, 1);

        if ((i == INVEN_MAIN_RING) || (i == INVEN_SUB_RING) || (i == INVEN_NECK) || (i == INVEN_LITE)) {
            continue;
        }
        if (!o_ptr->k_idx) {
            continue;
        }

        q_ptr->wipe();
        q_ptr->copy_from(o_ptr);
        q_ptr->discount = 0;
        q_ptr->curse_flags.clear();

        if (o_ptr->is_fixed_artifact()) {
            const auto &fixed_artifact = a_info.at(o_ptr->fixed_artifact_idx);
            level = (level + std::max(fixed_artifact.level - 8, 5)) / 2;
            level += std::min(20, fixed_artifact.rarity / (fixed_artifact.gen_flags.has(ItemGenerationTraitType::INSTA_ART) ? 10 : 3));
        } else if (o_ptr->is_ego()) {
            level += std::max(3, (e_info[o_ptr->ego_idx].rating - 5) / 2);
        } else if (o_ptr->art_name) {
            int32_t total_flags = flag_cost(o_ptr, o_ptr->pval);
            int fake_level;

            if (!o_ptr->is_weapon_ammo()) {
                if (total_flags < 15000) {
                    fake_level = 10;
                } else if (total_flags < 35000) {
                    fake_level = 25;
                } else {
                    fake_level = 40;
                }
            } else {
                if (total_flags < 20000) {
                    fake_level = 10;
                } else if (total_flags < 45000) {
                    fake_level = 25;
                } else {
                    fake_level = 40;
                }
            }

            level = std::max(level, (level + std::max(fake_level - 8, 5)) / 2 + 3);
        }

        value = object_value_real(q_ptr);
        if (value <= 0) {
            continue;
        }
        if ((o_ptr->tval == ItemKindType::SOFT_ARMOR) && (o_ptr->sval == SV_ABUNAI_MIZUGI) && (player_ptr->ppersonality != PERSONALITY_SEXY)) {
            value /= 32;
        }
        if (value > 5000000L) {
            value = 5000000L;
        }
        if ((o_ptr->tval == ItemKindType::DRAG_ARMOR) || (o_ptr->tval == ItemKindType::CARD)) {
            level /= 2;
        }

        if (o_ptr->is_artifact() || o_ptr->is_ego() || (o_ptr->tval == ItemKindType::DRAG_ARMOR) || ((o_ptr->tval == ItemKindType::HELM) && (o_ptr->sval == SV_DRAGON_HELM)) || ((o_ptr->tval == ItemKindType::SHIELD) && (o_ptr->sval == SV_DRAGON_SHIELD)) || ((o_ptr->tval == ItemKindType::GLOVES) && (o_ptr->sval == SV_SET_OF_DRAGON_GLOVES)) || ((o_ptr->tval == ItemKindType::BOOTS) && (o_ptr->sval == SV_PAIR_OF_DRAGON_GREAVE)) || ((o_ptr->tval == ItemKindType::SWORD) && (o_ptr->sval == SV_DIAMOND_EDGE))) {
            if (level > 65) {
                level = 35 + (level - 65) / 5;
            } else if (level > 35) {
                level = 25 + (level - 35) / 3;
            } else if (level > 15) {
                level = 15 + (level - 15) / 2;
            }
            exp = std::min<uint>(100000, value) / 2 * level * level;
            if (value > 100000L) {
                exp += (value - 100000L) / 8 * level * level;
            }
        } else {
            exp = std::min<uint>(100000, value) * level;
            if (value > 100000L) {
                exp += (value - 100000L) / 4 * level;
            }
        }
        if ((((i == INVEN_MAIN_HAND) || (i == INVEN_SUB_HAND)) && (has_melee_weapon(player_ptr, i))) || (i == INVEN_BOW)) {
            total_exp += exp / 48;
        } else {
            total_exp += exp / 16;
        }
        if (i == INVEN_BODY) {
            total_exp += exp / 32;
        }
    }

    player_ptr->exp = player_ptr->max_exp = total_exp;
    check_experience(player_ptr);
}
