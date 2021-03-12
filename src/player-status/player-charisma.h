#pragma once
#include "player-status/player-basic-statistics.h"

class PlayerCharisma : public PlayerBasicStatistics {
public:
    using PlayerBasicStatistics::PlayerBasicStatistics;
    PlayerCharisma() = delete;

    BIT_FLAGS getAllFlags() override;
    BIT_FLAGS getBadFlags() override;

protected:
    void set_locals() override;
    s16b battleform_value() override;
    s16b mutation_value() override;
    s16b set_exception_value(s16b value) override;
    s16b set_exception_use_status(s16b value) override;
};
