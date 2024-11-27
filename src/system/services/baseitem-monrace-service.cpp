/*!
 * @brief ベースアイテムとモンスター種族を接続するサービスクラス実装
 * @author Hourier
 * @date 2024/11/26
 */

#include "system/services/baseitem-monrace-service.h"
#include "locale/language-switcher.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-key.h"
#include "system/monster-race-info.h"
#include "term/z-form.h"
#include "util/enum-converter.h"
#include <map>
#include <set>

namespace {
const std::map<MonsterDropType, BaseitemKey> FIXED_GOLD_DROPS = {
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

EnumClassFlagGroup<MonsterDropType> make_fixed_gold_drop_flags()
{
    EnumClassFlagGroup<MonsterDropType> flags;
    for (const auto &[flag, _] : FIXED_GOLD_DROPS) {
        flags.set(flag);
    }

    return flags;
}
}

/*!
 * @brief ドロップ関連フラグリストから固定ドロップの財宝アイテムを取得する
 * @param flags ドロップ関連フラグリスト
 * @return 特定の財宝を落とすならそのアイテムのBaseitemKey、一般的な財宝ドロップならばnullopt
 */
std::optional<BaseitemKey> BaseitemMonraceService::lookup_fixed_gold_drop(const EnumClassFlagGroup<MonsterDropType> &flags)
{
    for (const auto &pair : FIXED_GOLD_DROPS) {
        if (flags.has_not(pair.first)) {
            continue;
        }
        return pair.second;
    }

    return std::nullopt;
}

std::optional<std::string> BaseitemMonraceService::check_specific_drop_gold_flags_duplication()
{
    const auto specific_gold_drop_flags = make_fixed_gold_drop_flags();

    const auto &monraces = MonraceList::get_instance();
    for (const auto &[monrace_id, monrace] : monraces) {
        const auto monrace_specific_gold_drop_flags = specific_gold_drop_flags & monrace.drop_flags;

        if (monrace_specific_gold_drop_flags.count() > 1) {
            constexpr auto fmt = _("財宝種別は同時に2つ以上指定できません。 ID: %d",
                "You cannot specify more than one treasure type at the same time. ID: %d");
            return format(fmt, enum2i(monrace_id));
        }
    }

    return std::nullopt;
}
