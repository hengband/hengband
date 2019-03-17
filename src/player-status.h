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

extern void take_turn(player_type *creature_ptr, PERCENTAGE need_cost);
extern void free_turn(player_type *creature_ptr);

extern bool player_place(POSITION y, POSITION x);

extern void wreck_the_pattern(void);

/* Temporary flags macro */
#define IS_FAST() (p_ptr->fast || music_singing(MUSIC_SPEED) || music_singing(MUSIC_SHERO))
#define IS_INVULN() (p_ptr->invuln || music_singing(MUSIC_INVULN))
#define IS_HERO() (p_ptr->hero || music_singing(MUSIC_HERO) || music_singing(MUSIC_SHERO))
#define IS_BLESSED() (p_ptr->blessed || music_singing(MUSIC_BLESS) || hex_spelling(HEX_BLESS))
#define IS_OPPOSE_ACID() (p_ptr->oppose_acid || music_singing(MUSIC_RESIST) || (p_ptr->special_defense & KATA_MUSOU))
#define IS_OPPOSE_ELEC() (p_ptr->oppose_elec || music_singing(MUSIC_RESIST) || (p_ptr->special_defense & KATA_MUSOU))
#define IS_OPPOSE_FIRE() (p_ptr->oppose_fire || music_singing(MUSIC_RESIST) || (p_ptr->special_defense & KATA_MUSOU))
#define IS_OPPOSE_COLD() (p_ptr->oppose_cold || music_singing(MUSIC_RESIST) || (p_ptr->special_defense & KATA_MUSOU))
#define IS_OPPOSE_POIS() (p_ptr->oppose_pois || music_singing(MUSIC_RESIST) || (p_ptr->special_defense & KATA_MUSOU))
#define IS_TIM_ESP() (p_ptr->tim_esp || music_singing(MUSIC_MIND) || (p_ptr->concent >= CONCENT_TELE_THRESHOLD))
#define IS_TIM_STEALTH() (p_ptr->tim_stealth || music_singing(MUSIC_STEALTH))

#define P_PTR_KI (p_ptr->magic_num1[0])