#pragma once

typedef struct monap_type monap_type;
struct player_type;
void calc_blow_disease(player_type *target_ptr, monap_type *monap_ptr);
void calc_blow_lose_strength(player_type *target_ptr, monap_type *monap_ptr);
void calc_blow_lose_intelligence(player_type *target_ptr, monap_type *monap_ptr);
void calc_blow_lose_wisdom(player_type *target_ptr, monap_type *monap_ptr);
void calc_blow_lose_dexterity(player_type *target_ptr, monap_type *monap_ptr);
void calc_blow_lose_constitution(player_type *target_ptr, monap_type *monap_ptr);
void calc_blow_lose_charisma(player_type *target_ptr, monap_type *monap_ptr);
void calc_blow_lose_all(player_type *target_ptr, monap_type *monap_ptr);
