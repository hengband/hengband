#pragma once
#include "player-status/player-basic-statistics.h"

class PlayerWisdom : public PlayerBasicStatistics {
public:
    using PlayerBasicStatistics::PlayerBasicStatistics;
    PlayerWisdom() = delete;

protected:
    void set_locals() override;
    int16_t battleform_value() override;
    int16_t mutation_value() override;
};
