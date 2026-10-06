#include "info-reader/jsonc-document-loader.h"
#include <iterator>

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
