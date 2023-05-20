#pragma once

#include "object-enchant/tr-flags.h"

enum class PlayerRaceType;
enum class PlayerRaceLifeType;
enum class PlayerRaceFoodType;
class PlayerType;
struct player_race_info;
class PlayerRace {
public:
    PlayerRace(PlayerType *player_ptr, bool base_race = false);
    virtual ~PlayerRace() = default;

    TrFlags tr_flags() const;
    const player_race_info *get_info() const;
    PlayerRaceLifeType life() const;
    PlayerRaceFoodType food() const;

    bool is_mimic_nonliving() const;
    bool has_cut_immunity() const;
    bool has_stun_immunity() const;
    bool equals(PlayerRaceType prace) const;

    int16_t speed() const;
    int16_t additional_strength() const;
    int16_t additional_dexterity() const;
    int16_t additional_constitution() const;
    char get_summon_symbol() const;

private:
    PlayerType *player_ptr;
    bool base_race;
};
