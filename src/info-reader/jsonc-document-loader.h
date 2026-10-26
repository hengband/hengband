#pragma once

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

/*!
 * @brief コメントと末尾カンマを許可して JSONC ファイルを読み込む
 * @details オープンと解析を分離し、診断と例外処理の範囲は呼び出し元が決める。
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

private:
    std::ifstream input;
};
