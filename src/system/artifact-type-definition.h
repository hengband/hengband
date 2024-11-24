#pragma once

#include "object-enchant/tr-flags.h"
#include "object-enchant/trg-types.h"
#include "system/angband.h"
#include "system/baseitem-info.h"
#include "util/dice.h"
#include "util/flag-group.h"
#include <map>
#include <string>

/*!
 * @class ArtifactType
 * @brief 固定アーティファクト情報の構造体 / Artifact structure.
 * @details is_generated とfloor_id フィールドのみセーブファイルへの保存対象
 */
enum class FixedArtifactId : short;
enum class RandomArtActType : short;
class ArtifactType {
public:
    ArtifactType();

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
    bool is_generated{}; /*! 生成済か否か (生成済でも、「保存モードON」かつ「帰還等で鑑定前に消滅」したら未生成状態に戻る) */
    FLOOR_IDX floor_id{}; /*! アイテムを落としたフロアのID / Leaved on this location last time */
    RandomArtActType act_idx{}; /*! 発動能力ID / Activative ability index */

    bool can_generate(const BaseitemKey &bi_key) const;
    std::optional<BaseitemKey> try_make_instant_artifact(int making_level) const;

private:
    bool can_make_instant_artifact() const;
    bool evaluate_shallow_instant_artifact(int making_level) const;
    bool evaluate_rarity() const;
    bool evaluate_shallow_baseitem(int making_level) const;
};

class ItemEntity;
class ArtifactList {
public:
    ArtifactList(const ArtifactList &) = delete;
    ArtifactList(ArtifactList &&) = delete;
    ArtifactList &operator=(const ArtifactList &) = delete;
    ArtifactList &operator=(ArtifactList &&) = delete;
    ~ArtifactList() = default;

    static ArtifactList &get_instance();
    std::map<FixedArtifactId, ArtifactType>::iterator begin();
    std::map<FixedArtifactId, ArtifactType>::iterator end();
    std::map<FixedArtifactId, ArtifactType>::const_iterator begin() const;
    std::map<FixedArtifactId, ArtifactType>::const_iterator end() const;
    std::map<FixedArtifactId, ArtifactType>::reverse_iterator rbegin();
    std::map<FixedArtifactId, ArtifactType>::reverse_iterator rend();
    std::map<FixedArtifactId, ArtifactType>::const_reverse_iterator rbegin() const;
    std::map<FixedArtifactId, ArtifactType>::const_reverse_iterator rend() const;
    const ArtifactType &get_artifact(const FixedArtifactId fa_id) const;
    ArtifactType &get_artifact(const FixedArtifactId fa_id);

    bool order(const FixedArtifactId id1, const FixedArtifactId id2) const;
    void emplace(const FixedArtifactId fa_id, const ArtifactType &artifact);
    void reset_generated_flags();
    std::optional<ItemEntity> try_make_instant_artifact(int making_level) const;

private:
    ArtifactList() = default;
    static ArtifactList instance;
    static ArtifactType dummy;

    std::map<FixedArtifactId, ArtifactType> artifacts{};
};
