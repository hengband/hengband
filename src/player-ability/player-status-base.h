#pragma once
#include "system/angband.h"
#include "player/player-status-flags.h"

typedef struct player_type player_type;
class PlayerStatusBase {
public:
    PlayerStatusBase(player_type *owner_ptr);
    PlayerStatusBase() = delete;
    virtual ~PlayerStatusBase() = default;
    virtual s16b get_value();
    virtual BIT_FLAGS get_all_flags();
    virtual BIT_FLAGS get_good_flags();
    virtual BIT_FLAGS get_bad_flags();

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
    virtual s16b class_base_value();
    virtual s16b personality_value();
    virtual s16b equipments_value();
    virtual s16b time_effect_value();
    virtual s16b battleform_value();
    virtual s16b mutation_value();
    virtual s16b riding_value();
    virtual s16b inventory_weight_value();
    virtual s16b action_value();
    virtual s16b set_exception_value(s16b value);
    virtual BIT_FLAGS equipments_flags(tr_type check_flag);
    virtual BIT_FLAGS equipments_bad_flags(tr_type check_flag);
};


