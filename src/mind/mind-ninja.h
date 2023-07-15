#pragma once

struct player_attack_type;
class PlayerType;
bool kawarimi(PlayerType *player_ptr, bool success);
bool rush_attack(PlayerType *player_ptr, bool *mdeath);
void process_surprise_attack(PlayerType *player_ptr, player_attack_type *pa_ptr);
void print_surprise_attack(player_attack_type *pa_ptr);
void calc_surprise_attack_damage(PlayerType *player_ptr, player_attack_type *pa_ptr);
bool hayagake(PlayerType *player_ptr);
bool set_superstealth(PlayerType *player_ptr, bool set);

enum class MindNinjaType : int;
bool cast_ninja_spell(PlayerType *player_ptr, MindNinjaType spell);
