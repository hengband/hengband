/*!
 * @brief ベースアイテムを強化する処理
 * @date 2022/03/22
 * @author Hourier
 */

#include "object-enchant/item-magic-applier.h"
#include "artifact/fixed-art-generator.h"
#include "dungeon/dungeon.h"
#include "object-enchant/enchanter-base.h"
#include "object-enchant/enchanter-factory.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/object-curse.h"
#include "object-enchant/special-object-flags.h"
#include "object/object-kind.h"
#include "player/player-status-flags.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "world/world.h"

/*!
 * @brief コンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 強化を与えたいオブジェクトの構造体参照ポインタ
 * @param lev 生成基準階
 * @param mode 生成オプション
 */
ItemMagicApplier::ItemMagicApplier(PlayerType *player_ptr, ObjectType *o_ptr, DEPTH lev, BIT_FLAGS mode)
    : player_ptr(player_ptr)
    , o_ptr(o_ptr)
    , lev(lev)
    , mode(mode)
{
    if (player_ptr->ppersonality == PERSONALITY_MUNCHKIN) {
        this->lev += randint0(player_ptr->lev / 2 + 10);
    }

    if (this->lev > MAX_DEPTH - 1) {
        this->lev = MAX_DEPTH - 1;
    }
}

/*!
 * @brief 生成されたベースアイテムに魔法的な強化を与えるメインルーチン
 * @details エゴ＆アーティファクトの生成、呪い、pval強化
 */
void ItemMagicApplier::execute()
{
    auto [chance_good, chance_great] = this->calculate_chances();
    auto power = this->calculate_power(chance_good, chance_great);
    auto rolls = this->calculate_rolls(power);
    this->try_make_artifact(rolls);
    if (this->set_fixed_artifact_generation_info()) {
        return;
    }

    auto enchanter = EnchanterFactory::create_enchanter(this->player_ptr, this->o_ptr, this->lev, power);
    enchanter->apply_magic();
    if (this->o_ptr->is_ego()) {
        apply_ego(this->o_ptr, this->lev);
        return;
    }

    this->apply_cursed();
}

/*!
 * @brief 上質及び高級品の生成確率 [%]を算出する
 * @return 上質と高級品の生成確率組み合わせ
 */
std::tuple<int, int> ItemMagicApplier::calculate_chances()
{
    auto chance_good = this->lev + 10;
    if (chance_good > d_info[this->player_ptr->dungeon_idx].obj_good) {
        chance_good = d_info[this->player_ptr->dungeon_idx].obj_good;
    }

    auto chance_great = chance_good * 2 / 3;
    if ((this->player_ptr->ppersonality != PERSONALITY_MUNCHKIN) && (chance_great > d_info[this->player_ptr->dungeon_idx].obj_great)) {
        chance_great = d_info[this->player_ptr->dungeon_idx].obj_great;
    }

    if (has_good_luck(this->player_ptr)) {
        chance_good += 5;
        chance_great += 2;
    } else if (this->player_ptr->muta.has(PlayerMutationType::BAD_LUCK)) {
        chance_good -= 5;
        chance_great -= 2;
    }

    return std::make_tuple(chance_good, chance_great);
}

/*!
 * @brief 上質/高級品/アーティファクト生成パワー計算
 * @param chance_good 上質品が生成される確率 [%]
 * @param chance_great 高級品が生成される確率 [%]
 * @return 生成パワー
 * @details アーティファクト生成はデバッグ専用
 */
int ItemMagicApplier::calculate_power(const int chance_good, const int chance_great)
{
    auto power = 0;
    if (any_bits(this->mode, AM_GOOD) || magik(chance_good)) {
        power = 1;
        if (any_bits(this->mode, AM_GREAT) || magik(chance_great)) {
            power = 2;
            if (any_bits(this->mode, AM_SPECIAL)) {
                power = 3;
            }
        }
    } else if (magik(chance_good)) {
        power = -1;
        if (magik(chance_great)) {
            power = -2;
        }
    }

    if (any_bits(this->mode, AM_CURSED)) {
        if (power > 0) {
            power = 0 - power;
        } else {
            power--;
        }
    }

    return power;
}

/*!
 * @brief 生成パワーを元にアーティファクト生成の試行回数を算出する
 * @param power 生成パワー
 */
int ItemMagicApplier::calculate_rolls(const int power)
{
    auto rolls = 0;
    if (power >= 2) {
        rolls = 1;
    }

    if (any_bits(this->mode, AM_GREAT | AM_SPECIAL)) {
        rolls = 4;
    }

    if (any_bits(this->mode, AM_NO_FIXED_ART) || this->o_ptr->fixed_artifact_idx) {
        rolls = 0;
    }

    return rolls;
}

/*!
 * @brief アーティファクト生成を試みる
 * @param power 試行回数
 */
void ItemMagicApplier::try_make_artifact(const int rolls)
{
    for (auto i = 0; i < rolls; i++) {
        if (make_artifact(this->player_ptr, this->o_ptr)) {
            break;
        }

        if (!has_good_luck(this->player_ptr) || !one_in_(77)) {
            continue;
        }

        if (make_artifact(this->player_ptr, this->o_ptr)) {
            break;
        }
    }
}

/*!
 * @brief 固定アーティファクトの生成情報を記憶する
 */
bool ItemMagicApplier::set_fixed_artifact_generation_info()
{
    if (!this->o_ptr->is_fixed_artifact()) {
        return false;
    }

    auto *a_ptr = apply_artifact(this->player_ptr, this->o_ptr);
    a_ptr->is_generated = true;
    if (w_ptr->character_dungeon) {
        a_ptr->floor_id = this->player_ptr->floor_id;
    }

    return true;
}

/*!
 * @brief アイテムに呪いフラグを付与する
 */
void ItemMagicApplier::apply_cursed()
{
    if (this->o_ptr->k_idx == 0) {
        return;
    }

    const auto *k_ptr = &k_info[this->o_ptr->k_idx];
    if (!k_info[this->o_ptr->k_idx].cost) {
        set_bits(this->o_ptr->ident, IDENT_BROKEN);
    }

    if (k_ptr->gen_flags.has(ItemGenerationTraitType::CURSED)) {
        this->o_ptr->curse_flags.set(CurseTraitType::CURSED);
    }

    if (k_ptr->gen_flags.has(ItemGenerationTraitType::HEAVY_CURSE)) {
        this->o_ptr->curse_flags.set(CurseTraitType::HEAVY_CURSE);
    }

    if (k_ptr->gen_flags.has(ItemGenerationTraitType::PERMA_CURSE)) {
        this->o_ptr->curse_flags.set(CurseTraitType::PERMA_CURSE);
    }

    if (k_ptr->gen_flags.has(ItemGenerationTraitType::RANDOM_CURSE0)) {
        this->o_ptr->curse_flags.set(get_curse(0, this->o_ptr));
    }

    if (k_ptr->gen_flags.has(ItemGenerationTraitType::RANDOM_CURSE1)) {
        this->o_ptr->curse_flags.set(get_curse(1, this->o_ptr));
    }

    if (k_ptr->gen_flags.has(ItemGenerationTraitType::RANDOM_CURSE2)) {
        this->o_ptr->curse_flags.set(get_curse(2, this->o_ptr));
    }
}
