#pragma once

#include "system/angband.h"

constexpr int16_t CONCENT_RADAR_THRESHOLD = 2;
constexpr int16_t CONCENT_TELE_THRESHOLD = 5;

class SniperData {
public:
    SniperData() = default;
    int16_t concent{}; //!< Concentration level
    bool reset_concent{}; //!< Concentration reset flag

    void reset_concentration_flag();
};
