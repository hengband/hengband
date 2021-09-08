#pragma once

struct player_type;
class PlayerClass {
public:
    PlayerClass() = delete;
    PlayerClass(player_type *player_ptr);
    virtual ~PlayerClass() = default;

    bool can_resist_stun() const;

private:
    player_type *player_ptr;
};
