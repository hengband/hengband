#pragma once

#include <string>

enum class StunRank {
    NONE = 0,
    NORMAL = 1,
    HARD = 2,
    UNCONSCIOUS = 3,
};

class PlayerStun {
public:
    PlayerStun() = default;
    virtual ~PlayerStun() = default;

    short current() const;
    StunRank get_rank() const;
    StunRank get_rank(short value) const;
    std::string_view get_stun_mes(StunRank stun_rank) const;
    int decrease_chance() const;
    bool is_stunned() const;
    void reset();
    void set(short value);

private:
    short stun = 0;
};
