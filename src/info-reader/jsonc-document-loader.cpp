#include "info-reader/jsonc-document-loader.h"
#include <iterator>
#include <nlohmann/json.hpp>

JsoncDocumentLoader::JsoncDocumentLoader(const std::filesystem::path &path)
    : input(path)
{
}

bool JsoncDocumentLoader::is_open() const
{
    return static_cast<bool>(this->input);
}

nlohmann::json JsoncDocumentLoader::parse()
{
    return nlohmann::json::parse(std::istreambuf_iterator<char>(this->input), std::istreambuf_iterator<char>(), nullptr, true, true, true);
}

/*!
 * @brief JSONC ファイルを開いて解析する
 * @param path 読み込むファイルのパス
 * @return 解析した文書。失敗時は、開けないか解析器の例外かの種類と、解析器の例外の文言
 * @details 捕まえるのは nlohmann::json の例外だけで、それ以外（メモリ不足など）は呼び出し元へ伝える。
 */
tl::expected<nlohmann::json, JsoncLoadError> JsoncDocumentLoader::load(const std::filesystem::path &path)
{
    JsoncDocumentLoader loader(path);
    if (!loader.is_open()) {
        return tl::make_unexpected(JsoncLoadError{ JsoncLoadErrorKind::CANNOT_OPEN, std::string() });
    }

    try {
        return loader.parse();
    } catch (const nlohmann::json::exception &e) {
        return tl::make_unexpected(JsoncLoadError{ JsoncLoadErrorKind::PARSE_ERROR, e.what() });
    }
}
