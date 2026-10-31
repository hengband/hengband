#include "info-reader/jsonc-document-loader.h"
#include <chrono>
#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
class JsoncFile {
public:
    JsoncFile()
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto root = std::filesystem::temp_directory_path();
        for (auto attempt = 0; attempt < 64; ++attempt) {
            const auto candidate = root / ("hengband-jsonc-document-test-" + std::to_string(stamp) + "-" + std::to_string(attempt));
            std::error_code ec;
            if (std::filesystem::create_directory(candidate, ec)) {
                this->directory = candidate;
                this->path = this->directory / "document.jsonc";
                return;
            }
            if (ec) {
                throw std::runtime_error("Cannot create JSONC document test directory: " + ec.message());
            }
        }
        throw std::runtime_error("Cannot allocate a unique JSONC document test directory");
    }

    JsoncFile(const JsoncFile &) = delete;
    JsoncFile &operator=(const JsoncFile &) = delete;

    ~JsoncFile()
    {
        std::error_code ec;
        std::filesystem::remove_all(this->directory, ec);
    }

    void write(std::string_view text) const
    {
        std::ofstream output(this->path);
        output.exceptions(std::ios::failbit | std::ios::badbit);
        output << text;
        output.close();
    }

    std::filesystem::path path;

private:
    std::filesystem::path directory;
};

/*!
 * @brief load() を通さずに、同じ設定 (コメントと末尾カンマを許可) でファイルを解析する
 * @details 解析器が投げる例外の文言を、load() が返す文言と比べるために使う。
 */
nlohmann::json parse_file_directly(const std::filesystem::path &path)
{
    std::ifstream input(path);
    REQUIRE(input);
    return nlohmann::json::parse(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>(), nullptr, true, true, true);
}
}

TEST_CASE("JsoncDocumentLoader::load exposes missing files without creating them or throwing")
{
    JsoncFile file;
    CHECK_NOTHROW((void)JsoncDocumentLoader::load(file.path));
    CHECK_FALSE(JsoncDocumentLoader::load(file.path).has_value());
    CHECK_FALSE(std::filesystem::exists(file.path));
}

TEST_CASE("JsoncDocumentLoader::load parses documents without schema validation or localization")
{
    JsoncFile file;
    file.write(R"({"name":{"ja":"\u65e5\u672c","en":"Japan"},"values":[true,null,-2,3.5],"unknown":{}})");
    const auto loaded = JsoncDocumentLoader::load(file.path);
    REQUIRE(loaded.has_value());
    const auto &document = *loaded;
    CHECK(document["name"]["ja"] == "\xe6\x97\xa5\xe6\x9c\xac");
    CHECK(document["name"]["en"] == "Japan");
    CHECK(document["values"] == nlohmann::json::array({ true, nullptr, -2, 3.5 }));
    CHECK(document["unknown"].is_object());
}

TEST_CASE("JsoncDocumentLoader::load allows comments and trailing commas with unchanged parsed dump")
{
    JsoncFile file;
    file.write(R"(// leading comment
{
    "z": [1, 2,], /* block comment */
    "a": {"value": "// not a comment",},
} // trailing comment
)");
    const auto loaded = JsoncDocumentLoader::load(file.path);
    REQUIRE(loaded.has_value());
    CHECK(loaded->dump() == R"({"a":{"value":"// not a comment"},"z":[1,2]})");
}

TEST_CASE("JsoncDocumentLoader::load distinguishes valid null and other JSON roots from missing files")
{
    JsoncFile file;
    for (const auto &text : { "null", "[]", "{}", "42", "true", "\"text\"" }) {
        CAPTURE(text);
        file.write(text);
        const auto loaded = JsoncDocumentLoader::load(file.path);
        REQUIRE(loaded.has_value());
        CHECK(*loaded == nlohmann::json::parse(text));
    }
}

TEST_CASE("JsoncDocumentLoader::load reports empty and malformed files as parse_error")
{
    JsoncFile file;
    for (const auto &text : { "", " \n\t", "// comment only", "{", "{\"value\":}", "[1,", "/* unterminated", "{} {}" }) {
        CAPTURE(text);
        file.write(text);
        const auto loaded = JsoncDocumentLoader::load(file.path);
        REQUIRE_FALSE(loaded.has_value());
        CHECK(loaded.error().kind == JsoncLoadErrorKind::PARSE_ERROR);
        // 解析器の例外の種類 (nlohmann::json::parse_error) は what() の先頭に出る
        CHECK(loaded.error().message.starts_with("[json.exception.parse_error."));
    }
}

TEST_CASE("JsoncDocumentLoader::load failure leaves caller output unchanged")
{
    JsoncFile file;
    file.write("{");
    nlohmann::json document = { { "sentinel", 7 } };
    const auto before = document;
    CHECK_NOTHROW(document = JsoncDocumentLoader::load(file.path).value_or(document));
    CHECK(document == before);
}

TEST_CASE("JsoncDocumentLoader::load reports missing files as CANNOT_OPEN with empty message")
{
    JsoncFile file;
    const auto loaded = JsoncDocumentLoader::load(file.path);
    REQUIRE_FALSE(loaded.has_value());
    CHECK(loaded.error().kind == JsoncLoadErrorKind::CANNOT_OPEN);
    CHECK(loaded.error().message.empty());
}

TEST_CASE("JsoncDocumentLoader::load returns the same document for valid JSONC and every JSON root")
{
    JsoncFile file;
    file.write(R"(// leading comment
{
    "z": [1, 2,], /* block comment */
    "a": {"value": "// not a comment",},
} // trailing comment
)");
    const auto loaded = JsoncDocumentLoader::load(file.path);
    REQUIRE(loaded.has_value());
    CHECK(loaded->dump() == R"({"a":{"value":"// not a comment"},"z":[1,2]})");

    for (const auto &text : { "null", "[]", "{}", "42", "true", "\"text\"" }) {
        CAPTURE(text);
        file.write(text);
        const auto root = JsoncDocumentLoader::load(file.path);
        REQUIRE(root.has_value());
        CHECK(*root == nlohmann::json::parse(text));
    }
}

TEST_CASE("JsoncDocumentLoader::load reports parse failures as PARSE_ERROR with the parser message")
{
    JsoncFile file;
    // 末尾の余計な値・数値のあふれ (out_of_range) も解析器の例外として同じ扱いにする
    for (const auto &text : { "", " \n\t", "// comment only", "{", "{\"value\":}", "[1,", "/* unterminated", "{} {}", "1e999" }) {
        CAPTURE(text);
        file.write(text);
        std::string expected_message;
        try {
            (void)parse_file_directly(file.path);
            FAIL("parsing must throw");
        } catch (const nlohmann::json::exception &e) {
            expected_message = e.what();
        }
        const auto loaded = JsoncDocumentLoader::load(file.path);
        REQUIRE_FALSE(loaded.has_value());
        CHECK(loaded.error().kind == JsoncLoadErrorKind::PARSE_ERROR);
        CHECK_FALSE(loaded.error().message.empty());
        CHECK(loaded.error().message == expected_message);
    }
}
