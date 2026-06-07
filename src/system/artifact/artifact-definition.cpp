#include "system/artifact/artifact-definition.h"
#include "artifact/fixed-art-types.h"
#include "object/tval-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#ifdef JP
#else
#include "util/string-processor.h"
#include <sstream>
#endif

ArtifactDefinition::ArtifactDefinition()
    : bi_key(BaseitemKey(ItemKindType::NONE))
{
}

bool ArtifactDefinition::can_generate(const BaseitemKey &generating_bi_key) const
{
    if (this->gen_flags.has(ItemGenerationTraitType::QUESTITEM)) {
        return false;
    }

    if (this->is_instant_artifact()) {
        return false;
    }

    return this->bi_key == generating_bi_key;
}

bool ArtifactDefinition::is_instant_artifact() const
{
    return this->gen_flags.has(ItemGenerationTraitType::INSTA_ART);
}

/*!
 * @brief アーティファクトを『』も考慮しつつ完全な名前を返す
 *
 * 日本語版：
 * FULL_NAMEフラグつきの場合、そのまま
 * 例1：運命のオーブ → 運命のオーブ
 * 例2：トゲトゲバット『エスカリボルグ』 → トゲトゲバット『エスカリボルグ』
 *
 * FULL_NAMEフラグなしの場合、ベースアイテム名を付与する
 * - 『』ありなら前置する
 * - 『』なしなら後置する
 * 例1：『ナルサンク』 → ダガー『ナルサンク』
 * 例2：シヴァの化身の → シヴァの化身の軟革ブーツ
 *
 * 「★」の要不要は表示したい箇所によって変わるので、このメソッドでは付けない.
 *
 * English version:
 * If the artifact has the FULL_NAME flag, return it as is.
 * All fixed artifacts with "The", then it starts capital always; there must not be any name starting other than "The", such as "the".
 * Example1: 'Excaliborg, the *thorny* bat' => The 'Excaliborg, the *thorny* bat'
 * Example2: The Orb of Fate => The Orb of Fate
 *
 * If an artifact does not have the FULL_NAME flag, the baseitem name is always prefixed and the fixed artifact name is appended.
 * Example1: 'Narthanc' => The Dagger 'Narthanc'
 * Example2: of Corwin => The Set of Gauntlets of Corwin
 */
std::string ArtifactDefinition::build_full_name() const
{
#ifdef JP
    if (this->flags.has(tr_type::TR_FULL_NAME)) {
        return this->name;
    }

    constexpr auto start = "『";
    const auto is_preposition = this->name.starts_with(start);
    const auto &baseitems = BaseitemList::get_instance();
    if (is_preposition) {
        return baseitems.lookup_baseitem(this->bi_key).name + this->name;
    }

    return this->name + baseitems.lookup_baseitem(this->bi_key).name;
#else
    constexpr auto definite_article = "The";
    if (this->flags.has(tr_type::TR_FULL_NAME)) {
        std::stringstream ss;
        if (!this->name.starts_with(definite_article)) {
            ss << definite_article << ' ';
        }

        ss << this->name;
        return ss.str();
    }

    const auto &baseitems = BaseitemList::get_instance();
    const auto &baseitem = baseitems.lookup_baseitem(this->bi_key);
    const auto baseitem_name_without_numeral = str_replace(baseitem.name, "&", "");
    const auto baseitem_name_singular = str_replace(baseitem_name_without_numeral, "~", "");
    std::stringstream ss;
    ss << definite_article;
    if (!baseitem_name_singular.starts_with(' ')) {
        ss << ' ';
    }

    ss << baseitem_name_singular << ' ' << this->name;
    return ss.str();
#endif
}
