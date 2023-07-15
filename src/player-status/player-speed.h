#pragma once
#include "player-status/player-status-base.h"

class PlayerSpeed : public PlayerStatusBase {
public:
    PlayerSpeed(PlayerType *player_ptr);

protected:
    void set_locals() override;
    int16_t race_bonus() override;
    int16_t class_bonus() override;
    int16_t personality_bonus() override;
    int16_t equipments_bonus() override;
    int16_t time_effect_bonus() override;
    int16_t stance_bonus() override;
    int16_t mutation_bonus() override;
    int16_t riding_bonus() override;
    int16_t inventory_weight_bonus() override;
    int16_t action_bonus() override;
    BIT_FLAGS equipments_flags(tr_type check_flag) override;
    int16_t special_weapon_set_value();
    int16_t set_exception_bonus(int16_t value) override;
};
