#pragma once

/*!
 * @class ArtifactDefinition
 * @brief 読み取り専用の固定アーティファクト定義
 * @author Hourier
 * @date 2026/05/10
 */

#include "object-enchant/tr-flags.h"
#include "object-enchant/trg-types.h"
#include "system/angband.h"
#include "system/baseitem/baseitem-key.h"
#include "util/dice.h"
#include "util/flag-group.h"
#include <map>
#include <string>
#include <tl/optional.hpp>

enum class RandomArtActType : short;
class ArtifactDefinition {
public:
    ArtifactDefinition();
    ArtifactDefinition(const ArtifactDefinition &) = delete;
    ArtifactDefinition &operator=(const ArtifactDefinition &) = delete;
    ArtifactDefinition(ArtifactDefinition &&) = default;
    ArtifactDefinition &operator=(ArtifactDefinition &&) = delete;

    std::string name; /*!< アーティファクト名 / Name */
    std::string text; /*!< アーティファクト解説 / Text */
    BaseitemKey bi_key;
    PARAMETER_VALUE pval{}; /*!< pval修正値 / Artifact extra info */
    HIT_PROB to_h{}; /*!< 命中ボーナス値 /  Bonus to hit */
    int to_d{}; /*!< ダメージボーナス値 / Bonus to damage */
    ARMOUR_CLASS to_a{}; /*!< ACボーナス値 / Bonus to armor */
    ARMOUR_CLASS ac{}; /*!< 上書きベースAC値 / Base armor */
    Dice damage_dice; /*!< ダイス値 / Damage when hits */
    WEIGHT weight{}; /*!< 重量 / Weight */
    PRICE cost{}; /*!< 基本価格 / Artifact "cost" */
    TrFlags flags{}; /*! アイテムフラグ / Artifact Flags */
    EnumClassFlagGroup<ItemGenerationTraitType> gen_flags; /*! アイテム生成フラグ / flags for generate */
    DEPTH level{}; /*! 基本生成階 / Artifact level */
    RARITY rarity{}; /*! レアリティ / Artifact rarity */
    RandomArtActType act_idx{}; /*! 発動能力ID / Activative ability index */

    bool can_generate(const BaseitemKey &generating_bi_key) const;
    bool is_instant_artifact() const;
};
