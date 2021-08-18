#pragma once

#include "player-ability/player-ability-types.h"
#include "player-status/player-status-base.h"

class PlayerBasicStatistics : public PlayerStatusBase {
public:
    using PlayerStatusBase::PlayerStatusBase;
    PlayerBasicStatistics() = delete;

    void update_value();
    short modification_value();
    short get_value() override;

protected:
    player_ability_type ability_type;
    short race_value() override;
    short class_value() override;
    short personality_value() override;
    void update_top_status();
    void update_use_status();
    void update_index_status();
    virtual short set_exception_use_status(short value);
};

#include "player-ability/player-charisma.h"
#include "player-ability/player-constitution.h"
#include "player-ability/player-dexterity.h"
#include "player-ability/player-intelligence.h"
#include "player-ability/player-strength.h"
#include "player-ability/player-wisdom.h"
