#pragma once

/*
 * This seems like a pretty standard "typedef"
 */
typedef int(*inven_func)(object_type *);

extern bool hates_acid(object_type *o_ptr);
extern bool hates_elec(object_type *o_ptr);
extern bool hates_fire(object_type *o_ptr);
extern bool hates_cold(object_type *o_ptr);
extern int set_acid_destroy(object_type *o_ptr);
extern int set_elec_destroy(object_type *o_ptr);
extern int set_fire_destroy(object_type *o_ptr);
extern int set_cold_destroy(object_type *o_ptr);
extern int inven_damage(inven_func typ, int perc);
extern HIT_POINT acid_dam(HIT_POINT dam, concptr kb_str, int monspell, bool aura);
extern HIT_POINT elec_dam(HIT_POINT dam, concptr kb_str, int monspell, bool aura);
extern HIT_POINT fire_dam(HIT_POINT dam, concptr kb_str, int monspell, bool aura);
extern HIT_POINT cold_dam(HIT_POINT dam, concptr kb_str, int monspell, bool aura);
