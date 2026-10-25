#pragma once
/*!
 * @file quest-reader.h
 * @brief 固定クエスト JSONC の名前を QuestType に、静的メタデータとレイアウトを QuestFixedMap に読み込む
 */

#include "info-reader/parse-error-types.h"
#include "system/dungeon/quest-fixed-map.h"
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <tl/expected.hpp>

class QuestType;
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

    tl::expected<void, ParseErrorType> read() const;

private:
    tl::expected<void, ParseErrorType> set_name(std::string &name) const;
    tl::expected<void, ParseErrorType> set_definition(QuestFixedMap &parsed) const;
    tl::expected<void, ParseErrorType> set_descriptions(QuestFixedMap &parsed) const;
    tl::expected<void, ParseErrorType> set_legend(QuestFixedMap &parsed) const;
    tl::expected<void, ParseErrorType> set_maps(QuestFixedMap &parsed) const;
    tl::expected<void, ParseErrorType> set_starts(QuestFixedMap &parsed) const;

    const nlohmann::json &quest_data;
    QuestType &quest;
    QuestFixedMap &fixed_map;
};

tl::expected<void, ParseErrorType> parse_quest_legend_cell(const nlohmann::json &cell_data, QuestLegendCell &out);

/*! @brief 地形・罠を未解決のタグとして保持する凡例セル。地形・罠の数値欄は0、対応するランダムビットは未設定。 */
struct DeferredQuestLegendCell {
    QuestLegendCell cell{};
    std::string terrain_tag = "NONE";
    std::string trap_tag = "NONE";
};

tl::expected<void, ParseErrorType> parse_quest_legend_cell_deferred(const nlohmann::json &cell_data, DeferredQuestLegendCell &out);
