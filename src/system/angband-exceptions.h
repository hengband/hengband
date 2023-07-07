#pragma once

#include <concepts>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string_view>

class SaveDataNotSupportedException : public std::runtime_error {
public:
    SaveDataNotSupportedException() = delete;
    using std::runtime_error::runtime_error;
};

namespace angband::exception::detail {

template <std::derived_from<std::exception> T>
[[noreturn]] void throw_exception(std::string_view msg, std::filesystem::path path, int line)
{
    std::stringstream ss;
    ss << path.filename().string() << ':' << line << ": " << msg;
    throw T(ss.str());
}

}

/*!
 * @brief EXCEPTION_T型の例外をスローする
 *
 * EXCEPTION_T型はstd::exceptionを継承している必要がある。
 * 例外メッセージは "バージョン: ファイル名:行番号: MSG" となる
 */
#define THROW_EXCEPTION(EXCEPTION_T, MSG)                                                  \
    do {                                                                                   \
        angband::exception::detail::throw_exception<EXCEPTION_T>(MSG, __FILE__, __LINE__); \
    } while (0)
