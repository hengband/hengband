#pragma once

typedef struct player_type player_type;
void safe_setuid_drop(void);
void safe_setuid_grab(player_type *player_ptr);
