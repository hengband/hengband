/*!
 * @brief ベースアイテムを強化する処理
 * @date 2020/06/03
 * @author Hourier
 * @todo 少し長い。要分割
 */

#include "object-enchant/item-magic-applier.h"
#include "artifact/fixed-art-generator.h"
#include "artifact/fixed-art-types.h"
#include "dungeon/dungeon.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/enchanter-base.h"
#include "object-enchant/enchanter-factory.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/object-curse.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/trg-types.h"
#include "object/object-kind.h"
#include "player/player-status-flags.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "world/world.h"

/*!
 * @brief 生成されたベースアイテムに魔法的な強化を与えるメインルーチン
 * Complete the "creation" of an object by applying "magic" to the item
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param lev 生成基準階
 * @param mode 生成オプション
 * @details エゴ＆アーティファクトの生成、呪い、pval強化
 */
void apply_magic_to_object(PlayerType *player_ptr, ObjectType *o_ptr, DEPTH lev, BIT_FLAGS mode)
{
    if (player_ptr->ppersonality == PERSONALITY_MUNCHKIN) {
        lev += randint0(player_ptr->lev / 2 + 10);
    }

    if (lev > MAX_DEPTH - 1) {
        lev = MAX_DEPTH - 1;
    }

    auto f1 = lev + 10;
    if (f1 > d_info[player_ptr->dungeon_idx].obj_good) {
        f1 = d_info[player_ptr->dungeon_idx].obj_good;
    }

    auto f2 = f1 * 2 / 3;
    if ((player_ptr->ppersonality != PERSONALITY_MUNCHKIN) && (f2 > d_info[player_ptr->dungeon_idx].obj_great)) {
        f2 = d_info[player_ptr->dungeon_idx].obj_great;
    }

    if (has_good_luck(player_ptr)) {
        f1 += 5;
        f2 += 2;
    } else if (player_ptr->muta.has(PlayerMutationType::BAD_LUCK)) {
        f1 -= 5;
        f2 -= 2;
    }

    auto power = 0;
    if (any_bits(mode, AM_GOOD) || magik(f1)) {
        power = 1;
        if (any_bits(mode, AM_GREAT) || magik(f2)) {
            power = 2;
            if (any_bits(mode, AM_SPECIAL)) {
                power = 3;
            }
        }
    } else if (magik(f1)) {
        power = -1;
        if (magik(f2)) {
            power = -2;
        }
    }

    if (any_bits(mode, AM_CURSED)) {
        if (power > 0) {
            power = 0 - power;
        } else {
            power--;
        }
    }

    auto rolls = 0;
    if (power >= 2) {
        rolls = 1;
    }

    if (any_bits(mode, AM_GREAT | AM_SPECIAL)) {
        rolls = 4;
    }

    if (any_bits(mode, AM_NO_FIXED_ART) || o_ptr->fixed_artifact_idx) {
        rolls = 0;
    }

    for (auto i = 0; i < rolls; i++) {
        if (make_artifact(player_ptr, o_ptr)) {
            break;
        }

        if (!has_good_luck(player_ptr) || !one_in_(77)) {
            continue;
        }

        if (make_artifact(player_ptr, o_ptr)) {
            break;
        }
    }

    if (o_ptr->is_fixed_artifact()) {
        auto *a_ptr = apply_artifact(player_ptr, o_ptr);
        a_ptr->cur_num = 1;
        if (w_ptr->character_dungeon) {
            a_ptr->floor_id = player_ptr->floor_id;
        }

        return;
    }

    auto enchanter = EnchanterFactory::create_enchanter(player_ptr, o_ptr, lev, power);
    enchanter->apply_magic();
    if (o_ptr->is_ego()) {
        apply_ego(o_ptr, lev);
        return;
    }

    if (o_ptr->k_idx) {
        auto *k_ptr = &k_info[o_ptr->k_idx];
        if (!k_info[o_ptr->k_idx].cost) {
            set_bits(o_ptr->ident, IDENT_BROKEN);
        }

        if (k_ptr->gen_flags.has(ItemGenerationTraitType::CURSED)) {
            o_ptr->curse_flags.set(CurseTraitType::CURSED);
        }

        if (k_ptr->gen_flags.has(ItemGenerationTraitType::HEAVY_CURSE)) {
            o_ptr->curse_flags.set(CurseTraitType::HEAVY_CURSE);
        }

        if (k_ptr->gen_flags.has(ItemGenerationTraitType::PERMA_CURSE)) {
            o_ptr->curse_flags.set(CurseTraitType::PERMA_CURSE);
        }

        if (k_ptr->gen_flags.has(ItemGenerationTraitType::RANDOM_CURSE0)) {
            o_ptr->curse_flags.set(get_curse(0, o_ptr));
        }

        if (k_ptr->gen_flags.has(ItemGenerationTraitType::RANDOM_CURSE1)) {
            o_ptr->curse_flags.set(get_curse(1, o_ptr));
        }

        if (k_ptr->gen_flags.has(ItemGenerationTraitType::RANDOM_CURSE2)) {
            o_ptr->curse_flags.set(get_curse(2, o_ptr));
        }
    }
}
