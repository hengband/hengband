#pragma once

#include "player-status/player-basic-statistics.h"

class PlayerType;
class PlayerDexterity : public PlayerBasicStatistics {
public:
    PlayerDexterity(PlayerType *player_ptr);

protected:
    void set_locals() override;
    int16_t race_bonus() override;
    int16_t time_effect_bonus() override;
    int16_t stance_bonus() override;
    int16_t mutation_bonus() override;
};
