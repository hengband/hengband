#pragma once
#include "defines.h"

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

#define TR_STR                 0      /* STR += "pval" */
#define TR_INT                 1      /* INT += "pval" */
#define TR_WIS                 2      /* WIS += "pval" */
#define TR_DEX                 3      /* DEX += "pval" */
#define TR_CON                 4      /* CON += "pval" */
#define TR_CHR                 5      /* CHR += "pval" */
#define TR_MAGIC_MASTERY       6      /* Later */
#define TR_FORCE_WEAPON        7      /* Later */
#define TR_STEALTH             8      /* Stealth += "pval" */
#define TR_SEARCH              9      /* Search += "pval" */
#define TR_INFRA               10     /* Infra += "pval" */
#define TR_TUNNEL              11     /* Tunnel += "pval" */
#define TR_SPEED               12     /* Speed += "pval" */
#define TR_BLOWS               13     /* Blows += "pval" */
#define TR_CHAOTIC             14
#define TR_VAMPIRIC            15
#define TR_SLAY_ANIMAL         16
#define TR_SLAY_EVIL           17
#define TR_SLAY_UNDEAD         18
#define TR_SLAY_DEMON          19
#define TR_SLAY_ORC            20
#define TR_SLAY_TROLL          21
#define TR_SLAY_GIANT          22
#define TR_SLAY_DRAGON         23
#define TR_KILL_DRAGON         24     /* Execute Dragon */
#define TR_VORPAL              25     /* Later */
#define TR_IMPACT              26     /* Cause Earthquakes */
#define TR_BRAND_POIS          27
#define TR_BRAND_ACID          28
#define TR_BRAND_ELEC          29
#define TR_BRAND_FIRE          30
#define TR_BRAND_COLD          31

#define TR_SUST_STR            32
#define TR_SUST_INT            33
#define TR_SUST_WIS            34
#define TR_SUST_DEX            35
#define TR_SUST_CON            36
#define TR_SUST_CHR            37
#define TR_RIDING              38
#define TR_EASY_SPELL          39
#define TR_IM_ACID             40
#define TR_IM_ELEC             41
#define TR_IM_FIRE             42
#define TR_IM_COLD             43
#define TR_THROW               44     /* Later */
#define TR_REFLECT             45     /* Reflect 'bolts' */
#define TR_FREE_ACT            46     /* Free Action */
#define TR_HOLD_EXP            47     /* Hold EXP */
#define TR_RES_ACID            48
#define TR_RES_ELEC            49
#define TR_RES_FIRE            50
#define TR_RES_COLD            51
#define TR_RES_POIS            52
#define TR_RES_FEAR            53     /* Added for Zangband */
#define TR_RES_LITE            54
#define TR_RES_DARK            55
#define TR_RES_BLIND           56
#define TR_RES_CONF            57
#define TR_RES_SOUND           58
#define TR_RES_SHARDS          59
#define TR_RES_NETHER          60
#define TR_RES_NEXUS           61
#define TR_RES_CHAOS           62
#define TR_RES_DISEN           63

#define TR_SH_FIRE             64     /* Immolation (Fire) */
#define TR_SH_ELEC             65     /* Electric Sheath */
#define TR_SLAY_HUMAN          66     /* Slay human */
#define TR_SH_COLD             67     /* cold aura */
#define TR_NO_TELE             68     /* Anti-teleportation */
#define TR_NO_MAGIC            69     /* Anti-magic */
#define TR_DEC_MANA            70     /* ??? */
#define TR_TY_CURSE            71     /* The Ancient Curse */
#define TR_WARNING             72     /* Warning */
#define TR_HIDE_TYPE           73     /* Hide "pval" description */
#define TR_SHOW_MODS           74     /* Always show Tohit/Todam */
#define TR_SLAY_GOOD           75
#define TR_LEVITATION          76     /* Feather Falling */
#define TR_LITE_1              77     /* Light Radius 1*/
#define TR_SEE_INVIS           78     /* See Invisible */
#define TR_TELEPATHY           79     /* Telepathy */
#define TR_SLOW_DIGEST         80     /* Item slows down digestion */
#define TR_REGEN               81     /* Item induces regeneration */
#define TR_XTRA_MIGHT          82     /* Bows get extra multiplier */
#define TR_XTRA_SHOTS          83     /* Bows get extra shots */
#define TR_IGNORE_ACID         84     /* Item ignores Acid Damage */
#define TR_IGNORE_ELEC         85     /* Item ignores Elec Damage */
#define TR_IGNORE_FIRE         86     /* Item ignores Fire Damage */
#define TR_IGNORE_COLD         87     /* Item ignores Cold Damage */
#define TR_ACTIVATE            88     /* Item can be activated */
#define TR_DRAIN_EXP           89     /* Item drains Experience */
#define TR_TELEPORT            90     /* Item teleports player */
#define TR_AGGRAVATE           91     /* Item aggravates monsters */
#define TR_BLESSED             92     /* Item is Blessed */
#define TR_ES_ATTACK           93     /* Fake flag for Smith */
#define TR_ES_AC               94     /* Fake flag for Smith */
#define TR_KILL_GOOD           95

#define TR_KILL_ANIMAL         96
#define TR_KILL_EVIL           97
#define TR_KILL_UNDEAD         98
#define TR_KILL_DEMON          99
#define TR_KILL_ORC            100
#define TR_KILL_TROLL          101
#define TR_KILL_GIANT          102
#define TR_KILL_HUMAN          103
#define TR_ESP_ANIMAL          104
#define TR_ESP_UNDEAD          105
#define TR_ESP_DEMON           106
#define TR_ESP_ORC             107
#define TR_ESP_TROLL           108
#define TR_ESP_GIANT           109
#define TR_ESP_DRAGON          110
#define TR_ESP_HUMAN           111
#define TR_ESP_EVIL            112
#define TR_ESP_GOOD            113
#define TR_ESP_NONLIVING       114
#define TR_ESP_UNIQUE          115
#define TR_FULL_NAME           116
#define TR_FIXED_FLAVOR        117
#define TR_ADD_L_CURSE         118
#define TR_ADD_H_CURSE         119
#define TR_DRAIN_HP            120
#define TR_DRAIN_MANA          121
#define TR_LITE_2			   122
#define TR_LITE_3			   123
#define TR_LITE_M1			   124    /* Permanent decrease Light Area (-1) */
#define TR_LITE_M2			   125    /* Permanent decrease Light Area (-1) */
#define TR_LITE_M3			   126    /* Permanent decrease Light Area (-1) */
#define TR_LITE_FUEL		   127	  /* Lights need Fuels */

#define TR_CALL_ANIMAL         128
#define TR_CALL_DEMON          129
#define TR_CALL_DRAGON         130
#define TR_CALL_UNDEAD         131
#define TR_COWARDICE           132
#define TR_LOW_MELEE           133
#define TR_LOW_AC              134
#define TR_LOW_MAGIC           135
#define TR_FAST_DIGEST         136
#define TR_SLOW_REGEN          137

#define TR_FLAG_MAX            138
#define TR_FLAG_SIZE           5


typedef struct object_type object_type;

struct object_type
{
	KIND_OBJECT_IDX k_idx;			/* Kind index (zero if "dead") */

	POSITION iy;			/* Y-position on map, or zero */
	POSITION ix;			/* X-position on map, or zero */

	/*
	 * The values for the "tval" field of various objects.
	 *
	 * This value is the primary means by which items are sorted in the
	 * player inventory_list, followed by "sval" and "cost".
	 *
	 * Note that a "BOW" with tval = 19 and sval S = 10*N+P takes a missile
	 * weapon with tval = 16+N, and does (xP) damage when so combined.  This
	 * fact is not actually used in the source, but it kind of interesting.
	 *
	 * Note that as of 2.7.8, the "item flags" apply to all items, though
	 * only armor and weapons and a few other items use any of these flags.
	 */

	#define TV_SKELETON      1      /* Skeletons ('s'), not specified */
	#define TV_BOTTLE        2      /* Empty bottles ('!') */
	#define TV_JUNK          3      /* Sticks, Pottery, etc ('~') */
	#define TV_WHISTLE       4      /* Whistle ('~') */
	#define TV_SPIKE         5      /* Spikes ('~') */
	#define TV_CHEST         7      /* Chests ('&') */
	#define TV_FIGURINE      8      /* Magical figurines */
	#define TV_STATUE        9      /* Statue, what a silly object... */
	#define TV_CORPSE       10      /* Corpses and Skeletons, specific */
	#define TV_CAPTURE      11      /* Monster ball */
	#define TV_NO_AMMO      15      /* Ammo for crimson */
	#define TV_SHOT         16      /* Ammo for slings */
	#define TV_ARROW        17      /* Ammo for bows */
	#define TV_BOLT         18      /* Ammo for x-bows */
	#define TV_BOW          19      /* Slings/Bows/Xbows */
	#define TV_DIGGING      20      /* Shovels/Picks */
	#define TV_HAFTED       21      /* Priest Weapons */
	#define TV_POLEARM      22      /* Axes and Pikes */
	#define TV_SWORD        23      /* Edged Weapons */
	#define TV_BOOTS        30      /* Boots */
	#define TV_GLOVES       31      /* Gloves */
	#define TV_HELM         32      /* Helms */
	#define TV_CROWN        33      /* Crowns */
	#define TV_SHIELD       34      /* Shields */
	#define TV_CLOAK        35      /* Cloaks */
	#define TV_SOFT_ARMOR   36      /* Soft Armor */
	#define TV_HARD_ARMOR   37      /* Hard Armor */
	#define TV_DRAG_ARMOR   38      /* Dragon Scale Mail */
	#define TV_LITE         39      /* Lites (including Specials) */
	#define TV_AMULET       40      /* Amulets (including Specials) */
	#define TV_RING         45      /* Rings (including Specials) */
	#define TV_CARD         50
	#define TV_STAFF        55
	#define TV_WAND         65
	#define TV_ROD          66
	#define TV_PARCHMENT    69
	#define TV_SCROLL       70
	#define TV_POTION       75
	#define TV_FLASK        77
	#define TV_FOOD         80
	#define TV_LIFE_BOOK    90
	#define TV_SORCERY_BOOK 91
	#define TV_NATURE_BOOK  92
	#define TV_CHAOS_BOOK   93
	#define TV_DEATH_BOOK   94
	#define TV_TRUMP_BOOK   95
	#define TV_ARCANE_BOOK  96
	#define TV_CRAFT_BOOK 97
	#define TV_DAEMON_BOOK  98
	#define TV_CRUSADE_BOOK 99
	#define TV_MUSIC_BOOK   105
	#define TV_HISSATSU_BOOK 106
	#define TV_HEX_BOOK     107
	#define TV_GOLD         127     /* Gold can only be picked up by players */

	#define TV_EQUIP_BEGIN    TV_SHOT
	#define TV_EQUIP_END      TV_CARD
	#define TV_MISSILE_BEGIN  TV_SHOT
	#define TV_MISSILE_END    TV_BOLT
	#define TV_WEARABLE_BEGIN TV_BOW
	#define TV_WEARABLE_END   TV_CARD
	#define TV_WEAPON_BEGIN   TV_BOW
	#define TV_WEAPON_END     TV_SWORD
	#define TV_ARMOR_BEGIN    TV_BOOTS
	#define TV_ARMOR_END      TV_DRAG_ARMOR
	OBJECT_TYPE_VALUE tval;			/* Item type (from kind) */


	OBJECT_SUBTYPE_VALUE sval;			/* Item sub-type (from kind) */

	PARAMETER_VALUE pval;			/* Item extra-parameter */

	DISCOUNT_RATE discount;		/* Discount (if any) */

	ITEM_NUMBER number;	/* Number of items */

	WEIGHT weight;		/* Item weight */

	ARTIFACT_IDX name1;		/* Artifact type, if any */
	EGO_IDX name2;			/* Ego-Item type, if any */


	/*
	 * 変愚ver1.5.0以前に使われていたアイテムの追加特性フラグ / Hack -- special "xtra" object powers
	 */
	#define EGO_XTRA_SUSTAIN        1 /*!< 旧版アイテムフラグ(非推奨): 追加維持能力 / Sustain one stat */
	#define EGO_XTRA_POWER          2 /*!< 旧版アイテムフラグ(非推奨): 追加上級耐性 / High resist */
	#define EGO_XTRA_ABILITY        3 /*!< 旧版アイテムフラグ(非推奨): 追加能力 / Special ability */
	XTRA8 xtra1;			/* Extra info type (now unused) */

	XTRA8 xtra2;			/* Extra info activation index */
	XTRA8 xtra3;			/* Extra info for weaponsmith */
	XTRA16 xtra4;			/*!< 光源の残り寿命、あるいは捕らえたモンスターの現HP / Extra info fuel or captured monster's current HP */
	XTRA16 xtra5;			/*!< 捕らえたモンスターの最大HP / Extra info captured monster's max HP */

	HIT_PROB to_h;			/* Plusses to hit */
	HIT_POINT to_d;			/* Plusses to damage */
	ARMOUR_CLASS to_a;			/* Plusses to AC */

	ARMOUR_CLASS ac;			/* Normal AC */

	DICE_NUMBER dd;
	DICE_SID ds;		/* Damage dice/sides */

	TIME_EFFECT timeout;	/* Timeout Counter */

	/*
	 * Special Object Flags
	 */
	#define IDENT_SENSE     0x01    /* Item has been "sensed" */
	#define IDENT_FIXED     0x02    /* Item has been "haggled" */
	#define IDENT_EMPTY     0x04    /* Item charges are known */
	#define IDENT_KNOWN     0x08    /* Item abilities are known */
	#define IDENT_STORE     0x10    /* Item is storebought !!!! */
	#define IDENT_MENTAL    0x20    /* Item information is known */
	#if 0
	#define IDENT_CURSED    0x40    /* Item is temporarily cursed */
	#endif
	#define IDENT_BROKEN    0x80    /* Item is permanently worthless */
	byte ident;			/* Special flags  */

	/*
	 * How object is marked (flags in object_type.mark)
	 * OM_FOUND --- original boolean flag
	 * OM_NOMSG --- temporary flag to suppress messages which were
	 *              already printed in autopick_pickup_items().
	 */
	#define OM_FOUND        0x01    /*!< アイテムを一度でも視界に収めたことがあるか */
	#define OM_NOMSG        0x02    /* temporary flag to suppress messages */
	#define OM_NO_QUERY     0x04    /* Query for auto-pick was already answered as 'No' */
	#define OM_AUTODESTROY  0x08    /* Destroy later to avoid illegal inventry shift */
	#define OM_TOUCHED      0x10    /* Object was touched by player */
	byte marked;		/* Object is marked */

	u16b inscription;	/* Inscription index */
	u16b art_name;      /* Artifact name (random artifacts) */

	byte feeling;          /* Game generated inscription number (eg, pseudo-id) */

	BIT_FLAGS art_flags[TR_FLAG_SIZE];        /* Extra Flags for ego and artifacts */
	BIT_FLAGS curse_flags;        /* Flags for curse */

	OBJECT_IDX next_o_idx;	/* Next object in stack (if any) */
	MONSTER_IDX held_m_idx;	/* Monster holding us (if any) */

	ARTIFACT_BIAS_IDX artifact_bias; /*!< ランダムアーティファクト生成時のバイアスID */
};

extern bool(*item_tester_hook)(object_type *o_ptr);
extern OBJECT_TYPE_VALUE item_tester_tval;
extern bool(*get_obj_num_hook)(KIND_OBJECT_IDX k_idx);

/* object1.c */
extern ITEM_NUMBER scan_floor(OBJECT_IDX *items, POSITION y, POSITION x, BIT_FLAGS mode);
extern COMMAND_CODE show_floor(int target_item, POSITION y, POSITION x, TERM_LEN *min_width);
extern void reset_visuals(void);
extern void object_flags(object_type *o_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE]);
extern void object_flags_known(object_type *o_ptr, BIT_FLAGS flgs[TR_FLAG_SIZE]);
extern concptr item_activation(object_type *o_ptr);
extern bool screen_object(object_type *o_ptr, BIT_FLAGS mode);
extern char index_to_label(int i);
extern INVENTORY_IDX label_to_inven(int c);
extern INVENTORY_IDX label_to_equip(int c);
extern s16b wield_slot(object_type *o_ptr);
extern concptr mention_use(int i);
extern concptr describe_use(int i);
extern bool check_book_realm(const OBJECT_TYPE_VALUE book_tval, const OBJECT_SUBTYPE_VALUE book_sval);
extern bool item_tester_okay(object_type *o_ptr);
extern void display_inven(void);
extern void display_equip(void);
extern COMMAND_CODE show_inven(int target_item, BIT_FLAGS mode);
extern COMMAND_CODE show_equip(int target_item, BIT_FLAGS mode);
extern void toggle_inven_equip(void);

/*
 * get_item()関数でアイテムの選択を行うフラグ / Bit flags for the "get_item" function
 */
#define USE_EQUIP 0x01 /*!< アイテム表示/選択範囲: 装備品からの選択を許可する / Allow equip items */
#define USE_INVEN 0x02 /*!< アイテム表示/選択範囲: 所持品からの選択を許可する /  Allow inven items */
#define USE_FLOOR 0x04 /*!< アイテム表示/選択範囲: 床下のアイテムからの選択を許可する /  Allow floor items */
#define USE_FORCE 0x08 /*!< 特殊: wキーで錬気術への切り替えを許可する */
#define IGNORE_BOTHHAND_SLOT 0x10 /*!< アイテム表示/選択範囲: 両手持ちスロットを選択に含めない */
#define USE_FULL  0x20 /*!< アイテム表示/選択範囲: 空欄まですべて表示する*/
extern bool can_get_item(void);
extern bool get_item(OBJECT_IDX *cp, concptr pmt, concptr str, BIT_FLAGS mode);
extern object_type *choose_object(OBJECT_IDX *idx, concptr q, concptr s, BIT_FLAGS option);
PERCENTAGE breakage_chance(object_type *o_ptr, SPELL_IDX snipe_type);

extern int bow_tval_ammo(object_type *o_ptr);

/* object2.c */
extern void excise_object_idx(OBJECT_IDX o_idx);
extern void delete_object_idx(OBJECT_IDX o_idx);
extern void delete_object(POSITION y, POSITION x);
extern void compact_objects(int size);
extern void wipe_o_list(void);
extern OBJECT_IDX o_pop(void);
extern OBJECT_IDX get_obj_num(DEPTH level, BIT_FLAGS mode);
extern void object_known(object_type *o_ptr);
extern void object_aware(object_type *o_ptr);
extern void object_tried(object_type *o_ptr);

/*
 * アイテムの簡易鑑定定義 / Game generated inscription indices. These are stored in the object,
 * and are used to index the string array from tables.c.
 */
#define FEEL_NONE              0 /*!< 簡易鑑定: 未鑑定 */
#define FEEL_BROKEN            1 /*!< 簡易鑑定: 壊れている */
#define FEEL_TERRIBLE          2 /*!< 簡易鑑定: 恐ろしい */
#define FEEL_WORTHLESS         3 /*!< 簡易鑑定: 無価値 */
#define FEEL_CURSED            4 /*!< 簡易鑑定: 呪われている */
#define FEEL_UNCURSED          5 /*!< 簡易鑑定: 呪われていない */
#define FEEL_AVERAGE           6 /*!< 簡易鑑定: 並 */
#define FEEL_GOOD              7 /*!< 簡易鑑定: 上質 */
#define FEEL_EXCELLENT         8 /*!< 簡易鑑定: 高級 */
#define FEEL_SPECIAL           9 /*!< 簡易鑑定: 特別 */
#define FEEL_MAX               9 /*!< 簡易鑑定の種別数 */
extern byte value_check_aux1(object_type *o_ptr);
extern byte value_check_aux2(object_type *o_ptr);

extern PRICE object_value(object_type *o_ptr);
extern PRICE object_value_real(object_type *o_ptr);
extern void distribute_charges(object_type *o_ptr, object_type *q_ptr, int amt);
extern void reduce_charges(object_type *o_ptr, int amt);
extern int object_similar_part(object_type *o_ptr, object_type *j_ptr);
extern bool object_similar(object_type *o_ptr, object_type *j_ptr);
extern void object_absorb(object_type *o_ptr, object_type *j_ptr);
extern IDX lookup_kind(OBJECT_TYPE_VALUE tval, OBJECT_SUBTYPE_VALUE sval);
extern void object_wipe(object_type *o_ptr);
extern void object_prep(object_type *o_ptr, KIND_OBJECT_IDX k_idx);
extern void object_copy(object_type *o_ptr, object_type *j_ptr);

/*
 * Bit flags for apply_magic() (etc)
 */
#define AM_NO_FIXED_ART 0x00000001 /*!< Don't allow roll for fixed artifacts */
#define AM_GOOD         0x00000002 /*!< Generate good items */
#define AM_GREAT        0x00000004 /*!< Generate great items */
#define AM_SPECIAL      0x00000008 /*!< Generate artifacts (for debug mode only) */
#define AM_CURSED       0x00000010 /*!< Generate cursed/worthless items */
#define AM_FORBID_CHEST 0x00000020 /*!< 箱からさらに箱が出現することを抑止する */
extern void apply_magic(object_type *o_ptr, DEPTH lev, BIT_FLAGS mode);

extern OBJECT_SUBTYPE_VALUE coin_type;

extern bool make_object(object_type *j_ptr, BIT_FLAGS mode);
extern void place_object(POSITION y, POSITION x, BIT_FLAGS mode);
extern bool make_gold(object_type *j_ptr);
extern void place_gold(POSITION y, POSITION x);
extern OBJECT_IDX drop_near(object_type *o_ptr, PERCENTAGE chance, POSITION y, POSITION x);
extern void inven_item_charges(INVENTORY_IDX item);
extern void inven_item_describe(INVENTORY_IDX item);
extern void inven_item_increase(INVENTORY_IDX item, ITEM_NUMBER num);
extern void inven_item_optimize(INVENTORY_IDX item);
extern void floor_item_charges(INVENTORY_IDX item);
extern void floor_item_describe(INVENTORY_IDX item);
extern void floor_item_increase(INVENTORY_IDX item, ITEM_NUMBER num);
extern void floor_item_optimize(INVENTORY_IDX item);
extern bool inven_carry_okay(object_type *o_ptr);
extern bool object_sort_comp(object_type *o_ptr, s32b o_value, object_type *j_ptr);
extern s16b inven_carry(object_type *o_ptr);
extern INVENTORY_IDX inven_takeoff(INVENTORY_IDX item, ITEM_NUMBER amt);
extern void inven_drop(INVENTORY_IDX item, ITEM_NUMBER amt);
extern void combine_pack(void);
extern void reorder_pack(void);
extern void display_koff(KIND_OBJECT_IDX k_idx);
extern void torch_flags(object_type *o_ptr, BIT_FLAGS *flgs);
extern void torch_dice(object_type *o_ptr, DICE_NUMBER *dd, DICE_SID *ds);
extern void torch_lost_fuel(object_type *o_ptr);
extern concptr essence_name[];

extern s32b flag_cost(object_type *o_ptr, int plusses);

extern bool get_item_floor(COMMAND_CODE *cp, concptr pmt, concptr str, BIT_FLAGS mode);
extern void py_pickup_floor(bool pickup);

/*
 * Return the "attr" for a given item.
 * Use "flavor" if available.
 * Default to user definitions.
 */
#define object_attr(T) \
	((k_info[(T)->k_idx].flavor) ? \
	 (k_info[k_info[(T)->k_idx].flavor].x_attr) : \
	 ((!(T)->k_idx || ((T)->tval != TV_CORPSE) || ((T)->sval != SV_CORPSE) || \
	   (k_info[(T)->k_idx].x_attr != TERM_DARK)) ? \
	  (k_info[(T)->k_idx].x_attr) : (r_info[(T)->pval].x_attr)))

