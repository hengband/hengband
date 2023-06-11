#pragma once

#include "term/term-color-types.h"
#include <string>
#include <tuple>

enum class PlayerCutRank {
    NONE = 0,
    GRAZING = 1,
    LIGHT = 2,
    BAD = 3,
    NASTY = 4,
    SEVERE = 5,
    DEEP = 6,
    MORTAL = 7,
};

class PlayerCut {
public:
    PlayerCut() = default;
    virtual ~PlayerCut() = default;

    static PlayerCutRank get_rank(short value);
    static std::string get_cut_mes(PlayerCutRank stun_rank);
    static short get_accumulation(int total, int damage);

    short current() const;
    PlayerCutRank get_rank() const;
    bool is_cut() const;
    std::tuple<term_color_type, std::string> get_expr() const;
    int get_damage() const;
    void set(short value);
    void reset();

private:
    short cut = 0;

    static int get_accumulation_rank(int total, int damage);
};
