#pragma once

#include <cstdint>

/*
 * Allow use of "ASCII" and "EBCDIC" for "indexes", "digits",
 * and "Control-Characters".
 *
 * Note that all "index" values must be "lowercase letters", while
 * all "digits" must be "digits".  Control characters can be made
 * from any legal characters.
 */

constexpr uint8_t A2I(int ch)
{
    return ch - 'a';
}

constexpr char I2A(int i)
{
    return static_cast<char>(i + 'a');
}

constexpr uint8_t D2I(int ch)
{
    return ch - '0';
}

constexpr char I2D(int i)
{
    return static_cast<char>(i + '0');
}

/*!
 * @brief 一覧の位置をラベルの文字にする (a〜z の後は 0 から続ける)
 */
constexpr char I2AD(int i)
{
    return (i < 26) ? I2A(i) : I2D(i - 26);
}

constexpr auto AD_LABEL_COUNT = 26 + 10; //!< 一覧のラベル (a〜z・0〜9) の数

/*!
 * @brief ラベルの文字を一覧の位置にする (I2AD() の逆)
 * @return a〜z なら 0〜25、0〜9 なら 26〜35、それ以外の文字なら -1
 * @details ラベルは AD_LABEL_COUNT 個なので、I2AD() が AD_LABEL_COUNT 以上で返す記号 (: など) は位置に戻さない。
 */
constexpr int AD2I(char ch)
{
    if (('a' <= ch) && (ch <= 'z')) {
        return A2I(ch);
    }

    if (('0' <= ch) && (ch <= '9')) {
        return D2I(ch) + 26;
    }

    return -1;
}

template <typename T>
constexpr T KTRL(T ch)
{
    return static_cast<T>(ch & 0x1F);
}

constexpr char ESCAPE = '\033';
