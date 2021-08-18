#pragma once
#include "player-status/player-status-base.h"

class PlayerStealth : public PlayerStatusBase {
public:
    using PlayerStatusBase::PlayerStatusBase;
    PlayerStealth() = delete;

    BIT_FLAGS get_bad_flags() override;

protected:
    void set_locals() override;
    short race_value() override;
    short class_value() override;
    short class_base_value() override;
    short personality_value() override;
    short time_effect_value() override;
    short mutation_value() override;
    short set_exception_value(short value) override;
    bool is_aggravated_s_fairy();
};
