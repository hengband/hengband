#pragma once

#include "system/angband.h"

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

template <typename T>
constexpr T KTRL(T ch)
{
    return static_cast<T>(ch & 0x1F);
}

constexpr char ESCAPE = '\033';
