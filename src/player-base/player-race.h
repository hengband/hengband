#pragma once

struct player_type;
class PlayerRace {
public:
    PlayerRace() = delete;
    PlayerRace(player_type *player_ptr);
    virtual ~PlayerRace() = default;

    bool is_mimic_nonliving() const;

private:
    player_type *player_ptr;
};
