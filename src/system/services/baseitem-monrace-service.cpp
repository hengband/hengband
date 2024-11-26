/*!
 * @brief ベースアイテムとモンスター種族を接続するサービスクラス実装
 * @author Hourier
 * @date 2024/11/26
 */

#include "system/services/baseitem-monrace-service.h"
#include "locale/language-switcher.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-key.h"
#include "system/baseitem/baseitem-list.h"
#include "system/monster-race-info.h"
#include "term/z-form.h"
#include "util/enum-converter.h"
#include <map>
#include <set>

namespace {
const std::map<MonsterDropType, BaseitemKey> SPECIFIC_GOLD_DROPS = {
    { MonsterDropType::DROP_COPPER, { ItemKindType::GOLD, 3 } },
    { MonsterDropType::DROP_SILVER, { ItemKindType::GOLD, 6 } },
    { MonsterDropType::DROP_GARNET, { ItemKindType::GOLD, 8 } },
    { MonsterDropType::DROP_GOLD, { ItemKindType::GOLD, 11 } },
    { MonsterDropType::DROP_OPAL, { ItemKindType::GOLD, 12 } },
    { MonsterDropType::DROP_SAPPHIRE, { ItemKindType::GOLD, 13 } },
    { MonsterDropType::DROP_RUBY, { ItemKindType::GOLD, 14 } },
    { MonsterDropType::DROP_DIAMOND, { ItemKindType::GOLD, 15 } },
    { MonsterDropType::DROP_EMERALD, { ItemKindType::GOLD, 16 } },
    { MonsterDropType::DROP_MITHRIL, { ItemKindType::GOLD, 17 } },
    { MonsterDropType::DROP_ADAMANTITE, { ItemKindType::GOLD, 18 } },
};
}

/*!
 * @brief モンスター種族IDから財宝アイテムの価値を引く
 * @param monrace_id モンスター種族ID
 * @return 特定の財宝を落とすならそのアイテムの価値オフセット、一般的な財宝ドロップならばnullopt
 */
std::optional<int> BaseitemMonraceService::lookup_specific_gold_drop_offset(const EnumClassFlagGroup<MonsterDropType> &flags)
{
    const auto &baseitems = BaseitemList::get_instance();
    for (const auto &pair : SPECIFIC_GOLD_DROPS) {
        if (flags.has_not(pair.first)) {
            continue;
        }

        return baseitems.lookup_gold_offset(pair.second);
    }

    return std::nullopt;
}
