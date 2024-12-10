/*!
 * @brief 文字種と文字色の組み合わせを表すデータクラス
 * @author Hourier
 * @date 2024/04/25
 */

#pragma once

#include <cstdint>

class DisplaySymbol {
public:
    constexpr DisplaySymbol()
        : DisplaySymbol(0, '\0')
    {
    }

    constexpr DisplaySymbol(uint8_t color, char character)
        : color(color)
        , character(character)
    {
    }

    bool operator==(const DisplaySymbol &other) const
    {
        return (this->color == other.color) && (this->character == other.character);
    }

    bool operator!=(const DisplaySymbol &other) const
    {
        return !(*this == other);
    }

    uint8_t color; //! @todo TERM_COLOR 型エイリアスをenum class に変えたら型を差し替える.
    char character;

    // @details 本来ASCII印字可能文字 (0x20～0x7F)だけ許可すべきだが、今のところそこまで厳密なチェックはしない.
    bool has_character() const
    {
        return this->character != '\0';
    }

    bool is_ascii_graphics() const
    {
        return this->color < 0x80;
    }
};

class DisplaySymbolPair {
public:
    constexpr DisplaySymbolPair(const DisplaySymbol &symbol_foreground, const DisplaySymbol &symbol_background)
        : symbol_foreground(symbol_foreground)
        , symbol_background(symbol_background)
    {
    }

    DisplaySymbol symbol_foreground;
    DisplaySymbol symbol_background;
};
