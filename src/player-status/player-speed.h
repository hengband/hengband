#pragma once
#include "player-status/player-status-base.h"

class PlayerSpeed : public PlayerStatusBase {
public:
    using PlayerStatusBase::PlayerStatusBase;
    PlayerSpeed() = delete;

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
    BIT_FLAGS equipments_flags(tr_type check_flag) override;
    s16b special_weapon_set_value();
    s16b set_exception_value(s16b value) override;
};
