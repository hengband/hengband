#pragma once

#include <filesystem>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <tl/expected.hpp>

//! JSONC ファイルを読み込めなかった理由の種類
enum class JsoncLoadErrorKind {
    CANNOT_OPEN, //!< ファイルを開けない
    PARSE_ERROR, //!< 解析器が例外を投げた（構文エラー・数値のあふれなど）
};

//! JSONC ファイルを読み込めなかった理由
struct JsoncLoadError {
    JsoncLoadErrorKind kind; //!< 失敗の種類
    std::string message; //!< 解析器の例外の what()。ファイルを開けない時は空
};

/*!
 * @brief コメントと末尾カンマを許可して JSONC ファイルを読み込む
 * @details load() が、開く・解析する・解析器の例外を捕まえて JsoncLoadError に変える、までをまとめて行う。
 * 開けないか解析できないかの区別と解析器の文言を返すので、診断の表示は呼び出し元が決める。
 * 実体は持たず、load() だけを使う。
 */
class JsoncDocumentLoader {
public:
    JsoncDocumentLoader() = delete;

    static tl::expected<nlohmann::json, JsoncLoadError> load(const std::filesystem::path &path);
};
