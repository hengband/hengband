#pragma once

#include "system/angband.h"
#include "realm/realm-hex-numbers.h"

struct monap_type;
struct player_type;
class RealmHex {
public:
    RealmHex() = delete;
    RealmHex(player_type *player_ptr);
    RealmHex(player_type *player_ptr, monap_type *monap_ptr);
    virtual ~RealmHex() = default;

    bool stop_one_spell();
    void decrease_mana();
    bool stop_all_spells();
    bool is_casting_full_capacity() const;
    void continue_revenge();
    void store_vengeful_damage(HIT_POINT dam);
    bool check_hex_barrier(MONSTER_IDX m_idx, realm_hex_type type) const;
    bool is_spelling_specific(int hex) const;
    bool is_spelling_any() const;
    void eyes_on_eyes();
    void thief_teleport();

private:
    player_type *player_ptr;
    std::vector<int> casting_spells;
    monap_type *monap_ptr = nullptr;
    
    bool select_spell_stopping(char *out_val, char &choice);
    void display_casting_spells_list();
    bool process_mana_cost(const bool need_restart);
    bool check_restart();
    int calc_need_mana();
    void gain_exp_from_hex();
    bool gain_exp_skilled(const int spell);
    bool gain_exp_expert(const int spell);
    void gain_exp_master(const int spell);
};

#define casting_hex_flags(P_PTR) ((P_PTR)->magic_num1[0])
#define casting_hex_num(P_PTR) ((P_PTR)->magic_num2[0])
#define hex_revenge_power(P_PTR) ((P_PTR)->magic_num1[2])
#define hex_revenge_turn(P_PTR) ((P_PTR)->magic_num2[2])
#define hex_revenge_type(P_PTR) ((P_PTR)->magic_num2[1])
