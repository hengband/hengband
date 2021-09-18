#pragma once

#include "term/term-color-types.h"
#include <string>
#include <tuple>

enum class PlayerStunRank {
    NONE = 0,
    NORMAL = 1,
    HARD = 2,
    UNCONSCIOUS = 3,
};

class PlayerStun {
public:
    PlayerStun() = default;
    virtual ~PlayerStun() = default;

    static PlayerStunRank get_rank(short value);
    static std::string_view get_stun_mes(PlayerStunRank stun_rank);
    
    short current() const;
    PlayerStunRank get_rank() const;
    int get_chance_penalty() const;
    short get_damage_penalty() const;
    bool is_stunned() const;
    std::tuple<term_color_type, std::string_view> get_expr() const;
    void reset();
    void set(short value);

private:
    short stun = 0;
};
