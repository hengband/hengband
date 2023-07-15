#pragma once

struct player_attack_type;
class PlayerType;
void decide_blood_sucking(PlayerType *player_ptr, player_attack_type *pa_ptr);
void calc_drain(player_attack_type *pa_ptr);
void process_drain(PlayerType *player_ptr, player_attack_type *pa_ptr, const bool is_human, bool *drain_msg);
