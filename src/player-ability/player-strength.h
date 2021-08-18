#pragma once
#include "player-status/player-basic-statistics.h"

class PlayerStrength : public PlayerBasicStatistics {
public:
    using PlayerBasicStatistics::PlayerBasicStatistics;
    PlayerStrength() = delete;

protected:
    void set_locals() override;
    short race_value() override;
    short time_effect_value() override;
    short battleform_value() override;
    short mutation_value() override;
};
