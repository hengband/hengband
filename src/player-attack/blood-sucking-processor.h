#pragma once

struct player_attack_type;
struct player_type;
void decide_blood_sucking(player_type *player_ptr, player_attack_type *pa_ptr);
void calc_drain(player_attack_type *pa_ptr);
void process_drain(player_type *player_ptr, player_attack_type *pa_ptr, const bool is_human, bool *drain_msg);
