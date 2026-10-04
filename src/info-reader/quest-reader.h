#pragma once
/*!
 * @file quest-reader.h
 * @brief 固定クエスト JSONC の名前を QuestType に、静的メタデータとレイアウトを QuestFixedMap に読み込む
 */

#include <nlohmann/json_fwd.hpp>
#include <string>

enum parse_error_type : int;
class QuestType;
struct QuestFixedMap;
struct QuestLegendCell;
struct QuestDescriptionBlock;

/*!
 * @brief 固定クエスト JSONC 読み込みクラス
 *
 * 既存の *Reader 群と同じ規約: メンバは const 参照、キーアクセスは get_json_value 経由。
 * 名前を QuestType へ、静的メタデータと地形レイアウトを QuestFixedMap へ書き込む。
 * QuestType の進行状態や報酬の確定は変更しない。
 */
class QuestReader {
public:
    QuestReader(const nlohmann::json &quest_data, QuestType &quest, QuestFixedMap &fixed_map);
    QuestReader(nlohmann::json &&, QuestType &, QuestFixedMap &) = delete;
    QuestReader(const QuestReader &) = delete;
    QuestReader(QuestReader &&) = delete;
    QuestReader &operator=(const QuestReader &) = delete;
    QuestReader &operator=(QuestReader &&) = delete;

    /*! @brief 全件成功時のみ名前と固定マップを置き換える。失敗・例外時は既存の出力を保持する。 */
    int read() const;

private:
    int set_name(std::string &name) const;
    int set_definition(QuestFixedMap &parsed) const;
    int set_descriptions(QuestFixedMap &parsed) const;
    int set_legend(QuestFixedMap &parsed) const;
    int set_maps(QuestFixedMap &parsed) const;
    int set_starts(QuestFixedMap &parsed) const;

    const nlohmann::json &quest_data;
    QuestType &quest;
    QuestFixedMap &fixed_map;
};

/*!
 * @brief JSONC の gridDefinition 1件を凡例セルへ変換する (set_legend から使用)
 * @return エラーコード
 */
parse_error_type parse_quest_legend_cell(const nlohmann::json &cell_data, QuestLegendCell &out);
