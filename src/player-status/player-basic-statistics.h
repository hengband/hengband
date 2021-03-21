#pragma once
#include "player-status/player-status-base.h"

class PlayerBasicStatistics : public PlayerStatusBase {
public:
    using PlayerStatusBase::PlayerStatusBase;
    PlayerBasicStatistics() = delete;

    void update_value();
    s16b modification_value();
    s16b get_value() override;

protected:
    base_status_type status_type;
    s16b race_value() override;
    s16b class_value() override;
    s16b personality_value() override;
    void update_top_status();
    void update_use_status();
    void update_index_status();
    virtual s16b set_exception_use_status(s16b value);
};

#include "player-status/player-strength.h"
#include "player-status/player-intelligence.h"
#include "player-status/player-wisdom.h"
#include "player-status/player-dextarity.h"
#include "player-status/player-constitution.h"
#include "player-status/player-charisma.h"
