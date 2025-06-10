/*!
 * @brief ベースアイテムとモンスター種族を接続するサービスクラス定義
 * @author Hourier
 * @date 2024/11/26
 * @details サービスクラスはステートレスであるべきなのでメソッドは全てstatic である
 */

#pragma once

#include "monster-race/race-drop-flags.h"
#include "system/baseitem/baseitem-definition.h"
#include "util/flag-group.h"
#include <string>
#include <tl/optional.hpp>

class BaseitemMonraceService {
public:
    BaseitemMonraceService() = delete;
    static tl::optional<BaseitemKey> lookup_fixed_gold_drop(const EnumClassFlagGroup<MonsterDropType> &flags);
    static tl::optional<std::string> check_specific_drop_gold_flags_duplication();
};
