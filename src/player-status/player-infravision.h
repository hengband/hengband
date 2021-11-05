﻿#pragma once
#include "player-status/player-status-base.h"

class PlayerInfravision : public PlayerStatusBase {
public:
    PlayerInfravision(PlayerType *player_ptr);

protected:
    void set_locals() override;
    int16_t race_value() override;
    int16_t time_effect_value() override;
    int16_t mutation_value() override;
};
