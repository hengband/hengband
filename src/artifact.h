#pragma once
#include "cmd-activate.h"

typedef struct artifact_type artifact_type;

/*!
 * @struct artifact_type
 * @brief 固定アーティファクト情報の構造体 / Artifact structure.
 * @details
 * @note
 * the save-file only writes "cur_num" to the savefile.
 * "max_num" is always "1" (if that artifact "exists")
 */
struct artifact_type
{
	STR_OFFSET name;			/*!< アーティファクト名(headerオフセット参照) / Name (offset) */
	STR_OFFSET text;			/*!< アーティファクト解説(headerオフセット参照) / Text (offset) */

	OBJECT_TYPE_VALUE tval;		/*!< ベースアイテム大項目ID / Artifact type */
	OBJECT_SUBTYPE_VALUE sval;	/*!< ベースアイテム小項目ID / Artifact sub type */

	PARAMETER_VALUE pval;	/*!< pval修正値 / Artifact extra info */

	HIT_PROB to_h;			/*!< 命中ボーナス値 /  Bonus to hit */
	HIT_POINT to_d;		/*!< ダメージボーナス値 / Bonus to damage */
	ARMOUR_CLASS to_a;			/*!< ACボーナス値 / Bonus to armor */

	ARMOUR_CLASS ac;			/*!< 上書きベースAC値 / Base armor */

	DICE_NUMBER dd;
	DICE_SID ds;	/*!< ダイス値 / Damage when hits */

	WEIGHT weight;		/*!< 重量 / Weight */

	PRICE cost;			/*!< 基本価格 / Artifact "cost" */

	BIT_FLAGS flags[TR_FLAG_SIZE];       /*! アイテムフラグ / Artifact Flags */

	#define TRG_INSTA_ART           0x00000001L     /* Item must be an artifact */
	#define TRG_QUESTITEM           0x00000002L     /* quest level item -KMW- */
	#define TRG_XTRA_POWER          0x00000004L     /* Extra power */
	#define TRG_ONE_SUSTAIN         0x00000008L     /* One sustain */
	#define TRG_XTRA_RES_OR_POWER   0x00000010L     /* Extra resistance or power */
	#define TRG_XTRA_H_RES          0x00000020L     /* Extra high resistance */
	#define TRG_XTRA_E_RES          0x00000040L     /* Extra element resistance */
	#define TRG_XTRA_L_RES          0x00000080L     /* Extra lordly resistance */
	#define TRG_XTRA_D_RES          0x00000100L     /* Extra dragon resistance */
	#define TRG_XTRA_RES            0x00000200L     /* Extra resistance */
	#define TRG_CURSED              0x00000400L     /* Item is Cursed */
	#define TRG_HEAVY_CURSE         0x00000800L     /* Item is Heavily Cursed */
	#define TRG_PERMA_CURSE         0x00001000L     /* Item is Perma Cursed */
	#define TRG_RANDOM_CURSE0       0x00002000L     /* Item is Random Cursed */
	#define TRG_RANDOM_CURSE1       0x00004000L     /* Item is Random Cursed */
	#define TRG_RANDOM_CURSE2       0x00008000L     /* Item is Random Cursed */
	#define TRG_XTRA_DICE           0x00010000L     /* Extra dice */
	#define TRG_POWERFUL            0x00020000L     /* Item has good value even if Cursed */
	BIT_FLAGS gen_flags;		/*! アイテム生成フラグ / flags for generate */

	DEPTH level;		/*! 基本生成階 / Artifact level */
	RARITY rarity;		/*! レアリティ / Artifact rarity */

	byte cur_num;		/*! 現在の生成数 / Number created (0 or 1) */
	byte max_num;		/*! (未使用)最大生成数 / Unused (should be "1") */

	FLOOR_IDX floor_id;      /*! アイテムを落としたフロアのID / Leaved on this location last time */

	byte act_idx;		/*! 発動能力ID / Activative ability index */
};

extern artifact_type *a_info;
extern char *a_name;
extern char *a_text;

extern ARTIFACT_IDX max_a_idx;

/* artifact.c */
extern bool create_artifact(object_type *o_ptr, bool a_scroll);
extern int activation_index(object_type *o_ptr);
extern const activation_type* find_activation_info(object_type *o_ptr);
extern void random_artifact_resistance(object_type * o_ptr, artifact_type *a_ptr);
extern bool create_named_art(ARTIFACT_IDX a_idx, POSITION y, POSITION x);
extern bool make_artifact(object_type *o_ptr);
extern bool make_artifact_special(object_type *o_ptr);

/*** Artifact indexes (see "lib/edit/a_info.txt") ***/

/* Lites */
#define ART_GALADRIEL            1
#define ART_ELENDIL              2
#define ART_JUDGE                3
#define ART_EDISON               7
#define ART_PALANTIR             15
#define ART_STONE_LORE           17
#define ART_FLY_STONE            147
#define ART_ORB_OF_FATE          245 
/* Amulets */
#define ART_CARLAMMAS            4
#define ART_INGWE                5
#define ART_DWARVES              6
#define ART_FARAMIR              18
#define ART_BOROMIR              143
#define ART_MAGATAMA             149
#define ART_INROU                166
#define ART_NIGHT                215
#define ART_SACRED_KNIGHTS       217
#define ART_HELL                 218
#define ART_CHARMED              219
#define ART_GOGO                 220

/* Rings */
#define ART_FRAKIR               8
#define ART_TULKAS               9
#define ART_NARYA               10
#define ART_NENYA               11
#define ART_VILYA               12
#define ART_POWER               13
#define ART_AHO                 14

/* Dragon Scale */
#define ART_RAZORBACK           129
#define ART_BLADETURNER         130
#define ART_SEIRYU              201

/* Hard Armour */
#define ART_SOULKEEPER          19
#define ART_ISILDUR             20
#define ART_ROHIRRIM            21
#define ART_LOHENGRIN           22
#define ART_JULIAN              23
#define ART_ARVEDUI             24
#define ART_CASPANION           25
#define ART_GILES               168
#define ART_MORLOK              203
#define ART_VETERAN             206

/* Soft Armour */
#define ART_SHIVA_JACKET        26
#define ART_HITHLOMIR           27
#define ART_THALKETTOTH         28
#define ART_HIMRING             127
#define ART_ICANUS              131
#define ART_NAMAKE_ARMOR        183
#define ART_GHB                 192
#define ART_DASAI               200
#define ART_KESHO               204
#define ART_MILIM               246

/* Shields */
#define ART_THORIN              30
#define ART_CELEGORM            31
#define ART_ANARION             32
#define ART_GIL_GALAD           138
#define ART_YENDOR              141
#define ART_YATA                151
#define ART_EARENDIL            186
#define ART_PERSEUS             197

/* Helms and Crowns */
#define ART_INDRA               33
#define ART_CHAOS               34
#define ART_BERUTHIEL           35
#define ART_THRANDUIL           36
#define ART_THENGEL             37
#define ART_HAMMERHAND          38
#define ART_DOR                 39
#define ART_HOLHENNETH          40
#define ART_TERROR              41
#define ART_AMBER               42
#define ART_NUMENOR             132
#define ART_STONEMASK           146

/* Cloaks */
#define ART_JACK                43
#define ART_COLLUIN             44
#define ART_HOLCOLLETH          45
#define ART_THINGOL             46
#define ART_THORONGIL           47
#define ART_COLANNON            48
#define ART_LUTHIEN             49
#define ART_TUOR                50
#define ART_MOOK                205
#define ART_HEAVENLY_MAIDEN     233

/* Gloves */
#define ART_CAMBELEG            52
#define ART_CAMMITHRIM          53
#define ART_PAURHACH            54
#define ART_CORWIN              55
#define ART_PAURAEGEN           56
#define ART_PAURNEN             57
#define ART_THANOS              58
#define ART_FINGOLFIN           59
#define ART_PAURNIMMEN          185

/* Boots */
#define ART_FEANOR              60
#define ART_FLORA               61
#define ART_THROR               62
#define ART_SHIVA_BOOTS         63
#define ART_GLASS               165
#define ART_GETA                210

/* Digging */
#define ART_NAIN                211

/* Swords */
#define ART_MAEDHROS            64
#define ART_CAINE               65
#define ART_NARTHANC            66
#define ART_NIMTHANC            67
#define ART_DETHANC             68
#define ART_RILIA               69
#define ART_FIONA               70
#define ART_CALRIS              71
#define ART_GRAYSWANDIR         72
#define ART_GLAMDRING           73
#define ART_NOTHUNG             74
#define ART_ORCRIST             75
#define ART_GURTHANG            76
#define ART_ZARCUTHRA           77
#define ART_TWILIGHT            78
#define ART_GONDRICAM           79
#define ART_CRISDURIAN          80
#define ART_AGLARANG            81
#define ART_RINGIL              82
#define ART_ANDURIL             83
#define ART_WEREWINDLE          84
#define ART_CHAINSWORD          85
#define ART_FORASGIL            86
#define ART_CARETH              87
#define ART_STING               88
#define ART_SOULSWORD           89
#define ART_MERLIN              90
#define ART_DOOMCALLER          91
#define ART_VORPAL_BLADE        92
#define ART_SLAYER              123
#define ART_KUSANAGI            128
#define ART_HURIN               133
#define ART_AZAGHAL             135
#define ART_NOVA                137
#define ART_CHARIOT             140
#define ART_WORPAL_BLADE        142
#define ART_MURAMASA            144
#define ART_ZANTETSU            150
#define ART_SOULCRUSH           154
#define ART_FALIS               155
#define ART_HRUNTING            156
#define ART_ANUBIS              158
#define ART_GURENKI             160
#define ART_TAILBITER           167
#define ART_MUSASI_KATANA       171
#define ART_MUSASI_WAKIZASI     172
#define ART_QUICKTHORN          174
#define ART_TINYTHORN           175
#define ART_EXCALIBUR           176
#define ART_EXCALIPUR           177
#define ART_EXCALIBUR_J         179
#define ART_ARUNRUTH            184
#define ART_HAKABUSA            189
#define ART_STORMBRINGER        190
#define ART_NARSIL              191
#define ART_KANNU               193
#define ART_GRIMTOOTH           196
#define ART_KAMUI               198
#define ART_GOURYU              207
#define ART_EOWYN               216
#define ART_NANACHO             248
#define ART_ROBINTON            251

/* Polearms */
#define ART_THEODEN             93
#define ART_PAIN                94
#define ART_OSONDIR             95
#define ART_TIL                 96
#define ART_RUNESPEAR           97
#define ART_DESTINY             98
#define ART_HAGEN               99
#define ART_EORLINGAS           100
#define ART_DURIN               101
#define ART_EONWE               102
#define ART_BALLI               103
#define ART_LOTHARANG           104
#define ART_DWARVES_AXE         105
#define ART_BARUKKHELED         106
#define ART_WRATH               107
#define ART_ULMO                108
#define ART_AVAVIR              109
#define ART_BENKEI              152
#define ART_TAIKOBO             159
#define ART_TONBO               161
#define ART_GAEBOLG             163
#define ART_ARRYU               164
#define ART_AEGLOS              187
#define ART_BLOOD               199
#define ART_NUMAHOKO            202

/* The sword of the Dawn */
#define ART_DAWN                110

/* Hafted */
#define ART_GROND               111
#define ART_TOTILA              112
#define ART_THUNDERFIST         113
#define ART_BLOODSPIKE          114
#define ART_FIRESTAR            115
#define ART_TARATOL             116
#define ART_AULE                117
#define ART_NAR                 118
#define ART_ERIRIL              119
#define ART_GANDALF             120
#define ART_DEATHWREAKER        121
#define ART_TURMIL              122
#define ART_MJOLLNIR            136
#define ART_WINBLOWS            139
#define ART_XIAOLONG            145
#define ART_NYOIBOU             157
#define ART_JONES               162
#define ART_HYOUSIGI            169
#define ART_MATOI               170
#define ART_IRON_BALL           173
#define ART_SAMSON              178
#define ART_NAMAKE_HAMMER       181
#define ART_BOLISHOI            188
#define ART_SHUTEN_DOJI         194
#define ART_G_HAMMER            195
#define ART_AEGISFANG           208
#define ART_HERMIT              209
#define ART_GOTHMOG             212
#define ART_JIZO                213
#define ART_FUNDIN              214
#define ART_AESCULAPIUS         225

/* Bows */
#define ART_BELTHRONDING        124
#define ART_BARD                125
#define ART_BRAND               126
#define ART_CRIMSON             16
#define ART_BUCKLAND            134
#define ART_YOICHI              148
#define ART_HARAD               180
#define ART_NAMAKE_BOW          182
#define ART_ROBIN_HOOD          221
#define ART_HELLFIRE            222

/* Arrows */
#define ART_BARD_ARROW          153


/* "Biases" for random artifact gen */
#define BIAS_ELEC            1 /*!< ランダムアーティファクトバイアス:電撃 */
#define BIAS_POIS            2 /*!< ランダムアーティファクトバイアス:毒 */
#define BIAS_FIRE            3 /*!< ランダムアーティファクトバイアス:火炎 */
#define BIAS_COLD            4 /*!< ランダムアーティファクトバイアス:冷気 */
#define BIAS_ACID            5 /*!< ランダムアーティファクトバイアス:酸 */
#define BIAS_STR             6 /*!< ランダムアーティファクトバイアス:腕力 */
#define BIAS_INT             7 /*!< ランダムアーティファクトバイアス:知力 */
#define BIAS_WIS             8 /*!< ランダムアーティファクトバイアス:賢さ */
#define BIAS_DEX             9 /*!< ランダムアーティファクトバイアス:器用さ */
#define BIAS_CON            10 /*!< ランダムアーティファクトバイアス:耐久 */
#define BIAS_CHR            11 /*!< ランダムアーティファクトバイアス:魅力 */
#define BIAS_CHAOS          12 /*!< ランダムアーティファクトバイアス:混沌 */
#define BIAS_PRIESTLY       13 /*!< ランダムアーティファクトバイアス:プリースト系 */
#define BIAS_NECROMANTIC    14 /*!< ランダムアーティファクトバイアス:死霊 */
#define BIAS_LAW            15 /*!< ランダムアーティファクトバイアス:法 */
#define BIAS_ROGUE          16 /*!< ランダムアーティファクトバイアス:盗賊系 */
#define BIAS_MAGE           17 /*!< ランダムアーティファクトバイアス:メイジ系 */
#define BIAS_WARRIOR        18 /*!< ランダムアーティファクトバイアス:戦士系 */
#define BIAS_RANGER         19 /*!< ランダムアーティファクトバイアス:レンジャー系 */
#define MAX_BIAS            20 /*!< ランダムアーティファクトバイアス:最大数 */
