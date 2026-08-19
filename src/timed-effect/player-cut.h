#pragma once

#include "term/term-color-types.h"
#include "timed-effect/timed-effect.h"

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

class PlayerCut : public TimedEffect<struct TagPlayerCut> {
public:
    PlayerCut() = default;
    ~PlayerCut() = default;
    PlayerCut(const PlayerCut &) = delete;
    PlayerCut(PlayerCut &&) = delete;
    PlayerCut &operator=(const PlayerCut &) = delete;
    PlayerCut &operator=(PlayerCut &&) = delete;

    static PlayerCutRank get_rank(short value);
    static std::string get_cut_mes(PlayerCutRank cut_rank);
    static short get_accumulation(int total, int damage);

    PlayerCutRank get_rank() const;
    std::tuple<term_color_type, std::string> get_expr() const;
    int get_damage() const;

private:
    static int get_accumulation_rank(int total, int damage);
};
