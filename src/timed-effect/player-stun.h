#pragma once

#include "term/term-color-types.h"
#include <string>
#include <tuple>

enum class PlayerStunRank;
class PlayerStun {
public:
    PlayerStun() = default;
    virtual ~PlayerStun() = default;

    static PlayerStunRank get_rank(short value);
    static std::string_view get_stun_mes(PlayerStunRank stun_rank);
    static short get_accumulation(int rank);
    static int get_accumulation_rank(int total, int damage);

    short current() const;
    PlayerStunRank get_rank() const;
    int get_magic_chance_penalty() const;
    int get_item_chance_penalty() const;
    short get_damage_penalty() const;
    bool is_stunned() const;
    bool is_knocked_out() const;
    std::tuple<term_color_type, std::string_view> get_expr() const;
    void set(short value);
    void reset();

private:
    short stun = 0;
};
