#pragma once

extern void day_break(player_type *subject_ptr);
extern void night_falls(player_type *subject_ptr);
extern MONSTER_NUMBER count_all_hostile_monsters(floor_type *floor_ptr);
extern void update_dungeon_feeling(player_type *subject_ptr);
extern void glow_deep_lava_and_bldg(player_type *subject_ptr);
extern void forget_lite(floor_type *floor_ptr);
extern void update_lite(player_type *subject_ptr);
extern void forget_view(floor_type *floor_ptr);
extern void update_view(player_type *subject_ptr, floor_type *floor_ptr);
extern void update_mon_lite(player_type *subject_ptr);
extern void clear_mon_lite(floor_type *floor_ptr);
