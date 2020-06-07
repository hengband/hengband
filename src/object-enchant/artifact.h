#pragma once

#include "system/angband.h"
#include "cmd-item/cmd-activate.h"

/*!
 * @struct artifact_type
 * @brief 固定アーティファクト情報の構造体 / Artifact structure.
 * @details
 * @note
 * the save-file only writes "cur_num" to the savefile.
 * "max_num" is always "1" (if that artifact "exists")
 */
typedef struct artifact_type {
	STR_OFFSET name;			/*!< アーティファクト名(headerオフセット参照) / Name (offset) */
	STR_OFFSET text;			/*!< アーティファクト解説(headerオフセット参照) / Text (offset) */
	tval_type tval;		/*!< ベースアイテム大項目ID / Artifact type */
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
	BIT_FLAGS gen_flags;		/*! アイテム生成フラグ / flags for generate */
	DEPTH level;		/*! 基本生成階 / Artifact level */
	RARITY rarity;		/*! レアリティ / Artifact rarity */
	byte cur_num;		/*! 現在の生成数 / Number created (0 or 1) */
	byte max_num;		/*! (未使用)最大生成数 / Unused (should be "1") */
	FLOOR_IDX floor_id;      /*! アイテムを落としたフロアのID / Leaved on this location last time */
	byte act_idx;		/*! 発動能力ID / Activative ability index */
} artifact_type;

extern artifact_type *a_info;
extern char *a_name;
extern char *a_text;
extern ARTIFACT_IDX max_a_idx;

bool become_random_artifact(player_type *player_ptr, object_type *o_ptr, bool a_scroll);
int activation_index(object_type *o_ptr);
const activation_type* find_activation_info(object_type *o_ptr);
void random_artifact_resistance(player_type *player_ptr, object_type *o_ptr, artifact_type *a_ptr);
bool create_named_art(player_type *player_ptr, ARTIFACT_IDX a_idx, POSITION y, POSITION x);
bool make_artifact(player_type *player_ptr, object_type *o_ptr);
bool make_artifact_special(player_type *player_ptr, object_type *o_ptr);

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
