
extern bool heal_monster(DIRECTION dir, HIT_POINT dam);
extern bool speed_monster(DIRECTION dir, int power);
extern bool slow_monster(DIRECTION dir, int power);
extern bool sleep_monster(DIRECTION dir, int power);
extern bool stasis_monster(DIRECTION dir);    /* Like sleep, affects undead as well */
extern bool stasis_evil(DIRECTION dir);    /* Like sleep, affects undead as well */
extern bool confuse_monster(DIRECTION dir, PLAYER_LEVEL plev);
extern bool stun_monster(DIRECTION dir, PLAYER_LEVEL plev);
extern bool fear_monster(DIRECTION dir, PLAYER_LEVEL plev);
extern bool poly_monster(DIRECTION dir, int power);
extern bool clone_monster(DIRECTION dir);
extern void stop_singing(player_type *creature_ptr);
extern bool time_walk(player_type *creature_ptr);
extern void roll_hitdice(player_type *creature_ptr, SPOP_FLAGS options);
extern bool_hack life_stream(player_type *creature_ptr, bool_hack message, bool_hack virtue_change);
extern bool_hack heroism(player_type *creature_ptr, int base);
extern bool_hack berserk(player_type *creature_ptr, int base);
extern bool_hack cure_light_wounds(player_type *creature_ptr, DICE_NUMBER dice, DICE_SID sides);
extern bool_hack cure_serious_wounds(player_type *creature_ptr, DICE_NUMBER dice, DICE_SID sides);
extern bool_hack cure_critical_wounds(player_type *creature_ptr, HIT_POINT pow);
extern bool_hack true_healing(player_type *creature_ptr, HIT_POINT pow);
extern bool_hack restore_mana(player_type *creature_ptr, bool_hack magic_eater);
extern bool restore_all_status(player_type *creature_ptr);

extern bool fishing(player_type *creature_ptr);
extern bool cosmic_cast_off(player_type *creature_ptr, object_type *o_ptr);
extern void apply_nexus(monster_type *m_ptr, player_type *target_ptr);
extern void status_shuffle(player_type *creature_ptr);
