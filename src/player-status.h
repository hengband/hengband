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
extern void sanity_blast(monster_type *m_ptr, bool necro);

extern void check_experience(void);
extern void wreck_the_pattern(void);
extern void cnv_stat(int val, char *out_val);
extern s16b modify_stat_value(int value, int amount);
extern long calc_score(void);

extern const s32b player_exp[PY_MAX_LEVEL];
extern const s32b player_exp_a[PY_MAX_LEVEL];


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

/*
 * Player "food" crucial values
 */
#define PY_FOOD_MAX     15000   /*!< 食べ過ぎ～満腹の閾値 / Food value (Bloated) */
#define PY_FOOD_FULL    10000   /*!< 満腹～平常の閾値 / Food value (Normal) */
#define PY_FOOD_ALERT   2000    /*!< 平常～空腹の閾値 / Food value (Hungry) */
#define PY_FOOD_WEAK    1000    /*!< 空腹～衰弱の閾値 / Food value (Weak) */
#define PY_FOOD_FAINT   500     /*!< 衰弱～衰弱(赤表示/麻痺)の閾値 / Food value (Fainting) */
#define PY_FOOD_STARVE  100     /*!< 衰弱(赤表示/麻痺)～飢餓ダメージの閾値 / Food value (Starving) */

/*
 * Player regeneration constants
 */
#define PY_REGEN_NORMAL         197     /* Regen factor*2^16 when full */
#define PY_REGEN_WEAK           98      /* Regen factor*2^16 when weak */
#define PY_REGEN_FAINT          33      /* Regen factor*2^16 when fainting */
#define PY_REGEN_HPBASE         1442    /* Min amount hp regen*2^16 */
#define PY_REGEN_MNBASE         524     /* Min amount mana regen*2^16 */
