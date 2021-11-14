#pragma once

#include "player-status/player-basic-statistics.h"

class PlayerType;
class PlayerIntelligence : public PlayerBasicStatistics {
public:
    PlayerIntelligence(PlayerType *player_ptr);

protected:
    void set_locals() override;
    int16_t stance_value() override;
    int16_t mutation_value() override;
};
