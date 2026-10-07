#include "test/temporary-json-files.h"
#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <future>
#include <iterator>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace {

std::string read_raw(const std::filesystem::path &path)
{
    std::ifstream input(path, std::ios::binary);
    input.exceptions(std::ios::badbit);
    if (!input) {
        throw std::runtime_error("Cannot read temporary test file");
    }
    return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}

static_assert(!std::is_copy_constructible_v<test::TemporaryJsonFiles>);
static_assert(!std::is_move_constructible_v<test::TemporaryJsonFiles>);
static_assert(!std::is_copy_assignable_v<test::TemporaryJsonFiles>);
static_assert(!std::is_move_assignable_v<test::TemporaryJsonFiles>);

}

TEST_CASE("TemporaryJsonFiles writes nested JSON and exact raw bytes and closes outputs")
{
    test::TemporaryJsonFiles files;
    const nlohmann::json data = { { "value", 17 }, { "rows", { "AB", "BA" } } };
    files.write("nested/towns/map.jsonc", data);
    CHECK(read_raw(files.directory / "nested/towns/map.jsonc") == data.dump());
    const std::string raw("// JSONC\n{\"value\":1}\n\0tail", 26);
    files.write_raw("nested/raw.jsonc", raw);
    const auto renamed = files.directory / "nested/renamed.jsonc";
    REQUIRE_NOTHROW(std::filesystem::rename(files.directory / "nested/raw.jsonc", renamed));
    CHECK(read_raw(renamed) == raw);
    files.write_raw("nested/renamed.jsonc", "{}");
    CHECK(read_raw(renamed) == "{}");
}

TEST_CASE("TemporaryJsonFiles isolates simultaneous live fixtures and their cleanup")
{
    std::promise<void> start;
    const auto ready = start.get_future().share();
    std::vector<std::future<std::unique_ptr<test::TemporaryJsonFiles>>> pending;
    pending.reserve(16);
    try {
        for (auto i = 0; i < 16; ++i) {
            pending.push_back(std::async(std::launch::async, [ready] {
                ready.wait();
                auto files = std::make_unique<test::TemporaryJsonFiles>();
                files->write_raw("shared.jsonc", "isolated");
                return files;
            }));
        }
    } catch (...) {
        start.set_value();
        throw;
    }
    start.set_value();
    std::vector<std::unique_ptr<test::TemporaryJsonFiles>> fixtures;
    std::set<std::filesystem::path> directories;
    for (auto &result : pending) {
        fixtures.push_back(result.get());
        CHECK(directories.insert(fixtures.back()->directory).second);
    }
    REQUIRE(fixtures.size() == 16);
    const auto removed = fixtures.front()->directory;
    fixtures.front().reset();
    CHECK_FALSE(std::filesystem::exists(removed));
    for (std::size_t i = 1; i < fixtures.size(); ++i) {
        CHECK(read_raw(fixtures[i]->directory / "shared.jsonc") == "isolated");
    }
    fixtures.clear();
    for (const auto &directory : directories) {
        CHECK_FALSE(std::filesystem::exists(directory));
    }
}

TEST_CASE("TemporaryJsonFiles cleans nested files during exception unwinding")
{
    test::TemporaryJsonFiles survivor;
    survivor.write_raw("keep.jsonc", "untouched");
    std::filesystem::path removed;
    const auto throw_with_files = [&removed] {
        test::TemporaryJsonFiles files;
        removed = files.directory;
        files.write("nested/map.jsonc", { { "value", 1 } });
        throw std::runtime_error("test exception");
    };
    CHECK_THROWS_AS(throw_with_files(), std::runtime_error);
    CHECK_FALSE(std::filesystem::exists(removed));
    CHECK(read_raw(survivor.directory / "keep.jsonc") == "untouched");
    const auto fail_with_files = [&removed] {
        test::TemporaryJsonFiles files;
        removed = files.directory;
        std::filesystem::create_directory(files.directory / "blocked.jsonc");
        files.write_raw("blocked.jsonc", "bad");
    };
    CHECK_THROWS_AS(fail_with_files(), std::ios_base::failure);
    CHECK_FALSE(std::filesystem::exists(removed));
    CHECK(read_raw(survivor.directory / "keep.jsonc") == "untouched");
}

TEST_CASE("TemporaryJsonFiles rejects escaping paths and reports file errors")
{
    test::TemporaryJsonFiles files;
    test::TemporaryJsonFiles survivor;
    survivor.write_raw("keep.jsonc", "untouched");
    CHECK_THROWS_AS(files.write_raw(survivor.directory / "keep.jsonc", "bad"), std::invalid_argument);
    CHECK_THROWS_AS(files.write_raw("../escape.jsonc", "bad"), std::invalid_argument);
    CHECK_THROWS_AS(files.write_raw("nested/../../escape.jsonc", "bad"), std::invalid_argument);
    CHECK_THROWS_AS(files.write_raw("", "bad"), std::invalid_argument);
    CHECK_THROWS_AS(files.write_raw(".", "bad"), std::invalid_argument);
    CHECK(read_raw(survivor.directory / "keep.jsonc") == "untouched");
    std::filesystem::create_directory(files.directory / "blocked.jsonc");
    CHECK_THROWS_AS(files.write_raw("blocked.jsonc", "bad"), std::ios_base::failure);
    files.write_raw("parent.jsonc", "not a directory");
    CHECK_THROWS_AS(files.write_raw("parent.jsonc/child.jsonc", "bad"), std::filesystem::filesystem_error);
    CHECK(read_raw(files.directory / "parent.jsonc") == "not a directory");
    CHECK_FALSE(std::filesystem::exists(files.directory / "nested"));
}
