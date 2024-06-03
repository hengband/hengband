#pragma once

#include <cstdint>

enum class CharacterEncoding : uint8_t {
    UNKNOWN = 0,
    US_ASCII = 1,
    EUC_JP = 2,
    SHIFT_JIS = 3,
};
