#pragma once

#include "info-reader/parse-error-types.h"
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <vector>

struct TownMapFeatureRule {
    char symbol = '\0';
    std::string terrain;
    int cave_info = 0;
    std::string monster;
    std::string object;
    std::string ego;
    std::string artifact;
    std::string trap;
    int special = 0;
    std::optional<std::string> condition;
};

struct TownMapBuildingRule {
    int index = 0;
    bool english = false;
    char command = '\0';
    std::vector<std::string> fields;
    std::optional<std::string> condition;
};

struct TownMapVariant {
    std::vector<std::string> rows;
    std::optional<std::string> condition;
};

struct TownMapStartingPosition {
    int y = 0;
    int x = 0;
    std::optional<std::string> condition;
};

struct TownMapDefinition {
    std::vector<TownMapFeatureRule> features;
    std::vector<TownMapBuildingRule> buildings;
    std::vector<TownMapVariant> maps;
    std::vector<TownMapStartingPosition> starts;
};

/*!
 * @brief 町マップJSONCを検証し、ゲーム状態に触れず定義データへ読み込む。
 * 条件式とUTF-8文字列はそのまま保持し、評価・文字コード変換は適用側で行う。
 */
class TownMapReader {
public:
    explicit TownMapReader(const nlohmann::json &data);
    TownMapReader(nlohmann::json &&) = delete;
    TownMapReader(const TownMapReader &) = delete;
    TownMapReader(TownMapReader &&) = delete;
    TownMapReader &operator=(const TownMapReader &) = delete;
    TownMapReader &operator=(TownMapReader &&) = delete;

    /*!
     * @brief 成功時のみ出力を置き換える。建物専用モードではマップ・開始位置の読み取りを省略する。
     */
    parse_error_type read(TownMapDefinition &definition, int maximum_height, int maximum_width, bool only_buildings = false) const;

private:
    parse_error_type read_features(TownMapDefinition &definition) const;
    parse_error_type read_buildings(TownMapDefinition &definition) const;
    parse_error_type read_maps(TownMapDefinition &definition, int maximum_height, int maximum_width) const;
    parse_error_type read_starts(TownMapDefinition &definition) const;

    const nlohmann::json &data;
};
