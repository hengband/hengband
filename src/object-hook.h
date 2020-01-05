
extern bool item_tester_hook_convertible(object_type *o_ptr);
extern bool item_tester_hook_recharge(object_type *o_ptr);
extern bool item_tester_hook_orthodox_melee_weapons(object_type *o_ptr);
extern bool item_tester_hook_melee_weapon(object_type *o_ptr);
extern bool item_tester_hook_ammo(object_type *o_ptr);
extern bool item_tester_hook_broken_weapon(object_type *o_ptr);
extern bool item_tester_hook_boomerang(object_type *o_ptr);
extern bool item_tester_hook_eatable(object_type *o_ptr);
extern bool item_tester_hook_mochikae(object_type *o_ptr);
extern bool item_tester_hook_activate(object_type *o_ptr);
extern bool item_tester_hook_wear(object_type *o_ptr);
extern bool item_tester_hook_use(object_type *o_ptr);
extern bool item_tester_hook_quaff(object_type *o_ptr);
extern bool item_tester_hook_readable(object_type *o_ptr);
extern bool item_tester_hook_melee_ammo(object_type *o_ptr);
extern bool item_tester_hook_weapon_except_bow(object_type *o_ptr);
extern bool item_tester_hook_cursed(object_type *o_ptr);
extern bool item_tester_hook_nameless_weapon_armour(object_type *o_ptr);
extern bool item_tester_hook_identify(object_type *o_ptr);
extern bool item_tester_hook_identify_weapon_armour(object_type *o_ptr);
extern bool item_tester_hook_identify_fully(object_type *o_ptr);
extern bool item_tester_hook_identify_fully_weapon_armour(object_type *o_ptr);
extern bool item_tester_hook_recharge(object_type *o_ptr);

extern bool item_tester_learn_spell(object_type *o_ptr);
extern bool item_tester_high_level_book(object_type *o_ptr);
extern bool item_tester_refill_lantern(object_type *o_ptr);

extern bool object_is_potion(object_type *o_ptr);
extern bool object_is_bounty(object_type *o_ptr);
extern bool object_is_favorite(object_type *o_ptr);
extern bool object_is_rare(object_type *o_ptr);
extern bool object_is_weapon(object_type *o_ptr);
extern bool object_is_weapon_ammo(object_type *o_ptr);
extern bool object_is_ammo(object_type *o_ptr);
extern bool object_is_armour(object_type *o_ptr);
extern bool object_is_weapon_armour_ammo(object_type *o_ptr);
extern bool object_is_melee_weapon(object_type *o_ptr);
extern bool object_is_wearable(object_type *o_ptr);
extern bool object_is_equipment(object_type *o_ptr);
extern bool object_refuse_enchant_weapon(object_type *o_ptr);
extern bool object_allow_enchant_weapon(object_type *o_ptr);
extern bool object_allow_enchant_melee_weapon(object_type *o_ptr);
extern bool object_is_smith(object_type *o_ptr);
extern bool object_is_artifact(object_type *o_ptr);
extern bool object_is_random_artifact(object_type *o_ptr);
extern bool object_is_nameless(object_type *o_ptr);
extern bool object_allow_two_hands_wielding(object_type *o_ptr);
extern bool object_can_refill_torch(object_type *o_ptr);
extern bool can_player_destroy_object(object_type *o_ptr);
extern bool object_is_quest_target(object_type *o_ptr);

/*
 * Determine if a given inventory item is "aware"
 */
#define object_is_aware(T) (k_info[(T)->k_idx].aware)

 /*
  * Determine if a given inventory item is "tried"
  */
#define object_is_tried(T) (k_info[(T)->k_idx].tried)

 /*
  * Determine if a given inventory item is "known"
  * Test One -- Check for special "known" tag
  * Test Two -- Check for "Easy Know" + "Aware"
  */
#define object_is_known(T) (((T)->ident & (IDENT_KNOWN)) || (k_info[(T)->k_idx].easy_know && k_info[(T)->k_idx].aware))

#define OBJECT_IS_VALID(T) ((T)->k_idx != 0)

#define OBJECT_IS_HELD_MONSTER(T) ((T)->held_m_idx != 0)

/*
 * Artifacts use the "name1" field
 */
#define object_is_fixed_artifact(T) ((T)->name1 ? TRUE : FALSE)

/*
 * Ego-Items use the "name2" field
 */
#define object_is_ego(T) ((T)->name2 ? TRUE : FALSE)

/*
 * Broken items.
 */
#define object_is_broken(T) ((T)->ident & (IDENT_BROKEN))

/*
 * Cursed items.
 */
#define object_is_cursed(T) ((T)->curse_flags)

extern bool item_tester_okay(object_type *o_ptr, OBJECT_TYPE_VALUE tval);
