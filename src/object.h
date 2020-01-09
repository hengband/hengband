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

#define OBJ_GOLD_LIST   480     /* First "gold" entry */
#define MAX_GOLD        18      /* Number of "gold" entries */

/* Lite flag macro */
#define HAVE_LITE_FLAG(ARRAY) \
	(have_flag(ARRAY, TR_LITE_1) || have_flag(ARRAY, TR_LITE_2) || have_flag(ARRAY, TR_LITE_3))

#define HAVE_DARK_FLAG(ARRAY) \
	(have_flag(ARRAY, TR_LITE_M1) || have_flag(ARRAY, TR_LITE_M2) || have_flag(ARRAY, TR_LITE_M3))

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
	XTRA8 xtra3;			/*!< 複数の使用用途 捕らえたモンスターの速度，付加した特殊なエッセンスID / Extra info for weaponsmith */
	XTRA16 xtra4;			/*!< 複数の使用用途 光源の残り寿命、あるいは捕らえたモンスターの現HP / Extra info fuel or captured monster's current HP */
	XTRA16 xtra5;			/*!< 複数の使用用途 捕らえたモンスターの最大HP / Extra info captured monster's max HP */

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

	#define TRC_CURSED              0x00000001L
	#define TRC_HEAVY_CURSE         0x00000002L
	#define TRC_PERMA_CURSE         0x00000004L
	#define TRC_XXX1                0x00000008L
	#define TRC_TY_CURSE            0x00000010L
	#define TRC_AGGRAVATE           0x00000020L
	#define TRC_DRAIN_EXP           0x00000040L
	#define TRC_SLOW_REGEN          0x00000080L
	#define TRC_ADD_L_CURSE         0x00000100L
	#define TRC_ADD_H_CURSE         0x00000200L
	#define TRC_CALL_ANIMAL         0x00000400L
	#define TRC_CALL_DEMON          0x00000800L
	#define TRC_CALL_DRAGON         0x00001000L
	#define TRC_COWARDICE           0x00002000L
	#define TRC_TELEPORT            0x00004000L
	#define TRC_LOW_MELEE           0x00008000L
	#define TRC_LOW_AC              0x00010000L
	#define TRC_LOW_MAGIC           0x00020000L
	#define TRC_FAST_DIGEST         0x00040000L
	#define TRC_DRAIN_HP            0x00080000L
	#define TRC_DRAIN_MANA          0x00100000L
	#define TRC_CALL_UNDEAD         0x00200000L
	#define MAX_CURSE 18
	#define TRC_TELEPORT_SELF       0x00000001L
	#define TRC_CHAINSWORD          0x00000002L
	#define TRC_SPECIAL_MASK \
		(TRC_TY_CURSE | TRC_AGGRAVATE)
	#define TRC_HEAVY_MASK   \
		(TRC_TY_CURSE | TRC_AGGRAVATE | TRC_DRAIN_EXP | TRC_ADD_H_CURSE | \
		 TRC_CALL_DEMON | TRC_CALL_DRAGON | TRC_CALL_UNDEAD | TRC_TELEPORT)
	#define TRC_P_FLAG_MASK  \
		(TRC_TELEPORT_SELF | TRC_CHAINSWORD | \
		 TRC_TY_CURSE | TRC_DRAIN_EXP | TRC_ADD_L_CURSE | TRC_ADD_H_CURSE | \
		 TRC_CALL_ANIMAL | TRC_CALL_DEMON | TRC_CALL_DRAGON | TRC_COWARDICE | \
		 TRC_TELEPORT | TRC_DRAIN_HP | TRC_DRAIN_MANA | TRC_CALL_UNDEAD)
	BIT_FLAGS curse_flags;        /* Flags for curse */

	OBJECT_IDX next_o_idx;	/* Next object in stack (if any) */
	MONSTER_IDX held_m_idx;	/*!< アイテムを所持しているモンスターID (いないなら 0) / Monster holding us (if any) */

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

#define SCROBJ_FAKE_OBJECT  0x00000001
#define SCROBJ_FORCE_DETAIL 0x00000002
extern bool screen_object(object_type *o_ptr, BIT_FLAGS mode);

extern char index_to_label(int i);
extern s16b wield_slot(player_type *owner_ptr, object_type *o_ptr);

extern bool check_book_realm(const OBJECT_TYPE_VALUE book_tval, const OBJECT_SUBTYPE_VALUE book_sval);

/*
 * get_item()関数でアイテムの選択を行うフラグ / Bit flags for the "get_item" function
 */
#define USE_EQUIP 0x01 /*!< アイテム表示/選択範囲: 装備品からの選択を許可する / Allow equip items */
#define USE_INVEN 0x02 /*!< アイテム表示/選択範囲: 所持品からの選択を許可する /  Allow inven items */
#define USE_FLOOR 0x04 /*!< アイテム表示/選択範囲: 床下のアイテムからの選択を許可する /  Allow floor items */
#define USE_FORCE 0x08 /*!< 特殊: wキーで錬気術への切り替えを許可する */
#define IGNORE_BOTHHAND_SLOT 0x10 /*!< アイテム表示/選択範囲: 両手持ちスロットを選択に含めない */
#define USE_FULL  0x20 /*!< アイテム表示/選択範囲: 空欄まですべて表示する*/
extern bool can_get_item(OBJECT_TYPE_VALUE tval);
extern bool get_item(player_type *owner_ptr, OBJECT_IDX *cp, concptr pmt, concptr str, BIT_FLAGS mode, OBJECT_TYPE_VALUE tval);

#define REF_ITEM(P_PTR, FLOOR_PTR, ID) ((ID >= 0 ? &(P_PTR)->inventory_list[ID] : &(FLOOR_PTR)->o_list[0 - item]))

extern int bow_tval_ammo(object_type *o_ptr);

/*!
* todo ここに置くとコンパイルは通る (このファイルの冒頭やobject2.cでincludeするとコンパイルエラー)、しかし圧倒的にダメなので要調整
*/
#include "floor.h"

/* object2.c */
extern void excise_object_idx(floor_type *floor_ptr, OBJECT_IDX o_idx);
extern void delete_object_idx(floor_type *floor_ptr, OBJECT_IDX o_idx);
extern void delete_object(floor_type *floor_ptr, POSITION y, POSITION x);

extern OBJECT_IDX o_pop(floor_type *floor_ptr);
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
extern bool make_gold(object_type *j_ptr);
extern OBJECT_IDX drop_near(player_type *owner_type, object_type *o_ptr, PERCENTAGE chance, POSITION y, POSITION x);
extern void vary_item(INVENTORY_IDX item, ITEM_NUMBER num);
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
extern s16b inven_carry(player_type *owner_ptr, object_type *o_ptr);
extern INVENTORY_IDX inven_takeoff(INVENTORY_IDX item, ITEM_NUMBER amt);
extern void drop_from_inventory(player_type *owner_type, INVENTORY_IDX item, ITEM_NUMBER amt);
extern void combine_pack(player_type *owner_ptr);
extern void reorder_pack(void);
extern void display_koff(KIND_OBJECT_IDX k_idx);
extern void torch_flags(object_type *o_ptr, BIT_FLAGS *flgs);
extern void torch_dice(object_type *o_ptr, DICE_NUMBER *dd, DICE_SID *ds);
extern void torch_lost_fuel(object_type *o_ptr);
extern concptr essence_name[];

extern s32b flag_cost(object_type *o_ptr, int plusses);

extern bool get_item_floor(player_type *creature_ptr, COMMAND_CODE *cp, concptr pmt, concptr str, BIT_FLAGS mode, OBJECT_TYPE_VALUE tval);
extern void py_pickup_floor(player_type *creature_ptr, bool pickup);
extern void prepare_label_string(char *label, BIT_FLAGS mode, OBJECT_TYPE_VALUE tval);

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


 /*** Object "sval" codes ***/

 /* Any subvalue */
#define SV_ANY 					255

/* The "sval" codes for TV_FIGURINE */
#define SV_FIGURINE_NORMAL		0

#define SV_CAPTURE_NONE		0

/* The "sval" codes for TV_STATUE */
#define SV_WOODEN_STATUE		0
#define SV_CLAY_STATUE			1
#define SV_STONE_STATUE			2
#define SV_IRON_STATUE			3
#define SV_COPPER_STATUE		4
#define SV_SILVER_STATUE		5
#define SV_GOLDEN_STATUE		6
#define SV_IVORY_STATUE			7
#define SV_MITHRIL_STATUE		8
#define SV_ORNATE_STATUE		9
#define SV_PHOTO			50

/* The "sval" codes for TV_CORPSE */
#define SV_SKELETON 			0
#define SV_CORPSE			1

/* The "sval" codes for TV_SHOT/TV_ARROW/TV_BOLT */
#define SV_AMMO_LIGHT                    0	/* pebbles */
#define SV_AMMO_NORMAL                   1	/* shots, arrows, bolts */
#define SV_AMMO_HEAVY                    2	/* seeker arrows and bolts, mithril shots */

/* The "sval" codes for TV_BOW (note information in "sval") */
#define SV_SLING                         2	/* (x2) */
#define SV_SHORT_BOW                    12	/* (x2) */
#define SV_LONG_BOW                     13	/* (x3) */
#define SV_LIGHT_XBOW                   23	/* (x3) */
#define SV_HEAVY_XBOW                   24	/* (x4) */
#define SV_CRIMSON                      50	/* (x0) */
#define SV_HARP                         51	/* (x0) */
#define SV_NAMAKE_BOW                   63	/* (x3) */

/* The "sval" codes for TV_DIGGING */
#define SV_SHOVEL                        1
#define SV_GNOMISH_SHOVEL                2
#define SV_DWARVEN_SHOVEL                3
#define SV_PICK                          4
#define SV_ORCISH_PICK                   5
#define SV_DWARVEN_PICK                  6
#define SV_MATTOCK                       7

/* The "sval" values for TV_HAFTED */
#define SV_CLUB                          1	/* 1d4  */
#define SV_WHIP                          2	/* 1d6  */
#define SV_QUARTERSTAFF                  3	/* 1d9  */
#define SV_NUNCHAKU                      4	/* 2d3  */
#define SV_MACE                          5	/* 2d4  */
#define SV_BALL_AND_CHAIN                6	/* 2d4  */
#define SV_JO_STAFF                      7	/* 1d7  */
#define SV_WAR_HAMMER                    8	/* 3d3  */
#define SV_THREE_PIECE_ROD              11	/* 3d3  */
#define SV_MORNING_STAR                 12	/* 2d6  */
#define SV_FLAIL                        13	/* 2d6  */
#define SV_BO_STAFF                     14	/* 1d11 */
#define SV_LEAD_FILLED_MACE             15	/* 3d4  */
#define SV_TETSUBO                      16	/* 2d7  */
#define SV_TWO_HANDED_FLAIL             18	/* 3d6  */
#define SV_GREAT_HAMMER                 19	/* 4d6  */
#define SV_MACE_OF_DISRUPTION           20	/* 5d8  */
#define SV_WIZSTAFF                     21	/* 1d2  */
#define SV_GROND                        50	/* 3d9  */
#define SV_NAMAKE_HAMMER                63	/* 1d77 */

/* The "sval" values for TV_POLEARM */
#define SV_HATCHET                       1	/* 1d5 */
#define SV_SPEAR                         2	/* 1d6 */
#define SV_SICKLE                        3	/* 2d3 */
#define SV_AWL_PIKE                      4	/* 1d8 */
#define SV_TRIDENT                       5	/* 1d9 */
#define SV_FAUCHARD                      6  /* 1d10 */
#define SV_BROAD_SPEAR                   7	/* 1d9 */
#define SV_PIKE                          8	/* 2d5 */
#define SV_NAGINATA                      9  /* 2d6 */
#define SV_BEAKED_AXE                   10	/* 2d6 */
#define SV_BROAD_AXE                    11	/* 2d6 */
#define SV_LUCERNE_HAMMER               12	/* 2d5  */
#define SV_GLAIVE                       13	/* 2d6 */
#define SV_LAJATANG                     14	/* 2d7 */
#define SV_HALBERD                      15	/* 3d4 */
#define SV_GUISARME                     16  /* 2d5 */
#define SV_SCYTHE                       17	/* 5d3 */
#define SV_LANCE                        20	/* 2d8 */
#define SV_BATTLE_AXE                   22	/* 2d8 */
#define SV_GREAT_AXE                    25	/* 4d4 */
#define SV_TRIFURCATE_SPEAR             26	/* 2d9 */
#define SV_LOCHABER_AXE                 28	/* 3d8 */
#define SV_HEAVY_LANCE                  29  /* 4d8 */
#define SV_SCYTHE_OF_SLICING            30	/* 8d4 */
#define SV_TSURIZAO                     40	/* 1d1 */
#define SV_DEATH_SCYTHE                 50	/* 10d10 */

/* The "sval" codes for TV_SWORD */
#define SV_BROKEN_DAGGER                 1  /* 1d1 */
#define SV_BROKEN_SWORD                  2  /* 1d2 */
#define SV_DAGGER                        4  /* 1d4 */
#define SV_MAIN_GAUCHE                   5  /* 1d5 */
#define SV_TANTO                         6  /* 1d5 */
#define SV_RAPIER                        7  /* 1d6 */
#define SV_SMALL_SWORD                   8  /* 1d6 */
#define SV_BASILLARD                     9  /* 1d8 */
#define SV_SHORT_SWORD                  10  /* 1d7 */
#define SV_SABRE                        11  /* 1d7 */
#define SV_CUTLASS                      12  /* 1d7 */
#define SV_WAKIZASHI                    13  /* 2d4 */
#define SV_KHOPESH                      14  /* 2d4 */
#define SV_TULWAR                       15  /* 2d4 */
#define SV_BROAD_SWORD                  16  /* 2d5 */
#define SV_LONG_SWORD                   17  /* 2d5 */
#define SV_SCIMITAR                     18  /* 2d5 */
#define SV_NINJATO                      19  /* 1d9 */
#define SV_KATANA                       20  /* 3d4 */
#define SV_BASTARD_SWORD                21  /* 3d4 */
#define SV_GREAT_SCIMITAR               22  /* 4d5 */
#define SV_CLAYMORE                     23  /* 2d8 */
#define SV_ESPADON                      24  /* 2d9 */
#define SV_TWO_HANDED_SWORD             25  /* 3d6 */
#define SV_FLAMBERGE                    26  /* 3d7 */
#define SV_NO_DACHI                     27  /* 5d4 */
#define SV_EXECUTIONERS_SWORD           28  /* 4d5 */
#define SV_ZWEIHANDER                   29  /* 4d6 */
#define SV_BLADE_OF_CHAOS               30  /* 6d5 */
#define SV_DIAMOND_EDGE                 31  /* 7d5 */
#define SV_DOKUBARI                     32  /* 1d1 */
#define SV_HAYABUSA                     33  /* 1d6 */

/* The "sval" codes for TV_SHIELD */
#define SV_SMALL_LEATHER_SHIELD          2
#define SV_SMALL_METAL_SHIELD            3
#define SV_LARGE_LEATHER_SHIELD          4
#define SV_LARGE_METAL_SHIELD            5
#define SV_DRAGON_SHIELD                 6
#define SV_KNIGHT_SHIELD                 7
#define SV_MIRROR_SHIELD                10
#define SV_YATA_MIRROR                  50

/* The "sval" codes for TV_HELM */
#define SV_HARD_LEATHER_CAP              2
#define SV_METAL_CAP                     3
#define SV_JINGASA                       4  /* 4 */
#define SV_IRON_HELM                     5
#define SV_STEEL_HELM                    6
#define SV_DRAGON_HELM                   7
#define SV_KABUTO                        8  /* 7 */

/* The "sval" codes for TV_CROWN */
#define SV_IRON_CROWN                   10
#define SV_GOLDEN_CROWN                 11
#define SV_JEWELED_CROWN                12
#define SV_CHAOS                        50

/* The "sval" codes for TV_BOOTS */
#define SV_PAIR_OF_SOFT_LEATHER_BOOTS    2
#define SV_PAIR_OF_HARD_LEATHER_BOOTS    3
#define SV_PAIR_OF_DRAGON_GREAVE         4
#define SV_PAIR_OF_METAL_SHOD_BOOTS      6

/* The "sval" codes for TV_CLOAK */
#define SV_CLOAK                         1
#define SV_ELVEN_CLOAK                   2
#define SV_FUR_CLOAK                     3
#define SV_ETHEREAL_CLOAK                5
#define SV_SHADOW_CLOAK                  6

/* The "sval" codes for TV_GLOVES */
#define SV_SET_OF_LEATHER_GLOVES         1
#define SV_SET_OF_GAUNTLETS              2
#define SV_SET_OF_DRAGON_GLOVES          3
#define SV_SET_OF_CESTI                  5

/* The "sval" codes for TV_SOFT_ARMOR */
#define SV_T_SHIRT                       0
#define SV_FILTHY_RAG                    1
#define SV_ROBE                          2
#define SV_PAPER_ARMOR                   3  /* 4 */
#define SV_SOFT_LEATHER_ARMOR            4
#define SV_SOFT_STUDDED_LEATHER          5
#define SV_HARD_LEATHER_ARMOR            6
#define SV_HARD_STUDDED_LEATHER          7
#define SV_RHINO_HIDE_ARMOR              8
#define SV_CORD_ARMOR                    9  /*  6 */
#define SV_PADDED_ARMOR                 10  /*  4 */
#define SV_LEATHER_SCALE_MAIL           11
#define SV_LEATHER_JACK                 12
#define SV_KUROSHOUZOKU                 13
#define SV_STONE_AND_HIDE_ARMOR         15  /* 15 */
#define SV_ABUNAI_MIZUGI                50
#define SV_YOIYAMI_ROBE                 60
#define SV_NAMAKE_ARMOR                 63

/* The "sval" codes for TV_HARD_ARMOR */
#define SV_RUSTY_CHAIN_MAIL              1  /* 14- */
#define SV_RING_MAIL                     2  /* 12  */
#define SV_METAL_SCALE_MAIL              3  /* 13  */
#define SV_CHAIN_MAIL                    4  /* 14  */
#define SV_DOUBLE_RING_MAIL              5  /* 15  */
#define SV_AUGMENTED_CHAIN_MAIL          6  /* 16  */
#define SV_DOUBLE_CHAIN_MAIL             7  /* 16  */
#define SV_BAR_CHAIN_MAIL                8  /* 18  */
#define SV_METAL_BRIGANDINE_ARMOUR       9  /* 19  */
#define SV_SPLINT_MAIL                  10  /* 19  */
#define SV_DO_MARU                      11  /* 20  */
#define SV_PARTIAL_PLATE_ARMOUR         12  /* 22  */
#define SV_METAL_LAMELLAR_ARMOUR        13  /* 23  */
#define SV_HARAMAKIDO                   14  /* 17  */
#define SV_FULL_PLATE_ARMOUR            15  /* 25  */
#define SV_O_YOROI                      16  /* 24  */
#define SV_RIBBED_PLATE_ARMOUR          18  /* 28  */
#define SV_MITHRIL_CHAIN_MAIL           20  /* 28+ */
#define SV_MITHRIL_PLATE_MAIL           25  /* 35+ */
#define SV_ADAMANTITE_PLATE_MAIL        30  /* 40+ */

/* The "sval" codes for TV_DRAG_ARMOR */
#define SV_DRAGON_BLACK                  1
#define SV_DRAGON_BLUE                   2
#define SV_DRAGON_WHITE                  3
#define SV_DRAGON_RED                    4
#define SV_DRAGON_GREEN                  5
#define SV_DRAGON_MULTIHUED              6
#define SV_DRAGON_SHINING               10
#define SV_DRAGON_LAW                   12
#define SV_DRAGON_BRONZE                14
#define SV_DRAGON_GOLD                  16
#define SV_DRAGON_CHAOS                 18
#define SV_DRAGON_BALANCE               20
#define SV_DRAGON_POWER                 30

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

/* The "sval" codes for TV_FLASK */
#define SV_FLASK_OIL                   0

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


#define SV_ROD_MIN_DIRECTION    12 /*!< この値以降の小項目IDを持ったロッドは使用時にターゲットを要求する / Special "sval" limit -- first "aimed" rod */

#define SV_CHEST_MIN_LARGE      4  /*!< この値以降の小項目IDを持った箱は大型の箱としてドロップ数を増やす / Special "sval" limit -- first "large" chest */
#define SV_CHEST_KANDUME        50 /*!< 箱アイテムの小項目ID: おもちゃのカンヅメ */

/*
 * Special "sval" limit -- first "good" magic/prayer book
 */
#define SV_BOOK_MIN_GOOD    2

