#include "system/monrace/monrace-service.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monrace/monrace-records.h"
#include "util/string-processor.h"

/*!
 * @brief モンスターを引数で与えたフィルタ関数で検索する
 *
 * @param filter このフィルタ関数がtrueを返すモンスターを検索する
 * @param is_known_only trueならばプレイヤーが既知のモンスターのみを対象とする。falseならば全てのモンスターを対象とする。
 * @return std::vector<MonraceId> 検索結果のモンスター種族IDリスト
 */
std::vector<MonraceId> MonraceService::search(const std::function<bool(const MonraceDefinition &)> &filter, bool is_known_only)
{
    std::vector<MonraceId> result_ids;
    const auto &records = MonraceRecords::get_instance();
    for (const auto &[id, monrace] : MonraceList::get_instance()) {
        if (!monrace->is_valid()) {
            continue;
        }

        if (is_known_only && !records.has_been_seen(id)) {
            continue;
        }

        if (filter(*monrace)) {
            result_ids.push_back(id);
        }
    }

    return result_ids;
}

/*!
 * @brief モンスターを名前で検索する
 *
 * 引数で与えた名前を含む(部分一致)モンスターを検索する。
 *
 * @param name 検索するモンスターの名前
 * @param is_known_only trueならばプレイヤーが既知のモンスターのみを対象とする。falseならば全てのモンスターを対象とする。
 * @return std::vector<MonraceId> 検索結果のモンスター種族IDリスト
 */
std::vector<MonraceId> MonraceService::search_by_name(std::string_view name, bool is_known_only)
{
    std::vector<MonraceId> result_ids;
    const auto lowered_search_name = str_tolower(name);

    auto filter = [&](const MonraceDefinition &monrace) {
        const auto lowered_en_name = str_tolower(monrace.name.en_string());

#ifdef JP
        return str_find(lowered_en_name, lowered_search_name) || str_find(monrace.name.string(), lowered_search_name);
#else
        return str_find(lowered_en_name, lowered_search_name);
#endif
    };

    return search(std::move(filter), is_known_only);
}

/*!
 * @brief モンスターのシンボルで検索する
 *
 * @param symbol 検索するモンスターのシンボル
 * @param is_known_only trueならばプレイヤーが既知のモンスターのみを対象とする。falseならば全てのモンスターを対象とする。
 * @return std::vector<MonraceId> 検索結果のモンスター種族IDリスト
 */
std::vector<MonraceId> MonraceService::search_by_symbol(char symbol, bool is_known_only)
{
    auto filter = [&](const MonraceDefinition &monrace) {
        return monrace.symbol_char_is_any_of(std::string(1, symbol));
    };

    return search(std::move(filter), is_known_only);
}
