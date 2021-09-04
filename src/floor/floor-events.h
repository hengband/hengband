#pragma once

struct player_type;
struct floor_type;
void day_break(player_type *subject_ptr);
void night_falls(player_type *subject_ptr);
void update_dungeon_feeling(player_type *subject_ptr);
void glow_deep_lava_and_bldg(player_type *subject_ptr);
void forget_lite(floor_type *floor_ptr);
void forget_view(floor_type *floor_ptr);
