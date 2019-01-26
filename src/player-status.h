extern concptr your_alignment(void);
extern int weapon_exp_level(int weapon_exp);
extern int riding_exp_level(int riding_exp);
extern int spell_exp_level(int spell_exp);

extern s16b calc_num_fire(object_type *o_ptr);
extern void calc_bonuses(void);
extern WEIGHT weight_limit(void);
extern bool has_melee_weapon(int i);
extern bool is_heavy_shoot(object_type *o_ptr);
extern bool heavy_armor(void);
extern void update_creature(player_type *creature_ptr);
extern BIT_FLAGS16 empty_hands(bool riding_control);
extern bool player_has_no_spellbooks(void);
