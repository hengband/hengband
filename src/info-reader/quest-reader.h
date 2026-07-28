#pragma once
/*!
 * @file quest-reader.h
 * @brief 1つの固定クエスト JSONC を QuestType(メタデータ) と QuestFixedMap(レイアウト) に読み込む
 */

#include <nlohmann/json.hpp>

enum parse_error_type : int;
class QuestType;
struct QuestFixedMap;
struct QuestLegendCell;
struct QuestDescriptionBlock;

/*!
 * @brief 固定クエスト JSONC 読み込みクラス
 *
 * 既存の *Reader 群と同じ規約: メンバは const 参照、キーアクセスは get_json_value 経由。
 * メタデータ(type/level/flags/reward/name 等)を QuestType へ、地形レイアウト
 * (legend/map/descriptions/start)を QuestFixedMap へ書き込む。
 */
class QuestReader {
public:
    QuestReader(const nlohmann::json &quest_data, QuestType &quest, QuestFixedMap &fixed_map);
    QuestReader(nlohmann::json &&, QuestType &, QuestFixedMap &) = delete;
    QuestReader(const QuestReader &) = delete;
    QuestReader(QuestReader &&) = delete;
    QuestReader &operator=(const QuestReader &) = delete;
    QuestReader &operator=(QuestReader &&) = delete;

    int read() const;

private:
    int set_name() const;
    int set_definition() const;
    int set_descriptions() const;
    int set_legend() const;
    int set_maps() const;
    int set_starts() const;

    const nlohmann::json &quest_data;
    QuestType &quest;
    QuestFixedMap &fixed_map;
};

/*!
 * @brief JSONC の gridDefinition 1件を凡例セルへ変換する (set_legend から使用)
 * @return エラーコード
 */
parse_error_type parse_quest_legend_cell(const nlohmann::json &cell_data, QuestLegendCell &out);
