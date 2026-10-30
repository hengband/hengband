#include "info-reader/jsonc-document-loader.h"
#include <chrono>
#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
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
}

TEST_CASE("JsoncDocumentLoader exposes missing files without parsing or throwing")
{
    JsoncFile file;
    JsoncDocumentLoader loader(file.path);
    CHECK_FALSE(loader.is_open());
    CHECK_FALSE(std::filesystem::exists(file.path));
}

TEST_CASE("JsoncDocumentLoader parses documents without schema validation or localization")
{
    JsoncFile file;
    file.write(R"({"name":{"ja":"\u65e5\u672c","en":"Japan"},"values":[true,null,-2,3.5],"unknown":{}})");
    JsoncDocumentLoader loader(file.path);
    REQUIRE(loader.is_open());
    const auto document = loader.parse();
    CHECK(document["name"]["ja"] == "\xe6\x97\xa5\xe6\x9c\xac");
    CHECK(document["name"]["en"] == "Japan");
    CHECK(document["values"] == nlohmann::json::array({ true, nullptr, -2, 3.5 }));
    CHECK(document["unknown"].is_object());
}

TEST_CASE("JsoncDocumentLoader allows comments and trailing commas with unchanged parsed dump")
{
    JsoncFile file;
    file.write(R"(// leading comment
{
    "z": [1, 2,], /* block comment */
    "a": {"value": "// not a comment",},
} // trailing comment
)");
    JsoncDocumentLoader loader(file.path);
    REQUIRE(loader.is_open());
    const auto document = loader.parse();
    CHECK(document.dump() == R"({"a":{"value":"// not a comment"},"z":[1,2]})");
}

TEST_CASE("JsoncDocumentLoader distinguishes valid null and other JSON roots from missing files")
{
    JsoncFile file;
    for (const auto &text : { "null", "[]", "{}", "42", "true", "\"text\"" }) {
        CAPTURE(text);
        file.write(text);
        JsoncDocumentLoader loader(file.path);
        REQUIRE(loader.is_open());
        CHECK(loader.parse() == nlohmann::json::parse(text));
    }
}

TEST_CASE("JsoncDocumentLoader propagates parse errors for empty and malformed files")
{
    JsoncFile file;
    for (const auto &text : { "", " \n\t", "// comment only", "{", "{\"value\":}", "[1,", "/* unterminated", "{} {}" }) {
        CAPTURE(text);
        file.write(text);
        JsoncDocumentLoader loader(file.path);
        REQUIRE(loader.is_open());
        CHECK_THROWS_AS(loader.parse(), nlohmann::json::parse_error);
    }
}

TEST_CASE("JsoncDocumentLoader parse failure leaves caller output unchanged")
{
    JsoncFile file;
    file.write("{");
    JsoncDocumentLoader loader(file.path);
    REQUIRE(loader.is_open());
    nlohmann::json document = { { "sentinel", 7 } };
    const auto before = document;
    CHECK_THROWS_AS(document = loader.parse(), nlohmann::json::parse_error);
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

TEST_CASE("JsoncDocumentLoader::load returns the same document as parse for valid JSONC")
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
        {
            JsoncDocumentLoader loader(file.path);
            REQUIRE(loader.is_open());
            try {
                (void)loader.parse();
                FAIL("parse() must throw");
            } catch (const nlohmann::json::exception &e) {
                expected_message = e.what();
            }
        }
        const auto loaded = JsoncDocumentLoader::load(file.path);
        REQUIRE_FALSE(loaded.has_value());
        CHECK(loaded.error().kind == JsoncLoadErrorKind::PARSE_ERROR);
        CHECK_FALSE(loaded.error().message.empty());
        CHECK(loaded.error().message == expected_message);
    }
}
