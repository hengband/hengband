#pragma once

typedef struct msr_type msr_type;
struct player_type;
void add_cheat_remove_flags_element(player_type *player_ptr, msr_type *msr_ptr);
void check_element_resistance(msr_type *msr_ptr);
