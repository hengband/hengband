/*!
 * @brief ベースアイテムとモンスター種族を接続するサービスクラス定義
 * @author Hourier
 * @date 2024/11/26
 * @details サービスクラスはステートレスであるべきなのでメソッドは全てstatic である
 */

#pragma once

#include "monster-race/race-drop-flags.h"
#include "util/flag-group.h"
#include <optional>
#include <string>

class BaseitemMonraceService {
public:
    BaseitemMonraceService() = delete;
    static std::optional<int> lookup_specific_gold_drop_offset(const EnumClassFlagGroup<MonsterDropType> &flags);
    static std::optional<std::string> check_drop_flags();
};
