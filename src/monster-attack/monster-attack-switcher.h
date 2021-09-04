#pragma once

typedef struct monap_type monap_type;
struct player_type;
void switch_monster_blow_to_player(player_type *target_ptr, monap_type *monap_ptr);
