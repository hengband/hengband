#pragma once
#include "player-status/player-status-base.h"

class PlayerSpeed : public PlayerStatusBase {
public:
    PlayerSpeed(player_type *owner_ptr)
        : PlayerStatusBase(owner_ptr){};

    s16b getValue() override;

protected:
    void set_locals() override;
    s16b race_value() override;
    s16b class_value() override;
    s16b personality_value() override;
    s16b equipments_value() override;
    s16b time_effect_value() override;
    s16b battleform_value() override;
    s16b mutation_value() override;
    s16b riding_value() override;
    s16b inventory_weight_value() override;
    s16b action_value() override;
    BIT_FLAGS equipments_flags() override;
    s16b special_weapon_set_value();
};
