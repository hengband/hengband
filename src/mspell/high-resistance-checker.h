#pragma once

struct msr_type;
class PlayerType;
void add_cheat_remove_flags_others(PlayerType *player_ptr, msr_type *msr_ptr);
void check_high_resistances(PlayerType *player_ptr, msr_type *msr_ptr);
