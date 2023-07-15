#pragma once

#include "system/angband.h"

enum class MonkStanceType : uint8_t {
    NONE = 0,
    GENBU = 1, //!< 玄武の構え
    BYAKKO = 2, //!< 白虎の構え
    SEIRYU = 3, //!< 青竜の構え
    SUZAKU = 4, //!< 朱雀の構え
};

struct monk_data_type {
    MonkStanceType stance{};
};
