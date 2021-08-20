#pragma once
#include "player-status/player-basic-statistics.h"

class PlayerCharisma : public PlayerBasicStatistics {
public:
    using PlayerBasicStatistics::PlayerBasicStatistics;
    PlayerCharisma() = delete;

    BIT_FLAGS get_all_flags() override;
    BIT_FLAGS get_bad_flags() override;

protected:
    void set_locals() override;
    int16_t battleform_value() override;
    int16_t mutation_value() override;
    int16_t set_exception_value(int16_t value) override;
    int16_t set_exception_use_status(int16_t value) override;
};
