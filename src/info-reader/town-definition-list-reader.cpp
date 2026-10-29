#include "info-reader/town-definition-list-reader.h"
#include "info-reader/json-reader-util.h"
#include <nlohmann/json.hpp>
#include <string>

TownDefinitionListReader::TownDefinitionListReader(const nlohmann::json &data)
    : data(data)
{
}

/*!
 * @brief 町番号と街モードに対応する町マップのファイル名を選ぶ
 * @param town_index 町番号（町の外は0）
 * @param mode 街モード
 * @param map_file 有効な町が見つかった場合のみ書き換える町マップのファイル名
 * @return 成功時は空の値、失敗時は解析エラーコード。
 * 町の外（町番号が1未満）は成功として出力を保持する。町番号が1以上で一覧に無い場合は、
 * 店の無い町が黙ってできないよう ParseErrorType::INVALID_VALUE を返し、出力を保持する。
 */
tl::expected<void, ParseErrorType> TownDefinitionListReader::read(int town_index, TownMapMode mode, std::string &map_file) const
{
    if (const auto result = info_validate_version(this->data, 1, ParseErrorType::INVALID_TYPE); !result) {
        return result;
    }
    if (!this->data.contains("towns") || !this->data["towns"].is_object() || this->data["towns"].empty()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }

    const auto &towns = this->data["towns"];
    const auto town = towns.find(std::to_string(town_index));
    if (town == towns.end()) {
        if (town_index > 0) {
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        return {};
    }

    const nlohmann::json *selected = &*town;
    if (selected->is_object()) {
        const char *key = nullptr;
        switch (mode) {
        case TownMapMode::NORMAL:
            key = "normal";
            break;
        case TownMapMode::LITE:
            key = "lite";
            break;
        case TownMapMode::NONE:
            key = "none";
            break;
        default:
            return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
        }
        const auto file = selected->find(key);
        if (file == selected->end()) {
            return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
        }
        selected = &*file;
    }

    if (!selected->is_string()) {
        return tl::make_unexpected(ParseErrorType::INVALID_TYPE);
    }
    const auto &candidate = selected->get_ref<const std::string &>();
    if (!candidate.starts_with("towns/") || !candidate.ends_with(".jsonc") || candidate.find("..") != std::string::npos ||
        candidate.find('\\') != std::string::npos) {
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    }
    map_file = candidate;
    return {};
}
