#pragma once

#include "system/angband.h"

struct player_type;
class RealmHex {
public:
    RealmHex() = delete;
    RealmHex(player_type *caster_ptr);
    virtual ~RealmHex() = default;

    bool stop_hex_spell();
    void check_hex();
    bool stop_hex_spell_all();
    bool hex_spell_fully() const;
    void revenge_spell();
    void revenge_store(HIT_POINT dam);
    bool teleport_barrier(MONSTER_IDX m_idx);
    bool magic_barrier(MONSTER_IDX m_idx);

private:
    player_type *caster_ptr;

    bool select_spell_stopping(int *sp, char *out_val, char *choice);
    void display_spells_list(int *sp);
    void process_mana_cost(const bool need_restart);
    bool check_restart();
    int calc_need_mana();
    void gain_exp_from_hex();
    bool gain_exp_skilled(const int spell);
    bool gain_exp_expert(const int spell);
    void gain_exp_master(const int spell);
};

bool multiply_barrier(player_type *caster_ptr, MONSTER_IDX m_idx);
bool hex_spelling(player_type *caster_ptr, int hex);
bool hex_spelling_any(player_type *caster_ptr);

#define casting_hex_flags(P_PTR) ((P_PTR)->magic_num1[0])
#define casting_hex_num(P_PTR) ((P_PTR)->magic_num2[0])
#define hex_revenge_power(P_PTR) ((P_PTR)->magic_num1[2])
#define hex_revenge_turn(P_PTR) ((P_PTR)->magic_num2[2])
#define hex_revenge_type(P_PTR) ((P_PTR)->magic_num2[1])
