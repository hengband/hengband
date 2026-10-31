#pragma once

#include <filesystem>
#include <fstream>
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
 * @details オープンと解析を分離し、診断と例外処理の範囲は呼び出し元が決める。
 * 開く・解析する・解析器の例外を捕まえる、までをまとめて行う load() もある。
 */
class JsoncDocumentLoader {
public:
    explicit JsoncDocumentLoader(const std::filesystem::path &path);
    JsoncDocumentLoader(const JsoncDocumentLoader &) = delete;
    JsoncDocumentLoader &operator=(const JsoncDocumentLoader &) = delete;
    // 解析前に確認する。従来の ifstream の真偽値チェックと同じストリーム状態を返す。
    bool is_open() const;
    // 文書全体を一度だけ解析する。構文エラーの例外は呼び出し元へそのまま伝える。
    nlohmann::json parse();

    static tl::expected<nlohmann::json, JsoncLoadError> load(const std::filesystem::path &path);

private:
    std::ifstream input;
};
