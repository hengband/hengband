#pragma once

#include "info-reader/parse-error-types.h"
#include <nlohmann/json_fwd.hpp>
#include <string>

enum class TownMapMode {
    NORMAL,
    LITE,
    NONE,
};

/*!
 * @brief 町番号と街モードからJSONC町マップのファイル名を選ぶ。
 * 選択と入力検証だけを行い、世界状態やファイルシステムには触れない。
 */
class TownDefinitionListReader {
public:
    explicit TownDefinitionListReader(const nlohmann::json &data);
    TownDefinitionListReader(nlohmann::json &&) = delete;
    TownDefinitionListReader(const TownDefinitionListReader &) = delete;
    TownDefinitionListReader(TownDefinitionListReader &&) = delete;
    TownDefinitionListReader &operator=(const TownDefinitionListReader &) = delete;
    TownDefinitionListReader &operator=(TownDefinitionListReader &&) = delete;

    /*!
     * @brief 有効な町が見つかった場合のみmap_fileを書き換える。
     * 町が存在しない場合は成功として出力を保持する。
     */
    parse_error_type read(int town_index, TownMapMode mode, std::string &map_file) const;

private:
    const nlohmann::json &data;
};
