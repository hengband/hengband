#pragma once

/*
 * Object information, for a specific object.
 *
 * Note that a "discount" on an item is permanent and never goes away.
 *
 * Note that inscriptions are now handled via the "quark_str()" function
 * applied to the "note" field, which will return NULL if "note" is zero.
 *
 * Note that "object" records are "copied" on a fairly regular basis,
 * and care must be taken when handling such objects.
 *
 * Note that "object flags" must now be derived from the object kind,
 * the artifact and ego-item indexes, and the two "xtra" fields.
 *
 * Each grid points to one (or zero) objects via the "o_idx"
 * field (above).  Each object then points to one (or zero) objects
 * via the "next_o_idx" field, forming a singly linked list, which
 * in game terms, represents a "stack" of objects in the same grid.
 *
 * Each monster points to one (or zero) objects via the "hold_o_idx"
 * field (below).  Each object then points to one (or zero) objects
 * via the "next_o_idx" field, forming a singly linked list, which
 * in game terms, represents a pile of objects held by the monster.
 *
 * The "held_m_idx" field is used to indicate which monster, if any,
 * is holding the object.  Objects being held have "ix=0" and "iy=0".
 */

#define OBJ_GOLD_LIST   480     /* First "gold" entry */

#include "object/object-util.h"

/* object1.c */
extern void reset_visuals(player_type *owner_ptr, void(*process_autopick_file_command)(char*));
extern void object_flags(object_type *o_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE]);
extern void object_flags_known(object_type *o_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE]);
extern concptr item_activation(object_type *o_ptr);

#define SCROBJ_FAKE_OBJECT  0x00000001
#define SCROBJ_FORCE_DETAIL 0x00000002
extern bool screen_object(player_type *player_ptr, object_type *o_ptr, BIT_FLAGS mode);

extern char index_to_label(int i);
extern s16b wield_slot(player_type *owner_ptr, object_type *o_ptr);

extern bool check_book_realm(player_type *owner_ptr, const tval_type book_tval, const OBJECT_SUBTYPE_VALUE book_sval);
extern object_type *ref_item(player_type *owner_ptr, INVENTORY_IDX item);
extern TERM_COLOR object_attr(object_type *o_ptr);

/*!
* todo ここに置くとコンパイルは通る (このファイルの冒頭やobject2.cでincludeするとコンパイルエラー)、しかし圧倒的にダメなので要調整
*/
#include "floor/floor.h"

/* object2.c */

extern OBJECT_SUBTYPE_VALUE coin_type;
extern s32b flag_cost(object_type *o_ptr, int plusses);

extern bool (*get_obj_num_hook)(KIND_OBJECT_IDX k_idx);

extern int bow_tval_ammo(object_type *o_ptr);

extern void excise_object_idx(floor_type *floor_ptr, OBJECT_IDX o_idx);
extern void delete_object_idx(player_type *owner_ptr, OBJECT_IDX o_idx);
extern void delete_object(player_type *owner_ptr, POSITION y, POSITION x);

extern OBJECT_IDX o_pop(floor_type *floor_ptr);
extern OBJECT_IDX get_obj_num(player_type *o_ptr, DEPTH level, BIT_FLAGS mode);
extern void object_known(object_type *o_ptr);
extern void object_aware(player_type *owner_ptr, object_type *o_ptr);
extern void object_tried(object_type *o_ptr);

extern byte value_check_aux1(object_type *o_ptr);
extern byte value_check_aux2(object_type *o_ptr);

extern PRICE object_value(object_type *o_ptr);
extern PRICE object_value_real(object_type *o_ptr);
extern void distribute_charges(object_type *o_ptr, object_type *q_ptr, int amt);
extern void reduce_charges(object_type *o_ptr, int amt);
extern int object_similar_part(object_type *o_ptr, object_type *j_ptr);
extern bool object_similar(object_type *o_ptr, object_type *j_ptr);
extern void object_absorb(object_type *o_ptr, object_type *j_ptr);
extern IDX lookup_kind(tval_type tval, OBJECT_SUBTYPE_VALUE sval);
extern void object_wipe(object_type *o_ptr);
extern void object_copy(object_type *o_ptr, object_type *j_ptr);
extern void object_prep(object_type *o_ptr, KIND_OBJECT_IDX k_idx);

extern void apply_magic(player_type *owner_type, object_type *o_ptr, DEPTH lev, BIT_FLAGS mode);

extern bool make_object(player_type *owner_ptr, object_type *j_ptr, BIT_FLAGS mode);
extern bool make_gold(floor_type *floor_ptr, object_type *j_ptr);
extern OBJECT_IDX drop_near(player_type *owner_type, object_type *o_ptr, PERCENTAGE chance, POSITION y, POSITION x);
extern void vary_item(player_type *owner_ptr, INVENTORY_IDX item, ITEM_NUMBER num);
extern void inven_item_charges(player_type *owner_ptr, INVENTORY_IDX item);
extern void inven_item_describe(player_type *owner_ptr, INVENTORY_IDX item);
extern void inven_item_increase(player_type *owner_ptr, INVENTORY_IDX item, ITEM_NUMBER num);
extern void inven_item_optimize(player_type *owner_ptr, INVENTORY_IDX item);
extern void floor_item_charges(floor_type *owner_ptr, INVENTORY_IDX item);
extern void floor_item_increase(floor_type *floor_ptr, INVENTORY_IDX item, ITEM_NUMBER num);
extern void floor_item_optimize(player_type *owner_ptr, INVENTORY_IDX item);
extern bool inven_carry_okay(object_type *o_ptr);
extern bool object_sort_comp(object_type *o_ptr, s32b o_value, object_type *j_ptr);
extern s16b inven_carry(player_type *owner_ptr, object_type *o_ptr);
extern INVENTORY_IDX inven_takeoff(player_type *owner_ptr, INVENTORY_IDX item, ITEM_NUMBER amt);
extern void drop_from_inventory(player_type *owner_type, INVENTORY_IDX item, ITEM_NUMBER amt);
extern void combine_pack(player_type *owner_ptr);
extern void reorder_pack(player_type *owner_ptr);
extern void display_koff(player_type *owner_ptr, KIND_OBJECT_IDX k_idx);
extern void torch_flags(object_type *o_ptr, BIT_FLAGS *flgs);
extern void torch_dice(object_type *o_ptr, DICE_NUMBER *dd, DICE_SID *ds);
extern void torch_lost_fuel(object_type *o_ptr);

/* The sval codes for TV_LITE */
#define SV_LITE_TORCH                    0
#define SV_LITE_LANTERN                  1
#define SV_LITE_FEANOR                   2
#define SV_LITE_EDISON                   3
#define SV_LITE_GALADRIEL                4
#define SV_LITE_ELENDIL                  5
#define SV_LITE_JUDGE                    6
#define SV_LITE_LORE                     7
#define SV_LITE_PALANTIR                 8
#define SV_LITE_FLY_STONE                9

/* The "sval" codes for TV_AMULET */
#define SV_AMULET_DOOM                   0
#define SV_AMULET_TELEPORT               1
#define SV_AMULET_ADORNMENT              2
#define SV_AMULET_SLOW_DIGEST            3
#define SV_AMULET_RESIST_ACID            4
#define SV_AMULET_SEARCHING              5
#define SV_AMULET_BRILLIANCE             6
#define SV_AMULET_CHARISMA               7
#define SV_AMULET_THE_MAGI               8
#define SV_AMULET_REFLECTION             9
#define SV_AMULET_CARLAMMAS             10
#define SV_AMULET_INGWE                 11
#define SV_AMULET_DWARVES               12
#define SV_AMULET_NO_MAGIC              13
#define SV_AMULET_NO_TELE               14
#define SV_AMULET_RESISTANCE            15
#define SV_AMULET_TELEPATHY             16
#define SV_AMULET_FARAMIR               17
#define SV_AMULET_BOROMIR               18
#define SV_AMULET_MAGATAMA              19
#define SV_AMULET_INROU                 20
#define SV_AMULET_INTELLIGENCE          21
#define SV_AMULET_WISDOM                22
#define SV_AMULET_MAGIC_MASTERY         23
#define SV_AMULET_NIGHT                 24

/* The sval codes for TV_RING */
#define SV_RING_WOE                      0
#define SV_RING_AGGRAVATION              1
#define SV_RING_WEAKNESS                 2
#define SV_RING_STUPIDITY                3
#define SV_RING_TELEPORTATION            4
#define SV_RING_SLOW_DIGESTION           6
#define SV_RING_LEVITATION_FALL             7
#define SV_RING_RESIST_FIRE              8
#define SV_RING_RESIST_COLD              9
#define SV_RING_SUSTAIN_STR             10
#define SV_RING_SUSTAIN_INT             11
#define SV_RING_SUSTAIN_WIS             12
#define SV_RING_SUSTAIN_CON             13
#define SV_RING_SUSTAIN_DEX             14
#define SV_RING_SUSTAIN_CHR             15
#define SV_RING_PROTECTION              16
#define SV_RING_ACID                    17
#define SV_RING_FLAMES                  18
#define SV_RING_ICE                     19
#define SV_RING_RESIST_POIS             20
#define SV_RING_FREE_ACTION             21
#define SV_RING_SEE_INVIS               22
#define SV_RING_SEARCHING               23
#define SV_RING_STR                     24
#define SV_RING_ELEC                    25
#define SV_RING_DEX                     26
#define SV_RING_CON                     27
#define SV_RING_ACCURACY                28
#define SV_RING_DAMAGE                  29
#define SV_RING_SLAYING                 30
#define SV_RING_SPEED                   31
#define SV_RING_FRAKIR                  32
#define SV_RING_TULKAS                  33
#define SV_RING_NARYA                   34
#define SV_RING_NENYA                   35
#define SV_RING_VILYA                   36
#define SV_RING_POWER                   37
#define SV_RING_RES_FEAR                38
#define SV_RING_RES_LD                  39
#define SV_RING_RES_NETHER              40
#define SV_RING_RES_NEXUS               41
#define SV_RING_RES_SOUND               42
#define SV_RING_RES_CONFUSION           43
#define SV_RING_RES_SHARDS              44
#define SV_RING_RES_DISENCHANT          45
#define SV_RING_RES_CHAOS               46
#define SV_RING_RES_BLINDNESS           47
#define SV_RING_LORDLY                  48
#define SV_RING_ATTACKS                 49
#define SV_RING_AHO                     50
#define SV_RING_SHOTS                   51
#define SV_RING_SUSTAIN                 52
#define SV_RING_DEC_MANA                53
#define SV_RING_WARNING                 54
#define SV_RING_MUSCLE                  55

#define SV_EXPRESS_CARD                  0

/* The "sval" codes for TV_STAFF */
#define SV_STAFF_DARKNESS                0
#define SV_STAFF_SLOWNESS                1
#define SV_STAFF_HASTE_MONSTERS          2
#define SV_STAFF_SUMMONING               3
#define SV_STAFF_TELEPORTATION           4
#define SV_STAFF_IDENTIFY                5
#define SV_STAFF_REMOVE_CURSE            6
#define SV_STAFF_STARLITE                7
#define SV_STAFF_LITE                    8
#define SV_STAFF_MAPPING                 9
#define SV_STAFF_DETECT_GOLD            10
#define SV_STAFF_DETECT_ITEM            11
#define SV_STAFF_DETECT_TRAP            12
#define SV_STAFF_DETECT_DOOR            13
#define SV_STAFF_DETECT_INVIS           14
#define SV_STAFF_DETECT_EVIL            15
#define SV_STAFF_CURE_LIGHT             16
#define SV_STAFF_CURING                 17
#define SV_STAFF_HEALING                18
#define SV_STAFF_THE_MAGI               19
#define SV_STAFF_SLEEP_MONSTERS         20
#define SV_STAFF_SLOW_MONSTERS          21
#define SV_STAFF_SPEED                  22
#define SV_STAFF_PROBING                23
#define SV_STAFF_DISPEL_EVIL            24
#define SV_STAFF_POWER                  25
#define SV_STAFF_HOLINESS               26
#define SV_STAFF_GENOCIDE               27
#define SV_STAFF_EARTHQUAKES            28
#define SV_STAFF_DESTRUCTION            29
#define SV_STAFF_ANIMATE_DEAD           30
#define SV_STAFF_MSTORM                 31
#define SV_STAFF_NOTHING                32


/* The "sval" codes for TV_WAND */
#define SV_WAND_HEAL_MONSTER             0
#define SV_WAND_HASTE_MONSTER            1
#define SV_WAND_CLONE_MONSTER            2
#define SV_WAND_TELEPORT_AWAY            3
#define SV_WAND_DISARMING                4
#define SV_WAND_TRAP_DOOR_DEST           5
#define SV_WAND_STONE_TO_MUD             6
#define SV_WAND_LITE                     7
#define SV_WAND_SLEEP_MONSTER            8
#define SV_WAND_SLOW_MONSTER             9
#define SV_WAND_CONFUSE_MONSTER         10
#define SV_WAND_FEAR_MONSTER            11
#define SV_WAND_HYPODYNAMIA              12
#define SV_WAND_POLYMORPH               13
#define SV_WAND_STINKING_CLOUD          14
#define SV_WAND_MAGIC_MISSILE           15
#define SV_WAND_ACID_BOLT               16
#define SV_WAND_CHARM_MONSTER           17
#define SV_WAND_FIRE_BOLT               18
#define SV_WAND_COLD_BOLT               19
#define SV_WAND_ACID_BALL               20
#define SV_WAND_ELEC_BALL               21
#define SV_WAND_FIRE_BALL               22
#define SV_WAND_COLD_BALL               23
#define SV_WAND_WONDER                  24
#define SV_WAND_DISINTEGRATE            25
#define SV_WAND_DRAGON_FIRE             26
#define SV_WAND_DRAGON_COLD             27
#define SV_WAND_DRAGON_BREATH           28
#define SV_WAND_ROCKETS                 29
#define SV_WAND_STRIKING                30
#define SV_WAND_GENOCIDE                31

/* The "sval" codes for TV_ROD */
#define SV_ROD_DETECT_TRAP               0
#define SV_ROD_DETECT_DOOR               1
#define SV_ROD_IDENTIFY                  2
#define SV_ROD_RECALL                    3
#define SV_ROD_ILLUMINATION              4
#define SV_ROD_MAPPING                   5
#define SV_ROD_DETECTION                 6
#define SV_ROD_PROBING                   7
#define SV_ROD_CURING                    8
#define SV_ROD_HEALING                   9
#define SV_ROD_RESTORATION              10
#define SV_ROD_SPEED                    11
#define SV_ROD_PESTICIDE                12
#define SV_ROD_TELEPORT_AWAY            13
#define SV_ROD_DISARMING                14
#define SV_ROD_LITE                     15
#define SV_ROD_SLEEP_MONSTER            16
#define SV_ROD_SLOW_MONSTER             17
#define SV_ROD_HYPODYNAMIA               18
#define SV_ROD_POLYMORPH                19
#define SV_ROD_ACID_BOLT                20
#define SV_ROD_ELEC_BOLT                21
#define SV_ROD_FIRE_BOLT                22
#define SV_ROD_COLD_BOLT                23
#define SV_ROD_ACID_BALL                24
#define SV_ROD_ELEC_BALL                25
#define SV_ROD_FIRE_BALL                26
#define SV_ROD_COLD_BALL                27
#define SV_ROD_HAVOC                    28
#define SV_ROD_STONE_TO_MUD             29
#define SV_ROD_AGGRAVATE                30


/* The "sval" codes for TV_SCROLL */

#define SV_SCROLL_DARKNESS               0
#define SV_SCROLL_AGGRAVATE_MONSTER      1
#define SV_SCROLL_CURSE_ARMOR            2
#define SV_SCROLL_CURSE_WEAPON           3
#define SV_SCROLL_SUMMON_MONSTER         4
#define SV_SCROLL_SUMMON_UNDEAD          5
#define SV_SCROLL_SUMMON_PET             6
#define SV_SCROLL_TRAP_CREATION          7
#define SV_SCROLL_PHASE_DOOR             8
#define SV_SCROLL_TELEPORT               9
#define SV_SCROLL_TELEPORT_LEVEL        10
#define SV_SCROLL_WORD_OF_RECALL        11
#define SV_SCROLL_IDENTIFY              12
#define SV_SCROLL_STAR_IDENTIFY         13
#define SV_SCROLL_REMOVE_CURSE          14
#define SV_SCROLL_STAR_REMOVE_CURSE     15
#define SV_SCROLL_ENCHANT_ARMOR         16
#define SV_SCROLL_ENCHANT_WEAPON_TO_HIT 17
#define SV_SCROLL_ENCHANT_WEAPON_TO_DAM 18
/* xxx enchant missile? */
#define SV_SCROLL_STAR_ENCHANT_ARMOR    20
#define SV_SCROLL_STAR_ENCHANT_WEAPON   21
#define SV_SCROLL_RECHARGING            22
#define SV_SCROLL_MUNDANITY             23
#define SV_SCROLL_LIGHT                 24
#define SV_SCROLL_MAPPING               25
#define SV_SCROLL_DETECT_GOLD           26
#define SV_SCROLL_DETECT_ITEM           27
#define SV_SCROLL_DETECT_TRAP           28
#define SV_SCROLL_DETECT_DOOR           29
#define SV_SCROLL_DETECT_INVIS          30
/* xxx (detect evil?) */
#define SV_SCROLL_SATISFY_HUNGER        32
#define SV_SCROLL_BLESSING              33
#define SV_SCROLL_HOLY_CHANT            34
#define SV_SCROLL_HOLY_PRAYER           35
#define SV_SCROLL_MONSTER_CONFUSION     36
#define SV_SCROLL_PROTECTION_FROM_EVIL  37
#define SV_SCROLL_RUNE_OF_PROTECTION    38
#define SV_SCROLL_TRAP_DOOR_DESTRUCTION 39
/* xxx */
#define SV_SCROLL_STAR_DESTRUCTION      41
#define SV_SCROLL_DISPEL_UNDEAD         42
#define SV_SCROLL_SPELL                 43
#define SV_SCROLL_GENOCIDE              44
#define SV_SCROLL_MASS_GENOCIDE         45
#define SV_SCROLL_ACQUIREMENT           46
#define SV_SCROLL_STAR_ACQUIREMENT      47
#define SV_SCROLL_FIRE                  48
#define SV_SCROLL_ICE                   49
#define SV_SCROLL_CHAOS                 50
#define SV_SCROLL_RUMOR                 51
#define SV_SCROLL_ARTIFACT              52
#define SV_SCROLL_RESET_RECALL          53
#define SV_SCROLL_SUMMON_KIN            54
#define SV_SCROLL_AMUSEMENT             55
#define SV_SCROLL_STAR_AMUSEMENT        56

/* The "sval" codes for TV_POTION */
#define SV_POTION_WATER                  0
#define SV_POTION_APPLE_JUICE            1
#define SV_POTION_SLIME_MOLD             2
/* xxx (fixed color) */
#define SV_POTION_SLOWNESS               4
#define SV_POTION_SALT_WATER             5
#define SV_POTION_POISON                 6
#define SV_POTION_BLINDNESS              7
/* xxx */
#define SV_POTION_BOOZE              9
/* xxx */
#define SV_POTION_SLEEP                 11
/* xxx */
#define SV_POTION_LOSE_MEMORIES         13
/* xxx */
#define SV_POTION_RUINATION             15
#define SV_POTION_DEC_STR               16
#define SV_POTION_DEC_INT               17
#define SV_POTION_DEC_WIS               18
#define SV_POTION_DEC_DEX               19
#define SV_POTION_DEC_CON               20
#define SV_POTION_DEC_CHR               21
#define SV_POTION_DETONATIONS           22
#define SV_POTION_DEATH                 23
#define SV_POTION_INFRAVISION           24
#define SV_POTION_DETECT_INVIS          25
#define SV_POTION_SLOW_POISON           26
#define SV_POTION_CURE_POISON           27
#define SV_POTION_BOLDNESS              28
#define SV_POTION_SPEED                 29
#define SV_POTION_RESIST_HEAT           30
#define SV_POTION_RESIST_COLD           31
#define SV_POTION_HEROISM               32
#define SV_POTION_BESERK_STRENGTH       33
#define SV_POTION_CURE_LIGHT            34
#define SV_POTION_CURE_SERIOUS          35
#define SV_POTION_CURE_CRITICAL         36
#define SV_POTION_HEALING               37
#define SV_POTION_STAR_HEALING          38
#define SV_POTION_LIFE                  39
#define SV_POTION_RESTORE_MANA          40
#define SV_POTION_RESTORE_EXP           41
#define SV_POTION_RES_STR               42
#define SV_POTION_RES_INT               43
#define SV_POTION_RES_WIS               44
#define SV_POTION_RES_DEX               45
#define SV_POTION_RES_CON               46
#define SV_POTION_RES_CHR               47
#define SV_POTION_INC_STR               48
#define SV_POTION_INC_INT               49
#define SV_POTION_INC_WIS               50
#define SV_POTION_INC_DEX               51
#define SV_POTION_INC_CON               52
#define SV_POTION_INC_CHR               53
/* xxx */
#define SV_POTION_AUGMENTATION          55
#define SV_POTION_ENLIGHTENMENT         56
#define SV_POTION_STAR_ENLIGHTENMENT    57
#define SV_POTION_SELF_KNOWLEDGE        58
#define SV_POTION_EXPERIENCE            59
#define SV_POTION_RESISTANCE            60
#define SV_POTION_CURING                61
#define SV_POTION_INVULNERABILITY       62
#define SV_POTION_NEW_LIFE              63
#define SV_POTION_NEO_TSUYOSHI          64
#define SV_POTION_TSUYOSHI              65
#define SV_POTION_POLYMORPH             66

/* The "sval" codes for TV_FOOD */
#define SV_FOOD_POISON                   0
#define SV_FOOD_BLINDNESS                1
#define SV_FOOD_PARANOIA                 2
#define SV_FOOD_CONFUSION                3
#define SV_FOOD_HALLUCINATION            4
#define SV_FOOD_PARALYSIS                5
#define SV_FOOD_WEAKNESS                 6
#define SV_FOOD_SICKNESS                 7
#define SV_FOOD_STUPIDITY                8
#define SV_FOOD_NAIVETY                  9
#define SV_FOOD_UNHEALTH                10
#define SV_FOOD_DISEASE                 11
#define SV_FOOD_CURE_POISON             12
#define SV_FOOD_CURE_BLINDNESS          13
#define SV_FOOD_CURE_PARANOIA           14
#define SV_FOOD_CURE_CONFUSION          15
#define SV_FOOD_CURE_SERIOUS            16
#define SV_FOOD_RESTORE_STR             17
#define SV_FOOD_RESTORE_CON             18
#define SV_FOOD_RESTORING               19
/* many missing mushrooms */
#define SV_FOOD_BISCUIT                 32
#define SV_FOOD_JERKY                   33
#define SV_FOOD_RATION                  35
#define SV_FOOD_SLIME_MOLD              36
#define SV_FOOD_WAYBREAD                37
#define SV_FOOD_PINT_OF_ALE             38
#define SV_FOOD_PINT_OF_WINE            39
