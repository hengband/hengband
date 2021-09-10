#pragma once

typedef struct msr_type msr_type;
struct player_type;
void add_cheat_remove_flags_others(player_type *player_ptr, msr_type *msr_ptr);
void check_high_resistances(player_type *player_ptr, msr_type *msr_ptr);
