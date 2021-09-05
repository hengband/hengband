#pragma once

#include "system/angband.h"

struct magic_type;
struct player_type;
class RealmHex {
public:
    RealmHex() = delete;
    RealmHex(player_type *caster_ptr);
    virtual ~RealmHex() = default;

    bool stop_hex_spell();
    void check_hex();

private:
    player_type *caster_ptr;

    bool select_spell_stopping(int *sp, char *out_val, char *choice);
    void process_mana_cost(const bool need_restart);
    bool check_restart();
    int calc_need_mana();
    void gain_exp_from_hex();
    bool gain_exp_skilled(const int spell);
    bool gain_exp_expert(const int spell, const magic_type *s_ptr);
    void gain_exp_master(const int spell, const magic_type *s_ptr);
};

bool stop_hex_spell_all(player_type *caster_ptr);
bool hex_spell_fully(player_type *caster_ptr);
void revenge_spell(player_type *caster_ptr);
void revenge_store(player_type *caster_ptr, HIT_POINT dam);
bool teleport_barrier(player_type *caster_ptr, MONSTER_IDX m_idx);
bool magic_barrier(player_type *caster_ptr, MONSTER_IDX m_idx);
bool multiply_barrier(player_type *caster_ptr, MONSTER_IDX m_idx);
bool hex_spelling(player_type *caster_ptr, int hex);
bool hex_spelling_any(player_type *caster_ptr);

#define casting_hex_flags(P_PTR) ((P_PTR)->magic_num1[0])
#define casting_hex_num(P_PTR) ((P_PTR)->magic_num2[0])
#define hex_revenge_power(P_PTR) ((P_PTR)->magic_num1[2])
#define hex_revenge_turn(P_PTR) ((P_PTR)->magic_num2[2])
#define hex_revenge_type(P_PTR) ((P_PTR)->magic_num2[1])
