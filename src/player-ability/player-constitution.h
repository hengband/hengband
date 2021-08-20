#pragma once
#include "player-status/player-basic-statistics.h"

class PlayerConstitution : public PlayerBasicStatistics {
public:
    using PlayerBasicStatistics::PlayerBasicStatistics;
    PlayerConstitution() = delete;

protected:
    void set_locals() override;
    int16_t race_value() override;
    int16_t time_effect_value() override;
    int16_t battleform_value() override;
    int16_t mutation_value() override;
};
