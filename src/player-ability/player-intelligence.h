#pragma once
#include "player-status/player-basic-statistics.h"

class PlayerIntelligence : public PlayerBasicStatistics {
public:
    using PlayerBasicStatistics::PlayerBasicStatistics;
    PlayerIntelligence() = delete;

protected:
    void set_locals() override;
    int16_t battleform_value() override;
    int16_t mutation_value() override;
};
