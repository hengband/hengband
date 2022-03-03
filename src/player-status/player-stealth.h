#pragma once
#include "player-status/player-status-base.h"

class PlayerStealth : public PlayerStatusBase {
public:
    PlayerStealth(PlayerType *player_ptr);

    BIT_FLAGS get_bad_flags() override;

protected:
    void set_locals() override;
    int16_t race_bonus() override;
    int16_t class_bonus() override;
    int16_t class_base_bonus() override;
    int16_t personality_bonus() override;
    int16_t time_effect_bonus() override;
    int16_t mutation_bonus() override;
    int16_t set_exception_bonus(int16_t value) override;
    bool is_aggravated_s_fairy();
};
