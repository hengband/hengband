#pragma once
#include "player-status/player-status-base.h"

class PlayerSpeed : public PlayerStatusBase {
public:
    PlayerSpeed(player_type *player_ptr);

protected:
    void set_locals() override;
    int16_t race_value() override;
    int16_t class_value() override;
    int16_t personality_value() override;
    int16_t equipments_value() override;
    int16_t time_effect_value() override;
    int16_t stance_value() override;
    int16_t mutation_value() override;
    int16_t riding_value() override;
    int16_t inventory_weight_value() override;
    int16_t action_value() override;
    BIT_FLAGS equipments_flags(tr_type check_flag) override;
    int16_t special_weapon_set_value();
    int16_t set_exception_value(int16_t value) override;
};
