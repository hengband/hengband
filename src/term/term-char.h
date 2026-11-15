/*!
 * @file term-char.h
 * @brief 画面の1マスに置かれる文字
 */

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

/*!
 * @brief 画面の1マスに置かれる文字
 * @details
 * 1マスに1文字の内部文字コードのバイト列 (最大4バイト) を持つ。
 * 2マス幅の文字 (全角文字) は左のマスに文字全体を持ち、右のマスには文字を持たず
 * 「左のマスの文字の続き」という印だけを持つ。
 * ASCII の文字とタイルの番号 (最上位ビットの立ったバイト) は1バイトの文字として持つ。
 */
class TermChar {
public:
    constexpr TermChar() = default;

    //! 1バイトの文字 (ASCII の文字またはタイルの番号) を作る
    static constexpr TermChar from_byte(char ch)
    {
        TermChar tc;
        tc.bytes[0] = ch;
        tc.length = 1;
        return tc;
    }

    //! 2マス幅の文字を作る。4バイトを超える部分は捨てる
    static constexpr TermChar wide(std::string_view ch)
    {
        TermChar tc;
        tc.length = static_cast<uint8_t>(std::min(ch.length(), tc.bytes.size()));
        std::copy_n(ch.begin(), tc.length, tc.bytes.begin());
        tc.kind = Kind::WIDE_LEAD;
        return tc;
    }

    //! 2マス幅の文字の右のマスを作る
    static constexpr TermChar wide_trail()
    {
        TermChar tc;
        tc.length = 0;
        tc.kind = Kind::WIDE_TRAIL;
        return tc;
    }

    //! 文字のバイト列を返す。2マス幅の文字の右のマスなら空
    constexpr std::string_view view() const
    {
        return { this->bytes.data(), this->length };
    }

    //! 文字の先頭バイトを返す。2マス幅の文字の右のマスなら '\0'
    constexpr char front() const
    {
        return this->bytes[0];
    }

    constexpr bool is_wide_lead() const
    {
        return this->kind == Kind::WIDE_LEAD;
    }

    constexpr bool is_wide_trail() const
    {
        return this->kind == Kind::WIDE_TRAIL;
    }

    constexpr bool operator==(const TermChar &other) const = default;

private:
    enum class Kind : uint8_t {
        NARROW, //!< 1マス幅の文字
        WIDE_LEAD, //!< 2マス幅の文字の左のマス
        WIDE_TRAIL, //!< 2マス幅の文字の右のマス
    };

    std::array<char, 4> bytes{};
    uint8_t length = 1;
    Kind kind = Kind::NARROW;
};
