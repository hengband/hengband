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
    short battleform_value() override;
    short mutation_value() override;
    short set_exception_value(short value) override;
    short set_exception_use_status(short value) override;
};
