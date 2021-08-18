#pragma once
#include "system/angband.h"
#include "player/player-status-flags.h"

typedef struct player_type player_type;
class PlayerStatusBase {
public:
    PlayerStatusBase(player_type *owner_ptr);
    PlayerStatusBase() = delete;
    virtual ~PlayerStatusBase() = default;
    virtual short get_value();
    virtual BIT_FLAGS get_all_flags();
    virtual BIT_FLAGS get_good_flags();
    virtual BIT_FLAGS get_bad_flags();

protected:
    short default_value;
    short min_value;
    short max_value;
    player_type *owner_ptr;
    tr_type tr_flag;
    tr_type tr_bad_flag;
    virtual void set_locals();
    virtual short race_value();
    virtual short class_value();
    virtual short class_base_value();
    virtual short personality_value();
    virtual short equipments_value();
    virtual short time_effect_value();
    virtual short battleform_value();
    virtual short mutation_value();
    virtual short riding_value();
    virtual short inventory_weight_value();
    virtual short action_value();
    virtual short set_exception_value(short value);
    virtual BIT_FLAGS equipments_flags(tr_type check_flag);
    virtual BIT_FLAGS equipments_bad_flags(tr_type check_flag);
};
