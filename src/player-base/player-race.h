#pragma once

#include "object-enchant/tr-flags.h"

enum class PlayerRaceType;
enum class PlayerRaceLife;
enum class PlayerRaceFood;
struct player_type;
struct player_race_info;
class PlayerRace {
public:
    PlayerRace(player_type *player_ptr, bool base_race = false);
    virtual ~PlayerRace() = default;

    TrFlags tr_flags() const;
    const player_race_info *get_info() const;
    PlayerRaceLife life() const;
    PlayerRaceFood food() const;

    bool is_mimic_nonliving() const;
    bool has_cut_immunity() const;
    bool has_stun_immunity() const;
    bool equals(PlayerRaceType prace) const;

    int16_t speed() const;
    int16_t additional_strength() const;
    int16_t additional_dexterity() const;
    int16_t additional_constitution() const;

private:
    player_type *player_ptr;
    bool base_race;
};
