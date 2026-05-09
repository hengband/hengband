#include "system/artifact/artifact-service.h"
#include "artifact/fixed-art-types.h"
#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-list.h"
#include "system/artifact/artifact-record.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/item/item-entity.h"

tl::optional<FixedArtifactId> ArtifactService::find_generatable_fixed_artifact(const BaseitemKey &bi_key, int dungeon_level)
{
    const auto &records = ArtifactRecords::get_instance();
    for (const auto &[fa_id, artifact] : ArtifactList::get_instance()) {
        if (!records.can_generate(fa_id) || !artifact.can_generate(bi_key)) {
            continue;
        }

        if (!evaluate_shallow_fixed_artifact(artifact, dungeon_level)) {
            continue;
        }

        if (!one_in_(artifact.rarity)) {
            continue;
        }

        return fa_id;
    }

    return tl::nullopt;
}

tl::optional<ItemEntity> ArtifactService::try_make_instant_artifact(int making_level)
{
    for (const auto &[fa_id, artifact] : ArtifactList::get_instance()) {
        const auto bi_key = try_make_instant_artifact(fa_id, artifact, making_level);
        if (bi_key) {
            ItemEntity instant_artifact(*bi_key);
            instant_artifact.fa_id = fa_id;
            return instant_artifact;
        }
    }

    return tl::nullopt;
}

/*!
 * @brief INSTA_ART型の固定アーティファクト生成を試みる
 * @param 生成基準階層 (現在フロアそのものではなくボーナスつき)
 * @param fa_id 固定アーティファクトID
 * @return 生成に成功したらそのアイテム、失敗したらnullopt
 */
tl::optional<BaseitemKey> ArtifactService::try_make_instant_artifact(FixedArtifactId fa_id, const ArtifactDefinition &artifact, int making_level)
{
    if (!can_make_instant_artifact(fa_id, artifact)) {
        return tl::nullopt;
    }

    if (!evaluate_shallow_fixed_artifact(artifact, making_level)) {
        return tl::nullopt;
    }

    if (!one_in_(artifact.rarity)) {
        return tl::nullopt;
    }

    if (!evaluate_shallow_baseitem(artifact, making_level)) {
        return tl::nullopt;
    }

    return artifact.bi_key;
}

/*!
 * @brief INSTA_ARTフラグ付きアーティファクトの生成可否を判定する
 * @return 生成可否
 * @details 生成済、クエスト属性付き、非INSTA_ARTはfalse、普通のINSTA_ARTはtrue
 */
bool ArtifactService::can_make_instant_artifact(FixedArtifactId fa_id, const ArtifactDefinition &artifact)
{
    auto can_make = ArtifactRecords::get_instance().can_generate(fa_id);
    can_make &= artifact.gen_flags.has_not(ItemGenerationTraitType::QUESTITEM);
    can_make &= artifact.gen_flags.has(ItemGenerationTraitType::INSTA_ART);
    return can_make;
}

/*!
 * @brief 標準生成階層より浅い階層での生成制限を判定する
 * @return 生成可否
 * @details 1/(不足階層*2) を満たさないと生成しない
 */
bool ArtifactService::evaluate_shallow_fixed_artifact(const ArtifactDefinition &artifact, int making_level)
{
    if (artifact.level <= making_level) {
        return true;
    }

    return one_in_((artifact.level - making_level) * 2);
}

/*!
 * @brief 標準生成階層より浅い階層でのベースアイテム生成制限を判定する
 * @return 生成可否
 * @details 1/(不足階層*5) を満たさないと生成しない
 */
bool ArtifactService::evaluate_shallow_baseitem(const ArtifactDefinition &artifact, int making_level)
{
    const auto &baseitems = BaseitemList::get_instance();
    const auto &baseitem = baseitems.lookup_baseitem(artifact.bi_key);
    if (baseitem.level <= making_level) {
        return true;
    }

    return one_in_((baseitem.level - making_level) * 5);
}
