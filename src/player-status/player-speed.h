#pragma once
#include "player-status/player-status-base.h"

class PlayerSpeed : public PlayerStatusBase {
public:
    using PlayerStatusBase::PlayerStatusBase;
    PlayerSpeed() = delete;

protected:
    void set_locals() override;
    short race_value() override;
    short class_value() override;
    short personality_value() override;
    short equipments_value() override;
    short time_effect_value() override;
    short battleform_value() override;
    short mutation_value() override;
    short riding_value() override;
    short inventory_weight_value() override;
    short action_value() override;
    BIT_FLAGS equipments_flags(tr_type check_flag) override;
    short special_weapon_set_value();
    short set_exception_value(short value) override;
};
