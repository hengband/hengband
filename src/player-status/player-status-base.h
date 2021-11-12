#pragma once
#include "system/angband.h"
#include "player/player-status-flags.h"

class PlayerType;
class PlayerStatusBase {
public:
    virtual ~PlayerStatusBase() = default;
    virtual int16_t get_value();
    virtual BIT_FLAGS get_all_flags();
    virtual BIT_FLAGS get_good_flags();
    virtual BIT_FLAGS get_bad_flags();

protected:
    PlayerStatusBase(PlayerType *player_ptr);

    int16_t default_value;
    int16_t min_value;
    int16_t max_value;
    PlayerType *player_ptr;
    tr_type tr_flag;
    tr_type tr_bad_flag;
    virtual void set_locals();
    virtual int16_t race_value();
    virtual int16_t class_value();
    virtual int16_t class_base_value();
    virtual int16_t personality_value();
    virtual int16_t equipments_value();
    virtual int16_t time_effect_value();
    virtual int16_t stance_value();
    virtual int16_t mutation_value();
    virtual int16_t riding_value();
    virtual int16_t inventory_weight_value();
    virtual int16_t action_value();
    virtual int16_t set_exception_value(int16_t value);
    virtual BIT_FLAGS equipments_flags(tr_type check_flag);
    virtual BIT_FLAGS equipments_bad_flags(tr_type check_flag);
};
