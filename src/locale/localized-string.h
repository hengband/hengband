#pragma once

#include "locale/language-switcher.h"
#include <fmt/format.h>
#include <ostream>
#include <string>
#include <string_view>

/*!
 * @brief 日英両方の文字列を保持するクラス
 *
 * 日本語版において日本語の文字列とそれに対応する英語の文字列が
 * 必要な場合に使用する std::string のラッパー的なクラス
 */
class LocalizedString {
public:
    LocalizedString() = default;
    LocalizedString(const std::string &localized, const std::string &english);

    const std::string *operator->() const noexcept;
    operator std::string_view() const noexcept;
    const char *data() const noexcept;
    const std::string &string() const noexcept;
    const std::string &en_string() const noexcept;

    bool operator==(const LocalizedString &rhs) const noexcept = default;

private:
#ifdef JP
    std::string localized;
#endif
    std::string english;
};

std::ostream &operator<<(std::ostream &os, const LocalizedString &str);

/// LocalizedStringオブジェクトを fmt::format の引数として渡せるようにするためのテンプレート特殊化
template <>
struct fmt::formatter<LocalizedString> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx)
    {
        return ctx.begin();
    }
    template <typename FormatContext>
    constexpr auto format(const LocalizedString &str, FormatContext &ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", str.string());
    }
};
