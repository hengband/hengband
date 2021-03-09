#pragma once
#include "system/angband.h"
#include "player/player-status-flags.h"

class PlayerStatusBase {
public:
    PlayerStatusBase(player_type *owner_ptr);
    PlayerStatusBase() = delete;
    virtual ~PlayerStatusBase() = default;
    virtual s16b getValue();
    virtual BIT_FLAGS getFlags();
    virtual BIT_FLAGS getBadFlags();
protected:
    s16b default_value;
    s16b min_value;
    s16b max_value;
    player_type *owner_ptr;
    tr_type tr_flag;
    tr_type tr_bad_flag;
    virtual void set_locals();
    virtual s16b race_value();
    virtual s16b class_value();
    virtual s16b personality_value();
    virtual s16b equipments_value();
    virtual s16b time_effect_value();
    virtual s16b battleform_value();
    virtual s16b mutation_value();
    virtual s16b riding_value();
    virtual s16b inventory_weight_value();
    virtual s16b action_value();
    virtual BIT_FLAGS equipments_flags();
    virtual BIT_FLAGS equipments_bad_flags();
};
