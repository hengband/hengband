
extern void day_break(void);
extern void night_falls(void);
extern MONSTER_NUMBER count_all_hostile_monsters(floor_type *floor_ptr);
extern byte get_dungeon_feeling(floor_type *floor_ptr);
extern void update_dungeon_feeling(floor_type *floor_ptr);
extern void glow_deep_lava_and_bldg(floor_type *floor_ptr);
extern void forget_lite(void);
extern void update_lite(void);
extern void forget_view(void);
extern void update_view(void);
extern void update_mon_lite(floor_type *floor_ptr);
extern void clear_mon_lite(floor_type *floor_ptr);
