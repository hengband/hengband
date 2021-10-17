#pragma once

#include "player-ability/player-ability-types.h"
#include "player-status/player-status-base.h"

struct player_type;
class PlayerBasicStatistics : public PlayerStatusBase {
public:
    void update_value();
    int16_t modification_value();
    int16_t get_value() override;

protected:
    PlayerBasicStatistics(player_type *player_ptr);

    player_ability_type ability_type{};
    int16_t race_value() override;
    int16_t class_value() override;
    int16_t personality_value() override;
    void update_top_status();
    void update_use_status();
    void update_index_status();
    virtual int16_t set_exception_use_status(int16_t value);
};

#include "player-ability/player-charisma.h"
#include "player-ability/player-constitution.h"
#include "player-ability/player-dexterity.h"
#include "player-ability/player-intelligence.h"
#include "player-ability/player-strength.h"
#include "player-ability/player-wisdom.h"
