#pragma once

#include "term/term-color-types.h"
#include "timed-effect/timed-effect.h"

#include <string_view>
#include <tuple>

enum class PlayerStunRank {
    NONE = 0,
    SLIGHT = 1,
    NORMAL = 2,
    HARD = 3,
    UNCONSCIOUS = 4,
    KNOCKED = 5,
};

class PlayerStun : public TimedEffect<struct TagPlayerStun> {
public:
    PlayerStun() = default;
    ~PlayerStun() = default;
    PlayerStun(const PlayerStun &) = delete;
    PlayerStun(PlayerStun &&) = delete;
    PlayerStun &operator=(const PlayerStun &) = delete;
    PlayerStun &operator=(PlayerStun &&) = delete;

    static PlayerStunRank get_rank(short value);
    static std::string_view get_stun_mes(PlayerStunRank stun_rank);
    static short get_accumulation(int rank);
    static int get_accumulation_rank(int total, int damage);

    PlayerStunRank get_rank() const;
    int get_magic_chance_penalty() const;
    int get_item_chance_penalty() const;
    short get_damage_penalty() const;
    bool is_knocked_out() const;
    std::tuple<term_color_type, std::string_view> get_expr() const;
};
