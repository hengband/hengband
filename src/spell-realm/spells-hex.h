#pragma once

#include "system/angband.h"
#include "realm/realm-hex-numbers.h"
#include <tuple>

enum class SpellHexRevengeType : byte {
    NONE = 0,
    PATIENCE = 1,
    REVENGE = 2,
};

struct monap_type;
struct player_type;
struct spell_hex_data_type;
class SpellHex {
public:
    SpellHex() = delete;
    SpellHex(player_type *player_ptr);
    SpellHex(player_type *player_ptr, monap_type *monap_ptr);
    virtual ~SpellHex() = default;

    bool stop_spells_with_selection();
    void decrease_mana();
    void stop_all_spells();
    bool is_casting_full_capacity() const;
    void continue_revenge();
    void store_vengeful_damage(HIT_POINT dam);
    bool check_hex_barrier(MONSTER_IDX m_idx, spell_hex_type type) const;
    bool is_spelling_specific(int hex) const;
    bool is_spelling_any() const;
    void eyes_on_eyes();
    void thief_teleport();
    void set_casting_flag(spell_hex_type type);
    void reset_casting_flag(spell_hex_type type);
    int32_t get_casting_num() const;
    int32_t get_revenge_power() const;
    void set_revenge_power(int32_t power, bool substitution);
    byte get_revenge_turn() const;
    void set_revenge_turn(byte power, bool substitution);
    SpellHexRevengeType get_revenge_type() const;
    void set_revenge_type(SpellHexRevengeType type);

private:
    player_type *player_ptr;
    std::vector<int> casting_spells;
    monap_type *monap_ptr = nullptr;
    std::shared_ptr<spell_hex_data_type> spell_hex_data;

    std::tuple<bool, bool, char> select_spell_stopping(char *out_val);
    void display_casting_spells_list();
    bool process_mana_cost(const bool need_restart);
    bool check_restart();
    int calc_need_mana();
    void gain_exp();
    bool gain_exp_skilled(const int spell);
    bool gain_exp_expert(const int spell);
    void gain_exp_master(const int spell);
};

#define hex_revenge_type(P_PTR) ((P_PTR)->magic_num2[1])
