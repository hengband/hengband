#pragma once

/*!
 * @brief モンスター種族定義とプレイ中の記録を組み合わせて、モンスター種族に関する動的な情報を提供するサービスクラス
 * @author Hourier
 * @date 2026/05/26
 */

#include <functional>
#include <string_view>
#include <vector>

enum class MonraceId : short;
class MonraceDefinition;
class MonraceService {
public:
    MonraceService() = delete;

    static std::vector<MonraceId> search(const std::function<bool(const MonraceDefinition &)> &filter, bool is_known_only = false);
    static std::vector<MonraceId> search_by_name(std::string_view name, bool is_known_only = false);
    static std::vector<MonraceId> search_by_symbol(char symbol, bool is_known_only);
};
