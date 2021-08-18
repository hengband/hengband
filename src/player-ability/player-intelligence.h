#pragma once
#include "player-status/player-basic-statistics.h"

class PlayerIntelligence : public PlayerBasicStatistics {
public:
    using PlayerBasicStatistics::PlayerBasicStatistics;
    PlayerIntelligence() = delete;

protected:
    void set_locals() override;
    short battleform_value() override;
    short mutation_value() override;
};
