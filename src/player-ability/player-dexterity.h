#pragma once
#include "player-status/player-basic-statistics.h"

class PlayerDexterity : public PlayerBasicStatistics {
public:
    using PlayerBasicStatistics::PlayerBasicStatistics;
    PlayerDexterity() = delete;

protected:
    void set_locals() override;
    short race_value() override;
    short time_effect_value() override;
    short battleform_value() override;
    short mutation_value() override;
};
