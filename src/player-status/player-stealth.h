#pragma once
#include "player-status/player-status-base.h"

class PlayerStealth : public PlayerStatusBase {
public:
    using PlayerStatusBase::PlayerStatusBase;
    PlayerStealth() = delete;

    BIT_FLAGS get_bad_flags() override;

protected:
    void set_locals() override;
    s16b race_value() override;
    s16b class_value() override;
    s16b class_base_value() override;
    s16b personality_value() override;
    s16b time_effect_value() override;
    s16b mutation_value() override;
    s16b set_exception_value(s16b value) override;
    bool is_aggravated_s_fairy();
};
