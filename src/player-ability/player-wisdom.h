#pragma once

#include "player-status/player-basic-statistics.h"

struct player_type;
class PlayerWisdom : public PlayerBasicStatistics {
public:
    PlayerWisdom(player_type *player_ptr);

protected:
    void set_locals() override;
    int16_t battleform_value() override;
    int16_t mutation_value() override;
};
