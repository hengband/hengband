#pragma once

#include "info-reader/parse-error-types.h"
#include "system/dungeon/quest-fixed-map.h"
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <variant>
#include <vector>

struct TownMapFeatureRule {
    char symbol = '\0';
    std::string terrain; //!< 条件が成立したときにのみ地形タグを解決する。
    std::string trap; //!< 建物専用モードでも地形データの初期化を要求しない。
    QuestLegendCell cell;
    std::optional<std::string> condition;
};

struct TownMapBuildingNames {
    std::string name;
    std::string owner_name;
    std::string owner_race;
};

struct TownMapBuildingAction {
    int index = 0;
    std::string name;
    int member_cost = 0;
    int other_cost = 0;
    std::string letter;
    int action = 0;
    int restriction = 0;
};

enum class TownMapBuildingMembershipKind { CLASS,
    RACE,
    REALM };

struct TownMapBuildingMembership {
    TownMapBuildingMembershipKind kind = TownMapBuildingMembershipKind::CLASS;
    std::vector<int> values;
};

struct TownMapBuildingNoop {
    // Zの引数も従来どおり文字コード変換を検証するが、建物へは適用しない。
    std::vector<std::string> fields;
};

struct TownMapBuildingRule {
    int index = 0;
    bool english = false;
    std::variant<TownMapBuildingNames, TownMapBuildingAction, TownMapBuildingMembership, TownMapBuildingNoop> directive;
    std::optional<std::string> condition;
};

/*!
 * @brief 検証済みの建物指示を文字コード変換し、対象言語の建物へ直接適用する。
 */
parse_error_type apply_town_building_rule(const TownMapBuildingRule &rule);

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

/*! @brief TownMapReaderで検証し、条件評価済みの町凡例を適用する。失敗時は既存の letter[] を保持する。 */
parse_error_type apply_town_map_feature(const FloorType &floor, const TownMapFeatureRule &feature);

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
