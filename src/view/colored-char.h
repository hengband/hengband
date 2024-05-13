/*!
 * @brief 文字種と文字色の組み合わせを表すデータクラス
 * @author Hourier
 * @date 2024/04/25
 */

#pragma once

#include <cstdint>

class ColoredChar {
public:
    constexpr ColoredChar()
        : ColoredChar(0, '\0')
    {
    }

    constexpr ColoredChar(uint8_t color, char character)
        : color(color)
        , character(character)
    {
    }

    bool operator==(const ColoredChar &other) const
    {
        return (this->color == other.color) && (this->character == other.character);
    }

    bool operator!=(const ColoredChar &other) const
    {
        return !(*this == other);
    }

    uint8_t color; //! @todo TERM_COLOR 型エイリアスをenum class に変えたら型を差し替える.
    char character;
};
