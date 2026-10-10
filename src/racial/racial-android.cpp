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
#include "system/artifact/artifact-definition.h"
#include "system/item/item-entity.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"
#include <cstdint>

bool android_inside_weapon(PlayerType *player_ptr)
{
    const auto dir = get_aim_dir(player_ptr);
    if (!dir) {
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

/*!
 * @brief 装備価格・基準レベルから部位による除算前のアンドロイド経験値を返す
 * @param value 0～5000000へ補正済みの装備価格
 * @param level 装備基準レベル。呼び出し側の定義域は0～167。負値は0として扱う。
 * @param special エゴ等の特別な装備の補正式を使うか
 * @return 部位補正前の経験値
 */
uint64_t android_item_experience(uint64_t value, int64_t level, bool special)
{
    level = std::max<int64_t>(level, 0);
    uint64_t exp;
    if (special) {
        if (level > 65) {
            level = 35 + (level - 65) / 5;
        } else if (level > 35) {
            level = 25 + (level - 35) / 3;
        } else if (level > 15) {
            level = 15 + (level - 15) / 2;
        }
        // 基本レベル最大128から8を引き、エゴ評価最大100による47を加えた167以下。
        const auto adjusted_level = static_cast<uint64_t>(level);
        const auto factor = std::min<uint64_t>(100000, value) / 2 + (value > 100000 ? (value - 100000) / 8 : 0);
        exp = factor * adjusted_level * adjusted_level;
    } else {
        exp = std::min<uint64_t>(100000, value) * static_cast<uint64_t>(level);
        if (value > 100000L) {
            exp += (value - 100000L) / 4 * static_cast<uint64_t>(level);
        }
    }
    return exp;
}

/*!
 * @brief アンドロイドの装備から経験値を再計算し、上限へ収めてレベルを更新する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details 死亡中または他種族なら変更しない。装備の割引・呪いは価格評価から除外する。
 */
void calc_android_exp(PlayerType *player_ptr)
{
    uint64_t total_exp = 0;
    if (player_ptr->is_dead || !PlayerRace(player_ptr).equals(PlayerRaceType::ANDROID)) {
        return;
    }

    for (const auto i_idx : INVEN_WIELDING_SLOTS) {
        auto *o_ptr = player_ptr->inventory[i_idx].get();
        uint64_t value, exp;
        auto level = std::max<int64_t>(int64_t{ o_ptr->get_baseitem_level() } - 8, 1);

        if ((i_idx == INVEN_MAIN_RING) || (i_idx == INVEN_SUB_RING) || (i_idx == INVEN_NECK) || (i_idx == INVEN_LITE)) {
            continue;
        }
        if (!o_ptr->is_valid()) {
            continue;
        }

        if (o_ptr->is_fixed_artifact()) {
            const auto &artifact = o_ptr->get_fixed_artifact();
            level = (level + std::max<int64_t>(int64_t{ artifact.level } - 8, 5)) / 2;
            level += std::min(20, artifact.rarity / (artifact.gen_flags.has(ItemGenerationTraitType::INSTA_ART) ? 10 : 3));
        } else if (o_ptr->is_ego()) {
            level += std::max(3, (o_ptr->get_ego().rating - 5) / 2);
        } else if (o_ptr->is_random_artifact()) {
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

            level = std::max<int64_t>(level, (level + std::max(fake_level - 8, 5)) / 2 + 3);
        }

        // 装備品の割引や呪いはアンドロイドの経験値計算に影響しない
        auto item = o_ptr->clone();
        item.discount = 0;
        item.curse_flags.clear();

        value = object_value_real(&item);
        if (value <= 0) {
            continue;
        }

        const auto &bi_key = o_ptr->bi_key;
        if ((bi_key == BaseitemKey(ItemKindType::SOFT_ARMOR, SV_ABUNAI_MIZUGI)) && (player_ptr->ppersonality != PERSONALITY_SEXY)) {
            value /= 32;
        }

        if (value > 5000000L) {
            value = 5000000L;
        }

        const auto tval = o_ptr->bi_key.tval();
        if ((tval == ItemKindType::DRAG_ARMOR) || (tval == ItemKindType::CARD)) {
            level /= 2;
        }

        auto is_dragon_protector = tval == ItemKindType::DRAG_ARMOR;
        is_dragon_protector |= bi_key == BaseitemKey(ItemKindType::HELM, SV_DRAGON_HELM);
        is_dragon_protector |= bi_key == BaseitemKey(ItemKindType::SHIELD, SV_DRAGON_SHIELD);
        is_dragon_protector |= bi_key == BaseitemKey(ItemKindType::GLOVES, SV_SET_OF_DRAGON_GLOVES);
        is_dragon_protector |= bi_key == BaseitemKey(ItemKindType::BOOTS, SV_PAIR_OF_DRAGON_GREAVE);
        const auto is_diamond_edge = bi_key == BaseitemKey(ItemKindType::SWORD, SV_DIAMOND_EDGE);
        exp = android_item_experience(value, level, o_ptr->is_fixed_or_random_artifact() || o_ptr->is_ego() || is_dragon_protector || is_diamond_edge);
        if ((((i_idx == INVEN_MAIN_HAND) || (i_idx == INVEN_SUB_HAND)) && (has_melee_weapon(player_ptr, i_idx))) || (i_idx == INVEN_BOW)) {
            total_exp += exp / 48;
        } else {
            total_exp += exp / 16;
        }
        if (i_idx == INVEN_BODY) {
            total_exp += exp / 32;
        }
    }

    player_ptr->exp = player_ptr->max_exp = static_cast<EXP>(std::min<uint64_t>(total_exp, PY_MAX_EXP));
    check_experience(player_ptr);
}
