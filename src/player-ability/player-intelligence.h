#pragma once

#include "player-status/player-basic-statistics.h"

struct player_type;
class PlayerIntelligence : public PlayerBasicStatistics {
public:
    PlayerIntelligence(player_type *player_ptr);

protected:
    void set_locals() override;
    int16_t stance_value() override;
    int16_t mutation_value() override;
};
