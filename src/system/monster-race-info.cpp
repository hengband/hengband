#include "system/monster-race-info.h"
#include "monster-race/race-indice-types.h"
#include "monster/horror-descriptions.h"
#include <algorithm>

/*!
 * @brief エルドリッチホラーの形容詞種別を決める
 * @return エルドリッチホラーの形容詞
 */
const std::string &MonsterRaceInfo::decide_horror_message() const
{
    const int horror_desc_common_size = horror_desc_common.size();
    auto horror_num = randint0(horror_desc_common_size + horror_desc_evil.size());
    if (horror_num < horror_desc_common_size) {
        return horror_desc_common[horror_num];
    }

    if (this->kind_flags.has(MonsterKindType::EVIL)) {
        return horror_desc_evil[horror_num - horror_desc_common_size];
    }

    return horror_desc_neutral[horror_num - horror_desc_common_size];
}

/*!
 * @brief モンスターが生命体かどうかを返す
 * @return 生命体ならばtrue
 */
bool MonsterRaceInfo::has_living_flag() const
{
    return this->kind_flags.has_none_of({ MonsterKindType::DEMON, MonsterKindType::UNDEAD, MonsterKindType::NONLIVING });
}

bool MonsterRaceInfo::is_explodable() const
{
    return std::any_of(std::begin(this->blows), std::end(this->blows),
        [](const auto &blow) { return blow.method == RaceBlowMethodType::EXPLODE; });
}

/*!
 * @brief モンスターを撃破した際の述語メッセージを返す
 * @return 撃破されたモンスターの述語
 */
std::string MonsterRaceInfo::get_died_message() const
{
    const auto is_explodable = this->is_explodable();
    if (this->has_living_flag()) {
        return is_explodable ? _("は爆発して死んだ。", " explodes and dies.") : _("は死んだ。", " dies.");
    }

    return is_explodable ? _("は爆発して粉々になった。", " explodes into tiny shreds.") : _("を倒した。", " is destroyed.");
}

/*!
 * @brief モンスターが特殊能力上、賞金首から排除する必要があるかどうかを返す
 * @return 賞金首から排除するか否か
 * @details 実質バーノール＝ルパート用
 * @todo idxに依存している。monraces_info が既にmapなのでMonsterRaceList クラスのオブジェクトメソッドに移す
 */
bool MonsterRaceInfo::no_suitable_questor_bounty() const
{
    switch (this->idx) {
    case MonsterRaceId::BANORLUPART:
    case MonsterRaceId::BANOR:
    case MonsterRaceId::LUPART:
        return true;
    default:
        return false;
    }
}
