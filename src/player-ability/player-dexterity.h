#pragma once

#include "player-status/player-basic-statistics.h"

struct player_type;
class PlayerDexterity : public PlayerBasicStatistics {
public:
    PlayerDexterity(player_type *player_ptr);

protected:
    void set_locals() override;
    int16_t race_value() override;
    int16_t time_effect_value() override;
    int16_t stance_value() override;
    int16_t mutation_value() override;
};
