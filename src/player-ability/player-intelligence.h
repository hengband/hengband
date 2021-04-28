#pragma once
#include "player-status/player-basic-statistics.h"

class PlayerIntelligence : public PlayerBasicStatistics {
public:
    using PlayerBasicStatistics::PlayerBasicStatistics;
    PlayerIntelligence() = delete;

protected:
    void set_locals() override;
    s16b battleform_value() override;
    s16b mutation_value() override;
};
