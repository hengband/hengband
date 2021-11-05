#pragma once
#include "player-status/player-status-base.h"

class PlayerStealth : public PlayerStatusBase {
public:
    PlayerStealth(PlayerType *player_ptr);

    BIT_FLAGS get_bad_flags() override;

protected:
    void set_locals() override;
    int16_t race_value() override;
    int16_t class_value() override;
    int16_t class_base_value() override;
    int16_t personality_value() override;
    int16_t time_effect_value() override;
    int16_t mutation_value() override;
    int16_t set_exception_value(int16_t value) override;
    bool is_aggravated_s_fairy();
};
