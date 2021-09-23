#pragma once

struct player_type;
class PlayerClass {
public:
    PlayerClass() = delete;
    PlayerClass(player_type *player_ptr);
    virtual ~PlayerClass() = default;

    bool can_resist_stun() const;
    bool is_wizard() const;

    bool lose_balance();

private:
    player_type *player_ptr;
};
