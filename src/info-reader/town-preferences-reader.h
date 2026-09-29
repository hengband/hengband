#pragma once

#include "info-reader/general-parser.h"
#include "info-reader/parse-error-types.h"
#include <nlohmann/json_fwd.hpp>
#include <utility>
#include <vector>

struct QuestLegendCell;

using TownPreferencesLegend = std::vector<std::pair<unsigned char, dungeon_grid>>;
using TownPreferencesCellParser = parse_error_type (*)(const nlohmann::json &, QuestLegendCell &);

/*!
 * @brief 町の共通凡例 JSONC を検証して読み込む。
 * 地形タグの解決は既存のクエスト凡例パーサーに委ね、ゲーム状態への反映は呼び出し側で行う。
 */
class TownPreferencesReader {
public:
    explicit TownPreferencesReader(const nlohmann::json &data);
    TownPreferencesReader(nlohmann::json &&) = delete;
    TownPreferencesReader(const TownPreferencesReader &) = delete;
    TownPreferencesReader(TownPreferencesReader &&) = delete;
    TownPreferencesReader &operator=(const TownPreferencesReader &) = delete;
    TownPreferencesReader &operator=(TownPreferencesReader &&) = delete;

    /*! @brief 全件成功した場合に限り出力を置き換える。 */
    parse_error_type read(TownPreferencesLegend &legend, TownPreferencesCellParser parse_cell) const;

private:
    const nlohmann::json &data;
};
