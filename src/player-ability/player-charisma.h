#pragma once

#include "player-status/player-basic-statistics.h"

class PlayerType;
class PlayerCharisma : public PlayerBasicStatistics {
public:
    PlayerCharisma(PlayerType *player_ptr);

    BIT_FLAGS get_all_flags() override;
    BIT_FLAGS get_bad_flags() override;

protected:
    void set_locals() override;
    int16_t stance_bonus() override;
    int16_t mutation_bonus() override;
    int16_t set_exception_bonus(int16_t value) override;
    int16_t set_exception_use_status(int16_t value) override;
};
