#pragma once

enum class player_race_type;
struct player_type;
class PlayerRace {
public:
    PlayerRace() = delete;
    PlayerRace(player_type *player_ptr);
    virtual ~PlayerRace() = default;

    bool is_mimic_nonliving() const;
    bool can_resist_cut() const;
    bool equals(player_race_type prace) const;

private:
    player_type *player_ptr;
};
