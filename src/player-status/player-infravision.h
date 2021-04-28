#pragma once
#include "player-status/player-status-base.h"

class PlayerInfravision : public PlayerStatusBase {
public:
    using PlayerStatusBase::PlayerStatusBase;
    PlayerInfravision() = delete;

protected:
    void set_locals() override;
    s16b race_value() override;
    s16b time_effect_value() override;
    s16b mutation_value() override;
};
