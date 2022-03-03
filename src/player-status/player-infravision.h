#pragma once
#include "player-status/player-status-base.h"

class PlayerInfravision : public PlayerStatusBase {
public:
    PlayerInfravision(PlayerType *player_ptr);

protected:
    void set_locals() override;
    int16_t race_bonus() override;
    int16_t time_effect_bonus() override;
    int16_t mutation_bonus() override;
};
